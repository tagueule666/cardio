// main.cpp : programme principal du cardio-fréquencemètre CARDIO ECE.
//
// Architecture : chaque fonction secondaire est un module (fsX_*.h/.cpp).
// loop() ne contient AUCUNE attente : à chaque passage, chaque module fait
// un petit morceau de travail puis rend la main (ordonnancement coopératif).
//   - FS1 : l'acquisition est faite par interruption Timer1 (10 ms exactes) ;
//           loop() traite les échantillons en attente.
//   - FS5 : une seule bande d'écran (~3 ms) est envoyée par passage.
// Un passage dure donc quelques ms : les bips partent avec une latence très
// faible (ET4.3) et aucune mesure n'est perdue.
//
// Projet PlatformIO : les .h sont dans include/, les .cpp dans src/.
// La bibliothèque U8g2 est téléchargée automatiquement (lib_deps dans platformio.ini).
// EEPROM et Wire sont fournies avec le framework Arduino.

#include <Arduino.h>
#include "config.h"
#include "zone_cardiaque.h"
#include "fs1_ppg.h"
#include "fs2_rtc.h"
#include "fs3_led.h"
#include "fs4_buzzer.h"
#include "fs5_oled.h"
#include "fs6_eeprom.h"
#include "entrees.h"
#include "langue.h"
#include "menu.h"

// Tests unitaires : MODE_TEST (config.h) remplace setup()/loop() par ceux du
// test choisi (fichiers dans src/tests/). Ce ne sont pas des tests Unity :
// ce sont des programmes de démonstration de chaque module sur le matériel.
#if MODE_TEST == 1
#include "tests/test_fs1.h"
#elif MODE_TEST == 2
#include "tests/test_fs2.h"
#elif MODE_TEST == 3
#include "tests/test_fs3.h"
#elif MODE_TEST == 4
#include "tests/test_fs4.h"
#elif MODE_TEST == 5
#include "tests/test_fs5.h"
#elif MODE_TEST == 6
#include "tests/test_fs6.h"
#elif MODE_TEST == 7
#include "tests/test_entrees.h"
#else

// Appelée à chaque battement détecté par FS1.
static void gererBattement() {
  bool valide = fs1BpmValide();
  ZoneCardiaque zone = classerBpm(valide, fs1GetBpm());
  fs4Bip(zone);  // ET4.1 : un bip par battement, hauteur selon la zone
  // ET6.2 : chaque battement fournit une mesure (bpm sur la dernière période).
  fs6AjouterMesure(valide, fs1GetBpmInstantane(), fs2Maintenant());
}

void setup() {
#if DEBUG_TELEPLOT
  Serial.begin(115200);
#endif
  fs6Initialiser();                         // en premier : la langue est en EEPROM
  langueDefinir((Langue)fs6LireLangue());
  fs2Initialiser();
  fs3Initialiser();
  fs4Initialiser();
  entreesInitialiser();
  menuInitialiser();
  fs5Initialiser(menuPreparerEcran, menuDessinerEcran);
  fs1Initialiser();                         // en dernier : démarre l'échantillonnage
}

void loop() {
  // 1. Traitement du signal : tous les échantillons acquis depuis le dernier passage.
  while (fs1TraiterEchantillon()) {
    fs5AjouterEchantillon(fs1GetNormalise());
    if (fs1BattementDetecte()) gererBattement();
#if DEBUG_TELEPLOT
    // Format Teleplot : ">nom:valeur"
    Serial.print(F(">brut:"));
    Serial.println(fs1GetBrut());
    Serial.print(F(">sans_derive:"));
    Serial.println(fs1GetSansDerive());
    Serial.print(F(">normalise:"));
    Serial.println(fs1GetNormalise());
#endif
  }

  // 2. Voyants : reflètent en permanence l'état courant (s'éteignent dès que
  //    la mesure devient invalide, même sans nouveau battement).
  fs3Afficher(classerBpm(fs1BpmValide(), fs1GetBpm()));

  // 3. Entrées utilisateur
  entreesMettreAJour();
  if (entreesAppuiBip()) {
    fs4BasculerActif();                     // ET4.3
    fs5RafraichirInfo();
  }
  if (entreesAppuiEnregistrer() && !menuEstOuvert()) {
    fs6DemarrerEnregistrement();            // ET6.2
    fs5RafraichirInfo();
  }
  int8_t crans = entreesLireCrans();
  bool appuiEncodeur = entreesAppuiEncodeur();
  bool appuiRetour = entreesAppuiRetour();
  if (menuEstOuvert()) {
    menuGererEntrees(crans, appuiEncodeur, appuiRetour);
  } else {
    if (crans != 0) fs5ChangerEchelle(crans);  // ET5.4 : échelle de temps du PPG
    if (appuiEncodeur) menuOuvrir();
  }

  // 4. Horloge et procédure d'enregistrement
  fs2MettreAJour();
  fs6MettreAJour();

  // 5. Affichage (une bande d'écran)
  fs5MettreAJour();
}

#endif  // MODE_TEST
