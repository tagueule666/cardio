// langue.h : textes de l'interface en français et en anglais (ET6.5).
#ifndef LANGUE_H
#define LANGUE_H

#include <Arduino.h>

enum Langue : uint8_t { LANGUE_FR = 0, LANGUE_EN = 1 };

enum IdTexte : uint8_t {
  T_MENU,
  T_CONSULTER,
  T_EFFACER,
  T_LANGUE,
  T_QUITTER,
  T_MESURE,
  T_AUCUNE_MESURE,
  T_TOUT_EFFACER,
  T_NON,
  T_OUI,
  T_MESURES_EFFACEES,
  T_BIP_ON,
  T_BIP_OFF,
  T_ZONE_BASSE,
  T_ZONE_NORMALE,
  T_ZONE_ELEVEE,
  T_ATTENTE_SIGNAL,
  T_ENREGISTREMENT,
  T_ENREGISTRE,
  T_ECHEC,
  NB_TEXTES
};

void langueDefinir(Langue langue);
Langue langueActuelle();

// Renvoie le texte dans la langue courante. ATTENTION : tampon partagé,
// utiliser le résultat tout de suite (un seul texte() par instruction).
const char *texte(IdTexte id);

#endif
