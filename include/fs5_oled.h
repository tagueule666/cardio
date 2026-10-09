// fs5_oled.h : FS5 - afficher l'heure, la fréquence cardiaque et le PPG sur
// deux écrans OLED I2C.
//  - écran INFO (1er écran) : contenu fourni par le module menu (fonctions de rappel) ;
//  - écran PPG (2e écran)   : courbe gérée ici, échelle de temps réglable.
#ifndef FS5_OLED_H
#define FS5_OLED_H

#include <Arduino.h>
#include <U8g2lib.h>

// preparer() : appelée au début de chaque image de l'écran INFO, pour figer
//              les données à afficher (elles ne doivent pas changer pendant
//              l'envoi des 8 bandes de l'image).
// dessiner(e) : dessine l'écran INFO avec les fonctions U8g2.
typedef void (*FonctionPreparation)();
typedef void (*FonctionDessin)(U8G2 &ecran);

void fs5Initialiser(FonctionPreparation preparer, FonctionDessin dessiner);

// À appeler pour CHAQUE échantillon traité par FS1 (valeur normalisée -1..1).
void fs5AjouterEchantillon(float valeurNormalisee);

// Encodeur : +1 / -1 cran = échelle de temps plus lente / plus rapide (ET5.4).
void fs5ChangerEchelle(int8_t crans);
uint16_t fs5MillisecondesParPixel();

// Demande de redessiner l'écran INFO dès que possible (après une action).
void fs5RafraichirInfo();

// À appeler à chaque loop() : envoie UNE bande de 8 lignes (~3 ms) puis rend
// la main. Les deux écrans sont mis à jour à tour de rôle, sans jamais bloquer.
void fs5MettreAJour();

#endif
