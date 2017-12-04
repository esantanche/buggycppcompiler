//========================================================================
// MM_PERIO : Classi per date e periodicita
//========================================================================
//


#ifndef HO_MM_PERIO_HPP
#define HO_MM_PERIO_HPP


#ifndef HO_ALFA_HPP
#include "alfa0.hpp"
#endif

//----------------------------------------------------------------------------
// Prototipi Funzioni
//----------------------------------------------------------------------------
DWORD MaskPartenzeCompatibili( DWORD OrariPartenza, DWORD OrariArrivo  , 
   BYTE MinPercorrenza, BYTE MaxPercorrenza, BYTE Margine );

struct GGMMAA {
  char GG[2];  // Giorno Inizio Periodo
  char MM[2];  // Mese   Inizio Periodo
  char AA[2];  // Anno   Inizio Periodo
  operator STRINGA() const;     // Conversione in stringa (Formato GG/MM/AAAA)
};

struct PERIODIC {  // Struttura Periodicita'
  char    Codice        [ 2] ;    // Codice di periodicita'
  GGMMAA  InizioPeriodo      ;    // Inizio Periodo
  GGMMAA  FinePeriodo        ;    // Fine   Periodo
  STRINGA Analisi(); // Dump in chiaro di cosa rappresenta
};

// Array dinamica ad alte prestazioni di SDATA
struct S_DATE :  protected BUFR {
   SDATA & operator[] (UINT Indice){
      #ifdef BOUND_CHECK
      if(Indice >= Dim())BoundCheckError(Indice,Dim());
      #endif
      return * ( ((SDATA*)this->Dati) +Indice);
   };
   ULONG Dim(){return (this->Length /sizeof(SDATA));};
   S_DATE(ULONG i=0):BUFR(i* sizeof(SDATA)){};
   S_DATE(S_DATE & From):BUFR(From.Alloc){ THIS = From; };
   void ReDim(ULONG NumNodi){BUFR::ReDim(NumNodi* sizeof(SDATA));};
   S_DATE& operator=  (const S_DATE & From){return (S_DATE&)BUFR::operator=(From);};
   void operator+= (SDATA & Date){BUFR::Store(& Date, sizeof(Date));};
   void operator-= (UINT Indice);
   BUFR::Clear;  // Rendo accessibili
   SDATA& Last(){
      #ifdef BOUND_CHECK
      if(!Length)BoundCheckError(0,0);
      #endif
      return *(SDATA*)(Dati + Length -sizeof(SDATA));
   };
};   


// Array dinamica ad alte prestazioni di T_PERIODICITA
struct ARRAY_T_PERIODICITA :  protected BUFR {
   class T_PERIODICITA & operator[] (UINT Indice);
   ULONG Dim();
   ARRAY_T_PERIODICITA(ULONG i=0);
   ARRAY_T_PERIODICITA(ARRAY_T_PERIODICITA & From):BUFR(From.Alloc){ THIS = From; };
   void SetDimAndInitialize(ULONG Len,BYTE Dato);
   void ReDim(ULONG NumElem);
   ARRAY_T_PERIODICITA& operator=  (const ARRAY_T_PERIODICITA & From){return (ARRAY_T_PERIODICITA&)BUFR::operator=(From);};
   void operator+= (const class T_PERIODICITA & Date);
   
   BUFR::Clear;  // Rendo accessibili
   class T_PERIODICITA& Last();
   // Torna l' indice dell' oggetto o -1 se non trovato
   int Cerca(const T_PERIODICITA & Target);
   // Torna true o false
   BOOL Contiene(const T_PERIODICITA & Target){return Cerca(Target) != -1;};
   // Questa funzione identifica l' indice di una T_PERIODICITA nell' array.
   // Se non presente la aggiunge all' array
   int Indice(const T_PERIODICITA & Target);
   // Queste funzioni leggono o scrivono l' array da file. Tornano FALSE su errore
   BOOL Get(const STRINGA& NomeFile);
   BOOL Put(const STRINGA& NomeFile);
};

enum GIORNI_SETTIMANA {LUNEDI = 0,MARTEDI,MERCOLEDI,GIOVEDI,VENERDI,SABATO,DOMENICA};

//----------------------------------------------------------------------------
// T_PERIODICITA : Struttura per la gestione della periodicita' (su base giorno)
// derivata da MM_PERIODICITA aggiunge le funzioni di manipolazione
//----------------------------------------------------------------------------
// Serve per il caricamento dati e per particolari decodifiche on-line
// Ogni giorno e' rappresentato da un bit
// Gestisce un solo orario per volta

class T_PERIODICITA : public PERIODICITA {
   
   public:
   

   BYTE Tipo; // 1 = Effettuazione;  0 = Sospensione
   // Nota: La periodicita' rappresenta sempre i giorni in cui SI EFFETTUA
   // la corsa. Se si combinano piu' periodicita':
   // - Di tipo 0: si combinano in AND
   // - Di tipo 1: si combinano in OR
   // Rispettando l' ordine
   
