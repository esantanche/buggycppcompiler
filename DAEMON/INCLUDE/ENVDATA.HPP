/* This file is included only in environ.cpp because it generates
 * code. It contains the table of the tokens for parsing the cfgFile.
 */

enum cfgTokens { QUERYFORM_TOK, QUERYURL_TOK, QUERYTEMPLATE_TOK,
                 DETAILDIR_TOK, DETAILERR_TOK
};

static int getKeywordToken(IString& keywString);

typedef struct _cfgPairs {
          char *keyword;
          int token;
} CFGPAIRS;


static CFGPAIRS cfgVect[]={
                   "FS_CGIQUERYFORM"     ,      QUERYFORM_TOK       ,
                   "FS_CGIQUERYURL"      ,      QUERYURL_TOK        ,
                   "FS_CGIQUERYTEMPLATE" ,      QUERYTEMPLATE_TOK   ,
                   "FS_DETAILDIR"        ,      DETAILDIR_TOK       ,
                   "FS_DETAILERRORURL"   ,      DETAILERR_TOK
};
                                      


static int getKeywordToken(IString& keywString)
{
   for(int i=0; i<(sizeof(cfgVect)/sizeof(cfgVect[0])) ; i++)
       if (keywString==cfgVect[i].keyword) return cfgVect[i].token;

   return -1;
}


