//----------------------------------------------------------------------------
// ML_CLASS: Crea un file con l' elenco delle classifiche definite
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "motglue.hpp"
#include "FILE_RW.HPP"

#include "FT_PATHS.HPP"  // Path da utilizzare

struct DESCRIZIONE_CLASSIFICHE {
   char   Codice[2]      ;
   char   Acronimo[12]   ;
   char   Descrizione[50];
   char   CR             ;
   char   LF             ;
};

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      
                                                                                    
int  main(int argc,char *argv[]){                                                   
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          

  DESCRIZIONE_CLASSIFICHE Wrk;
  Wrk.CR = '\r';
  Wrk.LF = '\n';

  FILE_RW Out(PATH_OUT "ML_CLASS.DB2"); Out.SetSize(0);

  MM_INFO::EncodTipoMezzo(STRINGA(" ")); // Per caricare la tabella

  ORD_FORALL(MM_INFO::Decod_TIPO_MEZZO,i){
     STRINGA( i ).ToFix(Wrk.Codice,  2); 
     STRINGA Decod = MM_INFO::Decod_TIPO_MEZZO[i];
     Decod.ToFix(Wrk.Descrizione, 50); 
     ELENCO_S Parole = Decod.Tokens(" ");
     Parole.Last().UpCase().ToFix(Wrk.Acronimo, 12); 
     Out.Scrivi(&Wrk,sizeof(Wrk));
  }
  return 0;
}

