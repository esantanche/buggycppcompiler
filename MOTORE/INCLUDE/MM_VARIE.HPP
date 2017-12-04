//========================================================================
// Miscellanea utilizzata dal motore
//========================================================================
// 1G

#ifndef HO_MM_VARIE_HPP
#define HO_MM_VARIE_HPP

#ifndef HO_FILES_HPP
#include "FILE_RW.HPP"
#endif

#ifndef HO_CHNTEMPL_HPP
#include "CHNTEMPL.HPP"
#endif

// EMS004 #define PROFILER_ABILITATO // NB: Quando si abilita / disabilita ricompilare prova6 o meglio fare DO ALL
// Abilito - Disabilito il trace sugli inner loops
//#define ENABLE_INNER

// -----------------------------------------------------------------------
// FILE_FISSO : template di definizione rapida files con records a dimensione fissa
// -----------------------------------------------------------------------
template <class STRUTTURA>
class FILE_FISSO: public FILE_FIX {
   public:
   FILE_FISSO(const STRINGA& NomeFile,ULONG BufSize=64000):
      FILE_FIX(NomeFile,sizeof(STRUTTURA),BufSize){};
   STRUTTURA&  operator [](UINT NumRecord){ Posiziona(NumRecord); return *(STRUTTURA*) RecordC; };
   STRUTTURA&  RecordCorrente(){ return *(STRUTTURA*) RecordC; };
};

//----------------------------------------------------------------------------
// Asserzioni: Vengono testate delle condizioni, e scritto un TRACE con un BEEP
// in caso contrario. Usano la stessa define del BOUND_CHECK
//----------------------------------------------------------------------------
#ifdef BOUND_CHECK
   BOOL SegnalaAssert(char * RTN,const char* Asserzione, char * File, int Line);
   #define assert(_a) if(!(_a))SegnalaAssert(TRCRTN , #_a,__FILE__,__LINE__)
   #define assert2(_a,_b) if(!(_a)){SegnalaAssert(TRCRTN ,(const char*)(STRINGA(_b)+" "+STRINGA(#_a)),__FILE__,__LINE__) ;}
   #define assert3(_a,_b) if(!(_a)){SegnalaAssert(TRCRTN ,#_a,__FILE__,__LINE__) ;_b;}
#else
   #define assert(_a)       {}
   #define assert2(_a,_b)   {}
   #define assert3(_a,_b)   {}
#endif
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Questo e' un oggetto di tipo Stazioni che puo' essere utilizzato dappertutto.
//----------------------------------------------------------------------------
// E' aperto dalla funzione di inizializzazione del grafo e chiuso da quella di terminazione

extern struct STAZIONI & Stazioni;

//----------------------------------------------------------------------------
// Questa cronometra un intervallo in microsecondi:
//----------------------------------------------------------------------------
// -> poiche' sottraggo circa 30 microsecondi di tempo di esecuzione ho una certa imprecisione
//    sugli intervalli corti
// -> La prima volta che e' chiamata si autocalibra
//----------------------------------------------------------------------------
DWORD _export Cronometra(BOOL StartStop, DWORD & Elapsed);

//----------------------------------------------------------------------------
// Queste macro per funzioni frequenti spicciole
//----------------------------------------------------------------------------
#define min(_a,_b) (((_a) < (_b))? (_a) : (_b))
#define max(_a,_b) (((_a) > (_b))? (_a) : (_b))
#define Top(_a,_b) if((_b) > (_a)) _a = (_b)
#define Bottom(_a,_b) if((_b) < (_a)) _a = (_b)
#define BF1(_a) BUFR(&(_a),sizeof(_a))
#define VRB(_a) &(_a),sizeof(_a)
#define VRS(x) " " #x " = "+STRINGA(x)
#define VRX(x) " " #x " = "+STRINGA((void*)int(x))
#define VRO(x) " " #x " = "+STRINGA(ORA(x))


// Macro trace utilizzate di frequente
#ifdef OKTRACE
#define TRACEID(_a) { if(trchse >= LtrcIDS ) { TRV1;TRV2(#_a); PrtText2(TRCR,TRCR2,(CPSZ) ( STRINGA(_a)+" "+STRINGA(Stazioni.DecodificaIdStazione(_a)) )); } }
#define TRACEID_L(_l,_a) { if(trchse >= _l ) { TRV1;TRV2(#_a); PrtText2(TRCR,TRCR2,(CPSZ) ( STRINGA(_a)+" "+STRINGA(Stazioni.DecodificaIdStazione(_a)) )); } }
#define TRACEVHHMM(_a) TRACESTRING2(#_a,(STRINGA(_a)+" ("+STRINGA(ORA(_a))+")"))
#else
#define TRACEID(_a) {}
#define TRACEID_L(_l,_a) {}
#define TRACEVHHMM(_a) {}
#endif

