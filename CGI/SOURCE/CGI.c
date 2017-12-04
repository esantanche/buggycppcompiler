//
//   Project: SIPAX
//   File: CGIMAIN.C
//   Author: Ing. Emanuele Maria Santanch‚
//   Description:
//   Note:

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#define INCL_DOSPROCESS

#include "dummy.h"
//#include <std.h>
//typedef unsigned long BOOL;
//#include <stringa.h>
#include "cgiwin.rh"
#include <windows.h>

// definizioni prelevate da os2def.h che non è
// possibile includere tutto perché ci sono molte definizioni
// in conflitto con Windows (winbase.h)
typedef unsigned long  APIRET;
typedef unsigned long  LHANDLE;
typedef PVOID *PPVOID;
typedef LHANDLE PID;            /* pid  */
typedef LHANDLE TID;            /* tid  */
//typedef LHANDLE HMODULE;        /* hmod */
typedef HMODULE *PHMODULE;
typedef PID *PPID;
typedef TID *PTID;
typedef int ( APIENTRY _PFN)  ();
typedef _PFN    *PFN;
typedef struct _QWORD          /* qword */
{
   ULONG   ulLo;
   ULONG   ulHi;
} QWORD;
typedef QWORD *PQWORD;
#define BOOL32 BOOL
#define PBOOL32 PBOOL
#define CPSZ const char *

#include <stdio.h>
#include <time.h>
extern "C" {
#include <bsedos.h>
}
#include <stringa.h>
#include "trc2.h"
#include "htapi.h"
#include "asconfig.h"
#include "asfsutil.h"
#include "ascgiarg.h"

//#include "winbase.h"    //Windows



HINSTANCE hInstanza_applicazione;
HWND hwndDialog_principale = NULL;
char cNome_file_input[MAX_PATH];
char cNome_file_output[MAX_PATH];
char cQuery_string_da_command_line[512];

BOOL fWin32s;

#define MSG_INIZIALE "Operazione in corso\nAttendere ..."

APIRET Messaggio_sulla_dialog(char * cMessaggio);
long Leggi_query_string(unsigned char *value,        /* o; buffer in which to place the value */
                                            unsigned long *value_length); /* i/o; size of buffer/length of value */
APIRET Apertura_finestra_messaggio(void);
APIRET Chiusura_finestra_messaggio(void);
BOOL Parsing_command_line(LPSTR lpszCmdLine);
BOOL Richiesta_pagina_iniziale(unsigned char *ucQuery_string);
void Pagina_di_attesa(void);
void Metti_dialog_in_primo_piano(void);

void HTTPD_LINKAGE AsFsQ5(unsigned char *handle, long *return_code );
void HTTPD_LINKAGE AsFsT5(unsigned char *handle, long *return_code );
void HTTPD_LINKAGE AsFsI5(unsigned char *handle, long *return_code );
void Tentativo_risposta_rapida(void);

// Questa funzione si trova in FILE.C della libreria BASELIB
APIRET Ultimo_errore(void);

extern ConfORARIO config_orario;  // EMS ?C

#undef TRCbegin
#define TRCbegin { \
   FILE * Ftrace = fopen("CGIMY.TRC","a");    \
   char cMsg[300];  \
   sprintf(cMsg,

#undef TRCend
#define TRCend );  \
   fprintf(Ftrace, "%s\n", cMsg);  \
   fclose(Ftrace);                 \
   }

#define TRCstart  { \
   FILE * Ftrace = fopen("CGIMY.TRC","w");    \
   fprintf(Ftrace, "INIZIO Trace alternativa\n");  \
   fclose(Ftrace);                 \
   }

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///                         MAIN
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

