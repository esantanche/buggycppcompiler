//========================================================================
// MM_BASIC :
//========================================================================
//
#pragma option -O2 -O-c -O-z      // Mi serve il massimo delle performance (NB: Oz rallenta)

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2


#include "BASE.HPP"
#include "MM_VARIE.HPP"
#include "MM_BASIC.HPP"
#include "MYALLOC.H"


//----------------------------------------------------------------------------
// Defines opzionali di DEBUG
//----------------------------------------------------------------------------
//#define DBG1  // Segue allocazione a livello di blocco, ed informazioni di massima su memoria utilizzata
//#define DBG2  // Segue ogni singola allocazione / deallocazione
//#define DBG3  // Traccia input ed output della funzione di Hash
//#define TEST_HASH // Compila routine di aiuto al debugging
//----------------------------------------------------------------------------


DWORD BASE_HASH::Random[32];


// ---------------------------------------------------------------------
// BASE_ALLOCATOR::MinBlocco
// ---------------------------------------------------------------------
// Cerca il blocco con il minor numero di elementi in uso e lo mette in testa alla lista
BASE_ALLOCATOR::BLOCCO * MMB_FASTCALL BASE_ALLOCATOR::L_BLOCCHI::MinBlocco(){
   
   #undef TRCRTN
   #define TRCRTN "BASE_ALLOCATOR::MinBlocco"
   
   if(Root == NULL)return NULL; // Lista vuota
   
   USHORT Used  = Root->NumUsed;
   BLOCCO * Wrk = Root;
   BLOCCO ** Padre = NULL;  // E' il puntatore al blocco con il minimo numero di elementi usati
   while (Wrk->Next) {
      if(Wrk->Next->NumUsed < Used){
         Used  = Wrk->Next->NumUsed;
         Padre = &(Wrk->Next);
      }
      Wrk = Wrk->Next;
   } /* endwhile */
   if(Padre){
      Wrk = *Padre;       // Blocco con il minimo numero di elementi usati
      *Padre = Wrk->Next; // Tolgo dalla linked list
      Push(Wrk);          // E lo metto in testa
      
      return Root;
   } else {
      return NULL;
   }
//<<< BASE_ALLOCATOR::BLOCCO * MMB_FASTCALL BASE_ALLOCATOR::L_BLOCCHI::MinBlocco   
};

// ---------------------------------------------------------------------
// BASE_ALLOCATOR::Get
// ---------------------------------------------------------------------
void * MMB_FASTCALL BASE_ALLOCATOR::Get (){
   #undef TRCRTN
   #define TRCRTN "BASE_ALLOCATOR::Get"
   
   TotGet ++;  // A fini statistici
   
   if(ListaBlocchi.Vuota()){ // All' inizio o dopo un reset
      NumBlocchiTotali ++;
      ListaBlocchi.Push(AllocaBLOCCO());
      #ifdef DBG1
      TRACEPOINTER2("Allocato primo blocco ad indirizzo , Allocator",ListaBlocchi.First(),this);
      #endif
      // Non tocco NumBlocchiVuoti perche' il blocco tra un attimo non e' piu' vuoto
   } else if(ListaBlocchi.First()->Pieno()){
      #ifdef DBG1
      TRACEPOINTER("Blocco pieno: ne serve un altro ; Allocator",this);
      #endif
      if(ListaBlocchi.MinBlocco() == NULL){ // Metto in testa se posso un elemento con possibilita' di allocare
         NumBlocchiTotali ++;
         ListaBlocchi.Push(AllocaBLOCCO());
         #ifdef DBG1
         TRACEPOINTER2("Allocato nuovo blocco ad indirizzo , Allocator",ListaBlocchi.First(),this);
         #endif
         // Non tocco NumBlocchiVuoti perche' il blocco tra un attimo non e' piu' vuoto
      } else if(!ListaBlocchi.First()->NumUsed){  // Era vuoto ed ora non lo sara' piu'
         NumBlocchiVuoti --;         // Ho un blocco vuoto in meno
      }
//<<< if ListaBlocchi.Vuota     // All' inizio o dopo un reset
   }
   
   BLOCCO & Owner = *ListaBlocchi.First();
   
   void * Out;
   if (Owner.NextDati < Owner.MaxDati) { // Posso allocare sequenzialmente
      Out = Owner.NextDati ;
      Owner.NextDati +=  Size;
   } else {                              // Lo prendo dalla free list
      Out = Owner.FreeList ;
      Owner.FreeList  = *(void **)Owner.FreeList;
   } /* endif */
   Owner.NumUsed ++;
   #ifdef DBG2
   TRACEVPOINTER(Out);
   #endif
   return Out;
//<<< void * MMB_FASTCALL BASE_ALLOCATOR::Get    
};

