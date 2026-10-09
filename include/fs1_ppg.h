// fs1_ppg.h : FS1 - acquisition et traitement du signal PPG (écrit à la main,
// aucune bibliothèque de détection de battements).
#ifndef FS1_PPG_H
#define FS1_PPG_H

#include <Arduino.h>

// À appeler une fois dans setup(). Démarre l'échantillonnage par Timer1.
void fs1Initialiser();

// Traite UN échantillon en attente (acquis par l'interruption). Renvoie false
// s'il n'y en a plus. Usage dans loop() : while (fs1TraiterEchantillon()) {...}
bool fs1TraiterEchantillon();

// Renvoie true UNE SEULE FOIS par battement détecté (le drapeau est consommé).
bool fs1BattementDetecte();

// false si : pas encore de mesure, doigt absent, signal plat ou saturé,
// période hors plage, ou plus de battement récent => afficher "--".
bool fs1BpmValide();
uint16_t fs1GetBpm();            // moyenne des NB_PERIODES_MOYENNE dernières périodes
uint16_t fs1GetBpmInstantane();  // calculé sur la dernière période seule

// Valeurs intermédiaires (affichage de la courbe, Teleplot, soutenance).
int16_t fs1GetBrut();         // valeur du CAN (0..1023)
int16_t fs1GetSansDerive();   // brut - moyenne glissante
float fs1GetNormalise();      // sans dérive / max du bloc précédent (-1..1)

#endif
