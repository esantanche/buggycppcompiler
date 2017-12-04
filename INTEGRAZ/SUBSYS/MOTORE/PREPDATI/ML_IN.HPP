//----------------------------------------------------------------------------
// ML_IN.HPP : Contiene le strutture di partenza per il caricamento del motore
//----------------------------------------------------------------------------
// Versione 1: dati alimentati a partire dai dati ".T"
// Questi dati potrebbero essere caricati anche a partire da un DB, e sono il
// punto di partenza per il caricamento dei dati del motore
//----------------------------------------------------------------------------
// Nota: Tutti i files che hanno come chiave il mezzo virtuale
// sono del tipo BS in quanto vi possono essere dei buchi nella
// numerazione dei mezzi virtuali dovuti allo split per periodicita'
// ed ai mezzi virtuali scartati.
//----------------------------------------------------------------------------

#ifndef  HO__ML_IN
#define  HO__ML_IN

#ifndef  HO__ML_BASE
#include "ML_BASE.HPP"
#endif

//#ifndef HO_MM_DETTA_HPP
//#include "MM_DETTA.HPP"
//#endif
//
//#include "MM_CRIT2.HPP"

//----------------------------------------------------------------------------
// Tabella MEZZI_VIAGG
//----------------------------------------------------------------------------
struct MEZZO_VIAGG      {
   BIT           NumeroTreno                   :17 ;     // Posizione progressiva in TABORARI+1. E' la chiave di accesso al file
   BIT           TipoMezzo                     : 6 ;     // Vedi enum TIPO_MEZZO
   IDTRENO       IdTreno                           ;     // Identificativo treno ( a 10 caratteri ) : Univoco
   char          KeyTreno                      [5] ;     // Identificativo treno ( a 5  caratteri ) : Univoco per i prenotabili
   char          NomeMezzoViaggiante          [20] ;     // Opzionale
   MM_INFO       Servizi                           ;     // Servizi aggregati del treno & Tipo mezzo codificato
   MM_INFO       ServiziUniformi                   ;     // Servizi UNIFORMI del treno & Tipo mezzo codificato
};

// Uso un file FIX e non un file BS perche' la chiave e' semplicemente eguale al numero record + 1
class F_MEZZO_VIAGG    : public FILE_FIX {  // "MZVIAG.TMP"
   public:
   // Costruttore
   F_MEZZO_VIAGG(const char* NomeFile,ULONG Size=4096) : FILE_FIX(NomeFile,sizeof(MEZZO_VIAGG),Size){};
   // Definizione operatori
   MEZZO_VIAGG&  operator [](int Rec){ Posiziona(Rec); return *(MEZZO_VIAGG*) RecordC; };
   MEZZO_VIAGG&  RecordCorrente(){ return *(MEZZO_VIAGG*) RecordC; };
};

//----------------------------------------------------------------------------
// Tabella treni VIRTUALI
//----------------------------------------------------------------------------
struct MEZZO_VIRTUALE {

   WORD  MezzoVirtuale               ; // KEY: Mezzo virtuale

   // Dati riassuntivi a livello di treno virtuale, ottenuti dalla combinazione dei dati
   // dei mezzi viaggianti:
   BIT   TipoMezzoDominante      : 6 ; // Tipo predominante (dopo le ultime modifiche: significativo solo se ho un unico treno)
   BIT   GiornoSuccessivo        : 2 ; // Giorni tra 1ø partenza del MV ed arrivo finale
   BIT   PeriodicitaDisuniformi  : 1 ; // Indica che le periodicita' sono esplose a livello di fermata
   BIT   DaCarrozzeDirette       : 1 ; // Indica che il MV e' stato formato a partire dalle carrozze dirette
   BIT   DaAltriServizi          : 1 ; // Se TRUE vuol dire che esistono servizi diretti che implicano la prosecuzione
   BIT   DaOrigineDestino        : 1 ; // Se TRUE vuol dire che uno dei treni continua nell' altro
   BIT   FittizioLinearizzazione : 1 ; // Indica che il MV e' fittizio: serve solo per linearizzare le fermate di un' altro MV
   BIT   CambiDeboli             : 3 ; // Numero di cambi deboli (vedi FT_STEPA.CPP).
   BIT   InLoop                  : 1 ; // Indica che vi sono treni presenti piu' volte
   BIT   NumeroFermateTransiti   : 10; // Comprende anche i transiti, ma non le fermate scartate
                                       // NB: Dopo la linearizzazione conta UNA SOLA VOLTA le fermate duplicate
   BIT   NumeroFermateValide     : 10; // NON comprende anche i transiti
                                       // NB: Dopo la linearizzazione conta UNA SOLA VOLTA le fermate duplicate
   T_PERIODICITA PeriodicitaMV       ; // = OR di tutte le periodicita' componenti
   MM_INFO ServiziUniformi           ; // Servizi uniformi del mezzo virtuale

