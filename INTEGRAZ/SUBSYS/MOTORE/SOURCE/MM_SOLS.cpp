//========================================================================
// MM_SOLS  : Metodi delle soluzioni
//========================================================================
//

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

#include "BASE.HPP"
#include "ALFA0.HPP"
#include "MM_CRIT2.HPP"
#include "MM_PATH.HPP"
#include "MM_COMBI.HPP"
#include "mm_detta.hpp"

//----------------------------------------------------------------------------
// Defines opzionali di DEBUG
//----------------------------------------------------------------------------
//#define DBG1        // Fa vedere le dimensioni del problema di consolidamento
//#define DBG2        // Mostra semplificazione delle soluzioni
//#define DBG2A       // Mostra dettaglio semplificazione delle soluzioni
//#define DBG3        // Debug di GetNote
//#define DBG25       // trace sull' algoritmo di consolidamento
//#define DBG27       // mostra percorsi sul GRAFO (a livello trace = 2)
//#define DBG29B      // Numero di soluzioni trovate
//----------------------------------------------------------------------------
ALLOCATOR<SOLUZIONE> SOLUZIONE::Allocator(80);

//----------------------------------------------------------------------------
// MM_SOLUZIONE::new e delete
//----------------------------------------------------------------------------
void * MM_SOLUZIONE::operator new(size_t ){
   return (MM_SOLUZIONE*) SOLUZIONE::Allocator.Get();
};
void MM_SOLUZIONE::operator delete(void * Item){
   SOLUZIONE::Allocator.Free((SOLUZIONE*)Item);
};
//----------------------------------------------------------------------------
// @SOLUZIONE e @MM_SOLUZIONE
//----------------------------------------------------------------------------
SOLUZIONE::SOLUZIONE(){
   #undef TRCRTN
   #define TRCRTN "@SOLUZIONE"
   // static ULONG Prog;
   // TRACEVLONG(Prog++);
   ZeroFill(THIS);
   Valida   = Buona         = TRUE;
   Percorso                 = 0xff; // Per indicare che non e' valido
   InfoSoluzione.Clear();
   Period().Set();
};

