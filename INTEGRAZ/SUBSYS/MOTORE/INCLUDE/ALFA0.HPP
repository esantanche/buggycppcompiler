//========================================================================
// ALFA : Risolutore Viaggi --> Soluzione modello di trasporto
//========================================================================
// Attenzione: Cambiando i formati dei files dati del motore in modo da
// renderlo incompatibile, modificare la #define ORARIO_FORMATO_ATTUALE  
// Vedere la classe VALIDITA_ORARIO
//========================================================================
#define ORARIO_FORMATO_ATTUALE  101  // Version 1 Modification 01
//
#ifndef HO_ALFA_HPP
#define HO_ALFA_HPP

#ifndef HO_MM_VARIE_HPP
#include "mm_varie.hpp"
#endif

#ifndef HO_MM_GRAFO_HPP
#include "mm_grafo.hpp"
#endif

#ifndef HO_MM_BASIC_HPP
#include "MM_BASIC.HPP"
#endif

//----------------------------------------------------------------------------
// Lingua correntemente in uso
//----------------------------------------------------------------------------
// Questo e' un metodo interno al motore: ALL' esterno del motore utilizzare l' impostazione
// che si trove su RETE_FS
BYTE __fastcall LinguaInUso(); // Torna i valori dell' enum LINGUA

//----------------------------------------------------------------------------
// Definizione comoda per indicare l' orario corrente
//----------------------------------------------------------------------------
#define Orario  (*DATI_ORARIO_FS::Corrente)   // Di tipo DATI_ORARIO_FS & 

//----------------------------------------------------------------------------
// Indicazione se opero in modalita' MonoThread o MultiThread
//----------------------------------------------------------------------------
extern BOOL MultiThread;

//----------------------------------------------------------------------------
// Collider (Usato normalmente per ID delle stazioni)
//----------------------------------------------------------------------------
extern COLLIDER<WORD,WORD> Collider;


//----------------------------------------------------------------------------
// FUNZIONI INLINE
//----------------------------------------------------------------------------
inline ID Interno(ID Esterno);
inline ID Esterno(ID Interno);

//----------------------------------------------------------------------------
// PERIODICITA : Struttura per la gestione della periodicita' (su base giorno)
//----------------------------------------------------------------------------
// Ogni giorno e' rappresentato da un bit
// Questa classe contiene i metodi interni utilizzati solo dal MOTORE
// Nota:           >>3 == /8           &0x7 == %8   (per velocizzare)
//----------------------------------------------------------------------------
class _export PERIODICITA : public MM_PERIODICITA {     
   
   public:
   
   void Set();
   void Set(WORD OffsetInizioOrario){ 
      // TRACELONG("Set: Offset = ",OffsetInizioOrario);
      Dati[(OffsetInizioOrario >> 3)] |= 1 << (OffsetInizioOrario & 0x7);};
   void Set(SDATA InizioPeriodo , SDATA FinePeriodo);    // Intervallo CHIUSO


   void ReSet();
   void ReSet(WORD OffsetInizioOrario){
      // TRACELONG("ReSet: Offset = ",OffsetInizioOrario);
      Dati[(OffsetInizioOrario >> 3)] &= ~ (1 << (OffsetInizioOrario & 0x7));};
   void ReSet(SDATA InizioPeriodo , SDATA FinePeriodo);  // Intervallo CHIUSO


   BOOL Circola (WORD OffsetInizioOrario)const { // metodo interno
      return Bool(Dati [OffsetInizioOrario>>3] & ( ((unsigned char) 1) << (OffsetInizioOrario & 0x7))) ;
   };

   // Torna un intero con la circolazione per i 4 giorni a partire dal giorno corrente.
   // Il parametro ParteGiornoPrecedente shifta di un giorno per compensare le fermate
   // effettuate il giorno successivo alla partenza del treno
   BYTE __fastcall Circola4(BYTE ParteGiornoPrecedente );

   // Operatori logici su periodo

   // Negazione Logica
   void operator ~();
   
   void operator &=(PERIODICITA & From);
   void operator |=(PERIODICITA & From);
   BOOL operator ==(const PERIODICITA & From)const ;
   BOOL operator !=(const PERIODICITA & From)const {return ! operator ==(From);};
   BOOL Empty();

   // Questo metodo torna una serie di righe presentabili con la periodicita'.
   // Giorni della settimana nella forma: LMmGVSD
   // Con testata: Include due linee di testata
   // Doublespace: 2 caratteri per giorno 
   // Points: puntini per facilitare la lettura. Di solito inutile se DoubleSpace = TRUE
   ELENCO_S Decod(BOOL ConTestata=TRUE,BOOL DoubleSpace=FALSE,BOOL Points=TRUE);

   // E questa mi ritorna una stringa con i dati di periodicita' in esadecimale
   STRINGA Hex();

   // Caricati da DATI_ORARIO_FS all' inizio dell' algoritmo (periodo = giorno partenza)
   // mediante il metodo ImpostaProblema();
   static void ImpostaProblema(SDATA & InizioDatiCaricati, SDATA & InizioOrarioCorrente, SDATA & FineOrarioCorrente, SDATA & DataCorrente);

