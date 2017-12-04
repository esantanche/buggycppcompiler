#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "asfsdat8.h"
#include "asfsutil.h"
#include "asconfig.h"
#include "asfsprn9.h"


int ParteDomani(int, int, char *);
void TraUnMinuto(int, int, int, int, int,
                 int *, int *, int *, int *, int *);
void prtpairlist(char *,PAIRLIST, char *);
void Domani(int, int, int, int *, int *, int *);
void Write_Q5_Log(char *);
int AddTemplateWithVars(char **, char *, PAIRLIST, PAIRLIST);
int AddTemplateWithoutVars(char **, char *, PAIRLIST);

int SL01(char **p, SOLUZIONI r,
         char *langcode, PAIRLIST sl_template,
         IMGLIST service, IMGLIST train)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;


  v = Set_PairList();
  Add_PairList(&v, "STAZIONE_P_ESTESA", r.stazione_p);
  Add_PairList(&v, "STAZIONE_A_ESTESA", r.stazione_a);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "SL01.HTML", sl_template, v);
  Free_PairList(v);

  return err;
}

int SL02(char **p, SOLUZIONI r,
         char *langcode, PAIRLIST sl_template,
         IMGLIST service, IMGLIST train)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;
  char buf[80];


  v = Set_PairList();

  Add_PairList(&v, "STAZIONE_P_ESTESA", r.stazione_p);
  Add_PairList(&v, "STAZIONE_A_ESTESA", r.stazione_a);
  sprintf(buf,"%d/%d/%d",r.giorno,r.mese,r.anno);
  Add_PairList(&v, "DATA_RICHIESTA", buf);
  sprintf(buf,"%02d:%02d",r.ora,r.minuto);
  Add_PairList(&v, "ORA_RICHIESTA", buf);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "SL02.HTML", sl_template, v);
  Free_PairList(v);

  return err;
}


int SL03(char **p, SOLUZIONI r, char *querystring, ConfORARIO config, INDATA id,
         char *langsuff, char *langcode, int indpage, int npages,
         PAIRLIST sl_template, IMGLIST service, IMGLIST train,
         int *n_stazpart, char ***stazpart,
         int *n_stazarr,  char ***stazarr,
         int *n_stazcoin, char ***stazcoin,
         int *n_servizi,  int   **servlist,
         int *n_treni,    int   **trenilist,
         int *flag_domani)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  SOLUZIONE sol;
  int err;
  int isol, j;
  char buf[80];
  int isrv;
  char *gif, *alt;
  char rowspan[80];
  int k, ic, found;
  int isolstr, isolend, flag_pagine;
  char buffer[1024];
  int flag_coincidenze;


  isolstr = config.max_sol * (indpage - 1);
  isolend = config.max_sol * indpage;
  if (isolend > r.num_soluzioni) isolend = r.num_soluzioni;
  if (isolstr == 0 && isolend == r.num_soluzioni) flag_pagine = 0;
  else flag_pagine = 1;

  if (r.flag_coincidenze != 0 && flag_pagine != 0) {
    flag_coincidenze = 0;
    for (isol=isolstr; isol<isolend; isol++) {
      sol = *(r.soluzioni + isol);
      if (sol.num_coincidenze != 0) {
        flag_coincidenze = 1;
        break;
      }
    }
  }
  else flag_coincidenze = r.flag_coincidenze;



  if (flag_coincidenze == 0) {
// Write_Q5_Log("SL03a1a");
    v = Set_PairList();
    Add_PairList(&v, "LANG_SUFFIX", langsuff);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "SL03A1A.HTML", sl_template, v);
    Free_PairList(v);

    if (r.num_soluzioni != 1) {
      v = Set_PairList();
      sprintf(buffer,"stazin=%s&stazout=%s&datag=%d&datam=%d&dataa=%d&timsh=%d&timsm=%d&lang=%s&page=%d",
              id.stazione_p,id.stazione_a,r.giorno,r.mese,r.anno,r.ora,r.minuto,id.lingua,1);
      /* EMS Modifica per trattamento query string
      for (k=0; k<strlen(buffer); k++) {
        if (' ' == buffer[k]) buffer[k] = '+';
      }
      */
      Add_PairList(&v, "QUERY_STRING", buffer);

      Sort_PairList_I_A(v);
      if (r.sort == 0) {
// Write_Q5_Log("SL03a1b1");
        err = AddTemplateWithVars(p, "SL03A1B1.HTML", sl_template, v);
      } else if (r.sort==1) {
// Write_Q5_Log("SL03a1b2");
        err = AddTemplateWithVars(p, "SL03A1B2.HTML", sl_template, v);
      } else  {
// Write_Q5_Log("SL03a1b3");
        err = AddTemplateWithVars(p, "SL03A1B3.HTML", sl_template, v);  /* mod. 27/02/97 RS */
      }
      Free_PairList(v);
    }

// Write_Q5_Log("SL03a1c");
    err = AddTemplateWithoutVars(p, "SL03A1C.HTML", sl_template);
  }

  else {
// Write_Q5_Log("SL03a2a");
    v = Set_PairList();
    Add_PairList(&v, "LANG_SUFFIX", langsuff);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "SL03A2A.HTML", sl_template, v);
    Free_PairList(v);

    if (r.num_soluzioni != 1) {
      v = Set_PairList();
      sprintf(buffer,"stazin=%s&stazout=%s&datag=%d&datam=%d&dataa=%d&timsh=%d&timsm=%d&lang=%s&page=%d",
              id.stazione_p,id.stazione_a,r.giorno,r.mese,r.anno,r.ora,r.minuto,id.lingua,1);
      /* EMS Modifica per trattamento query string
      for (k=0; k<strlen(buffer); k++) {
        if (' ' == buffer[k]) buffer[k] = '+';
      }
      */
      Add_PairList(&v, "QUERY_STRING", buffer);

      Sort_PairList_I_A(v);
      if (r.sort == 0) {
// Write_Q5_Log("SL03a2b1");
        err = AddTemplateWithVars(p, "SL03A2B1.HTML", sl_template, v);
      } else if (r.sort==1) {
// Write_Q5_Log("SL03a2b2");
        err = AddTemplateWithVars(p, "SL03A2B2.HTML", sl_template, v);
      } else  {
// Write_Q5_Log("SL03a2b3");
        err = AddTemplateWithVars(p, "SL03A2B3.HTML", sl_template, v);  /* mod. 27/02/97 RS */
      }
      Free_PairList(v);
    }

// Write_Q5_Log("SL03a2c");
    err = AddTemplateWithoutVars(p, "SL03A2C.HTML", sl_template);
  }
// Write_Q5_Log(*p);

