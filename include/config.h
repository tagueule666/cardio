// config.h : brochage et paramètres de réglage de tout le projet CARDIO.
// Toutes les constantes modifiables sont ici, et seulement ici.
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===========================================================================
// BROCHAGE - carte Arduino Nano (ATmega328P, mêmes broches que le Uno)
// ===========================================================================
// Brochage RÉEL du montage du groupe.
// Remarques :
//  - I2C matériel imposé par l'ATmega328P : SDA = A4, SCL = A5 (les 2 OLED).
//  - L'encodeur est sur D9/D10, qui n'ont pas d'interruption externe
//    (INT0/INT1 = D2/D3, occupées par des boutons) : on utilise les
//    interruptions de CHANGEMENT D'ÉTAT (PCINT), disponibles sur toutes les
//    broches. Voir entrees.cpp.
//  - D0/D1 laissées libres : liaison série USB (téléversement, Teleplot).
//  - Timer1 sert à l'échantillonnage, Timer2 à tone() : la PWM est perdue sur
//    D9, D10, D3, D11. Sans importance : aucune de ces broches ne fait de PWM.

// Capteur PPG WPSE340 : fil signal -> A0, + -> 5V, - -> GND.
#define PIN_PPG A0

// Écrans OLED : SDA -> A4, SCL -> A5, VCC -> 5V (ou 3,3V selon module), GND.
// (pas de constante : broches fixes du bus I2C matériel)

// Encodeur rotatif (type KY-040) : + -> 5V, GND -> GND.
// Si le sens de rotation est inversé, mettre ENCODEUR_SENS_INVERSE à 1
// (plus bas) au lieu de recâbler.
#define PIN_ENCODEUR_A 9          // CLK  (PCINT1)
#define PIN_ENCODEUR_B 10         // DT   (PCINT2)
#define PIN_ENCODEUR_BOUTON A2    // SW   (appui = ouvrir menu / valider)

// Module RTC DS1302 : VCC -> 5V, GND -> GND.
// Ordre supposé = ordre des broches sur le module (CLK, DAT, RST).
// ATTENTION Nano : la LED "L" est câblée directement sur D13 (avec ~1 kohm).
// Elle charge la ligne DAT quand le DS1302 émet : si la RTC lit des valeurs
// incohérentes, déplacer DAT sur une broche libre (A1, A3) et changer ici.
#define PIN_RTC_CLK 12            // CLK  (horloge série)
#define PIN_RTC_DAT 13            // DAT  (donnée, bidirectionnelle)
#define PIN_RTC_RST 11            // RST  (= CE, sélection du circuit)

// Buzzer PASSIF (+ -> D8, - -> GND). Un buzzer actif ne sait faire qu'une
// seule note : il faut un buzzer passif pour les 3 sons de ET4.2.
#define PIN_BUZZER 8

// LEDs : broche -> résistance 220 ohms -> anode LED, cathode -> GND.
#define PIN_LED_VERTE 5
#define PIN_LED_JAUNE 6
#define PIN_LED_ROUGE 7

// Boutons poussoirs avec résistance de 10 kohms EXTERNE en pull-down.
// Rôle de chaque bouton : à permuter ici si besoin (le test 7 affiche le
// nom du bouton appuyé).
#define PIN_BOUTON_BIP 2           // ET4.3 : couper / remettre les bips
#define PIN_BOUTON_ENREGISTRER 3   // ET6.2 : lancer un enregistrement
#define PIN_BOUTON_RETOUR 4        // ET6.4 : retour / quitter le menu
//  1 = PULL-DOWN : 10k entre la broche et GND, bouton entre la broche et 5V
//                  => repos = LOW, appuyé = HIGH
//  0 = PULL-UP   : 10k entre la broche et 5V, bouton entre la broche et GND
//                  => repos = HIGH, appuyé = LOW
#define BOUTONS_PULL_DOWN_EXTERNE 1

// ===========================================================================
// FS1 : traitement du signal PPG
// ===========================================================================
// Période d'échantillonnage FIXE, garantie par une interruption du Timer1.
// 10 ms => 100 Hz : 40 à 150 points par battement (40..200 bpm).
#define PERIODE_ECHANTILLONNAGE_MS 10
// Nombre d'échantillons par minute : sert à convertir une période en bpm.
#define ECHANTILLONS_PAR_MINUTE (60000UL / PERIODE_ECHANTILLONNAGE_MS)

// Moyenne glissante : 100 échantillons = 1 s (couvre ~1 battement).
#define TAILLE_FENETRE_MOYENNE 100
// Bloc de recherche du maximum : 200 échantillons = 2 s (>= 1 battement à 40 bpm).
#define TAILLE_BLOC_MAXIMUM 200

// Seuil de détection sur le signal normalisé, et seuil de réarmement (hystérésis).
#define SEUIL_NORMALISE 0.5f
#define SEUIL_REARMEMENT 0.3f

// Période réfractaire : 300 ms (= 200 bpm) mini entre deux battements.
#define PERIODE_REFRACTAIRE_MS 300

// Plage plausible. Hors plage => valeur aberrante => "--", aucune LED.
#define BPM_MIN 40
#define BPM_MAX 200