   // Questi sono i dati relativi a giorno ed orario corrente
   static SDATA  InizioOrario          ; // Inizio Dati caricati
   static SDATA  InizioOrarioFS        ; // Inizio validita' orario
   static SDATA  FineOrarioFS          ; // Fine   validita' orario
   static BYTE   GiornoInizioOrario    ; // Lunedi' = 0, Martedi = 1 .... E' il giorno di inizio dei dati caricati
   static SDATA  DataC                 ; // Data Corrente ( = Data della richiesta)
   static WORD   offsetPeriod          ; // Offset della data corrente (giorni da inizio dati caircati)

   void  Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   BYTE Sett();     // Informazione riassuntiva per giorni della settimana


   // Questo effettua uno shift generale di un giorno indietro
   void GiornoPrecedente();
   // o avanti
   void GiornoSeguente();

};

//----------------------------------------------------------------------------
// Questa classe serve per la gestione dei gruppi.
//----------------------------------------------------------------------------
// Equivale ad insiemi di max 40 elementi
#define MAXGRUPPO 40    // Numero massimo di bit di un gruppo
class GRUPPO { 
   public:
   operator STRINGA() const;
   GRUPPO operator |= (GRUPPO Dato){Bytes_0_3 |= Dato.Bytes_0_3; Byte_4 |= Dato.Byte_4; return THIS ; };
   BOOL   operator == (GRUPPO Dato){return (Bytes_0_3 == Dato.Bytes_0_3  &&  Byte_4 == Dato.Byte_4);};
   BOOL   operator != (GRUPPO Dato){return (Bytes_0_3 != Dato.Bytes_0_3  ||  Byte_4 != Dato.Byte_4);};
   BOOL   operator &  (GRUPPO Dato){return (Bytes_0_3 &  Dato.Bytes_0_3) || (Byte_4 &  Dato.Byte_4);};
   GRUPPO And(GRUPPO Dato){ GRUPPO Out; 
      Out.Bytes_0_3 =  Bytes_0_3 &  Dato.Bytes_0_3; Out.Byte_4 =  Byte_4 &  Dato.Byte_4; return Out; };
   void Clear(){Bytes_0_3=0;Byte_4=0;};

   // Nota:         & 0x7 == %8   (per velocizzare)
   void Set(BYTE BitNum){if(BitNum < 32){Bytes_0_3 |= 1 << BitNum;}else{Byte_4 |= 1 << (BitNum & 0x7 );};};
   
   private:
   DWORD Bytes_0_3;
   BYTE  Byte_4;
};

//----------------------------------------------------------------------------
// COSTO_SOL
//----------------------------------------------------------------------------
// Costo di una soluzione: utilizzata per i criteri di confronto
struct COSTO_SOL {
   WORD  MinutiEquivalenti ; // Minuti equivalenti della soluzione (= Tempo di percorrenza corretto)
   WORD  IndiceConnessione ; // Indice di connessione della soluzione: e' la sommatoria dell' inverso
                             // del numero di COLLEGAMENTI (= collegamenti diretti verso altre stazioni 
                             // di cambio) posseduto dalle stazioni di cambio intermedie della soluzione.
                             // Per motivi di calcolo e' moltiplicato per 32000.

   COSTO_SOL(){ MinutiEquivalenti = IndiceConnessione = 0; };
   BOOL operator == (COSTO_SOL & B){ return MinutiEquivalenti == B.MinutiEquivalenti && IndiceConnessione == B.IndiceConnessione ; };
   BOOL operator != (COSTO_SOL & B){ return MinutiEquivalenti != B.MinutiEquivalenti || IndiceConnessione != B.IndiceConnessione ; };

   // Questo operatore non e' ben posto (non vale la regola triangolare) ma dovrebbe essere sufficiente
   BOOL operator < (COSTO_SOL & B){
      int DeltaMinuti = int(MinutiEquivalenti) - int(B.MinutiEquivalenti) ;
      if( DeltaMinuti == 0 ){             // Decido solo  in base alla connettivita' 
         return IndiceConnessione <= B.IndiceConnessione ;
      } else if( abs(DeltaMinuti) < 7 ) { // Decido anche in base alla connettivita' 
         // 333 e' un numeretto magico da aggiustare
         return (DeltaMinuti + ((int(IndiceConnessione) - int(B.IndiceConnessione) ) / 333 )) <= 0;
      }
      return DeltaMinuti <= 0;
   };
   BOOL operator > (COSTO_SOL & B){ return !(THIS < B);};
   void operator  = (WORD B){ MinutiEquivalenti  = B; IndiceConnessione = 0; };
   void operator += (WORD B){ MinutiEquivalenti += B; };
   void operator -= (WORD B){ MinutiEquivalenti -= B; };
   void Set(WORD IndCon){ IndiceConnessione = IndCon; };

   operator STRINGA() const { return STRINGA(MinutiEquivalenti) +" (IdxConn="+STRINGA(IndiceConnessione)+")";};
};


//----------------------------------------------------------------------------
// NOD && ARRAY_NOD
//----------------------------------------------------------------------------
// Questa struttura serve per l' identificazione dei nodi
struct NOD {
   #undef TRCRTN
   #define TRCRTN "NOD"
   BYTE Progressivo;
   BYTE Ferma;
   ID   Id;
};
// Funzione per sortare le array di NOD
int SortPiu( const void *a, const void *b);
typedef ARRAY_DINAMICA<NOD> ARRAY_NOD ;

