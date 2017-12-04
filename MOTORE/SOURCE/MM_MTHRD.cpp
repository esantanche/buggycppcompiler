//----------------------------------------------------------------------------
// MM_MTHRD.CPP
//----------------------------------------------------------------------------
// Contiene le routines che controllano ed implementano il flusso principale
// di risoluzione del problema del trasporto.
// In particolare contiene la gestione del multithread
// Contiene anche costruttore e distruttore di DATI_ORARIO_FS, che con
// il multithread debbono essere sincronizzati
//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

// EMS001
typedef unsigned long BOOL;

//0
#define MODULO_OS2_DIPENDENTE
#define  NO_TRC_FOPEN // Per motivi di performance: Evito la ridefinizione delle funzioni di semaforizzazione
#define INCL_DOSSEMAPHORES   /* Semaphore values */
#define INCL_DOSPROCESS   /* Semaphore values */
#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS

#define MM_STACKSIZE 32000

#include "BASE.hpp"
#include "alfa0.hpp"
#include "mm_detta.hpp"
#include "mm_mthrd.hpp"
#include "mm_crit2.hpp"
#include "mm_path.hpp"
#include "mm_COMBI.hpp"
#include "myalloc.h"

BOOL MultiThread; // Indica se opero in modo monothread o multithread
char * _export DecodificaRcOS2(int Rc);

// Per test sincronizzato con OOLIB
extern void HeapTest(char * Rtn);


//----------------------------------------------------------------------------
// Debug opzionale
//----------------------------------------------------------------------------
//#define  DBG2             // livello  minimo di trace su DATI_ORARIO_FS
//#define  DBG3             // trace dati dei nodi
//#define  DBGthread        // Per Debug della gestione del multithread
//#define  DBG4             // Trace PATH_CAMBI utilizzati
//#define  DBGMem           // Dealloca fisicamente tutte le aree prima di iniziare per evidenziare memory leackages
//----------------------------------------------------------------------------


BYTE DATI_ORARIO_FS::ProgressivoRichiesta = 0;

//----------------------------------------------------------------------------
// Gestione MultiThread
//----------------------------------------------------------------------------
// Coda di prefetch: accede ai dati dei clusters
// Gli elementi sono dei PERCORSI_ALFA da elaborare
static MT_FIFO * CodaPrefetch;
// Coda di elaborazione: trova le soluzioni
static MT_FIFO * CodaElabora;
// Gli elementi sono dei PERCORSI_ALFA da elaborare

// Treads per le funzioni di prefetch ed elaborazione
static TID ThreadPrefetch,ThreadElaborazione;
// Funzione di creazione dei threads
BOOL CreaThreads();

static BOOL  CreatiThreads=FALSE;
// Questa variabile per una corretta gestione dei criteri di arresto
static BOOL  ArrestaElaborazione;
static ULONG NumElaborati       ;

// Semaforo per la main routine
HEV SemaforoMain;

