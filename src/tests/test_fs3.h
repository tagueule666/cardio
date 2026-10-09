// test_fs3.h : FS3 seul. Fait défiler des valeurs de bpm toutes les 2 s.
// Attendu : aucune LED, jaune, vert, vert, rouge, aucune LED ;
// jamais deux LEDs allumées en même temps (ET3.1).
#include "fs3_led.h"
#include "zone_cardiaque.h"

// 0 = mesure invalide ; 250 = valeur aberrante (aussi invalide).
static const uint16_t BPM_TEST[] = {0, 50, 60, 85, 130, 250};
static const bool VALIDE_TEST[] = {false, true, true, true, true, false};
#define NB_TESTS (sizeof(BPM_TEST) / sizeof(BPM_TEST[0]))
static uint8_t indexTest;
static unsigned long instantChangement;

void setup() {
  Serial.begin(115200);
  fs3Initialiser();
  instantChangement = millis() - 2000;  // premier affichage immédiat
}

void loop() {
  if (millis() - instantChangement < 2000) return;
  instantChangement = millis();
  ZoneCardiaque zone = classerBpm(VALIDE_TEST[indexTest], BPM_TEST[indexTest]);
  fs3Afficher(zone);
  Serial.print(F("bpm = "));
  Serial.print(BPM_TEST[indexTest]);
  Serial.print(F("  zone = "));
  Serial.println(zone);  // 0 invalide, 1 basse, 2 normale, 3 elevee
  indexTest = (indexTest + 1) % NB_TESTS;
}