//----------------------------------------------------------------------------
// SOLUZIONE
//----------------------------------------------------------------------------
// Attenzione: Quando leggo i dati dalla cache non genero SOLUZIONE ma MM_SOLUZIONE
//----------------------------------------------------------------------------
struct SOLUZIONE : public MM_SOLUZIONE { // Soluzione di viaggio     
   
   BYTE   Valida        ;   // Se != 0 la soluzione e' valida, altrimenti deve essere scartata
   BYTE   Buona         ;   // False se ha qualche problema
   WORD   Km            ;   // Lunghezza (Ricavata dai dati dei treni, Disponibile PRIMA di valorizzare Percorso)
   COSTO_SOL Costo      ;   // Funzione Costo: E' essenzialmente una combinazione lineare dei parametri della soluzione
                            // e serve per determinare se una data soluzione sia preferibile ad un' altra
                            // E' espressa in minuti, e considera anche le penalizzazioni
                            // Opera solo come preferenza in aggiunta
   COSTO_SOL CostoAssoluto; // Idem: questa e' una preferenza in sostituzione
   ID     IdTraghetto   ;   // Id traghetto utilizzato o 0 se la soluzione non usa traghetti
   BYTE   Percorso      ;   // Punta al percorso: indice entro PercorsiGrafo del GRAFO
                            // E' calcolato SOLO alla fine dell' elaborazione, dopo l' arresto del ciclo di ricerca
   BYTE   InAttenzione  ;   // Utilizzata per dei trace
   BYTE   IsTargetAutoChk ; // Utilizzata per autodiagnostica

   SOLUZIONE();

   void * operator new(size_t Size){return Allocator.Get();};
   void operator delete(void * Item){Allocator.Free((SOLUZIONE*)Item);};

   static ALLOCATOR<SOLUZIONE> Allocator;

   void __fastcall CalcolaCosto();
   
   BOOL OkSost(SOLUZIONE & Sol2);    // Controlla se la soluzione possa sostituire Sol2

   void GetNote();

   PERIODICITA & Period(){ return (PERIODICITA &)Per;};

   void Semplifica(); // Vede se un sottoinsieme di treni puo' comunque portare a destinazione.

   STRINGA IdSol(); // Ritorna una stringa compatta di identificazione della soluzione 

   // Questo metodo identifica le stazioni lungo il percorso.
   // Torna FALSE in caso di errore
   BOOL IdentificaNodiDiTransito(ARRAY_ID & NodiDiTransito, int MinTratta = 0, int MaxTratta = 9999);

};

// Questo classificatore e' utilizzato dal motore per identificare le tratto con cattivo utilizzo 
// dei mezzi di qualita' 
int  __fastcall CattivoUsoMezzoDiQualita(MM_SOLUZIONE::TRATTA_TRENO & Tratta);

//----------------------------------------------------------------------------
// SOL_IMAGE
//----------------------------------------------------------------------------
// Questa struttura e' per autodiagnostica: rappresenta i dati fondamentali di
// una soluzione.
// Dati contenuti: Treni Utilizzati ( Urbani = Urb )
//----------------------------------------------------------------------------
typedef char TREN_TRATTA[6] ;
struct SOL_IMAGE {
   TREN_TRATTA Treni[MAX_TRATTE];
   int NumTratte  ;
   int NumSostituzioni; 
   void Imposta(const STRINGA & ElencoTreni); // Usare "Urb" per i collegamenti urbani
   void Imposta(SOLUZIONE & Sol);
   int  Eqv(SOLUZIONE & Sol); // 1 = Si, 0 = No, 2 = Alternativa (stesso path e 1ø treno)
   BOOL TrenoInAttenzione(char * IdTreno); // Vero se e' uno dei treni della soluzione in attenzione
};

//----------------------------------------------------------------------------
// ELENCO_SOLUZIONI
//----------------------------------------------------------------------------
// Elenco dinamico di SOLUZIONI                          
typedef SOLUZIONE * P_SO;
#define DIN_ARRAY ELENCO_SOLUZIONI_INT
#define CLAS P_SO
#include "dinarray.h"

struct ELENCO_SOLUZIONI : public ELENCO_SOLUZIONI_INT {
   ELENCO_SOLUZIONI():ELENCO_SOLUZIONI_INT(0){};
   ~ELENCO_SOLUZIONI();

