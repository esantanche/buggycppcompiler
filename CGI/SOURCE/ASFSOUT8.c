//EMS ?C?C #include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asfsdat8.h"
#include "asconfig.h"
#include "asfsutil.h"

char *Output_and_Free(RISPOSTA, char *, char *, ConfORARIO);
char *Build_Page(RISPOSTA, char *, ConfORARIO);
char *Build_Soluzioni(RISPOSTA, char *, ConfORARIO);
char *Build_Dettagli(RISPOSTA, char *, ConfORARIO);
char *Build_Errori(RISPOSTA, char *, ConfORARIO);
void Write_Httpd( unsigned char *, char *, long *, char * );
void Log_HTTPD_error(unsigned char *, char * );
void prtpairlist(char *,PAIRLIST,char *);
int ErrorCode(ERRORI, int *, int **);
void Write_Q5_Log(char *);

int SL01(char **, SOLUZIONI,
         char *, PAIRLIST,
         IMGLIST, IMGLIST);
int SL02(char **, SOLUZIONI,
         char *, PAIRLIST,
         IMGLIST, IMGLIST);
int SL03(char **, SOLUZIONI, char *, ConfORARIO, INDATA,
         char *, char *, int, int,
         PAIRLIST, IMGLIST, IMGLIST,
         int *, char ***,
         int *, char ***,
         int *, char ***,
         int *, int   **,
         int *, int   **,
         int *);
int SL04(char **, int, PAIRLIST);
int SL05(char **, SOLUZIONI , INDATA,
         PAIRLIST, char *, int);
int SL06(char **, SOLUZIONI,
         int, char **,
         PAIRLIST);
int SL07(char **, SOLUZIONI,
         int, char **,
         PAIRLIST);
int SL08(char **, SOLUZIONI,
         int, int *,
         PAIRLIST, IMGLIST);
int SL09(char **, SOLUZIONI,
         int, int *,
         PAIRLIST, IMGLIST);
int SL10(char **, SOLUZIONI,
         int, char **,
         PAIRLIST);
int SL12(char **, PAIRLIST);
int DT01(char **, DETTAGLI,
         char *, PAIRLIST,
         IMGLIST, IMGLIST);
int DT02(char **, DETTAGLI,
         char *, PAIRLIST,
         IMGLIST, IMGLIST);
int DT03(char **, DETTAGLI, ConfORARIO,
         char *, PAIRLIST,
         IMGLIST, IMGLIST);
int DT04(char **, DETTAGLI, INDATA, char *, ConfORARIO,
         char *, char *,
         PAIRLIST, IMGLIST, IMGLIST,
         int *,  int   **,
         int *,  int   **);
int DT05(char **, DETTAGLI,
         int, int *,
         PAIRLIST, IMGLIST);
int DT06(char **, DETTAGLI,
         int, int *,
         PAIRLIST, IMGLIST);
int DT08(char **, PAIRLIST);
int DT11(char **, DETTAGLI, ConfORARIO, PAIRLIST);
int DT21(char **, DETTAGLI, ConfORARIO, PAIRLIST);
int ER01(char **, ERRORI, INDATA,
         char *, PAIRLIST, ERRMSG,
         int, int *);
int ER02(char **, ERRORI, INDATA,
         char *, PAIRLIST, ERRMSG,
         int, int *);
int ER03(char **, ERRORI, INDATA,
         char *, PAIRLIST, ERRMSG,
         int, int *);
int ER04(char **, ERRORI, INDATA,
         char *, PAIRLIST, ERRMSG,
         int, int *);
int ET01(char **, ERRORI, INDATA,
         char *, PAIRLIST, ERRMSG,
         int, int *);
int ET02(char **, ERRORI, INDATA,
         char *, PAIRLIST, ERRMSG,
         int, int *);
int ET03(char **, ERRORI, INDATA,
         char *, PAIRLIST, ERRMSG,
         int, int *);


