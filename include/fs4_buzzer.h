// fs4_buzzer.h : FS4 - beeper le rythme cardiaque.
#ifndef FS4_BUZZER_H
#define FS4_BUZZER_H

#include "zone_cardiaque.h"

void fs4Initialiser();

// Émet un bip (non bloquant) dont la hauteur dépend de la zone (ET4.2).
// Aucun bip si les bips sont coupés ou si la zone est invalide.
void fs4Bip(ZoneCardiaque zone);

// Coupe / remet les bips (bouton, ET4.3). Effet immédiat.
void fs4BasculerActif();
bool fs4EstActif();

#endif
