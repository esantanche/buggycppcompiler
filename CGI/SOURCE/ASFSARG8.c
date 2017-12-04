#include "dummy.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//EMS ?C?C
//#include "winbase.h"    //Windows
#include "windows.h"    //Windows

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

//EMS ?C?C?C #include <os2.h>
//EMS ?C?C
typedef unsigned long  APIRET;
typedef unsigned long  LHANDLE;
#define PBOOL32 PBOOL
typedef PVOID *PPVOID;
typedef LHANDLE PID;            /* pid  */
//EMS ?C?C

extern "C" {
#include <bsedos.h>
}
#include <time.h>
#include "HTAPI.h"
#include "ascgiarg.h"
#include "asfsdat8.h"
#include "asconfig.h"
#include "asfsutil.h"
typedef const char * CPSZ;   ///?C?C?C? EMS
#include "trc2.h"

int Check_Input(char *, char *,
                char *, char *, char *,
                char *, char *, char *, char *, char *, char *,
                ConfORARIO,
                char **, char **,
                int *, int *, int *,
                int *, int *, int *, int *, int *, int *,
                ERRORI *);
char *stripblank(char *);
short CheckTime(char *, char *, int *, int *);
short CheckDate(char *, char *, char *, int *, int *, int *);
int IsValidDate(char *, char *, char *, int, int *, int *, int *);
int IsEmpty(char *);
int IsValidNum(char *);
void Write_Q5_Log(char *);


