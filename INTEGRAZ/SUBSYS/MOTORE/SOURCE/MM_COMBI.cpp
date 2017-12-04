//========================================================================
// MM_COMBI : classi per la fase di combinazione dei treni ed identificazione soluzioni
//========================================================================
//

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

#include "BASE.HPP"
#include "ALFA0.HPP"
#include "MM_CRIT2.HPP"
#include "MM_PATH.HPP"
#include "MM_COMBI.HPP"

//----------------------------------------------------------------------------
// Defines opzionali di DEBUG
//----------------------------------------------------------------------------
//#define DBG_AUTOCHECK // Equivale a DBG5 + DBG5A + DBG29 + SELECT_AUTOCHECK
//#define DBG1      // Mostra clusters utilizzabili per tratta
//#define DBG2      // Segue caricamento dati dei collegamenti diretti                   | Vedere anche
//#define DBG2A     // Segue caricamento dati dei collegamenti diretti TRENO PER TRENO   | MM_CRITE.CPP
//#define DBG3      // Mostra costruttore e distruttore di IPOTESI
//#define DBG4      // Segui TrovaSoluzione
//#define DBG5      // ESPLODE dettagli interni di TrovaSoluzione e Posiziona: usare SOLO a path selezionato
//#define DBG5A     // Mostra dati presenti sui collegamenti diretti per la soluzione selezionata
//#define DBG6      // Dice quante soluzioni ho trovato su ogni PATH
//#define DBG7      // Debug dei cluster (Minimale)
//#define DBG7A     // Debug dei cluster (Esteso)
//#define DBG7B     // Debug dei cluster (Anche periodicita' : NON richiede DBG7A )
//#define DBG7C     // Limita il debug dei cluster ai soli dati delle stazioni
//#define DBG8      // Funzionamento interno di CalcolaLimiteTreni
//#define DBG23     // Compilazione opzionale : Mostra le soluzioni trovate (a livello trace = 2)
//#define DBG29     // Compilazione opzionale : mostra il PATH
//#define DBG30     // Mostra soluzione in preparazione
//#define DBG34     // Mostra periodicita' di soluzione
//#define DBG34A    // Mostra periodicita' tratta per tratta e combinazione periodicita
//#define DBG35     // Mostra le chiamata a getdati
//----------------------------------------------------------------------------
//#define IN_WORK   // Compila le parti che sono in lavorazione
//----------------------------------------------------------------------------
// Per facilitare il debug: Alterazioni dell' algoritmo
//#define SELECT_AUTOCHECK // Seleziona (per il trace) le stazioni del path di autocheck
//#define SELECT_STAZIONI  { 21 ,152 ,166 ,273 ,112}// Seleziona solo un dato insieme ordinato di stazioni di cambio
//#define SELECT_STAZIONI2 { 146 ,112 ,949 }; // Seleziona solo un dato insieme ordinato di stazioni di cambio
//#define SELECT_GEN_STAZIONI { 73 , 90 ,  75 , 84 , 7, 113 }; // Elimina soluzioni con stazioni di cambio al di fuori di tale insieme
// La selezione sui cluster funziona solo per il TRACE
//#define SELECT_CLUSTERS { 1021 };
//#define SELECT_SOLO_TRACE  // Se definito anche la selezione per le stazioni funziona solo per i trace
//----------------------------------------------------------------------------
#ifdef DBG_AUTOCHECK // Equivale a DBG5 + DBG5A + DBG29 + SELECT_AUTOCHECK
#define DBG5
#define DBG5A
#define DBG29
#define SELECT_AUTOCHECK
#endif
#if ( defined(DBG5) || defined(DBG5A)) && !( defined(SELECT_STAZIONI) || defined(SELECT_GEN_STAZIONI) || defined(SELECT_AUTOCHECK))
#error "USARE DBG5 / DBG5A solo selezionando il PATH"
#endif
#if  defined(SELECT_STAZIONI) || defined(SELECT_GEN_STAZIONI)
BOOL Traccia = TRUE;
#else
BOOL Traccia = FALSE;
#endif
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Variabili statiche
//----------------------------------------------------------------------------
CACHE<COLLEGAMENTO_DIRETTO>         IPOTESI::Cache(256,4)                    ; // Per ora MAX 256 elementi, poi vediamo
CLUSTER                          *  CLUSTER::Clusters                        ; // Dati dei clusters
int                                 CLUSTER::NumClusters                     ; // Numero di clusters presenti (elemento 0 e' vuoto)
ELENCO_SOLUZIONI                    IPOTESI::Soluzioni                       ; // Contiene le soluzioni trovate: Poi trasferite su DATI_ORARIO_FS
WORD                                IPOTESI::NextTreno                       ; // Per TrovaSoluzione
BOOL                                IPOTESI::UsaDatiInCache                  ; // Quando e' vera inibisco gli I/O ma continuo a lavorare con i dati che ho gia' in memoria
WORD                                COLLEGAMENTO_DIRETTO::OraLimite          ; // Per i treni con ora < OraLimite considero un passaggio attraverso la mezzanotte
WORD                                COLLEGAMENTO_DIRETTO::OraMinimaPartenza  ; // Per i metodi di posizionamento. E' eguale a OraLimite per tutte le tratte tranne la prima (per cui non e' definita)
WORD                                COLLEGAMENTO_DIRETTO::OraMassimaArrivo   ; // Per IdentificaAlternative
WORD                                COLLEGAMENTO_DIRETTO::GiorniDaInizio     ; // Per i metodi di posizionamento
WORD                                COLLEGAMENTO_DIRETTO::LastOraArrivo      ; // Per calcolare l' attesa valida
BYTE                                COLLEGAMENTO_DIRETTO::LastMultistazione  ; // Per i metodi di posizionamento
T_TRENO                          *  COLLEGAMENTO_DIRETTO::Last               ; // Per i metodi di posizionamento
BOOL                                T_TRENO::PerDirettiPrenotabili           ; // Questo flag inibisce in modo assoluto il controllo di periodicita' e le selezioni sui treni

//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::InitForCache()
//----------------------------------------------------------------------------
BOOL __fastcall COLLEGAMENTO_DIRETTO::InitForCache(void *, void *){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::InitForCache()"
   
   T_CLUST Clu[64]; // Per caricare i dati: poi copio
   NumClusters     = 0;
   MinCoincidenza  = MM_CR.TempoCoincidenza[NODO_CAMBIO::NodiDiCambio[Key.Arrivo].ClasseCoincidenza];
   NumTreni        = 0;
   NumTreniAllocati= 0;
   LimiteTreni     = 0;
   TrenoCorrente   = 0;
   DatiValidi      = 0;
   MultiStaz       = 0;
   
   STAZN_CLUS  * Clu1   = NODO_CAMBIO::StzClusters + NODO_CAMBIO::NodiDiCambio[Key.Partenza].IdxClusters;
   STAZN_CLUS  * Clu2   = NODO_CAMBIO::StzClusters + NODO_CAMBIO::NodiDiCambio[Key.Arrivo  ].IdxClusters;
   int L1 = NODO_CAMBIO::NodiDiCambio[Key.Partenza].NumeroClusters;
   int L2 = NODO_CAMBIO::NodiDiCambio[Key.Arrivo  ].NumeroClusters;
   int P1=0,P2 = 0;
   while (P1 < L1 && P2 < L2) {   // Il ciclo e' un merge scan: I clusters sono ordinati per ID
      if (Clu1->IdCluster == Clu2->IdCluster) { // Possibile Match
         // Verifica Gruppi
         if(Clu1->Gruppi & Clu2->Gruppi) {
            CLUSTER  & Clst  = CLUSTER::Clusters[Clu1->IdCluster];
            if (Clst.Tipo == CLUSTER_MULTISTAZIONE) {
               // I dati vanno riportati direttamente sul COLLEGAMENTO_DIRETTO
               // Accedo pertanto in linea ai dati del cluster
               // Se sono in fase di estensione pero' non accedo ai dati (altrimenti potrei creare abend)
               CLU_BUFR * Buf = CLU_BUFR::GetCluster(Clu1->IdCluster , TRUE, !IPOTESI::UsaDatiInCache);
               if (Buf == NULL) {
                  if(!IPOTESI::UsaDatiInCache){ // Altrimenti e' OK, sono in modo estensione
                     BEEP;
                  }
               } else {
                  int PIn  = Buf->PosNodoInCluster(Esterno(Key.Partenza));
                  int POut = Buf->PosNodoInCluster(Esterno(Key.Arrivo  ));
                  if(PIn < 0 || POut < 0){
                     BEEP;
                  } else {
                     COLLEG_URBANO & Clg = Buf->Collegamento(PIn,POut);
                     if (Clg.Minuti > 0) {
                        MultiStaz = TRUE;
                        MS_Minuti = Clg.Minuti;
                        MS_Km     = Clg.Km;
                        MS.IdCluster   = Clu1->IdCluster ;
                        MS.DatiCluster = Buf;
                        MS.PNodoIn     = PIn;
                        MS.PNodoOut    = POut;
                     } /* endif */
                  } /* endif */
//<<<          if  Buf == NULL   
               } /* endif */
//<<<       if  Clst.Tipo == CLUSTER_MULTISTAZIONE   
            } else {
               T_CLUST  & Comb  = Clu[NumClusters];
               Comb.Concorde    = Clu1->Progressivo <= Clu2->Progressivo;
               Comb.IdCluster   = Clu1->IdCluster;
               Comb.DatiCluster = NULL;
               Comb.PNodoIn     = 0;
               Comb.PNodoOut    = 0;
               // Elimino Clusters che non ho se sono in fase di estensione
               if(! (IPOTESI::UsaDatiInCache &&
                     (CLU_BUFR::GetCluster(Comb.IdCluster ,Comb.Concorde, FALSE) == NULL))
               ){
                  #ifdef DBG1
                  Bprintf3("Tratta %i => %i  Servita da cluster %c%i", Esterno(Key.Partenza),Esterno(Key.Arrivo),(Comb.Concorde ? ' ':'#'), Clu1->IdCluster);
                  #endif
                  if (NumClusters == 63) {
                     TRACESTRING("Raggiunto limite strutturale di 63 clusters");
                     break;
                  } else {
                     NumClusters ++;
                  } /* endif */
               }
//<<<       if  Clst.Tipo == CLUSTER_MULTISTAZIONE   
            } /* endif */
//<<<    if Clu1->Gruppi & Clu2->Gruppi   
         }
         Clu1 ++; Clu2 ++; P1 ++; P2 ++;
//<<< if  Clu1->IdCluster == Clu2->IdCluster    // Possibile Match
      } else if (Clu1->IdCluster < Clu2->IdCluster) {
         Clu1 ++; P1 ++;
      } else {
         Clu2 ++; P2 ++;
      } /* endif */
//<<< while  P1 < L1 && P2 < L2      // Il ciclo e' un merge scan: I clusters sono ordinati per ID
   } /* endwhile */
   
   Illegale = !(MultiStaz || NumClusters);
   if (Illegale) {
      NumClusters = 0;
      Clusters = NULL;
      #if defined(DBG1) || defined(DBG3) || defined(DBG6)
      Bprintf3("Tratta %i => %i  NON avendo treni la dichiaro illegale",Esterno(Key.Partenza),Esterno(Key.Arrivo));
      #endif
   } else if(NumClusters > 0){
      Clusters = new T_CLUST[NumClusters];
      memmove(Clusters,Clu, NumClusters * sizeof(T_CLUST)); // Copio i dati trovati
   } /* endif */
   
   #ifdef DBG1
   Bprintf3("Tratta %i => %i  i treni sono ripartiti su %i clusters",Esterno(Key.Partenza),Esterno(Key.Arrivo),NumClusters);
   #endif
   return TRUE;
//<<< BOOL __fastcall COLLEGAMENTO_DIRETTO::InitForCache void *, void *  
};
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::RemoveFromCache()
//----------------------------------------------------------------------------
void __fastcall COLLEGAMENTO_DIRETTO::RemoveFromCache(void *){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::RemoveFromCache()"
   
   #ifdef DBG1
   Bprintf3("Tratta %i => %i  rilascio dati dei treni e dei clusters",Esterno(Key.Partenza),Esterno(Key.Arrivo));
   #endif
   if(NumClusters) delete[] Clusters;
   if(NumTreniAllocati ) delete[] Treni   ;
   NumClusters = NumTreni = NumTreniAllocati = 0;
};

//----------------------------------------------------------------------------
// Sort dei treni per ora di partenza
//----------------------------------------------------------------------------
// I treni non validi vanno in fondo!
int SortTreni( const void *a, const void *b){
   T_TRENO & A = *(T_TRENO*)a;
   T_TRENO & B = *(T_TRENO*)b;
   if(A.Valido != B.Valido) return A.Valido ? -1 : 1;
   return int(A.OraPartenza) - int(B.OraPartenza);
};

