#include "dummy.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// begin EMS
// EMS ?C?C #include <os2.h>
#include <windows.h>
// end EMS

#include "HTAPI.h"

void Write_Httpd( unsigned char *, char *, long *, char * );
void Log_HTTPD_error(unsigned char *, char * );
void Write_Q5_Log(char *);

char *GetQueryString(unsigned char *handle, long *return_code)
{
   char bufint[20];
   unsigned long lc, lr, lt;
   int i;
   long     rc;
   char *qs, *q;
   unsigned long name_len, value_len;
   char qm[20];
   char *QueryString;


   name_len  = strlen("REQUEST_METHOD");
   value_len = 19;
   HTTPD_extract (handle, (UCHAR *)"REQUEST_METHOD", &name_len, (UCHAR *)qm, &value_len, &rc);
   *(qm+value_len) = '\0';

   QueryString = (char *)NULL;

   if (0 == strcmp(qm,"GET")) {
      name_len  = strlen("QUERY_STRING");
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
      *(qs+value_len) = '\0';
   }
   else {
/* **** Inizio parte di codice che funziona solo con ICS 4.2 *******
      name_len  = strlen("CONTENT_LENGTH");
      value_len = 19;
      HTTPD_extract (handle, (UCHAR *)"CONTENT_LENGTH", &name_len, (UCHAR *)bufint, &value_len, &rc);
      lc = atoi(bufint);
      qs = (char *)malloc(lc+1);
      lr = lc + 1;
      HTTPD_read(handle, (UCHAR *)qs, &lr, &rc);
      qs[lc] = '\0';
**** Fine parte di codice che funziona solo con ICS 4.2 ******* */
/* **** Inizio parte di codice che funziona sia con ICS 4.1 sia con 4.2 ***** */
      qs = (char *)malloc(1);
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
      qs = (char *)realloc(qs,lt+1);
/* **** Fine parte di codice che funziona sia con ICS 4.1 sia con 4.2 ***** */
   }
   QueryString = (char *)malloc(strlen(qs) + 1);
   strcpy(QueryString,qs);
   free(qs);

   return QueryString;

}

void PutHtmlPage(char *page, unsigned char *handle, long *return_code)
{
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

OutExit:

   return;
}

void Write_Httpd( unsigned char *handle,
                  char *string,
                  long *return_code, char *msg) {

   unsigned long string_length;
   long     rc;
   char     error_msg[5000];

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
}

void Log_HTTPD_error( unsigned char *handle, char *msg ) {

   unsigned long msg_length;
   long     rc;

   msg_length = strlen(msg);
   HTTPD_log_error (handle, (UCHAR *)msg, &msg_length, &rc);
   (void)fprintf   (stderr, msg);
   fflush(stderr);
}

char *Get_HTTPD_Variable(char *var_name, unsigned char *handle, long *return_code)
{
   long     rc;
   char *qs;
   unsigned long name_len, value_len;

   name_len  = strlen(var_name);
   value_len = 1023;
   qs = (char *)malloc(1023);
   HTTPD_extract (handle, (UCHAR *)var_name, &name_len,
                  (UCHAR *)qs, &value_len, &rc);
   if (HTTPD_BUFFER_TOO_SMALL == rc)
   {
      qs = (char *)realloc(qs,value_len+1);
      HTTPD_extract (handle, (UCHAR *)var_name, &name_len,
                     (UCHAR *)qs, &value_len, &rc);
   }
   *(qs+value_len) = '\0';
   return qs;
}
