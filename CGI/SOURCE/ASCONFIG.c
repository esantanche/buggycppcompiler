#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include "asfsutil.h"
#include "asconfig.h"

IMGLIST ReadImgList(char *, char);
ERRMSG ReadErrMsg(char *);

void Write_Q5_Log(char *);

int ReadConfig_Orario(ConfORARIO *conf)
{
  PAIRLIST configdata;
  char *etc;
  char *fileconf, *filetemp;
  int i, j, found;
  char *nm, *vl;
  int n_lang;
  ConfORARIO cnf;
  PAIRLIST templ;
  IMGLIST imgl;
  ERRMSG errl;
  char *sl_template, *dt_template, *er_template, *et_template;
  char *service_file, *train_file, *error_msg;
  char *staz_abbrev;
  int n_sl, n_dt, n_er, n_et, n_sv, n_tr, n_em;

  int nmandatory = 23;
  char *mandatory[] = {
                        "MAX_SOL",
                        "N_LANG",
                        "IMAGE_PATH",
                        "BACKGROUND_IMAGE",
                        "NEW_SEARCH_IMAGE",
                        "DETAIL_IMAGE",
                        "HELP_IMAGE",
                        "SOLUZIONI_IMAGE",
                        "PREV_SOL_IMAGE",
                        "NEXT_SOL_IMAGE",
                        "SORT_IMAGE",
                        "GOTOP_IMAGE",
                        "SEARCH_HTTP",
                        "HELP_PATH",
                        "HELP_PREFIX",
                        "SL_TEMPLATE",
                        "DT_TEMPLATE",
                        "ER_TEMPLATE",
                        "ET_TEMPLATE",
                        "SERVICE_FILE",
                        "TRAIN_FILE",
                        "ERROR_MSG",
                        "MY_HTTP",
                      };

// Get ETC variable
  etc = getenv("ETC");
  if (NULL == etc) return 1;
  fileconf = (char *)malloc(14+strlen(etc));
  strcpy(fileconf,etc);
  strcat(fileconf,"\\fsorario.cnf");

// Read configuration file in a PAIRLIST
  configdata = ReadConfig(fileconf);
  free(fileconf);
  if (0 == configdata.count) {
    return 2;
  }

// Check presence of mandatory keywords
  for (i=0; i<nmandatory; i++) {
    found = 0;
    for (j=0; j<configdata.count; j++) {
      if (0 == strcmpi(mandatory[i], ((configdata.pairlist)+j)->name)) {
        found = 1;
        break;
      }
    }
    if (0 == found) {
      Free_PairList(configdata);
      return 3;
    }
  }

// Initialization
  cnf.variables = Set_PairList();
  cnf.languages = Set_PairList();
  cnf.staz_abbrev = Set_PairList();
  n_sl = 0;
  n_dt = 0;
  n_er = 0;
  n_et = 0;
  n_sv = 0;
  n_tr = 0;
  n_em = 0;
  staz_abbrev = NULL;


  for (i=0; i<configdata.count; i++) {
    nm = ((configdata.pairlist)+i)->name;
    vl = ((configdata.pairlist)+i)->value;

// Set search maximum number of solutions to a large number
    if (0 == strcmpi(nm, "MAX_SOL")) {
      cnf.max_sol = atoi(vl);
      if (cnf.max_sol <= 0) cnf.max_sol = 1000000;
    }

// If keywork is N_LANG set a PAIRLIST to couples 'langsuffix, landcode'
    else if (0 == strcmpi(nm, "N_LANG")) {
      n_lang = atoi(vl);
      if (n_lang <= 0) goto ErrExit;
      if ((i + n_lang) >= configdata.count) goto ErrExit;
      cnf.languages = Set_PairList();
      for (j=0; j<n_lang; j++) {
        Add_PairList(&(cnf.languages), ((configdata.pairlist)+i+j+1)->name,
                                      ((configdata.pairlist)+i+j+1)->value );
      }
      i = i + n_lang;
    }

// Prefix for Solutions page template files
    else if (0 == strcmpi(nm, "SL_TEMPLATE")) {
      sl_template = vl;
    }

// Prefix for Details page template files
    else if (0 == strcmpi(nm, "DT_TEMPLATE")) {
      dt_template = vl;
    }

// Prefix for Error with form page template files
    else if (0 == strcmpi(nm, "ER_TEMPLATE")) {
      er_template = vl;
    }

// Prefix for Error without form page template files
    else if (0 == strcmpi(nm, "ET_TEMPLATE")) {
      et_template = vl;
    }

// Prefix for file containing image, alt. text and dscription for services
    else if (0 == strcmpi(nm, "SERVICE_FILE")) {
      service_file = vl;
    }

// Prefix for file containig image, alt. text and dscription for train types
    else if (0 == strcmpi(nm, "TRAIN_FILE")) {
      train_file = vl;
    }

// Prefix for file containing error messages
    else if (0 == strcmpi(nm, "ERROR_MSG")) {
      error_msg = vl;
    }

    else if (0 == strcmpi(nm, "STAZ_ABBREV")) {
      staz_abbrev = vl;
    }

// Otherwise it's a configurarion variable
    else {
      Add_PairList(&(cnf.variables),nm,vl);
    }
  }

// Read Solutions template files for each language
  cnf.sl_template = (PAIRLIST *)NULL;
  for (i=0; i<n_lang; i++) {
    filetemp = (char *)malloc(strlen(sl_template) + strlen( (((cnf.languages).pairlist)+i)->value ) + 2);
    strcpy(filetemp,sl_template);
    strcat(filetemp,".");
    strcat(filetemp,(((cnf.languages).pairlist)+i)->value);
    templ = ReadTemplate(filetemp,"$","$");
    free(filetemp);
    if ( templ.count == 0) goto ErrExit;
    n_sl++;
    cnf.sl_template = (PAIRLIST *)realloc(cnf.sl_template,n_sl*sizeof(PAIRLIST));
    *(cnf.sl_template+i) = templ;
  }

// Read Details template files for each language
  cnf.dt_template = (PAIRLIST *)NULL;
  for (i=0; i<n_lang; i++) {
    filetemp = (char *)malloc(strlen(dt_template) + strlen( (((cnf.languages).pairlist)+i)->value ) + 2);
    strcpy(filetemp,dt_template);
    strcat(filetemp,".");
    strcat(filetemp,(((cnf.languages).pairlist)+i)->value);
    templ = ReadTemplate(filetemp,"$","$");
    free(filetemp);
    if ( templ.count == 0) goto ErrExit;
    n_dt++;
    cnf.dt_template = (PAIRLIST *)realloc(cnf.dt_template,n_dt*sizeof(PAIRLIST));
    *(cnf.dt_template+i) = templ;
  }

// Read Errors with form template files for each language
  cnf.er_template = (PAIRLIST *)NULL;
  for (i=0; i<n_lang; i++) {
    filetemp = (char *)malloc(strlen(er_template) + strlen( (((cnf.languages).pairlist)+i)->value ) + 2);
    strcpy(filetemp,er_template);
    strcat(filetemp,".");
    strcat(filetemp,(((cnf.languages).pairlist)+i)->value);
    templ = ReadTemplate(filetemp,"$","$");
    free(filetemp);
    if ( templ.count == 0) goto ErrExit;
    n_er++;
    cnf.er_template = (PAIRLIST *)realloc(cnf.er_template,n_er*sizeof(PAIRLIST));
    *(cnf.er_template+i) = templ;
  }

// Read Errors without form template files for each language
  cnf.et_template = (PAIRLIST *)NULL;
  for (i=0; i<n_lang; i++) {
    filetemp = (char *)malloc(strlen(et_template) + strlen( (((cnf.languages).pairlist)+i)->value ) + 2);
    strcpy(filetemp,et_template);
    strcat(filetemp,".");
    strcat(filetemp,(((cnf.languages).pairlist)+i)->value);
    templ = ReadTemplate(filetemp,"$","$");
    free(filetemp);
    if ( templ.count == 0) goto ErrExit;
    n_et++;
    cnf.et_template = (PAIRLIST *)realloc(cnf.et_template,n_et*sizeof(PAIRLIST));
    *(cnf.et_template+i) = templ;
  }

// Read Service files for each language
  cnf.service_file = (IMGLIST *)NULL;
  for (i=0; i<n_lang; i++) {
    filetemp = (char *)malloc(strlen(service_file) + strlen( (((cnf.languages).pairlist)+i)->value ) + 2);
    strcpy(filetemp,service_file);
    strcat(filetemp,".");
    strcat(filetemp,(((cnf.languages).pairlist)+i)->value);
    imgl = ReadImgList(filetemp,'');
    free(filetemp);
    if ( imgl.count == 0) goto ErrExit;
    n_sv++;
    cnf.service_file = (IMGLIST *)realloc(cnf.service_file,n_sv*sizeof(IMGLIST));
    *(cnf.service_file+i) = imgl;
  }

// Read Train type files for each language
  cnf.train_file = (IMGLIST *)NULL;
  for (i=0; i<n_lang; i++) {
    filetemp = (char *)malloc(strlen(train_file) + strlen( (((cnf.languages).pairlist)+i)->value ) + 2);
    strcpy(filetemp,train_file);
    strcat(filetemp,".");
    strcat(filetemp,(((cnf.languages).pairlist)+i)->value);
    imgl = ReadImgList(filetemp,'');
    free(filetemp);
    if ( imgl.count == 0) goto ErrExit;
    n_tr++;
    cnf.train_file = (IMGLIST *)realloc(cnf.train_file,n_tr*sizeof(IMGLIST));
    *(cnf.train_file+i) = imgl;
  }

// Read Error messages type files for each language
  cnf.error_msg = (ERRMSG *)NULL;
  for (i=0; i<n_lang; i++) {
    filetemp = (char *)malloc(strlen(error_msg) + strlen( (((cnf.languages).pairlist)+i)->value ) + 2);
    strcpy(filetemp,error_msg);
    strcat(filetemp,".");
    strcat(filetemp,(((cnf.languages).pairlist)+i)->value);
    errl = ReadErrMsg(filetemp);
    free(filetemp);
    if ( errl.count == 0) goto ErrExit;
    n_em++;
    cnf.error_msg = (ERRMSG *)realloc(cnf.error_msg,n_em*sizeof(ERRMSG));
    *(cnf.error_msg+i) = errl;
  }

  if (NULL != staz_abbrev) {
    cnf.staz_abbrev = ReadConfig(staz_abbrev);
    Sort_PairList_I_A(cnf.staz_abbrev);
  }

// Free temporary configuration data
  *conf = cnf;
  Free_PairList(configdata);

// Sort configuarion variable for searching
  Sort_PairList_I_A(cnf.variables);

  return 0;

// If an error occurred free allocated memory
ErrExit:
  Free_PairList(cnf.variables);
  Free_PairList(cnf.languages);

  for (i=0; i<n_sl; i++) Free_PairList(*(cnf.sl_template+i));
  if (0 != n_dt) free(cnf.sl_template);

  for (i=0; i<n_dt; i++) Free_PairList(*(cnf.dt_template+i));
  if (0 != n_dt) free(cnf.dt_template);

  for (i=0; i<n_er; i++) Free_PairList(*(cnf.er_template+i));
  if (0 != n_er) free(cnf.er_template);

  for (i=0; i<n_et; i++) Free_PairList(*(cnf.et_template+i));
  if (0 != n_et) free(cnf.et_template);

/*  *********** COMM.TO 03/03/97 RS
** for (i=0; i<n_sv; i++) {
**   imgl = *(cnf.service_file);
**   for (j=0; j<imgl.count; j++) {
**     free( ((imgl.imglist)+j)->gif );
**     free( ((imgl.imglist)+j)->alt );
**     free( ((imgl.imglist)+j)->descr );
**   }
** }
** if (0 != n_sv) free(cnf.service_file);
**
** for (i=0; i<n_tr; i++) {
**   imgl = *(cnf.train_file);
**   for (j=0; j<imgl.count; j++) {
**     free( ((imgl.imglist)+j)->gif );
**     free( ((imgl.imglist)+j)->alt );
**     free( ((imgl.imglist)+j)->descr );
**   }
** }
** if (0 != n_tr) free(cnf.train_file);
**
** for (i=0; i<n_em; i++) {
**   errl = *(cnf.error_msg);
**   for (j=0; j<errl.count; j++) {
**     free( *((errl.msg)+j) );
**   }
** }
** if (0 != n_em) free(cnf.error_msg);
*******************************************************/

  for (i=n_sv; i>0; i--) {
    imgl = *(cnf.service_file+(i-1));
    for (j=imgl.count; j>0; j--) {
      free( ((imgl.imglist)+(j-1))->gif );
      free( ((imgl.imglist)+(j-1))->alt );
      free( ((imgl.imglist)+(j-1))->descr );
    }
    free(cnf.service_file+(i-1));
  }

  for (i=n_tr; i>0; i--) {
    imgl = *(cnf.train_file+(i-1));
    for (j=imgl.count; j>0 ;j--) {
      free( ((imgl.imglist)+(j-1))->gif );
      free( ((imgl.imglist)+(j-1))->alt );
      free( ((imgl.imglist)+(j-1))->descr );
    }
    free(cnf.train_file+(i-1));
  }

  for (i=n_em; i>0; i--) {
    errl = *(cnf.error_msg+(i-1));
    for (j=errl.count; j>0; j--) {
      free( *((errl.msg)+(j-1)) );
    }
    free(cnf.error_msg+(i-1));
  }

  Free_PairList(configdata);
  return 4;
}

