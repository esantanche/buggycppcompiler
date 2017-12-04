#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asfsdat8.h"

#define GetStringField(a) p=strchr(s,'\1'); *p='\0'; (a)=(char *)malloc(1+strlen(s)); strcpy((char *)(a),s); s=p+1;
#define GetIntField(a) p=strchr(s,'\1'); *p='\0'; sscanf(s,"%d",&(a)); s=p+1;


void Parse_Risposta(char *string, RISPOSTA *risposta)
{
   char *p, *s;

   s = string;


   p = strchr(s,'\1');
   *p = '\0';
   sscanf(s,"%d",&(risposta->retcode));

   s = p + 1;

   if      (0 == risposta->retcode) (risposta->Dati).soluzioni = Parse_Soluzioni(s);
   else if (1 == risposta->retcode) (risposta->Dati).dettagli  = Parse_Dettagli(s);
   else if (2 == risposta->retcode) (risposta->Dati).errori    = Parse_Errori(s);

   return;
}


SOLUZIONI Parse_Soluzioni(char *string)
{

   char *p, *s;
   SOLUZIONI r;

   int i, j;

   s = string;

   GetStringField(r.stazione_p);
   GetStringField(r.stazione_a);

   GetIntField(r.giorno);
   GetIntField(r.mese);
   GetIntField(r.anno);
   GetIntField(r.ora);
   GetIntField(r.minuto);

   GetIntField(r.num_soluzioni);
   GetIntField(r.num_soluzioni_possibili);

   GetIntField(r.flag_coincidenze);
   GetIntField(r.sort);

   r.soluzioni = (SOLUZIONE *)malloc(r.num_soluzioni * sizeof(SOLUZIONE));

   for (i=0; i<r.num_soluzioni; i++)
   {
      GetStringField(((r.soluzioni)+i)->orario_p);
      GetStringField(((r.soluzioni)+i)->stazione_p);

      GetStringField(((r.soluzioni)+i)->orario_a);
      GetStringField(((r.soluzioni)+i)->stazione_a);

      GetStringField(((r.soluzioni)+i)->tempo);

      GetIntField(((r.soluzioni)+i)->num_coincidenze);

      if (0 == ((r.soluzioni)+i)->num_coincidenze ) {
         ((r.soluzioni)+i)->coincidenze = (COINCIDENZA *)NULL;
      }
      else {
         ((r.soluzioni)+i)->coincidenze = (COINCIDENZA *)malloc( ((r.soluzioni)+i)->num_coincidenze * sizeof(COINCIDENZA));

         for (j=0; j<((r.soluzioni)+i)->num_coincidenze; j++)
         {
            GetStringField( ((((r.soluzioni)+i)->coincidenze)+j)->stazione);
            GetStringField( ((((r.soluzioni)+i)->coincidenze)+j)->arrivo);
            GetStringField( ((((r.soluzioni)+i)->coincidenze)+j)->attesa);
         }
      }

      GetIntField(((r.soluzioni)+i)->num_servizi);

      if (0 == ((r.soluzioni)+i)->num_servizi ) {
         ((r.soluzioni)+i)->servizi = (int *)NULL;
      }
      else {
         ((r.soluzioni)+i)->servizi = (int *)malloc( ((r.soluzioni)+i)->num_servizi * sizeof(int));
         for (j=0; j<((r.soluzioni)+i)->num_servizi; j++) {
            GetIntField ( *((((r.soluzioni)+i)->servizi)+j) );
         }
      }

      GetIntField( ((r.soluzioni)+i)->num_tipitreni );

      if (0 == ((r.soluzioni)+i)->num_tipitreni ) {
         ((r.soluzioni)+i)->tipitreni = (int *)NULL;
      }
      else {
         ((r.soluzioni)+i)->tipitreni = (int *)malloc( ((r.soluzioni)+i)->num_tipitreni * sizeof(int));
         for (j=0; j<((r.soluzioni)+i)->num_tipitreni; j++) {
            GetIntField ( *((((r.soluzioni)+i)->tipitreni)+j) );
         }
      }

      GetStringField( ((r.soluzioni)+i)->prezzo1 );
      GetStringField( ((r.soluzioni)+i)->prezzo2 );

   }

   GetIntField(r.num_acronimi);

   if (0 == r.num_acronimi) {
      r.acronimi = (ACRONIMO *)NULL;
   }
   else {
      r.acronimi = (ACRONIMO *)malloc(r.num_acronimi * sizeof(ACRONIMO));
      for (i=0; i<r.num_acronimi; i++)
        {
           GetStringField( ((r.acronimi)+i)->corto );
           GetStringField( ((r.acronimi)+i)->lungo );
        }                                                        /* End for*/
   }

   return r;
}