//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::CalcolaLimiteTreni
//----------------------------------------------------------------------------
// Questa funzione calcola il LimiteTreni
// Come step preparatorio ordina i treni e filtra via i treni non validi
//
//  Il problema e' il seguente : data la serie di orari di arrivo dei
//  treni (disposti in ordine di ora di partenza) trovare un intero l
//  tale che per ogni coppia di treni T1,T2 tali che Pos(T2) - Pos(T1) >= l
//  si abbia che l' ora di arrivo di T2 > ora di arrivo di T1.
//
//  Condizione necessaria e sufficiente e' la seguente: per ogni treno T1
//  i treni  T1+l ... T1+2l debbono avere ora di arrivo > ora di arrivo di T1
//
//  Chiamiamo tale condizione (= Logical function) Vr(T1,l) si avra' allora
//  che si deve trovare il minimo intero l tale che Vr(T1,l) sia vera per tutti i T1.
//
//  Si puo' evitare di fare tutti i confronti: infatti (Indicando con OA l' ora arrivo )
//  Se OA(T1+1) > OA(T1) si ha :
//     ( OA(T1+l) > OA(T1) & Vr(T1+1,l) ) Implica Vr(T1,l)
//     Quindi si passa il confronto all' elemento successivo
//  Se invece OA(T1+1) < OA(T1) si ha:
//     ( OA(T1+1+2l) > OA(T1+1) & Vr(T1,l) ) Implica Vr(T1+1,l)
//     Quindi si salta il confronto per l' elemento successivo
//
//
//  In questo secondo caso si puo' confrontare OA(T1) con OA(T1+2) ...
//  fino a che non si trova un' ora di arrivo superiore ad OA(T1)
//
//  Se alla fine la condizione si rivela falsa si deve aumentare l e riprovare.
//  In pratica poiche' Vr(Tx,l) & Vr(Tx+l,l) implica Vr(Tx,l+1) non e' necessario ricontrollare
//  la condizione tranne che per gli ultimi l+1 treni
//----------------------------------------------------------------------------
void COLLEGAMENTO_DIRETTO::CalcolaLimiteTreni(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::CalcolaLimiteTreni"
   if(NumTreni < 2){
      LimiteTreni = 0; // Corrisponde ad l == 1
      return;
   };
   if(MultiThread) DOE1; // I Sort vanno semaforizzati per problemi Borland
   qsort(Treni,NumTreni,sizeof(T_TRENO),SortTreni);
   if(MultiThread) DOE2;
   
   // Ora aggiorno il numero dei treni togliendo gli inibiti
   T_TRENO * TLast     = Treni + NumTreni - 1;
   while(NumTreni > 0 && !TLast->Valido){
      TLast --;
      NumTreni --;
   }
   
   #ifdef DBG8
   Trace("Situazione dopo il sort e prima del calcolo del limite treni, NumTreni validi = "+STRINGA(NumTreni));
   #endif
   int l = 1;
   T_TRENO * T1     = Treni;
   
   BOOL Fail = FALSE;
   for (int i = 0; i < NumTreni; ) {
      WORD OA1 =  OA(T1,T1); // E' la base
      #ifdef DBG8
      TRACESTRING(VRS(i) + VRS(l) + VRS(T1 - Treni) + VRO(OA(T1,T1)) + VRO(OA(T1,Trn(i+l))) );
      #endif
      if( OA1 > OA(T1,Trn(i+l)) ){ // FAIL
         Fail = TRUE;
         #ifdef DBG8
         TRACESTRING("Fail al punto 1");
         #endif
      } else if(l == 1) { // Non servono ulteriori test
         i ++;
         T1 ++; // Non va oltre il limite per definizione
      } else {  // Se l e' 1 ripeto banalmente il test precedente
         for (int k = 1;k <= l ; k ++ ) {  // So gia' che la condizione e' al massimo vera per K == l
            T_TRENO * Tk = Trn(i+k);
            if (OA1 <= OA(T1,Tk) ) { // Ok: posso passare alla verifica del nodo i + k
               i += k;
               T1 = Trn(i);
               break;
            } else if( OA(Tk,Tk) > OA(Tk,Trn(i+k+2*l)) ){ // Verifico plausibilita' nodo successivo
               #ifdef DBG8
               TRACESTRING("Fail al punto 2 per: " VRS(k) + VRO(OA(Tk,Tk)) + VRO(OA(Tk,Trn(i+k+2*l))));
               #endif
               Fail = TRUE;
               break;
            } /* endif */
         } /* endfor */
//<<< if  OA1 > OA T1,Trn i+l      // FAIL
      } /* endif */
      
      if (Fail) {
         Fail = FALSE;
         l++;
         if(l >= NumTreni -1){ // BASTA!
            break;
         };
         if(i > l){  // l gia' incrementato
            i -= l;
         } else { // Daccapo
            i = 0;
         }
         T1     = Trn(i) ;
      };
//<<< for  int i = 0; i < NumTreni;    
   } /* endfor */
   LimiteTreni = l-1;
   assert2(LimiteTreni < 25, STRINGA(LimiteTreni) + VRS(Esterno(Key.Partenza)) + VRS(Esterno(Key.Arrivo)) ); // Altrimenti ho errori !
//<<< void COLLEGAMENTO_DIRETTO::CalcolaLimiteTreni   
};

//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::FreeDati
//----------------------------------------------------------------------------
void COLLEGAMENTO_DIRETTO::FreeDati(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::FreeDati"
   
   for (int i = 0;i < NumClusters ; i++) {
      T_CLUST & Clu = Clusters[i];
      if(Clu.DatiCluster) Clu.DatiCluster ->FreeCluster(Clu.IdCluster , Clu.Concorde );
   };
};

