#include "dummy.h"
//----------------------------------------------------------------------------
// FILE ELENCO_O.H
//----------------------------------------------------------------------------
// Definisce la classe:
// ELENCO_Oggetti
//----------------------------------------------------------------------------
// Viene normalmente incluso da OGGETTI.H
// NON INCLUDERE DIRETTAMENTE
//----------------------------------------------------------------------------
// La possibilita' di associare dei dati ad ogni elemento della lista fa' si'
// che questa classe si possa utilizzare sia per gestire degli elenchi,
// che per gestire delle relazioni (anche con attributi)
//----------------------------------------------------------------------------


#ifndef HO_ELENCO_O_H
#define HO_ELENCO_O_H  // Indica che questo file e' stato incluso

#ifndef HO_STD_H      // Includo sempre le librerie standard del progetto
#include <std.h>
#endif

#ifndef HO_STRINGA_H   // Gli oggetti vogliono che le stringhe siano definite
#include <stringa.h>
#endif

#ifndef HO_OGGETTO_H
#error Il file ELENCO_O.h non deve essere incluso direttamente ma solo via oggetto.h
#endif

//----------------------------------------------------------------------------
// Definisce il prototipo per il metodo di Sorting dell'elenco di Oggetti
//----------------------------------------------------------------------------
typedef int (PFN_QSORT) ( const void*, const void* );

//----------------------------------------------------------------------------
// Estrae il ptr all'OGGETTO dal param. inviato alla funzione da QSORT.
// Esempio:
// int pByName( const void* a, const void* b)
// {
//    return strcmp( (CPSZ)(OGGETTO_FROM_QSORT(a)->Nome) ,
//                   (CPSZ)(OGGETTO_FROM_QSORT(b)->Nome)   );
// }
//----------------------------------------------------------------------------
#define OGGETTO_FROM_QSORT( x ) ((OGGETTO*)*(LONG*)x)

//----------------------------------------------------------------------------
// La dichiarazione seguente non puo' stare in oggetto.h per problemi di precedenza
//----------------------------------------------------------------------------
enum TIPOGGE  {
   MODELLO,           // Oggetto del modello (RECORD o derivato)
   VALUE,             // Valore del modello (= CAMPO)
   TABELLAdec,        // Tabella di decodifica
   MAPPA,             // Mappa
   CONTROLLO,         // Controllo della Mappa (Entry, ListBox, ...)
   PACCHETTO,         // Pacchetto di comunicazione con Host
   LINKS,             // Link tra oggetti
   OBJSET,            // Object Set
   UNDEF,             // Non definito
   BITMAPs,           // BitMap, Pointer od Icona
   ALTRI,             // Varie
   TABELLAs,          // Nomi delle TABELLA utilizzate
   FONTs,             // Fonts
   OGGSCP,            // Oggetti SCP (di Comunicazione)
   SCP_AREATR,        // Aree di trasmissione
   SCP_CAMPODAT,      // Campo dell' area di trasmissione
   SCP_CI             // Comportamento Installabile
} ;

//----------------------------------------------------------------------------
// Classe ELENCO_Oggetti
//----------------------------------------------------------------------------
// Questa classe definisce un elenco di oggetti.
// E' implementata mediante un' array di puntatori ad oggetti.
//----------------------------------------------------------------------------
// NOTA: Deve essere garantito che l' indice degli oggetti
// segua l' ordine di inserzione: cioe' che oggetti inseriti dopo
// occupino un indice > di quelli inseriti prima, e che l' indice
// non cambi se gli oggetti sono cancellati a partire dall' ultimo.
// Cio' permette di scandire semplicemente la lista
// (In ordine diretto in creazione ed inverso in cancellazione);
//----------------------------------------------------------------------------
// Uno stesso Oggetto puo' apparire piu' volte nell' elenco.
// Tale operazione dovrebbe essere vietata ove questo possa creare
// problemi. Piu' che utilizzare il subclassing converra'
// eventualmente aggiungere una variabile BOOL che faccia eseguire,
// ove necessario, questo controllo. Al momento e' meglio evitare
// perche' rallenta.
//----------------------------------------------------------------------------
// Operazioni ammesse:
//
//   Creazione dando opzionalmente numero di oggetti da allocare subito,
//    numero MASSIMO di oggetti che possono comparire nella lista (se si supera
//    ABEND);
//   costruttore di copia
//   distruttore
// []  Accede all' iesimo oggetto  (tornato per reference)
//     se l' indice e' una stringa fa' una ricerca PER NOME e poi accede
// +=  Aggiunge un oggetto o un elenco alla lista
// -=  Toglie un oggetto o un elenco alla lista
//----------------------------------------------------------------------------
// Metodi previsti:
// Dim               :   Numero di oggetti dell' elenco
// Contiene          :   Indica se contiene un dato oggetto
// Associa           :   Dato un puntatore ad oggetto associa a tale
//                       oggetto una copia di un' area od un Numero tipizzato
//                       (e' un USHORT)
// AreaAssociata     :   Dato un puntatore ad oggetto ritorna un puntatore
//                       all' area dati associata.
//                       Opzionalmente puo' anche ritornare la lunghezza
//                       dell' area associata.
// NumAssociato      :   Variante: Ritorna il Numero (USHORT) associato
// TipoAreaAssociata :   Ritorna il tipo di area associata
// CercaPerNome      :   Dato il nome ritorna per reference l' oggetto (o NULL)
// CercaPerPuntatore :   Ritorna l' indice dell' oggetto nell' array oppure
//                       -1 se non e' trovato.
// CercaPerValore    :   Dato un valore ritorna il o gli eventuali oggetti con tale valore.
// CercaPerTipo      :   Crea un elenco temporaneo in cui mette solo gli oggetti
//                       dell' elenco appartenenti al tipo indicato.
// CercaPerNumAssoc  :   Idem se e' associato un numero
// CercaNonAssoc     :   ... solo quelli senza relazioni applicative
// ContaPerNome      :   Ritorna il numero degli oggetti con quel nome
// ContaPerTipo      :   Ritorna il numero degli oggetti di quel tipo
// ContaPerNumAssoc  :   Ritorna il numero degli oggetti con quel Num associato
// Varianti: CercaPerNome e CercaPerValore possono accettare come argomento
//    opzionale un puntatore ad oggetto.
//    Vuol dire: continua la ricerca a partire da tale oggetto.
//----------------------------------------------------------------------------
// Classi derivate principali:
// Sono previste le classi derivate OGGETTO_View ed OGGETTO_Modello
// OGGETTO_View    : oggetti di tipo Presentation Manager (mappe, controlli)
// OGGETTO_Modello : oggetti del modello (esempio: Presa)
//----------------------------------------------------------------------------
// Classi da cui derivo:
//----------------------------------------------------------------------------
// Classi collegate:
// OGGETTO
//----------------------------------------------------------------------------

