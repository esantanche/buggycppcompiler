#include "dummy.h"
// #define NO_STRINGA_INLINE

//----------------------------------------------------------------------------
// FILE STRINGA.H
//----------------------------------------------------------------------------
// Definisce la classe STRINGA
//
//
//----------------------------------------------------------------------------
// Viene normalmente incluso da OGGETTI.H
//----------------------------------------------------------------------------


#ifndef HO_STRINGA_H
#define HO_STRINGA_H   // Indica che questo file e' stato incluso

#ifndef HO_STD_H      // Includo sempre le librerie standard del progetto
#include <std.h>
#endif

#include <values.h>   // Per MAXINT MAXLONG MAXFLOAT MAXDOUBLE

//----------------------------------------------------------------------------
// Classe Stringa
//----------------------------------------------------------------------------
// Definisce un tipo stringa che puo' contenere dei valori di tipo
// carattere o altro.
// I valori possono essere di altro tipo, ad esempio numerici, float o data,
// e vengono gestiti da apposite sottoclassi della classe STRINGA.
// Quindi tutti i valori sono rappresentati internamente come una stringa.
//----------------------------------------------------------------------------
// La classe STRINGA e' case sensitive nei confronti
//----------------------------------------------------------------------------
// Operazioni ammesse:
//
// Creazione come stringa vuota
// Creazione come copia di una array di caratteri (char *) e conversione
// Creazione come copia di una stringa
// Creazione come conversione di un char in formato stringa
// Creazione come conversione di un long in formato stringa
// Creazione come conversione di un double in formato stringa
// Creazione come conversione di un unsigned long in formato stringa
// Creazione come copia di un range di caratteri di un' altra stringa
// Conversione della stringa in una array costante di caratteri
// Concatenazione di due stringhe                        Operatori + +=
// Concatenazione di un carattere ad una stringa         Operatori + +=
// Copia di una stringa su di un' altra                  Operatore =
// Paragone di due stringhe                              Operatori < == > <= >=
// Elemento ennesimo della stringa                       Operatore []
// Sottostringa                                          Operatore (Primo,Ultimo)
// Separazione di una stringa in piu' stringhe (alla strtok)
// Ricerca di una sottostringa
// Distruttore
//----------------------------------------------------------------------------
// Metodi previsti:
// Clear     : vuota la stringa (ma senza distruggerla)
// Dim       : Dimensione della stringa
// Store     : Aggiunge la stringa alla fine di una struttura di salvataggio
//             su Disco.
// ReStore   : Ricostruisce una stringa a partire dalla struttura di salvataggio
// Tokens    : Separazione di una stringa in piu' stringhe (alla strtok)
// Pos       . Ricerca di una sottostringa (-1 se non trovata, o primo carattere)
// Strip     : Elimina spazi e Tabs iniziali e finali, converte sequenze di
//             spazi e Tabs in un singolo spazio
// UpCase    : Imposta tutte maiuscole
// LoCase    : Imposta tutte minuscole
// Last      : Accede all' ultimo carattere
//----------------------------------------------------------------------------
// Classi derivate principali:
//----------------------------------------------------------------------------
// Classi da cui derivo:
//----------------------------------------------------------------------------
// Classi collegate:
//----------------------------------------------------------------------------
class _export STRINGA {

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
    STRINGA();                       // stringa vuota
    STRINGA(const char *a);          // copia di una array di caratteri (char *) e conversione
    STRINGA(const STRINGA& a);       // copia di una stringa
    STRINGA(const char a);           // conversione di un char  in formato stringa
    STRINGA(const int  a);           // conversione di un int   in formato stringa
    STRINGA(const short a);          // conversione di un short in formato stringa
    STRINGA(const long a);           // conversione di un long  in formato stringa
    STRINGA(const double a);         // conversione di un double in formato stringa
    STRINGA(const unsigned int  a);  // conversione di un unsigned int   in formato stringa
    STRINGA(const unsigned short a); // conversione di un unsigned short in formato stringa
    STRINGA(const unsigned long a);  // conversione di un unsigned long  in formato stringa
    STRINGA(void * Ptr );            // conversione di un void * in formato stringa Hex
    virtual ~STRINGA();                     // Distruttore

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Conversioni da STRINGA a.........
//++++++++++++++++++++++++++++++++++++++++++++++++++
// area a lunghezza fissa ( per COBOL)
   void ToFix(char * Dest, USHORT Leng, char Riemp=' ') const;

// Riempie con ch la parte finale della stringa
   STRINGA& Pad(  USHORT Len, char ch = ' ');

// Riempie con ch la parte iniziale della stringa
   STRINGA& RPad( USHORT Len, char ch = ' ');
   STRINGA& Rovescia();

   STRINGA Prt() const;                        // formato "stringa" con escape sequence
                                               // utilizzata per stampare le stringhe
   operator const char *() const;              // ad array di char
   void * ToPtr() const;                       // da stringa Hex a puntatore
   double ToDouble(BOOL * Rounded=NULL) const; // a double (0 se fallisce)
   float  ToFloat (BOOL * Rounded=NULL) const; // a float (0 se fallisce)
   LONG   ToLong  (BOOL * Rounded=NULL) const; // a long (0 se fallisce)
   int    ToInt   (BOOL * Rounded=NULL) const; // ad int  (0 se fallisce)

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Operatori ridefiniti
//++++++++++++++++++++++++++++++++++++++++++++++++++
// RICORDA !
// Gli operatori definiti come funzioni friend sono
// commutativi e permettono conversioni automatiche,
// gli altri no
// (permettono la conversione solo sul secondo argomento)
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Concatenazione di due stringhe                        Operatori + +=

