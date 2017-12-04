//----------------------------------------------------------------------------
// ML_KMECZ.CPP: Forzatura Km eccezionali per test modifiche
//----------------------------------------------------------------------------
// Modifica i dati chilometrici dei collegamenti diretti (tra nodi di cambio o meno)
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
#include "mm_grafo.hpp"
#include "mm_path.hpp"

#include "FT_PATHS.HPP"  // Path da utilizzare

#define PGM      "ML_KMECZ"

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
      // if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari

      DosError(2); // Disabilita Popup di errore

      SetPriorita(); // Imposta la priorita'
      
      GESTIONE_ECCEZIONI_ON
      TryTime(0);
      

      // ---------------------------------------------------------
      // Mi carico le correzioni al grafo
      // ---------------------------------------------------------
      // Il file KmEccz.MIO deve stare sulla directory corrente
      // Crea sulla directory DEL GRAFO KmEccz.DB
      CORREZIONI_RAMI * Ecz;
      int TotCorrezioniRami;
      {
         STRINGA Linea;
         FILE_RO CorrIn("KmEccz.MIO");
         FILE_RW CorrOut(PATH_DATI "MM_KMECZ.DB");
         CorrOut.SetSize(0);
      
         while (CorrIn.gets(Linea)){
            if(Linea.Dim() == 0 )continue;
            if(Linea[0] == ';') continue ; // Commento
            ELENCO_S Toks = Linea.Tokens(",");
            if(Toks.Dim() < 3)continue   ;
            CORREZIONI_RAMI Correzione;
            Correzione.Id1       = Toks[0].ToInt();
            Correzione.Id2       = Toks[1].ToInt();
            Correzione.KmDaUsare = Toks[2].ToInt();
            CorrOut.Scrivi(VRB(Correzione));
            // Ramo inverso
            Correzione.Id2       = Toks[0].ToInt();
            Correzione.Id1       = Toks[1].ToInt();
            Correzione.KmDaUsare = Toks[2].ToInt();
            CorrOut.Scrivi(VRB(Correzione));
         } /* endwhile */
         CorrOut.Posiziona(0);
         BUFR * Buf = CorrOut.Leggi(CorrOut.FileSize());
         Ecz = (CORREZIONI_RAMI*)Buf->Dati;
         TotCorrezioniRami = Buf->Dim() / sizeof( CORREZIONI_RAMI );
      }

      FIX_COLLEGAMENTO_F CollegamentiCambio(PATH_OUT "MM_COLL.DB");
      CollegamentiCambio.PermettiScrittura();
      ORD_FORALL(CollegamentiCambio,c){
         COLLEGAMENTO_F & Rec =  CollegamentiCambio[c];
         CORREZIONI_RAMI * Ecz2 = Ecz;
         for (int i = TotCorrezioniRami; i > 0 ; i--,Ecz2 ++ ){
            if(Ecz2->Id1 == Rec.StazionePartenza && Ecz2->Id2 == Rec.StazioneArrivo){ 
               Rec.KmMinimi = Ecz2->KmDaUsare;
               CollegamentiCambio.ModifyCurrentRecord();
            }
         }
      }


      // ---------------------------------------------------------
      // Fine
      // ---------------------------------------------------------
      TryTime(0);
      GESTIONE_ECCEZIONI_OFF
      TRACETERMINATE;
      
      
   return 0;
//<<< void main(int argc,char *argv[]){
}
