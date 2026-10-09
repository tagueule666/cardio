// test_fs6.h : FS6 seul, piloté au moniteur série (115200 bauds, "Pas de fin de ligne").
//   a : ajoute un enregistrement fictif
//   m : simule une procédure complète (10 mesures de 70 à 79 bpm => 75 attendu)
//   l : liste les enregistrements
//   e : efface tout
//   f : bascule la langue mémorisée
// ET6.1 : ajouter des mesures, débrancher, rebrancher, "l" => elles sont toujours là.
#include "fs6_eeprom.h"

static DateHeure dateFictive() {
  DateHeure d = {26, 10, 9, 14, 30, (uint8_t)(millis() / 1000 % 60)};
  return d;
}

static void lister() {
  Serial.print(fs6Nombre());
  Serial.println(F(" enregistrement(s), du plus recent au plus ancien :"));
  for (uint8_t i = 0; i < fs6Nombre(); i++) {
    Enregistrement e;
    fs6Lire(i, e);
    char ligne[40];
    snprintf(ligne, sizeof(ligne), "#%u  %02u/%02u/20%02u %02u:%02u:%02u  %u bpm", i + 1,
             e.dateHeure.jour, e.dateHeure.mois, e.dateHeure.annee, e.dateHeure.heure,
             e.dateHeure.minute, e.dateHeure.seconde, e.bpm);
    Serial.println(ligne);
  }
}

void setup() {
  Serial.begin(115200);
  fs6Initialiser();
  Serial.println(F("Commandes : a m l e f"));
  lister();
}

void loop() {
  fs6MettreAJour();
  if (!Serial.available()) return;
  char commande = Serial.read();
  switch (commande) {
    case 'a': {
      Enregistrement e = {72, dateFictive()};
      fs6Ajouter(e);
      Serial.println(F("Ajoute 72 bpm"));
      break;
    }
    case 'm':
      fs6DemarrerEnregistrement();
      for (uint8_t i = 0; i < NB_MESURES_PAR_ENREGISTREMENT; i++) {
        fs6AjouterMesure(true, 70 + i, dateFictive());
      }
      Serial.print(F("Procedure terminee, moyenne = "));
      Serial.println(fs6DernierBpmEnregistre());
      break;
    case 'l': lister(); break;
    case 'e':
      fs6ToutEffacer();
      Serial.println(F("Tout efface"));
      break;
    case 'f':
      fs6EcrireLangue(fs6LireLangue() ? 0 : 1);
      Serial.println(fs6LireLangue() ? F("Langue : EN") : F("Langue : FR"));
      break;
  }
}
