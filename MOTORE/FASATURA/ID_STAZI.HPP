//============================================================================
// 3.2     ±±±±±±±±±±±±±±±±±±±± MODIFICATO ±±±±±±±±±±±±±±±±±±±±±±±±
//============================================================================
/* Montagna
   A partire dal rilascio per la 3.2 sono state fatte importanti
   modifiche:

      - E' cambiato il formato di R_STRU.  Gli attributi esistenti sono
         stati riorganizzati in modo da rispecchiare la suddivisione
         logica per classi.
         Tale suddivisione e' descritta in un diagramma a parte.
         Non dovrebbe creare problemi.

      - NON esistono piu' fisicamente i files con le "estrazioni" delle
         stazioni per nome.
         Esistono invece classi che "SIMULANO" un comportamento analogo
         al precedente.
         Si noti che possono ritornare direttamente un record ID_STAZI
         per cui si potrebbe semplificare il coding.

      - Analogamente vale per CCR_ID

      - Il file delle stazioni viene "rinforzato" con una cache
         dal funzionamento trasparente (funziona anche in multithread).
         Viene aperta UNA SOLA istanza fisica del file stazioni.

      - I codici CCR sono a 17 o a 24 bits:  nel primo caso si
         riferiscono a stazioni italiane (con il codice rete 83
         implicito) , nel secondo caso a stazioni estere.

         In ogni caso si utilizza un INTERO binario il cui valore modulo
         100000 e' il codice CCR e il cui quoziente nella divisione per
         100000 e' il codice rete.

         Una stazione italiana viene riconosciuta sia con che senza il
         codice rete:  es Roma = 8409 oppure 8308409
         Il CCR delle stazioni italiane e' conservato SENZA il codice
         rete nei campi a 17 bits, e con il codice rete nei campi
         a 24 bits.

      - Gli ID delle stazioni sono ora delle WORD a 16 bits, permettendo
         di gestire fino a 65500 stazioni.  Il limite e' strutturale e
         per rimuoverlo sono necessarie modifiche al motore.

   Tutto cio' si e' reso necessario in vista della gestione delle
   stazioni internazionali, per la quale si passera' dalla
   attuale gestione di ~ 4000 stazioni a ~ 50000.
*/
//============================================================================

#ifndef HO_STAZIONI_HPP
#define HO_STAZIONI_HPP

#ifndef HO_BASE_HPP
#include "BASE.HPP"
#endif

#include "string.H"

#ifndef HO_FILES_HPP
#include "FILE_RW.HPP"
#endif

#ifndef INCL_DOSSEMAPHORES
   typedef  ULONG    HMTX;            /* hmtx */
   typedef  ULONG    HMUX;            /* hmux */
#endif

// Usato solo nella fornitura di dati al DB2
enum TIPO_STAZIONE {
  ST_UNDEFINED = 0,          // Tipo stazione non pertinente (record non caricato)
  ST_RAMO=       1,          // Stazione di Ramo FS
  ST_NODO=       2,          // Nodo FS o CUMULATIVO
  ST_TERMINALE=  3,          // Stazione Terminale FS o CUMULATIVO
  ST_NON_FS=     5,          // Da qui in poi le stazioni non FS
  ST_CUMUL=      6,          // Stazione DI RAMO del servizio cumulativo
  ST_TRAMAR=     7,          // Fittizia transito Mare
  };

// NB: Le stazioni estere sono classificate come ST_NON_FS

enum TIPO_IMPIANTO_FISICO {
  IMF_UNDEFINED = 0,         // Non pertinente
  IMF_STAZIONE  = 1,         // Stazione
  IMF_CASA_CAN  = 2,         // Casa Cantoniera (e' anche fermata)
  IMF_FERM_IMPR = 3,         // Fermata Impresenziata
  IMF_SCALO     = 4,         // Scalo (Merci)
  IMF_SCALO_MAR = 5          // Scalo Marittimo
  };