//----------------------------------------------------------------------------
// DATI_ORARIO_FS::NewRisolvi_ST(...);
//----------------------------------------------------------------------------
BOOL DATI_ORARIO_FS::NewRisolvi_ST( ID Da, ID A, SDATA  Data, WORD Ora){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::NewRisolvi_ST()"

   MultiThread = FALSE;
   ProgressivoRichiesta ++; // Utilizzato dalle funzioni di accesso ai clusters
   TRACEINT("Esecuzione in ambiente monothread, richiest Nø ",ProgressivoRichiesta);


   PROFILER::Cronometra(10,TRUE) ;
   Free(); // Per rilasciare le risorse interne


   #ifdef DBGMem
   static ULONG NumReq=0;
   static ULONG PerReq=  getenv("MOTORE_MYALLOCTRC_NUMREQ") == NULL ? 0 : atoi(getenv("MOTORE_MYALLOCTRC_NUMREQ")) ;
   if(PerReq && ((++NumReq % PerReq ) == 1) ){
      int TRCHSE = trchse;
      trchse = max(trchse,1);
      CleanUp();
      CLU_BUFR::ClearPool(TRUE); // Pulisce il Pool
      HeapTest(TRCRTN);
      SOLUZIONE::Allocator.Trace("Richiesta Nø "+STRINGA(NumReq) +", Soluzioni attualmente allocate:",1);
      PATH_CAMBI::Allocator.Trace("Path Cambi :: Allocator",1);
      ERRINT("Path Cambi :: Heap Percorsi Dimensione allocata = ", PATH_CAMBI::HeapPercorsi.AreaAllocata());
      ERRINT("Path Cambi :: HashIllegali Dimensione allocata = ", PATH_CAMBI::HashIllegali.AreaAllocata());
      PATH_CAMBI::HashIllegali.Trace("Path Cambi :: HashIllegali ",1);
      ERRINT("IPOTESI :: Cache Dimensione allocata = ", IPOTESI::Cache.AreaAllocata());
      IPOTESI::Cache.Trace("IPOTESI :: Cache", 1);
      Mem_Ctrl();
      trchse = TRCHSE;
      CLU_BUFR::ClearPool(); // Riacquisisce il POOL
   };
   #endif

   #ifdef DBG2
   TRACESTRING("chiamo imposta problema");
   #endif
   OraOrd = Ora;
   if(!ImpostaProblema(Da,A,Data)){
      ERRSTRING("Errore impostando il problema");
      return FALSE;
   };
   PROFILER::Cronometra(10,FALSE) ;

   PROFILER::Cronometra(6,TRUE );
   TRACESTRING("Prima di risolvi in NewRisolvi mm_mthrd");
   TRACEVLONG(IdOrigine());
   TRACEVLONG(IdDestinazione());
   if(!NODO_CAMBIO::Risolvi(IdOrigine(),IdDestinazione())){     //
      ERRSTRING("Errore : Nell' orario attuale i due nodi non sono realmente collegati");
      PROFILER::Cronometra(6,FALSE );
      return FALSE;
   };
   PATH_CAMBI * Path = new PATH_CAMBI;
   Path->Nodi[Path->NumNodi++].SetDiramabile(IdOrigine());
   //TRACESTRING("Prima di Path->CompletaPercorso();");
   Path->CompletaPercorso();
   //TRACESTRING("Dopo di Path->CompletaPercorso();");
   if(Path->NumNodi > 4){
      TRACESTRING("Percorso richiede sempre 4 o piu' treni");
   };
   if(Path->PuoAncoraDiramare())Path->Put(); // Metto nella Heap
   PROFILER::Cronometra(6,FALSE );

   ArrestaElaborazione = FALSE;
   PATH_CAMBI::NumPaths = 0;

   for(NumElaborati = 0; !ArrestaElaborazione ; ){
      if(!Path){
         #ifdef DBG3
         TRACESTRING("Non vi sono ulteriori soluzioni");
         #endif
         ArrestaElaborazione = TRUE;
         break;
      };
      PROFILER::Cronometra(8,TRUE );
      if (!Path->Illegale){
         #ifdef DBG4
         TRACESTRING2("Path Nø "+STRINGA(PATH_CAMBI::NumPaths),*Path);
         #endif

         PROFILER::Cronometra(14,TRUE);
         IPOTESI & Ipotesi = * new IPOTESI(*Path);  // Genero i dati estesi collegati al path
         PROFILER::Cronometra(14,FALSE);

         PROFILER::Cronometra(15,TRUE);
         Ipotesi.PreFetch()      ;
         PROFILER::Cronometra(15,FALSE);

         PROFILER::Cronometra(12,TRUE);
         Ipotesi.Combina()    ;  // Identifica le soluzioni di viaggio
         PROFILER::Cronometra(12,FALSE);

         PROFILER::Cronometra(13,TRUE);
         Ipotesi.Soluzioni.Semplifica() ;  // Elimina le soluzioni poco valide
         PROFILER::Cronometra(13,FALSE);

         ArrestaElaborazione = !NewCriterioDiArresto(NumElaborati);
         NumElaborati ++;

         // Qui il distruttore di IPOTESI rende riutilizzabili gli slot di cache per clusters e collegamenti diretti
         PROFILER::Cronometra(14,TRUE);
         delete &Ipotesi;
         PROFILER::Cronometra(14,FALSE);
//<<< if  !Path->Illegale
      }
      PROFILER::Cronometra(8,FALSE);

      PROFILER::Cronometra(6,TRUE );
      Path  = PATH_CAMBI::DiramaNext();
      // Path->CompatibilePrecedenti(); // Controllo di compatibilita': Gia' effettuato in CompletaPercorso
      PROFILER::Cronometra(6,FALSE );
//<<< for NumElaborati = 0; !ArrestaElaborazione ;
   };

   Classifica(); // Elimina le soluzioni troppo lente

   PROFILER::Cronometra(10,TRUE) ;
   TRACESTRING("Inizio Semplifica");
   Semplifica();   // Semplificazione Finale
   PROFILER::Cronometra(10,FALSE) ;
   PROFILER::Cronometra(9,TRUE);
   Orario.Soluzioni.Consolida()  ;  // Collega le soluzioni al GRAFO
   PROFILER::Cronometra(9,FALSE);
   if (ElaborazioneInterrotta) {
      ERRSTRING("Elaborazione interrotta anzitempo: elimino dati di risposta");
      Free();
   } /* endif */
   PROFILER::Cronometra(10,TRUE) ;
   TRACESTRING("Fine e fine Loop, PATH_CAMBI esaminati= "+STRINGA(NumElaborati)+" da "+STRINGA(PATH_CAMBI::NumPaths)+" Paths");
   CLU_BUFR::TraceTotalIo();
   TRACESTRING("Inizio CleanUp");
   CleanUp();
   TRACESTRING("Fine CleanUp");
   PROFILER::Cronometra(10,FALSE) ;
   return TRUE;
//<<< BOOL DATI_ORARIO_FS::NewRisolvi_ST  ID Da, ID A, SDATA  Data, WORD Ora
} ;

