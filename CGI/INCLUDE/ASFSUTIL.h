#ifndef ASFSUTIL
#define ASFSUTIL

typedef struct _pair
  {
     char      *name;
     char      *value;
  } PAIR;

typedef struct _pairlist
  {
     int        count;
     PAIR      *pairlist;
  } PAIRLIST;

PAIRLIST Set_PairList(void);
void Add_PairList(PAIRLIST *, char *, char *);
void Add_PairList_N(PAIRLIST *, char *, int, char *, int);
void Free_PairList(PAIRLIST);
int Find_PairList_Index(PAIRLIST, char *);
char *Get_PairList_Value(PAIRLIST, char *);
int Find_PairList_Index_I(PAIRLIST, char *);
char *Get_PairList_Value_I(PAIRLIST, char *);
char *Get_PairList_Value_by_Index(PAIRLIST, int);
char *Get_PairList_Name_by_Index(PAIRLIST, int);
int Get_PairList_Count(PAIRLIST);
void Sort_PairList_I_A(PAIRLIST);
int Find_SortedPairList_Index_I(PAIRLIST, char *);
char *Get_SortedPairList_Value_I(PAIRLIST, char *);
char *ConfigSubstitute(char *, PAIRLIST, char *, char *, int, int *);
char *change_realloc(char *, char *, char *, int);
int linein(FILE *, char **, int);

#endif
