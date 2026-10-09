// fs6_eeprom.h : FS6 - enregistrer et consulter les mesures dans l'EEPROM
// interne de l'ATmega328P (1024 octets, conservés sans alimentation).
#ifndef FS6_EEPROM_H
#define FS6_EEPROM_H

#include <Arduino.h>
#include "fs2_rtc.h"

// Un enregistrement = 7 octets (ET6.3 : bpm + date + heure).
struct Enregistrement {
  uint8_t bpm;           // 40..200 tient sur 1 octet
  DateHeure dateHeure;   // 6 octets
};

// Vérifie la signature de l'EEPROM ; la formate si elle est vierge.
void fs6Initialiser();

// --- Stockage ---------------------------------------------------------------
uint8_t fs6Nombre();
// rang 0 = le plus récent, rang fs6Nombre()-1 = le plus ancien.
bool fs6Lire(uint8_t rang, Enregistrement &enregistrement);
// Si la mémoire est pleine, le plus ancien est écrasé (file circulaire).
void fs6Ajouter(const Enregistrement &enregistrement);
void fs6ToutEffacer();

// Langue de l'interface (ET6.5), conservée elle aussi en EEPROM.
uint8_t fs6LireLangue();
void fs6EcrireLangue(uint8_t langue);

// --- Procédure d'enregistrement ET6.2 (moyenne de 10 mesures consécutives) ---
enum EtatEnregistrement : uint8_t {
  ENREG_INACTIF,
  ENREG_EN_COURS,
  ENREG_REUSSI,   // affiché DUREE_MESSAGE_MS puis retour à INACTIF
  ENREG_ECHEC     // délai dépassé
};

void fs6DemarrerEnregistrement();
// À appeler à CHAQUE battement détecté. Une mesure invalide remet le compte à
// zéro : les 10 mesures doivent être consécutives.
void fs6AjouterMesure(bool valide, uint16_t bpm, const DateHeure &maintenant);
// Gère le délai maximal et la durée des messages (à appeler à chaque loop()).
void fs6MettreAJour();
EtatEnregistrement fs6EtatEnregistrement();
uint8_t fs6NbMesuresAcquises();
uint8_t fs6DernierBpmEnregistre();

#endif