enum CARATTERISTICHE_IMPIANTO_FISICO {
  CIMF_UNDEFINED = 0,        // Non pertinente
  CIMF_Concessa  = 1,        // Stazione ferrovia concessa
  CIMF_Frontiera = 2,        // Stazione di frontiera
  CIMF_Interna   = 3,        // Stazione interna
  CIMF_Nodo      = 4,        // Nodo / Bivio (Risulta su orario ... )
                             // Nota: Per queste stazioni fittizie viene FORZATO lo
                             // stato a CHIUSO.
  CIMF_Scambio   = 5,        // Punto di scambio tra FS e ferrovia(e) concessa(e)
  CIMF_Virtuale  = 6         // Impianto Virtuale (Non risulta su orario)
                             // Nota: Per queste stazioni fittizie viene FORZATO lo
                             // stato a CHIUSO.
  };

enum CODICI_ISTAT_REGIONE {
 IST_PIEMONTE     = 1, IST_VALLE_AOSTA  = 2, IST_LOMBARDIA    = 3, IST_TRENTINO_AA  = 4,
 IST_VENETO       = 5, IST_FRIULI_VG    = 6, IST_LIGURIA      = 7, IST_EMILIA_R     = 8,
 IST_TOSCANA      = 9, IST_UMBRIA       =10, IST_MARCHE       =11, IST_LAZIO        =12,
 IST_ABRUZZO      =13, IST_MOLISE       =14, IST_CAMPANIA     =15, IST_PUGLIA       =16,
 IST_BASILICATA   =17, IST_CALABRIA     =18, IST_SICILIA      =19, IST_SARDEGNA     =20
};

enum CODICI_TARIFFA_REGIONE {
 TRF_UNDEFINED    = 0, // Non si applica tariffa regione
 TRF_ABRUZZO      = 1, TRF_BASILICATA   = 2, TRF_BOLZANO      = 3, TRF_CALABRIA     = 4,
 TRF_CAMPANIA     = 5, TRF_EMILIA_R     = 6, TRF_FRIULI_VG    = 7, TRF_LAZIO        = 8,
 TRF_LIGURIA      = 9, TRF_LOMBARDIA    =10, TRF_MARCHE       =11, TRF_MOLISE       =12,
 TRF_PIEMONTE     =13, TRF_PUGLIA       =14, TRF_SARDEGNA     =15, TRF_SICILIA      =16,
 TRF_TOSCANA      =17, TRF_TRENTINO     =18, TRF_UMBRIA       =19, TRF_VALLE_AOSTA  =20,
 TRF_VENETO       =21
};

