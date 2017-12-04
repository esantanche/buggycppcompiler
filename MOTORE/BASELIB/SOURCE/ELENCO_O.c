//----------------------------------------------------------------------------
// FILE ELENCO_O.C
//----------------------------------------------------------------------------
// Definizioni di funzioni per le classi:
// ELENCO_Oggetti
// ELENCO_Oggetti_Modello
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 6

#include "oggetto.h"

#include "minmax.h"
#include "myalloc.h"

TRACEVARIABLES

//----------------------------------------------------------------------------
// Classe ELENCO_Oggetti
//----------------------------------------------------------------------------
// Questa classe definisce un elenco di oggetti.
// E' implementata mediante un' array di puntatori ad oggetti.
ELENCO_Oggetti::ELENCO_Oggetti(){ NumAlloc = NumOggetti = 0; MaxOggetti = 32000; HoAssociati = Assc_UnDef; ArrayAssociati = NULL; ArrayOggetti = NULL; };

//----------------------------------------------------------------------------
// Contruttore di default
//----------------------------------------------------------------------------
ELENCO_Oggetti::ELENCO_Oggetti(unsigned int num,unsigned int Max)
{
#undef TRCRTN
#define TRCRTN "@ELENCO_Oggetti"
   NumOggetti = 0;                 // Numero di oggetti presenti nell' elenco
   if(Max == 0)Max = 32000;
   MaxOggetti = Max;               // Numero massimo di oggetti inseribili nell' elenco
   NumAlloc = num;                 // Dimensione area allocata nell' array
   HoAssociati = Assc_UnDef;       // NON ho dati associati.
   ArrayAssociati = NULL;          // Array di dati associati agli oggetti.
   if(NumAlloc > 0)
   {
      ArrayOggetti = new OGGETTO * [NumAlloc];
      MEMIGNORE(ArrayOggetti);
      memset(ArrayOggetti, '\0', NumAlloc * sizeof(OGGETTO *));
   }
   else
      ArrayOggetti = NULL;
   TRACEPOINTER2("Creato Elenco Oggetti , array ", this,this->ArrayOggetti);
}

//----------------------------------------------------------------------------
// Costruttore di copia : Crea un nuovo elenco uguale a quello indicato
//----------------------------------------------------------------------------
ELENCO_Oggetti::ELENCO_Oggetti(const ELENCO_Oggetti & A)
{
   NumOggetti = NumAlloc = 0;   // Init dimensioni
   HoAssociati = Assc_UnDef;    // NON ho dati associati.
   ArrayAssociati = NULL;       // Array di dati associati agli oggetti.
   * this = A;                  // Assegnazione
}

//----------------------------------------------------------------------------
// Operatore = Assegna a questo elenco l'elenco indicato
//----------------------------------------------------------------------------
ELENCO_Oggetti& ELENCO_Oggetti::operator=(const ELENCO_Oggetti & A)
{
   Clear();
   NumOggetti = A.NumOggetti;
   MaxOggetti = A.MaxOggetti;
   NumAlloc = A.NumAlloc;
   HoAssociati = A.HoAssociati;

   if(HoAssociati)
   {
      if(HoAssociati < Generic)
      {
      // Immagazzino come array di USHORT
         ArrayNums = new USHORT[NumAlloc];
         MEMIGNORE(ArrayNums);
         memcpy(ArrayNums, A.ArrayNums, NumAlloc * sizeof(USHORT));
      }
      else
      {
         ArrayAssociati = new L_BUFFER[NumAlloc];
         MEMIGNORE(ArrayAssociati);
         memset(ArrayAssociati, '\0', NumAlloc * sizeof(L_BUFFER));
         FORALL(*this, i)
         {
            USHORT Lg = ArrayAssociati[i].Length=A.ArrayAssociati[i].Length;
            ArrayAssociati[i].Dati = new char[Lg];
            MEMIGNORE(ArrayAssociati[i].Dati);
            memcpy(ArrayAssociati[i].Dati,A.ArrayAssociati[i].Dati,Lg);
         }
      }
   }

   if(NumAlloc > 0)
   {
      ArrayOggetti = new OGGETTO * [NumAlloc];
      MEMIGNORE(ArrayOggetti);
      memcpy(ArrayOggetti, A.ArrayOggetti, NumAlloc * sizeof(OGGETTO *));
   }
   else
      ArrayOggetti = NULL;
   TRACEPOINTER2("Copiato Elenco Oggetti, su ", &A, this);
   return * this;
}