PAIRLIST ReadConfig(char *fileid)
{
  FILE *fc;
  char *line, *var, *value;
  int rc, ret;
  PAIRLIST config;

  config = Set_PairList();

  fc = fopen(fileid,"r");
  if (NULL == fc) return config;

// For each line of configuration file set a PAIR with name=keyword (1.st word)
// and value=rest of yhe line
  line = (char *)NULL;
  while (0 >= (rc = linein(fc,&line,0))) {
    ret = ParseConfigLine(line, &var, &value);
    if (0 == ret) Add_PairList(&config, var, value);
    if (NULL != var) free(var);
    if (NULL != value) free(value);
    free(line);
    line = (char *)NULL;
  }
  fclose(fc);

  return config;
}

int ParseConfigLine(char *line, char **var, char **value)
{
  size_t ips, lv;
  char *ps, *pe, *p;

  *var = NULL;
  *value = NULL;

  if ('#' == *line) return 1;

  ips = strspn(line," ");
  if (ips == strlen(line)) return 2;

  ps = line + ips;
  pe = strchr(ps,' ');
  if (NULL == pe) {
    *var = (char *)malloc(1+strlen(ps));
    strcpy(*var,ps);
    *value = (char *)malloc(1);
    *(*value) = '\0';
    return 0;
  }

  lv = pe - ps;
  *var = (char *)malloc(1+lv);
  strncpy(*var,ps,lv);
  *((*var)+lv) = '\0';

  ips = strspn(pe," ");
  if (ips == strlen(pe)) {
    *value = (char *)malloc(1);
    *(*value) = '\0';
    return 0;
  }

  ps = pe + ips;

  for (p=ps+strlen(ps)-1; p>=ps; p--) {
    if (' ' != *p) {
      pe = p + 1;
      break;
    }
  }

  lv = pe - ps;
  *value = (char *)malloc(1+lv);
  strncpy(*value,ps,lv);
  *((*value)+lv) = '\0';

  return 0;
}