//int main(int argc,char * argv[])
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
            LPSTR lpszCmdLine, int nCmdShow)
{
   //HINSTANCE hInstance;                /* handle of current instance */
   //HINSTANCE hPrevInstance;            /* handle of previous instance   */
   //LPSTR lpszCmdLine;                  /* address of command line */
   //int nCmdShow;                       /* show state of window */

   #undef TRCRTN
   #define TRCRTN "WinMain_CGI"

   long Rc;
   unsigned char ucQuery_string[4096];
   unsigned long ulLunghezza;
   APIRET arUltimo_errore;
   char cMsg[2000];
   MSG msgMessaggio_coda;
   FILE * fOut;
   long lReturn_code = 0;

   fWin32s = GetVersion() & 0x80000000;           // EMS

   TRCstart
   TRCbegin "Entro in CGIWIN" TRCend

   if (fWin32s)
      TRCbegin "PIATTAFORMA WIN32s ****************************" TRCend
   else
      TRCbegin "PIATTAFORMA NONONONON WIN32s ****************************" TRCend

   if (lpszCmdLine != NULL)
      TRCbegin "Command line: %s", lpszCmdLine TRCend
   else
      TRCbegin "lpszCmdLine nullo" TRCend

   //?C?C?CAPIRET Inizializza_utilizzo_modulo_FILE(void)

   // EMS se c'è gia una istanza dell'applicazione aperta
   // esco
   if (hPrevInstance != NULL) return 0;

   TRCbegin "Supero controllo (hPrevInstance != NULL)" TRCend

   // buttare:
   /*
   FILE * Fasc = fopen("ASCII.TXT", "w");
   char cTable[256];
   char cLine[80];
   char cCurrent_char_string[5];
   char cCurrent_char;
   for (int i=0; i<16; i++) {
      cLine[0] = '\0';
      sprintf(cLine, "|%3d|%X_", i*16, i);
      for (int j=0; j<16; j++) {
         cCurrent_char = i*16+j;
         if (cCurrent_char == 10)
            cCurrent_char = 'L';
         else
         if (cCurrent_char == 9)
            cCurrent_char = 'H';
         else
         if (cCurrent_char == 0)
            cCurrent_char = 'N';
         sprintf(cCurrent_char_string , "| %c ", cCurrent_char);
         strcat(cLine, cCurrent_char_string);
      }
      fprintf(Fasc, "%s|\n", cLine);
   }
   fclose(Fasc);
   */

   hInstanza_applicazione = hInstance;

   TRACEREGISTER2(NULL,"CGI","CGI.TRC");

   // Tentativo_risposta_rapida();  // EMS ?C??C
   //DosSleep(10000);
   Parsing_command_line(lpszCmdLine);
   //Pagina_di_attesa();

   arUltimo_errore = Apertura_finestra_messaggio();
   if (arUltimo_errore != NO_ERROR) {
      TRACEINT("Apertura_finestra_messaggio fallita", arUltimo_errore);
      TRACETERMINATE;
      return arUltimo_errore;
   }

   if (!Parsing_command_line(lpszCmdLine)) {
      Messaggio_sulla_dialog("Errore tecnico (vedere log errori)\nProgramma in chiusura, attendere prego");
      DosSleep(10000);
      TRCbegin "Errore in Parsing_command_line" TRCend
      TRACESTRING("Parsing_command_line(lpszCmdLine)");
      TRACEVSTRING2(lpszCmdLine);
      TRACETERMINATE;
      Chiusura_finestra_messaggio();
      return 0;
   }

   //strcpy(cNome_file_output, "C:\\temp\\cgiout.cgi");  // ?C?C?C?C?C

   ulLunghezza = 4096;



   Rc = Leggi_query_string(ucQuery_string,    /* o; buffer in which to place the value */
                           &ulLunghezza);     /* i/o; size of buffer/length of value */

   TRCbegin "Query string: %s", ucQuery_string TRCend

   if (Rc != HTTPD_SUCCESS) {
      TRACEINT("Errore in Leggi_query_string:",Rc);
      TRACETERMINATE;
      Chiusura_finestra_messaggio();
      return 0;
   }

   // Questo serve solo per sapere il nome del file di output
   {
   char cPath_report[MAX_PATH];
   sprintf(cPath_report, "%s\\REPORT.CGI", getenv("ETC"));
   FILE * Freport = fopen(cPath_report, "w");
   struct tm *newtime;
   time_t ltime;
   time(&ltime);
   newtime = localtime(&ltime);
   fprintf(Freport, "CGI avviata in data/ora: %s\n", asctime(newtime));
   fprintf(Freport, "COMMAND LINE:%s\n", lpszCmdLine);
   fprintf(Freport, "FILE INPUT  : %s\n", cNome_file_input);
   fprintf(Freport, "FILE OUTPUT : %s\n", cNome_file_output);
   fprintf(Freport, "QUERY STRING: %s\n", ucQuery_string);
   fclose(Freport);
   FILE * Fcmd = fopen("d:\\ems\\cmd\\r.bat","w");
   fprintf(Fcmd, "d:\\32ivpro\\iviewp32 %s\n", cNome_file_output);
   fclose(Fcmd);
   }

   //sprintf(cMsg, "Query string:|%s|", (char *)ucQuery_string);
   //TRACESTRING("Query string:|"+(STRINGA((char *)ucQuery_string)+"|"));
   //TRACESTRING(cMsg);
   TRACEINT("Lunghezza:", ulLunghezza);
   TRACEINT("Rc:", Rc);

   AsFsI5((unsigned char *)"dummy", &lReturn_code);
   TRCbegin "Return code AsFsI5: %d (0x%x)", lReturn_code, lReturn_code TRCend

   TRCbegin "max_sol=%d lang=%s %s", config_orario.max_sol, config_orario.languages.pairlist->name,
            config_orario.languages.pairlist->value TRCend

   TRCbegin "sl_templ=%s %s", config_orario.sl_template->pairlist->name,
            config_orario.sl_template->pairlist->name TRCend

   if (Richiesta_pagina_iniziale(ucQuery_string)) {
      AsFsT5((unsigned char *)"dummy", &lReturn_code );
      TRCbegin "Return code AsFsT5: %d (0x%x)", lReturn_code, lReturn_code TRCend
      Chiusura_finestra_messaggio();
      TRACETERMINATE;
      TRACETERMINATE;
      return 0;
   }

   Metti_dialog_in_primo_piano();
   AsFsQ5((unsigned char *)"dummy", &lReturn_code );
   TRCbegin "Return code AsFsQ5: %d (0x%x)", lReturn_code, lReturn_code TRCend

   AsFsT5((unsigned char *)"dummy", &lReturn_code );
   TRCbegin "Return code AsFsT5: %d (0x%x)", lReturn_code, lReturn_code TRCend

   TRACESTRING("Sto uscendo 102");

   Chiusura_finestra_messaggio();

   TRACETERMINATE;
   TRACETERMINATE;

   /*
   struct tm *newtime;
   time_t ltime;
   time(&ltime);
   newtime = localtime(&ltime);
   FILE * Fgo = fopen("c:\\ems\\cgi\\exewin\\go.del","w");
   fprintf(Fgo, "CGI terminata %s\n", asctime(newtime));
   fclose(Fgo);
   */

   //?C?C?C APIRET Concludi_utilizzo_modulo_FILE(void)

   TRCbegin "Dopo questa trace c'è solo ExitProcess\n" TRCend

   ExitProcess(0);
   return 0;
}

