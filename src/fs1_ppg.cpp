// fs1_ppg.cpp : chaîne de traitement du signal PPG.
//
//   Timer1 (toutes les 10 ms) -> analogRead -> file d'attente
//   loop() -> fs1TraiterEchantillon() :
//     1. moyenne glissante, puis soustraction     (supprime dérive + lumière ambiante)
//     2. max du bloc, division par le max, seuil  (normalisation 0..1)
//     3. front montant, période, conversion bpm
//
// Le TEMPS est compté en nombre d'échantillons (1 échantillon = 10 ms exactes,
// cadencées par le quartz via Timer1) : aucune dérive, même si loop() est
// ralentie par l'affichage (ET1.4).

#include "fs1_ppg.h"
#include "config.h"

// Conversions ms -> nombre d'échantillons
#define REFRACTAIRE_ECH (PERIODE_REFRACTAIRE_MS / PERIODE_ECHANTILLONNAGE_MS)
#define SANS_BATTEMENT_ECH (DELAI_SANS_BATTEMENT_MS / PERIODE_ECHANTILLONNAGE_MS)
#define SATURATION_ECH (DUREE_INVALIDATION_SATURATION_MS / PERIODE_ECHANTILLONNAGE_MS)
#define PERIODE_MIN_ECH (ECHANTILLONS_PAR_MINUTE / BPM_MAX)  // 30 ech = 300 ms
#define PERIODE_MAX_ECH (ECHANTILLONS_PAR_MINUTE / BPM_MIN)  // 150 ech = 1,5 s

// ---------------------------------------------------------------------------
// File d'attente entre l'interruption (producteur) et loop() (consommateur)
// ---------------------------------------------------------------------------
// 32 cases = 320 ms de marge si loop() est occupée. Taille puissance de 2 :
// "& MASQUE_FILE" remplace un modulo, plus rapide.
#define TAILLE_FILE 32
#define MASQUE_FILE (TAILLE_FILE - 1)
static volatile int16_t fileBrute[TAILLE_FILE];
static volatile uint8_t teteFile;   // écrite par l'ISR
static volatile uint8_t queueFile;  // écrite par loop()
static volatile uint8_t echantillonsPerdus;

// ---------------------------------------------------------------------------
// État du traitement (static = privé à ce fichier)
// ---------------------------------------------------------------------------
static uint32_t compteurEchantillons;  // horloge du module, en échantillons

// Étape 1 : moyenne glissante (tampon circulaire + somme courante)
static int16_t tamponMoyenne[TAILLE_FENETRE_MOYENNE];
static uint8_t indexMoyenne;
static uint8_t nbValeursMoyenne;
static int32_t sommeFenetre;

// Étape 2 : maximum par bloc
static int16_t maxBlocCourant;
static int16_t minBlocCourant;
static uint16_t compteurBloc;
static int16_t maxReference;        // max du bloc PRÉCÉDENT (0 = inconnu)
static int16_t amplitudeReference;  // max - min du bloc précédent

// Étape 3 : détection et calcul
static bool detecteurArme;
static bool premierBattementVu;
static uint32_t instantDernierBattement;  // en échantillons
static uint16_t periodesEch[NB_PERIODES_MOYENNE];
static uint8_t nbPeriodes;
static uint8_t indexPeriode;
static uint16_t bpmMoyen;
static uint16_t bpmInstantane;
static bool bpmCalcule;
static bool drapeauBattement;

// Validité
static bool saturationVue;
static uint32_t instantDerniereSaturation;

// Dernières valeurs (débogage / courbe)
static int16_t valeurBrute;
static int16_t valeurSansDerive;
static float valeurNormalisee;

// ---------------------------------------------------------------------------
// Interruption Timer1 : acquisition à période fixe
// ---------------------------------------------------------------------------
// Appelée par le matériel toutes les 10 ms, quoi que fasse loop(). Elle ne
// fait que le strict nécessaire (lire le CAN, ranger la valeur) : le calcul
// se fait hors interruption.
ISR(TIMER1_COMPA_vect) {
  // analogRead : le CAN 10 bits convertit la tension 0..5 V en 0..1023
  // (approximations successives, ~110 µs).
  int16_t valeur = (int16_t)analogRead(PIN_PPG);
  uint8_t suivant = (teteFile + 1) & MASQUE_FILE;
  if (suivant == queueFile) {  // file pleine : loop() a pris trop de retard
    echantillonsPerdus++;
    return;
  }
  fileBrute[teteFile] = valeur;
  teteFile = suivant;
}