PAIRLIST ReadTemplate(char *fileid, char *strsep, char *endsep)
{
  PAIRLIST templ;
  FILE *fc;
  int fn;
  long nchars, nretc;
  char *buffer, *name, *value;
  char *p, *psn, *pen, *psv, *pev;
  int ln, lv;
  int lss, les;

  templ = Set_PairList();

  fc = fopen(fileid,"r");
  if (NULL == fc) return templ;

  fn = fileno(fc);
  nchars = filelength(fn);
  buffer = (char *)malloc(nchars+1);
  nretc = read(fn, buffer, nchars);
  if (nretc <= 0) {
    free(buffer);
    fclose(fc);
    return templ;
  }
  buffer[nretc] = '\0';

  p = buffer;
  lss = strlen(strsep);
  les = strlen(endsep);
  for (;;) {
    psn = strstr(p,strsep);
    if (NULL == psn) break;
    pen = strstr(p,endsep);
    ln = pen - psn - lss;
    name = (char *)malloc(ln + 1);
    strncpy(name,psn+lss,ln);
    name[ln] = '\0';

    psv = pen + 1 + les;

    pev = strstr(psv,strsep);
    if (NULL == pev) pev = p + strlen(p);
    lv = pev - psv;
    value = (char *)malloc(lv + 1);
    strncpy(value,psv,lv);
    value[lv] = '\0';

    Add_PairList(&templ,name,value);
    free(name);
    free(value);

    p = pev;
  }

  Sort_PairList_I_A(templ);

  free(buffer);
  fclose(fc);
  return templ;
}

