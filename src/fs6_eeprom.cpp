// fs6_eeprom.cpp : stockage des mesures en EEPROM interne.
//
// Plan mémoire (adresses en octets) :
//   0..1  signature 'C','E' : prouve que l'EEPROM a déjà été formatée par nous
//   2     version du format
//   3     nombre d'enregistrements (0..EEPROM_NB_MAX_ENREGISTREMENTS)
//   4     index de la prochaine case à écrire (file circulaire)
//   5     langue (0 = français, 1 = anglais)
//   8..   enregistrements de 7 octets
//
// L'EEPROM supporte ~100 000 écritures par case. EEPROM.put() / update()
// n'écrivent un octet QUE s'il a changé, et "tout effacer" ne remet à zéro que
// le compteur (1 octet) au lieu de réécrire les 700 octets : usure minimale.

#include "fs6_eeprom.h"
#include "config.h"
#include <EEPROM.h>

#define ADR_SIGNATURE 0
#define ADR_VERSION 2
#define ADR_NOMBRE 3
#define ADR_INDEX_ECRITURE 4
#define ADR_LANGUE 5
#define ADR_DONNEES 8
#define SIGNATURE_EEPROM_0 'C'
#define SIGNATURE_EEPROM_1 'E'
#define VERSION_FORMAT 1

// Vérifié à la compilation : les données tiennent dans l'EEPROM (E2END = 1023).
static_assert(ADR_DONNEES + EEPROM_NB_MAX_ENREGISTREMENTS * sizeof(Enregistrement) <= E2END + 1,
              "Trop d'enregistrements pour l'EEPROM");
static_assert(sizeof(Enregistrement) == 7, "Enregistrement doit faire 7 octets");

// Copies en RAM des compteurs (évite de relire l'EEPROM à chaque fois).
static uint8_t nombre;
static uint8_t indexEcriture;

// Procédure d'enregistrement
static EtatEnregistrement etatEnreg;
static uint8_t nbMesures;
static uint16_t sommeBpm;
static unsigned long instantDebut;
static unsigned long instantFin;
static uint8_t dernierBpmEnregistre;

static void formater() {
  EEPROM.update(ADR_SIGNATURE, SIGNATURE_EEPROM_0);
  EEPROM.update(ADR_SIGNATURE + 1, SIGNATURE_EEPROM_1);
  EEPROM.update(ADR_VERSION, VERSION_FORMAT);
  EEPROM.update(ADR_NOMBRE, 0);
  EEPROM.update(ADR_INDEX_ECRITURE, 0);
  EEPROM.update(ADR_LANGUE, 0);
}

void fs6Initialiser() {
  if (EEPROM.read(ADR_SIGNATURE) != SIGNATURE_EEPROM_0 || EEPROM.read(ADR_SIGNATURE + 1) != SIGNATURE_EEPROM_1 ||
      EEPROM.read(ADR_VERSION) != VERSION_FORMAT) {
    formater();  // EEPROM neuve (remplie de 0xFF) ou ancien format
  }
  nombre = EEPROM.read(ADR_NOMBRE);
  indexEcriture = EEPROM.read(ADR_INDEX_ECRITURE);
  // Sécurité si une valeur est corrompue (coupure pendant une écriture).
  if (nombre > EEPROM_NB_MAX_ENREGISTREMENTS || indexEcriture >= EEPROM_NB_MAX_ENREGISTREMENTS) {
    nombre = 0;
    indexEcriture = 0;
    EEPROM.update(ADR_NOMBRE, 0);
    EEPROM.update(ADR_INDEX_ECRITURE, 0);
  }
  etatEnreg = ENREG_INACTIF;
}

uint8_t fs6Nombre() { return nombre; }

static int adresseCase(uint8_t indexCase) {
  return ADR_DONNEES + (int)indexCase * (int)sizeof(Enregistrement);
}

bool fs6Lire(uint8_t rang, Enregistrement &enregistrement) {
  if (rang >= nombre) return false;
  // La case la plus récente est juste avant indexEcriture (en tournant).
  uint8_t indexCase = (indexEcriture + EEPROM_NB_MAX_ENREGISTREMENTS - 1 - rang) % EEPROM_NB_MAX_ENREGISTREMENTS;
  EEPROM.get(adresseCase(indexCase), enregistrement);
  return true;
}

void fs6Ajouter(const Enregistrement &enregistrement) {
  // Ordre d'écriture choisi pour qu'une coupure au milieu ne corrompe rien :
  // d'abord les données, ensuite les compteurs qui les rendent "visibles".
  EEPROM.put(adresseCase(indexEcriture), enregistrement);
  indexEcriture = (indexEcriture + 1) % EEPROM_NB_MAX_ENREGISTREMENTS;
  if (nombre < EEPROM_NB_MAX_ENREGISTREMENTS) nombre++;
  EEPROM.update(ADR_INDEX_ECRITURE, indexEcriture);
  EEPROM.update(ADR_NOMBRE, nombre);
}

void fs6ToutEffacer() {
  nombre = 0;
  indexEcriture = 0;
  EEPROM.update(ADR_NOMBRE, 0);
  EEPROM.update(ADR_INDEX_ECRITURE, 0);
}

uint8_t fs6LireLangue() { return EEPROM.read(ADR_LANGUE) == 1 ? 1 : 0; }
void fs6EcrireLangue(uint8_t langue) { EEPROM.update(ADR_LANGUE, langue); }

// ---------------------------------------------------------------------------
// Procédure d'enregistrement (ET6.2)
// ---------------------------------------------------------------------------

void fs6DemarrerEnregistrement() {
  etatEnreg = ENREG_EN_COURS;
  nbMesures = 0;
  sommeBpm = 0;
  instantDebut = millis();
}

void fs6AjouterMesure(bool valide, uint16_t bpm, const DateHeure &maintenant) {
  if (etatEnreg != ENREG_EN_COURS) return;
  if (!valide) {  // série interrompue : on recommence à compter
    nbMesures = 0;
    sommeBpm = 0;
    return;
  }
  sommeBpm += bpm;
  if (++nbMesures < NB_MESURES_PAR_ENREGISTREMENT) return;

  // 10 mesures : moyenne arrondie, horodatée avec l'heure de la dernière mesure.
  Enregistrement e;
  e.bpm = (uint8_t)((sommeBpm + NB_MESURES_PAR_ENREGISTREMENT / 2) / NB_MESURES_PAR_ENREGISTREMENT);
  e.dateHeure = maintenant;
  fs6Ajouter(e);
  dernierBpmEnregistre = e.bpm;
  etatEnreg = ENREG_REUSSI;
  instantFin = millis();
}

void fs6MettreAJour() {
  unsigned long maintenant = millis();
  if (etatEnreg == ENREG_EN_COURS && maintenant - instantDebut > DELAI_MAX_ENREGISTREMENT_MS) {
    etatEnreg = ENREG_ECHEC;
    instantFin = maintenant;
  } else if ((etatEnreg == ENREG_REUSSI || etatEnreg == ENREG_ECHEC) &&
             maintenant - instantFin > DUREE_MESSAGE_MS) {
    etatEnreg = ENREG_INACTIF;
  }
}

EtatEnregistrement fs6EtatEnregistrement() { return etatEnreg; }
uint8_t fs6NbMesuresAcquises() { return nbMesures; }
uint8_t fs6DernierBpmEnregistre() { return dernierBpmEnregistre; }