//----------------------------------------------------------------------------
// SOLUZIONE::Semplifica
//----------------------------------------------------------------------------
// Questa funzione controlla se la soluzione possa essere semplificata
// eliminando un treno intermedio (e se si lo elimina).
// NB: Il primo e l' ultimo treno  NON sono toccati
// Inoltre verifica se il percorso possa essere migliorato riorganizzando le
// stazioni di cambio.
// Infine imposta il TipoPeriodicita della soluzione
// NB: Elimina eventuali soluzioni che, per errori di algoritmo, non
// circolassero mai
//----------------------------------------------------------------------------
void SOLUZIONE::Semplifica(){
   #undef TRCRTN
   #define TRCRTN "SOLUZIONE::Semplifica"
   
   #ifdef DBG2A
   Trace("Soluzione da semplificare:");
   #endif
   
   int i;
   
   // Due semplificazioni gestite:
   // 1: Possibilita' di collegamento diretto tra il treno della tratta i-2 ed i
   // 2: Possibilita' di cambiare ad una stazione che accorci il percorso
   
   
   for (i = NumeroTratte-1;i >=2 ;i-- ) { // 1: ....
      static ARRAY_ID Match;
      
      Match.Clear();
      
      TRATTA_TRENO &  T1 = Tratte[i-2];
      TRATTA_TRENO &  T3 = Tratte[i]  ;
      
      if(T1.InfoTreno.TipoMezzo == COLLEGAMENTO_URBANO) continue;
      if(T3.InfoTreno.TipoMezzo == COLLEGAMENTO_URBANO) continue;
      
      // Debbo accedere alle fermate dei treni
      CLU_BUFR * Buf1 = CLU_BUFR::GetCluster(T1.IdCluster , T1.Concorde, TRUE);
      CLUSTERS_STAZ_EXT & Dati1 = * Buf1->Dat;
      Buf1->FindTreno(T1.OfsTreno);
      CLUSTRENO * Treno1 = Buf1->Treno; // Debbo salvare perche' il cluster potrebbe essere lo stesso usato per la tratta 2 o 3
      
      // Come prima cosa identifico SE vi siano delle fermate in comune e quali siano
      Collider.Reset(); // Per una corretta valutazione dei tempi di soluzione
      
      // Uso gli id interni per non ridimensionare il collider
      for (int i1 = 0 ; i1 < Dati1.NumeroNodi;  i1++ ) {
         CLUSSTAZ & Staz1 = Buf1->Nodi[i1];
         INFOSTAZ & Info1 = Treno1->Nodi[i1];
         if(Info1.Arrivo){
            Collider.Set(Interno( Staz1.IdNodo));
         }
      } /* endfor */
      
      CLU_BUFR * Buf3 = CLU_BUFR::GetCluster(T3.IdCluster , T3.Concorde, TRUE);
      Buf3->FindTreno(T3.OfsTreno);
      CLUSTRENO * Treno3 = Buf3->Treno; // Debbo salvare perche' il cluster potrebbe essere lo stesso usato per la tratta 2 o 3
      for (int i3 = 0 ; i3 < Buf3->Dat->NumeroNodi;  i3++ ) {
         CLUSSTAZ & Staz3 = Buf3->Nodi[i3];
         INFOSTAZ & Info3 = Treno3->Nodi[i3];
         if(Info3.Partenza){
            if(Collider.Test(Interno( Staz3.IdNodo))){ // possibile shortcut
               Match += i3; // Conservo direttamente il puntamento
            }
         }
      } /* endfor */
      if(Match.Dim() == 0)continue; // No  match
      
      TRATTA_TRENO &  T2 = Tratte[i-1]  ;
      CLU_BUFR * Buf2 = CLU_BUFR::GetCluster(T2.IdCluster , T2.Concorde, TRUE);
      Buf2->FindTreno(T2.OfsTreno);
      // CLUSTRENO * Treno2 = Buf2->Treno; // Debbo salvare perche' il cluster potrebbe essere lo stesso usato per la tratta 2
      WORD Km2;
      if(T2.InfoTreno.TipoMezzo == COLLEGAMENTO_URBANO){
         Km2 = Buf2->Collegamento(T2.OfsIn,T2.OfsOut).Km;
      } else {
         Km2 = Buf2->PercorsiKm(T2.OfsIn,T2.OfsOut);
      }
      
      // Trovo il migliore match per collegamento diretto tra treni
      int BestMatch   = -1;
      int KmBestMatch = 9999;
      FORALL(Match,k){
         i3 = Match[k];
         ID Id = Buf3->Nodi[i3].IdNodo;
         if(Id == 0 || Interno(Id) ==0 ) continue;
         i1 = Buf1->PosNodoInCluster(Id);
         assert(i1 >= 0);
         // if( i1 >= Dati1.NumeroNodi)continue; // Non e' un nodo di cambio
         
         // Verifico che sto scegliendo la giusta direzione
         if(Treno1->Nodi[i1].ProgressivoStazione <= T1.PrgIn ) continue;
         if(Treno3->Nodi[i3].ProgressivoStazione >= T3.PrgOut) continue;
         
         // Ok ho un match: adesso debbo controllare se i tempi di coincidenza sono rispettati
         // e se il percorso viene allungato oltre misura
         // Non controllo la data di circolazione (Inutile poiche'  sono sullo stesso record del cluster)
         Buf1->FindTreno(T1.OfsTreno);
         BOOL RidottoTempoCoinc = Buf1->RidottoTempoCoinc(i1);
         Buf3->FindTreno(T3.OfsTreno);
         RidottoTempoCoinc |= Buf3->RidottoTempoCoinc(i3);
         WORD TempoMinimoCoincidenza = MM_CR.TempoCoincidenza[NODO_CAMBIO::NodiDiCambio[Interno(Id)].ClasseCoincidenza];
         if(RidottoTempoCoinc) TempoMinimoCoincidenza -= 5;
         INFOSTAZ & Info1 = Treno1->Nodi[i1];
         INFOSTAZ & Info3 = Treno3->Nodi[i3];
         
         if(TempoTrascorso(Info1.OraArrivo, Info3.OraPartenza ) < TempoMinimoCoincidenza) continue;
         
         // Debbo controllare i tempi massimi di coincidenza per evitare di avere problemi con il cambio giorno.
         // Lascio comunque 30 minuti di margine oltre la norma
         BOOL Navale    = Treno1->InfoUniforme.Navale()    || Treno3->InfoUniforme.Navale()   ;
         if (Navale) {
            if(AttesaValida(Info1.OraArrivo, Info3.OraPartenza ) > MAX_ATTESA_TRAGHETTI + 30) continue;  // Mai piu' di dodici ore
         } else {
            if(AttesaValida(Info1.OraArrivo, Info3.OraPartenza ) > MM_CR.MaxAttesa      + 30) continue;
         }
         
         // Poiche' non risparmio tempo debbo stabilire un margine di allungamento oltre il quale
         // preferisco lasciare i due treni invece di usare il cambio diretto. Al momento fisso arbitrariamente
         // 50 Km (del tutto arbitrario).
         int  DeltaKm =  Buf1->PercorsiKm(T1.OfsIn,i1) + Buf3->PercorsiKm(i3,T3.OfsOut) - // Nuova distanza
         (Buf1->PercorsiKm(T1.OfsIn,T1.OfsOut) + Buf3->PercorsiKm(T3.OfsIn,T3.OfsOut) + Km2) ; // Vecchia Distanza
         if(DeltaKm > 50) continue;
         #ifdef DBG2
         Orario.TraceIdEsterno("P2: Possibile match a stazione ",Id);
         TRACESTRING(VRS(i1)+VRS(i3) + VRS(St(Buf1->IdTrenoCorrente(i1))) + VRS(St(Buf3->IdTrenoCorrente(i3))) );
         TRACESTRING(VRS(RidottoTempoCoinc) + VRO(TempoMinimoCoincidenza)+VRO(Info1.OraArrivo)+VRO(Info3.OraPartenza));
         TRACESTRING(VRS(DeltaKm) + VRS(Info1.Arrivo) + VRS(Info1.Partenza)+ VRS(T1.Concorde)+VRS(T3.Concorde));
         #endif
         // Adesso debbo individuare il MIGLIORE match (quello con DeltaKm inferiore)
         if(DeltaKm < KmBestMatch){
            KmBestMatch = DeltaKm;
            BestMatch   = k;
         };
//<<< FORALL Match,k  
      }
      if(BestMatch >= 0){
         // OK Ho trovato una vera sostituzione !!!!
         i3 = Match[BestMatch];
         ID Id = Buf3->Nodi[i3].IdNodo;
         i1 = Buf1->PosNodoInCluster(Id);
         assert(i1 >= 0) ;
         // if( i1 >= Dati1.NumeroNodi)continue; // Non e' un nodo di cambio
         #ifdef DBG2
         Orario.TraceIdEsterno("Soluzione semplificata al nodo ",Id);
         Trace("Soluzione da semplificare");
         #endif
         
         // Debbo aggiustare la vecchia tratta e la nuova;
         INFOSTAZ & Info1 = Treno1->Nodi[i1];
         INFOSTAZ & Info3 = Treno3->Nodi[i3];
         T1.IdStazioneOut = T3.IdStazioneIn  = Id;
         if(Info1.ProgressivoStazione > T1.PrgOut){ // Ho prolungato la prima tratta
            if( Info1.OraArrivo < T1.OraOut ) T1._GiorniShiftA ++; // Passaggio attraverso la mezzanotte
         } else if(Info1.ProgressivoStazione < T1.PrgOut){ // Ho accorciato la prima tratta
            if( Info1.OraArrivo > T1.OraOut ) T1._GiorniShiftA --; // Passaggio attraverso la mezzanotte
         }
         T1.OraOut         = Info1.OraArrivo;
         T1.P_MezzoViagOut = Info1.P_MezzoViaggiante;
         T1.PrgOut         = Info1.ProgressivoStazione;
         T1.OfsOut         = i1;
         strcpy(T1.IdentOut,GRAFO::Gr()[Id].Nome7());    // Identificatore della stazione di uscita (8 caratteri)
         if(Info3.ProgressivoStazione < T3.PrgIn ){ // Ho prolungato la seconda tratta
            if( Info3.OraPartenza > T3.OraIn ) T3._GiorniShiftP --; // Passaggio attraverso la mezzanotte
         } else if(Info3.ProgressivoStazione > T3.PrgIn ) { // Ho accorciato la seconda tratta
            if( Info3.OraPartenza < T3.OraIn ) T3._GiorniShiftP ++; // Passaggio attraverso la mezzanotte
         }
         T3.OraIn          = Info3.OraPartenza;
         T3.P_MezzoViagIn  = Info3.P_MezzoViaggiante;
         T3.PrgIn          = Info3.ProgressivoStazione;
         T3.OfsIn          = i3;
         
         // Distruggo la vecchia tratta intermedia
         T2.TRATTA_TRENO::~TRATTA_TRENO();
         
         // E shifto tutte le tratte
         memmove(&T2,&T3,(NumeroTratte-i)*sizeof(TRATTA_TRENO));
         ZeroFill(Tratte[NumeroTratte-1]);  // Ad evitare possibili abend per doppia deallocazione
         NumeroTratte --;
         
         #ifdef DBG2
         Trace("Soluzione semplificata");
         #endif
//<<< if BestMatch >= 0  
      }
//<<< for  i = NumeroTratte-1;i >=2 ;i--     // 1: ....
   } /* endfor */
   
   // Adesso vedo se riesco ad individuare una stazione che accorci il percorso
   for (i = NumeroTratte-1;i >=1 ;i-- ) { // 2: ...
      static ARRAY_ID Match;
      
      Match.Clear();
      
      TRATTA_TRENO &  T1 = Tratte[i-1];
      TRATTA_TRENO &  T3 = Tratte[i]  ;
      
      if(T1.InfoTreno.TipoMezzo == COLLEGAMENTO_URBANO) continue;
      if(T3.InfoTreno.TipoMezzo == COLLEGAMENTO_URBANO) continue;
      
      // Debbo accedere alle fermate dei treni
      CLU_BUFR * Buf1 = CLU_BUFR::GetCluster(T1.IdCluster , T1.Concorde, TRUE);
      CLUSTERS_STAZ_EXT & Dati1 = * Buf1->Dat;
      Buf1->FindTreno(T1.OfsTreno);
      CLUSTRENO * Treno1 = Buf1->Treno; // Debbo salvare perche' il cluster potrebbe essere lo stesso usato per la tratta 2 o 3
      
      // Come prima cosa identifico SE vi siano delle fermate in comune e quali siano
      Collider.Reset(); // Per una corretta valutazione dei tempi di soluzione
      
      // Uso gli id interni per non ridimensionare il collider
      for (int i1 = 0;  i1 < Dati1.NumeroNodi; i1++ ) {
         CLUSSTAZ & Staz1 = Buf1->Nodi[i1];
         INFOSTAZ & Info1 = Treno1->Nodi[i1];
         if(Info1.Arrivo){
            Collider.Set(Interno( Staz1.IdNodo));
         }
      } /* endfor */
      
      CLU_BUFR * Buf3 = CLU_BUFR::GetCluster(T3.IdCluster , T3.Concorde, TRUE);
      Buf3->FindTreno(T3.OfsTreno);
      CLUSTRENO * Treno3 = Buf3->Treno; // Debbo salvare perche' il cluster potrebbe essere lo stesso usato per la tratta 2 o 3
      for (int i3 = 0 ; i3 < Buf3->Dat->NumeroNodi;  i3++ ) {
         CLUSSTAZ & Staz3 = Buf3->Nodi[i3];
         INFOSTAZ & Info3 = Treno3->Nodi[i3];
         if(Info3.Partenza){
            if(Collider.Test(Interno( Staz3.IdNodo))){
               Match += i3; // Conservo direttamente il puntamento
            }
         }
      } /* endfor */
      if(Match.Dim() <= 1)continue; // No  match oltre alla stazione di cambio
      
      ID Idx1 = T1.PrgOut, Idx3 = T3.PrgIn;
      int Sv1,Sv3;
      FORALL(Match,k){
         i3 = Match[k];
         INFOSTAZ & Info3 = Treno3->Nodi[i3];
         ID Id = Buf3->Nodi[i3].IdNodo;
         #ifdef DBG2A
         TRACESTRING(" Try " VRS(Id) + VRS(Info3.ProgressivoStazione) + VRS(Idx3));
         #endif
         if( Info3.ProgressivoStazione <= Idx3  ) continue;
         if(Id == 0 || Interno(Id) ==0 ) continue;
         i1 = Buf1->PosNodoInCluster(Id);
         assert(i1 >= 0)  ;
         // if( i1 >= Dati1.NumeroNodi)continue;
         INFOSTAZ & Info1 = Treno1->Nodi[i1];
         #ifdef DBG2A
         TRACESTRING(VRS(Info1.ProgressivoStazione) + VRS(Idx1));
         #endif
         if( Info1.ProgressivoStazione >= Idx1 ) continue;
         
         // Verifico che sto scegliendo la giusta direzione
         if(Info1.ProgressivoStazione <= T1.PrgIn ) continue;
         if(Info3.ProgressivoStazione >= T3.PrgOut) continue;
         
         // Ok ho un match: adesso debbo controllare se i tempi minimi di coincidenza sono rispettati
         // Non controllo la data di circolazione (Inutile poiche'  sono sullo stesso record del cluster)
         Buf1->FindTreno(T1.OfsTreno);
         BOOL RidottoTempoCoinc = Buf1->RidottoTempoCoinc(i1);
         Buf3->FindTreno(T3.OfsTreno);
         RidottoTempoCoinc |= Buf3->RidottoTempoCoinc(i3);
         WORD TempoMinimoCoincidenza = MM_CR.TempoCoincidenza[NODO_CAMBIO::NodiDiCambio[Interno(Id)].ClasseCoincidenza];
         if(RidottoTempoCoinc) TempoMinimoCoincidenza -= 5;
         #ifdef DBG2A
         TRACESTRING(VRS(TempoTrascorso(Info1.OraArrivo, Info3.OraPartenza )) + VRS(TempoMinimoCoincidenza));
         #endif
         if(TempoTrascorso(Info1.OraArrivo, Info3.OraPartenza ) < TempoMinimoCoincidenza) continue;
         
         // Debbo controllare i tempi massimi di coincidenza per evitare di avere problemi con il cambio giorno.
         // Lascio comunque 30 minuti di margine oltre la norma
         BOOL Navale    = Treno1->InfoUniforme.Navale() || Treno3->InfoUniforme.Navale()   ;
         if (Navale) {
            if(AttesaValida(Info1.OraArrivo, Info3.OraPartenza ) > MAX_ATTESA_TRAGHETTI + 30) continue;  // Mai piu' di dodici ore
         } else {
            if(AttesaValida(Info1.OraArrivo, Info3.OraPartenza ) > MM_CR.MaxAttesa      + 30) continue;
         }
         
         // Alleluia
         Idx1 = Info1.ProgressivoStazione;
         Idx3 = Info3.ProgressivoStazione;
         Sv1 = i1;
         Sv3 = i3;
         
         #ifdef DBG2
         Orario.TraceIdEsterno("P1: Possibile match a stazione ",Id);
         TRACESTRING(VRS(Id) + VRS(i1)+VRS(i3) + VRS(Buf1->IdTrenoCorrente(i1)) + VRS(Buf3->IdTrenoCorrente(i3)) );
         #endif
//<<< FORALL Match,k  
      }
      if(Idx1 < T1.PrgOut){ // OK Ho trovato una vera sostituzione !!!!
         i1 = Sv1; i3 = Sv3;
         ID Id = Buf3->Nodi[i3].IdNodo;
         
         // Debbo aggiustare la vecchia tratta e la nuova;
         INFOSTAZ & Info1 = Treno1->Nodi[i1];
         INFOSTAZ & Info3 = Treno3->Nodi[i3];
         T1.IdStazioneOut = T3.IdStazioneIn  = Id;
         if( Info1.OraArrivo > T1.OraOut ) T1._GiorniShiftA --; // Passaggio attraverso la mezzanotte
         T1.OraOut         = Info1.OraArrivo;
         T1.P_MezzoViagOut = Info1.P_MezzoViaggiante;
         T1.PrgOut         = Info1.ProgressivoStazione;
         T1.OfsOut         = i1;
         if( Info3.OraPartenza < T3.OraIn ) T3._GiorniShiftP ++; // Passaggio attraverso la mezzanotte
         T3.OraIn          = Info3.OraPartenza;
         T3.P_MezzoViagIn  = Info3.P_MezzoViaggiante;
         T3.PrgIn          = Info3.ProgressivoStazione;
         T3.OfsIn          = i3;
         strcpy(T1.IdentOut,GRAFO::Gr()[Id].Nome7());    // Identificatore della stazione di uscita (8 caratteri)
         
         #ifdef DBG2
         Trace("Soluzione semplificata : Stazione " VRS(Id));
         #endif
//<<< if Idx1 < T1.PrgOut   // OK Ho trovato una vera sostituzione !!!!
      }
//<<< for  i = NumeroTratte-1;i >=1 ;i--     // 2: ...
   } /* endfor */
   
   // Imposto il tipo di periodicitÖ
   PERIODICITA & Perio = (PERIODICITA &) Per;
   if (Perio == Orario.ValiditaOrario.CircolaSempre                              ) {
      TipoPeriodicita = NON_PERIODICO               ;
   } else if (Perio == Orario.ValiditaOrario.CircolaNeiLavorativi                ) {
      TipoPeriodicita = GIORNI_FERIALI              ;
   } else if (Perio == Orario.ValiditaOrario.CircolaNeiFestivi                   ) {
      TipoPeriodicita = GIORNI_FESTIVI              ;
   } else if (Perio == Orario.ValiditaOrario.CircolaNeiLavorativiEsclusoIlSabato ) {
      TipoPeriodicita = LAVORATIVI_ESCLUSO_SABATO   ;
   } else if (Perio == Orario.ValiditaOrario.CircolaTuttiIGiorniEsclusoIlSabato  ) {
      TipoPeriodicita = ESCLUSO_SABATO              ;
   } else if (Perio == Orario.ValiditaOrario.CircolaIlSabatoENeiFestivi          ) {
      TipoPeriodicita = SABATO_E_FESTIVI            ;
   } else {
      TipoPeriodicita = PERIODICITA_COMPLESSA       ;
   } /* endif */
   
   // Questo controllo non sarebbe necessario: tuttavia
   // lo faccio per eventuali errori: altrimenti avrei differenze tra
   // una richiesta e richieste successive con stessa data ed ora
   // (e' un grosso problema per internet)
   int Succ = Orario.OraOrd > OraPartenza();
   assert(Succ == SuccessivoRichiesta);
   if(!Perio.Circola(PERIODICITA::offsetPeriod + Succ)){
      assert(Perio.Circola(PERIODICITA::offsetPeriod + Succ));
      Trace("Soluzione errata: ",1);
      Per.Trace("PeriodicitÖ",1);
      Valida = FALSE; // Invalido la soluzione !
   } /* endif */
   
//<<< void SOLUZIONE::Semplifica   
}

