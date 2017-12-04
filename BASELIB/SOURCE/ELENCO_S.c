//----------------------------------------------------------------------------
// FILE ELENCO_S.C
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

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 6

// EMS001
typedef unsigned long BOOL;

#include "ELENCO_S.H"

#include "MINMAX.H"
#include "myalloc.h"

TRACEVARIABLES

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°° CLASSE ELENCO_S °°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° COSTRUTTORI °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

ELENCO_S::ELENCO_S(){ NumStringhe = NumAlloc = 0; MaxStringhe = 32000; InsPoint = -1; ArrayStringhe = NULL;};
//----------------------------------------------------------------------------
// Costruttore di default
// il parametro num (10) indica il numero di elementi da allocare
// alla creazione dell'elenco, max (32000) e' il numero massimo di
// elementi di cui puo' essere costituito l'elenco, se si
// supera tale numero il programma va in abend.
//----------------------------------------------------------------------------
ELENCO_S::ELENCO_S(unsigned int num, unsigned int Max)
{
#undef TRCRTN
#define TRCRTN "@ELENCO_S"
//#RCC
//   BytesStrEle = 0;
//#RCC
   NumStringhe = 0;
   if(Max == 0)Max = 32000;
   MaxStringhe = Max;
   NumAlloc = num;
   InsPoint = -1;
   if(num > 0)
   {
      ArrayStringhe = new STRINGA * [NumAlloc];
      MEMIGNORE(ArrayStringhe);
      memset(ArrayStringhe, '\0', NumAlloc * sizeof(STRINGA *));
   }
   else
      ArrayStringhe = NULL;
   TRACEPOINTER("Creato Elenco Stringhe ",this);
}

//----------------------------------------------------------------------------
// Costruttore a partire da max 10 stringhe
// Crea un elenco con le stringhe specificate come parametri,
// al massimo possono essere 10.
//----------------------------------------------------------------------------
ELENCO_S::ELENCO_S(const STRINGA & s1,const STRINGA & s2,const STRINGA & s3,
const STRINGA & s4,const STRINGA & s5,const STRINGA & s6 ,const STRINGA & s7,
const STRINGA & s8,const STRINGA & s9 ,const STRINGA & s0)
{
//#RCC
//   BytesStrEle = 0;
//#RCC
   NumStringhe = 0;
   MaxStringhe = 32000;
   NumAlloc = 12;
   InsPoint = -1;
   ArrayStringhe = new STRINGA * [NumAlloc];
   MEMIGNORE(ArrayStringhe);
   memset(ArrayStringhe, '\0', NumAlloc * sizeof(STRINGA *));
   TRACEPOINTER("Creato Elenco Stringhe ", this);

// Ricordo che le stringhe NUSTR non sono aggiunte (ma quelle a lunghezza 0 si' )
// L' implementazione e' stata leggermente cambiata per velocizzare

//#RCC Aggiunto if( s?.Dim() ) BytesStrEle += s?.Dim()+1;

   USHORT ContLastNoNull=0;
   int i;
   if (&s1 !=  NULL)
     { (* this) += s1;/* BytesStrEle += s1.Dim() + 1;*/} // No else il primo deve esserci
   if (&s2 !=  NULL)
     { (* this) += s2;/* BytesStrEle += s2.Dim() + 1;*/}
   else
     ContLastNoNull++;
   if (&s3 !=  NULL)
    { for (i=0;i<ContLastNoNull;i++)(* this) += ""; ContLastNoNull=0;
      (* this) += s3; /*BytesStrEle += s3.Dim() + 1;*/}
   else
     ContLastNoNull++;
   if (&s4 !=  NULL)
     { for (i=0;i<ContLastNoNull;i++)(* this) += ""; ContLastNoNull=0;
       (* this) += s4;/* BytesStrEle += s4.Dim() + 1;*/}
   else
     ContLastNoNull++;
   if (&s5 !=  NULL)
     { for (i=0;i<ContLastNoNull;i++)(* this) += ""; ContLastNoNull=0;
       (* this) += s5; /*BytesStrEle += s5.Dim() + 1;*/}
   else
     ContLastNoNull++;
   if (&s6 !=  NULL)
     { for (i=0;i<ContLastNoNull;i++)(* this) += ""; ContLastNoNull=0;
       (* this) += s6;/* BytesStrEle += s6.Dim() + 1;*/}
   else
     ContLastNoNull++;
   if (&s7 !=  NULL)
     { for (i=0;i<ContLastNoNull;i++)(* this) += ""; ContLastNoNull=0;
       (* this) += s7;/* BytesStrEle += s7.Dim() + 1;*/}
   else
     ContLastNoNull++;
   if (&s8 !=  NULL)
     { for (i=0;i<ContLastNoNull;i++)(* this) += ""; ContLastNoNull=0;
       (* this) += s8;/* BytesStrEle += s8.Dim() + 1;*/}
   else
     ContLastNoNull++;
   if (&s9 !=  NULL)
     { for (i=0;i<ContLastNoNull;i++)(* this) += ""; ContLastNoNull=0;
       (* this) += s9; /*BytesStrEle += s9.Dim() + 1;*/}
   else
     ContLastNoNull++;
   if (&s0 !=  NULL)
     { for (i=0;i<ContLastNoNull;i++)(* this) += ""; ContLastNoNull=0;
       (* this) += s0;/* BytesStrEle += s0.Dim() + 1;*/}

}