IMGLIST ReadImgList(char *fileid, char sep)
{
  FILE *fc;
  IMGLIST il;
  char *line, *ps, *pe;
  int rc;
  int n, l;

  il.count = 0;
  il.imglist = NULL;

  fc = fopen(fileid,"r");
  if (NULL == fc) return il;

  line = (char *)NULL;
  while (0 >= (rc = linein(fc,&line,0))) {
    if ('#' != *line) {
      n = il.count;
      il.imglist = (IMG *)realloc(il.imglist,(n+1)*sizeof(IMG));
      ((il.imglist)+n)->gif = (char *)malloc(1);
      ((il.imglist)+n)->alt = (char *)malloc(1);
      ((il.imglist)+n)->descr = (char *)malloc(1);
      *(((il.imglist)+n)->gif) = '\0';
      *(((il.imglist)+n)->alt) = '\0';
      *(((il.imglist)+n)->descr) = '\0';
      il.count = n+1;

      ps = line;
      pe = strchr(ps,sep);
      if (NULL != pe) {
        l = pe - ps;
        ((il.imglist)+n)->gif = (char *)realloc(((il.imglist)+n)->gif, l + 1);
        strncpy( ((il.imglist)+n)->gif, ps, l);
        *((((il.imglist)+n)->gif)+l) = '\0';

        ps = pe + 1;
        if (0 != strlen(ps)) {
          pe = strchr(ps,sep);
          if (NULL != pe) {
            l = pe - ps;
            ((il.imglist)+n)->alt = (char *)realloc(((il.imglist)+n)->alt, l + 1);
            strncpy( ((il.imglist)+n)->alt, ps, l);
            *((((il.imglist)+n)->alt)+l) = '\0';

            ps = pe + 1;
            if (0 != strlen(ps)) {
              l = strlen(ps);
              ((il.imglist)+n)->descr = (char *)realloc(((il.imglist)+n)->descr, l + 1);
              strncpy( ((il.imglist)+n)->descr, ps, l);
              *((((il.imglist)+n)->descr)+l) = '\0';
            }
          }
        }
      }
    }
    free(line);
    line = (char *)NULL;
  }
  fclose(fc);

  return il;
}