//----------------------------------------------------------------------------
// ~ELENCO_SOLUZIONI
//----------------------------------------------------------------------------
ELENCO_SOLUZIONI::~ELENCO_SOLUZIONI(){
   #undef TRCRTN
   #define TRCRTN "~ELENCO_SOLUZIONI"
   SCAN(THIS, PSol, P_SO ){
      if(PSol)delete PSol;
   } ENDSCAN
};
//----------------------------------------------------------------------------
// ELENCO_SOLUZIONI::Clear()
//----------------------------------------------------------------------------
void ELENCO_SOLUZIONI::Clear(){
   #undef TRCRTN
   #define TRCRTN "ELENCO_SOLUZIONI::Clear()"
   SCAN(THIS, PSol, P_SO ) {
      if(PSol)delete PSol;
   } ENDSCAN
   ELENCO::Clear();
};
//----------------------------------------------------------------------------
// ELENCO_SOLUZIONI::SpostaSoluzione()
//----------------------------------------------------------------------------
// Sposta una soluzione togliendola da un elenco (lascia un puntatore NULL)
// e concatenandola ad un altro.
void ELENCO_SOLUZIONI::SpostaSoluzione(const ELENCO_SOLUZIONI& From, int Idx){
   #undef TRCRTN
   #define TRCRTN "ELENCO_SOLUZIONI::SpostaSoluzione"
   SOLUZIONE * Sol = From[Idx];
   if(Sol != NULL){
      THIS += Sol; From[Idx] = NULL;  // Sposto il puntatore
   };
};
//----------------------------------------------------------------------------
// ELENCO_SOLUZIONI::Trace
//----------------------------------------------------------------------------
void ELENCO_SOLUZIONI::Trace(const STRINGA& Msg, int Livello){
   #undef TRCRTN
   #define TRCRTN "ELENCO_SOLUZIONI::Trace"
   if(Livello >trchse)return;
   TRACESTRING(Msg);
   ORD_FORALL(THIS,i)THIS[i]->Trace("Soluzione N. "+STRINGA(i),1);
};

//----------------------------------------------------------------------------
// int SortPiu( const void *a, const void *b)
//----------------------------------------------------------------------------
int SortPiu( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "SortPiu"
   return int(((NOD*)a)->Progressivo) - int(((NOD*)b)->Progressivo);
};

