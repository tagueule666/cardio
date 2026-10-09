// fs2_rtc.cpp : pilote du DS1302.
//
// Le DS1302 compte le temps grâce à un quartz 32,768 kHz (2^15 Hz : diviser
// 15 fois par 2 donne exactement 1 Hz). Une pile le maintient quand
// l'Arduino est éteint (ET2.3).
//
// Protocole série synchrone 3 fils (proche du SPI, mais DAT est
// bidirectionnelle, alors que le SPI a deux lignes MOSI/MISO séparées) :
//  - RST (CE) à l'état haut pendant tout l'échange ;
//  - 1 octet de commande (adresse + bit lecture/écriture), puis les données ;
//  - bits envoyés poids FAIBLE en premier ;
//  - le DS1302 lit DAT sur le front MONTANT de CLK et écrit DAT sur le front
//    DESCENDANT.
// Les valeurs sont stockées en BCD : 0x47 = "4" et "7" = 47.
//
// Pas de délai ajouté : un digitalWrite dure ~4 µs, bien plus que les ~1 µs
// minimum exigés par le DS1302 entre deux fronts.

#include "fs2_rtc.h"
#include "config.h"

// Commandes (bit 0 : 0 = écriture, 1 = lecture)
#define DS1302_SECONDES_LECTURE 0x81
#define DS1302_PROTECTION_ECRITURE 0x8E
#define DS1302_RAFALE_HORLOGE_ECRITURE 0xBE
#define DS1302_RAFALE_HORLOGE_LECTURE 0xBF
#define DS1302_BIT_ARRET_HORLOGE 0x80  // bit CH (Clock Halt) du registre secondes

static DateHeure heureCourante;
static bool rtcValide;
static unsigned long instantDerniereLecture;

// ---------------------------------------------------------------------------
// Couche bas niveau : échange de bits
// ---------------------------------------------------------------------------

static uint8_t bcdVersDecimal(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
static uint8_t decimalVersBcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }

static void debutEchange() {
  digitalWrite(PIN_RTC_CLK, LOW);   // CLK doit être bas quand RST monte
  digitalWrite(PIN_RTC_RST, HIGH);
}

static void finEchange() {
  digitalWrite(PIN_RTC_RST, LOW);
  pinMode(PIN_RTC_DAT, INPUT);
}

// Envoie un octet, bit de poids faible d'abord. Si une lecture suit, on
// libère DAT juste avant le dernier front descendant, car c'est sur ce front
// que le DS1302 commence à émettre (évite que les deux pilotent la ligne).
static void envoyerOctet(uint8_t octet, bool lectureEnsuite) {
  pinMode(PIN_RTC_DAT, OUTPUT);
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(PIN_RTC_DAT, (octet >> i) & 1);
    digitalWrite(PIN_RTC_CLK, HIGH);  // le DS1302 lit le bit ici
    if (i == 7 && lectureEnsuite) pinMode(PIN_RTC_DAT, INPUT);
    digitalWrite(PIN_RTC_CLK, LOW);
  }
}

// Reçoit un octet : chaque bit est déjà présent sur DAT (émis au front
// descendant précédent), on le lit puis on génère l'impulsion suivante.
static uint8_t recevoirOctet() {
  uint8_t octet = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (digitalRead(PIN_RTC_DAT)) octet |= (1 << i);
    digitalWrite(PIN_RTC_CLK, HIGH);
    digitalWrite(PIN_RTC_CLK, LOW);
  }
  return octet;
}

static uint8_t lireRegistre(uint8_t commandeLecture) {
  debutEchange();
  envoyerOctet(commandeLecture, true);
  uint8_t valeur = recevoirOctet();
  finEchange();
  return valeur;
}

static void ecrireRegistre(uint8_t commandeEcriture, uint8_t valeur) {
  debutEchange();
  envoyerOctet(commandeEcriture, false);
  envoyerOctet(valeur, false);
  finEchange();
}

// ---------------------------------------------------------------------------
// Lecture / écriture de l'heure (mode rafale : les 7 registres d'un coup,
// ce qui garantit une heure cohérente, sans changement de seconde au milieu).
// ---------------------------------------------------------------------------