#define NstazDIM  36
class FILE_STZ; // File stazioni
class FILE_CCR; // File con codici CCR
struct CCR_STRU {
   DWORD CodiceCCR       ; // KEY: e' il codice CCR (con codice rete per le sole stazioni estere)
   ID    Id              ; // ID Corrispondente
   BOOL InitForCache(class CCR_STRU * Key, void * Cache);
   void RemoveFromCache( void * Cache){};
};
//----------------------------------------------------------------------------
// Struttura record delle stazioni
//----------------------------------------------------------------------------
// NB: Questa struttura dati e' in realta' una realizzazione
// di una gerarchia ISA, in cui tutti i records sono eguali.
// Per cui non tutti i campi sono sempre valorizzati, oppure
// alcuni campi hanno un significato multiplo.
// Si veda la documentazione per la descrizione della gerarchia
// E' disponibile un diagramma di Coad.
//----------------------------------------------------------------------------
// All' inizio del commento e' indicata la classe logica di appartenza
// di ogni attributo, con la seguente decodifica:
// ST   STAZIONE              Tutte le stazioni
// IC   IMPIANTO COMMERCIALE  Le stazioni che possono vendere biglietti
// GR   IMPIANTO GRAFO        Le stazioni che sono presenti sul grafo
// RM   DI RAMO               Le stazioni che appartengono ad un ramo del grafo
// NZ   NAZIONALE             Le stazioni italiane
// FS   FS                    Le stazioni delle FS
// EM   EMISSIONE             Le stazioni FS che possono emettere biglietti con il SIPAX
// IS   INSTRADAMENTO         Le stazioni di instradamento secondo normativa
// CU   CONCESSA              Le stazioni delle ferrovie concesse
// ES   ESTERE                Le stazioni di reti estere
// CE   CONFINE ESTERE        Le stazioni di confine tra reti estere
// PR   PRENOTABILE           Le stazioni che possono essere prenotate (via TPN od HERMES)
// XX   IMPROPRIO             Per compatibilita' o altre amenita'
// ......................................................
// NB: Per il funzionamento della cache IdStazione deve
//     stare all' inizio del record.
// NB: Ho usato la stessa area fisica per il nome 20 della stazione Italiana ed il 3ø nome estero della stazione
// ......................................................
struct STAZ_REC {
    ID   IdStazione                 ;// ST: GR: Id Interno = Codice DKM per le stazioni del grafo
                                     // Se e' 0 vuol dire che per quel codice non e' definita una stazione
    char NomeStazione     [NstazDIM];// ST: 35 caratteri : Descrittivo
    char NomeStaz20             [21];// ST: 20 caratteri : Stampa Biglietto (Nonche' 3ø Nome estero)
    char Nome7                  [ 8];// ST: 7  caratteri : Instradamenti (solo stazioni instradamento altrimenti "???????")
    ID   IdStazione1                ;// RM: Primo nodo del ramo   : 0 per i nodi
    ID   IdStazione2                ;// RM: Secondo nodo del ramo : 0 per i nodi e terminali
    BYTE Distanza1                  ;// RM: Distanza verso il primo   nodo del ramo (max 255 Km)
    BYTE Distanza2                  ;// RM: Distanza verso il secondo nodo del ramo (max 255 Km)
    BYTE ProgRamo                   ;// RM: Progressivo stazione nell' ambito del ramo
                                     //     Definito solo per le stazioni non nodali (con entrambi gli id diversi da 0):
                                     //     Ha range max da 1 a 62
    char NomeStaz12             [13];// EM: 12 caratteri : Impianto Emittente (solo sedi SIPAX)
    char NomeEstero             [21];// CE: Stazioni di confine tra due nazioni
    char NomeEstero2            [21];// CE: Stazioni di confine tra due nazioni
    char * NomeEstero3(){ return NomeStaz20;}; // Perche' non puo' essere contemporaneamente Italiana e di confine tra tre nazioni estere
    BIT  StazioneCumulativo     : 1 ;// ST: e' una stazione del servizio cumulativo
    BIT  StazioneFs             : 1 ;// ST: e' una stazione delle FS
    BIT  estera                 : 1 ;// ST: e' una stazione di una o piu' reti estere
    BIT  Aperta                 : 1 ;// ST: e' aperta (puo' anche essere fittizia, merci ecc).
    BIT  vendibile              : 1 ;// ST: ES: e' vendibile (ES: E' vendibile con i precaricati)
    BIT  TipoStazione           : 3 ;// ST: vedere enum TIPOSTAZIONE per i valori
    BIT  ImpiantoCommerciale    :17 ;// IC: Codice Impianto Commerciale
    BIT  Fittizia               : 1 ;// GR: e' una stazione fittizia
    BIT  IstatRegione           : 5 ;// NZ: Codice Istat regione di appartenenza
    BIT  TariffaRegione         : 5 ;// NZ: Codice tariffario regione
    BIT  Prima_Estensione       : 5 ;// NZ: 1ø regione di estensione
    BIT  Seconda_Estensione     : 5 ;// NZ: 2ø regione di estensione
    BIT  Terza_Estensione       : 5 ;// NZ: 3ø regione di estensione (solo se transito: non vale per origine e destinazione)
    BIT  StazioneTraMarCum      : 1 ;// NZ:  e' una stazione su cui ho un transito mare Tirrenia.
    BIT  CodiceCCR              :17 ;// FS: SOLO per stazioni FS
    BIT  CodiceInstradamentoCvb :10 ;// IS: Codice di instradamento CVB
    BIT  CCRCumulativo1         :17 ;// CU: Per stazioni cumulative
    BIT  SocietaConcessa1       : 7 ;// CU:    e relativa societa
    BIT  CCRCumulativo2         :17 ;// CU: Per stazioni cumulative
    BIT  SocietaConcessa2       : 7 ;// CU:    e relativa societa
    BIT  CCREstero              :24 ;// ES: Per stazioni estere
    BIT  CCRVendita             :24 ;// ES: Per stazioni estere per cui si deve vendere sino ad un' altra stazione
    BIT  Informativa            : 1 ;// ES: Si deve dare informativa per la stazione
    BIT  InPrenotazione         : 1 ;// ES: Deve essere visibile nelle schermate di prenotazione
                                     // NB: Non e' detto che tutte le stazioni prenotabili siano visibili ...
    BIT  CCREstero2             :24 ;// CE: Stazioni di confine tra due nazioni
    BIT  CCREstero3             :24 ;// CE: Stazioni di confine tra tre nazioni
    BIT  CodiceContabAusiliario :24 ;// CE: Codice contabile univoco per le stazioni di confine
    BIT  CodiceTPN              :17 ;// PR: Codice TPN della stazione
    BIT  DestinazCumulativo     : 1 ;// XX: 1 se puo' essere destinazione di un biglietto cumulativo
                                     //     Puo' anche essere una stazione FS presente su RELTREVE
    BIT  TipoImpiantoFisico     : 3 ;// XX: Vedi enum TIPO_IMPIANTO_FISICO per i valori
    BIT  CarImpiantoFisico      : 3 ;// XX: Vedi enum CARATTERISTICHE_IMPIANTO_FISICO per i valori
};