//----------------------------------------------------------------------------
// SOLUZIONE::IdentificaNodiDiTransito
//----------------------------------------------------------------------------
// Metto in un' ARRAY_ID i nodi di transito o fermata
BOOL SOLUZIONE::IdentificaNodiDiTransito(ARRAY_ID & NodiDiTransito, int MinTratta, int MaxTratta){
   #undef TRCRTN
   #define TRCRTN "SOLUZIONE::IdentificaNodiDiTransito"
   
   MinTratta = max(MinTratta,0);
   MaxTratta = min(MaxTratta,NumeroTratte-1);
   
   static ARRAY_NOD Transiti(100); // Transiti DI UNA TRATTA
   
   #if defined(DBG25) || defined(DBG27)
   STRINGA Msg;
   Msg = "€€€€€€€€€€€€€Soluzione: Tratte da "+STRINGA(MinTratta) + " a "+STRINGA(MaxTratta) + " Treni =";
   for (int ii = MinTratta; ii <= MaxTratta; ii ++ )  Msg += STRINGA(Tratte[ii].IdTreno)+" ";
   TRACESTRING(Msg);
   if (!Valida) TRACESTRING("Soluzione NON VALIDA: return FALSE");
   #endif
   
   NodiDiTransito.Clear();
   if (!Valida) return FALSE;
   
   #ifdef DBG25
   Msg = "Nodi Toccati: ";
   #endif
   for (int t = MinTratta; t <= MaxTratta ;t++ ) {
      Transiti.Clear();
      
      SOLUZIONE::TRATTA_TRENO    & Tratta = Tratte[t];
      CLU_BUFR & Clu = *CLU_BUFR::GetCluster(Tratta.IdCluster , Tratta.Concorde, TRUE);
      if(&Clu == NULL){
         ERRSTRING("Errore nell' accesso ai dati dei treni: Invalido soluzione");
         Valida = FALSE;
         NodiDiTransito.Clear();
         return FALSE;
      }
      #ifdef DBG25
      if(t != MinTratta)Msg += " ∫ "; // Limite di tratta
      #endif
      
      if(Tratta.IdMezzoVirtuale == 0){ // E' una tratta MULTISTAZIONE
         if(t != MinTratta ){
            NodiDiTransito += Clu.Nodi[Tratta.OfsIn].IdNodo;
         }
         #ifdef DBG25
         TRACESTRING("Tratta N¯"+STRINGA(t)+ VRS(Tratta.IdMezzoVirtuale) + VRS(Tratta.OfsIn) + VRS(Tratta.OfsOut));
         Msg += "MS" ;
         Msg +=STRINGA(Clu.Nodi[Tratta.OfsIn].IdNodo)+ " ";
         #endif
      } else {
         
         ID Da = Clu.Nodi[Tratta.OfsIn].IdNodo;  // Prima stazione della tratta
         ID A  = Clu.Nodi[Tratta.OfsOut].IdNodo; // Ultima stazione della tratta
         NodiDiTransito += Da;
         #ifdef DBG25
         Msg += "F["+STRINGA(Clu.Nodo(Tratta.OfsIn).ProgressivoStazione)+"]";
         Msg += STRINGA(Da)+" ";
         TRACESTRING(Msg);
         #endif
         Clu.FindTreno(Tratta.OfsTreno);
         BOOL Sort= FALSE;
         BYTE Pin  = Clu.Nodo(Tratta.OfsIn).ProgressivoStazione;
         BYTE Pout = Clu.Nodo(Tratta.OfsOut).ProgressivoStazione;
         #ifdef DBG25
         TRACESTRING("Tratta N¯"+STRINGA(t)+ VRS(Tratta.IdMezzoVirtuale) + VRS(Tratta.OfsIn) + VRS(Tratta.OfsOut) + VRS(Clu.Concorde) +VRS(Pin) +VRS(Pout) );
         #endif
         // Qui deternino i limiti della scansione: si noti che se le stazioni limite
         // di tratta sono entrambe dei nodi di cambio reali e' possibile
         // scandire meno nodi limitandosi a quelli entro l' intervallo.
         // I due nodi estremi possono essere esclusi
         int MinIdx, MaxIdx;
         if (Clu.Concorde) {
            // Verifico che i nodi limite siano entrambi reali
            if ( Tratta.OfsIn < Tratta.OfsOut && Interno(A) <= Orario.BaseNodi ) {
               MinIdx = Tratta.OfsIn+1;
               MaxIdx = Tratta.OfsOut ;
            } else {
               MinIdx = 1;                   // Posso escludere il primo nodo in quanto non puo' essere un intermedio
               MaxIdx = Clu.Dat->NumeroNodi; // Posso escludere l' ultimo nodo in quanto non puo' essere un intermedio
            }
         } else {
            if ( Tratta.OfsIn > Tratta.OfsOut && Interno(Da) <= Orario.BaseNodi ) {
               MinIdx = Tratta.OfsIn-1;
               MaxIdx = Tratta.OfsOut ;
            } else {
               MinIdx = Clu.Dat->NumeroNodi - 1; // Posso escludere l' ultimo nodo in quanto non puo' essere un intermedio
               MaxIdx = 0;                       // Posso escludere il primo nodo in quanto non puo' essere un intermedio
            }
         } /* endif */
         #ifdef DBG25
         TRACESTRING("Scansione tratta da nodo Idx = " + STRINGA(MinIdx) + " a nodo Idx = " + STRINGA(MaxIdx) );
         #endif
         INFOSTAZ * Stz = &Clu.Nodo(MinIdx);
         INFOSTAZ * Lim = &Clu.Nodo(MaxIdx);
         CLUSSTAZ * Nd  = &Clu.Nodi[MinIdx];
         BYTE Last = 0;
         while ( Stz != Lim){
            INFOSTAZ & INodo = *Stz;
            // Si noti che non includo le stazioni iniziale e terminale:
            // Questo perche' potrebbero essere stazioni non di cambio
            // e quindi fuori ordine. Cio' elimina praticamente del tutto la
            // necessita' di fare i sort, tranne che per la mancata linearizzazione SUI TRANSITI
            if(  // Selezione delle stazioni di transito
               (INodo.ProgressivoStazione >  Pin ) && // Skip stazioni fuori range
               (INodo.ProgressivoStazione <  Pout) && // Skip stazioni fuori range
               (INodo.Ferma() || (INodo.TransitaOCambiaTreno && IsNodo(Nd->IdNodo))) // Stazione da considerare per il treno
            ){
               NOD Transito;
               Transito.Id          = Nd->IdNodo;
               Transito.Progressivo = INodo.ProgressivoStazione;
               Transito.Ferma       = INodo.Ferma();
               Transiti += Transito;
               if (Transito.Progressivo == 0) {
                  ERRSTRING("Errore: la stazione non dovrebbe essere valida per l' instradamento del treno");
                  BEEP;
               } /* endif */
               // ::::::::::::::::::::::::::::::::::::::::::::::::::::
               // Gestione CASO ANOMALO:
               // Causa ordinamento o percorso anomalo del treno l' ordine
               // degli ID puo' non essere lo stesso delle stazioni del treno
               // CIO' IN PARTICOLARE SUI TRANSITI
               // Ignoro eventuali stazioni fuori dell' intervallo MinIdx - MaxIdx
               // perche' verranno poi ripescate dall' algoritmo di minima distanza
               // (Possono comunque essere solo transiti)
               // ::::::::::::::::::::::::::::::::::::::::::::::::::::
               if (!Sort && INodo.ProgressivoStazione < Last) {
                  #ifdef DBG25
                  TRACESTRING("Stazione "+STRINGA(Nd->IdNodo)+" Inversione sequenza: Necessita Sort");
                  TRACEVLONG(Last);
                  #endif
                  Sort = TRUE;
               } /* endif */
               Last = INodo.ProgressivoStazione;
//<<<       if   // Selezione delle stazioni di transito
            };
            if (Clu.Concorde) {
               Stz ++;
               Nd  ++;
            } else {
               Stz --;
               Nd  --;
            } /* endif */
//<<<    while   Stz != Lim  
         } ;
         if (Sort) {
            #ifdef DBG25
            TRACESTRING("Eseguo un Sort per progressivo stazione");
            STRINGA Msg2("Transiti prima del Sort:");
            for (int j= 0; j < Transiti.Dim();j ++ ) { if(!Transiti[j].Ferma) Msg2 += "T"; Msg2 +=STRINGA(Transiti[j].Id)+ " "; }
            TRACESTRING(Msg2);
            #endif
            if(MultiThread) DOE1; // I Sort vanno semaforizzati per problemi Borland
            qsort(&Transiti[0],Transiti.Dim(),sizeof(NOD),SortPiu);
            if(MultiThread) DOE2;
            #ifdef DBG25
            Msg2 = "Transiti dopo il Sort:";
            for (j= 0; j < Transiti.Dim();j ++ ) { if(!Transiti[j].Ferma) Msg2 += "T"; Msg2 +=STRINGA(Transiti[j].Id)+ " "; }
            TRACESTRING(Msg2);
            #endif
         } /* endif */
         SCAN(Transiti,Tr,NOD){
            NodiDiTransito += Tr.Id;
            #ifdef DBG25
            Msg += Tr.Ferma ? "F[" : "T[";
            Msg += STRINGA(Tr.Progressivo)+"]";
            Msg +=STRINGA(Tr.Id)+ " ";
            #endif
         } ENDSCAN ;
//<<< if Tratta.IdMezzoVirtuale == 0   // E' una tratta MULTISTAZIONE
      }
      
      if(t == MaxTratta){ // Ultima tratta
         if(Tratta.IdMezzoVirtuale != 0) NodiDiTransito += Tratta.IdStazioneOut;
         #ifdef DBG25
         {
            WORD P_Destinazione = Tratta.OfsOut;
            Msg += "F["+STRINGA(Clu.Nodo(P_Destinazione).ProgressivoStazione)+"]";
            Msg += STRINGA(Clu.Nodi[P_Destinazione].IdNodo);
            while (Msg.Dim() > 0) {
               TRACESTRING(Msg(0,119));
               Msg = Msg(120,9999);
            } /* endwhile */
         }
         #endif
      } /* endfor */
//<<< for  int t = MinTratta; t <= MaxTratta ;t++    
   }
   
   #if defined(DBG25) || defined(DBG27)
   NodiDiTransito.Trace(GRAFO::Gr(),"NodiDiTransito");
   #endif
   
   return TRUE;
//<<< BOOL SOLUZIONE::IdentificaNodiDiTransito ARRAY_ID & NodiDiTransito, int MinTratta, int MaxTratta  
};

