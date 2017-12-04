//========================================================================
// MM_PATH : classi per l' identificazione del PATH_CAMBI
//========================================================================
// Gestione della prima fase della soluzione: identificazione di una
// serie di stazioni di cambio per spostarsi dalla origine alla
// destinazione.
//========================================================================
// Si veda l' HPP per una descrizione del costo di un path
//========================================================================
//

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

#include "BASE.HPP"
#include "ALFA0.HPP"
#include "MM_CRIT2.HPP"
#include "MM_PATH.HPP"
#include "MM_PERIO.HPP"

//----------------------------------------------------------------------------
// Defines opzionali di DEBUG
//----------------------------------------------------------------------------
//#define DBG1            // Seguo NODO_CAMBIO::Risolvi : Sommario
//#define DBG2            // Mostra tempi di NODO_CAMBIO::Risolvi
//#define DBG3            // Seguo NODO_CAMBIO::Risolvi : Mostra i nodi SCANNED
//#define DBG4            // Seguo NODO_CAMBIO::Risolvi : Mostra i nodi LABELED
//#define DBG5            // Trace funzionamento di estendi
//#define DBG6            // Trace risultati di estendi
//#define DBG7            // Controlla che non vi siano nodi SCANNED che vengono riattivati
//#define DBG8            // Segue NextTentativo
//#define DBG8A           // Segue dettagli di NextTentativo
//#define DBG9            // Segue completa() : Mostra i PATH generati
//#define DBG10           // Segue completa()
//#define DBG11           // Controlla simmetria collegamenti ed altri vincoli
//#define DBG12           // Segue ResetTentativi
//#define DBG13           // Segue DiramaNext
//#define DBG14           // Mostra gli orari di OrariCompatibili
//#define DBG15           // Mostra la gestione dei paths incompatibili
//#define DBGPONTE        // Mostra la gestione dei nodi ponte
//----------------------------------------------------------------------------
//#define OKPONTE         // Abilita risoluzione via STAZIONI PONTE (al momento non implementato)
//#define KOORARI32       // Disabilita filtraggio via orari a 32 BITs
//#define OKORARI10       // Abilita filtraggio via orari a 10 BITs
//----------------------------------------------------------------------------
// Variabili statiche
//----------------------------------------------------------------------------
NODO_CAMBIO         * NODO_CAMBIO::NodiDiCambio          ;
int                   NODO_CAMBIO::TotNodiDiCambio       ;
COLLEGAMENTO        * NODO_CAMBIO::AllCollegamenti       ;
STAZN_CLUS          * NODO_CAMBIO::StzClusters           ;
WORD                  COLLEGAMENTO::Ctratte = 15         ;
WORD                  COLLEGAMENTO::C_MS    =  5         ;
WORD                  COLLEGAMENTO::Ctreni    = 10       ;
ID                    PATH_CAMBI::Origine                ; // Id Interno dell' Origine
ID                    PATH_CAMBI::Destinazione           ; // Id Interno della destinazione
WORD                  PATH_CAMBI::LastCosto              ; // Per una assert di controllo
WORD                  PATH_CAMBI::MinKm                  ; // Minimi Km di un percorso CHE DIA ORIGINE A SOLUZIONI NON SCARTATE
WORD                  PATH_CAMBI::LimKm                  ; // Limite Km dei percorsi validi
ULONG                 PATH_CAMBI::NumPaths               ; // Numero dei Paths esaminati
BOOL                  PATH_CAMBI::StopCombinazione       ; // Indica che non ho altre combinazioni
COMBINA_CLUSTER       PATH_CAMBI::IdxClusters[MAX_TRATTE]; // Puntamento ai clusters utilizzabili sul path corrente
HEAP<PATH_CAMBI>      PATH_CAMBI::HeapPercorsi(500)      ;
HASH<PATH_ILLEGALE>   PATH_CAMBI::HashIllegali(250,125)  ;
ALLOCATOR<PATH_CAMBI> PATH_CAMBI::Allocator(500)         ;
BYTE                  PATH_CAMBI::NodoC1                 ; // Nodo-citta' della stazione origine oppure 0xff
BYTE                  PATH_CAMBI::NodoC2                 ; // Nodo-citta' della stazione destinazione oppure 0xff
BYTE                  PATH_CAMBI::IsTargetAutoChk        ; // Utilizzata per autodiagnostica
COLLIDER<WORD,WORD>   Collider                           ; // Collider (usi varii)

inline int HEAP<NODO_CAMBIO>::Confronta(NODO_CAMBIO * A, NODO_CAMBIO *B){ return int(A->Costo1) - int(B->Costo1);};

