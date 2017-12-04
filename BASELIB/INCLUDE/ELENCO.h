#include "dummy.h"
//----------------------------------------------------------------------------
// FILE ELENCO.H
//----------------------------------------------------------------------------
// Definisce le classi:
// ELENCO                 // Elenco dinamico di puntatori
// DIN_ARRAY              // Template: array dinamica di qualsiasi oggetto
// Utilizzata per definire subito:
// DIN_ARRAY_LONG         // Subclass: array dinamica di LONG
// DIN_ARRAY_USHORT       // Analoga : array dinamica di USHORT
// DIN_ARRAY_INT          // Analoga : array dinamica di INT
// ELENCO_EL_S            // Elenco di elenchi di stringhe (ALLOCAZIONE  INTERNA)
//----------------------------------------------------------------------------


#ifndef HO_ELENCO_H
#define HO_ELENCO_H  // Indica che questo file e' stato incluso

#ifndef HO_STD_H      // Includo sempre le librerie standard del progetto
#include <std.h>
#endif

#ifndef HO_MYALLOC_H      // Includo sempre le librerie standard del progetto
#include <myalloc.h>
#endif


// #ifndef BEEP
// #define BEEP {Beep(__FILE__,__LINE__,LIVELLO_DI_TRACE_DEL_PROGRAMMA); }
// void _export Beep(char* File,int Line,USHORT TLev);
// #endif

//----------------------------------------------------------------------------
// Classe ELENCO
//----------------------------------------------------------------------------
// Questa classe definisce un elenco di puntatori a PVOID
//----------------------------------------------------------------------------
// NOTA: Deve essere garantito che l' indice dei puntatori
// segua l' ordine di inserzione: cioe' che PVOID inseriti dopo
// occupino un indice > di quelli inseriti prima, e che l' indice
// non cambi se i PVOID sono cancellati a partire dall' ultimo.
// Cio' permette di scandire semplicemente la lista
// (In ordine diretto in creazione ed inverso in cancellazione);
//----------------------------------------------------------------------------
// Operazioni ammesse:
//
//   Creazione dando opzionalmente numero di puntatori da allocare subito,
//    numero MASSIMO di puntatori che possono comparire nella lista
//    (se si supera ABEND);
//   costruttore di copia (copia i puntatori: potrebbe essere inappropriato)
//   distruttore (virtuale)
// []  Accede all' iesimo PVOID  (tornato per reference !)
// +=  Aggiunge un PVOID alla lista
// -=  Toglie un PVOID dalla lista
// Elimina : Come sopra, ma si basa sulla posizione
// Last  : Accede all' ultimo elemento
// Notare che non vi sono metodi per aggiungere o togliere una lista da una lista
//----------------------------------------------------------------------------
// Metodi previsti:
// Dim               :   Numero di PVOID dell' elenco
// CercaPerPuntatore :   Ritorna l' indice del PVOID nell' array oppure
//                       -1 se non e' trovato .
// ContaPerPuntatore :   Ritorna il numero degli oggetti con quel Puntatore
//----------------------------------------------------------------------------

class _export ELENCO{


//++++++++++++++++++++++++++++++++++++++++++++++++++
// Area publica
//++++++++++++++++++++++++++++++++++++++++++++++++++
   public :

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Costruttore di default
//++++++++++++++++++++++++++++++++++++++++++++++++++
   ELENCO(unsigned int num  ,unsigned int max=32000);
   ELENCO();

// Costruttore di copia

   ELENCO(const ELENCO& a);

// Distruttore virtuale non di default
   virtual ~ELENCO();

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Operatori ridefiniti
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Operatore di assegnazione

   ELENCO& operator=(const ELENCO& A);

// []  Accede all' iesimo PVOID  (tornato per reference)

   PVOID& operator[](unsigned int idx) const;

// +=  Aggiunge un PVOID alla lista
// +   Aggiunge un PVOID alla lista  (e' lo stesso operatore di +=)

