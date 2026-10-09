// fs3_led.h : FS3 - afficher l'état de santé avec 3 voyants.
#ifndef FS3_LED_H
#define FS3_LED_H

#include "zone_cardiaque.h"

void fs3Initialiser();

// Allume UN SEUL voyant selon la zone (ET3.1), aucun si ZONE_INVALIDE (ET3.3).
void fs3Afficher(ZoneCardiaque zone);

#endif