//----------------------------------------------------------------------------
// DATI_ORARIO_FS::MezziDirettiPrenotabili(ID Da,ID A)
//----------------------------------------------------------------------------
BOOL DATI_ORARIO_FS::MezziDirettiPrenotabili(ID Da,ID A,SDATA Data){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::MezziDirettiPrenotabili"

   MultiThread = FALSE;
   ProgressivoRichiesta ++; // Utilizzato dalle funzioni di accesso ai clusters
   TRACEINT("Esecuzione in ambiente monothread, richiesta Nø ",ProgressivoRichiesta);

   Free(); // Per rilasciare le risorse interne

   OraOrd = 0; // Mezzanotte
   if(!ImpostaProblema(Da,A,Data)){
      ERRSTRING("Errore impostando il problema");
      return FALSE;
   };

   if(!NODO_CAMBIO::Risolvi(IdOrigine(),IdDestinazione())){
      ERRSTRING("Errore : Nell' orario attuale i due nodi non sono realmente collegati");
      return FALSE;
   };
   PATH_CAMBI * Path = new PATH_CAMBI;
   Path->Nodi[Path->NumNodi++].SetDiramabile(IdOrigine());
   Path->CompletaPercorso();
   if(Path->NumNodi > 2){
      TRACESTRING("Percorso richiede sempre 2 o piu' treni");
      return FALSE;
   };
   if (Path->Illegale){
      TRACESTRING("Percorso non risolubile con un collegamento diretto legale");
      return FALSE;
   };

   T_TRENO::PerDirettiPrenotabili = TRUE;
   ArrestaElaborazione = FALSE;
   PATH_CAMBI::NumPaths = 0;

   {
      IPOTESI Ipotesi(*Path);  // Genero i dati estesi collegati al path

      Ipotesi.PreFetch()      ;

      Ipotesi.Combina()    ;  // Identifica le soluzioni di viaggio

      Ipotesi.Soluzioni.Semplifica() ;  // Elimina le soluzioni poco valide

      NumElaborati = 1;
      // Qui il distruttore di IPOTESI rende riutilizzabili gli slot di cache per clusters e collegamenti diretti
   }

   Classifica()                  ;  // Elimina le soluzioni troppo lente
   Orario.Soluzioni.Consolida()  ;  // Collega le soluzioni al GRAFO
   Semplifica();   // Semplificazione Finale
   CLU_BUFR::TraceTotalIo();
   CleanUp();
   return TRUE;
} ;
//----------------------------------------------------------------------------
// DATI_ORARIO_FS:: Costruttore  e distruttore
//----------------------------------------------------------------------------
DATI_ORARIO_FS::DATI_ORARIO_FS(const STRINGA& PathOr, const STRINGA & PathFastStart) : Grafo(*GRAFO::Grafo) {

   #undef TRCRTN
   #define TRCRTN "@DATI_ORARIO_FS"

   ERRSTRING("Inizio");

   PathOrario = PathOr;
   if ( PathFastStart == NUSTR) {
      PathFast  = PathOr;
   } else {
      PathFast  = PathFastStart;
   } /* endif */

   ImpostazioneOK          =  0 ;
   ClustExt                =  0 ;
   IdStazioneOrigine       =  0 ;
   IdStazioneDestinazione  =  0 ;
   DistanzaMinima          =  0 ;
   LimiteNodi              =  0 ;
   BaseNodi                =  0 ;
   NumClusters             =  0 ;
   AllPeriodicita          =  NULL;

   FileDettagli         =  new  F_DETTAGLI_MV  (PathOrario + "MM_DETMV.DB" );
   FileDettagliExt      =  new  FILE_RO        (PathOrario + "MM_DETMV.EXT");
   FileNote             =  new  IDX_TESTI_NOTE (PathOrario + "MM_NOTE.DB"  );
   FileNoteExt          =  new  FILE_RO        (PathOrario + "MM_NOTE.EXT" );
   ClustExt             =  new  FILE_RO        (PathOrario + "CLUSSTAZ.EXT");
   StazCluster          =  new  F_STAZN_CLUS   (PathOrario + "MM_STCLU.DB" );
   FilStaz              =  new  BS_STAZIONE_F  (PathOrario + "MM_STAZN.DB" );

   TRACEVLONG(FileDettagli->DimBuffers() );
   TRACEVLONG(FileNote->DimBuffers() );
   TRACEVLONG(FilStaz->DimBuffers() );
   TRACEVLONG(StazCluster->DimBuffers() );
   PROFILER::Somma(0,FileDettagli->DimBuffers() );
   PROFILER::Somma(0,FileNote->DimBuffers() );
   PROFILER::Somma(0,FilStaz->DimBuffers() );
   PROFILER::Somma(0,StazCluster->DimBuffers() );
   PROFILER::Somma(2,FileDettagli->DimBuffers() );
   PROFILER::Somma(2,FileNote->DimBuffers() );
   PROFILER::Somma(2,FilStaz->DimBuffers() );
   PROFILER::Somma(2,StazCluster->DimBuffers() );


   FILE_RO Validita(PathOrario+"VALIDITA.DB");
   if (Validita.FileHandle() == 0) {
      ERRSTRING("Errore: non posso determinare validita' orari");
      BEEP;
   } else {
      if(Validita.Leggi(VRB(ValiditaOrario))){
         ValiditaOrario.Trace("Validita' dell' orario letto da File");
      } else {
         ERRSTRING("Errore: non posso leggere VALIDITA.DB");
         BEEP;
      } /* endif */
   } /* endif */
   Validita.Posiziona(sizeof(VALIDITA_ORARIO));
   Validita.Leggi(VRB(Header));
   Header.Trace("Header letto da file:",1);

   TRACEVLONGL(sizeof(PERIODICITA),1);
   TRACEVLONGL(sizeof(GRUPPO),1);
   TRACEVLONGL(sizeof(SOLUZIONE),1);
   TRACEVLONGL(sizeof(PERCORSO_GRAFO),1);
   TRACEVLONGL(sizeof(STAZ_FS),1);
   TRACEVLONGL(sizeof(RAMO),1);
   TRACEVLONGL(sizeof(POLIMETRICA),1);
   TRACEVLONGL(sizeof(NODO_CAMBIO),1);
   TRACEVLONGL(sizeof(COLLEGAMENTO),1);
   TRACEVLONGL(sizeof(PATH_CAMBI),1);
   TRACEVLONGL(sizeof(STAZN_CLUS),1);
   TRACEVLONGL(sizeof(IPOTESI),1);
   TRACEVLONGL(sizeof(COLLEGAMENTO_DIRETTO),1);
   TRACEVLONGL(sizeof(T_TRENO),1);
   TRACEVLONGL(sizeof(T_CLUST),1);
   TRACEVLONGL(Grafo.TotStazioniGrafo + 1,1);

   TRACESTRING("Inits spicciole");

   All += this; // Aggiungo all' elenco

   ERRSTRING("Fine");

};

