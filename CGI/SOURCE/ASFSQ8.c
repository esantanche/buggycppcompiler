// EMS ?C?C #include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HTAPI.h"
#include "asconfig.h"


/* variabili globali dall'inizializzatore */
ConfORARIO config_orario;
int        flag_config_orario;
/*                                        */

#define INITERR "<html><h1>Ricerca sull'orario ufficiale delle Ferrovie dello Stato S.p.A.</h1><p><h2>Spiacenti, abbiamo un problema nell'applicazione</h2><p><h2>Sorry, we have some problem in our application</h2></html>"


void PutHtmlPage(char *, unsigned char *, long *);
char *GetQueryString(unsigned char *, long *);
int ReadConfig_Orario(ConfORARIO *);
void FreeConfig_Orario(ConfORARIO);
char *Search_Orario(char *, ConfORARIO);
char *Get_HTTPD_Variable(char *, unsigned char *, long *);
void Log_HTTPD_error(unsigned char *, char * );
void Write_Q5_Log(char *);

void HTTPD_LINKAGE AsFsQ5(unsigned char *handle, long *return_code )
{
   char *qs;
   char *page;

// Write_Q5_Log("ciao");

// Se l'inizializzazione e' andata a vuoto manda pagina di errore
   if (flag_config_orario != 0) {
     PutHtmlPage(INITERR, handle, return_code);
     return;
   }

// Estrazione stringa di query
// Write_Q5_Log("GQS");
   qs = GetQueryString(handle, return_code);

   if (0 != strlen(qs)) {
// Ricerca e preparazione della pagina di output
// Write_Q5_Log("SO");
      page = Search_Orario(qs, config_orario);
   }
   else {
      free(qs);
      page = (char *)malloc(14);
      strcpy(page,"<html></html>");
   }

// Invio della pagina
// Write_Q5_Log("PHP");
   PutHtmlPage(page, handle, return_code);
   free(page);

   return;

}


void HTTPD_LINKAGE AsFsI5(unsigned char *handle, long *return_code )
{
  flag_config_orario = ReadConfig_Orario(&config_orario);
  if (flag_config_orario == 0) {
    Log_HTTPD_error(handle, "ASFSQ8 Initialization succeeded");
//    Write_Q5_Log("ASFSQ6 Initialization succeeded");
  }
  else {
     // EMS
     char cMsg[100];
     sprintf(cMsg, "ASFSQ8 Initialization failed flag_config_orario=%d", flag_config_orario);
     Log_HTTPD_error(handle, cMsg);
//    Write_Q5_Log("ASFSQ6 Initialization failed");
  }

   *return_code = HTTP_NOACTION;
   return;
}

void HTTPD_LINKAGE AsFsT5(unsigned char *handle, long *return_code )
{
  int ret;

  *return_code = HTTP_NOACTION;
  if (flag_config_orario != 0) return;
  FreeConfig_Orario(config_orario);
  return;
}

void Write_Q5_Log(char *msg)
{
  char *fileid = "\\www\\logs\\asfsq8.log";
  FILE *logfile;
  logfile = fopen(fileid,"a");
  fprintf(logfile,"%s\n",msg);
  fclose(logfile);
}