ERRMSG ReadErrMsg(char *fileid)
{
  FILE *fc;
  ERRMSG em;
  char *line;
  int rc;
  int n;

  em.count = 0;
  em.msg = NULL;

  fc = fopen(fileid,"r");
  if (NULL == fc) return em;

  line = (char *)NULL;
  while (0 >= (rc = linein(fc,&line,0))) {
    if ('#' != *line) {
      n = em.count;
      em.msg = (char **)realloc(em.msg,(n+1)*sizeof(char *));
      *((em.msg)+n) = (char *)malloc(strlen(line)+1);
      strcpy(*((em.msg)+n), line);
      em.count = n+1;
    }
    free(line);
    line = (char *)NULL;
  }
  fclose(fc);

  return em;
}

void FreeConfig_Orario(ConfORARIO config)
{
  int n_lang, i, j;
  IMGLIST imgl;
  ERRMSG errl;


  n_lang = Get_PairList_Count(config.languages);

  Free_PairList(config.variables);
  Free_PairList(config.languages);

  for (i=0; i<n_lang; i++) {
    Free_PairList(*(config.sl_template+i));
  }
  free(config.sl_template);

  for (i=0; i<n_lang; i++) {
    Free_PairList(*(config.dt_template+i));
  }

  free(config.dt_template);

  for (i=0; i<n_lang; i++) Free_PairList(*(config.er_template+i));
  free(config.er_template);

  for (i=0; i<n_lang; i++) Free_PairList(*(config.et_template+i));
  free(config.et_template);

/******************FOR modificato - 03/03/97 RS*/
  for (i=n_lang; i>0; i--) {
    imgl = *(config.service_file+(i-1));
    for (j=imgl.count; j>0; j--) {
      free( ((imgl.imglist)+(j-1))->gif );
      free( ((imgl.imglist)+(j-1))->alt );
      free( ((imgl.imglist)+(j-1))->descr );
    }
    free(config.service_file+(i-1));
  }

  for (i=n_lang; i>0; i--) {
    imgl = *(config.train_file+(i-1));
    for (j=imgl.count; j>0 ;j--) {
      free( ((imgl.imglist)+(j-1))->gif );
      free( ((imgl.imglist)+(j-1))->alt );
      free( ((imgl.imglist)+(j-1))->descr );
    }
    free(config.train_file+(i-1));
  }

  for (i=n_lang; i>0; i--) {
    errl = *(config.error_msg+(i-1));
    for (j=errl.count; j>0; j--) {
      free( *((errl.msg)+(j-1)) );
    }
    free(config.error_msg+(i-1));
  }
/********************end mod RS*********/
  return;

}