///////////////////////////////////////////////////////////////////////////////////
//            Pagina_di_attesa
///////////////////////////////////////////////////////////////////////////////////

void Pagina_di_attesa(void)
{
   FILE * Fout = fopen(cNome_file_output,"w");
   //strcpy(cNome_file_output, "C:\\temp\\cgiout.cgi");
   fprintf(Fout, "<HTML>\n" );
   fprintf(Fout, "<HEAD>\n" );
   fprintf(Fout, "<TITLE>Orario ufficiale delle Ferrovie dello Stato S.p.A.</TITLE>\n" );
   //cNome_file_output[0]=toupper(cNome_file_output[0]);
   //fprintf(Fout, "<META HTTP-EQUIV=\"REFRESH\" CONTENT=\"15\">\n");
   fprintf(Fout, "</HEAD><BODY BACKGROUND=\"d:\\ems\\hpfs\\immagini\\cluback.jpg\">\n" );
   fprintf(Fout, "<br><br><center><h3>Spiacenti: il programma e' in via di perfezionamento<br>\n");
   fprintf(Fout, "Le chiediamo di attendere alcuni secondi e poi di selezionare<br>\n");
   fprintf(Fout, "il pulsante ReLoad.</h3>\n");
   fprintf(Fout, "<h4>L'applicazione di ricerca delle soluzioni<br>\n");
   fprintf(Fout, "indica esattamente il termine dell'elaborazione</h4><br><br><br>\n");
   fprintf(Fout, "<h3>Sorry: the software is going to be completed<br>\n");
   fprintf(Fout, "We ask you to wait some seconds and later to select<br>\n");
   fprintf(Fout, "the ReLoad pushbutton</h3>\n");
   fprintf(Fout, "<h4>The Solutions Search Application will show you the<br>\n");
   fprintf(Fout, "exact end of computations</h4>\n");
   fprintf(Fout, "</body></html>\n" );
   fclose(Fout);
   //TRACETERMINATE;
   //ExitProcess(0);
   //return 0;  ù
   return;
}