   // Dati necessari per la gestione
   // NB: InizioOrario di PERIODICITA deve coincidere con Inizio_Dati_Caricati di T_PERIODICITA 
   static SDATA   Inizio_Dati_Caricati     ; // = Inizio_Orario_FS - 1
   static SDATA   Fine_Dati_Caricati       ; // = Fine_Orario_FS + 1
   static SDATA   Inizio_Orario_FS         ; // = Max (Inizio_DatiForniti, Inizio_Orario_FS originale)  
   static SDATA   Fine_Orario_FS           ; // = Min (Fine_DatiForniti, Fine_Orario_FS originale)
   static SDATA   Inizio_Dati_Forniti      ;
   static SDATA   Fine_Dati_Forniti        ;
   static SDATA   Inizio_Ora_Legale        ;
   static SDATA   Fine_Ora_Legale          ;
   static WORD    Ora_Inizio_Ora_Legale    ;
   static WORD    Ora_Fine_Ora_Legale      ;
   static SDATA   Inizio_AnnoScolastico    ;
   static SDATA   Fine_AnnoScolastico      ;
   static SDATA   Inizio_AnnoScolastico2   ;
   static SDATA   Fine_AnnoScolastico2     ;
   static S_DATE  GiorniFestivi            ;
   static S_DATE  FestivitaSoloScolastiche ;
                  
   // Dati relativi al caricamento
   static int     ProgressivoFornitura     ;
   static STRINGA NomeOrario               ;

   // Questi sono i limiti dell' orario
   static T_PERIODICITA InOrario   ;  // Giorni dell' orario
   static T_PERIODICITA InLimitiOrario ;  // Giorni dell' orario + 1 giorno per cambio data 
   static T_PERIODICITA FuoriOrario;  // Giorni fuori dell' orario
   
   // Queste sono le periodicita' corrispondenti ai tipi di periodicita'
   // Le periodicita' corrispondono a quelle definite sui FILE_T
   static ARRAY_T_PERIODICITA PeriodicitaPerCodice;
   static ELENCO_S       DecodPeriodicitaPerCodice;
   
   // Capita di dover confrontare due periodicit…, una delle quali deriva da un' altra
   // periodicit… shiftata (avanti od indietro) di N giorni.
   // Questo metodo confronta le due periodicit…, evitando i primi N giorni dell' orario
   // (se N negativo) oppure gli ultimi N giorni dell' orario (se N positivo)
   // Questo gestisce correttamente il fatto che i dati dell' orario sono troncati.
   // Si suppone che la periodicit… di base non sia shiftata, e quindi sia 
   // affidabile per tutto l' orario.
   // Il margine di shift accettabile va da -1 a +3 giorni
   // Il metodo ha due varianti, la seconda delle quali esegue anche lo shift
   // (alterando la periodicit… fornita).
   // Torna TRUE se le due periodicit… sono equivalenti
   BOOL __fastcall ConfrontaShiftata( const T_PERIODICITA & PeriodShiftata, signed char Shift) const;
   // La seguente ha il side effect di alterare PeriodDaShiftare
   BOOL __fastcall ConfrontaShiftando( T_PERIODICITA & PeriodDaShiftare, signed char Shift) const;
   void __fastcall ShiftMe(signed char Shift);

   void R_SetGiornoDellaSett(
      GIORNI_SETTIMANA Giorno, 
      BOOL s,
      SDATA InizioPeriodo,
      SDATA FinePeriodo
   );
   
   
   void R_SetFeste(
      BOOL s,
      SDATA InizioPeriodo,
      SDATA FinePeriodo
   );
   
   
   void R_SetFesteScol(
      BOOL s,
      SDATA InizioPeriodo,
      SDATA FinePeriodo
   );
   
   
   void R_SetPreFeste(
      BOOL s,
      SDATA InizioPeriodo,
      SDATA FinePeriodo  
   );
   
   
   void R_SetPostFeste(
      BOOL s,
      SDATA InizioPeriodo,
      SDATA FinePeriodo
   );

   void R_SetPostFeriale(
      BOOL s,
      SDATA InizioPeriodo,
      SDATA FinePeriodo
   );