// ---------------------------------------------------------------------
// BASE_ALLOCATOR::Free
// ---------------------------------------------------------------------
void   MMB_FASTCALL BASE_ALLOCATOR::Free (void* Dati){ // Se Dati non e' un' area allocata da ALLOCATOR non fa nulla
   #undef TRCRTN
   #define TRCRTN "BASE_ALLOCATOR::Free"
   
   TotFree ++;  // A fini statistici
   
   #ifdef DBG2
   TRACEVPOINTER(Dati);
   #endif
   // Cerco il blocco da cui ho allocato i dati
   BLOCCO * Owner = ListaBlocchi.First();
   while (Owner) {
      if(Dati >= Owner->Dati && Dati < Owner->MaxDati){
         break;
      };
      Owner = Owner->Next;
   } /* endwhile */

   assert(Owner != NULL); // Richiesta free su area non allocato da questo allocator
   
   if(Owner){
      Owner->NumUsed --;
      if(Owner->NumUsed == 0){ // Il blocco e' divenuto vuoto
         if(NumBlocchiVuoti >= 2){ // Va proprio deallocato
            NumBlocchiTotali --;
            #ifdef DBG1
            TRACEPOINTER("Cancellato un blocco, Numero blocchi presenti = "+STRINGA(NumBlocchiTotali)+"; Allocator",this);
            #endif
            ListaBlocchi.Del(Owner); // Lo tolgo dalla linked list
            free(Owner);           // Lo dealloco fisicamente
         } else {
            #ifdef DBG1
            TRACEPOINTER("Divenuto LIBERO un blocco, Numero blocchi presenti = "+STRINGA(NumBlocchiTotali)+"; Allocator",this);
            #endif
            NumBlocchiVuoti ++;      // Ho un blocco vuoto in piu'
            Owner->Init();           // E' tutto vuoto
         } /* endif */
      } else {  // Aggiungo alla free list
         *(void**)Dati  = Owner->FreeList;  // Inserisco nella free list
         Owner->FreeList = Dati;
      }
//<<< if Owner  
   };
//<<< void   MMB_FASTCALL BASE_ALLOCATOR::Free  void* Dati   // Se Dati non e' un' area allocata da ALLOCATOR non fa nulla
};

// ---------------------------------------------------------------------
// BASE_ALLOCATOR::Reset
// ---------------------------------------------------------------------
void   BASE_ALLOCATOR::Reset (){  // Dealloca (fisicamente) tutto
   #undef TRCRTN
   #define TRCRTN "BASE_ALLOCATOR::Reset"
   
   #ifdef DBG1
   TRACEPOINTER("Richiesta di liberare fisicamente tutti i blocchi; Allocator",this);
   #endif
   BLOCCO * Wrk      = ListaBlocchi.Root;
   NumBlocchiVuoti   = 0 ;
   NumBlocchiTotali  = 0 ;
   ListaBlocchi.Root = NULL;
   while (Wrk) {
      BLOCCO * Next = Wrk->Next;
      free(Wrk);
      Wrk = Next;
   } /* endwhile */
};

// ---------------------------------------------------------------------
// BASE_ALLOCATOR::Clear
// ---------------------------------------------------------------------
void   BASE_ALLOCATOR::Clear (){  // Dealloca logicamente tutto, ma lascia uno o due blocchi vuoti allocati
   #undef TRCRTN
   #define TRCRTN "BASE_ALLOCATOR::Clear"
   NumBlocchiVuoti   = 0 ;
   #ifdef DBG1
   TRACEPOINTER("Richiesta di liberare logicamente tutti i blocchi; Allocator",this);
   #endif
   BLOCCO * Wrk = ListaBlocchi.Root;
   if(Wrk){
      Wrk->Init();
      Wrk = Wrk->Next;
      if(Wrk){
         Wrk->Init();
         BLOCCO * Tmp = Wrk;
         Wrk = Wrk->Next;
         Tmp->Next = NULL;
         while (Wrk) {
            BLOCCO * Next = Wrk->Next;
            NumBlocchiTotali --;
            free(Wrk);
            Wrk = Next;
         } /* endwhile */
      }
   }
   #ifdef DBG1
   TRACEPOINTER("OK: Blocchi rimasti = "+STRINGA(NumBlocchiTotali)+"; Allocator",this);
   #endif
//<<< void   BASE_ALLOCATOR::Clear      // Dealloca logicamente tutto, ma lascia uno o due blocchi vuoti allocati
};

// ---------------------------------------------------------------------
// @BASE_ALLOCATOR
// ---------------------------------------------------------------------
BASE_ALLOCATOR::BASE_ALLOCATOR (USHORT DimElemento, USHORT NumElementiPerBlocco ) {
   #undef TRCRTN
   #define TRCRTN "@BASE_ALLOCATOR"
   
   Size              = max(DimElemento,4);
   DimBlocco         = NumElementiPerBlocco * Size;
   NumElementiBlocco = NumElementiPerBlocco ;
   NumBlocchiVuoti   = 0 ;
   NumBlocchiTotali  = 0 ;
   
   TotGet            = 0 ;
   TotFree           = 0 ;
   
   #ifdef DBG1
   TRACEPOINTER("Allocator creato , Size elemento = "+STRINGA(Size)+" ad indirizzo ",this);
   #endif
   
};

