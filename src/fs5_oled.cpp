// fs5_oled.cpp : deux écrans OLED 128x64 sur le même bus I2C.
//
// I2C : bus série à 2 fils (SDA = données, SCL = horloge) partagé par
// plusieurs esclaves. Chaque esclave a une ADRESSE de 7 bits ; le maître
// (l'Arduino) commence chaque échange par l'adresse visée. Les deux écrans
// sont donc branchés en parallèle sur A4/A5 et distingués par leur adresse
// (0x3C et 0x3D).
//
// Bibliothèque U8g2 en mode "page" (constructeurs _1_) : au lieu d'un tampon
// image de 1024 octets par écran (128 x 64 / 8), soit 2048 octets pour deux
// écrans, ce qui représente TOUTE la RAM de l'ATmega328P, on dessine l'image
// par bandes de 8 lignes (128 octets). On appelle le dessin une fois par
// bande et U8g2 ne garde que ce qui tombe dans la bande courante.
// Ici, on n'envoie qu'UNE bande par passage dans loop() : l'affichage ne
// bloque jamais plus de ~3 ms, ce qui garde les bips réactifs.
// Les deux écrans partagent le MÊME tampon de bande (U8g2 le déclare une seule
// fois) : c'est sans risque car une image est toujours finie avant de
// commencer celle de l'autre écran.

#include "fs5_oled.h"
#include "config.h"

#if OLED_CONTROLEUR_SH1106
typedef U8G2_SH1106_128X64_NONAME_1_HW_I2C TypeEcranOled;
#else
typedef U8G2_SSD1306_128X64_NONAME_1_HW_I2C TypeEcranOled;
#endif

static TypeEcranOled ecranInfo(U8G2_R0, U8X8_PIN_NONE);
static TypeEcranOled ecranPpg(U8G2_R0, U8X8_PIN_NONE);

static FonctionPreparation preparerInfo;
static FonctionDessin dessinerInfo;

// --- Géométrie du graphique ------------------------------------------------
#define LARGEUR_GRAPHIQUE 128
#define Y_HAUT_GRAPHIQUE 11       // lignes 0..10 : texte d'échelle
#define Y_CENTRE_GRAPHIQUE 37     // valeur normalisée 0
#define DEMI_HAUTEUR_GRAPHIQUE 25 // valeur 1 => y = 12 ; valeur -1 => y = 62
#define PIXELS_PAR_DIVISION 25    // graduations verticales

// Échelles de temps disponibles (échantillons de 10 ms par pixel) :
// 10, 20, 50, 100 ms/pixel => l'écran couvre 1,28 s, 2,56 s, 6,4 s, 12,8 s.
static const uint8_t ECHELLES[] = {1, 2, 5, 10};
#define NB_ECHELLES (sizeof(ECHELLES) / sizeof(ECHELLES[0]))
static uint8_t indexEchelle = 1;

// Historique circulaire : une ordonnée (en pixels) par colonne.
static uint8_t historique[LARGEUR_GRAPHIQUE];
static uint8_t indexEcriture;
static uint8_t nbPoints;
// Pendant qu'une image de la courbe est envoyée (8 bandes), l'historique ne
// doit pas bouger, sinon les bandes ne montreraient pas la même courbe.
// Les nouveaux points attendent ici et sont ajoutés à la fin de l'image.
// (8 cases suffisent : une image dure ~40 ms, soit 4 points à 10 ms/pixel.
// Une copie complète de l'historique coûterait 128 octets de RAM.)
#define TAILLE_ATTENTE 8
static uint8_t pointsEnAttente[TAILLE_ATTENTE];
static uint8_t nbEnAttente;

// Décimation : moyenne de N échantillons pour 1 pixel.
static float sommeDecimation;
static uint8_t compteurDecimation;

// --- Ordonnancement des deux écrans ----------------------------------------
enum EcranEnCours : uint8_t { AUCUN, ECRAN_INFO, ECRAN_PPG };
static EcranEnCours ecranEnCours = AUCUN;
static unsigned long instantDerniereImageInfo;
static bool rafraichirInfoDemande;

static uint8_t valeurVersY(float v) {
  if (v > 1.0f) v = 1.0f;
  if (v < -1.0f) v = -1.0f;
  // L'axe y de l'écran est orienté vers le BAS, d'où le signe moins.
  return (uint8_t)(Y_CENTRE_GRAPHIQUE - (int8_t)(v * DEMI_HAUTEUR_GRAPHIQUE));
}

static void viderHistorique() {
  indexEcriture = 0;
  nbPoints = 0;
  nbEnAttente = 0;
  sommeDecimation = 0.0f;
  compteurDecimation = 0;
}

void fs5Initialiser(FonctionPreparation preparer, FonctionDessin dessiner) {
  preparerInfo = preparer;
  dessinerInfo = dessiner;

  // U8g2 attend l'adresse décalée d'un bit (format 8 bits : adresse << 1).
  ecranInfo.setI2CAddress(OLED_ADRESSE_ECRAN_INFO * 2);
  ecranPpg.setI2CAddress(OLED_ADRESSE_ECRAN_PPG * 2);
  ecranInfo.setBusClock(400000);  // I2C "fast mode" 400 kHz
  ecranPpg.setBusClock(400000);
  ecranInfo.begin();
  ecranPpg.begin();
  ecranInfo.setFontMode(1);  // texte transparent (permet le texte inversé)
  ecranPpg.setFontMode(1);
  viderHistorique();
  instantDerniereImageInfo = millis();
  rafraichirInfoDemande = true;
}

