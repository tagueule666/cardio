// fs3_led.cpp : voyants rouge / vert / jaune.
// Une broche à HIGH fournit 5 V ; avec 220 ohms et ~2 V de tension de seuil
// de LED, le courant vaut (5 - 2) / 220 ≈ 14 mA, sous la limite de 20 mA par broche.

#include "fs3_led.h"
#include "config.h"

static ZoneCardiaque zoneAffichee;

void fs3Initialiser() {
  pinMode(PIN_LED_ROUGE, OUTPUT);
  pinMode(PIN_LED_VERTE, OUTPUT);
  pinMode(PIN_LED_JAUNE, OUTPUT);
  digitalWrite(PIN_LED_ROUGE, LOW);
  digitalWrite(PIN_LED_VERTE, LOW);
  digitalWrite(PIN_LED_JAUNE, LOW);
  zoneAffichee = ZONE_INVALIDE;
}

void fs3Afficher(ZoneCardiaque zone) {
  if (zone == zoneAffichee) return;  // rien à changer
  zoneAffichee = zone;
  // Chaque LED est calculée à partir de la MÊME variable zone : il est
  // impossible d'en allumer deux à la fois (ET3.1).
  digitalWrite(PIN_LED_JAUNE, zone == ZONE_BASSE ? HIGH : LOW);
  digitalWrite(PIN_LED_VERTE, zone == ZONE_NORMALE ? HIGH : LOW);
  digitalWrite(PIN_LED_ROUGE, zone == ZONE_ELEVEE ? HIGH : LOW);
}