//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::Trace
//----------------------------------------------------------------------------
// Questa funzione fa un trace di tutti i treni caricati
void  COLLEGAMENTO_DIRETTO::Trace(const STRINGA&  Msg, int Livello){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::Trace"
   if(Livello > trchse)return;
   if(Msg != NUSTR)ERRSTRING(Msg);
   ID From = NODO_CAMBIO::NodiDiCambio[Key.Partenza].Id;
   ID To   = NODO_CAMBIO::NodiDiCambio[Key.Arrivo  ].Id;
   ERRSTRING("Collegamenti diretti tra la stazione "+STRINGA(From)+" "+GRAFO::Gr()[From].Nome7()+ " e la stazione "+STRINGA(To)+" "+GRAFO::Gr()[To].Nome7());
   ERRSTRING(VRS(LimiteTreni) + VRS(Illegale) + VRS(NumTreni) + VRS(NumTreniAllocati) + VRS(MinCoincidenza)+ VRS(MultiStaz));
   for (int i = 0; i < NumTreniAllocati; i++ ) {
      T_TRENO & Tr = Treni[i];
      T_CLUST & Clu = Clusters[Tr.P_Cluster];
      CLU_BUFR & Buffer = *Clu.DatiCluster ;  // Dati del cluster
      Buffer.FindTreno(Tr.P_Treno);
      Bprintf3("  MV N¯ %i Treno N¯ %5.5s del cluster %i Parte alle %s arriva alle %s GSP %i Circola 4 gg: %x %s",
         Buffer.Treno->IdMezv,
         Buffer.IdTrenoCorrente( Clu.PNodoIn ),
         Clu.IdCluster,
         ORA(Tr.OraPartenza),
         ORA(Tr.OraArrivo),
         Buffer.Nodo(Clu.PNodoIn).GiornoSuccessivoPartenza,
         Tr.GiorniCircolazione,
         Tr.Valido  ?  "" : "Inibito"
      );
   } /* endfor */
//<<< void  COLLEGAMENTO_DIRETTO::Trace const STRINGA&  Msg, int Livello  
}
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::GetDati
//----------------------------------------------------------------------------
// Funzione per ottenere i dati (In particolare: I mezzi virtuali)
// Torna FALSE se i dati non sono disponibili
// I dati letti rimangono in Lock
//----------------------------------------------------------------------------
// PuoiEseguireIO  = 0 controlla se ho gia' i dati
// PuoiEseguireIO  = 1 accede ai dati se non li ha
// PuoiEseguireIO  = 2 Utilizza solo i dati che ho gia' in cache
//----------------------------------------------------------------------------
BOOL COLLEGAMENTO_DIRETTO::GetDati(BYTE PuoiEseguireIO){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::GetDati"
   
   static T_TRENI TmpTreni(512);
   
   #if defined(DBG2) || defined(DBG35)
   STRINGA Iden = STRINGA(Esterno(Key.Partenza)) + STRINGA(" => ") + STRINGA(Esterno(Key.Arrivo));
   #endif
   
   #ifdef DBG35
   TRACESTRING2( Iden , " Chiamato con " VRS(PuoiEseguireIO));
   #endif
   
   BOOL Problemi = FALSE;
   
   
   // Per tutti i clusters  ottengo i dati aggiornati dei cluster: inoltre informo la cache
   // dei clusters che ho intenzione di utilizzare questi dati
   for (int i = 0;i < NumClusters ; i++) {
      
      T_CLUST & Clu = Clusters[i];
      
      // Accedo o tento l' accesso ai dati del cluster
      // La funzione di accesso gestisce una SUA logica di lock sui dati del CLUSTER, che pertanto debbono
      // poi essere rilasciati dal distruttore di IPOTESI (cioe' lascio indipendenti
      // il buffering dei CLUSTERS ed il buffering dei COLLEGAMENTI_DIRETTI).
      Clu.DatiCluster = CLU_BUFR::GetCluster(Clu.IdCluster , Clu.Concorde , (PuoiEseguireIO == 1) );
      if (Clu.DatiCluster == NULL) { // Non ho ottenuto i dati
         if (PuoiEseguireIO == 1) BEEP;   // E' un errore inaspettato; Comunque continuo perche' potrei accedere ai dati di altri clusters
         Problemi = TRUE;
      } else {
         int PIn  = Clu.DatiCluster->PosNodoInCluster(Esterno(Key.Partenza));
         int POut = Clu.DatiCluster->PosNodoInCluster(Esterno(Key.Arrivo  ));
         if(PIn < 0 || POut < 0){  // Non trovate le stazioni di origine e/o destinazione
            BEEP;   // E' un errore inaspettato; Comunque continuo perche' potrei accedere ai dati di altri clusters
            // NON libero subito i dati perche' il cluster potrebbe essere utilizzato anche su altra tratta della stessa IPOTESI
            Clu.DatiCluster = NULL;
            Problemi = TRUE;
         } else {
            Clu.PNodoIn  = PIn;
            Clu.PNodoOut = POut;
         }
      }
      
//<<< for  int i = 0;i < NumClusters ; i++   
   } /* endfor */
   
   
   // A differenza dei dati dei clusters, che vanno comunque rinfrescati per sincronizzarsi con la
   // cache di I/O, i dati dei treni vengono caricati una sola volta e rimangono validi.
   if (DatiValidi) {
      #ifdef DBG2
      TRACESTRING2(Iden , " Ho gia' i dati dei treni");
      #endif
   } else if(Problemi && !PuoiEseguireIO ){
      #ifdef DBG2
      TRACESTRING2( Iden , " Non tutti i dati dei cluster disponibili: non accedo ai treni");
      #endif
   } else {
      #ifdef DBG2
      if (Problemi) {
         TRACESTRING2( Iden, " Accedo ai dati dei treni MA SALTANDO ALCUNI CLUSTER IN ERRORE");
      } else {
         TRACESTRING2( Iden , " Accedo ai dati dei treni");
      } /* endif */
      #endif
      TmpTreni.Clear();
      DatiValidi = TRUE;
      
      for (int i = 0;i < NumClusters ; i++) {
         
         T_CLUST & Clu = Clusters[i];
         CLU_BUFR & Buffer = *Clu.DatiCluster ;  // Dati del cluster
         if(&Buffer == NULL)continue; // Il cluster era in errore
         Buffer.FindTreno(0);
         DWORD IdMezv = 0xffff;
         
         for (int k=0,j = Buffer.Dat->NumeroTreni; k < j  ; k ++,Buffer.NextTreno()) {
            assert(Buffer.Treno->IdMezv > 0 && Buffer.Treno->IdMezv < 32000);
            T_TRENO Tr;
            Tr.P_Cluster     =  i;
            Tr.P_Treno       =  k;
            if(!Tr.CaricaEValidaTreno(Buffer,Clu)){
               continue;
            }
            // Ora gestisco le differenti periodicita' dei mezzi virtuali
            if(IdMezv ==  Buffer.Treno->IdMezv){    // Secondo mezzo virtuale dello stesso treno
               T_TRENO & Last = TmpTreni.Last();    // Avevo gia' caricato il treno
               assert2(Tr.Valido  == Last.Valido, VRS(IdMezv) + VRS(Clu.IdCluster) );  // I do hope so!
               Last.GiorniCircolazione |= Tr.GiorniCircolazione ;
            } else {
               IdMezv  =  Buffer.Treno->IdMezv;
               TmpTreni += Tr;
            }
         } /* endfor */
//<<< for  int i = 0;i < NumClusters ; i++   
      }
      
      NumTreni = NumTreniAllocati = TmpTreni.Dim();
      if(NumTreni) {
         Treni = new T_TRENO[NumTreni];
         memmove(Treni, &TmpTreni[0], NumTreni * sizeof(T_TRENO));
         
         // ---------------------------
         // Ora calcolo LimiteTreni
         // ---------------------------
         // Come side effects:
         // - Elimina i treni inibiti
         // - Aggiorna NumTreni
         // - Sorta i treni per ora di partenza
         CalcolaLimiteTreni();
         
         #ifdef DBG2
         TRACESTRING2(Iden," Caricati N¯ "+STRINGA(NumTreni)+" Treni validi; Limite Treni = "+STRINGA(LimiteTreni));
         #endif
         #ifdef DBG2A
         Trace(NUSTR);
         #endif
      } else if(!MultiStaz){
         Illegale = TRUE;
         #ifdef DBG2
         TRACESTRING2(Iden," Non ho potuto caricare alcun treno");
         #endif
//<<< if NumTreni   
      };
//<<< if  DatiValidi   
   } /* endif */
   return DatiValidi;
//<<< BOOL COLLEGAMENTO_DIRETTO::GetDati BYTE PuoiEseguireIO  
}
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO   Metodi di Posizionamento
//----------------------------------------------------------------------------
COMB __fastcall COLLEGAMENTO_DIRETTO::PosizionaPrimo()     { // Per prima tratta o usato internamente dagli altri Posiziona
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::PosizionaPrimo()"
   
   
   UsareMultistazione = FALSE;
   LastMultistazione = UsareMultistazione;
   if(NumTreni == 0){
      TrenoCorrente = MinimoUtilizzabile =  -1;
      #ifdef DBG5
      if(PATH_CAMBI::IsTargetAutoChk || Traccia){
         TRACESTRING("NumTreni = 0: Non possibile posizionamento");
      }
      #endif
      return COMB_KO;
   }
   T_TRENO * Tc = &Treni[TrenoCorrente];
   Tc->DataPartenza = ( Tc->OraPartenza < OraLimite ? 1 : 0);
   while (!Tc->Circola()) { // Se non circola il giorno dato
      Tc->Utilizzabile = FALSE;
      #ifdef DBG5
      if(PATH_CAMBI::IsTargetAutoChk || Traccia){
         TRACESTRING("Ignorato per circolazione"+DesTreno());
      }
      #endif
      TrenoCorrente ++;
      Tc ++;
      if(TrenoCorrente == NumTreni){ // Sulla tratta iniziale NON posso ricominciare daccapo
         TrenoCorrente = MinimoUtilizzabile =  -1;
         #ifdef DBG5
         if(PATH_CAMBI::IsTargetAutoChk || Traccia){
            TRACESTRING("Finiti i treni: Non possibile posizionamento");
         }
         #endif
         return COMB_KO;
      };
      Tc->DataPartenza = ( Tc->OraPartenza < OraLimite ? 1 : 0);
   } /* endwhile */
   Tc->Utilizzabile = TRUE;
   #ifdef DBG5
   if(PATH_CAMBI::IsTargetAutoChk || Traccia){
      TRACESTRING(DesTreno());
   }
   #endif
   
   // Impostazioni Finali
   WORD TempoTratta = TempoTrascorso(OraLimite,Tc->OraPartenza) + TempoTrascorso(Tc->OraPartenza,Tc->OraArrivo)+ MinCoincidenza -5;
   if(OraLimite + TempoTratta > 1440 ) GiorniDaInizio ++;
   OraLimite    =  Tc->OraArrivo + MinCoincidenza - 5; // i 5 minuti per i regionali
   if(OraLimite   > 1440){
      OraLimite   -= 1440;  // A cavallo della mezzanotte
   }
   OraMinimaPartenza  = OraLimite        ;
   MinimoUtilizzabile = TrenoCorrente    ;
   LastOraArrivo = Tc->OraArrivo;
   Last = Tc;
   
   return COMB_OK;
//<<< COMB __fastcall COLLEGAMENTO_DIRETTO::PosizionaPrimo         // Per prima tratta o usato internamente dagli altri Posiziona
}
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::PosizionaIntermedio
//----------------------------------------------------------------------------
COMB __fastcall COLLEGAMENTO_DIRETTO::PosizionaIntermedio(){ // Per tratte intermedie
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::PosizionaIntermedio"
   
   COMB OkTreno = PosizionaUltimo();
   
   if(OkTreno != COMB_OK){
      if (MultiStaz && !LastMultistazione) { // Posso anche usare un multistazione
         // Vedo quanto ci metterei con il multistazione: Si noti che non vi sono MAI i 5 minuti per il regionale
         // Inoltre: Se il tempo di collegamento della multistazione e' <= 5 minuti ne
         // deduco che o siamo all' interno dello stesso edificio o l' altra
         // stazione sta nel piazzale antistante: pertanto NON aggiungo il secondo tempo
         // margine
         WORD TempoCo = MM_CR.TempoCoincidenza[NODO_CAMBIO::NodiDiCambio[Key.Partenza].ClasseCoincidenza];
         if(MS_Minuti <= 5) {
            TempoCo = 0; // E' la stessa stazione
         }
         OraLimite =  LastOraArrivo + MinCoincidenza + MS_Minuti + TempoCo ;
         if(OraLimite   > 1440){
            OraLimite   -= 1440;  // A cavallo della mezzanotte
         }
         UsareMultistazione = TRUE;
         Last = NULL;
         OraMinimaPartenza  = OraLimite ;
         LastOraArrivo   += MS_Minuti + TempoCo ;
         #ifdef DBG5
         if(PATH_CAMBI::IsTargetAutoChk || Traccia){
            TRACESTRING("Utilizzo MULTISTAZIONE, " VRO(OraMinimaPartenza) + VRO(LastOraArrivo));
         }
         #endif
//<<< if  MultiStaz && !LastMultistazione    // Posso anche usare un multistazione
      } else {
         LastMultistazione = FALSE;
         return OkTreno;
      } /* endif */
      
//<<< if OkTreno != COMB_OK  
   } else {
      
      // Impostazioni Finali
      T_TRENO * Tc = &Treni[TrenoCorrente];
      WORD TempoTratta = TempoTrascorso(OraLimite,Tc->OraPartenza) + TempoTrascorso(Tc->OraPartenza,Tc->OraArrivo)+ MinCoincidenza -5;
      if(OraLimite + TempoTratta > 1440 ) GiorniDaInizio ++;
      OraLimite    =  Tc->OraArrivo + MinCoincidenza - 5; // i 5 minuti per i regionali
      
      if (MultiStaz && !LastMultistazione) { // Posso anche usare un multistazione
         // Vedo quanto ci metterei con il multistazione: Si noti che non vi sono MAI i 5 minuti per il regionale
         WORD OraLimite2 =  LastOraArrivo + MinCoincidenza + MS_Minuti + MM_CR.TempoCoincidenza[NODO_CAMBIO::NodiDiCambio[Key.Partenza].ClasseCoincidenza];
         if (OraLimite2 < OraLimite ) {
            OraLimite = OraLimite2;
            LastOraArrivo   += MS_Minuti + MM_CR.TempoCoincidenza[NODO_CAMBIO::NodiDiCambio[Key.Partenza].ClasseCoincidenza];
            UsareMultistazione = TRUE;
            Last = NULL;
         } /* endif */
      } /* endif */
      if(!UsareMultistazione)LastOraArrivo   = Tc->OraArrivo  ;
      
      if(OraLimite   > 1440){
         OraLimite   -= 1440;  // A cavallo della mezzanotte
      }
      OraMinimaPartenza  = OraLimite ;
//<<< if OkTreno != COMB_OK  
   }
   LastMultistazione = UsareMultistazione;
   
   return COMB_OK;
//<<< COMB __fastcall COLLEGAMENTO_DIRETTO::PosizionaIntermedio    // Per tratte intermedie
}

