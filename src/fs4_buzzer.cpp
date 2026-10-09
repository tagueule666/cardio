// fs4_buzzer.cpp : bips avec tone() (bibliothèque Tone intégrée à Arduino).
//
// Fonctionnement de tone(broche, f, durée) sur l'ATmega328P :
//  - il utilise le Timer2 (8 bits) en mode CTC : le compteur TCNT2 monte de 0
//    à OCR2A puis revient à 0 en déclenchant l'interruption TIMER2_COMPA ;
//  - il choisit le plus petit prédiviseur p tel que
//    OCR2A = F_CPU / (2 * p * f) - 1 tienne sur 8 bits (<= 255) ;
//    ex : f = 880 Hz, p = 64 => OCR2A = 16e6 / (2*64*880) - 1 = 141 ;
//  - à chaque interruption, la broche est INVERSÉE : deux inversions = une
//    période, d'où le facteur 2. On obtient un signal carré de fréquence f
//    qui fait vibrer la membrane du buzzer passif ;
//  - avec une durée, l'interruption décompte 2 * f * durée / 1000 inversions
//    puis arrête le timer elle-même.
// tone() rend donc la main IMMÉDIATEMENT : le son est produit en arrière-plan
// par le timer, ce qui en fait une fonction non bloquante.

#include "fs4_buzzer.h"
#include "config.h"

static bool bipsActifs;

void fs4Initialiser() {
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  bipsActifs = true;
}

void fs4Bip(ZoneCardiaque zone) {
  if (!bipsActifs) return;
  unsigned int frequence;
  switch (zone) {
    case ZONE_BASSE: frequence = FREQUENCE_BIP_GRAVE_HZ; break;
    case ZONE_NORMALE: frequence = FREQUENCE_BIP_MODERE_HZ; break;
    case ZONE_ELEVEE: frequence = FREQUENCE_BIP_AIGU_HZ; break;
    default: return;  // valeur aberrante : pas de bip
  }
  tone(PIN_BUZZER, frequence, DUREE_BIP_MS);
}

void fs4BasculerActif() {
  bipsActifs = !bipsActifs;
  if (!bipsActifs) noTone(PIN_BUZZER);  // coupe aussi un bip en cours
}

bool fs4EstActif() { return bipsActifs; }
