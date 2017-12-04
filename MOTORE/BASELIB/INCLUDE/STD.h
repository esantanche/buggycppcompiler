#include "dummy.h"

//----------------------------------------------------------------------------
// FILE STD.H
//----------------------------------------------------------------------------

// °°°°°°°°°°°°°°°°°°°°°°°°°°°° REPORT AGGIORNAMENTI °°°°°°°°°°°°°°°°°°°°°°°°°°°

// Data ultima modifica documentazione:         01/03/93;

// Data ultima modifica codice:                 00/00/93;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° DESCRIZIONE °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Include e dichiarazioni di prototipi che devono essere sempre inclusi da
// tutti i moduli che compongono il W.F

#ifndef HO_STD_H
#define HO_STD_H        // Indica che questo file Š gi… stato incluso;


// In fase di sviluppo delle classi basi del W.F. pone le i metodi definiti come
// INLINE non inline;

#if !defined(INLINE)
#if defined(IN_TEST)
#define INLINE
#else
#define INLINE inline
#endif
#endif


// Include delle librerie basi del C;

#include <stdlib.h>   // Libreria standard del C;
#include <string.h>   // Libreria standard del C per le stringhe;
#include <stdio.h>    // Libreria standard del C per gestione I/O;


// Le seguenti definizioni sono rese necessarie in quanto non viene incluso il
// file OS2.H. Le righe seguenti sono state tratte da OS2.H ed opportunamente
// modificate;

#define OS2_INCLUDED
extern "C"
{
   #include <os2def.h>    // Common definitions
}

#if defined(MODULO_OS2_DIPENDENTE) || defined(MODULO_OS2BASE_DIPENDENTE)
   extern "C"
   {
      #include <bse.h>       // OS/2 Base Include File
   }
#endif

#if defined(MODULO_OS2_DIPENDENTE)
   extern "C"
   {
      #include <pm.h>        // OS/2 Presentation Manager Include File
   }

   #undef MPFROMSHORT
   #define MPFROMSHORT(a) ((MPARAM)(ULONG)(a))

#else
   typedef VOID FAR * MPARAM;
   typedef VOID FAR * MRESULT;

   #define CS_SIZEREDRAW    0x00000004L
#endif

// Per tenere traccia delle fopen ed fclose (e DosOpen e DosClose)
// E' necessario poi includere fopentrc.obj
#ifndef NO_TRC_FOPEN
// #include <fopentrc.h>
#endif

// Ridefinizione di TRUE e FALSE;
#undef TRUE
#undef FALSE
#define TRUE  BOOL(1)
#define FALSE BOOL(0)


// Definizione dei prototipi delle funzioni utilizzate nel WorkFrame;

// Prototipo delle Window Procedure Presentation Manager utilizzate negli
// oggetti Presentation Manager del W.F;

typedef MRESULT EXPENTRY WINPROC(HWND,ULONG,MPARAM,MPARAM);


// Prototipo di un puntatore a funzione per richiamare i costruttori statici
// delle classi utilizzate per la fiormattazione dei dati contenuti nel file
// .OBS;

typedef void * (*INSTALLER)(void*);


// Definizione tipo dato CPSZ come un puntatore costante ad una stringa
// terminata da '\0';

#define CPSZ const char *


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°° STRUCT L_BUFFER °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Dichiarazione della struttura dati L_BUFFER utilizzata per l'implementazione
// di aree di bufferizzazione di dati. Vengono definiti per questo tipo di dato
// delle funzioni per una efficiente gestione di queste aree dati;
// Questa struttura dati viene utilizzata solo nella classe ELENCO_O;

struct L_BUFFER
{
   USHORT Length;    // Lunghezza effettiva dei dati contenuti nell'area;
   void   * Dati;    // Puntatore all'area di memoria contenente i dati;
};


// °°°°°°°°°°°°°°°°°°°°°°°°° DICHIARAZIONE DELLA CLASSE °°°°°°°°°°°°°°°°°°°°°°°°

