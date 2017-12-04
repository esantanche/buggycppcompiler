//========================================================================
// MM_PERIO.CPP : Classi per il caricamento iniziale dei files "t"
//========================================================================
//

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 1
// Le periodicita' di base sono mostrate a livello di trace 2


#include "id_stazi.hpp"
#include "mm_varie.hpp"
#include "MM_GRAFO.HPP"
#include "alfa0.hpp"
#include "mm_perio.hpp"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ML_OUT.HPP"

#include "FT_PATHS.HPP"  // Path da utilizzare

#define PGM      "TA"

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

void main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
   
   { // Per garantire la chiamata dei distruttori dei files
      
      TRACEREGISTER2(NULL,PGM, PGM ".TRC");
      TRACEPARAMS(argc,argv);
      TRACEEXE(argv[0]);
      DosError(2); // Disabilita Popup di errore

      // TRACE DIMENSIONI DELLE PRINCIPALI CLASSI
      #define Tr(_a) TRACEVLONG(sizeof(_a))
      Tr(MEZZO_VIRTUALE);
      Tr(NOTE_VARIE);
      Tr(FERMATE_VIRT);
      Tr(PERIODICITA_FERMATA_VIRT);
      Tr(MEZZO_VIRTUALE_2);
      Tr(FERMATE_VIRT_2);
      Tr(NOTE_VIRT);
      Tr(STAZIONE_MV);
      Tr(CLUSTER_MV);
      Tr(CLUSTER_STAZIONE);
      Tr(STAZIONE_CLUSTER);
      Tr(MEZZOV_CLUSTER);
      Tr(CLUSTER_MEZZOV);


      PROFILER::Clear(FALSE); // Per aprire il grafo

      // APRO il grafo
      GRAFO::Grafo   = new GRAFO(PATH_DATI);
      if (CCR_ID::CcrId == NULL) {
         CCR_ID::CcrId = new CCR_ID(PATH_DATI);
      } /* endif */
      TRACEVPOINTER(CCR_ID::CcrId);

      // Leggo i dati delle multistazioni
      GEST_MULTI_STA Gestore("E:\\CVB_UFF\\",stdout);
      // E faccio un dump
      Gestore.Trace("Dati delle multistazioni:");


      // // Carico i limiti dell' orario fornito
      // T_PERIODICITA::Init(PATH_IN,"DATE.T");
      // 
      // 
      // T_PERIODICITA Test;
      // Test.ReSet();
      // Test.Set(T_PERIODICITA::Fine_Orario_FS - T_PERIODICITA::Inizio_Orario_FS );
      // Test.Set(0);
      // Test.Set(2);
      // Test.Set(8);
      // for (int i = 0;i  < 200 ; i++ ) {
      //    Test.Trace("Shift Nø "+STRINGA(i));
      //    // Test.GiornoPrecedente();
      //    Test.GiornoSeguente();
      // } /* endfor */
      
      // ---------------------------------------------------------
      // Fine
      // ---------------------------------------------------------
      TRACETERMINATE;
      
   }
   
   exit(0);
}


