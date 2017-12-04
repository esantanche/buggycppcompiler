//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3

// EMS001
typedef unsigned long BOOL;
#include "base.hpp"
#include "id_stazi.hpp"

//----------------------------------------------------------------------------
// BoundCheckError
//----------------------------------------------------------------------------
void BoundCheckError(int Indice, int Dim){
   #undef TRCRTN
   #define TRCRTN "BoundCheckError"
   ERRSTRING("Errore: Puo' causare Abend; Richiesto indice "+STRINGA(Indice)+" su elementi Nø "+STRINGA(Dim));
   BEEP;
};

//----------------------------------------------------------------------------
// BUFR::ReDim
//----------------------------------------------------------------------------
void BUFR::ReDim(ULONG Size){
   #undef TRCRTN
   #define TRCRTN "BUFR::ReDim"

   // Fix
   // Per tollerare eventuali errate impostazioni di DeltaSize a 0 lo forzo a 256.
   // Capita quando si fa un memset della struttura a 0 (il che e' utile in certi casi)
   if(Size == Alloc) Size += 256;

   // Libera l' area solo se = 0, altrimenti la lascia allocata, per privilegiare le performance
   if(Size < Alloc){
      if(Size < Length)Length   = Size;
      if(Size < Pointer)Pointer = Size;
      if(Size==0){delete Dati; Dati = NULL;Alloc = 0;};
   } else if(Size > Alloc){
      Dati = (BYTE*)realloc(Dati,Size);
      Alloc=Size;
   };
};
//----------------------------------------------------------------------------
// BUFR::operator=
//----------------------------------------------------------------------------
// Assegnazione per BUFR e copy constructor
// Alloca una nuova copia dei dati
BUFR& BUFR::operator=(const BUFR & From){
   #undef TRCRTN
   #define TRCRTN "BUFR::operator="
   Length  = From.Length;
   Pointer = From.Pointer;
   ReDim(From.Alloc);
   if(Length)memcpy(Dati,From.Dati,Length);
   return *this;
};
//BUFR::BUFR(UINT Size){DeltaSize = 256; Alloc=Size;Clear(); if(Size > 0) Dati = (BYTE*)malloc(Size); else Dati = NULL; };
//BUFR::~BUFR(){ if(Alloc>0)free(Dati);};
//----------------------------------------------------------------------------
// @BUFR
//----------------------------------------------------------------------------
BUFR::BUFR(const BUFR & From){
   #undef TRCRTN
   #define TRCRTN "@BUFR"
   DeltaSize = 256;
   Length  = From.Length;
   Pointer = From.Pointer;
   Alloc   = From.Alloc;
   DeltaSize = From.DeltaSize ;
   if(From.Alloc > 0){
      Dati = (BYTE*)malloc(From.Alloc); // Alloca l' area
      // EMS002 Win memset a zero dopo malloc
      memset(Dati, 0L, From.Alloc);
   } else Dati = NULL;
   if(Length)memcpy(Dati,From.Dati,Length);
};
//----------------------------------------------------------------------------
// BUFR::Swap
//----------------------------------------------------------------------------
// Swap di due BUFR
void BUFR::Swap(BUFR & Primo, BUFR & Secondo){
   #undef TRCRTN
   #define TRCRTN "BUFR::Swap"
   BYTE* Tmp;
   ULONG Tmp2;
   Tmp  = Primo.Dati      ; Primo.Dati      = Secondo.Dati      ; Secondo.Dati      = Tmp;
   Tmp2 = Primo.Length    ; Primo.Length    = Secondo.Length    ; Secondo.Length    = Tmp2;
   Tmp2 = Primo.Pointer   ; Primo.Pointer   = Secondo.Pointer   ; Secondo.Pointer   = Tmp2;
   Tmp2 = Primo.Alloc     ; Primo.Alloc     = Secondo.Alloc     ; Secondo.Alloc     = Tmp2;
   Tmp2 = Primo.DeltaSize ; Primo.DeltaSize = Secondo.DeltaSize ; Secondo.DeltaSize = Tmp2;
};