//----------------------------------------------------------------------------
// BOOL ELENCO_SOLUZIONI::Consolida()
//----------------------------------------------------------------------------
BOOL ELENCO_SOLUZIONI::Consolida(){
   #undef TRCRTN
   #define TRCRTN "ELENCO_SOLUZIONI::Consolida()"
   
   if (Orario.ElaborazioneInterrotta) return TRUE;
   
   #ifdef DBG1
   DWORD Elapsed;Cronometra(TRUE,Elapsed);
   #endif
   
   int Limit = Dim();
   for (int i = 0;i < Limit ;i++ ) {
      SOLUZIONE & Sol = *THIS[i];
      // ..................................
      // Metto in un' ARRAY_ID i nodi di transito o fermata
      // ..................................
      if (Sol.Valida) {
         ARRAY_ID NodiDiTransito;
         BOOL Ok = Sol.IdentificaNodiDiTransito(NodiDiTransito);
         if (!Ok)continue;        // Ho avuto qualche errore
         
         // ..................................
         // Trovo il percorso fisico corrispondente
         // ..................................
         PERCORSO_GRAFO * Pf = new PERCORSO_GRAFO(NodiDiTransito);
         #ifdef DBG27
         Pf->Nodi.Trace(GRAFO::Gr(),"Percorso Completo corrispondente ai nodi di transito");
         #endif
         
         // ..................................
         // Cerco il percorso per vedere se corrisponde ad un instradamento noto
         // ..................................
         BOOL Found = FALSE;
         ORD_FORALL(GRAFO::Gr().PercorsiGrafo,i1) {
            PERCORSO_GRAFO & Istr = *GRAFO::Gr().PercorsiGrafo[i1];
            Found = Istr == *Pf;
            if(Found){
               Sol.Percorso= i1;
               #ifdef DBG25
               TRACESTRING("Soluzione trovata tra i percorsi noti");
               #endif
               break;
            };
         } /* endfor */
         // ..................................
         // Valorizzo Variabili di filtro
         // ..................................
         if(!Found){    // L' instradamento corretto e' quello appena aggiunto
            GRAFO::Gr().PercorsiGrafo += Pf;
            Sol.Percorso= GRAFO::Gr().PercorsiGrafo.Dim()-1;
            #if defined(DBG25) || defined(DBG27)
            TRACESTRINGL("Creato un nuovo percorso / instradamento "+STRINGA(Pf->Len())+" Km Indice = "+STRINGA(Sol.Percorso)+":"+Pf->Nodi.ToStringa(),2);
            #endif
         } else {
            delete Pf;
         } /* endif */
         
         // Trace dell' istradamento corrispondente al percorso
         #ifdef DBG25
         TRACEVLONG(Sol.Percorso);
         #endif
//<<< if  Sol.Valida   
      } /* endif */
//<<< for  int i = 0;i < Limit ;i++    
   } /* endfor */
   
   #ifdef DBG1
   Cronometra(FALSE,Elapsed);
   TRACESTRING(VRS(Dim()) + VRS(Orario.Soluzioni.Dim())+ VRS(Elapsed));
   #endif
   
   #if defined(DBG29B)
   TRACESTRING("Soluzioni N¯ "+STRINGA(Orario.Soluzioni.Dim())+
      " + "+STRINGA(Dim())+" in preparazione"+
      " Instradamenti N¯"+
      STRINGA(GRAFO::Gr().Instradamenti.Dim())+" Percorsi N¯"+
      STRINGA(GRAFO::Gr().PercorsiGrafo.Dim()));
   // ORD_FORALL(Orario.Soluzioni,t1) TRACEPOINTER("Soluzione N¯ "+STRINGA(t1)+" Address:",Orario.Soluzioni[t1]);
   #endif
   
   return TRUE;
//<<< BOOL ELENCO_SOLUZIONI::Consolida   
};

//----------------------------------------------------------------------------
// SOLUZIONE::IdSol
//----------------------------------------------------------------------------
STRINGA SOLUZIONE::IdSol(){ // Ritorna una stringa compatta di identificazione della soluzione (HH:mm Staz7 -> HH:mm Staz7) [Treno ...]
   #undef TRCRTN
   #define TRCRTN "SOLUZIONE::IdSol"
   STRINGA Msg;
   Msg += "(";
   Msg += ORA(OraPartenzaVera());
   Msg += " ";
   if(Tratte[0].IdMezzoVirtuale) Msg += GRAFO::Gr()[Tratte[0].IdStazioneIn].Nome7();
//<<< TRACESTRING "Soluzioni N¯ "+STRINGA Orario.Soluzioni.Dim   +
   else Msg += GRAFO::Gr()[Tratte[1].IdStazioneIn].Nome7();
   Msg += " -> ";
   Msg += ORA(OraArrivoVera());
   Msg += " ";
   if(Tratte[NumeroTratte - 1].IdMezzoVirtuale) Msg += GRAFO::Gr()[Tratte[NumeroTratte-1].IdStazioneOut].Nome7();
   else Msg += GRAFO::Gr()[Tratte[NumeroTratte-2].IdStazioneOut].Nome7();
   Msg += " [";
   for (int i = 0; i < NumeroTratte ; i++ )  Msg += STRINGA(Tratte[i].IdTreno)+" ";
   Msg += "]";
   return Msg;
}
//----------------------------------------------------------------------------
// Trace di SOLUZIONE
//----------------------------------------------------------------------------
void MM_SOLUZIONE::Trace(const STRINGA& Mess, int Livello){
   #undef TRCRTN
   #define TRCRTN "SOL::Trace"
   if(Livello > trchse)return;
   STRINGA Msg(Mess);
   Msg += " Tempo Percorrenza: ";
   Msg += ORA(TempoTotaleDiPercorrenza);
   Msg += " Attesa: ";
   Msg += ORA(TempoTotaleDiAttesa);
   Msg += " Circol.: ";
   Msg += Circolazione_STRINGA();
   Msg += " CostoAssoluto: ";
   Msg += STRINGA(((SOLUZIONE*)this)->CostoAssoluto);
   Msg += " Costo: ";
   Msg += STRINGA(((SOLUZIONE*)this)->Costo);
   ERRSTRING(Msg);
   for(int ii =0;ii < NumeroTratte; ii++){
      Msg = "MezzoV ";
      Msg += STRINGA(Tratte[ii].IdMezzoVirtuale) + " of "+STRINGA(Tratte[ii].IdCluster);
      Msg += " Treno ";
      Msg += STRINGA(Tratte[ii].IdTreno);
      Msg += " "+ Tratte[ii].InfoTreno.Decodifica();
      Msg += " Part. nodo "+ STRINGA(Tratte[ii].IdStazioneIn);
      Msg += STRINGA(" ") + Stazioni.DecodificaIdStazione(Tratte[ii].IdStazioneIn);
      Msg += " Alle " + STRINGA(ORA(Tratte[ii].OraIn));
      Msg += " Arrivo nodo "+ STRINGA(Tratte[ii].IdStazioneOut);
      Msg += STRINGA(" ") + Stazioni.DecodificaIdStazione(Tratte[ii].IdStazioneOut);
      Msg += " Alle " + STRINGA(ORA(Tratte[ii].OraOut));
      if(Tratte[ii].TrattaCorta)Msg += " /TC/";
      ERRSTRING(Msg);
   };
//<<< void MM_SOLUZIONE::Trace const STRINGA& Mess, int Livello  
};