RISPOSTA Engine_Request(char **Request, char *QueryString,
                        ConfORARIO config)
{

   #undef TRCRTN
   #define TRCRTN "Engine_Request"

//   char *qs;
   char *cl;
   int lc;
   int i;
   CGIPARMS cgip;
   RISPOSTA rsp;
   char *r;
   char *vstp, *vsta;
   int vdtg, vdtm, vdta, vorh, vorm, vdet, vlng, vlip, vsrt;
   char bufint[20];
   char *det;
   ERRORI errors;
   int ret;
   int extr;
   unsigned long name_len, value_len;
   long     rc;
   char qm[20];
   char cData_Ora[20];   // EMS
   char * cToken; //EMS

   TRACESTRING(QueryString);

   *Request = (char *)NULL;

// Write_Q5_Log("Enter in ER");
   rsp.retcode = 0;
   rsp.Dati.errori.motore = 0;
   rsp.Dati.errori.richiesta = 0;
   rsp.Dati.errori.stazione_p = 0;
   rsp.Dati.errori.stazione_a = 0;
   rsp.Dati.errori.data = 0;
   rsp.Dati.errori.ora = 0;
   rsp.Dati.errori.soluzioni = 0;
   (rsp.indata).stazione_p = (char *)NULL;
   (rsp.indata).stazione_a = (char *)NULL;
   (rsp.indata).giorno     = (char *)NULL;
   (rsp.indata).mese       = (char *)NULL;
   (rsp.indata).anno       = (char *)NULL;
   (rsp.indata).ora        = (char *)NULL;
   (rsp.indata).minuto     = (char *)NULL;
   (rsp.indata).dettaglio  = (char *)NULL;
   (rsp.indata).lingua     = (char *)NULL;
   (rsp.indata).indpage    = (char *)NULL;
   (rsp.indata).sort       = (char *)NULL;

// Write_Q5_Log("Parsing parms");
// Write_Q5_Log(QueryString);

   cgip = Parse_Args(QueryString);
//   free(qs);

// Write_Q5_Log("Setting parms");
   extr = 0;
   for (i=0; i<cgip.nparms; i++) {

           if ( 0 == strcmpi( ((cgip.parms)+i)->name, "stazin" ) ) {
               // && (rsp.indata).stazione_p == NULL) {            // EMS
              (rsp.indata).stazione_p = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).stazione_p,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "stazout") ) {
              //  && (rsp.indata).stazione_a == NULL) {
              (rsp.indata).stazione_a = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).stazione_a,((cgip.parms)+i)->value);
           }
      // begin EMS Inserisco il processing delle keyword date e time che contengono data e ora
      // in formato gg.mm.aaaa (oppure mm.dd.yyyy) e hh:mm)
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "date" ) ) {
              strcpy(cData_Ora, ((cgip.parms)+i)->value );
              cToken = strtok(cData_Ora, ".");
              if (cToken) {
                 (rsp.indata).giorno     = (char *)malloc(1+max(strlen(cToken),4));
                 strcpy((rsp.indata).giorno, cToken);
                 cToken = strtok(NULL, ".");
                 if (cToken) {
                    (rsp.indata).mese       = (char *)malloc(1+max(strlen(cToken),4));
                    strcpy((rsp.indata).mese, cToken);
                    cToken = strtok(NULL, ".");
                    if (cToken) {
                       (rsp.indata).anno       = (char *)malloc(1+strlen(cToken));
                       strcpy((rsp.indata).anno, cToken);
                    }
                 }
              }
          }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "time" ) ) {
              strcpy(cData_Ora, ((cgip.parms)+i)->value );
              cToken = strtok(cData_Ora, ":");
              if (cToken) {
                 (rsp.indata).ora       = (char *)malloc(1+strlen(cToken));
                 strcpy((rsp.indata).ora, cToken);
                 cToken = strtok(NULL, ":");
                 if (cToken) {
                    (rsp.indata).minuto     = (char *)malloc(1+strlen(cToken));
                    strcpy((rsp.indata).minuto, cToken);
                 }
              }
           }
      // end EMS
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "datag"  ) ) {
              (rsp.indata).giorno     = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).giorno    ,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "datam"  ) ) {
              (rsp.indata).mese       = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).mese      ,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "dataa"  ) ) {
              (rsp.indata).anno       = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).anno      ,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "timsh"  ) ) {
              (rsp.indata).ora        = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).ora       ,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "timsm"  ) ) {
              (rsp.indata).minuto     = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).minuto    ,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "det"    ) ) {
              (rsp.indata).dettaglio  = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).dettaglio ,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "lang"   ) ) {
              (rsp.indata).lingua     = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).lingua    ,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "page"   ) ) {
              (rsp.indata).indpage    = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).indpage   ,((cgip.parms)+i)->value);
           }
      else if ( 0 == strcmpi( ((cgip.parms)+i)->name, "sort"   ) ) {
              (rsp.indata).sort       = (char *)malloc(1+strlen(((cgip.parms)+i)->value));
              strcpy((rsp.indata).sort      ,((cgip.parms)+i)->value);
           }
      else extr = 1;
   }
   Free_Args(cgip);

   // begin EMS
   // ?C?C invertire giorno e mese se la lingua è inglese
   if (strcmp((rsp.indata).lingua, "en") == 0) {
      // qui cData_Ora è usata solo come appoggio
      strcpy(cData_Ora, (rsp.indata).giorno);
      strcpy((rsp.indata).giorno, (rsp.indata).mese);
      strcpy((rsp.indata).mese, cData_Ora);
   }
   // end EMS

   if ( 0 != extr )
   {
      rsp.retcode = 2;
      rsp.Dati.errori.richiesta = 1;
      return rsp;
   }

   if ( (NULL == (rsp.indata).stazione_p) ||
        (NULL == (rsp.indata).stazione_a) ||
        (NULL == (rsp.indata).giorno    ) ||
        (NULL == (rsp.indata).mese      ) ||
        (NULL == (rsp.indata).anno      ) ||
        (NULL == (rsp.indata).ora       ) ||
        (NULL == (rsp.indata).minuto    ) )
   {
      rsp.retcode = 2;
      rsp.Dati.errori.richiesta = 1;
      return rsp;
   }