// Write_Q5_Log("vars");
  *stazpart  = (char **)NULL;
  *stazarr   = (char **)NULL;
  *stazcoin  = (char **)NULL;
  *servlist  = (int *)NULL;
  *trenilist = (int *)NULL;
  *n_stazpart = 0;
  *n_stazarr  = 0;
  *n_stazcoin = 0;
  *n_servizi  = 0;
  *n_treni    = 0;
  *flag_domani = 0;

  sol = *(r.soluzioni + r.num_soluzioni - 1);

  isolstr = config.max_sol * (indpage - 1);
  isolend = config.max_sol * indpage;
  if (isolend > r.num_soluzioni) isolend = r.num_soluzioni;
  if (isolstr == 0 && isolend == r.num_soluzioni) flag_pagine = 0;
  else flag_pagine = 1;

  if (r.flag_coincidenze != 0 && flag_pagine != 0) {
    flag_coincidenze = 0;
    for (isol=isolstr; isol<isolend; isol++) {
      sol = *(r.soluzioni + isol);
      if (sol.num_coincidenze != 0) {
        flag_coincidenze = 1;
        break;
      }
    }
  }
  else flag_coincidenze = r.flag_coincidenze;


  for (isol=isolstr; isol<isolend; isol++) {

// Write_Q5_Log("begin loop");
    sol = *(r.soluzioni + isol);


    if (strlen(sol.stazione_p) != 0) {
      found = 0;
      for (k=0; k<*n_stazpart; k++) {
        if (0 == strcmpi(sol.stazione_p, *((*stazpart)+k)) ) {
          found = 1;
          break;
        }
      }
      if (0 == found) {
        *stazpart = (char **)realloc(*stazpart, (1 + *n_stazpart) * sizeof(char *));
        *((*stazpart)+*n_stazpart) = (char *)malloc(1 + strlen(sol.stazione_p));
        strcpy(*((*stazpart)+*n_stazpart), sol.stazione_p);
        (*n_stazpart)++;
      }
    }

    if (strlen(sol.stazione_a) != 0) {
      found = 0;
      for (k=0; k<*n_stazarr; k++) {
        if (0 == strcmpi(sol.stazione_a, *((*stazarr)+k)) ) {
          found = 1;
          break;
        }
      }
      if (0 == found) {
        *stazarr = (char **)realloc(*stazarr, (1 + *n_stazarr) * sizeof(char *));
        *((*stazarr)+*n_stazarr) = (char *)malloc(1 + strlen(sol.stazione_a));
        strcpy(*((*stazarr)+*n_stazarr), sol.stazione_a);
        (*n_stazarr)++;
      }
    }

    if (flag_coincidenze != 0) {
      for (ic=0; ic<sol.num_coincidenze; ic++) {
        found = 0;
        for (k=0; k<*n_stazcoin; k++) {
          if (0 == strcmpi( ((sol.coincidenze)+ic)->stazione, *((*stazcoin)+k)) ) {
            found = 1;
            break;
          }
        }
        if (0 == found) {
          *stazcoin = (char **)realloc(*stazcoin, (1 + *n_stazcoin) * sizeof(char *));
          *((*stazcoin)+*n_stazcoin) = (char *)malloc(1 + strlen(((sol.coincidenze)+ic)->stazione));
          strcpy(*((*stazcoin)+*n_stazcoin), ((sol.coincidenze)+ic)->stazione);
          (*n_stazcoin)++;
        }
      }
    }

    for (ic=0; ic<sol.num_servizi; ic++) {
      found = 0;
      for (k=0; k<*n_servizi; k++) {
        if (*(sol.servizi+ic) == *((*servlist)+k) ) {
          found = 1;
          break;
        }
      }
      if (0 == found) {
        *servlist = (int *)realloc(*servlist, (1 + *n_servizi) * sizeof(int));
        *((*servlist)+*n_servizi) = *(sol.servizi+ic);
        (*n_servizi)++;
      }
    }

    for (ic=0; ic<sol.num_tipitreni; ic++) {
      found = 0;
      for (k=0; k<*n_treni; k++) {
        if (*(sol.tipitreni+ic) == *((*trenilist)+k) ) {
          found = 1;
          break;
        }
      }
      if (0 == found) {
        *trenilist = (int *)realloc(*trenilist, (1 + *n_treni) * sizeof(int));
        *((*trenilist)+*n_treni) = *(sol.tipitreni+ic);
        (*n_treni)++;
      }
    }



    if (flag_coincidenze == 0) {

// Write_Q5_Log("SL03b1a");
      v = Set_PairList();
      Add_PairList(&v, "QUERY_STRING", querystring);
      sprintf(buf,"%d",isol+1);
      Add_PairList(&v, "SOL_INDEX", buf);
      Add_PairList(&v, "SOL_ORARIO_P", sol.orario_p);
      if (0 == ParteDomani(r.ora, r.minuto, sol.orario_p)) {
        Add_PairList(&v, "SOL_DOMANI", "");
      }
      else {
        Add_PairList(&v, "SOL_DOMANI", "*");
        *flag_domani = 1;
      }
      Add_PairList(&v, "SOL_STAZIONE_P", sol.stazione_p);
      Add_PairList(&v, "SOL_ORARIO_A", sol.orario_a);
      Add_PairList(&v, "SOL_STAZIONE_A", sol.stazione_a);
      Add_PairList(&v, "SOL_TEMPO", sol.tempo);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL03B1A.HTML", sl_template, v);
      Free_PairList(v);

// Write_Q5_Log("SL03b1b");
      for (j=0; j<sol.num_servizi; j++) {
        isrv = *(sol.servizi + j);
        gif = ((service.imglist)+isrv)->gif;
        if (0 != strlen(gif)) {
          v = Set_PairList();
          alt = ((service.imglist)+isrv)->alt;
          Add_PairList(&v, "SERVICE_IMAGE", gif);
          Add_PairList(&v, "SERVICE_ALT", alt);

          Sort_PairList_I_A(v);
          err = AddTemplateWithVars(p, "SL03B1B.HTML", sl_template, v);
          Free_PairList(v);
        }
      }

// Write_Q5_Log("SL03b1c");
      err = AddTemplateWithoutVars(p, "SL03B1C.HTML", sl_template);

// Write_Q5_Log("SL031d");
      for (j=0; j<sol.num_tipitreni; j++) {
        isrv = *(sol.tipitreni + j);
        gif = ((train.imglist)+isrv)->gif;
        if (0 != strlen(gif)) {
          v = Set_PairList();
          alt = ((train.imglist)+isrv)->alt;
          Add_PairList(&v, "TRAIN_IMAGE", gif);
          Add_PairList(&v, "TRAIN_ALT", alt);

          Sort_PairList_I_A(v);
          err = AddTemplateWithVars(p, "SL03B1D.HTML", sl_template, v);
          Free_PairList(v);
        }
      }

// Write_Q5_Log("SL03b1e");
      err = AddTemplateWithoutVars(p, "SL03B1E.HTML", sl_template);


    }

    else if (flag_coincidenze != 0 && sol.num_coincidenze < 2) {

// Write_Q5_Log("SL03b2a");
      v = Set_PairList();
      Add_PairList(&v, "QUERY_STRING", querystring);
      sprintf(buf,"%d",isol+1);
      Add_PairList(&v, "SOL_INDEX", buf);
      Add_PairList(&v, "SOL_ORARIO_P", sol.orario_p);
      if (0 == ParteDomani(r.ora, r.minuto, sol.orario_p)) {
        Add_PairList(&v, "SOL_DOMANI", "");
      }
      else {
        Add_PairList(&v, "SOL_DOMANI", "*");
        *flag_domani = 1;
      }
      Add_PairList(&v, "SOL_STAZIONE_P", sol.stazione_p);
      Add_PairList(&v, "SOL_ORARIO_A", sol.orario_a);
      Add_PairList(&v, "SOL_STAZIONE_A", sol.stazione_a);
      Add_PairList(&v, "SOL_TEMPO", sol.tempo);
      if (sol.num_coincidenze == 0) {
        Add_PairList(&v, "COINC_STAZIONE", "&nbsp;");
        Add_PairList(&v, "COINC_ARRIVO", "&nbsp;");
        Add_PairList(&v, "COINC_ATTESA", "&nbsp;");
      }
      else {
        Add_PairList(&v, "COINC_STAZIONE", (sol.coincidenze)->stazione);
        Add_PairList(&v, "COINC_ARRIVO", (sol.coincidenze)->arrivo);
        Add_PairList(&v, "COINC_ATTESA", (sol.coincidenze)->attesa);
      }

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL03B2A.HTML", sl_template, v);
      Free_PairList(v);
// Write_Q5_Log(*p);

// Write_Q5_Log("SL03b2b");
      for (j=0; j<sol.num_servizi; j++) {
        isrv = *(sol.servizi + j);
        gif = ((service.imglist)+isrv)->gif;
        if (0 != strlen(gif)) {
          v = Set_PairList();
          alt = ((service.imglist)+isrv)->alt;
          Add_PairList(&v, "SERVICE_IMAGE", gif);
          Add_PairList(&v, "SERVICE_ALT", alt);

          Sort_PairList_I_A(v);
          err = AddTemplateWithVars(p, "SL03B2B.HTML", sl_template, v);
          Free_PairList(v);
// Write_Q5_Log(*p);
        }
      }

// Write_Q5_Log("SL03b2c");
      err = AddTemplateWithoutVars(p, "SL03B2C.HTML", sl_template);

// Write_Q5_Log("SL03b2d");
      for (j=0; j<sol.num_tipitreni; j++) {
        isrv = *(sol.tipitreni + j);
        gif = ((train.imglist)+isrv)->gif;
        if (0 != strlen(gif)) {
          v = Set_PairList();
          alt = ((train.imglist)+isrv)->alt;
          Add_PairList(&v, "TRAIN_IMAGE", gif);
          Add_PairList(&v, "TRAIN_ALT", alt);

          Sort_PairList_I_A(v);
          err = AddTemplateWithVars(p, "SL03B2D.HTML", sl_template, v);
          Free_PairList(v);
        }
      }

// Write_Q5_Log("SL03b2e");
      err = AddTemplateWithoutVars(p, "SL03B2E.HTML", sl_template);

    }

    else if (flag_coincidenze != 0 && sol.num_coincidenze > 1) {

      sprintf(rowspan,"%d",sol.num_coincidenze);

// Write_Q5_Log("SL03b3a");
      v = Set_PairList();
      Add_PairList(&v, "NUM_COINCIDENZE", rowspan);
      Add_PairList(&v, "QUERY_STRING", querystring);
      sprintf(buf,"%d",isol+1);
      Add_PairList(&v, "SOL_INDEX", buf);
      Add_PairList(&v, "SOL_ORARIO_P", sol.orario_p);
      if (0 == ParteDomani(r.ora, r.minuto, sol.orario_p)) {
        Add_PairList(&v, "SOL_DOMANI", "");
      }
      else {
        Add_PairList(&v, "SOL_DOMANI", "*");
        *flag_domani = 1;
      }
      Add_PairList(&v, "SOL_STAZIONE_P", sol.stazione_p);
      Add_PairList(&v, "SOL_ORARIO_A", sol.orario_a);
      Add_PairList(&v, "SOL_STAZIONE_A", sol.stazione_a);
      Add_PairList(&v, "SOL_TEMPO", sol.tempo);
      Add_PairList(&v, "COINC_STAZIONE", (sol.coincidenze)->stazione);
      Add_PairList(&v, "COINC_ARRIVO", (sol.coincidenze)->arrivo);
      Add_PairList(&v, "COINC_ATTESA", (sol.coincidenze)->attesa);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL03B3A.HTML", sl_template, v);
      Free_PairList(v);

// Write_Q5_Log("SL03b3b");
      for (j=0; j<sol.num_servizi; j++) {
        isrv = *(sol.servizi + j);
        gif = ((service.imglist)+isrv)->gif;
        if (0 != strlen(gif)) {
          v = Set_PairList();
          alt = ((service.imglist)+isrv)->alt;
          Add_PairList(&v, "SERVICE_IMAGE", gif);
          Add_PairList(&v, "SERVICE_ALT", alt);

          Sort_PairList_I_A(v);
          err = AddTemplateWithVars(p, "SL03B3B.HTML", sl_template, v);
          Free_PairList(v);
        }
      }

// Write_Q5_Log("SL03b2c");
      v = Set_PairList();
      Add_PairList(&v, "NUM_COINCIDENZE", rowspan);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL03B3C.HTML", sl_template, v);
      Free_PairList(v);

// Write_Q5_Log("SL03b2d");
      for (j=0; j<sol.num_tipitreni; j++) {
        isrv = *(sol.tipitreni + j);
        gif = ((train.imglist)+isrv)->gif;
        if (0 != strlen(gif)) {
          v = Set_PairList();
          alt = ((train.imglist)+isrv)->alt;
          Add_PairList(&v, "TRAIN_IMAGE", gif);
          Add_PairList(&v, "TRAIN_ALT", alt);

          Sort_PairList_I_A(v);
          err = AddTemplateWithVars(p, "SL03B3D.HTML", sl_template, v);
          Free_PairList(v);
        }
      }

// Write_Q5_Log("SL03b3e");
      err = AddTemplateWithoutVars(p, "SL03B3E.HTML", sl_template);

// Write_Q5_Log("SL03b3f");
      for (j=1; j<sol.num_coincidenze; j++) {
        v = Set_PairList();
        Add_PairList(&v, "COINC_STAZIONE", ((sol.coincidenze)+j)->stazione);
        Add_PairList(&v, "COINC_ARRIVO", ((sol.coincidenze)+j)->arrivo);
        Add_PairList(&v, "COINC_ATTESA", ((sol.coincidenze)+j)->attesa);

        Sort_PairList_I_A(v);
        err = AddTemplateWithVars(p, "SL03B3F.HTML", sl_template, v);
        Free_PairList(v);
      }

    }

  }

  if (flag_pagine == 0) {
// Write_Q5_Log("SL03c1");
    err = AddTemplateWithoutVars(p, "SL03C1.HTML", sl_template);
  }

  else {
    if (flag_coincidenze == 0) {

// Write_Q5_Log("SL03c2a");
      v = Set_PairList();
      sprintf(buf,"%d",indpage);
      Add_PairList(&v, "IND_PAGE", buf);
      sprintf(buf,"%d",npages);
      Add_PairList(&v, "NUM_PAGES", buf);
      sprintf(buf,"%d",isolstr+1);
      Add_PairList(&v, "PRIMA_SOLUZIONE", buf);
      sprintf(buf,"%d",isolend);
      Add_PairList(&v, "ULTIMA_SOLUZIONE", buf);
      sprintf(buf,"%d",r.num_soluzioni);
      Add_PairList(&v, "NUMERO_SOLUZIONI", buf);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL03C2A.HTML", sl_template, v);
      Free_PairList(v);

// Write_Q5_Log("SL03c2b");
      for (j=1; j<=npages; j++) {
        if (j != indpage) {
          v = Set_PairList();
          if ((j == npages) || ( (j == npages-1) && (indpage == npages) ) ) {
            sprintf(buf,"%d",j);
          }
          else {
            sprintf(buf,"%d - ",j);
          }
          Add_PairList(&v, "OTHER_PAGE", buf);
          sprintf(buffer,"stazin=%s&stazout=%s&datag=%d&datam=%d&dataa=%d&timsh=%d&timsm=%d&lang=%s&page=%d&sort=%d",
                  id.stazione_p,id.stazione_a,r.giorno,r.mese,r.anno,r.ora,r.minuto,id.lingua,j,r.sort);
          /* EMS Modifica per trattamento query string
          for (k=0; k<strlen(buffer); k++) {
            if (' ' == buffer[k]) buffer[k] = '+';
          }
          */
          Add_PairList(&v, "QUERY_OTHER_PAGE", buffer);

          Sort_PairList_I_A(v);
          err = AddTemplateWithVars(p, "SL03C2B.HTML", sl_template, v);
          Free_PairList(v);
        }
      }

// Write_Q5_Log("SL03c2c");
      err = AddTemplateWithoutVars(p, "SL03C2C.HTML", sl_template);

    }
    else {
// Write_Q5_Log("SL03c3a");
      v = Set_PairList();
      sprintf(buf,"%d",indpage);
      Add_PairList(&v, "IND_PAGE", buf);
      sprintf(buf,"%d",npages);
      Add_PairList(&v, "NUM_PAGES", buf);
      sprintf(buf,"%d",isolstr+1);
      Add_PairList(&v, "PRIMA_SOLUZIONE", buf);
      sprintf(buf,"%d",isolend);
      Add_PairList(&v, "ULTIMA_SOLUZIONE", buf);
      sprintf(buf,"%d",r.num_soluzioni);
      Add_PairList(&v, "NUMERO_SOLUZIONI", buf);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL03C3A.HTML", sl_template, v);
      Free_PairList(v);

// Write_Q5_Log("SL03c3b");
      for (j=1; j<=npages; j++) {
        if (j != indpage) {
          v = Set_PairList();
          if ((j == npages) || ( (j == npages-1) && (indpage == npages) ) ) {
            sprintf(buf,"%d",j);
          }
          else {
            sprintf(buf,"%d - ",j);
          }
          Add_PairList(&v, "OTHER_PAGE", buf);
          sprintf(buffer,"stazin=%s&stazout=%s&datag=%d&datam=%d&dataa=%d&timsh=%d&timsm=%d&lang=%s&page=%d&sort=%d",
                  id.stazione_p,id.stazione_a,r.giorno,r.mese,r.anno,r.ora,r.minuto,id.lingua,j,r.sort);
          /* EMS Modifica per trattamento query string
          for (k=0; k<strlen(buffer); k++) {
            if (' ' == buffer[k]) buffer[k] = '+';
          }
          */
          Add_PairList(&v, "QUERY_OTHER_PAGE", buffer);

          Sort_PairList_I_A(v);
          err = AddTemplateWithVars(p, "SL03C3B.HTML", sl_template, v);
          Free_PairList(v);
        }
      }

// Write_Q5_Log("SL03c3c");
      err = AddTemplateWithoutVars(p, "SL03C3C.HTML", sl_template);

    }
  }

  return 0;
}

