#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "asfsutil.h"
#include <ctype.h>

typedef unsigned long BOOL;
#define TRUE 1
#define FALSE 0
void Write_Q5_Log(char *);
void Converti_query_string(char * name, char ** cValore_convertito, char * value);

PAIRLIST Set_PairList(void)
{
   PAIRLIST pl;

   pl.count = 0;
   pl.pairlist = NULL;

   return pl;
}

void Add_PairList(PAIRLIST *pl, char *name, char *value)
{
   int n;
   char *cValore_convertito; // EMS

   Converti_query_string(name, &cValore_convertito, value);

   n = pl->count;
   pl->pairlist = (PAIR *)realloc(pl->pairlist, (n + 1)*sizeof(PAIR));
   ((pl->pairlist)+n)->name = (char *)malloc(1+strlen(name));
   strcpy(((pl->pairlist)+n)->name,name);
   ((pl->pairlist)+n)->value = (char *)malloc(1+strlen(cValore_convertito));
   strcpy(((pl->pairlist)+n)->value,cValore_convertito);
   n++;
   pl->count = n;

   free(cValore_convertito);  // EMS

   return;
}

// EMS

BOOL ishtml(char c)
{

   if (strchr("=&", c))
      return TRUE;
   else
      return FALSE;
}

char * Variabili_da_convertire[] =
   {
      "QUERY_STRING",
      "QUERY_OTHER_PAGE",
      "SOLUZIONI_HTTP",
      "NEXT_SOL",
      "PREV_SOL",
      ""          // tappo
   };

void Converti_query_string(char * name, char ** cValore_convertito, char * value)
{

   /*
   QUERY_STRING       o buffer o querystring
                  SL03 e DT04 ricevono querystring senza modificarla
                  solo Build_Soluzioni usa SL03 e gli passa la query string senza modifiche
                  solo Build_Dettagli  usa DT04 e gli passa la query string senza modifiche
                  Build_Dettagli e Build_Soluzioni sono usate solo in ASFSOUT8.C
                  la query string viene immodificata da Output_and_free
                  ma Output_and_free la riceve da Search_Orario che la passa anche ad Engine_Request
                  per cui non può essere stata modificata
   QUERY_OTHER_PAGE   solo buffer
   SOLUZIONI_HTTP     solo buffer
   NEXT_SOL           solo buffer
   PREV_SOL           solo buffer
   */

   char * pcCursore_in_lettura;
   char * pcCursore_in_scrittura;

   *cValore_convertito = NULL;

   if (value == NULL) return;

   if (*value == '\0') {
      *cValore_convertito = (char *) malloc(1);
      **cValore_convertito = '\0';
      return;
   }

   BOOL bNon_convertire = TRUE;
   for (int i=0;Variabili_da_convertire[i][0];i++)
      if (strcmp(Variabili_da_convertire[i], name) == 0) {
         bNon_convertire = FALSE;
         break;
      }
   if (bNon_convertire) {
      *cValore_convertito = (char *) malloc(strlen(value)+1);
      memcpy(*cValore_convertito, value, strlen(value)+1 );
      return;
   }

   *cValore_convertito = (char *) malloc(strlen(value)*3+1);

   memset(*cValore_convertito, 0, strlen(value)*3+1);

   pcCursore_in_lettura = (char *) value;
   pcCursore_in_scrittura = (char *) (*cValore_convertito);
   for (; *pcCursore_in_lettura; pcCursore_in_lettura++, pcCursore_in_scrittura++) {
      if (*pcCursore_in_lettura == ' ')
         *pcCursore_in_scrittura = '+';
      else
      if ( (ispunct(*pcCursore_in_lettura) || isupper(*pcCursore_in_lettura)) &&
            !ishtml(*pcCursore_in_lettura) ) {
         *pcCursore_in_scrittura = '%';
         pcCursore_in_scrittura++;
         sprintf(pcCursore_in_scrittura,"%2X", *pcCursore_in_lettura);
         pcCursore_in_scrittura++;
      }
      else
         *pcCursore_in_scrittura = *pcCursore_in_lettura;
   }; // endfor
   *pcCursore_in_scrittura = '\0';

   return;
}

void Add_PairList_N(PAIRLIST *pl, char *name, int lname, char *value, int lvalue)
{
   int n;

   n = pl->count;
   pl->pairlist = (PAIR *)realloc(pl->pairlist, (n + 1)*sizeof(PAIR));
   ((pl->pairlist)+n)->name = (char *)malloc(1+lname);
   strncpy(((pl->pairlist)+n)->name,name,lname);
   *((((pl->pairlist)+n)->name)+lname) = '\0';
   ((pl->pairlist)+n)->value = (char *)malloc(1+lvalue);
   strncpy(((pl->pairlist)+n)->value,value,lvalue);
   *((((pl->pairlist)+n)->value)+lvalue) = '\0';
   n++;
   pl->count = n;
   return;
}