   // Questo compone le periodicita' in output
   void ComponiPeriodicita(int NumPeriodic, class PERIODIC * Periodicita, BOOL Reset );
   
   
   // Questo metodo legge il file di definizione orario e carica in memoria le variabili
   // statiche di classe. Torna FALSE se ci sono errore
   static BOOL Init(const STRINGA &  PathFileDefinizione,const STRINGA & NomeFileDefinizione="D_ORARIO.TXT");
   // Formato del file definizione: (e' previsto sia caricato a mano o scaricando tabelle DB2)
   // GIORNO_INIZIO_ORARIO_FS  =LUNEDI|MARTEDI|MERCOLEDI|GIOVEDI|VENERDI|SABATO|DOMENICA
   // INIZIO_ORARIO_FS         =GG/MM/AAAA
   // FINE_ORARIO_FS           =GG/MM/AAAA
   // INIZIO_ANNOSCOLASTICO    =GG/MM/AAAA
   // FINE_ANNOSCOLASTICO      =GG/MM/AAAA
   // GIORNOFESTIVO            =GG/MM/AAAA
   // GIORNOFESTIVO            =GG/MM/AAAA
   //  ....
   // FESTIVITASOLOSCOLASTICA  =GG/MM/AAAA
   // FESTIVITASOLOSCOLASTICA  =GG/MM/AAAA
   //  ....
   

   WORD Cardinalita(); // Ritorna il numero di giorni di circolazione nell' ambito dell' orario

   // Questo metodo mette le periodicita' in una forma caricabile dal TPF.
   // NB: Richiede che sia stato chiamato il metodo Init()
   // Alloca ex novo una struttura della classe PERIODICITA_IN_CHIARO
   // La distruzione di tale struttura deve essere effettuata dal chiamante
   // NB: Euristic e' un parametro interno per chiamate ricorsive: non utilizzare
   class PERIODICITA_IN_CHIARO * EsplodiPeriodicita(BYTE Euristic=0)const;

   // Questa e' utile per controlli rapidi: Mette tutto in forma di stringa (anche lunga)
   STRINGA InChiaro(); // Mette in chiaro i dati del periodo

   // Questi metodi identificano il primo periodo che gode di una certa proprieta'
   // E' indicato a lato il criterio di fine periodo
   void PrimoPeriodoDiEffettuazione(SDATA & Dal, SDATA& Al);          // Si blocca per 15 giorni consecutivi di non effettuazione
   void PrimoPeriodoCostante(SDATA & Dal, SDATA& Al, BYTE Euristic);  // Si blocca quando cambia la periodicita' settimanale
   
//<<< class T_PERIODICITA : public PERIODICITA {
};

//----------------------------------------------------------------------------
// PERIODO_PERIODICITA
//----------------------------------------------------------------------------
// Questa struttura rappresenta in chiaro un periodo della periodicita'
struct PERIODO_PERIODICITA  {

   SDATA Dal;
   SDATA Al;

   T_PERIODICITA FormaCompatta    ; // Usato per le elaborazioni, non significativo alla fine

   BYTE  GiorniDellaSettimana     ; // Ogni bit rappresenta un giorno della settimana

   void Compute()                 ; // Ricalcola Dal, Al

   BYTE Tipo; // 1 = Effettuazione;  0 = Sospensione

   enum TIPO_PERIODO {
       TUTTI_I_GIORNI = 0           // = Si effettua / Sospeso
      ,GIORNI_LAVORATIVI   
      ,GIORNI_FESTIVI               // Giorni della settimana e' sempre 0x7f
      ,GIORNI_FESTIVI_E_SABATO      // Giorni della settimana e' sempre 0x7f 
      ,PRE_FESTIVI                  // Giorni della settimana e' sempre 0x7f 
      ,POST_FESTIVI                 // Giorni della settimana e' sempre 0x7f 
      ,SCOLASTICI                   // Giorni della settimana e' sempre 0x7f 
      ,NON_SCOLASTICI               // Giorni della settimana e' sempre 0x7f 
      ,TAPPO                        // Per scandire i tipi
   } TipoPeriodo;

   static ELENCO_S Decod_Tipo_Periodo;
};

//----------------------------------------------------------------------------
// PERIODICITA_IN_CHIARO
//----------------------------------------------------------------------------
// Questa struttura rappresenta in chiaro la periodicita'
struct  PERIODICITA_IN_CHIARO :  public ARRAY_DINAMICA<PERIODO_PERIODICITA> {

   PERIODICITA_IN_CHIARO(){HoTipiParticolari = FALSE;};

   void operator+= (const PERIODO_PERIODICITA &Elemento){
      ARRAY_DINAMICA<PERIODO_PERIODICITA>::operator+=(Elemento);
      if(Elemento.TipoPeriodo >= PERIODO_PERIODICITA::PRE_FESTIVI) HoTipiParticolari = TRUE;
   };

   ELENCO_S PeriodicitaLeggibile();

   void Trace(const STRINGA&  Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   protected:

   static ARRAY_T_PERIODICITA PeriodicitaBase; // Periodicita' corrispondenti ai TipiPeriodo
   void Popola(PERIODO_PERIODICITA & Origine, BYTE Euristic); // Riempie la struttura a partire da un periodo in forma NON normale

   static T_PERIODICITA          InSoppressione;

   friend class T_PERIODICITA;

   BOOL HoTipiParticolari;

};

#endif
