//----------------------------------------------------------------------------
// PROVANOT.CPP: Prova per le note e servizi
//----------------------------------------------------------------------------
// Controlla se le periodicit… fornite su INFOCOMM siano relative
// alla stazione di inizio servizio od alla partenza del treno
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
// #pragma option -Od
#include "FT_PATHS.HPP"  // Path da utilizzare



#include "oggetto.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_IN.HPP"
#include "elenco.h"
#include "eventi.h"
#include "seq_proc.hpp"
#include "file_t.hpp"

#define PGM      "PROVANOT"

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int  main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"
   
   int Rc = 0;
   
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   SetStatoProcesso(PGM, SP_PARTITO);
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
   SetPriorita(); // Imposta la priorita'
   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore
   
   TryTime(0);
   
   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   
   // Apro l' archivio decodifica codici CCR
   CCR_ID Ccr_Id(PATH_DATI);
   CCR_ID::CcrId = & Ccr_Id;
   
   
   // Ottengo i dati dei treni
   PTRENO::Restore(); // Parto dai dati caricati l' ultima volta
   
   STRINGA PathDa(PATH_IN),ExtFl(".T");
   if (getenv("USADATIESTRATTI")){ PathDa=PATH_OUT;ExtFl=".XTR";};
   FILE_TABORARI TabOr(PathDa + "TABORARI" + ExtFl);
   FILE_INFOCOMM InfoComm(PathDa + "INFOCOMM" + ExtFl);
   
   PTRENO * TrenoCorrente=NULL;
   DWORD  CcrInizioNota;
   BOOL   Try;
   int NumNota;
   
   // Debbo verificare
   // - Che la periodicit… della nota non sia "tutti i giorni"
   // - Che la stazione di inizio nota sia ad una data successiva rispetto alla stazione iniziale
   ORD_FORALL(InfoComm,i2){
      TryTime(i2);
      INFOCOMM & Ic = InfoComm[i2];
      if (Ic.TipoRecord == '1') {
         TrenoCorrente = PTRENO::Get(Ic.R1.IdentTreno) ;
         Try = FALSE;
      } else if (Ic.TipoRecord == '2'){
         NumNota = It(Ic.R2.Nota);
         Try = Ic.R2.StazioneInizioServizio != TrenoCorrente->CcrDa;
         Try &= Ic.R2.StazioneInizioServizio.Cod2() != 0;
         int PLoc, PTab;
         if(Try)Try &= TrenoCorrente->CercaCCR(Ic.R2.StazioneInizioServizio.Cod2(), PLoc, PTab);
         if(Try){
            CcrInizioNota = Ic.R2.StazioneInizioServizio.Cod2();
            TABORARI & To = TabOr[PTab];
            assert(To.TipoRecord == '5');
            if(To.R5.Partenza.MinMz() == 0 ||
                  To.R5.Partenza.MinMz() >= TrenoCorrente->OraPartenzaTreno)Try = FALSE;
         }
      } else if (Ic.TipoRecord == '3' && Try){
         T_PERIODICITA PeriodicitaNota;
         PeriodicitaNota.ComponiPeriodicita(5,Ic.R3.Periodicita,TRUE);
         while(i2 < InfoComm.Dim() -1 && InfoComm[i2+1].TipoRecord == '3') PeriodicitaNota.ComponiPeriodicita(5,InfoComm[++i2].R3.Periodicita,FALSE);
         if( PeriodicitaNota != T_PERIODICITA::InLimitiOrario ){
            T_PERIODICITA PeriodicitaNota2 = PeriodicitaNota;
            if( TrenoCorrente->PeriodicitaTreno == PeriodicitaNota){
               Bprintf("Treno %s Nota %i Stazione CCR %i Nota con periodicit… riferita alla partenza",
                  St(TrenoCorrente->IdTreno), NumNota, CcrInizioNota);
            } else if( TrenoCorrente->PeriodicitaTreno.ConfrontaShiftando(PeriodicitaNota2,1)){
               Bprintf("Treno %s Nota %i Stazione CCR %i Nota con periodicit… riferita alla stazione di cambio",
                  St(TrenoCorrente->IdTreno), NumNota, CcrInizioNota );
            } else {
               Bprintf("Treno %s Nota %i Stazione CCR %i Nota con periodicit… differente",
                  St(TrenoCorrente->IdTreno), NumNota, CcrInizioNota );
            } /* endif */
            PeriodicitaNota.Trace(STRINGA("Periodicita' nota ") +STRINGA(NumNota)+" del treno: "+St(TrenoCorrente->IdTreno));
            TrenoCorrente->PeriodicitaTreno.Trace(STRINGA("Periodicita' del treno: ")+St(TrenoCorrente->IdTreno));
         }
//<<< if  Ic.TipoRecord == '1'   
      };
//<<< ORD_FORALL InfoComm,i2  
   };
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return Rc;
//<<< int  main int argc,char *argv    
};