// ---------------------------------------------------------------------
// BASE_ALLOCATOR::Trace
// ---------------------------------------------------------------------
void  BASE_ALLOCATOR::Trace(const STRINGA&  Msg, int Livello ){
   #undef TRCRTN
   #define TRCRTN "BASE_ALLOCATOR::Trace"
   if(Livello > trchse)return;
   ULONG BlocchiInUso = 0;
   BLOCCO * Wrk = ListaBlocchi.First();
   while(Wrk){ if(Wrk->NumUsed > 0)BlocchiInUso ++;Wrk = Wrk->Next;};
   ERRSTRING( Msg );
   ERRSTRING( VRS(NumBlocchiTotali)
      + VRS(NumElementiBlocco)
      + VRS(ElementiInUso())
      + VRS(ElementiNonInUso())
      + VRS(BlocchiInUso)
   );
}
// ---------------------------------------------------------------------
// BASE_ALLOCATOR::AllocaBLOCCO
// ---------------------------------------------------------------------
BASE_ALLOCATOR::BLOCCO * MMB_FASTCALL BASE_ALLOCATOR::AllocaBLOCCO(){
   #undef TRCRTN
   #define TRCRTN "BASE_ALLOCATOR::AllocaBLOCCO"
   #ifdef DBG1
   TRACELONG("Richiesta allocazione blocco di Bytes ",DimBlocco);
   #endif
   BLOCCO * Wrk = (BLOCCO*)malloc( DimBlocco + sizeof(BLOCCO));
   MEMIGNORE(Wrk);
   Wrk->Next              = NULL;
   Wrk->Dati              = (void*)(Wrk+1);
   Wrk->MaxDati           = (BYTE *)Wrk->Dati + DimBlocco;
   Wrk->NumElementiBlocco = NumElementiBlocco;
   Wrk->Init();
   #ifdef DBG1
   TRACEPOINTER("Allocato blocco ad indirizzo",Wrk);
   #endif
   return Wrk;
};



// ---------------------------------------------------------------------
// @BASE_HASH
// ---------------------------------------------------------------------
BASE_HASH::BASE_HASH(BYTE Log2DimMatrice, USHORT DimElemento,USHORT NumDatiAllocIniziali):
Allocator(DimElemento+sizeof(ESTENSIONE),NumDatiAllocIniziali)
{
   #undef TRCRTN
   #define TRCRTN "@BASE_HASH"
   
   // Tabella dei pseudorandom: vedere Knuth
   if(Random[0] == 0){
      DWORD Rand = 0xABCDEF01;
      for (int i = 0;i < 32 ; i++) {
         Random[i]  = Rand;
         Rand = Rand * 5 + 0xABCDEF01;
      } /* endfor */
   }
   
   NumCelle = 1 << Log2DimMatrice; // potenza di due
   Celle = new CELLA[NumCelle];
   MEMIGNORE(Celle);
   HashMask = NumCelle-1;
   NextElemento=NULL;
   NumCellaCorrente = 0;
   ElementoCorrente = NULL;
   
//<<< Allocator DimElemento+sizeof ESTENSIONE ,NumDatiAllocIniziali 
};
BASE_HASH::~BASE_HASH(){ delete Celle;};


// ---------------------------------------------------------------------
// BASE_HASH::Clear
// ---------------------------------------------------------------------
void  MMB_FASTCALL BASE_HASH::Clear (){
   
   // Dealloca logicamente, mantiene le allocazioni fisiche
   #undef TRCRTN
   #define TRCRTN "BASE_HASH::Clear"
   
   memset(Celle,0,NumCelle*sizeof(CELLA)); // Truccaccio: attenzione alle modifiche della classe Cella
   Allocator.Clear();   // Dealloca l' area degli elementi
   NextElemento = NULL;
   NumCellaCorrente = 0;
   ElementoCorrente = NULL;
   CurrentDeleted = FALSE;
};

// ---------------------------------------------------------------------
// BASE_HASH::First
// ---------------------------------------------------------------------
void * MMB_FASTCALL BASE_HASH::First(ITERATORE & i){
   #undef TRCRTN
   #define TRCRTN "BASE_HASH::First"
   
   i.ItCurrentDeleted = FALSE;
   for(i.ItNumCellaCorrente = 0; i.ItNumCellaCorrente < NumCelle; i.ItNumCellaCorrente ++){
      i.ItElementoCorrente = Celle[i.ItNumCellaCorrente].First();
      if(i.ItElementoCorrente)return i.ItElementoCorrente->Dati();
   }
   // Fine ricerca
   i.ItNumCellaCorrente = 0;
   i.ItElementoCorrente = NULL;
   return NULL;
};

// ---------------------------------------------------------------------
// BASE_HASH::Next
// ---------------------------------------------------------------------
void * MMB_FASTCALL BASE_HASH::Next (ITERATORE & i){
   #undef TRCRTN
   #define TRCRTN "BASE_HASH::Next"
   if(i.ItElementoCorrente == NULL)return NULL;
   if (i.ItCurrentDeleted) {
      i.ItCurrentDeleted = FALSE;
      if(i.ItElementoCorrente){
         return i.ItElementoCorrente->Dati();
      } else {
         i.ItNumCellaCorrente = 0;
         return NULL;
      }
   } /* endif */
   if(i.ItElementoCorrente->Next){
      i.ItElementoCorrente = i.ItElementoCorrente->Next;
      return i.ItElementoCorrente->Dati();
   } else {
      while(++ i.ItNumCellaCorrente < NumCelle){
         i.ItElementoCorrente = Celle[i.ItNumCellaCorrente].First();
         if(i.ItElementoCorrente)return i.ItElementoCorrente->Dati();
      }
      // Fine ricerca
      i.ItNumCellaCorrente = 0;
      i.ItElementoCorrente = NULL;
      return NULL;
   }
//<<< void * MMB_FASTCALL BASE_HASH::Next  ITERATORE & i  
};