DATI_ORARIO_FS::~DATI_ORARIO_FS(){
   #undef TRCRTN
   #define TRCRTN "~DATI_ORARIO_FS"
   Free();

   All -= this; // Tolgo dall' elenco

   // Chiusura dei files e deallocazione aree
   if( ClustExt            ) delete ClustExt            ; ClustExt           = NULL;
   if( FilStaz             ) delete FilStaz             ; FilStaz            = NULL;
   if( FileDettagli        ) delete FileDettagli        ; FileDettagli       = NULL;
   if( FileDettagliExt     ) delete FileDettagliExt     ; FileDettagliExt    = NULL;
   if( FileNote            ) delete FileNote            ; FileNote           = NULL;
   if( FileNoteExt         ) delete FileNoteExt         ; FileNoteExt        = NULL;
   if( StazCluster         ) delete StazCluster         ; StazCluster        = NULL;

   if(All.Dim() == 0){ // In fase di chiusura del motore: debbo deallocare tutto il possibile
      // Tabella in cui tengo gli orari
      if(TabelleDeiDatiOrario) free(TabelleDeiDatiOrario);
      TabelleDeiDatiOrario=NULL;
      AreaAllocataPerOrari = 0;
      // Code di gestione multithread e relativo semaforo
      if(CodaPrefetch)delete CodaPrefetch;
      if(CodaElabora )delete CodaElabora ;
      CodaPrefetch= NULL;
      CodaElabora = NULL;
      if(SemaforoMain)DosCloseEventSem(SemaforoMain); SemaforoMain = NULL;
      CreatiThreads = FALSE;
      // Pool di buffers di I/O
      CLU_BUFR::ClearPool(TRUE); // Dealloco le risorse del POOL
      // Dealloco le aree di uso generale
      PATH_CAMBI::Allocator.Reset();
      PATH_CAMBI::HashIllegali.Reset();
      SOLUZIONE::Allocator.Reset();
      IPOTESI::Cache.Reset();
      Corrente = NULL;
   };

   #ifdef DBG2
   TRACESTRING("Distrutti Dati Orario");
   #endif
//<<< DATI_ORARIO_FS::~DATI_ORARIO_FS
};
//----------------------------------------------------------------------------
// MT_FIFO
//----------------------------------------------------------------------------
MT_FIFO::MT_FIFO( USHORT FDim, const STRINGA & Nome , OGGETTO * Compo):
OGGETTO(Nome, ALTRI, Compo),FifoDim(FDim)
{
   #undef TRCRTN
   #define TRCRTN "@MT_FIFO"

   Mutex = NULL; Piena = NULL; Vuota = NULL; Coda = NULL;
   IndiceLeggi = IndiceScrivi = 0;
   FermaSuPut = FermaSuGet = FALSE;
   Bad = DosCreateMutexSem(NULL ,&Mutex  ,0L,0L);

   if(Bad) {
      ERRSTRING("Errore:non  e' possibile creare il semaforo MUTEX su coda "+Nome+" "+STRINGA(DecodificaRcOS2(Bad)));
      BEEP;
   }
   if(!Bad){
      Bad = DosCreateEventSem(NULL ,&Piena  ,0L,0L);
      if(Bad) {
         ERRSTRING("Errore:non  e' possibile creare il semaforo di coda Piena su "+Nome+" "+STRINGA(DecodificaRcOS2(Bad)));
         BEEP;
      }
   }
   if(!Bad){
      Bad = DosCreateEventSem(NULL ,&Vuota  ,0L,0L);
      if(Bad) {
         ERRSTRING("Errore:non  e' possibile creare il semaforo di coda Vuota su "+Nome+" "+STRINGA(DecodificaRcOS2(Bad)));
         BEEP;
      }
   }
   if(!Bad){
      Coda = (void **) malloc(FifoDim * sizeof(void*));
      if(Coda == NULL){
         Bad = 99999999;
         ERRSTRING("Errore allocando memoria per la coda, FifoDim = "+STRINGA(FifoDim));
      } else {
         memset(Coda,0,FifoDim * sizeof(void*));
      }
   }
};

