//
//   Project: SIPAX
//   File: HTAPI.C
//   Author: Ing. Emanuele Maria Santanché
//   Description: riscrittura delle API dell'interfacciamento
//   Note:

#include "dummy.h"

#include <windows.h>
#include "htapi.h"
#include <stdio.h>
#define CPSZ const char *
#include "trc2.h"

// EMS Proveniente da bsedos.h
typedef unsigned long APIRET;
APIRET APIENTRY DosSleep(ULONG msec);

// Variabili dichiarate in cgi.c
extern char cNome_file_input[MAX_PATH];
extern char cNome_file_output[MAX_PATH];
extern char cQuery_string_da_command_line[512];

long Leggi_query_string(unsigned char *value,        /* o; buffer in which to place the value */
                                            unsigned long *value_length); /* i/o; size of buffer/length of value */
BOOL Converti_escape_sequences(unsigned char *value);

#undef TRCRTN
#define TRCRTN "HTAPI.C-UNDEF"

////////////////////////////////////////////////////////////////////////
//                  HTTPD_extract
////////////////////////////////////////////////////////////////////////

/* extract the value of a variable associated with this request */
/* the available variables are the same as CGI                  */
/* valid in all steps, though not all variables will be         */

void
HTTPD_LINKAGE
HTTPD_extract(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *name,         /* i; name of the value to extract */
    unsigned long *name_length,  /* i; length of the name */
    unsigned char *value,        /* o; buffer in which to place the value */
    unsigned long *value_length, /* i/o; size of buffer/length of value */
             long *return_code)  /* o; return code */
{
   /*
   char qm[20];
   name_len  = strlen("REQUEST_METHOD");
   value_len = 19;
   HTTPD_extract (handle, (UCHAR *)"REQUEST_METHOD",
               &name_len, (UCHAR *)qm, &value_len, &rc);
   *(qm+value_len) = '\0';

   value_len = 4095;
   qs = (char *)malloc(4096);
   HTTPD_extract (handle, (UCHAR *)"QUERY_STRING", &name_len,
                                         (UCHAR *)qs, &value_len, &rc);
   if (HTTPD_BUFFER_TOO_SMALL == rc)
      {
         qs = (char *)realloc(qs,value_len+1);
         HTTPD_extract (handle, (UCHAR *)"QUERY_STRING", &name_len,
                                            (UCHAR *)qs, &value_len, &rc);
      }
   */
   //?C

   // Deve rispondere solo alla "REQUEST_METHOD" con un "GET"
   // e poi alla "QUERY_STRING" con la query string

   if (name == NULL || value == NULL || value_length == NULL ||
       return_code == NULL) {
      if (return_code != NULL)
         *return_code = HTTPD_PARAMETER_ERROR;
      return;
   }

   if (strcmp((const char *)name, "REQUEST_METHOD") == 0) {
      if (*value_length < 4) {
         *value_length = 4;
         *return_code = HTTPD_BUFFER_TOO_SMALL;
         return;
      }
      strcpy((char *)value, "GET");
      *value_length = 4;
   }
   else
   if (strcmp((const char *)name, "QUERY_STRING") == 0) {
      //unsigned char *value,        /* o; buffer in which to place the value */
      //unsigned long *value_length, /* i/o; size of buffer/length of value */
      *return_code = Leggi_query_string(value,
                                                            value_length);
      return;
   }
   else {
      *return_code = HTTPD_PARAMETER_ERROR;
      return;
   }

   //?C impostare return_code
   // *return_code = ???
   *return_code = HTTPD_SUCCESS;

   return;
}


////////////////////////////////////////////////////////////////////////
//                     Leggi_query_string
////////////////////////////////////////////////////////////////////////

