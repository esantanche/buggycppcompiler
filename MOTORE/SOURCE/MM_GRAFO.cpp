//----------------------------------------------------------------------------
// MM_GRAFO.CPP
//----------------------------------------------------------------------------
// Il file contiene tutte le routines che permettono di risolvere
// la componente del modello FS che non dipende dall' orario (GRAFO)
// In particolare:
//
// - La componente topologica vera e propria (Grafo)
// - La componente tariffaria (Polimetriche)
//
//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

// EMS001
typedef unsigned long BOOL;

// begin EMS002 Win sostituisco tutti i __fastcall con WIN_FASTCALL
// e definisco WIN_FASTCALL nulla
#define WIN_FASTCALL
// end EMS002 Win

#define LtrcIDS   LIVELLO_DI_TRACE_DEL_PROGRAMMA  // Livello di TRACE per TRACEID
#include "base.hpp"
#include "alfa0.hpp"
#include "scandir.h"


#ifdef OKTRACE   // Ad evitare strani abend
//----------------------------------------------------------------------------
// Controllo opzioni
//----------------------------------------------------------------------------
//#define DBGSTD         // DBG2 + DBG2P + DBG5 + DBG11 + DBG12
//#define DBG1           // traccia chiamate a Risolvi
//#define DBG1A          // Segue dettagli di Risolvi
//#define DBG1B          // Mostra anche le potenziali attivazioni scartate
//#define DBGP           // traccia chiamate a Risolvi Polimetriche
//#define DBG1P          // Segue dettagli di Risolvi Polimetriche
//#define DBG2           // Segue la valorizzazione dei percorsi via polimetriche
//#define DBG2P          // Mostra il percorso grafo originale ed eventuali correzioni
//#define DBG3           // Segue la formazione del PERCORSO_GRAFO
//#define DBG4           // Controllo LOOP di prolunga
//#define DBG5           // Segue identificazione nodi dominanti
//#define DBG6           // Segue identificazione nodi dominanti dettaglio
//#define DBG7           // Dettaglio polimetriche PRIMA dell' accorpamento
//#define DBG8           // Controllo simmetria grafo ed altri
//#define DBG9           // Dettaglio estremo identificazione stazioni vincolanti
//#define DBG10          // Dettaglio identificazione forma leggibile instradamento
//#define DBG11          // Mostra operazioni di conciliazione tra percorso grafo e polimetriche
//#define DBG11A         // Mostra dettaglio operazioni di conciliazione tra percorso grafo e polimetriche
//#define DBG12          // Mostra gestione eccezioni di normativa
//#define DBG_TAR_REG    // Dettaglio tutte stazioni toccate per tariffe regionali
//#define DBGCACHE       // Dettaglio costruzione cache (Store e ReStore)
#endif
//----------------------------------------------------------------------------
// Flags di controllo logica dell' algoritmo : DI NORMA DISABILITATI !
//----------------------------------------------------------------------------
// Servono per test particolari
//#define NO_CUMULATIVO  // Non usa i rami cumulativi per i percorsi minimi
//#define NO_CONDIZIONAT // Non rispetta i vincoli delle stazioni di diramazione condizionata
//#define SI_ZONE        // Usa le polimetriche delle sole zone selezionate per trovare i percorsi minimi
//#define SI_DIRAMAZION  // Usa le polimetriche di diramazione ed allacciamento per i percorsi minimi
//#define NO_CORR_POLI   // Per test: disabilita l' uso delle polimetriche per individuare i percorsi liberi sul grafo
//----------------------------------------------------------------------------
// Flags di controllo logica accorpamento finale polimetriche
//----------------------------------------------------------------------------
//#define ACCORPAINDIM     // Permette l' accorpamento delle polimetriche solo se diminuisce la percorrenza complessiva
#define LIMACCORPAMENTO 3  // Km di delta (assoluti) oltre i quali non permette l' accorpamento
#define CUMTOLLERANZA   3  // Km di delta (assoluti) entro i quali tariffa cumunque con i Km della polimetrica cumulativa
//----------------------------------------------------------------------------
#define DBG_TAR_REG_LV 3 // Livello di trace cui mostro il dettaglio
//----------------------------------------------------------------------------
#ifdef DBGSTD         // DBG2 + DBG2P + DBG5 + DBG11
#define DBG2
#define DBG2P
#define DBG5
#define DBG11
#define DBG12
#endif

// Queste funzioni per aprire il file stazioni
BOOL InitStazioni(const STRINGA& Path);
void StopStazioni();
GRAFO * GRAFO::Grafo = NULL;   // EMS003 VA azzero puntatore GRAFO::Grafo
GRAFO & GRAFO::Gr(){return *GRAFO::Grafo;};
BOOL _export RiconciliazioneAbilitata = !(STRINGA(getenv("DISABILITA_RICONCILIAZIONE")) == "SI");
BOOL _export RiconciliazioneNecessaria = FALSE;

//----------------------------------------------------------------------------
// Variabili statiche
//----------------------------------------------------------------------------
// Queste variabili per i varii metodi Risolvi / RisolviPolimetriche
// Nota: utilizzare array statiche per motivi di performance
static ARRAY_ID Attivi(512);          // Id degli elementi attivi
static ARRAY_ID AttiviNextStep(512);  // Id degli elementi che saranno attivi nello step seguente
// Questa deve essere visibile in fp_tre
BOOL PrivilegiaRamoDiretto=TRUE;
DIST GRAFO::Limite;

// Questa variabile per un controllo fine dell' utilizzo delle polimetriche di diramazione
// Necessario per gli algoritmi di controllo INTERNO delle polimetriche
#ifdef SI_DIRAMAZION
BOOL _export Si_Diramazion = TRUE;
#else
BOOL _export Si_Diramazion = FALSE;
#endif

ELENCO_S TipiPoli("", "CUMULATIVA" ,"???" ,"MARE_CUM" ,"URBANA_CUM" ,"LOCALE","SOLO_LOCALE" ,"DIRAMAZIONE" ,"ALLACCIAMENTO" ,"MARE_FS");

//----------------------------------------------------------------------------
// @F_ECCZ_NORM
//----------------------------------------------------------------------------
F_ECCZ_NORM::F_ECCZ_NORM(const char* NomeFile,ULONG Size) :
FILE_FIX(NomeFile,sizeof(ECCEZ_NORMATIVA),Size){
   #undef TRCRTN
   #define TRCRTN "@F_ECCZ_NORM"
   TRACESTRING("Entro nel costruttore di F_ECCZ_NORM");
   for (int j = 0; j < (sizeof(IdxTipi) / sizeof(IdxTipi[0])) ; j++ ) IdxTipi[j] = -1;
   TRACESTRING("Piu avanti nel costruttore di F_ECCZ_NORM");
   TRACEVLONG(THIS.Dim());
   ORD_FORALL(THIS, i ){
      ECCEZ_NORMATIVA & Ecz = THIS[i];
      if(IdxTipi[Ecz.Tipo] < 0){
         IdxTipi[Ecz.Tipo] = i;
         #ifdef DBG12
         TRACESTRING("IdxTipi["+STRINGA(Ecz.Tipo)+"] = "+ STRINGA(IdxTipi[Ecz.Tipo]));
         #endif
      }
      switch (Ecz.Tipo) {
      case 3:
         if(!StazioniConInibizione.Contiene(Ecz.Id1) )StazioniConInibizione += Ecz.Id1;
         break;
      case 4:
         if(Ecz.Id1 != 0){
            GrandiCorrezioni.Set(Ecz.Id1);
            GrandiCorrezioni.Set(Ecz.Id2);
         };
         break;
      case 5:
         if(!CoppieConInibizione.Contiene(Ecz.Id1) )CoppieConInibizione += Ecz.Id1;
         break;
      case 6:
         if(!StazioniDiGruppoInibiz.Contiene(Ecz.Id1) )StazioniDiGruppoInibiz += Ecz.Id1;
         break;
      } /* endswitch */
   }
   #ifdef DBG12
   TRACEVLONG(THIS.Dim());
   StazioniConInibizione.Trace(Stazioni,"StazioniConInibizione");
   CoppieConInibizione.Trace(Stazioni,"CoppieConInibizione");
   StazioniDiGruppoInibiz.Trace(Stazioni,"StazioniDiGruppoInibiz");
   #endif
//<<< FILE_FIX NomeFile,sizeof ECCEZ_NORMATIVA ,Size
};
//----------------------------------------------------------------------------
// F_ECCZ_NORM::DeterminaIdx
//----------------------------------------------------------------------------
void WIN_FASTCALL F_ECCZ_NORM::DeterminaIdx(int Tipo, int & Minimo, int & Massimo){ // Determina il minimo e massimo di scansione per tipo
   #undef TRCRTN
   #define TRCRTN "F_ECCZ_NORM::DeterminaIdx"
   Minimo = IdxTipi[Tipo];
   Massimo = Minimo;
   if (Minimo < 0 ) return;
   int NumTipi = sizeof(IdxTipi) / sizeof(IdxTipi[0]) ;
   for ( int i = Tipo +1; i < NumTipi ; i++ ) {
      if ( IdxTipi[i] > Minimo ) {
         Massimo = IdxTipi[i];
         return;
      } /* endif */
   } /* endfor */
   Massimo = Dim();
};
//----------------------------------------------------------------------------
// GRAFO::GRAFO
//----------------------------------------------------------------------------
GRAFO::GRAFO(const STRINGA& Path) {   // Carica dai files
   #undef TRCRTN
   #define TRCRTN "@GRAFO"

   TRACEVSTRING2(Path);
   if(GRAFO::Grafo == NULL){ // Solo la prima Istance e' registrata
      GRAFO::Grafo = this;
      TRACEPOINTER("GRAFO::Grafo = ", GRAFO::Grafo);
   }
   if(PROFILER::NonAttivo())PROFILER::Clear(TRUE); // Ad evitare ABENDS

   AbilitaCumulativo = FALSE; // Inizializzazione

   ProgressivoCorrente = 0xff  ;
   A_Valido            = 0     ;

   if(Path == NUSTR) {
      StazioniGrafo       = NULL  ;
      Rami                = NULL  ;
      Polim               = NULL  ;
      IstrCond            = NULL  ;
      FilePolim           = NULL  ;
      IstrEccez           = NULL  ;
      IstrEccez_REG       = NULL  ;
      EccezioniNormativa  = NULL  ;
      TotStazioniGrafo    = 0     ;
      TotRami             = 0     ;
      TotPolim            = 0     ;
      TotIstrCond         = 0     ;
   } else {
      InitStazioni(Path);
      PROFILER::Somma(0,Stazioni.DimBuffers() );

      FILE_RO Grafo1DB(Path +  "MM_GRAF1.DB");
      FILE_RO Grafo1EX(Path +  "MM_GRAF1.EXT");
      FILE_RO Grafo2DB(Path +  "MM_GRAF2.DB");
      FILE_RO Grafo2EX(Path +  "MM_GRAF2.EXT");
      FILE_RO KmEcczDB(Path +  "MM_KMECZ.DB");
      FilePolim = new F_POLIM (Path +  "MM_GRAF3.DB");
      PROFILER::Somma(0,FilePolim->DimBuffers() );
      PROFILER::Somma(1,FilePolim->DimBuffers() );
      if (TestFileExistance(Path +  "MM_ISCU2.DB")) {
         IstrEccez = new F_ISTRECC (Path +  "MM_ISCU2.DB");
         PROFILER::Somma(0,IstrEccez->DimBuffers() );
         PROFILER::Somma(1,IstrEccez->DimBuffers() );
      } else {
         IstrEccez = NULL;
      } /* endif */
      if (TestFileExistance(Path +  "MM_ECZRG.DB")) {
         IstrEccez_REG = new F_ISTRECC_REG (Path +  "MM_ECZRG.DB");
         PROFILER::Somma(0,IstrEccez_REG->DimBuffers() );
         PROFILER::Somma(1,IstrEccez_REG->DimBuffers() );
      } else {
         IstrEccez_REG = NULL;
      } /* endif */
      if (TestFileExistance(Path +  "MM_ECZNR.DB")) {
         EccezioniNormativa = new F_ECCZ_NORM (Path +  "MM_ECZNR.DB");
         PROFILER::Somma(0,EccezioniNormativa->DimBuffers() );
         PROFILER::Somma(1,EccezioniNormativa->DimBuffers() );
      } else {
         EccezioniNormativa = NULL;
      } /* endif */
      F_POLIM & Grafo3DB = *FilePolim;
      FILE_RO Grafo3EX(Path +  "MM_GRAF3.EXT");
      FILE_RO Grafo4DB(Path +  "MM_GRAF4.DB");

      #define Get(_a,_b,_c) {                                       \
         _b.Leggi(_b.FileSize(),Buffer);                            \
         _a    = (_c*)Buffer.Dati;                                  \
         Tot##_a = Buffer.Length/ sizeof (_c);                      \
         Buffer.Dati=NULL; Buffer.Length = Buffer.Alloc = 0;        \
      }
      #define GetExt(_a,_b) {                 \
         _b.Leggi(_b.FileSize(),Ext##_a);     \
      }

      BUFR Buffer;
      Get(StazioniGrafo,Grafo1DB,STAZ_FS);
      Get(Rami,Grafo2DB,RAMO);
      GetExt(StazioniGrafo,Grafo1EX);
      GetExt(Rami,Grafo2EX);
      GetExt(Polim,Grafo3EX);
      Get(IstrCond,Grafo4DB,ISTR_COND);
      Get(CorrezioniRami,KmEcczDB,CORREZIONI_RAMI);
      #undef Get
      #undef GetExt

      Polim = new POLIMETRICA[Grafo3DB.Dim()];
      TotPolim = Grafo3DB.Dim();
      ORD_FORALL(Grafo3DB,i){
         Polim[i] = Grafo3DB[i]; // Copia solo i dati della superclasse, ma non le descrizioni
      }

      TRACEVLONG(TotStazioniGrafo * sizeof(STAZ_FS    ) );
      TRACEVLONG(TotRami          * sizeof(RAMO       ) );
      TRACEVLONG(TotPolim         * sizeof(POLIMETRICA) );
      TRACEVLONG(TotIstrCond      * sizeof(ISTR_COND  ) );
      TRACEVLONG(ExtStazioniGrafo.Dim()                 );
      TRACEVLONG(ExtRami.Dim()                          );
      TRACEVLONG(ExtPolim.Dim()                         );

      PROFILER::Somma(0,TotStazioniGrafo * sizeof(STAZ_FS    ) );
      PROFILER::Somma(0,TotRami          * sizeof(RAMO       ) );
      PROFILER::Somma(0,TotPolim         * sizeof(POLIMETRICA) );
      PROFILER::Somma(0,TotIstrCond      * sizeof(ISTR_COND  ) );
      PROFILER::Somma(0,ExtStazioniGrafo.Dim()                 );
      PROFILER::Somma(0,ExtRami.Dim()                          );
      PROFILER::Somma(0,ExtPolim.Dim()                         );

      PROFILER::Somma(1,TotStazioniGrafo * sizeof(STAZ_FS    ) );
      PROFILER::Somma(1,TotRami          * sizeof(RAMO       ) );
      PROFILER::Somma(1,TotPolim         * sizeof(POLIMETRICA) );
      PROFILER::Somma(1,TotIstrCond      * sizeof(ISTR_COND  ) );
      PROFILER::Somma(1,ExtStazioniGrafo.Dim()                 );
      PROFILER::Somma(1,ExtRami.Dim()                          );
      PROFILER::Somma(1,ExtPolim.Dim()                         );

      // Questa per la memoria statica utilizzata da Risolvi ed in tutto il file (approssimato)
      PROFILER::Somma(0,4096);
      PROFILER::Somma(1,4096);

      #ifdef DBG8
      TRACESTRING("Controllo Simmetria");
      for (int s= 1;s < TotStazioniGrafo ;s++ ) {
         STAZ_FS & Staz = StazioniGrafo[s];
         if (!Staz.Nodale)continue;
         STAZ_FS::PRIMO_V * Pv = Staz.PrimiVicini();
         for (int i = 0;i < Staz.NumeroPrimiVicini ; i++ ) {
            STAZ_FS::PRIMO_V & PrimoVicino = Pv[i];
            STAZ_FS & Staz2 = StazioniGrafo[PrimoVicino.Id];
            STAZ_FS::PRIMO_V & PrimoVicino2 = Staz2.PvDaId(s);
            if(& PrimoVicino2 == NULL || PrimoVicino2.Id != s || PrimoVicino2.Distanza != PrimoVicino.Distanza ){
               ERRSTRING("Fallito controllo simmetria, ramo "+STRINGA(s)+"=>"+STRINGA(PrimoVicino.Id));
            }
         } /* endfor */
      } /* endfor */
      // Controllo che tutti i rami concessi abbiano un codice di societa' concessa
      for (s = 1; s < TotRami;s++ ) {
         RAMO & Ramo = Rami[s];
         if(Ramo.Concesso && Ramo.CodConcessa == 0){
            ERRSTRING("Fallito controllo, ramo concesso "+STRINGA(Ramo.IdStaz1)+"=>"+STRINGA(Ramo.IdStaz2)+" non ha impostato codice societa' concessa");
         }
      } /* endfor */
      // Controllo che tutte le polimetriche concesse abbiano un codice di societa' concessa
      for (s = 1; s < TotPolim;s++ ) {
         POLIMETRICA & Poli = Polim[s];
         // Le polimetriche di collegamento tra stazioni di transito e stazioni cumulative (URBANA_CUM)
         // sono concesse ma hanno codice di ferrovia concessa pari a 0.
         if(Poli.Concessa() && Poli.TipoPoli != POLIMETRICA::URBANA_CUM &&  Poli.SocietaConcessa == 0){
            STRINGA DesPoli = "ID= " + STRINGA(Poli.IdPoli);
            DesPoli += " Nø "+ STRINGA((*FilePolim)[Poli.IdPoli].Nome)+ " " ;
            DesPoli += STRINGA(FilePolim->RecordCorrente().Descrizione).Strip();
            ERRSTRING("Fallito controllo, polimetrica concessa "+DesPoli+" Tipo "+TipiPoli[Poli.TipoPoli]+" non ha impostato codice societa' concessa");
         }
      } /* endfor */
      #endif

//<<< if Path == NUSTR
   } /* endif */

   TRACEPOINTER("GRAFO::Grafo = 2", GRAFO::Grafo);

//<<< GRAFO::GRAFO const STRINGA& Path      // Carica dai files
};
GRAFO::~GRAFO(){
   #undef TRCRTN
   #define TRCRTN "~GRAFO"
   TRACESTRING("Fine");
   Clear();
   if(GRAFO::Grafo == this){ // Solo la prima Istance e' registrata
      GRAFO::Grafo = NULL;
      StopStazioni();
   }
   if( StazioniGrafo      != NULL ) delete StazioniGrafo      ;
   if( Rami               != NULL ) delete Rami               ;
   if( Polim              != NULL ) delete Polim              ;
   if( IstrCond           != NULL ) delete IstrCond           ;
   if( FilePolim          != NULL ) delete FilePolim          ;
   if( IstrEccez          != NULL ) delete IstrEccez          ;
   if( IstrEccez_REG      != NULL ) delete IstrEccez_REG      ;
   if( EccezioniNormativa != NULL ) delete EccezioniNormativa ;
};

//----------------------------------------------------------------------------
// GRAFO::Clear
//----------------------------------------------------------------------------
void GRAFO::Clear(){
   #undef TRCRTN
   #define TRCRTN "GRAFO::Clear"
   Instradamenti.Clear();
   PercorsiGrafo.Clear();
}
//----------------------------------------------------------------------------
// GRAFO::TraceId()
//----------------------------------------------------------------------------
void GRAFO::TraceId(const STRINGA& Msg, ID Id, int Livello ){
   #undef TRCRTN
   #define TRCRTN "\t"
   if(Livello > trchse)return;
   STRINGA Out;
   Out = Msg;
   Out += " Id= " ;
   Out += STRINGA(Id);
   STAZ_FS & Stz = THIS[Id];
   Out += " ";
   Out += Stz.Nome7();
   if(Out.Dim() % 8)Out.Pad(Out.Dim() - Out.Dim() % 8 + 8); // Piu' vicino multiplo di 8
   Out += STRINGA(" ") + Stazioni.DecodificaIdStazione(Id);
   if(Out.Dim() % 8)Out.Pad(Out.Dim() - Out.Dim() % 8 + 8); // Piu' vicino multiplo di 8
   if(Stazioni.RecordCorrente().TariffaRegione)
      Out += " TRg. "+STRINGA(Stazioni.RecordCorrente().TariffaRegione);
   if(Stazioni.RecordCorrente().Prima_Estensione)
      Out += " E1 "+STRINGA(Stazioni.RecordCorrente().Prima_Estensione);
   if(Stazioni.RecordCorrente().Seconda_Estensione)
      Out += " E2 "+STRINGA(Stazioni.RecordCorrente().Seconda_Estensione);
   if(Stazioni.RecordCorrente().Terza_Estensione)
      Out += " E3 "+STRINGA(Stazioni.RecordCorrente().Terza_Estensione);
   if(Stz.ProgRamo)
      Out += " PrRam "+STRINGA(Stz.ProgRamo);
   ERRSTRING(Out);
//<<< void GRAFO::TraceId const STRINGA& Msg, ID Id, int Livello
}
//----------------------------------------------------------------------------
// Grafo::DistanzaTra
//----------------------------------------------------------------------------
// Macro utile quando serve solo la distanza: torna BigDist  su errore
// Calcola in Particolare: Km Multistazioni e tutti i ProgKm sulle fermate
// NB: Su alcune tratte la distanza e' corretta, tramite apposito file KmECCZ.MIO .
DIST WIN_FASTCALL GRAFO::DistanzaTra(ID Da, ID A){
   #undef TRCRTN
   #define TRCRTN "GRAFO::DistanzaTra"
   // Vedo se e' una relazione eccezionale
   CORREZIONI_RAMI * Ecz = CorrezioniRami    ;
   for (int i = TotCorrezioniRami; i > 0 ; i--,Ecz ++ )if(Ecz->Id1 == Da && Ecz->Id2 == A) return  Ecz->KmDaUsare;
   if(Risolvi(Da,A))return THIS[Da].Distanza1;
//<<< if GRAFO::Grafo == this   // Solo la prima Istance e' registrata
   else return  BigDIST;
};
//----------------------------------------------------------------------------
// CalcolaDistanzaTra
//----------------------------------------------------------------------------
// Versione esportata della precedente
int GRAFO::CalcolaDistanzaTra(ID Da, ID A){
   #undef TRCRTN
   #define TRCRTN "CalcolaDistanzaTra"
   DIST Km = Grafo->DistanzaTra(Da,A);
   if(Km == BigDIST) return -1;
   return int(Km);
};
//----------------------------------------------------------------------------
// Grafo::Risolvi
//----------------------------------------------------------------------------
// Torna FALSE in caso di errore
// NON E' RIENTRANTE
//----------------------------------------------------------------------------
// NB: Introdotto il vincolo che tutti i rami DEBBONO seguire il
// percorso minimo: non piu' necessario gestire PrivilegiaRamoDiretto
// tranne per fp_tre che deve fare il controllo

// Questa e' generale a livello di grafo
BOOL GRAFO::RisolviGrafo(ID A){ // Tutti i percorsi minimi
   #undef TRCRTN
   #define TRCRTN "GRAFO::RisolviGrafo(A)"
   return Risolvi(0,A);
};

