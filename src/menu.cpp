// menu.cpp : machine à états du menu et dessin de l'écran INFO.
//
//   MENU_FERME --appui encodeur--> MENU_PRINCIPAL
//   MENU_PRINCIPAL : [Consulter] -> MENU_CONSULTATION (tourner = mesure suivante)
//                    [Effacer]   -> MENU_CONFIRMATION (Non / Oui)
//                    [Langue]    -> bascule FR/EN, sauvegardée en EEPROM
//                    [Quitter] ou bouton retour -> MENU_FERME

#include "menu.h"
#include "config.h"
#include "langue.h"
#include "zone_cardiaque.h"
#include "fs1_ppg.h"
#include "fs2_rtc.h"
#include "fs4_buzzer.h"
#include "fs5_oled.h"
#include "fs6_eeprom.h"

enum EtatMenu : uint8_t { MENU_FERME, MENU_PRINCIPAL, MENU_CONSULTATION, MENU_CONFIRMATION };

static const IdTexte ELEMENTS_MENU[] = {T_CONSULTER, T_EFFACER, T_LANGUE, T_QUITTER};
#define NB_ELEMENTS_MENU 4
enum { CHOIX_CONSULTER, CHOIX_EFFACER, CHOIX_LANGUE, CHOIX_QUITTER };

static EtatMenu etat;
static uint8_t choix;
static uint8_t rangConsulte;
static bool confirmationOui;
static unsigned long instantMessageEffacement;
static bool messageEffacementActif;

// Données figées au début de chaque image (voir fs5_oled.h).
static struct {
  EtatMenu etat;
  uint8_t choix;
  bool confirmationOui;
  bool messageEffacement;
  DateHeure date;
  bool rtcValide;
  bool bpmValide;
  uint16_t bpm;
  ZoneCardiaque zone;
  bool bipsActifs;
  EtatEnregistrement enregistrement;
  uint8_t nbMesures;
  uint8_t bpmEnregistre;
  uint8_t nbEnregistrements;
  uint8_t rang;
  Enregistrement consulte;
} instantane;

void menuInitialiser() {
  etat = MENU_FERME;
  choix = 0;
  messageEffacementActif = false;
}

bool menuEstOuvert() { return etat != MENU_FERME; }

void menuOuvrir() {
  etat = MENU_PRINCIPAL;
  choix = 0;
  fs5RafraichirInfo();
}

void menuGererEntrees(int8_t crans, bool appuiValider, bool appuiRetour) {
  if (crans == 0 && !appuiValider && !appuiRetour) return;
  fs5RafraichirInfo();  // réponse visuelle immédiate

  switch (etat) {
    case MENU_PRINCIPAL:
      if (crans != 0) {
        // Défilement circulaire ; (crans % N) peut être négatif, d'où le + N.
        choix = (uint8_t)((choix + NB_ELEMENTS_MENU + crans % NB_ELEMENTS_MENU) % NB_ELEMENTS_MENU);
      }
      if (appuiRetour) {
        etat = MENU_FERME;
      } else if (appuiValider) {
        switch (choix) {
          case CHOIX_CONSULTER:
            rangConsulte = 0;
            etat = MENU_CONSULTATION;
            break;
          case CHOIX_EFFACER:
            confirmationOui = false;  // "Non" par défaut : pas d'effacement par erreur
            etat = MENU_CONFIRMATION;
            break;
          case CHOIX_LANGUE: {
            Langue nouvelle = (langueActuelle() == LANGUE_FR) ? LANGUE_EN : LANGUE_FR;
            langueDefinir(nouvelle);
            fs6EcrireLangue(nouvelle);
            break;
          }
          default:
            etat = MENU_FERME;
            break;
        }
      }
      break;

    case MENU_CONSULTATION: {
      int16_t nombre = fs6Nombre();
      if (crans != 0 && nombre > 0) {
        int16_t rang = (int16_t)rangConsulte + crans;
        rangConsulte = (uint8_t)constrain(rang, 0, nombre - 1);
      }
      if (appuiValider || appuiRetour) etat = MENU_PRINCIPAL;
      break;
    }

    case MENU_CONFIRMATION:
      if (crans > 0) confirmationOui = true;   // tourner à droite = Oui
      if (crans < 0) confirmationOui = false;  // tourner à gauche = Non
      if (appuiRetour) {
        etat = MENU_PRINCIPAL;
      } else if (appuiValider) {
        if (confirmationOui) {
          fs6ToutEffacer();
          messageEffacementActif = true;
          instantMessageEffacement = millis();
        }
        etat = MENU_PRINCIPAL;
      }
      break;

    default:
      break;
  }
}

void menuPreparerEcran() {
  if (messageEffacementActif && millis() - instantMessageEffacement > DUREE_MESSAGE_MS) {
    messageEffacementActif = false;
  }
  instantane.etat = etat;
  instantane.choix = choix;
  instantane.confirmationOui = confirmationOui;
  instantane.messageEffacement = messageEffacementActif;
  instantane.date = fs2Maintenant();
  instantane.rtcValide = fs2Valide();
  instantane.bpmValide = fs1BpmValide();
  instantane.bpm = fs1GetBpm();
  instantane.zone = classerBpm(instantane.bpmValide, instantane.bpm);
  instantane.bipsActifs = fs4EstActif();
  instantane.enregistrement = fs6EtatEnregistrement();
  instantane.nbMesures = fs6NbMesuresAcquises();
  instantane.bpmEnregistre = fs6DernierBpmEnregistre();
  instantane.nbEnregistrements = fs6Nombre();
  instantane.rang = rangConsulte;
  if (etat == MENU_CONSULTATION) fs6Lire(rangConsulte, instantane.consulte);
}