//----------------------------------------------------------------------------
// Stazioni
//----------------------------------------------------------------------------
// Si basa su di un file contenente TUTTE le stazioni (id consecutivi).
// Il formato del singolo record del file non comprende il null finale (Filler)
// La struttura e' una realizzazione di una complessa gerarchia di
// ereditarieta' multipla: si veda tale struttura per comprendere la
// signaficativita' dei varii attributi.
//----------------------------------------------------------------------------
class _export STAZIONI { // Accesso per Id o per codice CCR
   public:

   struct R_STRU : public STAZ_REC {

      // Caratterizzazione della stazione :
      // Ho usato funzioni funzioni di accesso per dare stabilita' al codice
      // Si prega di NON usare direttamente i bits del record ove sia disponibile la funzione

      BOOL Sarda(){return IstatRegione == IST_SARDEGNA;};
      BOOL Siciliana(){return IstatRegione == IST_SICILIA;};
      BOOL Continentale(){return IstatRegione < IST_SICILIA;}; // Anche le undefined !?
      BOOL Fs(){return StazioneFs !=0;};
      BOOL IsCumulativa(){return StazioneCumulativo != 0;};
      BOOL Nazionale(){return StazioneFs || StazioneCumulativo ;};
      BOOL Estera(){return estera ;};
      BOOL DestinazioneCumulativa(){return (BOOL)DestinazCumulativo;};
      BOOL TransitoMareTirrenia(){return (BOOL) StazioneTraMarCum;};
      BOOL Vendibile(){return (BOOL)vendibile;};
      BOOL Prenotabile(){return CodiceTPN != 0;};

      BOOL InitForCache(class R_STRU * Key, void * Cache);
      void RemoveFromCache( void * Cache){};
   };

   // Costruttore e distruttore
   STAZIONI(const STRINGA& Path=".\\",const STRINGA & NomeFile="ID_STAZI.DB",int PageSize=-1);
   virtual ~STAZIONI();

   // operatori standard
   R_STRU&  RecordCorrente(){ return *Corrente; };
   R_STRU&  operator [](ID Id){ Posiziona(Id); return *Corrente; };