//----------------------------------------------------------------------------
// Questa routine fa un controllo che non si utilizzino piu' di n bits
//----------------------------------------------------------------------------
// begin EMS001 Win
//#define Chk(_a,_b)  Chk2(_a,_b,__LINE__)
#define Chk(_a,_b)  (_a)
// end EMS001
#define ChkS(_a,_b)  Chk3(_a,_b,__LINE__)
void _export ChkSegnala(UINT i, UINT n,int Linea);
inline UINT Chk2(UINT i, UINT n,int Linea){
   if (i >= (1 << n))ChkSegnala(i,n,Linea);
   return i;
}
inline int  Chk3(int  i, UINT n,int Linea){
   if (abs(i) >= (1 << (n-1)))ChkSegnala(abs(i),n-1,Linea);
   return i;
}

//----------------------------------------------------------------------------
// Questa routine fornisce il timestamp di una DLL dato il nome.
//----------------------------------------------------------------------------
// Il nome non deve comprendere ne path ne estensione
F_DTM _export GetTimestampDll(const char * NomeDLL);


//----------------------------------------------------------------------------
// Gestione dei tempi
//----------------------------------------------------------------------------
inline short TempoTrascorso(short Tra,short E){ short T= E-Tra;if(T<0)T += 1440 ;return T; };
// Questa torna un valore assoluto: e' la differenza di tempo tra due ore ignorando i cambi giorno
inline short DeltaTempo(short Tra,short E){ short T= E-Tra; if(T<0)T += 1440;if(T > 720)T=1440-T;return T; };

//----------------------------------------------------------------------------
// Funzioni di conversione (garantita) ad un boolean
//----------------------------------------------------------------------------
inline unsigned int Negative(int i){ return (i<0) ? 1 : 0;};
// inline unsigned int Bool(int i){ return (i) ? 1 : 0;};
inline unsigned int Bool(int i){ return i != 0 ;};

//----------------------------------------------------------------------------
// Funzioncina per leggere una serie di coefficienti da environment
//----------------------------------------------------------------------------
void GetEnvArray(int * Array, int NumEle, char * EnvName);

//----------------------------------------------------------------------------
// Queste strutture raccolgono informazioni statistiche sul motore
//----------------------------------------------------------------------------
// la define permette di abilitare/disabilitare la raccolta di informazioni
// I dati sono accumulati su TotaleParziale e poi trasferiti su ToTaleTotale
// All' inizio e' necessario dare Clear (indifferente se totale o parziale)
//----------------------------------------------------------------------------
// Bisogna definire anche l' elenco di stringhe di commenti PRIMA di chiamare
// il metodo Clear(). Altrimenti carica le definizioni del motore
//----------------------------------------------------------------------------
struct _export PROFILER {

    int   TotaleTotale;
    int   TotaleParziale;
    ULONG PrimoTempo;
    int   CallCount;       // Per avere il tempo medio
    int   CallCountTotale; // Per avere il tempo medio

    const char * Descrizione; // Puntano ad aree statiche
    static BOOL NonAttivo(){return Buffers==NULL;};

    PROFILER(){memset(this,0,sizeof(PROFILER));};

    #ifdef PROFILER_ABILITATO

    static void Conta(WORD Idx){Buffers[Idx].TotaleParziale ++; NumCall++;  };       // Conta qualcosa

    static void Somma(WORD Idx,int x){ Buffers[Idx].TotaleParziale +=x;NumCall++;};  // Totalizza qualcosa

    static void Cronometra(WORD Idx,BOOL StartStop); // Totalizza i tempi (Pur con tutte le limitazioni)
                                                     // Start := (StartStop == TRUE);
                                                     // Nota: si possono dare piu' stop dopo uno start,
                                                     // vengono accumulati i tempi parziali
                                                     // Se si passa la mezzanotte i dati non sono significativi
                                                     // Se si supera un' ora i dati non sono significativi
                                                     // I tempi sono accumulati in MICROSECONDI chiedendoli al
                                                     // timer a basso livello.