//------------------------------------------------------------------------
// Grafo di connessione dei nodi di cambio
//------------------------------------------------------------------------
void NODO_CAMBIO::TraceId(const STRINGA& Msg, int Livello ){
   Orario.TraceIdInterno(Msg,(this - NodiDiCambio),Livello);
};
int sort_function_Collegamento( const void *a, const void *b){
   #define A (*(COLLEGAMENTO*)a)
   #define B (*(COLLEGAMENTO*)b)
   #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
   PROFILER::Conta(26);
   #endif
   return (int)A.CostoTotale() - (int)B.CostoTotale();
   #undef A
   #undef B
};
void NODO_CAMBIO::Estendi(){ // Esegue il SORT dei primi vicini
   #undef TRCRTN
   #define TRCRTN "NODO_CAMBIO::Estendi()"

   assert(Id != 0);
   // assert(Stato == SCANNED );

   // Debbo eseguire un sort per distanza totale dalla destinazione
   if(MultiThread) DOE1; // I Sort vanno semaforizzati per problemi Borland
   // NB: Conviene cambiare il sort. Oltretutto un InsertSort in linea sarebbe forse
   // piu' veloce. To be done.
   qsort((void *)Collegamenti(), NumeroCollegamenti, sizeof(COLLEGAMENTO), sort_function_Collegamento);
   if(MultiThread) DOE2;
   #ifdef DBG6
   TRACESTRING(VRS(THIS.Id)+VRS(Costo1)+VRS(Nodo1) +" ˆ " +STRINGA(Esterno(Nodo1)) + " " +GRAFO::Gr()[Esterno(Nodo1)].Nome7());
   #endif
   // Poiche' posso avere piu' primi vicini alla stessa distanza complessiva (anche se
   // il caso e' infrequente) metto in testa quello trovato nella fase di risoluzione,
   // altrimenti potrei avere dei comportamenti anomali
   COLLEGAMENTO * Clg = Collegamenti();
   if (Clg->Id != Nodo1) {
      #ifdef DBG5
      #ifndef DBG6
      TRACESTRING(VRS(THIS.Id)+VRS(Costo1)+VRS(Nodo1) +" ˆ " +STRINGA(Esterno(Nodo1)) + " " +GRAFO::Gr()[Esterno(Nodo1)].Nome7());
      #endif
      TRACESTRING("Altro nodo in testa SORT" VRS(Clg->Id)+VRS(Nodo1) +" ˆ " +STRINGA(Esterno(Nodo1)) + " " +GRAFO::Gr()[Esterno(Nodo1)].Nome7());
      TRACESTRING(VRS(Clg->CostoTotale())+VRS(Costo1)+VRS(NumeroCollegamenti));
      #endif
      // assert(NodiDiCambio[Clg->Id].Stato == SCANNED);
      assert( Clg->CostoTotale() == Costo1);
      COLLEGAMENTO * Wrk = Clg;
      for (int i = NumeroCollegamenti; i > 0 ; i--,Wrk++) {
         #ifdef DBG5
         TRACESTRING(VRS(Wrk->Id) +" ˆ " +STRINGA(Wrk->Eid()) + " " +GRAFO::Gr()[Wrk->Eid()].Nome7() + " "+ STRINGA(Wrk->CostoTotale()) +" Ramo: "+STRINGA(Wrk->Costo) );
         #endif
         if(Wrk->Id == Nodo1){
            COLLEGAMENTO Tmp   = *Clg; *Clg = *Wrk; *Wrk = Tmp;  // Swap
            break;
         }
      } /* endfor */
      assert(Clg->Id == Nodo1);
   }
   #ifdef DBG6
   COLLEGAMENTO * Wrk = Clg;
   TRACESTRING(" Vi sono "+STRINGA(NumeroCollegamenti)+" collegamenti:");
   for (int i = NumeroCollegamenti; i > 0 ; i--,Wrk++) {
      TRACESTRING(VRS(Wrk->Id) +" ˆ " +STRINGA(Wrk->Eid()) + " " +GRAFO::Gr()[Wrk->Eid()].Nome7()  + " "+ STRINGA(Wrk->CostoTotale()) +" Ramo: "+STRINGA(Wrk->Costo) );
   } /* endfor */
   #endif
   Esteso = TRUE;
//<<< void NODO_CAMBIO::Estendi    // Esegue il SORT dei primi vicini
}
//---------------------------------------------------------------------------
//  NODO_CAMBIO::CollegamentoDaId(ID Id);
//----------------------------------------------------------------------------
COLLEGAMENTO * NODO_CAMBIO::CollegamentoDaId(ID Target){
   #undef TRCRTN
   #define TRCRTN "NODO_CAMBIO::CollegamentoDaId"
   COLLEGAMENTO * Coll = Collegamenti();
   for (int i = NumeroCollegamenti; i > 0;i -- ) {
      if(Coll->Id == Target)return Coll;
      Coll ++;
   } /* endfor */
   return NULL;
}
//---------------------------------------------------------------------------
//  NODO_CAMBIO::Risolvi
//----------------------------------------------------------------------------
// Trova lo "shortest path tree" del grafo dei collegamenti diretti da citta' a citta'
// Si veda Tarjan : Data structures and network algorithms o analoghi testi
//----------------------------------------------------------------------------
BOOL NODO_CAMBIO::Risolvi(ID Origine, ID Destinazione){
   #undef TRCRTN
   #define TRCRTN "NODO_CAMBIO::Risolvi"

   #ifdef DBG11
   static FirstTime = TRUE;
   if (FirstTime) {
      TRACESTRING("Test Simmetria collegamenti");
      NODO_CAMBIO * Nodo = NodiDiCambio + 1; // Il primo NON e' valorizzato
      for (int i1 = TotNodiDiCambio-1; i1 > 0  ;i1 -- ) {
         if (Nodo->NumeroCollegamenti == 0) {
            TRACESTRING("Nodo isolato: " VRS(Nodo->Id));
         } /* endif */
         COLLEGAMENTO * Coll = Nodo->Collegamenti();
         for (int i2 = Nodo->NumeroCollegamenti ; i2 > 0 ; i2-- ) {
            COLLEGAMENTO * Coll1 = NodiDiCambio[Coll->Id].CollegamentoDaId(Nodo->Iid());
            if (Coll1 == NULL) {
               TRACESTRING("Collegamento asimmetrico tra nodo "+STRINGA(Nodo->Id)+" e "+STRINGA(Coll->Eid()));
            } else if (Coll1->Km != Coll->Km) {
               TRACESTRING("Collegamento asimmetrico (KM) tra nodo "+STRINGA(Nodo->Id)+" e "+STRINGA(Coll->Eid()) + VRS(Coll1->Km) + VRS(Coll->Km) );
            } else if (Coll1->Penalita != Coll->Penalita) {
               TRACESTRING("Collegamento asimmetrico (Penalita) tra nodo "+STRINGA(Nodo->Id)+" e "+STRINGA(Coll->Eid()) + VRS(Coll1->Penalita ) + VRS(Coll->Penalita ) );
            } else if (Coll1->PesoTratta != Coll->PesoTratta) {
               TRACESTRING("Collegamento asimmetrico (PesoTratta) tra nodo "+STRINGA(Nodo->Id)+" e "+STRINGA(Coll->Eid()) + VRS(Coll1->PesoTratta) + VRS(Coll->PesoTratta) );
            } /* endif */
            Coll ++;
         } /* endfor */
         Nodo ++;
      } /* endfor */
      TRACESTRING("Fine Test");
//<<< if  FirstTime
   } /* endif */
   FirstTime = FALSE;
   #endif


   #ifdef DBG2
   TRACESTRING("Inizio");
   DWORD TempoDiRisolvi, TempoDiCiclo;
   Cronometra(TRUE,TempoDiRisolvi);
   #endif

   assert(DATI_ORARIO_FS::Corrente != NULL);
   assert(Origine > 0 && Destinazione > 0);
   assert(Origine != Destinazione);


   #ifdef DBG1
   NodiDiCambio[Destinazione].TraceId(TRCRTN " Destinazione"); // Sono Id Interni
   NodiDiCambio[Origine].TraceId(TRCRTN "   Origine     ");
   TRACESTRING("Inizio Loop");
   #endif

   // ................................
   // Inits
   // ................................
   // Init Grafo connessioni
   // Poiche' debbo risolvere tutti i nodi e' piu' efficiente impostare le distanze a infinito
   // con una scansione lineare che usare il trucco del progressivo come con il grafo della rete
   #ifdef DBG1
   STRINGA Msg(Orario.EIdOrigine()); Msg += " ==> ";Msg += STRINGA(Orario.EIdDestinazione());
   Msg += VRS(TotNodiDiCambio); Msg += VRS(NodiDiCambio[TotNodiDiCambio-1].Id) ; Msg += VRS(NodiDiCambio[TotNodiDiCambio-2].Id);
   ERRSTRING(Msg);
   #endif
   SCAN_NUM(NodiDiCambio,TotNodiDiCambio,Nodo,NODO_CAMBIO){
      Nodo.Costo1     = 0xffff;
      Nodo.Nodo1      = 0 ;
      Nodo.Stato      = UNLABELED;
      Nodo.Esteso     = 0 ;
      Nodo.Ponte      = 0 ;
   } ENDSCAN ;
   // Qui dovrei inizializzare il costo di tutti i collegamenti
   // Poichä pero' SICURAMENTE ricalcolo il costo di tutti (visto che
   // visito TUTTO il grafo) posso saltare lo step

   // Init destinazione
   NodiDiCambio[Destinazione].Costo1 = 0;
   NodiDiCambio[Destinazione].Nodo1 = Destinazione;

   // Init strutture dell' algoritmo: Uso una heap ed il classico algoritmo di Djskra (= modifica di Tarjan)
   static HEAP2<NODO_CAMBIO> Heap(512);
   Heap.Insert(NodiDiCambio + Destinazione);     // Per iniziare l' algoritmo

   #ifdef DBG2
   int CountScanned  = 0;
   int CountLabeled  = 0;
   int CountTried    = 0;
   #endif
   // ................................
   // Ciclo
   // ................................
   #ifdef DBG2
   Cronometra(TRUE,TempoDiCiclo);
   #endif
   while (Heap.Dim()) {
      NODO_CAMBIO & NodoScanned = * Heap.DeleteMin();
      NodoScanned.Stato = SCANNED;

      #ifdef DBG3
      NodoScanned.TraceId("€€€€€€€ IdNodoScanned Costo = "+STRINGA(NodoScanned.Costo1)+ " NumeroCollegamenti "+STRINGA(NodoScanned.NumeroCollegamenti));
      #endif

      #ifdef DBG2
      CountScanned ++;
      #endif

      // Per tutti i primi vicini del nodo attivo
      SCAN_NUM(NodoScanned.Collegamenti(),NodoScanned.NumeroCollegamenti,CollegamentoCorrente,COLLEGAMENTO){

         NODO_CAMBIO & NextNodo   = NodiDiCambio[CollegamentoCorrente.Id];

         // Ri-Calcolo il costo del  collegamento.
         // Si noti che il calcolo viene fatto una sola volta (*2 se si considera A->B e B->A che perï in generale hanno costi diversi)
         // Il costo del collegmento e' funzione di varii parametri, tra cui
         // alcuni dipendono anche dalla richiesta
         CollegamentoCorrente.CalcolaCosto();

         #ifndef DBG7
         if(NextNodo.Stato == SCANNED)continue; // I nodi possono essere scanditi una sola volta: Speedup
         #endif


         WORD NextCosto = NodoScanned.Costo1 + CollegamentoCorrente.Costo;

         // Se una delle distanze > distanza partendo dal nodo corrente sostituisco
         if(NextNodo.Costo1 > NextCosto){
            NextNodo.Costo1 = NextCosto;
            NextNodo.Nodo1  = NodoScanned.Iid();

            #ifndef DBG7
            assert2(NextNodo.Stato != SCANNED,VRS(NodoScanned.Id)+VRS(NextNodo.Id));
            #endif

            if (NextNodo.Stato == LABELED) {
               Heap.DecrementKey(&NextNodo);
            } else {
               NextNodo.Stato  = LABELED;
               Heap.Insert(&NextNodo);
            } /* endif */
            #ifdef DBG4
            NextNodo.TraceId(" LABEL Nodo Costo = "+STRINGA(NextNodo.Costo1) +" Ramo Verso "+STRINGA(NodoScanned.Id)+ " Costo "+STRINGA(CollegamentoCorrente.Costo) );
            #endif
            #ifdef DBG2
            CountLabeled ++;
            #endif
         }
         #ifdef DBG2
         CountTried ++;
         #endif
//<<< SCAN_NUM NodoScanned.Collegamenti  ,NodoScanned.NumeroCollegamenti,CollegamentoCorrente,COLLEGAMENTO
      } ENDSCAN

//<<< while  Heap.Dim
   } /* endwhile */

   #ifdef DBG2
   Cronometra(FALSE,TempoDiCiclo);
   #endif

   Heap.Clear();

   #ifdef DBG2
   Cronometra(FALSE,TempoDiRisolvi);
   TRACESTRING("Fine, Elapsed in microsecondi = "+STRINGA(TempoDiRisolvi));
   TRACESTRING("Fine, TempoDiCiclo in microsecondi = "+STRINGA(TempoDiCiclo));
   printf("Elapsed = %i (%i)\n",TempoDiRisolvi,TempoDiCiclo);
   TRACEVLONG(CountScanned);
   TRACEVLONG(CountLabeled);
   TRACEVLONG(CountTried);
   #endif

   if(NodiDiCambio[Origine].Stato != SCANNED){ // Si ha solo se non e' stato mai attivato
      TRACESTRING("Non vi sono treni che colleghino i due nodi indicati");
      return FALSE;
   } /* endif */


   return TRUE;
//<<< BOOL NODO_CAMBIO::Risolvi ID Origine, ID Destinazione
};

