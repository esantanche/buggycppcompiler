//========================================================================
// MM_BASIC : Classi che realizzano alcuni algoritmi basilari
//========================================================================
//
// SET       : Classe utilizzata per la gestione di insiemi di dimensione arbitraria
//    Occupa:  (MaxElement / 8) bytes
//
// LISTA     : Micro implementazione di liste semplici Endogene
//    Occupa: 8 Bytes per lista, 4 Bytes addizionali per elemento inserito
//
// ALLOCATOR : Allocatore ad alte performance per aree a dimensione fissa
//    Occupa: Alloca blocchi di dimensione fissa, occupa bytes extra per
//            frammentazione e perche mantiene due blocchi sempre allocati.
//
// ALLOCATOR2: Allocatore ad alte performance per aree a dimensione variabili
//             che, una volta allocate, non debbono piu' essere liberate fino alla
//             fine del programma. Se le aree sono > 1024 bytes chiama la malloc
//    Occupa:  Praticamente non occupa memoria extra
//
// HASH      : Hash semplificata. Elementi di dimensione fissa e chiave di max 256 bytes
//    Occupa:  4 bytes per cella di Hash + 5 bytes EXTRA per ogni elemento inserito +
//             occupazione addizionale di allocator.
//
// HEAP      : Heap - ordered   D-ary tree: Esogeno   4 Bytes ad elemento.
//
// COLLIDER  : Serve a rilevare interi duplicati
//
// CACHE     : Semplice cache in memoria realizzata combinando le funzioni precedenti.
//             Gestisce direttamente l' allocazione degli elementi
//             Permette di gestire un Lock degli elementi
//             Il numero degli elementi deve essere una potenza di due
//             Utilizza una logica LRU
//
// ARRAY_DINAMICA
//           : Array dinamiche
//
// ITERA     : Macro di iterazione per HASH
// ITERA2    : Macro di iterazione per LISTA
// Per le HEAP, le  ARRAY_DINAMICA e le CACHE si puo' utilizzare un ORD_FORALL:
//   l' output non e' ordinato tranne che per le ARRAY_DINAMICA
//
// Le classi privilegiano le performance. HASH, HEAP, CACHE ed ALLOCATOR possono
// deallocare in un sol colpo tutte le strutture utilizzate.
//
//========================================================================
// Funzioncine utili
//========================================================================
// NumeroBits : Conta il numero dei bits settati in un intero
//========================================================================
// Opzioni di gestione
#define MM_BASIC_EXPORT   // export     // puo' esportare o meno le funzioni

// EMS001 Win elimino MMB_FASTCALL
#define MMB_FASTCALL          // Parametri nei registri: incompatibile con il precedente
//#define MMB_FASTCALL      __fastcall    // Parametri nei registri: incompatibile con il precedente

//========================================================================
//
#ifndef HO_MM_BASIC_HPP
#define HO_MM_BASIC_HPP

extern BYTE NumBits[256];
inline BYTE NumeroBits(BYTE   Byte){ return NumBits[Byte];};
inline BYTE NumeroBits(WORD   Word){ return NumBits[Word & 0xff] + NumBits[Word >> 8];};
inline BYTE NumeroBits(DWORD DWord){ return NumBits[DWord & 0xff] + NumBits[(DWord >> 8) & 0xff] + NumBits[(DWord >> 16) & 0xff]+ NumBits[DWord >> 24]; };