//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::PosizionaUltimo
//----------------------------------------------------------------------------
COMB __fastcall COLLEGAMENTO_DIRETTO::PosizionaUltimo()    { // Per ultima tratta
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::PosizionaUltimo"
   
   #ifdef DBG5
   if(PATH_CAMBI::IsTargetAutoChk || Traccia){
      TRACESTRING( VRO(OraMinimaPartenza) + VRO(OraLimite) + VRO(LastOraArrivo) + VRS(GiorniDaInizio) );
   }
   #endif
   UsareMultistazione = FALSE;
   if(NumTreni == 0){
      TrenoCorrente = MinimoUtilizzabile =  -1;
      return COMB_KO;
   }
   
   // Uso una ricerca dicotomica per trovare il primo treno che parte DOPO l' ora data
   // Se vi fossero due treni che partono alla stessa ora ho risultati non troppo
   // definiti ( a volte salto il primo, a volte no) ma non importa
   if(Treni[NumTreni - 1].OraPartenza < OraMinimaPartenza){ // E' dopo l' ultimo treno
      TrenoCorrente = 0; // Allora debbo partire con il primo treno del mattino
      // Gestisco casi particolari di wrap che superano le 23 ore (altrimenti con il tempo di cambio
      // potrei non rispettare il tempo di cambio di 5 .. 15 minuti)
      if(Treni[0].OraPartenza + 60 > OraMinimaPartenza && Treni[0].OraPartenza < OraMinimaPartenza){ // Caso anaomalo
         #ifdef DBG5
         if(PATH_CAMBI::IsTargetAutoChk || Traccia){
            TRACESTRING("P1: Finiti i treni: Non possibile posizionamento");
         }
         #endif
         TrenoCorrente = MinimoUtilizzabile =  -1;
         return COMB_KO;
      }
   } else if(Treni[0].OraPartenza > OraMinimaPartenza){     // Gia' il primo verifica la condizione
      TrenoCorrente = 0; // Il treno e' ok
   } else {
      short int Min = 0;            // In Min ho sempre un treno che parte PRIMA dell' ora data
      short int Max = NumTreni-1;   // In Max ho sempre un treno che parte DOPO l' ora data o all' ora data
      while (Min < Max-1){ // Finche' l' intervallo non e' definito
         short int i = (Min + Max ) >> 1 ;    //  >>1   =   /2  ma piu' veloce se non ho optimizer
         WORD Orap = Treni[i].OraPartenza;
         if( Orap <  OraMinimaPartenza )Min = i;
         if( Orap >= OraMinimaPartenza )Max = i;
      }
      TrenoCorrente = Max;
//<<< if Treni NumTreni - 1 .OraPartenza < OraMinimaPartenza   // E' dopo l' ultimo treno
   }
   
   // Ora mi posiziono al primo VALIDO
   //   - Considero la circolazione dei treni
   //   - Considero i 5 minuti di margine per i regionali
   //   - Considero il tempo massimo di coincidenza dal cambio precedente : Tramite il metodo ControllaAttesaValida()
   int Init = TrenoCorrente;
   T_TRENO * Tc = & Treni[TrenoCorrente];
   Tc->DataPartenza = GiorniDaInizio + ( Tc->OraPartenza < OraLimite ? 1 : 0);
   // Last == NULL comporta che l' ultima tratta fosse MULTISTAZIONE e quindi non inibisco i primi 5 minuti
   Tc->Utilizzabile = Tc->Circola() && ( TempoTrascorso(OraLimite,Tc->OraPartenza) >= 5 || Last == NULL || Tc->RidottoTempoCoincP);
   #ifdef DBG5
   if(PATH_CAMBI::IsTargetAutoChk || Traccia){
      TRACESTRING( STRINGA(DesTreno())+ STRINGA(VRS(Tc->RidottoTempoCoincP)) + VRS(Tc->Circola()));
   }
   #endif
   while (!Tc->Utilizzabile) {
      #ifdef DBG5
      if(PATH_CAMBI::IsTargetAutoChk || Traccia){
         TRACESTRING("Ignorato per circolazione o per i 5 minuti di margine regionali:"+DesTreno());
      }
      #endif
      TrenoCorrente ++;
      Tc ++;
      if(TrenoCorrente == NumTreni){TrenoCorrente = 0; Tc = Treni;};
      if(Init == TrenoCorrente){   // Finiti i treni
         #ifdef DBG5
         if(PATH_CAMBI::IsTargetAutoChk || Traccia){
            TRACESTRING("Finiti i treni: Non possibile posizionamento");
         }
         #endif
         TrenoCorrente = MinimoUtilizzabile =  -1;
         return COMB_KO;
      }
      Tc->DataPartenza = GiorniDaInizio + ( Tc->OraPartenza < OraLimite ? 1 : 0);
      // Last == NULL comporta che l' ultima tratta fosse MULTISTAZIONE e quindi non inibisco i primi 5 minuti
      Tc->Utilizzabile = Tc->Circola() && ( TempoTrascorso(OraLimite,Tc->OraPartenza) >= 5 || Last == NULL || Tc->RidottoTempoCoincP);
      #ifdef DBG5
      if(PATH_CAMBI::IsTargetAutoChk || Traccia){
         TRACESTRING("Successivo treno che parte DOPO l' ora minima:"+DesTreno()+ " "+ VRO(Tc->OraPartenza) + VRS(Tc->DataPartenza));
      }
      #endif
//<<< while  !Tc->Utilizzabile   
   } /* endwhile */
   if(!ControllaAttesaValida() ){
      #ifdef DBG5
      if(PATH_CAMBI::IsTargetAutoChk || Traccia){
         TRACESTRING("Treno "+STRINGA(IdTreno())+" scartato per attesa eccessiva: Ritorno con COMB_RETRY ");
         WORD AttesaV = AttesaValida(LastOraArrivo, OraLimite) + AttesaValida(OraLimite, Treni[TrenoCorrente].OraPartenza);
         TRACESTRING( VRO(AttesaV) + VRO(LastOraArrivo) + VRO(OraLimite) + VRO(Tc->OraPartenza) + VRO(TempoTrascorso(LastOraArrivo,Tc->OraPartenza)));
      }
      #endif
      TrenoCorrente = MinimoUtilizzabile =  -1;
      return COMB_RETRY;
   }
   MinimoUtilizzabile = TrenoCorrente    ;
   
   // Ed ora trovo quello che arriva PRIMA
   if (LimiteTreni) {  // Altrimenti ho gia' quello che arriva prima
      Tc =  & Treni[TrenoCorrente];
      WORD Elapsed = TempoTrascorso(OraMinimaPartenza,Tc->OraPartenza) +TempoTrascorso(Tc->OraPartenza,Tc->OraArrivo);
      
      // Calcolo il limite di scansione (si noti che supero il numero dei treni)
      WORD Limite = min(LimiteTreni + TrenoCorrente+1, Init + ((Init <= TrenoCorrente) ? NumTreni : 0) );
      
      for (WORD Corr = TrenoCorrente+1 ; Corr < Limite; Corr++ ) {  // Corr puo' superare il numero dei treni
         Tc = Trn(Corr);
         Tc->DataPartenza = GiorniDaInizio + ( Tc->OraPartenza < OraLimite ? 1 : 0);
         // TRACESTRING("P1," VRS(Corr) + VRS(Tc->DataPartenza) + VRS(GiorniDaInizio) );
         Tc->Utilizzabile = Tc->Circola();
         if(!Tc->Utilizzabile)continue;
         WORD Elapsed1 = TempoTrascorso(OraMinimaPartenza,Tc->OraPartenza) +TempoTrascorso(Tc->OraPartenza,Tc->OraArrivo);
         // TRACEINT("P2, Elapsed1 ",Elapsed1);
         if (Elapsed1 < Elapsed) {
            Elapsed = Elapsed1;
            TrenoCorrente = Corr;
            if(TrenoCorrente >= NumTreni) TrenoCorrente -= NumTreni;
            // TRACESTRING("Found :"+DesTreno());
         } /* endif */
      } /* endfor */
      #ifdef DBG5
      if(PATH_CAMBI::IsTargetAutoChk || Traccia){
         TRACESTRING("Primo Treno che arriva Selezionato:"+DesTreno());
      }
      #endif
//<<< if  LimiteTreni     // Altrimenti ho gia' quello che arriva prima
   } /* endif */
   
   // Impostazioni Finali: Si noti che NON DEVE cambiare OraMinimaPartenza  o LastOraArrivo perche' vanno usate se sono chiamato da PosizionaIntermedio
   Last = &Treni[TrenoCorrente];
   
   return COMB_OK;
//<<< COMB __fastcall COLLEGAMENTO_DIRETTO::PosizionaUltimo        // Per ultima tratta
}
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::IdentificaAlternative
//----------------------------------------------------------------------------
// Identifica le possibili alternative
// Last e' vero se e' l' ultima tratta
//----------------------------------------------------------------------------
int  __fastcall COLLEGAMENTO_DIRETTO::IdentificaAlternative(BOOL Last) {
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::IdentificaAlternative"
   
   if(UsareMultistazione){
      OraMassimaArrivo += 1440 - MS_Minuti - MinCoincidenza;
      if(OraMassimaArrivo > 1440) OraMassimaArrivo -= 1440;
      #ifdef DBG5
      if(PATH_CAMBI::IsTargetAutoChk || Traccia){
         TRACESTRING("Multistaz " VRO(OraMassimaArrivo) );
      }
      #endif
      return 1; // Non ho alternative
   };
   if(NumTreni < 2){
      OraMassimaArrivo =  Treni[TrenoCorrente].OraPartenza;
      #ifdef DBG5
      TRACESTRING("Un solo Coll: " VRO(OraMassimaArrivo));
      #endif
      return 1; // Non ho alternative
   }
   WORD OraStandard  = Treni[TrenoCorrente].OraArrivo;
   WORD LimiteArrivo ; // Massima tolleranza
   if (Last) {
      LimiteArrivo = 30;  // Accetto soluzioni che mi fanno arrivare sino a 30 minuti dopo
   } else {
      // Considero nel limite un tempo convenzionale di coincidenza di 5 minuti
      LimiteArrivo = TempoTrascorso(OraStandard, OraMassimaArrivo) - 5 ;
      assert(LimiteArrivo < 1440);
   } /* endif */
   // Per la prossima Volta
   OraMassimaArrivo =  Treni[TrenoCorrente].OraPartenza;
   WORD Lim1 = 0;
   
   MassimoUtilizzabile = MinimoUtilizzabile;
   int Idx = MinimoUtilizzabile +1 ;
   T_TRENO * Tc = & Treni[Idx];
   int Stop = -1;
   while (Idx != MinimoUtilizzabile) {
      if( TempoTrascorso(OraStandard, Tc->OraArrivo) <= LimiteArrivo) {
         // Il treno e' (Probabilmente) utilizzabile
         MassimoUtilizzabile = Idx;
         // Vedo se posso ritoccare l' ora di partenza
         WORD Lim2 = TempoTrascorso( Treni[TrenoCorrente].OraPartenza, Tc->OraPartenza );
         if(Lim2 > Lim1 && Lim2 < 90){ // Non considero treni che partono oltre un' ora e mezza dopo la partenza del treno corrente
            OraMassimaArrivo =  Tc->OraPartenza;
            Lim1 = Lim2;
         }
      } else if(Stop < 0) {
         Stop = LimiteTreni; // Dopo limite treni inutile considerare l' ora di arrivo
      } /* endif */
      Idx ++;
      Tc ++;
      if(Idx == NumTreni){ Idx = 0; Tc = Treni;};
      if(Stop -- == 0) break;
   } /* endwhile */
   int Res = MassimoUtilizzabile - MinimoUtilizzabile;
   if(Res < 0) Res += NumTreni;
   Res ++;
   #ifdef DBG5
   if(PATH_CAMBI::IsTargetAutoChk || Traccia){
      TRACESTRING(VRS(Res) + VRS(MinimoUtilizzabile ) + VRS(MassimoUtilizzabile) + VRO(OraMassimaArrivo) );
   }
   #endif
   return Res;
//<<< int  __fastcall COLLEGAMENTO_DIRETTO::IdentificaAlternative BOOL Last   
}
//----------------------------------------------------------------------------
// AttesaValida
//----------------------------------------------------------------------------
WORD __fastcall AttesaValida( WORD LastOraArrivo, WORD OraPartenza){ // Calcola l' attesa tra due ore (escludendo le ore notturne)
   #undef TRCRTN
   #define TRCRTN "AttesaValida"
   WORD AttesaValida;
   // Debbo escludere dall' attesa le ore dall' 1 alle 6 (INIZIO_NOTTE ... FINE_NOTTE )
   // Si noti che trascuro i casi in cui LastOraArrivo > OraPartenza &&  LastOraArrivo < FINE_NOTTE, visto che
   // la massima attesa sarebbe comunque eccessiva
   if(OraPartenza >= LastOraArrivo){ // Non Ho attraversamento della mezzanotte
      if (LastOraArrivo >= FINE_NOTTE || OraPartenza <= INIZIO_NOTTE) {
         AttesaValida  = OraPartenza - LastOraArrivo;
      } else if (LastOraArrivo >= INIZIO_NOTTE){
         if (OraPartenza > FINE_NOTTE) {
            AttesaValida  =  OraPartenza - FINE_NOTTE;
         } else {
            AttesaValida  =  0;
         } /* endif */
      } else {
         if (OraPartenza > FINE_NOTTE) {
            AttesaValida  =  OraPartenza - LastOraArrivo - (FINE_NOTTE - INIZIO_NOTTE);
         } else {
            AttesaValida  =  INIZIO_NOTTE - LastOraArrivo;
         } /* endif */
      } /* endif */
   } else {
      if (OraPartenza <= INIZIO_NOTTE) {
         AttesaValida  = OraPartenza + 1440 - LastOraArrivo;
      } else  if (OraPartenza > FINE_NOTTE) {
         AttesaValida  =  OraPartenza + 1440 - LastOraArrivo - (FINE_NOTTE - INIZIO_NOTTE);
      } else {
         AttesaValida  =  INIZIO_NOTTE + 1440 - LastOraArrivo ;
      } /* endif */
//<<< if OraPartenza > LastOraArrivo   // Non Ho attraversamento della mezzanotte
   }
   return AttesaValida;
//<<< WORD __fastcall AttesaValida  WORD LastOraArrivo, WORD OraPartenza   // Calcola l' attesa tra due ore  escludendo le ore notturne 
}
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::GetTrenoInCluster
//----------------------------------------------------------------------------
CLUSTRENO * __fastcall COLLEGAMENTO_DIRETTO::GetTrenoInCluster(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::GetTrenoInCluster"
   T_TRENO & Treno = Treni[TrenoCorrente];
   CLU_BUFR & Clu = * Clusters[Treno.P_Cluster].DatiCluster ;  // Dati del cluster
   Clu.FindTreno(Treno.P_Treno);
   return Clu.Treno;
};
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::GetDatiStazioneInCluster
//----------------------------------------------------------------------------
INFOSTAZ  * __fastcall COLLEGAMENTO_DIRETTO::GetDatiStazioneInCluster(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::GetDatiStazioneInCluster"
   T_TRENO & Treno = Treni[TrenoCorrente];
   T_CLUST & Clust = Clusters[Treno.P_Cluster];
   CLU_BUFR & Clu = * Clust.DatiCluster ;  // Dati del cluster
   Clu.FindTreno(Treno.P_Treno);
   return & Clu.Treno->Nodi[Clust.PNodoIn];
};
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::IdMezv
//----------------------------------------------------------------------------
int __fastcall COLLEGAMENTO_DIRETTO::IdMezv(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::IdMezv"
   if(UsareMultistazione) return 0;
   return GetTrenoInCluster()->IdMezv;
};
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::IdCluster
//----------------------------------------------------------------------------
int __fastcall COLLEGAMENTO_DIRETTO::IdCluster(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::IdCluster"
   if(UsareMultistazione) return 0;
   T_TRENO & Treno = Treni[TrenoCorrente];
   T_CLUST & Clust = Clusters[Treno.P_Cluster];
   return Clust.IdCluster;
};
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::IdTreno
//----------------------------------------------------------------------------
char  *  __fastcall COLLEGAMENTO_DIRETTO::IdTreno(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::IdTreno"
   static char Buf[10 * (sizeof(ID_VIAG)+1)];
   static int Ptr;
   char * Out = Buf + Ptr;
   Ptr += sizeof(ID_VIAG) + 1;
   if(Ptr >= 10 * (sizeof(ID_VIAG)+1))Ptr = 0;
   memset(Out,0,sizeof(ID_VIAG)+1);
   if(!UsareMultistazione) {
      T_TRENO & Treno = Treni[TrenoCorrente];
      T_CLUST & Clust = Clusters[Treno.P_Cluster];
      CLU_BUFR & Clu = * Clust.DatiCluster ;  // Dati del cluster
      Clu.FindTreno(Treno.P_Treno);
      memmove(Out,Clu.IdTrenoCorrente(Clust.PNodoIn),sizeof(ID_VIAG));
   }
   return Out;
};
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::DesTreno
//----------------------------------------------------------------------------
STRINGA COLLEGAMENTO_DIRETTO::DesTreno(WORD Pos){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::DesTreno"
   WORD Last = TrenoCorrente ;
   TrenoCorrente = Pos;
   STRINGA Tmp = DesTreno();
   TrenoCorrente = Last;
   return Tmp;
};
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::DesTreno
//----------------------------------------------------------------------------
STRINGA COLLEGAMENTO_DIRETTO::DesTreno(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::DesTreno"
   return
   STRINGA(" Tr ")+ STRINGA(IdTreno()) +
   // Queste linee commentate per far vedere le ore in termini di fasce di 45 minuti
   STRINGA(" P. ")+ STRINGA(ORA(Treni[TrenoCorrente].OraPartenza)) + 
// + " " + STRINGA((void*)(1<<(Treni[TrenoCorrente].OraPartenza/45)))+ 
   STRINGA(" A. ")+ STRINGA(ORA(Treni[TrenoCorrente].OraArrivo)) + 
// + " " + STRINGA((void*)(1<<(Treni[TrenoCorrente].OraArrivo/45)))+
   STRINGA(" MV ")+ STRINGA(IdMezv()) +
   STRINGA(" Clu ")+ STRINGA(IdCluster())+
   STRINGA(" DataP ")+ STRINGA(Treni[TrenoCorrente].DataPartenza)+
   STRINGA(" Ut. ")+ STRINGA(Treni[TrenoCorrente].Utilizzabile)+
   " ";
};

