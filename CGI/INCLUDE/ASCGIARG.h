typedef struct _cgiparm
{
   char *name;
   char *value;
} CGIPARM;

typedef struct _cgiparms
{
   int      nparms;
   CGIPARM *parms;
} CGIPARMS;


char x2c(char *);
char *chartransl(char *);
CGIPARMS Parse_Args(char *);
void Free_Args(CGIPARMS);