   // Posizionamento per ID
   BOOL Posiziona(ID Id);
   BOOL Next();

   // Posizionamento per CCR (torna 0 se non riesce a posizionarsi)
   // Side effect: Il record corrente e' quello trovato
   ID   PosizionaPerCCR(DWORD CCR);

   // Decodifica CCR : Puo' modificare o meno il posizionamento del file
   ID   DecodificaCCR(DWORD CCR);

   int Dim(); // Ritorna il numero di stazioni definite

 // ......................................................
 // DecodificaIdStazione
 // ......................................................
 // Specifica della classe
   const char *  DecodificaIdStazione(WORD Id)   // NULL Terminated
      {if(!Posiziona(Id))return EMPTYSTR; return RecordCorrente().NomeStazione; };
   const char *  Id2Ident(WORD Id)   // NULL Terminated
      {if(!Posiziona(Id))return EMPTYSTR; return RecordCorrente().Nome7; };

   // Questo metodo mi ritorna la memoria totale utilizzata dalla classe
   ULONG DimBuffers();

   protected:

          R_STRU    * Corrente         ; // Puntatore al record corrente

   static FILE_STZ  * FStazioni        ; // File delle stazioni
   static FILE_CCR  * FCcr             ; // File dei codici CCR
   static int        CountOpened       ; // Registra il numero delle istanze: quando va a 0 chiude il file
   static HMTX       Mutex             ; // Mutex per sincronizzare accessi multithread;
   static BOOL       GetMutex()        ; // Ottiene il controllo delle risorse . return FALSE su errore
   static BOOL       FreeMutex()       ; // Rilascia il controllo delle risorse. return FALSE su errore

   friend class R_STRU;
   friend class CCR_STRU;

};

//----------------------------------------------------------------------------
// Questa classe permette l' accesso alle stazioni per nome
//----------------------------------------------------------------------------
// Simula la vecchia struttura (Nome + Id) ma in realta' contiene fisicamente il
// solo ID: gli altri dati sono presi da ID_STAZI.
// PageSize consigliato : 116
//----------------------------------------------------------------------------
struct _export STAZIONI_BYNAME : public FILE_KEY_TEXT  {
   public:
   // Costruttore
   STAZIONI_BYNAME(const STRINGA& Path,const STRINGA & NomeFile,ULONG PageSize):
    FILE_KEY_TEXT(Path+NomeFile,sizeof(ID),NstazDIM-1,PageSize,3,0), MieStazioni(Path){Init();};

   const char *  NomeStazioneCorrente(){return ChiaveCorrente();};  // NULL Terminated

   // EMS001 VA R_STRU --> BYN_R_STRU
   struct BYN_R_STRU {
      char  NomeStazione[NstazDIM];     // 35 caratteri : Descrittivo
      ULONG OrdStazione           ;     // Numero d' ordine della stazione nella lista
      WORD  IdStazione            ;     // Id Interno = Codice DKM
      BYTE  Tipologia             ;     // 1 = FS 2 = Cumulativo 4 = Internazionale (e le somme)
   } ;

   // EMS001 VA R_STRU --> BYN_R_STRU
   BYN_R_STRU&  RecordCorrente(){ return *(BYN_R_STRU*)ChiaveCorrente();};
   BYN_R_STRU&  operator [](ULONG Indice){ Posiziona(Indice); return RecordCorrente(); };

   virtual void KeySet();
   virtual void ExtraSet();