//----------------------------------------------------------------------------
// @IPOTESI
//----------------------------------------------------------------------------
IPOTESI::IPOTESI(PATH_CAMBI & PtC) : Path(PtC) {
   #undef TRCRTN
   #define TRCRTN "@IPOTESI"
   
   #ifdef DBG3
   TRACESTRING2("Path : ",STRINGA(Path));
   #endif
   
   NumTratte = 0;
   Illegale = 0;
   for (int i = 0;i < Path.NumNodi-1 ;i++ ) {
      // Vedo se il collegamento diretto esiste gia', Se non esiste lo alloco
      COLLEGAMENTO_DIRETTO::KEY Key;
      Key.Partenza = Path.Nodi[i].Id;
      Key.Arrivo   = Path.Nodi[i+1].Id;
      
      // Ottengo il collegamento diretto
      // Viene COMUNQUE messo in lock finche' esiste l' IPOTESI, anche se la stessa e' illegale
      PColls[i] =  Cache.GetSlot((COLLEGAMENTO_DIRETTO*)&Key, LR_LOCK);
      
      // Cerco di ottenere i dati se gia' disponibili
      PColls[i]->GetDati(FALSE);
      
      // Verifico se illegale
      if( PColls[i]->Illegale ){
         Illegale = TRUE;
         #if defined(DBG3) || defined(DBG6)
         TRACESTRING("Ipotesi illegale per Collegamento N¯ "+STRINGA(i) +" "+STRINGA(Esterno(Key.Partenza)) + STRINGA(" => ") + STRINGA(Esterno(Key.Arrivo)));
         #endif
         break;
      };
      
      NumTratte ++;
//<<< for  int i = 0;i < Path.NumNodi-1 ;i++    
   } /* endfor */
//<<< IPOTESI::IPOTESI PATH_CAMBI & PtC  : Path PtC   
};
//----------------------------------------------------------------------------
// ~@IPOTESI
//----------------------------------------------------------------------------
IPOTESI::~IPOTESI(){
   #undef TRCRTN
   #define TRCRTN "~IPOTESI"
   
   #ifdef DBG3
   TRACESTRING2("Path : ",STRINGA(Path));
   #endif
   
   // Rilascio i Locks
   for (int i = 0; i < NumTratte ; i++) {
      PColls[i]->FreeDati();
      Cache.UnLock(*PColls[i]);
   }
}
//----------------------------------------------------------------------------
// IPOTESI::PreFetch
//----------------------------------------------------------------------------
void IPOTESI::PreFetch(){
   #undef TRCRTN
   #define TRCRTN "IPOTESI::PreFetch"
   
   if(Illegale)return; // Non effettua il prefetch
   
   BYTE Modo = 1; // Lettura dati
   if(UsaDatiInCache) Modo = 2;
   for (int i = 0;i < NumTratte  ;i++ ) {
      // Cerco di ottenere i dati
      PColls[i]->GetDati(Modo);
      
      // Verifico se illegale
      if( PColls[i]->Illegale ){
         Illegale = TRUE;
         break;
      };
      
   } /* endfor */
};

//----------------------------------------------------------------------------
// IPOTESI::TrovaSoluzione()
//----------------------------------------------------------------------------
// Il metodo trova una soluzione di viaggio
// La soluzione e' successiva all' ultima trovata.
// (variabile PTreno presente in ogni cluster).
// I cluster multistazione debbono essere gestiti diversamente dagli altri:
//    - Il primo e l' ultimo debbono essere ignorati
//    - Quelli intermedi debbono generare un mezzo virtuale fittizio
// Ritorna FALSE se non ho piu' soluzioni
//----------------------------------------------------------------------------
/*

   Si ipotizzi di avere una IPOTESI con tre tratte:

     Ora               Treni       Ora         Ora               Treni      Ora          Ora               Treni      Ora
     Partenza                      Arrivo      Partenza                     Arrivo       Partenza                     Arrivo
     tratta 1                      tratta 1    tratta 2                     tratta 2     tratta 3
                                                                                                                      
   ≥          ≥                 ≥           ≥           ≥                ≥            ≥           ≥                 ≥
   ≥ 8:10     ≥ Treno 1 ÕÕÕÕÕª  ≥           ≥           ≥                ≥            ≥           ≥                 ≥
   ≥ 8:20     ≥ Treno 3ƒƒƒƒø ∫  ≥           ≥           ≥                ≥            ≥           ≥                 ≥
   ≥          ≥            ≥ »Õ>≥ 10:30 ... ≥           ≥                ≥            ≥           ≥                 ≥
   ≥ 8:40     ≥ Treno 5 *******>≥ 10:40 ... ≥           ≥                ≥            ≥           ≥                 ≥
   ≥          ≥            ¿ƒƒƒ>≥ 10:45 ... ≥           ≥                ≥            ≥           ≥                 ≥
   ≥ 8:50     ≥ Treno 6 ƒƒø     ≥         . ≥           ≥                ≥            ≥           ≥                 ≥
   ≥          ≥           ¿ƒƒƒƒ>≥ 11:26 . . ≥           ≥                ≥            ≥           ≥                 ≥
   ≥          ≥                 ≥       . ..≥ 11:35     ≥ Tr. Urb. ÕÕª   ≥            ≥           ≥                 ≥
   ≥          ≥                 ≥       ....≥ 11:40     ≥ Treno 4 ƒƒø∫   ≥            ≥           ≥                 ≥
   ≥          ≥                 ≥           ≥           ≥           ≥»ÕÕ>≥ 12:10 ...  ≥           ≥                 ≥
   ≥          ≥                 ≥           ≥           ≥           ¿ƒƒƒ>≥ 12:15 ...  ≥           ≥                 ≥
   ≥          ≥                 ≥           ≥           ≥                ≥         .  ≥           ≥                 ≥
   ≥          ≥                 ≥           ≥           ≥                ≥         ...≥ 12:45     ≥ Treno 2 ÕÕÕÕÕª  ≥
   ≥          ≥                 ≥           ≥           ≥                ≥            ≥           ≥              »Õ>≥ 14:50
   ≥          ≥                 ≥           ≥           ≥                ≥            ≥           ≥                 ≥


   L' algoritmo opera nel modo seguente:
   1) Parto da uno specifico treno sulla tratta 1 (nel caso in esame
      il treno 1 ) e trovo la serie di treni che (arrivando PRIMA ) su
      ogni tratta mi porta ad arrivare prima alla destinazione. Nel caso
      in esame ho indicato con una doppia linea tale soluzione.

   2) Si trovano per ogni tratta i treni alternativi che permettono di
      arrivare a destinazione con lo stesso treno.  Per la prima tratta
      non posso utilizzare treni precedenti al primo indicato.

   3) Per ogni tratta in cui si utilizza un trasporto urbano si elimina
      tale T.U.  se e' disponibile un treno FS che svolga la stessa
      funzione (NB:  Meglio fare questo step prima dei successivi:  si
      perdono alcune soluzioni piu' rapide ma a rischio (causa trasporti
      urbani)).

   4) Si verifica che non vi siano DUE tratte urbane successive
      (altrimenti passo al successivo treno in partenza)

   5) Si sceglie, tra tutte le combinazioni di treni possibili con le
      precedenti condizioni, quella che minimizza una data funzione
      obiettivo.

   6) Il prossimo treno da considerare e' quello successivo a:
      - L' ultimo treno possibile tra quelli della prima tratta se
        Tale ultimo treno parte ENTRO UN' ORA dal treno prescelto
      - Il treno successivo al treno prescelto altrimenti.


   Si noti che tale logica NON gestisce correttamente i trasporti urbani
      sulla prima ed ultima tratta: tali casi vanno trattati
      SEPARATAMENTE con delle richieste differenziate allo stesso
      algoritmo (con una stazione in meno ) e gestendo a parte l'
      aggiunta di tale tratte.

  *************************************
  Prima Versione
  *************************************
  Mi limito allo STEP 1 e lascio ai moduli
  successivi il filtraggio delle soluzioni in eccesso

*/
//----------------------------------------------------------------------------
COMB IPOTESI::TrovaSoluzione(BOOL Iniziale, BYTE PrimaTratta, BYTE UltimaTratta){
   #undef TRCRTN
   #define TRCRTN "IPOTESI::TrovaSoluzione()"
   
   #ifdef DBG5
   if(PATH_CAMBI::IsTargetAutoChk || Traccia){
      TRACESTRING("==========================================================");
   }
   #endif
   #ifdef DBG4
   TRACESTRING(VRS(Iniziale) + VRS(PrimaTratta) + VRS(UltimaTratta));
   #endif
   
   int i;
   COMB Ok;
   static int AntiLoop;
   
   if(Iniziale){
      AntiLoop = 0;
      PColls[PrimaTratta]->TrenoCorrente       = 0;
      #ifdef DBG4
      TRACEVLONG(PColls[PrimaTratta]->NumTreni);
      #endif
   } else if(NextTreno >= PColls[PrimaTratta]->NumTreni){
      #ifdef DBG5
      if(PATH_CAMBI::IsTargetAutoChk || Traccia){
         TRACESTRING("Fine Combinazioni");
      }
      #endif
      return COMB_KO; // Fine combinazioni
   } else {
      PColls[PrimaTratta]->TrenoCorrente = NextTreno;
      #ifdef DBG4
      TRACEVLONG(PColls[PrimaTratta]->TrenoCorrente);
      #endif
   }
   
   #ifdef DBG4
   TRACESTRING("Situazione di partenza: PColls["+STRINGA(PrimaTratta)+"]->TrenoCorrente ="+STRINGA( PColls[PrimaTratta]->TrenoCorrente ));
   #endif
   
   // Controllo antiloop;
   if(AntiLoop ++ >= 500){
      assert(AntiLoop < 500);
      return COMB_KO;
   }
   
   // ...........................
   // 1) Trovo soluzione "FAST"
   // ...........................
   
   // Init
   COLLEGAMENTO_DIRETTO::OraLimite = Orario.OraOrd; // L' ora della richiesta
   COLLEGAMENTO_DIRETTO::GiorniDaInizio = 0;
   
   // Mi posiziono sul primo treno compatibile >= treno corrente per la prima tratta
   Ok = PColls[PrimaTratta]->PosizionaPrimo();
   #ifdef DBG4
   if(Ok != COMB_OK){
      TRACESTRING("Nessun treno trovato su tratta Iniziale, N¯ "+STRINGA(PrimaTratta)+ VRS(Ok));
   } else {
      TRACESTRING("1) Prima Tratta posizionata su PColls["+STRINGA(PrimaTratta)+"]->TrenoCorrente ="+STRINGA( PColls[PrimaTratta]->TrenoCorrente ));
   }
   #endif
   if(Ok != COMB_OK) goto fine;
   Top(MaxTrattaOk , PrimaTratta);
   
   // Se ho una sola tratta ho finito, altrimenti mi posiziono sulle tratte intermedie e sull' ultima
   for (i = PrimaTratta + 1; Ok == COMB_OK && i < UltimaTratta ; i++) {
      
      // Mi posiziono sul primo treno compatibile >= Del treno corrente per le altre tratte
      // OPPURE su un collegamento multistazione
      Ok = PColls[i]->PosizionaIntermedio();
      
      #ifdef DBG4
      if(Ok != COMB_OK){
         TRACESTRING("Problemi su tratta intermedia N¯ "+STRINGA(i)+ VRS(Ok));
      } else if (PColls[i]->UsareMultistazione) {
         TRACESTRING("1) Tratta intermedia N¯ "+STRINGA(i)+" Usata MULTISTAZIONE");
      } else {
         TRACESTRING("1) Tratta intermedia : PColls["+STRINGA(i)+"]->TrenoCorrente ="+STRINGA( PColls[i]->TrenoCorrente ));
      } /* endif */
      #endif
      
      if(Ok == COMB_OK)Top(MaxTrattaOk , i);
      
   } /* endfor */
   if(Ok != COMB_OK) goto fine;
   
   if(PrimaTratta < UltimaTratta){
      Ok = PColls[UltimaTratta]->PosizionaUltimo();
      #ifdef DBG4
      if(Ok != COMB_OK){
         TRACESTRING("Nessun treno trovato su tratta Finale N¯ "+STRINGA(i)+ VRS(Ok));
      } else {
         TRACESTRING("1) Ultima Tratta posizionata su PColls["+STRINGA(UltimaTratta)+"]->TrenoCorrente ="+STRINGA( PColls[UltimaTratta]->TrenoCorrente ));
      }
      #endif
      if(Ok != COMB_OK) goto fine;
      Top(MaxTrattaOk , UltimaTratta);
   }
   
   PROFILER::Conta(31); // Numero di soluzioni generate
   
   // ...........................
   // 2) Identifico treni alternativi
   // ...........................
   #ifdef IN_WORK   // Compila le parti che sono in lavorazione
   {
      int NumAlternative = 1;
      for (i = UltimaTratta ; i > PrimaTratta ; i--) {
         NumAlternative *= PColls[i]->IdentificaAlternative(i == UltimaTratta );
      } /* endfor */
      PROFILER::Somma(32,NumAlternative); // Numero di soluzioni generate
      #ifdef DBG4
      TraceSoluzioneInPreparazione(VRS(NumAlternative));
      #endif
   }
   #endif
   
   // ...........................
   // 3) Elimina se possibile TU
   // 4) Verifica 2 TU successivi
   // ...........................
   
   // ...........................
   // 5) Scelta migliore alternativa
   // ...........................
   
   // ...........................
   // 6) Lascio pulito per prossima volta
   // ...........................
   fine:
   NextTreno = PColls[PrimaTratta]->TrenoCorrente + 1;
   #ifdef DBG5
   if(PATH_CAMBI::IsTargetAutoChk || Traccia){
      TRACEVLONG(NextTreno);
   }
   #endif
   
   
   return Ok;
//<<< COMB IPOTESI::TrovaSoluzione BOOL Iniziale, BYTE PrimaTratta, BYTE UltimaTratta  
};