//----------------------------------------------------------------------------
// Operatore += Aggiunge all'elenco un nuovo oggetto
// Non controlla che se lo stesso oggetto e' gia' presente in elenco
//----------------------------------------------------------------------------
ELENCO_Oggetti& ELENCO_Oggetti::operator+=(OGGETTO & Ogg)
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::operator+="
   int const Delta = max(30, NumAlloc / 5);
   int const TotAlloc = NumAlloc + Delta;
   TRACEPOINTER("Elenco Oggetti ",this);
   if(&Ogg != NULL)
   {
      if(NumAlloc <= NumOggetti)
      {
         OGGETTO * * Nuovo = new OGGETTO * [TotAlloc];
         if(NumAlloc>0)
         {
            memcpy(Nuovo, ArrayOggetti, NumAlloc * sizeof(OGGETTO *));
            delete ArrayOggetti;
         }
         memset(Nuovo + NumAlloc, 0, Delta * sizeof(OGGETTO *));
         ArrayOggetti = Nuovo;
         MEMIGNORE(ArrayOggetti);
         if(HoAssociati && NumAlloc > 0)
         {
            if(HoAssociati < Generic)
            {
            // Immagazzino come array di USHORT
               USHORT * Nuovo = new USHORT[TotAlloc];
               memcpy(Nuovo, ArrayNums, NumAlloc * sizeof(USHORT));
               memset(Nuovo + NumAlloc, 0, Delta * sizeof(USHORT));
               delete ArrayNums;
               ArrayNums = Nuovo;
               MEMIGNORE(ArrayNums);
            }
            else
            {
               L_BUFFER * Nuovo = new L_BUFFER[TotAlloc];
               memcpy(Nuovo, ArrayAssociati, NumAlloc * sizeof(L_BUFFER));
               memset(Nuovo + NumAlloc, 0, Delta * sizeof(L_BUFFER));
               delete ArrayAssociati;
               ArrayAssociati = Nuovo;
               MEMIGNORE(ArrayAssociati);
            }
         }
         NumAlloc = TotAlloc;
         TRACELONG("Portata dimensione (NumAlloc) a ",NumAlloc);
// Se supero i MaxOggetti oggetti Abend
         if(NumAlloc > MaxOggetti){
            if(MaxOggetti > 0){ // Ok ho superato un limite valido
               Abend(3);
            } else {
               ERRSTRING("ATTENZIONE: Dubbia sequenza di attivazione delle DLL");
               // BEEP; // Elimino il beep per motivi di opportunita'.
            } /* endif */
         };
      }
      ArrayOggetti[NumOggetti] = & Ogg;
      if(HoAssociati)
      {
         if(HoAssociati < Generic)
         {
         // Immagazzino come array di USHORT
            ArrayNums[NumOggetti] = 0;
         }
         else
         {
            ArrayAssociati[NumOggetti].Length = 0;
            ArrayAssociati[NumOggetti].Dati = NULL;
         }
      }
      NumOggetti++;
      TRACESTRING2("Aggiunto oggetto ",(CPSZ)Ogg.Nome);
      TRACEVLONG(NumOggetti);
   }
   else
   {
      TRACESTRING("Richiesta inserimento oggetto nullo: RIFIUTATA");
   }
   return * this;
}

//----------------------------------------------------------------------------
// Operatore -= Elimina dall'elenco l'oggetto indicato
//----------------------------------------------------------------------------
ELENCO_Oggetti& ELENCO_Oggetti::operator-=(const OGGETTO & Ogg)
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::operator-="
   TRACEPOINTER("Elenco Oggetti ", this);
   int pos = CercaPerPuntatore(& Ogg);
   if(pos >= 0)
   {
      if(HoAssociati &&
         HoAssociati >=  Generic &&
         ArrayAssociati[pos].Dati != NULL)
         delete ArrayAssociati[pos].Dati;
      if(pos < NumOggetti)
      {
         if( ( NumOggetti - pos - 1 ) > 0 )
               memmove(& ArrayOggetti[pos], & ArrayOggetti[pos + 1], (NumOggetti - pos - 1) * sizeof(OGGETTO *));

         if(HoAssociati)
         {
            if(HoAssociati < Generic)
            {
            // Immagazzino come array di USHORT
               if( ( NumOggetti - pos - 1 ) > 0 )
                     memmove(& ArrayNums[pos], & ArrayNums[pos + 1], (NumOggetti - pos - 1) * sizeof(USHORT));
            }
            else
            {
               if( ( NumOggetti - pos - 1 ) >0 )
                     memmove(& ArrayAssociati[pos], & ArrayAssociati[pos + 1], (NumOggetti - pos - 1) * sizeof(L_BUFFER));
            }
         }
      }
      NumOggetti --;
      TRACESTRING2("Eliminato oggetto " ,(CPSZ)Ogg.Nome);
      TRACEVLONG(NumOggetti);
   }
   return * this;
}

