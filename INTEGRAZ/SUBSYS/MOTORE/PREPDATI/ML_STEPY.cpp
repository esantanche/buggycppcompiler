//----------------------------------------------------------------------------
// ML_STEP6.CPP: Statistiche e gestione collegamenti
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

#include "oggetto.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_OUT.HPP"
#include "elenco.h"
#include "eventi.h"
#include "seq_proc.hpp"
#include "scandir.h"
#include "mm_basic.hpp"
#include "mm_path.hpp"

#include "FT_PATHS.HPP"  // Path da utilizzare
#define PGM      "ML_STEPY"

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int  main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
      char PrtBuf[500];
      
      TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
      TRACEPARAMS(argc,argv);
      TRACEEXE(argv[0]);
      SetStatoProcesso(PGM, SP_PARTITO);

      
      SetPriorita(); // Imposta la priorita'

      GESTIONE_ECCEZIONI_ON
      DosError(2); // Disabilita Popup di errore
      TryTime(0);

      //FILE * Out;
      //Out = fopen(PATH_OUT PGM ".OUT","wt");
      //AddFileToBprintf(Out);


      // APRO il grafo
      GRAFO::Grafo   = new GRAFO(PATH_DATI);
      if (CCR_ID::CcrId == NULL) {
         CCR_ID::CcrId = new CCR_ID(PATH_DATI);
      } /* endif */

      F_CLUSTER_STAZIONE ClusStaz(PATH_OUT "CLUSSTAZ.TMP");
      ClusStaz.PermettiScrittura();
      ClusStaz.ReSortFAST();
      F_FERMATE_VIRT_2   TabFv(PATH_OUT "M2_FERMV.TMP");
      F_MEZZO_VIRTUALE_2 TabTv(PATH_OUT "M2_TRENV.TMP");
      F_MEZZOV_CLUSTER   ClusMezv(PATH_OUT "CLUSMEZV.TMP",512);

      WORD LastMV      = 0;
      MEZZO_VIRTUALE_2 * Mv;
      BOOL skip = FALSE;
      int MaxDelta = -10000000;
      int MinDelta =  10000000;
      int KmDominante = 0;
      int KmCorretti  = 0;
      BOOL Concorde;

      ORD_FORALL(TabFv,f){
         TryTime(f);
         FERMATE_VIRT & Fermata = TabFv.FixRec(f); 
         if(Fermata.Transito)continue; // Ignoro i transiti
         // Bprintf3("Mv %i Id %i Prog%i LastMv %i Transita %i Ferma %i ", Fermata.MezzoVirtuale, Fermata.Id , Fermata.Progressivo, LastMV ,Fermata.Transito, Fermata.FermataPartenza || Fermata.FermataArrivo );
         if (Fermata.MezzoVirtuale != LastMV) {
            LastMV =  Fermata.MezzoVirtuale;
            TabTv.Seek(LastMV); Mv = &TabTv.RecordCorrente();
            skip = FALSE;
            if (Mv->NumeroFermateValide == 1){
               skip = TRUE;
               continue; // Skip dei mezzi virtuali inutili
            }
            // Identifico il Cluster
            if(!ClusMezv.Seek(Mv->MezzoVirtuale)){
               Bprintf("Non identificato Cluster per MV Nø %i",Mv->MezzoVirtuale);
               skip = TRUE;
               continue;
            };
            KmDominante = ClusMezv.RecordCorrente().Distanza1;
            Concorde    = ClusMezv.RecordCorrente().VersoConcorde  ;
         };
         if(skip)continue; // Vado al prossimo MV
         // Mi posiziono su Cluster-Stazione
         if(!ClusStaz.Seek( ClusMezv.RecordCorrente().IdCluster , Fermata.Id )){
            Bprintf("Non identificato Cluster Nø %i Fermata %i",ClusMezv.RecordCorrente().IdCluster, Fermata.Id );
            TRACESTRING( VRS( ClusStaz.RecordCorrente().IdCluster ) + VRS(ClusStaz.RecordCorrente().IdStazione ));
            continue;
         };
         CLUSTER_STAZIONE & Cs = ClusStaz.RecordCorrente();
         if (Concorde) {
            KmCorretti = Fermata.ProgKm - KmDominante   ;
         } else {
            KmCorretti = KmDominante    - Fermata.ProgKm;
         } /* endif */
         if (abs(Cs.Distanza) < abs(KmCorretti)) {
            Bprintf("Anomalia nei Km per fermata Id %i Cluster %i, MV %i, Km Su Clust %i Km su MV %i",Fermata.Id ,ClusMezv.RecordCorrente().IdCluster, LastMV
            ,Cs.Distanza , Fermata.ProgKm - KmDominante
            );
         } else {
            int Delta = KmCorretti - Cs.Distanza;
            Top(MaxDelta,Delta);
            Bottom(MinDelta,Delta);
            // Bprintf3("ProgKm %i   KmDominante %i  ProgKmDaDominante %i  Clust.Distanza %i Delta %i", Fermata.ProgKm, KmDominante , KmCorretti , Cs.Distanza , Delta);
         } /* endif */
      }
      Bprintf("MaxDelta = %i, MinDelta = %i",MaxDelta,MinDelta);

      // ---------------------------------------------------------
      // Fine
      // ---------------------------------------------------------
      TryTime(0);
      GESTIONE_ECCEZIONI_OFF
      TRACETERMINATE;
      return 0;
}