MT_FIFO::~MT_FIFO(){
   #undef TRCRTN
   #define TRCRTN "~MT_FIFO"
   Clear(TRUE);   // Svuoto la coda
   // Wait 1 secondo per aspettare che la situazione si regolarizzi
   DosSleep(1000);
   if(Mutex) DosCloseMutexSem(Mutex); Mutex = NULL;
   if(Piena) DosCloseEventSem(Piena); Piena = NULL;
   if(Vuota) DosCloseEventSem(Vuota); Vuota = NULL;
   if(Coda)  free(Coda);
};

USHORT MT_FIFO::QueueUsed(){            // Ritorna il numero di slots occupati
   if(Bad)return 0;
   if(CodaPiena()) return FifoDim;
   return (FifoDim+IndiceScrivi-IndiceLeggi) % FifoDim;
}

BOOL MT_FIFO::GetMutex(){
   // Ottiene il controllo della coda. return FALSE su errore
   #undef TRCRTN
   #define TRCRTN "MT_FIFO::GetMutex"
   ULONG Rc = DosRequestMutexSem( Mutex , SEM_INDEFINITE_WAIT); // Ottengo il controllo della coda
   if(Rc){
      ERRSTRING("Errore nell' accesso al Mutex su "+Nome+" "+STRINGA(DecodificaRcOS2(Rc)));
      BEEP;
      return FALSE;
   }
   return TRUE;
};
BOOL MT_FIFO::FreeMutex(){
   // Rilascia il controllo della coda. return FALSE su errore
   #undef TRCRTN
   #define TRCRTN "MT_FIFO::FreeMutex"
   ULONG Rc = DosReleaseMutexSem( Mutex );
   if(Rc){
      ERRSTRING("Errore nel rilascio del Mutex su "+Nome+" "+STRINGA(DecodificaRcOS2(Rc)));
      BEEP;
      return FALSE;
   }
   return TRUE;
};

