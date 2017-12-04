//
// Project Folder
// Progetto: SIPAX
//

#include <stdio.h>
#include "comptest.hpp"

int STAZIONI::iDato_stazioni = 0;

const char * funzProva_cast_const(const char * ccPar)
{
};

const char * ccpProva_cast_const;

int main(int argc, char **argv)
{

   // begin 001 test struttura dentro funzione
   struct PROVA1 {
      char cDummy2;
   };
   PROVA1 prova1;
   // end test struttura dentro funzione

   // begin 002 prova cast PVOID scritto come costruttore
   void * vpProva_cast_pvoid;
   char * cpProva_cast_pvoid;
   vpProva_cast_pvoid = PVOID(cpProva_cast_pvoid);
   // end prova cast PVOID scritto come costruttore

   // begin 003 prova bitfield segnati
   CLUSSTAZ clusstaz;
   clusstaz.MediaKm = -1;
   if (clusstaz.MediaKm != -1)
      printf("ERRORE! fallito test 003 clusstaz.MediaKm = %d\n", clusstaz.MediaKm);
   // end prova bitfield segnati

   // begin 004 prova cast const
   // VA char * cpProva_cast_const = funzProva_cast_const(cpProva_cast_pvoid);
   char * cpProva_cast_const = (char *) funzProva_cast_const(cpProva_cast_pvoid);
   // VA cpProva_cast_const = ccpProva_cast_const;
   cpProva_cast_const = (char *) ccpProva_cast_const;
   // end prova cast const

   // begin 005 prova altri cast
   // VA PBYTE pbProva_cast_pbyte = cpProva_cast_pvoid;
   PBYTE pbProva_cast_pbyte = (PBYTE) cpProva_cast_pvoid;
   // VA cpProva_cast_const = funzProva_cast_const(cpProva_cast_pvoid);
   cpProva_cast_const = (char *) funzProva_cast_const(cpProva_cast_pvoid);
   // VA cpProva_cast_const = ccpProva_cast_const;
   cpProva_cast_const = (char *) ccpProva_cast_const;
   // end prova altri const

   // begin 006 prova enum
   EPROVA eProva;
   // VA int(eProva)++;
   int * ipeProva = (int*) &eProva;
   (*ipeProva)++;
   // end prova enum

   // begin 007 prova file
   // VA FILE * fProva = fopen("COMPTEST.DEL","wt");
   FILE * fProva = fopen("COMPTEST.DEL","w");
   if (fProva != NULL)
      {fprintf(fProva, "Prova 007 riuscita.\n");fclose(fProva);}
   else
      printf("ERRORE! fallito test 007 apert. file\n");
   // end 007 prova file



};

// begin prova accessibilità membri da struttura definita dentro la classe
// Qui VisualAge da errore chiedendo di specificare l'appartenenza a STAZIONI
// di iDato_stazioni
int STAZIONI::R_STRU::Metodo(int Parametro)
{
   int y;

   // VA y = iDato_stazioni;
   y = STAZIONI::iDato_stazioni;

   return y;
}

// end prova accessibilità membri da struttura definita dentro la classe