   void SpostaSoluzione(const ELENCO_SOLUZIONI& From, int Idx); // Sposta una soluzione (concatena)
   void Trace(const STRINGA& Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   // Consolidamento delle soluzioni:
   // Le soluzioni valide trovate vengono aggiunte a Orario.Soluzioni.
   // Per ogni soluzione si identifica il percorso (grafo) di appartenenza
   // Torna FALSE se in errore
   BOOL Consolida();

   // Abbattimento delle soluzioni:
   // Le soluzioni valide trovate vengono confrontate con Orario.Soluzioni per
   // identificare eventuali conflitti (es: cambi inutili).
   // Torna FALSE se in errore
   BOOL Semplifica();

   void Clear(); // Cancella soluzioni non piu' utilizzate

};

//----------------------------------------------------------------------------
// VALIDITA_ORARIO
//----------------------------------------------------------------------------
// File validita' orario : Su VALIDITA.DB (FILE_RW) un solo record di tipo VALIDITA_ORARIO
// Poi una struttura DATI_ORARIO_FS::HEADER
struct _export VALIDITA_ORARIO {                                         
   DWORD FormatoDeiDati    ; // Se non corrisponde non posso usare i dati
   SDATA Inizio            ; // Limite dati contenuti
   SDATA Fine              ; // Limite dati contenuti 
   SDATA InizioUfficiale   ;
   SDATA FineUfficiale     ;
   SDATA InizioOraLegale   ; // All 0 se non ha ora legale
   SDATA FineOraLegale     ; // All 0 se non ha ora legale
   WORD  OraInizioOraLegale; // 0 se non ha ora legale
   WORD  OraFineOraLegale  ; // 0 se non ha ora legale
   int   ProgressivoFornitura;
   char  NomeOrario[80]    ;
   // Queste sono le periodicit… particolari da mostrare all' operatore
   PERIODICITA  CircolaSempre                            ;  // Codice NON_PERIODICO            
   PERIODICITA  CircolaNeiLavorativi                     ;  // Codice GIORNI_FERIALI           
   PERIODICITA  CircolaNeiFestivi                        ;  // Codice GIORNI_FESTIVI           
   PERIODICITA  CircolaNeiLavorativiEsclusoIlSabato      ;  // Codice LAVORATIVI_ESCLUSO_SABATO
   PERIODICITA  CircolaTuttiIGiorniEsclusoIlSabato       ;  // Codice ESCLUSO_SABATO           
   PERIODICITA  CircolaIlSabatoENeiFestivi               ;  // Codice SABATO_E_FESTIVI         
   void  Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
   static BOOL OrarioValido(const STRINGA & Path); // Mi dice se l' orario e' valido
};

//----------------------------------------------------------------------------
// DATI_ORARIO_FS               
//----------------------------------------------------------------------------
class DATI_ORARIO_FS {                                           
   
   public:

   DATI_ORARIO_FS(const STRINGA& PathOrario,const STRINGA & PathFastStart);  // Carica i Dati Orario dai files
   ~DATI_ORARIO_FS();
   
   BOOL NewRisolvi_ST( ID Origine, ID Destinazione, SDATA  Data,WORD Ora);
   BOOL MezziDirettiPrenotabili(ID Da,ID A, SDATA Data);
   
   BOOL ImpostaProblema( ID Origine, ID Destinazione, SDATA Data);
   void Free();
   
   void ValorizzaSoluzioni(); // Applica la tariffazione FS

   BOOL NodoDiCambio(ID Esterno)const{ID Int = Interno(Esterno); return Int && Int <= BaseNodi;};

   // Soluzioni trovate. Azzerata da Free()
   ELENCO_SOLUZIONI Soluzioni;                  // Contiene le soluzioni trovate e Valide
                                                                                                         
   STRINGA  PathOrario  ;
   STRINGA  PathFast    ;  // Path su cui stanno i files di fast start
   VALIDITA_ORARIO ValiditaOrario;

   BOOL ElaborazioneInterrotta;

   // Queste saranno utilizzate dall' interfaccia utente
   BOOL  Activate(); // Attiva i Dati Orario (carica in memoria i suoi dati fondamentali)

   void CleanUp();   // Pulizie effettuate alla fine della risoluzione di un problema

   static BYTE ProgressivoRichiesta; 
   static DATI_ORARIO_FS * Corrente;    // Dati Orario correntemente attivi

   // L' header e' public solo per le funzioni di caricamento dati
   struct HEADER {
      ULONG   TotalSize                    ;  // Dimensione (in BYTES)
      ULONG   IndirizzoIniziale            ;  // Indirizzo con cui ho riallocato l' area
      WORD    NumNodiDiCambio              ;  // Numero dei nodi di cambio
      WORD    NumClusters                  ;  // Numero dei clusters
      WORD    NumCollegamenti              ;  // Numero dei COLLEGAMENTI (comprende margini)
      WORD    NumStzCluster                ;  // Numero delle relazioni Stazione-Cluster  (Comprende margini)
      WORD    TotIdInterni                 ;  // Dimensione della tabella ( = Dim() delle Stazioni)
      WORD    TotPeriodicita               ;  // Numero delle differenti periodicita'.
      void    Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
      F_DTM   TimeStampMotore              ;  // Timestamp di MOTORE.DLL
   } ;

   // Queste per i trace
   void TraceIdInterno(const STRINGA& Msg, ID Interno, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA); 
   void TraceIdEsterno(const STRINGA& Msg, ID Esterno, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA); 
   void TraceCluster(const STRINGA& Msg, ID Id, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA); 

   // Accessors 
   ID  IdOrigine()const{return IdStazioneOrigine;};                     // Id interno
   ID  IdDestinazione()const{return IdStazioneDestinazione;};           // Id interno
   ID  EIdOrigine()const{return Esterno(IdStazioneOrigine);};           // Id Esterno
   ID  EIdDestinazione()const{return Esterno(IdStazioneDestinazione);}; // Id Esterno

  BOOL GestioneParticolareTraghetti; // Per viaggi da/ verso sardegna

  // Questo da' la periodicita' a partire dall' indice di periodicita'
  inline friend PERIODICITA & Per(int IdxPeriodicita){ return Orario.AllPeriodicita[IdxPeriodicita];};

   class IDX_TESTI_NOTE   & FileNot(){ return  * FileNote;};