DETTAGLI Parse_Dettagli(char *string)
{

   char *p, *s;
   DETTAGLI r;

   int i, j;

   s = string;

   GetStringField(r.stazione_p);
   GetStringField(r.stazione_a);

   GetIntField(r.giorno);
   GetIntField(r.mese);
   GetIntField(r.anno);
   GetIntField(r.ora);
   GetIntField(r.minuto);

   GetStringField(r.orario);

   GetIntField(r.ind_soluzione);
   GetIntField(r.ind_soluzione_orig);
   GetIntField(r.num_soluzioni);

   GetStringField(r.prezzo1);
   GetStringField(r.prezzo2);

   GetIntField(r.km);
   GetStringField(r.instradamento);

   GetIntField(r.sort);

   GetIntField(r.num_treni);

   r.treni = (TRENO *)malloc(r.num_treni * sizeof(TRENO));

   for (i=0; i<r.num_treni; i++) {
      GetIntField( ((r.treni)+i)->tipo );
      GetStringField( ((r.treni)+i)->numero );
      GetStringField( ((r.treni)+i)->nome );
      GetStringField( ((r.treni)+i)->stazione_p );
      GetStringField( ((r.treni)+i)->orario_p );
      GetStringField( ((r.treni)+i)->stazione_a );
      GetStringField( ((r.treni)+i)->orario_a );

      GetIntField(((r.treni)+i)->num_servizi);

      if (0 == ((r.treni)+i)->num_servizi ) {
         ((r.treni)+i)->servizi = (int *)NULL;
      }
      else {
         ((r.treni)+i)->servizi = (int *)malloc( ((r.treni)+i)->num_servizi * sizeof(int));
         for (j=0; j<((r.treni)+i)->num_servizi; j++) {
            GetIntField ( *((((r.treni)+i)->servizi)+j) );
         }
      }

      GetStringField( ((r.treni)+i)->note );
   }                                                             /* End for*/

   return r;
}


ERRORI Parse_Errori(char *string)
{

   char *p, *s;
   ERRORI r;

   int i, j;

   s = string;

   GetIntField(r.motore    );
   GetIntField(r.richiesta );
   GetIntField(r.stazione_p);
   GetIntField(r.stazione_a);
   GetIntField(r.data      );
   GetIntField(r.ora       );
   GetIntField(r.soluzioni );

   if (2 == r.stazione_p) {
      GetIntField(r.num_stazioni_p);
      r.stazioni_p = (char **)malloc(r.num_stazioni_p * sizeof(char *));
      for (i=0; i<r.num_stazioni_p; i++) {
         GetStringField( *((r.stazioni_p)+i) )
      } /* endfor */
   } /* endif */

   if (2 == r.stazione_a) {
      GetIntField(r.num_stazioni_a);
      r.stazioni_a = (char **)malloc(r.num_stazioni_a * sizeof(char *));
      for (i=0; i<r.num_stazioni_a; i++) {
         GetStringField( *((r.stazioni_a)+i) )
      } /* endfor */
   } /* endif */

   return r;
}