char *Output_and_Free(RISPOSTA r, char *req, char *qs, ConfORARIO config)
{
   char *frmt;


   frmt = Build_Page(r, qs, config);

   if (NULL != req) free(req);
   if (NULL != qs) free(qs);
   Free_Risposta(r);

   return frmt;
}

char *Build_Page(RISPOSTA risposta, char *qs, ConfORARIO config)
{
   char *p;

   if      (0 == risposta.retcode) p = Build_Soluzioni(risposta, qs, config);
   else if (1 == risposta.retcode) p = Build_Dettagli(risposta, qs, config);
   else if (2 == risposta.retcode) p = Build_Errori(risposta, qs, config);

   return p;

}

char *Build_Soluzioni(RISPOSTA r, char *qs, ConfORARIO config)
{
   PAIRLIST languages;
   int i;
   int n_lang, lang_index;
   char *lng, *langcode, *langsuff;
   PAIRLIST sl_template;
   PAIRLIST variables;
   IMGLIST service_file;
   IMGLIST train_file;
   char *p, *out;
   int ret, err;
   int k;
         int n_stazpart;
         char **stazpart;
         int n_stazarr;
         char **stazarr;
         int n_stazcoin;
         char **stazcoin;
         int n_servizi;
         int   *servlist;
         int n_treni;
         int   *trenilist;
         int flag_domani;
    int IndPage, NumPages;
char *errbuf[80];

// Write_Q5_Log("starting 1");
   if (NULL == (r.indata).indpage) IndPage = 1;
   else IndPage = atoi((r.indata).indpage);
   if (IndPage == 0) IndPage = 1;
   NumPages = ((r.Dati).soluzioni).num_soluzioni / config.max_sol;
   if ( ((r.Dati).soluzioni).num_soluzioni % config.max_sol != 0) NumPages++;
   if (IndPage > NumPages) IndPage = 1;


// Write_Q5_Log("starting 2");
   languages = config.languages;
   n_lang = Get_PairList_Count(languages);
   lng = r.indata.lingua;
   for (i=0; i<n_lang; i++) {
     if (0 == strcmpi(lng, Get_PairList_Name_by_Index(languages,i))) {
       lang_index = i;
       langcode = Get_PairList_Name_by_Index(languages,i);
       langsuff = Get_PairList_Value_by_Index(languages,i);
       break;
     }
   }

   sl_template = *(config.sl_template + lang_index);
   service_file = *(config.service_file + lang_index);
   train_file = *(config.train_file + lang_index);
   variables = config.variables;

   p = (char *)malloc(1);
   *p = '\0';

// Write_Q5_Log("SL01");
   ret = SL01(&p, (r.Dati).soluzioni,
              langcode, sl_template, service_file, train_file);

// Write_Q5_Log("SL02");
   ret = SL02(&p, (r.Dati).soluzioni,
              langcode, sl_template, service_file, train_file);

// Write_Q5_Log("SL03");
   ret = SL03(&p, (r.Dati).soluzioni, qs, config, r.indata,
              langsuff, langcode, IndPage, NumPages,
              sl_template, service_file, train_file,
             &n_stazpart, &stazpart,
             &n_stazarr,  &stazarr,
             &n_stazcoin, &stazcoin,
             &n_servizi,  &servlist,
             &n_treni,    &trenilist,
             &flag_domani);

// Write_Q5_Log("SL04");
   ret = SL04(&p, flag_domani, sl_template);

// Write_Q5_Log("SL06");
   ret = SL06(&p, (r.Dati).soluzioni,
              n_stazpart, stazpart,
              sl_template);

// Write_Q5_Log("SL07");
   ret = SL07(&p, (r.Dati).soluzioni,
              n_stazarr, stazarr,
              sl_template);

// Write_Q5_Log("SL08");
   ret = SL08(&p, (r.Dati).soluzioni,
              n_servizi, servlist,
              sl_template, service_file);

// Write_Q5_Log("SL09");
   ret = SL09(&p, (r.Dati).soluzioni,
              n_treni, trenilist,
              sl_template, train_file);

// Write_Q5_Log("SL10");
   ret = SL10(&p, (r.Dati).soluzioni,
              n_stazcoin, stazcoin,
              sl_template);

// Write_Q5_Log("SL12");
   ret = SL12(&p, sl_template);



// Write_Q5_Log("fine-------------------------------------------------------");
// Write_Q5_Log(p);
   out = ConfigSubstitute(p, variables, "$", "$", 1, &err);


// Write_Q5_Log("start free");
   if (0 != n_stazpart) {
     for (k=0; k<n_stazpart; k++) free(*(stazpart+k));
     free(stazpart);
   }
   if (0 != n_stazarr) {
     for (k=0; k<n_stazarr; k++) free(*(stazarr+k));
     free(stazarr);
   }
   if (0 != n_stazcoin) {
     for (k=0; k<n_stazcoin; k++) free(*(stazcoin+k));
     free(stazcoin);
   }
   if (0 != n_servizi) free(servlist);
   if (0 != n_treni) free(trenilist);
// Write_Q5_Log("end free");


   return out;
}

