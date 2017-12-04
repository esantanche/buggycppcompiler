//----------------------------------------------------------------------------
// ALFA3.CPP : continua alfa.cpp
//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

// EMS001
typedef unsigned long BOOL;

// EMS002
#define __WIN_FASTCALL

#define MODULO_OS2_DIPENDENTE
#define INCL_DOSPROCESS

#include "BASE.hpp"
#include "alfa0.hpp"
#include "mm_detta.hpp"
#include "mm_crit2.hpp"
#include "mm_path.hpp"
#include "mm_combi.hpp"

//----------------------------------------------------------------------------
// Debug opzionale
//----------------------------------------------------------------------------
//#define SHOW_IO_TIME      // Mostra i tempi di I/O
//#define  DBG4             // Per debug di GetCluster
//#define  DBG4A            // Per debug di GetCluster : Solo accessi fisici
//#define  DBG4B            // Mostra i dati del cluster prima e dopo la riorganizzazione (Usa metodo in MM_COMBI.CPP ...)
//#define  DBGsoluz         // Per Debug della valorizzazione soluzioni
//#define  DBGPV            // Segue nel dettaglio il caricamento dei dati di DATI_ORARIO_FS
//#define  DBGClust         // Trace su situazione complessiva di I/O (Livello di trace 1) e memoria allocata
//#define  DBGClust1        // Controlla quanti clusters siano effettivamente stati utilizzati
//#define  DBGClust2  10    // Scrive un trace ogni 10  I/O
//#define  DBGLOAD          // Mostra nei dettagli il caricamento delle aree iniziali: SOLO se si e' caricato un piccolo sottoinsieme dell' orario
//----------------------------------------------------------------------------
#define  TRCE               // togliere se non si vuole TRACE per errore
//----------------------------------------------------------------------------
// Dimensionamento della cache interna di I/O
//----------------------------------------------------------------------------
#define POOL_SIZE  180 // Max Numero elementi
#define POOL_SIZE1 120 // Elementi < 2K
#define POOL_SIZE2  40 // Elementi < 16 K
#define POOL_SIZE3  20 // Elementi > 16 K
//----------------------------------------------------------------------------
#if POOL_SIZE != (POOL_SIZE1 + POOL_SIZE2 + POOL_SIZE3)
#error "POOL con dimensioni errate"
#endif

//----------------------------------------------------------------------------
// Questa define e' il nome della DLL
//----------------------------------------------------------------------------
#ifndef NOME_MOTORE
#define NOME_MOTORE "MOTORE"
#endif

// Questa variabile di ambiente mi fa tenere i dati di orario TUTTI IN MEMORIA
static BOOL KEEP_IN_MEMORY = (getenv("ORARI_IN_MEMORIA") != NULL);

int SortSoluzioni( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "SortSoluzioni()"

   SOLUZIONE * A = *(SOLUZIONE **) a;
   SOLUZIONE * B = *(SOLUZIONE **) b;
   return int(A->Ordine) - int(B->Ordine);
};

//----------------------------------------------------------------------------
// DATI STATICI
//----------------------------------------------------------------------------
DATI_ORARIO_FS * DATI_ORARIO_FS::Corrente            ;  // Dati Orario alfa correntemente attivi
BYTE           * DATI_ORARIO_FS::TabelleDeiDatiOrario;  // Allocata con dimensione sufficiente a contenere
ULONG            DATI_ORARIO_FS::AreaAllocataPerOrari;
ELENCO           DATI_ORARIO_FS::All                 ;
BOOL             DATI_ORARIO_FS::AutoDiagnostica     ;
BOOL             DATI_ORARIO_FS::AutoDiagnosticaPath ;
ARRAY_ID         DATI_ORARIO_FS::StazioniTarget      ;
SOL_IMAGE        DATI_ORARIO_FS::Target              ; // La soluzione su cui faccio autodiagnostica
DATI_POOL      * CLU_BUFR::Pool;
ULONG            CLU_BUFR::ProgressivoCorrente;
int              CLU_BUFR::NumClustersLetti[3];
int              CLU_BUFR::NumBytesLetti[3];
ID             * CLU_BUFR::TabClust;
WORD           * CLU_BUFR::TabNumNodi;
BYTE             CLU_BUFR::LastProg = 0xff;
BYTE           * CLU_BUFR::FastAccess ;            // Tabella per accesso rapido ai cluster