//----------------------------------------------------------------------------
// Operatore -= Elimina dall'elenco l'oggetto col nome indicato
//----------------------------------------------------------------------------
ELENCO_Oggetti& ELENCO_Oggetti::operator-=(const STRINGA& OggName)
{
   OGGETTO * Ogg = CercaPerNome(OggName);
   if(Ogg != NULL)
      operator-=(* Ogg);
   return * this;
}

//----------------------------------------------------------------------------
// Operatore += Aggiunge all'elenco un altro elenco di oggetti
// I dati associati NON sono mai aggiunti all' elenco !!! anche perche' i
// tipi di solito non corrispondono
//----------------------------------------------------------------------------
ELENCO_Oggetti& ELENCO_Oggetti::operator+=(const ELENCO_Oggetti & Elenco)
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::operator+="
   int Dim = Elenco.Dim();
   TRACEPOINTER("Elenco Oggetti ",this);
   TRACEPOINTER("Elenco Oggetti da aggiungere:",&Elenco);
   IFTRC(Elenco.Trace());
   if(Dim > 0)
   {
      int const Delta=max(Dim,NumAlloc/5);
// Ocio! prima TotAlloc veniva messo uguale a NumOggetti+Delta,
// la memoria allocata era quindi pari a (NumOggetti+Delta)*sizeof(OGGETTO*)
// ma subito dopo ci sono una memcpy e una memset per un totale di
// (NumAlloc+Delta) memoria. In altre parole si andava a scrivere oltre i
// limiti della memoria allocata. Era quindi necessario mettere TotAlloc
// uguale a NumAlloc+Delta.
      int const TotAlloc=NumAlloc + Delta;
// Eventuale riallocazione dell' area
      if(NumAlloc <  NumOggetti + Dim)
      {
         OGGETTO * * Nuovo = new OGGETTO * [TotAlloc];
         if(NumAlloc>0)
         {
            memcpy(Nuovo, ArrayOggetti, NumAlloc * sizeof(OGGETTO *));
            delete ArrayOggetti;
         }
         memset(Nuovo + NumAlloc,0,Delta * sizeof(OGGETTO *));
         ArrayOggetti=Nuovo;
         MEMIGNORE(ArrayOggetti);
         if(HoAssociati && NumAlloc > 0)
         {
            if(HoAssociati < Generic)
            {
            // Immagazzino come array di USHORT
               USHORT * Nuovo  =new USHORT[TotAlloc];
               memcpy(Nuovo, ArrayNums, NumAlloc * sizeof(USHORT));
               memset(Nuovo + NumAlloc, 0, Delta * sizeof(USHORT));
               delete ArrayNums;
               ArrayNums = Nuovo;
               MEMIGNORE(ArrayNums);
            }
            else
            {
               L_BUFFER * Nuovo = new L_BUFFER[TotAlloc];
               memcpy(Nuovo, ArrayAssociati, NumAlloc * sizeof(L_BUFFER));
               memset(Nuovo + NumAlloc, 0, Delta * sizeof(L_BUFFER));
               delete ArrayAssociati;
               ArrayAssociati = Nuovo;
               MEMIGNORE(ArrayAssociati);
            }
         }
         NumAlloc = TotAlloc;
         TRACELONG("Portata dimensione (NumAlloc) a ",NumAlloc);
// Se supero i MaxOggetti oggetti Abend
         if(NumAlloc > MaxOggetti)
            Abend(3);
      }

// Aggiunta degli oggetti dell' elenco
      memcpy(ArrayOggetti + NumOggetti, Elenco.ArrayOggetti, Dim * sizeof(OGGETTO *));
      NumOggetti += Dim;
      TRACELONG("Aggiunti oggetti:",Dim);
   }
   else
   {
      TRACESTRING("Richiesta aggiunta elenco vuoto: RIFIUTATA");
   }
   return *this;
}

//----------------------------------------------------------------------------
// Operatore -= Elimina dall'elenco un sottoelenco di oggetti
//----------------------------------------------------------------------------
ELENCO_Oggetti& ELENCO_Oggetti::operator-=(const ELENCO_Oggetti & Elenco)
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::operator-="
   TRACEPOINTER("Elenco Oggetti ",this);
   TRACEPOINTER("Elenco Oggetti da togliere:", & Elenco);
   IFTRC(Elenco.Trace());
   FORALL(Elenco,i)
      * this -= Elenco[i];
   return * this;
}