//----------------------------------------------------------------------------
// IPOTESI::OraPartenza
//----------------------------------------------------------------------------
WORD __fastcall IPOTESI::OraPartenza(BYTE Tratta){
   #undef TRCRTN
   #define TRCRTN "IPOTESI::OraPartenza"
   COLLEGAMENTO_DIRETTO & Cd = * PColls[Tratta];
   int Ora;
   if (Cd.UsareMultistazione) {
      if (Tratta == 0) {
         COLLEGAMENTO_DIRETTO & Cd2 = * PColls[Tratta+1];
         Ora = Cd2.Treni[Cd2.TrenoCorrente].OraPartenza ;
         // Se il tempo di collegamento della multistazione e' <= 5 minuti ne
         // deduco che o siamo all' interno dello stesso edificio o l' altra
         // stazione sta nel piazzale antistante: pertanto NON aggiungo il secondo tempo
         // margine
         if (Cd.MS_Minuti > 5) {
            Ora -= Cd.MinCoincidenza ;
         } /* endif */
         Ora -= Cd.MS_Minuti;
         if(Ora < 0)Ora += 1440;
      } else {
         COLLEGAMENTO_DIRETTO & Cd2 = * PColls[Tratta-1];
         Ora = Cd2.Treni[Cd2.TrenoCorrente].OraArrivo ;
         // Se il tempo di collegamento della multistazione e' <= 5 minuti ne
         // deduco che o siamo all' interno dello stesso edificio o l' altra
         // stazione sta nel piazzale antistante: pertanto NON aggiungo il secondo tempo
         // margine
         if (Cd.MS_Minuti > 5) {
            Ora += Cd2.MinCoincidenza ;
         } /* endif */
         if(Ora > 1440)Ora -= 1440;
      } /* endif */
   } else {
      Ora = Cd.Treni[Cd.TrenoCorrente].OraPartenza;
   } /* endif */
   return Ora;
//<<< WORD __fastcall IPOTESI::OraPartenza BYTE Tratta  
}
//----------------------------------------------------------------------------
// IPOTESI::OraArrivo
//----------------------------------------------------------------------------
WORD __fastcall IPOTESI::OraArrivo(BYTE Tratta){
   #undef TRCRTN
   #define TRCRTN "IPOTESI::OraArrivo"
   int Ora;
   COLLEGAMENTO_DIRETTO & Cd = * PColls[Tratta];
   if (Cd.UsareMultistazione) {
      if (Tratta == 0) {
         COLLEGAMENTO_DIRETTO & Cd2 = * PColls[Tratta+1];
         Ora = Cd2.Treni[Cd2.TrenoCorrente].OraPartenza ;
         // Se il tempo di collegamento della multistazione e' <= 5 minuti ne
         // deduco che o siamo all' interno dello stesso edificio o l' altra
         // stazione sta nel piazzale antistante: pertanto NON aggiungo il secondo tempo
         // margine
         if (Cd.MS_Minuti > 5) {
            Ora -= Cd.MinCoincidenza ;
         } /* endif */
         if(Ora < 0)Ora += 1440;
      } else {
         COLLEGAMENTO_DIRETTO & Cd2 = * PColls[Tratta-1];
         Ora = Cd2.Treni[Cd2.TrenoCorrente].OraArrivo ;
         // Se il tempo di collegamento della multistazione e' <= 5 minuti ne
         // deduco che o siamo all' interno dello stesso edificio o l' altra
         // stazione sta nel piazzale antistante: pertanto NON aggiungo il secondo tempo
         // margine
         if (Cd.MS_Minuti > 5) {
            Ora += Cd2.MinCoincidenza ;
         } /* endif */
         Ora += Cd.MS_Minuti;
         if(Ora > 1440)Ora -= 1440;
      } /* endif */
   } else {
      Ora = Cd.Treni[Cd.TrenoCorrente].OraArrivo;
   } /* endif */
   return Ora;
//<<< WORD __fastcall IPOTESI::OraArrivo BYTE Tratta  
};
//----------------------------------------------------------------------------
// IPOTESI::Clust
//----------------------------------------------------------------------------
T_CLUST & __fastcall IPOTESI::Clust(BYTE i){
   #undef TRCRTN
   #define TRCRTN "IPOTESI::Clust"
   
   COLLEGAMENTO_DIRETTO & Coll = * PColls[i];
   T_CLUST  * Clust;
   if (Coll.UsareMultistazione) {
      Clust =  &Coll.MS;
   } else {
      T_TRENO & Treno = Coll.Treni[Coll.TrenoCorrente];
      Clust = &Coll.Clusters[Treno.P_Cluster];
   } /* endif */
   return *Clust;
}