//----------------------------------------------------------------------------
// Costruttore di copia : Crea un nuovo elenco uguale a quello indicato
// Crea un elenco di stringhe copiando da un'altro specificato.
//----------------------------------------------------------------------------
ELENCO_S::ELENCO_S(const ELENCO_S & A)
{
#undef TRCRTN
#define TRCRTN "@ELENCO_S"
   NumStringhe = 0;
   NumAlloc = 0;
   InsPoint = -1;
   * this = A;       // Assegnazione
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° DISTRUTTORE °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

ELENCO_S::~ELENCO_S()
{
#undef TRCRTN
#define TRCRTN "~ELENCO_S()"
   TRACEPOINTER("Distruggo Elenco Stringhe ",this);
   Clear();
}

//----------------------------------------------------------------------------
// SetInsPoint
// Permette di settare il punto di inserimento delle stringa che verr…
// successivamente inserita invocando l'operatore +=.
//----------------------------------------------------------------------------

BOOL ELENCO_S::SetInsPoint(long Indice)
{
   if(Indice > -1 && Indice < NumStringhe)
   {
      InsPoint = Indice;
      return TRUE;
   }
   else
   {
      InsPoint = -1;
      return FALSE;
   }
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°° OPERATORI °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

//----------------------------------------------------------------------------
// Operatore = Assegna a questo elenco, l'elenco indicato
//----------------------------------------------------------------------------
ELENCO_S & ELENCO_S::operator=(const ELENCO_S & A)
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::operator="
   Clear();  // Pulisco l' elenco
   NumStringhe = A.NumStringhe;
   MaxStringhe = A.MaxStringhe;
   NumAlloc    = A.NumAlloc;

//#RCC
//   BytesStrEle = A.BytesStrEle;
//#RCC

   if(NumAlloc > 0)
   {
      ArrayStringhe = new STRINGA * [NumAlloc];
      MEMIGNORE(ArrayStringhe);
      memset(ArrayStringhe, '\0', NumAlloc * sizeof(STRINGA *));
// crea una copia di tutte le stringhe
      FORALL(A,i)
      {
         // Creo una copia della stringa Iesima
         ArrayStringhe[i] = new STRINGA(A[i]);
         MEMIGNORE(ArrayStringhe[i]);
      }
   }
   else
      ArrayStringhe = NULL;
   TRACEPOINTER("Copiato Elenco Stringhe ",& A);
   TRACEPOINTER("Copiato su ",this);
   return * this;
}

//----------------------------------------------------------------------------
// Operatore += Versione 1
// Prende in input un char * e lo inserisce nell'elenco nella posizione
// richiesta in precedenza oppure (di default) in coda.
//----------------------------------------------------------------------------

ELENCO_S & ELENCO_S::operator+=(const char * Strg)
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::Operatore +="

//-------------------------------------------------------------------------
// 1) Controllo se la stringa Š valida se non lo Š esco
//-------------------------------------------------------------------------

   if(Strg == NULL)
   {
        ERRSTRING("Richiesta inserimento char * nulla: RIFIUTATA");
        InsPoint = -1;
        return * this;
   }

//-------------------------------------------------------------------------
// 2) Controllo se bisogna allocare nuova memoria, in caso affermativo
//    alloco la nuova area, copio la vecchia nella nuova, setto a 0 la
//    parte eccedente, elimino la vecchia e aggiorno NumAlloc.
//-------------------------------------------------------------------------

    if(NumAlloc < NumStringhe + 1)
    {
       short Delta = max(50, NumAlloc/5);
       STRINGA * * Nuovo = new STRINGA * [NumAlloc + Delta];
       if(NumAlloc > 0)
       {
          memcpy(Nuovo, ArrayStringhe, NumAlloc * sizeof(STRINGA *));
          delete ArrayStringhe;
       }
       memset(Nuovo + NumAlloc, 0, Delta * sizeof(STRINGA *));
       ArrayStringhe = Nuovo;
       MEMIGNORE(ArrayStringhe);
       NumAlloc += Delta;
       if(NumAlloc > MaxStringhe){
          BEEP;
          Abend(3);
       }

    }

//-------------------------------------------------------------------------
// 3) Controllo l'InsPoint e se ha un valore compreso tra zero e NumRighe
//    faccio una memmove per permettere l'inserimento della nuova stringa
//    nella posizione richiesta dell'array
//-------------------------------------------------------------------------

   if(InsPoint > -1 && InsPoint < NumStringhe)
      memmove(ArrayStringhe + InsPoint + 1, ArrayStringhe + InsPoint, (NumStringhe - InsPoint) * sizeof(STRINGA *));
   else
      InsPoint = NumStringhe;

//-------------------------------------------------------------------------
// 4) Aggiungo la nuova stringa e aggiorno NumStringhe e BytesStrEle
//-------------------------------------------------------------------------

   ArrayStringhe[InsPoint] = new STRINGA(Strg);
   MEMIGNORE(ArrayStringhe[InsPoint]);
   NumStringhe++;
   // short LunStri = (ArrayStringhe[InsPoint])->Dim();
   // if (LunStri > 0)
   //    BytesStrEle += LunStri + 1;

//-------------------------------------------------------------------------
// 5) Uscita
//-------------------------------------------------------------------------

   InsPoint = -1;
   return * this;
}