    static void Clear(BOOL Parziale)        ; // Azzera i totali o parziali
    static void Clear(WORD Idx,BOOL Parziale);// Azzera un totale o parziale
    static void Acquire(BOOL Clear)         ; // Somma i dati di parziale in Totale; Opzionalmente azzera Parziale

    static ELENCO_S GetTotale(BOOL Parziale); // Ottengo i dati (Come elenco di stringhe)
    static STRINGA  GetTotale(WORD Idx,BOOL Parziale); // Ottengo i dati

    static void  Trace(BOOL Parziale,int Livello) ; // Scrive i dati sul trace
    static void  Trace(WORD Idx,BOOL Parziale,int Livello) ; // Scrive i dati sul trace

    static void  PutStdOut(BOOL Parziale)   ; // Scrive i dati sullo standard output
    static void  AttivaDescrizioniMotore()  ; // Attiva le descrizioni standard del motore

    #else
    // Poi sono cancellate dall' optimizer
    static void Conta(WORD){};
    static void Somma(WORD,int  ){};
    static void Cronometra(WORD,BOOL){};
    static void Clear(BOOL){};
    static void Clear(WORD,BOOL){};
    static void Acquire(BOOL){};
    static ELENCO_S GetTotale(BOOL){};
    static STRINGA  GetTotale(WORD,BOOL){};
    static void  Trace(BOOL,int){};
    static void  Trace(WORD,BOOL,int){};
    static void  PutStdOut(BOOL){};
    static void  AttivaDescrizioniMotore(){};
    #endif

    static ELENCO_S Descrizioni     ;   // Descrizioni (per il motore: metodo standard AttivaDescrizioniMotore)

    private:
    static PROFILER * Buffers       ;
    static int      NumProfilers    ;   // Numero effettivamente utilizzato dall' applicazione
    static DWORD    Freq            ;   // Numero di cicli di clock DEL TIMER al secondo
    static ULONG    Delay           ;   // Tempo necessario alla funzione Cronometra per una chiamata
    static ULONG    NumCall         ;
    static ULONG    TotCall         ;
    static ULONG    DelayPer1000Call;

};
#if defined(PROFILER_ABILITATO) && defined(ENABLE_INNER)
#define ScanCount(_a) PROFILER::Conta(20),_a
#else
#define ScanCount(_a) _a
#endif

//----------------------------------------------------------------------------
// RIDEFINIZIONE DEL BEEP
//----------------------------------------------------------------------------
// A scopo di test faccio vedere il beep anche a livello di trace 0
extern STRINGA LastBeep;
#ifdef BEEP_A_TRC0
#ifdef BEEP_A_VIDEO
#define _Bpr Bprintf
#else
#define _Bpr Bprintf3
#endif

#undef BEEP
#define BEEP {                                                        \
      int TRCHSE = trchse; if(trchse == 0)trchse = 1;                 \
      _Bpr("\n =======  BEEP : Vedere il trace ==========");          \
      _Bpr("File %s Linea %u\n",__FILE__,__LINE__);                   \
      Beep(__FILE__,__LINE__,LIVELLO_DI_TRACE_DEL_PROGRAMMA);         \
      LastBeep = STRINGA(__FILE__) + ": "+STRINGA(__LINE__) ;         \
      trchse = TRCHSE;                                                \
   }