// Questa e' puntuale
BOOL GRAFO::Risolvi(ID Da, ID A, int MaxDist ){
   // Se imposto maxdist la soluzione e' valida per tutte le stazioni nodali con distanza <= MaxDist
   #undef TRCRTN
   #define TRCRTN "GRAFO::Risolvi(Da,A,MaxD)"

   if(++ ProgressivoCorrente  == 0){  // Reset di tutti i progressivi (la prima volta )
      ProgressivoCorrente = 1;
      // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
      SCAN_NUM_WIN(StazioniGrafo,TotStazioniGrafo,Staz,STAZ_FS) {
         Staz->Progressivo = 0;
      } ENDSCAN ;
   };

   #ifdef DBG1
   TRACESTRING( VRS(Da) + VRS(A) + VRS(MaxDist) );
   TRACEVLONG(GRAFO::Grafo->ProgressivoCorrente);
   //TRACEVLONG(PrivilegiaRamoDiretto);
   #endif

   STAZ_FS & sDa = THIS[Da];
   STAZ_FS & sA  = THIS[A ];
   sA.Distanza1=0;
   sA.Grf.NumCum =0;
   sA.Grf.SumId  =0;
   sA.Nodo1      =0;
   sA.Progressivo  = ProgressivoCorrente;
   sDa.Nodo1     =0;

   // -----------------------------------------------------
   // Gestione rapida ramo diretto
   // -----------------------------------------------------
   if (PrivilegiaRamoDiretto) {
      ID IdRamo;
      int Km;
      if (IdentificaRamo(Da,A,IdRamo,Km)){
         sDa.Nodo1       = A;
         sDa.Distanza1   = abs(Km);
         sDa.Grf.NumCum  = Rami[IdRamo].Concesso;
         sDa.Grf.SumId   = Da + A;
         sDa.Progressivo = ProgressivoCorrente;
         #ifdef DBG1
         TRACESTRING("Sono entrambi nodi dello stesso ramo Distanza = "+STRINGA(sDa.Distanza1)+" Concesso = "+STRINGA(sDa.Grf.NumCum));
         #endif
         return TRUE;
      } /* endif */
   } /* endif */

   PROFILER::Cronometra(16,TRUE);
   // -----------------------------------------------------
   // Variabili di algoritmo
   // -----------------------------------------------------
   Attivi.Clear();
   AttiviNextStep.Clear();
   WORD  NextDistanza ;
   BYTE  NextNumCum   ;
   ULONG NextSumId    ;
   Limite = MaxDist <= 0 ? BigDIST : MaxDist;
   BYTE AbilitazioniAttive = sA.Abilitazione | sDa.Abilitazione;
   if (AbilitazioniAttive == (STAZ_FS::SICILIA | STAZ_FS::SARDEGNA)) {
      // Per andare da Sicilia a Sardegna debbo passare per il continente
      AbilitazioniAttive |= STAZ_FS::CONTINENTE;
   } /* endif */
   ID StopId;
   #ifdef DBG1A
   TRACEVLONG(AbilitazioniAttive);
   TRACEVLONG(AbilitaCumulativo);
   #endif

   DIST ExtraDist = 0;
   // Inizializzazione destinazione
   if (sA.IsNodo()) {
      Attivi += A;
   } else {
      RAMO & Ramo = Rami[sA.IdRamo];
      STAZ_FS & S1 = StazioniGrafo[Ramo.IdStaz1];
      STAZ_FS & S2 = StazioniGrafo[Ramo.IdStaz2];
      Attivi += S1.Id;
      S1.Nodo1        = A;
      S1.Distanza1    = sA.Km1;
      ExtraDist = S1.Distanza1;
      S1.Grf.NumCum   = Ramo.Concesso;
      S1.Grf.SumId    = A;
      S1.Progressivo  = ProgressivoCorrente;
      if(S2.IsNodo()){
         Attivi += Ramo.IdStaz2;
         S2.Nodo1       = A;
         S2.Distanza1   = Ramo.KmRamo - sA.Km1;
         Top(ExtraDist,S2.Distanza1);
         S2.Grf.NumCum  = Ramo.Concesso;
         S2.Grf.SumId   = A;
         S2.Progressivo = ProgressivoCorrente;
      }
//<<< if  sA.IsNodo
   }
   #ifdef DBG1A
   Attivi.Trace(THIS,"Nodi attivi all' inizio: ");
   #endif
   // Inizializzazione origine
   if(Da == 0 || sDa.IsNodo()){
      StopId = Da;
   } else {
      DIST ExtraDist2 = 0;
      RAMO & Ramo = Rami[sDa.IdRamo];
      // STAZ_FS & S1 = StazioniGrafo[Ramo.IdStaz1];
      ExtraDist2 = sDa.Km1;
      STAZ_FS & S2 = StazioniGrafo[Ramo.IdStaz2];
      if(S2.IsNodo())Top(ExtraDist2,(Ramo.KmRamo - sDa.Km1));
      ExtraDist += ExtraDist2;
      // Se sono entrambi nodi la stazione di Stop e' la piu' vicina all' origine
      if (!S2.IsNodo() || sDa.Km1 <= (Ramo.KmRamo /2)) {
         StopId = Ramo.IdStaz1;
      } else {
         StopId = Ramo.IdStaz2;
      } /* endif */
   }
   #ifdef DBG1A
   TRACEVLONG(StopId);
   #endif


   // -----------------------------------------------------
   // Risoluzione del problema del percorso minimo
   // -----------------------------------------------------
   while(Attivi.Dim()){
      // Per tutti i nodi attivi
      #ifdef DBG1A
      TRACESTRING(" ============================== Scan Stazioni Attive ======================");
      #endif
      SCAN(Attivi,IdNodoAttivo,ID) { // Per tutti i nodi attivi
         STAZ_FS & NodoAttivo = StazioniGrafo[IdNodoAttivo];    // Nodo attivo attuale
         #ifdef DBG1A
         TRACESTRING("Nodo Attivo: "+STRINGA(IdNodoAttivo)+" Nodo1 : "+STRINGA(NodoAttivo.Nodo1)+" Km "+STRINGA(NodoAttivo.Distanza1) +"  NumCum = "+ STRINGA(NodoAttivo.Grf.NumCum) +"  SumId = "+ STRINGA(NodoAttivo.Grf.SumId) );
         #endif
         if(NodoAttivo.Distanza1 > Limite)continue;     // Velocizza per nodi molto vicini (Attenzione: se eguale si deve continuare per testare le societa' concesse)
         if(!(NodoAttivo.Abilitazione & AbilitazioniAttive))continue; // Per gestire le zone geografiche

         //  Per tutti i primi vicini del nodo attivo
         STAZ_FS::PRIMO_V * PrimiVic = NodoAttivo.PrimiVicini();
         NextSumId      = NodoAttivo.Grf.SumId  + IdNodoAttivo;
         // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
         SCAN_NUM_WIN(PrimiVic,NodoAttivo.NumeroPrimiVicini,PrimoVicinoCorrente,STAZ_FS::PRIMO_V){
            #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
            PROFILER::Conta(24);
            #endif
            #ifdef NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
            if(PrimoVicinoCorrente->Concesso)continue;
            #else
            if(PrimoVicinoCorrente->Concesso){
               if(!AbilitaCumulativo)continue;
            } else {
               if(AbilitaCumulativo == 99)continue; // Solo cumulativi
            }
            #endif
            NextDistanza   = PrimoVicinoCorrente->Distanza + NodoAttivo.Distanza1;
            NextNumCum     = NodoAttivo.Grf.NumCum + PrimoVicinoCorrente->Concesso;
            if(NextDistanza <= Limite) {   // Altrimenti non e' sicuramente sul cammino minimo (Attenzione: se eguale si deve continuare per testare le societa' concesse)
               STAZ_FS & NodoPrimoVicinoCorrente  = StazioniGrafo[PrimoVicinoCorrente->Id];
               #ifdef DBG1B
               TRACESTRING("TRY "+STRINGA(PrimoVicinoCorrente->Id)+" Dist = "+STRINGA(NextDistanza) +" NumCum= "+STRINGA(NextNumCum) + " SumId = "+STRINGA(NextSumId)
                  +" Vs. CorrDist = "+ STRINGA(NodoPrimoVicinoCorrente.Distanza1) +"  CorrCum = "+ STRINGA(NodoPrimoVicinoCorrente.Grf.NumCum) +"  CorrSumId = "+ STRINGA(NodoPrimoVicinoCorrente.Grf.SumId) );
               #endif
               // Se una delle distanze > distanza partendo dal nodo corrente sostituisco
               // NB : A parita' di distanza preferisco comunque privilegiare sempre un cammino per
               // dare stabilita' all' algoritmo ed evitare potenziali errori di tariffazione.
               // A tal fine privilegio nell' ordine
               // - I percorsi con meno tratte cumulative
               // - I percorsi con minore somma degli ID
               if(NodoPrimoVicinoCorrente.Progressivo != ProgressivoCorrente ||  // Nodo non ancora toccato
                  NodoPrimoVicinoCorrente.Distanza1 > NextDistanza ||
                  ( NodoPrimoVicinoCorrente.Distanza1 == NextDistanza &&
                                         (NodoPrimoVicinoCorrente.Grf.NumCum > NextNumCum ||
                                            (NodoPrimoVicinoCorrente.Grf.NumCum == NextNumCum && NodoPrimoVicinoCorrente.Grf.SumId > NextSumId
                                            )))){
                  #ifdef DBG1A
                  TRACESTRING("Attivazione di "+STRINGA(PrimoVicinoCorrente->Id)+" Dist = "+STRINGA(NextDistanza) +" NumCum= "+STRINGA(NextNumCum) +" SumId= "+STRINGA(NextSumId));
                  #endif
                  NodoPrimoVicinoCorrente.Distanza1    = NextDistanza;
                  NodoPrimoVicinoCorrente.Grf.NumCum   = NextNumCum;
                  NodoPrimoVicinoCorrente.Grf.SumId    = NextSumId;
                  NodoPrimoVicinoCorrente.Nodo1        = NodoAttivo.Id;
                  NodoPrimoVicinoCorrente.Progressivo  = ProgressivoCorrente;

                  if(PrimoVicinoCorrente->Id == StopId && MaxDist <= 0 ){
                                         Limite = NextDistanza + ExtraDist;
                                         #ifdef DBG1A
                                         TRACEVLONG(Limite);
                                         #endif
                  };
                  AttiviNextStep += PrimoVicinoCorrente->Id; // Debbo rendere attivo anche lo STOP-ID per evitare problemi nella determinazione delle stazioni vincolanti quando la stazione terminale non e' nodale (ed ExtraDist > 0)
//<<<          if NodoPrimoVicinoCorrente.Progressivo != ProgressivoCorrente ||  // Nodo non ancora toccato
               } /* endif */
//<<<       if NextDistanza <= Limite      // Altrimenti non e' sicuramente sul cammino minimo  Attenzione: se eguale si deve continuare per testare le societa' concesse
            }
//<<<    SCAN_NUM PrimiVic,NodoAttivo.NumeroPrimiVicini,PrimoVicinoCorrente,STAZ_FS::PRIMO_V
         } ENDSCAN ;
//<<< SCAN Attivi,IdNodoAttivo,ID    // Per tutti i nodi attivi
      } ENDSCAN ;          //  Per tutti i primi vicini del nodo attivo

      // Swap dei nodi attivi <-> Futuri
      ARRAY_ID::Swap(Attivi,AttiviNextStep);
      AttiviNextStep.Clear();

//<<< while Attivi.Dim
   }   // Per tutti i nodi attivi


   // -----------------------------------------------------
   // Gestione eventuale stazione iniziale NON nodale
   // -----------------------------------------------------
   if(Da)EstendiRisolvi(Da);

   PROFILER::Cronometra(16,FALSE);

   if (sDa.Progressivo == ProgressivoCorrente) {
      #ifdef DBG1
      TRACELONG("Distanza finale da origine a destinazione = ",sDa.Distanza1);
      #endif
      return TRUE;
   } else {
      #ifdef DBG1
      TRACESTRING("Non riuscito a collegare "+STRINGA(Da) + " e "+STRINGA(A));
      #endif
      return FALSE;
   } /* endif */

//<<< BOOL GRAFO::Risolvi ID Da, ID A, int MaxDist
};
//----------------------------------------------------------------------------
// GRAFO::EstendiRisolvi
//----------------------------------------------------------------------------
void WIN_FASTCALL GRAFO::EstendiRisolvi(ID Da){
   #undef TRCRTN
   #define TRCRTN "GRAFO::EstendiRisolvi"
   STAZ_FS & sDa = THIS[Da];
   if(sDa.Progressivo == ProgressivoCorrente)return;
   if (sDa.IsNodo()) return;

   // Debbo scegliere il minore percorso
   RAMO & Ramo = Rami[sDa.IdRamo];
   STAZ_FS & S1 = StazioniGrafo[Ramo.IdStaz1];
   STAZ_FS & S2 = StazioniGrafo[Ramo.IdStaz2];
   BOOL ScelgoS1;
   if( S1.Progressivo != ProgressivoCorrente ){           // S1 proprio da non considerare
      if( S2.Progressivo != ProgressivoCorrente ) return; // anche S2 proprio da non considerare
      ScelgoS1 = FALSE;
   } else if( S2.Progressivo != ProgressivoCorrente ){    // S2 proprio da non considerare
      ScelgoS1 = TRUE;
   } else { // Debbo considerare entrambi
      ScelgoS1 = !S2.IsNodo() || (S1.Distanza1 + sDa.Km1) < (S2.Distanza1 + Ramo.KmRamo - sDa.Km1);
      if(!ScelgoS1 && ((S1.Distanza1 + sDa.Km1) == (S2.Distanza1 + Ramo.KmRamo - sDa.Km1))){
         // A parita' di distanza scelgo il percorso con meno ferrovie concesse
         ScelgoS1 |= S1.Grf.NumCum < S2.Grf.NumCum;
         if(S1.Grf.NumCum == S2.Grf.NumCum){ // Ed infine quello con la minore somma degli ID
            ScelgoS1 |= S1.Grf.SumId < S2.Grf.SumId;
         }
      }
   }

   if (ScelgoS1){
      sDa.Nodo1 = S1.Id;
      sDa.Distanza1  = S1.Distanza1 + sDa.Km1;
      sDa.Grf.NumCum = S1.Grf.NumCum + Ramo.Concesso;
      sDa.Grf.SumId  = S1.Grf.SumId  + S1.Id;
      sDa.Progressivo = ProgressivoCorrente;
   } else {
      sDa.Nodo1 = S2.Id;
      sDa.Distanza1= S2.Distanza1 +Ramo.KmRamo - sDa.Km1;
      sDa.Grf.NumCum = S2.Grf.NumCum + Ramo.Concesso;
      sDa.Grf.SumId   = S2.Grf.SumId + S2.Id;
      sDa.Progressivo = ProgressivoCorrente;
   }
//<<< void __fastcall GRAFO::EstendiRisolvi ID Da
} /* endif */
//----------------------------------------------------------------------------
// Grafo::RisolviPolimetriche
//----------------------------------------------------------------------------
// Il metodo risolve il problema del percorso minimo utilizzando le
// polimetriche.
//----------------------------------------------------------------------------
BOOL GRAFO::RisolviPolimetriche(ID Da, ID A, ARRAY_ID & FerrovieConcesseCompatibili){
   #undef TRCRTN
   #define TRCRTN "GRAFO::RisolviPolimetriche"

   static ARRAY_ID Wrk(64); // static per performance
   static ARRAY_ID PolimOrigine(64); // static per Performance
   #ifdef DBGP
   TRACESTRING(VRS(Da) + VRS(A));
   TRACESTRING("FerrovieConcesseCompatibili="+FerrovieConcesseCompatibili.ToStringa());
   #endif

   int NumStep = 0;

   if(++ ProgressivoCorrente  == 0){  // Reset di tutti i progressivi (la prima volta )
      ProgressivoCorrente = 1;
      // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
      SCAN_NUM_WIN(StazioniGrafo,TotStazioniGrafo,Staz,STAZ_FS){
         Staz->Progressivo = 0;
      } ENDSCAN ;
   };
   STAZ_FS & sDa = THIS[Da];
   STAZ_FS & sA  = THIS[A ];

   sDa.Nodo1  =0;
   sDa.Plm.Poli1  =0;
   sDa.Plm.Verso  =0;
   sDa.Plm.NumStep=0;
   sA.Distanza1=0;
   sA.Nodo1  =0;
   sA.Plm.Poli1  =0;
   sA.Plm.Verso  =0;
   sA.Plm.NumStep=0;
   sA.Progressivo  = ProgressivoCorrente;


   if(!sA.StazioneFS && !sA.DiTransito && !sDa.StazioneFS && !sDa.DiTransito ){  // Debbo avere una stazione FS coinvolta
      sDa.SetPolimetricheNoDiramaz(Wrk, FerrovieConcesseCompatibili);
      sA.AndPolimetriche(Wrk);
      if (Wrk.Dim()) {
         TRACESTRING("Collegamento diretto via polimetriche di due stazioni del servizio cumulativo");
         TRACEID(Da);
         TRACEID(A);
         sDa.Nodo1       = A;
         sDa.Plm.Poli1   = Wrk[0];
         sDa.Distanza1   = 9999;
         sDa.Progressivo = ProgressivoCorrente;
         return TRUE;
      } else {
         ERRSTRING("ERRORE:Non posso collegare direttamente via polimetriche due stazioni del servizio cumulativo");
         TRACEID_L(1,Da);
         TRACEID_L(1,A);
         return FALSE;
      } /* endif */
   };

   #if defined(DBG1P) && defined(SI_ZONE)
   {
      ARRAY_ID PolimDa(64);
      ARRAY_ID PolimA(64);
      sDa.SetPolimetriche(PolimDa, FerrovieConcesseCompatibili);
      sA.SetPolimetriche(PolimA, FerrovieConcesseCompatibili);
      TRACEVSTRING2(PolimDa.ToStringa());
      TRACEVSTRING2(PolimA.ToStringa());
      WORD ZoneDa= 0,ZoneA = 0;
      ORD_FORALL(PolimDa,iDa){
         POLIMETRICA & Pol = Polim[PolimDa[iDa]];
         if (!(Pol.TipoPoli == POLIMETRICA::ALLACCIAMENTO)) {
            ZoneDa |= Pol.Zone;
         } /* endif */
      }
      ORD_FORALL(PolimA,iA){
         POLIMETRICA & Pol = Polim[PolimA[iA]];
         if (!(Pol.TipoPoli == POLIMETRICA::ALLACCIAMENTO)) {
            ZoneA |= Pol.Zone;
         } /* endif */
      }
      TRACEVPOINTER(ZoneDa);
      TRACEVPOINTER(ZoneA);
//<<< #if defined DBG1P  && defined SI_ZONE
   }
   #endif


   Attivi.Clear();
   AttiviNextStep.Clear();

   WORD NextDistanza ;
   Limite = BigDIST;
   BYTE AbilitazioniAttive = sA.Abilitazione | sDa.Abilitazione;
   if (AbilitazioniAttive == (STAZ_FS::SICILIA | STAZ_FS::SARDEGNA)) {
      // Per andare da Sicilia a Sardegna debbo passare per il continente
      AbilitazioniAttive |= STAZ_FS::CONTINENTE;
   } /* endif */
   WORD ZoneAttive=0; // Tutte le zone delle polimetriche dell' origine o della destinazione

   #ifndef SI_ZONE
   ZoneAttive = 0xffff;
   #else
   sA.SetPolimetriche(Wrk, FerrovieConcesseCompatibili);
   sDa.OrPolimetriche(Wrk);
   ORD_FORALL(Wrk,i1){
      POLIMETRICA & Pol = Polim[Wrk[i1]];
      if (!(Pol.TipoPoli == POLIMETRICA::ALLACCIAMENTO)) {
         ZoneAttive |= Pol.Zone;
      } /* endif */
   }
   #endif

   #ifdef DBG1P
   TRACEVPOINTER(ZoneAttive);
   TRACEVLONG(AbilitazioniAttive);
   #endif

   Attivi += A;

   if(Si_Diramazion){
      sDa.SetPolimetriche(PolimOrigine, FerrovieConcesseCompatibili);
   } else {
      sDa.SetPolimetricheNoDiramaz(PolimOrigine, FerrovieConcesseCompatibili);
   }

   // -----------------------------------------------------
   // Risoluzione del problema del percorso minimo
   // -----------------------------------------------------
   while(Attivi.Dim()){
      NumStep ++;
      #ifdef DBG1P
      TRACESTRING(" ============================== Scan Stazioni Attive ======================");
      #endif
      // Per tutte le stazioni attive
      SCAN(Attivi,IdNodoAttivo,ID) { // Per tutti i nodi attivi
         STAZ_FS & NodoAttivo = StazioniGrafo[IdNodoAttivo];    // Nodo attivo attuale

         #ifdef DBG1P
         if(Si_Diramazion){
            NodoAttivo.SetPolimetriche(Wrk, FerrovieConcesseCompatibili,ZoneAttive);
         } else {
            NodoAttivo.SetPolimetricheNoDiramaz(Wrk, FerrovieConcesseCompatibili, ZoneAttive);
         }
         TRACESTRING(" === > Stazione attiva "+STRINGA(IdNodoAttivo)+
            " Abilitazione "+STRINGA((void*)NodoAttivo.Abilitazione)+
            " Distanza1 "+STRINGA(NodoAttivo.Distanza1)+
            " su polimetrica "+STRINGA(NodoAttivo.Plm.Poli1)+
            " Tipo "+TipiPoli[Polim[NodoAttivo.Plm.Poli1].TipoPoli]+
            " \nPolimetriche collegate per le zone attive"+Wrk.ToStringa());
         #endif

         if(NodoAttivo.Distanza1 > Limite)continue;   // (Attenzione: se eguale si deve continuare per ulteriori test)
         if(!(NodoAttivo.Abilitazione & AbilitazioniAttive))continue; // Per gestire le zone geografiche

         if(Si_Diramazion){
            NodoAttivo.SetPolimetriche(Wrk, FerrovieConcesseCompatibili,ZoneAttive);
         } else {
            NodoAttivo.SetPolimetricheNoDiramaz(Wrk, FerrovieConcesseCompatibili,ZoneAttive);
         }

         // Identifico la posizione del nodo attivo sulla polimetrica
         // con cui sono arrivato
         // int PosNdA = NodoAttivo.HaPolimetrica(NodoAttivo.Plm.Poli1);

         // Identifico l' eventuale inibizione per diramazione condizionata
         BOOL PossibileDiramazioneCondizionata = TRUE;
         ISTR_COND & Origine  = StazioniGrafo[NodoAttivo.Nodo1].IstrCond(); // NB: Per la destinazione e' VUOTO
         ISTR_COND MaskDiramazioneCondizionata;
         #ifdef NO_CONDIZIONAT
         Origine.Clear(); // Inibisce il tutto
         #endif
         if(!NodoAttivo.DiDiramazione || Origine.Empty() ){
            PossibileDiramazioneCondizionata = FALSE;
         } else {
            MaskDiramazioneCondizionata = Origine ;
            MaskDiramazioneCondizionata &= NodoAttivo.Diramazione().IstrCondInibit();
         };

         // Per tutte le polimetriche del nodo attivo appartenenti alle corrette zone

         #ifdef DBG1P
         TRACESTRING(" =====  Scan polimetriche relative ");
         #endif

         ORD_FORALL(Wrk,j){

            POLIMETRICA & Pol = Polim[Wrk[j]];
            #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
            PROFILER::Conta(25);
            #endif

            // Non e' accettabile sommare due tratti sulla stessa polimetrica
            if(Pol.IdPoli == NodoAttivo.Plm.Poli1)continue;

            // Se la polimetrica e' solo locale e' accettabile se
            // 1: La stazione origine NON e' di diramazione ed appartiene alla polimetrica
            // 2: La stazione destinazione NON e' di diramazione ed appartiene alla polimetrica
            // 3: Sia Origine che destinazione sono di diramazione ed appartengono alla polimetrica
            if (Pol.TipoPoli == POLIMETRICA::SOLO_LOCALE) {
               if (!sDa.DiDiramazione) {
                  if(sDa.HaPolimetrica(Wrk[j])== -1){
                     if(sA.DiDiramazione || sA.HaPolimetrica(Wrk[j])== -1) continue;
                  }
               } else if (!sA.DiDiramazione) {
                  if(sA.HaPolimetrica(Wrk[j])== -1)continue;
               } else {
                  if(sDa.HaPolimetrica(Wrk[j])== -1)continue;
                  if(sA.HaPolimetrica(Wrk[j])== -1)continue;
               } /* endif */
            } /* endif */

            #ifdef DBG1P
            TRACELONG("  === > Test Polimetrica , tipo = "+STRINGA(Pol.TipoPoli)+" Id = ",Pol.IdPoli);
            #endif

            // Verifico se la polimetrica e' una di quelle che contiene l' origine
            // Il test e' necessario perche' il ciclo succcessivo e' esteso alle sole
            // stazioni di DIRAMAZIONE
            if (PolimOrigine.Contiene(Pol.IdPoli)) {
               #ifdef DBG1P
               TRACESTRING("La polimetrica contiene l' origine");
               #endif

               // Identifico l' eventuale inibizione per diramazione condizionata
               if(PossibileDiramazioneCondizionata &&
                  MaskDiramazioneCondizionata.Inibizione(sDa.IstrCond())){
                  #ifdef DBG1P
                  TRACESTRING("Esclusa origine per check di diramazione condizionata");
                  #endif
               } else {

                  BYTE Verso;                   // Variabile di lavoro
                  WORD Dist = Pol.DistanzaTra(Da,IdNodoAttivo,Verso)+ NodoAttivo.Distanza1;
                  // Scelta Polimetrica: a parita' di KM
                  // preferisco meno polimetriche oppure le polimetriche "piu' nobili"
                  if(sDa.Progressivo != ProgressivoCorrente ||
                     Dist < sDa.Distanza1 ||
                     (Dist == sDa.Distanza1 && sDa.Plm.NumStep == NumStep && Pol.PreferibileA(Polim[sDa.Plm.Poli1]))){
                     // TRACEVLONG(Dist);
                     // TRACEVLONG(sDa.Distanza1);
                     // TRACEVLONG(Pol.KmPoli);
                     // TRACEVLONG(Polim[sDa.Plm.Poli1].KmPoli);
                     sDa.Distanza1 = Dist;
                     sDa.Nodo1     = IdNodoAttivo;
                     sDa.Plm.Poli1     = Pol.IdPoli;
                     sDa.Progressivo = ProgressivoCorrente;
                     sDa.Plm.Verso       = Verso;
                     sDa.Plm.NumStep     = NumStep ;
                     Limite = sDa.Distanza1;
                     #ifdef DBG1P
                     TRACELONG("Attivata origine su polimetrica con ID = "+STRINGA(Pol.IdPoli)+" Distanza = ",Dist);
                     TRACEVLONG(Pol.KmPoli);
                     TRACEVLONG(Polim[sDa.Plm.Poli1].KmPoli);
                     #endif
                  }
//<<<          if PossibileDiramazioneCondizionata &&
               }
//<<<       if  PolimOrigine.Contiene Pol.IdPoli
            }

            // Per tutte le stazioni di diramazione della polimetrica.
            POLIMETRICA::PO_DIRA * Sd = Pol.StazioniDiramazione();
            // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
            SCAN_NUM_WIN(Sd,Pol.NumStazDiram,SdCorrente,POLIMETRICA::PO_DIRA){
               #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
               PROFILER::Conta(25);
               #endif
               ID Id = SdCorrente->Id;
               if(Id == NodoAttivo.Id)continue;
               STAZ_FS & PrimV  = StazioniGrafo[Id];

               // Identifico l' eventuale inibizione per diramazione condizionata
               if(PossibileDiramazioneCondizionata &&
                  MaskDiramazioneCondizionata.Inibizione(PrimV.IstrCond())){
                  #ifdef DBG1P
                  TRACESTRING("Esclusa stazione "+ STRINGA(Id)+ " per check di diramazione condizionata");
                  #endif
                  continue;
               }

               // Questa logica pare NON sia vera: si veda Formia - Mercato S. Severino
               //// Escludo le stazioni appartenenti alla polimetrica con cui
               //// sono arrivato al NodoAttivo (se hanno lo stesso verso)
               //// perche' in tal caso avrei potuto proseguire direttamente.
               //// ^ = XOR
               //int IdxPv = PrimV.HaPolimetrica(NodoAttivo.Plm.Poli1);
               ////TRACEVLONG(IdxPv);
               //if( IdxPv >= 0 && (NodoAttivo.Plm.Verso ^ Bool(IdxPv > PosNdA))){
               //   #ifdef DBG1P
               //   TRACESTRING("Esclusa stazione "+ STRINGA(Id)+ " perche' appartenente a polimetrica di arrivo "+ STRINGA(NodoAttivo.Plm.Poli1) );
               //   #endif
               //   continue;
               //}

               BYTE Verso;                   // Variabile di lavoro
               NextDistanza = Pol.DistanzaTra(Id,IdNodoAttivo,Verso) +NodoAttivo.Distanza1;
               #ifdef DBG1P
               TRACESTRING("Possibile Staz Diram Id = "+STRINGA(PrimV.Id)+ "   Distanza = "+STRINGA(NextDistanza) + " Vs. "+STRINGA(PrimV.Distanza1) );
               #endif
               if(NextDistanza <= Limite){    // Altrimenti non e' sicuramente sul cammino minimo (Attenzione: se eguale si deve continuare per ulteriori test)

                  // -------------------------------------------------
                  // Qui introduco il concetto che a parita' di KM
                  // preferisco usare meno polimetriche oppure polimetriche piu' "nobili"
                  // -------------------------------------------------
                  if(PrimV.Progressivo != ProgressivoCorrente ||  // Nodo non ancora toccato
                                         PrimV.Distanza1 > NextDistanza ||
                                         (PrimV.Distanza1 == NextDistanza && PrimV.Plm.NumStep == NumStep &&
                                            Pol.PreferibileA(Polim[PrimV.Plm.Poli1]))
                  ){
                                         #ifdef DBG1P
                                         TRACESTRING("Attivazione Staz Diram Id = "+STRINGA(PrimV.Id)+ "   Distanza = "+STRINGA(NextDistanza));
                                         #endif
                                         PrimV.Distanza1    = NextDistanza;
                                         PrimV.Nodo1        = IdNodoAttivo;
                                         PrimV.Plm.Poli1    = Pol.IdPoli;
                                         PrimV.Progressivo  = ProgressivoCorrente;
                                         PrimV.Plm.Verso    = Verso;
                                         PrimV.Plm.NumStep  = NumStep;
                                         if (Id == Da) {   // Sono arrivato all' origine (che e' stazione di diramazione)
                                            Limite = sDa.Distanza1;
                                         } else {
                                            if(!AttiviNextStep.Contiene(PrimV.Id)){
                                               AttiviNextStep += PrimV.Id;
                                            }
                                         } /* endif */
//<<<             if PrimV.Progressivo != ProgressivoCorrente ||  // Nodo non ancora toccato
                  } /* endif */
//<<<          if NextDistanza <= Limite      // Altrimenti non e' sicuramente sul cammino minimo  Attenzione: se eguale si deve continuare per ulteriori test
               };
//<<<       SCAN_NUM Sd,Pol.NumStazDiram,SdCorrente,POLIMETRICA::PO_DIRA
            } ENDSCAN ;

//<<<    ORD_FORALL Wrk,j
         }
//<<< SCAN Attivi,IdNodoAttivo,ID    // Per tutti i nodi attivi
      }  ENDSCAN ;             // Per tutti i nodi attivi

      // Swap dei nodi attivi <-> Futuri
      ARRAY_ID::Swap(Attivi,AttiviNextStep);
      AttiviNextStep.Clear();

//<<< while Attivi.Dim
   }
   // Trace dettaglio soluzione trovata
   #if defined(DBG1P)
   TRACESTRING("========================================================");
   TRACESTRING("Soluzione del sottoproblema da "+STRINGA(Da)+" a "+STRINGA(A)+ " Km "+STRINGA(sDa.Distanza1));
   TRACESTRING("========================================================");
   #endif
   #if defined(DBG1P) || defined(DBGP)
   ID St = Da;
   int i = MAX_TRATTE_POLIMETRICA;
   while (St != A) {
      STAZ_FS & Stz = THIS[St];
      TRACESTRING(
         STRINGA(Stz.Distanza1 - THIS[Stz.Nodo1].Distanza1) +
         STRINGA(" Km da stazione ") +
         STRINGA(St)+ " " +
         STRINGA(Stazioni.DecodificaIdStazione(St)).Strip()+
         STRINGA(" su polimetrica ID= ") +
         STRINGA(Stz.Plm.Poli1)+
         " Nø "+
         STRINGA((*FilePolim)[Stz.Plm.Poli1].Nome)+ " " +
         STRINGA(FilePolim->RecordCorrente().Descrizione).Strip()
      );
      St = Stz.Nodo1;
      if(i-- < 0 ){    // Assicurazione antiloop
         BEEP;
         break;
      }
   } /* endwhile */
   #endif
   #if defined(DBG1P)
   TRACESTRING("Stazione finale "+ STRINGA(St)+" "+Stazioni.DecodificaIdStazione(St));
   TRACESTRING("========================================================");
   #endif

   if (sDa.Progressivo == ProgressivoCorrente) {
      #ifdef DBGP
      TRACELONG("Distanza finale da origine a destinazione = ",sDa.Distanza1);
      #endif
      return TRUE;
   } else {
      #ifdef DBGP
      ERRSTRING("Non riuscito a collegare "+STRINGA(Da) + " e "+STRINGA(A));
      #endif
      return FALSE;
   } /* endif */
//<<< BOOL GRAFO::RisolviPolimetriche ID Da, ID A, ARRAY_ID & FerrovieConcesseCompatibili
};

//----------------------------------------------------------------------------
// BOOL GRAFO::SulMinimumPath
//----------------------------------------------------------------------------
BOOL GRAFO::SulMinimumPath(ID x,ID Da, ID A){
   #undef TRCRTN
   #define TRCRTN "GRAFO::SulMinimumPath"
   if(x == Da || x == A )return TRUE;
   ID Stop,Stop2;
   if(!IsNodo(x)){
      RAMO & Ramo = Rami[THIS[x].IdRamo];
      Stop  = Ramo.IdStaz1;
      Stop2 = Ramo.IdStaz2;
      if(Stop2 == x) return FALSE; // E' la stazione terminale
      // Non controllo Sa Da ed A sono nodi perche' in tal caso IdRamo == 0 e tutto e' Ok.
      if(THIS[Da].IdRamo == THIS[x].IdRamo){
         // Appartengono allo stesso ramo
         if(THIS[A].IdRamo == THIS[x].IdRamo){
            // Tutti e tre
            return ( THIS[A].ProgRamo >= THIS[x].ProgRamo &&
               THIS[Da].ProgRamo <= THIS[x].ProgRamo) ||
            (THIS[A].ProgRamo <= THIS[x].ProgRamo &&
               THIS[Da].ProgRamo >= THIS[x].ProgRamo);
         }
         if ( THIS[Da].ProgRamo <= THIS[x].ProgRamo){
            return SulMinimumPath(Ramo.IdStaz2,Da,A);
         } else {
            return SulMinimumPath(Ramo.IdStaz1,Da,A);
         }
      }else if(THIS[A].IdRamo == THIS[x].IdRamo){
         // Appartengono allo stesso ramo
         if ( THIS[A].ProgRamo <= THIS[x].ProgRamo){
            return SulMinimumPath(Ramo.IdStaz2,Da,A);
         } else {
            return SulMinimumPath(Ramo.IdStaz1,Da,A);
         }
//<<< if THIS Da .IdRamo == THIS x .IdRamo
      } else if (!IsNodo(Stop2)){ // Sono su di un ramo terminale
         return FALSE;
      }
//<<< if !IsNodo x
   } else {
      Stop = x;
   };
   if(!Risolvi(Da,A))return FALSE;
   int Loop = 3000;
   ID Last = 0;
   for (ID Id = Da; Id != A && Loop > 0; Id = THIS[Id].Nodo1) {
      if(Id == Stop){
         if(IsNodo(x) || (Last == Stop2 ) || (THIS[Id].Nodo1 == Stop2)){
            return TRUE;
         }
         break;
      }
      Last = Id;
      Loop --;
   } /* endfor */
   if(Loop <= 0)BEEP;
   return FALSE;
//<<< BOOL GRAFO::SulMinimumPath ID x,ID Da, ID A
};
//----------------------------------------------------------------------------
// GRAFO::CodiceTariffaRegionale
//----------------------------------------------------------------------------
// Per abbonamenti regionali
//   0    = Applicare tariffa nazionale
//   0xFF = NON SO (disabilitare vendita abbonamenti)
BYTE GRAFO::CodiceTariffaRegionale(PERCORSO_GRAFO & Percorso,BOOL Cumulativo){
   #undef TRCRTN
   #define TRCRTN "GRAFO::CodiceTariffaRegionale"

   TRACESTRING("Inizio");
   ARRAY_ID & Nodi = Percorso.Nodi;

   if (IstrEccez_REG != NULL) {
      if(IstrEccez_REG->Seek(Nodi[0])){
         for ( ;
            &IstrEccez_REG->RecordCorrente() && IstrEccez_REG->RecordCorrente().NodiStd[0] == Nodi[0];
            IstrEccez_REG->Next() ) {

            ISTRADAMENTO_ECCEZIONALE_REG & Iec = IstrEccez_REG->RecordCorrente();
            if(Nodi.Dim() != Iec.NumeroNodi) continue;
            BOOL Ok= TRUE;
            FORALL(Nodi,i){
               #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
               PROFILER::Conta(21);
               #endif
               if(Nodi[i] != IstrEccez_REG->RecordCorrente().NodiStd[i]){
                  Ok = FALSE;
                  break;
               }
            }
            if (Ok) {
               TRACESTRING("Instradamento eccezionale: Impostato direttamente il codice regione");
               return Iec.CodiceRegione;
            } /* endif */
         }
//<<< if IstrEccez_REG->Seek Nodi 0
      }
//<<< if  IstrEccez_REG != NULL
   } /* endif */

   //.............................................................
   // Primo ciclo: sulle stazioni nodali
   //.............................................................
   // NB: La funzione Terminal filtra via la terza estensione
   // (che in base agli accordi presi con il CVB e' di solo transito).
   QUAD_TAR Tariffe = THIS[Nodi[0]].Tariffe.Terminal();
   int PsNVal = Nodi.Dim() - 1; // Indice ultimo nodo valido
   Tariffe &= THIS[Nodi[PsNVal]].Tariffe.Terminal();
   // Il ciclo e' ripetuto due volte per motivi di performance
   // Infatti se a livello di nodi non ho tariffe regionali valide e'
   // inutile andare a livello delle stazioni di ramo
   #ifdef  DBG_TAR_REG
   TRACESTRING("Primo ciclo: su stazioni nodali: Tariffe = "
      + STRINGA(Tariffe.TariffaRegione    ) + " " + STRINGA(Tariffe.Prima_Estensione  ) + " "
      + STRINGA(Tariffe.Seconda_Estensione) + " " + STRINGA(Tariffe.Terza_Estensione  ) + VRS(Tariffe.NonValido));
   #endif
   for (int i = PsNVal; i > 0 && !(Tariffe.Empty() || Tariffe.NonValido >0 ) ;  i-- ) {
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(21);
      #endif
      STAZ_FS & Stazn = THIS[Nodi[i]];
      if(!Stazn.Vendibile){
         #ifdef  DBG_TAR_REG
         TraceId("Ignorata nodo non vendibile",Stazn.Id);
         #endif
      } else if(Stazn.Tariffe.TariffaRegione == 0 && Stazn.StazioneFS){ // Stazione anomala senza codice regione impostato (di solito: Smistamento)
         #ifdef  DBG_TAR_REG
         TraceId("Ignorato nodo anomalo senza codice regione impostato",Stazn.Id);
         #endif
      } else {
         #ifdef  DBG_TAR_REG
         TraceId("Nodo Toccato ",Stazn.Id);
         #endif
         Tariffe &= Stazn.Tariffe;
      }
   } /* endfor */

   //.............................................................
   // Secondo ciclo: sulle stazioni interne dei rami
   //.............................................................
   // Se ho una tariffa regionale valida eseguo un controllo piu' fino
   #ifdef  DBG_TAR_REG
   TRACESTRING("Secondo ciclo: su stazioni interne dei rami: Tariffe = "
      + STRINGA(Tariffe.TariffaRegione    ) + " " + STRINGA(Tariffe.Prima_Estensione  ) + " "
      + STRINGA(Tariffe.Seconda_Estensione) + " " + STRINGA(Tariffe.Terza_Estensione  ) + VRS(Tariffe.NonValido));
   #endif
   for (i = PsNVal; i > 0 && !(Tariffe.Empty() || Tariffe.NonValido >0 ) ;  i-- ) {
      STAZ_FS & Primo   = THIS[Nodi[i-1]];
      STAZ_FS & Secondo = THIS[Nodi[i]];
      ID IdRamo = 0;
      int Start, Stop , Step ;
      if (!Primo.IsNodo() ) {
         IdRamo = Primo.IdRamo;
      } else if (!Secondo.IsNodo() ) {
         IdRamo = Secondo.IdRamo;
      } else {
         STAZ_FS::PRIMO_V & Pv =  Primo.PvDaId(Secondo.Id);
         if(&Pv != NULL) IdRamo = Pv.IdRamo;
      }
      if(IdRamo == 0) {
         ERRSTRING("Caso anomalo: investigare");
         BEEP;
         continue;
      }
      RAMO & Ramo = Rami[IdRamo];
      if(Ramo.NumStazioni < 1)continue; // Non ha stazioni intermedie
      if (Primo.Id == Ramo.IdStaz1){
         Start = 0;
      } else if (Primo.Id == Ramo.IdStaz2){
         Start = Ramo.NumStazioni-1;
      } else {
         Start = Primo.ProgRamo-1;
      }
      if (Secondo.Id == Ramo.IdStaz1){
         Stop = 0;
      } else if (Secondo.Id == Ramo.IdStaz2){
         Stop = Ramo.NumStazioni-1;
      } else {
         Stop = Secondo.ProgRamo-1;
      }
      if(Stop >= Start){
         Step = 1;
      } else {
         Step = -1;
      }

      #ifdef  DBG_TAR_REG
      TRACESTRING("Controllo Stazioni interne del Ramo "+ STRINGA(IdRamo)+"  "+
         STRINGA(Ramo.IdStaz1)+"->"+STRINGA(Ramo.IdStaz2)+" Stazioni da Nø "+
         STRINGA(Start)+ " a Nø "+ STRINGA(Stop));
      TRACESTRING("Tariffe = "
         + STRINGA(Tariffe.TariffaRegione    ) + " " + STRINGA(Tariffe.Prima_Estensione  ) + " "
         + STRINGA(Tariffe.Seconda_Estensione) + " " + STRINGA(Tariffe.Terza_Estensione  ) + VRS(Tariffe.NonValido));
      #endif

      RAMO::STAZIONE_R * StzStop  = Ramo.StazioniDelRamo() + Stop;
      RAMO::STAZIONE_R * Stazione = Ramo.StazioniDelRamo() + Start;
      // Controllo tariffa regionale sui progressivi da Start a Stop del ramo IdRamo
      do {
         #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
         PROFILER::Conta(21);
         #endif
         STAZ_FS & Stazn = THIS[Stazione->Id];
         if(!Stazn.Vendibile){
            #ifdef  DBG_TAR_REG
            TraceId("Ignorata stazione non vendibile",Stazn.Id);
            #endif
         } else if(Stazn.Tariffe.TariffaRegione == 0 && Stazn.StazioneFS){ // Stazione anomala senza codice regione impostato (di solito: Smistamento)
            #ifdef  DBG_TAR_REG
            TraceId("Ignorata stazione anomala senza codice regione impostato",Stazn.Id);
            #endif
         } else {
            #ifdef  DBG_TAR_REG
            TraceId("Stazione Toccata ",Stazn.Id);
            #endif
            Tariffe &= Stazn.Tariffe;
         }
         if (Stazione == StzStop)break;
         Stazione += Step;
//<<< do
      } while (TRUE); /* enddo */
//<<< for  i = PsNVal; i > 0 && ! Tariffe.Empty   || Tariffe.NonValido >0   ;  i--
   } /* endfor */
   #ifdef  DBG_TAR_REG
   TRACESTRING("Fine: Tariffe = "
      + STRINGA(Tariffe.TariffaRegione    ) + " " + STRINGA(Tariffe.Prima_Estensione  ) + " "
      + STRINGA(Tariffe.Seconda_Estensione) + " " + STRINGA(Tariffe.Terza_Estensione  ) + VRS(Tariffe.NonValido));
   #endif
   BYTE TariffaRegionale = Tariffe.Identifica();
   // Controllo se origine e destinazione appartengono alla stessa regione
   // La regola non si applica per i percorsi cumulativi
   if(THIS[Nodi[0]].Tariffe.TariffaRegione == THIS[Nodi.Last()].Tariffe.TariffaRegione && !Cumulativo){
      #ifdef  DBG_TAR_REG
      TRACESTRING("Origine e destinazione appartengono alla stessa regione: Accetto tariffa regionale");
      #endif
      TariffaRegionale = THIS[Nodi[0]].Tariffe.TariffaRegione;
   }
   #ifdef  DBG_TAR_REG
   TRACEVLONG(TariffaRegionale);
   #endif
   return TariffaRegionale;
//<<< BYTE GRAFO::CodiceTariffaRegionale PERCORSO_GRAFO & Percorso,BOOL Cumulativo
};