void Free_PairList(PAIRLIST pl)
{
   int i;

   if (0 == pl.count) return;

   for (i=0; i<pl.count; i++) {
      free( ((pl.pairlist)+i)->name );
      free( ((pl.pairlist)+i)->value);
   }
   free(pl.pairlist);
   return;
}

int Find_PairList_Index(PAIRLIST pl, char *name)
{
   int i, f;

   f = -1;
   for (i=0; i<pl.count; i++) {
      if (0 == strcmp(name,((pl.pairlist)+i)->name)) {
         f = i;
         break;
      }
   }
   return f;
}

int Find_PairList_Index_I(PAIRLIST pl, char *name)
{
   int i, f;

   f = -1;
   for (i=0; i<pl.count; i++) {
      if (0 == strcmpi(name,((pl.pairlist)+i)->name)) {
         f = i;
         break;
      }
   }
   return f;
}

char *Get_PairList_Value(PAIRLIST pl, char *name)
{
   int f;

   if (-1 == (f = Find_PairList_Index(pl,name))) return NULL;
   return ((pl.pairlist)+f)->value;
}

char *Get_PairList_Value_I(PAIRLIST pl, char *name)
{
   int f;

   if (-1 == (f = Find_PairList_Index_I(pl,name))) return NULL;
   return ((pl.pairlist)+f)->value;
}

char *Get_PairList_Value_by_Index(PAIRLIST pl, int index)
{
   if (index < 0 || index > pl.count) return NULL;
   return ((pl.pairlist)+index)->value;
}

char *Get_PairList_Name_by_Index(PAIRLIST pl, int index)
{
   if (index < 0 || index > pl.count) return NULL;
   return ((pl.pairlist)+index)->name;
}

int Get_PairList_Count(PAIRLIST pl)
{
   return pl.count;
}

void Sort_PairList_I_A(PAIRLIST pl)
{
   int gap, i, j;
   PAIR t;

   for (gap = pl.count/2; gap > 0; gap /= 2) {
      for (i = gap; i < pl.count; i++) {
         for (j=i-gap;
              j>=0 &&
              strcmpi( ((pl.pairlist)+j)->name, ((pl.pairlist)+j+gap)->name ) > 0;
              j-=gap) {
            t = *((pl.pairlist)+j);
            *((pl.pairlist)+j) = *((pl.pairlist)+j+gap);
            *((pl.pairlist)+j+gap) = t;
         }
      }
   }
   return;
}

int Find_SortedPairList_Index_I(PAIRLIST pl, char *name)
{
   int f, l, ind, p, z;

   if (0 == pl.count) return -1;
   f = 0;
   l = pl.count - 1;
   ind = -1;
   for (;;) {
      p = (f + l) / 2;
      z = strcmpi( ((pl.pairlist)+p)->name, name);
      if (z == 0) {
         ind = p;
         break;
      }
      else if (z > 0) l = p-1;
      else f = p+1;
      if (f > l) break;
   }

   return ind;
}

char *Get_SortedPairList_Value_I(PAIRLIST pl, char *name)
{
   int f;

   if (-1 == (f = Find_SortedPairList_Index_I(pl,name))) return NULL;
   return ((pl.pairlist)+f)->value;
}