   // ---------------------------------------------------------------------
   // E questo e' per l' autodiagnostica
   // ---------------------------------------------------------------------
   // Se la variabile e' impostata la autodiagnostica e' attiva
   static BOOL AutoDiagnostica;
   static BOOL AutoDiagnosticaPath;

   static SOL_IMAGE Target; // La soluzione su cui faccio autodiagnostica
                            // Man mano che la disgnostica procede pu• essere
                            // sostituito da altre soluzioni

   static ARRAY_ID  StazioniTarget;

   protected:

   BOOL Semplifica();
   BOOL NewCriterioDiArresto(const int i);
   
   GRAFO & Grafo;
   
   BOOL  ImpostazioneOK;    // Se false non parte la soluzione del problema
   
   // Files con i dati di orario che servono durante OGNI elaborazione
   FILE_RO               * ClustExt    ;  // File con i treni dei cluster                   PathOrario + "CLUSSTAZ.EXT"
   class  F_STAZN_CLUS   * StazCluster ;  // File con le relazioni stazione-> Cluster       PathOrario + "MM_STCLU.DB" 
   class  BS_STAZIONE_F  * FilStaz     ;  // File con dati stazioni dipendenti da orario    PathOrario + "MM_STAZN.DB"

   // Questi files sono per gli accessi di dettaglio ai nomi dei treni ed alle note
   class F_DETTAGLI_MV          * FileDettagli;
   class FILE_RO                * FileDettagliExt;
   class IDX_TESTI_NOTE         * FileNote;
   class FILE_RO                * FileNoteExt;

   // Questo Buffer tiene TUTTI i dati dell' orario se debbo tenere l' orario in memoria
   BUFR AllExt;

   // Tabelle utilizzate dal motore
   ID                     * TabIdInterni   ;  // Area di decodifica degli ID
   ID                     * TabIdEsterni   ;  // Area di decodifica degli ID
   PERIODICITA            * AllPeriodicita ;  // Area delle periodicita'
   class NODO_CAMBIO      * NodiDiCambio   ;  // Nodi di cambio
   class STAZN_CLUS       * StzClusters    ;  // Relazioni Stazioni -> Clusters
   class CLUSTER          * Clusters       ;  // Dati dei Clusters
   class COLLEGAMENTO     * Collegamenti   ;  // Collegamenti diretti

   ID         IdStazioneOrigine;
   ID         IdStazioneDestinazione;

   DIST       DistanzaMinima               ;  // Distanza minima sul grafo tra Origine e Destinazione
   DIST       DistanzaLimite               ;  // Distanza massima sul grafo per i percorsi accettabili
                                              // E' calcolata con i seguenti criteri:
                                              // LIM_DIST % in piu' rispetto alla distanza della soluzione piu' breve trovata
                                              // Oppure 50 Km
                                              // Non utilizzata per i traghetti

   WORD       LimiteNodi                   ;  // Max Id dei soli nodi = Numero dei nodi di cambio compresi i nodi fittizi
   WORD       BaseNodi                     ;  // Max Id dei soli nodi = Numero dei nodi di cambio: senza nodi fittizi
                                              // Sono definiti 2 nodi fittizi: uno per l' origine ed uno per la destinazione
   WORD       NumClusters                  ;  // Numero di clusters reali

   HEADER  Header;
   

   BOOL DefinisciNodoDiCambio(ID Esterno);
   
   WORD OraOrd; // Ora da cui cercare le soluzioni

   BOOL NuoveSoluzioni; // per il criterio di arresto
   WORD Base          ; // per il criterio di arresto
   WORD Delta         ; // per il criterio di arresto
   WORD Extend        ; // per il criterio di arresto
   enum { DIRETTO_FACILE , FACILE, DIFFICILE } DifficoltaProblema ; // per il criterio di arresto 
   void Classifica()  ; // Classifica le soluzioni e discrimina il problema (cioe' ne determina la difficolta')

   friend class ELENCO_SOLUZIONI;
   friend class CLU_BUFR;
   friend class SOLUZIONE;
   friend class CELLA;
   friend class CLUSTRENO;
   friend void  ElaboraPercorsi();

   friend inline ID Interno(ID Esterno);
   friend inline ID Esterno(ID Interno);


   static BYTE * TabelleDeiDatiOrario; // Allocata con dimensione sufficiente a contenere
                                        // l' area maggiore necessaria per gli orari
   static ULONG  AreaAllocataPerOrari;
   static ELENCO All;                   // Elenco di tutti i DATI_ORARIO_FS
};


//----------------------------------------------------------------------------
// Struttura fisica dei files
//----------------------------------------------------------------------------
// Tabella Clusters->Stazioni 
struct CLUSTERS_STAZ {        // PathOrario + "CLUSSTAZ.DB"        
   BIT  IdCluster        :11; // Id del Cluster
   BIT  TipoCluster      : 4; // Vedere ML_OUT.HPP
   BIT  OffSetBytes      :23; // Offset nel file estensione (in bytes) Max 8 M
   BIT  DimDatiC         :19; // Dimensione del cluster                Max 500 K
   BIT  DimDatiD         :19; // Dimensione del cluster                Max 500 K
   BIT  Id1              :13; // Primo   nodo del cluster
   BIT  Id2              :13; // Secondo nodo del cluster
};
// Classe di accesso  (solo sequenziale)
class _export CLUSTA: public FILE_FIX { // Accesso per id = chiave     
   public:
   // Costruttore
   CLUSTA(const STRINGA& Path=".",const STRINGA & NomeFile="CLUSSTAZ.DB",ULONG PageSize=sizeof(CLUSTERS_STAZ)*64) :
   FILE_FIX(Path+"\\"+NomeFile,sizeof(CLUSTERS_STAZ),PageSize) {};
   
