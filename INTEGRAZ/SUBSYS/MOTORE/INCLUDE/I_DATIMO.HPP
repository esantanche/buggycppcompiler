//========================================================================
// Dati privati interfaccia con il motore
//========================================================================
// Consistono negli oggetti che interfacciano il motore ed i precaricati

#ifndef HO_I_DATIMO_HPP
#define HO_I_DATIMO_HPP

// ---------------------------------------------------------------------
// Classe per gestione degli instradamenti M300 (relazioni interne)
// ---------------------------------------------------------------------
// Ha due file:
// ID_ISTRA = Relazione interne
// ID_ISCTR = Relazione interne per cumulativo traghetti
// Entrambi hanno la stessa struttura
#define MAX_INSTRPRE 8
struct INSTRPRE { // 28 Bytes ad instradamento: 10000 instradamenti = 300K
   WORD IdStazioneDestinazione;     // Key (non univoca)
   WORD IdStazionePartenza;         // Key (non univoca) 
   WORD KmReali     ;
   WORD KmAggiuntivi;
   WORD KmMare      ;
   BYTE CodiceTariffaRegionale;     
   //    Per abbonamenti regionali
   //    0 = Non ammette abbonamenti regionali
   //    0xff = NON SO (usare motore)
   BYTE Ordine;                     // Ordine (nell' ambito della relazione)
   WORD Stazioni[MAX_INSTRPRE];     // Massimo MAX_INSTRPRE, se == 0 sono finite.
   void Inverti();                  // Inverte Origine e destinazione
   int NumeroStazioniInstradamento(); // Ritorna il numero di "Stazioni" > 0
   void Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
   void AssegnaA(ARRAY_ID & Percorso); // Usato solo dentro testmot !
};

class  GET_INSTRPRE : public FILE_BS {
   public:
   GET_INSTRPRE(const STRINGA& Path,const STRINGA& File= "ID_ISTRA.DB", int size = 1024):
   FILE_BS((CPSZ)(Path + File),sizeof(INSTRPRE),4,size,0,FALSE){
      Init();
   };
   virtual int Compare( const void * Key1, const void * Key2, ULONG DimConfronto){
      return Compare2Word_D(Key1,Key2,DimConfronto);
   };
   
   // I metodi seguenti impostano Ok a FALSE se non trovano la relazione
   // Altrimenti aggiornano Out
   void Cerca(ID Da,ID A,INSTRPRE & Out);
   void Cerca(ID A,INSTRPRE & Out);
   
   // I metodi seguenti scandiscono in sequenza il file
   // impostano Ok a FALSE alla fine file o se cambia la chiave rispetto a quella
   // contenuta in Out
   void CercaNext(INSTRPRE & Out); // Cambia Origine o Destinazione
   void CercaNextDestinazione(INSTRPRE & Out);

   INSTRPRE&  RecordCorrente(){ return *(INSTRPRE*) RecordC; };


   BOOL Ok;
   
//<<< class  GET_INSTRPRE : public FILE_BS {
};

// ---------------------------------------------------------------------
// Classe per gestione delle relazioni cumulative
// ---------------------------------------------------------------------
// File ID_ISCUM
struct RELCUMUL { // 27 Bytes a relazione, 2000 relazioni = 48K
   WORD IdStazioneDestinazione;     // Key (non univoca): Puo' essere FS o FC
   WORD TipoRelazione ;             // 0 = Semplice 1= Composta
   WORD IdStazioneTransitoFS;       // Sia per semplici che composte
   WORD CodiceCVB;                  // Codice della relazione CVB
   WORD KmConcessi1 ;               // Sia per semplici che composte
   WORD CodConcessione1 ;           // Sia per semplici che composte
   WORD CodLinea1;                  // Sia per semplici che composte
   WORD IdStazioneTransitoFS2;      // Solo composte
   WORD IdStazioneTransitoFS3;      // Solo composte (puo' essere 0)
   WORD KmFs        ;               // Solo composte
   WORD KmConcessi2 ;               // Solo composte
   WORD CodConcessione2 ;           // Solo composte
   WORD CodLinea2;                  // Solo composte
   BYTE CodiceTariffaRegionale;     
   //    Per abbonamenti regionali
   //    0 = Non ammette abbonamenti regionali
   //    0xff = NON SO (usare motore)
   void Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
};

// Classe per relazioni cumulative
class  GET_RELCUMUL : private FILE_BS {
   public:
   GET_RELCUMUL(const STRINGA& Path,const STRINGA& File= "ID_ISCUM.DB"):
   FILE_BS((CPSZ)(Path + File ),sizeof(RELCUMUL),2,512,0,FALSE){
      Init();
   };

   virtual int Compare( const void * Key1, const void * Key2, ULONG DimConfronto){ return CompareWord(Key1,Key2);};
   
   // Il metodo seguente imposta Ok a FALSE se non trova la relazione
   // Altrimenti aggiorna Out
   void Cerca(ID A,RELCUMUL & Out);
   
