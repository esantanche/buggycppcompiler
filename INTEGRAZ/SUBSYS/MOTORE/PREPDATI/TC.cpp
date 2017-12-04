#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 1
// Le periodicita' di base sono mostrate a livello di trace 2


#include "id_stazi.hpp"
#include "mm_varie.hpp"
#include "MM_GRAFO.HPP"
#include "alfa0.hpp"
#include "ft_paths.hpp"

#define PGM      "TC"



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

      //char Buffer[250];
      //int NumBits = 5;
      //int Size = 0;
      //
      //for (int i = 0;i < 5 ;i++ ) {
      //   for (int j = 0 ; j <= i ;j ++ ) {
      //      int Elem = j + 5*i;
      //      SetTrgBits(Buffer,Size,i,j,NumBits,Elem);
      //   } /* endfor */
      //} /* endfor */
      //
      //TRACEVLONG(Size);
      //TRACEHEX("Buffer",Buffer,32);
      //
      //for (i = 0;i < 5 ;i++ ) {
      //   for (int j = 0 ; j <= i ;j ++ ) {
      //      int Elem = GetTrgBits(Buffer,i,j,NumBits);
      //      TRACESTRING("Elemento "+STRINGA(i)+","+STRINGA(j)+" = "+STRINGA(Elem));
      //   } /* endfor */
      //} /* endfor */

      SET A;
      A.Clear();
      TRACEVSTRING2(A.ToHex());
      A.Set(16);
      TRACEVSTRING2(A.ToHex());
      A.Set(26);
      TRACEVSTRING2(A.ToHex());
      A.Set(29);
      TRACEVSTRING2(A.ToHex());
      A.Set(63);
      TRACEVSTRING2(A.ToHex());

      PROFILER::Clear(0);
      PROFILER::Cronometra(0,1);
      PROFILER::Cronometra(0,0);
      PROFILER::Trace(0,1,1);
      PROFILER::Cronometra(0,0);
      PROFILER::Trace(0,1,1);


      // TRACE DIMENSIONI DELLE PRINCIPALI CLASSI
      #define Tr(_a) TRACEVLONG(sizeof(_a))
      Tr(STAZ_FS);
      Tr(STAZ_FS::PRIMO_V);
      Tr(STAZ_FS::P_POLI);
      Tr(STAZ_FS::D_INSTR);
      Tr(RAMO);
      Tr(RAMO::STAZIONE_R);
      Tr(POLIMETRICA);
      Tr(POLIMETRICA::PO_STAZ);
      Tr(POLIMETRICA::PO_DIRA);
      Tr(POLIMETRICA_SU_FILE);
      // ---------------------------------------------------------
      // Fine
      // ---------------------------------------------------------
      TRACETERMINATE;
      
   }
   
   exit(0);
}