//----------------------------------------------------------------------------
// GRUPPO::operator STRINGA
//----------------------------------------------------------------------------
_export GRUPPO::operator STRINGA() const {
   #undef TRCRTN
   #define TRCRTN "GRUPPO::operator  STRINGA"
   STRINGA Out;
   if(Byte_4) Out += STRINGA((void*)Byte_4)(6,7);
   Out += ((void*)Bytes_0_3);
   return Out;
};
//----------------------------------------------------------------------------
// DATI_ORARIO_FS:: Gestione caricamento veloce
//----------------------------------------------------------------------------
BOOL DATI_ORARIO_FS::Activate(){ // Attiva i Dati Orario (carica in memoria i suoi dati fondamentali)
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::Activate()"

   if(Corrente == this)return TRUE; // E' gia' impostato

   PROFILER::Cronometra(19,TRUE);
   if(Corrente != NULL){
      if(Corrente->LimiteNodi > Corrente->BaseNodi)Corrente->Free();
      Corrente= NULL;
   }

   //TRACESTRING("Inizio");

   Corrente = this;

   // Eventuale (Ri)allocazione dell' area che contiene i Dati Orario alfa
   if(AreaAllocataPerOrari < Header.TotalSize){
      TRACESTRING("Riallocazione area Dati Orario");
      if(TabelleDeiDatiOrario){
         PROFILER::Somma(0, -AreaAllocataPerOrari);
         PROFILER::Somma(2, -AreaAllocataPerOrari);
         free(TabelleDeiDatiOrario);
      }
      // Arrotondo l' area allocata ad un numero intero di pagine
      AreaAllocataPerOrari = Header.TotalSize;
      AreaAllocataPerOrari += 4095;
      AreaAllocataPerOrari -= AreaAllocataPerOrari % 4096;
      TabelleDeiDatiOrario = (BYTE*)malloc(AreaAllocataPerOrari);
      TRACEVLONG( AreaAllocataPerOrari );
      TRACEVPOINTER(TabelleDeiDatiOrario);
      PROFILER::Somma(0, AreaAllocataPerOrari);
      PROFILER::Somma(2, AreaAllocataPerOrari);
   };


   if (TabelleDeiDatiOrario == NULL) {
      ERRSTRING("NON e' allocata l' area 'TabelleDeiDatiOrario'");
      PROFILER::Cronometra(19,FALSE);
      BEEP;
      return FALSE;
   } /* endif */

   // Determino il TimeStamp della Dll del motore
   F_DTM TmsMotDll = GetTimestampDll(NOME_MOTORE);

   // Controllo se esiste il file STARTMOT.ORE e se ha la stessa data/ora di VALIDITA.DB
   FILE_RW  FastStart(PathFast +"STARTMOT.ORE");
   FILE_RO  Validita(PathOrario+"VALIDITA.DB");
   BOOL ReBuild = TRUE; // Indica che debbo ricostruire i dati

   // Rimetto a posto i puntatori alle varie aree
   ULONG OffSet   = sizeof(HEADER);  // In testa ho l' header
   TabIdInterni   = (ID*)(TabelleDeiDatiOrario + OffSet);  // Area di decodifica degli ID
   OffSet += Header.TotIdInterni   * sizeof(ID);
   TabIdEsterni   = (ID*)(TabelleDeiDatiOrario + OffSet);  // Area di decodifica degli ID
   OffSet += (Header.NumNodiDiCambio + 3) * sizeof(ID);
   AllPeriodicita = (PERIODICITA*)(TabelleDeiDatiOrario + OffSet);  // Area delle periodicita'
   OffSet += Header.TotPeriodicita * sizeof(PERIODICITA);
   NodiDiCambio   = (NODO_CAMBIO*)(TabelleDeiDatiOrario + OffSet);  // Area dei nodi di cambio
   OffSet += (Header.NumNodiDiCambio + 3) * sizeof(NODO_CAMBIO);
   StzClusters    = (STAZN_CLUS *)(TabelleDeiDatiOrario + OffSet);  // Area dei collegamenti Stazioni -> clusters
   OffSet += Header.NumStzCluster        * sizeof(STAZN_CLUS );
   Clusters       = (CLUSTER  *)(TabelleDeiDatiOrario + OffSet);    // Area dei Clusters
   OffSet += (Header.NumClusters+1)  * sizeof(CLUSTER)      ;
   Collegamenti   = (COLLEGAMENTO*)(TabelleDeiDatiOrario + OffSet); // Area dei collegamenti diretti tra stazioni
   OffSet += Header.NumCollegamenti      * sizeof(COLLEGAMENTO);

   memset( TabIdInterni, 0, sizeof(ID) * Header.TotIdInterni);
   memset( TabIdEsterni, 0, sizeof(ID) * (Header.NumNodiDiCambio+3));

   TRACEVPOINTER(TabIdInterni);
   TRACEVPOINTER(AllPeriodicita);

   BaseNodi         = Header.NumNodiDiCambio     ;
   NumClusters      = Header.NumClusters         ;

   if(OffSet != Header.TotalSize){
      ERRSTRING("I conti non tornano!");
      TRACEVLONGL(Header.TotalSize,1);
      TRACEVLONGL(OffSet,1);
      BEEP;
   } else if(FastStart.FileHandle() && FastStart.FileSize() > 0 && FastStart.FileTms() == Validita.FileTms()){
      // Utilizzo i dati gia' presenti su file
      if(!FastStart.Leggi(TabelleDeiDatiOrario,FastStart.FileSize())){
         ERRSTRING("HAI! Non riuscito a leggere i dati del motore");
         BEEP;
      } else {
         HEADER & Head2 = *(HEADER*) TabelleDeiDatiOrario;

         if(TmsMotDll != Head2.TimeStampMotore){
            ERRSTRING("Cambiato Timestamp di " NOME_MOTORE ".DLL: Ricarico i dati del motore");
         } else {
            ReBuild = FALSE; // posso utilizzare i dati gia' presenti
            LimiteNodi       = Header.NumNodiDiCambio;
            TRACESTRING("Letti i dati di STARTMOT.ORE");
         }
      }
//<<< if OffSet != Header.TotalSize
   }

   // begin EMS003 Win
   // Nell'impossibilità di determinare il timestamp della DLL del motore (non più esistente
   // causa compilazione sotto forma di LIB) anche a causa della non disponibilità di API
   // opportune, forzo la ricostruzione dei dati che avverrebbe solo in caso di variazione
   // del timestamp
   //ReBuild = TRUE;
   //FastStart.Leggi(TabelleDeiDatiOrario,FastStart.FileSize());
   if(!FastStart.Leggi(TabelleDeiDatiOrario,FastStart.FileSize())){
      ERRSTRING("HAI! Non riuscito a leggere i dati del motore ++++");
      BEEP;
   }
   {
   //HEADER & Head2 = *(HEADER*) TabelleDeiDatiOrario;
   LimiteNodi = Header.NumNodiDiCambio;
   ReBuild = FALSE;
   TRACESTRING("+++++++++++++++++++++++++++++++++++++++++++++++");
   TRACEVLONG(ReBuild);
   TRACEVLONG(LimiteNodi);
   }
   // end EMS003

   if(ReBuild){
      ERRSTRING("Debbo ricostruire i dati a partire dai dati trasferiti");
      LimiteNodi       = 0;

      //--------------------------------------
      // Init
      //--------------------------------------
      int i;
      memset(TabelleDeiDatiOrario ,0,Header.TotalSize);
      NODO_CAMBIO::NodiDiCambio = NULL;

      // ............................................
      // 1) Apertura Files
      // ............................................
      TRACESTRING("Apertura Files");
      CLUSTA      * Clust     ;
      BUFR        * BufferExt ;       // Per la lettura dell' estensione
      Clust        = new CLUSTA(PathOrario,"CLUSSTAZ.DB");

      FILE_RO     Periodic (PathOrario + "PERIODIC.DB");
      int NumPer =  Periodic.FileSize() / sizeof(PERIODICITA);
      if (NumPer != Header.TotPeriodicita) {
         ERRSTRING("Dati inconsistenti, files disallineati o corrotti!");
         BEEP;
      } else {
         Periodic.Leggi(AllPeriodicita,NumPer * sizeof(PERIODICITA));
         #ifdef DBGLOAD
         TRACESTRING( VRS(NumPer) + VRS(NumPer*sizeof(PERIODICITA)));
         #endif
      } /* endif */

      // ............................................
      // 2) Identifica le stazioni di cambio e le aggiunge.
      // ............................................
      TRACESTRING("Caricamento stazioni di Cambio (MM_STAZN.DB)");
      BS_STAZIONE_F & FileStaz = * FilStaz;

      // begin EMS004 Win Rec viene portato fuori ciclo
      STAZIONE_F Rec;
      ORD_FORALL(FileStaz,Stz2){
         //STAZIONE_F & Rec = FileStaz[Stz2];
         Rec = FileStaz[Stz2];
         if(Rec.Id == 0)continue;
         if(Rec.StazioneDiCambio()){
            #ifdef DBGLOAD
            TRACESTRING(VRS(Stz2) + VRS(Rec.Id) + VRS(Rec.TipoStazione) + VRS(Rec.Peso));
            #endif
            if (!DefinisciNodoDiCambio(Rec.Id)) { // L' aggiunta del nodo di cambio e' andata male
               TabIdInterni[Rec.Id]=0;
               #ifdef TRCE
               TRACESTRING("Fallita aggiunta nodo di cambio, ID = "+STRINGA(Rec.Id));
               #endif
               PROFILER::Cronometra(19,FALSE);
               BEEP;
               return FALSE;
            } /* endif */
         }
      }
      // end EMS004

      assert2(LimiteNodi == BaseNodi,LimiteNodi);

      TRACESTRING("Effettuato caricamento stazioni di cambio");

      // ............................................
      // 3) Caricamento dei dati dei collegamenti
      // ............................................
      TRACESTRING("Caricamento dati dei collegamenti (MM_COLL.DB)");
      static BYTE PENALITA[32] =
      //  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31
      {   7,  7,  6,  5,  5,  4,  4,  4,  3,  3,  3,  3,  2,  2,  2,  2,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
      FIX_COLLEGAMENTO_F FCollegamenti(PathOrario+ "MM_COLL.DB");
      TRACESTRING("Sto caricando i collegamenti ALFA3.CPP 273");
      F_STAZN_CLUS & FClusters = * StazCluster;
      TRACEVLONG(FCollegamenti.Dim());
      TRACEVLONG(FClusters.Dim());
      NODO_CAMBIO::TotNodiDiCambio = Header.NumNodiDiCambio+1; // +1 perche' l' elemento 0 non e' valorizzato
      CLUSTER::NumClusters         = Header.NumClusters;
      NODO_CAMBIO::NodiDiCambio    = NodiDiCambio;
      NODO_CAMBIO::AllCollegamenti = Collegamenti;

      NODO_CAMBIO::StzClusters     = StzClusters ;
      CLUSTER::Clusters            = Clusters    ;
      TRACEVLONG(NODO_CAMBIO::TotNodiDiCambio)   ;
      TRACEVLONG(CLUSTER::NumClusters);
      assert( NODO_CAMBIO::TotNodiDiCambio == BaseNodi + 1);
      int Last = -1;
      NODO_CAMBIO * Nodo = NULL;

      int Count = 0;
      // begin EMS005 Win Cst non è più un reference e lo stesso per Coll
      // Coll, che era prima un reference s COLLEGAMENTO, è ora una define
      // che sostituisce nel sorgente l'oggetto referenziato
      // Cst che era un reference a COLLEGAMENTO_F, è ora un COLLEGAMENTO_F
      //COLLEGAMENTO Coll;  diventa:
      #define Coll NODO_CAMBIO::AllCollegamenti[Count]
      COLLEGAMENTO_F Cst;
      ORD_FORALL(FCollegamenti,i1){ // Il file contiene i soli collegamenti tra nodi di cambio
         Cst = FCollegamenti[i1];
         if(Cst.StazionePartenza != Last){
            Last = Cst.StazionePartenza;
            Nodo = NODO_CAMBIO::NodiDiCambio +  Interno(Cst.StazionePartenza);
            Nodo->Id = Cst.StazionePartenza;
            Nodo->Citta = 0;
            Nodo->ClasseCoincidenza = 0;
            if(Count > 0)Count += 2; // Margine per stazioni non nodali
            Nodo->IdxCollegamenti = Count;
            Nodo->NumeroCollegamenti = 0;
         } /* endif */
         BOOL Traghetto =
            (Grafo[Cst.StazionePartenza].Abilitazione & STAZ_FS::SARDEGNA) !=
            (Grafo[Cst.StazioneArrivo].Abilitazione & STAZ_FS::SARDEGNA);

         Coll.Id            = Interno(Cst.StazioneArrivo);
         Coll.Km            = Cst.KmMinimi;
         Coll.PesoTratta    = Cst.PesoTratta;
         // EMS Win la riga seguente proviene dalla versione di alfa3.cpp
         // che si trova in \integraz\subsys\motore\source
         Coll.Privilegiato  = Cst.Privilegiato ;
         Coll.Penalita      = PENALITA[Cst.Count];
         Coll.OrariPartenza = Cst.OrariPartenza;
         Coll.OrariArrivo   = Cst.OrariArrivo  ;
         Coll.TcollMin      = Cst.TcollMin     ;
         Coll.TcollMax      = Cst.TcollMax     ;
         Coll.Partenza32    = Cst.Partenza32   ;
         Coll.Arrivo32      = Cst.Arrivo32     ;
         if(Traghetto){ // Disabilito la logica di orari di partenza ed arrivo
            // Necessario solo per gli orari a 32 Bits: per gli orari a 10 Bits e'
            // gi… considerato.
            Coll.Partenza32    = Coll.Arrivo32  = 0xffffffff;
         }

         Nodo->NumeroCollegamenti ++;
         Count ++;

      //<<< ORD_FORALL FCollegamenti,i1   // Il file contiene i soli collegamenti tra nodi di cambio
      }
      #undef Coll
      // end EMS005 Win

      // ............................................
      // 4) Caricamento delle relazioni stazione-cluster
      // ............................................
      Last = -1;
      Nodo = NULL;
      BOOL Inibit = FALSE;
      Count = 0;
      TRACESTRING("Caricamento delle relazioni stazione-cluster (MM_STCLU.DB)");
      ORD_FORALL(FClusters,j){ // Il file contiene TUTTE le relazioni Stazione -> Cluster
         STAZN_CLUS & Scl = FClusters.FixRec(j);
         if(Scl.Stazione != Last){
            Last = Scl.Stazione;
            Inibit = Interno(Scl.Stazione) == 0; // Non e' un nodo di cambio
            if(!Inibit){
               Nodo = NODO_CAMBIO::NodiDiCambio +  Interno(Scl.Stazione);
               Nodo->IdxClusters = Count ;
               Nodo->NumeroClusters = 0;
            }
         } /* endif */
         if(!Inibit){
            STAZN_CLUS & Scl2 = NODO_CAMBIO::StzClusters[Count++];
            Scl2 = Scl;
            Scl2.Stazione = Interno(Scl2.Stazione);
            Nodo->NumeroClusters ++;
            #ifdef DBGLOAD
            TRACESTRING( VRS(Scl.Stazione) +VRS(Scl2.Stazione) +VRS(Scl.IdCluster) +VRS(Scl.Progressivo) +VRS(Scl.Gruppi) );
            #endif
         }
      }
      // ............................................
      // 5) Caricamento dei dati delle stazioni
      // ............................................
      TRACESTRING("Caricamento dei dati delle stazioni (MM_STAZN.DB)");
      ORD_FORALL(FileStaz,Stz5){
         STAZIONE_F & Rec = FileStaz[Stz5];
         ID Id = Interno(Rec.Id);
         if (Id) {  // E' un nodo di cambio  (altrimenti i dati li carico dopo)
            NODO_CAMBIO::NodiDiCambio[Id].ClasseCoincidenza = Rec.ClasseCoincidenza ;
            NODO_CAMBIO::NodiDiCambio[Id].FascePartenza     = Rec.FascePartenza     ;
            NODO_CAMBIO::NodiDiCambio[Id].FasceArrivo       = Rec.FasceArrivo       ;
            NODO_CAMBIO::NodiDiCambio[Id].Citta             = Rec.Citta             ;
            #ifdef DBGLOAD
            TRACESTRING(VRS(Rec.Id)+VRS(Rec.ClasseCoincidenza)+VRS(Rec.FascePartenza)+VRS(Rec.FasceArrivo)+VRS(Rec.Citta));
            #endif
         }/*endif*/
      }

      // ............................................
      // 6) Caricamento dei dati dei clusters
      // ............................................
      ZeroFill(Clusters[0]);
      assert(Clust->NumRecordsTotali() == Header.NumClusters);
      TRACESTRING("Caricamento dei dati dei clusters");
      for (i = 1; i <= Header.NumClusters ; i++) {
         CLUSTER & Clu = Clusters[i];
         Clust->Posiziona(i-1);
         assert(i == Clust->RecordCorrente().IdCluster );
         Clu.IdCluster    = i                                   ;
         Clu.OffSetBytes  = Clust->RecordCorrente().OffSetBytes ;
         Clu.DimDatiC     = Clust->RecordCorrente().DimDatiC    ;
         Clu.DimDatiD     = Clust->RecordCorrente().DimDatiD    ;
         Clu.Tipo         = Clust->RecordCorrente().TipoCluster ;
         Clu.Id1          = Clust->RecordCorrente().Id1         ;
         Clu.Id2          = Clust->RecordCorrente().Id2         ;
         #ifdef DBGLOAD
         TRACESTRING("Cluster Nø "+STRINGA(i)+ VRS(Clu.Id1) + VRS(Clu.Id2) + VRS(Clu.OffSetBytes) + VRS(Clu.DimDatiC) + VRS(Clu.DimDatiD) );
         #endif
      } /* endfor */

      // ---------------------------------------------------
      // Se e' andato tutto OK allora salvo i dati caricati
      // ---------------------------------------------------
      Header.IndirizzoIniziale = (ULONG)TabelleDeiDatiOrario;
      Header.TimeStampMotore = TmsMotDll;
      memcpy(TabelleDeiDatiOrario , &Header, sizeof(Header));
      FastStart.SetSize(0);
      FastStart.Scrivi( TabelleDeiDatiOrario , Header.TotalSize);
      FastStart.SetTms(Validita.FileTms()); // Imposto stessa data ed ora del file Validita.db

//<<< if ReBuild
   } else {
      NODO_CAMBIO::NodiDiCambio    = NodiDiCambio;
      NODO_CAMBIO::AllCollegamenti = Collegamenti;
      NODO_CAMBIO::StzClusters     = StzClusters;
      NODO_CAMBIO::TotNodiDiCambio = Header.NumNodiDiCambio + 1; // +1 perche' l' elemento 0 non e' valorizzato
      CLUSTER::NumClusters         = Header.NumClusters;
      CLUSTER::Clusters            = Clusters;
   };

   CLU_BUFR::ClearPool();                     // Acquisisco la cache di I/O

   TRACESTRING("Fine");
   PROFILER::Cronometra(19,FALSE);


   return TRUE;
//<<< BOOL DATI_ORARIO_FS::Activate    // Attiva i Dati Orario  carica in memoria i suoi dati fondamentali
}
//----------------------------------------------------------------------------
// DATI_ORARIO_FS::CleanUp
//----------------------------------------------------------------------------
void DATI_ORARIO_FS::CleanUp(){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::CleanUp"
   CLU_BUFR::ClearPool2();           // Pulizia parziale
   PATH_CAMBI::Allocator.Clear();    // e tutti i path cambi
   PATH_CAMBI::HeapPercorsi.Clear(); // e relativa Heap
   PATH_CAMBI::HashIllegali.Clear(); // e l' hash con i sotto-percorsi illegali
   PATH_CAMBI::LastCosto = 0;
   IPOTESI::Cache.Clear();
};
//----------------------------------------------------------------------------
// DATI_ORARIO_FS:: ValorizzaSoluzioni()
//----------------------------------------------------------------------------
// Esegue la valorizzazione tariffaria in base alla normativa FS
//----------------------------------------------------------------------------
void DATI_ORARIO_FS::ValorizzaSoluzioni(){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::ValorizzaSoluzioni()"
   #ifdef  DBGsoluz
   TRACESTRING("Inizio");
   #endif

   PROFILER::Cronometra(7,TRUE);
   Grafo.AbilitaCumulativo = TRUE; // E' il percorso che mi dice se tariffare sul cumulativo

   if(OraOrd == 0xffff)OraOrd = Time() / 6000;
   int i;
   for (i=Soluzioni.Dim()-1;i>=0 ;i-- ) {
      Soluzioni[i]->Ordine = 0; // Viene calcolato dall' interfaccia
      PERCORSO_GRAFO & Perg = *Grafo.PercorsiGrafo[Soluzioni[i]->Percorso];
      if(&Perg == NULL){
         ERRSTRING("Attenzione: Soluzione senza percorso su grafo");
         Soluzioni[i]->Trace(1);
         BEEP;
         continue;
      }
      if (Perg.Istr == NULL) {  // Se non e' gia' tariffato
         Perg.LunghezzaTariffabile(1); // calcolo le tariffe FS, tenendo conto delle eccezioni
      } /* endif */
      // TRACEPOINTER("Soluzione Nø "+STRINGA(i)+" Istr=",Perg.Istr);
   } /* endfor */
   #ifdef  DBGsoluz
   TRACESTRING("Fine");
   TRACESTRING("Soluzioni valide Nø "+STRINGA(Soluzioni.Dim())+" Instradamenti Nø"+
      STRINGA(Grafo.Instradamenti.Dim())+" Percorsi Nø"+
      STRINGA(Grafo.PercorsiGrafo.Dim()));
   #endif
   PROFILER::Cronometra(7,FALSE);
//<<< void DATI_ORARIO_FS::ValorizzaSoluzioni
}
//----------------------------------------------------------------------------
// DATI_ORARIO_FS::HEADER:: Trace()
//----------------------------------------------------------------------------
void DATI_ORARIO_FS::HEADER::Trace(const STRINGA&  Msg, int Livello ){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::HEADER.Trace()"
   if(Livello >trchse)return;

   ERRSTRING(Msg);
   TRACEVLONGL(TotalSize       ,1 );
   TRACEVLONGL(NumNodiDiCambio ,1 );
   TRACEVLONGL(NumClusters     ,1 );
   TRACEVLONGL(NumCollegamenti ,1 );
   TRACEVLONGL(NumStzCluster   ,1 );
   TRACEVLONGL(TotIdInterni    ,1 );
   TRACEVLONGL(TotPeriodicita  ,1 );
   TRACEVPOINTER(IndirizzoIniziale );
};

//----------------------------------------------------------------------------
// Funzioni di CLU_BUFR
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ~CLU_BUFR
//----------------------------------------------------------------------------
CLU_BUFR::~CLU_BUFR(){
   #undef TRCRTN
   #define TRCRTN "~CLU_BUFR"
   if(KEEP_IN_MEMORY){
      Clear();
      Dati = NULL;
      Alloc = 0;
   }
}
//----------------------------------------------------------------------------
// @CLU_BUFR
//----------------------------------------------------------------------------
CLU_BUFR::CLU_BUFR(ULONG Size): BUFR(Size) {
   #undef TRCRTN
   #define TRCRTN "@CLU_BUFR"
   Dat = NULL;Nodi = NULL;Treno = NULL;
};

//----------------------------------------------------------------------------
// CLU_BUFR::KmMediDaId
//----------------------------------------------------------------------------
short int CLU_BUFR::KmMediDaId(ID Id){
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::KmMediDaId"
   // EMS006 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
   SCAN_NUM_WIN(Nodi,Dat->TotStazioni,Nod,CLUSSTAZ){
      if (Nod->IdNodo == Id)return Nod->MediaKm;
   } ENDSCAN ;
   return 0; // Per tornare qualcosa
};

//----------------------------------------------------------------------------
// CLU_BUFR::EsisteNodo
//----------------------------------------------------------------------------
INFOSTAZ * CLU_BUFR::EsisteNodo(ID Id){
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::EsisteNodo"
   // EMS006 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
   SCAN_NUM_WIN(Nodi,Dat->TotStazioni,Nod,CLUSSTAZ){
      if (Nod->IdNodo == Id) {
         INFOSTAZ  & No  = Nodo(SCAN_IDX2(Nod,Dat->TotStazioni));
         return &No;
      }
   } ENDSCAN ;
   return NULL;
};

//----------------------------------------------------------------------------
// DATI_POOL::Clear
//----------------------------------------------------------------------------
void DATI_POOL::Clear(){  // Quando e' chiamato sono in modalita' monothread o gia' sotto semaforo
   #undef TRCRTN
   #define TRCRTN "DATI_POOL::Clear"
   Stato = LIBERO;
   Cluster  = 0;
   Concorde = TRUE;
   NumInvalidate = 0;
   Origine   = Destinazione = 0; // Per forzare la riorganizzazione dell' area

   if(KEEP_IN_MEMORY){
      if(Bufr.Dat){
         #ifdef DBG4B
         int SlotNo = this - CLU_BUFR::Pool;
         TRACESTRING("Slot Nø " + STRINGA(SlotNo) + " Clear dei dati del cluster Nø "+STRINGA(Bufr.Dat->IdCluster)+" Set "+VRS(Bufr.Dat->NumeroNodi)+" a "+STRINGA(NumeroNodi));
         #endif
      }
      Bufr.Clear();
   } else {
      Bufr.ReDim(0);                     // Libera FISICAMENTE l' area allocata
   };
};
//----------------------------------------------------------------------------
// PosNodoInCluster
//----------------------------------------------------------------------------
// Questa funzione cerca un nodo tra i dati del cluster, e ne ritorna l' indice (o -1 se non trovato)
int __WIN_FASTCALL CLU_BUFR::PosNodoInCluster(ID Id){  // Id e' l' ID Esterno del nodo
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::PosNodoInCluster"

   CLUSSTAZ  * CNodi = Nodi; // Array con i nodi del cluster
   // Cerco nel cluster l' ID della stazione
   for (WORD i = 0; i < Dat->NumeroNodi ; i++,CNodi ++) {
      if(CNodi->IdNodo == Id ) return i;
   } /* endfor */
   return -1;
};
//----------------------------------------------------------------------------
// ClearPool ...
//----------------------------------------------------------------------------
void CLU_BUFR::ClearPool2(){        // Quando e' chiamato sono in modalita' monothread o comunque in quel momento e' attivo un solo thread
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::ClearPool2"
   for (int i = 0 ; i < POOL_SIZE ;i++ ) {
      DATI_POOL & Item = Pool[i];
      Item.Clear();
      if(Item.Stato != LIBERO  && Item.NumRichiesta != LastProg){
         Item.Clear();
      } else if(Item.Stato == INLETTURA ){ // Non dovrebbe mai capitare pero' ...
         Item.Clear();
      } else if(Item.Stato == DATILETTI ){
         Item.Stato = DATIUSATI;
      }
   } /* endfor */
}
//----------------------------------------------------------------------------
// CLU_BUFR::ClearPool
//----------------------------------------------------------------------------
void _export CLU_BUFR::ClearPool(BOOL Dealloca ){ // Quando e' chiamato sono in modalita' monothread
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::ClearPool"

   int Size = 0;
   if (Dealloca) {
      // Sono in fase di deallocazione risorse Oppure di controllo memoria
      TRACESTRING("Deallocazione completa aree di memoria del POOL");
      if(Pool){
         CLU_BUFR::ProgressivoCorrente = 1;
         for (int i = 0;i < POOL_SIZE ;i++ ) Pool[i].Clear();
         delete[] Pool;
         Pool = NULL;
      }
      if(FastAccess){
         free(FastAccess);
         FastAccess = NULL;
      }
      if(TabNumNodi){
         free(TabNumNodi);
         TabNumNodi = NULL;
      }
      #ifdef DBGClust1
      if(TabClust){
         free(TabClust);
         TabClust = NULL;
      }
      #endif
//<<< if  Dealloca
   } else {
      // Sto semplicemente attivando un altro orario per cui debbo svuotare i buffers non utilizzati
      if(Pool == NULL ) Pool = new DATI_POOL[POOL_SIZE];
      PROFILER::Somma(0, sizeof(DATI_POOL) * POOL_SIZE );
      PROFILER::Somma(2, sizeof(DATI_POOL) * POOL_SIZE );
      Size= 2 * Orario.NumClusters;
      // FastAccess e' un indice che indica DOVE nel pool e' stato immagazzinato
      // ogni cluster l' ultima volta.
      FastAccess = (BYTE*) realloc(FastAccess,Size);
      PROFILER::Somma(0, Size);
      PROFILER::Somma(2, Size);
      memset(FastAccess,0,Size);
      if (KEEP_IN_MEMORY) {
         TabNumNodi = (WORD*) realloc(TabNumNodi,sizeof(WORD)*Size);
         memset(TabNumNodi,0,sizeof(WORD)*Size);
         PROFILER::Somma(0, sizeof(WORD)*Size);
         PROFILER::Somma(2, sizeof(WORD)*Size);
      }
      TRACESTRING("Deallocazione aree di memoria");
      CLU_BUFR::ProgressivoCorrente = 1;
      for (int i = 0;i < POOL_SIZE ;i++ ) Pool[i].Clear();
      #ifdef DBGClust1
      TabClust = (ID*)realloc(TabClust,2*Size);
      memset(TabClust,0,2*Size);
      #endif
   } /* endif */

   ZeroFill(NumClustersLetti);
   ZeroFill(NumBytesLetti);
//<<< void _export CLU_BUFR::ClearPool BOOL Dealloca    // Quando e' chiamato sono in modalita' monothread
};


//----------------------------------------------------------------------------
// CLU_BUFR::FreeCluster
//----------------------------------------------------------------------------
void __WIN_FASTCALL CLU_BUFR::FreeCluster(ID IdCluster , BYTE Concorde){
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::FreeCluster"

   #if defined(DBG4)
   STRINGA Ncluster = "Cluster "+STRINGA(Concorde ? "":"#")+STRINGA(IdCluster);
   #ifdef DBG4
   TRACESTRING("Rilasciato "+Ncluster);
   #endif
   #endif

   int   FA_Point = IdCluster - 1;
   if(Concorde)FA_Point += Orario.NumClusters;

   int IdxSlot = FastAccess[FA_Point]; // Ricorda l' ultimo slot utilizzato
   DATI_POOL *  Slot = Pool + IdxSlot;

   // Se lo stato e' DATILETTI non ho problemi di concorrenza in quanto lo SLOT
   // non puo' assolutamente essere stato riutilizzato.
   // Ho ritenuto che l' overhead e le complicazioni per gestire un contatore di utilizzo
   // non valessero la pena. Pertanto se ho altre richieste pending i dati divengono
   // comunque liberabili , nel caso improbabile venissero sovrascritti si dovranno rileggere.
   if(Slot->Stato == DATILETTI && Slot->Cluster==IdCluster && Slot->Concorde==Concorde){
      Slot->Stato = DATIUSATI; // Questo stato permette di riutilizzarel o slot.
   }
//<<< void __WIN_FASTCALL CLU_BUFR::FreeCluster ID IdCluster , BYTE Concorde
}

//----------------------------------------------------------------------------
// CLU_BUFR::GetCluster
//----------------------------------------------------------------------------
CLU_BUFR * __WIN_FASTCALL CLU_BUFR::GetCluster(ID IdCluster , BYTE Concorde, BYTE PuoiEseguireIO ){
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::GetCluster"

   #if defined(PROFILER_ABILITATO) && defined(ENABLE_INNER)
   PROFILER::Cronometra(17,TRUE);
   #elif defined(PROFILER_ABILITATO )
   PROFILER::Conta(17);
   #endif
   #if defined(DBG4) || defined(DBG4A)
   STRINGA Ncluster = "Cluster "+STRINGA(Concorde ? "":"#")+STRINGA(IdCluster);
   #ifdef DBG4
   TRACESTRING("Richiesto "+Ncluster+" PuoiEseguireIO = "+STRINGA(PuoiEseguireIO));
   #endif
   #endif

   assert(IdCluster != 0);
   assert(IdCluster <= Orario.NumClusters );

   // -----------------------------------------------------------------------------------
   // Gestione nuova richiesta: Qui sono ancora in monothread
   // Invalido tutti i clusters che hanno corrotte origine e/o destinazione
   // Riorganizzo quelli che le hanno valide ma in una colonna sbagliata.
   // Nota: perdo qualcosa (pochissimo) in performance facendolo subito, ma evitando le
   // riorganizzazioni mentre eseguo evito il rischio di corruzione dei dati.
   // Si ricordi che l' accesso al POOL non e' semaforizzato.
   // Se si pospone la riorganizzazione si deve rivedere tutta la logica successiva
   // -----------------------------------------------------------------------------------
   if (DATI_ORARIO_FS::ProgressivoRichiesta != LastProg) {
      TRACESTRING("Riorganizzazione dati di tutti i Cluster attivi");
      LastProg = DATI_ORARIO_FS::ProgressivoRichiesta ;
      ID Origine      = Orario.EIdOrigine();      // Id Esterno
      ID Destinazione = Orario.EIdDestinazione(); // Id Esterno
      // Per tutti i clusters con dati validi
      for (int i = 0;i < POOL_SIZE ;i++ ) {
         DATI_POOL *  Slot = Pool + i;
         if(Slot->Stato >= DATILETTI){ // Ha dati validi
            if(!Slot->RiorganizzaDatiCluster(Origine,Destinazione)){ // Riorganizzo i dati
               Slot->Clear(); // Non ci sono riuscito
            } else {
               Slot->Stato = DATIUSATI; // Ok
            }
         } else if(Slot->Stato != LIBERO){
            Slot->Clear(); // Il caso e' anomalo: implica una chiusura forzata della ricerca precedente
         };
      } /* endfor */
      #ifdef DBGClust
      ZeroFill(NumClustersLetti);
      ZeroFill(NumBytesLetti);
      #endif
//<<< if  DATI_ORARIO_FS::ProgressivoRichiesta != LastProg
   } /* endif */

   int   FA_Point = IdCluster - 1;
   if(Concorde) FA_Point +=  Orario.NumClusters;

   #ifdef DBGClust1
   TabClust[FA_Point] = DATI_ORARIO_FS::ProgressivoRichiesta;
   #endif


   int IdxSlot = FastAccess[FA_Point]; // Ricorda l' ultimo slot utilizzato
   DATI_POOL *  Slot = Pool + IdxSlot;
   #ifdef DBG4
   TRACESTRING("Slot Nø"+STRINGA(IdxSlot)+ " Occupato dal cluster "+STRINGA(Slot->Concorde ? "":"#")+STRINGA(Slot->Cluster));
   #endif


   // Controllo se ho ancora i dati nello SLOT. Dovrei anche controllare che lo stato NON sia
   // LIBERO ma posso farne a meno in quanto l' operazione di CLEAR mette IdCluster a 0;
   // (che e' un valore illegale)
   if(Slot->Cluster==IdCluster && Slot->Concorde==Concorde ){

      // Questa istruzione protegge per diversi millisecondi lo slot. E' come proteggerlo con un semaforo
      // Si noti che questa NON e' comunque una operazione distruttiva e non crea problemi in caso
      // di accesso concorrente mentre aggiorno Slot->Progressivo
      Slot->Progressivo    =  ProgressivoCorrente;  // Protezione

      // Ripeto il test perche' mentre aggiornavo avrei potuto avere un accesso concorrente
      if(Slot->Cluster==IdCluster && Slot->Concorde==Concorde){

         // Se i dati sono in lettura
         if( Slot->Stato == INLETTURA){
            // Se era solo un test ritorno dicendo che i dati non sono presenti
            if(!PuoiEseguireIO){
               #if defined(PROFILER_ABILITATO) && defined(ENABLE_INNER)
               PROFILER::Cronometra(17,FALSE);
               #endif
               #ifdef DBG4
               TRACESTRING("Al momento i dati sono in lettura: NON SONO PERTANTO DISPONIBILI");
               #endif
               return NULL;
            }
            // Se arriva qui il caso e' anomalo: vuol dire che un buffer e' stato invalidato mentre
            // lo avevo in coda di elaborazione, poi e' stato di nuovo richiesto da due
            // threads contemporaneamente.
            // Gestisco il tutto con un ciclo brutale
            ERRSTRING("Caso anomalo: Puo' essere un errore di logica; Possibile LOOP in tal caso");
            while (Slot->Stato == INLETTURA) {
               DosSleep(50);
               Slot->Progressivo    =  ProgressivoCorrente;  // Per mantenere la protezione
            } /* endwhile */
         };

         // Se lo stato era DATIUSATI allora la richiesta li deve rendere riutilizzabili e non sostituibili
         if( Slot->Stato == DATIUSATI ) Slot->Stato = DATILETTI;     // Aggiorno lo stato

         // Aggiorno il progressivo di utilizzo dei dati
         Slot->Progressivo  = ++ProgressivoCorrente;
         Slot->NumRichiesta = DATI_ORARIO_FS::ProgressivoRichiesta;

         #ifdef DBG4
         TRACESTRING(Ncluster+STRINGA(" Trovato nel POOL, posizione = ")+STRINGA(FastAccess[FA_Point]));
         #endif
         #if defined(PROFILER_ABILITATO) && defined(ENABLE_INNER)
         PROFILER::Cronometra(17,FALSE);
         #endif
         return &Slot->Bufr;

         #ifdef DBG4
//<<< if Slot->Cluster==IdCluster && Slot->Concorde==Concorde
      } else {
         TRACESTRING("Accesso concorrente: non posso utilizzare lo slot");
         #endif
      } /* endif */
      #ifdef DBG4
//<<< if Slot->Cluster==IdCluster && Slot->Concorde==Concorde
   } else if(Slot->Cluster == 0){
      TRACESTRING("Slot libero: debbo rileggere i dati");
   } else {
      TRACESTRING("Slot riutilizzato: non posso utilizzare i dati");
      #endif
   } /* endif */

   // Se son qui non ho gia' i dati e Slot NON e' valido
   // Se era solo un test ritorno
   if(!PuoiEseguireIO){
      #if defined(PROFILER_ABILITATO) && defined(ENABLE_INNER)
      PROFILER::Cronometra(17,FALSE);
      #endif
      return NULL;
   }

   // Debbo leggere da file

   CLUSTER & Clus   = CLUSTER::Clusters[IdCluster];
   assert(Clus.IdCluster == IdCluster);
   int   Dimensione, Posizione;
   if (Concorde) {
      Dimensione= Clus.DimDatiC;
      Posizione = Clus.OffSetBytes;
   } else {
      Dimensione= Clus.DimDatiD;
      Posizione = Clus.OffSetBytes + Clus.DimDatiC;
   }

   #if defined(SHOW_IO_TIME) || defined(DBG4)
   TRACESTRING("Lettura del "+Ncluster+" Bytes:"+VRS(Dimensione) + VRS(Dimensione));
   #endif


   // Identificazione della parte del POOL da utilizzare
   int Min,Max ;
   int PoolIdx;
   if (Dimensione <= 2000) {
      Min       = 0;                        Max = POOL_SIZE1;
      PoolIdx   = 0;
   } else if (Dimensione <= 16000) {
      Min       = POOL_SIZE1;               Max = Min + POOL_SIZE2;
      PoolIdx   = 1;
   } else {
      Min       = POOL_SIZE1 + POOL_SIZE2;  Max = Min + POOL_SIZE3;
      PoolIdx   = 2;
   } /* endif */


   BOOL Segnala = TRUE;
   // Identificazione dello slot su cui eseguire l' I/O
   do {    // Se non identifico nessuno slot utilizzabile aspetto finche non se ne libera uno
      if(MultiThread)DosEnterCritSec();
      Slot = Pool + Min; // Prima ipotesi: utilizzo il primo slot
      for (int j= Min;j < Max ;j++ ) {
         DATI_POOL *  SlotJ = Pool + j;
         if(SlotJ->Stato == LIBERO){
            // Ok inutile cercare ancora
            Slot =  SlotJ;
            break;
         } else if(SlotJ->Stato == DATIUSATI && (Slot->Stato != DATIUSATI || Slot->Progressivo > SlotJ->Progressivo)){
            Slot =  SlotJ;
         } /* endif */
      } /* endfor */
      if (Slot->Stato != LIBERO && Slot->Stato != DATIUSATI){
         if(Segnala){
            TraceTotalIo();
            if (MultiThread) {
               ERRINT("Non ho identificato nessuno slot utilizzabile! Wait finche' non ne libero UNO; PoolIdx = ",PoolIdx);
            } else {
               ERRINT("Non ho identificato nessuno slot utilizzabile! Ritorno con errore ; PoolIdx = ",PoolIdx);
               BEEP;
               return NULL; // Ritorno con errore !
            } /* endif */
            Segnala = FALSE;
         };
         Slot = NULL;
      } else {
         Slot->Progressivo   = ProgressivoCorrente; // Non e' necessario incrementarlo: lo faro' quando uso i dati
         Slot->NumRichiesta  = DATI_ORARIO_FS::ProgressivoRichiesta;
         Slot->Stato         = INLETTURA          ;
         IdxSlot = (Slot - Pool); // Numero dello slot
         #ifdef DBG4
         TRACESTRING(Ncluster+STRINGA(" sostituisce il cluster ")+STRINGA(Slot->Concorde ? "":"#")+STRINGA(Slot->Cluster)+" in posizione = "+STRINGA(IdxSlot));
         #endif
         FastAccess[FA_Point] = IdxSlot ;
         Slot->Cluster        = IdCluster;
         Slot->Concorde       = Concorde;
//<<< if  Slot->Stato != LIBERO && Slot->Stato != DATIUSATI
      } /* endif */
      if(MultiThread)DosExitCritSec();
      if (Slot == NULL)DosSleep(20); // Wait 20 millisecondi
//<<< do      // Se non identifico nessuno slot utilizzabile aspetto finche non se ne libera uno
   } while ( Slot == NULL ); /* enddo */

   // ..............................
   // Esecuzione I/O fisico
   // ..............................
   #if defined(DBG4) || defined(DBG4A)
   TRACESTRING("Eseguo I/O del cluster "+STRINGA(Ncluster)+" nello Slot Nø "+STRINGA(IdxSlot)+" Byts Nø "+STRINGA(Dimensione));
   #endif
   if(KEEP_IN_MEMORY){
      if (Orario.AllExt.Length == 0) {  // Non ho ancora letto i dati
         Orario.ClustExt->Leggi(Orario.ClustExt->FileSize(),0,Orario.AllExt);
         PROFILER::Somma(0, Orario.ClustExt->FileSize());
      } /* endif */
      Slot->Bufr.Dati   = Orario.AllExt.Dati + Posizione;
      Slot->Bufr.Length = Dimensione;
   } else {
      PROFILER::Cronometra(18,TRUE);
      Orario.ClustExt->Leggi(Dimensione,Posizione,Slot->Bufr); // Leggo e sovrascrivo il buffer
      PROFILER::Cronometra(18,FALSE);
   }

   Slot->Bufr.Set();  // Attivo i dati
   Slot->Bufr.Tipo     = Clus.Tipo;
   Slot->Bufr.Concorde = Concorde;

   // Debug
   #ifdef DBG4
   TRACESTRING("Terminato I/O del cluster "+Ncluster);
   TRACEVLONG(Slot->Bufr.Dat->NumeroNodi);
   TRACEVPOINTER(Slot->Bufr.Dat);
   #endif
   #ifdef DBGClust
   NumBytesLetti[PoolIdx] += Dimensione;
   NumClustersLetti[PoolIdx] ++;
   #ifdef DBGClust2
   if((NumClustersLetti[PoolIdx] % DBGClust2) == 0){
      TraceTotalIo();
   };
   #endif
   #endif
   // TRACEHEX("Dati del cluster in esadecimale",Slot->Bufr.Dati,Slot->Bufr.Length);

   // Ora riorganizzo i dati dello slot
   if (KEEP_IN_MEMORY) {
      if ( TabNumNodi[FA_Point] == 0 ) {
         TabNumNodi[FA_Point] = Slot->NumeroNodi = Slot->Bufr.Dat->NumeroNodi ; // Salvo il numero nodi per le riorganizzazioni
         #if defined(DBG4) || defined (DBG4A)
         TRACESTRING("Letto da file NumeroNodi =" + STRINGA(Slot->NumeroNodi) + VRS(FA_Point) );
         #endif
      } else {
         Slot->NumeroNodi = TabNumNodi[FA_Point] ;
         TRACESTRING("Ripristinato NumeroNodi =" + STRINGA(Slot->NumeroNodi) + VRS(FA_Point) );
      } /* endif */
   } else {
      Slot->NumeroNodi    = Slot->Bufr.Dat->NumeroNodi ; // Salvo il numero nodi per le riorganizzazioni
   } /* endif */
   Slot->Origine       = Slot->Destinazione = 0;      // Invalido finche' non riorganizzo
   Slot->NumInvalidate = 0;
   ID Origine          = Esterno(Orario.IdStazioneOrigine)     ; // Id Esterno
   ID Destinazione     = Esterno(Orario.IdStazioneDestinazione); // Id Esterno
   if(!Slot->RiorganizzaDatiCluster(Origine,Destinazione)){  // Riorganizzo i dati per l' origine
      // E' anomalo che ritorni FALSE;
      ERRSTRING("Errore riorganizzando i dati dello Slot: seguira' quasi sicuramente Abend");
      BEEP;
      Slot->Clear(); // Non sono dati validi
      #if defined(PROFILER_ABILITATO) && defined(ENABLE_INNER)
      PROFILER::Cronometra(17,FALSE);
      #endif
      return NULL;
   }


   // OK: Termino l' operazione di I/O aggiornando lo stato dello slot
   Slot->Stato = DATILETTI; // Ok
   #ifdef SHOW_IO_TIME
   TRACEINT("Letto il Cluster ed impostato",IdCluster);
   #endif

   #if defined(PROFILER_ABILITATO) && defined(ENABLE_INNER)
   PROFILER::Cronometra(17,FALSE);
   #endif
   return &Slot->Bufr;
//<<< CLU_BUFR * __WIN_FASTCALL CLU_BUFR::GetCluster ID IdCluster , BYTE Concorde, BYTE PuoiEseguireIO
};


//----------------------------------------------------------------------------
// DATI_POOL::RiorganizzaDatiCluster
//----------------------------------------------------------------------------
BOOL DATI_POOL::RiorganizzaDatiCluster(ID Org, ID Dest){
   #undef TRCRTN
   #define TRCRTN "DATI_POOL::RiorganizzaDatiCluster"

   // Riorganizza i dati del cluster rendendo la stazione
   // Staz stazione di cambio (se non lo e' gia')
   // Torna FALSE se non e' possibile riorganizzare i dati.
   // Gli ID sono esterni
   // E' sempre eseguita in monothread:
   //    Sul thread di generazione percorsi alfa al primo I/O della richiesta
   //    Poi solo sul thread di I/O

   if(Org == Origine && Dest == Destinazione) return TRUE; // E' gia' Ok

   #ifdef DBG4B
   TRACESTRING("Dump Cluster Nø " +STRINGA(Bufr.Dat->IdCluster)+" prima della riorganizzazione; " VRS(Org) + VRS(Dest));
   Bufr.Dump(0,0);   // Da ed A sono Id Esterni
   #endif

   // Invalido eventuali altri dati gia' presenti
   Bufr.Dat->NumeroNodi = NumeroNodi; // Ripristino il numero nodi al valore che aveva prima delle modifiche


   // Init
   BOOL DestinazioneAggiunta = FALSE;
   Origine = Org ; Destinazione = Dest;
   WORD & Target = Bufr.Dat->NumeroNodi;

   // Gestione Origine
   if (!Orario.NodoDiCambio(Origine)){
      // Vedo se e' gia' OK: Frequente in caso di richieste ripetute
      if(Bufr.Nodi[Target].IdNodo == Origine){ // Impedisco la ricopertura
         Target ++;
      } else {

         int i=0; // Indice del nodo
         // EMS006 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
         SCAN_NUM_WIN(Bufr.Nodi,Bufr.Dat->TotStazioni,Nodo,CLUSSTAZ){
            if (Nodo->IdNodo == Origine) {
               if (i < Bufr.Dat->NumeroNodi){
                  #ifdef TRCE
                  ERRINT("Errore: nodo fittizio "+STRINGA(Origine)+" e' in realta' di cambio su cluster",Bufr.Dat->IdCluster);
                  #endif
                  BEEP;
               } else {
                  #ifdef DBG4
                  TRACEVLONG(Target);
                  TRACEVPOINTER(&Target);
                  #endif
                  if(Bufr.Nodi[Target].IdNodo == Destinazione){ // Impedisco la ricopertura
                                         Target ++ ; // La valido
                                         #ifdef DBG4
                                         TRACESTRING("Inibizione Ricopertura");
                                         TRACELONG("Nuovo valore di target per aggiunta stazione destinazione: ",Target);
                                         #endif
                                         DestinazioneAggiunta = TRUE;
                  };
                  if (i != Target){ // Non e' il primo
                                         // Qui avrei la scelta tra fare uno swap dei dati o gestire una logica di invalidazione.
                                         // Faccio lo swap per permettere (opzionalmente) di tenere tutti i dati in memoria
                                         CLUSSTAZ Tmp1 =  Bufr.Nodi[Target];
                                         Bufr.Nodi[Target] = Bufr.Nodi[i] ; // Copio i dati del nodo
                                         Bufr.Nodi[i] = Tmp1;
                                         // E copio anche i dati di tutti i treni
                                         Bufr.FindTreno(0);
                                         for (int j=0;j < Bufr.Dat->NumeroTreni ; j++) {
                                            INFOSTAZ Tmp2 = Bufr.Treno->Nodi[Target];
                                            Bufr.Treno->Nodi[Target] = Bufr.Treno->Nodi[i];
                                            Bufr.Treno->Nodi[i] = Tmp2;
                                            Bufr.NextTreno();
                                         } /* endfor */
                  } /* endif */
                  Target ++; // Attivo il nodo fittizio
                  #ifdef DBG4
                  TRACELONG("Nuovo valore di target per aggiunta stazione origine: ",Target);
                  #endif
//<<<          if  i < Bufr.Dat->NumeroNodi
               } /* endif */
               break;
//<<<       if  Nodo.IdNodo == Origine
            } /* endif */
            i++;
//<<<    SCAN_NUM Bufr.Nodi,Bufr.Dat->TotStazioni,Nodo,CLUSSTAZ
         } ENDSCAN ;
         Bufr.FindTreno(0);         // Per lasciare la situazione pulita
//<<< if Bufr.Nodi Target .IdNodo == Origine   // Impedisco la ricopertura
      }
//<<< if  !Orario.NodoDiCambio Origine
   } /* endif */

   // La logica per la destinazione e' praticamente eguale a quella per l' origine
   if (!Orario.NodoDiCambio(Destinazione) && ! DestinazioneAggiunta) {
      int i=0;
      // EMS006 Win sostituisco SCAN_NUM_WIN a SCAN_NUM
      SCAN_NUM_WIN(Bufr.Nodi,Bufr.Dat->TotStazioni,Nodo,CLUSSTAZ){
         if (Nodo->IdNodo == Destinazione) {
            if (i < Bufr.Dat->NumeroNodi){
               #ifdef TRCE
               ERRINT("Errore: nodo fittizio "+STRINGA(Destinazione)+" e' in realta' di cambio su cluster",Bufr.Dat->IdCluster);
               #endif
               BEEP;
            } else {
               #ifdef DBG4
               TRACEVLONG(Target);
               TRACEVPOINTER(&Target);
               #endif
               if (i != Target){ // Non e' il primo
                  // Qui avrei la scelta tra fare uno swap dei dati o gestire una logica di invalidazione.
                  // Faccio lo swap per permettere (opzionalmente) di tenere tutti i dati in memoria
                  CLUSSTAZ Tmp1 =  Bufr.Nodi[Target];
                  Bufr.Nodi[Target] = Bufr.Nodi[i] ; // Copio i dati del nodo
                  Bufr.Nodi[i] = Tmp1;
                  // E copio anche i dati di tutti i treni
                  Bufr.FindTreno(0);
                  for (int j=0;j < Bufr.Dat->NumeroTreni ; j++) {
                                         INFOSTAZ Tmp2 = Bufr.Treno->Nodi[Target];
                                         Bufr.Treno->Nodi[Target] = Bufr.Treno->Nodi[i];
                                         Bufr.Treno->Nodi[i] = Tmp2;
                                         Bufr.NextTreno();
                  } /* endfor */
               } /* endif */
               Target ++; // Attivo il nodo fittizio
               #ifdef DBG4
               TRACELONG("Nuovo valore di target per aggiunta stazione destinazione: ",Target);
               #endif
//<<<       if  i < Bufr.Dat->NumeroNodi
            } /* endif */
            break;
//<<<    if  Nodo.IdNodo == Destinazione
         } /* endif */
         i++;
//<<< SCAN_NUM Bufr.Nodi,Bufr.Dat->TotStazioni,Nodo,CLUSSTAZ
      } ENDSCAN ;
      Bufr.FindTreno(0);
//<<< if  !Orario.NodoDiCambio Destinazione  && ! DestinazioneAggiunta
   } /* endif */
   #ifdef DBG4B
   TRACESTRING("Dump Cluster dopo la riorganizzazione; " VRS(Org) + VRS(Dest));
   Bufr.Dump(0,0);   // Da ed A sono Id Esterni
   #endif
   return TRUE; // OK
//<<< BOOL DATI_POOL::RiorganizzaDatiCluster ID Org, ID Dest
};


//----------------------------------------------------------------------------
// CLU_BUFR::TraceTotalIo
//----------------------------------------------------------------------------
void _export CLU_BUFR::TraceTotalIo(){
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::TraceTotalIo"

   int i ;

   #ifdef DBGClust
   ERRSTRING(VRS(DATI_ORARIO_FS::ProgressivoRichiesta));

   typedef  int  STATO[4];
   int   AreaAllocata[3];ZeroFill(AreaAllocata);
   STATO StatoSlots[3]  ;ZeroFill(StatoSlots);
   int TotAllocata=0;
   for (i = 0;i < POOL_SIZE ;i ++ ) {
      TotAllocata += Pool[i].Bufr.Dim();
      if(i < POOL_SIZE1){
         AreaAllocata[0] += Pool[i].Bufr.Dim();
         StatoSlots[0][Pool[i].Stato] ++;
      } else if(i < POOL_SIZE1 + POOL_SIZE2){
         AreaAllocata[1] += Pool[i].Bufr.Dim();
         StatoSlots[1][Pool[i].Stato] ++;
      } else {
         AreaAllocata[2] += Pool[i].Bufr.Dim();
         StatoSlots[2][Pool[i].Stato] ++;
      }
   } /* endfor */

   int Nc=0,Tb=0;
   for (i = 0;i < 3 ;i++ ) {
      ERRSTRING("NumClustersLetti["+STRINGA(i)+"]="+STRINGA(NumClustersLetti[i])+" NumBytesLetti="+STRINGA(NumBytesLetti[i])+" AreaAllocata ="+STRINGA(AreaAllocata[i])+
         " NumSlotsLiberi ="+STRINGA(StatoSlots[i][0]) +" NumSlotsInLettura ="+STRINGA(StatoSlots[i][1]) +" NumSlotsLetti ="+STRINGA(StatoSlots[i][2]) +" NumSlotsUsati ="+STRINGA(StatoSlots[i][3]) );
      Nc += NumClustersLetti[i];
      Tb += NumBytesLetti[i];
   } /* endfor */
   ERRSTRING("Totale NumClustersLetti="+STRINGA(Nc)+" NumBytesLetti="+STRINGA(Tb)+" AreaAllocata ="+STRINGA(TotAllocata));
   #endif

   #ifdef DBGClust1
   ULONG Count = 0;
   for (i = 2 * Orario.NumClusters -2;i > 0 ;i-- ) {
      if( TabClust[i] == DATI_ORARIO_FS::ProgressivoRichiesta) Count ++;
   } /* endfor */
   ERRSTRING("La soluzione del problema ha richiesto "+STRINGA(Count)+" clusters / verso ");
   #endif

   #ifdef DBGClust
   HeapTest(TRCRTN);
   SOLUZIONE::Allocator.Trace("Soluzioni attualmente allocate:",1);
   // Mem_Ctrl();
   #endif
//<<< void _export CLU_BUFR::TraceTotalIo
};

//----------------------------------------------------------------------------
// VALIDITA_ORARIO::OrarioValido
//----------------------------------------------------------------------------
// Mi dice se l' orario e' valido
BOOL VALIDITA_ORARIO::OrarioValido(const STRINGA & Path){
   #undef TRCRTN
   #define TRCRTN "VALIDITA_ORARIO::OrarioValido"

   FILE_RO Validita(Path+"VALIDITA.DB");
   if (Validita.Rc() != 0 || Validita.FileHandle() == 0) {
      return FALSE;
   } else if (Validita.FileSize() < sizeof(VALIDITA_ORARIO)){
      ERRSTRING("Non valido il file "+Path+ "VALIDITA.DB");
      return FALSE;
   } else {
      VALIDITA_ORARIO ValiditaOrario;
      if(Validita.Leggi(VRB(ValiditaOrario))){
         if (ValiditaOrario.FormatoDeiDati == ORARIO_FORMATO_ATTUALE) {
            ValiditaOrario.Trace("Validita' dell' orario letto da File");
         } else {
            ERRSTRING("Vecchio formato dei dati : ignoro il file "+Path+ "VALIDITA.DB");
            return FALSE;
         } /* endif */
      } else {
         ERRSTRING("Errore: Non riuscito a leggere il file "+Path+ "VALIDITA.DB");
         return FALSE;
      } /* endif */
   } /* endif */
   return TRUE;
//<<< BOOL VALIDITA_ORARIO::OrarioValido const STRINGA & Path
};

//----------------------------------------------------------------------------
// VALIDITA_ORARIO::Trace
//----------------------------------------------------------------------------
void VALIDITA_ORARIO::Trace(const STRINGA&  Msg, int Livello ){
   #undef TRCRTN
   #define TRCRTN "VALIDITA_ORARIO::Trace"
   if(Livello >trchse)return;
   TRACESTRING(Msg);
   TRACEVSTRING2(Inizio         );
   TRACEVSTRING2(Fine           );
   TRACEVSTRING2(InizioUfficiale);
   TRACEVSTRING2(FineUfficiale  );
   TRACEVSTRING2(InizioOraLegale);
   TRACEVSTRING2(FineOraLegale  );
   TRACEVSTRING2(NomeOrario     );
   TRACEVLONG(OraInizioOraLegale  );
   TRACEVLONG(OraFineOraLegale    );
   TRACEVLONG(ProgressivoFornitura);
};