// ---------------------------------------------------------------------------
// Dessin (appelé une fois par bande de 8 lignes : uniquement des lectures)
// ---------------------------------------------------------------------------

static void dessinerTitre(U8G2 &e, const char *titre) {
  e.setFont(u8g2_font_6x10_tr);
  e.drawBox(0, 0, 128, 11);  // bandeau plein, texte "en négatif"
  e.setDrawColor(0);
  e.drawStr(2, 9, titre);
  e.setDrawColor(1);
}

static void dessinerGrandBpm(U8G2 &e, bool valide, uint16_t bpm, uint8_t yBase) {
  char texteBpm[6];
  if (valide) {
    utoa(bpm, texteBpm, 10);
  } else {
    strcpy(texteBpm, "--");  // ET5.1 : tiret si valeur non conforme / pas de doigt
  }
  e.setFont(u8g2_font_logisoso28_tn);
  e.drawStr(8, yBase, texteBpm);
  e.setFont(u8g2_font_6x10_tr);
  e.drawStr(80, yBase, "bpm");
}

static void dessinerEcranPrincipal(U8G2 &e) {
  char ligne[24];
  e.setFont(u8g2_font_6x10_tr);

  // Ligne 1 : heure et date
  const DateHeure &d = instantane.date;
  if (instantane.rtcValide) {
    snprintf(ligne, sizeof(ligne), "%02u:%02u:%02u  %02u/%02u/%02u",
             d.heure, d.minute, d.seconde, d.jour, d.mois, d.annee);
  } else {
    strcpy(ligne, "--:--:--  RTC ?");
  }
  e.drawStr(0, 9, ligne);

  // État des bips, au-dessus de "bpm"
  e.drawStr(80, 26, texte(instantane.bipsActifs ? T_BIP_ON : T_BIP_OFF));

  // Fréquence cardiaque en grand
  dessinerGrandBpm(e, instantane.bpmValide, instantane.bpm, 46);

  // Ligne du bas : enregistrement en cours / résultat, sinon zone cardiaque
  switch (instantane.enregistrement) {
    case ENREG_EN_COURS:
      snprintf(ligne, sizeof(ligne), "%s %u/%u", texte(T_ENREGISTREMENT),
               instantane.nbMesures, NB_MESURES_PAR_ENREGISTREMENT);
      break;
    case ENREG_REUSSI:
      snprintf(ligne, sizeof(ligne), "%s %u bpm", texte(T_ENREGISTRE), instantane.bpmEnregistre);
      break;
    case ENREG_ECHEC:
      strcpy(ligne, texte(T_ECHEC));
      break;
    default:
      switch (instantane.zone) {
        case ZONE_BASSE: strcpy(ligne, texte(T_ZONE_BASSE)); break;
        case ZONE_NORMALE: strcpy(ligne, texte(T_ZONE_NORMALE)); break;
        case ZONE_ELEVEE: strcpy(ligne, texte(T_ZONE_ELEVEE)); break;
        default: strcpy(ligne, texte(T_ATTENTE_SIGNAL)); break;
      }
      break;
  }
  e.drawStr(0, 62, ligne);
}

static void dessinerMenuPrincipal(U8G2 &e) {
  dessinerTitre(e, texte(instantane.messageEffacement ? T_MESURES_EFFACEES : T_MENU));
  for (uint8_t i = 0; i < NB_ELEMENTS_MENU; i++) {
    uint8_t y = 23 + i * 12;
    if (i == instantane.choix) e.drawStr(0, y, ">");
    e.drawStr(10, y, texte(ELEMENTS_MENU[i]));
  }
}

static void dessinerConsultation(U8G2 &e) {
  char ligne[24];
  if (instantane.nbEnregistrements == 0) {
    dessinerTitre(e, texte(T_MESURE));
    e.drawStr(0, 36, texte(T_AUCUNE_MESURE));
    return;
  }
  // "Mesure 1/12" : 1 = la plus récente
  snprintf(ligne, sizeof(ligne), "%s %u/%u", texte(T_MESURE), instantane.rang + 1, instantane.nbEnregistrements);
  dessinerTitre(e, ligne);
  const DateHeure &d = instantane.consulte.dateHeure;
  snprintf(ligne, sizeof(ligne), "%02u/%02u/20%02u %02u:%02u:%02u",
           d.jour, d.mois, d.annee, d.heure, d.minute, d.seconde);
  e.drawStr(0, 23, ligne);
  dessinerGrandBpm(e, true, instantane.consulte.bpm, 60);
}

static void dessinerConfirmation(U8G2 &e) {
  dessinerTitre(e, texte(T_TOUT_EFFACER));
  // Deux "boutons" : celui sélectionné est inversé.
  const uint8_t xNon = 14, xOui = 74, largeur = 40, y = 30, hauteur = 16;
  bool oui = instantane.confirmationOui;
  e.drawFrame(xNon, y, largeur, hauteur);
  e.drawFrame(xOui, y, largeur, hauteur);
  e.drawBox(oui ? xOui : xNon, y, largeur, hauteur);
  e.setDrawColor(oui ? 1 : 0);
  e.drawStr(xNon + 10, y + 12, texte(T_NON));
  e.setDrawColor(oui ? 0 : 1);
  e.drawStr(xOui + 10, y + 12, texte(T_OUI));
  e.setDrawColor(1);
}

void menuDessinerEcran(U8G2 &e) {
  e.setFont(u8g2_font_6x10_tr);
  switch (instantane.etat) {
    case MENU_PRINCIPAL: dessinerMenuPrincipal(e); break;
    case MENU_CONSULTATION: dessinerConsultation(e); break;
    case MENU_CONFIRMATION: dessinerConfirmation(e); break;
    default: dessinerEcranPrincipal(e); break;
  }
}
