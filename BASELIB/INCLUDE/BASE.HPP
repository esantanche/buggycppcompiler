//========================================================================
// Tipi dati Standard
//========================================================================

#ifndef HO_BASE_HPP
#define HO_BASE_HPP

#ifndef HO_ELENCO_S_H
#include <elenco_s.h>
#endif

#define BIT   unsigned int
#define BYTE  unsigned char
#define WORD  unsigned short
#define DWORD unsigned long
#define HO_DELTASIZE  // Per compatibilita' SIPAX 3.1.1/3.1.2

// EMS001 typedef LHANDLE HFILE;

#define WORD_NON_VALIDA 0xffff

void _export BoundCheckError(int Indice,int Dim);

//----------------------------------------------------------------------------
// BUFR
//----------------------------------------------------------------------------
// Questa classe definisce un Buffer dinamico.
// L' implementazione e' ad alte prestazioni.
// Il BUFR puo' anche essere utilizzato come un array (dinamica) di bytes.
// Subclassando il BUFR si possono facilmente ottenere array dinamiche di
// strutture di dimensioni maggiori

class _export BUFR { // Buffer con allocazione: Analogo all' INI_BUFFER ma > 64K
// Inoltre i singoli bytes possono essere acceduti singolarmente
   public:
   ULONG  Length ;   // Lunghezza dei dati contenuti nell'area dati puntata
                     // dal puntatore Dati;

   BYTE   * Dati ;   // Puntatore all'area di memoria contenente i dati;

   ULONG  Pointer;   // Attributo utilizzato per indicare lo spiazzamento
                     // durante la scansione dell'area dati;

   ULONG  Alloc  ;   // Dimensione dell'area dati allocata per contenere i dati;

   ULONG  DeltaSize; // Dimensione da incrementare : di default 256 o 10% allocazione iniziale, puo' essere modificata dinamicamente

   BUFR(UINT Size=0){ DeltaSize = Size > 2560 ? Size / 10:256;Alloc=Size;Clear(); if(Size > 0) Dati = (BYTE*)malloc(Size); else Dati = NULL; };
   BUFR(const BUFR& From);
   BUFR(const void * Data, ULONG Size){
      DeltaSize = Size > 2560 ? Size / 10:256;Alloc=Size;Clear();
      if(Size > 0){
         Dati = (BYTE*)malloc(Size);
         memcpy(Dati,(void*)Data,Size);
         Length = Size;
      } else Dati = NULL;
   };
   BUFR& operator=  (const BUFR & From);
   BOOL  operator==  (const BUFR & From);
   BYTE & operator[] (UINT Indice)const{
      #ifdef BOUND_CHECK
      if(Indice >= Dim() )BoundCheckError(Indice,Dim());
      #endif
      return * (Dati + Indice);
   };
   BYTE & Last() const{
      #ifdef BOUND_CHECK
      if(!Length)BoundCheckError(0,0);
      #endif
      return *(Dati + Length -1);
   };
   ULONG Dim()const{return this->Length;};
   // ~BUFR();
   BUFR::~BUFR(){ if(Alloc>0)free(Dati);};
   void Clear(){Pointer=Length=0;};
   void ReDim(ULONG Size); // Libera la memoria SOLO se Size == 0
   void SetDim(ULONG Len){Length = Len;};
   void SetDimAndInitialize(ULONG Len,BYTE Dato){if(Len > Alloc)ReDim(Len);Length = Len;memset(Dati,Dato,Len);};
   void Store(const BUFR& From);                // Concatena dei dati (intero buffer)
   void Store(const BUFR& From,ULONG NumBytes); // Concatena dei dati dalla posizione indicata da From.Pointer
   void Store(const void* From,ULONG NumBytes); // Concatena dei dati (generici)
   void Insert(BYTE Dato,ULONG Pos);   // Inserisce un singolo byte in posizione intermedia
   void operator+= (const BYTE& Dato){Store(Dato);};
   void operator+= (const BUFR& Dati){Store(Dati);};
   void operator-= (UINT Pos);     // Elimina il byte alla posizione Pos;
// Queste inline per performance
   void Store(const DWORD& Num){
       if(Alloc < Length + sizeof(DWORD))ReDim(Alloc+DeltaSize);
       *(DWORD*)(Dati + Length) = Num; Length += sizeof(DWORD); };
   void Store(const WORD& Num){
       if(Alloc < Length + sizeof(WORD))ReDim(Alloc+DeltaSize);
       *(WORD*)(Dati + Length) = Num; Length += sizeof(WORD); };
   void Store(const BYTE& Num){
       if(Alloc < Length + sizeof(BYTE))ReDim(Alloc+DeltaSize);
       *(Dati + Length) = Num; Length += sizeof(BYTE); };
   void Store(const STRINGA& From){Store((CPSZ)From,From.Dim()+1);};
   static void Swap(BUFR & Primo, BUFR & Secondo); // Inverte i dati di due buffer (senza copiarli)

