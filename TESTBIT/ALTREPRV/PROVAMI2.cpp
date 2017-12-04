//----------------------------------------------------------------------------
// PROVAMI.CPP
//----------------------------------------------------------------------------

#include "stdio.h"
#include "conio.h"
//#include <windows.h>

typedef unsigned BIT;

FILE * Fprova;

#define TRACERAPIDA(stringa)  \
   Fprova = fopen("PROVAMIf.TRC", "a"); \
   fprintf(Fprova, "%s\n", stringa);        \
   fclose(Fprova);

#pragma pack (1)

typedef struct _PROVA01 {
   BIT Prima   : 6;
   BIT Seconda : 6;
   BIT Terza   : 4;
} PROVA01;

#pragma pack (1)

typedef struct _PROVA02 {
   BIT Prima   : 12;
   BIT Seconda : 12;
   BIT Terza   : 12;
} PROVA02;

#pragma pack (1)

typedef struct _PROVA03 {
   BIT Prima   : 28;
   BIT Seconda : 28;
   BIT Terza   : 28;
} PROVA03;

#pragma pack (1)

typedef struct _PROVA04 {
   BIT Prima1   : 14;
   BIT Prima2   : 14;
   BIT Seconda1 : 14;
   BIT Seconda2 : 14;
   BIT Terza1   : 14;
   BIT Terza2   : 14;
} PROVA04;

#pragma pack (1)

typedef struct _PROVA05 {
   BIT  StazioneCumulativo     : 1 ;// ST: e' una stazione del servizio cumulativo
   BIT  StazioneFs             : 1 ;// ST: e' una stazione delle FS
   BIT  estera                 : 1 ;// ST: e' una stazione di una o piu' reti estere
   BIT  Aperta                 : 1 ;// ST: e' aperta (puo' anche essere fittizia, merci ecc).
   BIT  vendibile              : 1 ;// ST: ES: e' vendibile (ES: E' vendibile con i precaricati)
   BIT  TipoStazione           : 3 ;// ST: vedere enum TIPOSTAZIONE per i valori
} PROVA05;

#pragma pack (1)

typedef struct _PROVA06 {
   BIT  StazioneCumulativo     : 1 ;// ST: e' una stazione del servizio cumulativo
   BIT  StazioneFs             : 1 ;// ST: e' una stazione delle FS
   BIT  estera                 : 1 ;// ST: e' una stazione di una o piu' reti estere
   BIT  Aperta                 : 1 ;// ST: e' aperta (puo' anche essere fittizia, merci ecc).
   BIT  vendibile              : 1 ;// ST: ES: e' vendibile (ES: E' vendibile con i precaricati)
   BIT  TipoStazione           : 3 ;// ST: vedere enum TIPOSTAZIONE per i valori
   BIT  ImpiantoCommerciale    :17 ;// IC: Codice Impianto Commerciale
} PROVA06;

#pragma pack (1)

typedef struct _PROVA07 {
   BIT  StazioneCumulativo     : 1 ;// ST: e' una stazione del servizio cumulativo
   BIT  StazioneFs             : 1 ;// ST: e' una stazione delle FS
   BIT  estera                 : 1 ;// ST: e' una stazione di una o piu' reti estere
   BIT  Aperta                 : 1 ;// ST: e' aperta (puo' anche essere fittizia, merci ecc).
   BIT  vendibile              : 1 ;// ST: ES: e' vendibile (ES: E' vendibile con i precaricati)
   BIT  TipoStazione           : 3 ;// ST: vedere enum TIPOSTAZIONE per i valori
   BIT  ImpiantoCommerciale    :17 ;// IC: Codice Impianto Commerciale
   BIT  Fittizia               : 1 ;// GR: e' una stazione fittizia
   BIT  IstatRegione           : 5 ;// NZ: Codice Istat regione di appartenenza
   BIT  TariffaRegione         : 5 ;// NZ: Codice tariffario regione
   BIT  Prima_Estensione       : 5 ;// NZ: 1ø regione di estensione
   BIT  Seconda_Estensione     : 5 ;// NZ: 2ø regione di estensione
   BIT  Terza_Estensione       : 5 ;// NZ: 3ø regione di estensione (solo se transito: non vale per origine e destinazione)
   BIT  StazioneTraMarCum      : 1 ;// NZ:  e' una stazione su cui ho un transito mare Tirrenia.
} PROVA07;

#pragma pack (1)

typedef struct _PROVA08 {
   BIT  StazioneCumulativo     : 1 ;// ST: e' una stazione del servizio cumulativo
   BIT  StazioneFs             : 1 ;// ST: e' una stazione delle FS
   BIT  estera                 : 1 ;// ST: e' una stazione di una o piu' reti estere
   BIT  Aperta                 : 1 ;// ST: e' aperta (puo' anche essere fittizia, merci ecc).
   BIT  vendibile              : 1 ;// ST: ES: e' vendibile (ES: E' vendibile con i precaricati)
   BIT  TipoStazione           : 3 ;// ST: vedere enum TIPOSTAZIONE per i valori
   BIT  ImpiantoCommerciale    :17 ;// IC: Codice Impianto Commerciale
   BIT  Fittizia               : 1 ;// GR: e' una stazione fittizia
   BIT  IstatRegione           : 5 ;// NZ: Codice Istat regione di appartenenza
   BIT  TariffaRegione         : 5 ;// NZ: Codice tariffario regione
   BIT  Prima_Estensione       : 5 ;// NZ: 1ø regione di estensione
   BIT  Seconda_Estensione     : 5 ;// NZ: 2ø regione di estensione
   BIT  Terza_Estensione       : 5 ;// NZ: 3ø regione di estensione (solo se transito: non vale per origine e destinazione)
   BIT  StazioneTraMarCum      : 1 ;// NZ:  e' una stazione su cui ho un transito mare Tirrenia.
   BIT  CodiceCCR              :17 ;// FS: SOLO per stazioni FS
} PROVA08;