// ----------------------------------------------------------------------------------------
// Iterazione su HASH TABLE o altre classi che definiscono gli operatori First e Next
// ----------------------------------------------------------------------------------------
// NB: E' possibile utilizzare all' interno di ITERA un Cerca()
// Gli iteratori sono deallocati alla fine del blocco ...
// In ITERA e' possibile cancellare l' elemento corrente con l' apposito
// metodo (MA: non e' definito il risultato aggiungendone di nuovi).
// In ITERA2 si deve utilizzare del codice ad Hoc (rammento che si presume
// che l' allocazione / deallocazione sia esterna).
// ----------------------------------------------------------------------------------------
// Per HASH
class ITERATORE;
#define ITERA(_Collection,_Var,_TipoElemento)                 \
   ITERATORE Iter##_Var;                                      \
   for(_TipoElemento * _P_##_Var = _Collection.First(Iter##_Var);\
       _P_##_Var;                                             \
       _P_##_Var = _Collection.Next(Iter##_Var)){           \
       _TipoElemento & _Var = *_P_##_Var;

// Questo cancella l' elemento correntemente sotto iterazione
// Deve essere chiamato come un metodo.
#define DeleteITERATO(_Var) DeleteCurrentlyIterated(Iter##_Var)

// Per LISTA
//#define ITERA2(_Collection,_Var,_TipoElemento)          \
// for(_TipoElemento * _P_##_Var = _Collection.First(); \
//     _P_##_Var ;                                      \
//     _P_##_Var  = _Collection.Next(_P_##_Var  )) {    \
//    _TipoElemento & _Var = *_P_##_Var;
#define ITERA2(_Collection,_Var,_TipoElemento)          \
   for(_TipoElemento * _Var = _Collection.First(); \
       _Var ;                                      \
       _Var = _Collection.Next(_Var)) {

#define END_ITERA }

//----------------------------------------------------------------------------
// SET: Classe utilizzata per la gestione di insiemi di dimensione arbitraria
//----------------------------------------------------------------------------
// Max 64K elementi
class MM_BASIC_EXPORT SET {

   public:

   SET(DWORD CardinalitaPrevista=0); // Il parametro opzionale velocizza dando un' allocazione iniziale
   ~SET();

   // Nota:         >>5 == /32          & 0x1f == %32   (per velocizzare)
   void Set(DWORD BitNum)    { Realloc(BitNum); Dati[BitNum>>5] |= 1 << (BitNum & 0x1f); };
   void ReSet(DWORD BitNum)  { Realloc(BitNum); Dati[BitNum>>5] &= ~(1 << (BitNum & 0x1f )); };
   BOOL Test(DWORD BitNum)   { if(BitNum >= BitsAllocati)return FALSE; return (BOOL)(Dati[BitNum>>5] & 1 << (BitNum & 0x1f)); };

   BOOL   MMB_FASTCALL Empty()                 ; // True se vuoto
   SET &  MMB_FASTCALL operator  = (SET & From); // Copia
   SET &  MMB_FASTCALL operator |= (SET & Dato); // Unione
   SET &  MMB_FASTCALL operator &= (SET & Dato); // Intersezione
   BOOL   MMB_FASTCALL operator == (SET & Dato);
   BOOL   MMB_FASTCALL operator != (SET & Dato){return ! (THIS == Dato);};
   BOOL   MMB_FASTCALL operator >= (SET & Dato); // Inclusione
   BOOL   MMB_FASTCALL operator &  (SET & Dato); // Bit AND tra set TRUE se un bit e' set
   void   MMB_FASTCALL Reset()                 ; // Dealloca (fisicamente) tutto
   void   MMB_FASTCALL Clear()                 ; // Vuota l' insieme

   void   MMB_FASTCALL ToFix(void * To, ULONG NumBits);    // Appoggio su area di dimensione fissa
   void   MMB_FASTCALL FromFix(void * From,ULONG NumBits); // Viceversa

   STRINGA MMB_FASTCALL ToHex();  // Stringa esadecimale
   STRINGA MMB_FASTCALL ToBit();  // Stringa Binaria (' '/'*')
   STRINGA MMB_FASTCALL ToInt();  // Stringa di interi corrispondenti ai bit settati


   void Realloc(DWORD MaxBit){
      if(MaxBit >= BitsAllocati){
         DWORD OldAlloc = BitsAllocati;
         BitsAllocati = (MaxBit + 32) & 0xffe0;
         if(BitsAllocati > OldAlloc){
            Dati = (DWORD*) realloc(Dati,BitsAllocati / 8);
            // memset(&Dati[OldAlloc / 32],0,(BitsAllocati - OldAlloc)/8);
            memset(((BYTE*)Dati)+(OldAlloc >>3),0,(BitsAllocati - OldAlloc)>>3);
         };
      };
   };

   DWORD MMB_FASTCALL Cardinalita(); // Ritorna il numero di elementi impostati ad 1;

   DWORD NumBits(){return BitsAllocati;};

   protected:

   DWORD BitsAllocati;
   DWORD * Dati;  // Alloco in unita' di 32 bits
};

//----------------------------------------------------------------------------
// Log2 : Logaritmo in base due di un numero positivo
//----------------------------------------------------------------------------
inline int Log2(DWORD i){ int Lg = 0; while((1 << Lg) < i)Lg ++; return Lg; };


// ---------------------------------------------------------------------
// Micro Lista / stack : Solo 8 bytes e tutto il codice in linea
// Versione Endogena. Allocazione dei nodi esterna
// Puo' funzionare come LIFO (usare Push) o FIFO (Usare Put)
// ---------------------------------------------------------------------
// Push     : Aggiunge un nodo allo stack
// PushAfter: Aggiunge un nodo dopo un nodo dato
// Put      : Aggiunge un nodo alla fine della lista
// Pop      : Toglie un nodo dallo stack (il primo)
// Del      : Toglie un nodo con indirizzo dato dallo stack (Torna prossimo elemento)
// Vuota    : TRUE se vuota
// First    : Mi ritorna il primo NODO dello stack
// Next     : Mi ritorna il NODO successivo a nodo dato
// Prev     : Mi ritorna il NODO precedente a nodo dato (Lenta!)
// Clear    : La vuota
// ---------------------------------------------------------------------
// La classe deve avere un elemento di nome Next che punta
// all' elemento successivo. Non deve necessariamente essere in testa.
// L' iterazione si puo' fare con ITERA2
// Togliendo un elemento dalla lista all' interno di un ciclo ITERA2
// si puo' continuare l' iterazione, PURCHE' L' ELEMENTO NON SIA
// ANCHE DEALLOCATO FISICAMENTE (o spostato su altre liste)
// NB: Oppure sostituire il puntatore al prossimo elemento che e' tornato da Del();
// Si veda ad esempio cosa fa la HASH .
// ---------------------------------------------------------------------
template <class NODO>
class MM_BASIC_EXPORT LISTA {
   public:

   NODO* Root ;
   NODO* Tail ;

   void Push(NODO* Nodo){
      if(Root == NULL)Tail = Nodo;
      Nodo->Next = Root;
      Root = Nodo;
   };

   void PushAfter(NODO* Nodo, NODO * After){
      if(Tail == After) Tail = Nodo;
      Nodo->Next = After->Next;
      After->Next = Nodo;
   };
   void Put(NODO* Nodo){
      Nodo->Next = NULL;
      if(Tail)Tail->Next = Nodo;
      else Root = Nodo;
      Tail = Nodo;
   };

   NODO * Pop(){
      NODO * Tmp = Root;
      Root = Root->Next;
      if(Root == NULL)Tail = NULL;
      return Tmp;
   };

   NODO * First(){return Root;};

   NODO * Next(NODO* Nodo){return Nodo->Next;};

   NODO * Prev(NODO* Nodo){
      if(Root == Nodo)return NULL;
      NODO * Tmp = Root;
      while( Tmp && Tmp->Next != Nodo) Tmp = Tmp->Next ;
      return Tmp;
   };

   NODO * MMB_FASTCALL Del(NODO* Nodo){    // Vedi nota sopra per l' utilizzo nei LOOPS
      if(Root == Nodo){
         Root = Nodo->Next;
         if (Nodo == Tail) Tail = NULL;
         return NULL;
      } else {
         NODO * Tmp = Root;
         while( Tmp && Tmp->Next != Nodo) Tmp = Tmp->Next ;
         if (Tmp) {
            if (Nodo == Tail) Tail = Tmp;
            Tmp->Next = Nodo->Next;
            return Tmp->Next ;
         } else {
            ERRSTRING("Errore: Elemento non appartenente alla linked list!");
            BEEP;
            return NULL;  // EMS002 MVC la funzione deve ritornare un NODO *
         } /* endif */
      }
   };

   BOOL Vuota(){return Root == NULL;};

   LISTA(){Root = Tail = NULL;};
   // EMS003 MVC aggiungo ritorno void per metodo Clear
   void Clear(){Root = Tail = NULL;};

//<<< class LISTA {
};

// ---------------------------------------------------------------------
// ALLOCATOR: Alloca / Dealloca rapidamente blocchi dimensione fissa
// ---------------------------------------------------------------------
// ATTENZIONE: E' monothread!
// -> I blocchi debbono essere allocati tutti dallo stesso thread
// -> Anche le deallocazioni debbono avvenire sullo stesso thread
// -----------------------------------------------------------------------
// Costruttore: si deve passare la dimensione del blocco interno in unita'
// di allocazione. Max 0xffff
// ALLOCATOR Alloca memoria in unita' di Blocchi.
// Se in un dato momento vi sono piu' di due blocchi LIBERI uno viene deallocato.
// E' prevista anche una funzione di deallocazione globale dell' area allocata:
// utile per strutture da pulire in modo globale.
// La dimensione della classe deve essere di almeno 4 BYTES per permettere
// di porlo nella linked list: altrimenti viene incrementata automaticamente.
// ATTENZIONE: Le doppie deallocazioni possono creare loops, abend ecc...
// -----------------------------------------------------------------------
class  MM_BASIC_EXPORT BASE_ALLOCATOR {

   public:
   void * MMB_FASTCALL Get();
   void   MMB_FASTCALL Free(void* Dati);  // Se Dati non e' un' area allocata da ALLOCATOR non fa nulla
   void   Reset();               // Dealloca (fisicamente) tutto
   void   Clear();               // Dealloca logicamente tutto, ma lascia uno o due blocchi vuoti allocati
   DWORD  AreaAllocata(){        // Area totale allocata (via malloc)
      return DimBlocco * NumBlocchiTotali;
   };
   DWORD  ElementiInUso(){       // Numero complessivo di ELEMENTI in uso
      DWORD Tot=0; BLOCCO * Wrk = ListaBlocchi.First();
      while(Wrk){Tot += Wrk->NumUsed;Wrk = Wrk->Next;};
      return Tot;
   };
   DWORD  ElementiNonInUso(){    // Numero complessivo di ELEMENTI allocati ma non in uso
      return NumElementiBlocco * NumBlocchiTotali - ElementiInUso() ;
   };

   void  Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   BASE_ALLOCATOR(USHORT DimElemento, USHORT NumElementiPerBlocco);
   ~BASE_ALLOCATOR(){Reset();};

   // Queste variabili ai soli fini statistici
   ULONG TotGet;  // Numero totale di chiamate alla Get
   ULONG TotFree; // Numero totale di chiamate alla Free

   protected:


   struct BLOCCO {

      BLOCCO  *   Next       ;
      void    *   FreeList   ;
      void    *   Dati       ;
      void    *   MaxDati    ;
      BYTE    *   NextDati   ; // Per gestire in modo efficace le operazioni di Init (= Clear della base class)
      USHORT      NumUsed    ;
      USHORT NumElementiBlocco;

      // Queste per comodita
      void Init(){NumUsed = 0; FreeList = NULL;NextDati = (BYTE*)Dati;};
      BOOL Pieno(){return NumUsed >= NumElementiBlocco;};
   };

   struct L_BLOCCHI: public LISTA<BLOCCO> {
      // Cerca il blocco con il minor numero di elementi in uso e lo mette in testa alla lista
      // Torna NULL se la lista e' vuota o se la Root e' gia' il blocco con il minor
      // numero di elementi.
      BLOCCO * MMB_FASTCALL MinBlocco();
   };

   BLOCCO * MMB_FASTCALL AllocaBLOCCO();

   USHORT    Size;
   USHORT    NumElementiBlocco;
   ULONG     DimBlocco;
   int       NumBlocchiVuoti;  // Quando supera 2 dealloco
   int       NumBlocchiTotali;
   L_BLOCCHI ListaBlocchi;


//<<< class  BASE_ALLOCATOR {
};

template <class ELEMENT>
class  ALLOCATOR : protected BASE_ALLOCATOR {

   public:

   ALLOCATOR(USHORT NumElementiPerBlocco )               // Costruttore
   :BASE_ALLOCATOR(sizeof(ELEMENT),NumElementiPerBlocco){};
   ELEMENT* Get(){return (ELEMENT*)BASE_ALLOCATOR::Get();};          // Ottiene un elemento
   void Free(ELEMENT* Dati){BASE_ALLOCATOR::Free(Dati);};      // Libera un elemento

   BASE_ALLOCATOR::Reset;         // Dealloca (fisicamente) tutto
   BASE_ALLOCATOR::Clear;         // Dealloca logicamente tutto, ma lascia uno o due blocchi vuoti allocati
   BASE_ALLOCATOR::AreaAllocata;  // Area totale allocata (via malloc)
   BASE_ALLOCATOR::ElementiInUso; // Numero complessivo di ELEMENTI in uso
   BASE_ALLOCATOR::Trace        ; // Esegue un trace

};

// ---------------------------------------------------------------------
// ALLOCATOR2: Allocatore ad alte performance per aree a dimensione variabili
// ---------------------------------------------------------------------
class  ALLOCATOR2 {
   public:

   ALLOCATOR2(DWORD InternalBlockSize = 64000){ BlkSize =  InternalBlockSize; BytesLeft = 0; Current = NULL; };

   void * Get(DWORD Size ){
      if(Size == 0  )return NULL;
      if(Size > 1024)return malloc(Size);
      if(BytesLeft < Size){ Current = (BYTE*)malloc(BlkSize); BytesLeft = BlkSize;};
      void * Out = Current;
      Current += Size;
      BytesLeft -= Size;
      return Out;
   };

   protected:
   DWORD BlkSize  ;
   DWORD BytesLeft;
   BYTE  * Current;
};

// ---------------------------------------------------------------------
// HASH
// ---------------------------------------------------------------------
// Hash semplificata:
//    - Matrice a dimensione fissa, deve essere una potenza di 2:  Se specificata
//      piu' piccola viene portata a potenza di 2
//    - Dati a dimensione fissa, chiave a dimensione variabile
//    - Gestisce direttamente i dati, che alloca e dealloca internamente: no copia
//    - Max lunghezza chiave : 256 Bytes
//    - Chiave sempre all' inizio del record
//    - MonoThread !!!
//    - Permette duplicati
//    - Solo funzioni base.
//    - Fast
//    - Iterazione interna (multiple). Durante l' interazione si possono cancellare ma non aggiungere elementi.
//    - L' allocazione degli elementi e' fatta dalla HASH: essi pertanto NON debbono avere un costruttore
// Area occupata:
//    - 4 bytes per cella della hash table.
//    - 5 bytes extra per ogni elemento allocato.
//      Alla creazione della hash Table viene allocato un blocco
//      comunque sufficiente a contenere NumDatiAllocIniziali elementi
//      Si consiglia di scegliere questo numero a circa 1/2 ~ 1/3 degli
//      elementi che si stimano presenti sulla hash table.
//    - La massima area che rimane allocata dopo una clear e' il doppio dell' area iniziale.
//
// Esempio: Tabella da 1024 celle, 250 bytes cella, si stimano 800 elementi
//    mediamente presenti.
//
//    Si fissa NumDatiAllocIniziali a 250 ->
//    Celle occupano 4K
//    Allocazione iniziale dati elementi: 250 * 255 ~= 64K
//
// L' iterazione si puo' fare con ITERA
// ---------------------------------------------------------------------
class MM_BASIC_EXPORT BASE_HASH {

   public:


   void   Reset(){       // Dealloca (fisicamente) tutto (NB: Non le celle)
     Clear();            // Pulisce le celle e le variabili interne
     Allocator.Reset();
   };

   BASE_HASH(BYTE Log2DimMatrice, USHORT DimElemento,USHORT NumDatiAllocIniziali);

   ~BASE_HASH();

   // EMS004 VA Necessario un const altrimenti HASH::Cerca non viene compilata
   const void * MMB_FASTCALL Cerca(void* Key,BYTE KLen) { // NULL se non e' gia' presente
      NumCellaCorrente = Hash(Key,KLen);
      //TRACEINT("NumCellaCorrente = ",NumCellaCorrente);
      //TRACEINT("Key = ", *((ID *)Key));
      //TRACEINT("KLen = ", (int) KLen);
      ElementoCorrente = Celle[NumCellaCorrente].Seek(Key,KLen);
      CurrentDeleted   = FALSE;
      return ElementoCorrente ? ElementoCorrente->Dati() : NULL ;
   };

   void   MMB_FASTCALL Delete(void * Key,BYTE KLen){    // Cancella un elemento data la chiave
      //TRACESTRING("in Celle[Hash(Key,KLen)].Del(Key,KLen);");
      // EMS005 Win espando la chiamata
      DWORD dwRisultato_hash = Hash(Key,KLen);
      Celle[dwRisultato_hash].Del(Key,KLen);
      //TRACESTRING("out Celle[Hash(Key,KLen)].Del(Key,KLen);");
   };

   void * MMB_FASTCALL Alloca(){                        // Alloca e ritorna un elemento : non devono essere fatte
      if(!NextElemento){                                // altre allocazioni prima che l' elemento sia utilizzato
          NextElemento = (ESTENSIONE* )Allocator.Get(); // altrimenti si considera abortita la prima richiesta
      }
      return NextElemento->Dati();
   };

   void   MMB_FASTCALL Metti (BYTE KLen){               // Mette nella hash table l' elemento allocato
      NextElemento->KeyLen = KLen;
      // EMS005 Win begin
      // Ho dovuto espandere l'istruzione precedentemente contenuta
      // in una singola riga in più righe
      void * Puntatore_dati = NextElemento->Dati();
      DWORD dwRisultato_hash = Hash(Puntatore_dati,KLen);
      Celle[dwRisultato_hash].Push(NextElemento);
      // EMS005 Win end
      NextElemento = NULL;
   };

   void   MMB_FASTCALL Clear ();            // Dealloca logicamente, mantiene le allocazioni fisiche

   void   Istogram(const STRINGA& Msg)const;// Fa un istogramma della Hash-Table sul trace

   DWORD  AreaAllocata(){                   // Area totale allocata (via malloc)
      return Allocator.AreaAllocata() + NumCelle * sizeof(CELLA);
   };

   // Funzioni per iterare. E' possibile solo una iterazione per volta.
   // Si puo' usare Cerca() all' interno della iterazione
   void * MMB_FASTCALL First(ITERATORE & i)   ; // Resetta l' iterazione e ritorna il primo elemento
   void * MMB_FASTCALL Next(ITERATORE & i)    ; // Si deve chiamare prima First()
   void * Current(){ // Ultimo elemento trovato con Cerca() o CercaNext()
      return ElementoCorrente;
   };
   void DeleteCurrent(){ // Cancella l' elemento corrente dalla HASH
      if(ElementoCorrente && ! CurrentDeleted){
         //TRACESTRING("in  IONE>::Del(ElementoCorrente); 494");
         void * Ne = Celle[NumCellaCorrente].LISTA<ESTENSIONE>::Del(ElementoCorrente);
         //TRACESTRING("out IONE>::Del(ElementoCorrente); 494");
         Allocator.Free(ElementoCorrente);
         ElementoCorrente = (ESTENSIONE*)Ne;
         CurrentDeleted   = TRUE;
      };
   };

   // Questo cancella un elemento dalla HASH e lo dealloca:
   // bisogna dare l' esatto puntatore dell' elemento
   void MMB_FASTCALL DeleteElementFromHash (void * Element){
      ESTENSIONE * Est = ((ESTENSIONE*)Element)-1;
      // begin EMS005 Win espando anche questa chiamata
      DWORD dwRisultato_hash = Hash(Element,Est->KeyLen);
      void * Ne =Celle[dwRisultato_hash].LISTA<ESTENSIONE>::Del(Est);
      // end EMS005
      if(Est == ElementoCorrente){
         ElementoCorrente = (ESTENSIONE*)Ne;
         CurrentDeleted   = TRUE;
      }
      Allocator.Free(Est);
   };

   void * MMB_FASTCALL CercaNext();  // Si deve chiamare prima  Cerca()
   // CercaNext opera cercando, nella lista puntata dalla cella corrente, i prossimi elementi
   // che hanno la stessa chiave dell' elemento corrente

   void DeleteCurrentlyIterated(ITERATORE &i); // Cancella l' elemento corrente (nella iterazione) dalla HASH

   protected :

   struct ESTENSIONE {
      ESTENSIONE * Next    ; // Punta a prossima estensione o NULL
      BYTE         KeyLen  ; // Lunghezza chiave elemento
      void       * Dati(){return this+1;}  ; // Dati
   };


   struct CELLA : public LISTA<ESTENSIONE> {  // Richiede solo 4 bytes

      void MMB_FASTCALL Del(void * Key,BYTE KLen){
         ESTENSIONE * Wrk = Seek(Key,KLen);
         //TRACESTRING("in  SIONE>::Del(Wrk); 537");
         if(Wrk){ LISTA<ESTENSIONE>::Del(Wrk); }
         //TRACESTRING("out SIONE>::Del(Wrk); 537");
      };

      ESTENSIONE * MMB_FASTCALL Seek(void * Key,BYTE KLen) { // NULL se non e' gia' presente

         // begin EMS006 Win

         /*

         prima questa funzione era così:

         ITERA2(THIS,Ext,ESTENSIONE){
            if(Ext->KeyLen == KLen && !memcmp(Ext->Dati(),Key,KLen))return Ext;
         } END_ITERA
         return NULL;

         e la macro ITERA2 era definita come segue:

         #define ITERA2(_Collection,_Var,_TipoElemento)          \
         for(_TipoElemento * _Var = _Collection.First(); \
             _Var ;                                      \
             _Var = _Collection.Next(_Var)) {

         */

         #undef TRCRTN
         #define TRCRTN "BASE_HASH::CELLA::Seek"

         ESTENSIONE * Ext;
         Ext = (ESTENSIONE *) NULL;
         for (Ext = First(); Ext; Ext = Next(Ext))
            if (Ext->KeyLen == KLen && !memcmp(Ext->Dati(),Key,KLen)) break;

         return Ext;

         // end EMS006

      };
   };

   // Funzione di Hashing
   DWORD MMB_FASTCALL Hash(void * Chiave, BYTE LunghezzaChiave)const;

   ESTENSIONE * NextElemento;
   DWORD        HashMask;
   CELLA      * Celle;
   DWORD        NumCelle;
   BASE_ALLOCATOR Allocator;

   static DWORD Random[32]; // 32 WORDS pseudorandom

   // Dati per ricerca sequenziale (a prita' di chiave).
   DWORD         NumCellaCorrente;
   ESTENSIONE *  ElementoCorrente;
   BYTE          CurrentDeleted  ;

   friend void MM_BASIC_TEST(); // Funzioncina di test
   friend class ITERATORE;

//<<< class BASE_HASH {
};
class ITERATORE {
   DWORD                    ItNumCellaCorrente;
   BASE_HASH::ESTENSIONE *  ItElementoCorrente;
   BYTE                     ItCurrentDeleted  ;
   friend class BASE_HASH;
};

template <class ELEMENT>
class HASH : public BASE_HASH {

   public:

   HASH(int NumCelle, int NumElementiAllocatiInizialmente = 0):
   BASE_HASH(Log2(NumCelle), sizeof(ELEMENT), (NumElementiAllocatiInizialmente ?  NumElementiAllocatiInizialmente : NumCelle / 3 ) ){};

   // EMS007 VA eliminata keyword const
   ELEMENT * Cerca(ELEMENT* Key,BYTE KLen) { return (ELEMENT*)BASE_HASH::Cerca(Key,KLen); };
   ELEMENT* Alloca(){return (ELEMENT*)BASE_HASH::Alloca();};
   // void   Metti (BYTE KLen);       // Mette nella hash table l' elemento allocato: Definito nella classe base

   // Queste per iterare
   ELEMENT * First(ITERATORE &i){ return (ELEMENT*)BASE_HASH::First(i)   ;};
   ELEMENT * Next(ITERATORE &i) { return (ELEMENT*)BASE_HASH::Next(i)    ;};
   ELEMENT * Current()          { return (ELEMENT*)BASE_HASH::Current()  ;};
   ELEMENT * CercaNext()        { return (ELEMENT*)BASE_HASH::CercaNext();};

   // Questa definita nella superclasse : DeleteCurrentlyIterated

   void  Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA){ Allocator.Trace(Msg,Livello);};

   DWORD NumeroElementiInHash(){ return Allocator.ElementiInUso();};

};

// ---------------------------------------------------------------------
// HEAP (esogena)
// ---------------------------------------------------------------------
// Realizzata tramite albero D-ario: Si veda : R. E. Tarjan
// Data structures and Network Algorithms
// Si autorealloca se necessario
// Alcune funzioni comportano la necessita' di gestire un puntatore
// dagli elementi alla posizione che l' elemento occupa nell' HEAP:
// In tal caso l' elemento si deve chiamare HeapPos ed e' gestito
// direttamente dall' HEAP, e si deve utilizzare il TEMPLATE HEAP2
// ---------------------------------------------------------------------
// Metodi :
//  - HEAP         : Costruttore lista vuota
//  - Insert       : Inserimento elemento
//  - FindMin      : Ricerca elemento con chiave minima
//  - DeleteMin    : Ricerca e rimuove dall' HEAP elemento con chiave minima
//  - MinChangeKey : Riorganizza l' Heap dopo che e' stata cambiata l' elemento minimo.
//  - Clear        : Pulisce l' HEAP senza deallocare fisicamente
//  - Reset        : Pulisce l' HEAP deallocando fisicamente
// in HEAP2
//  - Delete       : Ricerca e rimuove dall' HEAP un elemento arbitrario
//  - IncrementKey : Riorganizza l' Heap dopo che e' stata aumentata la chiave di un elemento
//  - DecrementKey : Riorganizza l' Heap dopo che e' stata diminuita la chiave di un elemento
// Per iterare : Dim e Operator []
// ---------------------------------------------------------------------
// Necessario fornire per la caratterizzazione della classe:
//    - Il tipo degli elementi esterni
//    - L' ordine dell' albero : Di default e' 3 (da' le migliore performance)
// Nella sottoclasse:
//    - una funzione di confronto tra elementi (solo chiave)
// Nel costruttore:
//    - Il numero approssimato degli elementi da trattare
// ---------------------------------------------------------------------
template <class ITEM , int D=3 >
class  HEAP {

   public:

   // Da ridefinire !!!!!!!!
   // EMS008 Win tolgo inline
   static int Confronta(ITEM* a,ITEM*b);

   ULONG Dim(){return Sz;};
   ITEM & operator [](int NumElemento){return * Nd[NumElemento];};

   HEAP(int NumElAll){             // Crea la Heap Vuota
      NumElementiAllocazione = (NumElAll+1)/2;
      Sz = 0;
      NumElementiAllocati = 2*NumElementiAllocazione;
      Nd = (ITEM**)malloc(NumElementiAllocati * sizeof(ITEM*));
      // EMS009 Win memset di azzeramento dopo malloc
      memset((char *)(Nd), 0L, (NumElementiAllocati * sizeof(ITEM*)));
   };

   DWORD  AreaAllocata() {                  // Area totale allocata (via malloc)
      return (NumElementiAllocati * sizeof(ITEM *));
   };

   ITEM * FindMin(){               // Ritorna l' elemento minimo
      return (Sz ? Nd[0] : NULL);
   };
   ITEM * FindLast(){               // Ritorna l' ultimo elemento aggiunto
      return (Sz ? Nd[Sz-1] : NULL);
   };
   void Insert(ITEM* i){           // Inserisce un elemento
      if(Sz == NumElementiAllocati){
         NumElementiAllocati += NumElementiAllocazione;
         Nd = (ITEM**)realloc(Nd,NumElementiAllocati* sizeof(ITEM*));
         // EMS010 Win memset di azzeramento dopo realloc ma solo della parte aggiunta
         memset(Nd + Sz, 0L, NumElementiAllocazione* sizeof(ITEM*));
      };
      SiftUp(i,Sz);
      Sz ++; // Aggiorno il conteggio del numero di elementi
   };
   ITEM * DeleteMin(){             // Cancella l' elemento minimo
      if(!Sz)return NULL;
      ITEM * i = Nd[0];
      Sz --;
      if(Sz){
         ITEM * j = Nd[Sz];
         SiftDown(j,0);
      } /* endif */
      return i;
   };
   void MinChangeKey(){            // Cambia la chiave dell' elemento minimo
      if(!Sz)return ;
      ITEM * i = Nd[0];
      SiftDown(i,0);
   };
   void Clear(){                   // Resetta la struttura (senza cambiare l' allocazione fisica)
      Sz = 0;
      NumElementiAllocati = 2*NumElementiAllocazione;
      Nd = (ITEM**)realloc(Nd,NumElementiAllocati* sizeof(ITEM*));
      // EMS011 Win memset di azzeramento in funzione Clear
      memset(Nd, 0L, NumElementiAllocati* sizeof(ITEM*));
  };
   void Reset(){                   // Resetta la struttura (eliminando l' allocazione fisica)
      Sz = 0;
      NumElementiAllocati = 0;
      if(Nd)free(Nd);
      Nd = NULL;
  };

   protected:

   ITEM ** Nd;
   int  NumElementiAllocazione;
   int  NumElementiAllocati;
   int  Sz;    // Numero elementi attuali

   int Parent(int x){             // Indice dell' elemento padre dell' elemento con indice x
      return (x ? (x-1)/D : -1);
   };

    void virtual SiftUp(ITEM * i, int x){  // Vedere Tarjan
      int p;
      p = Parent(x);
      while (p >= 0 && Confronta(Nd[p], i) > 0) {
         Nd[x] = Nd[p];
         x     = p;
         p = Parent(x);
      } /* endwhile */
      Nd[x] = i;
   };
    void virtual SiftDown(ITEM * i, int x){ // Vedere Tarjan
      int c;
      c = minchild(x);
      while(c >= 0 && Confronta(Nd[c], i) < 0){
         Nd[x] = Nd[c];
         x     = c    ;
         c = minchild(x);
      };
      Nd[x] = i;
   };
   int MMB_FASTCALL minchild(int x){    // Indice del figlio con chiave minima di un elemento con indice x (o -1 se non c' e')
      int C1 = D*x + 1;
      int C2 = C1 + D - 1;           // int C2 = D*(x + 1);
      if(C2 >= Sz)C2 = Sz - 1;
      int Min = C1;
      for (int j = C1+1; j <= C2 ; j++) {
         if(Confronta(Nd[Min],Nd[j]) > 0)Min = j;
      } /* endfor */
      if(Min > C2)Min = -1; // Si ha solo se C1 > C2
      return Min;
   };

   // EMS012 VA
   friend class CACHE<class ELEMENT>;

};


template <class ITEM , int D=3 >
class  HEAP2 : public HEAP<ITEM,D> {

   public:

   HEAP2(int NumElAll) : HEAP<ITEM,D>(NumElAll){};             // Crea la Heap Vuota

   void DecrementKey(ITEM * i){         // Diminuisce la chiave dell' elemento
      int Idx = i->HeapPos;
      SiftUp(i,Idx);
   };
   void IncrementKey(ITEM * i){         // Aumenta la chiave dell' elemento
      int Idx = i->HeapPos;
      SiftDown(i,Idx);
   };
   BOOL MMB_FASTCALL Delete(ITEM * i){  // Cancella un elemento arbitrario: FALSE se non trovato
      int Idx = i->HeapPos;
      if(Idx >= Sz)return FALSE; // Non trovato
      Sz --;
      if(Idx != Sz){ // Sono di certo eguali se Sz == 0
         ITEM * j = Nd[Sz];
         if (Confronta(j,i) <= 0) {
            SiftUp(j,Idx);
         } else {
            SiftDown(j,Idx);
         } /* endif */
      } /* endif */
      return TRUE;
   };
   protected:

   void SiftUp(ITEM * i, int x){  // Vedere Tarjan
      int p;
      p = Parent(x);
      while (p >= 0 && Confronta(Nd[p], i) > 0) {
         Nd[x] = Nd[p];
         Nd[x]->HeapPos = x;
         x     = p;
         p = Parent(x);
      } /* endwhile */
      Nd[x] = i;
      i->HeapPos = x;
   };
   void SiftDown(ITEM * i, int x){ // Vedere Tarjan
      int c;
      c = minchild(x);
      while(c >= 0 && Confronta(Nd[c], i) < 0){
         Nd[x] = Nd[c];
         Nd[x]->HeapPos = x;
         x     = c    ;
         c = minchild(x);
      };
      Nd[x] = i;
      i->HeapPos = x;
   };
};

// ---------------------------------------------------------------------
// ARRAY Dinamiche
// ---------------------------------------------------------------------
typedef int (*ARRAY_SORT_FUNC)( const void *a, const void *b);

template <class ELEMENT>
class  ARRAY_DINAMICA : protected BUFR {

   public:

   // Basic
   ARRAY_DINAMICA(int NumElementiIniziali=0): BUFR(NumElementiIniziali * sizeof(ELEMENT)){
      #ifdef HO_DELTASIZE
      DeltaSize = (NumElementiIniziali > 40 ? NumElementiIniziali / 10 : 4) * sizeof(ELEMENT) ;
      #endif
   };
   ARRAY_DINAMICA(ARRAY_DINAMICA<ELEMENT> & From):BUFR(From.Alloc){ // EMS013 VA
      THIS = From;
      #ifdef HO_DELTASIZE
      DeltaSize = From.DeltaSize;
      #endif
   };
   void ReDim(ULONG NumElementi){BUFR::ReDim(NumElementi* sizeof(ELEMENT));};
   void SetDimAndInitialize(ULONG NumElementi,BYTE Dato){BUFR::SetDimAndInitialize(NumElementi* sizeof(ELEMENT),Dato);};
   void SetDim(ULONG NumElementi){BUFR::SetDim(NumElementi* sizeof(ELEMENT));};
   void Store(const ELEMENT &Elemento){BUFR::Store(&Elemento,sizeof(ELEMENT));};
   // EMS013 VA
   ARRAY_DINAMICA<ELEMENT> & operator=  (const ARRAY_DINAMICA<ELEMENT> & From){return (ARRAY_DINAMICA<ELEMENT>&)BUFR::operator=(From);};
   BUFR::Clear     ; // Rendo accessibile
   #ifdef HO_DELTASIZE
   BUFR::DeltaSize ; // Rendo accessibile
   #endif

   // Operatori a tempo costante (salvo riallocazione )
   ELEMENT& operator[] (UINT Indice)const{
      #ifdef BOUND_CHECK
      if(Indice >= Dim())BoundCheckError(Indice,Dim());
      #endif
      return * ( ((ELEMENT*)this->Dati) +Indice);
   };
   ULONG Dim()const {return (this->Length / sizeof(ELEMENT));};
   void operator+= (const ELEMENT &Elemento){BUFR::Store(&Elemento, sizeof(ELEMENT));};
   static void Swap(ARRAY_DINAMICA<ELEMENT> & Prima,ARRAY_DINAMICA<ELEMENT> & Seconda){ BUFR::Swap(Prima,Seconda);}; // EMS013 VA
   ELEMENT& Last()const {
      #ifdef BOUND_CHECK
      if(!Length)BoundCheckError(0,0);
      #endif
      return *(ELEMENT*)(Dati + Length -sizeof(ELEMENT));
   };
   void Elimina(UINT Pos){  // Equivale a -=  Opera a tempo costante ma non preserva l' ordine
      UINT DIM = Dim();
      if(Pos >= DIM-1 ){
         if(Pos >= DIM ){
            #ifdef BOUND_CHECK
            BoundCheckError(Pos,DIM);
            #endif
            return;
         }
      } else {
         THIS[Pos] =  THIS[DIM -1];
      };
      Length -= sizeof(ELEMENT);
   };

   // Operatori a tempo lineare
   void operator+= (const ARRAY_DINAMICA<ELEMENT> & Elementi){BUFR::Store(Elementi);};  //EMS013 VA
   void operator-= (int Pos){  // Preserva l' ordine degli elementi
      UINT DIM = Dim();
      if(Pos<0 || Pos >= DIM ){
         #ifdef BOUND_CHECK
         BoundCheckError(Pos,DIM);
         #endif
         return;
      };
      if (DIM > (Pos + 1)) { // Test se e' l' ultimo elemento
         ELEMENT * x = (ELEMENT*)Dati + Pos;
         memmove(x,(x+1),(DIM-(Pos+1))* sizeof(ELEMENT) );
      } /* endif */
      Length -= sizeof(ELEMENT);
      if(Pointer > Pos)Pointer -= sizeof(ELEMENT);
   };
   void Insert(const ELEMENT & Elemento,UINT Pos){
      if(Pos >= Dim()){
         THIS += Elemento; // Aggiungo alla fine
      } else {
         if(Alloc < Length + sizeof(ELEMENT))ReDim(Alloc+256);
         ELEMENT * Base = (ELEMENT*)Dati;
         memmove((Base + Pos+1),(Base + Pos),Length - (sizeof(ELEMENT)*Pos)); // Scalo array
         THIS[Pos] = Elemento;
         Length += sizeof(ELEMENT);
      } /* endif */
   } ;

   void Sort(ARRAY_SORT_FUNC SortFunction){ qsort(Dati, Length / sizeof(ELEMENT) , sizeof(ELEMENT) , SortFunction); };

//<<< class  ARRAY_DINAMICA : protected BUFR {
};
// ---------------------------------------------------------------------
// COLLIDER
// ---------------------------------------------------------------------
// Serve a rilevare rapidamente se vi sia duplicazione di elementi
// In una serie di interi.
// Vuole come parametri:
// - Il tipo di intero (BYTE,WORD,DWORD, int ecc)
// - Il tipo di tavola di controllo (BYTE o WORD)
//     Se e' byte ogni 255 controlli deve essere fatto un reset di tutta la tavola
//     Se e' WORD ogni 65535 controlli deve essere fatto un reset di tutta la tavola
// Gli elementi da controllare debbono essere limitati in valore, comunque
// e' possibili modificare tale limite dinamicamente
// Equivale grossomodo ad un SET, ma l' operazione di reset e' molto piu' rapida
// e lo spazio occupato e' maggiore
// ---------------------------------------------------------------------
template <class TIPO_TAVOLA, class TIPO_INTERO>
class COLLIDER {
    public:
    COLLIDER(){ TavolaCollisioni = NULL;SizeTavolaCollisioni = 0; Prg = 0; Prg --;};
    ~COLLIDER(){ if(TavolaCollisioni && SizeTavolaCollisioni)free(TavolaCollisioni); };
    inline void Redim(int Size);
    inline void Reset();
    inline BOOL Test(TIPO_INTERO Target){ return TavolaCollisioni[Target] == Prg;}; // Vede se e' gia' settato
    inline void Set(TIPO_INTERO Target){ TavolaCollisioni[Target] = Prg;};
    inline BOOL TestAndSet(TIPO_INTERO Target);
    DWORD AreaAllocata(){return sizeof(TIPO_TAVOLA) * SizeTavolaCollisioni;};

    private:
    TIPO_TAVOLA * TavolaCollisioni;
    int SizeTavolaCollisioni;
    TIPO_TAVOLA Prg;

};
template <class TIPO_TAVOLA, class TIPO_INTERO>
inline void COLLIDER<TIPO_TAVOLA,TIPO_INTERO>::Redim(int Size){
   if(SizeTavolaCollisioni < Size){
      TavolaCollisioni = (TIPO_TAVOLA *)realloc(TavolaCollisioni,sizeof(TIPO_TAVOLA) * Size);
      SizeTavolaCollisioni =  Size;
      Prg = 0; Prg --; // Al prossimo reset faccio una inizializzazione
    }
};
template <class TIPO_TAVOLA, class TIPO_INTERO>
inline void COLLIDER<TIPO_TAVOLA,TIPO_INTERO>::Reset(){
   if(++Prg == 0){ // Attenzione: Se si mette il ++ su di un' altra linea il Borland va in tilt
      Prg ++;
      memset(TavolaCollisioni,0, sizeof(TIPO_TAVOLA) * SizeTavolaCollisioni);
   }
};
template <class TIPO_TAVOLA, class TIPO_INTERO>
inline BOOL COLLIDER<TIPO_TAVOLA,TIPO_INTERO>::TestAndSet(TIPO_INTERO Target){

    TIPO_TAVOLA & Cella = TavolaCollisioni[Target];
   if ( Cella == Prg) {
      return TRUE; // Ho avuto collisione
   } else {
      Cella = Prg;
      return FALSE;
   } /* endif */
};

// ---------------------------------------------------------------------
// CACHE
// ---------------------------------------------------------------------
// Realizza una semplice cache con logica LRU. Massimo numero elementi 0xffff
//
// La classe base deve soddisfare le condizioni necessarie per le HASH tables
//   In particolare la chiave deve stare in testa alla classe.
//   Inoltre deve essere a dimensione FISSA di KeyLen Bytes
//
// Se l' elemento e' allocato ex novo e' chiamata la funzione InitForCache()
// Se l' elemento e' deallocato e' chiamata la funzione RemoveFromCache()
//    Entrambe le funzioni debbono essere definite nella classe base
// InitForCache e' chiamata a record gia' inserito in Heap ed Hash: non
//    modificare la chiave. Se ritorna FALSE vuol dire che l' inizializzazione e'
//    fallita: in tal caso GetSlot torna NULL e l' elemento NON viene
//    inserito nella CACHE.
//
// E' possibile mettere in Lock degli elementi in modo che non possano
//   essere deallocati: E' gestito un Lock - Count
//   La CacheSize viene AUTOMATICAMENTE aumentata se viene fatta una richiesta per
//   uno slot e tutti gli slot disponibili sono Locked: la size verra'
//   diminuita di nuovo al prossimo Clear()
//
// Sono ammessi ORD_FORALL e Clear()
// ---------------------------------------------------------------------
// Ridefinendo il metodo virtuale Equivalente() e' possibile
// distinguere tra piu' elementi con la stessa chiave.
// Di default il metodo torna TRUE per cui a parita' di chiave gli elementi
// SONO equivalenti.
// Es: Risultati di precedenti elaborazione registrati per Data ed ora,
// con un criterio di equivalenza basato sul fatto che un risultato
// richiesto all' ora x puo' essere considerato valido anche per un certo
// tempo a seguire.
// ---------------------------------------------------------------------

enum LOCKREQ { LR_NO_LOCK=0, LR_LOCK=1 };
template <class ELEMENT>
class  CACHE {

   protected:
   struct CACHE_ELEMENT : public ELEMENT {
      DWORD     UsagePrg;
      WORD      HeapPos ;
      short int LockCount;
   };

   public:

   void   Reset(BOOL Remove = TRUE){  // Dealloca (fisicamente) tutto
     if(Remove)ORD_FORALL(Heap,i)Heap[i].RemoveFromCache(this); // Per attivare i metodi virtuali.
     Hash.Reset();
     Heap.Reset();
   };

   // Per la Hash genero un numero di celle pari a due volte il numero di elementi previsti
   // Per le allocazioni considero un margine di 16 possibili elementi in soprannumero
   CACHE(int NumElementiMax, BYTE KLen) : Hash( 2*NumElementiMax, NumElementiMax+16), Heap(NumElementiMax +16) {
      NextPrg = 0;CacheSize = InitialCacheSize = NumElementiMax; KeyLen = KLen;
   };

   // NB: Cerca non fa' un Refresh dell' LRU, Get si
   ELEMENT * Cerca( ELEMENT * Key){
      CACHE_ELEMENT * Res = Hash.Cerca((CACHE_ELEMENT*)Key,KeyLen);
      while (Res != NULL && !Equivalente(Key,Res) ) Res = Hash.CercaNext();
      return Res;
   };
   ELEMENT * MMB_FASTCALL Get( ELEMENT * Key){
      // Ripeto il codice di Cerca per velocizzare un poco
      CACHE_ELEMENT * Res = Hash.Cerca((CACHE_ELEMENT*)Key,KeyLen);
      while (Res != NULL && !Equivalente(Key,Res) ) Res = Hash.CercaNext();
      if (Res){
         Res->UsagePrg = NextPrg ++;
         Heap.IncrementKey(Res);
      };
      return Res;
   };

   // Questa funzione cerca il dato nella cache: se non lo trova crea uno slot (vuoto) e vi copia la
   // chiave (Il resto del record puo' essere volendo copiato dalla funzione InitForCache() )
   // NB: Se lo trova gia' o ne trova uno equivalente non copia nulla
   // Si puo' opzionalmente richiedere un lock sul dato (di default : Non mette il lock)
   ELEMENT * MMB_FASTCALL  GetSlot( ELEMENT * Key, BYTE Lr = LR_NO_LOCK){ // La chiave deve essere di KeyLen Bytes esatti
      #undef TRCRTN
      #define TRCRTN "CACHE::GetSlot"
      CACHE_ELEMENT * Res = (CACHE_ELEMENT*)Get(Key);
      if (Res == NULL){
         Res = Hash.Alloca();
         Res->LockCount = 0;
         Res->UsagePrg = NextPrg ++;
         memmove(Res,Key,KeyLen);          // Vi copio solo la chiave
         if(Heap.Dim() ==  CacheSize){
            CACHE_ELEMENT * Del = Heap.FindMin();
            if (Del->LockCount) { // Tutti gli elementi in Lock
               CacheSize ++; // Incremento temporaneamente la dimensione massima
            } else {
               Del->RemoveFromCache(this);
               Heap.DeleteMin();
               Hash.DeleteElementFromHash(Del);
            } /* endif */
         };
         BOOL Ok = Res->InitForCache(Key,this); // Se torna FALSE l' inserzione non e' andata a buon fine
         if (Ok) {
            Hash.Metti(KeyLen);
            Heap.Insert(Res);
         } else {
            TRACEHEXL(" Fallita inserzione elemento in CACHE, Key = ",Key,KeyLen,4);
            Res = NULL;
         } /* endif */
      } /* endif */
      if(Lr && Res)Res->LockCount ++;
      return Res;
   };

   // Dice se due ELEMENT con la stessa chiave debbano essere considerati equivalenti
   // NB: Il primo elemento e' la chiave, il secondo i dati letti da Hash
   virtual BOOL Equivalente(ELEMENT* Key,ELEMENT* DaHash){return TRUE;};

   ULONG Dim(){return Heap.Dim();};
   ELEMENT & operator [](int NumElemento){return Heap[NumElemento];};

   DWORD  AreaAllocata(){                  // Area totale allocata (via malloc)
      return Hash.AreaAllocata() + Heap.AreaAllocata();
   };

   // Lock ed UnLock ritornano il Lock Count aggiornato;
   // Si noti che non si deve passare solo la chiave ma proprio l' elemento!
   int MMB_FASTCALL Lock(ELEMENT & El){
      CACHE_ELEMENT & Res = (CACHE_ELEMENT&)El;
      return ++ Res.LockCount;
   };
   int MMB_FASTCALL UnLock(ELEMENT & El){
      CACHE_ELEMENT & Res = (CACHE_ELEMENT&)El;
      return -- Res.LockCount;
   };

   void Clear(BOOL Remove = TRUE){    // Pulisce la cache
      if(Remove)ORD_FORALL(Heap,i)Heap[i].RemoveFromCache(this); // Per attivare i metodi virtuali.
      Heap.Clear(); Hash.Clear();
      CacheSize = InitialCacheSize;
      NextPrg = 0;
   };

   void  Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA){ Hash.Trace(Msg,Livello);};

   protected:

   // EMS014 Win devo definire esternamente la funzione HEAP<CACHE_ELEMENT>::Confronta
   //friend int HEAP<CACHE_ELEMENT>::Confronta(CACHE_ELEMENT * a,CACHE_ELEMENT * b){
   //   if(a->LockCount != b->LockCount) return a->LockCount - b->LockCount;
   //   return (int)(a->UsagePrg)  - (int)(b->UsagePrg);
   //};

   // EMS015 VA
   friend class HEAP<class ITEM>;

   int  CacheSize       ;
   int  InitialCacheSize;
   BYTE KeyLen          ;

   DWORD NextPrg;

   HASH<CACHE_ELEMENT> Hash;
   HEAP2<CACHE_ELEMENT> Heap;

};

// begin EMS014 VA

template<class C> int HEAP<CACHE<C>::CACHE_ELEMENT>::Confronta(CACHE<C>::CACHE_ELEMENT * a,
                                                               CACHE<C>::CACHE_ELEMENT * b){
   if(a->LockCount != b->LockCount) return a->LockCount - b->LockCount;
   return (int)(a->UsagePrg)  - (int)(b->UsagePrg);
};

// end EMS014 VA

void MM_BASIC_TEST(); // Funzioncina di test
#endif