//----------------------------------------------------------------------------
// BOOL GRAFO::IdentificaRamo()
//----------------------------------------------------------------------------
// Questa funzione mi dice se due stazioni hanno un ramo in comune.
// Ritorna: TRUE se lo hanno
// Ha anche due parametri in cui ritorna il ramo in comune
// ed i Km tra le due stazioni (negativi se le le due stazioni sono in
// verso opposto al ramo).
//----------------------------------------------------------------------------
// ATTENZIONE: Sfrutto due regole:
// - Due stazioni non possono avere piu' di un ramo in comune
// - L' orientamento del ramo e' dalla stazione con Id minore a quella con Id maggiore.
// Si noti che NON ritorno il verso di percorrenza del ramo se le due stazioni
// distano meno di un Km: credo non comporti problemi.
//----------------------------------------------------------------------------
BOOL GRAFO::IdentificaRamo(ID Staz1,ID Staz2, ID & IdRamo,int & Km){
   #undef TRCRTN
   #define TRCRTN "GRAFO::IdentificaRamo()"

   if(Staz1 == 0 || Staz2 == 0){
      IdRamo = 0; Km = 0; return FALSE;
   }

   STAZ_FS & sDa = THIS[Staz1];
   STAZ_FS & sA  = THIS[Staz2];
   BOOL Ok=FALSE;

   #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
   PROFILER::Conta(4);
   #endif

   if (sDa.IsNodo()) {
      if (sA.IsNodo()) {
         // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
         SCAN_NUM_WIN(sA.PrimiVicini(),sA.NumeroPrimiVicini,PrimoVicinoCorrente,STAZ_FS::PRIMO_V){
            if (PrimoVicinoCorrente->Id == Staz1){
               #ifdef NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
               if(PrimoVicinoCorrente->Concesso)continue;
               #else
               if(PrimoVicinoCorrente->Concesso){
                  if(!AbilitaCumulativo)continue;
               } else {
                  if(AbilitaCumulativo == 99)continue; // Solo cumulativi
               }
               #endif
               IdRamo = PrimoVicinoCorrente->IdRamo;
               Km   = (Staz2 > Staz1) ? PrimoVicinoCorrente->Distanza : -PrimoVicinoCorrente->Distanza;
               Ok = TRUE;
               break;
            } /* endif */
         } ENDSCAN ;
      } else {
         RAMO & Ramo = Rami[sA.IdRamo];
         #ifdef NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
         if(Ramo.Concesso)return FALSE;
         #else
         if(Ramo.Concesso){
            if(!AbilitaCumulativo)return FALSE;
         } else {
            if(AbilitaCumulativo == 99) return FALSE;  // Solo cumulativi
         }
         #endif
         if (Ramo.IdStaz1 == Staz1) {
            IdRamo = Ramo.IdRamo;
            Km   = sA.Km1;
            Ok = TRUE;
         } else if (Ramo.IdStaz2 == Staz1) {
            IdRamo = Ramo.IdRamo;
            Km   = sA.Km1 - Ramo.KmRamo;
            Ok = TRUE;
         } /* endif */
//<<< if  sA.IsNodo
      }
//<<< if  sDa.IsNodo
   } else if (sA.IsNodo()) {
      RAMO & Ramo = Rami[sDa.IdRamo];
      #ifdef NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
      if(Ramo.Concesso)return FALSE;
      #else
      if(Ramo.Concesso){
         if(!AbilitaCumulativo)return FALSE;
      } else {
         if(AbilitaCumulativo == 99) return FALSE;  // Solo cumulativi
      }
      #endif
      if (Ramo.IdStaz1 == Staz2 ) {
         IdRamo = Ramo.IdRamo;
         Km   = -sDa.Km1;
         Ok = TRUE;
      } else if (Ramo.IdStaz2 == Staz2) {
         IdRamo = Ramo.IdRamo;
         Km     = Ramo.KmRamo - sDa.Km1;
         Ok = TRUE;
      } /* endif */
   } else if (sA.IdRamo == sDa.IdRamo) {
      RAMO & Ramo = Rami[sDa.IdRamo];
      #ifdef NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
      if(Ramo.Concesso)return FALSE;
      #else
      if(Ramo.Concesso){
         if(!AbilitaCumulativo)return FALSE;
      } else {
         if(AbilitaCumulativo == 99) return FALSE;  // Solo cumulativi
      }
      #endif
      IdRamo = sA.IdRamo;
      Km = sA.Km1 - sDa.Km1;
      Ok = TRUE;
//<<< if  sDa.IsNodo
   } /* endif */
   if(!Ok){
      // Non hanno un ramo in comune
      IdRamo = 0; Km = 0;
   }
   return Ok;
//<<< BOOL GRAFO::IdentificaRamo ID Staz1,ID Staz2, ID & IdRamo,int & Km
}
//----------------------------------------------------------------------------
// GRAFO::IdentificaStazioniDiDiramazione
//----------------------------------------------------------------------------
BOOL GRAFO::IdentificaStazioniDiDiramazione(ID Da, ID A, ARRAY_ID & FerrovieConcesseCompatibili, ARRAY_ID & Out){
   #undef TRCRTN
   #define TRCRTN "GRAFO::IdentificaStazioniDiDiramazione"
   if(RisolviPolimetriche(Da,A,FerrovieConcesseCompatibili)){
      while (Da != A) {
         STAZ_FS & sDa = THIS[Da];
         Da = sDa.Nodo1;
         Out += Da;
      }
      return TRUE;
   } else {
      Out += A;
      return FALSE;
   }
}
//----------------------------------------------------------------------------
// QUAD_TAR
//----------------------------------------------------------------------------
void QUAD_TAR::Set(BYTE Tar,BYTE Es1,BYTE Es2,BYTE Es3){
   #undef TRCRTN
   #define TRCRTN "QUAD_TAR::Set"
   // Impostazione a partire dai dati sciolti
   TariffaRegione     = Tar;
   if(Es1 != Tar)Prima_Estensione   = Es1;
   if(Es2 != Tar && Es2 != Es1)Seconda_Estensione = Es2;
   if(Es3 != Tar && Es3 != Es2 && Es3 != Es1)Terza_Estensione   = Es3;
   if(Tar){ // E' definita la regione di appartenenza
      NonValido    =0;
      VarieRegioni =0;
   } else {
      NonValido    =1;
      VarieRegioni =1;
   } /* endif */
};
//----------------------------------------------------------------------------
// QUAD_TAR::Empty
//----------------------------------------------------------------------------
BOOL QUAD_TAR::Empty() const {
   #undef TRCRTN
   #define TRCRTN "QUAD_TAR::Empty"
   return (TariffaRegione | Prima_Estensione | Seconda_Estensione | Terza_Estensione) == 0;
};
//----------------------------------------------------------------------------
// QUAD_TAR::operator &=
//----------------------------------------------------------------------------
QUAD_TAR& QUAD_TAR::operator &=(QUAD_TAR & Altro){
   #undef TRCRTN
   #define TRCRTN "QUAD_TAR::operator &="
   NonValido |=  Altro.NonValido;

   if(!Empty()){

      if (TariffaRegione != Altro.TariffaRegione)VarieRegioni =1;

      if (TariffaRegione     > 0                          &&
         TariffaRegione     != Altro.TariffaRegione      &&
         TariffaRegione     != Altro.Prima_Estensione    &&
         TariffaRegione     != Altro.Seconda_Estensione  &&
         TariffaRegione     != Altro.Terza_Estensione
      )   TariffaRegione     = 0   ;

      if (Prima_Estensione   > 0                          &&
         Prima_Estensione   != Altro.TariffaRegione      &&
         Prima_Estensione   != Altro.Prima_Estensione    &&
         Prima_Estensione   != Altro.Seconda_Estensione  &&
         Prima_Estensione   != Altro.Terza_Estensione
      )   Prima_Estensione   = 0   ;

      if (Seconda_Estensione > 0                          &&
         Seconda_Estensione != Altro.TariffaRegione      &&
         Seconda_Estensione != Altro.Prima_Estensione    &&
         Seconda_Estensione != Altro.Seconda_Estensione  &&
         Seconda_Estensione != Altro.Terza_Estensione
      )   Seconda_Estensione = 0   ;

      if (Terza_Estensione   > 0                          &&
         Terza_Estensione   != Altro.TariffaRegione      &&
         Terza_Estensione   != Altro.Prima_Estensione    &&
         Terza_Estensione   != Altro.Seconda_Estensione  &&
         Terza_Estensione   != Altro.Terza_Estensione
      )   Terza_Estensione   = 0   ;
//<<< if !Empty
   }
   return THIS;
//<<< QUAD_TAR& QUAD_TAR::operator &= QUAD_TAR & Altro
};

//----------------------------------------------------------------------------
// QUAD_TAR::Identifica
//----------------------------------------------------------------------------
BYTE QUAD_TAR::Identifica() const {
   #undef TRCRTN
   #define TRCRTN "QUAD_TAR::Identifica"
   // Identifica la tariffa regionale da applicare
   if(NonValido)return 255;
   if(Empty())return 0;

   int count = 0;
   if (TariffaRegione     > 0 ) count ++;
   if (Prima_Estensione   > 0 ) count ++;
   if (Seconda_Estensione > 0 ) count ++;
   if (Terza_Estensione   > 0 ) count ++;

   if (count ==  0) {
      BEEP;        // Dovrebbe essere stato gia' trattato
      return 255;
   } else if (count ==  1) {
      return  TariffaRegione +Prima_Estensione +Seconda_Estensione +Terza_Estensione;
   } else {
      if ( VarieRegioni == 0 ) { // Ok la regione (eguale per tutte) ha la precedenza
         return TariffaRegione;
      } else {
         STRINGA Msg;
         if (TariffaRegione     > 0 ) Msg += " " + STRINGA(TariffaRegione    );
         if (Prima_Estensione   > 0 ) Msg += " " + STRINGA(Prima_Estensione  );
         if (Seconda_Estensione > 0 ) Msg += " " + STRINGA(Seconda_Estensione);
         if (Terza_Estensione   > 0 ) Msg += " " + STRINGA(Terza_Estensione  );
         ERRSTRING("Caso sfortunato: ho piu' di una tariffa regionale applicabile, Codici = "+Msg);
         BEEP;
         return 255;
      } /* endif */
   } /* endif */
//<<< BYTE QUAD_TAR::Identifica   const
};

//----------------------------------------------------------------------------
// Metodi per gestire matrici triangolari di bits
//----------------------------------------------------------------------------
// per ogni elemento accetto al massimo 16 BITS !
// Size e' aggiornato e' riporta il numero di bytes utilizzati.
// Si suppone Buffer inizializzato a 0, e sufficientemente grande
// Torna FALSE su errore
BOOL _export SetBits(void * Buffer,int & Size,int IdxElem, int NumBits, WORD Valore){
   // Nota:           >>3 == /8           &0x7 == %8   (per velocizzare)
   #undef TRCRTN
   #define TRCRTN "SetBits()"
   BOOL Ok = TRUE;
   if( (1 << NumBits) <= Valore){
      ERRINT("Errore: "+STRINGA(NumBits)+" BITS non bastano per rappresentare un valore di",Valore);
      BEEP;
      Valore = (1 << NumBits)-1; // Tronco;
      Ok = FALSE;
   }

   int BitOffset = IdxElem * NumBits;
   int BitShift  = BitOffset &0x7;
   DWORD & Dato = *(DWORD*) ((BYTE*)Buffer  + (BitOffset >> 3));

   // Se mi fido dell' inizializzazione e non scrivo mai
   // piu' di una volta potrei rinunciare  all' operazione di mascheratura
   DWORD Mask = ((1 << NumBits)-1) << BitShift;   Mask = ~Mask;
   Dato &= Mask;

   DWORD Val = Valore; Val <<= BitShift;
   Dato |= Val;

   Size = max(Size, (BitOffset + NumBits + 7) >> 3);
   return Ok;
//<<< BOOL _export SetBits void * Buffer,int & Size,int IdxElem, int NumBits, WORD Valore
};
WORD _export GetBits(void * Buffer,int IdxElem, int NumBits){
   #undef TRCRTN
   #define TRCRTN "GetBits()"

   int BitOffset = IdxElem * NumBits;
   int BitShift  = BitOffset &0x7;

   // NB: conto sul fatto di poter sempre accedere ad una doubleword: a seconda delle
   // modalita' di allocazione potrebbe non essere vero.
   DWORD & Dato = *(DWORD*) ((BYTE*)Buffer  + (BitOffset >> 3));

   // Se mi fido dell' inizializzazione e non scrivo mai
   // piu' di una volta potrei rinunciare  all' operazione di mascheratura
   DWORD Mask = ((1 << NumBits)-1) << BitShift;
   DWORD Val = (Dato & Mask) >> BitShift;

   return Val;
};
//----------------------------------------------------------------------------
// STAZ_FS::PosDaid()
// STAZ_FS::PvDaid()
// STAZ_FS::DistDaid()
//----------------------------------------------------------------------------
WORD STAZ_FS::PosDaId(ID IdNodo2)const{ // Cerca nell' array dei primi vicini
   #undef TRCRTN
   #define TRCRTN "STAZ_FS::PosDaid()"

   PRIMO_V * Current = PrimiVicini();
   for (WORD i = 0; i < NumeroPrimiVicini ;Current ++,i++ ) {
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(5);
      #endif
      if(Current->Id == IdNodo2){
         return i;
      } /* endif */
   } /* endfor */
   ERRSTRING("Errore: qui non dovrebbe mai arrivare "+ STRINGA(IdNodo2));
   ERRSTRING("   Id del nodo in cui cerco: "+ STRINGA(Id));
   ERRSTRING("Possono capitare LOOP od abend");
   BEEP;
   return WORD_NON_VALIDA;
};
STAZ_FS::PRIMO_V & STAZ_FS::PvDaId(ID IdNodo2)const{ // Cerca nell' array dei primi vicini
   #undef TRCRTN
   #define TRCRTN "STAZ_FS::PvDaid()"

   PRIMO_V * Current = PrimiVicini();
   for (WORD i = 0; i < NumeroPrimiVicini ;Current ++,i++ ) {
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(5);
      #endif
      if(Current->Id == IdNodo2){
         return *Current;
      } /* endif */
   } /* endfor */
   // Potrebbe anche non esistere
   ERRSTRING("Errore: qui non dovrebbe mai arrivare "+ STRINGA(IdNodo2));
   ERRSTRING("   Id del nodo in cui cerco: "+ STRINGA(Id));
   ERRSTRING("Possono capitare LOOP od abend");
   BEEP;
   return * (PRIMO_V*)NULL;
};
DIST STAZ_FS::DistDaId(ID IdNodo2) const { // E ritorna la distanza
   #undef TRCRTN
   #define TRCRTN "STAZ_FS::DistDaid()"

   GRAFO & Grafo = *GRAFO::Grafo;
   if (IsNodo()){
      if (::IsNodo(IdNodo2)) {
         PRIMO_V * Current = PrimiVicini();
         for (WORD i = 0; i < NumeroPrimiVicini ;Current ++,i++ ) {
            #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
            PROFILER::Conta(5);
            #endif
            if(Current->Id == IdNodo2){
               return Current->Distanza;
            } /* endif */
         } /* endfor */
         ERRSTRING("Errore: qui non dovrebbe mai arrivare "+ STRINGA(IdNodo2));
         ERRSTRING("   Id del nodo in cui cerco: "+ STRINGA(Id));
         ERRSTRING("Possono capitare LOOP od abend");
         BEEP;
         return 0;
      } else {
         #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
         PROFILER::Conta(5);
         #endif
         STAZ_FS & Nodo2 = Grafo[IdNodo2];
         RAMO    & Ramo  = Grafo.Rami[Nodo2.IdRamo];
         if (Ramo.IdStaz1 == Id) {
            return Nodo2.Km1;
         } else if (Ramo.IdStaz2 == Id) {
            return Ramo.KmRamo  - Nodo2.Km1;
         } else {
            ERRSTRING("Errore: chiesta distanza tra due stazioni non appartenenti a stesso ramo");
            ERRSTRING("Nodo di base: "+STRINGA(Id));
            ERRSTRING("Altra stazione: "+STRINGA(IdNodo2));
            BEEP;
            return 0;
         } /* endif */
//<<< if  ::IsNodo IdNodo2
      } /* endif */
//<<< if  IsNodo
   } else {
      if (::IsNodo(IdNodo2)) {
         #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
         PROFILER::Conta(5);
         #endif
         RAMO    & Ramo  = Grafo.Rami[IdRamo];
         if (Ramo.IdStaz1 == IdNodo2) {
            return Km1;
         } else if (Ramo.IdStaz2 == IdNodo2) {
            return Ramo.KmRamo  - Km1;
         } else {
            ERRSTRING("Errore: chiesta distanza tra due stazioni non appartenenti a stesso ramo");
            ERRSTRING("Nodo di base: "+STRINGA(Id));
            ERRSTRING("Altra stazione: "+STRINGA(IdNodo2));
            BEEP;
            return 0;
         } /* endif */
      } else {
         #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
         PROFILER::Conta(5);
         #endif
         STAZ_FS & Nodo2 = Grafo[IdNodo2];
         if (IdRamo == Nodo2.IdRamo) {
            return abs(Km1 - Nodo2.Km1);
         } else {
            ERRSTRING("Errore: chiesta distanza tra due stazioni non appartenenti a stesso ramo");
            ERRSTRING("Nodo di base: "+STRINGA(Id));
            ERRSTRING("Altra stazione: "+STRINGA(IdNodo2));
            BEEP;
            return 0;
         } /* endif */
//<<< if  ::IsNodo IdNodo2
      } /* endif */
//<<< if  IsNodo
   } /* endif */
//<<< DIST STAZ_FS::DistDaId ID IdNodo2  const   // E ritorna la distanza
};
BYTE STAZ_FS::ConcDaId(ID IdNodo2) const {
   #undef TRCRTN
   #define TRCRTN "STAZ_FS::ConcDaid()"

   GRAFO & Grafo = *GRAFO::Grafo;
   if (IsNodo()){
      if (::IsNodo(IdNodo2)) {
         return PvDaId(IdNodo2).Concesso;
      } else {
         return Grafo.Rami[Grafo[IdNodo2].IdRamo].Concesso;
      } /* endif */
   } else {
      return Grafo.Rami[IdRamo].Concesso;
   } /* endif */
};
// Metodi per rilevare la coincidenza tra piu' polimetriche
// Le polimetriche debbono appartenere alle zone indicate.
// Rispetta la variabile GRAFO::AbilitaCumulativo
void STAZ_FS::SetPolimetriche(ARRAY_ID & Polims,const ARRAY_ID & Concesse, WORD Zone){
   #undef TRCRTN
   #define TRCRTN "STAZ_FS::SetPolimetriche"
   Polims.Clear();
   // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
   SCAN_NUM_WIN(Polimetriche(),NumeroPolimetriche,PPol,P_POLI){
      POLIMETRICA & Pol = GRAFO::Grafo->Polim[PPol->IdPoli];
      BOOL Cumulativa = Pol.Concessa();

      #ifdef NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
      if(Cumulativa)continue;
      #else
      if(Cumulativa){
         if(!GRAFO::Grafo->Gr().AbilitaCumulativo)continue;
         if(&Concesse && !Concesse.Contiene(Pol.SocietaConcessa)){
            //#if defined(DBG6) || defined(DBG9)
            //TRACESTRING("Stazione "+STRINGA(Id)+" Ignoro cumulativa Concessa = "+STRINGA(Pol.SocietaConcessa));
            //#endif
            continue;
         }
      } else {
         if(GRAFO::Grafo->Gr().AbilitaCumulativo == 99)continue;
      }
      #endif

      //TRACEVLONG(Pol.IdPoli);
      if(Pol.Zone & Zone)Polims+=Pol.IdPoli;
//<<< SCAN_NUM Polimetriche  ,NumeroPolimetriche,PPol,P_POLI
   } ENDSCAN ;
//<<< void STAZ_FS::SetPolimetriche ARRAY_ID & Polims,const ARRAY_ID & Concesse, WORD Zone
}
void STAZ_FS::SetPolimetricheNoDiramaz(ARRAY_ID & Polims,const ARRAY_ID & Concesse, WORD Zone){
   #undef TRCRTN
   #define TRCRTN "STAZ_FS::SetPolimetricheNoDiramaz"
   Polims.Clear();
   // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
   SCAN_NUM_WIN(Polimetriche(),NumeroPolimetriche,PPol,P_POLI){
      POLIMETRICA & Pol = GRAFO::Grafo->Polim[PPol->IdPoli];

      if(Pol.TipoPoli == POLIMETRICA::DIRAMAZIONE)continue;
      if(Pol.TipoPoli == POLIMETRICA::ALLACCIAMENTO)continue;
      BOOL Cumulativa = Pol.Concessa();

      #ifdef NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
      if(Cumulativa)continue;
      #else
      if(Cumulativa){
         if(!GRAFO::Grafo->Gr().AbilitaCumulativo)continue;
         if(& Concesse && !Concesse.Contiene(Pol.SocietaConcessa)){
            //#if defined(DBG6) || defined(DBG9)
            //TRACESTRING("Stazione "+STRINGA(Id)+" Ignoro cumulativa Concessa = "+STRINGA(Pol.SocietaConcessa));
            //#endif
            continue;
         }
      } else {
         if(GRAFO::Grafo->Gr().AbilitaCumulativo == 99)continue;
      }
      #endif

      //TRACEVLONG(Pol.IdPoli);
      if(Pol.Zone & Zone)Polims+=Pol.IdPoli;
//<<< SCAN_NUM Polimetriche  ,NumeroPolimetriche,PPol,P_POLI
   } ENDSCAN ;
//<<< void STAZ_FS::SetPolimetricheNoDiramaz ARRAY_ID & Polims,const ARRAY_ID & Concesse, WORD Zone
}
void STAZ_FS::AndPolimetriche(ARRAY_ID & Polims,WORD Zone){
   #undef TRCRTN
   #define TRCRTN "STAZ_FS::AndPolimetriche"
   static ARRAY_ID Po;
   SetPolimetriche(Po,*(ARRAY_ID*)NULL,Zone);
   Polims.Intersecato(Po);
}
void STAZ_FS::OrPolimetriche(ARRAY_ID & Polims,WORD Zone){
   #undef TRCRTN
   #define TRCRTN "STAZ_FS::OrPolimetriche"
   static ARRAY_ID Po;
   SetPolimetriche(Po,*(ARRAY_ID*)NULL,Zone);
   Polims.Unito(Po);
}
int STAZ_FS::HaPolimetrica(ID IdPolimetrica){
   #define TRCRTN "STAZ_FS::HaPolimetrica"
   #undef TRCRTN
   // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
   SCAN_NUM_WIN(Polimetriche(),NumeroPolimetriche,Pol,P_POLI){
      if(Pol->IdPoli == IdPolimetrica)return Pol->IdxStazione;
   } ENDSCAN ;
   return -1;
}
//----------------------------------------------------------------------------
// Nome7
//----------------------------------------------------------------------------
const char * _export STAZ_FS::Nome7()const{  // Definito solo per le stazioni di instradamento
   static char OutBuf[80];
   static int Idx;
   char * Out = OutBuf+ (8 * Idx);
   Idx ++; Idx &= 7;
   memmove(Out,IdentIstr,7);
   Out[7] = 0;
   return Out;
};
//----------------------------------------------------------------------------
// RAMO::PrimaStazioneDiInstradamento()
//----------------------------------------------------------------------------
// Da ed A debbono appartenere al ramo
// Torna 0 se non ho stazione di instradamento
ID RAMO::PrimaStazioneDiInstradamento(ID Da, ID A){
   #undef TRCRTN
   #define TRCRTN "RAMO::PrimaStazioneDiInstradamento() "

   if (GRAFO::Gr()[Da].DiInstradamento)return Da;

   if(Instradamento){ // Altrimenti inutile controllare le stazioni intermedie
      int Start, Stop , Step ;
      if (Da == IdStaz1){
         Start = 0;
      } else if (Da == IdStaz2){
         Start = NumStazioni-1;
      } else {
         Start = GRAFO::Gr()[Da].ProgRamo-1;
      }
      if (A  == IdStaz1){
         Stop  = 0;
      } else if (A  == IdStaz2){
         Stop  = NumStazioni-1;
      } else {
         Stop = GRAFO::Gr()[A].ProgRamo-1;
      }
      if(Stop >= Start){
         Step = 1;
      } else {
         Step = -1;
      }
      STAZIONE_R * StzStop  = StazioniDelRamo() + Stop;
      STAZIONE_R * Stazione = StazioniDelRamo() + Start;
      do {
         if(Stazione->Instrad) return Stazione->Id;
         if (Stazione == StzStop)break;
         Stazione += Step;
      } while (TRUE); /* enddo */
//<<< if Instradamento   // Altrimenti inutile controllare le stazioni intermedie
   }
   if (GRAFO::Gr()[A].DiInstradamento)return A;
   return 0;
//<<< ID RAMO::PrimaStazioneDiInstradamento ID Da, ID A
};
//----------------------------------------------------------------------------
// Funzioni di sort
//----------------------------------------------------------------------------
int Cmp_RAMO(const void * a, const void * b){
   #undef TRCRTN
   #define TRCRTN "Cmp_RAMO"
   RAMO & A = *(RAMO*)a;
   RAMO & B = *(RAMO*)b;
   if(A.IdStaz1  != B.IdStaz1) return (int)A.IdStaz1 - (int)B.IdStaz1;
   return (int)A.IdStaz2 - (int)B.IdStaz2;
};
//----------------------------------------------------------------------------
// POLIMETRICA::Contiene
//----------------------------------------------------------------------------
BOOL POLIMETRICA::Contiene(ID Staz){

   #undef TRCRTN
   #define TRCRTN "POLIMETRICA::Contiene"

   GRAFO & Grafo = *GRAFO::Grafo;
   STAZ_FS & S1 = Grafo.StazioniGrafo[Staz];

   // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
   SCAN_NUM_WIN(S1.Polimetriche(),S1.NumeroPolimetriche,Pol1,STAZ_FS::P_POLI){
      if(Pol1->IdPoli == IdPoli) return TRUE;
   } ENDSCAN ;
   return FALSE;
}
//----------------------------------------------------------------------------
// POLIMETRICA::DistanzaTra
//----------------------------------------------------------------------------
DIST POLIMETRICA::DistanzaTra(ID From,ID To,BYTE & Verso){ // Torna BigDIST se non appartengono entrambi ala polimetrica

   #undef TRCRTN
   #define TRCRTN "POLIMETRICA::DistanzaTra"

   Verso = 0    ;
   if(From == To)return 0;

   int idx1=-1,idx2=-1;
   int liv1,liv2;

   GRAFO & Grafo = *GRAFO::Grafo;

   STAZ_FS & S1 = Grafo.StazioniGrafo[From];
   STAZ_FS & S2 = Grafo.StazioniGrafo[To];

   // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
   SCAN_NUM_WIN(S1.Polimetriche(),S1.NumeroPolimetriche,Pol1,STAZ_FS::P_POLI){
      if(Pol1->IdPoli == IdPoli){
         idx1 = Pol1->IdxStazione;
         break;
      }
   } ENDSCAN ;
   if (idx1 <0 ){
      ERRSTRING("La stazione 'from' " + STRINGA(From)+ " non appartiene alla polimetrica con ID "+STRINGA(IdPoli));
      return BigDIST;
   }
   // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
   SCAN_NUM_WIN(S2.Polimetriche(),S2.NumeroPolimetriche,Pol2,STAZ_FS::P_POLI){
      if(Pol2->IdPoli == IdPoli){
         idx2 = Pol2->IdxStazione;
      }
   } ENDSCAN ;

   if (idx2 <0 ){
      ERRSTRING("La stazione 'to' " + STRINGA(To)+ " non appartiene alla polimetrica con ID "+STRINGA(IdPoli));
      return BigDIST;
   }

   Verso = Bool(idx1 > idx2);

   return DistanzaTra(idx1,idx2,StazioniDellaPolimetrica());
//<<< DIST POLIMETRICA::DistanzaTra ID From,ID To,BYTE & Verso   // Torna BigDIST se non appartengono entrambi ala polimetrica
};