    friend STRINGA _export operator+ (const STRINGA& a,const STRINGA& b);
    STRINGA& operator+= (const STRINGA& b);

// Concatenazione di un carattere ad una stringa         Operatori + +=

    friend STRINGA _export operator+ (const STRINGA& a,const char b);
    friend STRINGA _export operator+ (const STRINGA& a,const char *b);
    friend STRINGA _export operator+ (const char *b,const STRINGA &a);
    STRINGA& operator+= (const char b);

// Copia di una stringa o di un carattere su di un' altra Operatore =

    STRINGA& operator= (const STRINGA& b);
    STRINGA& operator= (const char * b);

// Paragone di due stringhe                              Operatori < == > <= >=
    friend BOOL _export operator< (const STRINGA& a,const STRINGA& b);
    friend BOOL _export operator> (const STRINGA& a,const STRINGA& b);
    friend BOOL _export operator<=(const STRINGA& a,const STRINGA& b);
    friend BOOL _export operator>=(const STRINGA& a,const STRINGA& b);
    friend BOOL _export operator==(const STRINGA& a,const STRINGA& b);
    friend BOOL _export operator==(const char *   a,const STRINGA& b); // Di uso molto frequente
    friend BOOL _export operator==(const STRINGA& a,const char *   b); // Di uso molto frequente
    friend BOOL _export operator!=(const STRINGA& a,const STRINGA& b);

// Questa funzione puo' essere utilizzata per confrontare due
// stringhe in operazioni di sort o simili.
// Ritorna >=< 0 a seconda che A >=< B
// Per tabelle di stringhe

    friend int cdecl _export STRINGACmp(STRINGA & A , STRINGA & B);
    friend int _export PSTRINGACmp(const void * A , const void * B);

// Elemento ennesimo della stringa                       Operatore []
    char operator[](unsigned int Pos)const;

    //char & operator[](unsigned int Pos){ return operator[]((unsigned long)Pos)};
    char & operator[](unsigned int Pos);

// Sottostringa
    STRINGA operator()(int Primo, int Ultimo)const;

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Metodi    (pubblici)
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Clear     : vuota la stringa (ma senza distruggerla)
    void Clear();

// Dim       : Dimensione della stringa
   const USHORT Dim()const {return (this == NULL ? (USHORT)0 : Len);};

// Store     : Aggiunge la stringa alla fine di una struttura di salvataggio
//             su Disco.
// Allocato puo' essere aggiornato (Per riallocazione)
   void Store(INI_BUFFER & Save)const;

// ReStore   : Ricostruisce una stringa a partire dalla struttura di salvataggio
// e la imposta nella stringa corrente.
   void ReStore(INI_BUFFER& Save);

// Separazione di una stringa in piu' stringhe (alla strtok ma senza modificare)
   class ELENCO_S Tokens(const STRINGA& Delimitatori,
                         class ELENCO_S * Seps = NULL)const;

// Ricerca di una sottostringa (-1 se non trovata, o primo carattere)
   int   Pos(const STRINGA& SubStr) const;

// Strip     : Elimina spazi e Tabs iniziali e finali, converte sequenze di
//             spazi e Tabs in un singolo spazio
   STRINGA & Strip();

// UpCase    : Imposta tutte maiuscole
   STRINGA & UpCase();

// LoCase    : Imposta tutte minuscole
   STRINGA & LoCase();

// Last      : Accede all' ultimo carattere
   char Last() const;

   // EMS001 MVC sostituita invocazione operatore []
   //char Last(){return (Len == 0 ? '\0' : this->operator[](Len-1));};
   char Last(){return (Len == 0 ? '\0' : this->Dati[Len-1]);};

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Area privata
//++++++++++++++++++++++++++++++++++++++++++++++++++
//   private  :
// Modifica momentanea;

protected :
   USHORT Len;   // Max 64 k
   char * Dati;
#define STRINGA_AUTO_ALLOC_DIM 34
   char DatiDiretti[STRINGA_AUTO_ALLOC_DIM];
   STRINGA(const char *a,USHORT Leng); // costruttore veloce per le funzioni interne
   STRINGA(const char *a,USHORT Lenga,const char* b,USHORT Lengb); // costruttore veloce per le funzioni interne
   friend class ELENCO_S;
   friend class ELENCO_Oggetti;

   public:
   char Filler[8];


};                      // Fine classe stringa

// -----------------------------------------------------------------
// Funzioni inline per velocizzare
// -----------------------------------------------------------------
#ifndef NO_STRINGA_INLINE
// Distruttore
inline STRINGA::~STRINGA()
{ if(Len >= STRINGA_AUTO_ALLOC_DIM) free(Dati); }
// Costruttore di default
inline STRINGA::STRINGA() { Len = 0; Dati = DatiDiretti; *Dati = '\0'; }
#endif

#endif