   // Il metodo seguente scandisce in sequenza il file
   // imposta Ok a FALSE alla fine file o se cambia la chiave rispetto a quella
   // contenuta in Out
   void CercaNext(RELCUMUL & Out);
   
   BOOL Ok;

   FILE_BS::ControllaSequenza; // Rende visibile
};

// ---------------------------------------------------------------------
// Classe per gestione delle traversate mare 
// ---------------------------------------------------------------------
// FILE ID_MACUM
// Ogni relazione e' ripetuta due volte (normale ed invertita)
// Classe per relazioni cumulative

class  GET_MARECUM  : private FILE_BS {
   public:
   GET_MARECUM(const STRINGA& Path,const STRINGA& File= "ID_MACUM.DB"):
   FILE_BS((CPSZ)(Path + File),sizeof(TRAVERSATA_MARE),2,512,0,FALSE){
      Init();
   };
   
   virtual int Compare( const void * Key1, const void * Key2, ULONG DimConfronto){ return Compare2Word_D(Key1,Key2,DimConfronto);};
   
   // Il metodo seguente imposta Ok a FALSE se non trova la relazione
   // Altrimenti aggiorna Out
   void Cerca(ID A,TRAVERSATA_MARE & Out);
   
   // Il metodo seguente scandisce in sequenza il file
   // imposta Ok a FALSE alla fine file o se cambia la chiave rispetto a quella
   // contenuta in Out
   void CercaNext(TRAVERSATA_MARE & Out);

   BOOL Ok;
   
   
   FILE_BS::Posiziona;         // Rende visibile
   FILE_BS::Next     ;         // Rende visibile
   FILE_BS::ControllaSequenza; // Rende visibile
//<<< class  GET_MARECUM  : private FILE_BS {
};

// ---------------------------------------------------------------------
// Classi per la gestione del caching dei risultati del motore
// ---------------------------------------------------------------------
// NB: La cache viene persa alla chiusura dell' applicazione
struct RICHIESTA_MOTORE {

   struct  KEY { // Uso i bit non tanto per guadagnar spazio quanto per migliorare la randomizzazione
      BIT  From            :13 ;
      BIT  To              :13 ;
      BIT  Livello         : 2 ;
      BIT  Giorno          : 5 ;
      BIT  Mese            : 4 ;
      BIT  _Anno           : 7 ; // Anno - 1980
      // Qui andranno altre eventuali caratteristiche di ricerca
   } KeyAssoluta;

   F_DTM DataEdOraRichiesta; // La data e' duplicata, ma conviene cosi'

   void  Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

};
// NB: se si cambia questa struttura aggiornare il metodo InitForCache()
struct RISPOSTA_MOTORE : public RICHIESTA_MOTORE {
   F_DTM  DataEdOraMaxUtilizzabile; // I risultati della ricerca sono utilizzabili fino a 4 ore dopo
   WORD   NumSlot;                  // Slot su cui metto i dati
   WORD   SlotDim ;                 // Dimensione dati

   BOOL  InitForCache(class RISPOSTA_MOTORE * Key, void * Cache);
   void  RemoveFromCache( void * Cache);
   void  Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
};

struct FREE_SLOT {
   FREE_SLOT * Next;
   WORD NumSlot; 
};

class  CACHE_MOTORE : public CACHE<RISPOSTA_MOTORE> {
   public:

   // Questo metodo cerca su Cache i dati della richiesta: se li trova
   // torna un MM_ELENCO_PERCORSO_E_SOLUZIONI (allocato ex novo), altrimenti
   // torna NULL.
   // NB: Vengono anche ristorati i percorsi del GRAFO
   class MM_ELENCO_PERCORSO_E_SOLUZIONI * GetSol(RICHIESTA_MOTORE & Richiesta, class DATI_ORARIO_FS & Orar);

   // Questo metodo mette su Cache la soluzione. In caso di errore torna FALSE
   BOOL PutSol(RICHIESTA_MOTORE & Richiesta, class MM_ELENCO_PERCORSO_E_SOLUZIONI * Soluzione, class DATI_ORARIO_FS & Orar);

   BOOL  Attiva     ; // Indica se e' attiva
   STRINGA CacheDir ; // Directory con il file di cache
   FILE_RW File     ; // File di cache

   CACHE_MOTORE(const STRINGA & Dir);
   ~CACHE_MOTORE();

   // Questa lista per trovare il prossimo slot LIBERO
   LISTA<FREE_SLOT> FreeSlots;
   FREE_SLOT *      Slots;

   virtual BOOL Equivalente(RISPOSTA_MOTORE* Key, RISPOSTA_MOTORE* DaHash){

      #ifdef DBGCACHE2
      ((RICHIESTA_MOTORE*)Key)->Trace("Key:");
      DaHash->Trace("In Hash:");
      #endif
      BOOL Result = Key->DataEdOraRichiesta >= DaHash->DataEdOraRichiesta  && 
      Key->DataEdOraRichiesta <= DaHash->DataEdOraMaxUtilizzabile;
      #ifdef DBGCACHE2
      TRACEVLONG(Result);
      #endif
      return Result;
   }

};
#endif
