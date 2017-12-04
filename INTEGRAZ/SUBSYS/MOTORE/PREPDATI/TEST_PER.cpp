//----------------------------------------------------------------------------
// TEST_PER.CPP: Programma di prova per esplosione periodicita
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "eventi.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "elenco.h"
#include "oggetto.h"

#include "FT_PATHS.HPP"  // Path da utilizzare

#define PGM      "TEST_PER"

class  FILE_PERIODICITA : public FILE_FIX {
   public:
   FILE_PERIODICITA(const STRINGA& NomeFile,ULONG BufSize= 64000): FILE_FIX(NomeFile,sizeof(PERIODICITA),BufSize){};

   // Definizione operatori
   PERIODICITA&  operator [](WORD Num){ Posiziona(Num); return *(PERIODICITA*) RecordC; };
   PERIODICITA&  RecordCorrente(){ return *(PERIODICITA*) RecordC; };
};


//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
      
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   
   GESTIONE_ECCEZIONI_ON

   Bprintf("Prova Periodicita\n");

   // La variabile di environment "VECCHIO_ORARIO" mi fa prendere i dati dal vecchio orario
   STRINGA Datet,Period;
   if (STRINGA(getenv("VECCHIO_ORARIO")) == "SI") {
      Datet   = PATH_IN2;
      Period  =  PATH_OUT2 "PERIODIC.DB";
   } else {
      Datet   = PATH_IN;
      Period  =  PATH_OUT "PERIODIC.DB";
   } /* endif */


   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(Datet,"DATE.T");
   PERIODICITA::ImpostaProblema( T_PERIODICITA::Inizio_Dati_Caricati, T_PERIODICITA::Inizio_Orario_FS, T_PERIODICITA::Fine_Orario_FS, T_PERIODICITA::Inizio_Dati_Caricati);

   FILE_PERIODICITA FileP(Period); // Gestisce file non esistente o vuoto

   int Filtro = 0;
   if(argc > 1)Filtro = atoi(argv[1]);

   DWORD Tempo1 = Time();
   int NumOk  = 0;
   int NumBad = 0;
   int NumPer = 0;
   T_PERIODICITA Wrk; Wrk.Tipo = 1;
   ORD_FORALL(FileP, i){
      // if(i > 40)break; // per ora
      if(Filtro && i != Filtro)continue; 
      memmove(&Wrk,&FileP[i],sizeof(PERIODICITA));
      TRACESTRING("=========================================================");
      Wrk.Trace("Periodicita["+STRINGA(i)+"] : ");
      PERIODICITA_IN_CHIARO * PerIc; 
      if(argc > 2){
        PerIc = Wrk.EsplodiPeriodicita(atoi(argv[2]));
      } else {
        PerIc = Wrk.EsplodiPeriodicita();
      }
      if(PerIc == NULL) continue;
      PerIc->Trace("Periodicita' Esplosa:"); 
      NumPer += PerIc->Dim();
      if(PerIc->Dim() > 4){
         NumBad ++;
         TRACESTRING("CATTIVA ESPANSIONE");
         ELENCO_S Tmp;
         Tmp += "=========================================================";
         Tmp += "Periodicita["+STRINGA(i)+"] : ";
         Tmp +=  Wrk.Decod();
         Tmp += "Esplosione della periodicita : ";
         Tmp +=  PerIc->PeriodicitaLeggibile();
         ORD_FORALL(Tmp,t)Bprintf("%s",(CPSZ)Tmp[t]);
      } else {
         NumOk ++;
      }
      delete PerIc;
   }
   if(NumBad){
      Bprintf("=========================================================");
   };
   Bprintf3("NumOk = %i NumBad = %i NumPer = %i Tempo= %i",NumOk,NumBad,NumPer,10*(Time() - Tempo1));
   fprintf(stderr,"NumOk = %i NumBad = %i NumPer = %i Tempo= %i",NumOk,NumBad,NumPer,10*(Time() - Tempo1));

   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
      
   return 0;
}

