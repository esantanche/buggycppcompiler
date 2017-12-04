#include "dummy.h"
//----------------------------------------------------------------------------
// FILE ELENCO_S.H
//----------------------------------------------------------------------------
// °°°°°°°°°°°°°°°°°°°°°°°°° REPORT AGGIORNAMENTI °°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Data ultima modifica documentazione: 20-10-92

// Data ultima modifica codice        :   -  -92

// °°°°°°°°°°°°°°°°°°°°°°°° DESCRIZIONE DELLA CLASSE °°°°°°°°°°°°°°°°°°°°°°°°°
//
// Questa classe definisce un elenco di Stringhe.
// E' implementata mediante un' array di puntatori a Stringhe.
// L'indice del primo elemento e' 0 (zero)
// NOTA: Deve essere garantito che l' indice degli oggetti
// segua l' ordine di inserzione: cioe' che oggetti inseriti dopo
// abbiano un indice maggiore di quelli inseriti prima, e che l' indice
// non cambi se gli oggetti sono cancellati a partire dall' ultimo.
// Cio' permette di scandire semplicemente la lista
// (In ordine diretto in creazione ed inverso in cancellazione);

#ifndef HO_ELENCO_S_H
#define HO_ELENCO_S_H  // Indica che questo file e' stato incluso

#ifndef HO_STD_H       // Includo sempre le librerie standard del progetto
#include <std.h>
#endif


//#ifndef BEEP
//#define BEEP {Beep(__FILE__,__LINE__,LIVELLO_DI_TRACE_DEL_PROGRAMMA); }
//void _export Beep(char* File,int Line,USHORT TLev);
//#endif

// °°°°°°°°°°°°°°°°°°°°°°° DICHIARAZIONE DELLA CLASSE °°°°°°°°°°°°°°°°°°°°°°°°

class _export ELENCO_S {

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°° AREA PUBBLICA °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   public :

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° METODI °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   ELENCO_S(unsigned int num, unsigned int max=32000);

             // Costruttore di default
             // il parametro num indica il numero di elementi da allocare
             // alla creazione dell'elenco, max e' il numero massimo di
             // elementi di cui puo' essere costituito l'elenco, se si
             // supera tale numero il programma va in abend.

   ELENCO_S();


   ELENCO_S(const STRINGA& s1,const STRINGA& s2=NUSTR,const STRINGA& s3=NUSTR
     ,const STRINGA& s4=NUSTR,const STRINGA& s5=NUSTR,const STRINGA& s6=NUSTR
     ,const STRINGA& s7=NUSTR,const STRINGA& s8=NUSTR,const STRINGA& s9=NUSTR
     ,const STRINGA& s0=NUSTR);

             // Crea un elenco con le stringhe specificate come parametri,
             // al massimo possono essere 10.


   ELENCO_S(const ELENCO_S& a);

             // Crea un elenco di stringhe copiando da un'altro specificato.


   virtual ~ELENCO_S();

             // Distruttore virtuale.


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°° OPERATORI °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   ELENCO_S & operator = (const ELENCO_S & A);

             // =  Operatore di assegnazione
             // assegna l'elenco A all'elenco.


   ELENCO_S& operator+=(const STRINGA& Strg);

             // += aggiunge una stringa in coda all'elenco



   ELENCO_S& operator+=(const ELENCO_S& Elenco);

             // += aggiunge in coda all'elenco un altro elenco di stringhe


   ELENCO_S& operator+=(const char *  Strg);

             // += aggiunge all'elenco una stringa passata come char *


   ELENCO_S& operator-=(const STRINGA& Strg);

             // -= elimina dall'elenco la stringa Strg se esiste altrimenti
             // l'elenco rimane immodificato


   ELENCO_S& operator-=(int Posizione);

             // -=  elimina dall'elenco la stringa che occupa la posizione
             // specificata dal valore Posizione


   const STRINGA& operator[](unsigned int idx) const;

   STRINGA& operator[](unsigned int idx);

             // []  ritorna per reference la stringa con posizione specificata
             // tra parentesi idx.


   const STRINGA& operator[](const STRINGA & Target) const;

   STRINGA& operator[](const STRINGA & Target) ;

             // []  ritorna per reference la stringa uguale a Target contenuta
             // nell'elenco se esiste, altrimenti ritorna una stringa nulla.


   INLINE ELENCO_S operator()(UINT Primo,UINT Ultimo)const;

             // () ritorna il sottoelenco compreso tra gli estremi
             // specificati: Primo ed Ultimo.


 // °°°°°°°°°°°°°°°°°°°°°°°°°°°° METODI PUBBLICI °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   int Dim() const{return NumStringhe;};

             // ritorna il numero di Stringhe contenute nell'elenco

// La funzione e' poco usata e la gestisco con una scansione
   ULONG BytesAlloc();

   void Trace(USHORT TrcLev = LIVELLO_DI_TRACE_DEL_PROGRAMMA )const;

             // scrive nel file di trace le stringhe contenute nell'elenco


   void Store(INI_BUFFER & Save) const;

             // aggiunge l'elenco alla fine di un'area salvataggio Save
             // (buffer).
             // Allocato puo' essere aggiornato (Per riallocazione)
             // ATTENZIONE!!! la definizione del metodo e' nel file TOKENS.C !!!


   void ReStore(INI_BUFFER& Save);

             // ricostruisce un elenco a partire dall'area di salvataggio Save
             // Se l' elenco non e' vuoto viene vuotato
             // Offset viene aggiornato
             // ATTENZIONE!!! la definizione del metodo e' nel file TOKENS.C !!!


   void Clear();

             // elimina tutte le stringhe contenute nell'elenco
             // senza distruggere l'elenco


   ELENCO_S Strip();

             // elimina tutte le stringhe con Dim = 0 contenute nell'elenco
             // ritornando un altro elenco con stringhe piene

   int CercaPerNome(const STRINGA& Nome_da_cercare, int Last=-1) const ;

             // ricerca la stringa Nome_da_cercare nell'elenco,
             // se viene trovata ritorna la sua posizione, altrimenti
             // ritorna -1.


   BOOL Contiene(const STRINGA& Nome_da_cercare) const ;

             // ritorna TRUE o FALSE se trova, o meno, la stringa nell'elenco


   ELENCO_S & Sort();

             // ordina le stringhe contenute nell'elenco


   STRINGA& Last(){return *(ArrayStringhe[NumStringhe-1]);}

             // ritorna l'ultima stringa dell'elenco


   STRINGA Tokenize(char Separator);

             // converte l'elenco in una stringa in cui ogni stringa e'
             // separata dalla sucessiva dal separatore Separator.

   BOOL SetInsPoint(long Indice);

             // permette di settare il punto di inserimento delle stringa
             // che verr… successivamente inserita invocando l'operatore +=.

   long GetInsPoint(){return InsPoint;}

             // Ritorna l'attuale valore di InsPoint.

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°° AREA PROTETTA °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   protected  :

// °°°°°°°°°°°°°°°°°°°°°°°°°° DATI AREA PROTETTA °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
    
   unsigned int NumAlloc;        // Dimensione area allocata nell'array

   unsigned int NumStringhe;     // Numero di stringhe contenute nell'elenco

   unsigned int MaxStringhe;     // Numero massimo di stringhe inseribili

   STRINGA * * ArrayStringhe;    // Array di Stringhe dell'elenco

   long InsPoint;                // Indice in cui verr… inserito il prossimo

   public:
   char Filler[8];

                                 // item tramite operatore +=
};

#endif