   void Store2(const BUFR& From){Store(From.Length);Store(From);}; // Concatena lunghezza e dati

   void ReStore(void* To,LONG NumBytes);           // Legge dei dati dalla posizione indicata da Pointer
// Queste inline per comodita'
   void ReStore(BYTE  & To){ReStore(&To,sizeof(BYTE ));};
   void ReStore(WORD  & To){ReStore(&To,sizeof(WORD ));};
   void ReStore(DWORD & To){ReStore(&To,sizeof(DWORD));};
   void ReStore(STRINGA& To){To = ((char*)Dati)+Pointer;Pointer += To.Dim()+1;};
   void ReStore2(BUFR & To){ DWORD Lg; ReStore(Lg); To.SetDimAndInitialize(Lg,0);ReStore(To.Dati,Lg);};
};

//----------------------------------------------------------------------------
// ARRAY_ID
//----------------------------------------------------------------------------
// E' un subclass di BUFR che gestisce ID (= WORD 2 Bytes)
// Gli Id possono rappresentare qualunque cosa, se pero' sono ID di stazioni
// il metodo Trace stampa sul trace le stazioni puntate dall' array.

typedef WORD ID;
#define ID_NON_VALIDO   0xffff
struct _export ARRAY_ID : protected BUFR {
// Questa deve essere inline
    ID& operator[] (UINT Indice)const{
       #ifdef BOUND_CHECK
         if(Indice >= Dim())BoundCheckError(Indice,Dim());
       #endif
       return * ( ((ID*)this->Dati) +Indice);
    };
    ULONG Dim()const {return (this->Length >> 1);};
// Ridefinizioni
    ARRAY_ID(ULONG i=0):BUFR(i* sizeof(ID)){};
    ARRAY_ID(ARRAY_ID & From):BUFR(From.Alloc){ THIS = From; };
    void ReDim(ULONG NumNodi){BUFR::ReDim(NumNodi* sizeof(ID));};
    void SetDimAndInitialize(ULONG NumId,BYTE Dato){BUFR::SetDimAndInitialize(NumId* sizeof(ID),Dato);};
    void Store(ID IdNodo){BUFR::Store(IdNodo);};
    ARRAY_ID& operator=  (const ARRAY_ID & From){return (ARRAY_ID&)BUFR::operator=(From);};
    void operator+= (ID IdNodo){Store(IdNodo);};
    void operator+= (const ARRAY_ID & Nodi){BUFR::Store(Nodi);};
    void operator-= (WORD Pos);
    BOOL operator== (const ARRAY_ID & B)const;
    BUFR::Clear;  // Rendo accessibili
    static void Swap(ARRAY_ID & Prima,ARRAY_ID & Seconda){ BUFR::Swap(Prima,Seconda);};
    WORD Posizione(ID Id)const;  // Ritorna la posizione o WORD_NON_VALIDA se non trovato l' Id
    WORD Posizione(ID Id,WORD Min)const;  // Ritorna la posizione o WORD_NON_VALIDA se non trovato l' Id
                                     // Ricerca solo a partire da Min
    BOOL Contiene(ID Id)const{return Posizione(Id)!= WORD_NON_VALIDA;};
    void Insert(ID Id,WORD Pos);
    ID& Last()const {
      #ifdef BOUND_CHECK
      if(!Length)BoundCheckError(0,0);
      #endif
      return *(ID*)(Dati + Length -sizeof(ID));
    };
    void Mirror(); // Inverte l' ordine degli ID

    BOOL IdDuplicati(); // Vera se un Id e' contenuto piu' volte nell' ARRAY_ID;
                        // ATTENZIONE: METODO NON RIENTRANTE: usa un' area statica per motivi di performance