DIST POLIMETRICA::DistanzaTra(int idx1,int idx2,PO_STAZ * Sp){

   if(idx1 == idx2) return 0;

   if (idx1 > idx2) {
      // Debbo invertire il verso
      int t = idx1;
      idx1 = idx2;
      idx2 = t;
   } /* endif */

   int D2 = 0;

   switch (TipoPoli) {
   case URBANA_CUM :
      // Di collegamento
      break;
   case LOCALE  :
   case SOLO_LOCALE  :
   case MARE_FS :
      if(idx1 == 0){
         D2 = Sp[idx2].Km;
      } else {
         D2 = Base + GetTrgBits(StazioniDiramazione()+NumStazDiram,idx2-2,idx1-1,NumBits);
         PO_STAZ & St1 = Sp[idx1];
         PO_STAZ & St2 = Sp[idx2];
         if(St1.Livello == St2.Livello){
            D2 +=  St2.Km - St1.Km ;
         } else if(St1.Livello > St2.Livello){
            PO_STAZ & StRadice = Sp[St1.Radice];
            D2 +=  St1.Km - StRadice.Km;
            D2 +=  DistanzaTra(idx2,St1.Radice,Sp);
         } else {
            PO_STAZ & StRadice = Sp[St2.Radice];
            D2 +=  St2.Km - StRadice.Km;
            D2 +=  DistanzaTra(idx1,St2.Radice,Sp);
         } /* endif */
      } /* endif */
      break;
//<<< switch  TipoPoli
   case CUMULATIVA  :
   case MARE_CUM    :
      if(idx1 == 0){
         D2 = Sp[idx2].Km;
      } else {
         D2 = GetTrgBits(StazioniDiramazione()+NumStazDiram,idx2-2,idx1-1,NumBits);
      }
      if( D2 == 0 ) D2 = 9999; // NON Collegato
      break;
   default:
      if(idx1 == 0){
         D2 = Sp[idx2].Km;
      } else {
         D2 = GetTrgBits(StazioniDiramazione()+NumStazDiram,idx2-2,idx1-1,NumBits);
      }
      break;
   } /* endswitch */

   return D2;
//<<< DIST POLIMETRICA::DistanzaTra int idx1,int idx2,PO_STAZ * Sp
}
//----------------------------------------------------------------------------
// PERCORSO.SetUtilizzandoPolimetriche()
//----------------------------------------------------------------------------
BOOL PERCORSO_GRAFO::SetUtilizzandoPolimetriche(ARRAY_ID& NodiViaggio,BOOL SegnalaErrore){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::SetUtilizzandoPolimetriche()"

   #ifdef NO_CORR_POLI
   return Set(NodiViaggio, SegnalaErrore);
   #else

   #ifdef DBG2P
   NodiViaggio.Trace(*GRAFO::Grafo,"Stazioni vincolanti originali");
   #endif

   ARRAY_ID FerrovieConcesseUtilizzate;
   GRAFO & Grafo = *GRAFO::Grafo;
   int Ab1 = Grafo[NodiViaggio[0]].Abilitazione;
   int Ab2 = Grafo[NodiViaggio.Last()].Abilitazione;
   // TRACESTRING(VRS(Ab1) + VRS(Ab2));
   if((Ab1 & STAZ_FS::SARDEGNA) != (Ab2 & STAZ_FS::SARDEGNA)){
      FerrovieConcesseUtilizzate += 98; // Tirrenia
      #ifdef DBG2P
      TRACESTRING("Aggiunta abilitazione tirrenia Da/A Sardegna");
      #endif
   }
   FORALL(NodiViaggio,i){
      ID Id = NodiViaggio[i];
      // Gestione ESCMAR
      if(Id > 3400 && Id < 3410){
         if(!FerrovieConcesseUtilizzate.Contiene(98)){
            FerrovieConcesseUtilizzate += 98;
            #ifdef DBG2P
            TRACESTRING("Aggiunta societa' Tirrenia per presenza ESCMARE");
            #endif
         }
         continue;
      }
      STAZ_FS & Stz = Grafo[Id];
      // TRACESTRING(VRS(Stz.Id) + VRS(Stz.NumeroConcesse));
      for(int j = 0 ; j < Stz.NumeroConcesse ; j++){
         ID Soc = Stz.Concesse()[j].SocietaConcessa;
         if (Soc != 98 && !Grafo.AbilitaCumulativo)continue; // Se non ho abilitato il cumulativo posso utilizzare solo la 98
         // Questo per disabilitare le stazioni solo FS che fanno da transito per la Tirrenia
         // Altrimenti fa il Napoli Palermo via mare (ed idem per Genova)
         if(Soc == 98 && Stz.Concesse()[j].CCRCumulativo == 0) Soc = 0;
         // TRACESTRING(VRS(Soc));
         if(Soc && !FerrovieConcesseUtilizzate.Contiene(Soc)){
            FerrovieConcesseUtilizzate += Soc;
            #ifdef DBG2P
            TRACESTRING("Aggiunta societa' concessa "+STRINGA(Soc)+" da stazione "+STRINGA(NodiViaggio[i]));
            #endif
         }
      }
//<<< FORALL NodiViaggio,i
   }
   if(FerrovieConcesseUtilizzate.Dim()){
      FerrovieConcesseUtilizzate += 0;
      Grafo.AbilitaCumulativo = TRUE;    // Per gestione Tirrenia su tratte servizio interno
   }


   ARRAY_ID NodiCorretti1;
   ORD_FORALL(NodiViaggio,i1){ // Gestione ESCMAR
      ID Id = NodiViaggio[i1];
      if(Id > 3400 && Id < 3410)continue;
      NodiCorretti1 += Id;
   }
   ARRAY_ID NodiCorretti;
   NodiCorretti += NodiCorretti1[0];
   for (i = 1; i < NodiCorretti1.Dim() ; i++) {
      GRAFO::Gr().IdentificaStazioniDiDiramazione(NodiCorretti1[i-1],NodiCorretti1[i],FerrovieConcesseUtilizzate,NodiCorretti);
   }
   #ifdef DBG2P
   NodiCorretti.Trace(*GRAFO::Grafo,"Stazioni vincolanti con correzioni per polimetriche");
   #endif
   return Set(NodiCorretti, SegnalaErrore);
   #endif
//<<< BOOL PERCORSO_GRAFO::SetUtilizzandoPolimetriche ARRAY_ID& NodiViaggio,BOOL SegnalaErrore
};
//----------------------------------------------------------------------------
// PERCORSO.Set()
//----------------------------------------------------------------------------
// Ritorna il percorso completo vincolato ai nodi dell' array
// Il percorso e' tornato in forma normale
BOOL PERCORSO_GRAFO::Set(ARRAY_ID& NodiViaggio,BOOL SegnalaErrore){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::Set()"

   #ifdef DBG3
   NodiViaggio.Trace(*GRAFO::Grafo,"Stazioni vincolanti");
   #endif

   this->Clear();

   SCAN(NodiViaggio,NodoV,ID){
      if(!Prolunga(NodoV)){
         if(SegnalaErrore){
            NodiViaggio.Trace(*GRAFO::Grafo, "PERCORSO STRUTTURALMENTE ERRATO: Stazioni vincolanti",1);
            TRACECALLSTACK("Call StacK:");
         }
         return FALSE; // E' in errore
      }
   } ENDSCAN ;
   Normalizza();
   #ifdef DBG3
   Nodi.Trace(*GRAFO::Grafo,"Stazioni del percorso");
   TRACEVLONG(Len());
   #endif
   return TRUE;
//<<< BOOL PERCORSO_GRAFO::Set ARRAY_ID& NodiViaggio,BOOL SegnalaErrore
};

//----------------------------------------------------------------------------
// PERCORSO_GRAFO::TraceShortest
//----------------------------------------------------------------------------
// Questo e' un trace del percorso piu' breve tra due nodi
void PERCORSO_GRAFO::TraceShortest(ID Da,ID A, int Livello){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::TraceShortest"
   if(Livello > trchse)return;
   ARRAY_ID Tmp ;
   Tmp += Da;
   Tmp += A;
   PERCORSO_GRAFO Tmp2(Tmp);
   Tmp2.Trace("Percorso piu' breve che unisce i nodi "+STRINGA(Da)+" e "+STRINGA(A),1);
}
//----------------------------------------------------------------------------
// PERCORSO_GRAFO::Normalizza()
//----------------------------------------------------------------------------
// Qui il grafo e' gia' completo, ma puo' avere delle stazioni non nodali in piu'
void PERCORSO_GRAFO::Normalizza(){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::Normalizza()"
   GRAFO & Grafo = GRAFO::Gr();
   for (int i = Nodi.Dim() - 2; i > 0 ; i--) {  // Non tocco origine o destinazione
      // Va eliminato se non e' :
      //   - nodo
      //   - stazione di instradamento eccezionale
      //   - punto di inversione
      if(!Grafo[Nodi[i]].IsInstrGrafo()){
         if(!IsPuntoDiInversione(i))Nodi -= i;
      }
   } /* endfor */
};
//----------------------------------------------------------------------------
// PERCORSO_GRAFO::IsPuntoDiInversione()
//----------------------------------------------------------------------------
// NB: Il percorso puo' o meno essere in forma normalizzata
BOOL PERCORSO_GRAFO::IsPuntoDiInversione(int i){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::IsPuntoDiInversione()"

   if (i <= 0)return FALSE;      // Origine
   int Limit = Nodi.Dim() -1;
   if (i >= Limit)return FALSE;             // Destinazione

   ID * Nodo = &Nodi[i];
   ID Id  = *Nodo;
   ID Id1 = *(Nodo-1);
   ID Id2 = *(Nodo+1);
   if(Id1 == Id2 ){
      #ifdef DBG9
      TRACEINT("Punto di inversione, Id = ",Id);
      #endif
      return TRUE;    // Evidente punto di inversione
   }

   // A questo punto posso avere un punto di inversione
   // - Se e' il primo Nodo
   // - Se e' l' ultimo Nodo
   // - Se il nodo precedente e' NON NODALE
   // - Se il nodo successivo e' NON NODALE
   // In tutti gli altri casi i punti di inversione hanno sempre
   // Nodi[i-1] == Nodi [i+1]

   if(i != 1 && i != Limit - 1 && IsNodo(Id1) && IsNodo(Id2))return FALSE;


   ID IdRamo1,IdRamo2;
   int Km1,Km2;
   BOOL Ok;
   GRAFO & Grafo = GRAFO::Gr();
   Ok  = Grafo.IdentificaRamo(Id1,Id, IdRamo1, Km1);
   Ok &= Grafo.IdentificaRamo(Id2,Id, IdRamo2, Km2);
   BOOL Inversione = Ok && (IdRamo1 == IdRamo2) && ((Km1*Km2)>0) ;
   #ifdef DBG9
   if(Inversione)TRACEINT("Punto di inversione, Id = ",Id);
   #endif
   return Inversione;

//<<< BOOL PERCORSO_GRAFO::IsPuntoDiInversione int i
};
//----------------------------------------------------------------------------
// PERCORSO::Prolunga
//----------------------------------------------------------------------------
BOOL PERCORSO_GRAFO::Prolunga(ID Arrivo){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::Prolunga()"

   if(Nodi.Dim() == 0){
      Nodi += Arrivo;
      Lg = 0;
      return TRUE;
   }


   GRAFO & Grafo = *GRAFO::Grafo;
   int Numc = Grafo[Nodi[0]].Grf.NumCum;
   ID Origine = Nodi.Last(); // Origine = Ultimo nodo del percorso
   STAZ_FS& NodoOrigine = Grafo[Origine];
   #ifdef DBG4
   TRACESTRING("Da "+STRINGA(Origine)+" A "+STRINGA(Arrivo)+STRINGA(" Prog=")+STRINGA(Grafo.ProgressivoCorrente));
   #endif
   if(Origine == Arrivo ){
      TRACESTRINGL("Origine == Arrivo == "+STRINGA(Origine),2);
      return FALSE;
   }

   if(!Grafo.Risolvi(Origine,Arrivo)){
      Grafo[Nodi[0]].Grf.NumCum = Numc ;
      // Questo e' normale purtroppo per gli EscMare
      if(Arrivo >= 3400 && Arrivo < 3420) { // E' un Escmar sui precaricati
         return TRUE; // Lo ignoro proprio
      };
      this->Trace(TRCRTN "Errore: impossibile completare",1);
      Grafo.TraceId(TRCRTN "Nodo di Partenza   : ",Origine,1);
      Grafo.TraceId(TRCRTN "Nodo da Raggiungere: ",Arrivo,1);
      Lg = 0;
      BEEP;
      return FALSE;
   };

   if (Lg >= 0) Lg += NodoOrigine.Distanza1;
   Grafo[Nodi[0]].Grf.NumCum = Numc +NodoOrigine.Grf.NumCum;

   // Adesso estendo il percorso, facendo attenzione ad evitare le stazioncine inutili
   ID IdNodo = Origine;
   while( IdNodo != Arrivo ){          // Finche' non ho incluso la destinazione
      STAZ_FS & Nodo = Grafo[IdNodo];
      IdNodo = Nodo.Nodo1;
      Nodi   += IdNodo;
      #ifdef DBG4
      Grafo.TraceId(STRINGA(TRCRTN)+ "Nodo Aggiunto: (Prog = "+STRINGA(Nodo.Progressivo)+") ",IdNodo);
      #endif
      if(Nodi.Dim() > 300){
         ERRSTRING("LOOP IRRIMEDIABILE probabile abend");
         Trace("Percorso in errore",1);
         Grafo.TraceId("Nodo di prolungamento",Arrivo,1);
         Lg = 0;
         BEEP;
         return FALSE;
      };
   } /* endfor */
   return TRUE;
//<<< BOOL PERCORSO_GRAFO::Prolunga ID Arrivo
};


//----------------------------------------------------------------------------
// PERCORSO_GRAFO::Trace()
//----------------------------------------------------------------------------
void PERCORSO_GRAFO::Trace(const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN " "
   if(Livello > trchse)return;
   GRAFO & Grafo = *GRAFO::Grafo;
   if(Messaggio != NUSTR)ERRSTRING(Messaggio);
   STAZ_FS & Orig = Grafo[Nodi[0]];
   if (Lg >= 0) {
      ERRSTRING("Lunghezza: " + STRINGA(Lg)+" Km  Numero Nodi: "+STRINGA(Nodi.Dim()) +" NumCum = "+STRINGA(Orig.Grf.NumCum) +" SumId = "+STRINGA(Orig.Grf.SumId) );
   } else {
      ERRSTRING("Lunghezza non definita,  Numero Nodi: "+STRINGA(Nodi.Dim()));
   } /* endif */
   ORD_FORALL(Nodi,i){
      STRINGA Msg;
      Msg += "Nodi["+ STRINGA(i)+ STRINGA("] ");
      if (i < (Nodi.Dim()-1)) {
         int Km;
         ID IdRamo;
         if(Grafo.IdentificaRamo(Nodi[i], Nodi[i+1],IdRamo, Km)){
            Msg +="("+STRINGA(abs(Km))+" Km, Ramo "+STRINGA(IdRamo)+" ) ";
         }
      }
      Grafo.TraceId(Msg,Nodi[i],1);
   };
   ERRSTRING("");
//<<< void PERCORSO_GRAFO::Trace const STRINGA& Messaggio, int Livello
};


