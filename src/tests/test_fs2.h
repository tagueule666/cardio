// test_fs2.h : FS2 seul.
//  - Affiche l'heure de la RTC sur le moniteur série (115200 bauds).
//  - Fait clignoter la LED verte à 1 Hz, cadencée par la RTC : allumée à
//    chaque changement de seconde, éteinte 500 ms après. Chronométrer 10
//    allumages et diviser par 10 pour vérifier la période (indication du sujet).
//  - ET2.1 / ET2.2 : noter l'écart avec une horloge de référence (heure
//    officielle) au départ, puis 24 h plus tard.
//  - ET2.3 : débrancher l'Arduino, rebrancher : l'heure doit avoir continué.
#include "fs2_rtc.h"
#include "config.h"

static uint8_t secondePrecedente = 255;
static unsigned long instantAllumage;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_VERTE, OUTPUT);
  fs2Initialiser();
  Serial.println(fs2Valide() ? F("RTC OK") : F("RTC incoherente : verifier le cablage"));
}

void loop() {
  fs2MettreAJour();
  const DateHeure &d = fs2Maintenant();
  if (fs2Valide() && d.seconde != secondePrecedente) {
    secondePrecedente = d.seconde;
    digitalWrite(PIN_LED_VERTE, HIGH);
    instantAllumage = millis();
    char ligne[32];
    snprintf(ligne, sizeof(ligne), "%02u/%02u/20%02u %02u:%02u:%02u", d.jour, d.mois, d.annee,
             d.heure, d.minute, d.seconde);
    Serial.println(ligne);
  }
  if (millis() - instantAllumage >= 500) digitalWrite(PIN_LED_VERTE, LOW);
}