// Write_Q5_Log("Checking input");
   ret = Check_Input( (rsp.indata).stazione_p,
                      (rsp.indata).stazione_a,
                      (rsp.indata).giorno    ,
                      (rsp.indata).mese      ,
                      (rsp.indata).anno      ,
                      (rsp.indata).ora       ,
                      (rsp.indata).minuto    ,
                      (rsp.indata).dettaglio ,
                      (rsp.indata).lingua    ,
                      (rsp.indata).indpage   ,
                      (rsp.indata).sort      ,
                      config,
                      &vstp,&vsta,
                      &vdtg, &vdtm, &vdta,
                      &vorh, &vorm, &vdet, &vlng, &vlip, &vsrt,
                      &errors);

   if (ret != 0) {
      rsp.retcode = 2;
      rsp.Dati.errori.motore     = errors.motore    ;
      rsp.Dati.errori.richiesta  = errors.richiesta ;
      rsp.Dati.errori.stazione_p = errors.stazione_p;
      rsp.Dati.errori.stazione_a = errors.stazione_a;
      rsp.Dati.errori.data       = errors.data      ;
      rsp.Dati.errori.ora        = errors.ora       ;
      rsp.Dati.errori.soluzioni  = errors.soluzioni ;
      return rsp;
   }


// Write_Q5_Log("Preparing string");
   r = (char *)malloc(3+strlen(vstp)+strlen(vsta));
   strcpy(r,vstp);
   strcat(r,"\001");

   strcat(r,vsta);
   strcat(r,"\001");

   sprintf(bufint,"%d",vdtg);
   r = (char *)realloc(r,2+strlen(r)+strlen(bufint));
   strcat(r,bufint);
   strcat(r,"\001");

   sprintf(bufint,"%d",vdtm);
   r = (char *)realloc(r,2+strlen(r)+strlen(bufint));
   strcat(r,bufint);
   strcat(r,"\001");

   sprintf(bufint,"%d",vdta);
   r = (char *)realloc(r,2+strlen(r)+strlen(bufint));
   strcat(r,bufint);
   strcat(r,"\001");

   sprintf(bufint,"%d",vorh);
   r = (char *)realloc(r,2+strlen(r)+strlen(bufint));
   strcat(r,bufint);
   strcat(r,"\001");

   sprintf(bufint,"%d",vorm);
   r = (char *)realloc(r,2+strlen(r)+strlen(bufint));
   strcat(r,bufint);
   strcat(r,"\001");

   sprintf(bufint,"%d",vdet);
   r = (char *)realloc(r,2+strlen(r)+strlen(bufint));
   strcat(r,bufint);
   strcat(r,"\001");

   sprintf(bufint,"%d",100000);
   r = (char *)realloc(r,2+strlen(r)+strlen(bufint));
   strcat(r,bufint);
   strcat(r,"\001");

   sprintf(bufint,"%d",vsrt);
   r = (char *)realloc(r,2+strlen(r)+strlen(bufint));
   strcat(r,bufint);
   strcat(r,"\001");

// Write_Q5_Log("Exiting");
   *Request = r;

   return rsp;
}


