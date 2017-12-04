//----------------------------------------------------------------------------
// POLIM.HPP : Contiene le strutture dei dati di tariffazione via polimetriche
//----------------------------------------------------------------------------
// Sono definiti sia i records originali forniti da FS che le strutture 
// intermedie di caricamento
//----------------------------------------------------------------------------

#ifndef HO_POLIM_HPP
#define HO_POLIM_HPP

#include "stddef.h"

#define MAXPOLI  14
#define TOPPOLI  500    // Max 500 polimetriche

struct REC1 {
   char   TipoRecord    ; // Must Be '1'
   char   Ident[10]     ; // Identificativo della tavola
   char   NumStaz[3]    ; // Numero di stazioni contenute
   char   Zona[2]       ; // Zona di appartenenza della tavola
   char   Tipologia[2]  ; // Tipologia della polimetrica: si veda POLIMETRICA_CIP
   char   Descrizione[1]; // Descrizione della tavola
};

struct NODOCONDIZIONATO {
   char   Insieme[2];
   char   Flag;
};

struct REC2 {
   char   TipoRecord    ; // Must Be '2'
   struct RADICI {
      char CCR_Radice[5]; // Codice CCR della stazione radice (o "....." )
      char Filler;
   } Radici[9];
};

struct REC3 {
   char   TipoRecord   ;  // Must Be '3'
   char   CodCCR[5]    ;  // Codice CCR
   char   NomeStaz[20] ;  // Nome della stazione
   char   Diramazione           ;
   char   Allacciamento         ;
   char   Soppressa             ;
   char   TransitoInternazionale;
   char   Impresenziata         ;
   char   CoincidenzaConAutobus ;
   char   CasaCantoniera        ;
   char   TrattaSecondaria      ;
   char   NonAbilitata          ;
   char   DiInstradamento       ;
   NODOCONDIZIONATO NodiC[10]   ;
   char   Filler                ;
   char   RadiceStazione        ;
   char   RadiceStazione2       ;
   char   Filler2               ;
} ;

//----------------------------------------------
// Strutture derivate
//----------------------------------------------
// Le seguenti strutture sono caricate in memoria 
// direttamente leggendole dal file di CIPRIANO.

struct POLIMETRICA_CIP {   // Accesso per Id Polimetrica 

   // Attenzione: L' enum deve essere coerente con quella dichiarata in MM_GRAFO.HPP
   enum  TIPO_POLIMETRICA { 
      CUMULATIVA   = 1,     // Cumulativa semplice (percorso piu' breve)
      MARE_CUM     = 3,     // Cumulativa mare (Tirrenia)
      URBANA_CUM   = 4,     // Tratta urbana di congiunzione tra stazione di diramazione FS e stazione cumulativa
      LOCALE       = 5,     // Polimetrica locale, contenuta all' interno di una zona
      SOLO_LOCALE  = 6,     // Polimetrica SOLO locale, non puo' essere usata per tariffare altro che tratte contenute
      DIRAMAZIONE  = 7,     // Polimetrica di diramazione
      ALLACCIAMENTO= 8,     // Polimetrica di allacciamento
      MARE_FS      = 9
   };

   WORD     IdPolimetrica   ;  // Interno, assegnato sequenzialmente, 0 lasciato libero
   BYTE     TipoPolimetrica ;
   char     Nome[11]        ;
   char     Descrizione[51] ;
   WORD     Zona            ; // Zona di appartenenza
   ARRAY_ID Stazioni;   // Stazioni della polimetrica
   ARRAY_ID Radici;     // Radici della polimetrica (NB: eventuali Radici non valide: metto 0 come codice)
                        // E' riportato l' ID della redice

   POLIMETRICA_CIP(){ memset(this,0,offsetof(POLIMETRICA_CIP,Stazioni)); };

   WORD IdxRadice(BYTE Livello){ if(Livello < Radici.Dim()){ WORD Idx  = Stazioni.Posizione(Radici[Livello]); if(Idx  != WORD_NON_VALIDA)return Idx; }ERRINT("Errore: Livello non gestibile",Livello); return 0; };

};

//struct _export ARRAY_POLI : protected BUFR {
//    POLIMETRICA_CIP& operator[] (UINT Indice){ return * ( ((POLIMETRICA_CIP*)this->Dati) +Indice); };
//    ULONG Dim(){return (this->Length / sizeof(POLIMETRICA_CIP));};
//    ARRAY_POLI(ULONG i=0):BUFR(i* sizeof(POLIMETRICA_CIP)){};
//    void operator+= (POLIMETRICA_CIP& Item){BUFR::Store(&Item,sizeof(POLIMETRICA_CIP));};
//    BUFR::Clear;  // Rendo accessibili
//};