void Free_Risposta(RISPOSTA risposta)
{

   if (NULL != (risposta.indata).stazione_p) free((risposta.indata).stazione_p);
   if (NULL != (risposta.indata).stazione_a) free((risposta.indata).stazione_a);
   if (NULL != (risposta.indata).giorno    ) free((risposta.indata).giorno    );
   if (NULL != (risposta.indata).mese      ) free((risposta.indata).mese      );
   if (NULL != (risposta.indata).anno      ) free((risposta.indata).anno      );
   if (NULL != (risposta.indata).ora       ) free((risposta.indata).ora       );
   if (NULL != (risposta.indata).minuto    ) free((risposta.indata).minuto    );
   if (NULL != (risposta.indata).dettaglio ) free((risposta.indata).dettaglio );
   if (NULL != (risposta.indata).lingua    ) free((risposta.indata).lingua    );
   if (NULL != (risposta.indata).indpage   ) free((risposta.indata).indpage   );
   if (NULL != (risposta.indata).sort      ) free((risposta.indata).sort      );

   if      (0 == risposta.retcode) Free_Soluzioni((risposta.Dati).soluzioni);
   else if (1 == risposta.retcode) Free_Dettagli((risposta.Dati).dettagli);
   else if (2 == risposta.retcode) Free_Errori((risposta.Dati).errori);

   return;
}


void Free_Soluzioni(SOLUZIONI r)
{
   int i, j;

   free(r.stazione_p);
   free(r.stazione_a);

   for (i=0; i<r.num_soluzioni; i++) {
      free( ((r.soluzioni)+i)->orario_p );
      free( ((r.soluzioni)+i)->stazione_p );
      free( ((r.soluzioni)+i)->orario_a );
      free( ((r.soluzioni)+i)->stazione_a );
      free( ((r.soluzioni)+i)->tempo );
      if (0 != ((r.soluzioni)+i)->num_coincidenze) {
         for (j=0; j<((r.soluzioni)+i)->num_coincidenze; j++) {
            free( ((((r.soluzioni)+i)->coincidenze)+j)->stazione );
            free( ((((r.soluzioni)+i)->coincidenze)+j)->arrivo );
            free( ((((r.soluzioni)+i)->coincidenze)+j)->attesa );
         } /* endfor */
         free( ((r.soluzioni)+i)->coincidenze );
      } /* endif */
      if (0 != ((r.soluzioni)+i)->num_servizi) free( ((r.soluzioni)+i)->servizi );
      if (0 != ((r.soluzioni)+i)->num_tipitreni) free( ((r.soluzioni)+i)->tipitreni );
      free( ((r.soluzioni)+i)->prezzo1 );
      free( ((r.soluzioni)+i)->prezzo2 );
   } /* endfor */
   free(r.soluzioni);
   if (0 != r.num_acronimi) {
      for (i=0; i<r.num_acronimi; i++) {
         free( ((r.acronimi)+i)->corto );
         free( ((r.acronimi)+i)->lungo );
      } /* endfor */
      free(r.acronimi);
   } /* endif */

   return;
}


void Free_Dettagli(DETTAGLI r)
{
   int i;

   free(r.stazione_p);
   free(r.stazione_a);
   free(r.orario);
   free(r.prezzo1);
   free(r.prezzo2);
   free(r.instradamento);
   if (0 != r.num_treni) {
      for (i=0; i<r.num_treni; i++) {
         free( ((r.treni)+i)->numero );
         free( ((r.treni)+i)->nome );
         free( ((r.treni)+i)->stazione_p );
         free( ((r.treni)+i)->orario_p );
         free( ((r.treni)+i)->stazione_a );
         free( ((r.treni)+i)->orario_a );
         if (0 != ((r.treni)+i)->num_servizi ) {
            free( ((r.treni)+i)->servizi );
         } /* endif */
         free( ((r.treni)+i)->note );
      } /* endfor */
   } /* endif */

   return;
}


void Free_Errori(ERRORI r)
{
   int i;

   if (2 == r.stazione_p) {
      for (i=0; i<r.num_stazioni_p; i++) {
         free( *((r.stazioni_p)+i) );
      } /* endfor */
      free(r.stazioni_p);
   } /* endif */

   if (2 == r.stazione_a) {
      for (i=0; i<r.num_stazioni_a; i++) {
         free( *((r.stazioni_a)+i) );
      } /* endfor */
      free(r.stazioni_a);
   } /* endif */

   return;
}
