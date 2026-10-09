// fs2_rtc.h : FS2 - compter le temps avec le module RTC DS1302.
// Pilote écrit à la main (protocole série 3 fils), sans bibliothèque.
#ifndef FS2_RTC_H
#define FS2_RTC_H

#include <Arduino.h>

struct DateHeure {
  uint8_t annee;    // 0..99 (= 2000..2099)
  uint8_t mois;     // 1..12
  uint8_t jour;     // 1..31
  uint8_t heure;    // 0..23
  uint8_t minute;   // 0..59
  uint8_t seconde;  // 0..59
};

// Configure les broches ; met la RTC à l'heure de compilation seulement si
// son oscillateur était arrêté (ou si RTC_FORCER_MISE_A_L_HEURE vaut 1).
void fs2Initialiser();

// À appeler à chaque loop() : relit la RTC toutes les RTC_PERIODE_LECTURE_MS.
void fs2MettreAJour();

// Dernière heure lue (copie en cache, lecture instantanée).
const DateHeure &fs2Maintenant();

// false si la RTC renvoie des valeurs incohérentes (absente, mal câblée).
bool fs2Valide();

// Écrit une date/heure dans la RTC.
void fs2Regler(const DateHeure &dateHeure);

#endif
