// test_entrees.h : encodeur et boutons seuls (moniteur série, 115200 bauds).
// Attendu : +1 par cran dans le sens horaire, -1 dans l'autre ; un seul
// message par appui (pas de rebond). Si 1 cran donne 2 ou 0,5 : ajuster
// ENCODEUR_TRANSITIONS_PAR_CRAN dans config.h.
#include "entrees.h"

static long position;

void setup() {
  Serial.begin(115200);
  entreesInitialiser();
  Serial.println(F("Tourner l'encodeur / appuyer sur les boutons"));
}

void loop() {
  entreesMettreAJour();
  int8_t crans = entreesLireCrans();
  if (crans != 0) {
    position += crans;
    Serial.print(F("Encodeur : "));
    Serial.println(position);
  }
  if (entreesAppuiEncodeur()) Serial.println(F("Appui encodeur (SW)"));
  if (entreesAppuiBip()) Serial.println(F("Appui bouton BIP"));
  if (entreesAppuiEnregistrer()) Serial.println(F("Appui bouton ENREGISTRER"));
  if (entreesAppuiRetour()) Serial.println(F("Appui bouton RETOUR"));
}