int SL04(char **p, int flag_domani,
         PAIRLIST sl_template)
{
  int err;

  if (flag_domani == 0) return 0;

  err = AddTemplateWithoutVars(p, "SL04.HTML", sl_template);

  return 0;
}

int SL06(char **p, SOLUZIONI r,
         int n_stazpart, char **stazpart,
         PAIRLIST sl_template)
{
  char *to, *t1, *t2;
  int k, i, j;
  int err, found;
  PAIRLIST v;

  if (0 == n_stazpart) return 0;

  err = AddTemplateWithoutVars(p, "SL06A.HTML", sl_template);

  for (k=0; k<n_stazpart; k++) {
    v = Set_PairList();
    found = -1;
    for (i=0; i<r.num_acronimi; i++) {
      if ( 0 == strcmpi(*(stazpart+k), (r.acronimi+i)->corto)) {
        found = i;
        break;
      }
    }
    if (found != -1) {
      Add_PairList(&v, "N_STAZIONE_P_CORTO", (r.acronimi+found)->corto);
      Add_PairList(&v, "N_STAZIONE_P_LUNGO", (r.acronimi+found)->lungo);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL06B.HTML", sl_template, v);
      Free_PairList(v);
    }
  }

  err = AddTemplateWithoutVars(p, "SL06C.HTML", sl_template);

  return 0;

}