   // Definizione operatori
   CLUSTERS_STAZ&  RecordCorrente(){ return *(CLUSTERS_STAZ*) RecordC; };
};

struct CLUSSTAZ {            // PathOrario + "CLUSSTAZ.EXT"            
   BIT Fermata           :1; // Vero se almeno un treno vi ferma
   BIT ClasseCoincidenza :2; // Classi di coincidenza: Di norma:
                             // 0 =   Normale (10 Minuti )
                             // 1 =   Esteso  (15 Minuti )
                             // 2,3 Attualmente non definite.
                             // I tempi effettivi dipendono dal livello di ricerca
   BIT       IdNodo      :13;// Id della stazione.
   short int MediaKm     :13;// Distanza Mediana in Km del nodo dal nodo dominante del Cluster (media su tutti i MV del cluster)
   GRUPPO    Gruppi         ;// Per filtraggio soluzioni
   
};

struct INFOSTAZ {             // PathOrario + "CLUSSTAZ.EXT"           
// NB. Per i transiti sono valorizzati solo Transita e ProgressivoStazione, il resto e' a 0
   BIT   Arrivo                  : 1; // Vero se il treno ferma alla stazione e vi ARRIVA
   BIT   Partenza                : 1; // Vero se il treno ferma alla stazione e vi PARTE
   BIT   GiornoSuccessivoArrivo  : 1; // Se arriva alla fermata il giorno successivo alla partenza del treno
   BIT   GiornoSuccessivoPartenza: 1; // Se parte dalla fermata il giorno successivo alla partenza del treno
   BIT   TransitaOCambiaTreno    : 1; // Se Arriva o Parte indica che alla stazione cambia numero Treno
                                      // Altrimenti Š vero se il treno vi transita senza fermare
   BIT   P_MezzoViaggiante       : 3; // Mezzo viaggiante da indicare sul biglietto partendo dalla stazione
   BIT   OraArrivo               :11; // in minuti dopo la mezzanotte
   BIT   OraPartenza             :11; // in minuti dopo la mezzanotte
   int   DeltaKm                 : 9; // Progressivo Km della stazione: va sommato a MediaKm di ClusStaz; Da -128 a 127 + 1 bit di margine sicurezza
   BYTE  ProgressivoStazione        ; // Debbo riportarlo perche' in alcuni casi si ha un ordinamento anomalo
                                      // NB: Non posso usare l' ordinamento delle stazioni nel cluster per ordinare le
                                      // stazioni perche' per quelle di transito la linearizzazione non e' esatta. 
                                      // Per lo stesso motivo non posso al momento usare i Km, anche se con un ulteriore 
                                      // lavoro si potrebbe superare il problema e risparmiare dei dati su file.
   BOOL Ferma(){return Arrivo | Partenza;};
};

typedef char ID_VIAG[5]; // Id Mezzo viaggiante (Attenzione: Non e' NULL Terminated)
union VIAGG {
   ID_VIAG  IdMezzoViaggiante;
   struct {
      char Test      ;         // Impostato a 0 binario
      char Numero    ;         // Numero dei mezzi viaggianti caricati 
      WORD OffsetDati;         // Impostato con l' offset dei mezzi viaggianti
   } Multipli;
};

struct CLUSTRENO {             // PathOrario + "CLUSSTAZ.EXT"          
   BIT          IdMezv                 :17 ; // Id del mezzo virtuale
   BIT          PeriodicitaUniforme    : 1 ; // Indica se la periodicita' e' uniforme
   BIT          ProgressivoPeriodicita : 1 ; // Progressivo di periodicita' (solo 0 o diverso da 0)
   BIT          IdxPeriodicita         :11 ; // Indice periodicita' (Max 2048, sono in realta' circa 400)
   MM_INFO      InfoUniforme               ; // Servizi (uniformi) del Mezzo virtuale
   WORD         OffsetServizi              ; // Servizi e classifica disuniforme del treno
   BYTE         KmLimiteTrattaCorta        ; // Km oltre cui una tratta va considerata lunga (max 250 Km) : se < 250 e' 1/2 percorrenza totale mezzo virtuale
   GRUPPO       Gruppo                     ; // Gruppo del treno
   VIAGG        Viaggiante                 ; // Mezzo Viaggiante
   INFOSTAZ     Nodi[1]                    ; // La dimensione effettiva e' di TotStazioni, eguale per tutti i treni del cluster
   PERIODICITA & Per(){ return ::Per(IdxPeriodicita);};
};

// Estensione 
struct CLUSTERS_STAZ_EXT {     // PathOrario + "CLUSSTAZ.EXT"          
   DWORD     DimDati;             // Dimensione di questo cluster, in bytes
   WORD      IdCluster;           // Id del Cluster
   WORD      TotStazioni;         // Numero delle stazioni del cluster
   WORD      NumeroNodi;          // Numero stazioni di cambio (FERMATE e nodi di TRANSITO)
   WORD      NumeroTreni;         // Numero treni del cluster
   DWORD     OffsetIdTreni  ;     // Informazioni varie : ID dei treni
   DWORD     OffsetSrvTreni ;     // Informazioni varie : Servizi dei treni
   CLUSSTAZ  NodiDelCluster[1];   // La dimensione effettiva e' di TotStazioni
};

// Per le multistazioni
struct COLLEG_URBANO {
   WORD Minuti;
   WORD Km    ;
};
struct ACRONIMI_CITTA {
   char Acronimo[4];
};


//----------------------------------------------------------------------------
// CLU_BUFR : Strutture e POOL per accedere ai dati dei treni
//----------------------------------------------------------------------------
// NB: Per i cluster multistazione invece dei dati dei treni vi e' una matrice 
// quadrata di interscambio di strutture COLLEG_URBANO

class CLU_BUFR : public BUFR {                                         
   