static void ajouterPointHistorique(uint8_t y) {
  historique[indexEcriture] = y;
  indexEcriture = (indexEcriture + 1) % LARGEUR_GRAPHIQUE;
  if (nbPoints < LARGEUR_GRAPHIQUE) nbPoints++;
}

void fs5AjouterEchantillon(float valeurNormalisee) {
  sommeDecimation += valeurNormalisee;
  if (++compteurDecimation < ECHELLES[indexEchelle]) return;
  uint8_t y = valeurVersY(sommeDecimation / compteurDecimation);
  sommeDecimation = 0.0f;
  compteurDecimation = 0;
  if (ecranEnCours == ECRAN_PPG && nbEnAttente < TAILLE_ATTENTE) {
    pointsEnAttente[nbEnAttente++] = y;  // image en cours : on met de côté
  } else {
    ajouterPointHistorique(y);
  }
}

void fs5ChangerEchelle(int8_t crans) {
  int8_t nouvelIndex = (int8_t)indexEchelle + crans;
  if (nouvelIndex < 0) nouvelIndex = 0;
  if (nouvelIndex >= (int8_t)NB_ECHELLES) nouvelIndex = NB_ECHELLES - 1;
  if (nouvelIndex == indexEchelle) return;
  indexEchelle = nouvelIndex;
  viderHistorique();  // les anciens points n'ont plus la même base de temps
}

uint16_t fs5MillisecondesParPixel() {
  return (uint16_t)ECHELLES[indexEchelle] * PERIODE_ECHANTILLONNAGE_MS;
}

void fs5RafraichirInfo() { rafraichirInfoDemande = true; }

static void viderAttente() {
  for (uint8_t i = 0; i < nbEnAttente; i++) ajouterPointHistorique(pointsEnAttente[i]);
  nbEnAttente = 0;
}

static void dessinerEcranPpg() {
  U8G2 &e = ecranPpg;
  char texte[24];
  uint16_t msParPixel = fs5MillisecondesParPixel();

  // Graduation (ET5.3) : base de temps par pixel et par division.
  e.setFont(u8g2_font_5x7_tr);
  snprintf(texte, sizeof(texte), "%ums/px  div=%ums", msParPixel, msParPixel * PIXELS_PAR_DIVISION);
  e.drawStr(0, 7, texte);

  // Grille : traits verticaux pointillés toutes les PIXELS_PAR_DIVISION colonnes.
  for (uint8_t x = 0; x < LARGEUR_GRAPHIQUE; x += PIXELS_PAR_DIVISION) {
    for (uint8_t y = Y_HAUT_GRAPHIQUE; y < 64; y += 3) e.drawPixel(x, y);
  }
  // Ligne pointillée du seuil de détection (0,5) : on VOIT où les fronts sont détectés.
  uint8_t ySeuil = valeurVersY(SEUIL_NORMALISE);
  for (uint8_t x = 0; x < LARGEUR_GRAPHIQUE; x += 4) e.drawPixel(x, ySeuil);

  // Courbe : le point le plus récent est à droite (défilement type oscilloscope).
  // Tant que l'historique n'est pas plein, le plus ancien est en case 0 ;
  // ensuite, c'est la case qui va être écrasée (indexEcriture).
  uint8_t debut = (nbPoints < LARGEUR_GRAPHIQUE) ? 0 : indexEcriture;
  uint8_t xDepart = LARGEUR_GRAPHIQUE - nbPoints;
  uint8_t yPrecedent = historique[debut];
  for (uint8_t i = 1; i < nbPoints; i++) {
    uint8_t y = historique[(debut + i) % LARGEUR_GRAPHIQUE];
    e.drawLine(xDepart + i - 1, yPrecedent, xDepart + i, y);
    yPrecedent = y;
  }
}

void fs5MettreAJour() {
  if (ecranEnCours == AUCUN) {
    // Début d'une nouvelle image : l'écran INFO si son heure est venue,
    // sinon l'écran PPG (redessiné en continu, il doit être fluide).
    unsigned long maintenant = millis();
    if (rafraichirInfoDemande || maintenant - instantDerniereImageInfo >= PERIODE_RAFRAICHISSEMENT_INFO_MS) {
      rafraichirInfoDemande = false;
      instantDerniereImageInfo = maintenant;
      if (preparerInfo) preparerInfo();
      ecranInfo.firstPage();
      ecranEnCours = ECRAN_INFO;
    } else {
      ecranPpg.firstPage();
      ecranEnCours = ECRAN_PPG;
    }
  }

  // Une seule bande par appel. nextPage() envoie la bande à l'écran et
  // renvoie false quand l'image est complète.
  if (ecranEnCours == ECRAN_INFO) {
    if (dessinerInfo) dessinerInfo(ecranInfo);
    if (!ecranInfo.nextPage()) ecranEnCours = AUCUN;
  } else {
    dessinerEcranPpg();
    if (!ecranPpg.nextPage()) {
      ecranEnCours = AUCUN;
      viderAttente();  // image terminée : les points mis de côté entrent dans la courbe
    }
  }
}