//----------------------------------------------------------------------------
// Operatore -= Elimina dall'elenco un elemento
//----------------------------------------------------------------------------
ELENCO_Oggetti& ELENCO_Oggetti::operator-=(unsigned int Idx)
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::operator-="
   if( Idx >= NumOggetti )
      return THIS;

   if( ( NumOggetti - Idx - 1 ) > 0 )
         memmove(ArrayOggetti + Idx, ArrayOggetti + Idx + 1
                 , (NumOggetti - Idx - 1) * sizeof(OGGETTO *));

   if( HoAssociati ) {
      if( HoAssociati < Generic) {
          if( ( NumOggetti - Idx - 1 ) > 0 )
                memmove(ArrayNums      + Idx, ArrayNums      + Idx + 1
                        , (NumOggetti - Idx - 1) * sizeof(USHORT ));
      } else {
          if( ( NumOggetti - Idx - 1 ) > 0 )
                memmove(ArrayAssociati + Idx, ArrayAssociati + Idx + 1
                      , (NumOggetti - Idx - 1) * sizeof(L_BUFFER ));
      }
   }
   NumOggetti--;
   return THIS;
}
//----------------------------------------------------------------------------
// Distruttore di un elenco di oggetti
//----------------------------------------------------------------------------
ELENCO_Oggetti::~ELENCO_Oggetti()
{
#undef TRCRTN
#define TRCRTN "~ELENCO_Oggetti()"
   TRACESTRING("Distrutto elenco Oggetti");
   IFTRC(this->Trace());
   Clear();
}

//----------------------------------------------------------------------------
// Clear : Elimina tutti gli oggetti contenuti nell'elenco (ma non distrugge
// l'elenco)
//----------------------------------------------------------------------------
void ELENCO_Oggetti::Clear()
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::Clear()"
   if(NumAlloc)
   {
      delete ArrayOggetti;
      if(HoAssociati)
      {
         if(HoAssociati < Generic)
         {
         // Immagazzino come array di USHORT
                                delete ArrayNums;
         }
         else
         {
            FORALL(*this,i)
            {
               if(ArrayAssociati[i].Dati != NULL)
                  delete ArrayAssociati[i].Dati;
            }
                                delete ArrayAssociati;
         }
      }
   }
   NumOggetti = NumAlloc = 0;
   ArrayAssociati = NULL;
}
//----------------------------------------------------------------------------
// Funzioni per gestire dati associati
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Associa :  Dato un puntatore ad oggetto associa a tale oggetto una copia
// di un' area dati
//----------------------------------------------------------------------------
BOOL ELENCO_Oggetti::Associa(const OGGETTO * Ogg, const void * Data,
USHORT Len , TIPO_DATI_ASSOCIATI What)
{
// Vero se NON ho trovato l' oggetto nell' elenco
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::Associa(Dati)"
   TRACEPOINTER2("Associo Oggetto, ad elenco ", Ogg,this);
   int pos = CercaPerPuntatore(Ogg);
   if(pos < 0)      // L' oggetto non fa' parte dell' elenco
      return TRUE;
   return Associa(pos,Data,Len,What);
}

BOOL ELENCO_Oggetti::Associa(int pos, const void * Data, USHORT Len,
TIPO_DATI_ASSOCIATI What)
{
   if(What < Generic || ( HoAssociati  && HoAssociati != What))
   {
      ERRINT("Errore nel tipo di dato associato: Richiesto =",What);
      ERRINT("Attuale = ",HoAssociati);
//      BEEP;
      return TRUE;
   }
   if(Data == NULL || Len == 0)
   {
   // Dimensione 0 = OK elimino eventuale associazione precedente
      if(!HoAssociati)  // Non avevo associazione
         return FALSE;
      if(ArrayAssociati[pos].Dati !=NULL)
      {
         // Avevo gia' un dato associato: lo cancello
         delete ArrayAssociati[pos].Dati;  // Dealloco i dati precedenti;
         ArrayAssociati[pos].Dati = NULL;
         ArrayAssociati[pos].Length = 0;
      }
      return FALSE;
   }
   if(!HoAssociati)
   {
      ArrayAssociati =new L_BUFFER[NumAlloc];
      MEMIGNORE(ArrayAssociati);
      memset(ArrayAssociati,0,NumAlloc*sizeof(L_BUFFER));
      HoAssociati=What;
   }
   if(ArrayAssociati[pos].Dati !=NULL)
   {
      // Avevo gia' un dato associato: lo ricopro
      if(ArrayAssociati[pos].Length != Len)
      {
         // Cambia lunghezza : devo riallocare
         delete ArrayAssociati[pos].Dati;    // Dealloco i dati precedenti;
         ArrayAssociati[pos].Dati = new char[Len]; // E ne rialloco altri
         MEMIGNORE(ArrayAssociati[pos].Dati);
      }
   }
   else
   {
      ArrayAssociati[pos].Dati = new char[Len]; // Alloco i dati
      MEMIGNORE(ArrayAssociati[pos].Dati);
   }
   ArrayAssociati[pos].Length = Len;
   memcpy(ArrayAssociati[pos].Dati,Data,Len);
   return FALSE;
}