//----------------------------------------------------------------------------
// void _export SOLUZIONE::GetNote()
//----------------------------------------------------------------------------
void _export SOLUZIONE::GetNote(){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::GetNote"
   
   static ARRAY_ID NoteInibite  ; // Area di lavoro
   static ARRAY_ID ApplicabiliDa; // Area di lavoro
   static ARRAY_ID ApplicabiliA ; // Area di lavoro
   
   if(InfoNote && InfoNote->Linguaggio == LinguaInUso())return; // Gia' identificato l' insieme delle note
   
   #ifdef DBG3
   TRACESTRING("Acquisizione note della soluzione "+ IdSol());
   #endif
   
   if(InfoNote){
      delete InfoNote;
      // Non cancello i nomi dei treni in quanto non cambiano con il linguaggio
   }
   
   InfoNote = new INFO_NOTE;
   InfoNote->NumNoteDiVendita = 0;
   
   // Questa e' una classetta INTERNA di appoggio
   struct PREP_NOTE {
      void Add(ID Nota, int Tratta){
         WORD Idx = Note.Posizione(Nota);
         if (Idx == WORD_NON_VALIDA) {
            Note += Nota;
            TratteApplicabili += 0;
            Idx = Note.Dim() -1 ;
         } /* endif */
         TratteApplicabili[Idx] |= 1 << Tratta;
      };
      void Add(ID Nota, int Tratta , WORD IdxPeriodicita, MM_SOLUZIONE * Sol){
         #ifdef DBG3
         Bprintf3("   --- Applicabile (a parte periodicita' )");
         #endif
         if (IdxPeriodicita == 0) { // Non valida : Vedere quella propria della nota
            if(Orario.FileNot().Seek(Nota, LinguaInUso())){
               IdxPeriodicita = Orario.FileNot().RecordCorrente().IdxPeriodicita; // Sostituisco periodicita' della nota
            }
         }
         if (IdxPeriodicita == 0) { // Non ha periodicita'
            Add(Nota,Tratta);
         } else {
            // Identifico la periodicita' in forma espansa
            // Si ricorda che la periodicitÖ e' sempre espressa in termini di data di
            // partenza della soluzione (e' cioe' gia' shiftata)
            PERIODICITA & Perio = Per(IdxPeriodicita);
            // Valuto il giorno da considerare:
            int GG = PERIODICITA::offsetPeriod           ; // Data della richiesta
            GG    += Sol->SuccessivoRichiesta            ; // Data di partenza della soluzione
            // Se la periodicita' e' OK aggiungo la nota, altrimenti ignoro
            if(GG >= 0 && Perio.Circola(GG))Add(Nota,Tratta);
         } /* endif */
//<<< void Add ID Nota, int Tratta , WORD IdxPeriodicita, MM_SOLUZIONE * Sol  
      };
      
      void operator += (PREP_NOTE &  From){
         ORD_FORALL(From.Note,n){
            WORD Idx = Note.Posizione(From.Note[n]);
            if (Idx == WORD_NON_VALIDA) {
               Note += From.Note[n];
               TratteApplicabili += 0;
               Idx = Note.Dim() -1 ;
            } /* endif */
            TratteApplicabili[Idx] |= From.TratteApplicabili[n];
         }
      };
      
      BUFR TratteApplicabili;
      ARRAY_ID Note;
      void Clear(){TratteApplicabili.Clear(); Note.Clear();};
//<<< struct PREP_NOTE  
   };
   
   static PREP_NOTE PnoteV;
   static PREP_NOTE PnoteI;
   
   PnoteV.Clear();
   PnoteI.Clear();
   
   for (int n=0;n < NumeroTratte ;n++ ) {
      TRATTA_TRENO  & Tratta = Tratte[n];
      ID IdMzv = Tratta.IdMezzoVirtuale;
      if(IdMzv == 0) continue; // Tratta Multistazione
      
      // Aggiungo le note particolari:
      // - Attenzione: Breve tempo di interscambio                / Codice 0xff01
      // - Utilizzo di un treno di qualita' per una tratta corta  / Codice 0xff02
      if (n < NumeroTratte-1) {
         TRATTA_TRENO  & Tratta1 = Tratte[n+1];
         if (Tratta1.IdMezzoVirtuale) { // Non considero i tempi di scambio brevi VERSO trasporti urbani (ma ok DA trasporti urbani)
            WORD TempoInterscambio =  TempoTrascorso(Tratta.OraOut,Tratta1.OraIn);
            WORD TempoCoincidenza  = MM_CR.TempoCoincidenza[NODO_CAMBIO::NodiDiCambio[Interno(Tratta.IdStazioneOut)].ClasseCoincidenza];
            if (TempoInterscambio <= TempoCoincidenza+1) PnoteV.Add( 0xff01, n);
         } /* endif */
         // if (CattivoUsoMezzoDiQualita(Tratta)) PnoteV.Add( 0xff02, n);
      } /* endif */
      
      
      if(Orario.FileDettagli->Seek(IdMzv)){
         DETTAGLI_MV & Dettagli = Orario.FileDettagli->RecordCorrente();
         BUFR Buf;
         Orario.FileDettagliExt->Leggi(Dettagli.DimDati,Dettagli.OffsetDati,Buf);
         
         BYTE  MezziViaggianti=0;
         for (int  x = Tratta.P_MezzoViagIn; x <=  Tratta.P_MezzoViagOut; x++) {
            MezziViaggianti |= 1 << x;
         } /* endfor */
         
         // NO! la stazione seguente punta giÖ al prossimo treno
         // // Se la stazione finale della tratta e' stazione di cambio allora
         // // devo anche includere il mezzo viaggiante seguente
         // CLU_BUFR & Clu = *CLU_BUFR::GetCluster(Tratta.IdCluster , Tratta.Concorde, TRUE);
         // Clu.FindTreno(Tratta.OfsTreno);
         // INFOSTAZ & Ista = Clu.Nodo(Tratta.OfsOut);
         // if(Ista.Arrivo && Ista.Partenza && Ista.TransitaOCambiaTreno) MezziViaggianti |= 1 << x;
         
         #ifdef DBG3
         ERRSTRING(VRS(Tratta.IdMezzoVirtuale) + VRS(Dettagli.DimDati) + VRS(MezziViaggianti)+ VRS(Tratta.PrgIn) + VRS(Tratta.PrgOut));
         #endif
         // Prima scansione: Per determinare le note inibite
         Buf.Pointer = 0;
         NoteInibite .Clear();
         while (Buf.Pointer < Buf.Dim()) {
            BYTE Tipo = Buf[Buf.Pointer] & 7; // Solo 3 Bits
            switch (Tipo) {
            case NOME_MEZZO_VIRTUALE:
               {
                  NOME_MV & Rec = *(NOME_MV*)(Buf.Dati + Buf.Pointer);
                  Buf.Pointer += Rec.NumBytes;
               }
               break;
            case NOTA_COMPLESSA:
               {
                  NOTA_COMPLEX & Rec = *(NOTA_COMPLEX *)(Buf.Dati + Buf.Pointer);
                  if(Rec.TipoNota == 2 && (Rec.ProgDa == Tratta.PrgIn || Rec.ProgDa == Tratta.PrgOut)){
                     #ifdef DBG3
                     TRACESTRING("Inibita Nota N¯ "+STRINGA(Rec.IdNota)+" su tratta N¯ "+STRINGA(n));
                     #endif
                     NoteInibite += Rec.IdNota;
                  }
                  Buf.Pointer += sizeof(NOTA_COMPLEX) ;
               }
               break;
            case NOTA_MZV :
               Buf.Pointer += sizeof(NOTA_MV);
               break;
//<<<       switch  Tipo   
            case NOTA_FERMATA       :
               Buf.Pointer += sizeof(NOTA_FV);
               break;
            default:
               Buf.Pointer = Buf.Dim(); // Fine del gioco
               break;
            } /* endswitch */
//<<<    while  Buf.Pointer < Buf.Dim     
         }
         // Seconda scansione: carico i dati delle note
         Buf.Pointer = 0;
         ApplicabiliDa.Clear();
         ApplicabiliA.Clear();
         while (Buf.Pointer < Buf.Dim()) {
            BYTE Tipo = Buf[Buf.Pointer] & 7; // Solo 3 Bits
            switch (Tipo) {
            case NOME_MEZZO_VIRTUALE:
               {
                  NOME_MV & Rec = *(NOME_MV*)(Buf.Dati + Buf.Pointer);
                  #ifdef DBG3
                  Bprintf3("Nome mezzo virtuale = %s Su mezzi viaggianti %x Ok =%i",Rec.Nome,Rec.Mvg,Bool(Rec.Mvg & MezziViaggianti));
                  #endif
                  
                  // Per il nome preferisco adottare la logica di usarlo solo se e' valido
                  // alla stazione di partenza della tratta. Commentata la logica
                  // che lo abilita se un qualunque MV componente ha il nome
                  if(Rec.Mvg & (1 << Tratta.P_MezzoViagIn) ){
                     // if(Rec.Mvg & MezziViaggianti){
                     if(Tratta.NomeSpecifico == NULL)Tratta.NomeSpecifico = new STRINGA(Rec.Nome);
                  }
                  Buf.Pointer += Rec.NumBytes;
               }
               break;
            case NOTA_COMPLESSA:
               {
                  NOTA_COMPLEX & Rec = *(NOTA_COMPLEX *)(Buf.Dati + Buf.Pointer);
                  #ifdef DBG3
                  Bprintf3("Nota COMPLX Id = %i Tipo %i Evid %i da stazione Prog %i Id %i a stazione Prog %i Id %i MaxIn Prog %i Id %i PeriodicitÖ Idx = %i",
                     Rec.IdNota    , Rec.TipoNota, Rec.Evidenza,
                     Rec.ProgDa    , Clu.IdNodoDaProg(Rec.ProgDa) ,
                     Rec.ProgA     , Clu.IdNodoDaProg(Rec.ProgA),
                     Rec.ProgMaxDa , Clu.IdNodoDaProg(Rec.ProgMaxDa),
                     Rec.IdxPeriodicita);
                  #endif
                  // Debbo vedere se e' applicabile
                  BOOL Applicabile = FALSE;
                  switch (Rec.TipoNota) {
                  case 0: // MV
                     Applicabile = TRUE;
                     break;
                  case 1: // Di fermata
                     if (Rec.DueStaz) {
                        if (Rec.ProgDa == Tratta.PrgIn) {
                           if(ApplicabiliA.Contiene(Rec.IdNota)){
                              Applicabile = TRUE;
                           } else {
                              ApplicabiliDa += Rec.IdNota ;
                           } /* endif */
                        } else if (Rec.ProgDa == Tratta.PrgOut) {
                           if(ApplicabiliDa.Contiene(Rec.IdNota)){
                              Applicabile = TRUE;
                           } else {
                              ApplicabiliA += Rec.IdNota ;
                           } /* endif */
                        } /* endif */
                     } else {
                        Applicabile |= Rec.ProgDa == Tratta.PrgIn  ;
                        Applicabile |= Rec.ProgDa == Tratta.PrgOut ;
                     } /* endif */
                     break;
                  case 2: // Soppressa a fermata
                     Applicabile = TRUE;
                     // Non verifico la fermata, perchä giÖ gestito nell' inibizione
                     break;
                  case 3: // Da-A ed intermedie
                     if (Rec.DueStaz) {
                        if ( Rec.ProgDa <= Tratta.PrgIn && Tratta.PrgIn <  Rec.ProgA ) {
                           if(ApplicabiliA.Contiene(Rec.IdNota)){
                              Applicabile = TRUE;
                           } else {
                              ApplicabiliDa += Rec.IdNota ;
                           } /* endif */
                        } else if ( Rec.ProgDa < Tratta.PrgOut && Tratta.PrgOut <=  Rec.ProgA ) {
                           if(ApplicabiliDa.Contiene(Rec.IdNota)){
                              Applicabile = TRUE;
                           } else {
                              ApplicabiliA += Rec.IdNota ;
                           } /* endif */
                        } /* endif */
                        Applicabile |= Tratta.PrgIn <  Rec.ProgA  && Tratta.PrgOut >= Rec.ProgDa;
                        Applicabile &= Rec.ProgMaxDa == 0 || Tratta.PrgIn <=  Rec.ProgMaxDa;
                     } else {
                        Applicabile |= Tratta.PrgIn <  Rec.ProgA  && Tratta.PrgOut >= Rec.ProgDa;
                        Applicabile &= Rec.ProgMaxDa == 0 || Tratta.PrgIn <=  Rec.ProgMaxDa;
                     } /* endif */
                     break;
                  case 4: // Da-A esatta
                     Applicabile  = Tratta.PrgIn == Rec.ProgDa && Tratta.PrgOut == Rec.ProgA ;
                     break;
                  } /* endswitch */
                  if(NoteInibite.Contiene(Rec.IdNota))Applicabile = FALSE;
                  if(Applicabile){
                     if( Rec.Evidenza){
                        PnoteV.Add( Rec.IdNota, n, Rec.IdxPeriodicita, this);
                     } else {
                        PnoteI.Add( Rec.IdNota, n, Rec.IdxPeriodicita, this);
                     }
                  }
                  Buf.Pointer += sizeof(NOTA_COMPLEX) ;
//<<<       case NOTA_COMPLESSA:
               }
               break;
//<<<       switch  Tipo   
            case NOTA_MZV  :
               {
                  NOTA_MV & Rec = *(NOTA_MV*)(Buf.Dati + Buf.Pointer);
                  #ifdef DBG3
                  Bprintf3("Nota Id = %i Su mezzi viaggianti %x PeriodicitÖ Idx = ",Rec.IdNota,Rec.Mvg,Rec.IdxPeriodicita);
                  #endif
                  BOOL Applicabile = Rec.Mvg & MezziViaggianti;
                     if (Rec.DueStaz) {
                        BOOL OkDa = (1 << Tratta.P_MezzoViagIn ) & Rec.Mvg ;
                        BOOL OkA  = (1 << Tratta.P_MezzoViagOut) & Rec.Mvg ;
                        //if(Ista.Arrivo && Ista.Partenza && Ista.TransitaOCambiaTreno) {
                        //   OkA |= (1 << Tratta.P_MezzoViagOut + 1) & Rec.Mvg ; 
                        //}
                        Applicabile = OkDa && OkA;
                      }
                  if(NoteInibite.Contiene(Rec.IdNota))Applicabile = FALSE;
                  if(Applicabile){
                     if( Rec.Evidenza){
                        PnoteV.Add( Rec.IdNota, n, Rec.IdxPeriodicita, this);
                     } else {
                        PnoteI.Add( Rec.IdNota, n, Rec.IdxPeriodicita, this);
                     }
                  }
                  Buf.Pointer += sizeof(NOTA_MV);
               }
               break;
            case NOTA_FERMATA       :
               {
                  NOTA_FV & Rec = *(NOTA_FV*)(Buf.Dati + Buf.Pointer);
                  #ifdef DBG3
                  Bprintf3("Nota Id = %i Su fermata Prg =  %i Id %i", Rec.IdNota,Rec.PrgFermata,Clu.IdNodoDaProg(Rec.PrgFermata));
                  #endif
                  BOOL Applicabile = FALSE;
                  if (Rec.DueStaz) {
                     if (Rec.PrgFermata == Tratta.PrgIn) {
                        if(ApplicabiliA.Contiene(Rec.IdNota)){
                           Applicabile = TRUE;
                        } else {
                           ApplicabiliDa += Rec.IdNota ;
                        } /* endif */
                     } else if (Rec.PrgFermata == Tratta.PrgOut) {
                        if(ApplicabiliDa.Contiene(Rec.IdNota)){
                           Applicabile = TRUE;
                        } else {
                           ApplicabiliA += Rec.IdNota ;
                        } /* endif */
                     } /* endif */
                  } else {
                     Applicabile |= Rec.PrgFermata == Tratta.PrgIn  ;
                     Applicabile |= Rec.PrgFermata == Tratta.PrgOut ;
                  } /* endif */
                  if(NoteInibite.Contiene(Rec.IdNota))Applicabile = FALSE;
                  if(Applicabile){
                     if( Rec.Evidenza){
                        PnoteV.Add( Rec.IdNota, n, 0, this);
                     } else {
                        PnoteI.Add( Rec.IdNota, n, 0, this);
                     }
                  }
                  Buf.Pointer += sizeof(NOTA_FV);
               }
               break;
//<<<       switch  Tipo   
            default:
               BEEP;
               Buf.Pointer = Buf.Dim(); // Fine del gioco
               break;
            } /* endswitch */
//<<<    while  Buf.Pointer < Buf.Dim     
         } /* endwhile */
//<<< if Orario.FileDettagli->Seek IdMzv   
      }
//<<< for  int n=0;n < NumeroTratte ;n++    
   } /* endfor */
   
   // Imposto il numero delle note di vendita
   InfoNote->NumNoteDiVendita = PnoteV.Note.Dim();
   
   // Ho separato le note in modo da mettere prima quelle commerciali: ora le unisco
   PnoteV += PnoteI;
   
   // Consolido su InfoNote
   InfoNote->TratteApplicabili = PnoteV.TratteApplicabili;
   
   // Accedo alle informazioni testuali
   ORD_FORALL(PnoteV.Note,i1){
      // Decodifico a parte le note particolari:
      // - Attenzione: Breve tempo di interscambio                / Codice 0xff01
      // - Utilizzo di un treno di qualita' per una tratta corta  / Codice 0xff02
      if (PnoteV.Note[i1] == 0xff01) {
         InfoNote->Note += "Attenzione: Breve tempo di interscambio";
         //} else if (PnoteV.Note[i1] == 0xff02) {
         //   InfoNote->Note += "Utilizzo di un treno di qualita' per una tratta corta";
      } else if(Orario.FileNote->Seek(PnoteV.Note[i1], LinguaInUso())){
         //TRACEVLONG(PnoteV.Note[i1]);
         //TRACEVLONG(Orario.FileNote->RecordCorrente().DimDati);
         //TRACEVLONG(Orario.FileNote->RecordCorrente().OffsetDati);
         BUFR Buf2;
         Orario.FileNoteExt->Leggi(Orario.FileNote->RecordCorrente().DimDati,Orario.FileNote->RecordCorrente().OffsetDati,Buf2);
         //TRACEHEX("Buffer letto:",Buf2.Dati,Buf2.Dim());
         InfoNote->Note += STRINGA((char*)Buf2.Dati);
      } else {
         InfoNote->Note += "BAD";
         InfoNote->TratteApplicabili[i1] = 0;
      }
   }
   //InfoNote->Note.Trace();
   
   FORALL(InfoNote->Note,i2){
      #ifdef DBG3
      ERRSTRING("Nota["+STRINGA(i2)+"] "+"Tratte :"+STRINGA(InfoNote->TratteApplicabili[i2])+" = "+STRINGA(InfoNote->Note[i2])(0,100));
      #endif
      if( InfoNote->TratteApplicabili[i2] == 0){
         #ifdef DBG3
         ERRSTRING("Eliminata la Nota["+STRINGA(i2)+"] NON APPLICABILE a nessuna tratta");
         #endif
         InfoNote->Note -= i2;
         InfoNote->TratteApplicabili -= i2;
      }
   }
   
//<<< void _export SOLUZIONE::GetNote   
};
//----------------------------------------------------------------------------
// INFO_NOTE::ApplicabileATratta
//----------------------------------------------------------------------------
int _export INFO_NOTE::ApplicabileATratta(int NumTratta, int NumNota){
   #undef TRCRTN
   #define TRCRTN "INFO_NOTE::ApplicabileATratta"
   BYTE Tratte =   TratteApplicabili[NumNota];
   if((Tratte & (1 << NumTratta))==0)return 0;
   if(NumNota < NumNoteDiVendita) return 2;
   return 1;
};
//----------------------------------------------------------------------------
// MM_SOLUZIONE::DataPartenza
//----------------------------------------------------------------------------
// Data di partenza della tratta ( = giorno in cui si sale sul treno)
SDATA MM_SOLUZIONE::DataPartenza(BYTE Tr){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::DataPartenza"
   SDATA Out = PERIODICITA::DataC; // Data della richiesta
   int Offs = SuccessivoRichiesta + Tratte[Tr]._GiorniShiftP; // Offset DALLA DATA DI RICHIESTA
   while (Offs --) ++ Out;
   return Out;
};
//----------------------------------------------------------------------------
// MM_SOLUZIONE::DataArrivo
//----------------------------------------------------------------------------
// Data di arrivo della tratta ( = giorno in cui si scende dal treno)
SDATA MM_SOLUZIONE::DataArrivo(BYTE Tr){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::DataArrivo"
   SDATA Out = PERIODICITA::DataC; // Data della richiesta
   int Offs = SuccessivoRichiesta + Tratte[Tr]._GiorniShiftA; // Offset DALLA DATA DI RICHIESTA
   while (Offs --) ++ Out;
   return Out;
};
//----------------------------------------------------------------------------
// MM_SOLUZIONE::DataPartenzaMezzoVirtuale
//----------------------------------------------------------------------------
// Data in cui parte il mezzo virtuale della tratta .
SDATA MM_SOLUZIONE::DataPartenzaMezzoVirtuale(BYTE Tr){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::DataPartenzaMezzoVirtuale"
   SDATA Out = PERIODICITA::DataC; // Data della richiesta
   int Offs = SuccessivoRichiesta + Tratte[Tr]._GiorniShiftA - Tratte[Tr]._SuccPartenzaMV; // Offset DALLA DATA DI RICHIESTA
   if (Offs < 0) {
      while (Offs ++) -- Out ;
   } else {
      while (Offs --) ++ Out ;
   } /* endif */
   return Out;
};
//----------------------------------------------------------------------------
// SOL_IMAGE::Imposta
//----------------------------------------------------------------------------
// Usare "Urb" per i collegamenti urbani
void SOL_IMAGE::Imposta(const STRINGA & ElencoTreni){
   #undef TRCRTN
   #define TRCRTN "SOL_IMAGE::Imposta"
   ZeroFill(THIS);
   STRINGA Elenco2 = ElencoTreni;
   Elenco2.Strip();
   ELENCO_S Tmp = Elenco2.Tokens(" ;,/");
   ORD_FORALL(Tmp,i){
      if(Tmp[i] != STRINGA("Urb") ){ // Altrimenti vanno bene gli zeri binari che ho giÖ impostato
         strcpy(Treni[NumTratte], (CPSZ)Tmp[i].Pad(5));
      }
      NumTratte ++;
   }
};