char *ConfigSubstitute(char *templ, PAIRLIST cvl,
                       char *start_sep, char *end_sep,
                       int free_templ, int *err)
{
   char *s, *p, *n, *v, *ne, *z, *pt;
   int l, le, ll;
   int ls;


   s = (char *)malloc(strlen(templ)+1);
   strcpy(s,templ);
   ls = strlen(start_sep);

   *err = 0;

   pt = s;

   while (NULL != (p = strstr(pt,start_sep)))
   {
      z = strstr(pt,end_sep);
      if (NULL == z) {
         *err = 1;
         if (strlen(pt) >= ls) pt = pt + ls;
         else pt = p+strlen(pt);
      }
      else {
         l = z - p - strlen(start_sep);
         n = (char *)malloc(l+1);
         strncpy(n,p+strlen(start_sep),l);
         n[l] = '\0';

         v = Get_SortedPairList_Value_I(cvl, n);
         if (NULL == v) {
            *err = 1;
            pt = z + strlen(end_sep);
         }

         else {
            le = z - p + strlen(end_sep);
            ne = (char *)malloc(le+1);
            strncpy(ne,p,le);
            ne[le] = '\0';

            ll = pt - s;
            s = change_realloc(s, ne, v, free_templ);
            pt = s + ll + strlen(v);
            free(ne);
          }
         free(n);
      }
   }

   return s;
}
/*
char *ConfigSubstitute(char *templ, PAIRLIST cvl,
                       char *start_sep, char *end_sep,
                       int free_templ, int *err)
{
   char *s, *p, *n, *v, *ne, *z, *pt;
   int l, le, ll;
   int ls;


   s = (char *)malloc(strlen(templ)+1);
   strcpy(s,templ);
   ls = strlen(start_sep);

   *err = 0;

   pt = s;

   while (NULL != (p = strstr(pt,start_sep)))
   {
      z = strstr(pt,end_sep);
      if (NULL == z) {
         *err = 1;
         if (strlen(pt) >= ls) pt = pt + ls;
         else pt = p+strlen(pt);
      }
      else {
         l = z - p - strlen(start_sep);
         n = (char *)malloc(l+1);
         strncpy(n,p+strlen(start_sep),l);
         n[l] = '\0';

         v = Get_SortedPairList_Value_I(cvl, n);

         le = z - p + strlen(end_sep);
         ne = (char *)malloc(le+1);
         strncpy(ne,p,le);
         ne[le] = '\0';

         if (NULL == v) {
            *err = 1;
            v = ne;
         }
         ll = pt - s;
         s = change_realloc(s, ne, v, free_templ);
         pt = s + ll + strlen(v);
         free(n);
         free(ne);
      }
   }

   return s;
}
*/
/*
char *ConfigSubstitute(char *templ, PAIRLIST cvl,
                       char *start_sep, char *end_sep, int free_templ)
{
   char *s, *p, *n, *v, *ne, *z;
   int l, le;

   s = (char *)malloc(strlen(templ)+1);
   strcpy(s,templ);

   while (NULL != (p = strstr(s,start_sep)))
   {
      z = strstr(s,end_sep);
      if (NULL == z) {
         free(s);
         if (0 != free_templ) free(templ);
         return NULL;
      }
      l = z - p - strlen(start_sep);
      n = (char *)malloc(l+1);
      strncpy(n,p+strlen(start_sep),l);
      n[l] = '\0';
      v = Get_SortedPairList_Value_I(cvl, n);
      if (NULL == v) {
         free(s);
         free(n);
         if (0 != free_templ) free(templ);
         return NULL;
      }
      le = z - p + strlen(end_sep);
      ne = (char *)malloc(le+1);
      strncpy(ne,p,le);
      ne[le] = '\0';
      s = change_realloc(s, ne, v, free_templ);
      free(n);
      free(ne);
   }

   return s;
}
*/
/*
  Alloca il buffer per la nuova stringa
  se free_string != 0 disalloca il vecchio buffer
*/

char *change_realloc(char *string, char *sold, char *snew, int free_string)
{
   char *p;
   int n;
   int lo, ln;
   char *op, *rp;

   n = 0;
   p = string;
   lo = strlen(sold);
   while (NULL != (p = strstr(p,sold)))
   {
      n++;
      p += lo;
   }
   if (0 == n) {
      if (free_string != 0) {
         return string;
      }
      else {
         rp = (char *)malloc(1+strlen(string));
         strcpy(rp,string);
         return string;
      }
   }

   ln = strlen(snew);
   rp = (char *)malloc(1+strlen(string)+n*(ln-lo));
   rp[0] = '\0';

   p = op = string;

   while (NULL != (p = strstr(p,sold)))
   {
      if (p != op) strncat(rp,op,p-op);
      strcat(rp,snew);
      op = p += lo;
   }
   strcat(rp,op);
   if (0 != free_string) free(string);

   return rp;
}

/**********************************************************
   Legge una linea dal file gi… aperto il cui puntatore Š f.
   Se *line Š NULL la memoria per la linea viene allocata
   nella funzione, altrimenti *line Š l'indirizzo del
   buffer che dovr… contenere la linea. In quest'ultimo
   caso inlen Š la lunghezza del buffer e anche quindi
   massimo numero di caratteri leggibili (in caso di
   troncamento il file si posiziona all'inizio della riga
   successiva)

   codici di ritorno:
      0   O.K.
      1   f == NULL
      2   line == NULL
      3   EOF o errore in lettura
      4   out of memory
     -1   riga troncata
***********************************************************/
int linein(FILE *f, char **line, int inlen)
{

   #define BUF_LINEIN  255

   char buffer[BUF_LINEIN+1];
   long l, la, lb;
   char *ln, *p;
   int mem, fin, stop;


   if (NULL == f) return 1;
   if (NULL == line) return 2;

   if (NULL == *line) {
      *line = ln = (char *)malloc(1);
      mem = 1;
   }
   else {
      ln = *line;
      mem = 0;
   }
   strcpy(ln,"");

   l = 0;
   fin = 0;
   stop = 0;

   for (;;) {
      p = fgets(buffer,BUF_LINEIN,f);
      if (NULL == p) {
         if (1 == mem) {
            free(ln);
            *line = (char *)NULL;
         }
         return 3;
      }
      lb = strlen(buffer);

      if ('\n' == buffer[lb-1]) fin = 1;
      if (0 == stop) {
         if (1 == fin) la = lb - 1;
         else la = lb;
         if (1 == mem) {
            if (NULL == (*line = ln = (char *)realloc(ln,l+la+1))) return 4;
         }
         if ((0 == mem) && ((l + la) > (inlen - 1))) {
            strncat(ln,buffer,inlen - l - 1);
            stop = 1;
         }
         else {
            if (0 != la) strncat(ln,buffer,la);
            l += la;
         }
      }
      if (1 == fin) break;
   }

   if (1 == stop) return -1;
   return 0;

}