struct STAZ_POLI_CIP {     // Accesso diretto per ID SIPAX
   struct LEGAMI { // Fino a MAXPOLI polimetriche: se sono di meno lascio a 0 l' Id
     WORD  Id;
     BYTE  Tipo;
     BYTE  Livello  ;        // Se > 0 vuol dire che sta in una tratta secondaria in QUESTA polimetrica
                             // E' riportato il numero di radice della tratta secondaria
     ARRAY_ID Distanze;      // Seguono lo stesso ordine delle stazioni definito nella polimetrica.
                             // Se non ho dati di collegamento tra le due stazioni vale 0xffff
   } Polimetriche[MAXPOLI];
   BIT   Diramazione            :1 ;
   BIT   DiramazioneAnomala     :1 ;   // Vuol dire che NON compare in tavole di diramazione o allacciamento
   BIT   Allacciamento          :1 ;
   BIT   Soppressa              :1 ;
   BIT   TransitoInternazionale :1 ;
   BIT   Impresenziata          :1 ;
   BIT   CoincidenzaConAutobus  :1 ;
   BIT   CasaCantoniera         :1 ;
   BIT   TrattaSecondaria       :1 ;   // Vuol dire che sta in una tratta secondaria in almeno una polimetrica
   // SET   Zone                      ;
   WORD  IsCnd                     ;   // Istradamento condizionato
   WORD  IsCndInibit               ;   // Istradamento condizionato : Inibizione (per le stazioni di diramazione
   STAZ_POLI_CIP(){ Diramazione = Allacciamento = Soppressa = TransitoInternazionale = 
   Impresenziata = CoincidenzaConAutobus = CasaCantoniera = TrattaSecondaria = 0;
   IsCnd = IsCndInibit = 0;
   Polimetriche[0].Id = Polimetriche[1].Id = Polimetriche[2].Id = Polimetriche[3].Id = Polimetriche[4].Id = 0;
   };

   LEGAMI & DaIdPolim(WORD Id){
       for (int i = 0;i < MAXPOLI ; i ++ ) {
          if(Polimetriche[i].Id == 0 )break; // Non ne ho piu'
          if(Polimetriche[i].Id == Id)break; // Trovata
       } /* endfor */
       if(i < MAXPOLI && Polimetriche[i].Id == Id)return  Polimetriche[i];
       return *(LEGAMI*)NULL;
  };
  int NumeroPolimetriche(BYTE Tipo = 0){ // Se tipo == 0 considero tutti i tipi
     int Out = 0;
     for (int NumPolim=0; NumPolim < MAXPOLI ; ++NumPolim ){
        if(Polimetriche[NumPolim].Id == 0) break;
        if(Tipo == 0 || Tipo ==  Polimetriche[NumPolim].Tipo)Out ++;
     }
     return Out;
  }

};

struct _export ARRAY_ISTR_COND : protected BUFR {
    ISTR_COND& operator[] (UINT Indice){ return * ( ((ISTR_COND*)this->Dati) +Indice); };
    ULONG Dim(){return (this->Length / sizeof(ISTR_COND));};
    ARRAY_ISTR_COND(ULONG i=0):BUFR(i* sizeof(ISTR_COND)){};
    void operator+= (ISTR_COND& Item){BUFR::Store(&Item,sizeof(ISTR_COND));};
    BUFR::Clear;  // Rendo accessibili
};


// Questa la struttura che utilizzo per i dati dei rami
struct ETAB_RAMO {
   ID   IdRamo  ;
   ID   St1     ;
   ID   St2     ;
   BYTE Mare    ; // Indica se e' un ramo mare
   BYTE Concesso; // Indica se e' un ramo concesso
   BYTE SocCon  ; // Codice societa' concessa
   WORD Km      ; // Interi
};

// Questa la struttura che utilizzo per il rank delle stazioni
struct RANK_FILE {
   char Id[5];
   char Istr[3];
   char Rank[3];
   char NomeStaz[35];
};

//----------------------------------------------------------------------------
// RETI_CUMULATIVE
//----------------------------------------------------------------------------
struct RETI_CUMULATIVE {
   BYTE CodiceRete           ;
   char Nome[26]             ;
   WORD StazioneDitransito[8];
   WORD NumStazioni          ;
   BYTE HaInstradamentiAusiliari;
};
class F_RETI_CU : public FILE_BS {
   public:

   // Costruttore
   F_RETI_CU(const char* NomeFile,ULONG Size=0) : FILE_BS(NomeFile,sizeof(RETI_CUMULATIVE),1,Size,0,FALSE){Init();};
   
   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(BYTE Rete){ return Posiziona(BUFR(VRB(Rete))) && KeyEsatta;};
   
   // Definizione operatori
   RETI_CUMULATIVE &  RecordCorrente(){ return *(RETI_CUMULATIVE*) RecordC; };
   RETI_CUMULATIVE &  FixRec(DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(RETI_CUMULATIVE*) RecordC; };
};
struct POLIM_CUMULAT {   // Solo le polimetriche semplici: le complesse usano la struttura POLIM_CUMULAT_2
   BYTE CodiceRete             ; // Key: Codice Rete
   WORD Id                     ; // Key: Codice stazione
   WORD Distanze [8]           ; // Distanze fino a stazioni di transito  
};
struct POLIM_CUMULAT_1 {  // Polimetriche fittizie di collegamento con reti cumulative
   WORD Id1                    ; // Key: Codice stazione
   WORD Id2                    ; // Key: Codice stazione
};
struct POLIM_CUMULAT_2 {   // Solo le polimetriche complesse: le semplici usano la struttura POLIM_CUMULAT
   BYTE CodiceRete             ; // Key: Codice Rete
   WORD Id                     ; // Key: Codice stazione
   BYTE NumTransito            ; // Numero del transito
   WORD Transito               ; // Id della stazione di transito 
   WORD Distanza               ; // Distanza fino a stazione di transito
   WORD Instradamento [4]      ; // Instradamento
};

class F_POLIM_CUMULAT : public FILE_BS {

   public:

   // Costruttore
   F_POLIM_CUMULAT(const char* NomeFile,ULONG Size=0) : FILE_BS(NomeFile,sizeof(POLIM_CUMULAT),3,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(BYTE Rete){ return Posiziona(BUFR(VRB(Rete))) && KeyEsatta;};
   BOOL Seek(ID Stazione, BYTE Rete){ BUFR Wrk; Wrk.Store(Rete); Wrk.Store(Stazione); return Posiziona(Wrk) && KeyEsatta; };

   // Definizione operatori
   POLIM_CUMULAT&  RecordCorrente(){ return *(POLIM_CUMULAT*) RecordC; };
   POLIM_CUMULAT&  FixRec(DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(POLIM_CUMULAT*) RecordC; };
   
   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ 
      POLIM_CUMULAT & A = *(POLIM_CUMULAT*) Key1;
      POLIM_CUMULAT & B = *(POLIM_CUMULAT*) Key2;
      if(DimConfronto <  2 || A.CodiceRete != B.CodiceRete) return int(A.CodiceRete) - int(B.CodiceRete);
      return int(A.Id) - int(B.Id);
   };
};
class F_POLIM_CUMULAT_1 : public FILE_FIX {

   public:

   // Costruttore
   F_POLIM_CUMULAT_1(const char* NomeFile,ULONG Size=0) : FILE_FIX(NomeFile,sizeof(POLIM_CUMULAT_1),Size){};

   // Definizione operatori
   POLIM_CUMULAT_1 &  RecordCorrente(){ return *(POLIM_CUMULAT_1*) RecordC; };
   POLIM_CUMULAT_1 &  operator [](DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(POLIM_CUMULAT_1*) RecordC; };
};

class F_POLIM_CUMULAT_2 : public FILE_BS {

   public:

   // Costruttore
   F_POLIM_CUMULAT_2(const char* NomeFile,ULONG Size=0) : FILE_BS(NomeFile,sizeof(POLIM_CUMULAT_2),3,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(BYTE Rete){ return Posiziona(BUFR(VRB(Rete))) && KeyEsatta;};
   BOOL Seek(ID Stazione, BYTE Rete){ BUFR Wrk; Wrk.Store(Rete); Wrk.Store(Stazione); return Posiziona(Wrk) && KeyEsatta; };

   // Definizione operatori
   POLIM_CUMULAT_2 &  RecordCorrente(){ return *(POLIM_CUMULAT_2*) RecordC; };
   POLIM_CUMULAT_2 &  FixRec(DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(POLIM_CUMULAT_2*) RecordC; };
   
   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ 
      POLIM_CUMULAT_2 & A = *(POLIM_CUMULAT_2*) Key1;
      POLIM_CUMULAT_2 & B = *(POLIM_CUMULAT_2*) Key2;
      if(DimConfronto <  2 || A.CodiceRete != B.CodiceRete) return int(A.CodiceRete) - int(B.CodiceRete);
      return int(A.Id) - int(B.Id);
   };
};

#endif