int confprt(char *fileid, ConfORARIO conf)
{
  FILE *fc;
  int i, j;
  int nlang;
  PAIRLIST pl;
  IMGLIST il;
  ERRMSG em;

  fc = fopen(fileid,"a");
  if (NULL == fc) return 1;

  fprintf(fc,"max_sol=%d\n",conf.max_sol);

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"languages\n");
  nlang = Get_PairList_Count(conf.languages);
  for (i=1; i<Get_PairList_Count(conf.languages); i++) {
     fprintf(fc,"%d. code=%s, suffix=%s\n",
                i,Get_PairList_Name_by_Index(conf.languages,i),
                Get_PairList_Value_by_Index(conf.languages,i));
  }

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"variables\n");
  for (i=1; i<Get_PairList_Count(conf.variables); i++) {
     fprintf(fc,"%d. var=%s, val=%s\n",
                i,Get_PairList_Name_by_Index(conf.variables,i),
                Get_PairList_Value_by_Index(conf.variables,i));
  }

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"sl_templates\n");
  for (i=0; i<nlang; i++) {
     fprintf(fc,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
     fprintf(fc,"sl_template #%d\n",i);
     pl = *(conf.sl_template + 1);
     for (j=0; j<Get_PairList_Count(pl); j++) {
        fprintf(fc,"%d. code=%s, html=\n%s",
                   j,Get_PairList_Name_by_Index(pl,j),
                   Get_PairList_Value_by_Index(pl,j));
     }
  }

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"dt_templates\n");
  for (i=0; i<nlang; i++) {
     fprintf(fc,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
     fprintf(fc,"dt_template #%d\n",i);
     pl = *(conf.dt_template + 1);
     for (j=0; j<Get_PairList_Count(pl); j++) {
        fprintf(fc,"%d. code=%s, html=\n%s",
                   j,Get_PairList_Name_by_Index(pl,j),
                   Get_PairList_Value_by_Index(pl,j));
     }
  }

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"er_templates\n");
  for (i=0; i<nlang; i++) {
     fprintf(fc,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
     fprintf(fc,"er_template #%d\n",i);
     pl = *(conf.er_template + 1);
     for (j=0; j<Get_PairList_Count(pl); j++) {
        fprintf(fc,"%d. code=%s, html=\n%s",
                   j,Get_PairList_Name_by_Index(pl,j),
                   Get_PairList_Value_by_Index(pl,j));
     }
  }

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"et_templates\n");
  for (i=0; i<nlang; i++) {
     fprintf(fc,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
     fprintf(fc,"et_template #%d\n",i);
     pl = *(conf.et_template + 1);
     for (j=0; j<Get_PairList_Count(pl); j++) {
        fprintf(fc,"%d. code=%s, html=\n%s",
                   j,Get_PairList_Name_by_Index(pl,j),
                   Get_PairList_Value_by_Index(pl,j));
     }
  }

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"services_files\n");
  for (i=0; i<nlang; i++) {
     fprintf(fc,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
     fprintf(fc,"service_file #%d\n",i);
     il = *(conf.service_file + 1);
     for (j=0; j<il.count; j++) {
        fprintf(fc,"%d. gif=%s, alt=%s, descr=%s\n",
                   j,((il.imglist)+j)->gif,
                   ((il.imglist)+j)->alt,
                   ((il.imglist)+j)->descr);
     }
  }

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"train_files\n");
  for (i=0; i<nlang; i++) {
     fprintf(fc,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
     fprintf(fc,"train_file #%d\n",i);
     il = *(conf.train_file + 1);
     for (j=0; j<il.count; j++) {
        fprintf(fc,"%d. gif=%s, alt=%s, descr=%s\n",
                   j,((il.imglist)+j)->gif,
                   ((il.imglist)+j)->alt,
                   ((il.imglist)+j)->descr);
     }
  }

  fprintf(fc,"---------------------------------------------------------\n");
  fprintf(fc,"error_msgs\n");
  for (i=0; i<nlang; i++) {
     fprintf(fc,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
     fprintf(fc,"error_msg #%d\n",i);
     em = *(conf.error_msg + 1);
     for (j=0; j<em.count; j++) {
        fprintf(fc,"%d. msg=%s\n",
                   j,*((em.msg)+j));
     }
  }

  fclose(fc);

  return 0;
}