BOOL MT_FIFO::Put(void * Elemento){
   // Mette un elemento nella lista; Se la lista e' piena aspetta
   // NB : se nel frattempo interviene un CLEAR l' elemento non
   // viene realmente inserito
   #undef TRCRTN
   #define TRCRTN "MT_FIFO::Put"

   if(Bad){
      ERRSTRING("Errore: richiesta su coda in stato illegale o in distruzione");
      return FALSE ; // Non posso riempire la coda
   }
   if(Elemento == NULL){
      ERRSTRING("Errore: Non accetto inserimento di elementi NULL");
      return FALSE;
   }

   if(!GetMutex())return FALSE; // Ottengo l' accesso alla coda

   #ifdef DBGthread
   // TRACESTRING("Put di percorso alfa: "+((PERCORSO_ALFA*)Elemento)->StrClusters());
   #endif

   // Se e' piena mi debbo mettere in attesa
   while(CodaPiena()){
      #ifdef DBGthread
      TRACEINT("Wait su Coda piena '"+Nome+"' Idx = ",IndiceScrivi);
      TRACEVLONG(IndiceLeggi);
      #endif
      unsigned long Pcont;
      DosResetEventSem( Piena, &Pcont );    // semaforo gestito in post
      FermaSuPut ++;
      FreeMutex();
      DosWaitEventSem( Piena, SEM_INDEFINITE_WAIT );  // wait su semaforo coda piena
      // ==> In questo punto potrebbe nel frattempo essere ridiventata piena!
      GetMutex(); // Riottengo il semaforo per poter impostare i dati
      FermaSuPut --;
      if(Inibito){
         #ifdef DBGthread
         TRACESTRING("Put non completato perche' richiesto un Clear, coda: "+Nome+" Ritorno FALSE");
         #endif
         FreeMutex();
         return FALSE;
      }
      #ifdef DBGthread
      TRACESTRING("Fine wait su coda piena Coda "+Nome);
      #endif
//<<< while CodaPiena
   }
   BOOL CodaEraVuota = CodaVuota();
   Coda[IndiceScrivi] = Elemento ;
   #ifdef  DBGthread
   TRACESTRING("Inserito elemento {"+STRINGA(IndiceScrivi)+"} in coda "+Nome);
   #endif
   if(++IndiceScrivi >= FifoDim ) IndiceScrivi = 0 ;
   ULONG Rc = 0;
   if(CodaEraVuota)Rc = DosPostEventSem(Vuota )  ;  // rilascio semaforo di coda vuota
   FreeMutex();
   if(Rc && Rc != ERROR_ALREADY_POSTED){ // Comunque NON torno FALSE
      ERRSTRING("Errore nel POST coda vuota su "+Nome+" "+STRINGA(DecodificaRcOS2(Rc)));
      BEEP;
   }
   return TRUE;
//<<< BOOL MT_FIFO::Put void * Elemento
};