static bool lireHorloge(DateHeure &dh) {
  uint8_t r[7];
  debutEchange();
  envoyerOctet(DS1302_RAFALE_HORLOGE_LECTURE, true);
  for (uint8_t i = 0; i < 7; i++) r[i] = recevoirOctet();
  finEchange();
  // Ordre des registres : secondes, minutes, heures, jour, mois, jour de la semaine, année
  dh.seconde = bcdVersDecimal(r[0] & 0x7F);  // bit 7 = CH
  dh.minute = bcdVersDecimal(r[1] & 0x7F);
  dh.heure = bcdVersDecimal(r[2] & 0x3F);    // mode 24 h (bit 7 = 0)
  dh.jour = bcdVersDecimal(r[3] & 0x3F);
  dh.mois = bcdVersDecimal(r[4] & 0x1F);
  dh.annee = bcdVersDecimal(r[6]);
  // Contrôle de cohérence : une RTC absente renvoie 0x00 ou 0xFF partout.
  return dh.seconde < 60 && dh.minute < 60 && dh.heure < 24 &&
         dh.jour >= 1 && dh.jour <= 31 && dh.mois >= 1 && dh.mois <= 12 && dh.annee < 100;
}

void fs2Regler(const DateHeure &dh) {
  ecrireRegistre(DS1302_PROTECTION_ECRITURE, 0x00);  // lève la protection
  debutEchange();
  envoyerOctet(DS1302_RAFALE_HORLOGE_ECRITURE, false);
  envoyerOctet(decimalVersBcd(dh.seconde), false);  // CH = 0 : l'horloge tourne
  envoyerOctet(decimalVersBcd(dh.minute), false);
  envoyerOctet(decimalVersBcd(dh.heure), false);    // bit 7 = 0 : mode 24 h
  envoyerOctet(decimalVersBcd(dh.jour), false);
  envoyerOctet(decimalVersBcd(dh.mois), false);
  envoyerOctet(1, false);                           // jour de la semaine (inutilisé)
  envoyerOctet(decimalVersBcd(dh.annee), false);
  envoyerOctet(0x80, false);                        // 8e octet : protection réactivée
  finEchange();
}

// Date/heure de compilation, fournies par le compilateur :
// __DATE__ = "Oct  9 2026", __TIME__ = "14:32:05"
static DateHeure heureDeCompilation() {
  const char *date = __DATE__;
  const char *heure = __TIME__;
  static const char mois[] PROGMEM = "JanFebMarAprMayJunJulAugSepOctNovDec";
  DateHeure dh;
  dh.mois = 1;
  for (uint8_t m = 0; m < 12; m++) {
    if (date[0] == pgm_read_byte(&mois[m * 3]) && date[1] == pgm_read_byte(&mois[m * 3 + 1]) &&
        date[2] == pgm_read_byte(&mois[m * 3 + 2])) {
      dh.mois = m + 1;
    }
  }
  dh.jour = (date[4] == ' ' ? 0 : (date[4] - '0') * 10) + (date[5] - '0');
  dh.annee = (date[9] - '0') * 10 + (date[10] - '0');
  dh.heure = (heure[0] - '0') * 10 + (heure[1] - '0');
  dh.minute = (heure[3] - '0') * 10 + (heure[4] - '0');
  dh.seconde = (heure[6] - '0') * 10 + (heure[7] - '0');
  return dh;
}

// ---------------------------------------------------------------------------
// Interface publique
// ---------------------------------------------------------------------------

void fs2Initialiser() {
  pinMode(PIN_RTC_CLK, OUTPUT);
  pinMode(PIN_RTC_RST, OUTPUT);
  pinMode(PIN_RTC_DAT, INPUT);
  digitalWrite(PIN_RTC_RST, LOW);
  digitalWrite(PIN_RTC_CLK, LOW);

  // Bit CH à 1 = oscillateur arrêté : la RTC n'a jamais été réglée ou la pile
  // a été retirée. Seulement dans ce cas on la règle, pour respecter ET2.3.
  bool horlogeArretee = lireRegistre(DS1302_SECONDES_LECTURE) & DS1302_BIT_ARRET_HORLOGE;
  DateHeure lue;
  bool coherente = lireHorloge(lue);
  if (RTC_FORCER_MISE_A_L_HEURE || horlogeArretee || !coherente) {
    fs2Regler(heureDeCompilation());
  }
  rtcValide = lireHorloge(heureCourante);
  instantDerniereLecture = millis();
}

void fs2MettreAJour() {
  if (millis() - instantDerniereLecture < RTC_PERIODE_LECTURE_MS) return;
  instantDerniereLecture = millis();
  DateHeure lue;
  rtcValide = lireHorloge(lue);
  if (rtcValide) heureCourante = lue;
}

const DateHeure &fs2Maintenant() { return heureCourante; }
bool fs2Valide() { return rtcValide; }