//----------------------------------------------------------------------------
// SOL_IMAGE::Imposta
//----------------------------------------------------------------------------
void SOL_IMAGE::Imposta(SOLUZIONE & Sol){
   #undef TRCRTN
   #define TRCRTN "SOL_IMAGE::Imposta"
   NumSostituzioni ++;
   NumTratte = Sol.NumeroTratte;
   ZeroFill(Treni);
   for (int i = 0; i <  Sol.NumeroTratte; i++) {
      strcpy(Treni[i], Sol.Tratte[i].IdTreno);
   } /* endfor */
};

//----------------------------------------------------------------------------
// SOL_IMAGE::Eqv
//----------------------------------------------------------------------------
// La routine fa un confronto delle soluzione Sol con la soluzione
// sotto autodiagnostica.
// Sono possibili tre risultati:
// 0 : Non equivalenti
// 1 : Eguali
// 2 : Soluzione con lo stesso PATH_CAMBI e lo stesso 1¯ treno
//     In queste soluzioni la soluzione e' di per sä una alternativa pió rapida
//     al target
//----------------------------------------------------------------------------
int  SOL_IMAGE::Eqv(SOLUZIONE & Sol){
   #undef TRCRTN
   #define TRCRTN "SOL_IMAGE::Eqv"
   
   int NonEsattamenteEguale = 0;
   if (PATH_CAMBI::IsTargetAutoChk) {
      if(!strcmp(Treni[0], Sol.Tratte[0].IdTreno)){
         if (Treni[0][0] == 0) { // Iniziano entrambi con collegamento urbano
            if(!strcmp(Treni[1], Sol.Tratte[1].IdTreno)) NonEsattamenteEguale = 2;
         } else {
            NonEsattamenteEguale = 2;
         } /* endif */
      } /* endif */
   } /* endif */
   
   if(NumTratte != Sol.NumeroTratte)return NonEsattamenteEguale;
   for (int i = 0; i <  NumTratte; i++) {
      if(strcmp(Treni[i], Sol.Tratte[i].IdTreno)){
         // Se ho sostituito un treno ad un collegamento urbano e' lo stesso
         // Tranne che per prima ed ultima tratta
         if (
            (Treni[i][0] != 0 && Sol.Tratte[i].IdTreno[0] != 0) ||
            i == 0 || i == NumTratte - 1
         ){ 
            return NonEsattamenteEguale;
         }
      }
   } /* endfor */
   return 1;
   
//<<< int  SOL_IMAGE::Eqv SOLUZIONE & Sol  
};
//----------------------------------------------------------------------------
// SOL_IMAGE::TrenoInAttenzione
//----------------------------------------------------------------------------
// Vero se e' uno dei treni della soluzione in attenzione
   BOOL SOL_IMAGE::TrenoInAttenzione(char * IdTreno){
   #undef TRCRTN
   #define TRCRTN "SOL_IMAGE::TrenoInAttenzione"
   for (int i = 0; i <  NumTratte; i++) {
      if(!memcmp(Treni[i], IdTreno,5))return TRUE;
   } /* endfor */
   return FALSE;
}