int Check_Input(char *stazione_p, char *stazione_a,
                char *data_g, char *data_m, char *data_a,
                char *ora_h, char *ora_m, char *det, char *lingua,
                char *page, char *sort,
                ConfORARIO config,
                char **vstazione_p, char **vstazione_a,
                int *vdata_g, int *vdata_m, int *vdata_a,
                int *vora_h, int *vora_m, int *vdet, int *vlng,
                int *vlip, int *vsrt,
                ERRORI *err)
{
   ERRORI r;
   int ret;
   int rc;
   int i;
   PAIRLIST languages;
   int n_lang;

   ret = 0;

   r.motore = 0;
   r.richiesta = 0;
   r.stazione_p = 0;
   r.stazione_a = 0;
   r.data = 0;
   r.ora = 0;
   r.soluzioni = 0;

   // begin EMS
   // Se effettuo lo stripblank, un nome esatto di stazione non viene identificato
   // per cui non si riescono a risolvere casi di ambiguità del nome stazione

   //*vstazione_p = stripblank(stazione_p);
   //*vstazione_a = stripblank(stazione_a);

   *vstazione_p = stazione_p;
   *vstazione_a = stazione_a;

   // end EMS

   /*
   TRCbegin "
int Check_Input(char *stazione_p, char *stazione_a,
                char *data_g, char *data_m, char *data_a,
                char *ora_h, char *ora_m, char *det, char *lingua,
                char *page, char *sort,
   */

   if (strlen(*vstazione_p) < 2) {
      ret = 2;
      r.stazione_p = 3;
   }
   if (strlen(*vstazione_a) < 2) {
      ret = 2;
      r.stazione_a = 3;
   }
   if ( (0 == r.stazione_p) &&
        (0 == r.stazione_a) &&
        (0 == strcmpi(*vstazione_p,*vstazione_a)) ) {
      ret = 2;
      r.stazione_p = 4;
   }

   data_m = stripblank(data_m);
   data_g = stripblank(data_g);
   data_a = stripblank(data_a);
   rc = CheckDate(data_g, data_m, data_a, vdata_g, vdata_m, vdata_a);
   if (1 == rc)
   {
      ret = 2;
      r.data = 2;
   }

   ora_h = stripblank(ora_h);
   ora_m = stripblank(ora_m);
   rc = CheckTime(ora_h, ora_m, vora_h, vora_m);
   if (1 == rc)
   {
      ret = 2;
      r.ora = 1;
   }

   if (NULL != det) {
      det = stripblank(det);
      if (0 == strlen(det)) {
         *vdet = 0;
      }
      else {
         rc = IsValidNum(det);
         if (1 != rc) {
            ret = 2;
            r.richiesta = 2;
         }
         *vdet = atoi(det);
         if (*vdet < 0) {
            ret = 2;
            r.richiesta = 2;
         }
      }
   }
   else *vdet = 0;

   if (NULL != lingua) {
      languages = config.languages;
      n_lang = Get_PairList_Count(languages);

      lingua = stripblank(lingua);
      if (0 == strlen(lingua)) {
         *vlng = 0;
      }
      else {
         *vlng = -1;
         for (i=0; i<n_lang; i++) {
            if (0 == strcmpi(lingua, Get_PairList_Name_by_Index(languages,i))) {
               *vlng = i;
               break;
            }
         }
         if (-1 == *vlng) {
            ret = 2;
            r.richiesta = 2;
         }
      }
   }
   else *vlng = 0;

   if (NULL != page) {
      page = stripblank(page);
      if (0 == strlen(page)) {
         *vlip = 1;
      }
      else {
         rc = IsValidNum(page);
         if (1 != rc) {
            ret = 2;
            r.richiesta = 3;
         }
         *vlip = atoi(page);
         if (*vlip <= 0) {
            ret = 2;
            r.richiesta = 3;
         }
      }
   }
   else *vlip = 1;

   if (NULL != sort) {
      sort = stripblank(sort);
      if (0 == strlen(sort)) {
         *vsrt = 0;
      }
      else {
         rc = IsValidNum(sort);
         if (1 != rc) {
            ret = 2;
            r.richiesta = 4;
         }
         *vsrt = atoi(sort);

         if (*vsrt < 0 || *vsrt >2) {  /*mod 28/02/97 RS*/
            ret = 2;
            r.richiesta = 4;
         }
      }
   }
   else *vsrt = 0;

   *err = r;

   return ret;

}

/*****************************************************************************/
/* return pointer to first non blank character                              */
/* original string is modified to close it after the last non blank char.   */