//----------------------------------------------------------------------------
// PERCORSO_GRAFO:: operator ==
//----------------------------------------------------------------------------
BOOL PERCORSO_GRAFO::operator ==(PERCORSO_GRAFO & b){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::operator =="
   if(Nodi.Dim()!= b.Nodi.Dim())return FALSE;
   if(Lg != b.Lg && Lg > 0 && b.Lg > 0)return FALSE;
   ID * i1 = &Nodi[0];   // Puntatore al primo array di ID
   ID * i2 = &b.Nodi[0]; // Puntatore al secondo array di ID
   ID * Stop = i1 + Nodi.Dim();
   while (i1 < Stop) {  // Scansione dell' array
      if(*i1 != *i2)return FALSE;
      i1++; i2++;
   } /* endwhile */
   return TRUE;
};
//----------------------------------------------------------------------------
// PERCORSO_GRAFO::Len()
//----------------------------------------------------------------------------
LONG PERCORSO_GRAFO::Len(){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::Len()"
   if(Lg >= 0)return Lg;
   // FIX: Al momento non e' ancora gestito il calcolo posticipato della lunghezza.
   BEEP;
   return 0;
};
//----------------------------------------------------------------------------
// PERCORSO_GRAFO::FaCorrezione()
//----------------------------------------------------------------------------
// Eventuali correzioni al percorso dovute ad eccezioni di normativa
// Effettua una correzione: torna false se non vi erano correzioni da fare
// La modalita' indica se e' stato chiamato a mano o da motore:
// --- superato: dovrei piuttosto gestire un doppio chilometraggio
// Se chiamato a mano non effettua alcune correzioni (per permettere
// l' emissione degli abbonamenti).
// ---
// Il rc e' 1 se ho fatto la correzione; di solito il chiamante richiama
// iterativamente il metodo finche' Rc != 1.
//----------------------------------------------------------------------------
int  PERCORSO_GRAFO::FaCorrezione(){
   // int  PERCORSO_GRAFO::FaCorrezione(BYTE){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::FaCorrezione()"

   GRAFO & Grafo = *GRAFO::Grafo;

   ARRAY_ID & From = (NumeroCorrezioni == 0 ? Nodi : NodiCorretti );

   // Si noti che queste sono reference: Per permettere delle scansioni
   ID & Id1 = From[0];
   ID & Id2 = From.Last();

   // Identifico il tipo di correzione
   int TipoCorrezione=0; // Indica se devo correggere il percorso , e come
   int Idx = 0  ;        // Per capire DOVE deve essere corretto
   if(Grafo.EccezioniNormativa != NULL){
      // if(Grafo.EccezioniNormativa != NULL){
      // Correzioni dovute a percorso con inversione ad una delle stazioni particolari ...
      // Per prima cosa determino i punti di inversione (NB: non uso il metodo standard, basta
      // la verifica su nodo precedente e seguente (in quanto Id1 deve essere un nodo e cosi' di seguito le altre stazioni))
      ID * Stop = &Id1;
      for ( ID * Corrente = &From[From.Dim()-2];Corrente > Stop ;Corrente -- ) {
         if((*(Corrente - 1)) == (*(Corrente+1)) ){ // Punto di inversione
            ID PuntoDiInversione = *Corrente ;
            Idx = Corrente - Stop;
            // Cerco di identificarlo tra quelli eccezionali
            int minI,maxI;
            Grafo.EccezioniNormativa->DeterminaIdx(1, minI, maxI);
            for (int i = minI; i < maxI ;i++ ) {
               ECCEZ_NORMATIVA & Ecz = (*Grafo.EccezioniNormativa)[i];
               if( PuntoDiInversione == Ecz.Id2 ){ // Maybe
                  // La parte duplicata del percorso deve coincidere con quella specificata nell' eccezione
                  int MaxDup =  min(Idx+1, ( From.Dim() - Idx ) );
                  if(MaxDup < Ecz.DimPercorso ) continue; // Non puo' essere
                  for (int j = 1; j < Ecz.DimPercorso ; j++ ) {
                     if((*(Corrente - j)) != (*(Corrente+j)) )break;                      // Il percorso NON e' fatto avanti ed indietro
                     if((*(Corrente - j)) != Ecz.Percorso[Ecz.DimPercorso - j - 1])break; // Non corrisponde con il percorso dell' eccezzione
                  } /* endfor */
                  if( j ==  Ecz.DimPercorso ){
                     // L' inversione e' una di quelle catalogate a condizioni e tariffe
                     if (!Ecz.ValeEsatto){
                        #ifdef DBG12
                        TRACESTRING("Eccezione normativa su punto di inversione: Percorso da accorciare");
                        #endif
                        TipoCorrezione = 10; // Ok il percorso deve essere corretto
                        break;
                     } else {
                        #ifdef DBG12
                        TRACESTRING("Eccezione normativa su punto di inversione: Percorso da tariffare");
                        #endif
                     } /* endif */
                  } else {
                     //#ifdef DBG12
                     //TRACESTRING("Punto di inversione non eccezionale: Percorso da inibire");
                     //#endif
                                         // TipoCorrezione = 30; // Ok il percorso deve essere corretto
                                         // break;
                  }
//<<<          if  PuntoDiInversione == Ecz.Id2    // Maybe
               }
//<<<       for  int i = minI; i < maxI ;i++
            }
            if(TipoCorrezione)break;
//<<<    if  * Corrente - 1   ==  * Corrente+1      // Punto di inversione
         } /* endif */
//<<< for   ID * Corrente = &From From.Dim  -2 ;Corrente > Stop ;Corrente --
      } /* endfor */

      if(!TipoCorrezione){
         int minI,maxI;
         Grafo.EccezioniNormativa->DeterminaIdx(2, minI, maxI);
         for (int i = minI; i < maxI ;i++ ) {
            ECCEZ_NORMATIVA & Ecz = (*Grafo.EccezioniNormativa)[i];
            if(Ecz.DimPercorso <= From.Dim())continue; // Anche = perche' non debbono coincidere esattamente
            if(Ecz.Id1 == Id1){ // Potrebbero effettivamente coincidere  (parte iniziale)
               ID * Corrente = &Id1;
               ID * Perc = Ecz.Percorso;
               Idx = Ecz.Id2; // Numero di stazioni da eliminare
               for (int i = 0; i < Ecz.DimPercorso ; i++ ) {
                  if(*Corrente != *Perc)break;
                  Corrente ++; Perc ++;
               } /* endfor */
               if( i ==  Ecz.DimPercorso )TipoCorrezione = 20; // Ok il percorso deve essere corretto
            } else if(Ecz.Id1 == Id2){ // Potrebbero effettivamente coincidere  (parte finale)
               ID * Corrente = &Id2;
               ID * Perc = Ecz.Percorso;
               Idx = Ecz.Id2; // Numero di stazioni da eliminare
               for (int i = 0; i < Ecz.DimPercorso ; i++ ) {
                  if(*Corrente != *Perc)break;
                  Corrente --; Perc ++; // Notare che scandisco in senso inverso
               } /* endfor */
               if( i ==  Ecz.DimPercorso )TipoCorrezione = 21; // Ok il percorso deve essere corretto
            }
            if(TipoCorrezione)break;
//<<<    for  int i = minI; i < maxI ;i++
         }
//<<< if !TipoCorrezione
      }
//<<< if Grafo.EccezioniNormativa != NULL
   } /* endif */

   // L' inibizione di alcune stazioni va fatta sia se sono in viaggio libero che via treni
   if(TipoCorrezione == 0
      && Grafo.EccezioniNormativa->StazioniConInibizione.Contiene(Id1)
   ){
      TipoCorrezione = 30; // Percorso inibito
      TRACESTRING("Stazione Inibita : "+STRINGA(Id1));
      int minI,maxI;
      Grafo.EccezioniNormativa->DeterminaIdx(3, minI, maxI);
      for (int i = minI; i < maxI ;i++ ) {
         ECCEZ_NORMATIVA & Ecz = (*Grafo.EccezioniNormativa)[i];
         if(Ecz.Id1 == Id1 && Ecz.DimPercorso == From.Dim()){ // Potrebbero effettivamente coincidere
            ID * Corrente = &Id1;
            ID * Perc = Ecz.Percorso;
            for (int i = 0; i < Ecz.DimPercorso ; i++ ) {
               if(*Corrente != *Perc)break;
               Corrente ++; Perc ++;
            } /* endfor */
            if( i ==  Ecz.DimPercorso ){
               TipoCorrezione = 0; // Ok il percorso e' valido
               break;
            }
         }
      }
//<<< if TipoCorrezione == 0
   }

   if(TipoCorrezione == 0
      && Grafo.EccezioniNormativa->StazioniConInibizione.Contiene(Id2)
   ){
      TipoCorrezione = 30; // Percorso inibito
      TRACESTRING("Stazione Inibita : "+STRINGA(Id2));
      int minI,maxI;
      Grafo.EccezioniNormativa->DeterminaIdx(3, minI, maxI);
      for (int i = minI; i < maxI ;i++ ) {
         ECCEZ_NORMATIVA & Ecz = (*Grafo.EccezioniNormativa)[i];
         if(Ecz.Id1 == Id2 && Ecz.DimPercorso == From.Dim()){ // Potrebbero effettivamente coincidere
            ID * Corrente = &Id1 + From.Dim() - 1;
            ID * Perc = Ecz.Percorso;
            for (int i = 0; i < Ecz.DimPercorso ; i++ ) {
               if(*Corrente != *Perc)break;
               Corrente --;  Perc ++;
            } /* endfor */
            if( i ==  Ecz.DimPercorso ){
               TipoCorrezione = 0; // Ok il percorso e' valido
               break;
            }
         }
      }
//<<< if TipoCorrezione == 0
   }

   // L' inibizione di coppie O/D va fatta sia se sono in viaggio libero che via treni
   if(TipoCorrezione == 0 &&
      (
         Grafo.EccezioniNormativa->CoppieConInibizione.Contiene(Id1) ||
         Grafo.EccezioniNormativa->CoppieConInibizione.Contiene(Id2)
      )
   ){
      int minI,maxI;
      Grafo.EccezioniNormativa->DeterminaIdx(5, minI, maxI);
      for (int i = minI; i < maxI ;i++ ) {
         ECCEZ_NORMATIVA & Ecz = (*Grafo.EccezioniNormativa)[i];
         if(Ecz.Id1 == Id1){
            if(Ecz.Id2 == Id2  ){
               TipoCorrezione = 30; // Percorso inibito
               TRACESTRING("Coppia O/D Inibita : "+STRINGA(Id1) + "->" +STRINGA(Id2));
               break;
            } else if(Ecz.Id2 == 0 ){
               TipoCorrezione = 30; // Percorso inibito
               TRACESTRING("Stazione inibita : "+STRINGA(Id1));
               break;
            }
         } else if(Ecz.Id1 == Id2){
            if(Ecz.Id2 == Id1  ){
               TipoCorrezione = 30; // Percorso inibito
               TRACESTRING("Coppia O/D Inibita : "+STRINGA(Id1) + "->" +STRINGA(Id2));
               break;
            } else if(Ecz.Id2 == 0 ){
               TipoCorrezione = 30; // Percorso inibito
               TRACESTRING("Stazione inibita : "+STRINGA(Id1));
               break;
            }
         }
//<<< for  int i = minI; i < maxI ;i++
      }
//<<< if TipoCorrezione == 0 &&
   }

   // Inibizione di coppie O/D a gruppi
   if(TipoCorrezione == 0
      && Grafo.EccezioniNormativa->StazioniDiGruppoInibiz.Contiene(Id1)
      && Grafo.EccezioniNormativa->StazioniDiGruppoInibiz.Contiene(Id2)
   ){
      int minI,maxI;
      Grafo.EccezioniNormativa->DeterminaIdx(6, minI, maxI);
      ARRAY_ID Gruppi1;
      ARRAY_ID Gruppi2;
      for (int i = minI; i < maxI ;i++ ) {
         ECCEZ_NORMATIVA & Ecz = (*Grafo.EccezioniNormativa)[i];
         if(Ecz.Id1 == Id1)Gruppi1 += Ecz.Id2;
         if(Ecz.Id1 == Id2)Gruppi2 += Ecz.Id2;
      }
      FORALL(Gruppi1, i1){
         FORALL(Gruppi2, i2){
            if (
               (Gruppi1[i1] % 1000) == Gruppi2[i2] ||
               (Gruppi2[i2] % 1000) == Gruppi1[i1]
            ) {
               TipoCorrezione = 30; // Percorso inibito
               TRACESTRING("Coppia O/D Inibita : "+STRINGA(Id1) + "->" +STRINGA(Id2));
               break;
            } /* endif */
         }
         if(TipoCorrezione)break;
      }
//<<< if TipoCorrezione == 0
   }

   // Anche la sostituzione di percorso va fatta sempre
   if(!TipoCorrezione){
      ORD_FORALL(From,f){
         ID Idf = From[f];
         if(!Grafo.EccezioniNormativa->GrandiCorrezioni.Test(Idf))continue;
         int minI,maxI;
         Grafo.EccezioniNormativa->DeterminaIdx(4, minI, maxI);
         for (int i = minI; i < maxI ;i++ ) {
            ECCEZ_NORMATIVA Ecz = (*Grafo.EccezioniNormativa)[i];
            if(Ecz.Id1 == Idf){
               if(From.Dim() < f + Ecz.DimPercorso ) continue;
               if(f == 0 ){
                  if(From.Dim() == Ecz.DimPercorso){
                     if(!Ecz.ValeEsatto) continue;
                  } else {
                     ECCEZ_NORMATIVA Ecz2 = (*Grafo.EccezioniNormativa)[i+1];
                     if(From[Ecz.DimPercorso] == Ecz2.Percorso[Ecz2.DimPercorso-2] ) continue; // Altrimenti genererei un punto di inversione
                     if(!Ecz.ValeInizio) continue;
                  }
               } else {
                  ECCEZ_NORMATIVA Ecz2 = (*Grafo.EccezioniNormativa)[i+1];
                  if(From[f-1] == Ecz2.Percorso[1] ) continue; // Altrimenti genererei un punto di inversione
                  if(f == (From.Dim() - Ecz.DimPercorso)){
                     if(!Ecz.ValeFine) continue;
                  } else {
                     if(From[f+Ecz.DimPercorso] == Ecz2.Percorso[Ecz2.DimPercorso-2] ) continue; // Altrimenti genererei un punto di inversione
                     if(!Ecz.ValeInMezzo) continue;
                  }
               }
               ID * Corrente = &From[f];
               ID * Perc = Ecz.Percorso;
               for (int j = 0; j < Ecz.DimPercorso ; j++ ) {
                  if(*Corrente != *Perc)break;
                  Corrente ++; Perc ++;
               } /* endfor */
               if( j ==  Ecz.DimPercorso ){
                  (*Grafo.EccezioniNormativa)[i]; // Mi riposiziono
                  TipoCorrezione = 40; // Ok il percorso deve essere corretto
                  Idx = f;
               }
//<<<       if Ecz.Id1 == Idf
            } else if(Ecz.Id2 == Idf){
               if(!Ecz.ValeInverso) continue;
               if(From.Dim() < f + Ecz.DimPercorso ) continue;
               if(f == 0 ){
                  if(From.Dim() == Ecz.DimPercorso){
                     if(!Ecz.ValeEsatto) continue;
                  } else {
                     ECCEZ_NORMATIVA Ecz2 = (*Grafo.EccezioniNormativa)[i+1];
                     if(From[Ecz.DimPercorso] == Ecz2.Percorso[1] ) continue; // Altrimenti genererei un punto di inversione
                     if(!Ecz.ValeInizio) continue;
                  }
               } else {
                  ECCEZ_NORMATIVA Ecz2 = (*Grafo.EccezioniNormativa)[i+1];
                  if(From[f-1] == Ecz2.Percorso[Ecz2.DimPercorso-2] ) continue; // Altrimenti genererei un punto di inversione
                  if(f == (From.Dim() - Ecz.DimPercorso)){
                     if(!Ecz.ValeFine) continue;
                  } else {
                     if(From[f+Ecz.DimPercorso] == Ecz2.Percorso[1] ) continue; // Altrimenti genererei un punto di inversione
                     if(!Ecz.ValeInMezzo) continue;
                  }
               }
               ID * Corrente = &From[f]+Ecz.DimPercorso - 1;
               ID * Perc = Ecz.Percorso;
               for (int j = 0; j < Ecz.DimPercorso ; j++ ) {
                  if(*Corrente != *Perc)break;
                  Corrente --; Perc ++;
               } /* endfor */
               if( j ==  Ecz.DimPercorso ){
                  (*Grafo.EccezioniNormativa)[i]; // Mi riposiziono
                  TipoCorrezione = 41; // Ok il percorso deve essere corretto
                  Idx = f;
               }
//<<<       if Ecz.Id1 == Idf
            } /* endif */

            if(TipoCorrezione)break;
//<<<    for  int i = minI; i < maxI ;i++
         }
         if(TipoCorrezione)break;
//<<< ORD_FORALL From,f
      }
//<<< if !TipoCorrezione
   }

   // Effettuo la correzione
   int Rc = 0;
   if (TipoCorrezione == 30) {  // Pura inibizione
      Rc = 2;
      //#ifdef DBG12
      TRACESTRING("ECCEZIONE: Inibizione del percorso "+From.ToStringa(Stazioni)(0,100));
      //#endif
      NodiCorretti.Clear();
      NumeroCorrezioni ++ ;
   } else if (TipoCorrezione ) {
      Rc = 1;
      //#ifdef DBG12
      TRACESTRING("ECCEZIONE: Correzione del percorso: "+From.ToStringa(Stazioni)(0,100)+" Tipo="+STRINGA(TipoCorrezione)+VRS(Idx));
      //#endif
      NumeroCorrezioni ++ ;
      ARRAY_ID PercorsoCorretto;
      ECCEZ_NORMATIVA Ecz = Grafo.EccezioniNormativa->RecordCorrente();
      //#ifdef DBG12
      Ecz.Trace("Dettagli eccezione:");
      //#endif
      if (TipoCorrezione == 10) {
         // Debbo eliminare la doppia tratta
         int Point = Idx - Ecz.DimPercorso + 1; // Indice dell' ultima stazione da inserire
         assert( Point >= 0 );
         for (int j = 0; j <= Point ; j++) PercorsoCorretto += From[j];
         // Adesso salto la parte duplicata ed aggiungo l' eventuale parte finale del percorso
         for (j += 2* (Ecz.DimPercorso -1); j < From.Dim() ; j++) PercorsoCorretto += From[j];
      } else if (TipoCorrezione == 20) {
         // Debbo eliminare la tratta iniziale
         for (int j = Ecz.Id2; j < From.Dim() ; j++) PercorsoCorretto += From[j];
      } else if (TipoCorrezione == 21) {
         // Debbo eliminare la tratta finale
         for (int j = 0 ; j < From.Dim() - Ecz.Id2; j++) PercorsoCorretto += From[j];
      } else if (TipoCorrezione == 40) {
         Grafo.EccezioniNormativa->Next();
         ECCEZ_NORMATIVA Ecz2 = Grafo.EccezioniNormativa->RecordCorrente();
         #ifdef DBG12
         Ecz2.Trace("Dettagli percorso sostitutivo:");
         #endif
         // Debbo sostituire parte del percorso
         for (int j = 0;j < Idx ;j ++ ) PercorsoCorretto += From[j];
         for (j = 0;j < Ecz2.DimPercorso ;j ++ ) PercorsoCorretto += Ecz2.Percorso[j];
         for (j = Idx + Ecz.DimPercorso;j < From.Dim() ;j ++ ) PercorsoCorretto += From[j];
//<<< if  TipoCorrezione == 10
      } else if (TipoCorrezione == 41) {
         Grafo.EccezioniNormativa->Next();
         ECCEZ_NORMATIVA Ecz2 = Grafo.EccezioniNormativa->RecordCorrente();
         #ifdef DBG12
         Ecz2.Trace("Dettagli percorso sostitutivo:");
         #endif
         // Debbo sostituire parte del percorso
         for (int j = 0;j < Idx ;j ++ ) PercorsoCorretto += From[j];
         for (j = Ecz2.DimPercorso - 1 ; j >= 0 ;j-- ) PercorsoCorretto += Ecz2.Percorso[j];
         for (j = Idx + Ecz.DimPercorso;j < From.Dim() ;j ++ ) PercorsoCorretto += From[j];
      } /* endif */
      NodiCorretti = PercorsoCorretto;
      //#if defined(DBG2P) || defined(DBG12)
      NodiCorretti.Trace(Grafo,"Percorso Corretto ++++++++++++");
      //#endif
//<<< if  TipoCorrezione == 30     // Pura inibizione
   } /* endif */

   return Rc;
//<<< int  PERCORSO_GRAFO::FaCorrezione
};
//----------------------------------------------------------------------------
// PERCORSO_GRAFO::LunghezzaTariffabile()
//----------------------------------------------------------------------------
DATI_TARIFFAZIONE & PERCORSO_GRAFO::LunghezzaTariffabile(BYTE Modalita){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::LunghezzaTariffabile()"

   GRAFO & Grafo = *GRAFO::Grafo;
   PERCORSO_INSTRADAMENTI * OldIstr = Istr;

   //#ifdef DBG2P
   Trace("Percorso grafo da valorizzare Siamo in PERCORSO_GRAFO::LunghezzaTariffabile in MM_GRAFO.CPP");
   //#endif

   if (Istr == NULL) { // Debbo ancora calcolare i dati di instradamento
      #ifdef DBG2
      TRACESTRING("Creazione di un nuovo PERCORSO_INSTRADAMENTI");
      #endif
      Istr = PERCORSO_INSTRADAMENTI::CreaPercorso();
   } /* endif */

   // Debbo eventualmente valorizzare l' instradamento
   if (Istr->Stato == PERCORSO_INSTRADAMENTI::NON_VALORIZZATO) {
      #ifdef DBG2
      TRACESTRING("Valorizzo il PERCORSO_INSTRADAMENTI");
      #endif

      int TipoCorrezione = 0, Rc1 = 0;
      do { Rc1 = FaCorrezione(); Top(TipoCorrezione ,Rc1); } while (Rc1 == 1 );
      if (TipoCorrezione == 2) {  // Pura inibizione
         Istr->Valorizza(THIS);
         Istr->DatiTariffazione.Stato = DATI_TARIFFAZIONE::TARIFFA_VALIDA_SOLO_PER_INFORMATIVA;
         Istr->Stato = PERCORSO_INSTRADAMENTI::NON_TARIFFABILE;
      } else if (TipoCorrezione  == 1 ) { // Vera correzione
         PERCORSO_GRAFO Perc2(NodiCorretti);
         Istr->Valorizza(Perc2);
         Istr->Origine      = Nodi[0];
         Istr->Destinazione = Nodi.Last();
      } else {
         Istr->Valorizza(THIS);
      } /* endif */
   } else {
      Istr->Valorizza(THIS);
//<<< if  Istr->Stato == PERCORSO_INSTRADAMENTI::NON_VALORIZZATO
   }

   // Debbo vedere se l' instradamento creato e' superfluo
   // Solo se Modalita' > 0 ( Cioe' escludo i viaggi liberi)
   if(OldIstr == NULL){
      BOOL Superfluo = FALSE;
      if(Modalita) ORD_FORALL(Grafo.Instradamenti,i){
         if (*Grafo.Instradamenti[i] == *Istr ) {
            delete Istr;
            Istr = Grafo.Instradamenti[i];
            Superfluo = TRUE;
            #ifdef DBG2
            TRACESTRING("Il percorso grafo e' equivalente ad un percorso grafo precedente");
            #endif
            break;
         } /* endif */
      }
      if (!Superfluo) {
         Grafo.Instradamenti += Istr;
      } /* endif */
   }

   #ifdef DBG2
   Istr->DatiTariffazione.Trace("DatiTariffazione:");
   #endif

   TRACESTRING("Esco da LunghezzaTariffabile");

   return Istr->DatiTariffazione;
//<<< DATI_TARIFFAZIONE & PERCORSO_GRAFO::LunghezzaTariffabile BYTE Modalita
};
//----------------------------------------------------------------------------
// PERCORSO_GRAFO::DeterminaFerrovieConcesseUtilizzate
//----------------------------------------------------------------------------
// Questo metodo determina QUALI ferrovie concesse sono state utilizzate per il percorso grafo.
// Puo' tornare un ID per indicare che , causa un collegamento diretto tra due ferrovie concesse,
// non e' possibile tariffare il percorso (l' ID e' quello della stazione di passaggio
// tra le due ferrovie concesse )
ID PERCORSO_GRAFO::DeterminaFerrovieConcesseUtilizzate(ARRAY_ID & FerrovieConcesse){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::DeterminaFerrovieConcesseUtilizzate"
   FerrovieConcesse.Clear();
   ID Id = 0;
   ID LastConcessa = 0;
   for (int i = Nodi.Dim()-1;i > 0  ;i-- ) {
      ID IdRamo;
      int Km;
      if (GRAFO::Gr().IdentificaRamo(Nodi[i-1],Nodi[i],IdRamo,Km)){
         if(GRAFO::Gr().Rami[IdRamo].Concesso){
            #if defined(DBG6) || defined(DBG9)
            TRACESTRING("Ramo "+STRINGA(Nodi[i-1])+"=>"+STRINGA(Nodi[i])+" CodConcessa = "+STRINGA(GRAFO::Gr().Rami[IdRamo].CodConcessa));
            #endif
            ID CodConcessa = GRAFO::Gr().Rami[IdRamo].CodConcessa;
            if(CodConcessa != LastConcessa){
               if(!FerrovieConcesse.Contiene(CodConcessa)) FerrovieConcesse += CodConcessa;
               if(LastConcessa && ! GRAFO::Gr()[Nodi[i]].StazioneFS){
                  Id = Nodi[i]; // Cambio diretto tra ferrovie concesse
               }
               LastConcessa = CodConcessa;
            }
         } else {
            LastConcessa = 0;
         }
      } else {
         LastConcessa = 0;
      } /* endif */
//<<< for  int i = Nodi.Dim  -1;i > 0  ;i--
   } /* endfor */
   // Le tratte urbane di collegamento tra stazione di transito e ferrovie concesse hanno una gestione
   // particolare. Sono considerate concesse (e quindi attivate solo se gestisco le ferrovie
   // concesse) ma appartenenti alla rete FS ( e quindi non hanno un codice di ferrovia concessa
   // e non debbono essere disabilitate selettivamente).
   // Per gestire questa logica aggiungo alle ferrovie concesse abilitate la ferrovia
   // con codice 0 (FS) .
   if (FerrovieConcesse.Dim()) { // Se le ferrovie concesse sono in generale abilitate
      FerrovieConcesse += 0;
   } /* endif */
   return Id;
//<<< ID PERCORSO_GRAFO::DeterminaFerrovieConcesseUtilizzate ARRAY_ID & FerrovieConcesse
};
//----------------------------------------------------------------------------
// PERCORSO_GRAFO::DeterminaStazioniVincolanti()
//----------------------------------------------------------------------------
BOOL PERCORSO_GRAFO::DeterminaStazioniVincolanti(
   ARRAY_ID & IdxVincoliOrigine,ARRAY_ID & IdxVincoliDestinazione){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::DeterminaStazioniVincolanti()"


   IdxVincoliOrigine.Clear();
   IdxVincoliDestinazione.Clear();

   // Se ho due sole stazioni NON vi sono mai stazioni vincolanti (appartengono
   // ovviamente allo stesso ramo)
   if(Nodi.Dim() <= 2)return TRUE;

   int IdxVincolo = 0;
   do {
      IdxVincolo = VincolanteOrigine(IdxVincolo);
      if (IdxVincolo > 0) {
         IdxVincoliOrigine += IdxVincolo;
      } /* endif */
      if(IdxVincoliOrigine.Dim() > 100){
         ERRSTRING("LOOP IRRIMEDIABILE probabile abend");
         Trace("Percorso in errore",1);
         BEEP;
         return FALSE;
      };
   } while ( IdxVincolo > 0 ); /* enddo */

   IdxVincolo = Nodi.Dim() - 1;
   do {
      IdxVincolo = VincolanteDestinazione(IdxVincolo);
      if (IdxVincolo > 0) {
         IdxVincoliDestinazione += IdxVincolo;
      } /* endif */
      if(IdxVincoliDestinazione.Dim() > 100){
         ERRSTRING("LOOP IRRIMEDIABILE probabile abend");
         Trace("Percorso in errore",1);
         BEEP;
         return FALSE;
      };
   } while ( IdxVincolo > 0 ); /* enddo */
   IdxVincoliDestinazione.Mirror();
   #ifdef DBG5
   TRACEVSTRING2(IdxVincoliDestinazione.ToStringa());
   TRACEVSTRING2(IdxVincoliOrigine.ToStringa());
   BOOL AggiuntiVincoliPerStazioniDiTransito = FALSE;
   #endif

   if(IdxVincoliOrigine.Dim() != IdxVincoliDestinazione.Dim()){
      ERRSTRING("Inconsistente dimensione instradamenti");
      ERRINT("IdxVincoliOrigine.Dim()",IdxVincoliOrigine.Dim());
      ERRINT("IdxVincoliDestinazione.Dim()",IdxVincoliDestinazione.Dim());
      Trace("Percorso in errore",1);
      BEEP;
      return FALSE;
   } /* endif */

   // Adesso introduco delle correzioni:
   // - Le stazioni di transito sono vincolanti (nei due sensi)
   // - I punti di inversioni dovrebbero essere vincolanti (ma potrebbero non esserlo se a 0 Km da altre stazioni)
   GRAFO & Grafo = GRAFO::Gr();
   ORD_FORALL( Nodi,Idx){
      // Controllo punto di inversione: Solo se il trace e' abilitato
      if (trchse && IsPuntoDiInversione(Idx) ) {
         WORD Pos = IdxVincoliOrigine.Posizione(Idx);
         if(Pos == WORD_NON_VALIDA || IdxVincoliDestinazione[Pos] != Idx){
            ERRSTRING("ERRORE: Punto di inversione ID= "+STRINGA(Nodi[Idx])+" Idx = "+STRINGA(Idx)+" non e' strettamente vincolante "+ Nodi.ToStringa());
            BEEP;
         }
      } /* endif */
      BOOL Transito = FALSE;
      if( Grafo[Nodi[Idx]].DiTransito && Idx > 0 && Idx < (Nodi.Dim() - 1)){
         // Per vedere se e' effettivamente un transito:
         // - Il ramo precedente e' FS ed il successivo no
         // - oppure appartengono a due differenti concesse
         // - La stazione e' una stazione di imbarco Tirrenia (escludo i transiti non di imbarco)
         ID Ramo1, Ramo2;
         int Km;
         Transito = Grafo.IdentificaRamo(Nodi[Idx- 1], Nodi[Idx], Ramo1, Km);
         Transito &= Grafo.IdentificaRamo(Nodi[Idx], Nodi[Idx+1], Ramo2, Km);
         if(Transito){
            int Soc1  = Grafo.Rami[Ramo1].CodConcessa;
            int Soc2  = Grafo.Rami[Ramo2].CodConcessa;
            STAZ_FS Stz = Grafo[Nodi[Idx]];
            if(Soc1 == 98 || Soc2 == 98){
               Transito = Stz.Concesse()[0].CCRCumulativo != 0;
               Transito &= Stz.Concesse()[0].SocietaConcessa == 98;
            } else if(Soc1 != 0 || Soc2 != 0){
               Transito = Stz.StazioneFS &&
               (
                  (Soc1 != Soc2 )
                  || !Grafo[Nodi[Idx+1]].StazioneFS
                  || !Grafo[Nodi[Idx-1]].StazioneFS
               );
            } else {
               Transito = FALSE;
            }

         }
//<<< if  Grafo Nodi Idx  .DiTransito && Idx > 0 && Idx <  Nodi.Dim   - 1
      }
      if(Transito){
         // E' una stazione di transito
         #ifdef DBG5
         AggiuntiVincoliPerStazioniDiTransito = TRUE;
         TRACESTRING("P0: Identificata stazione di transito" VRS(Idx));
         #endif
         if (IdxVincoliOrigine.Dim() == 0) {
            // Debbo aggiungere il vincolo
            TRACESTRING("P1:Aggiungo come stazione vincolante la stazione di transito" VRS(Nodi[Idx]));
            IdxVincoliOrigine      += Idx;
            IdxVincoliDestinazione += Idx;
         } else {
            BOOL Done = FALSE;
            ORD_FORALL( IdxVincoliOrigine, i3 ){
               ID IdxO = IdxVincoliOrigine[i3];
               ID IdxD = IdxVincoliDestinazione[i3];
               if (IdxD <= Idx) {
                  if (IdxO >= Idx) { // Puo' essere sostitutivo
                     TRACESTRING("Impongo come stazione vincolante la stazione di transito" VRS(Idx));
                     IdxVincoliOrigine[i3] = Idx;
                     IdxVincoliDestinazione[i3]= Idx;
                     Done = TRUE;
                     break;
                  } /* endif */
               } else {  // Debbo aggiungere il vincolo
                  TRACESTRING("P2:Aggiungo come stazione vincolante la stazione di transito" VRS(Idx));
                  IdxVincoliOrigine.Insert(Idx,i3);
                  IdxVincoliDestinazione.Insert(Idx,i3);
                  Done = TRUE;
                  break;
               } /* endif */
            }
            if (!Done) {
               // Debbo aggiungere il vincolo
               TRACESTRING("P3:Aggiungo come stazione vincolante la stazione di transito" VRS(Idx));
               IdxVincoliOrigine      += Idx;
               IdxVincoliDestinazione += Idx;
            }
//<<<    if  IdxVincoliOrigine.Dim   == 0
         } /* endif */
//<<< if Transito
      } /* endif */
//<<< ORD_FORALL  Nodi,Idx
   }
   #ifdef DBG5
   if( AggiuntiVincoliPerStazioniDiTransito ){
      TRACESTRING("Vincoli dopo le correzioni per stazioni di transito");
      TRACEVSTRING2(IdxVincoliDestinazione.ToStringa());
      TRACEVSTRING2(IdxVincoliOrigine.ToStringa());
   }
   #endif
   #ifdef BOUND_CHECK
   ORD_FORALL(IdxVincoliOrigine , IdV){
      assert(IdxVincoliOrigine[IdV] >=  IdxVincoliDestinazione[IdV]);
   }
   #endif

   return TRUE;
//<<< BOOL PERCORSO_GRAFO::DeterminaStazioniVincolanti
};
//----------------------------------------------------------------------------
// int  PERCORSO_GRAFO::VincolanteOrigine()
//----------------------------------------------------------------------------
// Questa mi dice se in un dato tratto un percorso segue il minimum path
// Se si' torna -1
// Se no torna l' indice del nodo vincolante verso l' origine
// N.B. : Lavora su percorsi NORMALIZZATI
// N.B. : Le stazioni di TRANSITO sono comunque considerate vincolanti
// N.B. : I punti di inversione > 0 Km sono comunque considerati vincolanti
//----------------------------------------------------------------------------
int PERCORSO_GRAFO::VincolanteOrigine(WORD IdxOrig){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::VincolanteOrigine()"


   #if defined(DBG6) || defined(DBG9)
   TRACEVLONG(IdxOrig);
   #endif

   int Pos2 = Nodi.Dim() - 1;
   if (IdxOrig >= Pos2-1)return -1;
   GRAFO & Grafo = *GRAFO::Grafo;
   ID Orig = Nodi[IdxOrig];
   #ifdef DBG9
   TraceShortest(Nodi[Pos2],Orig);
   #endif
   BOOL PRIVILEGIARAMODIRETTO = PrivilegiaRamoDiretto ;
   PrivilegiaRamoDiretto = FALSE;
   Grafo.Risolvi(Nodi[Pos2],Orig, Len()+1);
   DIST Limite =  Grafo.Limite;
   BYTE Prog   = Grafo.ProgressivoCorrente;
   int PosNodoVincolante = -1;
   ID   Corr=Nodi[IdxOrig] , Next = Nodi[IdxOrig+1];
   Grafo.EstendiRisolvi(Next);
   #ifdef DBG9
   TRACESTRING( VRS(Limite) + VRS(Prog) + VRS(Len()));
   TRACESTRING("Nodo "+STRINGA(Next)+" Punta a "+STRINGA(Grafo[Next].Nodo1)+VRS(Corr) +VRS(Grafo[Next].Distanza1) );
   #endif
   // Questo test solo per gestire i casi particolari in cui un ramo non segue la via piu' breve, altrimenti fare un assert
   if(Grafo[Next].Distanza1 != Grafo[Next].DistDaId(Corr)){
      #ifdef DBG9
      TRACESTRING("Punto di inversione o caso particolare: Ramo non segue la via piu' breve");
      #endif
      PosNodoVincolante = IdxOrig+1 ;
   } else for (WORD i = IdxOrig+1 ; i < Pos2 ; i ++) {
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(22);
      #endif
      Corr = Next; Next = Nodi[i+1];
      //      if(IsPuntoDiInversione(i) && Grafo.DistanzaTra(Corr,Next) > 0){
      if(IsPuntoDiInversione(i) ){
         PosNodoVincolante = i;
         break;
      }
      STAZ_FS & NextNodo = Grafo[Next];
      STAZ_FS & ThisNodo = Grafo[Corr];
      Grafo.EstendiRisolvi(Next);
      WORD NextDist = ThisNodo.Distanza1 + NextNodo.DistDaId(Corr);
      if( NextNodo.Progressivo != Prog ||
         NextNodo.Distanza1 > Limite ){ // E' piu' distante di Pos2! Il caso e' infrequente ma non raro
         #ifdef DBG9
         TRACESTRING("Ripetizione risolvi per nodo "+STRINGA(Next) +VRS(NextNodo.Progressivo) +VRS(Prog) +VRS(NextNodo.Distanza1) +VRS(Limite) + VRS(Len()) );
         TraceShortest(Next,Orig);
         #endif
         Grafo.Risolvi(Next,Orig,Len() + 1);
         Limite = Grafo.Limite;
         Prog   = Grafo.ProgressivoCorrente;
      } /* endif */
      #ifdef DBG9
      TRACESTRING("Nodo "+STRINGA(Next)+" Punta a "+STRINGA(NextNodo.Nodo1)+VRS(Corr) +VRS(NextNodo.Distanza1) +VRS(NextDist) );
      #endif
      // E' vincolante se non sono sul cammino minimo ne su un cammino minimo degenere
      if(NextNodo.Nodo1 != Corr){
         // Per essere degenere deve avere la stessa distanza e lo stesso numero di ferrovie concesse
         BOOL Degenere = NextNodo.Distanza1 == NextDist && NextNodo.Grf.NumCum == (ThisNodo.Grf.NumCum + NextNodo.ConcDaId(Corr));
         // Per gestire i punti di instradamento eccezionale e punti di inversione multipli:
         // Controllo di non essere comunque sullo stesso ramo
         ID IdRamo1,IdRamo2;
         int Km1,Km2;
         BOOL Eccezione;
         Eccezione  = Grafo.IdentificaRamo(Corr, Next, IdRamo1, Km1);
         Eccezione &= Grafo.IdentificaRamo(NextNodo.Nodo1,Next, IdRamo2, Km2);
         Eccezione &= IdRamo1 == IdRamo2;
         Eccezione &= Km1*Km2 > 0;
         #ifdef DBG9
         TRACESTRING(VRS(i) +VRS(NextNodo.Nodo1)+VRS(Corr) +VRS(NextNodo.Distanza1) +VRS(NextDist) );
         TRACESTRING(VRS(NextNodo.Grf.NumCum) +VRS(ThisNodo.Grf.NumCum) +VRS(NextNodo.ConcDaId(Corr)) );
         TRACESTRING(VRS(Degenere) + VRS(Eccezione));
         #endif
         if(!(Degenere | Eccezione)){
            PosNodoVincolante = i;
            break;
         }
//<<< if NextNodo.Nodo1 != Corr
      };
//<<< if Grafo Next .Distanza1 != Grafo Next .DistDaId Corr
   } /* endfor */
   #if defined(DBG6) || defined(DBG9)
   TRACEVLONG(PosNodoVincolante);
   if(PosNodoVincolante > 0)Grafo.TraceId("Nodo vincolante: ",Corr);
   #endif
   PrivilegiaRamoDiretto = PRIVILEGIARAMODIRETTO ;
   return PosNodoVincolante;
//<<< int PERCORSO_GRAFO::VincolanteOrigine WORD IdxOrig
};
//----------------------------------------------------------------------------
// int  PERCORSO_GRAFO::VincolanteDestinazione()
//----------------------------------------------------------------------------
// Questa mi dice se in un dato tratto un percorso segue il minimum path
// Se si' torna -1
// Se no torna l' indice del nodo vincolante verso la Destinazione
// N.B. : Le stazioni di TRANSITO sono comunque considerate vincolanti
//----------------------------------------------------------------------------
int PERCORSO_GRAFO::VincolanteDestinazione(WORD IdxDest){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_GRAFO::VincolanteDestinazione()"


   #if defined(DBG6) || defined(DBG9)
   TRACEVLONG(IdxDest);
   #endif
   if (IdxDest <= 1)return -1;
   GRAFO & Grafo = *GRAFO::Grafo;
   ID Dest = Nodi[IdxDest];
   #ifdef DBG9
   TraceShortest(Nodi[0],Dest);
   #endif
   BOOL PRIVILEGIARAMODIRETTO = PrivilegiaRamoDiretto ;
   PrivilegiaRamoDiretto = FALSE;
   Grafo.Risolvi(Nodi[0],Dest, Len()+1);
   DIST Limite =  Grafo.Limite;
   int PosNodoVincolante = -1;
   BYTE Prog = Grafo.ProgressivoCorrente;
   ID Corr=Nodi[IdxDest],Next = Nodi[IdxDest-1];
   Grafo.EstendiRisolvi(Next);
   #ifdef DBG9
   TRACESTRING( VRS(Limite) + VRS(Prog) + VRS(Len()));
   TRACESTRING("Nodo "+STRINGA(Next)+" Punta a "+STRINGA(Grafo[Next].Nodo1)+VRS(Corr) +VRS(Grafo[Next].Distanza1) );
   #endif
   // Questo test solo per gestire i casi particolari in cui un ramo non segue la via piu' breve, altrimenti fare l' assert
   if(Grafo[Next].Distanza1 != Grafo[Next].DistDaId(Corr)){
      #ifdef DBG9
      TRACESTRING("Punto di inversione o caso particolare: Ramo non segue la via piu' breve");
      #endif
      PosNodoVincolante = IdxDest-1 ;
   } else for (WORD i = IdxDest-1 ; i >0 ; i --) {
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(22);
      #endif
      Corr = Next; Next = Nodi[i-1];
      //      if(IsPuntoDiInversione(i) && Grafo.DistanzaTra(Corr,Next) > 0){
      if(IsPuntoDiInversione(i) ){
         PosNodoVincolante = i;
         break;
      }
      STAZ_FS & NextNodo = Grafo[Next];
      STAZ_FS & ThisNodo = Grafo[Corr];
      Grafo.EstendiRisolvi(Next);
      WORD NextDist = ThisNodo.Distanza1 + NextNodo.DistDaId(Corr);
      if(NextNodo.Progressivo != Prog ||
         NextNodo.Distanza1 > Limite ){ // E' piu' distante dell' origine! Il caso e' infrequente ma non raro
         #ifdef DBG9
         TRACESTRING("Ripetizione risolvi per nodo "+STRINGA(Next) +VRS(NextNodo.Progressivo) +VRS(Prog) +VRS(NextNodo.Distanza1) +VRS(Limite) + VRS(Len()) );
         TraceShortest(Next,Dest);
         #endif
         Grafo.Risolvi(Next,Dest, Len() +1);
         Limite =  Grafo.Limite;
         Prog = Grafo.ProgressivoCorrente;
      } /* endif */
      #ifdef DBG9
      TRACESTRING("Nodo "+STRINGA(Next)+" Punta a "+STRINGA(NextNodo.Nodo1)+VRS(Corr) +VRS(NextNodo.Distanza1) +VRS(NextDist) );
      #endif
      // E' vincolante se non sono sul cammino minimo ne su un cammino minimo degenere
      if(NextNodo.Nodo1 != Corr){
         // Per essere degenere deve avere la stessa distanza e lo stesso numero di ferrovie concesse
         BOOL Degenere = NextNodo.Distanza1 == NextDist && NextNodo.Grf.NumCum == (ThisNodo.Grf.NumCum + NextNodo.ConcDaId(Corr));
         // Per gestire i punti di instradamento eccezionale e punti di inversione multipli:
         // Controllo di non essere comunque sullo stesso ramo
         ID IdRamo1,IdRamo2;
         int Km1,Km2;
         BOOL Eccezione;
         Eccezione  = Grafo.IdentificaRamo(Corr, Next, IdRamo1, Km1);
         Eccezione &= Grafo.IdentificaRamo(NextNodo.Nodo1,Next, IdRamo2, Km2);
         Eccezione &= IdRamo1 == IdRamo2;
         Eccezione &= Km1*Km2 > 0;
         #ifdef DBG9
         TRACESTRING(VRS(i) +VRS(NextNodo.Nodo1)+VRS(Corr) +VRS(NextNodo.Distanza1) +VRS(NextDist) );
         TRACESTRING(VRS(NextNodo.Grf.NumCum) +VRS(ThisNodo.Grf.NumCum) +VRS(NextNodo.ConcDaId(Corr)) );
         TRACESTRING(VRS(Degenere) + VRS(Eccezione));
         #endif
         if(!(Degenere | Eccezione)){
            PosNodoVincolante = i;
            break;
         }
//<<< if NextNodo.Nodo1 != Corr
      };
//<<< if Grafo Next .Distanza1 != Grafo Next .DistDaId Corr
   } /* endfor */
   #if defined(DBG6) || defined(DBG9)
   TRACEVLONG(PosNodoVincolante);
   if(PosNodoVincolante > 0)Grafo.TraceId("Nodo vincolante: ",Corr);
   #endif
   PrivilegiaRamoDiretto = PRIVILEGIARAMODIRETTO ;
   return PosNodoVincolante;
//<<< int PERCORSO_GRAFO::VincolanteDestinazione WORD IdxDest
};
//----------------------------------------------------------------------------
// @PERCORSO_INSTRADAMENTI
//----------------------------------------------------------------------------
PERCORSO_INSTRADAMENTI::PERCORSO_INSTRADAMENTI (){
   #undef TRCRTN
   #define TRCRTN "@PERCORSO_INSTRADAMENTI"
   Stato = NON_VALORIZZATO;
   ZeroFill(DatiTariffazione);
   MinPartenza         = 9999; // Non valorizzato
   NumPrimeSoluzioni = 0;
}
//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI::CreaPercorso()
//----------------------------------------------------------------------------
PERCORSO_INSTRADAMENTI * PERCORSO_INSTRADAMENTI::CreaPercorso(){ // Sostituisce il costruttore
   #undef TRCRTN
   #define TRCRTN "PERCORSO_INSTRADAMENTI::CreaPercorso()"
   // Il metodo deve creare un oggetto della opportuna sottoclasse
   return new PERCORSO_POLIMETRICHE;
}
//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI::operator ==
//----------------------------------------------------------------------------
// Deve far riferimento alla giusta sottoclasse
BOOL PERCORSO_INSTRADAMENTI::operator ==( const PERCORSO_INSTRADAMENTI & Altro)const{
   #undef TRCRTN
   #define TRCRTN "PERCORSO_INSTRADAMENTI op=="
   PERCORSO_POLIMETRICHE & A = *( PERCORSO_POLIMETRICHE*) this;
   PERCORSO_POLIMETRICHE & B = ( PERCORSO_POLIMETRICHE&) Altro;
   if(A.NumeroTratte != B.NumeroTratte)return FALSE;
   if(A.Origine != B.Origine)return FALSE;
   if(A.Destinazione != B.Destinazione)return FALSE;
   if(memcmp(A.Tratte,B.Tratte,sizeof(PERCORSO_POLIMETRICHE::TRATTA)*A.NumeroTratte))return FALSE;
   return TRUE;
}
//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI::Valorizza()
//----------------------------------------------------------------------------
BOOL PERCORSO_INSTRADAMENTI::Valorizza(ARRAY_ID & StazioniDiInstradamento){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_INSTRADAMENTI::Valorizza()"

   PERCORSO_GRAFO Perc(StazioniDiInstradamento);
   Valorizza(Perc);
   return Stato == TARIFFABILE;
}
//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI::StoreTo()
//----------------------------------------------------------------------------
void PERCORSO_INSTRADAMENTI::StoreTo(BUFR & To){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_INSTRADAMENTI::StoreTo()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: DimBuf = ",To.Dim());
   #endif
   To.Store(VRB(Stato)                     );
   To.Store2((BUFR&)StazioniDiInstradamento);
   To.Store(Origine                        );
   To.Store(Destinazione                   );
   To.Store(MinPartenza                    );
   To.Store(NumPrimeSoluzioni              );
   To.Store(VRB(DatiTariffazione)          );
   #ifdef DBGCACHE
   Trace("Fine: DimBuf = "+STRINGA(To.Dim()));
   #endif
}
//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI::ReStoreFrom()
//----------------------------------------------------------------------------
void PERCORSO_INSTRADAMENTI::ReStoreFrom(BUFR & From){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_INSTRADAMENTI::ReStoreFrom()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: Pointer = ",From.Pointer);
   #endif
   From.ReStore(VRB(Stato)                     );
   From.ReStore2((BUFR&)StazioniDiInstradamento);
   From.ReStore(Origine                        );
   From.ReStore(Destinazione                   );
   From.ReStore(MinPartenza                    );
   From.ReStore(NumPrimeSoluzioni              );
   From.ReStore(VRB(DatiTariffazione)          );
   #ifdef DBGCACHE
   Trace("Fine: Pointer = "+STRINGA(From.Pointer));
   #endif
}
//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI::Trace()
//----------------------------------------------------------------------------
void PERCORSO_INSTRADAMENTI::Trace(const STRINGA& Messaggio, int Livello ){
   #undef TRCRTN
   #define TRCRTN "PER_INSTR::Trace"
   if(Livello > trchse)return;
   TRACESTRING( Messaggio+ STRINGA(":")+
      DatiTariffazione.StatoInChiaro()+
      VRS(Stato) +
      VRS(Origine) +
      StazioniDiInstradamento.ToStringa() +
      VRS(Destinazione) +
      VRS(MinPartenza)+
      VRS(NumPrimeSoluzioni)
   );
};
//----------------------------------------------------------------------------
// PERCORSO_POLIMETRICHE::Trace()
//----------------------------------------------------------------------------
void PERCORSO_POLIMETRICHE::Trace(const STRINGA& Messaggio, int Livello ){
   #undef TRCRTN
   #define TRCRTN "PER_POLI::Trace"
   if(Livello > trchse)return;
   TRACESTRING( Messaggio + STRINGA(":")+VRS(NumeroTratte));
   for (int i1 = 0;i1 < NumeroTratte ;i1++ ) {
      PERCORSO_POLIMETRICHE::TRATTA & Tratta = Tratte[i1];
      char Linea[2048];
      char * L = Linea;
      L += sprintf(L,"Da %s ",(CPSZ)Stazioni[Tratta.IdStazioneIngresso].NomeStazione);
      L += sprintf(L,"A %s",(CPSZ)Stazioni[Tratta.IdStazioneUscita].NomeStazione);
      L += sprintf(L,"  %i Km su Polimetrica %i '%s' ",Tratta.Km,Tratta.IdPolimetrica,
         (*GRAFO::Gr().FilePolim)[Tratta.IdPolimetrica].Nome);
      L += sprintf(L,"%s",(*GRAFO::Gr().FilePolim)[Tratta.IdPolimetrica].Descrizione);
      TRACESTRING(Linea);
   } /* endfor */
};

