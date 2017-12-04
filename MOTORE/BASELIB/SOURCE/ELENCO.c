//----------------------------------------------------------------------------
// FILE ELENCO.C
//----------------------------------------------------------------------------
// Definizioni di funzioni per le classi:
// ELENCO
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  6

#include "elenco.h"

#include "minmax.h"
#include "myalloc.h"

TRACEVARIABLES

//----------------------------------------------------------------------------
// Classe ELENCO
//----------------------------------------------------------------------------
// Questa classe definisce un elenco di PVOID.
// E' implementata mediante un' array di puntatori
ELENCO::ELENCO(){ NumPVOID = NumAlloc = 0;MaxPVOID = 32000; ArrayPVOID = NULL; };

//----------------------------------------------------------------------------
// Costruttore di default
//----------------------------------------------------------------------------
ELENCO::ELENCO(unsigned int num,unsigned int Max)
{
#undef TRCRTN
#define TRCRTN "@ELENCO::ELENCO"
   NumPVOID = 0;
   if(Max == 0)Max = 32000;
   MaxPVOID = Max;
   NumAlloc = num;
   if(NumAlloc > 0)
   {
      ArrayPVOID = new PVOID [NumAlloc];
      MEMIGNORE(ArrayPVOID);
      memset(ArrayPVOID, '\0', NumAlloc * sizeof(PVOID *));
   }
   else
      ArrayPVOID = NULL;
   TRACEPOINTER("Creato Elenco PVOID ", this);
   TRACEPOINTER("Indirizzo array: ", this->ArrayPVOID);
}

//----------------------------------------------------------------------------
// Costruttore di copia : Crea un nuovo elenco uguale a quello indicato
//----------------------------------------------------------------------------
ELENCO::ELENCO(const ELENCO & A)
{
   NumPVOID = NumAlloc = 0;  // Init dimensioni
   * this = A;               // Assegnazione
}

//----------------------------------------------------------------------------
// Operatore = Assegna a questo elenco, l'elenco indicato
//----------------------------------------------------------------------------
ELENCO & ELENCO::operator=(const ELENCO & A)
{
#undef TRCRTN
#define TRCRTN "@ELENCO::operator="
   Clear();                 // Elimino gli eventuali valori precedenti
   NumPVOID = A.NumPVOID;
   MaxPVOID = A.MaxPVOID;
   NumAlloc = A.NumAlloc;
   if(NumAlloc > 0)
   {
      ArrayPVOID = new PVOID [NumAlloc];
      MEMIGNORE(ArrayPVOID);
      memcpy(ArrayPVOID, A.ArrayPVOID, NumAlloc * sizeof(PVOID *));
   }
   else
      ArrayPVOID = NULL;
   TRACEPOINTER("Copiato Elenco PVOID ",&A);
   TRACEPOINTER("Copiato su",this);
   return * this;
}

//----------------------------------------------------------------------------
// Operatore += Aggiunge all'elenco un nuovo PVOID
//----------------------------------------------------------------------------
ELENCO & ELENCO::operator+=(PVOID Ogg)
{
#undef TRCRTN
#define TRCRTN "ELENCO::operator+="
   int const Delta = max(30, NumAlloc/5);
   TRACEPOINTER("Elenco PVOID ",this);
   if(NumAlloc <= NumPVOID)
   {
      PVOID * * Nuovo =new PVOID * [NumAlloc + Delta];
      TRACEPOINTER("Indirizzo Nuova array: ",Nuovo);
      if(NumAlloc>0)
      {
         TRACEPOINTER("Cancello array: ", this->ArrayPVOID);
         memcpy(Nuovo, ArrayPVOID, NumAlloc * sizeof(PVOID *));
         memset(Nuovo + NumAlloc, 0, Delta * sizeof(PVOID *));
         delete ArrayPVOID;
      }
      memset(Nuovo + NumAlloc, 0, Delta * sizeof(PVOID *));
      ArrayPVOID = (PVOID *)Nuovo;
      MEMIGNORE(ArrayPVOID);
      NumAlloc += Delta;
      TRACELONG("Portata dimensione (NumAlloc) a ",NumAlloc);
// Se supero  MaxPVOID Abend
      if(NumAlloc > MaxPVOID)
         Abend(3);
   }
   this->ArrayPVOID[NumPVOID] = Ogg;
   NumPVOID++;
   TRACEPOINTER("Aggiunto PVOID ", Ogg);
   TRACEVLONG(NumPVOID);
   return * this;
}