//----------------------------------------------------------------------------
// BUFR::operator==
//----------------------------------------------------------------------------
// Operatore confronto
BOOL  BUFR::operator==  (const BUFR & From){
   #undef TRCRTN
   #define TRCRTN "BUFR::operator=="
   if (Length != From.Length) return FALSE;
   return !memcmp(Dati,From.Dati,Length);
};
//----------------------------------------------------------------------------
// BUFR::Store
//----------------------------------------------------------------------------
void BUFR::Store(const BUFR& From){          // Concatena dei dati (intero buffer)
   #undef TRCRTN
   #define TRCRTN "BUFR::Store"
   ULONG Size=Length + From.Length;
   if(Size > Alloc)ReDim(Size + DeltaSize); // Debbo riallocare: lascio DeltaSize bytes di margine
   char * To = (char *)Dati; To += Length;
   if(From.Length)memcpy(To,From.Dati,From.Length);
   Length = Size;
};
void BUFR::Store(const BUFR& From,ULONG NumBytes){ // Concatena dei dati dalla posizione indicata da From.Pointer
   ULONG Size=Length + NumBytes;
   if(Size > Alloc)ReDim(Size + DeltaSize ); // Debbo riallocare: lascio DeltaSize bytes di margine
   char * To = (char *)Dati;      To += Length;
   char * Da = (char *)From.Dati; Da += From.Pointer;
   if(NumBytes)memcpy(To,Da,NumBytes);
   Length = Size;
};
void BUFR::Store(const void*  From,ULONG NumBytes){ // Concatena dei dati dalla posizione indicata da From.Pointer
   ULONG Size=Length + NumBytes;
   if(Size > Alloc)ReDim(Size + DeltaSize ); // Debbo riallocare: lascio DeltaSize bytes di margine
   char * To = (char *)Dati;      To += Length;
   if(NumBytes)memcpy(To,From,NumBytes);
   Length = Size;
};
//----------------------------------------------------------------------------
// BUFR::ReStore
//----------------------------------------------------------------------------
void BUFR::ReStore(void* To,LONG NumBytes){        // Legge dei dati dalla posizione indicata da Pointer
   #undef TRCRTN
   #define TRCRTN "BUFR::ReStore"
   char * From = (char*)Dati; From += Pointer;
   Pointer +=NumBytes;
   if(Pointer > Length) BEEP; // Seguira' probabilmente abend
   if(NumBytes)memcpy(To,From,NumBytes);
}
//----------------------------------------------------------------------------
// ARRAY_ID::Posizione
//----------------------------------------------------------------------------
WORD ARRAY_ID::Posizione(ID Id)const{
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::Posizione"
   DWORD DimAr = Dim();
   for (DWORD i=0;i < DimAr ;i++ ) {
      if(THIS[i] == Id)return i;
   } /* endfor */
   return ID_NON_VALIDO;
};
WORD ARRAY_ID::Posizione(ID Id,WORD Min)const{
   DWORD DimAr = Dim();
   for (DWORD i=Min;i < DimAr ;i++ ) {
      if(THIS[i] == Id)return i;
   } /* endfor */
   return ID_NON_VALIDO;
};
// EMS003 Win #pragma option -Od   // Su questo modulo l' ottimizzatore crea problemi
//----------------------------------------------------------------------------
// ARRAY_ID::Mirror
//----------------------------------------------------------------------------
void ARRAY_ID::Mirror(){
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::Mirror"
   int Limit = Dim() -1;   // Max indice dell' array
   if(Limit < 1) return;
   ID Tmp;
   ID * Array = (ID*)Dati;
   for (int i=(Dim()/2)-1 ;i >=0 ; i--) {
      Tmp = Array[i];
      Array[i] = Array[Limit-i];
      Array[Limit-i] = Tmp;
   } /* endfor */
};
// EMS003 Win #pragma option -O2
//----------------------------------------------------------------------------
// ARRAY_ID::IdDuplicati()
//----------------------------------------------------------------------------
static ID * Set_Pointer = NULL;
static ULONG Set_Dim    =0;
static BYTE Set_Prog    = 0xff; // Impostato solo alla partenza del programma
// QUesto metodo lascia TUTTI gli id dell' array impostati nell' area globale
BOOL ARRAY_ID::IdDuplicati(){
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::IdDuplicati"
   if(++Set_Prog == 0) {
      if(Set_Dim)memset(Set_Pointer,0,Set_Dim * sizeof(ID));
      Set_Prog = 1;
   }
   ID * Base   = (ID*) Dati;
   ID * Limit  = Base + Dim();
   BOOL Result = FALSE;
   for (ID * pId = Base ; pId < Limit; pId ++) {
      ID Id = *pId;
      if (Id >= Set_Dim){ // Ridimensiono l' area statica
         ULONG OldSet_Dim = Set_Dim;
         Set_Dim = 512 * (Id/512 + 1); // Prossimo multiplo di 512 ID
         Set_Pointer = (ID*)realloc(Set_Pointer,Set_Dim * sizeof(ID));
         memset(Set_Pointer + OldSet_Dim, 0, (Set_Dim - OldSet_Dim) * sizeof(ID));
      };
      if(Set_Pointer[Id]== Set_Prog){
         Result = TRUE;
      } /* endif */
      Set_Pointer[Id]=Set_Prog;
   } /* endfor */
   return Result;
//<<< BOOL ARRAY_ID::IdDuplicati
};
//----------------------------------------------------------------------------
// IdDuplicato
//----------------------------------------------------------------------------
// Questo metodo lascia gli id testati nell' area globale
BOOL IdDuplicato(ID Id , BOOL Clear){
   #undef TRCRTN
   #define TRCRTN "IdDuplicato"
   if(Clear){
      if(++Set_Prog == 0) {
         if(Set_Dim)memset(Set_Pointer,0,Set_Dim * sizeof(ID));
         Set_Prog = 1;
      }
   }
   if (Id >= Set_Dim){ // Ridimensiono l' area statica
      ULONG OldSet_Dim = Set_Dim;
      Set_Dim = 512 * (Id/512 + 1); // Prossimo multiplo di 512 ID
      Set_Pointer = (ID*)realloc(Set_Pointer,Set_Dim * sizeof(ID));
      memset(Set_Pointer + OldSet_Dim, 0, (Set_Dim - OldSet_Dim) * sizeof(ID));
   };
   if(Set_Pointer[Id]== Set_Prog){
      return TRUE;
   } /* endif */
   Set_Pointer[Id]=Set_Prog;
   return FALSE;
};
//----------------------------------------------------------------------------
// ARRAY_ID::Elimina
// ARRAY_ID::Unito
// ARRAY_ID::Intersecato
//----------------------------------------------------------------------------
// Questi metodi permettono di gestire l' array come un set.
// Sono relativamente veloci su grossi elenchi, e soprattutto su SET sparsi
// ma non preservano l' ordine degli elementi.
// Il Pointer NON e' gestito!
// Questi metodi lasciano SOLO PARTE degli id delle array impostati nell' area globale
//----------------------------------------------------------------------------
// ARRAY_ID::Elimina
//----------------------------------------------------------------------------
void ARRAY_ID::Elimina(UINT Pos){ // Equivale a -=  ma opera a tempo costante
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::Elimina"
   if(Pos >= Dim() ){
      BoundCheckError(Pos,Dim());
      return;
   };
   THIS[Pos] =  THIS[Dim() -1];
   Length -= sizeof(ID);
}
//----------------------------------------------------------------------------
// ARRAY_ID::Unito
//----------------------------------------------------------------------------
void ARRAY_ID::Unito(ARRAY_ID & Array2){      // Opera a tempo lineare (non mette duplicazioni)
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::Unito"
   IdDuplicati();
   ID * Base   = (ID*) Array2.Dati;
   ID * Limit  = Base + Array2.Dim();
   for (ID * pId = Base ; pId < Limit; pId ++) {
      ID Id = *pId;
      if (Id >= Set_Dim || Set_Pointer[Id]!= Set_Prog){ // Non e' in comune ai due insiemi
         THIS += Id;
      } /* endif */
   } /* endfor */
}
//----------------------------------------------------------------------------
// ARRAY_ID::Intersecato
//----------------------------------------------------------------------------
void ARRAY_ID::Intersecato(ARRAY_ID & Array2){// Opera a tempo lineare
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::Intersecato"
   Array2.IdDuplicati();
   ID * Base   = (ID*) Dati;
   ID * Limit  = Base + Dim() -1;
   for (ID * pId = Limit; pId >= Base; pId --) {
      ID Id = *pId;
      if (Id >= Set_Dim || Set_Pointer[Id]!= Set_Prog){ // Non e' in comune ai due insiemi
         *pId = *Limit;
         Limit --;
         Length -= sizeof(ID);
      } /* endif */
   } /* endfor */
};
//----------------------------------------------------------------------------
// BUFR::Insert()
//----------------------------------------------------------------------------
void BUFR::Insert(BYTE Dato,ULONG Pos){
   #undef TRCRTN
   #define TRCRTN "BUFR::Insert"

   if(Pos >= Dim()){
      Store(Dato); // Aggiungo alla fine
   } else {
      if(Alloc < Length + sizeof(BYTE))ReDim(Alloc+DeltaSize );
      memmove((Dati + Pos+1),(Dati + Pos),(Length - Pos)); // Scalo array
      Dati[Pos] = Dato;
      Length ++;
   } /* endif */
};
//----------------------------------------------------------------------------
// BUFR::operator-=
//----------------------------------------------------------------------------
void BUFR::operator-= (UINT Pos){
   #undef TRCRTN
   #define TRCRTN "BUFR::operator-="
   if(Pos >= Dim() ){
      #ifdef BOUND_CHECK
      BoundCheckError(Pos,Dim());
      #endif
      return;
   };
   if (Dim() > (Pos + 1)) { // Test se e' l' ultimo elemento
      BYTE * x = Dati + Pos;
      memmove(x,(x+1),(Dim()-(Pos+1)));
   } /* endif */
   Length --;
   if(Pointer > Pos)Pointer --;
};
//----------------------------------------------------------------------------
// ARRAY_ID::operator-=
//----------------------------------------------------------------------------
void ARRAY_ID::operator-= (WORD Pos){
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::operator-="
   if(Pos >= Dim() ){
      BoundCheckError(Pos,Dim());
      return;
   };
   if (Dim() > (Pos + 1)) { // Test se e' l' ultimo elemento
      ID * x = & THIS[Pos];
      memmove(x,(x+1),sizeof(ID)*(Dim()-(Pos+1)));
   } /* endif */
   Length -= sizeof(ID);
   if(Pointer > Pos)Pointer -= sizeof(ID);
};
//----------------------------------------------------------------------------
// ARRAY_ID::operator==
//----------------------------------------------------------------------------
BOOL ARRAY_ID::operator== (const ARRAY_ID & b)const {
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::operator=="
   if(Dim()!= b.Dim())return FALSE;
   ID * i1 = &THIS[0];   // Puntatore al primo array di ID
   ID * i2 = &b[0]; // Puntatore al secondo array di ID
   ID * Stop = i1 + Dim();
   while (i1 < Stop) {  // Scansione dell' array
      if(*i1 != *i2)return FALSE;
      i1++; i2++;
   } /* endwhile */
   return TRUE;
};
//----------------------------------------------------------------------------
// ARRAY_ID::Insert()
//----------------------------------------------------------------------------
void ARRAY_ID::Insert(ID Id,WORD Pos){
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::Insert"

   if(Pos >= Dim()){
      THIS += Id; // Aggiungo alla fine
   } else {
      if(Alloc < Length + sizeof(ID))ReDim(Alloc+DeltaSize );
      ID * Base = (ID*)Dati;
      memmove((Base + Pos+1),(Base + Pos),Length - (sizeof(ID)*Pos)); // Scalo array
      THIS[Pos] = Id;
      Length += sizeof(ID);
   } /* endif */
};
//----------------------------------------------------------------------------
// ARRAY_ID::ToStringa()
//----------------------------------------------------------------------------
STRINGA ARRAY_ID::ToStringa(WORD MinIdx,WORD MaxIdx){
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::ToStringa"

   if(Dim() <= MinIdx || MaxIdx < MinIdx)return STRINGA("[]");

   char Tmp[1024];
   char * Out = Tmp;
   ORD_FORALL(THIS,i){
      if(i >= MinIdx && i <= MaxIdx){
         strcpy(Out,",");
         itoa(THIS[i],Out+1,10);
         Out = Out +strlen(Out);
      };
   };
   Tmp[0]='[';
   *Out = ']';
   *(Out+1) = '\0';
   return STRINGA(Tmp);
};