int SL07(char **p, SOLUZIONI r,
         int n_stazarr, char **stazarr,
         PAIRLIST sl_template)
{
  char *to, *t1, *t2;
  int k, i, j;
  int err, found;
  PAIRLIST v;

  if (0 == n_stazarr) return 0;

  err = AddTemplateWithoutVars(p, "SL07A.HTML", sl_template);

  for (k=0; k<n_stazarr; k++) {
    v = Set_PairList();
    found = -1;
    for (i=0; i<r.num_acronimi; i++) {
      if ( 0 == strcmpi(*(stazarr+k), (r.acronimi+i)->corto)) {
        found = i;
        break;
      }
    }
    if (found != -1) {
      Add_PairList(&v, "N_STAZIONE_A_CORTO", (r.acronimi+found)->corto);
      Add_PairList(&v, "N_STAZIONE_A_LUNGO", (r.acronimi+found)->lungo);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL07B.HTML", sl_template, v);
      Free_PairList(v);
    }
  }

  err = AddTemplateWithoutVars(p, "SL07C.HTML", sl_template);

  return 0;

}

int SL08(char **p, SOLUZIONI r,
         int n_servizi, int *servlist,
         PAIRLIST sl_template, IMGLIST service)
{
  char *to, *t1, *t2;
  int k, i, j;
  int err, isrv;
  PAIRLIST v;
  char *gif, *alt, *descr;

  if (0 == n_servizi) return 0;

  err = AddTemplateWithoutVars(p, "SL08A.HTML", sl_template);

  for (k=0; k<n_servizi; k++) {
    isrv = *(servlist+k);
    gif = (service.imglist+isrv)->gif;
    if (0 != strlen(gif)) {
      alt = (service.imglist+isrv)->alt;
      descr = (service.imglist+isrv)->descr;
      v = Set_PairList();
      Add_PairList(&v, "SERVICE_IMAGE", gif);
      Add_PairList(&v, "SERVICE_ALT", alt);
      Add_PairList(&v, "SERVICE_DESCR", descr);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL08B.HTML", sl_template, v);
      Free_PairList(v);
    }
  }

  err = AddTemplateWithoutVars(p, "SL08C.HTML", sl_template);

  return 0;

}

int SL09(char **p, SOLUZIONI r,
         int n_treni, int *trenilist,
         PAIRLIST sl_template, IMGLIST train)
{
  char *to, *t1, *t2;
  int k, i, j;
  int err, isrv;
  PAIRLIST v;
  char *gif, *alt, *descr;

  if (0 == n_treni) return 0;

  err = AddTemplateWithoutVars(p, "SL09A.HTML", sl_template);

  for (k=0; k<n_treni; k++) {
    isrv = *(trenilist+k);
    gif = (train.imglist+isrv)->gif;
    if (0 != strlen(gif)) {
      alt = (train.imglist+isrv)->alt;
      descr = (train.imglist+isrv)->descr;
      v = Set_PairList();
      Add_PairList(&v, "TRAIN_IMAGE", gif);
      Add_PairList(&v, "TRAIN_ALT", alt);
      Add_PairList(&v, "TRAIN_DESCR", descr);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL09B.HTML", sl_template, v);
      Free_PairList(v);
    }
  }

  err = AddTemplateWithoutVars(p, "SL09C.HTML", sl_template);

  return 0;

}

int SL10(char **p, SOLUZIONI r,
         int n_stazcoin, char **stazcoin,
         PAIRLIST sl_template)
{
  char *to, *t1, *t2;
  int k, i, j;
  int err, found;
  PAIRLIST v;

  if (0 == n_stazcoin) return 0;

  err = AddTemplateWithoutVars(p, "SL10A.HTML", sl_template);

  for (k=0; k<n_stazcoin; k++) {
    v = Set_PairList();
    found = -1;
    for (i=0; i<r.num_acronimi; i++) {
      if ( 0 == strcmpi(*(stazcoin+k), (r.acronimi+i)->corto)) {
        found = i;
        break;
      }
    }
    if (found != -1) {
      Add_PairList(&v, "ACR_STAZIONE_CORTO", (r.acronimi+found)->corto);
      Add_PairList(&v, "ACR_STAZIONE_LUNGO", (r.acronimi+found)->lungo);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "SL10B.HTML", sl_template, v);
      Free_PairList(v);
    }
  }

  err = AddTemplateWithoutVars(p, "SL10C.HTML", sl_template);

  return 0;

}

int SL12(char **p, PAIRLIST sl_template)
{
  char *to;
  int err;

  err = AddTemplateWithoutVars(p, "SL12.HTML", sl_template);

  return 0;

}


int DT01(char **p, DETTAGLI r,
         char *langcode, PAIRLIST dt_template,
         IMGLIST service, IMGLIST train)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;


  v = Set_PairList();

  Add_PairList(&v, "STAZIONE_P_ESTESA", r.stazione_p);
  Add_PairList(&v, "STAZIONE_A_ESTESA", r.stazione_a);
  Add_PairList(&v, "ORARIO_SOLUZIONE", r.orario);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "DT01.HTML", dt_template, v);
  Free_PairList(v);

  return 0;
}

int DT02(char **p, DETTAGLI r,
         char *langcode, PAIRLIST dt_template,
         IMGLIST service, IMGLIST train)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;
  char buf[80];

  v = Set_PairList();

  Add_PairList(&v, "STAZIONE_P_ESTESA", r.stazione_p);
  Add_PairList(&v, "STAZIONE_A_ESTESA", r.stazione_a);
  sprintf(buf,"%d/%d/%d",r.giorno,r.mese,r.anno);
  Add_PairList(&v, "DATA_RICHIESTA", buf);
  sprintf(buf,"%02d:%02d",r.ora,r.minuto);
  Add_PairList(&v, "ORA_RICHIESTA", buf);
  Add_PairList(&v, "ORARIO_SOLUZIONE", r.orario);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "DT02A.HTML", dt_template, v);
  Free_PairList(v);

  if (0 != ParteDomani(r.ora, r.minuto, r.orario)) {
    err = AddTemplateWithoutVars(p, "DT02B.HTML", dt_template);
  }

  err = AddTemplateWithoutVars(p, "DT02C.HTML", dt_template);

  return 0;
}