long Leggi_query_string(unsigned char *value,        /* o; buffer in which to place the value */
                                            unsigned long *value_length) /* i/o; size of buffer/length of value */
{
   #undef TRCRTN
   #define TRCRTN "Leggi_query_string"

   FILE * FQuery_string;
   int i;
   char cRiga[80];
   char cQuery_string[4096];

   TRACESTRING("Entro");

   if (*cQuery_string_da_command_line != '\0') {
      if (*value_length < strlen(cQuery_string_da_command_line)+1) {
         *value_length = strlen(cQuery_string_da_command_line)+1;
         return HTTPD_BUFFER_TOO_SMALL;
      }
      // Iview fornisce la query string (nel caso in cui la CGI sia
      // chiamata da link (URL) invece che da form) come parametro sulla command line
      // però dimentica di togliere il punto interrogativo iniziale
      if (cQuery_string_da_command_line[0] == '?')
         strcpy((char *)value, cQuery_string_da_command_line+1);
      else
         strcpy((char *)value, cQuery_string_da_command_line);
      if (Converti_escape_sequences(value))
         return HTTPD_SUCCESS;
      else
         return HTTPD_PARAMETER_ERROR;
   }

   cQuery_string[0] = '\0';
   FQuery_string = fopen(cNome_file_input, "r");
   fgets(cQuery_string, 4096, FQuery_string);

   TRACESTRING(cQuery_string);
   /*
   do {

      if (strncmp(cRiga, "TAPPO", 5) == 0) break;
      strcat(cQuery_string, cRiga);
      cQuery_string[strlen(cQuery_string)-1] = '&';
   } while (TRUE);
   */

   //cQuery_string[strlen(cQuery_string)-1] = '\0';

   fclose(FQuery_string);

   if (*value_length < strlen(cQuery_string)+1) {
      *value_length = strlen(cQuery_string)+1;
      return HTTPD_BUFFER_TOO_SMALL;
   }

   strcpy((char *)value, cQuery_string);

   if (!Converti_escape_sequences(value))
      return HTTPD_PARAMETER_ERROR;

   // EMS 7.11.97 Modifico al fine di accettare sia data e ora date con 3 e 2 campi rispettivamente,
   // sia date come due soli campi ("gg/mm/aaaa" e "hh:mm")

   return HTTPD_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
//                 Converti_escape_sequences
////////////////////////////////////////////////////////////////////////

BOOL Converti_escape_sequences(unsigned char *value)
{
   char * pcCursore_in_lettura;
   char * pcCursore_in_scrittura;
   BOOL bErrore_in_stringa = FALSE;
   char cHigh;
   char cLow;
   int iHigh;
   int iLow;

   for (pcCursore_in_lettura = pcCursore_in_scrittura = (char *) value;
        *pcCursore_in_lettura;
        pcCursore_in_lettura++, pcCursore_in_scrittura++) {
      if (*pcCursore_in_lettura == '+')
         *pcCursore_in_scrittura = ' ';
      else
      if (*pcCursore_in_lettura == '%') {
         if (*(pcCursore_in_lettura+1) == '\0') {
            bErrore_in_stringa = TRUE;
            break;
         }
         if (*(pcCursore_in_lettura+2) == '\0') {
            bErrore_in_stringa = TRUE;
            break;
         }
         if (isxdigit(*(pcCursore_in_lettura+1)) &&
             isxdigit(*(pcCursore_in_lettura+2))) {
            cHigh = toupper(*(pcCursore_in_lettura+1));
            cLow  = toupper(*(pcCursore_in_lettura+2));
            iHigh = (isdigit(cHigh) ? cHigh - '0' : cHigh - 'A' + 10);
            iLow  = (isdigit(cLow)  ? cLow  - '0' : cLow  - 'A' + 10);
            *pcCursore_in_scrittura = iHigh*16+iLow;
            pcCursore_in_lettura += 2;
         }
         else {
            bErrore_in_stringa = TRUE;
            break;
         }
      }
      else
         *pcCursore_in_scrittura = *pcCursore_in_lettura;
   }; // endfor

   *pcCursore_in_scrittura = '\0';

   if (bErrore_in_stringa)
      return FALSE;
   else
      return TRUE;
}

////////////////////////////////////////////////////////////////////////
//                 HTTPD_read
////////////////////////////////////////////////////////////////////////

/* read the body of the client's request - use set/extract for headers */
/* Keep reading until HTTPD_EOF is returned; 4k is a good buffer size  */
/* valid only in the PreFilter and Service steps                       */

void
HTTPD_LINKAGE
HTTPD_read(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *value,        /* i; buffer in which to place the value */
    unsigned long *value_length, /* i/o; size of buffer/length of header */
             long *return_code)  /* o; return code */
{
   /*
   lt = 0;
   for (;;) {
      lr = 512;
      qs = (char *)realloc(qs,1+lr+lt);
      q = qs + lt;
      HTTPD_read(handle, (UCHAR *)q, &lr, &rc);
      q[lr] = '\0';
      lt = strlen(qs);
      if (HTTPD_EOF == rc) break;
   }
   */

   // In realtà questa funzione non dovrebbe servire in quanto
   // la query string non arriva mai a superare i 4 Kb

   if (return_code != NULL)
      *return_code = HTTPD_UNSUPPORTED;

   return;
}

////////////////////////////////////////////////////////////////////////
//                 HTTPD_set
////////////////////////////////////////////////////////////////////////

/* set the value of a variable associated with this request     */
/* the available variables are the same as CGI                  */
/* - note that one can create variables with this function      */
/* - if created variables are prefixed by "HTTP_", they will    */
/*   be sent as headers in the response, without the "HTTP_"    */
/*   prefix                                                     */
/* valid in all steps, though not all variables are             */
void
HTTPD_LINKAGE
HTTPD_set(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *name,         /* i; name of the value to set */
    unsigned long *name_length,  /* i; length of the name */
    unsigned char *value,        /* o; buffer which contains the value */
    unsigned long *value_length, /* i; length of value */
             long *return_code)  /* o; return code */
{
   #undef TRCRTN
   #define TRCRTN "HTTPD_set"

   /*
   long     rc;
   char          *content_type       = "CONTENT_TYPE";
   char          *content_type_value = "text/html";
   unsigned long content_type_length;
   unsigned long content_type_value_length;

   content_type_length       = strlen(content_type);
   content_type_value_length = strlen(content_type_value);
   HTTPD_set(handle, (UCHAR *)content_type, &content_type_length,
             (UCHAR *)content_type_value, &content_type_value_length,
             return_code);
   if (HTTPD_SUCCESS == *return_code) {
     Write_Httpd( handle, page, return_code,"");
   }
   */
   //?C

   // Questa API viene usata una sola volta per comunicare che
   // i dati che verranno consegnati in risposta alla query consistono
   // in una pagina html
   // Non c'è niente quindi da fare.

   if (name == NULL || name_length == NULL || value == NULL ||
       value_length == NULL || return_code == NULL) {
      TRACESTRING("Parametri non validi (uno o più puntatori nulli)");
      if (return_code) *return_code = HTTPD_PARAMETER_ERROR;
      return;
   }

   if (*name_length < 12 || *value_length < 9) {
      TRACESTRING("Parametri non validi (lunghezze dati errate)");
      *return_code = HTTPD_PARAMETER_ERROR;
      return;
   }

   if (strcmp((const char *)name, "CONTENT_TYPE") != 0 ||
       strcmp((const char *)value, "text/html") != 0) {
      TRACESTRING("Parametri non validi (valore dati errato)");
      *return_code = HTTPD_PARAMETER_ERROR;
      return;
   }

   *return_code = HTTPD_SUCCESS;

   return;
}


////////////////////////////////////////////////////////////////////////
//                 HTTPD_write
////////////////////////////////////////////////////////////////////////

/* write the body of the response - use set/extract for headers */
/* valid only in the Sservice and PostFilter steps              */
void
HTTPD_LINKAGE
HTTPD_write(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *value,        /* i; data to send */
    unsigned long *value_length, /* i; length of the data */
             long *return_code)  /* o; return code */
{
   #undef TRCRTN
   #define TRCRTN "HTTPD_write"

   FILE * Foutput;
   MSG msgMessaggio_coda;

   /*
   void Write_Httpd( unsigned char *handle,
                  char *string,
                  long *return_code, char *msg) {

   unsigned long string_length;
   long     rc;
   char     error_msg[1000];

   *return_code = HTTP_OK;
   string_length = strlen(string);
   HTTPD_write(handle, (UCHAR *)string, &string_length, &rc);
   if (HTTPD_SUCCESS != rc) {
      *return_code = HTTP_SERVER_ERROR;
      sprintf (error_msg,
                                         "Write_Httpd: HTTPD_write failed (%ld) for %s\n, %ld, %ld\n, msg=%s",
                                         rc, string, string_length, *return_code, msg    );
      Log_HTTPD_error( handle, error_msg);
   }
   */

   //?C
   if (value == NULL || value_length == NULL || return_code == NULL) {
      TRACESTRING("Parametri non validi (uno o più puntatori nulli)");
      if (return_code) *return_code = HTTPD_PARAMETER_ERROR;
      return;
   }

   for (int i=0;i<10;i++) {

      Foutput = fopen(cNome_file_output,"w");
      if (Foutput != NULL) break;
      DosSleep(500);

      if (PeekMessage(&msgMessaggio_coda, NULL, 0, 0, PM_REMOVE)) {
         TranslateMessage(&msgMessaggio_coda);
         DispatchMessage(&msgMessaggio_coda);
      }

   }

   if (Foutput == NULL) {
      TRACESTRING("Impossibile aprire file di output");
      *return_code = HTTPD_INTERNAL_ERROR;
      return;
   }
   fprintf(Foutput, "%s", value);
   fclose(Foutput);

   *return_code = HTTPD_SUCCESS;

   return;
}



////////////////////////////////////////////////////////////////////////
//               HTTPD_log_error
////////////////////////////////////////////////////////////////////////

/* write a string to the server's error log */
void
HTTPD_LINKAGE
HTTPD_log_error(
    unsigned char *handle,       /* i; handle (NULL right now) */
    unsigned char *value,        /* i; data to write */
    unsigned long *value_length, /* i; length of the data */
             long *return_code)  /* o; return code */
{
   #undef TRCRTN
   #define TRCRTN "HTTPD_log_error"

   if (value != NULL)
      TRACESTRING(value);

   if (return_code) *return_code = HTTP_OK;

   return;
}