    // Questo metodo ritorna una stringa nella forma [id1,id2, .. idn]
    // e' possibile specificare opzionalmente indici minimo e massimo da cui partire
    STRINGA ToStringa(WORD MinIdx=0,WORD MaxIdx=999);
    // Idem ma mette in chiaro i nomi delle stazioni
    STRINGA ToStringa(class STAZIONI & Stazs,WORD MinIdx=0,WORD MaxIdx=999);
    // Questo torna una stazione per riga
    void Trace(class STAZIONI & Stazs,const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

    // Questo metodo funziona solo all' interno del MOTORE
    void Trace(class GRAFO & Grafo,const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

    // Questi metodi permettono di gestire l' array come un set.
    // Sono relativamente veloci su grossi elenchi, e soprattutto su SET sparsi
    // ma non preservano l' ordine degli elementi.
    // Il Pointer NON e' gestito!
    void Elimina(UINT Pos);             // Equivale a -=  Opera a tempo costante
    void Unito(ARRAY_ID & Array2);      // Opera a tempo lineare (non mette duplicazioni)
    void Intersecato(ARRAY_ID & Array2);// Opera a tempo lineare
};

BOOL _export IdDuplicato(ID Id,BOOL Clear=FALSE);
   // Come ARRAY_ID::IdDuplicati, ma si fornisce un ID alla volta.
   // La routine puo' essere COMBINATA con il metodo (ma non viceversa)
   // Se Clear e' vero reset degli Id gia' forniti
   // ATTENZIONE: METODO NON RIENTRANTE: usa un' area statica per motivi di performance

//----------------------------------------------------------------------------
// ARRAY_DW
//----------------------------------------------------------------------------
// E' un subclass di BUFR che gestisce DWORD (= DWORD 4 Bytes)
#define DW_NON_VALIDO   0xffffffff
struct _export ARRAY_DW : protected BUFR {
// Questa deve essere inline
    DWORD& operator[] (UINT Indice)const{
       #ifdef BOUND_CHECK
         if(Indice >= Dim())BoundCheckError(Indice,Dim());
       #endif
       return * ( ((DWORD*)this->Dati) +Indice);
    };
    ULONG Dim()const {return (this->Length >> 2);};
// Ridefinizioni
    ARRAY_DW(DWORD i=0):BUFR(i* sizeof(DWORD)){};
    ARRAY_DW(ARRAY_DW & From):BUFR(From.Alloc){ THIS = From; };
    void ReDim(ULONG NumElem){BUFR::ReDim(NumElem* sizeof(DWORD));};
    void SetDimAndInitialize(ULONG NumDw,BYTE Dato){BUFR::SetDimAndInitialize(NumDw* sizeof(DWORD),Dato);};
    void Store(DWORD DwElem){BUFR::Store(DwElem);};
    ARRAY_DW& operator=  (const ARRAY_DW & From){return (ARRAY_DW&)BUFR::operator=(From);};
    void operator += (DWORD DwElem){Store(DwElem);};
    void operator += (const ARRAY_DW & Elem){BUFR::Store(Elem);};
    void operator -= (DWORD Pos);
    BOOL operator == (const ARRAY_DW & B)const;
    BUFR::Clear;  // Rendo accessibile
    DWORD Posizione(DWORD Dw)const;           // Ritorna la posizione o DW_NON_VALIDO se non trovato
    DWORD Posizione(DWORD Dw,DWORD Min)const;  // Ritorna la posizione o DW_NON_VALIDO se non trovato
                                             // Ricerca solo a partire da Min
    BOOL Contiene(DWORD Dw)const{return Posizione(Dw)!= DW_NON_VALIDO;};
    void Insert(DWORD Dw,DWORD Pos);
    DWORD& Last()const {
      #ifdef BOUND_CHECK
      if(!Length)BoundCheckError(0,0);
      #endif
      return *(DWORD*)(Dati + Length -sizeof(DWORD));
    };
    STRINGA ToStringa(WORD MinIdx=0,WORD MaxIdx=999);
    void Mirror(); // Inverte l' ordine
};
//----------------------------------------------------------------------------
// Esempio
//----------------------------------------------------------------------------
// In questo esempio si subclassa BUFR Per creare un' array di strutture di Prova.
// Si consideri sempre che si puo' anche subclassare ELENCO utilizzando
// DIN_ARRAY.H

//  struct PROVA {
//     char Key[5]; // Null Terminated
//     BYTE Byte1;
//     WORD Word1;
//     int Pippo;
//  };
//  struct ARRAY_PROVA : public BUFR {
//  // Questa deve essere inline
//     PROVA& operator[] (int Indice){return * ( ((PROVA*)this->Dati) +Indice);};
//     ULONG Dim(){return this->Length / sizeof(PROVA);};
//  // Ridefinizioni
//     ARRAY_PROVA(ULONG i=8):BUFR(i* sizeof(PROVA)){};
//     ARRAY_PROVA(ARRAY_PROVA & From):BUFR(From.Alloc){THIS = From;};
//     ~ARRAY_PROVA(){};
//     void ReDim(ULONG Num){BUFR::ReDim(Num* sizeof(PROVA));};
//     void Store(PROVA& PROVA){BUFR::Store(&PROVA,sizeof(PROVA));};
//     ARRAY_PROVA& operator=  (const ARRAY_PROVA & From){return (ARRAY_PROVA&)BUFR::operator=(From);};
//     void operator+= (PROVA& PROVA){Store(PROVA);};
//     void operator-= (int Pos){ if(Pos<0 || Pos >= Dim() )return;
//        if (Dim() > (Pos + 1)) { PROVA * x = & THIS[Pos]; memmove(x,(x+1),sizeof(PROVA)*(Dim()-(Pos+1))); }
//        Length -= sizeof(PROVA); if(Pointer > Pos)Pointer -= sizeof(PROVA); };
//     BUFR::Clear;  // Rendo accessibile
//     static void Swap(ARRAY_PROVA & Prima,ARRAY_PROVA & Seconda){ BUFR::Swap(Prima,Seconda);};
//     PROVA& Last(){return THIS[Dim()-1];};
//     void Sort(){ qsort((void *)Dati, Dim(), sizeof(PROVA), stricmp);};
//  };

#endif