char *stripblank(char *string)
{
  char *p, *q, *f, *l;
  int ls;

  ls = strlen(string);

  if (1 == ls) {
     if (' ' == *string) *string = '\0';
     return string;
  }

  f = string + ls;
  for (p=string; p<p+ls; p++) {
      if (' ' != *p) {
         f = p;
         break;
     }
  }

  if (f == string + ls) {
     *string = '\0';
     return string;
  }

  l = string;
  for (p = string + ls - 1; p >= string; p--) {
     if (' ' != *p) {
        l = p;
        break;
     }
  }
  *(l+1) = '\0';

  return f;
}
/*****************************************************************************/
short CheckTime(char *shour, char *smin, int *hour, int *min)
{
   int ehour, emin;
   DATETIME dt;
   APIRET rc;

   ehour = IsEmpty(shour);
   emin = IsEmpty(smin);

   if (0 == ehour) {
      if (0 == IsValidNum(shour)) return 1;
      *hour = atoi(shour);
      if (*hour < 0 || *hour > 23) return 1;
   }
   if (0 == emin) {
      if (0 == IsValidNum(smin)) return 1;
      *min = atoi(smin);
      if (*min < 0 || *min > 59) return 1;
   }

   if (0 == ehour && 0 == emin)                 /* entrambi riempiti  */
   {
      return 0;
   }
   else if (0 == ehour && 0 != emin)            /* riempito solo ora   */
   {
      *min = 0;
      return 0;
   }
   else if (0 != ehour && 0 == emin)            /* riempito solo min   */
   {
      return 1;
   }
   else                                         /* entrambi vuoti      */
   {
      *hour = 6;
      *min  = 0;
      return 0;
   }

}


/*****************************************************************************/

short CheckDate(char *sday, char *smonth, char *syear,
                int *day, int *month, int *year)
{
   int ivd;
   DATETIME dt;
   APIRET rc;


   ivd = IsValidDate(sday,smonth,syear,4,day,month,year);
   if (*year < 100) *year += 1900;

   if (-1 == ivd) return 1;

   else if (1 == ivd) return 0;

   else {
      rc = DosGetDateTime(&dt);
      *day = dt.day;
      *month = dt.month;
      *year = dt.year;
      return 0;
   }

}



/*****************************************************************************/

int IsValidNum(char *string)
{
  int ls;
  char *p;

  for (p=string; p<string+strlen(string); p++) if (*p < '0' || *p > '9') return 0;
  return 1;
}

/*****************************************************************************/
/*   return=0 filled   return=1 empty                                        */

int IsEmpty(char *string)
{
  if (NULL == string) return 1;
  if (0 == strlen(string)) return 1;
  if (strspn(string," ") == strlen(string)) return 1;
  return 0;
}

/*****************************************************************************/
/*   filling<2   [[d]m]y      return=-1 error
     filling=2    [d]m y      return=0  empty
     filling>2     d m y      return=1  O.K.
*/

int IsValidDate(char *sday, char *smonth, char *syear, int filling,
                int *day, int *month, int *year)
{
  int lmonth[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int eday, emonth, eyear;

  eday = IsEmpty(sday);
  emonth = IsEmpty(smonth);
  eyear = IsEmpty(syear);

  if (eday == 1 && emonth == 1 && eyear == 1) return 0;

  if (0 == eday) {
     if (0 == IsValidNum(sday)) return -1;
  }
  if (0 == emonth) {
     if (0 == IsValidNum(smonth)) return -1;
  }
  if (0 == eyear) {
     if (0 == IsValidNum(syear)) return -1;
  }

  if (filling < 2) {
    if (eday == 0 && (emonth == 1 || eyear == 1)) return -1;
    if (emonth == 0 && eyear == 1) return -1;
  }
  else if (filling == 2) {
    if (emonth == 1 || eyear == 1) return -1;
  }
  else {
    if (eday == 1 || emonth == 1 || eyear == 1) return -1;
  }

  *year = atoi(syear);
  if (emonth == 0) {
    *month = atoi(smonth);
    if (*month < 1 || *month > 12) return -1;
  }
  else *month = 0;

  if (eday == 0) {
    *day = atoi(sday);
    if (0 != *year % 4) lmonth[1] = 28;
    else lmonth[1] = 29;
    if (*day < 1 || *day > lmonth[*month-1]) return -1;
  }
  else *day = 0;

  return 1;
}