//----------------------------------------------------------------------------
// BASE_HASH::DeleteCurrentlyIterated
//----------------------------------------------------------------------------
void BASE_HASH::DeleteCurrentlyIterated(ITERATORE &i){ // Cancella l' elemento corrente (nella iterazione) dalla HASH
   #undef TRCRTN
   #define TRCRTN "BASE_HASH::DeleteCurrentlyIterated"
   ITERATORE B = i;
   if(B.ItElementoCorrente && ! B.ItCurrentDeleted){
      Next(i);
      Celle[B.ItNumCellaCorrente].LISTA<ESTENSIONE>::Del(B.ItElementoCorrente);
      Allocator.Free(B.ItElementoCorrente);
      i.ItCurrentDeleted   = TRUE;
   };
};

//----------------------------------------------------------------------------
// BASE_HASH::CercaNext
//----------------------------------------------------------------------------
void * MMB_FASTCALL BASE_HASH::CercaNext() { // Si deve chiamare prima  Cerca()
   #undef TRCRTN
   #define TRCRTN "BASE_HASH::CercaNext"
   if(ElementoCorrente == NULL) return NULL;
   void * Base = ElementoCorrente->Dati();
   WORD Klen   = ElementoCorrente->KeyLen;
   if(!CurrentDeleted)ElementoCorrente = ElementoCorrente->Next;
   CurrentDeleted = FALSE;
   while( ElementoCorrente ){
      if(ElementoCorrente->KeyLen == Klen && memcmp(Base,ElementoCorrente->Dati(),Klen) == 0){
         return ElementoCorrente->Dati();
      }
      ElementoCorrente = ElementoCorrente->Next;
   }
   return NULL;
};
// ---------------------------------------------------------------------
// BASE_HASH::Istogram
// ---------------------------------------------------------------------
void   BASE_HASH::Istogram(const STRINGA& Msg)const{
   // Fa un istogramma della Hash-Table sul trace
   #undef TRCRTN
   #define TRCRTN "BASE_HASH::Istogram"
   
   // Istogramma fino a 32, poi > 32
   DWORD Counts[33];
   DWORD GT32 = 0;
   ULONG Tot  = 0;
   ZeroFill(Counts);
   SCAN_NUM(Celle,NumCelle,Cella,CELLA){
      DWORD Count = 0;
      ESTENSIONE * Ext = Cella.First();
      while (Ext) {
         Count ++;
         Ext = Ext->Next;
      } /* endwhile */
      if(Count > 32) {
         GT32 ++;
         Tot += Count;
      } else {
         Counts[Count]++;
      } /* endif */
   } ENDSCAN ;
   
   ERRSTRING(Msg);
   for (int i = 0;i < 33 ;i++ ) {
      if(Counts[i])Bprintf3("   Vi sono %i celle con catene lunghe %i",Counts[i],i);
      Tot += Counts[i] * i;
   } /* endfor */
   if(GT32)Bprintf3("   Vi sono %i celle con catene piu' lunghe di 32 elementi",GT32);
   ERRINT("Totale elementi nelle celle: ",Tot);
   
//<<< void   BASE_HASH::Istogram const STRINGA& Msg const 
};

// ---------------------------------------------------------------------
// Test
// ---------------------------------------------------------------------
#ifdef TEST_HASH
struct TEST {
   int Key;
   int Indice;
   int HeapPos;
};
inline int HEAP<TEST,2>::Confronta(TEST * a, TEST *b){return a->Key - b->Key;};
inline int HEAP<TEST,3>::Confronta(TEST * a, TEST *b){return a->Key - b->Key;};
inline int HEAP<TEST,4>::Confronta(TEST * a, TEST *b){return a->Key - b->Key;};
#endif