int DT03(char **p, DETTAGLI r, ConfORARIO cnf,
         char *langcode, PAIRLIST dt_template,
         IMGLIST service, IMGLIST train)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;
//  char buf[256];
  char buf[1024];

  v = Set_PairList();

  sprintf(buf,"%d",r.km);
  Add_PairList(&v, "SOL_KM", buf);

char *i1, *i2, *sc, *sl, *sc1;
int iab;
  i1 = i2 = (char *)malloc(1+strlen(r.instradamento));
  strcpy(i1,r.instradamento);
  buf[0] = '\0';
  sc = strtok(i2,"*");
  do {
    sc1 = (char *)malloc(1+strlen(sc));
    strcpy(sc1,sc);
    for (iab=0;iab<strlen(sc);iab++) if(sc[iab] == ' ') sc[iab] = '_';
    sl = Get_SortedPairList_Value_I(cnf.staz_abbrev,sc);
    if (NULL != sl) strcat(buf,sl);
    else strcat(buf,sc1);
    strcat(buf," - ");
  } while(sc = strtok(NULL,"*"));
  buf[strlen(buf)-3] = '\0';
  free(i1);
  Add_PairList(&v, "SOL_INSTRADA", buf);
//  Add_PairList(&v, "SOL_INSTRADA", r.instradamento);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "DT03.HTML", dt_template, v);
  Free_PairList(v);

  return 0;
}

int DT04(char **p, DETTAGLI r, INDATA id, char *querystring, ConfORARIO config,
         char *langsuff, char *langcode,
         PAIRLIST dt_template, IMGLIST service, IMGLIST train,
         int *n_servizi,  int   **servlist,
         int *n_treni,    int   **trenilist)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  TRENO trn;
  int err;
  int itrn, j;
  char buf[80];
  int isrv;
  char *gif, *alt;
  int k, ic, found;
  char buffer[1024];
  int itt;
  int indpage;

  *servlist  = (int *)NULL;
  *trenilist = (int *)NULL;
  *n_servizi  = 0;
  *n_treni    = 0;

  indpage = 1 + (r.ind_soluzione - 1) / config.max_sol;


  v = Set_PairList();
  sprintf(buffer,"stazin=%s&stazout=%s&datag=%s&datam=%s&dataa=%s&timsh=%s&timsm=%s&lang=%s&sort=%d&page=%d",
          id.stazione_p,id.stazione_a,id.giorno,id.mese,id.anno,id.ora,id.minuto,id.lingua,r.sort,indpage);
  /* EMS Modifica per trattamento query string
  for (k=0; k<strlen(buffer); k++) {
    if (' ' == buffer[k]) buffer[k] = '+';
  }
  */
  Add_PairList(&v, "LANG_SUFFIX", langsuff);
  Add_PairList(&v, "SOLUZIONI_HTTP", buffer);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "DT04A.HTML", dt_template, v);
  Free_PairList(v);


  for (itrn=0; itrn<r.num_treni; itrn++) {

    trn = *(r.treni + itrn);

    itt = trn.tipo;
    found = 0;
    for (k=0; k<*n_treni; k++) {
      if (itt == *((*trenilist)+k) ) {
        found = 1;
        break;
      }
    }
    if (0 == found) {
      *trenilist = (int *)realloc(*trenilist, (1 + *n_treni) * sizeof(int));
      *((*trenilist)+*n_treni) = itt;
      (*n_treni)++;
    }

    for (ic=0; ic<trn.num_servizi; ic++) {
      found = 0;
      for (k=0; k<*n_servizi; k++) {
        if (*(trn.servizi+ic) == *((*servlist)+k) ) {
          found = 1;
          break;
        }
      }
      if (0 == found) {
        *servlist = (int *)realloc(*servlist, (1 + *n_servizi) * sizeof(int));
        *((*servlist)+*n_servizi) = *(trn.servizi+ic);
        (*n_servizi)++;
      }
    }


    v = Set_PairList();
    itt = trn.tipo;
    Add_PairList(&v, "TRAIN_IMAGE", (train.imglist+itt)->gif);
    Add_PairList(&v, "TRAIN_ALT", (train.imglist+itt)->alt);
    Add_PairList(&v, "TRAIN_NAME", trn.nome);
    Add_PairList(&v, "TRAIN_ID", trn.numero);
    Add_PairList(&v, "STAZIONE_P", trn.stazione_p);
    Add_PairList(&v, "ORARIO_P", trn.orario_p);
    Add_PairList(&v, "STAZIONE_A", trn.stazione_a);
    Add_PairList(&v, "ORARIO_A", trn.orario_a);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "DT04B1A.HTML", dt_template, v);
    Free_PairList(v);

    for (j=0; j<trn.num_servizi; j++) {
      isrv = *(trn.servizi + j);
      gif = ((service.imglist)+isrv)->gif;
      if (0 != strlen(gif)) {
        v = Set_PairList();
        alt = ((service.imglist)+isrv)->alt;
        Add_PairList(&v, "SERVICE_IMAGE", gif);
        Add_PairList(&v, "SERVICE_ALT", alt);

        Sort_PairList_I_A(v);
        err = AddTemplateWithVars(p, "DT04B-B.HTML", dt_template, v);
        Free_PairList(v);
      }
    }

    err = AddTemplateWithoutVars(p, "DT04B-C.HTML", dt_template);

  }

  if (r.num_soluzioni == 1) {
    err = AddTemplateWithoutVars(p, "DT04C1.HTML", dt_template);
  }

  else if (r.num_soluzioni != 1 && r.ind_soluzione == 1) {
    v = Set_PairList();
    sprintf(buffer,"stazin=%s&stazout=%s&datag=%s&datam=%s&dataa=%s&timsh=%s&timsm=%s&lang=%s&sort=%d&det=%d",
            id.stazione_p,id.stazione_a,id.giorno,id.mese,id.anno,id.ora,id.minuto,id.lingua,r.sort,r.ind_soluzione+1);
    /* EMS Modifica per trattamento query string
    for (k=0; k<strlen(buffer); k++) {
      if (' ' == buffer[k]) buffer[k] = '+';
    }
    */
    Add_PairList(&v, "NEXT_SOL", buffer);
    sprintf(buffer,"%d",r.ind_soluzione);
    Add_PairList(&v, "IND_SOL", buffer);
    sprintf(buffer,"%d",r.num_soluzioni);
    Add_PairList(&v, "NUM_SOL", buffer);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "DT04C2.HTML", dt_template, v);
    Free_PairList(v);
  }

  else if (r.num_soluzioni != 1 && r.ind_soluzione == r.num_soluzioni) {
    v = Set_PairList();
    sprintf(buffer,"stazin=%s&stazout=%s&datag=%s&datam=%s&dataa=%s&timsh=%s&timsm=%s&lang=%s&sort=%d&det=%d",
            id.stazione_p,id.stazione_a,id.giorno,id.mese,id.anno,id.ora,id.minuto,id.lingua,r.sort,r.ind_soluzione-1);
    /* EMS Modifica per trattamento query string
    for (k=0; k<strlen(buffer); k++) {
      if (' ' == buffer[k]) buffer[k] = '+';
    }
    */
    Add_PairList(&v, "PREV_SOL", buffer);
    sprintf(buffer,"%d",r.ind_soluzione);
    Add_PairList(&v, "IND_SOL", buffer);
    sprintf(buffer,"%d",r.num_soluzioni);
    Add_PairList(&v, "NUM_SOL", buffer);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "DT04C4.HTML", dt_template, v);
    Free_PairList(v);
  }
  else {
    v = Set_PairList();
    sprintf(buffer,"stazin=%s&stazout=%s&datag=%s&datam=%s&dataa=%s&timsh=%s&timsm=%s&lang=%s&sort=%d&det=%d",
            id.stazione_p,id.stazione_a,id.giorno,id.mese,id.anno,id.ora,id.minuto,id.lingua,r.sort,r.ind_soluzione+1);
    /* EMS Modifica per trattamento query string
    for (k=0; k<strlen(buffer); k++) {
      if (' ' == buffer[k]) buffer[k] = '+';
    }
    */
    Add_PairList(&v, "NEXT_SOL", buffer);
    sprintf(buffer,"stazin=%s&stazout=%s&datag=%s&datam=%s&dataa=%s&timsh=%s&timsm=%s&lang=%s&sort=%d&det=%d",
            id.stazione_p,id.stazione_a,id.giorno,id.mese,id.anno,id.ora,id.minuto,id.lingua,r.sort,r.ind_soluzione-1);
    /* EMS Modifica per trattamento query string
    for (k=0; k<strlen(buffer); k++) {
      if (' ' == buffer[k]) buffer[k] = '+';
    }
    */
    Add_PairList(&v, "PREV_SOL", buffer);
    sprintf(buffer,"%d",r.ind_soluzione);
    Add_PairList(&v, "IND_SOL", buffer);
    sprintf(buffer,"%d",r.num_soluzioni);
    Add_PairList(&v, "NUM_SOL", buffer);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "DT04C3.HTML", dt_template, v);
    Free_PairList(v);
  }

  return 0;

}