// Dichiarazione della classe INI_BUFFER utilizzata per la bufferizzazione e
// la gestione dei dati provenienti dai file .OBS;
// L'implementazione dei metodi Š in tokens.c;

class _export INI_BUFFER
{

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°    AREA  PUBLIC    °°°°°°°°°°°°°°°°°°°°°°°°°°°°

   public:

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°     ATTRIBUTI    °°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   ULONG  Length ;   // Lunghezza dei dati contenuti nell'area dati puntata
                     // dal puntatore Dati;

   void   * Dati ;   // Puntatore all'area di memoria contenente i dati;

   ULONG  Pointer;   // Attributo utilizzato per indicare lo spiazzamento
                     // durante la scansione dell'area dati;

   ULONG  Alloc  ;   // Dimensione dell'area dati allocata per contenere i dati;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°      METODI      °°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   INI_BUFFER(ULONG  Size=0);

            // Costruttore di istanze della classe INI_BUFFER;

            // Parametri:

            //    Size        : Dimensione dell'area inizialmente allocata per
            //                  contenere i dati;

   INI_BUFFER(const INI_BUFFER& From);

            // Costruttore di istanze della classe INI_BUFFER che accetta come
            // parametro un'altra istanza della classe INI_BUFFER;

            // Parametri:

            //    From        : Istanza della classe INI_BUFFER da copiare nella
            //                  nuova istanza;


   ~INI_BUFFER();

            // Distruttore di istanze della classe INI_BUFFER;


   void ReDim(ULONG Size);

            // Metodo per effettuare la riallocazione dell'area atta a contenere
            // i dati;

            // Parametri:

            //    Size        : Dimensione dell'area dati da riallocare;


   void Clear();

            // Metodo che effettua la pulizia del buffer;


   INI_BUFFER & operator= (const INI_BUFFER & From);

            // Operatore di assegnamento;


   BOOL SkipToken();

            // Metodo che permette di posisionare il puntatore utilizzato per
            // la scansione dell'area dati sulla struttura dati contenuta nel
            // buffer successiva a quella correntemente puntata;


   BOOL CopyToken(INI_BUFFER & From);

            // Metodo che permette di copiare una struttura dati contenuta
            // in una istanza di un INI_BUFFER in un altro;

};


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Funzioni per il trace e la gestione di stringhe

#include <trc2.h>     // Libreria di debug (ed informazioni compilazione);
#include <STRINGA.h>  // Libreria di gestione delle stringhe;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Prototipi delle procedure che servono ad ottenere alcune variabili globali
// di sistema. Alcune di queste procedure dovrebbero essere, da un punto di
// vista logico, contenute in file diversi, ma sono state raggruppate tutte in
// questo;
// In questo file sono contenute solo le procedure di ottenimento delle
// variabili globali, quelle di inizializzazione delle stesse sono contenute nel
// file GLUE.H;

#ifdef MODULO_OS2_DIPENDENTE
// 
// HAB   _export GetHab();
// 
//             // Ottenimento del valore dell'Hancor Block del WorkFrame;
// 
// 
// HWND  _export GetEventHwnd();
// 
//             // Ottenimento dell'handle della window di dispatching dei messaggi
//             // associata al thread che gestisce gli oggetti P.M.
// 
// 
// HWND  GetEventHwnd2(USHORT Thread_Num=0);
// 
//             // Ottenimento dell'handle della finestra di dispatching messaggi
//             // associata al thread, Thread_Num, utilizzato per la gestione di
//             // oggetti C++;
// 
// 
// HWND  _export GetClientHwnd();
// 
//             // Ottenimento dell'handle della client area della Mappa Principale
//             // dell'applicazione;
// 
// 
// HWND  _export GetMainHwnd();
// 
//             // Ottenimento dell'handle della frame area della Mappa Principale
//             // dell'applicazione;
// 
// 
// HWND  _export GetHelpHwnd();
// 
//             // Ottenimento dell'handle dell'istanza dell'help manager utilizzata
//             // per gestire l'help dell'applicazione;
// 
// HWND  _export GetHandleAtomTable();
// 
//             // Ottenimento dell'handle dell'istanza dell'AtomTable utilizzata
//             // per gestire le relazioni applicative con nome;
#endif


