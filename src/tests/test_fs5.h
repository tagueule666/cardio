// test_fs5.h : FS5 seul (sans capteur). Un faux PPG à 72 bpm est généré
// toutes les 10 ms et tracé sur l'écran 2 ; l'écran 1 affiche un compteur.
// Tourner l'encodeur doit changer l'échelle de temps (ET5.4) et la
// graduation affichée (ET5.3). Si un écran reste noir : vérifier les adresses
// I2C dans config.h (0x3C / 0x3D) et OLED_CONTROLEUR_SH1106.
#include "fs5_oled.h"
#include "entrees.h"
#include "config.h"

static unsigned long instantEchantillon;
static uint32_t numeroEchantillon;
static char texteInfo[24];

static void preparerTest() {
  snprintf(texteInfo, sizeof(texteInfo), "Test FS5  %lu", numeroEchantillon / 100);
}

static void dessinerTest(U8G2 &e) {
  e.setFont(u8g2_font_6x10_tr);
  e.drawStr(0, 12, texteInfo);
  char ligne[20];
  snprintf(ligne, sizeof(ligne), "%u ms/pixel", fs5MillisecondesParPixel());
  e.drawStr(0, 30, ligne);
}

void setup() {
  entreesInitialiser();
  fs5Initialiser(preparerTest, dessinerTest);
  instantEchantillon = millis();
}

void loop() {
  if (millis() - instantEchantillon >= PERIODE_ECHANTILLONNAGE_MS) {
    instantEchantillon += PERIODE_ECHANTILLONNAGE_MS;
    numeroEchantillon++;
    // Pulsation simplifiée : fondamentale + harmonique, période 0,83 s (72 bpm).
    float phase = (numeroEchantillon % 83) / 83.0f * TWO_PI;
    fs5AjouterEchantillon(0.8f * sin(phase) + 0.2f * sin(2 * phase));
  }
  entreesMettreAJour();
  int8_t crans = entreesLireCrans();
  if (crans != 0) {
    fs5ChangerEchelle(crans);
    fs5RafraichirInfo();
  }
  fs5MettreAJour();
}