void Tentativo_risposta_rapida(LPSTR lpszCmdLine)
{
   //Parsing_command_line(lpszCmdLine);

   //ulLunghezza = 4096;

   //Rc = Leggi_query_string(ucQuery_string,    /* o; buffer in which to place the value */
   //                        &ulLunghezza);     /* i/o; size of buffer/length of value */
   /*
   FILE * FOut = fopen(cNome_file_output,"w");
   fprintf(Fout, "<html>\n" );
   fprintf(Fout, "<head>\n" );
   fprintf(Fout, "<title> Risposta tent rapido </title>\n" );
   fprintf(Fout, "</head><body>\n" );
   fprintf(FOut, "%s\n", ucQuery_string);
   fprintf(Fout, "</body></html>\n" );
   fclose(FOut);
   */

   return;
}

////////////////////////////////////////////////////
//            Apertura_finestra_messaggio
////////////////////////////////////////////////////

APIRET Apertura_finestra_messaggio(void)
{
   #undef TRCRTN
   #define TRCRTN "Apertura_finestra_messaggio"

   APIRET arUltimo_errore;
   int iDimensione_coda_messaggi;
   BOOL bCoda_messaggi_non_creata;

   // ??C
   //chiamarla con size minori finché non torna TRUE

   for (iDimensione_coda_messaggi = 100, bCoda_messaggi_non_creata = TRUE;
        iDimensione_coda_messaggi >= 10;
        iDimensione_coda_messaggi -= 10) {
      if (SetMessageQueue(iDimensione_coda_messaggi)) {
         bCoda_messaggi_non_creata = FALSE;
         break;
      }
   }

   if (bCoda_messaggi_non_creata) {
      arUltimo_errore = Ultimo_errore();
      TRACEINT("SetMessageQueue fallita", arUltimo_errore);
      return arUltimo_errore;
   }

   TRACEINT("Dimensione coda creata:", iDimensione_coda_messaggi);

   /*
   HWND CreateDialog(hinst, lpszTemplate, hwndOwner, dlgprc)

   HANDLE hinst;  /* handle of application instance  *
   LPCTSTR lpszTemplate;                 /* identifies dialog box template name *
   HWND hwndOwner;                       /* handle of owner window  *
   DLGPROC dlgprc;                       /* address of dialog box procedure *
   */

   hwndDialog_principale = CreateDialog(hInstanza_applicazione,
                                                            MAKEINTRESOURCE(DLG_PRINCIPALE),
                                                            NULL,
                                                            NULL);

   if (hwndDialog_principale == NULL) {
      arUltimo_errore = Ultimo_errore();
      TRACEINT("CreateDialog fallita", arUltimo_errore);
      return arUltimo_errore;
   }

   Metti_dialog_in_primo_piano();

   arUltimo_errore = Messaggio_sulla_dialog(MSG_INIZIALE);
   if (arUltimo_errore != NO_ERROR) {
      TRACEINT("Messaggio_sulla_dialog fallita", arUltimo_errore);
      return arUltimo_errore;
   }

   /*
   BOOL ShowWindow(hwnd, nCmdShow)
   HWND hwnd;  /* handle of window       *
   int nCmdShow;  /* show state of window   *
   */

   return NO_ERROR;
}