static void demarrerTimer1() {
  // Timer1 en mode CTC (Clear Timer on Compare) :
  // le compteur TCNT1 monte de 0 à OCR1A puis revient à 0 en déclenchant
  // l'interruption TIMER1_COMPA.
  // Horloge 16 MHz / prédiviseur 64 = 250 000 incréments/s, soit 4 µs chacun.
  // 10 ms / 4 µs = 2500 incréments => OCR1A = 2500 - 1.
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = (uint16_t)((F_CPU / 64UL / 1000UL) * PERIODE_ECHANTILLONNAGE_MS - 1);
  TCCR1B |= (1 << WGM12);               // mode CTC
  TCCR1B |= (1 << CS11) | (1 << CS10);  // prédiviseur 64
  TIMSK1 |= (1 << OCIE1A);              // autorise l'interruption de comparaison A
  interrupts();
}

// ---------------------------------------------------------------------------
// Fonctions internes
// ---------------------------------------------------------------------------

static void oublierMesures() {
  nbPeriodes = 0;
  indexPeriode = 0;
  bpmCalcule = false;
}

static void enregistrerBattement(uint32_t instant) {
  drapeauBattement = true;

  if (premierBattementVu) {
    uint32_t periode = instant - instantDernierBattement;  // en échantillons
    if (periode >= PERIODE_MIN_ECH && periode <= PERIODE_MAX_ECH) {
      // bpm = (échantillons par minute) / (échantillons par battement)
      // ex : période 80 ech = 800 ms => 6000 / 80 = 75 bpm.
      // "+ periode/2" = arrondi à l'entier le plus proche.
      bpmInstantane = (uint16_t)((ECHANTILLONS_PAR_MINUTE + periode / 2) / periode);

      periodesEch[indexPeriode] = (uint16_t)periode;
      indexPeriode = (indexPeriode + 1) % NB_PERIODES_MOYENNE;
      if (nbPeriodes < NB_PERIODES_MOYENNE) nbPeriodes++;

      // Moyenne sur plusieurs périodes : lisse les variations et améliore la
      // résolution (1 échantillon d'erreur sur 4 périodes au lieu d'une).
      uint32_t somme = 0;
      for (uint8_t i = 0; i < nbPeriodes; i++) somme += periodesEch[i];
      bpmMoyen = (uint16_t)((ECHANTILLONS_PAR_MINUTE * nbPeriodes + somme / 2) / somme);
      bpmCalcule = true;
    } else {
      // Période aberrante (battement manqué, parasite) : on repart de zéro.
      oublierMesures();
    }
  }
  premierBattementVu = true;
  instantDernierBattement = instant;
}

// ---------------------------------------------------------------------------
// Interface publique
// ---------------------------------------------------------------------------

void fs1Initialiser() {
  pinMode(PIN_PPG, INPUT);
  teteFile = queueFile = 0;
  echantillonsPerdus = 0;
  compteurEchantillons = 0;
  indexMoyenne = 0;
  nbValeursMoyenne = 0;
  sommeFenetre = 0;
  for (uint8_t i = 0; i < TAILLE_FENETRE_MOYENNE; i++) tamponMoyenne[i] = 0;
  maxBlocCourant = -32768;
  minBlocCourant = 32767;
  compteurBloc = 0;
  maxReference = 0;
  amplitudeReference = 0;
  detecteurArme = true;
  premierBattementVu = false;
  instantDernierBattement = 0;
  bpmMoyen = bpmInstantane = 0;
  drapeauBattement = false;
  saturationVue = false;
  instantDerniereSaturation = 0;
  valeurBrute = valeurSansDerive = 0;
  valeurNormalisee = 0.0f;
  oublierMesures();
  demarrerTimer1();
}