   public :

   CLU_BUFR(ULONG Size = 0);
   ~CLU_BUFR();

   BOOL             Concorde ; // Indica se contiene il verso Concorde o discorde
   WORD                Tipo  ; // Tipo Cluster
   CLUSTERS_STAZ_EXT * Dat   ; // Dati generali del cluster
   CLUSSTAZ  *         Nodi  ; // Array di nodi
   CLUSTRENO *         Treno ; // Treno corrente
   BYTE *         FirstTreno ; // Inizio area con i dati dei treni
   WORD DimTreno             ; // Dimensione in bytes occupata da un record treno

   // ========== PER CLUSTER MULTISTAZIONI (Colleg Urbani ) =============
   COLLEG_URBANO & Collegamento(WORD i,WORD j){ // Collegamento urbano tra due stazioni (dati gli INDICI)
      // Per semplicita' utilizzo gli stessi puntatori dei TRENI
      return ((COLLEG_URBANO *) FirstTreno)[i * Dat->TotStazioni + j];
   };
   ACRONIMI_CITTA & Acronimo(WORD i){
      return ((ACRONIMI_CITTA*)  (((COLLEG_URBANO *) FirstTreno)+(Dat->TotStazioni * Dat->TotStazioni)))[i];
   };
   // ========== PER CLUSTER MULTISTAZIONI (Colleg Urbani ) =============

  // Questa funzione ritorna l' ID del mezzo viaggiante.  
  // Opera sul MezzoVirtuale corrente 
  // Per le stazioni di cambio il treno considerato e' quello in partenza.
  char * IdTrenoCorrente(BYTE P_Nodo) { 
     if (Treno->Viaggiante.Multipli.Test) {
        return Treno->Viaggiante.IdMezzoViaggiante;
     } else {
        return ((char *)Dat ) + Dat->OffsetIdTreni + Treno->Viaggiante.Multipli.OffsetDati + 
        (Treno->Nodi[P_Nodo].P_MezzoViaggiante  * sizeof(ID_VIAG));
     } /* endif */
  };

  // Questa funzione mi determina se il treno corrente e' regionale o comunque
  // di un tipo che ha un ridotto tempo di coincidenza.
  // Poiche' la classifica varia tratta per tratta si deve specificare a 
  // quale stazione ci si riferisca. 
  // Per le stazioni di cambio si considera il tutto regionale sia 
  // che sia regionale il treno in arrivo che il treno in partenza.
  BOOL __fastcall RidottoTempoCoinc(BYTE P_Nodo);

  // Questa funzione imposta servizi e classifica, dati gli indici
  // delle stazioni limite della tratta. Opera sul MezzoVirtuale corrente
  // Vuole anche come input la data di partenza della SOLUZIONE (non della tratta)
  // espresso come offset da inizio orario
  // Puo' determinare i servizi in base alle sole tratte (GG < 0 ) o a tratte e data
  void __fastcall DeterminaServizi(BYTE P_Da, BYTE P_A, int GG, MM_INFO & Out);

  // Questa indica il Nø del treno nel cluster (usata per debug)
  int ProgTreno(){ return ( ((BYTE*)Treno) - ((BYTE*)FirstTreno) ) / DimTreno ;};

   void Dump(ID Da, ID A, BOOL Force);   // Dump del contenuto del cluster (opzionalmente selettivo) : Da ed A sono Id Esterni
                                         // Force aggira la selezione
   
   // Questa funzione cerca un nodo tra i dati del cluster, e ne ritorna l' indice (o -1 se non trovato)
   int __fastcall PosNodoInCluster(ID Id); // Id e' l' ID Esterno del nodo

   void Set(){
      Dat   = (CLUSTERS_STAZ_EXT*)Dati;
      Nodi  = Dat->NodiDelCluster;
      Treno = (CLUSTRENO*) (Nodi+Dat->TotStazioni);
      DimTreno = sizeof(CLUSTRENO) + (sizeof(INFOSTAZ) * (Dat->TotStazioni-1));
      FirstTreno = (BYTE*)Treno; // Inizio area con i dati dei treni
   };
   void FindTreno(WORD Indice){
      Treno = (CLUSTRENO*) (FirstTreno + (DimTreno * Indice));};
   void NextTreno(){ // Altera Treno e lo fa puntare al treno seguente
      Treno = (CLUSTRENO*) ( (BYTE*)Treno + DimTreno);};
   void PrevTreno(){ // Altera Treno e lo fa puntare al treno Precedente
      Treno = (CLUSTRENO*) ( (BYTE*)Treno - DimTreno);};