   // Mezzi viaggianti componenti
   BYTE  NumMezziComponenti          ;
   struct MEZZO_VIAGGIANTE {
      BIT NumeroMezzo            :17 ;
      BIT HaNome                 : 1 ; // Vero se il mezzo viaggiante ha un nome
      BIT CcrStazioneDiCambio    :24 ; // CCR stazione di cambio (0 per l' ultimo treno):
                                       // Puo' anche non corrispondere ad alcuna stazione !
                                       // (cambio su stazioni del merci)
      BIT ShiftPartenza          : 2 ; // Indica quanti giorni dopo la partenza del mezzo virtuale ( = del primo mezzo componente) si parte
                                       // NB:: Il treno potrebbe anche essere partito il giorno prima.
      BIT ShiftDaPTreno          : 2 ; // Indica quanti giorni dopo la partenza del TRENO si PARTE dalla stazione di cambio
      BIT TipoMezzo              : 6 ; // Vedi enum TIPO_MEZZO
      IDTRENO IdentTreno             ;
      char KeyTreno[5]               ;
   } Mv[MAX_MEZZI_REALI]             ;
   // Identifica un treno dalla stazione di cambio.
   int TrDaCambio(int Ccr){ for(int i = NumMezziComponenti -2; i > 0; i--) if(Ccr == Mv[i].CcrStazioneDiCambio)return i; return 0;};
//<<< struct MEZZO_VIRTUALE {
};

class F_MEZZO_VIRTUALE : public FILE_BS {  // "M0_TRENV.TM0 TM1 TMP"

   public:

   // Costruttore
   F_MEZZO_VIRTUALE(const char* NomeFile,ULONG Size=4096) : FILE_BS(NomeFile,sizeof(MEZZO_VIRTUALE),2,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Mezzo_Virtuale){ return Posiziona(BUFR(&Mezzo_Virtuale,sizeof(Mezzo_Virtuale))) && KeyEsatta;};

   // Definizione operatori
   MEZZO_VIRTUALE&  RecordCorrente(){ return *(MEZZO_VIRTUALE*) RecordC; };
   MEZZO_VIRTUALE&  FixRec(DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(MEZZO_VIRTUALE*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return CompareWord(Key1,Key2);};

};

//----------------------------------------------------------------------------
// Tabella fermate treni virtuali
//----------------------------------------------------------------------------
// Tutti i dati sono validi nell' ambito del treno virtuale
// (es: ProgKm non si riazzera al cambio di treno)
struct FERMATE_VIRT {
   WORD MezzoVirtuale        ; // KEY: Mezzo virtuale
   WORD Progressivo          ; // KEY: Progressivo fermata (in ambito treno virtuale, Parte da 1)
                               // NB: NON viene riordinato dopo la soppressione delle fermate ad opera di ML_STEP1
                               // Perche' e' anche foreign key per le periodicita', le note ed i servizi
   WORD Progressivo2         ; // Come sopra ma escludendo i transiti su stazioni non nodali ne di cambio
                               // Viene riordinato dopo aver individuato le stazioni di cambio
   BIT  CCR               :24; // CCR stazione fermata (come da file .T )
                               // Comprensivo del codice Rete se la stazione non e' italiana
   BIT  Id                :13; // Id stazione fermata (Cod DKM/Sipax)
   BIT  TrenoFisico       : 3; // Treno fisico di appartenenza (indice )
   BIT  OraArrivo         :11; // Ora di arrivo   (minuti dalla mezzanotte)
   BIT  OraPartenza       :11; // Ora di partenza (minuti dalla mezzanotte)
   BIT  GiornoSuccessivoArrivo  : 2; // Se arriva alla fermata il giorno successivo alla partenza del treno
   BIT  GiornoSuccessivoPartenza: 2; // Se parte dalla fermata il giorno successivo alla partenza del treno
   BIT  ProgKm            :12; // Progressivo Km
   BIT  Transito          : 1; // Vero se il treno transita solamente (O fermate di servizio)
   BIT  FermataFacoltativa: 1; // Vero se ???
   // Nota: a differenza dei files T i bit di partenza ed arrivo sono sempre valorizzati.
   BIT  FermataPartenza   : 1; // Vero se la fermata permette di partire
   BIT  FermataArrivo     : 1; // Vero se la fermata permette di arrivare
   BIT  FermataDiServizio : 1; // Vero se la fermata e' esclusivamente di servizio (in tal caso forzo Transito a TRUE)
   BIT  HaNote            : 1; // Vero se la fermata ha delle note
   BIT  HaNoteLocalita    : 1; // Vero se la STAZIONE di fermata ha delle note
};

class F_FERMATE_VIRT : public FILE_BS {//    "M0_FERMV.TM0 TM1 TMP"