///////////////////////////////////////////////////
//              Metti_dialog_in_primo_piano
///////////////////////////////////////////////////
void Metti_dialog_in_primo_piano(void)
{

   ShowWindow(hwndDialog_principale, SW_RESTORE);
   ShowWindow(hwndDialog_principale, SW_SHOW);
   BringWindowToTop(hwndDialog_principale);

   return;
}

////////////////////////////////////////////////////
//              Messaggio_sulla_dialog
////////////////////////////////////////////////////

APIRET Messaggio_sulla_dialog(char * cMessaggio)
{
   #undef TRCRTN
   #define TRCRTN "Messaggio_sulla_dialog"

   APIRET arUltimo_errore;

   // max 45 caratteri per linea

   /*
   BOOL SetDlgItemText(hwndDlg, idControl, lpsz)
   HWND hwndDlg;  /* handle of dialog box   *
   int idControl; /* identifier of control  *
   LPCTSTR lpsz;  /* text to set         *
   */

   if (!SetDlgItemText(hwndDialog_principale,   //hwndDlg,
                                           TXT_MESSAGGIO,           //idControl,
                                           cMessaggio)) {           //lpsz)
      arUltimo_errore = Ultimo_errore();
      TRACEINT("SetDlgItemText fallita", arUltimo_errore);
      return arUltimo_errore;
   }

   return NO_ERROR;
}

////////////////////////////////////////////////////
//           Chiusura_finestra_messaggio
////////////////////////////////////////////////////

APIRET Chiusura_finestra_messaggio(void)
{
   #undef TRCRTN
   #define TRCRTN "Chiusura_finestra_messaggio"

   APIRET arUltimo_errore;

   if (!DestroyWindow(hwndDialog_principale)) {
      arUltimo_errore = Ultimo_errore();
      TRACEINT("DestroyWindow fallita", arUltimo_errore);
      return arUltimo_errore;
   }

   return NO_ERROR;
}

////////////////////////////////////////////////////
//              Parsing_command_line
////////////////////////////////////////////////////

BOOL Parsing_command_line(LPSTR lpszCmdLine)
{
   #undef TRCRTN
   #define TRCRTN "Parsing_command_line"

   char * token;
   int iNumero_argomenti;
   BOOL bArgomenti_trovati = FALSE;
   char * cCommand_line;

   cCommand_line = (char *) malloc(strlen(lpszCmdLine)+1);
   memcpy(cCommand_line, lpszCmdLine, strlen(lpszCmdLine)+1);

   //char cNome_file_input[MAX_PATH];
   //char cNome_file_output[MAX_PATH];

   TRACESTRING(lpszCmdLine);

   cQuery_string_da_command_line[0] = '\0';
   cNome_file_input[0] = '\0';
   cNome_file_output[0] = '\0';

   iNumero_argomenti = 0;
   token = strtok(cCommand_line," ");
   do {

      TRACEINT("Argomento n.ro:", iNumero_argomenti);
      TRACESTRING(token);

      if (iNumero_argomenti == 0)
         strcpy(cNome_file_input, token);
      else
      if (iNumero_argomenti == 2) {
         strcpy(cNome_file_output, token);
         if (strcmp(cNome_file_input, "empty") != 0) {
            bArgomenti_trovati = TRUE;
            break;
         }
      }
      else
      if (iNumero_argomenti == 3 && !bArgomenti_trovati) {
         strcpy(cQuery_string_da_command_line, token);
            bArgomenti_trovati = TRUE;
      }

      iNumero_argomenti++;

   } while (token = strtok(NULL," "));

   free(cCommand_line);

   if (bArgomenti_trovati) {
      TRCbegin "Nome file IN :%s", cNome_file_input  TRCend
      TRCbegin "Nome file OUT:%s", cNome_file_output TRCend
      return TRUE;
   } else {
      TRACEINT("Errore: argomenti trovati solo:", iNumero_argomenti);
      TRACESTRING("Argomenti: file_input dummy file_output");
      return FALSE;
   }

}



