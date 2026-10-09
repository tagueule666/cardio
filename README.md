# CARDIO ECE : cardio-fréquencemètre Arduino

Projet étudiant ECE : un cardio-fréquencemètre sur Arduino Nano (ATmega328P) avec un
capteur PPG WPSE340. Le traitement du signal est écrit à la main, sans bibliothèque de
détection de battements.

## Fonctions
| FS | Rôle | Fichiers |
|---|---|---|
| FS1 | Capter le rythme : moyenne glissante, normalisation, seuil, front montant, bpm | `fs1_ppg` |
| FS2 | Compter le temps : RTC DS1302 (pilote 3 fils écrit à la main) | `fs2_rtc` |
| FS3 | Voyants rouge / vert / jaune | `fs3_led` |
| FS4 | Bips grave / moyen / aigu à chaque battement | `fs4_buzzer` |
| FS5 | 2 écrans OLED I2C : heure + bpm, courbe PPG graduée | `fs5_oled` |
| FS6 | Enregistrement en EEPROM interne, menu, FR/EN | `fs6_eeprom`, `menu`, `langue` |

## Brochage (Arduino Nano)
| Élément | Broches |
|---|---|
| Capteur PPG | A0 |
| OLED ×2 (I2C, adresses 0x3C / 0x3D) | SDA A4, SCL A5 |
| LED verte / jaune / rouge | D5 / D6 / D7 |
| Buzzer passif | D8 |
| Encodeur CLK / DT / SW | D9 / D10 / A2 |
| RTC DS1302 CLK / DAT / RST | D12 / D13 / D11 |
| Boutons bip / enregistrer / retour (pull-down 10 kΩ) | D2 / D3 / D4 |

Tout le brochage et les réglages sont dans [`include/config.h`](include/config.h).

## Compiler et téléverser
Le projet se compile avec [PlatformIO](https://platformio.org/) (extension VS Code).
1. Ouvrir ce dossier dans VS Code.
2. Cliquer sur ✓ (Build), puis sur → (Upload).

La bibliothèque U8g2 est téléchargée automatiquement.

## Tests par module
Dans `include/config.h`, `MODE_TEST` choisit le programme compilé :

| Valeur | Programme |
|---|---|
| 0 | projet complet |
| 1 | FS1 (courbes dans Teleplot) |
| 2 | FS2 (RTC) |
| 3 | FS3 (LEDs) |
| 4 | FS4 (buzzer) |
| 5 | FS5 (écrans) |
| 6 | FS6 (EEPROM) |
| 7 | encodeur et boutons |

Les tests se trouvent dans `src/tests/`.