int DT05(char **p, DETTAGLI r,
         int n_servizi, int *servlist,
         PAIRLIST dt_template, IMGLIST service)
{
  char *to, *t1, *t2;
  int k, i, j;
  int err, isrv;
  PAIRLIST v;
  char *gif, *alt, *descr;

  if (0 == n_servizi) return 0;

  err = AddTemplateWithoutVars(p, "DT05A.HTML", dt_template);

  for (k=0; k<n_servizi; k++) {
    isrv = *(servlist+k);
    gif = (service.imglist+isrv)->gif;
    if (0 != strlen(gif)) {
      alt = (service.imglist+isrv)->alt;
      descr = (service.imglist+isrv)->descr;
      v = Set_PairList();
      Add_PairList(&v, "SERVICE_IMAGE", gif);
      Add_PairList(&v, "SERVICE_ALT", alt);
      Add_PairList(&v, "SERVICE_DESCR", descr);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "DT05B.HTML", dt_template, v);
      Free_PairList(v);
    }
  }

  err = AddTemplateWithoutVars(p, "DT05C.HTML", dt_template);

  return 0;

}

int DT06(char **p, DETTAGLI r,
         int n_treni, int *trenilist,
         PAIRLIST dt_template, IMGLIST train)
{
  char *to, *t1, *t2;
  int k, i, j;
  int err, isrv;
  PAIRLIST v;
  char *gif, *alt, *descr;

  if (0 == n_treni) return 0;

  err = AddTemplateWithoutVars(p, "DT06A.HTML", dt_template);

  for (k=0; k<n_treni; k++) {
    isrv = *(trenilist+k);
    gif = (train.imglist+isrv)->gif;
    if (0 != strlen(gif)) {
      alt = (train.imglist+isrv)->alt;
      descr = (train.imglist+isrv)->descr;
      v = Set_PairList();
      Add_PairList(&v, "TRAIN_IMAGE", gif);
      Add_PairList(&v, "TRAIN_ALT", alt);
      Add_PairList(&v, "TRAIN_DESCR", descr);

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "DT06B.HTML", dt_template, v);
      Free_PairList(v);
    }
  }

  err = AddTemplateWithoutVars(p, "DT06C.HTML", dt_template);

  return 0;

}

int DT08(char **p, PAIRLIST dt_template)
{
  char *to;
  int err;

  err = AddTemplateWithoutVars(p, "DT08.HTML", dt_template);

  return 0;

}

int DT11(char **p, DETTAGLI r, ConfORARIO config, PAIRLIST dt_template)
{
  char *to, *t1, *t2;
  PAIRLIST v;
  char *url;
  char buf[512];
  int gg, mm, aa;
  char *b1, *b2, *b3, *b4, *b5, *b6, *b7, *b8;
  int i, err;
  TRENO t;
  POSTI z;

  url = Get_SortedPairList_Value_I(config.variables, "PREZZI_URL");
  if (NULL == url) return 0;

  v = Set_PairList();

  Add_PairList(&v, "STAZIN", r.stazione_p);
  Add_PairList(&v, "STAZOUT", r.stazione_a);
  sprintf(buf,"%d",r.giorno);
  Add_PairList(&v, "DATAG", buf);
  sprintf(buf,"%d",r.mese);
  Add_PairList(&v, "DATAM", buf);
  sprintf(buf,"%d",r.anno);
  Add_PairList(&v, "DATAA", buf);
  sprintf(buf,"%d",r.ora);
  Add_PairList(&v, "TIMSH", buf);
  sprintf(buf,"%d",r.minuto);
  Add_PairList(&v, "TIMSM", buf);

  sprintf(buf,"%d",r.ind_soluzione_orig);
  Add_PairList(&v, "IND_SOLUZIONE_ORIG", buf);

  sprintf(buf,"%d",r.num_treni);
  Add_PairList(&v, "N_TRENI", buf);

  if (0 != ParteDomani(r.ora, r.minuto, r.orario)) {
    Domani(r.giorno, r.mese, r.anno, &gg, &mm, &aa);
  }
  else {
    gg = r.giorno;
    mm = r.mese;
    aa = r.anno;
  }
  sprintf(buf,"%d/%d/%d",gg,mm,aa);
  Add_PairList(&v, "DATA_SOL", buf);

  Add_PairList(&v, "ORARIO_SOLUZIONE", r.orario);

  b1 = (char *)malloc(1);
  b2 = (char *)malloc(1);
  b3 = (char *)malloc(1);
  b4 = (char *)malloc(1);
  b5 = (char *)malloc(1);
  b6 = (char *)malloc(1);
  b7 = (char *)malloc(1);
  b8 = (char *)malloc(1);
  b1[0] = '\0';
  b2[0] = '\0';
  b3[0] = '\0';
  b4[0] = '\0';
  b5[0] = '\0';
  b6[0] = '\0';
  b7[0] = '\0';
  b8[0] = '\0';

  for (i=0; i<r.num_treni; i++) {
    t = *(r.treni + i);

    sprintf(buf,"%d",t.tipo);
    b1 = (char *)realloc(b1,strlen(b1) + strlen(buf) + 2);
    strcat(b1,buf);
    strcat(b1,"|");

    b2 = (char *)realloc(b2,strlen(b2) + strlen(t.numero) + 2);
    strcat(b2,t.numero);
    strcat(b2,"|");

    b3 = (char *)realloc(b3,strlen(b3) + strlen(t.nome) + 2);
    strcat(b3,t.nome);
    strcat(b3,"|");

    b4 = (char *)realloc(b4,strlen(b4) + strlen(t.stazione_p) + 2);
    strcat(b4,t.stazione_p);
    strcat(b4,"|");

    b5 = (char *)realloc(b5,strlen(b5) + strlen(t.stazione_a) + 2);
    strcat(b5,t.stazione_a);
    strcat(b5,"|");

    b6 = (char *)realloc(b6,strlen(b6) + strlen(t.orario_p) + 2);
    strcat(b6,t.orario_p);
    strcat(b6,"|");

    b7 = (char *)realloc(b7,strlen(b7) + strlen(t.orario_a) + 2);
    strcat(b7,t.orario_a);
    strcat(b7,"|");

    z = TipiPostiDisponibili(t.num_servizi, t.servizi);
    sprintf(buf,"%d %d %d %d %d %d %d %d %d",
                z.posti_1a, z.posti_2a, z.cuccette_1a, z.cuccette_2a,
                z.letti_1a, z.letti_2a,
                z.ristorante, z.prenotabilita, z.fumatori);
    b8 = (char *)realloc(b8,strlen(b8) + strlen(buf) + 2);
    strcat(b8,buf);
    strcat(b8,"|");
  }

  b1[strlen(b1)-1] = '\0';
  b2[strlen(b2)-1] = '\0';
  b3[strlen(b3)-1] = '\0';
  b4[strlen(b4)-1] = '\0';
  b5[strlen(b5)-1] = '\0';
  b6[strlen(b6)-1] = '\0';
  b7[strlen(b7)-1] = '\0';
  b8[strlen(b8)-1] = '\0';

  Add_PairList(&v, "TRENI_TIPI", b1);
  Add_PairList(&v, "TRENI_ID", b2);
  Add_PairList(&v, "TRENI_NOMI", b3);
  Add_PairList(&v, "TRENI_STAZIN", b4);
  Add_PairList(&v, "TRENI_STAZOUT", b5);
  Add_PairList(&v, "TRENI_ORARIOP", b6);
  Add_PairList(&v, "TRENI_ORARIOA", b7);
  Add_PairList(&v, "TRENI_POSTI", b8);
  free(b1);
  free(b2);
  free(b3);
  free(b4);
  free(b5);
  free(b6);
  free(b7);
  free(b8);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "DT11.HTML", dt_template, v);
  Free_PairList(v);

  return 0;
}