//----------------------------------------------------------------------------
// Associa : Analoghe che associano un USHORT
//----------------------------------------------------------------------------
BOOL ELENCO_Oggetti::Associa(const OGGETTO* Ogg, USHORT Num
  ,TIPO_DATI_ASSOCIATI What)
{
  // Vero se NON ho trovato l' oggetto nell' elenco
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::Associa(Num)"
   TRACEPOINTER2("Associo Oggetto, ad elenco ",Ogg,this);
   int pos = CercaPerPuntatore(Ogg);
   if(pos < 0)return TRUE; // L' oggetto non fa' parte dell' elenco
   return Associa(pos,Num,What);
}

BOOL ELENCO_Oggetti::Associa(int pos, USHORT Num ,TIPO_DATI_ASSOCIATI What)
{
   if(What >= Generic || !What || ( HoAssociati  && HoAssociati != What))
   {
      ERRINT("Errore nel tipo di dato associato: Richiesto =",What);
      ERRINT("Attuale = ",HoAssociati);
//      BEEP;
      return TRUE;
   }
   if(Num == 0 && !HoAssociati)    // Non avevo associazione
      return FALSE;
   if(!HoAssociati)
   {
      ArrayNums =new USHORT[NumAlloc];
      MEMIGNORE(ArrayNums);
      memset(ArrayNums,0,NumAlloc*sizeof(USHORT));
      HoAssociati=What;
   }
   ArrayNums[pos] = Num;
   return FALSE;
}

//----------------------------------------------------------------------------
// AreaAssociata : Dato un puntatore ad oggetto ritorna un puntatore all'area
// dati associata.
// Opzionalmente puo' anche ritornare la lunghezza dell' area associata.
//----------------------------------------------------------------------------
void * ELENCO_Oggetti::AreaAssociata(const OGGETTO* Ogg,USHORT * pLen)const
{
//Null se non trovo l' oggetto o se NON ha area associata.
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::AreaAssociata"
   TRACEPOINTER2("Ottengo area associata ad Oggetto, da elenco ",Ogg,this);
   int pos;
   if(HoAssociati)
      pos = CercaPerPuntatore(Ogg);
   if(!HoAssociati || pos < 0)
   {
      if(pLen != NULL)    // L' oggetto non fa' parte dell' elenco
         * pLen = 0;
      return NULL;
   }
   if(HoAssociati <  Generic )
   {
      if(pLen != NULL)
         * pLen = sizeof(USHORT);
      return (void *)ULONG(ArrayNums[pos]);
   }
   else
   {
      if(pLen != NULL)
         * pLen = ArrayAssociati[pos].Length;
      return ArrayAssociati[pos].Dati;
   }
}

//----------------------------------------------------------------------------
// Versione con accesso per indice
//----------------------------------------------------------------------------
void * ELENCO_Oggetti::AreaAssociata(unsigned int pos,USHORT * pLen)const
{
   //Null se non trovo l' oggetto o se NON ha area associata.
   if(!HoAssociati)
   {
      // Se non ho dati da ritornare
      if(pLen != NULL)
         * pLen = 0;
      return NULL;
   }
   if(HoAssociati <  Generic )
   {
      if(pLen != NULL)
         * pLen = sizeof(USHORT);
      return (void *)ULONG(ArrayNums[pos]);
   }
   else
   {
      if(pLen != NULL)
         * pLen = ArrayAssociati[pos].Length;
      return ArrayAssociati[pos].Dati;
   }
}