#pragma pack (1)

typedef struct _PROVA09 {
   BIT  StazioneCumulativo     : 1 ;// ST: e' una stazione del servizio cumulativo
   BIT  StazioneFs             : 1 ;// ST: e' una stazione delle FS
   BIT  estera                 : 1 ;// ST: e' una stazione di una o piu' reti estere
   BIT  Aperta                 : 1 ;// ST: e' aperta (puo' anche essere fittizia, merci ecc).
   BIT  vendibile              : 1 ;// ST: ES: e' vendibile (ES: E' vendibile con i precaricati)
   BIT  TipoStazione           : 3 ;// ST: vedere enum TIPOSTAZIONE per i valori
   BIT  ImpiantoCommerciale    :17 ;// IC: Codice Impianto Commerciale
   BIT  Fittizia               : 1 ;// GR: e' una stazione fittizia
   BIT  IstatRegione           : 5 ;// NZ: Codice Istat regione di appartenenza
   BIT  TariffaRegione         : 5 ;// NZ: Codice tariffario regione
   BIT  Prima_Estensione       : 5 ;// NZ: 1ø regione di estensione
   BIT  Seconda_Estensione     : 5 ;// NZ: 2ø regione di estensione
   BIT  Terza_Estensione       : 5 ;// NZ: 3ø regione di estensione (solo se transito: non vale per origine e destinazione)
   BIT  StazioneTraMarCum      : 1 ;// NZ:  e' una stazione su cui ho un transito mare Tirrenia.
   BIT  CodiceCCR              :17 ;// FS: SOLO per stazioni FS
   BIT  CodiceInstradamentoCvb :10 ;// IS: Codice di instradamento CVB
} PROVA09;

#pragma pack (1)

typedef struct _PROVA10 {
   BIT  CodiceCCR              :17 ;// FS: SOLO per stazioni FS
   BIT  IstatRegione           : 5 ;// NZ: Codice Istat regione di appartenenza
   BIT  TariffaRegione         : 5 ;// NZ: Codice tariffario regione
   BIT  Prima_Estensione       : 5 ;// NZ: 1ø regione di estensione
   BIT  CCRCumulativo1         :17 ;// CU: Per stazioni cumulative
   BIT  Seconda_Estensione     : 5 ;// NZ: 2ø regione di estensione
   BIT  Terza_Estensione       : 5 ;// NZ: 3ø regione di estensione (solo se transito: non vale per origine e destinazione)
   BIT  TipoStazione           : 3 ;// ST: vedere enum TIPOSTAZIONE per i valori
   BIT  StazioneCumulativo     : 1 ;// ST: e' una stazione del servizio cumulativo
   BIT  StazioneFs             : 1 ;// ST: e' una stazione delle FS
   BIT  ImpiantoCommerciale    :17 ;// IC: Codice Impianto Commerciale
   BIT  CodiceInstradamentoCvb :10 ;// IS: Codice di instradamento CVB
   BIT  estera                 : 1 ;// ST: e' una stazione di una o piu' reti estere
   BIT  Aperta                 : 1 ;// ST: e' aperta (puo' anche essere fittizia, merci ecc).
   BIT  vendibile              : 1 ;// ST: ES: e' vendibile (ES: E' vendibile con i precaricati)
   BIT  Fittizia               : 1 ;// GR: e' una stazione fittizia
   BIT  StazioneTraMarCum      : 1 ;// NZ:  e' una stazione su cui ho un transito mare Tirrenia.
} PROVA10;

#pragma pack (1)

typedef struct _PROVA11 {
   long  intero;
   char  pippo;
   long  intero2;
} PROVA11;


//int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
//                   LPSTR lpszCmdLine, int nCmdShow)
int main(int argc, char * argv[])
{
   char cMsg[100];

   sprintf(cMsg, "sizeof(PROVA01)=%d", sizeof(PROVA01));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA02)=%d", sizeof(PROVA02));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA03)=%d", sizeof(PROVA03));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA04)=%d", sizeof(PROVA04));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA05)=%d", sizeof(PROVA05));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA06)=%d", sizeof(PROVA06));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA07)=%d", sizeof(PROVA07));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA08)=%d", sizeof(PROVA08));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA09)=%d", sizeof(PROVA09));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA10)=%d", sizeof(PROVA10));
   TRACERAPIDA(cMsg);
   sprintf(cMsg, "sizeof(PROVA11)=%d", sizeof(PROVA11));
   TRACERAPIDA(cMsg);

   return 0;
}