   public:

   // Costruttore
   F_FERMATE_VIRT(const char* NomeFile,ULONG Size=64000) : FILE_BS(NomeFile,sizeof(FERMATE_VIRT),4,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(WORD Mezzo_Virtuale){ return Posiziona(BUFR(&Mezzo_Virtuale,sizeof(Mezzo_Virtuale))) && KeyEsatta;};
   BOOL Seek(WORD Mezzo_Virtuale,WORD Prog){ BUFR Buf; Buf.Store(Mezzo_Virtuale);Buf.Store(Prog); return Posiziona(Buf) && KeyEsatta;};

   // Questo metodo torna il record dato il mezzo virtuale ed il codice CCR
   // Se non trova il CCR torna *NULL
   // NB: Il CCR non viene trovato anche se presente nei dati originali dei treni
   //     qualora non risulti nell' anagrafica stazioni.
   //     Per gestire questi casi si deve accedere direttamente a tali dati
   FERMATE_VIRT & CercaCcrDiMv(WORD Mezzo_Virtuale, DWORD Ccr){
      if(!Seek(Mezzo_Virtuale))return *(FERMATE_VIRT *)NULL;
      DWORD CcrCorretto = Ccr;
      if(CcrCorretto >= 8300000 && CcrCorretto <= 8399999) CcrCorretto -= 8300000;
      for(;&RecordCorrente() && RecordCorrente().MezzoVirtuale == Mezzo_Virtuale; Next())
         if(RecordCorrente().CCR == CcrCorretto){
            return RecordCorrente();
         };
      return *(FERMATE_VIRT *)NULL;
   };

   // Definizione operatori
   FERMATE_VIRT&  RecordCorrente(){ return *(FERMATE_VIRT*) RecordC; };
   FERMATE_VIRT&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(FERMATE_VIRT*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return Compare2Word_D(Key1,Key2,DimConfronto);};

};

// Questa tabella contiene le periodicita' di TUTTE le fermate dei mezzi virtuali
// che non hanno periodicita' uniforme
// Nota: la periodicita' DI FERMATA (SE PRESENTE) e' comunque riferita
// alla data di fermata e non alla data di partenza del treno
struct PERIODICITA_FERMATA_VIRT {
   WORD MezzoVirtuale        ; // KEY: Mezzo virtuale
   WORD Progressivo          ; // KEY: Progressivo fermata (in ambito treno virtuale)
   T_PERIODICITA Periodicita ; // Periodicita' di fermata
};

class F_PERIODICITA_FERMATA_VIRT : public FILE_BS {   //    "M1_FERMV.TM0 TM1 TMP"

   public:

   // Costruttore
   F_PERIODICITA_FERMATA_VIRT(const char* NomeFile,ULONG Size=6400) : FILE_BS(NomeFile,sizeof(PERIODICITA_FERMATA_VIRT),4,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(WORD Mezzo_Virtuale){ return Posiziona(BUFR(&Mezzo_Virtuale,sizeof(Mezzo_Virtuale))) && KeyEsatta;};
   BOOL Seek(WORD Mezzo_Virtuale,WORD Prog){ BUFR Buf; Buf.Store(Mezzo_Virtuale);Buf.Store(Prog); return Posiziona(Buf) && KeyEsatta;};

   // Definizione operatori
   PERIODICITA_FERMATA_VIRT&  RecordCorrente(){ return *(PERIODICITA_FERMATA_VIRT*) RecordC; };
   PERIODICITA_FERMATA_VIRT&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(PERIODICITA_FERMATA_VIRT*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return Compare2Word_D(Key1,Key2,DimConfronto);};

};

//----------------------------------------------------------------------------
// INFO_AUX
//----------------------------------------------------------------------------
// Questa classe serve per contenere delle informazioni ausiliarie di mezzo virtuale
// Tali informazioni possono essere di varia natura: ad esempio Note o Servizi
// o classifica o nome.
// Ho preferito tale approccio "generico" perche' pi— facilmente estendibile.
// Ognuna di tali informazioni e' individuata genericamente da un codice, essendovi poi
// un "tipo" per distinguere le varie tipologie di informazione
// I dati sono SEMPRE organizzati per mezzo virtuale, distinguendo poi
// tra dati semplici (= senza periodicit…, relativi ad un mezzo viaggiante (o piu')
// o ad una singola fermata ) e dati complessi.
// Poiche i dati sono caricati per treno e poi aggregati, si possono avere situazioni
// in cui dati semplici sono poi aggregati a formare dati complessi.
// I dati sono comunque sempre rappresentabili come dati complessi.
//----------------------------------------------------------------------------
// I dati per poter essere aggregati debbono avere periodicit… comparabili e
// lo stesso valore di Extra
// Le periodicit… fanno sempre riferimento alla data di partenza DEL MEZZO VIRTUALE
// (sono gi… shiftate se necessario per tener conto delle differenti date di partenza
// dei treni)
//----------------------------------------------------------------------------
enum TIPO_INFORMAZIONE_AUX {
   AUX_NOME        = 1,
   AUX_CLASSIFICA     ,
   AUX_NOTA           ,
   AUX_SERVIZIO       ,
   AUX_TAPPO
};
struct INFO_AUX {
   WORD     MezzoVirtuale        ; // KEY
   WORD     Tipo                 ; // KEY : vedi TIPO_INFORMAZIONE_AUX
   WORD     Id                   ; // Identificatore
                                   // Per le note: Id nota
                                   // Per i servizi: Id numerico che corrisponde al servizio
                                   // Per i nomi dei treni: non utilizzato
                                   // Per la classifica treno: codice classifica
   DWORD    Info                 ; // Altre informazioni
                                   // Per le note: indica se e' una nota commerciale
                                   // Per i servizi: non utilizzato
                                   // Per i nomi dei treni: indica l' ID del treno da cui ottenere il nome (MZVIAG.TMP)
                                   // Per la classifica treno: non utilizzato
   T_PERIODICITA Periodicita     ; // Periodicita' (riferita alla data di partenza del mezzo virtuale)
   signed char   Shift           ; // Indica di quanti giorni Š stata shiftata la periodicit… per riportarla alla data di partenza del mezzo virtuale
   // Informazioni rappresentabili in modo pi— semplice:
   BIT   DiFermata :1            ; // Puo' essere dato di fermata : In tal caso ignorare la periodicit…
   BIT   DiMvg     :1            ; // Puo' essere dato di treno / treni
   BIT   Mvg       :MVG          ; // Indica quali mezzi viaggianti considerare
   // Per la rappresentazione completa (sempre valida)
   BIT      TipoAUX               : 3; // 0 = Per tutto il mezzo viaggiante o virtuale, a seconda dei casi.
                                       //     Oppure per una serie di mezzi viaggianti
                                       // 1 = Si effettua alla fermata indicata
                                       // 2 = Soppresso alla fermata indicata
                                       // 3 = Da stazione (non oltre MaxDa) a stazione e tutte le intermedie
                                       // 4 = Da stazione a Stazione ma non le intermedie
   BIT      Da                    :24; // Per TipoAUX = 1 / 2 e' la fermata di effettuazione / soppressione
   BIT      A                     :24; // Solo per TipoAUX = 3,4
   BIT      MaxDa                 :24; // Solo per TipoAUX = 3  : Ultima stazione in salita da cui si puo' usufruire del servizio
   // Questa funzione mi dice se e' uniforme.
   BOOL Uniforme(){ return TipoAUX == 0 && Periodicita == T_PERIODICITA::InOrario ;};
   void Trace(const char * Msg);
   STRINGA ToStringa();
};
class F_INFO_AUX : public FILE_BS {  // "INFO_AUX.TM0 TM1 TMP"