//----------------------------------------------------------------------------
// Accesso a dati Numerici associati
//----------------------------------------------------------------------------
USHORT ELENCO_Oggetti::NumAssociato(const class OGGETTO * Ogg)const
{
   // 0 se non trovo l' oggetto o se NON ha Dato associato.
   if(!HoAssociati || HoAssociati >=  Generic)
      return 0;
   int pos = CercaPerPuntatore(Ogg);
   if(pos < 0)
      return 0;
   return ArrayNums[pos];
}

USHORT ELENCO_Oggetti::NumAssociato(unsigned int pos)const
{
// 0 se non trovo l' oggetto o se NON ha Dato associato.
   if(!HoAssociati || HoAssociati >=  Generic)
      return 0;
   return ArrayNums[pos];
}

//----------------------------------------------------------------------------
// Operatore []  : Accede all'iesimo oggetto e lo ritorna per reference
//----------------------------------------------------------------------------

class OGGETTO & ELENCO_Oggetti::operator[](unsigned int idx) const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::Operator[]"
      if(idx < NumOggetti) return * (ArrayOggetti[idx]);
      ERRSTRING("Bound check error: Indice =  "+STRINGA(idx)+" Dim attuale = "+STRINGA(NumOggetti));
      BEEP;
      return * (OGGETTO *)NULL;
};

//----------------------------------------------------------------------------
// Operatore [] : Accede all' oggetto col nome indicato e lo ritorna per
// reference
//----------------------------------------------------------------------------
class OGGETTO & ELENCO_Oggetti::operator[](const STRINGA & Target) const
{
      return * CercaPerNome(Target);
};

//----------------------------------------------------------------------------
// Operatore []  : Accede all'oggetto indicato dal puntatore e lo ritorna
// per reference
//----------------------------------------------------------------------------
class OGGETTO & ELENCO_Oggetti::operator[](const class OGGETTO * Target) const
{
      int j = CercaPerPuntatore(Target);
      if(j == -1)
         return * (OGGETTO *)NULL;
      return * ArrayOggetti[j];
};

//----------------------------------------------------------------------------
// Dim : Ritorna il numero di oggetti contenuti nell' elenco
// E' un inline, definito direttamente nel .h
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CercaPerNome : Cerca l'oggetto con il nome indicato e ritorna il suo
// puntatore o NULL se non lo trova
//----------------------------------------------------------------------------
OGGETTO * ELENCO_Oggetti::CercaPerNome(const STRINGA & Nome_da_cercare,
const OGGETTO * Last)const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::CercaPerNome"
   TRACEPOINTER("Cerco per nome nell' Elenco Oggetti ",this);
   TRACESTRING2("Cerco oggetto ",(CPSZ)Nome_da_cercare);
   int Init = 0;
   if(Last != NULL)
   {
      Init = CercaPerPuntatore(Last);
      if(Init < 0)
         Init = 0;
      TRACEINT("Inizio ricerca da posizione ", Init);
   }
   // --------------------------------------------------------
   // invece di utilizzare l' operatore ==
   // lo esplodo nelle operazioni elementari, per velocizzare
   // Inoltre opero tramite addizione di puntatori perche'
   // sono piu' veloci delle moltiplicazioni
   // Questo e' infatti uno dei colli di bottiglia di OOLIB
   // --------------------------------------------------------
   //for(int i = Init, Dimn = Dim(); i < Dimn; i++)
   //{
   //   if(Nome_da_cercare == ArrayOggetti[i]->Nome)
   //   {
   //      TRACELONG("Trovato alla posizione", i);
   //      return ArrayOggetti[i];
   //   }
   //}
   // --------------------------------------------------------
   // Debbo rispettare l' ordine degli oggetti
   OGGETTO ** Idx  = ArrayOggetti +Init;
   OGGETTO ** Stop = ArrayOggetti +Dim();
   USHORT TargetLen  = (& Nome_da_cercare == NULL) ? 0 : Nome_da_cercare.Len;
   int i = -1;
   if (TargetLen) {
      char * TargetData = Nome_da_cercare.Dati;
      while(Idx < Stop){
         if((*Idx)->Nome.Len == TargetLen && !strcmp((*Idx)->Nome.Dati,TargetData) ) { // Stringhe eguali
            i = Idx - ArrayOggetti;
            break;
         } /* endif */
         Idx ++;
      }
   } else {
      while(Idx < Stop){
         if((*Idx)->Nome.Len == 0) { // Stringa vuota
            i = Idx - ArrayOggetti;
            break;
         } /* endif */
         Idx ++;
      }
   } /* endif */
   if (i >= 0) {
       TRACELONG("Trovato alla posizione", i);
       return ArrayOggetti[i];
   } else {
       TRACESTRING("Non Trovato");
       return (OGGETTO *)NULL;
   } /* endif */
}

