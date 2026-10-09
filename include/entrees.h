// entrees.h : encodeur rotatif et boutons poussoirs (anti-rebond non bloquant).
#ifndef ENTREES_H
#define ENTREES_H

#include <Arduino.h>

void entreesInitialiser();

// À appeler à chaque loop() : lit et filtre les boutons.
void entreesMettreAJour();

// Nombre de crans tournés depuis le dernier appel (+ horaire, - anti-horaire).
int8_t entreesLireCrans();

// Chaque fonction renvoie true UNE fois par appui (front d'appui filtré).
bool entreesAppuiEncodeur();
bool entreesAppuiBip();
bool entreesAppuiEnregistrer();
bool entreesAppuiRetour();

#endif