void * MT_FIFO::Get(){
   // Ottiene un elemento nella lista; Se la lista e' vuota aspetta
   // NB : se nel frattempo interviene un CLEAR ritorna NULL
   #undef TRCRTN
   #define TRCRTN "MT_FIFO::Get"

   if(Bad){
      TRACESTRING("Errore: richiesta su coda in stato illegale o in distruzione");
      return NULL ;
   }
   if(!GetMutex())return NULL; // Ottengo l' accesso alla coda


   // Se e' vuota mi debbo mettere in attesa
   while(CodaVuota()){
      #ifdef DBGthread
      TRACEINT("Wait su Coda vuota '"+Nome+"' Idx = ",IndiceLeggi);
      #endif
      unsigned long Pcont;
      DosResetEventSem( Vuota, &Pcont );    // semaforo gestito in post
      FermaSuGet ++;
      FreeMutex();
      DosWaitEventSem( Vuota, SEM_INDEFINITE_WAIT );  // wait su semaforo coda vuota
      // ==> In questo punto potrebbe nel frattempo essere ridiventata piena!
      GetMutex(); // Riottengo il semaforo per poter leggere i dati
      FermaSuGet --;
      if(Inibito){
         #ifdef DBGthread
         TRACESTRING("Get non completato perche' richiesto un Clear, coda: "+Nome+" Ritorno NULL");
         #endif
         FreeMutex();
         return NULL;
      }
      #ifdef DBGthread
      TRACESTRING("Fine wait su coda vuota Coda "+Nome);
      #endif
//<<< while CodaVuota
   }
   BOOL CodaEraPiena = CodaPiena();
   void * Out = Coda[IndiceLeggi];
   Coda[IndiceLeggi] = NULL;
   #ifdef  DBGthread
   TRACESTRING("Letto elemento {"+STRINGA(IndiceLeggi)+"} da coda "+Nome);
   #endif
   if(++IndiceLeggi >= FifoDim ) IndiceLeggi = 0 ;
   ULONG Rc = 0;
   if(CodaEraPiena)Rc = DosPostEventSem(Piena )  ;  // rilascio semaforo di coda piena
   #ifdef DBGthread
   // TRACESTRING("Get di percorso alfa: "+((PERCORSO_ALFA*)Out)->StrClusters());
   #endif
   FreeMutex();
   if(Rc && Rc != ERROR_ALREADY_POSTED){ // Comunque NON torno NULL
      ERRSTRING("Errore nel POST coda piena su "+Nome+" "+STRINGA(DecodificaRcOS2(Rc)));
      BEEP;
   }
   return Out;
//<<< void * MT_FIFO::Get
};

void MT_FIFO::Clear(BOOL Destroy){
   // Pulisce la lista; Termina eventuali Get e Put in sospeso
   // Se Inibit e' TRUE inibisce ulteriori utilizzi della coda
   #undef TRCRTN
   #define TRCRTN "MT_FIFO::Clear"
   if(Bad)return;                          // Nessuna operazione ammessa
   GetMutex();
   Inibito = TRUE;
   IndiceLeggi = IndiceScrivi = 0;         // Vuoto la coda
   memset(Coda,0,FifoDim * sizeof(void*)); // Vuoto la coda
   if(Destroy)Bad = 99999999;
   DosPostEventSem(Vuota );                // rilascio semaforo di coda vuota
   DosPostEventSem(Piena );                // rilascio semaforo di coda Piena
   FreeMutex();
};
// 2