//---------------------------------------------------------------------------
//  @PATH_CAMBI
//----------------------------------------------------------------------------
PATH_CAMBI::PATH_CAMBI(){
   #undef TRCRTN
   #define TRCRTN "@PATH_CAMBI"
   NumNodi = Illegale = 0;
   Km = 0;
   NextCosto = 0xffff;
   MiglioreDiramazione = 0xff;
   // Attenzione = 0;
};
PATH_CAMBI::PATH_CAMBI(PATH_CAMBI & PathFrom){
   Illegale = 0;
   Km = 0;
   NextCosto = 0xffff;
   MiglioreDiramazione = 0xff;
   Collider.Reset();
   NODO_DI_CAMBIO_ESTESO * To = Nodi;
   NODO_DI_CAMBIO_ESTESO * From = PathFrom.Nodi;
   for (int i = 0;i < PathFrom.MiglioreDiramazione ;i++ ) {
      To->SetNonDiramabile(From->Id,From->Livello);
      To ++; From ++;
   } /* endfor */
   To->SetDiramabile(From->Id, From->Livello +1); // Diramo
   From->Diramabile = 0; // Il diramante NON puo' piu' diramare a quel nodo
   NumNodi = PathFrom.MiglioreDiramazione + 1;
   PathFrom.CalcolaCostoDiramazione();
   // Attenzione = 0;
   CompletaPercorso();
};
//---------------------------------------------------------------------------
//  PATH_CAMBI::OrariCompatibili
//----------------------------------------------------------------------------
// Ritorna TRUE se l' insieme di orari di cambio alle stazioni e' compatibile
// Analogamente per il chilometraggio
// Imposta la variabile Illegale se non sono compatibili
void __fastcall PATH_CAMBI::OrariCompatibili(){
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::OrariCompatibili"

   // Attenzione = 0;

   if(Km > Orario.DistanzaLimite){
      Illegale = 5;
      return ; // Son fuori con i Km
   }

   NODO_DI_CAMBIO_ESTESO *Ne = Nodi;
   COLLEGAMENTO * LastColl =  NODO_CAMBIO::NodiDiCambio[Ne->Id].Collegamenti(Ne->Livello);
   DWORD OrariPartenzaComp =  LastColl->Partenza32;

   #ifndef KOORARI32
   Ne ++;
   BYTE Margine = (MM_CR.MaxAttesa+44) / 45;
   for (int i = 2 ; i < NumNodi ; i++) {
      COLLEGAMENTO * Coll =  NODO_CAMBIO::NodiDiCambio[Ne->Id].Collegamenti(Ne->Livello);
      // Calcolo i nuovi orari di partenza compatibili
      // Per i dettagli mettere il trace su MM_PERIA.CPP
      OrariPartenzaComp = MaskPartenzeCompatibili( OrariPartenzaComp, LastColl->Arrivo32, LastColl->TcollMin, LastColl->TcollMax,Margine);
      #ifdef DBG14
      Bprintf3("Cambio %i => %i Eid(%i) OrariPartenzaComp %X NewPartenze %X Combinazione %X",
         NODO_CAMBIO::NodiDiCambio[(Ne-1)->Id].Id,
         NODO_CAMBIO::NodiDiCambio[Ne->Id].Id,
         Coll->Eid(),
         OrariPartenzaComp,
         Coll->Partenza32,
         OrariPartenzaComp & Coll->Partenza32
      );
      #endif
      // Verifico i nuovi orari di partenza con gli orari di arrivo
      OrariPartenzaComp &= Coll->Partenza32;
      if(!OrariPartenzaComp){
         //ERRSTRING("Inibito per Orari32");
         //Attenzione = 1; break; // Per prove estemporanee: vede se filtro bene
         Illegale = 6;
         NumNodi = i+1;
         return;
      }
      LastColl = Coll;
      Ne ++;
//<<< for  int i = 2 ; i < NumNodi ; i++
   } /* endfor */
   #endif

   #ifdef OKORARI10
   if( MM_CR.LIM_180 ){    // Vedo se fare il controllo base di compatibilita' orari
      Ne = Nodi;
      LastColl =  NODO_CAMBIO::NodiDiCambio[Ne->Id].Collegamenti(Ne->Livello);
      Ne ++;
      for (int i = 2 ; i < NumNodi ; i++) {
         COLLEGAMENTO * Coll =  NODO_CAMBIO::NodiDiCambio[Ne->Id].Collegamenti(Ne->Livello);
         #ifdef DBG14
         Bprintf3("Cambio10 %i => %i => %i OrariArrivo %X OrariPartenza %X   Combinazione %X",
            NODO_CAMBIO::NodiDiCambio[(Ne-1)->Id].Id, NODO_CAMBIO::NodiDiCambio[Ne->Id].Id, Coll->Eid(), LastColl->OrariArrivo, Coll->OrariPartenza, LastColl->OrariArrivo & Coll->OrariPartenza);
         #endif
         if((Coll->OrariPartenza & LastColl->OrariArrivo) == 0){
            //ERRSTRING("Inibito per Orari10");
            Illegale = 6;
            NumNodi = i+1;
            return;
         }
         LastColl = Coll;
         Ne ++;
      } /* endfor */
//<<< if  MM_CR.LIM_180       // Vedo se fare il controllo base di compatibilita' orari
   }
   #endif

   #if defined(OKPONTE) || defined(DBGPONTE) // Abilita risoluzione via STAZIONI PONTE
   if (NumNodi >= 5) { // Ho un potenziale ponte
      for (int i = 2; i < NumNodi - 2 ; i++ ) {  // I nodi ponte vanno cercati nei nodi intermedi, con l' esclusione del primo e l' ultimo nodo di cambio
         if (NODO_CAMBIO::NodiDiCambio[Nodi[i].Id].Ponte) {
            #ifdef OKPONTE
            Illegale = 9;
            NumNodi = i+1; // Tutte le eventuali diramazioni sarebbero comunque gestite dal ponte
            #endif
            return;
         } /* endif */
      } /* endfor */
      // Imposto come ponte il nodo "centrale"
      NODO_CAMBIO::NodiDiCambio[Nodi[NumNodi/2].Id].Ponte = TRUE;
      #ifdef OKPONTE
      Illegale = 9;
      NumNodi = (NumNodi/2)+1; // Tutte le eventuali diramazioni sarebbero comunque gestite dal ponte
      #endif
      #ifdef DBGPONTE
      ERRSTRING("Impostato il nodo PONTE id = "+STRINGA(Nodi[NumNodi/2].Eid()));
      #endif
   } /* endif */
   #endif
//<<< void __fastcall PATH_CAMBI::OrariCompatibili
};
//---------------------------------------------------------------------------
//  PATH_ILLEGALE::Set()
//----------------------------------------------------------------------------
void __fastcall PATH_ILLEGALE::Set(const class  PATH_CAMBI & Da){
   #undef TRCRTN
   #define TRCRTN "PATH_ILLEGALE::Set"

   ID * To   = Nodi;
   const NODO_DI_CAMBIO_ESTESO * From = Da.Nodi;
   for (int i = Da.NumNodi;i > 0 ; i--) {
      *To = From->Id;
      To ++; From ++;
   } /* endfor */
   for (i = MAX_TRATTE + 1 - Da.NumNodi;i > 0 ; i--) {
      *To = 0; To ++;
   } /* endfor */
};
//---------------------------------------------------------------------------
//  PATH_CAMBI::CompatibilePrecedenti()
//----------------------------------------------------------------------------
void __fastcall PATH_CAMBI::CompatibilePrecedenti(){
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::CompatibilePrecedenti"

   PATH_ILLEGALE PathI;
   PathI.Set(THIS);
   for (int i = 2; i <= NumNodi ; i++) {
      if(HashIllegali.Cerca(&PathI,i+i) != NULL){ // Era effettivamente presente nei paths illegali
         Illegale = 7;
         return;
      }
   } /* endfor */
};
//---------------------------------------------------------------------------
//  PATH_CAMBI::DichiaraIncompatibile
//----------------------------------------------------------------------------
void __fastcall PATH_CAMBI::DichiaraIncompatibile(BYTE PosNodoDiCambio) {
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::DichiaraIncompatibile"

   #ifdef DBG15
   TRACESTRING("Dichiarato incompatibile dopo il nodo N¯ "+STRINGA(PosNodoDiCambio)+" Path: "+STRINGA(THIS));
   #endif

   PATH_ILLEGALE & PathI = *HashIllegali.Alloca();
   PathI.Set(THIS);
   BYTE KLen = 2* (PosNodoDiCambio+2);
   assert(HashIllegali.Cerca(&PathI,KLen) == NULL);
   HashIllegali.Metti(KLen);
};
//---------------------------------------------------------------------------
//  PATH_CAMBI:: STRINGA()
//----------------------------------------------------------------------------
// Elenco compatto dei nodi e nomi 7
PATH_CAMBI::operator STRINGA() const {
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::STRINGA()"
   STRINGA Msg = "[";
   for (int i = 0;i < NumNodi ; i ++) {
      ID Id = Nodi[i].Eid();
      Msg += STRINGA(Id);
      Msg += " ";
      Msg += GRAFO::Gr()[Id].Nome7();
      Msg += " ";
   } /* endfor */
   Msg += "]";
   return Msg;
};
//---------------------------------------------------------------------------
//  PATH_CAMBI::CalcolaCostoDiramazione()
//----------------------------------------------------------------------------
void __fastcall PATH_CAMBI::CalcolaCostoDiramazione(){
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::CalcolaCostoDiramazione()"

   NextCosto = 0xffff;
   MiglioreDiramazione = 0xff;
   NODO_DI_CAMBIO_ESTESO *  Ide = Nodi;
   WORD Costo = 0;
   for (int i = 0;i < NumNodi-1 ;i++,Ide ++ ) {
      NODO_CAMBIO * Nodo = NODO_CAMBIO::NodiDiCambio + Ide->Id;
      COLLEGAMENTO * Coll = Nodo->Collegamenti(Ide->Livello);
      if (Ide->Diramabile) {
         if (Ide->Livello < (Nodo->NumeroCollegamenti - 1)) {
            WORD NxCosto = Costo + (Coll + 1)->CostoTotale();
            if(NxCosto < NextCosto){
               NextCosto = NxCosto;
               MiglioreDiramazione = i;
            }
         } else {
            Ide->Diramabile = 0;
         } /* endif */
      } /* endif */
      Costo += Coll->Costo; // Costo fino al Prossimo nodo
   } /* endfor */
//<<< void __fastcall PATH_CAMBI::CalcolaCostoDiramazione
};
//---------------------------------------------------------------------------
//  PATH_CAMBI::CompletaPercorso()
//----------------------------------------------------------------------------
// Imposta Illegale se il percorso non e' utilizzabile
void __fastcall PATH_CAMBI::CompletaPercorso(){
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::CompletaPercorso()"

   #ifdef DBG10
   TRACESTRING("Nodi iniziali: "+STRINGA(THIS));
   #endif


   // Gestisco i nodi gia' inseriti.
   NODO_DI_CAMBIO_ESTESO *  Ide = Nodi;
   WORD Costo = 0;
   // Citta' dell' ultimo e penultimo nodo di cambio
   ID M2 = 0, M1 = 0;
   for (int i = 0;i < NumNodi-1 ;i++,Ide ++ ) {
      NODO_CAMBIO * Nodo = NODO_CAMBIO::NodiDiCambio + Ide->Id;
      M2 = M1; M1 = Nodo->Citta;   // Non distinguo le relazioni a breve percorrenza perche' perderei solo tempo
      Collider.Set(Ide->Id);
      COLLEGAMENTO * Coll = Nodo->Collegamenti(Ide->Livello);
      Km +=  Coll->Km;
      if (Ide->Diramabile) {
         if (Ide->Livello < (Nodo->NumeroCollegamenti - 1)) {
            WORD NxCosto = Costo + (Coll + 1)->CostoTotale();
            if(NxCosto < NextCosto){
               NextCosto = NxCosto;
               MiglioreDiramazione = i;
               //Bprintf3("[%i] Diramazione al nodo N¯ %i con costo stimato %i verso il nodo %i ",NumPaths,i,NextCosto,(Coll+1)->Eid());
               //Bprintf3("[%i] Costo fin qui  %i rimanente %i ",NumPaths,Costo,(Coll +1)->CostoTotale());
            }
         } else {
            Ide->Diramabile = 0;
         } /* endif */
      } /* endif */
      Costo += Coll->Costo; // Costo fino al Prossimo nodo
   } /* endfor */
   Collider.Set(Ide->Id);
   // Qui Ide punta all' ultimo nodo inserito. Diramabile per definizione se non e' la destinazione
   assert(Ide->Diramabile);
   //NODO_CAMBIO * NodoIns = NODO_CAMBIO::NodiDiCambio + Ide->Id; // Ultimo nodo del percorso
   //Bprintf3("[%i] Nodo cui e' avvenuta la diramazione N¯ %i Costo fin qui %i Rimanente %i ",NumPaths,i,Costo,NodoIns->Collegamenti(Ide->Livello)->CostoTotale());
   while( Ide->Id != Destinazione && NumNodi <= MAX_TRATTE ){
      #ifdef DBG10
      TRACEVLONG(Ide->Id);
      #endif
      NODO_CAMBIO * Nodo = NODO_CAMBIO::NodiDiCambio + Ide->Id; // Ultimo nodo del percorso
      // Per relazioni a lunga percorrenza non ammetto 2 cambi nella stessa citta'
      // Idem per relazioni in partenza od arrivo nella stessa citta'
      if(Orario.DistanzaMinima > 200 || Nodo->Citta == NodoC1 || Nodo->Citta == NodoC2 ){
         if(M2 && M2 == Nodo->Citta ){
            Illegale = 8;  // Due stazioni della stessa citta' non consecutive (per ora limito il test al secondo vicino)
            break;
            // Per relazioni a corta percorrenza mi basta non avere due percorsi urbani consecutivi:
            // Non faccio nulla perche' cio' e' gia' gestito dall' algoritmo di combinazione soluzioni
         } /* endif */
         M2 = M1; M1 = Nodo->Citta;
      };
      if(!Nodo->Esteso){ // Nodo non ancora esteso
         Nodo->Estendi();
      }
      COLLEGAMENTO * Coll = Nodo->Collegamenti(Ide->Livello); // Il livello e' <> 0 solo per il nodo verso cui ho diramato
      Km +=  Coll->Km;
      // TRACEVLONG(Costo + Coll->CostoTotale());  // Deve essere invariante
      if (Nodo->NumeroCollegamenti > (Ide->Livello + 1)) {
         WORD NxCosto = Costo + (Coll + 1)->CostoTotale();
         if(NxCosto < NextCosto){
            NextCosto = NxCosto;
            MiglioreDiramazione = i;
            // Bprintf3("[%i] Diramazione al nodo N¯ %i con costo stimato %i verso il nodo %i ",NumPaths,i,NextCosto,(Coll+1)->Eid());
            // Bprintf3("[%i] Costo fin qui  %i rimanente %i ",NumPaths,Costo,(Coll +1)->CostoTotale());
         }
      } else {
         Ide->Diramabile = 0;
      } /* endif */
      Costo += Coll->Costo; // Costo fino al Prossimo nodo
      i++;
      ID NextId = Coll->Id;
      Ide ++; NumNodi ++;
      Ide->SetDiramabile(NextId);
      // Si noti che in caso di LOOP metto comunque nel percorso il nodo ripetuto
      // Questo perche' l' ultimo nodo per definizione non e' diramabile
      if (Collider.TestAndSet(NextId)) { // LOOP
         Illegale = 1;
         break;
      } /* endif */
//<<< while  Ide->Id != Destinazione && NumNodi <= MAX_TRATTE
   }
   if(Illegale == 0){
      if( Ide->Id != Destinazione ){
         Illegale = 2; // Troppi Cambi : ecceduto limite strutturale
      } else if( Km > LimKm ){
         Illegale = 3; // Troppi Km
      } else {
         OrariCompatibili(); // Controlla casi piu' complessi
         if(Illegale == 0)CompatibilePrecedenti(); // Controllo di compatibilita'
      }
   }
   assert(Illegale || Costo >= LastCosto);
   Top(LastCosto,Costo);

   Progressivo = NumPaths ++;

   #if defined(DBG10) || defined(DBG9)
   TRACESTRING("["+STRINGA(Progressivo)+"] Illegale="+STRINGA(Illegale)+" Nodi finali: "+STRINGA(THIS)+" Km "+STRINGA(Km)+ " Costo "+STRINGA(Costo)+" NextCosto "+STRINGA(NextCosto)+" @ "+STRINGA(MiglioreDiramazione));
   #endif

   if (DATI_ORARIO_FS::AutoDiagnosticaPath && IsTarget()) {
      ERRSTRING(" Path Trovato: " +STRINGA(THIS));
      IsTargetAutoChk = TRUE;    // Utilizzata per autodiagnostica
   } else {
      IsTargetAutoChk = FALSE;   // Utilizzata per autodiagnostica
   } /* endif */

   #ifdef PROFILER_ABILITATO
   PROFILER::Conta(33);
   if(!Illegale) PROFILER::Conta(34);
   #endif

//<<< void __fastcall PATH_CAMBI::CompletaPercorso
}
//----------------------------------------------------------------------------
// PATH_CAMBI::IsTarget
//----------------------------------------------------------------------------
BOOL PATH_CAMBI::IsTarget(){
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::IsTarget"
   if(Illegale)return FALSE;
   if(NumNodi != DATI_ORARIO_FS::StazioniTarget.Dim() + 2) return FALSE;
   for (int i = 1;i < NumNodi-1 ;i++ ) {
      ID Id = NODO_CAMBIO::NodiDiCambio[Nodi[i].Id].Id;
      if(Id != DATI_ORARIO_FS::StazioniTarget[i-1] )return FALSE;
   }
   return TRUE;
};
//---------------------------------------------------------------------------
//  PATH_CAMBI::DiramaNext()
//----------------------------------------------------------------------------
// Torna FALSE se completando il percorso genera un LOOP o supera il numero delle stazioni  accettabili o simili
PATH_CAMBI * PATH_CAMBI::DiramaNext(){
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::DiramaNext()"

   PATH_CAMBI * Result;
   int Cicli = 0;

   while (NumPaths <= MM_CR.MAXPT ){
      Cicli ++;
      Result = NULL;

      // Determino quale percorso ha la minima lunghezza di diramazione
      PATH_CAMBI * Diramante;

      Diramante = HeapPercorsi.DeleteMin();
      if(Diramante == NULL){
         TRACESTRING("Fine: Non vi sono piu' percorsi");
         break;
      };
      #ifdef DBG13
      TRACESTRING("["+STRINGA(Diramante->Progressivo)+"] Dirama con " VRS(Diramante->NextCosto));
      #endif

      // Debbo diramare .
      Result = new PATH_CAMBI(*Diramante);
      #ifdef DBG13
      TRACESTRING("["+STRINGA(Result->Progressivo)+"] Generato con " + VRS(Result->NextCosto));
      #endif
      if(Diramante->PuoAncoraDiramare()){
         HeapPercorsi.Insert(Diramante);
      } else if(Diramante->Illegale || ! MultiThread){
         delete Diramante;
      } /* endif */

      if(Result->PuoAncoraDiramare()){
         HeapPercorsi.Insert(Result);
      }

      if(Result->Illegale ){
         if( !Result->PuoAncoraDiramare()){
            delete Result;
         }
         continue; // Prossimo Loop
      }

      break; // Esco dal LOOP
//<<< while  NumPaths <= MM_CR.MAXPT
   } // End While

   if(NumPaths > MM_CR.MAXPT ){
      ERRSTRING("Fine: Superato limite massimo di Paths Generabili");
      Result = NULL;
   };

   return Result;
//<<< PATH_CAMBI * PATH_CAMBI::DiramaNext
};
//---------------------------------------------------------------------------
// PATH_CAMBI::ImpostaProblema
//---------------------------------------------------------------------------
// Sono Id esterni, Imposta anche per le classi collegate
void PATH_CAMBI::ImpostaProblema(ID EOrigine, ID EDestinazione, DATI_ORARIO_FS * Orar){
   #undef TRCRTN
   #define TRCRTN "PATH_CAMBI::ImpostaProblema"
   // Imposto i coefficienti di costo per i PATH_CAMBI
   // Questi valori sono critici, e sono stati determinati con delle serie di prove
   // (si veda PROVE.TXT)
   int CoeffCtratte[2] = {  5,15 };   GetEnvArray(CoeffCtratte,2,"C_TRATTE");
   int CoeffC_MS[2]    = { 20,30 };   GetEnvArray(CoeffC_MS   ,2,"C_MS"    );
   int CoeffCtreni[2]  = {  3, 7 };   GetEnvArray(CoeffCtreni ,2,"C_TRENI" );
   // Si ricordi che poi viene moltiplicato per un coefficiente da 0 a 7: Non mettere valori troppo elevati

   COLLEGAMENTO::Ctratte = max(Orar->DistanzaMinima *  CoeffCtratte[0]/ 100, CoeffCtratte[1]);
   COLLEGAMENTO::C_MS    = max(Orar->DistanzaMinima *  CoeffC_MS[0]   / 100, CoeffC_MS[1]   );
   COLLEGAMENTO::Ctreni  = max(Orar->DistanzaMinima *  CoeffCtreni[0] / 100, CoeffCtreni[1] );

   // Per le richieste da continente a sardegna e viceversa adotto dei criteri differenti: in
   // particolare do' un peso prevalente al numero di cambi da effettuare
   if (Orario.GestioneParticolareTraghetti ){
      // Collegamento tra sardegna e continente
      COLLEGAMENTO::Ctratte *= 3     ; // Costo per cambio : triplicato (gestisce meglio i traghetti)
      ERRSTRING("¯¯¯¯¯¯¯¯¯¯¯¯  TRIPLICATO COSTO CAMBI PER SOLUZIONE CHE RICHIEDE TRAGHETTI ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯");
   } /* endif */


   TRACEVLONG(Orar->DistanzaMinima);
   TRACEVLONG(COLLEGAMENTO::Ctratte);
   TRACEVLONG(COLLEGAMENTO::C_MS   );
   TRACEVLONG(COLLEGAMENTO::Ctreni );

   Origine      = Interno(EOrigine);
   Destinazione = Interno(EDestinazione);
   NodoC1 = NODO_CAMBIO::NodiDiCambio[Origine].Citta;
   NodoC2 = NODO_CAMBIO::NodiDiCambio[Destinazione].Citta;
   if(NodoC1 == 0)NodoC1 = 0xff;
   if(NodoC2 == 0)NodoC2 = 0xff;
   Collider.Redim(NODO_CAMBIO::TotNodiDiCambio+1);
   Collider.Reset(); // Per una corretta valutazione dei tempi di soluzione
   PATH_CAMBI::MinKm  = 0xffff;
   PATH_CAMBI::LimKm  = 0xffff;
   PATH_CAMBI::NumPaths = 0;
//<<< void PATH_CAMBI::ImpostaProblema ID EOrigine, ID EDestinazione, DATI_ORARIO_FS * Orar
}
//---------------------------------------------------------------------------
// GestioneFasceOrarie
//---------------------------------------------------------------------------
// Intervalli chiusi a sinistra
static WORD FasceOrarieDiCollegamento[NUM_FASCE];
static WORD FasciaMediana;
static WORD CombinazioneFasce[NUM_FASCE][NUM_FASCE];
int  __fastcall IdentificaFasciaDiCollegamento(WORD Ora);
void Fasce_Init(){
   #undef TRCRTN
   #define TRCRTN "GestioneFasceOrarie"
   int Delta = 1440 - TempoTrascorso(INIZIO_NOTTE,FINE_NOTTE);
   Delta /= NUM_FASCE;
   int InizioFascia = FINE_NOTTE;
   //TRACEVLONG(INIZIO_NOTTE);
   //TRACEVLONG(FINE_NOTTE);
   //TRACEVLONG(Delta);
   //TRACEVLONG(InizioFascia);
   // Queste sono le fasce orarie
   for (int i= 0; i < NUM_FASCE ;i++ ) {
      FasceOrarieDiCollegamento[i] = InizioFascia;
      // Bprintf3("Fascia[%i): Inizia alle ore %s finisce alle %s",i,ORA(InizioFascia), ORA(InizioFascia + Delta));
      InizioFascia = InizioFascia + Delta;
   };
   FasciaMediana = FasceOrarieDiCollegamento[NUM_FASCE/2];
   // Precalcolo le configurazioni binarie che corrispondono alle fasce da ... a
   for (int Da= 0;Da < NUM_FASCE ; Da++ ) {
      for (int A = 0; A < NUM_FASCE ; A++ ) {
         WORD  Wrk = 1 << A;
         for (int k = Da;k != A ; k++) {
            Wrk |= 1 << k;
            if(k >= NUM_FASCE-1) k = -1;
         }
         CombinazioneFasce[Da][A] = Wrk;
      } /* endfor */
   } /* endfor */
   // Trace tutto il giorno per debug
   //for (int u = 0;u <= 1440 ; u += 10 ) {
   //   Bprintf3("Ore %i Fascia :%i",u,COLLEGAMENTO1::IdentificaFasciaDiCollegamento(u));
   //} /* endfor */
   //Bprintf3("Fasce di coincidenza per ore 1185 :%i",COLLEGAMENTO1::FasceDiCoincidenza(1185,180));
//<<< void Fasce_Init
}

