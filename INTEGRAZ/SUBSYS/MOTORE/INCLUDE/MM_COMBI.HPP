//========================================================================
// MM_COMBI : classi per la fase di combinazione dei treni ed identificazione soluzioni
//========================================================================
// Attenzione: Cambiando i formati dei files dati del motore in modo da
// renderlo incompatibile, modificare la #define ORARIO_FORMATO_ATTUALE  
// Vedere la classe VALIDITA_ORARIO
//========================================================================
//
#ifndef HO_MM_COMBI_HPP
#define HO_MM_COMBI_HPP

WORD __fastcall AttesaValida(WORD Ora1,WORD Ora2); // Calcola l' attesa tra due ore (escludendo le ore notturne)

enum COMB { COMB_OK , COMB_RETRY, COMB_KO } ; // possibili risposte combinando i treni

//------------------------------------------------------------------------
// CLUSTER
//------------------------------------------------------------------------
// Contiene i dati di accesso dei clusters
//------------------------------------------------------------------------
class  CLUSTER {    // 13 Bytes

   public: 

   BIT IdCluster        : 13 ; // Id del cluster     
   BIT OffSetBytes      : 23 ; // Offset nel file estensione (in bytes) : Max  8 MBytes
   BIT DimDatiC         : 19 ; // Dimensione del cluster: MV Concordi   : Max 512 K
   BIT DimDatiD         : 19 ; // Dimensione del cluster: MV Discordi   : Max 512 K 
   BIT Tipo             :  4 ; // Tipo cluster
   BIT Id1              : 13 ; // Stazione piu' importante      // Gli id delle due stazioni servono solo per il TRACE 
   BIT Id2              : 13 ; // 2ø Stazione piu' importante   // Gli id delle due stazioni servono solo per il TRACE 

   static CLUSTER * Clusters;     // Dati dei clusters
   static int       NumClusters;  // Numero di clusters presenti (elemento 0 e' vuoto)

};

//------------------------------------------------------------------------
// T_CLUST
//------------------------------------------------------------------------
// Dati dei cluster corrispondenti ad una tratta
// Non sono compresi i cluster multistazione per i quali 
// riporto i dati direttamente su TRATTA
//------------------------------------------------------------------------
struct T_CLUST {
   ID         IdCluster   ;  // Esterno
   BYTE       Concorde    ;  // Senso di percorrenza
   CLU_BUFR * DatiCluster ;  // Dati del cluster
   WORD       PNodoIn     ;  // Indice del nodo con cui accedo al cluster (nei nodi del cluster)
   WORD       PNodoOut    ;  // Indice del nodo con cui esco dal cluster (nei nodi del cluster)
};

//------------------------------------------------------------------------
// T_TRENO
//------------------------------------------------------------------------
// Dati dei treni corrispondenti ad una tratta
//------------------------------------------------------------------------
struct T_TRENO { // 6 BYTES compresi i dati di servizio
   BIT   OraPartenza             :11; // in minuti dopo la mezzanotte
   BIT   OraArrivo               :11; // in minuti dopo la mezzanotte
   BIT   GiorniCircolazione      : 4; // Indica se viaggia sulla tratta PARTENDO per quattro giorni a partire
                                      // alla DATA DELLA RICHIESTA. 
                                      // NB: gia' consuntiva i varii progressivi periodicita'
                                      // NB: Gia' considera i treni che partono il giorno precedente

   BIT   P_Cluster               : 6; // Puntatore al cluster (Nel COLLEGAMENTO_DIRETTO)
   BYTE  P_Treno                    ; // Indice del treno tra i treni del cluster

   // Dati utilizzati dall' algoritmo di combinazione.
   BIT   RidottoTempoCoincP      : 1; // Mi indica se e' un treno regionale (alla stazione di partenza)
   BIT   RidottoTempoCoincA      : 1; // Mi indica se e' un treno regionale (alla stazione di arrivo  )
   BIT   LettiOCuccette          : 1; // Mi indica se ha letti o cuccette (alla partenza o all' arrivo)
   BIT   Navale                  : 1; // Mi indica se ho usato traghetti o similari (questi mezzi hanno sempre classifica uniforme)

   // Dati di servizio
   BIT   DataPartenza            : 2; // Indica quanti giorni dopo la data della richiesta avviene la partenza del treno
   BIT   Utilizzabile            : 1; // Indica se e' un treno utilizzabile 

   // Questo indica se il treno (per qualche motivo) deve essere ignorato
   BIT   Valido                  : 1; // Se FALSE e' come non ci fosse (Lo lascio per debug) 