//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI::DeterminaPuntiDiInstradamento()
//----------------------------------------------------------------------------
// NB Percorso DEVE essere normalizzato
// Determina anche le ferrovie concesse utilizzate
BOOL PERCORSO_INSTRADAMENTI::DeterminaPuntiDiInstradamento(PERCORSO_GRAFO & PercorsoG){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_INSTRADAMENTI::DeterminaPuntiDiInstradamento()"

   static ARRAY_ID IdxOrigine;
   static ARRAY_ID IdxDestinazione;
   IdxOrigine.Clear();
   IdxDestinazione.Clear();

   GRAFO & Grafo = *GRAFO::Grafo;

   BOOL Dump=FALSE;

   StazioniDiInstradamento.Clear();
   VincoliAssoluti.Clear();
   Percorso = PercorsoG.Nodi; // Lo copio (Per pulizia mentale)

   if(!PercorsoG.DeterminaStazioniVincolanti(IdxOrigine,IdxDestinazione)){
      ERRSTRING("Errore determinando le stazioni vincolanti");
      return FALSE;
   } /* endif */

   int MaxVincoli = IdxOrigine.Dim()-1;
   int MaxNodi = Percorso.Dim()-1;
   ORD_FORALL(IdxOrigine,i){
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(29);
      #endif
      // Debbo identificare la stazione di instradamento dominante
      // presente tra le stazioni vincolanti.
      // In via subordinata va bene anche una stazione di diramazione
      // Se nessuna stazione di instradamento e' identificabile
      // vado in regressione sui rami successivi fino ad
      // identificare la stazione chilometricamente piu' vicina.
      // Infine verifico se tale stazione "sostitutiva" identificata
      // sia gia' situata sul cammino minimo tra le altre stazioni vincolanti.
      // Qualora vi siano piu' stazioni di instradamento alternativo preferisco
      // quella piu' vicina al punto medio del percorso (come numero
      // di stazioni nodali) purche' la differenza tra il numero delle
      // stazioni nodali superi 1, altrimenti quella con il rank maggiore

      ID MinVincolo = IdxDestinazione[i] ;
      ID MaxVincolo = IdxOrigine[i] ;

      // Check di consistenza
      if (MaxVincolo < MinVincolo) {
         ERRSTRING("Inconsistente set di vincoli");
         ERRSTRING("QUESTO E' UN ERRORE INTERNO DELL' ALGORITMO");
         ERRSTRING("Vincoli Destinazione:"+IdxDestinazione.ToStringa())
         ERRSTRING("Vincoli Origine:"+IdxOrigine.ToStringa())
         Percorso.Trace(Grafo,"Percorso:",1);
         BEEP;
         return FALSE;
      } /* endif */

      ID StazInstr=0;      // Stazione di instradamento per il vincolo
      ID StazDiram=0;      // Seconda scelta con stazione di diramazione su polimetrica
      ID StazInstr2=0;     // Seconda scelta considerando TUTTE le stazioni valide come punti di instradamento potenziali
      VINCOLO Vincolo, VincoloDiram, Vincolo2;
      // Inizializzo
      ZeroFill(Vincolo);
      Vincolo.Idx1  = MinVincolo; Vincolo.Idx2  = MaxVincolo;
      Vincolo2 = VincoloDiram = Vincolo;
      // Mi limito a prendere il NODO piu' vicino al centro percorso
      WORD Rank   =9999;
      int DistanzaDaCentroPercorso = 9999;
      int DistanzaDiramDaCentroPercorso = 9999;
      // Il punto medio e' calcolato dal vincolo precedente al successivo
      // Si noti che ometto la divisione per due (moltiplico poi tutto per 2)
      int PuntoMedio = (i > 0 ? IdxDestinazione[i-1] : 0) +
      (i < MaxVincoli  ?  IdxOrigine[i+1] : MaxNodi);
      #ifdef DBG5
      TRACESTRING("Vincolo "+STRINGA(i)+" indice punto mediano: "+STRINGA(PuntoMedio/2));
      #endif

      if (PercorsoG.IsPuntoDiInversione(MinVincolo)){

         // MinVincolo deve essere MaxVincolo
         // Check di consistenza
         if (MaxVincolo != MinVincolo){
            ERRSTRING("Inconsistente set di vincoli: punto di inversione "+ STRINGA(Percorso[MinVincolo])+ " non strettamente vincolante");
            ERRSTRING("QUESTO E' UN ERRORE INTERNO DELL' ALGORITMO");
            ERRSTRING("Vincoli Destinazione:"+IdxDestinazione.ToStringa())
            ERRSTRING("Vincoli Origine:"+IdxOrigine.ToStringa())
            Percorso.Trace(Grafo,"Percorso:",1);
            BEEP;
            return FALSE;
         } /* endif */
         STAZ_FS & Stazione =  Grafo[Percorso[MinVincolo]];
         if(Stazione.DiInstradamento) StazInstr =  Stazione.Id;
         if(Stazione.DiDiramazione  ) StazDiram =  Stazione.Id;
         StazInstr2    = Stazione.Id;
         Vincolo.Id    = Stazione.Id;
         Vincolo.Idx   = MinVincolo;
         Vincolo2 = VincoloDiram = Vincolo;
      } else {
         if (PuntoMedio <= 2*IdxDestinazione[i]) {
            Vincolo2.Idx = IdxDestinazione[i];
         } else if (PuntoMedio >= 2*IdxOrigine[i]) {
            Vincolo2.Idx = IdxOrigine[i];
         } else {
            Vincolo2.Idx = PuntoMedio/2;
         } /* endif */
         StazInstr2 = Vincolo2.Id   = Percorso[Vincolo2.Idx];

         ID Last = 0;
         // Estensione ai rami delle stazioni vincolanti estreme
         // si esclude il caso in cui il nodo precedente sia il precedente vincolo
         // (il che puo'' avvenire solo se la stazione precedente e'
         // non nodale, e quindi strettamente vincolante)
         if (IsNodo(Percorso[MinVincolo-1]) &&
               (i == 0 || Percorso[MinVincolo-1] != Percorso[IdxOrigine[i-1]])){
            Last = Percorso[MinVincolo-1];
         }
         for (int j= MinVincolo;j <= MaxVincolo ; j++) {
            #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
            PROFILER::Conta(29);
            #endif
            int DistanzaDaC = abs(2*j - PuntoMedio);
            STAZ_FS & Corrente = Grafo[Percorso[j]];

            // Qui potrei fare il controllo che la stazione NON deve essere un punto di inversione.
            // Pero' mi fido ed evito il controllo ridondante

            if (Corrente.DiInstradamento){
               STAZ_FS::D_INSTR & Instrad = Corrente.Instradamento();
               if (DistanzaDaC < (DistanzaDaCentroPercorso -2) ||
                     (DistanzaDaC <= (DistanzaDaCentroPercorso +2)
                     && Instrad.Rank < Rank)) {
                  Rank =  Instrad.Rank;
                  Vincolo.Idx = j; Vincolo.Offs = 0;
                  StazInstr = Vincolo.Id = Corrente.Id;
                  DistanzaDaCentroPercorso = DistanzaDaC;
               } /* endif */
            } /* endif */

            if (Corrente.DiDiramazione){
               if (DistanzaDaC < (DistanzaDiramDaCentroPercorso -2) ){  // Non considero il Rank
                  StazDiram = VincoloDiram.Id =  Corrente.Id;
                  VincoloDiram.Idx = j; VincoloDiram.Offs = 0;
                  DistanzaDiramDaCentroPercorso = DistanzaDaC;
               } /* endif */
            } /* endif */

            if(Corrente.IsNodo()){
               // Vedo se vi sono altre stazioni di istradamento sul ramo precedente
               if(Last != 0){
                  STAZ_FS::PRIMO_V & Pv = Corrente.PvDaId(Last);
                  if(&Pv != NULL && Pv.Instradamento){ // Ad evitare abend
                     // Scandisco tutte le stazioni del ramo
                     RAMO & Ramo = Grafo.Rami[Pv.IdRamo];
                     // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
                     SCAN_NUM_WIN(Ramo.StazioniDelRamo(),Ramo.NumStazioni,Stazione,RAMO::STAZIONE_R){
                        #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
                        PROFILER::Conta(29);
                        #endif
                        if(Stazione->Instrad){
                           STAZ_FS & StzRam = Grafo[Stazione->Id];
                           STAZ_FS::D_INSTR & Instrad = StzRam.Instradamento();
                           if (DistanzaDaC < (DistanzaDaCentroPercorso -2) ||
                               (DistanzaDaC <= (DistanzaDaCentroPercorso +2)
                                && Instrad.Rank < Rank)) {
                              Rank =  Instrad.Rank;
                              Vincolo.Idx = j-1; Vincolo.Offs = StzRam.ProgRamo;
                              StazInstr = Vincolo.Id = Stazione->Id;
                              DistanzaDaCentroPercorso = DistanzaDaC;
                           } /* endif */
                        }
                     } ENDSCAN ;
                  }
//<<<          if Last != 0
               }
               Last = Corrente.Id;
               // Estensione al ramo della stazione vincolante estrema
               if (j == MaxVincolo && j < MaxNodi &&  IsNodo(Percorso[j+1])) {
                  ID Next = Percorso[j+1];
                  STAZ_FS::PRIMO_V & Pv = Corrente.PvDaId(Next);
                  if(&Pv != NULL && Pv.Instradamento){ // Ad evitare abend
                     // Scandisco tutte le stazioni del ramo
                     RAMO & Ramo = Grafo.Rami[Pv.IdRamo];
                     // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
                     SCAN_NUM_WIN(Ramo.StazioniDelRamo(),Ramo.NumStazioni,Stazione,RAMO::STAZIONE_R){
                        TRACEINT("++ Stazione.Id ", Stazione->Id);
                        #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
                        PROFILER::Conta(29);
                        #endif
                        if(Stazione->Instrad){
                           STAZ_FS & StzRam = Grafo[Stazione->Id];
                           STAZ_FS::D_INSTR & Instrad = StzRam.Instradamento();
                           if (DistanzaDaC < (DistanzaDaCentroPercorso -2) ||
                                (DistanzaDaC <= (DistanzaDaCentroPercorso +2)
                                && Instrad.Rank < Rank)) {
                              Rank =  Instrad.Rank;
                              Vincolo.Idx = j; Vincolo.Offs = StzRam.ProgRamo;
                              StazInstr = Vincolo.Id = Stazione->Id;
                              DistanzaDaCentroPercorso = DistanzaDaC;
                           } /* endif */
                        }
                     } ENDSCAN ;
                  }
//<<<          if  j == MaxVincolo && j < MaxNodi &&  IsNodo Percorso j+1
               } /* endif */
//<<<       if Corrente.IsNodo
            } else if(!Corrente.IsInstrGrafo()) { // Altrimenti avrei segnalazioni a cumulativo disabilitato
               Last = 0; // Cosi' il prossimo nodo non cerca stazioni di instradamento sul ramo.
               // Lascio la segnalazione, ma non BEEP, in quanto potrebbe accadere a volte per stazioni con 0 KM di
               // distanza tra di loro: preferisco tutto sommato non introdurre un vincolo artificioso
               ERRSTRING("WARNING: Stazione (ID="+STRINGA(Corrente.Id)+") non nodale su tratto non strettamente vincolante");
               ERRSTRING("QUESTO POTREBBE ESSERE UN ERRORE INTERNO DELL' ALGORITMO");
               ERRSTRING("E' comunque possibile determinare l' instradamento del percorso");
               ERRSTRING("Vincoli Destinazione:"+IdxDestinazione.ToStringa())
               ERRSTRING("Vincoli Origine:"+IdxOrigine.ToStringa())
               Percorso.Trace(Grafo,"Percorso:",1);
               // BEEP;
            }

//<<<    for  int j= MinVincolo;j <= MaxVincolo ; j++
         } /* endfor */
//<<< if  PercorsoG.IsPuntoDiInversione MinVincolo
      }

      if(StazInstr!=0){
         StazioniDiInstradamento += StazInstr;
         VincoliAssoluti += Vincolo ;
      } else if(StazDiram !=0){
         StazioniDiInstradamento += StazDiram;
         VincoliAssoluti += VincoloDiram ;
      } else if(StazInstr2!=0){
         StazioniDiInstradamento += StazInstr2;
         VincoliAssoluti += Vincolo2 ;
         TRACESTRING("Non riuscito ad identificare una stazione di Instradamento/Diramazione tra le stazioni");
         Grafo.TraceId("",Percorso[MaxVincolo]);
         Grafo.TraceId("",Percorso[MinVincolo]);
         TRACESTRING("UTILIZZATO direttamente instradamento per stazione ");
         Grafo.TraceId("",StazInstr2);
      } else {
         ERRSTRING("Caso non previsto: Errore interno dell' algoritmo");
         Percorso.Trace(Grafo,"Percorso:",1);
         BEEP;
         return FALSE;
      }

//<<< ORD_FORALL IdxOrigine,i
   } /* endfor */

   // Per evitare artefatti dovuti a punti di inversione
   // elimino due eventuali instradamenti contigui identici.

   for (int y = StazioniDiInstradamento.Dim() -2; y >=0 ;y -- ) {
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(29);
      #endif
      if(StazioniDiInstradamento[y] ==StazioniDiInstradamento[y+1]){
         TRACESTRING("Semplificazione doppio instradamento ID="+STRINGA(StazioniDiInstradamento[y]));
         Percorso.Trace(Grafo,"Percorso:");
         StazioniDiInstradamento -= y+1;
         VincoliAssoluti         -= y+1;
      }
   } /* endfor */
   if(StazioniDiInstradamento.Dim() ){
      if( StazioniDiInstradamento [0] == Percorso[0]){
         StazioniDiInstradamento -= 0;
         VincoliAssoluti         -= 0;
      }
   }
   if(StazioniDiInstradamento.Dim() ){
      if(StazioniDiInstradamento[StazioniDiInstradamento.Dim()-1] == Percorso.Last()){
         StazioniDiInstradamento -= StazioniDiInstradamento.Dim() - 1;
         VincoliAssoluti         -= StazioniDiInstradamento.Dim() - 1;
      }
   }
   if(Dump)StazioniDiInstradamento.Trace(GRAFO::Gr(),"Instradamento determinato");

   return TRUE;
//<<< BOOL PERCORSO_INSTRADAMENTI::DeterminaPuntiDiInstradamento PERCORSO_GRAFO & PercorsoG
}
//----------------------------------------------------------------------------
// @PERCORSO_POLIMETRICHE
//----------------------------------------------------------------------------
PERCORSO_POLIMETRICHE::PERCORSO_POLIMETRICHE () : PERCORSO_INSTRADAMENTI(){
   #undef TRCRTN
   #define TRCRTN "@PERCORSO_POLIMETRICHE"
   NumeroTratte = 0;
}