char *Build_Dettagli(RISPOSTA r, char *qs, ConfORARIO config)
{
   PAIRLIST languages;
   int i;
   int n_lang, lang_index;
   char *lng, *langcode, *langsuff;
   PAIRLIST dt_template;
   PAIRLIST variables;
   IMGLIST service_file;
   IMGLIST train_file;
   char *p, *out;
   int ret, err;
   int k;
         int n_servizi;
         int   *servlist;
         int n_treni;
         int   *trenilist;

   languages = config.languages;
   n_lang = Get_PairList_Count(languages);
   lng = r.indata.lingua;
   for (i=0; i<n_lang; i++) {
     if (0 == strcmpi(lng, Get_PairList_Name_by_Index(languages,i))) {
       lang_index = i;
       langcode = Get_PairList_Name_by_Index(languages,i);
       langsuff = Get_PairList_Value_by_Index(languages,i);
       break;
     }
   }

   dt_template = *(config.dt_template + lang_index);
   service_file = *(config.service_file + lang_index);
   train_file = *(config.train_file + lang_index);
   variables = config.variables;

   p = (char *)malloc(1);
   *p = '\0';

   ret = DT01(&p, (r.Dati).dettagli,
              langcode, dt_template, service_file, train_file);

   ret = DT02(&p, (r.Dati).dettagli,
              langcode, dt_template, service_file, train_file);

   ret = DT03(&p, (r.Dati).dettagli, config,
              langcode, dt_template, service_file, train_file);

   ret = DT04(&p, (r.Dati).dettagli, r.indata, qs, config,
         langsuff, langcode,
         dt_template, service_file, train_file,
         &n_servizi,  &servlist,
         &n_treni,    &trenilist);

   ret = DT05(&p, (r.Dati).dettagli,
              n_servizi, servlist,
              dt_template, service_file);

   ret = DT06(&p, (r.Dati).dettagli,
              n_treni, trenilist,
              dt_template, train_file);


   ret = DT11(&p, (r.Dati).dettagli, config, dt_template);

   ret = DT21(&p, (r.Dati).dettagli, config, dt_template);


   ret = DT08(&p, dt_template);



   out = ConfigSubstitute(p, variables, "$", "$", 1, &err);


   if (0 != n_servizi) free(servlist);
   if (0 != n_treni) free(trenilist);

   return out;
}