   void ForzoCircolaSempre(){ GiorniCircolazione = 0xf;};

   // Questo flag inibisce in modo assoluto il controllo di periodicita' e le selezioni sui treni
   static BOOL PerDirettiPrenotabili;

  // NB: Se non e' valido i giorni di circolazione sono forzati a 0 !
   BOOL Circola(){return Bool(GiorniCircolazione  & (1 << DataPartenza)); };

   // Ritorna FALSE se il treno NON deve essere caricato
   BOOL __fastcall CaricaEValidaTreno(CLU_BUFR & Buffer,T_CLUST & Clu ); // Valida e carica i dati di T_TRENO

};
typedef ARRAY_DINAMICA<T_TRENO> T_TRENI;

//------------------------------------------------------------------------
//  COLLEGAMENTO_DIRETTO
//------------------------------------------------------------------------
// Collegamento diretto tra due stazioni: e' gestito con una CACHE
// in modo da poterlo riutilizzare per piu' IPOTESI
// Chiave: Partenza / Arrivo (4 Bytes)
//------------------------------------------------------------------------
class COLLEGAMENTO_DIRETTO {

   public: 

   struct KEY {
      WORD Partenza      ; // Stazione di partenza del collegamento (Id Interno)
      WORD Arrivo        ; // Stazione di arrivo   del collegamento (Id Interno)
   } Key;

   WORD NumTreni         ; // Numero dei treni che realizzano il collegamento diretto (in ordine di ora partenza)
                           // NB: Esclusi quelli inibiti
   WORD NumTreniAllocati ; // Idem compresi quelli inibiti
   BYTE LimiteTreni      ; // Numero limite dei treni che possono essere fuori ordine
                           // Due treni la cui posizione nel cluster sia piu' distante di 
                           // LimiteTreni sono sicuramente in ordine corretto di ora di arrivo

   BIT  Illegale      : 1; // Se vero vuol dire che il collegamento non ha Treni 
   BIT  DatiValidi    : 1; // Se vero vuol dire che i dati di treni e cluster (dovrebbero) essere validi
   BIT  NumClusters   : 6; // Numero dei clusters che contengono i dati dei treni (Max 63)
   BIT  MinCoincidenza :5; // Minuti di coincidenza della stazione di arrivo

   BIT  MultiStaz     : 1; // Indica se ho (anche) un collegamento multistazione tra le localita'
   BIT   MS_Minuti    : 7; // Eventuali minuti necessari per il collegamento
   BIT   MS_Km        : 7; // Eventuali Km     necessari per il collegamento
   T_CLUST MS            ; // Dati dell' eventuale cluster multistazione

   T_CLUST * Clusters    ; // Puntamento ai clusters : ve ne sono NumClusters
   T_TRENO * Treni       ; // Puntamento ai treni    : ve ne sono NumTreniAllocati 

   // Hooks di inizializzazione e terminazione dalla CACHE:  I parametri NON sono utilizzati
   BOOL  __fastcall InitForCache(void * , void * );
   void  __fastcall RemoveFromCache(void *);

   // Funzione per ottenere i dati . Torna FALSE se i dati non sono disponibili
   // I dati letti rimangono in Lock
   // PuoiEseguireIO  = 0 controlla se ho gia' i dati
   // PuoiEseguireIO  = 1 accede ai dati se non li ha
   // PuoiEseguireIO  = 2 Utilizza solo i dati che ho gia' in cache
   BOOL GetDati(BYTE PuoiEseguireIO); 

   // Questa funzione elimina il Lock dai cluster utilizzati
   void FreeDati();

  // Questa funzione calcola LimiteTreni
  void CalcolaLimiteTreni();

  // Questa funzione fa un trace di tutti i treni caricati
   void  Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

  private:
   T_TRENO * Trn(WORD i){ while(i >= NumTreni)i -= NumTreni; return Treni +i;}; // Per uso interno
   // Per gestire Ora di arrivo in routine di calcolo limite treni
   WORD OA( T_TRENO * Base, T_TRENO * x){ return TempoTrascorso(Base->OraPartenza,x->OraPartenza) + TempoTrascorso(x->OraPartenza,x->OraArrivo);};

   // Questi dati sono per gli algoritmi interni
   short  TrenoCorrente           ; // Puntatore al treno corrente in Treni; -1 se nessun treno e' utilizzabile 
   BYTE   UsareMultistazione      ; // Indica di utilizzare il collegamento multistazione per il cambio
   // Questi servono per identificare TUTTE le alternative di viaggio utili per arrivare a destinazione dati il primo e l' ultimo treno
   short  MinimoUtilizzabile      ; // <0 : Non ho alternative
   short  MassimoUtilizzabile     ; // <0 : Non ho alternative 

