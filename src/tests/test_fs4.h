// test_fs4.h : FS4 seul. Simule un cœur à 50, 75 puis 130 bpm (5 s chacun) :
// on doit entendre des bips graves, moyens puis aigus, au bon rythme.
// Le bouton "bip" coupe / remet le son immédiatement (ET4.3).
#include "fs4_buzzer.h"
#include "entrees.h"

static const uint16_t BPM_SIMULES[] = {50, 75, 130};
static uint8_t indexBpm;
static unsigned long instantDernierBip;
static unsigned long instantChangementBpm;

void setup() {
  Serial.begin(115200);
  fs4Initialiser();
  entreesInitialiser();
  instantChangementBpm = millis();
}

void loop() {
  entreesMettreAJour();
  if (entreesAppuiBip()) {
    fs4BasculerActif();
    Serial.println(fs4EstActif() ? F("Bips ON") : F("Bips OFF"));
  }
  if (millis() - instantChangementBpm >= 5000) {
    instantChangementBpm = millis();
    indexBpm = (indexBpm + 1) % 3;
    Serial.print(F("Simulation "));
    Serial.print(BPM_SIMULES[indexBpm]);
    Serial.println(F(" bpm"));
  }
  uint16_t bpm = BPM_SIMULES[indexBpm];
  if (millis() - instantDernierBip >= 60000UL / bpm) {  // période = 60000 / bpm ms
    instantDernierBip = millis();
    fs4Bip(classerBpm(true, bpm));
  }
}