// Nombre de périodes moyennées pour le bpm affiché.
#define NB_PERIODES_MOYENNE 4

// Plus de battement depuis ce délai => mesure invalide (doigt retiré).
#define DELAI_SANS_BATTEMENT_MS 2500

// Amplitude crête-à-crête minimale (unités CAN 0..1023) : en dessous, pas de
// doigt / signal plat. VALEUR DE DÉPART À CALIBRER avec test_fs1 + Teleplot.
#define AMPLITUDE_MIN_ADC 15

// Saturation du CAN (capteur ébloui ou doigt trop appuyé).
#define ADC_SATURATION_BAS 5
#define ADC_SATURATION_HAUT 1018
#define DUREE_INVALIDATION_SATURATION_MS 1000

// Mettre 1 si, sur Teleplot, le pic de chaque battement part vers le BAS.
#define PPG_SIGNAL_INVERSE 0

// ===========================================================================
// FS2 : horloge DS1302
// ===========================================================================
// Lecture de la RTC tous les 200 ms (inutile de la lire à chaque loop()).
#define RTC_PERIODE_LECTURE_MS 200
// 1 = remet l'horloge à l'heure de COMPILATION à chaque démarrage.
// À mettre à 1 pour UN téléversement, puis repasser à 0 et re-téléverser,
// sinon l'heure serait remise à une date passée à chaque coupure (ET2.3).
// Si l'oscillateur de la RTC est arrêté (pile neuve, jamais réglée), la mise
// à l'heure est faite automatiquement même à 0.
#define RTC_FORCER_MISE_A_L_HEURE 0

// ===========================================================================
// FS3 : zones de fréquence cardiaque (adulte au repos)
// ===========================================================================
// < 60 bpm : bradycardie (jaune) ; 60..100 : normal (vert) ; > 100 : tachycardie (rouge)
#define BPM_SEUIL_BAS 60
#define BPM_SEUIL_HAUT 100

// ===========================================================================
// FS4 : buzzer
// ===========================================================================
// Trois notes séparées d'une octave chacune : faciles à distinguer à l'oreille.
#define FREQUENCE_BIP_GRAVE_HZ 440    // fréquence basse
#define FREQUENCE_BIP_MODERE_HZ 880   // fréquence normale
#define FREQUENCE_BIP_AIGU_HZ 1760    // fréquence élevée
#define DUREE_BIP_MS 60               // < 300 ms : un bip ne chevauche pas le suivant

// ===========================================================================
// FS5 : écrans OLED
// ===========================================================================
// Modules supposés 128x64. Contrôleur : 0 = SSD1306 (le plus courant en
// 0,96"), 1 = SH1106 (souvent en 1,3"). Si l'image est décalée de 2 pixels
// ou pleine de parasites, essayer l'autre valeur.
#define OLED_CONTROLEUR_SH1106 0
// Adresses I2C 7 bits. Elles se choisissent par un pont/résistance au dos du
// module. Vérifier avec un scanner I2C si un écran reste noir.
#define OLED_ADRESSE_ECRAN_INFO 0x3C  // écran 1 : heure + bpm + menu
#define OLED_ADRESSE_ECRAN_PPG 0x3D   // écran 2 : courbe PPG
// Rafraîchissement de l'écran 1 (l'écran 2 est redessiné en continu).
#define PERIODE_RAFRAICHISSEMENT_INFO_MS 250

// ===========================================================================
// Entrées
// ===========================================================================
#define ANTI_REBOND_MS 30
// Transitions de l'encodeur par cran (4 pour un KY-040 classique ; si 1 cran
// fait avancer de 2, mettre 2... ou l'inverse).
#define ENCODEUR_TRANSITIONS_PAR_CRAN 4
// 1 = inverser le sens de rotation.
#define ENCODEUR_SENS_INVERSE 0

// ===========================================================================
// FS6 : EEPROM
// ===========================================================================
// 7 octets par enregistrement : 100 x 7 = 700 octets sur les 1024 de l'EEPROM.
#define EEPROM_NB_MAX_ENREGISTREMENTS 100
// ET6.2 : un enregistrement = moyenne de 10 mesures consécutives.
#define NB_MESURES_PAR_ENREGISTREMENT 10
// Abandon si les 10 mesures ne sont pas obtenues dans ce délai.
#define DELAI_MAX_ENREGISTREMENT_MS 20000
// Durée d'affichage du message "Enregistré" / "Échec".
#define DUREE_MESSAGE_MS 2000

// ===========================================================================
// Sélection du programme : projet complet ou test d'un seul module
// ===========================================================================
// 0 = projet complet ; 1 = test FS1 ; 2 = FS2 ; 3 = FS3 ; 4 = FS4 ;
// 5 = FS5 ; 6 = FS6 ; 7 = encodeur + boutons. Les tests sont dans src/tests/.
#ifndef MODE_TEST  // (peut aussi être fixé par build_flags = -DMODE_TEST=1)
#define MODE_TEST 0
#endif

// ===========================================================================
// Débogage
// ===========================================================================
// 1 = envoie le signal sur le port série au format Teleplot (115200 bauds).
// Laisser à 0 en démonstration : l'envoi série consomme du temps CPU.
#define DEBUG_TELEPLOT 0

#endif