   static WORD OraLimite          ; // Per i treni con ora < OraLimite considero un passaggio attraverso la mezzanotte
   static WORD OraMinimaPartenza  ; // Per i metodi di posizionamento. E' eguale a OraLimite per tutte le tratte tranne la prima (per cui non e' definita)
   static WORD OraMassimaArrivo   ; // Per IdentificaAlternative
   static WORD GiorniDaInizio     ; // Per i metodi di posizionamento
   static WORD LastOraArrivo      ; // Per calcolare l' attesa valida
   static T_TRENO * Last          ; // Per i metodi di posizionamento 
   static BYTE LastMultistazione  ; // Per i metodi di posizionamento  


   // Metodi di Posizionamento e processing
   COMB __fastcall PosizionaPrimo()      ; // Per prima tratta
   COMB __fastcall PosizionaIntermedio() ; // Per tratte intermedie
   COMB __fastcall PosizionaUltimo()     ; // Per ultima tratta
   int  __fastcall IdentificaAlternative(BOOL Last) ; // Identifica le possibili alternative

   // Metodi di trace
   struct CLUSTRENO * __fastcall GetTrenoInCluster();
   struct INFOSTAZ  * __fastcall GetDatiStazioneInCluster();
   int __fastcall IdMezv();
   int __fastcall IdCluster();
   char * __fastcall IdTreno(); // Rientrante solo 10 volte
   STRINGA DesTreno();
   STRINGA DesTreno(WORD Pos);

   // Criteri 
   BOOL __fastcall ControllaAttesaValida();

   friend class IPOTESI;
   friend class DATI_ORARIO_FS;

};
typedef COLLEGAMENTO_DIRETTO * P_COLLDIR;

//------------------------------------------------------------------------
// IPOTESI
//------------------------------------------------------------------------
// Questa struttura gestisce tutti i dati generati da un PATH_CAMBI
// 
// Il PATH CAMBI deve esistere prima di Creare l' IPOTESI e rimane
// in vita dopo che l' IPOTESI viene distrutta ( perche' puo' diramare)
//
// I dati dei cluster sono gestiti a cache dalla cache di I/O
//
// Dai dati dei cluster sono estratti dei puntamenti ai treni che 
// realizzano i collegamenti diretti tratta per tratta. 
// Anche tali puntamenti sono gestiti in cache ( e sono in lock finche'
// esiste una IPOTESI che li utilizza);
//------------------------------------------------------------------------
class IPOTESI {

   public:

   PATH_CAMBI & Path     ; // Path cambi originale

   BYTE         Illegale ; // Vuol dire che sicuramente NON e' collegata da treni

   IPOTESI(PATH_CAMBI & PtC);
   ~IPOTESI();

   // Funzioni di risoluzione
   void PreFetch()           ; // Accede fisicamente ai dati e blocca i cluster
   void Combina()            ; // Identifica le soluzioni di viaggio
   void CaricaSoluzione()    ; // Se la soluzione e' accettabile la carica in Soluzioni
   COMB TrovaSoluzione(BOOL Iniziale, BYTE PrimaTratta, BYTE UltimaTratta);

   // Funzioni accessorie
   WORD      __fastcall OraPartenza(BYTE Tratta);
   WORD      __fastcall OraArrivo(BYTE Tratta);
   T_CLUST & __fastcall Clust(BYTE i);

   // Metodi di trace
   void TraceSoluzioneInPreparazione(const STRINGA& Mess, int Livello= LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   COLLEGAMENTO_DIRETTO & operator [](int Idx){ return * PColls[Idx];};

   static ELENCO_SOLUZIONI Soluzioni       ; // Contiene le soluzioni trovate: Poi trasferite su DATI_ORARIO_FS

   private  : 

   int NumTratte;

   int MaxTrattaOk;       // Indice dell' ultima tratta che ho combinato con successo

   static WORD NextTreno;   // Per TrovaSoluzione

   P_COLLDIR PColls[MAX_TRATTE];

   static CACHE<COLLEGAMENTO_DIRETTO> Cache;

   static BOOL UsaDatiInCache; // Quando e' vera inibisco gli I/O ma continuo a lavorare con i dati che ho gia' in memoria

   friend class CLU_BUFR;
   friend class DATI_ORARIO_FS;
   friend class COLLEGAMENTO_DIRETTO;

};

#endif