//----------------------------------------------------------------------------
// ARRAY_DW::Posizione
//----------------------------------------------------------------------------
DWORD ARRAY_DW::Posizione(DWORD Dw)const{
   #undef TRCRTN
   #define TRCRTN "ARRAY_DW::Posizione"
   DWORD DimAr = Dim();
   for (DWORD i=0;i < DimAr ;i++ ) {
      if(THIS[i] == Dw)return i;
   } /* endfor */
   return DW_NON_VALIDO;
};
DWORD ARRAY_DW::Posizione(DWORD Dw,DWORD Min)const{
   DWORD DimAr = Dim();
   for (DWORD i=Min;i < DimAr ;i++ ) {
      if(THIS[i] == Dw)return i;
   } /* endfor */
   return DW_NON_VALIDO;
};
//----------------------------------------------------------------------------
// ARRAY_DW::operator-=
//----------------------------------------------------------------------------
void ARRAY_DW::operator-= (DWORD Pos){
   #undef TRCRTN
   #define TRCRTN "ARRAY_DW::operator-="
   if(Pos >= Dim() ){
      BoundCheckError(Pos,Dim());
      return;
   };
   if (Dim() > (Pos + 1)) { // Test se e' l' ultimo elemento
      DWORD * x = & THIS[Pos];
      memmove(x,(x+1),sizeof(DWORD)*(Dim()-(Pos+1)));
   } /* endif */
   Length -= sizeof(DWORD);
   if(Pointer > Pos)Pointer -= sizeof(DWORD);
};
//----------------------------------------------------------------------------
// ARRAY_DW::operator==
//----------------------------------------------------------------------------
BOOL ARRAY_DW::operator== (const ARRAY_DW & b)const {
   #undef TRCRTN
   #define TRCRTN "ARRAY_DW::operator=="
   if(Dim()!= b.Dim())return FALSE;
   DWORD * i1 = &THIS[0];   // Puntatore al primo array di DWORD
   DWORD * i2 = &b[0]; // Puntatore al secondo array di DWORD
   DWORD * Stop = i1 + Dim();
   while (i1 < Stop) {  // Scansione dell' array
      if(*i1 != *i2)return FALSE;
      i1++; i2++;
   } /* endwhile */
   return TRUE;
};
//----------------------------------------------------------------------------
// ARRAY_DW::Insert()
//----------------------------------------------------------------------------
void ARRAY_DW::Insert(DWORD Dw,DWORD Pos){
   #undef TRCRTN
   #define TRCRTN "ARRAY_DW::Insert"

   if(Pos >= Dim()){
      THIS += Dw; // Aggiungo alla fine
   } else {
      if(Alloc < Length + sizeof(DWORD))ReDim(Alloc+DeltaSize );
      DWORD * Base = (DWORD*)Dati;
      memmove((Base + Pos+1),(Base + Pos),Length - (sizeof(DWORD)*Pos)); // Scalo array
      THIS[Pos] = Dw;
      Length += sizeof(DWORD);
   } /* endif */
};