// Questa funzione e' utilizzata talmente tante volte che ho cercato di ottimizzarla
int  __fastcall IdentificaFasciaDiCollegamento(WORD Ora){
   if(Ora> 1440)Ora -= 1440;
   int Fascia;
   // Dimezzo il numero di cicli ma non vado oltre con la ricerca dicotomica
   // perche' non dovrei avere ulteriori vantaggi in performance
   if(Ora >= FasciaMediana){
      Fascia = NUM_FASCE -1 ;
      WORD *  w = FasceOrarieDiCollegamento + Fascia;
      while(Ora < *w){ w--;Fascia --;};
   } else {
      Fascia = -1;
      WORD *  w = FasceOrarieDiCollegamento;
      while(Ora >= *w){ w++;Fascia ++;};
      if(Fascia < 0)Fascia += NUM_FASCE;
   } /* endif */
   return Fascia;
};

WORD __fastcall FasciaOraria(WORD Ora){
   if (FasceOrarieDiCollegamento[0] == 0 )Fasce_Init();
   int Fascia = IdentificaFasciaDiCollegamento(Ora);
   return 1 << Fascia;
};

WORD __fastcall FasceDiCoincidenza(WORD OraDiArrivo, WORD MaxTempoDiCoincidenza){
   if (FasceOrarieDiCollegamento[0] == 0 )Fasce_Init();
   int Ora1 = OraDiArrivo + 5;                     // 5 minuti di margine MINIMO per le coincidenze
   int Ora2 = OraDiArrivo + MaxTempoDiCoincidenza; // Massimo tempo di attesa ammesso
   WORD Fascia1 = IdentificaFasciaDiCollegamento(Ora1);
   WORD Fascia2 = IdentificaFasciaDiCollegamento(Ora2);
   return CombinazioneFasce[Fascia1][Fascia2];
};

