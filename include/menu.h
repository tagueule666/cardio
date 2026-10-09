// menu.h : interface utilisateur de l'écran INFO (écran 1).
//  - menu fermé : heure, date, bpm (ou "--"), zone, état des bips, enregistrement ;
//  - menu ouvert (ET6.4) : consulter / effacer les mesures, changer de langue.
#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <U8g2lib.h>

void menuInitialiser();
bool menuEstOuvert();
void menuOuvrir();

// Entrées du menu quand il est ouvert : crans de l'encodeur, appui sur
// l'encodeur (valider), appui sur le bouton retour.
void menuGererEntrees(int8_t crans, bool appuiValider, bool appuiRetour);

// Fonctions de rappel passées à fs5Initialiser().
void menuPreparerEcran();
void menuDessinerEcran(U8G2 &ecran);

#endif
