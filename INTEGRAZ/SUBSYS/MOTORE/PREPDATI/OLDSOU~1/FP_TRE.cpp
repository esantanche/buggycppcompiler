//----------------------------------------------------------------------------                             c
// FP_TRE.CPP: Controllo che non vi siano rami che NON seguono il percorso minimo
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1


//----------------------------------------------------------------------------
//#define NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
//----------------------------------------------------------------------------

#include "oggetto.h"
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "ALFA0.HPP"
#include "POLIM.HPP"
#include "seq_proc.HPP"

#include "FT_PATHS.HPP"  // Path da utilizzare


#define PGM      "FP_TRE"


//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          

   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   SetStatoProcesso(PGM, SP_PARTITO);

   FILE_RW Out(PATH_OUT   PGM ".LOG");
   Out.SetSize(0);
   AddFileToBprintf(&Out);

   Bprintf("Elenco degli (eventuali) rami che non stanno sul percorso minimo tra due nodi");
   Bprintf("Ricordarsi di apportare equivalenti correzioni su MM_GRAFO");

   PROFILER::Clear(FALSE);
   PROFILER::Clear(TRUE);

   GRAFO::Grafo   = new GRAFO(PATH_DATI);
   GRAFO & Grafo = *GRAFO::Grafo;

   // Apro l' archivio decodifica codici CCR
   CCR_ID Ccr_Id(PATH_DATI);
   
   if (CCR_ID::CcrId == NULL) {
      CCR_ID::CcrId = & Ccr_Id;
   } /* endif */

   int Rc = 0;

   extern BOOL PrivilegiaRamoDiretto;
   PrivilegiaRamoDiretto=FALSE;

   for (int i = 1;i < Grafo.TotRami ; i++ ) {
      RAMO & Ramo = Grafo.Rami[i];

      #ifdef NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
      GRAFO::Gr().AbilitaCumulativo = FALSE;
      if(Ramo.Concesso)continue;
      #else
      GRAFO::Gr().AbilitaCumulativo = TRUE;
      #endif
      if(!Grafo.Risolvi(Ramo.IdStaz1,Ramo.IdStaz2)){
         Bprintf("Ramo  %i (%i -> %i ) non funziona Risolvi ",i, Ramo.IdStaz1,Ramo.IdStaz2);
         Bprintf("Da : %s ",Stazioni.DecodificaIdStazione(Ramo.IdStaz1));
         Bprintf("A  : %s",Stazioni.DecodificaIdStazione(Ramo.IdStaz2));
         if(Rc == 0)Rc = 99;
      } else if(Grafo[Ramo.IdStaz1].Distanza1 != Ramo.KmRamo){
         Bprintf("Ramo  %i (%i -> %i ) non segue il percorso minimo ",i, Ramo.IdStaz1,Ramo.IdStaz2);
         Bprintf("Da : %s ",Stazioni.DecodificaIdStazione(Ramo.IdStaz1));
         Bprintf("A  : %s",Stazioni.DecodificaIdStazione(Ramo.IdStaz2));
         Bprintf("KmGrafo= %i, Km Ramo=%i",Grafo[Ramo.IdStaz1].Distanza1,Ramo.KmRamo);
         Bprintf("===> La condizione e' anomala e deve essere rimossa");
         ID IdNodo = Ramo.IdStaz1;
         puts("Percorso minimo sul grafo:");
         while (IdNodo != Ramo.IdStaz2) {
            Bprintf("   %i %s",IdNodo,Stazioni.DecodificaIdStazione(IdNodo));
            IdNodo = Grafo[IdNodo].Nodo1;
         } /* endwhile */
         Bprintf("   %i %s",IdNodo,Stazioni.DecodificaIdStazione(IdNodo));
      }

      //if(Ramo.IdStaz1 == 455 && Ramo.IdStaz2 == 462){
      //   Bprintf("Ramo  %i (%i -> %i ) test percorso minimo ",i, Ramo.IdStaz1,Ramo.IdStaz2);
      //   Bprintf("Da : %s ",Stazioni.DecodificaIdStazione(Ramo.IdStaz1));
      //   Bprintf("A  : %s",Stazioni.DecodificaIdStazione(Ramo.IdStaz2));
      //   Bprintf("KmGrafo= %i, Km Ramo=%i",Grafo[Ramo.IdStaz1].Distanza1,Ramo.KmRamo);
      //   ID IdNodo = Ramo.IdStaz1;
      //   puts("Percorso minimo sul grafo:");
      //   while (IdNodo != Ramo.IdStaz2) {
      //      Bprintf("   %i %s",IdNodo,Stazioni.DecodificaIdStazione(IdNodo));
      //      IdNodo = Grafo[IdNodo].Nodo1;
      //   } /* endwhile */
      //   Bprintf("   %i %s",IdNodo,Stazioni.DecodificaIdStazione(IdNodo));
      //}
   } /* endfor */


   TRACETERMINATE;

   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   return Rc;
//<<< int main(int argc,char *argv[]){
}