//----------------------------------------------------------------------------
// PERCORSO_POLIMETRICHE::Valorizza()
//----------------------------------------------------------------------------
// NB: Percorso deve gia' essere normalizzato
BOOL PERCORSO_POLIMETRICHE::Valorizza(PERCORSO_GRAFO & PercorsoG){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_POLIMETRICHE::Valorizza()"

   static ARRAY_ID Wrk(64);                      // static per performance
   static ARRAY_ID FerrovieConcesseUtilizzate;   // static per performance
   static ARRAY_ID Stz;                          // static per performance
   static ARRAY_DINAMICA<TRATTA_DI_INSTRADAMENTO>  TratteInstradamento(8);

   if (Stato != NON_VALORIZZATO) return Stato == TARIFFABILE;


   FerrovieConcesseUtilizzate.Clear(); Stz.Clear(); TratteInstradamento.Clear();
   VincoliAssoluti.Clear(); NumeroTratte = 0;
   memset(&DatiTariffazione,0,sizeof(DATI_TARIFFAZIONE));
   GRAFO & Grafo = *GRAFO::Grafo;

   #ifdef DBG2
   TRACEVLONG(Grafo.AbilitaCumulativo);
   #endif

   Origine      = PercorsoG.Nodi[0];
   Destinazione = PercorsoG.Nodi.Last();

   if(!DeterminaPuntiDiInstradamento(PercorsoG)){ // Valorizza Percorso come side effect
      ERRSTRING("Percorso NON tariffabile per mancata identificazione punti di instradamento");
      Stato = NO_PERCORSO_POLIMETRICHE;
      DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRATTE_NON_TARIFFABILI;
      return FALSE;
   }


   #if defined(DBG2) || defined(DBG10)
   StazioniDiInstradamento.Trace(Grafo,"Vincoli Assoluti = Stazioni di instradamento minimali :");
   #endif
   DatiTariffazione.Diretta = StazioniDiInstradamento.Dim() == 0;


   ID CambioAnomaloTraConcesse = 0;
   if (Grafo.AbilitaCumulativo) {
      CambioAnomaloTraConcesse = PercorsoG.DeterminaFerrovieConcesseUtilizzate(FerrovieConcesseUtilizzate);
   } /* endif */

   if(FerrovieConcesseUtilizzate.Dim() > 3) { // 3 e' valido perche' oltre alle 2 ferrovie concesse aggiungo il codice 0 per tratte urbane di collegamento
      Stato = TRE_CUMULATIVO;
      DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRE_CUMULATIVO;
      return FALSE;
   } /* endif */

   if(CambioAnomaloTraConcesse){
      Stato = NON_TARIFFABILE;
      ERRSTRING("Il percorso non e' tariffabile causa cambio anomalo tra concesse alla stazione con ID= "+STRINGA(CambioAnomaloTraConcesse));
      DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRATTE_NON_TARIFFABILI;
      return FALSE;
   } /* endif */

   Stz += Percorso[0];
   Stz += StazioniDiInstradamento;
   Stz += Percorso.Last();

   #ifdef DBG2
   Stz.Trace(Grafo,"Percorso - Instradamento");
   #endif

   // ....................................................
   // Ciclo di identificazione polimetriche
   // ....................................................
   // In questo ciclo identifico, per le tratte comprese tra le stazioni
   // vincolanti identificate, la sequenza di polimetriche da utilizzare.
   // Correggo inoltre le stazioni vincolanti per tener conto delle
   // stazioni di diramazione condizionata.
   // NB: In caso di incoerenza tra grafo e polimetriche potrebbero essere
   // aggiunte ulteriori stazioni vincolanti.
   // Per questo nel ciclo si ritesta ad ogni iterazione Stz.Dim()
   // NB: Per alcuni istradamenti eccezionali ignoro alcuni vincoli
   // alterando al volo la variabile di ciclo "i"
   // Analogamente per riconciliare i percorsi grafo e polimetriche
   // ....................................................
   int AntiLoop = 100;
   for (int IdxVincolo = 1 ; IdxVincolo < Stz.Dim(); IdxVincolo++){
      if((AntiLoop -- ) == 0){
         ERRSTRING("Errore: Loop nell' espansione delle polimetriche");
         BEEP;
         Stato = NON_TARIFFABILE;
         DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRATTE_NON_TARIFFABILI;
         return FALSE;
      } /* endif */
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(23);
      #endif
      // V1 e V2 sono i vincoli della tratta da esplodere
      // V0 e' il precedente vincolo ( = L' ultimo V1)
      VINCOLO V0,V1,V2; ZeroFill(V0);ZeroFill(V1); ZeroFill(V2);
      if (IdxVincolo>1) {
         V1 = VincoliAssoluti[IdxVincolo-2];
      } else {
         V1.Id = Percorso[0];
      } /* endif */
      if (IdxVincolo>2) {
         V0 = VincoliAssoluti[IdxVincolo-3];
      } else if (IdxVincolo==2) {
         V0.Id = Percorso[0];
      } else {
         V0.Id = 0; // Non valido
      } /* endif */
      if (IdxVincolo < Stz.Dim() - 1) {
         V2 = VincoliAssoluti[IdxVincolo-1];
      } else {
         V2.Idx = Percorso.Dim() - 1; V2.Id = Percorso.Last();
      } /* endif */
      ID Da = Stz[IdxVincolo-1];
      ID A  = Stz[IdxVincolo]  ;
      assert(Da == V1.Id && A == V2.Id);
      #ifdef DBG11
      TRACESTRING("Esplosione polimetriche: " VRS(IdxVincolo) + VRS(Da)  + VRS(A) + VRS(NumeroTratte) + VRS(TratteInstradamento.Dim()));
      #endif

      TRATTA_DI_INSTRADAMENTO  TrattaInstr;
      TrattaInstr.Id1         = Da    ;
      TrattaInstr.Id2         = A     ;
      TrattaInstr.Sezionabile = FALSE ;
      TrattaInstr.Km          = 0     ;
      TrattaInstr.Vincolante1 = IdxVincolo > 1 ;

      // Gestione degli instradamenti particolari cumulativi (quelli con stazioni
      // intermedie di instradamento).
      // Li riconosco basandomi  solo sulle stazioni VINCOLANTI
      BOOL InstradamentoParticolareCumulativo = FALSE;
      STAZ_FS & sDa = Grafo[Da];
      if (sDa.NumeroConcesse && Grafo.IstrEccez) {
         // La cerco negli instradamenti eccezionali
         Grafo.IstrEccez->Seek(Da);
         while ( &Grafo.IstrEccez->RecordCorrente() &&
               Grafo.IstrEccez->RecordCorrente().StzVincolante[0] == Da &&
            !InstradamentoParticolareCumulativo) {
            for (int j = 1 ; j < NM_VINC ; j++ ) {
               #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
               PROFILER::Conta(23);
               #endif
               ID Id = Grafo.IstrEccez->RecordCorrente().StzVincolante[j];
               if( Id == 0){ // Ok : Found
                  #ifdef DBG2
                  TRACEINT("Istradamento cumulativo particolare, lunghezza = Km ",Grafo.IstrEccez->RecordCorrente().Km);
                  TRACEVLONG(j);
                  #endif
                  InstradamentoParticolareCumulativo = TRUE;
                  if(j> 2) IdxVincolo += (j-2) ; // Assorbe due o piu' tratte
                  A  = Stz[IdxVincolo]  ;        // Assorbe due o piu' tratte
                  sDa.Plm.Poli1 = Grafo.IstrEccez->RecordCorrente().Polimetrica;
                  sDa.Plm.Verso = Grafo.IstrEccez->RecordCorrente().Verso;
                  sDa.Nodo1     = A; // Questo fa si' che io abbia sempre un' unica tratta sul cumulativo
                  sDa.Distanza1 = Grafo.IstrEccez->RecordCorrente().Km;
                  break;
               } else if( (IdxVincolo+j-1) >= Stz.Dim() || Id != Stz[IdxVincolo+j-1]) {
                  break; // Le stazioni non coincidono: non e' l' instradamento richiesto
               } /* endif */
//<<<       for  int j = 1 ; j < NM_VINC ; j++
            } /* endfor */
            Grafo.IstrEccez->Next(); // Next record
//<<<    while   &Grafo.IstrEccez->RecordCorrente   &&
         } /* endwhile */

//<<< if  sDa.NumeroConcesse && Grafo.IstrEccez
      } /* endif */

      if (!InstradamentoParticolareCumulativo) {
         if(!Grafo.RisolviPolimetriche(Da,A,FerrovieConcesseUtilizzate)){
            // Gestione errore
            ERRSTRING("Non riuscito a collegare le stazioni via polimetriche ");
            Percorso.Trace(Grafo,"Percorso:",1);
            Stz.Trace(Grafo,"Stazioni di instradamento:",1);
            // BEEP;
            Stato = NO_PERCORSO_POLIMETRICHE;
            DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRATTE_NON_TARIFFABILI;
            return FALSE;
         }
      } /* endif */
      // ....................................................
      // Correggo le stazioni vincolanti per tener conto delle
      // stazioni di diramazione condizionata.
      // ....................................................
      if(IdxVincolo > 1 && Grafo[V1.Id].DiDiramazione && !V1.Inibita){
         // Per verificare se il vincolo e' inadatto debbo vedere se la stazione
         // e' una stazione che normalmente non utilizzerei come stazione di
         // diramazione tra le due polimetriche. Anche in tal caso il vincolo
         // potrebbe essere accettabile nel caso non avessi altre potenziali
         // stazioni vincolanti
         // Per evitare loops flaggo il vincolo in modo che la stessa logica
         // non possa essere applicata piu' di una volta.
         ID Id = Tratte[NumeroTratte-1].IdStazioneIngresso ;
         assert(Tratte[NumeroTratte-1].IdStazioneUscita == V1.Id);
         ISTR_COND MaskDiramazioneCondizionata = Grafo[Id].IstrCond();
         MaskDiramazioneCondizionata &= Grafo[V1.Id].Diramazione().IstrCondInibit();
         ID Id2 = sDa.Nodo1 ;
         // Identifico la seconda
         if(MaskDiramazioneCondizionata.Inibizione(Grafo[Id2].IstrCond())){
            #ifdef DBG11
            TRACESTRING("Potenziale inibizione per diramazione condizionata: "+STRINGA(V1.Id)+" "+STRINGA(Stazioni.DecodificaIdStazione(V1.Id)));
            #endif
            // Vedo se ho una alternativa
            if(V1.Idx1 != V1.Idx2){
               assert(V1.Idx1 <= V1.Idx && V1.Idx <= V1.Idx2);
               if( (V1.Idx - V1.Idx1) > (V1.Idx2 - V1.Idx) ){
                  V1.Idx --;
               } else {
                  V1.Idx ++;
               }
               // Sostituisco il vincolo
               V1.Inibita = V1.Id;
               V1.Offs =  0;
               V1.Id = Percorso[V1.Idx];
               Stz[IdxVincolo-1] = V1.Id;
               StazioniDiInstradamento[IdxVincolo-2] = V1.Id;
               VincoliAssoluti[IdxVincolo-2] = V1;
               // Riciclo
               NumeroTratte = 0;
               IdxVincolo = 0;
               TratteInstradamento.Clear();
               #ifdef DBG11
               V1.Trace("Sostituito vincolo per diramazione condizionata con: ");
               Stz.Trace(Grafo,"Nuovo percorso/Vincoli");
               #endif
               Da = A; // Per forzare il riciclo
//<<<       if V1.Idx1 != V1.Idx2
            } /* endif */
//<<<    if MaskDiramazioneCondizionata.Inibizione Grafo Id2 .IstrCond
         } /* endif */
//<<< if IdxVincolo > 1 && Grafo V1.Id .DiDiramazione && !V1.Inibita
      } /* endif */

      // ....................................................
      // Adesso aggiungo le polimetriche trovate alle tratte
      // ....................................................
      TrattaInstr.Pol1        = NumeroTratte ;
      TrattaInstr.Pol2        = NumeroTratte ;
      TrattaInstr.Km          = sDa.Distanza1;
      BOOL First = TRUE;
      while (Da != A) {
         #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
         PROFILER::Conta(23);
         #endif
         STAZ_FS & sDa = Grafo[Da];
         if (NumeroTratte >= MAX_TRATTE_POLIMETRICA) {
            ERRSTRING("Superato il massimo numero di polimetriche gestite");
            Percorso.Trace(Grafo,"Percorso:",1);
            Stz.Trace(Grafo,"Stazioni di instradamento:",1);
            BEEP;
            Stato = TROPPE_POLIMETRICHE;
            DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRATTE_NON_TARIFFABILI;
            return FALSE;
         } /* endif */
         TrattaInstr.Pol2 = NumeroTratte ;
         TRATTA & Tratta = Tratte[NumeroTratte++];
         Tratta.IdPolimetrica      = sDa.Plm.Poli1;
         Tratta.IdStazioneIngresso = Da;
         Tratta.IdStazioneUscita   = sDa.Nodo1;
         Tratta.Km                 = sDa.Distanza1;
         if(!Grafo.Polim[Tratta.IdPolimetrica].Concessa()) TrattaInstr.Sezionabile = TRUE ;
         if(!First){ // Poiche' i Km sono sempre calcolati verso la destinazione di RisolviPolimetriche serve una correzione
            Tratte[NumeroTratte-2].Km -= Tratta.Km;
         }
         Tratta.Verso = sDa.Plm.Verso;
         First = FALSE;
         Da = Tratta.IdStazioneUscita;
         #ifdef DBG11
         IFTRC(Bprintf3("Aggiunta tratta %i Da stazione %i a stazione %i : %i Km, Polimetrica Nø %i", NumeroTratte, Tratta.IdStazioneIngresso , Tratta.IdStazioneUscita , Tratta.Km  , Tratta.IdPolimetrica ));
         #endif

         // ....................................................
         // Logica di riconciliazione
         // ....................................................
         // Controllo che non vi sia incoerenza tra le polimetriche trovate ed il percorso grafo
         // Se vi fosse si attiva una logica di riconciliazione:
         // -  O viene sostituita una delle due stazioni vincolanti
         // -  O viene aggiunta una nuova stazione vincolante intermedia
         // -  Oppure viene deciso che la situazione e' insanabile
         // Nei primi due casi viene invalidata la situazione corrente e si ricomincia daccapo a
         // tradurre l' instradamento in polimetriche.
         // Nel terzo l' azione dipende dall' entita' del problema:
         //  - Se la differenza chilometrica (sul grafo) e' inferiore a 15 chilometri si ignora il problema
         //  - Altrimenti si dichiara il percorso NON TARIFFABILE
         // ....................................................
         // Criterio di congruenza:
         // Se ho una sola polimetrica e' congruente
         // Se ho due polimetriche consecutive eguali non considero la stazione di diramazione
         // Se ho due o piu' polimetriche e tutte le stazioni di
         //    diramazione da una polimetrica alla successiva compaiono
         //    nel percorso grafo originale e' coerente
         // ....................................................
         if (Da != A && Tratte[NumeroTratte].IdPolimetrica  != Tratta.IdPolimetrica && !InstradamentoParticolareCumulativo && RiconciliazioneAbilitata) {
            ID IdToTest = Da; // E' la stazione di cambio verso la prossima polimetrica
            int Incoerenti = ControllaCompatibilitaSuTratta(IdToTest, V1, V2);
            int ReasonCode = (Incoerenti > 1) ? 1 : 0;
            if(Incoerenti){ // Il percorso polimetriche va in tutt' altra direzione rispetto al grafo
               TRACESTRING("Incoerenza tra percorso grafo e percorso polimetriche: Cerco di sanarla");
               RiconciliazioneNecessaria = TRUE;
               VINCOLO Aggiuntivo; ZeroFill(Aggiuntivo);
               #ifdef DBG11
               TRACEVLONG(Tratta.IdPolimetrica   );
               TRACEID(Tratta.IdStazioneIngresso );
               TRACEID(Tratta.IdStazioneUscita   );
               TRACEID(IdToTest                  );
               V1.Trace("V1:");
               V2.Trace("V2:");
               #endif
               int Modo;
               if (ReasonCode > 0) {
                  TRACESTRING("Incoerenza di tipo non sanabile");
               } else if (V2.Idx < V1.Idx) {
                  // Non dovrebbe succedere mai
                  ReasonCode = 2;
               } else {
                  ReasonCode = IdentificaVincoloAggiuntivo(Aggiuntivo,V0,V1,V2,Modo,FerrovieConcesseUtilizzate );
               }
               if(ReasonCode){
                  ERRSTRING("POSSIBILE ERRORE: Non riuscito ad identificare un vincolo aggiuntivo che sani l' incongruenza tra grafo e polimetriche" VRS(ReasonCode));
                  if(ReasonCode < 3)BEEP;
                  Grafo.RisolviPolimetriche(Stz[IdxVincolo-1],Stz[IdxVincolo],FerrovieConcesseUtilizzate); // Necessario per poter continuare l' espansione
                  // Sono in ambasce: penso sia meglio lasciar tariffare comunque il percorso in quanto un' incongruenza non sanabile
                  // vuol dire che comunque non ho un vincolo da aggiungere ....
                  //Stato = NO_PERCORSO_POLIMETRICHE;
                  //DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRATTE_NON_TARIFFABILI;
                  //return FALSE;
               } else if(Modo == 0){
                  // Continuo senza sanare
                  TRACESTRING("Non riuscito a sanare incongruenza minore, ignorata");
                  Grafo.RisolviPolimetriche(Stz[IdxVincolo-1],Stz[IdxVincolo],FerrovieConcesseUtilizzate); // Necessario per poter continuare l' espansione
               } else {
                  STRINGA Msg;
                  switch (Modo) {
                  case 1:
                     Stz.Insert(Aggiuntivo.Id,IdxVincolo);
                     StazioniDiInstradamento.Insert(Aggiuntivo.Id,IdxVincolo-1);
                     VincoliAssoluti.Insert(Aggiuntivo,IdxVincolo-1);
                     Msg = "Inserito vincolo";
                     break;
                  case 2:
                     Stz[IdxVincolo-1] = Aggiuntivo.Id;
                     StazioniDiInstradamento[IdxVincolo-2] = Aggiuntivo.Id;
                     VincoliAssoluti[IdxVincolo-2] = Aggiuntivo;
                     Msg = "Sostituito 1ø vincolo";
                     break;
                  case 3:
                     Stz[IdxVincolo] = Aggiuntivo.Id;
                     StazioniDiInstradamento[IdxVincolo-1] = Aggiuntivo.Id;
                     VincoliAssoluti[IdxVincolo-1] = Aggiuntivo;
                     Msg = "Sostituito 2ø vincolo";
                     break;
                  } /* endswitch */
                  NumeroTratte = 0;
                  IdxVincolo = 0;
                  TratteInstradamento.Clear();
                  #ifdef DBG11
                  Aggiuntivo.Trace(Msg +": Aggiuntivo=");
                  Stz.Trace(Grafo,"Nuovo percorso/Vincoli");
                  #endif
                  Da = A; // Per forzare il riciclo
//<<<          if ReasonCode
               }
//<<<       if Incoerenti   // Il percorso polimetriche va in tutt' altra direzione rispetto al grafo
            } /* endif */
//<<<    if  Da != A && Tratte NumeroTratte .IdPolimetrica  != Tratta.IdPolimetrica && !InstradamentoParticolareCumulativo && RiconciliazioneAbilitata
         } /* endif */
         // ....................................................
         // FINE Controllo coerenza tra polimetriche e grafo e logica di riconciliazione
         // ....................................................
//<<< while  Da != A
      } /* endwhile */

      if(IdxVincolo > 0)TratteInstradamento += TrattaInstr; // Se 0 sto riciclando
//<<< for  int IdxVincolo = 1 ; IdxVincolo < Stz.Dim  ; IdxVincolo++
   }

   #ifdef DBG7
   Trace("Printout polimetriche prima dell' accorpamento");
   #endif


   // ....................................................
   // Cerco di accorpare le polimetriche
   // ....................................................
   BOOL Ok = AccorpaPolimetriche(PercorsoG,TratteInstradamento,FerrovieConcesseUtilizzate);
   if(Ok){
      // Ora passo a calcolare i Km
      #ifdef DBG2
      TRACESTRING("Polimetriche che tariffano la soluzione: ");
      TRACEVLONG(NumeroTratte);
      #endif
      for (int i = 0; i < NumeroTratte ; i ++ ) {
         #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
         PROFILER::Conta(23);
         #endif
         TRATTA & Tratta = Tratte[i];
         #ifdef DBG2
         STRINGA Msg;
         Msg = "Polimetrica ID "+STRINGA(Tratta.IdPolimetrica);
         Msg += " "+STRINGA(Tratta.Km)+" Km ";
         Msg += " da ";
         Msg += Stazioni[Tratta.IdStazioneIngresso].NomeStazione;
         Msg += " a ";
         Msg += Stazioni[Tratta.IdStazioneUscita].NomeStazione;
         TRACESTRING(Msg);
         #endif
         POLIMETRICA & Poli = Grafo.Polim[Tratta.IdPolimetrica];
         switch (Poli.TipoPoli) {
         case POLIMETRICA::CUMULATIVA  :
            if(Tratta.Km > 9000){
               STRINGA Msg;
               Msg = "ERRORE: Polimetrica ID "+STRINGA(Tratta.IdPolimetrica);
               Msg += " Relazione non tariffabile da ";
               Msg += Stazioni[Tratta.IdStazioneIngresso].NomeStazione;
               Msg += " a ";
               Msg += Stazioni[Tratta.IdStazioneUscita].NomeStazione;
               ERRSTRING(Msg);
               Ok = FALSE; // Relazione NON tariffabile
               // Soppresso il controllo perche' potrebbe anche essere cumulativa, se la
               // polimetrica precedente era di collegamento (tipo URBANA_CUM)
               //} else if (!Grafo[Tratta.IdStazioneIngresso].StazioneFS) {
               //   STRINGA Msg;
               //   Msg = "ERRORE: Polimetrica ID "+STRINGA(Tratta.IdPolimetrica);
               //   Msg += " "+STRINGA(Tratta.Km)+" Km ";
               //   Msg += " Stazione 'Di Transito' NON FS ";
               //   Msg += Stazioni[Tratta.IdStazioneIngresso].NomeStazione;
               //   Msg += " verso ";
               //   Msg += Stazioni[Tratta.IdStazioneUscita].NomeStazione;
               //   ERRSTRING(Msg);
               //   Ok = FALSE;
//<<<       if Tratta.Km > 9000
            } else if (DatiTariffazione.KmConcessi1) {
               DatiTariffazione.KmConcessi2     = Tratta.Km;
               DatiTariffazione.CodConcessione2 = Poli.SocietaConcessa;
               DatiTariffazione.CodLinea2       = 0;
               DatiTariffazione.TransitoFS2     = Tratta.IdStazioneIngresso;
            } else {
               DatiTariffazione.KmConcessi1     = Tratta.Km;
               DatiTariffazione.CodConcessione1 = Poli.SocietaConcessa;
               DatiTariffazione.CodLinea1       = 0;
               DatiTariffazione.TransitoFS1     = Tratta.IdStazioneIngresso;
            } /* endif */
            break;
//<<<    switch  Poli.TipoPoli
         case POLIMETRICA::MARE_CUM :
            if(Tratta.Km > 9000){
               STRINGA Msg;
               Msg = "ERRORE: Polimetrica ID "+STRINGA(Tratta.IdPolimetrica);
               Msg += " Relazione non tariffabile da ";
               Msg += Stazioni[Tratta.IdStazioneIngresso].NomeStazione;
               Msg += " a ";
               Msg += Stazioni[Tratta.IdStazioneUscita].NomeStazione;
               ERRSTRING(Msg);
               // BEEP;
               Ok = FALSE; // Relazione NON tariffabile
               // Soppresso il controllo perche' potrebbe anche essere cumulativa, se la
               // polimetrica precedente era di collegamento (tipo URBANA_CUM)
               //} else if (!Grafo[Tratta.IdStazioneIngresso].StazioneFS) {
               //   STRINGA Msg;
               //   Msg = "ERRORE: Polimetrica ID "+STRINGA(Tratta.IdPolimetrica);
               //   Msg += " "+STRINGA(Tratta.Km)+" Km ";
               //   Msg += " Stazione 'Di Transito' NON FS ";
               //   Msg += Stazioni[Tratta.IdStazioneIngresso].NomeStazione;
               //   Msg += " verso ";
               //   Msg += Stazioni[Tratta.IdStazioneUscita].NomeStazione;
               //   ERRSTRING(Msg);
               //   BEEP;
               //   Ok = FALSE;
//<<<       if Tratta.Km > 9000
            } else if (DatiTariffazione.Andata.KmMare > 0) {
               STRINGA Msg;
               Msg = "ERRORE TARIFFAZIONE: Polimetrica ID "+STRINGA(Tratta.IdPolimetrica);
               Msg += " "+STRINGA(Tratta.Km)+" Km ";
               Msg += " Gia' presente un tratto mare cumulativo";
               ERRSTRING(Msg);
               BEEP;        // File sorgente: .\mm_grafo.cpp Linea 3503
               Ok = FALSE;
            } else {
               DatiTariffazione.Andata.IdStazioneSbarco    = Tratta.IdStazioneUscita    ;
               DatiTariffazione.Andata.IdStazioneImbarco   = Tratta.IdStazioneIngresso  ;
               DatiTariffazione.Andata.KmMare              = Tratta.Km ;
               DatiTariffazione.Andata.CodSocietaMare      = Poli.SocietaConcessa ;
               DatiTariffazione.Andata.CodLineaMare        = 0  ;
               DatiTariffazione.Andata.IstatRegioneImbarco = 0  ; // Per ora non gestito
               DatiTariffazione.Andata.IstatRegioneSbarco  = 0  ; // Per ora non gestito
               STAZ_FS & Imbarco = Grafo[Tratta.IdStazioneIngresso];
               STAZ_FS & Sbarco  = Grafo[Tratta.IdStazioneUscita  ];
               strcpy(DatiTariffazione.Andata.Descrizione,Imbarco.Nome7());
               strcat(DatiTariffazione.Andata.Descrizione,"->");
               strcat(DatiTariffazione.Andata.Descrizione,Sbarco.Nome7());

//<<<       if Tratta.Km > 9000
            } /* endif */
            break;
//<<<    switch  Poli.TipoPoli
         case POLIMETRICA::MARE_FS :
            DatiTariffazione.KmMare  += Tratta.Km ;
            break;
         case POLIMETRICA::URBANA_CUM :
            // Di collegamento: non deve modificare i dati di tariffazione
            break;
         case POLIMETRICA::SOLO_LOCALE  :
         case POLIMETRICA::LOCALE  :
         default:
            DatiTariffazione.KmReali += Tratta.Km ;
            break;
         } /* endswitch */
//<<< for  int i = 0; i < NumeroTratte ; i ++
      }
//<<< if Ok
   }
   if (DatiTariffazione.Lunghezza() > 9000) {
      Ok = FALSE; // E' in realta' una tratta non tariffabile, imposto a > 9000 per segnalare errore
   } /* endif */
   if (Ok) {
      if(DatiTariffazione.KmReali == 0) {
         if (DatiTariffazione.KmConcessi1 || DatiTariffazione.Andata.KmMare ) {
            if(Grafo.AbilitaCumulativo == 99){     // Solo cumulativi
               Stato = TARIFFABILE;
               DatiTariffazione.Stato = DATI_TARIFFAZIONE::TARIFFA_VALIDA;
            } else {
               Stato = SOLO_CUMULATIVO;
               DatiTariffazione.Stato = DATI_TARIFFAZIONE::SOLO_CUMULATIVO;
            }
         } else {
            Stato = NON_TARIFFABILE;
            ERRSTRING("Percorso FS con Km a 0");
            DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRATTE_NON_TARIFFABILI;
         } /* endif */
      } else {
         Stato = TARIFFABILE;
         DatiTariffazione.Stato = DATI_TARIFFAZIONE::TARIFFA_VALIDA;
      }
   } else {
      Stato = NON_TARIFFABILE;
      DatiTariffazione.Stato = DATI_TARIFFAZIONE::TRATTE_NON_TARIFFABILI;
//<<< if  Ok
   } /* endif */
   #ifdef DBG2
   DatiTariffazione.Trace("Dati tariffazione:");
   #endif

   BOOL Cumulativo = DatiTariffazione.KmConcessi1 || DatiTariffazione.Andata.KmMare;
   DatiTariffazione.CodiceTariffaRegionale = Grafo.CodiceTariffaRegionale(PercorsoG,Cumulativo);

   // Adesso imposto i codici CCR di origine e destinazione
   // Se la stazione non e' concessa faccio un ciclo inutile, ma il
   // risultato e' corretto.
   {
      TRATTA & Tratta = Tratte[0];
      WORD SocietaConcessa = Grafo.Polim[Tratta.IdPolimetrica].SocietaConcessa;
      STAZ_FS::D_CONCESSE * Conc = Grafo[Origine].Concesse();
      DatiTariffazione.CodiceCCROrigine  = Grafo[Origine].CCR;
      // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
      SCAN_NUM_WIN(Conc,Grafo[Origine].NumeroConcesse,DatiConcessi,STAZ_FS::D_CONCESSE){
         if(DatiConcessi->SocietaConcessa == SocietaConcessa){
            DatiTariffazione.CodiceCCROrigine = DatiConcessi->CCRCumulativo;
            break;
         }
      } ENDSCAN ;
   }
   {
      TRATTA & Tratta = Tratte[NumeroTratte -1];
      WORD SocietaConcessa = Grafo.Polim[Tratta.IdPolimetrica].SocietaConcessa;
      STAZ_FS::D_CONCESSE * Conc = Grafo[Destinazione].Concesse();
      DatiTariffazione.CodiceCCRDestinazione  = Grafo[Destinazione].CCR;
      // EMS004 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
      SCAN_NUM_WIN(Conc,Grafo[Destinazione].NumeroConcesse,DatiConcessi,STAZ_FS::D_CONCESSE){
         if(DatiConcessi->SocietaConcessa == SocietaConcessa){
            DatiTariffazione.CodiceCCRDestinazione = DatiConcessi->CCRCumulativo;
            break;
         }
      } ENDSCAN ;
   }

   return Stato == TARIFFABILE;

//<<< BOOL PERCORSO_POLIMETRICHE::Valorizza PERCORSO_GRAFO & PercorsoG
}