//----------------------------------------------------------------------------
// Operatore -= Elimina dall'elenco il PVOID indicato
//----------------------------------------------------------------------------
ELENCO & ELENCO::operator-=(PVOID Ogg)
{
#undef TRCRTN
#define TRCRTN "ELENCO::operator-="
   TRACEPOINTER("Elenco PVOID ",this);
   int pos=CercaPerPuntatore(Ogg);
   if(pos >= 0 && pos < NumPVOID){
      if(pos < NumPVOID-1)memmove(& ArrayPVOID[pos], & ArrayPVOID[pos + 1], (NumPVOID - pos - 1) * sizeof(PVOID *));
      NumPVOID --;
      TRACEPOINTER("Eliminato PVOID ",Ogg);
      TRACEVLONG(NumPVOID);
   }
   return * this;
}

//----------------------------------------------------------------------------
// Elimina : Elimina (se esiste) dall'elenco l'elemento corrispondente
// all'indice specificato.
// Non utilizzo un overload dell' operatore -= perche' puo' creare problemi
// nelle subclass di ELENCO
//----------------------------------------------------------------------------
ELENCO & ELENCO::Elimina(int pos)
{
   TRACEPOINTER("Elenco PVOID ", this);
   if(pos >= 0 && pos < NumPVOID){
      if(pos < NumPVOID-1) memmove(& ArrayPVOID[pos], & ArrayPVOID[pos + 1], (NumPVOID - pos - 1) * sizeof(PVOID *));
      NumPVOID --;
   }
   return * this;
}

//----------------------------------------------------------------------------
// Clear : Elimina tutti gli elementi contenuti nell'elenco (ma non distrugge
// l'elenco)
//----------------------------------------------------------------------------
void ELENCO::Clear()
{
#undef TRCRTN
#define TRCRTN "ELENCO::Clear()"
   if(NumAlloc > 0)
      delete ArrayPVOID;
   NumPVOID = NumAlloc = 0;
}

//----------------------------------------------------------------------------
// Distruttore di un elenco PVOID
//----------------------------------------------------------------------------
ELENCO::~ELENCO()
{
#undef TRCRTN
#define TRCRTN "~ELENCO()"
   TRACEPOINTER("Distruggo Elenco PVOID ",this);
   TRACEPOINTER("Indirizzo arry: ",this->ArrayPVOID);
   Clear();
}

//----------------------------------------------------------------------------
// Questi metodi potrebbero divenire INLINE
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CercaPerPuntatore : Torna un puntatore al PVOID o -1 se non lo trova
//----------------------------------------------------------------------------
int ELENCO::CercaPerPuntatore(const void * PVOID_da_cercare) const
{
#undef TRCRTN
#define TRCRTN "ELENCO::CercaPerPuntatore"
   TRACEPOINTER("Cerco per puntatore nell' Elenco PVOID ",this);
   TRACEPOINTER("Cerco PVOID PM",PVOID_da_cercare);
   ORD_FORALL(*this,i)
      if(PVOID_da_cercare == ArrayPVOID[i])
      {
         TRACELONG("Trovato alla posizione",i);
         return i;
      }
   TRACESTRING("Non Trovato");
   return -1;
}

//----------------------------------------------------------------------------
// CercaPerPuntatore : Funziona come il metodo precedente ma permette di
// avere puntatori duplicati. All' inizio impostare LastMatch a -1;
//----------------------------------------------------------------------------
int ELENCO::CercaPerPuntatore(const void * PVOID_da_cercare, int LastMatch)const
{
   if(LastMatch == -1 || (LastMatch >= 0 && LastMatch < NumPVOID))
   {
      for(int i = LastMatch + 1; i< NumPVOID; i++)
         if(PVOID_da_cercare == ArrayPVOID[i])
            return i;
   }
   return -1;
}

//----------------------------------------------------------------------------
// ContaPerPuntatore : Ritorna il numero degli oggetti con quel Puntatore
//----------------------------------------------------------------------------
int ELENCO::ContaPerPuntatore(const void * PVOID_da_contare)const
{
   int Count=0;
   ORD_FORALL(*this,i)
      if(PVOID_da_contare == ArrayPVOID[i])
         Count++;
   return Count;
}

//----------------------------------------------------------------------------
// Operatore []: accede all' i-esimo elemento.
//----------------------------------------------------------------------------
PVOID& ELENCO::operator[](unsigned int idx) const
{
#undef TRCRTN
#define TRCRTN "ELENCO::Operator[]"
// #DGV Aggiunto cast a PVOID
      if(idx >= NumPVOID){
         ERRSTRING("Bound check error: Indice =  "+STRINGA(idx)+" Dim attuale = "+STRINGA(NumPVOID));
         BEEP;
         return *(PVOID *)NULL;
      };
      return (this->ArrayPVOID[idx]);
}