//----------------------------------------------------------------------------
// Operatore += Versione 2
// Prende in input una STRINGA e la inserisce nell'elenco nella posizione
// richiesta in precedenza oppure (di default) in coda.
//----------------------------------------------------------------------------
// Montagna: copiato il precedente perche' velocizza (evita una strlen)
// Del resto e' la funzione utilizzata piu' frequentemente
ELENCO_S & ELENCO_S::operator+=(const STRINGA& Strg)
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::Operatore +="

//-------------------------------------------------------------------------
// 2) Controllo se bisogna allocare nuova memoria, in caso affermativo
//    alloco la nuova area, copio la vecchia nella nuova, setto a 0 la
//    parte eccedente, elimino la vecchia e aggiorno NumAlloc.
//-------------------------------------------------------------------------

    if(NumAlloc < NumStringhe + 1)
    {
       short Delta = max(50, NumAlloc/5);
       STRINGA * * Nuovo = new STRINGA * [NumAlloc + Delta];
       if(NumAlloc > 0)
       {
          memcpy(Nuovo, ArrayStringhe, NumAlloc * sizeof(STRINGA *));
          delete ArrayStringhe;
       }
       memset(Nuovo + NumAlloc, 0, Delta * sizeof(STRINGA *));
       ArrayStringhe = Nuovo;
       MEMIGNORE(ArrayStringhe);
       NumAlloc += Delta;
       if(NumAlloc > MaxStringhe){
          BEEP;
          Abend(3);
       }

    }

//-------------------------------------------------------------------------
// 3) Controllo l'InsPoint e se ha un valore compreso tra zero e NumRighe
//    faccio una memmove per permettere l'inserimento della nuova stringa
//    nella posizione richiesta dell'array
//-------------------------------------------------------------------------

   if(InsPoint > -1 && InsPoint < NumStringhe)
      memmove(ArrayStringhe + InsPoint + 1, ArrayStringhe + InsPoint, (NumStringhe - InsPoint) * sizeof(STRINGA *));
   else
      InsPoint = NumStringhe;

//-------------------------------------------------------------------------
// 4) Aggiungo la nuova stringa e aggiorno NumStringhe e BytesStrEle
//-------------------------------------------------------------------------

   ArrayStringhe[InsPoint] = new STRINGA(Strg);
   MEMIGNORE(ArrayStringhe[InsPoint]);
   NumStringhe++;

//-------------------------------------------------------------------------
// 5) Uscita
//-------------------------------------------------------------------------

   InsPoint = -1;
   return * this;
}

//----------------------------------------------------------------------------
// Operatore += Versione 3
// Prende in input un ELENCO_S e lo inserisce nell'elenco nella posizione
// richiesta in precedenza oppure (di default) in coda.
//----------------------------------------------------------------------------