//----------------------------------------------------------------------------
// Trace di soluzione in preparazione
//----------------------------------------------------------------------------
void IPOTESI::TraceSoluzioneInPreparazione(const STRINGA& Mess, int Livello){
   #undef TRCRTN
   #define TRCRTN "IPOTESI::TraceSoluzioneInPreparazione"
   if(Livello > trchse)return;
   STRINGA Msg;
   Msg += " Parte alle ";
   Msg +=ORA(OraPartenza(0));
   Msg += " Arriva alle ";
   Msg +=ORA(OraArrivo(Path.NumNodi - 2));
   Msg += " ";
   Msg += Mess;
   ERRSTRING(Msg);
   for(int ii =0;ii < Path.NumNodi - 1; ii++){
      COLLEGAMENTO_DIRETTO & Corr = *PColls[ii];
      if(!Corr.UsareMultistazione){
         Msg = "MezzoV ";
         Msg += STRINGA(Corr.IdMezv());
         Msg += " of "+STRINGA(Corr.IdCluster());
         Msg += " Treno "+STRINGA(Corr.IdTreno());
         Msg += " "+ Corr.GetTrenoInCluster()->InfoUniforme.Decodifica();
      } else {
         Msg = " Collegamento urbano, ";
         Msg = " Cluster N¯" + STRINGA(Corr.MS.IdCluster);
      };
      Msg += " Da stazione "+ STRINGA(Esterno(Path.Nodi[ii].Id));
      // Msg += STRINGA(" ") + Stazioni.DecodificaIdStazione(Esterno(Path->Nodi[ii].Id));
      Msg += STRINGA(" ") + GRAFO::Gr()[Esterno(Path.Nodi[ii].Id)].Nome7();
      Msg += " Alle " + STRINGA(ORA(OraPartenza(ii)));
      Msg += " A  stazione "+ STRINGA(Esterno(Path.Nodi[ii+1].Id));
      // Msg += STRINGA(" ") + Stazioni.DecodificaIdStazione(Esterno(Path.Nodi[ii].Id));
      Msg += STRINGA(" ") + GRAFO::Gr()[Esterno(Path.Nodi[ii+1].Id)].Nome7();
      Msg += " Alle " + STRINGA(ORA(OraArrivo(ii)));
      ERRSTRING(Msg);
//<<< for int ii =0;ii < Path.NumNodi - 1; ii++  
   };
//<<< void IPOTESI::TraceSoluzioneInPreparazione const STRINGA& Mess, int Livello  
};
//----------------------------------------------------------------------------
// IPOTESI::Combina()
//----------------------------------------------------------------------------
// Metodo per identificare le soluzioni dell' IPOTESI
// Torna FALSE se in errore
//----------------------------------------------------------------------------
void IPOTESI::Combina(){
   #undef TRCRTN
   #define TRCRTN "IPOTESI::Combina()"
   
   #ifdef  SELECT_STAZIONI
   {
      BOOL Ok;
      {
         Ok = TRUE;
         int StazioniFiltro[] = SELECT_STAZIONI;
         if ((sizeof(StazioniFiltro ) / sizeof(int)) != Path.NumNodi ) { Ok = FALSE;
         } else for (int i = 0;Ok && i < Path.NumNodi ; i ++) if(Esterno(Path.Nodi[i].Id) != StazioniFiltro[i])Ok = FALSE;
      }
      #ifdef  SELECT_STAZIONI2
      if(!Ok){
         Ok = TRUE;
         int StazioniFiltro[] = SELECT_STAZIONI2;
         if ((sizeof(StazioniFiltro ) / sizeof(int)) != Path.NumNodi ){ Ok = FALSE;
         } else for (int i = 0;Ok && i < Path.NumNodi ; i ++) if(Esterno(Path.Nodi[i].Id) != StazioniFiltro[i])Ok = FALSE;
      }
      #endif
      Traccia = Ok;
      #ifndef SELECT_SOLO_TRACE
      if(!Ok)return ;
      #endif
   }
   #endif
   #ifdef  SELECT_GEN_STAZIONI
   {
      int StazioniFiltro[] = SELECT_GEN_STAZIONI;
      for (int i = Path.NumNodi - 1; i >= 0 ; i-- ){
         ID Id = Esterno(Path.Nodi[i].Id);
         for (int j = (sizeof(StazioniFiltro ) / sizeof(int)) -1 ;j >= 0 ; j-- ) if(StazioniFiltro[j] == Id)break;
         if(j <  0 )break;
      }
      Traccia = i >= 0;
      #ifndef SELECT_SOLO_TRACE
      if(i >= 0)return ;
      #endif
   }
   #endif
   
   #ifdef DBG29 // Mostro il percorso
   {
      STRINGA Msg("€€€€€€€€€€€€ ");
      Msg += STRINGA(Path);
      Msg+= " €€€€€€€€€€€€ " ;
      Msg+= STRINGA(Path.Km);
      Msg+= " Km   Path N¯ ";
      Msg+= STRINGA(Path.Progressivo);
      if(Illegale)Msg += " COLL_DIR e' ILLEGALE";
      TRACESTRING(Msg);
   }
   #endif
   
   if(Illegale)return ;
   PROFILER::Conta(35);
   
   // Debug dei clusters
   #ifdef DBG7
   for (int Ico = 0; Ico < NumTratte ; Ico ++) {
      COLLEGAMENTO_DIRETTO & Cd = THIS[Ico];
      if(Cd.MultiStaz)Cd.MS.DatiCluster->Dump( Esterno(Cd.Key.Partenza), Esterno(Cd.Key.Arrivo));
      for (int i = 0; i < Cd.NumClusters ; i++ ) {
         Cd.Clusters[i].DatiCluster->Dump( Esterno(Cd.Key.Partenza), Esterno(Cd.Key.Arrivo));
      } /* endfor */
   } /* endfor */
   #endif
   
   Soluzioni.Clear(); // Dovrebbe essere gia' vuoto
   MaxTrattaOk = -1 ;
   
   #ifdef DBG5A
   if(PATH_CAMBI::IsTargetAutoChk || Traccia){
      for    (int t = 0;t < NumTratte ;t++ ) {
         THIS[t].Trace("=============================================");
      } /* endfor */
      TRACESTRING("=============================================");
   }
   #endif
   
   // ..................................
   // La prima e l' ultima tratta NON Multistazione
   // ..................................
   
   COMB Ok;
   BOOL Iniziale =TRUE;
   do {
      Ok = TrovaSoluzione(Iniziale,0,NumTratte-1); // Identifico una soluzione
      Iniziale = FALSE;
      if(Ok == COMB_RETRY) continue;                  // Vi possono ancora essere soluzioni
      
      #ifdef DBG30
      TRACEVLONG(Ok);
      if(Ok != COMB_KO)TraceSoluzioneInPreparazione("Soluzione Potenziale:");
      #endif
      
      if(Ok == COMB_OK)CaricaSoluzione();     // Se la soluzione e' accettabile la carica in Soluzioni
      
   } while (Ok != COMB_KO) ; // Cicla finche' ho possibilita'
   
   // Se origine e destinazione sono nella stessa citta' non utilizzo le multistazione
   if (!PATH_CAMBI::RelazioneInUnaSolaCitta()) {
      // ..................................
      // La prima tratta Multistazione
      // ..................................
      if(NumTratte > 1 && PColls[0]->MultiStaz){
         Iniziale =TRUE;
         PColls[0]->UsareMultistazione = TRUE;
         do {
            Ok = TrovaSoluzione(Iniziale,1,NumTratte-1); // Identifico una soluzione
            Iniziale = FALSE;
            if(Ok == COMB_RETRY) continue;                  // Vi possono ancora essere soluzioni
            
            #ifdef DBG30
            TRACEVLONG(Ok);
            if(Ok != COMB_KO)TraceSoluzioneInPreparazione("Soluzione Potenziale:");
            #endif
            
            if(Ok == COMB_OK)CaricaSoluzione();     // Se la soluzione e' accettabile la carica in Soluzioni
            
         } while (Ok != COMB_KO) ; // Cicla finche' ho possibilita'
      }
      
      // ..................................
      // L' ultima tratta Multistazione
      // ..................................
      if(NumTratte > 1 && PColls[NumTratte-1]->MultiStaz){
         Iniziale =TRUE;
         PColls[NumTratte -1]->UsareMultistazione = TRUE;
         do {
            Ok = TrovaSoluzione(Iniziale,0,NumTratte-2); // Identifico una soluzione
            Iniziale = FALSE;
            if(Ok == COMB_RETRY) continue;                  // Vi possono ancora essere soluzioni
            
            #ifdef DBG30
            TRACEVLONG(Ok);
            if(Ok != COMB_KO)TraceSoluzioneInPreparazione("Soluzione Potenziale:");
            #endif
            
            if(Ok == COMB_OK)CaricaSoluzione();     // Se la soluzione e' accettabile la carica in Soluzioni
            
         } while (Ok != COMB_KO) ; // Cicla finche' ho possibilita'
      }
      
      // ..................................
      // La prima e l ' ultima tratta Multistazione
      // ..................................
      if(NumTratte > 2 && PColls[0]->MultiStaz && PColls[NumTratte-1]->MultiStaz){
         Iniziale =TRUE;
         PColls[0]->UsareMultistazione = TRUE;
         PColls[NumTratte -1]->UsareMultistazione = TRUE;
         do {
            Ok = TrovaSoluzione(Iniziale,1,NumTratte-2); // Identifico una soluzione
            Iniziale = FALSE;
            if(Ok == COMB_RETRY) continue;                  // Vi possono ancora essere soluzioni
            
            #ifdef DBG30
            TRACEVLONG(Ok);
            if(Ok != COMB_KO)TraceSoluzioneInPreparazione("Soluzione Potenziale:");
            #endif
            
            if(Ok == COMB_OK)CaricaSoluzione();     // Se la soluzione e' accettabile la carica in Soluzioni
            
         } while (Ok != COMB_KO) ; // Cicla finche' ho possibilita'
      }
//<<< if  !PATH_CAMBI::RelazioneInUnaSolaCitta     
   } /* endif */
   
   #ifdef DBG6
   TRACESTRING("Numero di soluzioni potenziali trovate: "+STRINGA(Soluzioni.Dim())+" Numero corrente soluzioni : "+STRINGA(Orario.Soluzioni.Dim()));
   #endif
   
   // Per prove estemporanee
   // if(Soluzioni.Dim() && Path.Attenzione)ERRSTRING("ATTENZIONE ! Non doveva trovare soluzioni");
   // if(!Soluzioni.Dim() && !Path.Attenzione)ERRSTRING("NOGOOD ! Doveva trovare soluzioni");
   
   if(MaxTrattaOk +1 < NumTratte)Path.DichiaraIncompatibile(MaxTrattaOk + 1);
   
   #ifdef PROFILER_ABILITATO
   if( Soluzioni.Dim() == 0 ) PROFILER::Conta(36);
   #endif
   
   return ;
   
//<<< void IPOTESI::Combina   
};
//----------------------------------------------------------------------------
// IPOTESI::CaricaSoluzione()
//----------------------------------------------------------------------------
// Metodo per caricare una soluzione in Soluzioni
// Scarta le soluzioni che utilizzano due volte di seguito lo stesso treno
//----------------------------------------------------------------------------
void IPOTESI::CaricaSoluzione() {
   #undef TRCRTN
   #define TRCRTN "IPOTESI::CaricaSoluzione()"
   
   SOLUZIONE * Sol= NULL;
   // ..................................
   // Determino la Soluzione
   // ..................................
   Sol= new SOLUZIONE;  // Inizializzata dal costruttore
   Sol->NumeroTratte   = Path.NumNodi -1;
   strcpy(Sol->IdentPartenza,GRAFO::Gr()[Orario.EIdOrigine()].Nome7());  // Identificatore della stazione di partenza
   
   BYTE GGP=0,GGA=0; // Giorni dalla partenza DELLA SOLUZIONE
   WORD OraLast = 0;
   PERIODICITA  PWrk;
   BOOL HoInfo = FALSE;
   CLU_BUFR * PClu  ;
   T_CLUST  * PClust;
   
   for(int i =0;i < Sol->NumeroTratte ;  i++){
      COLLEGAMENTO_DIRETTO & Coll = * PColls[i];
      SOLUZIONE::TRATTA_TRENO & Tratta = Sol->Tratte[i];
      Tratta.NoPenalita     = 0;
      Tratta.IdStazioneIn   = Esterno(Coll.Key.Partenza);
      Tratta.IdStazioneOut  = Esterno(Coll.Key.Arrivo);
      strcpy(Tratta.IdentOut,GRAFO::Gr()[Tratta.IdStazioneOut].Nome7());  // Identificatore della stazione di arrivo
      assert3(Coll.UsareMultistazione || Coll.Treni[Coll.TrenoCorrente].Utilizzabile,{
            TRACEVLONG(i);
            TRACEVLONG(Coll.UsareMultistazione);
            TRACEVLONG(Coll.TrenoCorrente);
            TRACEVLONG(Coll.Treni[Coll.TrenoCorrente].Utilizzabile);
         });
      
      if (Coll.UsareMultistazione) {
         CLU_BUFR & Clu =  *Coll.MS.DatiCluster ;  // Dati del cluster
         Tratta.IdMezzoVirtuale= 0;
         Tratta.IdCluster      = Coll.MS.IdCluster ;
         Tratta.OfsIn          = Coll.MS.PNodoIn;
         Tratta.OfsOut         = Coll.MS.PNodoOut;
         Tratta.Concorde       = TRUE;
         
         Tratta.OraIn         = OraPartenza(i);
         Tratta.OraOut        = OraArrivo(i);
         if(Coll.MS_Minuti <= 5 ) Tratta.NoPenalita = 1;
         Tratta.InfoTreno.Clear();
         Tratta.InfoTreno.TipoMezzo = COLLEGAMENTO_URBANO;
         // Si noti che non viene combinata con l' informazione della soluzione:
         // Altrimenti inibirebbe trasporto invalidi e biciclette.
         
         if(i == 0){
            memcpy(Sol->AcronimoStazionePartenza ,Clu.Acronimo(Tratta.OfsOut).Acronimo,4);
         }
         if(i == Sol->NumeroTratte-1) {
            memcpy(Sol->AcronimoStazioneArrivo ,Clu.Acronimo(Tratta.OfsIn).Acronimo,4);
         }
         if(!strcmp(Sol->AcronimoStazioneArrivo,"Mar") || !strcmp(Sol->AcronimoStazionePartenza,"Mar")){
            Tratta.NoPenalita = 1;
         }
         
         PWrk.Set();
         Sol->Km += Clu.Collegamento(Tratta.OfsIn , Tratta.OfsOut ).Km;
         
//<<< if  Coll.UsareMultistazione   
      } else {
         T_TRENO & Treno = Coll.Treni[Coll.TrenoCorrente];
         T_CLUST & Clust = Coll.Clusters[Treno.P_Cluster];
         PClust = &Clust;
         if (Clust.DatiCluster == NULL) {
            // Vuol dire che avevo acceduto al cluster, che sono in fase di estensione e che
            // i dati del cluster non sono piu' validi: in questo caso permetto comunque di fare I/O
            Clust.DatiCluster = CLU_BUFR::GetCluster(Clust.IdCluster , Clust.Concorde ,TRUE);
            if (Clust.DatiCluster == NULL) {  // Non c' e' verso: con tutta probabilita' ho finito i buffers
               ERRSTRING("Scartata soluzione causa errore in accesso dati dei treni");
               BEEP;
               Sol->Valida = FALSE;
               break;
            } else {
               Clust.PNodoIn  = Clust.DatiCluster->PosNodoInCluster(Esterno(Coll.Key.Partenza));
               Clust.PNodoOut = Clust.DatiCluster->PosNodoInCluster(Esterno(Coll.Key.Arrivo  ));
            } /* endif */
         } /* endif */
         CLU_BUFR & Clu = * Clust.DatiCluster ;  // Dati del cluster
         PClu = &Clu;
         Clu.FindTreno(Treno.P_Treno);
         //       TRACESTRING(VRS(Clust.IdCluster) + VRS(Clust.Concorde) + VRS(Clu.Treno) + VRS(Clu.ProgTreno()) +VRS(Clu.Treno->IdMezv) + VRS(Esterno(Coll.Key.Partenza)) + VRS(Esterno(Coll.Key.Arrivo  )) + VRS(Clust.PNodoIn )+ VRS(Clust.PNodoOut) );
         INFOSTAZ & Staz_In = Clu.Treno->Nodi[Clust.PNodoIn];
         INFOSTAZ & Staz_Out= Clu.Treno->Nodi[Clust.PNodoOut];
         
         Tratta.IdCluster      = Clust.IdCluster ;
         Tratta.OfsIn          = Clust.PNodoIn;
         Tratta.OfsOut         = Clust.PNodoOut;
         Tratta.Concorde       = Clust.Concorde;
         Tratta.IdMezzoVirtuale= Clu.Treno->IdMezv ;
         int KmTratta = Clu.PercorsiKm(Tratta.OfsIn , Tratta.OfsOut);
         Tratta.Km             = KmTratta;
         if (KmTratta < MM_CR.Lim2PenSuppl ) {
            Tratta.TrattaCorta  = 2;
         } else if( KmTratta < MM_CR.Lim1PenSuppl) {  //
            if(Clu.Treno->KmLimiteTrattaCorta > KmTratta) { // Non percorro metÖ del percorso del treno
               Tratta.TrattaCorta    = 1;
            } else {
               Tratta.TrattaCorta    = 0;
            } /* endif */
         } else {
            Tratta.TrattaCorta    = 0;
         } /* endif */
         Sol->NumeroTratteReali ++;
         Sol->Km += KmTratta ;
         Tratta._SuccPartenzaMV =  Staz_In.GiornoSuccessivoPartenza;
         Tratta.NomeSpecifico = NULL; // Verra' aggiunto alla fine PER LE SOLE SOLUZIONI VALIDE
         Tratta.OraIn         = Staz_In.OraPartenza;
         Tratta.OraOut        = Staz_Out.OraArrivo;
         strcpy(Tratta.IdTreno,Coll.IdTreno());
         Tratta.OfsTreno      = Treno.P_Treno;
         Tratta.PrgIn         = Staz_In.ProgressivoStazione;
         Tratta.PrgOut        = Staz_Out.ProgressivoStazione;
         Tratta.P_MezzoViagIn = Staz_In.P_MezzoViaggiante;
         Tratta.P_MezzoViagOut= Staz_Out.P_MezzoViaggiante;
         
         if (Clu.Treno->PeriodicitaUniforme) {
            PWrk = Clu.Treno->Per();
         } else {
            // Debbo combinare tutte le periodicita' del treno
            PWrk.ReSet();
            WORD IdT = Clu.Treno->IdMezv ;
            // Per non sfondare il cluster : Limit e' il primo BYTE non allocato
            BYTE * Limit = Clu.FirstTreno + (Clu.Dat->NumeroTreni * Clu.DimTreno);
            while(Clu.Treno->ProgressivoPeriodicita) {// Mi posiziono al primo record del treno
               Clu.PrevTreno();
            }
            while((BYTE*)Clu.Treno < Limit && Clu.Treno->IdMezv  == IdT){
               if(Clu.Treno->Nodi[Clust.PNodoIn].Partenza &&
                  Clu.Treno->Nodi[Clust.PNodoOut].Arrivo){
                  PWrk |= Clu.Treno->Per();
               } /* endif */
               Clu.NextTreno();
            }
            Clu.FindTreno(Treno.P_Treno); // Ripristino il treno corrente
         } /* endif */
//<<< if  Coll.UsareMultistazione   
      } /* endif */
      
      if(i == 0){
         Sol->SuccessivoRichiesta  = Tratta.OraIn < Orario.OraOrd;
         OraLast = Tratta.OraIn;
      }
      WORD Attesa = TempoTrascorso(OraLast,Tratta.OraIn);
      Sol->TempoTotaleDiAttesa      += Attesa;
      Sol->TempoTotaleDiPercorrenza += Attesa;
      Sol->TempoTotaleDiPercorrenza += TempoTrascorso(Tratta.OraIn,Tratta.OraOut); // Tempo totale di percorrenza
      if(GGA > GGP)GGP = GGA;
      if ( Tratta.OraIn < OraLast){ GGP ++; GGA = GGP; }
      OraLast = Tratta.OraIn ;
      if ( Tratta.OraOut < OraLast){ GGA ++; }
      OraLast = Tratta.OraOut ;
      Tratta._GiorniShiftP  = GGP;
      Tratta._GiorniShiftA  = GGA;
      // Calcolo di quanti giorni, dopo la partenza della SOLUZIONE, parta il
      // MEZZO VIRTUALE  di tratta
      int ShiftGG = GGP - Tratta._SuccPartenzaMV ;
      // E questa e' la data (offset) di partenza del MEZZO VIRTUALE di tratta
      int  GG = PERIODICITA::offsetPeriod      ; // Data della richiesta
      GG      += Sol->SuccessivoRichiesta      ; // Data di partenza della soluzione
      GG      += ShiftGG                       ; // Data di partenza del virtuale di tratta
      GG      -= Tratta._SuccPartenzaMV        ; // 1 Se la partenza di tratta si ha il giorno successivo alla data di partenza del mezzo virtuale della tratta.
      // A questo punto posso determinare i servizi
      if (!Coll.UsareMultistazione) {
         PClu->DeterminaServizi(PClust->PNodoIn, PClust->PNodoOut, GG, Tratta.InfoTreno);
         if (HoInfo) {
            Sol->InfoSoluzione.CombinaTratta(Tratta.InfoTreno);
         } else {
            Sol->InfoSoluzione = Tratta.InfoTreno;
            HoInfo = TRUE;
         } /* endif */
         // Registro l' id del traghetto, ma solo se vado dal continente in sardegna (i traghetti
         // da e verso la sicilia sono trattati come treni).
         if(Orario.GestioneParticolareTraghetti && Tratta.InfoTreno.Navale()) Sol->IdTraghetto = Tratta.IdMezzoVirtuale;
      } /* endif */
      // E riporto la periodicita' alla partenza DELLA SOLUZIONE:
      // Si noti che PWrk contiene la PERIODICITA' DEL MV di tratta .
      if (ShiftGG > 0) {
         while (ShiftGG -- ) PWrk.GiornoPrecedente();
      } else {
         while (ShiftGG ++ ) PWrk.GiornoSeguente();
      } /* endif */
      #ifdef DBG34A    // Mostra periodicita' tratta per tratta
      PWrk.Trace("Periodicita' della tratta N¯ "+STRINGA(i)+ " Normalizzata alla partenza della soluzione");
      #endif
      Sol->Period() &= PWrk;
      
//<<< for int i =0;i < Sol->NumeroTratte ;  i++  
   };
   
   // ..................................
   // Aggiungo la soluzione all' array Soluzioni
   // ..................................
   
   if (!Sol->Valida) {
      #ifdef DBG23
      Sol->Trace("Combina()--- > Soluzione non valida Cancellata ",2);
      #endif
      delete Sol;
   } else {
      // Calcolo il costo della soluzione
      Sol->CalcolaCosto();
      Soluzioni += Sol;
      // ..................................
      // Scrivo sul trace la soluzione
      // ..................................
      #ifdef DBG23
      Sol->Trace("Combina()--- > Soluzione Impostata",2);
      #endif
      #ifdef DBG34
      Sol->Period().Trace("Periodicita' della soluzione:");
      #endif
   } /* endif */
   
//<<< void IPOTESI::CaricaSoluzione    
};
void CLU_BUFR::Dump(ID Da, ID A , BOOL Force){   // Da ed A sono Id Esterni
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::Dump()"
   
   #ifdef  SELECT_CLUSTERS
   if(!Force){
      int ClustersFiltro[] = SELECT_CLUSTERS;
      for (int i = (sizeof(ClustersFiltro ) / sizeof(int))-1; i >= 0 ; i --) if( Dat->IdCluster == ClustersFiltro[i])break;
      if(i < 0 )return; // Cluster da non dumpare
   }
   #endif
   
   if(Dat == NULL){
      ERRSTRING("Cluster vuoto: ???");
      if(Force)ERRSTRING("Force = TRUE!");
      BEEP;
      return;
   }
   
   struct RICH { ID Cluster; ID Da; ID A;};
   static HASH<RICH> Hash(256,1024);
   
   if(Da != 0){  // Controllo di non ripetere lo stesso DUMP; Se 0 lo ripeto senza problemi
      RICH Rich;
      Rich.Cluster = Dat->IdCluster;
      #ifdef DBG7C
      Rich.Da = Rich.A = 0 ;
      #else
      Rich.Da = Da;
      Rich.A = A ;
      #endif
      if(Hash.Cerca(&Rich,6)!= NULL){
         TRACESTRING("Non ripetuta analisi del cluster "+STRINGA(Dat->IdCluster) +VRS(Da) + VRS(A));
         return;
      }
      *Hash.Alloca() = Rich; Hash.Metti(6);
   }
   
   int j,k;
   
   TRACESTRING("==========================================================");
   #ifdef DBG7C
   TRACESTRING("====== ANALISI DEL CLUSTER "+STRINGA(Dat->IdCluster) + " ========");
   #else
   TRACESTRING("====== ANALISI DEL CLUSTER "+STRINGA(Dat->IdCluster) +VRS(Da) + VRS(A)+" ========");
   #endif
   TRACESTRING("==========================================================");
   TRACESTRING("SensoNormaleDiPercorrenza: "+ STRINGA(Concorde ? "Normale" : "Inverso") );
   TRACEVLONG(Dat->IdCluster            );
   TRACEVLONG(Dat->NumeroNodi           );
   TRACEVLONG(Dat->TotStazioni          );
   TRACEVLONG(Dat->NumeroTreni          );
   TRACEVLONG(Dat->DimDati              );
   TRACELONG("Nodi del cluster: N¯ ",Dat->NumeroNodi);
   for (j=0;j < Dat->NumeroNodi ; j++ ) { // Trace dei nodi
      #if !defined(DBG7A) && !defined(DBG7C)
      if(Nodi[j].IdNodo != Da && Nodi[j].IdNodo != A)continue;
      #endif
      Orario.TraceIdEsterno(" Nodo["+STRINGA(j)+STRINGA("] ") + STRINGA(Nodi[j].MediaKm)+ STRINGA(" KmMedi") +
         " ["+STRINGA(Nodi[j].Gruppi)+"]" ,Nodi[j].IdNodo);
   } /* endfor */
   
   #ifndef DBG7C
   
   if(Da == 0 || A == 0) return;
   if(Tipo == CLUSTER_MULTISTAZIONE) { // Cluster multistazione
      for (k = 0; k < Dat->NumeroNodi; k++) {
         #ifndef DBG7A
         if(Nodi[k].IdNodo != Da && Nodi[k].IdNodo != A)continue;
         #endif
         for (j = 0;  j < Dat->NumeroNodi; j++) {
            #ifndef DBG7A
            if(Nodi[j].IdNodo != Da && Nodi[j].IdNodo != A)continue;
            #endif
            COLLEG_URBANO & Cu = Collegamento(k,j);
            TRACESTRING("Collegamento urbano tra stazioni "+STRINGA(Nodi[k].IdNodo)+" e "+STRINGA(Nodi[j].IdNodo)+" Km="+ STRINGA(Cu.Km)+" minuti="+STRINGA(Cu.Minuti));
         } /* endfor */
      } /* endfor */
   } else {
      FindTreno(0);
      int P_Da = PosNodoInCluster(Da);
      int P_A  = PosNodoInCluster(A);
      for (k=0 ;k < Dat->NumeroTreni; NextTreno(),k++ ) {
         // Se non ferma ad origine e destinazione SKIP
         #ifndef DBG7A
         if(! (Nodo(P_Da).Partenza && Nodo(P_A).Arrivo))continue;
         #endif
         char Tre[6]; memmove(Tre,IdTrenoCorrente(P_Da),5);Tre[5] = 0;
         STRINGA Prg="/"+STRINGA(Treno->ProgressivoPeriodicita );
         if(Treno->ProgressivoPeriodicita == 0)Prg.Clear();
         MM_INFO Info;
         //if (Treno->IdMezv == 9944 && Nodi[P_Da].IdNodo == 892) {
         //   TRACESTRING("Eccomi!");
         //} /* endif */
         DeterminaServizi(P_Da, P_A, -1, Info);
         
         TRACESTRING(" ...................... MV["+STRINGA(k) +STRINGA("] ")+
            STRINGA(Treno->IdMezv)+Prg+" Treno "+STRINGA(Tre) +
            " ["+STRINGA(Treno->Gruppo)+"]"                   +
            " LimiteTC: "+STRINGA(Treno->KmLimiteTrattaCorta) +
            " Info (dataric) : " + Info.Decodifica()
            // + " Info Uniforme: " +  Treno->InfoUniforme.Decodifica()
         );
         #ifdef DBG7B
         Treno->Per().Trace("Periodicita' del mezzo virtuale:");
         #endif
         WORD NNodi = Dat->NumeroNodi;
         for (j=0;j < NNodi;  j++ ) {
            #ifndef DBG7A
            if(Nodi[j].IdNodo != Da && Nodi[j].IdNodo != A)continue;
            #endif
            INFOSTAZ & NodoInEsame = Nodo(j);
            BOOL Ferma    = NodoInEsame.Ferma();
            if(!Ferma && ! NodoInEsame.TransitaOCambiaTreno )continue; // Skip dei nodi su cui non ferma ne transita
            // Dati relativi al passaggio del treno sul nodo.
            // Desunti da dati generali e poi corretti con dati specifici
            short int Sosta = (NodoInEsame.Partenza && NodoInEsame.Arrivo) ?
            (NodoInEsame.OraPartenza - NodoInEsame.OraArrivo) : 0; // Durata della sosta
            STRINGA Msg;
            Msg = "Nodo["+STRINGA(j)+STRINGA("]");
            Msg += "  ["+STRINGA(NodoInEsame.ProgressivoStazione)+"]\t";
            //WORD Ora = (NodoInEsame.OraArrivo ? NodoInEsame.OraArrivo : NodoInEsame.OraPartenza);
            // Msg += ORA(Ora)+STRINGA("\t");
            Msg += ORA(NodoInEsame.OraArrivo)+STRINGA("->");
            Msg += ORA(NodoInEsame.OraPartenza)+STRINGA("\t");
            Msg += " ["+STRINGA(Nodi[j].Gruppi)+"]";
            if(Sosta> 1){
               Msg += " Sosta "+STRINGA(Sosta)+" ";
            } else if(NodoInEsame.Arrivo && NodoInEsame.Partenza){
               Msg += " Ferma ";
            } else if(NodoInEsame.Partenza){
               Msg += " Parte ";
            } else if(NodoInEsame.Arrivo){
               Msg += " Arriva ";
            } else if(NodoInEsame.TransitaOCambiaTreno){
               Msg += " Transita ";
            }
            Orario.TraceIdEsterno(Msg,Nodi[j].IdNodo);
//<<<    for  j=0;j < NNodi;  j++    
         } /* endfor */
//<<< for  k=0 ;k < Dat->NumeroTreni; NextTreno  ,k++    
      } /* endfor */
//<<< if Tipo == CLUSTER_MULTISTAZIONE    // Cluster multistazione
   } /* endif */
   
   #endif
//<<< void CLU_BUFR::Dump ID Da, ID A , BOOL Force     // Da ed A sono Id Esterni
}
