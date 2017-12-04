//----------------------------------------------------------------------------
// MM_VARI2.CPP
//----------------------------------------------------------------------------
// Funzioncina per impostare i reference globali

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3

// EMS001
typedef unsigned long BOOL;

#include "id_stazi.hpp"

extern STAZIONI * Stazioni; // Truccaccio: in realta' e' un reference

BOOL InitStazioni(const STRINGA& Path){
   if(Stazioni == NULL){
      // Questa variabile di ambiente mi fa tenere i dati di orario TUTTI IN MEMORIA
      if(getenv("ORARI_IN_MEMORIA") != NULL){
         TRACESTRING("Gli orari sono stati messi in memoria");
         Stazioni = new STAZIONI((CPSZ)Path,"ID_STAZI.DB",0);  // Tutto in memoria
      } else {
         TRACESTRING("Gli orari NON sono stati messi in memoria");
         Stazioni = new STAZIONI((CPSZ)Path);
      } /* endif */
   }
   if(Stazioni == NULL)return FALSE;
   return TRUE;
};

void StopStazioni(){
   if(Stazioni != NULL)delete Stazioni;
   Stazioni = NULL;
};

