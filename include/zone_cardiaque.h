// zone_cardiaque.h : classement d'une fréquence en zone basse / normale /
// élevée. Partagé par FS3 (LEDs), FS4 (bips) et l'affichage.
#ifndef ZONE_CARDIAQUE_H
#define ZONE_CARDIAQUE_H

#include "config.h"

enum ZoneCardiaque : uint8_t {
  ZONE_INVALIDE,  // valeur aberrante ou pas de doigt : rien n'est signalé
  ZONE_BASSE,     // < BPM_SEUIL_BAS
  ZONE_NORMALE,   // BPM_SEUIL_BAS .. BPM_SEUIL_HAUT
  ZONE_ELEVEE     // > BPM_SEUIL_HAUT
};

inline ZoneCardiaque classerBpm(bool bpmValide, uint16_t bpm) {
  if (!bpmValide) return ZONE_INVALIDE;
  if (bpm < BPM_SEUIL_BAS) return ZONE_BASSE;
  if (bpm > BPM_SEUIL_HAUT) return ZONE_ELEVEE;
  return ZONE_NORMALE;
}

#endif