class _export ELENCO_Oggetti {


//++++++++++++++++++++++++++++++++++++++++++++++++++
// Area publica
//++++++++++++++++++++++++++++++++++++++++++++++++++
   public :

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Data area  (pubblica)
//++++++++++++++++++++++++++++++++++++++++++++++++++

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Costruttori e distruttori
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Costruttore di default

   ELENCO_Oggetti(unsigned int num,unsigned int max=32000);
   ELENCO_Oggetti();

// Costruttore di copia

   ELENCO_Oggetti(const ELENCO_Oggetti& a);

// Distruttore virtuale non di default

   virtual ~ELENCO_Oggetti();

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Conversioni
//++++++++++++++++++++++++++++++++++++++++++++++++++

   ELENCO_Oggetti& Sort( PFN_QSORT* Compare );

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Operatori ridefiniti
//++++++++++++++++++++++++++++++++++++++++++++++++++
// =  Assegnazione

   ELENCO_Oggetti& operator=(const ELENCO_Oggetti & A);

// []  Accede all' iesimo oggetto  (tornato per reference)
//     se l' indice e' una stringa fa' una ricerca PER NOME e poi accede

   class OGGETTO & operator[](unsigned int idx) const;
   class OGGETTO & operator[](const STRINGA & Target) const;
   class OGGETTO & operator[](const class OGGETTO * Target) const;

// +=  Aggiunge un oggetto alla lista

   ELENCO_Oggetti& operator+=(class OGGETTO& Ogg);

// +=  Aggiunge un elenco di oggetti alla lista
// NOTA: AGGIUNGE ANCHE I DATI ASSOCIATI (PER COPIA)

   ELENCO_Oggetti& operator+=(const ELENCO_Oggetti & Elenco);

// -=  Toglie un oggetto alla lista

   ELENCO_Oggetti& operator-=(const class OGGETTO& Ogg);
   ELENCO_Oggetti& operator-=(const class OGGETTO* Ogg) {return operator-=( *Ogg );};
   ELENCO_Oggetti& operator-=(const STRINGA& OggName);

// -=  Toglie un elenco di oggetti alla lista

   ELENCO_Oggetti& operator-=(const ELENCO_Oggetti & Elenco);

// -=  Toglie un elemento dalla lista

   ELENCO_Oggetti& operator-=(unsigned int Idx);

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Metodi    (pubblici)
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Numero di oggetti dell' elenco
   int Dim() const{return NumOggetti;};

// Puntatore all' oggetto dato il nome
   class OGGETTO * CercaPerNome(const STRINGA& Nome_da_cercare, const OGGETTO* Last=NULL ) const ;

// Puntatore all' oggetto dato il valore
   class OGGETTO * CercaPerValore(const STRINGA& Valore_da_cercare, const OGGETTO* Last=NULL ) const ;

// Torna l' indice dell' oggetto o -1 se non trovato
   int CercaPerPuntatore(const class OGGETTO * Oggetto_da_cercare) const;

// Contiene: vede se contiene l' Oggetto
   BOOL Contiene(const class OGGETTO* Ogg_da_cercare) const
   {return (CercaPerPuntatore(Ogg_da_cercare) >= 0);};

   BOOL Contiene(const STRINGA& Nome_da_cercare) const
   {return CercaPerNome(Nome_da_cercare) >= 0;};

// CercaPerTipo      :   Crea un elenco temporaneo in cui mette solo gli oggetti
//                       dell' elenco appartenenti al tipo indicato.