////////////////////////////////////////////////////
//           Richiesta_pagina_iniziale
////////////////////////////////////////////////////

BOOL Richiesta_pagina_iniziale(unsigned char *ucQuery_string)
{
   #undef TRCRTN
   #define TRCRTN "Richiesta_pagina_iniziale"

   CGIPARMS cgip;
   char *cDirectory_impostazioni_cgi;
   char cPath_file_pagina_iniziale[MAX_PATH];

   //TRACESTRING(STRINGA("Query string: |")+(STRINGA((const char *)ucQuery_string)+STRINGA("|")));

   cgip = Parse_Args((char *)ucQuery_string);

   //TRACEVLONG(cgip.nparms);
   //TRACESTRING(STRINGA("Name=|")+(cgip.parms->name+
   //            STRINGA("| Value=|"))+(cgip.parms->value+STRINGA("|")));

   if (cgip.nparms == 1 && 0 == strcmpi(cgip.parms->name, "lang") ) {
      cDirectory_impostazioni_cgi = getenv("ETC");
      //TRACEVSTRING(cDirectory_impostazioni_cgi);
      if (NULL == cDirectory_impostazioni_cgi) return FALSE;
      //strcpy(cPath_file_pagina_iniziale, etc);
      //strcat(cPath_file_pagina_iniziale,"\\");
      // Cerco il suffisso previsto per la lingua selezionata (vedi file
      // fsorario.cnf) per costruire il nome della pagina da restituire
      BOOL bLingua_trovata = FALSE;
      for (int i=0; i<config_orario.languages.count; i++) {
         if (strcmpi(cgip.parms->value, config_orario.languages.pairlist[i].name) == 0) {
            bLingua_trovata = TRUE;
            sprintf(cPath_file_pagina_iniziale, "%s\\ORAR%s.HTM", cDirectory_impostazioni_cgi,
                    config_orario.languages.pairlist[i].value);
         }
      }
      if (!bLingua_trovata) {
         ERRSTRING("Lingua non trovata, uso italiana");
         TRCbegin "Lingua %s non trovata, uso italiana", cgip.parms->value TRCend
         sprintf(cPath_file_pagina_iniziale, "%s\\ORARIT.HTM", cDirectory_impostazioni_cgi);
      }
      FILE * Fsource = fopen(cPath_file_pagina_iniziale, "r");
      if (Fsource == NULL) {
         TRCbegin "File pagina iniziale non trovato: %s", cPath_file_pagina_iniziale TRCend
         return FALSE;
      }
      FILE * Ftarget = fopen(cNome_file_output, "w");
      char cLine[500];
      while (!feof(Fsource)) {
         if (fgets(cLine, sizeof(cLine), Fsource) == NULL) break;
         fputs(cLine, Ftarget);
      }
      fclose(Fsource);
      fclose(Ftarget);

      return TRUE;
   }

   TRCbegin "max_sol=%d lang=%s %s", config_orario.max_sol, config_orario.languages.pairlist->name,
            config_orario.languages.pairlist->value TRCend

   Free_Args(cgip);

   return FALSE;
}