//----------------------------------------------------------------------------
// ARRAY_DW::ToStringa()
//----------------------------------------------------------------------------
STRINGA ARRAY_DW::ToStringa(WORD MinIdx,WORD MaxIdx){
   #undef TRCRTN
   #define TRCRTN "ARRAY_DW::ToStringa"

   if(Dim() <= MinIdx || MaxIdx < MinIdx)return STRINGA("[]");

   char Tmp[1024];
   char * Out = Tmp;
   ORD_FORALL(THIS,i){
      if(i >= MinIdx && i <= MaxIdx){
         strcpy(Out,",");
         itoa(THIS[i],Out+1,10);
         Out = Out +strlen(Out);
      };
   };
   Tmp[0]='[';
   *Out = ']';
   *(Out+1) = '\0';
   return STRINGA(Tmp);
};

// EMS003 Win #pragma option -Od   // Su questo modulo l' ottimizzatore crea problemi
//----------------------------------------------------------------------------
// ARRAY_DW::Mirror
//----------------------------------------------------------------------------
void ARRAY_DW::Mirror(){
   #undef TRCRTN
   #define TRCRTN "ARRAY_DW::Mirror"
   int Limit = Dim() -1;   // Max indice dell' array
   if(Limit < 1) return;
   DWORD Tmp;
   DWORD * Array = (DWORD*)Dati;
   for (int i=(Dim()/2)-1 ;i >=0 ; i--) {
      Tmp = Array[i];
      Array[i] = Array[Limit-i];
      Array[Limit-i] = Tmp;
   } /* endfor */
};
// EMS003 Win #pragma option -O2