int DT21(char **p, DETTAGLI r, ConfORARIO config, PAIRLIST dt_template)
{
  char *to, *t1, *t2;
  PAIRLIST v;
  char *url;
  char buf[512];
  int gg, mm, aa;
  int g1, m1, a1;
  unsigned long dt1;
  int g2, m2, a2;
  unsigned long dt2;
  char *b1, *b2, *b3, *b4, *b5, *b6, *b7, *b8;
  char *sg, *sm, *sa;
  int i, err;
  TRENO t;
  POSTI z;
  int pren;

  url = Get_SortedPairList_Value_I(config.variables, "PRENOT_URL");
  if (NULL == url) return 0;

  pren = 0;
  for (i=0; i<r.num_treni; i++) {
    t = *(r.treni + i);
    z = TipiPostiDisponibili(t.num_servizi, t.servizi);
    if (0 != z.prenotabilita) {
      pren = 1;
      break;
    }
  }
  if (0 == pren) {
    err = AddTemplateWithoutVars(p, "DT21A.HTML", dt_template);
    return 0;
  }

  g1 = r.giorno;
  m1 = r.mese;
  a1 = r.anno;
  dt1 = 10000 * (a1 % 100) + 100 * m1 + g1;
  _strdate(buf);
  sm = strtok(buf,"/");
  sg = strtok(NULL,"/");
  sa = strtok(NULL,"/");
  g1 = atoi(sg);
  m1 = atoi(sm);
  a1 = atoi(sa);
  for (i=0; i<5; i++) {
    Domani(g1, m1, a1, &g2, &m2, &a2);
    g1 = g2;
    m1 = m2;
    a1 = a2;
  }
  dt2 = 10000 * (a2 % 100) + 100 * m2 + g2;
  if (dt1 < dt2) {
    v = Set_PairList();
    sprintf(buf,"%d/%d/%d",g2,m2,1900+(a2%100));
    Add_PairList(&v, "PRIMO_GIORNO_PREN", buf);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "DT21B.HTML", dt_template, v);
    Free_PairList(v);

    return 0;

  }



  v = Set_PairList();

  Add_PairList(&v, "STAZIN", r.stazione_p);
  Add_PairList(&v, "STAZOUT", r.stazione_a);
  sprintf(buf,"%d",r.giorno);
  Add_PairList(&v, "DATAG", buf);
  sprintf(buf,"%d",r.mese);
  Add_PairList(&v, "DATAM", buf);
  sprintf(buf,"%d",r.anno);
  Add_PairList(&v, "DATAA", buf);
  sprintf(buf,"%d",r.ora);
  Add_PairList(&v, "TIMSH", buf);
  sprintf(buf,"%d",r.minuto);
  Add_PairList(&v, "TIMSM", buf);

  sprintf(buf,"%d",r.ind_soluzione_orig);
  Add_PairList(&v, "IND_SOLUZIONE_ORIG", buf);

  sprintf(buf,"%d",r.num_treni);
  Add_PairList(&v, "N_TRENI", buf);

  if (0 != ParteDomani(r.ora, r.minuto, r.orario)) {
    Domani(r.giorno, r.mese, r.anno, &gg, &mm, &aa);
  }
  else {
    gg = r.giorno;
    mm = r.mese;
    aa = r.anno;
  }
  sprintf(buf,"%d/%d/%d",gg,mm,aa);
  Add_PairList(&v, "DATA_SOL", buf);

  Add_PairList(&v, "ORARIO_SOLUZIONE", r.orario);

  b1 = (char *)malloc(1);
  b2 = (char *)malloc(1);
  b3 = (char *)malloc(1);
  b4 = (char *)malloc(1);
  b5 = (char *)malloc(1);
  b6 = (char *)malloc(1);
  b7 = (char *)malloc(1);
  b8 = (char *)malloc(1);
  b1[0] = '\0';
  b2[0] = '\0';
  b3[0] = '\0';
  b4[0] = '\0';
  b5[0] = '\0';
  b6[0] = '\0';
  b7[0] = '\0';
  b8[0] = '\0';

  for (i=0; i<r.num_treni; i++) {
    t = *(r.treni + i);

    sprintf(buf,"%d",t.tipo);
    b1 = (char *)realloc(b1,strlen(b1) + strlen(buf) + 2);
    strcat(b1,buf);
    strcat(b1,"|");

    b2 = (char *)realloc(b2,strlen(b2) + strlen(t.numero) + 2);
    strcat(b2,t.numero);
    strcat(b2,"|");

    b3 = (char *)realloc(b3,strlen(b3) + strlen(t.nome) + 2);
    strcat(b3,t.nome);
    strcat(b3,"|");

    b4 = (char *)realloc(b4,strlen(b4) + strlen(t.stazione_p) + 2);
    strcat(b4,t.stazione_p);
    strcat(b4,"|");

    b5 = (char *)realloc(b5,strlen(b5) + strlen(t.stazione_a) + 2);
    strcat(b5,t.stazione_a);
    strcat(b5,"|");

    b6 = (char *)realloc(b6,strlen(b6) + strlen(t.orario_p) + 2);
    strcat(b6,t.orario_p);
    strcat(b6,"|");

    b7 = (char *)realloc(b7,strlen(b7) + strlen(t.orario_a) + 2);
    strcat(b7,t.orario_a);
    strcat(b7,"|");

    z = TipiPostiDisponibili(t.num_servizi, t.servizi);
    sprintf(buf,"%d %d %d %d %d %d %d %d %d",
                z.posti_1a, z.posti_2a, z.cuccette_1a, z.cuccette_2a,
                z.letti_1a, z.letti_2a,
                z.ristorante, z.prenotabilita, z.fumatori);
    b8 = (char *)realloc(b8,strlen(b8) + strlen(buf) + 2);
    strcat(b8,buf);
    strcat(b8,"|");
  }

  b1[strlen(b1)-1] = '\0';
  b2[strlen(b2)-1] = '\0';
  b3[strlen(b3)-1] = '\0';
  b4[strlen(b4)-1] = '\0';
  b5[strlen(b5)-1] = '\0';
  b6[strlen(b6)-1] = '\0';
  b7[strlen(b7)-1] = '\0';
  b8[strlen(b8)-1] = '\0';

  Add_PairList(&v, "TRENI_TIPI", b1);
  Add_PairList(&v, "TRENI_ID", b2);
  Add_PairList(&v, "TRENI_NOMI", b3);
  Add_PairList(&v, "TRENI_STAZIN", b4);
  Add_PairList(&v, "TRENI_STAZOUT", b5);
  Add_PairList(&v, "TRENI_ORARIOP", b6);
  Add_PairList(&v, "TRENI_ORARIOA", b7);
  Add_PairList(&v, "TRENI_POSTI", b8);
  free(b1);
  free(b2);
  free(b3);
  free(b4);
  free(b5);
  free(b6);
  free(b7);
  free(b8);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "DT21C.HTML", dt_template, v);
  Free_PairList(v);

  return 0;
}


int ER01(char **p, ERRORI r, INDATA id,
         char *langcode, PAIRLIST er_template, ERRMSG msg,
         int nerr, int *errcodes)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;

  err = AddTemplateWithoutVars(p, "ER01.HTML", er_template);

  return 0;

}