   public:

   // Costruttore
   F_INFO_AUX(const char* NomeFile,ULONG Size=4096) : FILE_BS(NomeFile,sizeof(INFO_AUX),4,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Mezzo_Virtuale){ BUFR Wrk; Wrk.Store(Mezzo_Virtuale); return Posiziona(Wrk) && KeyEsatta;};
   BOOL Seek(WORD Mezzo_Virtuale, WORD Tipologia){ BUFR Wrk; Wrk.Store(Mezzo_Virtuale); Wrk.Store(Tipologia); return Posiziona(Wrk) && KeyEsatta;};

   // Definizione operatori
   INFO_AUX&  RecordCorrente(){ return *(INFO_AUX*) RecordC; };
   INFO_AUX&  FixRec(DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(INFO_AUX*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return Compare2Word_D(Key1,Key2,DimConfronto);};

};

//----------------------------------------------------------------------------
// Testi delle note
//----------------------------------------------------------------------------
class  FILE_DATI_NOTE : public FILE_FIX {    // "NOTEPART.TMP"
   public:

   // EMS001 VA R_STRU --> FDN_R_STRU
   struct FDN_R_STRU { // Dati delle note
      WORD IdNota                     ; // Id della nota
      BIT  Progressivo             : 4;
      BIT  Linguaggio              : 4; // Any = 0, Francese = 1, Inglese = 2, Italiano = 3, Spagnolo = 4, Tedesco = 5
      T_PERIODICITA Periodicita       ; // Eguale su tutti i records con stesso ID
      char Testo[71]                  ; // 70 caratteri e null terminator

   };
   FILE_DATI_NOTE(const STRINGA& NomeFile,ULONG BufSize=4096): FILE_FIX(NomeFile,sizeof(FDN_R_STRU),BufSize){};

   // Definizione operatori
   FDN_R_STRU&  operator [](WORD Nota){ Posiziona(Nota); return *(FDN_R_STRU*) RecordC; };
   FDN_R_STRU&  RecordCorrente(){ return *(FDN_R_STRU*) RecordC; };
};


//----------------------------------------------------------------------------
// Pricing O/D
//----------------------------------------------------------------------------
// Work  in progress

// ==========================================================
// MULTISTAZIONE : Caricata da un' apposita classe in FT_LIB
// ==========================================================

struct _export MULTI_STA {
   ARRAY_ID   Stazioni     ; // Stazioni
   COLLEG_URBANO * Colleg  ; // Matrice quadrata dei collegamenti tra stazioni
   MULTI_STA() {Colleg = NULL;};
   ~MULTI_STA() { if(Colleg)delete Colleg;};
   void Trace(STRINGA & Msg,int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
   COLLEG_URBANO & Collegamento(WORD Idx1, WORD Idx2){return  Colleg[Idx1*Stazioni.Dim() + Idx2];};
};

// Elenco dinamico di MULTI_STA
typedef MULTI_STA * P_MULTI_STA;
#define DIN_ARRAY EL_MULTI_STA
#define CLAS P_MULTI_STA
#define DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
#define CopiaMembro(_CLAS) (Beep(__FILE__,__LINE__,1),(P_MULTI_STA)NULL) // Vieto la copia
#include "dinarray.h"

struct _export GEST_MULTI_STA {
   ARRAY_ID       StazioniMS   ; // Elenco delle multistazioni
   ELENCO_S       Acronimi     ; // Acronimi delle stazioni
   ARRAY_ID       StazioniCoincidenzaEstesa; // Stazioni con tempo di coincidenza esteso
   EL_MULTI_STA   ClustersMultiStazione; // Clusters
   GEST_MULTI_STA(const STRINGA& Path,FILE_RW * Fout, class F_STAZIONE_MV & File = *(F_STAZIONE_MV*)NULL);
   // Carica i dati delle multistazioni
   // Richiede che il grafo sia GIA' caricato ! ed anche CCR_ID
   void Trace(const STRINGA & Msg,int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   // In questa array carica le stazioni appartenenti alla stessa citta'. Max  63 citta'
   ARRAY_ID  Citta[63];
   int     NumCitta;
};

//----------------------------------------------------------------------------
#endif