void _export MM_BASIC_TEST(){ // Funzioncina di test
   
   #define Limit 100000
   // #define ANCHE_DINAMICHE
   #ifdef TEST_HASH
   
   static ALLOCATOR<TEST>         Allocator(10000);
   static HEAP<TEST,4>            Heap4(Limit + 10);
   static HEAP<TEST,3>            Heap3(Limit + 10);
   static HEAP2<TEST,3>           Heap3T(Limit + 10);
   static HEAP<TEST,2>            Heap2(Limit + 10);
   static HASH<TEST>              Hash(15,10000);
   static ARRAY_DINAMICA<TEST>    Ad(Limit);
   
   int i,Last,LastIdx;
   DWORD Elapsed;
   
   Bprintf("Provo allocator");
   TEST ** Allocati = (TEST**)malloc(4 * Limit);
   Cronometra(TRUE,Elapsed);
   TEST ** Wrk = Allocati;
   for ( i = 0;i < Limit ; i++) {
      *Wrk =  Allocator.Get();
      Wrk ++;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per allocare %i = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Cronometra(TRUE,Elapsed);
   Wrk = Allocati;
   for ( i = 0;i < Limit/2 ; i++) {
      Allocator.Free(*Wrk); // Ne dealloco uno ogni due
      Wrk += 2;
   } /* endfor */
   Wrk = Allocati + 1;
   for ( i = 0;i < Limit/2 ; i++) {
      Allocator.Free(*Wrk); // Ne dealloco uno ogni due
      Wrk += 2;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per deallocare %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   
   free(Allocati);
   Allocator.Clear();
   
   // Costruisco la tabella di 100.000 interi casuali
   TEST *Rands = new TEST[Limit];
   DWORD Rand = 0x243565AF;
   for ( i = 0;i < Limit ; i++) {
      Rands[i].Key       = Rand & 0x3fffffff; // Limito il range utile
      Rands[i].Indice    = i;
      Rand = Rand * 5 + 0xABCDEF21;
   } /* endfor */
   
   Bprintf("Provo le hash table");
   TEST * Item = Rands;
   Cronometra(TRUE,Elapsed);
   for (i = 0; i <= Limit ;i++ ) {
      // Inserisco l' elemento nella tavola
      TEST & Test = * Hash.Alloca();
      Test = *Item;
      Item ++;
      Hash.Metti(4);
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per allocare ed inserire in hash %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   
   Item = Rands;
   Cronometra(TRUE,Elapsed);
   for ( i = 0;i <= Limit ; i++) {
      TEST * Test = Hash.Cerca(Item,4);
      if(Test == NULL){
         Bprintf("MANCATA RICERCA IN HASH TABLE i=%i",i);
         BEEP;
         break;
      } else if(Test->Key != Item->Key){
         Bprintf("MANCATA CORRISPONDENZA IN HASH TABLE i=%i",i);
         BEEP;
         break;
      }
      Item ++;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per estrarre da hash %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   
   Bprintf("Area allocata: %i",Hash.AreaAllocata());
   Hash.Istogram("Istogramma dei dati immessi");
   Hash.Clear();
   
   Bprintf("Provo le heaps D= 2");
   Cronometra(TRUE,Elapsed);
   Item = Rands;
   for (i = 0;i < Limit ; i++ ) {
      Heap2.Insert(Item);
      Item ++;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per inserire in Heap2 %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Cronometra(TRUE,Elapsed);
   Last = 0; // Sicuramente minore di tutti i rand (che ho generato positivi)
   for (i = 0;i < Limit ; i++ ) {
      TEST * x = Heap2.DeleteMin();
      // if(i <= 100)ERRSTRING("Key "+STRINGA(x->Key)+" Indice "+STRINGA(x->Indice));
      if(x->Key < Last || x->Indice == LastIdx ){
         Bprintf("ERRORE IN DELETEMIN i=%i",i);
         BEEP;
         break;
      }
      Last = x->Key; LastIdx = x->Indice;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per estrarre da Heap2 %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   if(Heap2.DeleteMin()){
      Bprintf("Errore: Heap2 dovrebbe essere vuota\n");
      BEEP;
   };
   Heap2.Clear();
   
   Bprintf("Provo le heaps D= 3");
   Cronometra(TRUE,Elapsed);
   Item = Rands;
   for (i = 0;i < Limit ; i++ ) {
      Heap3.Insert(Item);
      Item ++;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per inserire in Heap3 %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Cronometra(TRUE,Elapsed);
   Last = 0; // Sicuramente minore di tutti i rand (che ho generato positivi)
   for (i = 0;i < Limit ; i++ ) {
      TEST * x = Heap3.DeleteMin();
      // if(i <= 100)ERRSTRING("Key "+STRINGA(x->Key)+" Indice "+STRINGA(x->Indice));
      if(x->Key < Last || x->Indice == LastIdx ){
         Bprintf("ERRORE IN DELETEMIN i=%i",i);
         BEEP;
         break;
      }
      Last = x->Key; LastIdx = x->Indice;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per estrarre da Heap3 %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   if(Heap3.DeleteMin()){
      Bprintf("Errore: Heap3 dovrebbe essere vuota\n");
      BEEP;
   };
   Heap3.Clear();
   
   Bprintf("Provo le heaps D= 3T HOHEAP");
   Cronometra(TRUE,Elapsed);
   Item = Rands;
   for (i = 0;i < Limit ; i++ ) {
      Heap3T.Insert(Item);
      Item ++;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per inserire in Heap3T %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Heap3T.IncrementKey(Item);
   Cronometra(TRUE,Elapsed);
   Last = 0; // Sicuramente minore di tutti i rand (che ho generato positivi)
   for (i = 0;i < Limit ; i++ ) {
      TEST * x = Heap3T.DeleteMin();
      // if(i <= 100)ERRSTRING("Key "+STRINGA(x->Key)+" Indice "+STRINGA(x->Indice));
      if(x->Key < Last || x->Indice == LastIdx ){
         Bprintf("ERRORE IN DELETEMIN i=%i",i);
         BEEP;
         break;
      }
      Last = x->Key; LastIdx = x->Indice;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per estrarre da Heap3T %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   if(Heap3T.DeleteMin()){
      Bprintf("Errore: Heap3T dovrebbe essere vuota\n");
      BEEP;
   };
   Heap3T.Clear();
   
   Bprintf("Provo le heaps D= 4");
   Cronometra(TRUE,Elapsed);
   Item = Rands;
   for (i = 0;i < Limit ; i++ ) {
      Heap4.Insert(Item);
      Item ++;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per inserire in Heap4 %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Cronometra(TRUE,Elapsed);
   Last = 0; // Sicuramente minore di tutti i rand (che ho generato positivi)
   for (i = 0;i < Limit ; i++ ) {
      TEST * x = Heap4.DeleteMin();
      if(x->Key < Last || x->Indice == LastIdx ){
         Bprintf("ERRORE IN DELETEMIN i=%i",i);
         BEEP;
         break;
      }
      Last = x->Key; LastIdx = x->Indice;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per estrarre da Heap4 %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   if(Heap4.DeleteMin()){
      Bprintf("Errore: Heap4 dovrebbe essere vuota\n");
      BEEP;
   };
   Heap4.Clear();
   
   #ifdef ANCHE_DINAMICHE
   Bprintf("Provo le ArrayDinamiche");
   Cronometra(TRUE,Elapsed);
   Item = Rands;
   for (i = 0 ;i < Limit ; i++ ) {
      Ad += *Item;
      Item ++;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per inserire in Array dinamica %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Cronometra(TRUE,Elapsed);
   Item = Rands;
   for (i = 0 ;i < Limit ; i++ ) {
      int Ad_Dim = Ad.Dim();
      for (int j = 0; j < Ad_Dim ;j ++ ) {
         if(memcmp(&Ad[j],Item,sizeof(TEST))==0)break;
      } /* endfor */
      Item ++;
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("Tempo Necessario per cercare in Array dinamica %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Cronometra(TRUE,Elapsed);
   for (i = 0 ;i < Limit ; i++ ) {
      Ad -= 0; // Caso peggiore
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("WORST CASE Tempo Necessario per canellare da Array dinamica %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Item = Rands;
   for (i = 0 ;i < Limit ; i++ ) {
      Ad += *Item;
      Item ++;
   } /* endfor */
   Cronometra(TRUE,Elapsed);
   for (i = 0 ;i < Limit ; i++ ) {
      Ad -= Ad.Dim()-1; // Caso migliore
   } /* endfor */
   Cronometra(FALSE,Elapsed);
   Bprintf("BEST CASE Tempo Necessario per canellare da Array dinamica %i elementi = %i microsecondi media %i.%i",Limit, Elapsed,Elapsed/Limit,(10*Elapsed/Limit)%10);
   Ad.Clear();
   #endif
   
   delete []Rands;
   #endif
//<<< void _export MM_BASIC_TEST    // Funzioncina di test
};
//----------------------------------------------------------------------------
// SET: Classe utilizzata per la gestione di insiemi di dimensione arbitraria
//----------------------------------------------------------------------------
SET::SET(DWORD CardinalitaPrevista){
   #undef TRCRTN
   #define TRCRTN "@SET"
   BitsAllocati = 0;
   Dati = NULL;
   if(CardinalitaPrevista)Realloc(CardinalitaPrevista);
};
//----------------------------------------------------------------------------
// ~SET
//----------------------------------------------------------------------------
SET::~SET (){
   #undef TRCRTN
   #define TRCRTN "~SET"
   if(Dati)free(Dati);
};

//----------------------------------------------------------------------------
// SET::operator=
//----------------------------------------------------------------------------
SET & MMB_FASTCALL SET::operator= (SET & From){ // Copia
   #undef TRCRTN
   #define TRCRTN "SET::operator="
   // Queste variabili di appoggio mi servono ad evitare problemi con l' ottimizzatore
   int NumB   = From.BitsAllocati ;
   DWORD * Da = From.Dati ;
   if( NumB > 0){ // Ad evitare abend
      if( NumB < BitsAllocati)Clear();
      Realloc(NumB -1);
      memcpy( Dati, Da, NumB >>3);
   } else {
      Clear();
   }
   return THIS;
};


//----------------------------------------------------------------------------
// SET::Empty
//----------------------------------------------------------------------------
BOOL MMB_FASTCALL SET::Empty(){  // True se vuoto
   #undef TRCRTN
   #define TRCRTN "SET::Empty"
   
   if(BitsAllocati == 0)return TRUE; // Ad evitare abend
   for (int i = (BitsAllocati-1)/32; i >= 0 ; i -- ) {
      if(Dati[i])return FALSE;
   } /* endfor */
   return TRUE;
};

//----------------------------------------------------------------------------
// SET::operator|=
//----------------------------------------------------------------------------
SET & MMB_FASTCALL SET::operator|= (SET & Dato){ // Unione
   #undef TRCRTN
   #define TRCRTN "SET::operator|="
   if(Dato.BitsAllocati > 0){ // Ad evitare abend
      if(Dato.BitsAllocati > BitsAllocati) Realloc(Dato.BitsAllocati - 1);
      for (int i =(Dato.BitsAllocati-1)/32; i >= 0 ; i -- ) {
         Dati[i] |= Dato.Dati[i];
      } /* endfor */
   }
   return THIS;
};

//----------------------------------------------------------------------------
// SET::operator&=
//----------------------------------------------------------------------------
SET & MMB_FASTCALL SET::operator&= (SET & Dato){ // Intersezione
   #undef TRCRTN
   #define TRCRTN "SET::operator&="
   if(Dato.BitsAllocati > 0){ // Ad evitare abend
      if(Dato.BitsAllocati > BitsAllocati) Realloc(Dato.BitsAllocati - 1);
      for (int i =(Dato.BitsAllocati-1)/32; i >= 0 ; i -- ) {
         Dati[i] &= Dato.Dati[i];
      } /* endfor */
   } else {
      Clear();
   }
   return THIS;
};

//----------------------------------------------------------------------------
// SET::operator==
//----------------------------------------------------------------------------
BOOL  MMB_FASTCALL SET::operator== (SET & Dato){
   #undef TRCRTN
   #define TRCRTN "SET::operator=="
   if(Dato.BitsAllocati == 0){
      return (BitsAllocati == 0);
   } else if(BitsAllocati == 0){
      return FALSE;
   } else if(Dato.BitsAllocati > BitsAllocati){
      for (int i = (Dato.BitsAllocati-1)/32; i > (BitsAllocati-1)/32;  i-- ) {
         if(Dato.Dati[i])return FALSE;
      } /* endfor */
      for (i = (BitsAllocati-1)/32; i >=0 ;  i-- ) {
         if(Dati[i] != Dato.Dati[i])return FALSE;
      } /* endfor */
   } else {
      for (int i = (BitsAllocati-1)/32; i > (Dato.BitsAllocati-1)/32;  i-- ) {
         if(Dati[i])return FALSE;
      } /* endfor */
      for (i = (Dato.BitsAllocati-1)/32; i >=0 ;  i-- ) {
         if(Dati[i] != Dato.Dati[i])return FALSE;
      } /* endfor */
   }
   return TRUE;
//<<< BOOL  MMB_FASTCALL SET::operator==  SET & Dato  
};

//----------------------------------------------------------------------------
// SET::operator>=
//----------------------------------------------------------------------------
BOOL  MMB_FASTCALL SET::operator>= (SET & Dato){ // Inclusione
   #undef TRCRTN
   #define TRCRTN "SET::operator>="
   if(Dato.BitsAllocati == 0){
      return TRUE;
   } else if(BitsAllocati == 0){
      return FALSE;
   } else if(Dato.BitsAllocati > BitsAllocati){
      for (int i = (Dato.BitsAllocati-1)/32; i > (BitsAllocati-1)/32;  i-- ) {
         if(Dato.Dati[i])return FALSE;
      } /* endfor */
      for (i = (BitsAllocati-1)/32; i >=0 ;  i-- ) {
         if((~Dati[i]) & Dato.Dati[i] )return FALSE;
      } /* endfor */
   } else {
      for (int i = (Dato.BitsAllocati-1)/32; i >=0 ;  i-- ) {
         if((~Dati[i]) & Dato.Dati[i] )return FALSE;
      } /* endfor */
   }
   return TRUE;
};
//----------------------------------------------------------------------------
// SET::operator&
//----------------------------------------------------------------------------
BOOL  MMB_FASTCALL SET::operator&  (SET & Dato){ // Bit AND tra set TRUE se un bit e' set
   #undef TRCRTN
   #define TRCRTN "SET::operator&"
   int i;
   if(Dato.BitsAllocati == 0 || BitsAllocati == 0){
      return FALSE;
   } else if(Dato.BitsAllocati < BitsAllocati){
      i =(Dato.BitsAllocati-1)/32;
   } else {
      Realloc(Dato.BitsAllocati - 1);
      i =(BitsAllocati-1)/32;
   }
   for (; i >= 0 ; i -- ) {
      if(Dati[i] & Dato.Dati[i])return TRUE;
   } /* endfor */
   return FALSE;
};
//----------------------------------------------------------------------------
// SET::Reset
//----------------------------------------------------------------------------
void  MMB_FASTCALL SET::Reset(){    // Dealloca (fisicamente) tutto
   #undef TRCRTN
   #define TRCRTN "SET::Reset"
   if(Dati)free(Dati);
   Dati = NULL;
   BitsAllocati=0;
};
//----------------------------------------------------------------------------
// SET::Clear
//----------------------------------------------------------------------------
void  MMB_FASTCALL SET::Clear(){ // Vuota l' insieme
   #undef TRCRTN
   #define TRCRTN "SET::Clear"
   BitsAllocati = 0; // Questo velocizza comunque quando effettuo la realloc()
   Dati[0]      = 0;
};

// appoggio su area di dimensione fissa e viceversa
//----------------------------------------------------------------------------
// SET::ToFix
//----------------------------------------------------------------------------
void MMB_FASTCALL SET::ToFix(void * To, ULONG NumBits){
   #undef TRCRTN
   #define TRCRTN "SET::ToFix"
   if( NumBits <= BitsAllocati){
      if(NumBits)memmove(To,Dati,(NumBits +7)/8);
   } else if( BitsAllocati == 0){
      memset(To,0,(NumBits + 7)/8 );
   } else {
      memmove(To,Dati,(BitsAllocati +7)/8);
      memset((BYTE*)To+(BitsAllocati +7)/8,0,(NumBits + 7)/8 - (BitsAllocati +7)/8);
   }
};
//----------------------------------------------------------------------------
// SET::FromFix
//----------------------------------------------------------------------------
void MMB_FASTCALL SET::FromFix(void * From,ULONG NumBits){
   #undef TRCRTN
   #define TRCRTN "SET::FromFix"
   if(NumBits){
      Realloc(NumBits - 1);
      memmove(Dati,From,(NumBits +7)/8);
   } else Clear();
};
// Queste sono per debugging
//----------------------------------------------------------------------------
// SET::ToHex
//----------------------------------------------------------------------------
STRINGA  MMB_FASTCALL SET::ToHex(){  // Stringa esadecimale
   #undef TRCRTN
   #define TRCRTN "SET::ToHex"
   CPSZ ToHex="0123456789ABCDEF";
   STRINGA Out;
   BYTE * Dat = (BYTE*)Dati;
   for (int i = 0 ; i < BitsAllocati/8 ; i ++  ) {
      Out += ToHex[Dat[i]%0x10];
      Out += ToHex[Dat[i]/0x10];
   } /* endfor */
   return Out;
};

//----------------------------------------------------------------------------
// SET::ToBit
//----------------------------------------------------------------------------
STRINGA MMB_FASTCALL SET::ToBit(){  // Stringa Binaria (' '/'*'): Max 250 caratteri
   #undef TRCRTN
   #define TRCRTN "SET::ToBit"
   char Dati[255];
   int Bd = (BitsAllocati > 250) ? 250 : BitsAllocati;
   for (int i = 0;i < Bd ; i++ ) {
      Dati[i] = Test(i) ? '*' : ' ';
   } /* endfor */
   Dati[Bd] = 0;
   return STRINGA(Dati);
};
//----------------------------------------------------------------------------
// SET::ToInt
//----------------------------------------------------------------------------
STRINGA MMB_FASTCALL SET::ToInt(){  // Stringa di interi corrispondenti ai bit settati ; Max 128
   #undef TRCRTN
   #define TRCRTN "SET::ToInt"
   ARRAY_ID Tmp;
   int Bd = (BitsAllocati > 128) ? 128 : BitsAllocati;
   for (int i = 0;i < Bd ; i++ ) {
      if(Test(i))Tmp += i;
   } /* endfor */
   return Tmp.ToStringa();
};

//----------------------------------------------------------------------------
// InitNumBits
//----------------------------------------------------------------------------
BYTE NumBits[256];
int InitNumBits(){
   #undef TRCRTN
   #define TRCRTN "InitNumBits"
   ZeroFill(NumBits);
   for (int i = 0;i < 256 ;i++ ) for (int k = 0;k < 8 ;k++ ) if((i >> k) & 1)NumBits[i]++;
   return 0;
}
static int Dummy = InitNumBits();
//----------------------------------------------------------------------------
// SET::Cardinalita
//----------------------------------------------------------------------------
DWORD  MMB_FASTCALL SET::Cardinalita(){ // Ritorna il numero di elementi impostati ad 1;
   #undef TRCRTN
   #define TRCRTN "SET::Cardinalita"
   int Out=0;
   DWORD * Dat = Dati;
   for (int i = BitsAllocati/32 ; i>0;  i --  ) {
      Out += NumeroBits(*Dat);
      Dat ++;
   } /* endfor */
   return Out;
};

// Questa funzione non vuole ottimizzazioni
#pragma option -Od   // Su questo modulo l' ottimizzatore crea problemi
// ---------------------------------------------------------------------
// BASE_HASH::Hash"
// ---------------------------------------------------------------------
// Derivata dalla funzione di hash usata nel programma GFC:
// The hashing is done by setting up a table of pseudo-random numbers
// The value of each USHORT is multiplied by the next random number
// in the sequence, and added to the hash value.
// If the key-lenght is <= 3 the value of each BYTE is multiplied by the 
// next random number in the sequence, and added to the hash value.
// ---------------------------------------------------------------------
DWORD MMB_FASTCALL BASE_HASH::Hash(void * Chiave, BYTE KLen) const{
   
   #undef TRCRTN
   #define TRCRTN "BASE_HASH::Hash"
   
   assert(Chiave != NULL && 0 < KLen);
   
   #ifdef DBG3
   TRACEHEX("Chiave: ",Chiave,KLen);
   #endif
   
   DWORD  * Rand  = Random;
   DWORD    Out   = 0;
   if (KLen <= 3) {
      BYTE * Wrk   = (BYTE*)Chiave;
      BYTE * Stop  = Wrk + KLen ; 
      while (Wrk  < Stop) {
         Out += (*Rand) * (*Wrk); // Si potrebbe anche fare un XOR
         Rand ++;
         Wrk  ++;
      } /* endwhile */
   } else {
      // Se la chiave ha un numero dispari di bytes ignoro l' ultimo byte:
      // In pratica e' raro, e nel motore non mi serve. Cosi' velocizzo
      USHORT * Wrk   = (USHORT*)Chiave;
      USHORT * Stop  = (USHORT*)( (BYTE*)Chiave + KLen -1 ); // Il -1 gestisce l' extra byte
      while (Wrk  < Stop) {
         Out += (*Rand) * (*Wrk); // Si potrebbe anche fare un XOR
         Rand ++;
         Wrk  ++;
      } /* endwhile */
   } /* endif */

   Out &= HashMask; // NB: sarebbe meglio usare i MSB, ma per i nostri scopi e' OK  usare i LSB
   #ifdef DBG3
   TRACELONG("Valore della hash function:",Out);
   #endif
   return Out;
//<<< DWORD MMB_FASTCALL BASE_HASH::Hash void * Chiave, BYTE KLen  const 
};
#pragma option -Od   // Su questo modulo l' ottimizzatore crea problemi