#endif
//----------------------------------------------------------------------------
// Queste defines gestiscono alcuni limiti strutturali del motore
//----------------------------------------------------------------------------
// Per il motore e per le routines di caricamento dati
#define MAXNODI  500 // Massimo numero di nodi
#define BITS_ND_CAMBIO  11   // Bits necessari per indirizzare tra i nodi di cambio
#define MAXNODI_CAMBIO  (1 <<BITS_ND_CAMBIO)-1 // Massimo numero di nodi di cambio
#define LIMITE_MEZZI_CAMBI (2*MAX_TRATTE) // Non ammetto piu' di 8 cambi
#define CLUSTER_MULTISTAZIONE 10 // Tipo cluster per i clusters multistazione
#define MVG 6                    // Massimo numero di mezzi reali in composizione di un virtuale: limite struttuale
// Per le routines di caricamento dati
#define MAX_MEZZI_REALI 5  // Numero massimo di treni reali che possono comporre un mezzo virtuale
#define MAX_STAZIONI    6000 // Id massimo delle stazioni + 1 // Se supera 8192 si devono aumentare varie aree da 13 bits
#define MAX_STAZIONI_IN_CLUSTER  255 // Massimo numero delle stazioni che possono stare in un cluster
#define MAX_VIRTUALI 20000 // Numero massimo di treni virtuali che entrano nelle aree di processing
#define MAX_VIRTUALI_IN_CLUSTER    255 // Massimo numero di treni virtuali che possono stare in un cluster
#define MARGINE_VIRTUALI_IN_CLUSTER  5 // Margine da lasciare per permettere di aggiungere i MV con periodicita' disuniforme
#define MAX_NOTE_TRENO  4  // Numero massimo di note presenti su di un singolo treno
// Array di dimensione MVG o MVG bits in varie aree del programma
#if MAX_MEZZI_REALI > MVG
#error "Le strutture utilizzate vanno riviste: Limite MAX_MEZZI_REALI raggiunto"
#endif
// Solo un byte di indirizzamento
#if MAX_STAZIONI_IN_CLUSTER > 255
#error "Gli algoritmi di caricamento dati vanno rivisti: Limite MAX_STAZIONI_IN_CLUSTER raggiunto"
#endif
// Solo un byte di indirizzamento
#if MAX_VIRTUALI_IN_CLUSTER > 255
#error "Gli algoritmi di caricamento dati vanno rivisti: Limite MAX_STAZIONI_IN_CLUSTER raggiunto"
#endif
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// SCAN()
//----------------------------------------------------------------------------
// macro per scandire un' array, un elenco od altro
// OPERA AD ALTE PERFORMANCE, in quanto evita moltiplicazioni
// Vuole:
//    - L' array
//    - Il  nome di un reference (che viene creato) che accede all' elemento in scansione
//    - Il  nome del TIPO dei dati contenuti nell' array dinamica.
// Per ottenere l'indice corrente si utilizzi la macro SCAN_IDX
// (vuole array e nome del reference)
// Se la dimensione dell' array e' 2 si utilizzi SCAN_IDX_2 (molto piu' veloce)
// SCAN_NUM : Scandisce un' array VERA, vuole come parametro anche il numero elementi
// Queste macro ottengono lo stesso effetto ottenuto dall' optimizer
// del C++ (Stregth reduction) ma hanno il vantaggio di funzionare anche nel
// caso in cui l' accesso all' ARRAY si ottenga attraverso un metodo
// (nel qual caso l' optimizer SI DEVE arrendere)
//----------------------------------------------------------------------------
// Gli if sono necessari per evitare errori di bound check quando la dimensione e' 0
#define SCAN_LIM(_Array,_Start,_Stop,_Var,_Type)     \
int _Var##_ACount  = _Start ,_Var##_AStop  = _Stop;  \
for(_Type & _Var= _Array[_Var##_ACount]; _Var##_ACount < _Var##_AStop ; _Var##_ACount++,ScanCount((_Type*)(&_Var)++))
#define SCAN_NUM(_Array,_Num,_Var,_Type)             \
int _Var##_ACount  = _Num;                           \
if(_Var##_ACount)                                    \
for(_Type & _Var= _Array[0]; _Var##_ACount > 0 ; _Var##_ACount--,ScanCount((_Type*)(&_Var)++))

#define SCAN(_Array,_Var,_Type)                      \
int _Var##_ACount = _Array.Dim();                    \
if(_Var##_ACount)                                    \
for(_Type & _Var= _Array[0]; _Var##_ACount > 0 ; _Var##_ACount--,ScanCount((_Type*)(&_Var)++))
// #define SCAN_IDX(_Var) (&_Var - &_Array[0])
#define SCAN_IDX(_Var) _Var##_ACount  // Solo con Scan_LIM (per gli altri: numero di cicli rimanenti)
#define SCAN_IDX2(_Var,_Num) (_Num - _Var##_ACount) // Non funziona con SCAN_LIM
#define ENDSCAN

//----------------------------------------------------------------------------
// Modifiche necessarie per Windows
//----------------------------------------------------------------------------
#ifndef OLD_VERSION
#undef SCAN
#undef SCAN_LIM
#undef SCAN_NUM
#undef ENDSCAN

// Questa va da Nø x a Nø y
#define SCAN_LIM(_Array,_Start,_Stop,_Var,_Type)     \
int _Var##_ACount  = _Start ,_Var##_AStop  = _Stop;  \
for(_Type * _P_##_Var= &_Array[_Var##_ACount]; _Var##_ACount < _Var##_AStop ; _Var##_ACount++,ScanCount(_P_##_Var)++)){\
_Type & _Var = *_P_##_Var;
#define SCAN_NUM(_Array,_Num,_Var,_Type)             \
int _Var##_ACount  = _Num;                           \
if(_Var##_ACount)                                    \
for(_Type * _P_##_Var= &_Array[0]; _Var##_ACount > 0 ; _Var##_ACount--,ScanCount((_P_##_Var)++)){\
_Type & _Var = *_P_##_Var;

// EMS002 a sostituzione di SCAN_NUM
#define SCAN_NUM_WIN(_Array,_Num,_Var,_Type)        \
int _Var##_ACount = _Num;                           \
if(_Var##_ACount)                                   \
for(_Type * _Var= _Array; _Var##_ACount > 0 ; _Var##_ACount--,_Var++) {

#define SCAN(_Array,_Var,_Type)                      \
int _Var##_ACount = _Array.Dim();                    \
if(_Var##_ACount)                                    \
for(_Type * _P_##_Var= &_Array[0]; _Var##_ACount > 0 ; _Var##_ACount--,ScanCount((_P_##_Var)++)){\
_Type & _Var = *_P_##_Var;
// #define SCAN_IDX(_Var) (&_Var - &_Array[0])

// begin EMS003 Win
#define TEST_SIMMETRIA_COLLEGAMENTI {  \
int iNum_errori = 0;                                              \
TRACESTRING("Test Simmetria collegamenti");                            \
TRACEVSTRING2(__FILE__);    \
TRACEVINT(__LINE__);         \
NODO_CAMBIO * Nodo = NODO_CAMBIO::NodiDiCambio + 1; \
for (int i1 = NODO_CAMBIO::TotNodiDiCambio-1; i1 > 0 && iNum_errori < 50 ;i1 -- ) { \
   if (Nodo->NumeroCollegamenti == 0) {                          \
      iNum_errori++;                                               \
      TRACESTRING("Nodo isolato: " VRS(Nodo->Id));     \
   } /* endif */                                          \
   COLLEGAMENTO * Coll = Nodo->Collegamenti();             \
   TRACEVINT(i1);   \
   TRACEVINT(Nodo->NumeroCollegamenti);                   \
   for (int i2 = Nodo->NumeroCollegamenti ; i2 > 0 && iNum_errori < 50; i2-- ) {  \
      COLLEGAMENTO * Coll1 = NODO_CAMBIO::NodiDiCambio[Coll->Id].CollegamentoDaId(Nodo->Iid()); \
      if (Coll1 == NULL) {                                                        \
         iNum_errori++;                                                          \
         TRACESTRING("Collegamento asimmetrico tra nodo "+STRINGA(Nodo->Id)+" e "+STRINGA(Coll->Eid()));   \
         TRACEVINT(Nodo->Collegamenti());  \
         TRACEVINT(Nodo->NumeroCollegamenti);  \
      } else if (Coll1->Km != Coll->Km) {                                                \
         iNum_errori++;                                                      \
         TRACESTRING("Collegamento asimmetrico (KM) tra nodo "+STRINGA(Nodo->Id)+" e "+STRINGA(Coll->Eid()) + VRS(Coll1->Km) + VRS(Coll->Km) );  \
      } else if (Coll1->Penalita != Coll->Penalita) {  \
         iNum_errori++;                                  \
         TRACESTRING("Collegamento asimmetrico (Penalita) tra nodo "+STRINGA(Nodo->Id)+" e "+STRINGA(Coll->Eid()) + VRS(Coll1->Penalita ) + VRS(Coll->Penalita ) );  \
      } else if (Coll1->PesoTratta != Coll->PesoTratta) {  \
         iNum_errori++;                                     \
         TRACESTRING("Collegamento asimmetrico (PesoTratta) tra nodo "+STRINGA(Nodo->Id)+" e "+STRINGA(Coll->Eid()) + VRS(Coll1->PesoTratta) + VRS(Coll->PesoTratta) );  \
      } /* endif */  \
      Coll ++;        \
   } /* endfor */      \
   Nodo ++;             \
} /* endfor */           \
TRACESTRING("Fine Test"); \
}

#define TRACE_1_340  \
   TRACESTRING("TRACE_1_340");     \
   TRACEVSTRING2(__FILE__);   \
   TRACEVINT(__LINE__);    \
   TRACEVINT(Interno(1));   \
   TRACEVINT(Interno(340));   \
   TRACEVINT(Esterno(1));   \
   TRACEVINT(Esterno(324));

// end EMS003 Win

#define ENDSCAN  }
#endif
//----------------------------------------------------------------------------

#endif
