#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ascgiarg.h"


CGIPARMS Parse_Args(char *query_string)
{
   CGIPARMS a;
   int n, ln;
   char *token, *u;
   CGIPARM *p;
   char *qs;

   qs = (char *)malloc(1+strlen(query_string));
   strcpy(qs,query_string);

   n = 0;
   token = strtok(qs,"&");
   do {
      if (0 == n) a.parms = (CGIPARM *)malloc(sizeof(CGIPARM));
      else a.parms = (CGIPARM *)realloc(a.parms,(n+1)*sizeof(CGIPARM));
      p = (a.parms)+n;
      u = strchr(token,'=');
      if (NULL == u) {
         p->name = (char *)malloc(1);
         *(p->name) = '\0';
         p->value = chartransl(token);
      }
      else {
         ln = u - token;
         p->name = (char *)malloc(ln+1);
         strncpy(p->name,token,ln);
         *((p->name)+ln) = '\0';
         p->value = chartransl(u+1);
      }
      n++;
   } while (token = strtok(NULL,"&"));
   a.nparms = n;

   free(qs);
   return a;
}


char *chartransl(char *string)
{
   char *t, *p;
   int i, j;

   t = (char *)malloc(strlen(string)+1);

   for (i=0; string[i]; i++) if ('+' == string[i]) string[i] = ' ';

   for (i=0, j=0; string[i]; i++, j++) {
      if ((t[j] = string[i]) == '%') {
         t[j] = x2c(&string[i+1]);
         i += 2;
      }
   }
   t[j] = '\0';

   return t;
}

char x2c(char *hex)
{
   char d;

   d = (hex[0] >= 'A' ? ((hex[0] & 0xdf) - 'A')+10 : (hex[0] - '0'));
   d *= 16;
   d += (hex[1] >= 'A' ? ((hex[1] & 0xdf) - 'A')+10 : (hex[1] - '0'));
   return d;
}

void Free_Args(CGIPARMS p)
{
   int i;
   for (i=0; i<p.nparms; i++) {
      free( ((p.parms)+i)->name );
      free( ((p.parms)+i)->value);
   }
}