   protected :
   STAZIONI MieStazioni;
};
//----------------------------------------------------------------------------
// Stazioni per Nome (Solo FS)
//----------------------------------------------------------------------------
// PageSize consigliato : 2320
class _export STAZIONI_PER_NOME : public STAZIONI_BYNAME {
   public:
   STAZIONI_PER_NOME(const STRINGA& Path=".\\",const STRINGA & NomeFile="ID_STANO.DB",ULONG PageSize=2320):
    STAZIONI_BYNAME(Path,NomeFile,PageSize / 20){};
};
//----------------------------------------------------------------------------
// Stazioni per Nome (FS e Cumulativo)
//----------------------------------------------------------------------------
// PageSize consigliato : 2320
class _export STAZIONI_ALL_PER_NOME : public STAZIONI_BYNAME {
   public:
   STAZIONI_ALL_PER_NOME(const STRINGA& Path=".\\",const STRINGA & NomeFile="ID_STALL.DB",ULONG PageSize=2320):
    STAZIONI_BYNAME(Path,NomeFile,PageSize / 20){};
};
//----------------------------------------------------------------------------
// Stazioni per Nome (FS e Cumulativo & Estero )
//----------------------------------------------------------------------------
// PageSize consigliato : 2320
class _export STAZIONI_EST_PER_NOME : public STAZIONI_BYNAME {
   public:
   STAZIONI_EST_PER_NOME(const STRINGA& Path=".\\",const STRINGA & NomeFile="ID_STEST.DB",ULONG PageSize=2320):
    STAZIONI_BYNAME(Path,NomeFile,PageSize / 20){};
};

//----------------------------------------------------------------------------
// Classe di decodifica delle stazioni FS
//----------------------------------------------------------------------------
// CCR_ID: da codice CCR ad ID
// Comprende anche le stazioni del servizio cumulativo e le internazionali
//----------------------------------------------------------------------------
class _export CCR_ID : private STAZIONI{

  public:

  CCR_ID(const char * Path,const char * NomeFile="ID_STAZI.DB");
  ~CCR_ID();

  ID   operator[] (int CodiceCcr);  // Rende un ID interno di stazione dato il codice CCR
                                    // Torna 0 se al CCR NON corrisponde un ID

  int Id(int CodiceCcr);            // Eguale all' operatore: Ok anche per stazioni estere

  int CercaCCR(int CodiceCcr);      // rende un ID interno di stazione dato il codice CCR
                                    // Se la stazione non e' definita sul grafo (Estera)
                                    // ritorna -1
  static CCR_ID    * CcrId;         // Conversione da codice CCR ad Id
};

//----------------------------------------------------------------------------
// Stazioni Maiuscole per Nome : Per Internet
//----------------------------------------------------------------------------
class _export STAZIONI_MAIU_PER_NOME : STAZIONI_ALL_PER_NOME {

 // Costruttore
 STAZIONI_MAIU_PER_NOME(const STRINGA& Path=".\\"): STAZIONI_ALL_PER_NOME(Path){};

   const char *  NomeStazioneCorrente(){return ChiaveCorrente();};  // NULL Terminated
   virtual void KeySet(); // Mette in chiave la stazione "MAIUSCOLA"

};

// ---------------------------------------------------------------------
// Definizione della classe di accesso al file stazioni
// ---------------------------------------------------------------------
class  FILE_STZ : public FILE_FIX {    // "ID_STAZI.DB"
   public:
   FILE_STZ(const STRINGA& NomeFile,ULONG BufSize): FILE_FIX(NomeFile,sizeof(STAZIONI::R_STRU),BufSize){};
   STAZIONI::R_STRU&  RecordCorrente(){ return *(STAZIONI::R_STRU*) RecordC; };
};

//----------------------------------------------------------------------------
// Definizione della classe di accesso ai dati dei codici CCR
//----------------------------------------------------------------------------
class  FILE_CCR : public FILE_BS {  //    "ID_CCR.DB"
   public:

   // Costruttore
   FILE_CCR(const char* NomeFile,ULONG Size) : FILE_BS(NomeFile,sizeof(CCR_STRU),4,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(DWORD Ccr){ BUFR Wrk; Wrk.Store(Ccr); return Posiziona(Wrk) && KeyEsatta;};

   // Definizione operatori
   CCR_STRU &  RecordCorrente(){ return *(CCR_STRU*) RecordC; };
   CCR_STRU &  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(CCR_STRU*) RecordC; };

   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto ){ return CompareDoubleWord(Key1,Key2);};
};


#endif