//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI::ControllaCompatibilitaSuTratta
//----------------------------------------------------------------------------
ULONG PERCORSO_INSTRADAMENTI::ControllaCompatibilitaSuTratta(ID IdToTest, VINCOLO & V1, VINCOLO & V2){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_INSTRADAMENTI::ControllaCompatibilitaSuTratta"
   int Ret = 0;
   GRAFO & Grafo = GRAFO::Gr();
   if (!IsNodo(IdToTest)) {
      WORD Pos = Percorso.Posizione(IdToTest,V1.Idx);
      if (Pos == WORD_NON_VALIDA ) {
         if(Percorso.Posizione(Grafo.Rami[Grafo[IdToTest].IdRamo].IdStaz1,V1.Idx) > V2.Idx &&
               Percorso.Posizione(Grafo.Rami[Grafo[IdToTest].IdRamo].IdStaz2,V1.Idx) > V2.Idx){
            Ret = 0 ; // E' OK se ho anche uno solo dei due nodi
         }
      } else if (Pos > V2.Idx ) {   // Anomalia: e' un punto di inversione ma SUCCESSIVO
         Ret = 2; // Credo sia un' anomalia fondamentalmente non sanabile: BEEP per autodiagnostica ed invalido il percorso
      } /* endif */
   } else if( Percorso.Posizione(IdToTest,V1.Idx) > V2.Idx ){
      Ret = 1; // E' un' anomalia da sanare
   } /* endif */
   return Ret;
}
//----------------------------------------------------------------------------
// PERCORSO_POLIMETRICHE::IdentificaVincoloAggiuntivo
//----------------------------------------------------------------------------
// Questa routine viene chiamata quando vi e' la necessita' di una riconciliazione
// tra percorso grafo e percorso polimetriche
// Torna un ReasonCode diverso da 0 se non riesce ad identificare il vincolo
int PERCORSO_POLIMETRICHE::IdentificaVincoloAggiuntivo(VINCOLO & Aggiuntivo, VINCOLO & V0, VINCOLO & V1, VINCOLO & V2, int & Modo, ARRAY_ID & FerrovieConcesseCompatibili ){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_POLIMETRICHE::IdentificaVincoloAggiuntivo"

/* Vi sono due passi fondamentali:
      - Identifico delle stazioni candidate come vincoli aggiuntivi
      - Scelgo una stazione tra le possibili stazioni candidate

      Il primo passo e' relativamente lineare: se la tratta e' formata
      da un solo ramo aggiungo una stazione intermedia del ramo,
      altrimenti tutte le stazioni intermedie del percorso grafo

      Il secondo passo e' piu' complesso: in pratica gli elementi da
      considerare sono tre:

      - L' aggiunta del vincolo e' in grado di sanare il percorso
      polimetriche ed il percorso grafo ( o almeno di migliorare la congruenza) ?

      - Il vincolo e' aggiuntivo o sostituisce uno dei due vincoli
      precedenti?

      - Se sostituisce il precedente crea delle anomalie sul percorso
      fino a questo punto calcolato?

      Per soddisfare questa logica si procede nel seguente modo:

      - Si dividono le candidate in tre insiemi:
         + Quelle che possono sostituire il primo vincolo
         + Quelle che possono sostituire il secondo vincolo
         + Le altre

      Si testano le stazioni candidate in questo stesso ordine, partendo
      (in ogni insieme) dalle piu' vicine al primo vincolo.

      La prima stazione candidata che riesce a sanare il percorso e' la
      prescelta.

      A questo punto per vedere se debba andare in sostituzione od in
      aggiunta si fa una verifica se l' introduzione del vincolo crei
      una incongruenza accoppiato al vincolo precedente.

      Per il vincolo seguente NON faccio azioni.

   */

   int ReasonCode = 0; Modo = 0;
   GRAFO & Grafo = GRAFO::Gr();
   ARRAY_DINAMICA<VINCOLO> Candidati;
   // Ottengo i candidati aggiungendo tutte le stazioni del percorso tra i due vincoli, e poi
   // eventualmente una stazione intermedia dell' ultimo ramo ( oppure l' ultima stazione del percoroso)
   ID S1 = V1.Id; ID S2 = V2.Id; int Idx1 = V1.Idx; //  Identificatori dell' ultimo ramo / candidato
   for (int v = V1.Idx + 1; v < V2.Idx ; v++) { // Nodi (o stazioni) intermedi
      // Sono tutti candidati ragionevoli
      Aggiuntivo.Idx  = v;
      Aggiuntivo.Id = Percorso[Aggiuntivo.Idx];
      Aggiuntivo.Offs = 0;
      Candidati += Aggiuntivo;
      S1 = Aggiuntivo.Id;
      Idx1 = Aggiuntivo.Idx;
   } /* endfor */
   // Ultimo candidato : Accetto anche stazioni intermedie di ramo (o l' ultima stazione del grafo)
   if (V2.Offs > 0) { // Un buon candidato e' il nodo corrispondente al secondo vincolo
      Aggiuntivo.Idx  = V2.Idx;
      Aggiuntivo.Id = Percorso[Aggiuntivo.Idx];
      Aggiuntivo.Offs = 0;
      Candidati += Aggiuntivo;
   } else { // Provo con una stazione intermedia di ramo
      ID IdRamo; int Km;
      if (!Grafo.IdentificaRamo(S1,S2,IdRamo,Km) ) {
         if(Candidati.Dim() == 0)ReasonCode = 3; // Ramo non identificato e nessun candidato trovato
      } else {
         RAMO & Ramo = Grafo.Rami[IdRamo];
         int Off1 = Grafo[S1].ProgRamo;
         int Off2 = Grafo[S2].ProgRamo;
         if(Off1 == 0 && S1 == Ramo.IdStaz2)Off1 = Ramo.NumStazioni + 1;
         if(Off2 == 0 && S2 == Ramo.IdStaz2)Off2 = Ramo.NumStazioni + 1;
         if (abs(Off2 - Off1) < 2) {
            // Non vi sono stazioni da aggiungere
            if(Candidati.Dim() == 0)ReasonCode = 4;
         } else {
            Aggiuntivo.Idx = Idx1;
            Aggiuntivo.Offs = (Off1+Off2)/2;
            Aggiuntivo.Id   = Ramo.StazioniDelRamo()[Aggiuntivo.Offs - 1].Id;
            Candidati += Aggiuntivo;
            #ifdef DBG11
            TRACESTRING(VRS(Off1) + VRS(Off2) + VRS(IdRamo));
            #endif
         } /* endif */
      } /* endif */
//<<< if  V2.Offs > 0    // Un buon candidato e' il nodo corrispondente al secondo vincolo
   }
   if(Candidati.Dim() == 0 && ReasonCode == 0)ReasonCode = 5;
   if(ReasonCode)return ReasonCode;

   // Adesso debbo valutare e scegliere tra i candidati
   int CandidatoPrescelto = -1;
   ARRAY_ID StazioniDiDiramazione;
   ORD_FORALL(Candidati,i1){
      VINCOLO & Candidato = Candidati[i1];
      // Seleziono le sole stazioni che permettono di rimuovere il primo vincolo
      if(! (Candidato.Idx >= V1.Idx1 && Candidato.Idx <= V1.Idx2)) continue;
      #ifdef DBG11A
      Candidato.Trace("Candidato sostitutivo 1ø Vincolo");
      #endif
      // Debbo Identificare le stazioni di diramazione
      StazioniDiDiramazione.Clear();
      BOOL Ok;
      Ok  = Grafo.IdentificaStazioniDiDiramazione(V1.Id, Candidato.Id, FerrovieConcesseCompatibili, StazioniDiDiramazione);
      Ok &= Grafo.IdentificaStazioniDiDiramazione(Candidato.Id, V2.Id, FerrovieConcesseCompatibili, StazioniDiDiramazione);
      if(!Ok)continue;
      #ifdef DBG11A
      TRACESTRING(" StazioniDiDiramazione= "+ StazioniDiDiramazione.ToStringa());
      #endif
      // Adesso controllo che la situazione sia stata sanata
      ORD_FORALL(StazioniDiDiramazione,i){
         Ok = ControllaCompatibilitaSuTratta(StazioniDiDiramazione[i],V1,V2) == 0;
         if(!Ok){
            #ifdef DBG11A
            TRACESTRING(" Incompatibilita' a stazione di diramazione "+ STRINGA(StazioniDiDiramazione[i]));
            #endif
            break;
         }
      }
      if(!Ok)continue;
      // Ho trovato il candidato buono
      CandidatoPrescelto = i1;
      #ifdef DBG11A
      Candidati[CandidatoPrescelto].Trace("Candidato prescelto sostitutivo 1ø Vincolo");
      #endif
      break;
//<<< ORD_FORALL Candidati,i1
   }
   if(CandidatoPrescelto == -1) ORD_FORALL(Candidati,i2){
      VINCOLO & Candidato = Candidati[i2];
      // Seleziono le sole stazioni che permettono di rimuovere il secondo vincolo
      if(! (Candidato.Idx >= V2.Idx1 && Candidato.Idx <= V2.Idx2)) continue;
      #ifdef DBG11A
      Candidato.Trace("Candidato sostitutivo 2ø Vincolo");
      #endif
      // Debbo Identificare le stazioni di diramazione
      StazioniDiDiramazione.Clear();
      BOOL Ok;
      Ok  = Grafo.IdentificaStazioniDiDiramazione(V1.Id, Candidato.Id, FerrovieConcesseCompatibili, StazioniDiDiramazione);
      Ok &= Grafo.IdentificaStazioniDiDiramazione(Candidato.Id, V2.Id, FerrovieConcesseCompatibili, StazioniDiDiramazione);
      if(!Ok)continue;
      #ifdef DBG11A
      TRACESTRING(" StazioniDiDiramazione= "+ StazioniDiDiramazione.ToStringa());
      #endif
      // Adesso controllo che la situazione sia stata sanata
      ORD_FORALL(StazioniDiDiramazione,i){
         Ok = ControllaCompatibilitaSuTratta(StazioniDiDiramazione[i],V1,V2) == 0;
         if(!Ok){
            #ifdef DBG11A
            TRACESTRING(" Incompatibilita' a stazione di diramazione "+ STRINGA(StazioniDiDiramazione[i]));
            #endif
            break;
         }
      }
      if(!Ok)continue;
      // Ho trovato il candidato buono
      CandidatoPrescelto = i2;
      #ifdef DBG11A
      Candidati[CandidatoPrescelto].Trace("Candidato prescelto sostitutivo 1ø Vincolo");
      #endif
      break;
//<<< if CandidatoPrescelto == -1  ORD_FORALL Candidati,i2
   }
   if(CandidatoPrescelto == -1) ORD_FORALL(Candidati,i3){
      VINCOLO & Candidato = Candidati[i3];
      // Seleziono le stazioni che non permettono di rimuovere alcun vincolo
      if(Candidato.Idx >= V1.Idx1 && Candidato.Idx <= V1.Idx2) continue;
      if(Candidato.Idx >= V2.Idx1 && Candidato.Idx <= V2.Idx2) continue;
      #ifdef DBG11A
      Candidato.Trace("Candidato non sostitutivo di Vincolo");
      #endif
      // Debbo Identificare le stazioni di diramazione
      StazioniDiDiramazione.Clear();
      BOOL Ok;
      Ok  = Grafo.IdentificaStazioniDiDiramazione(V1.Id, Candidato.Id, FerrovieConcesseCompatibili, StazioniDiDiramazione);
      Ok &= Grafo.IdentificaStazioniDiDiramazione(Candidato.Id, V2.Id, FerrovieConcesseCompatibili, StazioniDiDiramazione);
      if(!Ok)continue;
      #ifdef DBG11A
      TRACESTRING(" StazioniDiDiramazione= "+ StazioniDiDiramazione.ToStringa());
      #endif
      // Adesso controllo che la situazione sia stata sanata
      ORD_FORALL(StazioniDiDiramazione,i){
         Ok = ControllaCompatibilitaSuTratta(StazioniDiDiramazione[i],V1,V2) == 0;
         if(!Ok){
            #ifdef DBG11A
            TRACESTRING(" Incompatibilita' a stazione di diramazione "+ STRINGA(StazioniDiDiramazione[i]));
            #endif
            break;
         }
      }
      if(!Ok)continue;
      // Ho trovato il candidato buono
      CandidatoPrescelto = i3;
      #ifdef DBG11A
      Candidati[CandidatoPrescelto].Trace("Candidato prescelto sostitutivo 1ø Vincolo");
      #endif
      break;
//<<< if CandidatoPrescelto == -1  ORD_FORALL Candidati,i3
   }
   if (CandidatoPrescelto < 0 ) {
      ERRSTRING("Impossibile riconciliare Grafo e polimetriche");
      // Adesso debbo vedere se la differenza tra i due percorsi e' (sul grafo)
      // inferiore a 15 chilometri: se si ignoro il problema, altrimenti dichiaro
      // il percorso non tariffabile.
      ARRAY_ID  P1,P2;
      P1 += V1.Id;
      for (int i = V1.Idx +1 ; i < V2.Idx ; i++ ) P1 += Percorso[i];
      P1 += V2.Id;
      P2 += V1.Id; StazioniDiDiramazione.Clear();
      Grafo.IdentificaStazioniDiDiramazione(V1.Id, V2.Id, FerrovieConcesseCompatibili, StazioniDiDiramazione);
      ORD_FORALL(StazioniDiDiramazione,j)P2 += StazioniDiDiramazione[j];
      PERCORSO_GRAFO PG1(P1);
      PERCORSO_GRAFO PG2(P2);
      int Km1 = PG1.Len();
      int Km2 = PG2.Len();
      #ifdef DBG11
      P1.Trace(Grafo,"Stazioni tra i vincoli in base al grafo, Km = "+STRINGA(Km1));
      P2.Trace(Grafo,"Stazioni tra i vincoli in base alle polimetriche, Km = "+STRINGA(Km2));
      #endif
      if (abs(Km1 - Km2) <= 15) {
         Modo = 0;  // Ignoro il problema
      } else {
         ReasonCode = 6; // Dichiaro non tariffabile
      } /* endif */
//<<< if  CandidatoPrescelto < 0
   } else {
      Modo = 1;
      Aggiuntivo = Candidati[CandidatoPrescelto];
      // Debbo vedere se posso realmente sostituire un vincolo , o se debbo andare in aggiunta
      if(Aggiuntivo.Idx >= V1.Idx1 && Aggiuntivo.Idx <= V1.Idx2){
         // Debbo controllare che non crei un' incoerenza nella tratta precedente
         if(V0.Id > 0){ // Altrimenti NON ho una tratta precedente e non posso sostituire
            BOOL Ok; StazioniDiDiramazione.Clear();
            Ok  = Grafo.IdentificaStazioniDiDiramazione(V0.Id, Aggiuntivo.Id, FerrovieConcesseCompatibili, StazioniDiDiramazione);
            ORD_FORALL(StazioniDiDiramazione,i) Ok &= ControllaCompatibilitaSuTratta(StazioniDiDiramazione[i],V0,V2) == 0;
            if(Ok)Modo = 2;
         }
      } else if(Aggiuntivo.Idx >= V2.Idx1 && Aggiuntivo.Idx <= V2.Idx2){
         Modo = 3 ; // Non e' necessario controllare coerenza in quanto non so se il prossimo vincolo sia Ok
      }
   } /* endif */
   #ifdef DBG11A
   TRACESTRING(VRS(Modo) + VRS(ReasonCode));
   #endif
   return ReasonCode;

//<<< int PERCORSO_POLIMETRICHE::IdentificaVincoloAggiuntivo VINCOLO & Aggiuntivo, VINCOLO & V0, VINCOLO & V1, VINCOLO & V2, int & Modo, ARRAY_ID & FerrovieConcesseCompatibili
}
//----------------------------------------------------------------------------
// PERCORSO_POLIMETRICHE::AccorpaPolimetriche
//----------------------------------------------------------------------------
// Accorpa le polimetriche
// Mette l' instradamento in forma leggibile
//----------------------------------------------------------------------------
BOOL PERCORSO_POLIMETRICHE::AccorpaPolimetriche(PERCORSO_GRAFO & PercorsoG, ARRAY_DINAMICA<TRATTA_DI_INSTRADAMENTO> & TratteInstradamento, ARRAY_ID & FerrovieConcesseUtilizzate){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_POLIMETRICHE::AccorpaPolimetriche"
   static ARRAY_ID Wrk(64);                      // static per performance
   static ARRAY_ID Vincoli ;                     // static per performance
   // ....................................................
   // Identificazione forma leggibile delle stazioni di instradamento
   // ....................................................
   // Adesso determino QUANTE dovrebbero essere le stazioni di instradamento "ottimali" da mostrare
   WORD Kmg = PercorsoG.Len();
   BYTE Ottimali= 6;
   GRAFO & Grafo = *GRAFO::Grafo;
   if(Kmg < 300) {
      if(       Kmg < 30 ){ Ottimali = 0;
      } else if(Kmg < 60 ){ Ottimali = 1;
      } else if(Kmg < 120){ Ottimali = 2;
      } else if(Kmg < 180){ Ottimali = 3;
      } else if(Kmg < 240){ Ottimali = 4;
      } else {  Ottimali = 5;
      }
   }
   #ifdef DBG10
   TRACEVLONG(Ottimali);
   ORD_FORALL(TratteInstradamento,Ti) TratteInstradamento[Ti].Trace("Tratta Instradamento["+STRINGA(Ti)+"]");
   #endif
   Ottimali ++; // Per tener conto che le tratte sono una in piu' delle stazioni di instradamento
   while (Ottimali > TratteInstradamento.Dim() ) {
      int TrattaDaSezionare = -1;
      int KmTratta = 29; // Non seziono tratte < 30 Km
      int Fact = 10;
      // Sugli ultimi due sezionamenti possibili predomina la sezionabilita'
      if(TratteInstradamento.Dim() >= 5) Fact = 200;
      int NumPiuPoli = 0;
      FORALL(TratteInstradamento,i){
         TRATTA_DI_INSTRADAMENTO  Tratta = TratteInstradamento[i];
         // Il "peso" della tratta e' dato da una combinazione del numero dei Km e del cambio di polimetrica
         int CambioPolimetrica = (Tratte[Tratta.Pol1].IdPolimetrica == Tratte[Tratta.Pol2].IdPolimetrica) ? 0 : Fact;
         int Peso = Tratta.Km + CambioPolimetrica ;
         #ifdef DBG10
         Tratta.Trace("Tratta Instradamento["+STRINGA(i)+"] Peso= "+STRINGA(Peso) );
         #endif
         if( Tratta.Sezionabile && Peso > KmTratta ){
            TrattaDaSezionare = i;
            KmTratta = Peso;
         }
         if( Tratta.Sezionabile && CambioPolimetrica )NumPiuPoli ++;
      }
      if ( TrattaDaSezionare < 0)break; // Non posso suddividere ulteriormente
      TRATTA_DI_INSTRADAMENTO  Tratta = TratteInstradamento[TrattaDaSezionare];
      #ifdef DBG10
      Tratta.Trace("Tratta ["+STRINGA(TrattaDaSezionare)+"] Da sezionare");
      TRACESTRING( VRS(Tratte[Tratta.Pol1].IdPolimetrica) + VRS(Tratte[Tratta.Pol2].IdPolimetrica) );
      #endif
      int CambioPolimetrica = (Tratte[Tratta.Pol1].IdPolimetrica == Tratte[Tratta.Pol2].IdPolimetrica) ? 0 : Fact;
      BOOL TRY = TRUE;
      if (Tratta.Km <= 20){
         #ifdef DBG10
         TRACESTRING("Tratta NON sezionabile : Meno di 20 Km");
         #endif
         TratteInstradamento[TrattaDaSezionare].Sezionabile = FALSE;
         TRY = FALSE;
      } else if (Tratta.Km <= 60 && CambioPolimetrica == 0 ){
         #ifdef DBG10
         TRACESTRING("Tratta NON sezionabile : Meno di 60 Km e niente cambio polimetrica");
         #endif
         TratteInstradamento[TrattaDaSezionare].Sezionabile = FALSE;
         TRY = FALSE;
      } else if (CambioPolimetrica) {  // Ho piu' di una polimetrica nella tratta
         #ifdef DBG10
         TRACESTRING("Tento sezionamento per cambio polimetrica");
         #endif
         TRY = FALSE;
         // Cerco di identificare la stazione di sezionamento come una delle stazioni di
         // passaggio da una polimetrica ad un' altra: la piu' vicina al centro tratta
         int NumTratta = -1;
         int KmCentro  = Tratta.Km / 2;
         int NumKm     = 10; // Oltre questa distanza una delle due tratte e' troppo piccola
         int Km = 0;
         for (int i = Tratta.Pol1; i < Tratta.Pol2 ; i++ ) {
            POLIMETRICA & Pol = Grafo.Polim[Tratte[i].IdPolimetrica];
            Km += Tratte[i].Km;
            if (Pol.Concessa()) continue;
            if( abs(Km - KmCentro) < abs(NumKm-KmCentro)){
               NumKm = Km ;
               NumTratta = i;
            }
         } /* endfor */
         if(NumTratta >=0 ){
            TRATTA_DI_INSTRADAMENTO & Tratta2 = TratteInstradamento[TrattaDaSezionare];
            Tratta.Id2  = Tratte[NumTratta].IdStazioneUscita;
            Tratta2.Id1 = Tratte[NumTratta].IdStazioneUscita;
            Tratta.Km   = NumKm;
            Tratta2.Km -= NumKm;
            Tratta2.Vincolante1 = FALSE;
            Tratta.Pol2 = NumTratta;
            Tratta2.Pol1= NumTratta+1;
            #ifdef DBG10
            Tratta.Trace("1ø parte Tratta ["+STRINGA(TrattaDaSezionare)+"] Sezionata");
            Tratta2.Trace("2ø parte Tratta ["+STRINGA(TrattaDaSezionare+1)+"] Sezionata");
            #endif
            TratteInstradamento.Insert(Tratta,TrattaDaSezionare);
         } else if (Tratta.Km <= 60 ){
            #ifdef DBG10
            TRACESTRING("Tratta NON sezionabile : Meno di 60 Km e fallito cambio polimetrica");
            #endif
            TratteInstradamento[TrattaDaSezionare].Sezionabile = FALSE;
            TRY = FALSE;
         } else {
            TRY = TRUE; // Provo a sezionarla in altro modo
//<<<    if NumTratta >=0
         }
//<<< if  Tratta.Km <= 20
      }
      if(TRY) { // Debbo ancora cercare di sezionare
         // NB: Non seziono in modo adeguato se ho piu' di una polimetrica e la PRIMA e' corta
         #ifdef DBG10
         TRACESTRING("Tento sezionamento per lunghezza");
         #endif
         POLIMETRICA & Pol = Grafo.Polim[Tratte[Tratta.Pol1].IdPolimetrica];
         if (Pol.Concessa()){
            #ifdef DBG10
            TRACESTRING("Tratta NON sezionabile : Polimetrica concessa");
            #endif
            TratteInstradamento[TrattaDaSezionare].Sezionabile = FALSE;
         } else {
            // Debbo trovare una stazione di instradamento o diramazione vicina al centro della polimetrica.
            int Stazione = -1;
            int NumKm    = 10; // Oltre questa distanza una delle due tratte e' troppo piccola
            int KmCentro  = Tratta.Km / 2;

            // Posso avere due tipi di gestione:
            // - Se identifico in modo adeguato le stazioni sul percorso grafo originale
            //   Cerco una stazione intermedia su di esse
            // - Altrimenti costruisco al volo il percorso piu' breve e provo a
            //   suddividere su quello
            // Mi appoggio al grafo per cercare di non impiccarmi con i rami secondari, ed anche
            // per limitarmi ai nodi. Sorry per le stazioni di instradamento non nodali
            WORD Idx1 = PercorsoG.Nodi.Posizione(Tratta.Id1);
            WORD Idx2 = PercorsoG.Nodi.Posizione(Tratta.Id2);
            BOOL ProvoSuOriginale = Idx1 < Idx2 && Idx2 != WORD_NON_VALIDA;
            if (ProvoSuOriginale) {
               for (int i = Idx2 - 1 ; i > Idx1 ; i-- ) {
                  ID Staz = PercorsoG.Nodi[i];
                  if(!(Grafo[Staz].DiDiramazione || Grafo[Staz].DiInstradamento))continue;
                  if(!Pol.Contiene(Staz))continue;
                  int Km =  Pol.DistanzaTra(Tratta.Id1,Staz);
                  if(Km >= 9000) continue;
                  if( abs(Km - KmCentro) < abs(NumKm-KmCentro)){
                     NumKm = Km ;
                     Stazione = Staz;
                  }
               } /* endfor */
            } else {
               Wrk.Clear(); Wrk += Tratta.Id1; Wrk += Tratta.Id2;
               PERCORSO_GRAFO PePo(Wrk);
               for (int i = PePo.Nodi.Dim() -2 ; i > 0 ; i-- ) {
                  ID Staz = PePo.Nodi[i];
                  if(!(Grafo[Staz].DiDiramazione || Grafo[Staz].DiInstradamento))continue;
                  if(!Pol.Contiene(Staz))continue;
                  int Km =  Pol.DistanzaTra(Tratta.Id1,Staz);
                  if( abs(Km - KmCentro) < abs(NumKm-KmCentro)){
                     NumKm = Km ;
                     Stazione = Staz;
                  }
               } /* endfor */
//<<<       if  ProvoSuOriginale
            } /* endif */
            if (Stazione > 0) {
               TRATTA_DI_INSTRADAMENTO & Tratta2 = TratteInstradamento[TrattaDaSezionare];
               Tratta.Id2  = Stazione;
               Tratta2.Id1 = Stazione;
               Tratta.Km   = NumKm;
               Tratta2.Km -= NumKm;
               Tratta.Sezionabile = FALSE;
               Tratta2.Sezionabile = FALSE;
               #ifdef DBG10
               Tratta.Trace("1ø parte Tratta ["+STRINGA(TrattaDaSezionare)+"] Sezionata");
               Tratta2.Trace("2ø parte Tratta ["+STRINGA(TrattaDaSezionare+1)+"] Sezionata");
               #endif
               TratteInstradamento.Insert(Tratta,TrattaDaSezionare);
            } else {
               #ifdef DBG10
               TRACESTRING("Tratta NON sezionabile : Non riuscito a suddividere polimetrica");
               #endif
               TratteInstradamento[TrattaDaSezionare].Sezionabile = FALSE;
            } /* endif */

//<<<    if  Pol.Concessa
         } /* endif */
//<<< if TRY    // Debbo ancora cercare di sezionare
      } /* endif */
      // Se ho percorsi sezionabili con piu' polimetriche e posso sezionare, seziono.
      if(NumPiuPoli > 0 && Ottimali < 7 && Ottimali == TratteInstradamento.Dim()){
         Ottimali ++;
      }
      #ifdef DBG10
      TRACESTRING( VRS(Ottimali) + VRS(NumPiuPoli) + VRS(TratteInstradamento.Dim())) ;
      #endif
//<<< while  Ottimali > TratteInstradamento.Dim
   } /* endif */
   StazioniDiInstradamento.Clear();
   BOOL LastTagliata=FALSE;
   for (int si = 1;si < TratteInstradamento.Dim() ;si++ ){
      TRATTA_DI_INSTRADAMENTO  Tratta = TratteInstradamento[si];
      TRATTA_DI_INSTRADAMENTO  LastTr = TratteInstradamento[si-1];
      // Taglio via le stazioni di instradamento distanziate meno di 25 Km: non pero' due consecutive
      // e non i casi in cui ho un qualche tipo di cambio polimetrica
      BOOL CambioPolimetrica = Tratte[Tratta.Pol1].IdPolimetrica != Tratte[Tratta.Pol2].IdPolimetrica;
      BOOL NuovaSpecifica  = Tratte[Tratta.Pol1].IdPolimetrica != Tratte[LastTr.Pol2].IdPolimetrica;
      BOOL OkKm            = Tratta.Km >= 25 && LastTr.Km >= 25;
      #ifdef DBG10
      TRACESTRING( VRS(Tratta.Id1) + VRS(CambioPolimetrica) + VRS(NuovaSpecifica) + VRS(OkKm) + VRS(Tratta.Vincolante1) );
      #endif
      if(Tratta.Vincolante1 || CambioPolimetrica || NuovaSpecifica || LastTagliata || OkKm ){
         StazioniDiInstradamento +=  Tratta.Id1;
         LastTagliata=FALSE;
      } else {
         #ifdef DBG10
         TRACESTRING("Tagliata via la stazione di instradamento ID= "+STRINGA(Tratta.Id1));
         #endif
         LastTagliata=TRUE;
      } /* endif */

//<<< for  int si = 1;si < TratteInstradamento.Dim   ;si++
   }
   #ifdef DBG10
   StazioniDiInstradamento.Trace(Grafo,"Stazioni vincolanti in forma standard:");
   #endif

   BOOL Ok = TRUE;

   // ....................................................
   // Eventuale accorpamento di polimetriche NON di DIRAMAZIONE in comune
   // ....................................................
   // N.B. : L' accorpamento NON viene effettuato nei punti di inversione del percorso
   for (int t = NumeroTratte-1 ; t > 0 ; t -- ) {
      #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
      PROFILER::Conta(23);
      #endif
      ULONG Accorpate = 0;
      TRATTA & Tratta1 = Tratte[t-1];
      TRATTA & Tratta2 = Tratte[t];
      // Accorpamento a parita' di polimetrica PER SOLE POLIMETRICHE CUMULATIVE
      // Per gestire polimetriche cumulative con mergine di tolleranza sul percorso minimo
      // Accorpamento particolare per tratte mare Tirrenia
      if ( Tratta2.IdPolimetrica == Tratta1.IdPolimetrica ) {
         POLIMETRICA & Polim = Grafo.Polim[Tratta1.IdPolimetrica];
         if (Polim.TipoPoli == POLIMETRICA::CUMULATIVA ) {
            // Non e' ammissibile, salvo per i casi eccezionali
            // o entro tre chilometri di tolleranza
            // Per vedere se sono entro 3 Km di tolleranza debbo risolvere il problema del
            // grafo nell' ambito delle due tratte (che per costruzione debbono seguire
            // un cammino minimo).
            BOOL InLimiteTolleranza = FALSE;
            Vincoli.Clear(); // I vincoli vanno estesi a tutte le tratte contigue della ferrovia concessa
            for (int i = t-2 ; i >= 0 && Tratte[i].IdPolimetrica == Tratta1.IdPolimetrica;i-- );
            while(++i <= t) Vincoli += Tratte[i].IdStazioneIngresso   ;
            Vincoli += Tratta2.IdStazioneUscita   ;
            BYTE Verso;
            #ifdef DBG2
            Vincoli.Trace(Stazioni,"Vincoli per calcolo tolleranza di 3 Km");
            #endif
            int Km = Polim.DistanzaTra(Vincoli[0],Vincoli.Last(),Verso);
            Grafo.AbilitaCumulativo = 99; // Abilita solo cumulativo
            PERCORSO_GRAFO P2;
            if(P2.Set(Vincoli) && (abs(P2.Len() - Km ) <= CUMTOLLERANZA)){
               InLimiteTolleranza = TRUE;
            }
            Grafo.AbilitaCumulativo = TRUE;

            if (InLimiteTolleranza) {
               TRACESTRING("TARIFFABILE entro il limite tolleranza per il cumulativo: Polimetrica ID "+STRINGA(Tratta2.IdPolimetrica));
               Accorpate = Vincoli.Dim() - 2;  // > 1 se accorpo piu' di due tratte
               TRATTA & Tratta11 = Tratte[t-Accorpate];
               Tratta11.Km    = Km;
               Tratta11.Verso = Verso;
               Tratta11.IdStazioneUscita = Vincoli.Last();
            } else {
               ERRSTRING("NON TARIFFABILE: Polimetrica ID "+STRINGA(Tratta2.IdPolimetrica)+ " Relazione non tariffabile percorso non minimo su polimetrica cumulativa DeltaKm = " + STRINGA(Km - P2.Len()));
               Ok = FALSE; // Relazione NON tariffabile
               break;
            } /* endif */
//<<<    if  Polim.TipoPoli == POLIMETRICA::CUMULATIVA
         } else if (Polim.TipoPoli == POLIMETRICA::MARE_CUM) {
            // Accorpo se e solo se la tratta accorpata ha Km differenti da 0 (dovrebbe essere sempre cosi')
            // NB: Se non accorpo poi avro' un errore perche' non posso tariffare due tratte mare
            BYTE Verso;
            int Km = Polim.DistanzaTra(Tratta1.IdStazioneIngresso,Tratta2.IdStazioneUscita,Verso);
            if (Km > 0){
               Accorpate     = 1;
               Tratta1.Km    = Km;
               Tratta1.Verso = Verso;
               Tratta1.IdStazioneUscita = Tratta2.IdStazioneUscita;
            } /* endif */
         } /* endif */
//<<< if   Tratta2.IdPolimetrica == Tratta1.IdPolimetrica
      } /* endif */
      if(!Accorpate){  // Qui gestisco tutti gli altri casi
         Grafo[Tratta1.IdStazioneIngresso].SetPolimetricheNoDiramaz(Wrk,FerrovieConcesseUtilizzate);
         Grafo[Tratta1.IdStazioneUscita  ].AndPolimetriche(Wrk);
         Grafo[Tratta2.IdStazioneUscita  ].AndPolimetriche(Wrk);
         if(Wrk.Dim()){  // Hanno polimetriche in comune
            #ifdef DBG2
            TRACESTRING2("Tratte consecutive con polimetriche in comune: ",Wrk.ToStringa());
            Grafo.TraceId("  In       :", Tratta1.IdStazioneIngresso);
            Grafo.TraceId("  Intermed :", Tratta1.IdStazioneUscita  );
            Grafo.TraceId("  Out      :", Tratta2.IdStazioneUscita  );
            TRACESTRING("Polimetrica della prima   tratta: "+ STRINGA(Tratta1.IdPolimetrica) +" Km "+STRINGA(Tratta1.Km)+" Verso "+STRINGA(Tratta1.Verso));
            TRACESTRING("Polimetrica della seconda tratta: "+ STRINGA(Tratta2.IdPolimetrica) +" Km "+STRINGA(Tratta2.Km)+" Verso "+STRINGA(Tratta2.Verso));
            #endif
            int KmRif = Tratta1.Km + Tratta2.Km;
            #ifdef ACCORPAINDIM     // Permette l' accorpamento delle polimetriche solo se diminuisce la percorrenza complessiva
            int Km1 = KmRif;  // Scelgo di accorpare solo se diminuisce la percorrenza chilometrica
            #else
            int Km1 = 99999;   // Scelgo di accorpare SEMPRE
            #endif
            ORD_FORALL(Wrk,i){ // Scelgo quella con percorrenza minore
               POLIMETRICA & Polim = Grafo.Polim[Wrk[i]];
               // Verifica punti di diramazione condizionati
               if ( Grafo[Tratta1.IdStazioneUscita].Diramazione().IstrCondInibit().Inibizione(
                     Grafo[Tratta1.IdStazioneIngresso].IstrCond(), Grafo[Tratta2.IdStazioneUscita].IstrCond()
                  )) continue; // E' inibita
               BYTE Verso;
               int Km = Polim.DistanzaTra(Tratta1.IdStazioneIngresso,Tratta2.IdStazioneUscita,Verso);
               // Scelgo la polimetrica con distanza minore
               // Impongo anche un limite di delta km di LIMACCORPAMENTO km, per evitare possibili casi
               // in cui ho un cambio di treno con inversione di percorso su di una tratta secondaria.
               BOOL PuoiAccorpare ;
               // Ammetto l' accorpamento solo sulla stessa polimetrica (Visto con FALLONE)
               // Ma: Verifico i casi di degenerazione (= Stesso chilometraggio)
               if(Wrk[i] == Tratta1.IdPolimetrica ){
                  if(Wrk[i] == Tratta2.IdPolimetrica ){
                     PuoiAccorpare = Km < Km1  && abs(KmRif - Km) <= LIMACCORPAMENTO ;
                  } else if (Tratta2.Km == Polim.DistanzaTra(Tratta2.IdStazioneIngresso,Tratta2.IdStazioneUscita,Verso)){
                     // Caso degenere
                     PuoiAccorpare = Km < Km1  && abs(KmRif - Km) <= LIMACCORPAMENTO ;
                  } else {
                     PuoiAccorpare = FALSE;
                  }
               } else if (Tratta1.Km == Polim.DistanzaTra(Tratta1.IdStazioneIngresso,Tratta1.IdStazioneUscita,Verso)){
                  if(Wrk[i] == Tratta2.IdPolimetrica ){
                     // Caso degenere
                     PuoiAccorpare = Km < Km1  && abs(KmRif - Km) <= LIMACCORPAMENTO ;
                  } else {
                     PuoiAccorpare = FALSE;
                  }
               } else {
                  PuoiAccorpare = FALSE;
               }
               if (PuoiAccorpare){
                  // Verifica verso
                  // NB: Fallone mi ha fatto notare che debbo accorpare anche in caso di
                  // inversione, purche' non faccia scomparire il punto di inversione.
                  // Una modalit… semplice per ottenere cio' e' di accorpare sempre se la
                  // lunghezza della tratta invertita e' > met… LIMACCORPAMENTO
                  // Resta fuori in pratica solo il caso di una inversione di 1 Km
                  // (Su cui non accorpo mai)
                  BYTE Verso1 = Verso;
                  // Valuto effettivamente il verso solo se la lunghezza della tratta e' di 1 Km
                  BOOL DaValutare = Tratta1.Km < (LIMACCORPAMENTO/2) || Tratta2.Km < (LIMACCORPAMENTO/2);
                  if(DaValutare)Polim.DistanzaTra(Tratta1.IdStazioneIngresso,Tratta1.IdStazioneUscita,Verso1);
                  if(Verso == Verso1){
                     Km1 = Km;
                     Accorpate     = 1;
                     Tratta1.Km    = Km;
                     Tratta1.Verso = Verso;
                     Tratta1.IdPolimetrica  = Wrk[i];
                     Tratta1.IdStazioneUscita = Tratta2.IdStazioneUscita;
                  } /* endif */
               } /* endif */
//<<<       ORD_FORALL Wrk,i   // Scelgo quella con percorrenza minore
            }
            #ifdef DBG2
            if (Accorpate) {
               TRACESTRING("==> Accorpate con polimetrica "+STRINGA(Tratta1.IdPolimetrica)+" Km "+STRINGA(Tratta1.Km));
            } else {
               TRACESTRING("==> NON Accorpate");
            } /* endif */
            #endif
//<<<    if Wrk.Dim      // Hanno polimetriche in comune
         }
//<<< if !Accorpate    // Qui gestisco tutti gli altri casi
      }
      if (Accorpate) {
         #ifdef DBG2
         TRATTA & Tratta12 = Tratte[t-Accorpate];
         TRACELONG("Accorpate "+STRINGA(Accorpate+1)+" tratte in una singola tratta di "+STRINGA(Tratta12.Km)+" Km su polimetrica ",Tratta1.IdPolimetrica);
         #endif
         if(NumeroTratte-t-1>0)memmove(&Tratte[t-Accorpate+1],&Tratte[t+1],(NumeroTratte-t-1)*sizeof(TRATTA));
         NumeroTratte -= Accorpate;
         t -= Accorpate - 1;
      } /* endif */
//<<< for  int t = NumeroTratte-1 ; t > 0 ; t --
   } /* endfor */

   // Regola: 1 sola polimetrica FS con 0 km : tariffare per 1 Km  (e' da considerare un coppia di stazioni molto vicine)
   if(NumeroTratte == 1 && Grafo.Polim[Tratte[0].IdPolimetrica].TipoPoli ==  POLIMETRICA::LOCALE && Tratte[0].Km == 0 ){
      #ifdef DBG2
      TRACESTRING("Correzione: Portati i Km da 0 ad 1 Km per stazioni vicine");
      #endif
      Tratte[0].Km = 1;
   }

   return Ok;
//<<< BOOL PERCORSO_POLIMETRICHE::AccorpaPolimetriche PERCORSO_GRAFO & PercorsoG, ARRAY_DINAMICA<TRATTA_DI_INSTRADAMENTO> & TratteInstradamento, ARRAY_ID & FerrovieConcesseUtilizzate
}
//----------------------------------------------------------------------------
// PERCORSO_POLIMETRICHE::StoreTo()
//----------------------------------------------------------------------------
void PERCORSO_POLIMETRICHE::StoreTo(BUFR & To){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_POLIMETRICHE::StoreTo()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: DimBuf = ",To.Dim());
   #endif
   PERCORSO_INSTRADAMENTI::StoreTo(To);
   To.Store(NumeroTratte );
   To.Store(Tratte,NumeroTratte* sizeof(TRATTA));
   #ifdef DBGCACHE
   Trace("Fine: DimBuf = "+STRINGA(To.Dim()));
   #endif
}
//----------------------------------------------------------------------------
// PERCORSO_POLIMETRICHE::ReStoreFrom()
//----------------------------------------------------------------------------
void PERCORSO_POLIMETRICHE::ReStoreFrom(BUFR & From){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_POLIMETRICHE::ReStoreFrom()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: Pointer = ",From.Pointer);
   #endif
   PERCORSO_INSTRADAMENTI::ReStoreFrom(From);
   From.ReStore(NumeroTratte );
   ZeroFill(Tratte);
   From.ReStore(Tratte,NumeroTratte* sizeof(TRATTA));
   #ifdef DBGCACHE
   Trace("Fine: Pointer = "+STRINGA(From.Pointer));
   #endif
}
//----------------------------------------------------------------------------
// DATI_TARIFFAZIONE_2::Clear
//----------------------------------------------------------------------------
void DATI_TARIFFAZIONE_2::Clear(){
   #undef TRCRTN
   #define TRCRTN "DATI_TARIFFAZIONE_2::Clear()"
   memset(this,0,sizeof(DATI_TARIFFAZIONE));
   FormaStandardStampabile.Clear();
   FormaStandard.Clear();
   IstradamentoCVB.Clear();
};
//----------------------------------------------------------------------------
// DATI_TARIFFAZIONE_2::Trace
//----------------------------------------------------------------------------
void DATI_TARIFFAZIONE_2::Trace(const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN "DATI_TARIFFAZIONE_2::Trace"
   if(Livello > trchse)return;
   DATI_TARIFFAZIONE::Trace(Messaggio,Livello);
   TRACEVSTRING2(FormaStandardStampabile);
   FormaStandard.Trace(Stazioni,"Forma Standard");
   TRACESTRING2("Codici CCR delle stazioni di instradamento: ",IstradamentoCVB.ToStringa());
};
//----------------------------------------------------------------------------
// DATI_TARIFFAZIONE::Trace
//----------------------------------------------------------------------------
STRINGA _export DATI_TARIFFAZIONE::StatoInChiaro(){
   #undef TRCRTN
   #define TRCRTN "DATI_TARIFFAZIONE::StatoInChiaro"
   STRINGA Out;
   switch (Stato) {
   case TARIFFA_NON_CALCOLATA:
      Out += "TARIFFA_NON_CALCOLATA";
      break;
   case TARIFFA_VALIDA:
      Out += "TARIFFA_VALIDA";
      break;
   case TARIFFA_VALIDA_SOLO_PER_INFORMATIVA:
      Out += "TARIFFA_VALIDA_SOLO_PER_INFORMATIVA";
      break;
   case TRATTE_ILLEGALI:
      Out += "TRATTE_ILLEGALI";
      break;
   case TRATTE_NON_TARIFFABILI:
      Out += "TRATTE_NON_TARIFFABILI";
      break;
   case SOLO_CUMULATIVO:
      Out += "SOLO_CUMULATIVO";
      break;
   case TRE_CUMULATIVO:
      Out += "TRE_CUMULATIVO";
      break;
//<<< switch  Stato
   default:
      Out += "Tariffa illegale Nø "+STRINGA(Stato);
      break;
   } /* endswitch */
   return Out;
//<<< STRINGA _export DATI_TARIFFAZIONE::StatoInChiaro
}
//----------------------------------------------------------------------------
// DATI_TARIFFAZIONE::Trace
//----------------------------------------------------------------------------
void DATI_TARIFFAZIONE::Trace(const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN "DATI_TARIFFAZIONE::Trace"
   if(Livello > trchse)return;
   ERRSTRING(" ");
   if(Messaggio != NUSTR)ERRSTRING(Messaggio);
   STRINGA Out;
   Out = "Stato : ";
   Out += StatoInChiaro();
   ERRSTRING(Out);
   TRACEVLONGL(Lunghezza() ,1); // L' unica da mostrare sempre
   if(KmReali        ){TRACEVLONGL(KmReali        ,1);};
   if(KmAggiuntivi   ){TRACEVLONGL(KmAggiuntivi   ,1);};
   if(KmMare         ){TRACEVLONGL(KmMare         ,1);};
   if(KmConcessi1    ){TRACEVLONGL(KmConcessi1    ,1);};
   if(KmConcessi2    ){TRACEVLONGL(KmConcessi2    ,1);};
   if(CodConcessione1){TRACEVLONGL(CodConcessione1,1);};
   if(CodLinea1      ){TRACEVLONGL(CodLinea1      ,1);};
   if(CodConcessione2){TRACEVLONGL(CodConcessione2,1);};
   if(CodLinea2      ){TRACEVLONGL(CodLinea2      ,1);};
   if(CodiceCCRDestinazione ){TRACEVLONGL(CodiceCCRDestinazione ,1);};
   if(CodiceCCROrigine      ){TRACEVLONGL(CodiceCCROrigine      ,1);};
   if(CodiceTariffaRegionale){TRACEVLONGL(CodiceTariffaRegionale,1);};
   if(TransitoFS1    ){TRACEID_L(1,TransitoFS1      );};
   if(TransitoFS2    ){TRACEID_L(1,TransitoFS2      );};
   if(TransitoFS3    ){TRACEID_L(1,TransitoFS3      );};
   if(Andata.KmMare){
      TRACEID_L(1,Andata.IdStazioneImbarco);
      TRACEID_L(1,Andata.IdStazioneSbarco);
      TRACEVLONGL(Andata.KmMare          ,1);
      TRACEVLONGL(Andata.CodSocietaMare  ,1);
      TRACEVLONGL(Andata.CodLineaMare    ,1);
      TRACESTRINGL((STRINGA("Andata.Descrizione =")+Andata.Descrizione),1);
      TRACEVLONGL(Andata.IstatRegioneImbarco,1);
      TRACEVLONGL(Andata.IstatRegioneSbarco ,1);
   } ;
   ERRSTRING(" ");
//<<< void DATI_TARIFFAZIONE::Trace const STRINGA& Messaggio, int Livello
}
//----------------------------------------------------------------------------
// ISTR_COND::operator const char *
//----------------------------------------------------------------------------
ISTR_COND::operator const char * () const { // Conversione a char *
   #undef TRCRTN
   #define TRCRTN "ISTR_COND::operator const char *"
   static char Out[128];
   Out[0]=0;
   char Wrk[20];
   for (int i = 0;i < 32 ;i++ ) {
      if(Bytes_0_3 & 1 << i){
         itoa(i+1, Wrk, 10);
         strcat(Out,Wrk);
         strcat(Out,",");
      }
   } /* endfor */
   for (i = 0;i < 16 ;i++ ) {
      if(Bytes_4_5 & 1 << i){
         itoa(i+33, Wrk, 10);
         strcat(Out,Wrk);
         strcat(Out,",");
      }
   } /* endfor */
   return Out;
//<<< ISTR_COND::operator const char *    const   // Conversione a char *
}
//----------------------------------------------------------------------------
// Ecccezioni di normativa
//----------------------------------------------------------------------------
// Tratti  doppi da non considerare
// Tratti Iniziali o finali da non considerare
void ECCEZ_NORMATIVA::Trace(const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN "ECCEZ_NORMATIVA::Trace"
   if(Livello > trchse)return;

   ERRSTRING(Messaggio);
   TRACEVLONGL(Tipo,1);
   TRACEVLONGL(Id1,1);
   TRACEVLONGL(Id2,1);
   if (Tipo == 4) {
      STRINGA Msg;
      if(ValeInizio   ) Msg += "ValeInizio ";
      if(ValeFine     ) Msg += "ValeFine ";
      if(ValeInMezzo  ) Msg += "ValeInMezzo ";
      if(ValeEsatto   ) Msg += "ValeEsatto ";
      if(ValeInverso  ) Msg += "ValeInverso ";
      TRACESTRING(Msg);
   } /* endif */
   TRACEVLONGL(DimPercorso,1);
   ARRAY_ID Stazn;
   for (int i = 0;i < DimPercorso ; i++ ) {
      Stazn += Percorso[i];
   } /* endfor */
   Stazn.Trace(GRAFO::Gr(),"Percorso:",Livello);
//<<< void ECCEZ_NORMATIVA::Trace const STRINGA& Messaggio, int Livello
};