   // Questa funzione mi da la distanza (in valore assoluto) tra due FERMATE del treno corrente
   // NON viene controllato che siano effettivamente due fermate (NB: Non funziona per i transiti !)
   WORD PercorsiKm(ID OfsIn , ID OfsOut){
      return abs( (Treno->Nodi[OfsIn].DeltaKm + Nodi[OfsIn].MediaKm) - (Treno->Nodi[OfsOut].DeltaKm + Nodi[OfsOut].MediaKm));
   };
   
   static void ClearPool(BOOL Dealloca=FALSE);   // Dealloca o inizializza le risorse fisiche del pool
                                                 // Chiamato all' attivazione di un nuovo orario od alla fine
   static void ClearPool2();                     // Libera solo parte delle risorse fisiche
                                                 // Chiamato dopo ogni ricerca
   static CLU_BUFR * __fastcall GetCluster(ID IdCluster , BYTE Concorde, BYTE PuoiEseguireIO = TRUE);
   static void __fastcall FreeCluster(ID IdCluster , BYTE Concorde);
   
   short int KmMediDaId(ID Id); // Distanza di un nodo dal nodo dominante del cluster (in Km)
   INFOSTAZ * EsisteNodo(ID Id); // Rende NULL se il nodo non esiste se no  il suo puntatore
   static void TraceTotalIo();
   
   INFOSTAZ & Nodo(int PNodo) { return Treno->Nodi[PNodo];};

   ID IdNodoDaProg(BYTE Prog) { 
      for (int i = 0; i < Dat->TotStazioni; i++ ) {
         if(Treno->Nodi[i].ProgressivoStazione == Prog)return Nodi[i].IdNodo;
      } /* endfor */
      return 0;
   };

   private:

   static struct DATI_POOL* Pool ;               // Cache di I/O
   static ULONG             ProgressivoCorrente; // Progressivo di I/O (attenzione: presumo non vada mai in overflow)
   static int               NumClustersLetti[3]; // Numero di clusters letti NELLA RICHIESTA ATTUALE
   static int               NumBytesLetti[3];    // Numero di bytes letti NELLA RICHIESTA ATTUALE
   static ID *              TabClust;            // Tabella per identificare quanti clusters differenti sono stati effettivamente richiesti
   static WORD *            TabNumNodi;          // Se tengo i dati in memoria mi mantiene il numero nodi originale su File
   static BYTE              LastProg;            // Progressivo di richiesta
   static BYTE *            FastAccess ;         // Tabella per accesso rapido ai cluster

   friend class DATI_POOL;

//<<< class CLU_BUFR : public BUFR {
};

enum STATO_POOL {
   LIBERO   ,         // Puo' essere utilizzato senza problemi
   INLETTURA,         // E' in fase di lettura dei dati
   DATILETTI,         // I dati sono stati letti ma non ancora utilizzati
   DATIUSATI          // I dati sono stati usati: sono ancora validi
   // Da DATIUSATI puo' subire le seguenti transizioni di stato:
   // -> INLETTURA  Vuol dire che lo slot e' stato riutilizzato per un altro
   //               Cluster: I dati non sono piu' validi
   // -> DATILETTI  Vuol dire che i dati sono stati nuovamente richiesti
};
// NB: Le transizioni di stato sono fatte SENZA protezione di semafori, in quanto la
// dimensione dei buffer di I/O e' sufficiente ad impedire accessi concorrenti allo
// stesso SLOT. Il ciclo di ricerca di uno SLOT libero e' fatto in una critic-sect
// in modo da avere una ricerca congruente. 
// Gli slot sono protetti, aggiornando prima Progressivo, ottendo in pratica gli
// stessi effetti dell' uso di uun semaforo.
// Attenzione: se si proteggesse tutto le performance sarebbero impattate !

struct DATI_POOL {
   STATO_POOL Stato                ; // Stato dello SLOT in esame
   CLU_BUFR   Bufr                 ; // Dati del cluster
   ID         Cluster              ; // Cluster immagazzianto nel buffer
   ULONG      NumRichiesta         ; // I dati piu' vecchi di una richiesta sono cancellati
   ULONG      Progressivo          ; // Progressivo di accesso ai cluster : Usato invece di un semaforo
   ID         Origine              ; // Id esterno della stazione origine della richiesta che ha caricato il buffer
   ID         Destinazione         ; // Idem per destinazione
   BYTE       NumInvalidate        ; // Numero delle stazioni invalidate
   BYTE       Concorde             ; // Mi indica se il buffer contiene la parte concorde o discorde
   WORD       NumeroNodi           ; // NumeroNodi prima delle modifiche
   DATI_POOL(){Clear();};
   void Clear();
   BOOL RiorganizzaDatiCluster(ID Org, ID Dest); // Riorganizza i dati del cluster 
      // Torna FALSE se non e' possibile riorganizzare i dati.

};

//----------------------------------------------------------------------------
// FUNZIONI INLINE
//----------------------------------------------------------------------------
inline ID Interno(ID Esterno){ return Orario.TabIdInterni[Esterno]; };
inline ID Esterno(ID Interno){ return Orario.TabIdEsterni[Interno];};

#endif