   ELENCO& operator+=(PVOID Ogg);
   ELENCO& operator+(PVOID Ogg);

// -=  Toglie un PVOID alla lista
// -   Toglie un PVOID alla lista  (e' lo stesso operatore di -=)
// Elimina : Come sopra, ma si basa sulla posizione

   ELENCO& operator-=(PVOID Ogg);
   ELENCO& operator-(PVOID Ogg);

   ELENCO& Elimina(int pos);

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Metodi    (pubblici)
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Numero di PVOID dell' elenco

   int Dim() const {
      return NumPVOID;};

// Torna l' indice dell' PVOID o -1 se non trovato

   int CercaPerPuntatore(const void * PVOID_da_cercare) const ;

// Idem ma permette di avere puntatori duplicati

   int CercaPerPuntatore(const void * PVOID_da_cercare,int LastMatch) const ;

// Ritorna il numero degli oggetti con quel Puntatore

   int ContaPerPuntatore(const void * PVOID_da_contare) const ;

// Vuota l' elenco

   void Clear();

// Fa' un trace dei PVOID contenuti

   void Trace(USHORT TrcLev = LIVELLO_DI_TRACE_DEL_PROGRAMMA ) const;

// Last      : Accede all' ultimo elemento

   PVOID& Last(){return THIS[THIS.Dim()-1];};

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Area protected
//++++++++++++++++++++++++++++++++++++++++++++++++++
   protected  :

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Data area  (protect)
//++++++++++++++++++++++++++++++++++++++++++++++++++

   unsigned int NumAlloc;       // Dimensione area allocata nell' array
   unsigned int NumPVOID;       // Numero di PVOID dell' elenco
   unsigned int MaxPVOID;       // Numero di PVOID dell' elenco
   PVOID * ArrayPVOID;         // Array di PVOID dell' elenco


//++++++++++++++++++++++++++++++++++++++++++++++++++
// Metodi    (protect)
//++++++++++++++++++++++++++++++++++++++++++++++++++

   public:
   char Filler[8];


//<<< class ELENCO{
};

//----------------------------------------------------------------------------
// Definizioni preimpostate
//----------------------------------------------------------------------------
// uso le macro invece delle template per lavorare sotto 2.0
//    DIN_ARRAY       = Nome che si vuol dare alla classe
//    CLAS            = Classe di cui faccio l' elenco
// le seguenti definizioni sono opzionali
//    CastFrom(PVOID) = Macro di cast da CLAS a PVOID
//    CastTo(CLAS)    = Macro di cast da PVOID a CLAS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// DIN_ARRAY_OF_LONG
//----------------------------------------------------------------------------
#define DIN_ARRAY DIN_ARRAY_OF_LONG
#define CLAS LONG
#include <dinarray.h>

//----------------------------------------------------------------------------
// DIN_ARRAY_OF_USHORT
//----------------------------------------------------------------------------
#define DIN_ARRAY DIN_ARRAY_OF_USHORT
#define CLAS USHORT
#define CastFrom(_ushort) PVOID(LONG(_ushort))
#include <dinarray.h>

//----------------------------------------------------------------------------
// DIN_ARRAY_OF_INT
//----------------------------------------------------------------------------
#define DIN_ARRAY DIN_ARRAY_OF_INT
#define CLAS int
#define CastFrom(_int)   PVOID(LONG(_int))
#include <dinarray.h>

//----------------------------------------------------------------------------
// ELENCO_EL_S
//----------------------------------------------------------------------------
// Classe per gestire gli elenchi di ELENCO_S
// La classe definita e' ELENCO_EL_S : elenco di puntatori ad ELENCO_S
// i puntatori debbono essere aggiunti SOLO con un NEW, perche' alla
// Clear() o al distruttore gli elenchi vengono distrutti automaticamente
//----------------------------------------------------------------------------
// Esempio di uso in compila.c
// typedef ELENCO_S * P_ELENCO_S;
// #define DIN_ARRAY ELENCO_EL_S
// #define CLAS P_ELENCO_S
// #define DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
// #include <dinarray.h>

#endif
