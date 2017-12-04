#ifndef ASFSUTIL
#include "asfsutil.h"
#endif

#ifndef ASCONFIG
#define ASCONFIG

typedef struct _errmsg
  {
    int       count;
    char    **msg;
  } ERRMSG;

typedef struct _img
  {
    char     *gif;
    char     *alt;
    char     *descr;
  } IMG;

typedef struct _imglist
  {
    int       count;
    IMG      *imglist;
  } IMGLIST;

typedef struct _conforario
  {
    int       max_sol;
    PAIRLIST  languages;
    PAIRLIST  variables;
    PAIRLIST *sl_template;
    PAIRLIST *dt_template;
    PAIRLIST *er_template;
    PAIRLIST *et_template;

    IMGLIST  *service_file;
    IMGLIST  *train_file;
    ERRMSG   *error_msg;
    PAIRLIST  staz_abbrev;
  } ConfORARIO;

PAIRLIST ReadConfig(char *);
int ParseConfigLine(char *, char **, char **);
PAIRLIST ReadTemplate(char *, char *, char *);
int ReadConfig_Orario(ConfORARIO *);
void FreeConfig_Orario(ConfORARIO);

#endif
