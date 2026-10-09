// langue.cpp : table de textes en mémoire FLASH (PROGMEM).
// Les chaînes constantes iraient sinon en RAM (2 Ko seulement) : on les laisse
// en Flash (32 Ko) et on copie celle dont on a besoin dans un petit tampon.
// Pas d'accents à l'écran : la police compacte utilisée ne contient que l'ASCII.

#include "langue.h"

static Langue langueCourante = LANGUE_FR;
static char tampon[22];

// Chaque texte : version FR puis EN.
static const char fr0[] PROGMEM = "MENU";              static const char en0[] PROGMEM = "MENU";
static const char fr1[] PROGMEM = "Consulter mesures"; static const char en1[] PROGMEM = "View records";
static const char fr2[] PROGMEM = "Effacer mesures";   static const char en2[] PROGMEM = "Delete records";
static const char fr3[] PROGMEM = "Langue : Francais"; static const char en3[] PROGMEM = "Language: English";
static const char fr4[] PROGMEM = "Quitter";           static const char en4[] PROGMEM = "Exit";
static const char fr5[] PROGMEM = "Mesure";            static const char en5[] PROGMEM = "Record";
static const char fr6[] PROGMEM = "Aucune mesure";     static const char en6[] PROGMEM = "No record";
static const char fr7[] PROGMEM = "Tout effacer ?";    static const char en7[] PROGMEM = "Delete all?";
static const char fr8[] PROGMEM = "Non";               static const char en8[] PROGMEM = "No";
static const char fr9[] PROGMEM = "Oui";               static const char en9[] PROGMEM = "Yes";
static const char fr10[] PROGMEM = "Mesures effacees"; static const char en10[] PROGMEM = "Records deleted";
static const char fr11[] PROGMEM = "Bip ON";           static const char en11[] PROGMEM = "Beep ON";
static const char fr12[] PROGMEM = "Bip OFF";          static const char en12[] PROGMEM = "Beep OFF";
static const char fr13[] PROGMEM = "Rythme bas";       static const char en13[] PROGMEM = "Low rate";
static const char fr14[] PROGMEM = "Rythme normal";    static const char en14[] PROGMEM = "Normal rate";
static const char fr15[] PROGMEM = "Rythme eleve";     static const char en15[] PROGMEM = "High rate";
static const char fr16[] PROGMEM = "Placez le doigt";  static const char en16[] PROGMEM = "Place finger";
static const char fr17[] PROGMEM = "Enreg.";           static const char en17[] PROGMEM = "Rec.";
static const char fr18[] PROGMEM = "Enregistre :";     static const char en18[] PROGMEM = "Saved:";
static const char fr19[] PROGMEM = "Echec enreg.";     static const char en19[] PROGMEM = "Rec. failed";

static const char *const TEXTES[NB_TEXTES][2] PROGMEM = {
    {fr0, en0}, {fr1, en1}, {fr2, en2}, {fr3, en3}, {fr4, en4},
    {fr5, en5}, {fr6, en6}, {fr7, en7}, {fr8, en8}, {fr9, en9},
    {fr10, en10}, {fr11, en11}, {fr12, en12}, {fr13, en13}, {fr14, en14},
    {fr15, en15}, {fr16, en16}, {fr17, en17}, {fr18, en18}, {fr19, en19}};

void langueDefinir(Langue langue) { langueCourante = (langue == LANGUE_EN) ? LANGUE_EN : LANGUE_FR; }
Langue langueActuelle() { return langueCourante; }

const char *texte(IdTexte id) {
  if (id >= NB_TEXTES) return "";
  // 1) lire en Flash l'adresse de la chaîne ; 2) copier la chaîne en RAM.
  const char *adresse = (const char *)pgm_read_ptr(&TEXTES[id][langueCourante]);
  strncpy_P(tampon, adresse, sizeof(tampon) - 1);
  tampon[sizeof(tampon) - 1] = '\0';
  return tampon;
}