//----------------------------------------------------------------------------
// CercaPerValore : ritorna il puntatore all' oggetto dato il Valore
// NULL se non trovato
//----------------------------------------------------------------------------
OGGETTO * ELENCO_Oggetti::CercaPerValore( const STRINGA & Valore_da_cercare,
                                          const OGGETTO * Last ) const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::CercaPerValore"

   TRACEPOINTER("Cerco per valore nell' Elenco Oggetti ",this);
   TRACESTRING2("Cerco valore ",(CPSZ)Valore_da_cercare);
   int Init=0;
   if(Last != NULL)
   {
      Init=CercaPerPuntatore(Last);
      if(Init < 0)
         Init = 0;
      TRACEINT("Inizio ricerca da posizione ",Init);
   }

   // --------------------------------------------------------
   // invece di utilizzare l' operatore ==
   // lo esplodo nelle operazioni elementari, per velocizzare
   // Inoltre opero tramite addizione di puntatori perche'
   // sono piu' veloci delle moltiplicazioni
   // Questo e' infatti uno dei colli di bottiglia di OOLIB
   // --------------------------------------------------------
   //for(int i=Init,Dimn=Dim();i < Dimn;i++)
   //{
   //   if(Valore_da_cercare == ArrayOggetti[i]->Valore())
   //   {
   //      TRACELONG("Trovato alla posizione",i);
   //      return ArrayOggetti[i];
   //   }
   //}
   // --------------------------------------------------------
   // Debbo rispettare l' ordine degli oggetti
   OGGETTO ** Idx  = ArrayOggetti +Init;
   OGGETTO ** Stop = ArrayOggetti +Dim();
   USHORT TargetLen  = (& Valore_da_cercare == NULL) ? 0 : Valore_da_cercare.Len;
   int i = -1;
   if (TargetLen) {
      char * TargetData = Valore_da_cercare.Dati;
      while(Idx < Stop){
         const STRINGA & Valor = (*Idx)->Valore();
         if(Valor.Len == TargetLen && !strcmp(Valor.Dati,TargetData) ) { // Stringhe eguali
            i = Idx - ArrayOggetti;
            break;
         } /* endif */
         Idx ++;
      }
   } else {
      while(Idx < Stop){
         if((*Idx)->Valore().Len == 0) { // Stringa vuota
            i = Idx - ArrayOggetti;
            break;
         } /* endif */
         Idx ++;
      }
   } /* endif */
   if (i >= 0) {
       TRACELONG("Trovato alla posizione", i);
       return ArrayOggetti[i];
   } else {
       TRACESTRING("Non Trovato");
       return (OGGETTO *)NULL;
   } /* endif */
}


//----------------------------------------------------------------------------
// CercaPerTipo : Crea un elenco temporaneo in cui mette solo gli oggetti
// dell' elenco appartenenti al tipo indicato.
// L' operazione e' efficiente se vi sono POCHI oggetti del tipo indicato:
// altrimenti sarebbe stato piu' efficiente evitare di fare tanti += e
// accumulare in un' array.
//----------------------------------------------------------------------------
ELENCO_Oggetti ELENCO_Oggetti::CercaPerTipo(TIPOGGE Tipo)const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::CercaPerTipo"
   ELENCO_Oggetti Out;
   ORD_FORALL(* this, i)
   {
      if(ArrayOggetti[i]->TipoOggetto == Tipo)
      {
         Out += * ArrayOggetti[i];
      }
   }
   return Out;
}

//----------------------------------------------------------------------------
// CercaPerNumAssoc : Crea un elenco temporaneo in cui mette solo gli oggetti
// dell' elenco con associato il numero Indicato.
// Se Num = 0 ritorna gli oggetti SENZA numero associato.
//----------------------------------------------------------------------------
ELENCO_Oggetti ELENCO_Oggetti::CercaPerNumAssoc(USHORT Num)const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::CercaPerNumAssoc"
   ELENCO_Oggetti Out;
   if(HoAssociati && HoAssociati < Generic )
   {
      ORD_FORALL(*this,i)
      {
         if(ArrayNums[i] == Num)
         {
            Out += * ArrayOggetti[i];
         }
      }
   }
   else
   {
      if(Num == 0)   // Nessuno ha un numero associato
         Out = THIS;
   }
   return Out;
}

