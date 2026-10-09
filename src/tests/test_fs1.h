// test_fs1.h : FS1 seul. Courbes et bpm envoyés au format Teleplot
// (extension VS Code "Teleplot", 115200 bauds). Le traceur série de l'IDE
// Arduino ne lit pas ce format : utiliser Teleplot.
// À vérifier : la courbe "normalise" franchit le seuil 0,5 une fois par
// battement ; bpm cohérent avec un comptage manuel du pouls sur 30 s x 2.
#include "fs1_ppg.h"
#include "config.h"

void setup() {
  Serial.begin(115200);
  fs1Initialiser();
}

void loop() {
  while (fs1TraiterEchantillon()) {
    Serial.print(F(">brut:"));
    Serial.println(fs1GetBrut());
    Serial.print(F(">sans_derive:"));
    Serial.println(fs1GetSansDerive());
    Serial.print(F(">normalise:"));
    Serial.println(fs1GetNormalise());
    Serial.print(F(">seuil:"));
    Serial.println(SEUIL_NORMALISE);
    if (fs1BattementDetecte()) {
      Serial.print(F(">bpm:"));
      Serial.println(fs1BpmValide() ? fs1GetBpm() : 0);  // 0 = invalide ("--")
    }
  }
}
