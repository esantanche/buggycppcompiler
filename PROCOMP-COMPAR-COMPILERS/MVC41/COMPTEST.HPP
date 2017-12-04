//
// Project Folder
// Progetto: SIPAX
//

#include "winport.hpp"

typedef unsigned long BIT;

// testare il supporto del segno
struct CLUSSTAZ {            // PathOrario + "CLUSSTAZ.EXT"
   BIT Fermata           :1; // Vero se almeno un treno vi ferma
   BIT ClasseCoincidenza :2; // Classi di coincidenza: Di norma:
                             // 0 =   Normale (10 Minuti )
                             // 1 =   Esteso  (15 Minuti )
                             // 2,3 Attualmente non definite.
                             // I tempi effettivi dipendono dal livello di ricerca
   BIT       IdNodo      :13;// Id della stazione.
   short int MediaKm     :13;// DA PROVARE ANCHE COME signed short int MediaKm : 13
   int       Gruppi         ;// Per filtraggio soluzioni

};

class _export STAZIONI { // Accesso per Id o per codice CCR
   public:

   struct R_STRU {
      char dummy;
      int Metodo(int Parametro);
   };

   STAZIONI();
   virtual ~STAZIONI();

   static int iDato_stazioni;

   friend class R_STRU;

};

class _export STAZIONI2 { // Accesso per Id o per codice CCR
   public:

   struct R_STRU {
      char cDummy2;
      int  iDummy2;
   };

   STAZIONI2();
   virtual ~STAZIONI2();

   friend class R_STRU;

};

// test per il problema di compilazione consistente nel dover
// specificare ARRAY_DINAMICA<ELEMENT> internamente alla classe
template <class ELEMENT>
class  ARRAY_DINAMICA {

   public:

   ARRAY_DINAMICA & operator=  (const ARRAY_DINAMICA & From)
         {return (ARRAY_DINAMICA &)From;};

   // Basic
   ARRAY_DINAMICA(ARRAY_DINAMICA & From) { 
      *this = From;
   };

   ARRAY_DINAMICA(){};

};

ARRAY_DINAMICA<STAZIONI> prova_array_dinamica;

// prova per problema isolato di non dichiarabilità di
// un metodo di una classe in un'altra classe friend
template <class ITEM , int D=3 >
class  HEAP {

   public:

   static int Confronta(ITEM* a,ITEM*b);

   HEAP(int NumElAll){             // Crea la Heap Vuota

   };

   protected:

   ITEM ** Nd;

   friend class CACHE<class ELEMENT>;

};

template <class ELEMENT>
class  CACHE {

   protected:

   struct CACHE_ELEMENT : public ELEMENT {
      DWORD     UsagePrg;
      WORD      HeapPos ;
      short int LockCount;
   };

   public:

   CACHE(int NumElementiMax, BYTE KLen) {
   };

   protected:

   friend class HEAP<class ITEM>;

   // EMS014 Win devo definire esternamente la funzione HEAP<CACHE_ELEMENT>::Confronta
   friend int HEAP<CACHE_ELEMENT>::Confronta(CACHE_ELEMENT * a,CACHE_ELEMENT * b){
      if(a->LockCount != b->LockCount) return a->LockCount - b->LockCount;
      return (int)(a->UsagePrg)  - (int)(b->UsagePrg);
   };

   int  CacheSize       ;
   int  InitialCacheSize;
   BYTE KeyLen          ;

   DWORD NextPrg;

};

struct PROVA_ELEMENT {
   int Prova1;
};

CACHE<PROVA_ELEMENT> prova_cache(10,2);

typedef void * PVOID;
// MVC typedef char * PBYTE;

// istruzione non compilabile ?
// MVC #if sizeof( CLUSSTAZ ) != sizeof( PVOID )
// MVC #endif


// prova della define Chk
// forse questo era per prove per cui dovrebbe funzionare
typedef unsigned int UINT;
#define Chk(_a,_b)  Chk2(_a,_b,__LINE__)
//#define Chk(_a,_b)  (_a)
#define ChkS(_a,_b)  Chk3(_a,_b,__LINE__)
void _export ChkSegnala(UINT i, UINT n,int Linea);
inline UINT Chk2(UINT i, UINT n,int Linea){
   if (i >= (1 << n))ChkSegnala(i,n,Linea);
   return i;
}
inline int  Chk3(int  i, UINT n,int Linea){
   if ((i>=0 ? i:-i) >= (1 << (n-1)))ChkSegnala((i>=0 ? i:-i) ,n-1,Linea);
   return i;
}

// prova incremento enum
enum EPROVA {
   ZERO,
   UNO,
   DUE
};

typedef unsigned short USHORT;

// per vedere se min e max sono gia' definite
inline const int _export  min(int a,int b)
{
   if(a>b)return b;
   return a;
}

inline const USHORT _export min(USHORT a,USHORT b)
{
   if(a>b)return b;
   return a;
}

inline const long  _export min(long a,long b)
{
   if(a>b)return b;
   return a;
}

inline const int  _export max(int a,int b)
{
   if(a<b)return b;
   return a;
}