bool fs1TraiterEchantillon() {
  // --- Récupération dans la file ----------------------------------------
  if (queueFile == teteFile) return false;  // rien en attente
  int16_t brut = fileBrute[queueFile];
  queueFile = (queueFile + 1) & MASQUE_FILE;

  // Échantillons perdus : on les compte quand même pour ne pas fausser le temps.
  if (echantillonsPerdus) {
    noInterrupts();
    uint8_t perdus = echantillonsPerdus;
    echantillonsPerdus = 0;
    interrupts();
    compteurEchantillons += perdus;
  }
  compteurEchantillons++;

  if (brut <= ADC_SATURATION_BAS || brut >= ADC_SATURATION_HAUT) {
    saturationVue = true;
    instantDerniereSaturation = compteurEchantillons;
  }
#if PPG_SIGNAL_INVERSE
  brut = 1023 - brut;
#endif
  valeurBrute = brut;

  // --- Étape 1 : moyenne glissante puis soustraction ---------------------
  // Somme courante : on retire la plus vieille valeur, on ajoute la nouvelle
  // => 2 opérations par échantillon au lieu de re-sommer 100 valeurs.
  sommeFenetre -= tamponMoyenne[indexMoyenne];
  tamponMoyenne[indexMoyenne] = brut;
  sommeFenetre += brut;
  indexMoyenne = (indexMoyenne + 1) % TAILLE_FENETRE_MOYENNE;
  if (nbValeursMoyenne < TAILLE_FENETRE_MOYENNE) nbValeursMoyenne++;
  int16_t moyenne = (int16_t)(sommeFenetre / nbValeursMoyenne);
  // La moyenne sur 1 s contient la composante LENTE : niveau continu, dérive,
  // lumière ambiante (ET1.3). En la retranchant, il ne reste que la composante
  // rapide, c'est-à-dire les battements, centrés autour de 0.
  int16_t sansDerive = brut - moyenne;
  valeurSansDerive = sansDerive;

  // --- Étape 2 : recherche du maximum, normalisation ---------------------
  if (sansDerive > maxBlocCourant) maxBlocCourant = sansDerive;
  if (sansDerive < minBlocCourant) minBlocCourant = sansDerive;
  if (++compteurBloc >= TAILLE_BLOC_MAXIMUM) {
    // Fin de bloc (2 s) : son max devient la référence du bloc suivant.
    maxReference = maxBlocCourant;
    amplitudeReference = maxBlocCourant - minBlocCourant;
    maxBlocCourant = -32768;
    minBlocCourant = 32767;
    compteurBloc = 0;
  }

  if (maxReference > 0) {
    // Division par le max : le sommet du pic vaut ~1 quelle que soit
    // l'amplitude (doigt plus ou moins appuyé, luminosité) => un seuil fixe convient.
    valeurNormalisee = (float)sansDerive / (float)maxReference;
    if (valeurNormalisee > 1.0f) valeurNormalisee = 1.0f;
    if (valeurNormalisee < -1.0f) valeurNormalisee = -1.0f;
  } else {
    valeurNormalisee = 0.0f;  // 2 premières secondes : pas encore de référence
  }

  // --- Étape 3 : seuillage et détection du front montant -----------------
  // Front montant = passage de "sous le seuil" à "au-dessus du seuil".
  // detecteurArme mémorise qu'on était bien redescendu sous SEUIL_REARMEMENT
  // (hystérésis : le bruit autour de 0,5 ne crée pas plusieurs fronts).
  if (maxReference > 0) {
    if (detecteurArme && valeurNormalisee >= SEUIL_NORMALISE) {
      detecteurArme = false;
      bool horsRefractaire = !premierBattementVu ||
                             (compteurEchantillons - instantDernierBattement) >= REFRACTAIRE_ECH;
      if (horsRefractaire) enregistrerBattement(compteurEchantillons);
    } else if (!detecteurArme && valeurNormalisee < SEUIL_REARMEMENT) {
      detecteurArme = true;
    }
  }
  return true;
}

bool fs1BattementDetecte() {
  if (!drapeauBattement) return false;
  drapeauBattement = false;
  return true;
}

bool fs1BpmValide() {
  if (!bpmCalcule) return false;
  // Plus de battement récent : doigt retiré ou signal perdu.
  if (compteurEchantillons - instantDernierBattement > SANS_BATTEMENT_ECH) return false;
  // Signal trop plat : ce qu'on détecte n'est que du bruit.
  if (amplitudeReference < AMPLITUDE_MIN_ADC) return false;
  // Capteur saturé récemment.
  if (saturationVue && compteurEchantillons - instantDerniereSaturation < SATURATION_ECH) return false;
  if (bpmMoyen < BPM_MIN || bpmMoyen > BPM_MAX) return false;
  return true;
}

uint16_t fs1GetBpm() { return bpmMoyen; }
uint16_t fs1GetBpmInstantane() { return bpmInstantane; }
int16_t fs1GetBrut() { return valeurBrute; }
int16_t fs1GetSansDerive() { return valeurSansDerive; }
float fs1GetNormalise() { return valeurNormalisee; }
