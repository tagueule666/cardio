// entrees.cpp : encodeur (par interruptions) et boutons (par scrutation).
//
// Encodeur incrémental : deux contacts A et B décalés d'un quart de période
// (signaux "en quadrature"). L'ordre dans lequel ils changent donne le sens :
//   horaire       : 00 -> 01 -> 11 -> 10 -> 00
//   anti-horaire  : 00 -> 10 -> 11 -> 01 -> 00
// Les deux broches déclenchent une interruption à chaque changement : aucune
// transition n'est perdue, même pendant l'affichage.
//
// Interruptions PCINT (Pin Change INTerrupt) : contrairement à INT0/INT1
// (D2/D3 seulement), elles existent sur TOUTES les broches, regroupées par port :
//   port B (D8..D13)  -> vecteur PCINT0_vect, masque PCMSK0
//   port C (A0..A5)   -> vecteur PCINT1_vect, masque PCMSK1
//   port D (D0..D7)   -> vecteur PCINT2_vect, masque PCMSK2
// Une interruption du groupe se déclenche quand une broche AUTORISÉE dans le
// masque change d'état. Seules les 2 broches de l'encodeur sont autorisées :
// les signaux de la RTC (D11..D13, même port B) ne déclenchent rien.

#include "entrees.h"
#include "config.h"

// Table de décodage : indice = (ancien état AB << 2) | nouvel état AB.
// +1 / -1 = un pas dans un sens ou l'autre, 0 = pas de mouvement ou
// transition impossible (rebond : deux bits changés à la fois).
static const int8_t TABLE_QUADRATURE[16] = {
    0, -1, +1, 0,
    +1, 0, 0, -1,
    -1, 0, 0, +1,
    0, +1, -1, 0};

static volatile int16_t transitionsEncodeur;
static volatile uint8_t etatPrecedentEncodeur;

static void isrEncodeur() {
  uint8_t etat = (digitalRead(PIN_ENCODEUR_A) << 1) | digitalRead(PIN_ENCODEUR_B);
  transitionsEncodeur += TABLE_QUADRATURE[(etatPrecedentEncodeur << 2) | etat];
  etatPrecedentEncodeur = etat;
}

// Les 3 vecteurs pointent vers le même traitement : seul le groupe contenant
// les broches de l'encodeur est activé, donc seul celui-ci sera appelé. Cela
// permet de déplacer l'encodeur sur n'importe quelles broches via config.h.
ISR(PCINT0_vect) { isrEncodeur(); }
ISR(PCINT1_vect) { isrEncodeur(); }
ISR(PCINT2_vect) { isrEncodeur(); }

// Autorise l'interruption de changement d'état sur une broche.
static void activerPcint(uint8_t broche) {
  *digitalPinToPCMSK(broche) |= bit(digitalPinToPCMSKbit(broche));  // broche dans le masque
  PCIFR |= bit(digitalPinToPCICRbit(broche));   // efface un éventuel drapeau en attente
  PCICR |= bit(digitalPinToPCICRbit(broche));   // active le groupe (port)
}

// --- Boutons -----------------------------------------------------------------
// Un contact mécanique "rebondit" pendant quelques ms. On n'accepte un
// changement que s'il reste stable ANTI_REBOND_MS, mesurés avec millis().
struct Bouton {
  uint8_t broche;
  bool resistanceExterne;  // true = résistance 10k externe (voir config.h)
  bool etatStable;         // true = appuyé
  bool derniereLecture;
  unsigned long instantChangement;
  bool appuiEnAttente;
};

enum { B_ENCODEUR, B_BIP, B_ENREGISTRER, B_RETOUR, NB_BOUTONS };
static Bouton boutons[NB_BOUTONS] = {
    // Le bouton SW de l'encodeur (KY-040) n'a pas de résistance : pull-up interne.
    {PIN_ENCODEUR_BOUTON, false, false, false, 0, false},
    {PIN_BOUTON_BIP, true, false, false, 0, false},
    {PIN_BOUTON_ENREGISTRER, true, false, false, 0, false},
    {PIN_BOUTON_RETOUR, true, false, false, 0, false}};

// Niveau logique lu quand le bouton est enfoncé.
static uint8_t niveauAppuye(const Bouton &b) {
  if (!b.resistanceExterne) return LOW;  // pull-up interne : appui = mise à la masse
  return BOUTONS_PULL_DOWN_EXTERNE ? HIGH : LOW;
}

void entreesInitialiser() {
  pinMode(PIN_ENCODEUR_A, INPUT_PULLUP);
  pinMode(PIN_ENCODEUR_B, INPUT_PULLUP);
  etatPrecedentEncodeur = (digitalRead(PIN_ENCODEUR_A) << 1) | digitalRead(PIN_ENCODEUR_B);
  transitionsEncodeur = 0;
  activerPcint(PIN_ENCODEUR_A);
  activerPcint(PIN_ENCODEUR_B);

  for (uint8_t i = 0; i < NB_BOUTONS; i++) {
    // Avec une résistance externe, on n'active PAS le pull-up interne (~35 k) :
    // en parallèle d'un pull-down de 10 k, il formerait un pont diviseur et la
    // broche flotterait vers ~1 V au repos, à la limite entre LOW et HIGH.
    pinMode(boutons[i].broche, boutons[i].resistanceExterne ? INPUT : INPUT_PULLUP);
  }
}

void entreesMettreAJour() {
  unsigned long maintenant = millis();
  for (uint8_t i = 0; i < NB_BOUTONS; i++) {
    Bouton &b = boutons[i];
    bool lecture = (digitalRead(b.broche) == niveauAppuye(b));  // true = appuyé
    if (lecture != b.derniereLecture) {
      b.derniereLecture = lecture;      // ça bouge encore : on relance le chrono
      b.instantChangement = maintenant;
    } else if (lecture != b.etatStable && maintenant - b.instantChangement >= ANTI_REBOND_MS) {
      b.etatStable = lecture;           // stable assez longtemps : accepté
      if (b.etatStable) b.appuiEnAttente = true;
    }
  }
}

int8_t entreesLireCrans() {
  // Section critique : l'ISR ne doit pas modifier la variable pendant la lecture
  // (un int16 se lit en 2 instructions sur un microcontrôleur 8 bits).
  noInterrupts();
  int16_t transitions = transitionsEncodeur;
  int16_t crans = transitions / ENCODEUR_TRANSITIONS_PAR_CRAN;
  transitionsEncodeur = transitions - crans * ENCODEUR_TRANSITIONS_PAR_CRAN;  // garde le reste
  interrupts();
#if ENCODEUR_SENS_INVERSE
  crans = -crans;
#endif
  return (int8_t)constrain(crans, -100, 100);
}

static bool consommerAppui(uint8_t i) {
  if (!boutons[i].appuiEnAttente) return false;
  boutons[i].appuiEnAttente = false;
  return true;
}

bool entreesAppuiEncodeur() { return consommerAppui(B_ENCODEUR); }
bool entreesAppuiBip() { return consommerAppui(B_BIP); }
bool entreesAppuiEnregistrer() { return consommerAppui(B_ENREGISTRER); }
bool entreesAppuiRetour() { return consommerAppui(B_RETOUR); }