   ELENCO_Oggetti CercaPerTipo(TIPOGGE Tipo) const;

// CercaPerNumAssoc  :   Crea un elenco temporaneo in cui mette solo gli oggetti
//                       dell' elenco con associato il Num indicato.

   ELENCO_Oggetti CercaPerNumAssoc(USHORT Num) const;

// CercaNonAssoc  :   Crea un elenco temporaneo in cui mette solo gli oggetti
//                       dell' elenco senza dati associati a relazioni applicative

   ELENCO_Oggetti CercaNonAssoc() const;

// ContaPerNome      :   Ritorna il numero degli oggetti con quel nome

   USHORT ContaPerNome(const STRINGA& Nome_da_cercare) const ;

// ContaPerTipo      :   Ritorna il numero degli oggetti di quel tipo

   USHORT ContaPerTipo(TIPOGGE Tipo) const ;

// ContaPerNumAssoc  :   Conta ...

   USHORT ContaPerNumAssoc(USHORT Num) const;

// Fa' un trace degli oggetti contenuti

   void Trace(USHORT TrcLev = LIVELLO_DI_TRACE_DEL_PROGRAMMA ) const;

// Vuota l' elenco

   void Clear();

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Associa           :   Dato un puntatore ad oggetto associa a tale
//                       oggetto una copia di un' area dati
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Attenzione: ad ogni elenco NON e' possibile associare piu'
// di un tipo di DATI: debbono essere dedicati

// Questi sono i tipi di dato che e' possibile associare
// Nota: se il tipo e' < Generic non allocano un' altra area
// ma conservano direttamente il dato come un USHORT (Len MUST BE <= 2)

   enum TIPO_DATI_ASSOCIATI {Assc_UnDef=0,Lnk_Role,Relaz_ID,Generic};

// Mi dice il tipo di area associata

   TIPO_DATI_ASSOCIATI TipoAreaAssociata()const {return HoAssociati;};

// Metodi che associano dati generici (What >= Generic)
// Vero se NON ho trovato l' oggetto nell' elenco o su altri errori

   BOOL Associa(const class OGGETTO* Ogg, const void * Data, USHORT Len
     ,TIPO_DATI_ASSOCIATI What);

// Se ho gia' l' indice dell' oggetto

   BOOL Associa(int i, const void * Data, USHORT Len,TIPO_DATI_ASSOCIATI What);

// Metodi che associano dati USHORT (What <  Generic)
// Vero se NON ho trovato l' oggetto nell' elenco o su altri errori

   BOOL Associa(const class OGGETTO* Ogg, USHORT Num, TIPO_DATI_ASSOCIATI What);

// Se ho gia' l' indice dell' oggetto

   BOOL Associa(int i, USHORT Num, TIPO_DATI_ASSOCIATI What);

//++++++++++++++++++++++++++++++++++++++++++++++++++
// AreaAssociata     :   Dato un puntatore ad oggetto ritorna un puntatore
//                       all' area dati associata.
//                       Opzionalmente puo' anche ritornare la lunghezza
//                       dell' area associata
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Alternativa: accetta l' indice dell' oggetto nell' elenco

   void * AreaAssociata(const class OGGETTO * Ogg, USHORT * pLen = NULL)const ; //Null se non trovo l' oggetto o se NON ha area associata.
   void * AreaAssociata(unsigned int idx,USHORT * pLen=NULL)const;  //Null se non trovo l' oggetto o se NON ha area associata.

// I void * che sono ritornati dalle funzioni Precedenti possono anche essere
// la rappresentazione binaria degli USHORT corrispondenti.
// In tale caso e' meglio usare le seguenti due funzioni

   USHORT NumAssociato(const class OGGETTO* Ogg)const ; // 0 se non trovo l' oggetto o se NON ha Dato associato.
   USHORT NumAssociato(unsigned int idx)const;          // 0 se non trovo l' oggetto o se NON ha Dato associato.

// Questa funzione mi ritorna un elenco temporaneo con tutti gli oggetti
// che hanno associato un dato

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Area protected
//++++++++++++++++++++++++++++++++++++++++++++++++++
   protected  :

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Data area  (protect)
//++++++++++++++++++++++++++++++++++++++++++++++++++
   unsigned int NumAlloc;            // Dimensione area allocata nell' array
   unsigned int NumOggetti;          // Numero di oggetti dell' elenco
   unsigned int MaxOggetti;          // Numero massimo di oggetti dell' elenco
   class OGGETTO * * ArrayOggetti;   // Array di oggetti dell' elenco
   TIPO_DATI_ASSOCIATI HoAssociati;  // Vero se ho dati associati.
   union
   {                                 // Possibili alternative per i dati associati:
      USHORT * ArrayNums;            // Array di USHORT associati agli oggetti.
      L_BUFFER * ArrayAssociati;     // Array di dati associati agli oggetti.
   };
   OGGETTO*  Singolo;                // Per evitare allocazioni dinamiche fino ad un solo oggetto
   
   public:
   char Filler[8];


};

#endif