ELENCO_S & ELENCO_S::operator+=(const ELENCO_S& Elenco)
{
   if( InsPoint < 0  || InsPoint >= NumStringhe){
      for (short a = 0; a < Elenco.Dim(); a++) {
         * this += (CPSZ)Elenco[a];
      }
   } else {
      long Ip = InsPoint;
      for (short a = 0; a < Elenco.Dim(); a++) {
         InsPoint = Ip ++;
         * this += (CPSZ)Elenco[a];
      }
   }
   return * this;
}

//----------------------------------------------------------------------------
// Operatore -= Elimina dall'elenco la stringa indicata
//----------------------------------------------------------------------------
ELENCO_S & ELENCO_S::operator-=(const STRINGA & Strg)
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::operator-="
   int Idx = CercaPerNome(Strg);
   return(* this -= Idx);
}

//----------------------------------------------------------------------------
// Operatore -= Elimina (se esiste) dall'elenco la stringa corrispondente
// all'indice specificato
//----------------------------------------------------------------------------
ELENCO_S & ELENCO_S::operator-=(int Idx)
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::operator-="
   if(Idx < 0 || Idx >= NumStringhe) // Non trovata
      return * this;
//#RCC
// STRINGA * Appo =  ArrayStringhe[Idx];
// if( Appo->Dim() )
//     BytesStrEle -= (Appo->Dim()+1);
//#RCC
   delete ArrayStringhe[Idx];
   ArrayStringhe[Idx] = NULL;
// Shift dell' array di puntatori: usa memmove per gestire l' overlap
   if(Idx < NumStringhe - 1 ) // Non shifto se la stringa eliminata e' l' ultima dell'elenco
      memmove(ArrayStringhe + Idx, ArrayStringhe + Idx + 1, (NumStringhe - Idx - 1) * sizeof(STRINGA *));
   NumStringhe --;
   return * this;
}

//----------------------------------------------------------------------------
// Operatore []: accede all' i-esimo elemento.
// ritorna per reference la stringa con posizione specificata
// tra parentesi idx.
//----------------------------------------------------------------------------
STRINGA & ELENCO_S::operator[](unsigned int idx)
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::Operator[]"
   if(idx >= NumStringhe){
// Montagna: il check sul bound non e' molto utile per le stringhe:
// infatti la stringa nulla e' legale.
//      if(idx > 0){
//         ERRSTRING("Bound check error: Indice =  "+STRINGA(idx)+" Dim attuale = "+STRINGA(NumStringhe));
//         BEEP;
//      };
      return * (STRINGA *)NULL;
   }
   return * (this->ArrayStringhe[idx]);
}
const STRINGA & ELENCO_S::operator[](unsigned int idx) const
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::Operator[]"
   if(idx >= NumStringhe){
// Montagna: il check sul bound non e' molto utile per le stringhe:
// infatti la stringa nulla e' legale.
//      if(idx > 0){
//         ERRSTRING("Bound check error: Indice =  "+STRINGA(idx)+" Dim attuale = "+STRINGA(NumStringhe));
//         BEEP;
//      };
      return * (STRINGA *)NULL;
   }
   return (const STRINGA&)* (this->ArrayStringhe[idx]);
}

//----------------------------------------------------------------------------
// Operatore []: accede alla stringa uguale a quella immessa
// ritorna per reference la stringa uguale a Target contenuta
// nell'elenco se esiste, altrimenti ritorna una stringa nulla.
//----------------------------------------------------------------------------
const STRINGA & ELENCO_S::operator[](const STRINGA & Target) const
{
   int j = CercaPerNome(Target);
   if(j == -1)
      return * (STRINGA *)NULL;
   return operator[](j);
}
STRINGA & ELENCO_S::operator[](const STRINGA & Target)
{
   int j = CercaPerNome(Target);
   if(j == -1)
      return * (STRINGA *)NULL;
   return operator[](j);
}

//----------------------------------------------------------------------------
// Operatore () : Ritorna il sottoelenco compreso tra gli estremi specificati
//----------------------------------------------------------------------------
ELENCO_S ELENCO_S::operator()(UINT Primo,UINT Ultimo)const
{
   ELENCO_S Tmp;
   for(int i = Primo; i < Dim() && i <= Ultimo; i++)
      Tmp += operator[](i);
   return Tmp;
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° Clear() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
//----------------------------------------------------------------------------
// Elimina tutte le stringhe contenute nell'elenco (ma non distrugge l'elenco)
//----------------------------------------------------------------------------
void ELENCO_S::Clear()
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::Clear"

   FORALL(* this, i) {
      delete this->ArrayStringhe[i];
      this->ArrayStringhe[i] = NULL;
   }
   if(NumAlloc > 0)
      delete ArrayStringhe;

   NumStringhe = NumAlloc = 0;
}



// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° Strip() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
//----------------------------------------------------------------------------
// Elimina tutte le stringhe con dim = 0 contenute nell'elenco (ma non distrugge
// l'elenco)
//----------------------------------------------------------------------------
ELENCO_S ELENCO_S::Strip()
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::Strip"
   ELENCO_S Out;
   ORD_FORALL(* this, i)
      if( THIS[i].Dim() > 0) Out += THIS[i];
   return Out;
}




// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°° CercaPerNome() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°
//----------------------------------------------------------------------------
// CercaPerNome : Cerca la stringa richiesta tra quelle dell'elenco
// e ritorna la sua posizione oppure -1 se non la trova.
//----------------------------------------------------------------------------
int ELENCO_S::CercaPerNome(const STRINGA & Nome_da_cercare, int Last) const
{
#undef TRCRTN
#define TRCRTN "ELENCO_S::CercaPerNome"
   TRACEPOINTER("Cerco per nome nell' Elenco stringhe ",this);
   TRACESTRING2("Cerco stringa ",(CPSZ)Nome_da_cercare);

   int Init=0;
   if(Last != -1)
   {
      Init = Last + 1;
      TRACEINT("Inizio ricerca da posizione ", Init);
   }
// si deve rispettare l' ordine degli oggetti
   // --------------------------------------------------------
   // Montagna: invece di utilizzare l' operatore ==
   // lo esplodo nelle operazioni elementari, per velocizzare
   // Inoltre opero tramite addizione di puntatori perche'
   // sono piu' veloci delle moltiplicazioni
   // Questo e' infatti uno dei colli di bottiglia di OOLIB
   // --------------------------------------------------------
   //for(int i = Init, Dimn = Dim(); i < Dimn; i++)
   //   if(Nome_da_cercare == * ArrayStringhe[i]) //   {
   //   TRACELONG("Trovato alla posizione", i);
   //   return i;
   //   }
   // --------------------------------------------------------
   STRINGA ** Idx  = ArrayStringhe +Init;
   STRINGA ** Stop = ArrayStringhe +Dim();
   USHORT TargetLen  = (& Nome_da_cercare == NULL) ? 0 : Nome_da_cercare.Len;
   int i = -1;
   if (TargetLen) {
      char * TargetData = Nome_da_cercare.Dati;
      while(Idx < Stop){
         if((*Idx)->Len == TargetLen && !strcmp((*Idx)->Dati,TargetData) ) { // Stringhe eguali
            i = Idx - ArrayStringhe;
            break;
         } /* endif */
         Idx ++;
      }
   } else {
      while(Idx < Stop){
         if((*Idx)->Len == 0) { // Stringa vuota
            i = Idx - ArrayStringhe;
            break;
         } /* endif */
         Idx ++;
      }
   } /* endif */
   if (i >= 0) {
       TRACELONG("Trovato alla posizione", i);
   } else {
       TRACESTRING("Non Trovato");
   } /* endif */
   return i;
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° Contiene() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
//----------------------------------------------------------------------------
// Contiene : ritorna TRUE o FALSE se trova, o meno, la stringa nell'elenco
//----------------------------------------------------------------------------
BOOL ELENCO_S::Contiene(const STRINGA & Nome_da_cercare) const
{
   return (CercaPerNome(Nome_da_cercare) >= 0);
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° Sort() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
//----------------------------------------------------------------------------
// Sort : Ordina le stringhe contenute nell'elenco
//----------------------------------------------------------------------------
ELENCO_S & ELENCO_S::Sort()
{
   if(NumStringhe > 0)
   {
      qsort(ArrayStringhe, NumStringhe, sizeof(STRINGA *), PSTRINGACmp);
   }
   return THIS;
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° Tokenize() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
//----------------------------------------------------------------------------
// Tokenize :
// converte l'elenco in una stringa in cui ogni stringa e'
// separata dalla sucessiva dal separatore Separator.
//----------------------------------------------------------------------------
STRINGA ELENCO_S::Tokenize(char Separator)
{
   STRINGA Out;
   ORD_FORALL(THIS,i)
   {
      Out += THIS[i];
      if(i < Dim() - 1)Out += Separator;
   }
   return Out;
}
// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° BytesAlloc() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
//----------------------------------------------------------------------------
// BytesAlloc :
// Ritorna l' allocazione totale in bytes
//----------------------------------------------------------------------------
ULONG ELENCO_S::BytesAlloc(){
   ULONG Tot = 0;
   FORALL(THIS,i)Tot += THIS[i].Dim()+1;
   return Tot;
};
