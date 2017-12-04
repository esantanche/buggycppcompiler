#include "dummy.h"

//----------------------------------------------------------------------------
// Nota: al momento le template non funzionano, usare le macro definendo:
//    DIN_ARRAY       = Nome che si vuol dare alla classe
//    CLAS            = Classe di cui faccio l' elenco
// le seguenti definizioni sono opzionali
//    CastFrom(PVOID) = Macro di cast da CLAS a PVOID
//    CastTo(CLAS)    = Macro di cast da PVOID a CLAS
//
// Questa definizione deve essere fatta SOLO se  nella classe vengono
// sempre aggiunti puntatori ottenuti via NEW: in tal caso
// impostando questa #define il distruttore distrugge
// tutti gli oggetti contenuti. idem il metodo Clear()
// Solo se CLAS e' una classe di puntatori !!
//    DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
// In tal caso e' obbligatoria anche la seguente definizione:
//    CopiaMembro(CLAS)  Riallocazione e copia
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Template DIN_ARRAY
//----------------------------------------------------------------------------
// Questa classe per definire i varii tipi specifici di elenchi
// Richiede che sia definita una macro od una funzione di cast
// da CLAS a PVOID CastFrom
// ed inversa da PVOID a CLAS CastTo
// Per i LONG ed i puntatori i defaults sono Ok.
//----------------------------------------------------------------------------
// Al momento gestisce solo dati <= 32 bits o dati in cui pongo un puntatore
// I dati vengono infatti passati per VALORE nelle chiamate
//----------------------------------------------------------------------------

// Controlli
#ifndef CLAS
#error Obbligatorio definire "CLAS" !
#endif

#ifndef DIN_ARRAY
#error Obbligatorio definire "DIN_ARRAY" !
#endif

#ifndef HO_ELENCO_H
#error Obbligatorio includere elenco.h !
#endif

// Questi sono dei defaults
#ifndef CastFrom
#define CastFrom(_x)   PVOID(_x)
#endif

#ifndef CastTo
#define CastTo(_x)   *(CLAS*)(& _x)
#endif

#define Ele (*(ELENCO*)this)  // Questo oggetto visto come elenco

#if defined(DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE) && ! defined(CopiaMembro)
#error Per le classi con puntatori dinamici e' obbligatorio definire la macro CopiaMembro
#endif

class DIN_ARRAY : public ELENCO{

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Area publica
//++++++++++++++++++++++++++++++++++++++++++++++++++
   public :

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Costruttore di default
//++++++++++++++++++++++++++++++++++++++++++++++++++
   DIN_ARRAY(unsigned int num=0,unsigned int max=32000):
      ELENCO(num,max){};

// Costruttore di copia
   DIN_ARRAY(const DIN_ARRAY& a ):
      ELENCO((ELENCO&) a){};

// Distruttore virtuale non di default
   ~DIN_ARRAY(){
       this->Clear();
   };

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Operatori ridefiniti
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Operatore di assegnazione
   DIN_ARRAY& operator=(const DIN_ARRAY& A){
#ifdef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
      this->Clear();
      FORALL(THIS,i){   // Opero con un ciclo di riallocazione.
         if(A[i] != (CLAS)NULL){
            THIS[i]= CopiaMembro(A[i]);
            MEMIGNORE(THIS[i]);
         } else THIS[i] = NULL;
      }
#else
      Ele = (ELENCO &)A;
#endif
      return THIS;
   };

// []  Accede all' iesimo elemento  (tornato per reference)
   CLAS& operator[](unsigned int idx) const{
      return CastTo(Ele[idx]);
   };
// +=  Aggiunge un PVOID alla lista
// +   Aggiunge un PVOID alla lista  (e' lo stesso operatore di +=)
   DIN_ARRAY& operator+=(CLAS Ogg){
      Ele += CastFrom(Ogg);
#ifdef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
      MEMIGNORE(CastFrom(Ogg));
#endif
      return THIS;
   };
   DIN_ARRAY& operator+(CLAS Ogg){
      Ele += CastFrom(Ogg);
#ifdef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
      MEMIGNORE(CastFrom(Ogg));
#endif
      return THIS;
   };
// -=  Toglie un PVOID alla lista
// -   Toglie un PVOID alla lista  (e' lo stesso operatore di -=)
// Elimina : Come sopra, ma si basa sulla posizione
   DIN_ARRAY& operator-=(CLAS Ogg){
// Modifica Montagna: Inoltre puo' distruggere la struttura puntata
#ifdef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
         if(Ogg != NULL) delete Ogg;
#endif
      Ele -= CastFrom(Ogg);
      return THIS;
   };
   DIN_ARRAY& operator-(CLAS Ogg){
#ifdef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
         if(Ogg != NULL) delete Ogg;
#endif
      Ele -= CastFrom(Ogg);
      return THIS;
   };
   DIN_ARRAY& Elimina(int Pos){
#ifdef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
         if(THIS[Pos]!= NULL) delete (CLAS)THIS[Pos];
#endif
      Ele.Elimina(Pos);
      return THIS;
   };

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Metodi    (pubblici)
//++++++++++++++++++++++++++++++++++++++++++++++++++
// Numero di CLAS dell' elenco
   int Dim() const { return Ele.Dim();};
// Torna l' indice del CLAS o -1 se non trovato
   int Cerca(const CLAS & a) const {
      return Ele.CercaPerPuntatore(CastFrom(a));
   };
// Idem ma permette di avere puntatori duplicati
   int Cerca(const CLAS & a,int LastMatch) const {
      return Ele.CercaPerPuntatore(CastFrom(a),LastMatch);
   };
// Ritorna il numero degli oggetti con quel Puntatore
   int Conta(const CLAS a) const {
      return Ele.ContaPerPuntatore(CastFrom(a));
   };
// Vuota l' array
   void Clear(){
#ifdef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
      FORALL(THIS,i){
// Aggiunto cast a CLAS
         if(THIS[i]!= NULL) delete (CLAS)THIS[i];
         THIS[i] = NULL; }
#endif
      Ele.Clear();
   };

// Last      : Accede all' ultimo elemento
   CLAS& Last(){return THIS[THIS.Dim() - 1];};


//++++++++++++++++++++++++++++++++++++++++++++++++++
//<<< class DIN_ARRAY : public ELENCO{
};
#undef Ele
#undef DIN_ARRAY
#undef CLAS
#undef CastFrom
#undef CastTo
#ifdef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
#undef DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
#endif
#ifdef CopiaMembro
#undef CopiaMembro
#endif