char *Build_Errori(RISPOSTA r, char *qs, ConfORARIO config)
{
   char *p, *out;
   int n_err, *err_codes;
   int err_type;
   PAIRLIST languages;
   int i;
   int n_lang, lang_index;
   char *lng, *langcode, *langsuff;
   PAIRLIST er_template, et_template;
   PAIRLIST variables;
   ERRMSG  error_msg;
   int ret, err;
   int k;

   languages = config.languages;
   n_lang = Get_PairList_Count(languages);
   lng = r.indata.lingua;
   for (i=0; i<n_lang; i++) {
     if (0 == strcmpi(lng, Get_PairList_Name_by_Index(languages,i))) {
       lang_index = i;
       langcode = Get_PairList_Name_by_Index(languages,i);
       langsuff = Get_PairList_Value_by_Index(languages,i);
       break;
     }
   }

   er_template = *(config.er_template + lang_index);
   et_template = *(config.et_template + lang_index);
   error_msg = *(config.error_msg + lang_index);
   variables = config.variables;

   p = (char *)malloc(1);
   *p = '\0';


   err_type = ErrorCode((r.Dati).errori, &n_err, &err_codes);

   if (err_type == 1) {
     ret = ER01(&p, (r.Dati).errori, r.indata,
                langcode, er_template, error_msg,
                n_err, err_codes);

     ret = ER02(&p, (r.Dati).errori, r.indata,
                langcode, er_template, error_msg,
                n_err, err_codes);

     ret = ER03(&p, (r.Dati).errori, r.indata,
                langcode, er_template, error_msg,
                n_err, err_codes);

     ret = ER04(&p, (r.Dati).errori, r.indata,
                langsuff, er_template, error_msg,
                n_err, err_codes);

   }

   else {
     ret = ET01(&p, (r.Dati).errori, r.indata,
                langcode, et_template, error_msg,
                n_err, err_codes);

     ret = ET02(&p, (r.Dati).errori, r.indata,
                langcode, et_template, error_msg,
                n_err, err_codes);

     ret = ET03(&p, (r.Dati).errori, r.indata,
                langcode, et_template, error_msg,
                n_err, err_codes);

   }


   out = ConfigSubstitute(p, variables, "$", "$", 1, &err);

   free(err_codes);

   return out;
}

int ErrorCode(ERRORI e, int *ne, int **ecs)
{
  int et, ie;

  *ne = 0;
  *ecs = (int *)NULL;

  if (e.motore != 0 || e.richiesta != 0) {
    et = 2;
    *ecs = (int *)malloc(sizeof(int));
    if (e.motore == 1) **ecs = 1;
    else if (e.motore == 0) {
      if (e.richiesta == 1) **ecs = 2;
      if (e.richiesta == 2) **ecs = 3;
      if (e.richiesta == 3) **ecs = 15;
      else **ecs = 16;
    }
  }

  else {
    et = 1;
    if (e.stazione_p != 0) {
      ie = *ne;
      (*ne)++;
      *ecs = (int *)realloc(*ecs,(*ne)*sizeof(int));
      if (e.stazione_p == 1) *(ie + *ecs) = 4;
      else if (e.stazione_p == 2) *(ie + *ecs) = 5;
      else if (e.stazione_p == 3) *(ie + *ecs) = 6;
      else *(ie + *ecs) = 7;
    }
    if (e.stazione_a != 0) {
      ie = *ne;
      (*ne)++;
      *ecs = (int *)realloc(*ecs,(*ne)*sizeof(int));
      if (e.stazione_a == 1) *(ie + *ecs) = 8;
      else if (e.stazione_a == 2) *(ie + *ecs) = 9;
      else *(ie + *ecs) = 10;
    }
    if (e.data != 0) {
      ie = *ne;
      (*ne)++;
      *ecs = (int *)realloc(*ecs,(*ne)*sizeof(int));
      if (e.data == 1) *(ie + *ecs) = 11;
      else *(ie + *ecs) = 12;
    }
    if (e.ora != 0) {
      ie = *ne;
      (*ne)++;
      *ecs = (int *)realloc(*ecs,(*ne)*sizeof(int));
      *(ie + *ecs) = 13;
    }
    if (e.soluzioni != 0) {
      ie = *ne;
      (*ne)++;
      *ecs = (int *)realloc(*ecs,(*ne)*sizeof(int));
      *(ie + *ecs) = 14;
    }

  }

  return et;

}