class EVQUEUE & _export GetEvQueue();    // Coda degli eventi Principale

            // Ottenimento del puntatore all'istanza della classe EVQUEUE che
            // funge da coda degli eventi del W.F.


void _export Abend(int Abcode,const class OGGETTO* Origine=NULL);   // Forza un abend (IN ERROR.C)

            // Funzione per mandare l'applicazione in abend dopo aver prodotto
            // un messaggio diagnostico. Il metodo Š implementato nel file
            // ERROR.C;

            // Parametri:

            //    Abcode      : Codice di abend;
            //    Origine     : Puntatore all'oggetto che ha causato l'Abend;


extern "C" {
APIRET APIENTRY DosBeep(ULONG freq, ULONG dur);
APIRET APIENTRY DosSleep(ULONG MilliSeconds);
}

// Funzione per fare un BEEP generico di attenzione
// Usare la macro BEEP che riporta anche un' informazione di trace
// (La riporta anche se il trace non e' compilato )
#define BEEP {Beep(__FILE__,__LINE__,LIVELLO_DI_TRACE_DEL_PROGRAMMA); }
void _export Beep(char* File,int Line,USHORT TLev);
            // Funzione per produrre un Beep. Implememntata nel file UTENTE.C;


// Macro di definizione di stringhe vuote;

#define NUSTR (*(STRINGA*)NULL)
#define NUPSTR ((STRINGA*)NULL)
#define EMPTYSTR (STRINGA)""

//----------------------------------------------------------------------------
// MACRO utili
//----------------------------------------------------------------------------
// Scansione di elenchi, per tutti gli oggetti, senza ordine garantito
// La scansione NON include eventuali oggetti creati dopo l' inizio.
// Nessun oggetto deve essere distrutto durante la scansione per evitare
// errori, TRANNE l' oggetto al momento sottoposto a scansione
// (Queste due caratteristiche POSSONO essere utilizzate)
// L' elenco deve essere passato per reference non per puntatore
// Non utilizzare la stessa variabile __i piu' volte all' interno di un blocco

#define FORALL(__Elenco,__i) for(int __i=(__Elenco).Dim()-1;__i>=0 ;__i--)

// Questa macro scandisce in ordine diretto gli oggetti.
// Utilizzarla solo se l' ordine e' importante.
// NON permette di distruggere oggetti !!!

#define ORD_FORALL(__Elenco,__i) for(int __i=0,__i##_dim=(__Elenco).Dim();__i<__i##_dim;__i++)

// Riempimento di un' area con zeri binari

#define ZeroFill(_a)  memset(&_a,'\0',sizeof(_a));

//----------------------------------------------------------------------------
// THIS
//----------------------------------------------------------------------------
// Fornisce un this gia' dereferenziato
// Comodo per le routines applicative

#define THIS (*this)

//----------------------------------------------------------------------------
// Queste definizioni sono state prese da varii include e spostate
// qui per la migrazione a Windows, per comodit…
//----------------------------------------------------------------------------
char * DecodificaRcOS2(int Rc);
// Routine per Informazioni Globali Copiate da eventi.h
ULONG _export Time();         // Tempo come centesimi di secondo a partire dalla mezzanotte
ULONG _export TimePart();     // Ritorna il tempo in centesimi di secondo dalla partenza del programma (gestisce cambi giorno / mese / anno)
// Queste da error.h
inline void Abend(int i, const OGGETTO * ){ exit(i);};
// Queste disabilitano la gestione delle eccezioni
#define GESTIONE_ECCEZIONI_ON
#define GESTIONE_ECCEZIONI_OFF

#endif