//----------------------------------------------------------------------------
// CercaNonAssoc : Crea un elenco temporaneo in cui mette solo gli oggetti
// dell' elenco senza associazioni di tipo applicativo
//----------------------------------------------------------------------------
ELENCO_Oggetti ELENCO_Oggetti::CercaNonAssoc()const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::CercaNonAssoc"
   if(TipoAreaAssociata() != ELENCO_Oggetti::Relaz_ID) return THIS;  // Opera per copia
   ELENCO_Oggetti Out;
   ORD_FORALL(*this,i) {
     if(ArrayNums[i] == 0) Out += * ArrayOggetti[i];
   }
   return Out;
}


//----------------------------------------------------------------------------
// ContaPerNome : Ritorna il numero degli oggetti col nome indicato
//----------------------------------------------------------------------------
USHORT ELENCO_Oggetti::ContaPerNome(const STRINGA& Nome_da_cercare)const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::ContaPerNome"
   USHORT Out=0;
   FORALL(*this,i)
   {
      if(ArrayOggetti[i]->Nome == Nome_da_cercare)
      {
         Out ++;
      }
   }
   return Out;
}

//----------------------------------------------------------------------------
// ContaPerTipo : Ritorna il numero degli oggetti del tipo indicato
//----------------------------------------------------------------------------
USHORT ELENCO_Oggetti::ContaPerTipo(TIPOGGE Tipo)const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::ContaPerTipo"
   USHORT Out=0;
   FORALL(*this,i)
   {
      if(ArrayOggetti[i]->TipoOggetto == Tipo)
         Out ++;
   }
   return Out;
}

//----------------------------------------------------------------------------
// ContaPerNumAssoc : Ritorna il numero degli oggetti con quel Num Associato.
// Se Num = 0 ritorna gli oggetti SENZA numero associato.
//----------------------------------------------------------------------------
USHORT ELENCO_Oggetti::ContaPerNumAssoc(USHORT Num)const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::ContaPerNumAssoc"
   USHORT Out=0;
   if(HoAssociati && HoAssociati < Generic )
   {
      FORALL(*this,i)
      {
         if(ArrayNums[i] == Num)
            Out ++;
      }
   }
   else
   {
      if(Num == 0)
         Out = NumOggetti;
   }
   return Out;
}


//----------------------------------------------------------------------------
// CercaPerPuntatore : Torna un puntatore all' oggetto o -1 se non trovato
//----------------------------------------------------------------------------
int ELENCO_Oggetti::CercaPerPuntatore(const OGGETTO * Oggetto_da_cercare)const
{
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::CercaPerPuntatore"
   TRACEPOINTER2("Cerco per puntatore nell' Elenco Oggetti, oggetto ",this,(CPSZ)Oggetto_da_cercare->Nome);
   // -------------------------------------------------------
   // modificato il codice per massima efficienza
   // la ricerca procede all' indietro perche' e' piu'
   // probabile che richeida oggetti creati recentemente
   // -------------------------------------------------------
   //FORALL(* this, i){
   //   if(Oggetto_da_cercare == ArrayOggetti[i]) {
   //      TRACELONG("Trovato alla posizione", i);
   //      return i;
   //   }
   //}
   // -------------------------------------------------------
   if(Dim() >0){ // Test necessario perche' vado a scendere
      OGGETTO ** Idx  = ArrayOggetti +Dim() -1;
      while(Idx >= ArrayOggetti){
         if(Oggetto_da_cercare == (*Idx)) {
            int i = Idx - ArrayOggetti;
            TRACELONG("Trovato alla posizione", i);
            return i;
         }
         Idx --;
      }
   }
   TRACESTRING("Non Trovato");
   return -1;
}

//----------------------------------------------------------------------------
// Sort: Sorting degli Oggetti contenuti nell'elenco di Oggetti con una
//       funzione di comparazione fornita dall'utente.
//----------------------------------------------------------------------------
// Per il momento non e' prevista la possibilit… di sortare un elenco di
// Oggetti che abbia dei dati associati.
//----------------------------------------------------------------------------
ELENCO_Oggetti& ELENCO_Oggetti::Sort( PFN_QSORT* Compare )
{
   if( Compare == NULL || !NumOggetti || HoAssociati)
      return THIS;

// Creo una copia temporanea.
   ELENCO_Oggetti Temp ( THIS );

   qsort(Temp.ArrayOggetti,Temp.NumOggetti,sizeof(OGGETTO*), Compare);

// Assegno all'elenco la nuova impostazione.
   THIS = Temp;
   return THIS;
}