int ER02(char **p, ERRORI r, INDATA id,
         char *langcode, PAIRLIST er_template, ERRMSG msg,
         int nerr, int *errcodes)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;
  int k, ie;

  err = AddTemplateWithoutVars(p, "ER02A.HTML", er_template);

  for (k=0; k<nerr; k++) {
    ie = (*(k + errcodes)) - 1;
    v = Set_PairList();
    Add_PairList(&v, "ERR_MSG", *(msg.msg+ie));

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "ER02B.HTML", er_template, v);
    Free_PairList(v);
  }

  err = AddTemplateWithoutVars(p, "ER02C.HTML", er_template);

  return 0;

}

int ER03(char **p, ERRORI r, INDATA id,
         char *langcode, PAIRLIST er_template, ERRMSG msg,
         int nerr, int *errcodes)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;
  int k, ie;

  err = AddTemplateWithoutVars(p, "ER03A.HTML", er_template);

  if (r.stazione_p != 2) {
    v = Set_PairList();
    Add_PairList(&v, "STAZIONE_R_P", id.stazione_p);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "ER03B1A.HTML", er_template, v);
    Free_PairList(v);
  }
  else {
    err = AddTemplateWithoutVars(p, "ER03B2A.HTML", er_template);

    for (k=0; k<r.num_stazioni_p; k++) {
      v = Set_PairList();
      Add_PairList(&v, "STAZIONE_O_P", *(k + r.stazioni_p));

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "ER03B2B.HTML", er_template, v);
      Free_PairList(v);
    }

    err = AddTemplateWithoutVars(p, "ER03B2C.HTML", er_template);
  }

  err = AddTemplateWithoutVars(p, "ER03C.HTML", er_template);

  if (r.stazione_a != 2) {
    v = Set_PairList();
    Add_PairList(&v, "STAZIONE_R_A", id.stazione_a);

    Sort_PairList_I_A(v);
    err = AddTemplateWithVars(p, "ER03D1A.HTML", er_template, v);
    Free_PairList(v);
  }
  else {
    err = AddTemplateWithoutVars(p, "ER03D2A.HTML", er_template);

    for (k=0; k<r.num_stazioni_a; k++) {
      v = Set_PairList();
      Add_PairList(&v, "STAZIONE_O_A", *(k + r.stazioni_a));

      Sort_PairList_I_A(v);
      err = AddTemplateWithVars(p, "ER03D2B.HTML", er_template, v);
      Free_PairList(v);
    }

    err = AddTemplateWithoutVars(p, "ER03D2C.HTML", er_template);
  }


  v = Set_PairList();
  Add_PairList(&v, "DATA_G", id.giorno);
  Add_PairList(&v, "DATA_M", id.mese);
  Add_PairList(&v, "DATA_A", id.anno);
  Add_PairList(&v, "ORA_H", id.ora);
  Add_PairList(&v, "ORA_M", id.minuto);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "ER03E.HTML", er_template, v);
  Free_PairList(v);


  return 0;

}

int ER04(char **p, ERRORI r, INDATA id,
         char *langsuff, PAIRLIST er_template, ERRMSG msg,
         int nerr, int *errcodes)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;
  int ie;

  v = Set_PairList();
  Add_PairList(&v, "LANG_SUFFIX", langsuff);

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "ER04.HTML", er_template, v);
  Free_PairList(v);

  return 0;

}

int ET01(char **p, ERRORI r, INDATA id,
         char *langcode, PAIRLIST et_template, ERRMSG msg,
         int nerr, int *errcodes)
{
  char *t1, *t2, *to;
  PAIRLIST v;
  int err;
  int ie;

  v = Set_PairList();
  ie = (*(errcodes)) - 1;
  Add_PairList(&v, "ERR_MSG", *(msg.msg+ie));

  Sort_PairList_I_A(v);
  err = AddTemplateWithVars(p, "ET01.HTML", et_template, v);
  Free_PairList(v);

  return 0;

}

int ET02(char **p, ERRORI r, INDATA id,
         char *langcode, PAIRLIST et_template, ERRMSG msg,
         int nerr, int *errcodes)
{
  int err;

  if (1 == *errcodes) return 0;

  err = AddTemplateWithoutVars(p, "ET02.HTML", et_template);

  return 0;

}

int ET03(char **p, ERRORI r, INDATA id,
         char *langcode, PAIRLIST et_template, ERRMSG msg,
         int nerr, int *errcodes)
{
  int err;

  err = AddTemplateWithoutVars(p, "ET03.HTML", et_template);

  return 0;

}


int AddTemplateWithoutVars(char **page, char *templ_name,
                           PAIRLIST templ_set)
{
  char *to;

  *page = (char *)realloc(*page, 1 + strlen(*page) + strlen(templ_name) + 9);
  strcat(*page,"<!-- ");
  strcat(*page,templ_name);
  strcat(*page," -->");

  to = Get_SortedPairList_Value_I(templ_set,templ_name);
  if (NULL == to) return 1;

  *page = (char *)realloc(*page, strlen(*page) + strlen(to) + 1);
  strcat(*page,to);

  return 0;

}

int AddTemplateWithVars(char **page, char *templ_name,
                        PAIRLIST templ_set, PAIRLIST vars)
{
  char *to, *t1, *t2;
  int err;

  *page = (char *)realloc(*page, 1 + strlen(*page) + strlen(templ_name) + 9);
  strcat(*page,"<!-- ");
  strcat(*page,templ_name);
  strcat(*page," -->");

  to = Get_SortedPairList_Value_I(templ_set,templ_name);
  if (NULL == to) return 1;

  t1 = (char *)malloc(strlen(to)+1);
  strcpy(t1,to);
  t2 = ConfigSubstitute(t1, vars, "$", "$", 1, &err);
  *page = (char *)realloc(*page, strlen(*page) + strlen(t2) + 1);
  strcat(*page,t2);
  free(t2);

  return err;

}



int ParteDomani(int ora, int minuto, char *parte)
{
  char *p, *q, *t;
  int ph, pm;

  p = q = (char *)malloc(1 + strlen(parte));
  strcpy(p,parte);
  t = strtok(p,":");
  ph = atoi(t);
  t = strtok(NULL,":");
  pm = atoi(t);
  if ( (100*ora + minuto) > (100*ph + pm) ) return 1;
  return 0;
}


void Domani(int g1, int m1, int a1, int *g2, int *m2, int *a2)
{
  int lm[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (0 != a1 % 4) lm[1] = 28;
  else lm[1] = 29;

  if (g1 < lm[m1-1])
  {
     *g2 = g1 + 1;
     *m2 = m1;
     *a2 = a1;
  }
  else
  {
     *g2 = 1;
     if (m1 < 12)
     {
        *m2 = m1 + 1;
        *a2 = a1;
     }
     else
     {
        *m2 = 1;
        *a2 = a1 + 1;
     } /* endif */
  } /* endif */
}

void TraUnMinuto(int dg1, int dm1, int da1, int th1, int tm1,
                 int *dg2, int *dm2, int *da2, int *th2, int *tm2)
{
   if (tm1 < 59)
   {
      *tm2 = tm1 + 1;
      *th2 = th1;
      *dg2 = dg1;
      *dm2 = dm1;
      *da2 = da1;
   }
   else
   {
      *tm2 = 0;
      if (th1 < 23)
      {
         *th2 = th1 + 1;
         *dg2 = dg1;
         *dm2 = dm1;
         *da2 = da1;
      }
      else
      {
         *th2 = 0;
         Domani(dg1,dm1,da1, dg2,dm2,da2);
      } /* endif */
   } /* endif */
}

void prtpairlist(char *file,PAIRLIST pl, char *msg)
{
  FILE *logfile;
  int k;
  logfile=fopen(file,"a");
  fprintf(logfile,"%s\n",msg);
  for (k=0; k<pl.count; k++) {
    fprintf(logfile,"%d. n=%s, v=%s\n",k,(pl.pairlist+k)->name,(pl.pairlist)->value);
  }
  fclose(logfile);
}
