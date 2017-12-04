//----------------------------------------------------------------------------
// ML_STEP2.CPP: Fase iniziale algoritmo preparazione dati del motore
//----------------------------------------------------------------------------
// Conteggio del numero delle fermate per stazione
// Identificazione dei cluster secondo il criterio di dominanza
// Caricamento delle relazioni base tra stazioni e cluster e tra mezzi virtuali e cluster
// Caricamento degli indici accessori:
//    Stazioni->Cluster
//    Clusters->TreniVirtuali
// Linearizzazione dei percorsi nell' ambito del cluster
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

// EMS001 Win
typedef unsigned long BOOL;

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "ML_IN.HPP"
#include "ML_WRK.HPP"
#include "seq_proc.hpp"
#include "mm_basic.hpp"
#include "scandir.h"
#include "ml_step2.hpp"
#define PGM  "ML_STEP2"

// Aree statiche
// EMS002 VA commento la dich. di Collider già presente in MM_PATH.CPP
// static  COLLIDER<WORD,WORD> Collider ;           // Per le operazioni di equivalenza tra percorsi logici

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int  main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"


   char PrtBuf[500];

   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   // EMS003 Win SetStatoProcesso(PGM, SP_PARTITO);
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari

   // EMS003 Win SetPriorita(); // Imposta la priorita'

   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore
   TryTime(0);
   CLASSIFICA_TRENI::CaricaClassifiche(PATH_DATI);

   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);

   // ---------------------------------------------------------
   // Inits
   // ---------------------------------------------------------
   // APRO il grafo (cio' inizializza anche stazioni)
   PROFILER::Clear(FALSE); // Per aprire il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);
   // Creo la classe di Appoggio
   WORK Work;
   int Rc;


   Stazioni[0]; // Per caricare i dati

   Collider.Redim(Stazioni.Dim()+1);

   // ---------------------------------------------------------
   // Conteggio Fermate
   // ---------------------------------------------------------
   Work.ContaFermate();

   // ---------------------------------------------------------
   // Scansione Treni, Identificazione Stazioni dominanti, Caricamento cluster e relazioni varie
   // ---------------------------------------------------------
   Work.ScandisciTreni();

   // ---------------------------------------------------------
   // Scarica su file le relazioni trovate e linearizza le distanze
   // ---------------------------------------------------------
   Work.ScaricaRelazioni();

   // ---------------------------------------------------------
   //  Scansione Treni Nø 2: identifica relazione fermate - cluster
   //  Scarica su files i relativi dati
   // ---------------------------------------------------------
   Rc = Work.ScandisciTreni2();

   // ---------------------------------------------------------
   // Controllo di aver eseguito bene la linearizzazione delle distanze
   // ---------------------------------------------------------
   if(Rc == 0)Rc = Work.IdentificaTreniNonLinearizzati();

   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------

   TRACESTRING("PROGRAMMA TERMINATO");

   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;

   return Rc;
//<<< int  main int argc,char *argv
}

// ---------------------------------------------------------
// Classe di appoggio: Costruttore
// ---------------------------------------------------------
WORK::WORK() :
TabTv(PATH_OUT "M0_TRENV.TMP"),
TabFv(PATH_OUT "M0_FERMV.TMP"),
CollStaz(16000),
CollClustStaz(64000),
ClustersIdx(4000),              // EMS004 Win Aumento il numero di celle da 2000 a 4000
Clust(PATH_OUT "MY_CLUST.TMP"),
ClusStaz(PATH_OUT "CLUSSTAZ.TMP"),
ClusMezv(PATH_OUT "CLUSMEZV.TMP"),
CollegamentiTraStazioni(PATH_OUT "COLLSTAZ.TMP" , 64000),
CollegamentiTraStazioniNelCluster(PATH_OUT "COLLCLUS.TMP",64000)
{
   #undef TRCRTN
   #define TRCRTN "@WORK"
   Staz2 = (STAZIONE_MV*) malloc(sizeof(STAZIONE_MV) * Stazioni.Dim());
   memset(Staz2,0,Stazioni.Dim()*sizeof(STAZIONE_MV));
   for (int i = 0; i < Stazioni.Dim() ; i++) Staz2[i].Id = i;
   Clust.Clear("Tabella Dati cluster");                  // Rendo R/W e vuoto
   CLUSTER_MV Buf2; ZeroFill(Buf2);
   Clust.AddRecordToEnd(VRB(Buf2)); // I cluster cominciano da 1
   ClusStaz.Clear("Relazione cluster stazioni");         // Rendo R/W e vuoto
   ClusMezv.Clear("Relazione cluster mezzi virtuali");   // Rendo R/W e vuoto
   CollegamentiTraStazioni.Clear();
   CollegamentiTraStazioniNelCluster.Clear();
}
//----------------------------------------------------------------------------
// ContaFermate
//----------------------------------------------------------------------------
// La routine conta quante fermate vengono effettuate per ogni stazione
// E' uno step preparatorio necessario per identificare le stazioni dominanti
//----------------------------------------------------------------------------
void WORK::ContaFermate(){
   #undef TRCRTN
   #define TRCRTN "WORK::ContaFermate"
   TRACESTRING("Conteggio fermate");
   // fprintf(Out,"=============================================================\n");
   // fprintf(Out,"Conteggio fermate\n");
   // fprintf(Out,"=============================================================\n");
   Bprintf("Conteggio fermate");
   ORD_FORALL(TabFv,f){
      TryTime(f);
      FERMATE_VIRT Fermata = TabFv.FixRec(f); // Lavoro per copia per permettere l' accesso ad altri records
      if(Fermata.Transito){
         Staz2[Fermata.Id].NumTransiti ++;
      } else {
         Staz2[Fermata.Id].NumFermate ++;
      }
   }
}

// ---------------------------------------------------------
// 1ø Scansione Treni
// ---------------------------------------------------------
// Operazioni svolte:
//   - Identificazione Stazioni dominanti
//   - Identificazione Cluster, scrittura su file ed aggiornamento relazione in memoria cluster-stazioni
//   - Generazione Array di fermate del mezzo virtuale
//   - Caricamento dei collegamenti diretti in memoria
// ---------------------------------------------------------
void WORK::ScandisciTreni(){
   #undef TRCRTN
   #define TRCRTN "WORK::ScandisciTreni"

   //TRACESTRING("Scansione Treni Nø 1: identifica fermate dominanti e cluster");
   //Bprintf("Scansione Treni Nø 1: identifica fermate dominanti e cluster");

   ARRAY_FERM FermateDelMV;
   WORD LastMV      = 0;
   MEZZOV_CLUSTER C_Mv;  // Cluster cui appartiene il mezzo virtuale
   ZeroFill(C_Mv);
   // prove CLUSTER_MV cmvRecord_corrente; // EMS

   ORD_FORALL(TabFv,f){
      TryTime(f);
      FERMATE_VIRT Fermata = TabFv.FixRec(f); // Lavoro per copia per permettere l' accesso ad altri records
      // Bprintf("%i %i %i \n", Fermata.MezzoVirtuale, Fermata.Progressivo, LastMV  );
      if (Fermata.MezzoVirtuale != LastMV) {
         TabTv.Seek(LastMV);
         if(LastMV != 0 && TabTv.RecordCorrente().NumeroFermateValide != FermateDelMV.Dim()){
            assert(LastMV == 0 || TabTv.RecordCorrente().NumeroFermateValide == FermateDelMV.Dim());
            //Bprintf("LastMV = %i  fermate da file MV = %i contate = %i",LastMV,TabTv.RecordCorrente().NumeroFermateValide,FermateDelMV.Dim());
         }
         MM_INFO Wrk;
         Wrk.TipoMezzo = TabTv.RecordCorrente().TipoMezzoDominante;
         FermateDelMV.Navale =  Wrk.Navale();
         if (FermateDelMV.Dim() > 1) { // Skip dei mezzi virtuali inutili
            int IdClu = FermateDelMV.IdentificaCluster(ClustersIdx,Clust,LastMV,TabTv.RecordCorrente());

            C_Mv.MezzoVirtuale = LastMV   ;
            C_Mv.IdCluster     = IdClu    ;
            C_Mv.Id1           = Chk(Clust.RecordCorrente().Id1,13);
            C_Mv.Id2           = Chk(Clust.RecordCorrente().Id2,13);
            C_Mv.TipoCluster   = Chk(Clust.RecordCorrente().TipoCluster,4);
            C_Mv.Distanza1     = Chk(FermateDelMV.Distanza1,12);
            C_Mv.Distanza2     = Chk(FermateDelMV.Distanza2,12);
            C_Mv.VersoConcorde = FermateDelMV.Distanza2 >= FermateDelMV.Distanza1;
            assert(FermateDelMV.Distanza2 != FermateDelMV.Distanza1);
            C_Mv.OraPartenza   = FermateDelMV.OraP1;
            // Non carico per ora GRUPPO ed INFO
            /* EMS Win prove
            if (C_Mv.Id1 == 0 || C_Mv.Id2 == 0) {
               TRACEINT("Scritto record irregolare in clusmezv.tmp LastMV=", LastMV);
               TRACEINT("C_Mv.Id1 = ",C_Mv.Id1);
               TRACEINT("C_Mv.Id2 = ",C_Mv.Id2);
               TRACEINT("Clust.NumRecordCorrente()=", Clust.NumRecordCorrente());
               TRACEINT("Clust.NumRecordsTotali()=",  Clust.NumRecordsTotali());
               cmvRecord_corrente = Clust.RecordCorrente();
               TRACEINT("cmvRecord_corrente.Id1=", cmvRecord_corrente.Id1);
               TRACEINT("cmvRecord_corrente.Id2=", cmvRecord_corrente.Id2);
            }
            */
            ClusMezv.AddRecordToEnd(VRB(C_Mv));
            // Bprintf("MV %i Cluster %i Distanza1 %i" ,ClusMezv.RecordCorrente().MezzoVirtuale ,ClusMezv.RecordCorrente().IdCluster ,ClusMezv.RecordCorrente().Distanza1);
            //TRACESTRING("Mezzo virtuale "+STRINGA(LastMV)+" Cluster: "+STRINGA(IdClu));

            // Adesso gestisco e carico le informazioni di COLLEGAMENTO
            FermateDelMV.CaricaCollegamenti( CollStaz, CollClustStaz);

//<<<    if  FermateDelMV.Dim   > 1    // Skip dei mezzi virtuali inutili
         } /* endif */
         LastMV = Fermata.MezzoVirtuale ;
         FermateDelMV.Clear();
         FermateDelMV.IdMezzo   = LastMV;
//<<< if  Fermata.MezzoVirtuale != LastMV
      } /* endif */

      if(!Fermata.Transito){
         FERM Ferm;
         Ferm.OraPartenza = Fermata.FermataPartenza ? Fermata.OraPartenza : -1;
         Ferm.OraArrivo   = Fermata.FermataArrivo   ? Fermata.OraArrivo   : -1;
         Ferm.Id     = Fermata.Id                   ;
         Ferm.Peso   = Staz2[Fermata.Id].NumFermate ;
         Ferm.ProgKm = Fermata.ProgKm               ;
         FermateDelMV += Ferm;
         // if(Fermata.MezzoVirtuale > 5508 && Fermata.MezzoVirtuale < 5530)TRACESTRING("MV "+STRINGA(Fermata.MezzoVirtuale)+" Staz Id "+STRINGA(Fermata.Id)+ " Peso "+ STRINGA(Staz2[Fermata.Id].NumFermate)+" Km "+STRINGA(Fermata.ProgKm));
      }
//<<< ORD_FORALL TabFv,f
   }
   // Ultimo treno virtuale
   if (FermateDelMV.Dim() > 1) { // Skip dei mezzi virtuali inutili
      TabTv.Seek(LastMV);
      assert(TabTv.RecordCorrente().NumeroFermateValide == FermateDelMV.Dim());
      int IdClu = FermateDelMV.IdentificaCluster(ClustersIdx,Clust,LastMV,TabTv.RecordCorrente());

      C_Mv.MezzoVirtuale = LastMV   ;
      C_Mv.IdCluster     = IdClu    ;
      C_Mv.Id1           = Chk(Clust.RecordCorrente().Id1,13);
      C_Mv.Id2           = Chk(Clust.RecordCorrente().Id2,13);
      C_Mv.TipoCluster   = Chk(Clust.RecordCorrente().TipoCluster,4);
      C_Mv.Distanza1     = Chk(FermateDelMV.Distanza1,12);
      C_Mv.Distanza2     = Chk(FermateDelMV.Distanza2,12);
      C_Mv.VersoConcorde = FermateDelMV.Distanza2 >= FermateDelMV.Distanza1;
      assert(FermateDelMV.Distanza2 != FermateDelMV.Distanza1);
      C_Mv.OraPartenza   = FermateDelMV.OraP1;
      // Non carico per ora GRUPPO
      ClusMezv.AddRecordToEnd(VRB(C_Mv));
      //TRACESTRING("Mezzo virtuale "+STRINGA(LastMV)+" Cluster: "+STRINGA(IdClu));

      // Adesso gestisco e carico le informazioni di COLLEGAMENTO
      FermateDelMV.CaricaCollegamenti( CollStaz, CollClustStaz);

//<<< if  FermateDelMV.Dim   > 1    // Skip dei mezzi virtuali inutili
   } /* endif */
//<<< void WORK::ScandisciTreni
}
// ---------------------------------------------------------
// Scarico relazioni da memoria a file
// ---------------------------------------------------------
void WORK::ScaricaRelazioni(){
   #undef TRCRTN
   #define TRCRTN "WORK::ScaricaRelazioni"

   Clust.Flush(); // Scarico degli aggiornamenti
   //Bprintf("Generati %i clusters",Clust.NumRecordsTotali()-1);

   // Linearizzazione delle distanze
   ITERA(ClustersIdx,ClusterCorrente,CLUSTER_IDX){
      ClusterCorrente.LinearizzaDistanze(); // Linearizza le distanze e carica Stazioni
   } END_ITERA

   CollStaz.Istogram("Istogramma relazioni tra nodi");
   CollClustStaz.Istogram("Istogramma relazioni Cluster -> Collegamento nodi");

   //Bprintf("Scrittura dei collegamenti stazioni su file");
   ITERA(CollStaz,Rec1,COLLEGAMENTO1){
      CollegamentiTraStazioni.AddRecordToEnd(VRB(Rec1));
   } END_ITERA
   //Bprintf("Scrittura dei collegamenti stazioni (Per cluster) su file");
   ITERA(CollClustStaz,Rec2,COLLEGAMENTO2){
      CollegamentiTraStazioniNelCluster.AddRecordToEnd(VRB(Rec2));
   } END_ITERA
   CollegamentiTraStazioni.Flush();
   CollegamentiTraStazioni.ReSortFAST();
   CollegamentiTraStazioniNelCluster.Flush();
   CollegamentiTraStazioniNelCluster.ReSortFAST();

   //Bprintf("Scrittura istogrammi collegamenti BIDIREZIONALI tra stazioni");
   ITERA(CollStaz,Recs,COLLEGAMENTO1){
      Staz2[Recs.StazionePartenza].NumCollegamenti ++;
      Staz2[Recs.StazioneArrivo].NumCollegamenti ++;
   } END_ITERA
   for (int i = 0; i < Stazioni.Dim() ; i++) {
      Staz2[i].NumCollegamenti /= 2;
   }

   // Istogramma fino a 2048 collegamenti
   WORD Counts[2048];
   ZeroFill(Counts);
   int CountNonColl = 0;
   for(i= Stazioni.Dim()-1; i >= 0 ; i--){
      Counts[Staz2[i].NumCollegamenti] ++;
      if(Staz2[i].NumCollegamenti == 0 && GRAFO::Gr()[i].Vendibile){
         //Bprintf2("WARNING: La stazione vendibile %s NON e' collegata ad altre stazioni", Stazioni[i].NomeStazione);
         CountNonColl ++;
      }
   } /* endfor */
   //if(CountNonColl)Bprintf("Attenzione: Vi sono %i Stazioni vendibili NON collegate: controllare su LOG", CountNonColl);
   //for(i=0;i < 2048; i++)if(Counts[i])Bprintf2("Vi sono %i Stazioni con %i Collegamenti BIDIREZIONALI",Counts[i],i);

   // Sort delle relazioni trovate
   Bprintf("Sort delle relazioni trovate");
   ClusMezv.ReSort(CompareWord);
//<<< void WORK::ScaricaRelazioni
}

// ---------------------------------------------------------
//  Scansione Treni Nø 2: identifica relazione fermate - cluster
// ---------------------------------------------------------
int WORK::ScandisciTreni2(){
   #undef TRCRTN
   #define TRCRTN "WORK::ScandisciTreni2"
   ARRAY_ID StazioniEccezionaliDiInstradamento;
   STRINGA Linea;
   // Le carico dal file StzIsEcc.MIO
   if(TestFileExistance(PATH_POLIM "StzIsEcc.MIO")){
      FILE_RO Ecc(PATH_POLIM "StzIsEcc.MIO");
      while(Ecc.gets(Linea,80)){
         int Id = Linea.ToInt();
         if(Id){
            //Bprintf("Stazione di instradamento eccezionale %i",Id);
            StazioniEccezionaliDiInstradamento += Id;
         }
      }
   }
   //TRACESTRING("Scansione Treni Nø 2: identifica relazione fermate - cluster");
   // fprintf(Out,"=============================================================\n");
   // fprintf(Out,"Scansione Treni Nø 2: identifica relazione fermate - cluster\n");
   // fprintf(Out,"=============================================================\n");
   //Bprintf("Scansione Treni Nø 2: identifica relazione fermate - cluster");
   CLUSTER_STAZIONE_2 Cs2; ZeroFill(Cs2);
   F_CLUSTER_STAZIONE_2 ClusStaz2(PATH_DUMMY "WRK1.TMP");
   ClusStaz2.Clear("Relazione cluster stazioni");         // Rendo R/W e vuoto
   WORD LastMV = 0;
   BOOL Skip = FALSE;
   int KmUltimaFermata, KmLinUltimaFermata , KmMedUltimaFermata ;
   CLUSTER_IDX * IdxCluster = NULL;  // Per linearizzazione distanze
   ORD_FORALL(TabFv,f){
      TryTime(f);
      FERMATE_VIRT Fermata = TabFv.FixRec(f); // Lavoro per copia per permettere l' accesso ad altri records

      // Trovo il treno virtuale ed il relativo cluster
      // Calcolo il DeltaKm dall' ultima FERMATA
      if (Fermata.MezzoVirtuale != LastMV) { // Cambio MV
         LastMV = Fermata.MezzoVirtuale ;
         TabTv.Seek(LastMV);
         if(TabTv.RecordCorrente().NumeroFermateValide < 2){
            Skip = TRUE;
            IdxCluster = NULL;
         } else {
            Skip = FALSE;
            if(!ClusMezv.Seek(LastMV)){
               //fprintf(Out,"Errore: non trovato cluster corrispondente a mezzo virtuale %i\n",LastMV);
               TRACEINT("Errore: non trovato cluster corrispondente a mezzo virtuale LastMV=",LastMV);
               //Bprintf("Errore: non trovato cluster corrispondente a mezzo virtuale %i",LastMV);
               return 999;
            }
            // Bprintf("MV %i Cluster %i Distanza1 %i" ,ClusMezv.RecordCorrente().MezzoVirtuale ,ClusMezv.RecordCorrente().IdCluster ,ClusMezv.RecordCorrente().Distanza1);
            CLUSTER_IDX Target;
            Target.Id1  = ClusMezv.RecordCorrente().Id1 ;
            Target.Id2  = ClusMezv.RecordCorrente().Id2 ;
            Target.Tipo = ClusMezv.RecordCorrente().TipoCluster ;
            if (Target.Id1 == 0) {
               TRACESTRING("Target.Id1 == 0");
            }
            ERRSTRING( VRS(Target.Id1 ) + VRS(Target.Id2 ) + VRS(Target.Tipo) + VRS(LastMV) );
            IdxCluster  = ClustersIdx.Cerca(&Target,5);
            assert (IdxCluster != NULL);
            if(IdxCluster->NumeroRecord != ClusMezv.RecordCorrente().IdCluster){
               // Potrebbe essere un cluster splittato
               Target.Tipo = 15;
               IdxCluster  = ClustersIdx.Cerca(&Target,5);
               assert (IdxCluster != NULL);
               while(IdxCluster->NumeroRecord != ClusMezv.RecordCorrente().IdCluster){
                  IdxCluster  = ClustersIdx.CercaNext();
                  assert (IdxCluster != NULL);
               }
            }
            KmUltimaFermata = -1; // Per gestire stazioni di transito iniziali
//<<<    if TabTv.RecordCorrente  .NumeroFermateValide < 2
         }
//<<< if  Fermata.MezzoVirtuale != LastMV    // Cambio MV
      } /* endif */

      if(Skip)continue;

      int IdxStzInCluster = IdxCluster->Cerca( Fermata.Id );
      assert2(Fermata.Transito || IdxStzInCluster >= 0, VRS(Fermata.Id) + VRS(ClusMezv.RecordCorrente().MezzoVirtuale) + VRS(ClusMezv.RecordCorrente().IdCluster) );
      if(!Fermata.Transito && IdxStzInCluster < 0){
         //ERRSTRING(VRS((void*)IdxCluster->StazLin) + VRS(IdxCluster->NumeroRecord) + VRS(IdxCluster->Id1) + VRS(IdxCluster->Id2));
         for (int i = 0; i < IdxCluster->NumStazioni; i++ ) {
            //ERRSTRING("Stz ["+STRINGA(i)+"] Id = "+STRINGA(IdxCluster->StazLin[i].Id));
         } /* endfor */
      }
      if ( IdxStzInCluster < 0 ) {  // E' sicuramente un transito
         if(KmUltimaFermata < 0){
            continue;  // Ignoro i transiti all' inizio del treno
         } else if(ClusMezv.RecordCorrente().VersoConcorde){
            Cs2.Distanza              = KmLinUltimaFermata + (Fermata.ProgKm - KmUltimaFermata);
            Cs2.DistanzaMediana       = KmMedUltimaFermata + (Fermata.ProgKm - KmUltimaFermata);
            // if(Fermata.Id == 126 && ClusMezv.RecordCorrente().IdCluster == 45)printf("P2 MV %i Dist = %i da %i %i %i\n",Fermata.MezzoVirtuale, Cs2.Distanza ,KmLinUltimaFermata , Fermata.ProgKm ,KmUltimaFermata);
         } else {
            Cs2.Distanza              = KmLinUltimaFermata - (Fermata.ProgKm - KmUltimaFermata);
            Cs2.DistanzaMediana       = KmMedUltimaFermata - (Fermata.ProgKm - KmUltimaFermata);
            // if(Fermata.Id == 126 && ClusMezv.RecordCorrente().IdCluster == 45)printf("P3 MV %i Dist = %i da %i %i %i\n",Fermata.MezzoVirtuale, Cs2.Distanza ,KmLinUltimaFermata , Fermata.ProgKm ,KmUltimaFermata);
         }
      } else {                      // E' una fermata (per questo od altri MV nel cluster)
         KmUltimaFermata    = Fermata.ProgKm ;
         KmLinUltimaFermata = Cs2.Distanza = IdxCluster->StazLin[IdxStzInCluster].Km;
         KmMedUltimaFermata = Cs2.DistanzaMediana = IdxCluster->StazLin[IdxStzInCluster].KmMedi;
      } /* endif */

      if(IsNodo(Fermata.Id)|| StazioniEccezionaliDiInstradamento.Contiene(Fermata.Id)
            ||  !Fermata.Transito){ // Nodo reale o fermata o stazione di istradamento eccezionale
         Cs2.IdCluster                = ClusMezv.RecordCorrente().IdCluster ;
         Cs2.IdStazione               = Fermata.Id                          ;
         // OBSOLETO
         // if(ClusMezv.RecordCorrente().VersoConcorde){
         //    Cs2.Distanza              = Fermata.ProgKm - ClusMezv.RecordCorrente().Distanza1 ;
         // } else {
         //    Cs2.Distanza              = ClusMezv.RecordCorrente().Distanza1 - Fermata.ProgKm ;
         // }
         Cs2.Ferma = !Fermata.Transito;

         // Scrivo sul file di lavoro
         ClusStaz2.AddRecordToEnd(VRB(Cs2));
      }
//<<< ORD_FORALL TabFv,f
   }

   //Bprintf("Sort del file di lavoro e scarico delle relazioni");
   ClusStaz2.ReSort(Compare2Word);

   WORD LastCluster=ClusStaz2[0].IdCluster;     // Inizializzo perche' non scarichi subito
   WORD LastId     =ClusStaz2[0].IdStazione;
   WORD Count= 0;
   short Dist = 0;
   short MDist = 0;
   CLUSTER_STAZIONE Cs; ZeroFill(Cs);
   ORD_FORALL(ClusStaz2,f2){
      TryTime(f2);
      // Al cambio di cluster o stazione
      if(ClusStaz2[f2].IdCluster != LastCluster || ClusStaz2.RecordCorrente().IdStazione!= LastId){
         // Scarico il record
         Cs.IdCluster        =  LastCluster ;
         Cs.IdStazione       =  LastId      ;
         Cs.TipoStazione     =  0           ;
         Cs.Id1              =  Chk(Clust[LastCluster].Id1,13);
         Cs.Id2              =  Chk(Clust[LastCluster].Id2,13);
         Cs.Distanza         =  Dist        ;
         Cs.DistanzaMediana  =  MDist       ;
         Cs.NumMzvFerm       =  Count       ;
         Cs.Prog             =  0           ;
         Cs.Prog2            =  0           ;
         // Gruppo non ancora gestito
         ClusStaz.AddRecordToEnd(VRB(Cs));
         Staz2[LastId].NumClusters ++;
         //TRACESTRING("Stazione "+STRINGA(LastId)+" Cluster "+STRINGA(LastCluster)+" TotClu "+STRINGA(Staz2[LastId].NumClusters));

         LastCluster = ClusStaz2.RecordCorrente().IdCluster ;
         LastId      = ClusStaz2.RecordCorrente().IdStazione;
         Dist        =0 ;
         Count       =0 ;
      };

      // Linearizzazione residua delle distanze (serve solo per le stazioni nodali di transito).
      if(abs(ClusStaz2.RecordCorrente().Distanza) > abs(Dist))Dist = ClusStaz2.RecordCorrente().Distanza;
      MDist = ClusStaz2.RecordCorrente().DistanzaMediana;

      Count += ClusStaz2.RecordCorrente().Ferma;
//<<< ORD_FORALL ClusStaz2,f2
   };
   // Scarico il record
   Cs.IdCluster     =  LastCluster ;
   Cs.IdStazione    =  LastId      ;
   Cs.TipoStazione  =  0           ;
   Cs.Id1           =  Chk(Clust[LastCluster].Id1,13);
   Cs.Id2           =  Chk(Clust[LastCluster].Id2,13);
   Cs.Distanza      =  Dist        ;
   Cs.DistanzaMediana =  MDist     ;
   Cs.NumMzvFerm    =  Count       ;
   Cs.Prog          =  0           ;
   Cs.Prog2         =  0           ;

   ClusStaz.AddRecordToEnd(VRB(Cs));
   Staz2[LastId].NumClusters ++;
   //TRACESTRING("Stazione "+STRINGA(LastId)+" Cluster "+STRINGA(LastCluster)+" TotClu "+STRINGA(Staz2[LastId].NumClusters));

   //Bprintf("Sort relazioni cluster-> stazioni");
   ClusStaz.ReSort(Compare2Word);


   // ---------------------------------------------------------
   // Consolidamento dati
   // ---------------------------------------------------------
   TRACESTRING("Consolidamento dati");
   // fprintf(Out,"=============================================================\n");
   // fprintf(Out,"Consolidamento dati\n");
   // fprintf(Out,"=============================================================\n");
   Bprintf("Consolidamento dati");
   // Consolido i dati
   F_STAZIONE_MV  FStaz2(PATH_OUT "MY_STAZI.TMP");
   FStaz2.Clear("Tabella Dati estesi stazioni");          // Rendo R/W e vuoto
   ORD_FORALL(Stazioni,i1){
      TRACEINT("Sto scrivendo MY_STAZI.TMP i1=", i1);
      TryTime(i1);
      FStaz2.AddRecordToEnd(&Staz2[i1],sizeof(STAZIONE_MV));
      FStaz2.InvalidateBuffer(); // EMS005 Win
   } /* endfor */

   // ---------------------------------------------------------
   // Caricamento degli indici accessori:
   // ---------------------------------------------------------
   //    Stazioni->Cluster
   Bprintf("Caricamento indici accessori");
   F_STAZIONE_CLUSTER  StazClus(PATH_OUT "CLUSSTAZ.IX1");
   StazClus.Clear();
   ORD_FORALL(ClusStaz,k0){
      TryTime(k0);
      ClusStaz.FixRec(k0);
      STAZIONE_CLUSTER  Rec;
      Rec.IdStazione = ClusStaz.RecordCorrente().IdStazione;
      Rec.IdCluster  = ClusStaz.RecordCorrente().IdCluster ;
      StazClus.AddRecordToEnd(VRB(Rec));
   };
   StazClus.ReSortFAST();
   //    Clusters->TreniVirtuali
   F_CLUSTER_MEZZOV  ClusMv(PATH_OUT "CLUSMEZV.IX1");
   ClusMv.Clear();
   ORD_FORALL(ClusMezv,k1){
      TryTime(k0);
      CLUSTER_MEZZOV  Rec;
      ClusMezv.FixRec(k1);
      Rec.IdCluster     = ClusMezv.RecordCorrente().IdCluster     ;
      Rec.VersoConcorde = ClusMezv.RecordCorrente().VersoConcorde ;
      Rec.OraPartenza   = ClusMezv.RecordCorrente().OraPartenza   ;
      Rec.MezzoVirtuale = ClusMezv.RecordCorrente().MezzoVirtuale ;
      ClusMv.AddRecordToEnd(VRB(Rec));
   };
   ClusMv.ReSortFAST();
   ClusStaz2.Clear(); // Non serve piu'
   return 0;
//<<< int WORK::ScandisciTreni2
};

// ---------------------------------------------------------
//  WORK::IdentificaTreniNonLinearizzati
// ---------------------------------------------------------
int WORK::IdentificaTreniNonLinearizzati(){
   #undef TRCRTN
   #define TRCRTN "WORK::IdentificaTreniNonLinearizzati"

   Bprintf("Controllo linearizzazione distanze Mezzi Virtuali");
   MEZZOV_CLUSTER Mv_Clu;
   USHORT LastMV = 0;
   int    LastKm = 0;
   int    LastKmMv = 0;
   int    LastId = 0;
   int NumTreniConFermateInvertite = 0;
   int FermateInvertite = 0;
   BOOL HoFermateInvertite = FALSE;
   BOOL HoDiscordanzaEccessiva = FALSE;
   ORD_FORALL(TabFv,f){
      TryTime(f);
      FERMATE_VIRT & Fermata = TabFv.FixRec(f);
      if(Fermata.Transito)continue;              // I transiti non sono significativi
      // if(Fermata.Transito && !IsNodo(Fermata.Id))continue; // I transiti non sono significativi a meno che non siano nodi
      if (Fermata.MezzoVirtuale != LastMV) {
         LastMV = 0; // Prima fermata di un nuovo mezzo virtuale
         TabTv.Seek(Fermata.MezzoVirtuale);
         if(TabTv.RecordCorrente().NumeroFermateValide < 2)continue; // NON e' valido
         if( HoFermateInvertite )NumTreniConFermateInvertite++;
         HoFermateInvertite = FALSE;
         ClusMezv.Seek(Fermata.MezzoVirtuale);
         Mv_Clu  =  ClusMezv.RecordCorrente();
      } /* endif */
      if(!ClusStaz.Seek(Mv_Clu.IdCluster,Fermata.Id))continue; // Ignoro le stazioni che non stanno sul cluster
      if(LastMV){
         if (Mv_Clu.VersoConcorde) {
            if(ClusStaz.RecordCorrente().Distanza <= LastKm){ // Modificata: ho aggiunto la condizione = per gestire casi anomali
               STRINGA Stz1(Stazioni[LastId].NomeStazione);
               STRINGA Stz2(Stazioni[Fermata.Id].NomeStazione);
               //TRACESTRING("MV "+STRINGA(LastMV)+" stazioni fuori sequenza: "+
               // STRINGA(LastId)+" "+Stz1+" ("+STRINGA(LastKmMv)+"/"+STRINGA(LastKm)+") -> "+
               // STRINGA(Fermata.Id)+" "+Stz2+" ("+STRINGA(Fermata.ProgKm)+"/"+STRINGA(ClusStaz.RecordCorrente().Distanza)+")");
               FermateInvertite ++ ;
               HoFermateInvertite = TRUE ;
            }
         } else {
            if(ClusStaz.RecordCorrente().Distanza >= LastKm){
               STRINGA Stz1(Stazioni[LastId].NomeStazione);
               STRINGA Stz2(Stazioni[Fermata.Id].NomeStazione);
               //TRACESTRING("MVDISC "+STRINGA(LastMV)+" stazioni fuori sequenza: "+
               // STRINGA(LastId)+" "+Stz1+" ("+STRINGA(LastKmMv)+"/"+STRINGA(LastKm)+") -> "+
               // STRINGA(Fermata.Id)+" "+Stz2+" ("+STRINGA(Fermata.ProgKm)+"/"+STRINGA(ClusStaz.RecordCorrente().Distanza)+")");
               FermateInvertite ++ ;
               HoFermateInvertite = TRUE ;
            }
         } /* endif */
//<<< if LastMV
      };
      LastMV = Fermata.MezzoVirtuale ;
      LastKm = ClusStaz.RecordCorrente().Distanza;
      LastKmMv = Fermata.ProgKm;
      LastId = Fermata.Id;
      int KmDaCluster;
      if (Mv_Clu.VersoConcorde) {
         KmDaCluster = Mv_Clu.Distanza1 + ClusStaz.RecordCorrente().DistanzaMediana;
      } else {
         KmDaCluster = Mv_Clu.Distanza1 - ClusStaz.RecordCorrente().DistanzaMediana;
      }
      if (abs(KmDaCluster - Fermata.ProgKm) > 255) {
         //Bprintf("Discordanza eccessiva a fermata %i del cluster %i Mezzo virtuale = %i",Fermata.Id, Mv_Clu.IdCluster, Fermata.MezzoVirtuale);
         HoDiscordanzaEccessiva = TRUE;
      } /* endif */
//<<< ORD_FORALL TabFv,f
   }
   if(FermateInvertite){
      Bprintf("Errore: Vi sono situazioni in cui non sono riuscito a linearizzare le distanze!");
      Bprintf("Numero MV con fermate invertite: %i per %i fermate", NumTreniConFermateInvertite , FermateInvertite );
      BEEP;
      return 99;
   };
   if (HoDiscordanzaEccessiva) {
      Bprintf("Errore: Vi sono situazioni in cui ho una discordanza chilometrica eccessiva");
      BEEP;
      return 99;
   } /* endif */
   return 0;
//<<< int WORK::IdentificaTreniNonLinearizzati
}


//----------------------------------------------------------------------------
// ARRAY_FERM::ARRAY_FERM
//----------------------------------------------------------------------------
ARRAY_FERM::ARRAY_FERM(ULONG i):
BUFR(i* sizeof(FERM)),
PercorsoCorrente(*(PERCORSO_LOGICO*)malloc(8000))
{
   #undef TRCRTN
   #define TRCRTN "@ARRAY_FERM"
};

//----------------------------------------------------------------------------
// Carica Collegamenti
//----------------------------------------------------------------------------
// Genera tutte le possibili combinazioni di fermate di un dato mezzo virtuale
// per trovare e caricare i collegamenti diretti.
void ARRAY_FERM::CaricaCollegamenti( HASH<COLLEGAMENTO1> & Coll1, HASH<COLLEGAMENTO2> & Coll2 ){
   #undef TRCRTN
   #define TRCRTN "ARRAY_FERM::CaricaCollegamenti"
   COLLEGAMENTO2 C2;
   COLLEGAMENTO1 C1;
   ORD_FORALL(THIS,u1){
      FERM & Ferm1 = THIS[u1];
      for (int u2 = THIS.Dim() - 1; u2 > u1 ;u2 -- ) {
         FERM & Ferm2 = THIS[u2];
         if(!(Ferm1.OraPartenza >= 0 && Ferm2.OraArrivo >=0 && Ferm1.Id != Ferm2.Id ))continue;
         C1.SetKey(Ferm1.Id, Ferm2.Id);
         C2.SetKey(C1,IdCluster,Concorde);
         WORD Km = Ferm2.ProgKm - Ferm1.ProgKm;
         if(Km >= 1<<12){
            Bprintf("Errore: Errore sequenza o Troppi Km = %i tra fermate ID = %i e %i \n",Km,Ferm1.Id, Ferm2.Id);
            BEEP;
         }
         if(THIS.TipoCluster == 11){ // Disabilito gli orari di coincidenza per i traghetti
            C1.Compute2(Ferm1.OraPartenza , Ferm2.OraArrivo, Km);
            C2.Compute2(Ferm1.OraPartenza , Ferm2.OraArrivo, Km);
         } else {
            C1.Compute(Ferm1.OraPartenza , Ferm2.OraArrivo, Km);
            C2.Compute(Ferm1.OraPartenza , Ferm2.OraArrivo, Km);
         }
         COLLEGAMENTO1 * H1 = Coll1.Cerca(&C1,4);
         if(H1){
            (*H1) += C1;
         } else {
            H1 = Coll1.Alloca();
            (*H1) = C1;
            Coll1.Metti(4);
         }
         COLLEGAMENTO2 * H2 = Coll2.Cerca(&C2,6);
         if(H2){
            (*H2) += C2;
         } else {
            H2 = Coll2.Alloca();
            (*H2) = C2;
            Coll2.Metti(6);
         }
//<<< for  int u2 = THIS.Dim   - 1; u2 > u1 ;u2 --
      } /* endfor */
//<<< ORD_FORALL THIS,u1
   }
//<<< void ARRAY_FERM::CaricaCollegamenti  HASH<COLLEGAMENTO1> & Coll1, HASH<COLLEGAMENTO2> & Coll2
};

//----------------------------------------------------------------------------
// Identifica cluster
//----------------------------------------------------------------------------
// Torna l' Id = il numero del record del cluster su file
// Se il cluster non esiste lo aggiunge, altrimenti aggiorna il file
//----------------------------------------------------------------------------
int  ARRAY_FERM::IdentificaCluster(HASH<CLUSTER_IDX> & ClustersIdx, F_CLUSTER_MV & Clust,WORD LastMV,MEZZO_VIRTUALE & MvLe){
   #undef TRCRTN
   #define TRCRTN "ARRAY_FERM::IdentificaCluster"

   // Init
   Nodo1 =  Distanza1 = Nodo2 =  Distanza2 = 0;
   int Peso = -1;
   OraP1 = 0;
   int Idx1 = 0;

   // Identifico: Dominanti, e tipo del cluster (da cui posso identificare il cluster)

   // Identifico il tipo cluster
   PercorrenzaTotale = abs(THIS[0].ProgKm - THIS[Dim() - 1].ProgKm); // Sono in ordine di fermata: posso quindi identificare facilmente la percorrenza totale
   TipoCluster = (PercorrenzaTotale >= 250) ? 2 : 1;
   if(Navale) TipoCluster = 11; // Cluster Traghetti
   // I mezzi virtuali derivati solo da carrozze dirette o da servizi diretti
   // vengono immagazzinati a parte, perche' di solito seguono strani percorsi
   if(MvLe.NumMezziComponenti > 1 && ! MvLe.DaOrigineDestino ) TipoCluster = 12;

   // Identifico il primo nodo dominante
   FORALL(THIS,i){
      FERM & Ferm = THIS[i];
      // TRACESTRING("Step1  Id "+STRINGA(THIS[i].Id)+ " Peso "+ STRINGA(THIS[i].Peso)+" Km "+STRINGA(THIS[i].ProgKm));

      BOOL Sostituire = Ferm.Peso > Peso ; // Privelegio i nodi cui fermano piu' treni
      if(Ferm.Peso == Peso ){  // A parita' di peso entrano in gioco altri criteri
         if (Idx1==0 || Idx1==Dim()-1) { // Se il nodo dominante attuale e' un capolinea
            Sostituire |= (i==0  || i==Dim()-1) &&  Ferm.Id < Nodo1; // Puo' essere sostituito da un capolinea con ID inferiore
         } else {
            Sostituire |= (i==0  || i==Dim()-1); // Puo' essere sostituito da un capolinea
            Sostituire |= Ferm.Id < Nodo1;       // O da un nodo con ID inferiore
         } /* endif */
      }

      if(Sostituire){
         Nodo1      = Ferm.Id    ;
         Distanza1  = Ferm.ProgKm;
         Peso       = Ferm.Peso  ;
         OraP1      = max(Ferm.OraPartenza,Ferm.OraArrivo);
         Idx1       = i;
      };
//<<< FORALL THIS,i
   };
   // TRACEVINT(Nodo1);

   // Il secondo nodo e' soggetto al vincolo di essere ad almeno 50 Km (o 25 % percorrenza totale)
   // di distanza dal dominante.
   int LimitePercorrenzaTotale = abs(THIS[0].ProgKm - THIS[Dim() - 1].ProgKm) / 4;
   if(LimitePercorrenzaTotale > 50 )LimitePercorrenzaTotale =  50 ;
   Top(LimitePercorrenzaTotale,3); // Mai meno di 3 Km
   // Identifico il secondo nodo dominante
   Peso = -1;
   FORALL(THIS,j){
      FERM & Ferm = THIS[j];
      if(Ferm.Id == Nodo1)continue;
      // TRACESTRING("Step2  Id "+STRINGA(Ferm.Id)+ " Peso "+ STRINGA(Ferm.Peso)+" Km "+STRINGA(Ferm.ProgKm));
      BOOL Sostituire = Ferm.Peso > Peso ; // Privilegio i nodi cui fermano piu' treni
      // Se possibile cerco sempre di utilizzare stazioni con percorrenza superiore al limite
      Sostituire |= ( abs( Distanza1 - Ferm.ProgKm ) >= LimitePercorrenzaTotale ) && ( abs( Distanza1 - Distanza2  ) <  LimitePercorrenzaTotale );
      if(Ferm.Peso == Peso ){  // A parita' di peso entrano in gioco altri criteri
         if (Idx1==0 || Idx1==Dim()-1) { // Se il nodo dominante attuale e' un capolinea
            Sostituire |= (i==0  || i==Dim()-1) &&  Ferm.Id < Nodo1; // Puo' essere sostituito da un capolinea con ID inferiore
         } else {
            Sostituire |= (i==0  || i==Dim()-1); // Puo' essere sostituito da un capolinea
            Sostituire |= Ferm.Id < Nodo2;       // O da un nodo con ID inferiore
         } /* endif */
      }

      if(Sostituire){
         Nodo2      = Ferm.Id;
         Distanza2  = Ferm.ProgKm;
         Peso       = Ferm.Peso  ;
      };
//<<< FORALL THIS,j
   };
   // TRACESTRING( VRS(Nodo1) + VRS(Nodo2) + VRS(LimitePercorrenzaTotale) + VRS(Distanza1) + VRS(Distanza2) );
   Concorde = Distanza2 >= Distanza1;
   assert(Distanza2 != Distanza1);

   // Set di PercorsoCorrente
   PercorsoCorrente.NumeroStazioni = Dim();
   PercorsoCorrente.Next           = NULL ;
   int LastKm;
   if (Concorde) {
      ORD_FORALL(THIS,j){
         FERM & Ferm = THIS[j];
         PercorsoCorrente.Stazioni[j].Id = Ferm.Id;
         assert(PercorsoCorrente.Stazioni[j].Id < 10000);
         if (j == 0) {
            PercorsoCorrente.Stazioni[j].dKm = 0;
         } else {
            PercorsoCorrente.Stazioni[j].dKm  =  abs(Ferm.ProgKm - LastKm);
         } /* endif */
         LastKm = Ferm.ProgKm;
         if(Ferm.Id == Nodo1) PercorsoCorrente.OffsetDominante = j;
      }
   } else {
      int i = 0;
      FORALL(THIS,j){
         FERM & Ferm = THIS[j];
         PercorsoCorrente.Stazioni[i].Id = Ferm.Id;
         assert(PercorsoCorrente.Stazioni[i].Id < 10000);
         if (i == 0) {
            PercorsoCorrente.Stazioni[i].dKm = 0;
         } else {
            PercorsoCorrente.Stazioni[i].dKm  =  abs(Ferm.ProgKm - LastKm);
         } /* endif */
         LastKm = Ferm.ProgKm;
         if(Ferm.Id == Nodo1) PercorsoCorrente.OffsetDominante = i;
         i++;
      }
//<<< if  Concorde
   } /* endif */

   // Cerco nella tabella dei cluster se e' gia' stato definito
   CLUSTER_IDX Target; Target.Id1 = Nodo1 ; Target.Id2 = Nodo2 ; Target.Tipo = TipoCluster ;
   int NextTipo = 3;
   int Found = -1;
   CLUSTER_IDX * ClusterIdx = NULL;
   do {
      ClusterIdx = ClustersIdx.Cerca(&Target,5);
      if(ClusterIdx){
         // Se ho raggiunto il limite di MV per cluster non posso utilizzare il
         // cluster gia' definito: allora vado in SPLIT
         ClusterIdx->UsageCount ++;
         if ( ClusterIdx->UsageCount > MAX_VIRTUALI_IN_CLUSTER - MARGINE_VIRTUALI_IN_CLUSTER) {
            // "Chiudo" il cluster e ne utilizzo un altro
            //Bprintf("Split di cluster %u : %u => %u per raggiunto numero di MV",ClusterIdx->NumeroRecord,Nodo1,Nodo2);
            // Faccio divenire il precedente cluster di tipo 15
            CLUSTER_IDX * ClusterIdx2 = ClustersIdx.Alloca();  // Alloco
            (*ClusterIdx2) = (*ClusterIdx);                    // Copio
            ClusterIdx2->Tipo = 15;                            // Cambio la chiave: 15 = Splittato
            Clust[ClusterIdx2->NumeroRecord].TipoCluster = ClusterIdx2->Tipo;
            Clust.ModifyCurrentRecord();
            /* EMS Win PROVE
            Clust.InvalidateBuffer();
            if (Clust[ClusterIdx2->NumeroRecord].Id1 == 0 ||
                Clust[ClusterIdx2->NumeroRecord].Id2 == 0) {
               TRACESTRING("Errore in modifica di Clust 867");
               TRACEINT("ClusterIdx2->NumeroRecord=",ClusterIdx2->NumeroRecord);
               TRACEINT("Clust[ClusterIdx2->NumeroRecord].TipoCluster=",
                                         Clust[ClusterIdx2->NumeroRecord].TipoCluster);
            }
            */
            ClusterIdx2->UsageCount --;                        // Perche' il MV non e' stato aggiunto
            ClustersIdx.Metti(5);                              // Metto nella Hash Table
            // Pulisco il cluster
            ClusterIdx->PercorsiLogici.Clear();
            ClusterIdx->NumeroRecord = Clust.NumRecordsTotali();
            ClusterIdx->UsageCount = 0;
            Found = -1;
            LinearizzazioneIncompatibile(*ClusterIdx); // Per aggiungere il Percorso Logico
         } else if (LinearizzazioneIncompatibile(*ClusterIdx)) { // Controllo linearizzazione: Se vi sono problemi di incompatibilita' debbo utilizzare un altro cluster
            // Continua il ciclo cambiando il tipo cluster
            //Bprintf3("Incompatibilita' su cluster %u : %u => %u ",ClusterIdx->NumeroRecord,Nodo1,Nodo2);
            Target.Tipo = NextTipo ++;
            if (Target.Tipo > 9) {
               Bprintf("Assoluta impossibilita' di linearizzare: ABORT");
               BEEP;
               exit(999);
            } /* endif */
            continue; //  Altro ciclo
//<<<    if   ClusterIdx->UsageCount > MAX_VIRTUALI_IN_CLUSTER - MARGINE_VIRTUALI_IN_CLUSTER
         } else {
            Found = ClusterIdx->NumeroRecord;
         } /* endif */
//<<< if ClusterIdx
      } else {
         ClusterIdx = ClustersIdx.Alloca(); // NB: Non chiama il costruttore ma e' inessenziale perche' --->
         (*ClusterIdx) = Target;            // --> Qui ricopro con un' area ben inizializzata
         ClusterIdx->NumeroRecord = Clust.NumRecordsTotali();
         ClusterIdx->UsageCount = 1;
         LinearizzazioneIncompatibile(*ClusterIdx); // Per aggiungere il Percorso Logico
         ClustersIdx.Metti(5);
         Found = -1;
      };
      break; // Di solito non deve ritentare
//<<< do
   } while(TRUE);
   TipoCluster = Target.Tipo;

   if(Found == -1){
      Found = Clust.NumRecordsTotali();

      CLUSTER_MV Clu;
      Clu.Id                  = Chk(Found,13)            ;
      Clu.TipoCluster         = Chk(TipoCluster,4) ;
      Clu.Id1                 = Chk(Nodo1 ,13)      ;
      Clu.Id2                 = Chk(Nodo2 ,13)      ;
      /* EMS Win PROVE
      if (Clu.Id1 == 0 || Clu.Id2 == 0) {
         TRACESTRING("Clu irregol in IdentificaCluster");
         TRACEINT("Clu.Id1 = ",Clu.Id1);
         TRACEINT("Clu.Id2 = ",Clu.Id2);
         TRACEINT("Nodo1=",Nodo1);
         TRACEINT("Nodo2=",Nodo2);
      }
      */
      Clu.NumeroMezziVirtuali = 1                        ;
      assert(Distanza2 != Distanza1);
      if(Distanza2 >= Distanza1){
         Clu.NumeroMezziVirtualiC= 1                        ;
         Clu.NumeroMezziVirtualiD= 0                        ;
      } else {
         Clu.NumeroMezziVirtualiC= 0                        ;
         Clu.NumeroMezziVirtualiD= 1                        ;
      }
      Clu.ExtraMezziVirtualiC = 0                        ;
      Clu.ExtraMezziVirtualiD = 0                        ;
      Clu.NumeroNodi          = 0                        ;
      Clu.NumeroNodiCambio    = 0                        ;
      Clu.NumeroStazioni      = 0                        ;
      Clu.NumElementiGruppi   = 0                        ;
      Clust.AddRecordToEnd(VRB(Clu));
      //Bprintf3("Generato cluster Nø %i : %i->%i da mezzo virtuale Nø %i",Found,Nodo1,Nodo2,LastMV);
      Clust.Posiziona(Found);
      Clust.InvalidateBuffer();      // EMS006 Win
      Clust.Posiziona(Found);
//<<< if Found == -1
   } else {
      Clust[Found].NumeroMezziVirtuali ++;
      // Se la assert fallisce non funziona lo split dei clusters
      assert(Clust.RecordCorrente().NumeroMezziVirtuali <= MAX_VIRTUALI_IN_CLUSTER - MARGINE_VIRTUALI_IN_CLUSTER);
      if(Distanza2 >= Distanza1){
         Clust.RecordCorrente().NumeroMezziVirtualiC ++ ;
      } else {
         Clust.RecordCorrente().NumeroMezziVirtualiD ++ ;
      }
      Clust.ModifyCurrentRecord();
      // EMS006 Win
      Clust.InvalidateBuffer();
      Clust.Posiziona(Found);
   }
   IdCluster = Found;

   //Bprintf3("Mezzo virtuale %i Appartiene a cluster %i Nodo1 = %i Nodo2 = %i TipoClust = %i Distanza1 = %i Distanza2 = %i LimPerc= %i", LastMV, IdCluster, Nodo1, Nodo2, TipoCluster, Distanza1, Distanza2, LimitePercorrenzaTotale);

   return Found;
//<<< int  ARRAY_FERM ::IdentificaCluster HASH<CLUSTER_IDX> & ClustersIdx, F_CLUSTER_MV & Clust,WORD LastMV
};
//----------------------------------------------------------------------------
// ARRAY_FERM::LinearizzazioneIncompatibile
//----------------------------------------------------------------------------
// Identifica i casi di linearizzazione incompatibile
BOOL ARRAY_FERM::LinearizzazioneIncompatibile(CLUSTER_IDX & Cluster){
   #undef TRCRTN
   #define TRCRTN "ARRAY_FERM::LinearizzazioneIncompatibile"

   // Controllo la compatibilita' del percorso corrente con tutti i percorsi logici del cluster
   BOOL Sostituisce = FALSE;
   ITERA2(Cluster.PercorsiLogici,PercorsoLogico,PERCORSO_LOGICO){
      // EMS007 Win avendo cambiato la ITERA2 sostituendo un puntatore
      // al reference, devo cambiare il corpo del ciclo di conseguenza
      int Relazione = PercorsoCorrente.Confronta(*PercorsoLogico);
      switch (Relazione) {
      case 0:    // 0  = I due percorsi logici sono eguali
      case 2:    // 2  = B Contiene il percorso logico
         return FALSE; // Ok e' equivalente ad un percorso logico che gia' avevo
      case 1:    // 1  = Il percorso logico contiene B
         Sostituisce = TRUE; // Memorizzo che ho almeno un percorso da sostituire
         break;
      case 3:    // 3  = I percorsi logici sono diversi ma compatibili
         break;
      case 4:    // 4  = I percorsi logici sono incompatibili
         return TRUE;
      default:
         Bprintf("??? Qui non dovrebbe mai arrivare");
         BEEP;
         exit(999);
         break;
      } /* endswitch */
   } END_ITERA
   if(Sostituisce) { // Ripeto effettuando realmente la sostituzione
      ITERA2(Cluster.PercorsiLogici,PercorsoLogico,PERCORSO_LOGICO){
         // EMS007 Win avendo cambiato la ITERA2 sostituendo un puntatore
         // al reference, devo cambiare il corpo del ciclo di conseguenza
         if (PercorsoCorrente.Confronta(*PercorsoLogico) == 1 ) {
            // Sostituisco il percorso precedente: L' area purtroppo non puo' essere deallocata
            // EMS007 Win avendo cambiato la ITERA2 sostituendo un puntatore
            // al reference, devo cambiare il corpo del ciclo di conseguenza
            Cluster.PercorsiLogici.Del(PercorsoLogico); // E' legale cancellare all' interno del ciclo, purche' non deallochi l' area
         }
      } END_ITERA
   };
   // Aggiungo il percorso corrente
   int SizeP = sizeof(PERCORSO_LOGICO) + ((PercorsoCorrente.Dim()-1) * sizeof(STZ));
   PERCORSO_LOGICO * NewP = (PERCORSO_LOGICO *)malloc(SizeP);
   memmove(NewP,&PercorsoCorrente,SizeP);
   assert(NewP->Dim() == Dim());
   Cluster.PercorsiLogici.Push(NewP);
   // TRACEPOINTER("Aggiunto percorso logico "+STRINGA(SizeP)+" da "+STRINGA(NewP->Dim())+" Stazioni @ ",NewP);
   //ORD_FORALL((*NewP),i1){
   //   TRACELONG("NewP,  Stazioni["+STRINGA(i1)+"].Id = ", NewP->Stazioni[i1].Id);
   //}
   return FALSE;
//<<< BOOL ARRAY_FERM::LinearizzazioneIncompatibile CLUSTER_IDX & Cluster
};

//----------------------------------------------------------------------------
// PERCORSO_LOGICO::Confronta
//----------------------------------------------------------------------------
int PERCORSO_LOGICO::Confronta( PERCORSO_LOGICO & B){
   #undef TRCRTN
   #define TRCRTN "PERCORSO_LOGICO::Confronta"

   if(B.NumeroStazioni > NumeroStazioni ){
      // TRACESTRING("Confronto invertito");
      int Ret = B.Confronta(THIS);
      if(Ret == 1) Ret = 2;
      return Ret;
   }

   // TRACEPOINTER2("This , B",this, &B);

   Collider.Reset();
   ORD_FORALL(THIS,i1){
      // TRACELONG("Stazioni["+STRINGA(i1)+"].Id = ", Stazioni[i1].Id);
      assert(Stazioni[i1].Id< 10000);
      Collider.Set(Stazioni[i1].Id);
   }

   int Idx = 0;
   BOOL VincoliAddizionali = FALSE;
   ORD_FORALL(B,i2){
      ID Id = B.Stazioni[i2].Id;
      // TRACEPOINTER2("B.Stazioni["+STRINGA(i2)+"].Id = ", (void*)B.Stazioni[i2].Id ,&B.Stazioni[i2].Id );
      if (Collider.Test(Id)) {
         for (int Km = -Stazioni[Idx].dKm; Idx < NumeroStazioni ; Idx ++ ) { // Non resettando Idx trovo tutte le stazioni solo se sono nello stesso ordine che ho su B
            Km += Stazioni[Idx].dKm;
            if(Stazioni[Idx].Id == Id){
               if (i2 > 0 && B.Stazioni[i2].dKm > Km) { // Ho un vincolo addizionale dovuto ai KM
                  VincoliAddizionali = TRUE; // Introduco un vincolo addizionale (come se fosse non contenuta)
               } /* endif */
               break;
            };
         } /* endfor */
         if(Idx >=  NumeroStazioni) return 4; // Percorsi incompatibili
      } else {
         VincoliAddizionali = TRUE;
      } /* endif */
   }
   // I percorsi sono compatibili
   if (!VincoliAddizionali) {  // B e' incluso
      if(B.NumeroStazioni == NumeroStazioni ) return 0;  // E' eguale
      return 1; // Inclusione propria
   } /* endif */
   return 3;  // Compatibile ma non contenuto
//<<< int PERCORSO_LOGICO::Confronta  PERCORSO_LOGICO & B
}
//----------------------------------------------------------------------------
// CLUSTER_IDX::Cerca
//----------------------------------------------------------------------------
// Attenzione: Fa' uso del fatto che LinearizzaDistanze sorta per ID
int CLUSTER_IDX::Cerca(ID Id){ // Cerca una stazione in StazLin e torna l' indice (o -1 se non trovata)
   #undef TRCRTN
   #define TRCRTN "CLUSTER_IDX::Cerca"
   if(NumStazioni == 0)return -1;
   int Min = 0;
   int Max = NumStazioni-1;
   int i;
   do {
      i = (Min + Max ) /2;
      ID  Val = StazLin[i].Id;
      if(Val < Id){
         Min = i;
      } else if(Val == Id){
         // Non puo' essere duplicato // while(i > 0 && StazLin[i-1].Id == Id) i--;
         return i;
      } else {
         Max = i-1;
      };
      if(Min == Max - 1) { // Posizione di stallo
         if(StazLin[Min].Id == Id){
            return Min;
         }
         if(StazLin[Max].Id == Id){
            return Max;
         }
         return -1;
      };
   } while (Min < Max);
   if(StazLin[Min].Id == Id){
      return Min;
   }
   return -1;
//<<< int CLUSTER_IDX::Cerca ID Id   // Cerca una stazione in StazLin e torna l' indice  o -1 se non trovata
}

//----------------------------------------------------------------------------
// CLUSTER_IDX::LinearizzaDistanze
//----------------------------------------------------------------------------
void CLUSTER_IDX::LinearizzaDistanze(){ // Linearizza le distanze e carica Stazioni
   #undef TRCRTN
   #define TRCRTN "CLUSTER_IDX::LinearizzaDistanze"

   //TRACESTRING("===================================================");
   //TRACESTRING("Linearizzazione cluster Nø "+STRINGA(NumeroRecord)+" "+STRINGA(Id1)+"-->"+STRINGA(Id2));
   //TRACESTRING("===================================================");

   NumStazioni= 0;
   static ARRAY_ID IdxStazioni;
   static short int * KmStazioni;
   static short int * MinKmStazioni;
   static short int * MaxKmStazioni;
   IdxStazioni.Clear();
   if(KmStazioni == NULL){
      KmStazioni = (short int * ) malloc(( Stazioni.Dim()+1) * sizeof(short int));
      MinKmStazioni = (short int * ) malloc(( Stazioni.Dim()+1) * sizeof(short int));
      MaxKmStazioni = (short int * ) malloc(( Stazioni.Dim()+1) * sizeof(short int));
   }

   // Identifico le stazioni
   Collider.Reset();
   ITERA2(PercorsiLogici,PercorsoLogico,PERCORSO_LOGICO){
      FORALL(*PercorsoLogico,i){
         // EMS007 Win avendo cambiato la ITERA2 sostituendo un puntatore
         // al reference, devo cambiare il corpo del ciclo di conseguenza
         ID Id = PercorsoLogico->Stazioni[i].Id;
         if(!Collider.TestAndSet(Id)){
            IdxStazioni += Id;
            KmStazioni[Id] = 0;
            MaxKmStazioni[Id] = -30000;
            MinKmStazioni[Id] = 30000;
         }
      }
   } END_ITERA
   StazLin = (STZ2*) malloc(IdxStazioni.Dim() * sizeof(STZ2));

   // Sort per ID: Necessario per il successivo uso della funzione Cerca()
   qsort((void *)&IdxStazioni[0], IdxStazioni.Dim(), sizeof(ID), CompareWord);

   // Linearizzo i Km delle stazioni che seguono la stazione dominante
   BOOL Modificato;
   do {
      Modificato = FALSE;
      int Pl = 0;
      ITERA2(PercorsiLogici,PercorsoLogico,PERCORSO_LOGICO){
         Pl ++;
         int Km = 0;
         int KmVeri = 0;
         // EMS007 Win avendo cambiato la ITERA2 sostituendo un puntatore
         // al reference, devo cambiare il corpo del ciclo di conseguenza
         for (int i = PercorsoLogico->OffsetDominante + 1; i < PercorsoLogico->NumeroStazioni ; i++ ) {
            Km     += PercorsoLogico->Stazioni[i].dKm;
            KmVeri += PercorsoLogico->Stazioni[i].dKm;
            ID Id = PercorsoLogico->Stazioni[i].Id;
            if (Km > KmStazioni[Id]) {
               if(KmStazioni[Id] != 0 ){
                  //TRACESTRING(VRS(Pl)+" Streck Stazione ID = "+STRINGA(Id)+" Di " + STRINGA(Km - KmStazioni[Id]) + " Km Da: " +STRINGA(KmStazioni[Id]) +" a "+ STRINGA(Km) );
               }
               KmStazioni[Id] = Km;
               Modificato = TRUE;
            } else {
               Km = KmStazioni[Id];
            } /* endif */
            Top(MaxKmStazioni[Id],KmVeri);
            Bottom(MinKmStazioni[Id],KmVeri);
         } /* endfor */
         if(Km > 3000){ Bprintf("Errore: LOOP"); BEEP; exit(999); }
      } END_ITERA
   } while (Modificato); /* enddo */
   // Linearizzo i Km delle stazioni che precedono la stazione dominante

   do {
      Modificato = FALSE;
      int Pl = 0;
      ITERA2(PercorsiLogici,PercorsoLogico,PERCORSO_LOGICO){
         int Km = 0;
         int KmVeri = 0;
         Pl ++;
         // EMS007 Win avendo cambiato la ITERA2 sostituendo un puntatore
         // al reference, devo cambiare il corpo del ciclo di conseguenza
         for (int i = PercorsoLogico->OffsetDominante ; i > 0 ; i-- ) {
            Km     -= PercorsoLogico->Stazioni[i].dKm;
            KmVeri -= PercorsoLogico->Stazioni[i].dKm;
            ID Id = PercorsoLogico->Stazioni[i-1].Id;
            if (Km < KmStazioni[Id]) {
               if(KmStazioni[Id] != 0 ){
                  //TRACESTRING(VRS(Pl)+" Streck Stazione ID = "+STRINGA(Id)+" Di -" + STRINGA(KmStazioni[Id]-Km) + " Km Da: " +STRINGA(KmStazioni[Id]) +" a "+ STRINGA(Km) );
               }
               KmStazioni[Id] = Km;
               Modificato = TRUE;
            } else {
               Km = KmStazioni[Id];
            } /* endif */
            Top(MaxKmStazioni[Id],KmVeri);
            Bottom(MinKmStazioni[Id],KmVeri);
         } /* endfor */
         if(Km < -3000){ Bprintf("Errore: LOOP"); BEEP; exit(999); }
      } END_ITERA
   } while (Modificato); /* enddo */

   // Scarico i Km Linearizzati e mediani
   NumStazioni= IdxStazioni.Dim();
   //TRACESTRING("Km Linearizzati: ");
   ORD_FORALL(IdxStazioni,i){
      StazLin[i].Id     = IdxStazioni[i];
      StazLin[i].Km     = KmStazioni[IdxStazioni[i]];
      StazLin[i].KmMedi = (MinKmStazioni[IdxStazioni[i]] + MaxKmStazioni[IdxStazioni[i]]) / 2;
      // Scrivo sul trace i Km Linearizzati
      //TRACESTRING(STRINGA(NumeroRecord)+STRINGA(": [")+STRINGA(i)+"] Id="+STRINGA(StazLin[i].Id)+" Km = "+STRINGA(StazLin[i].Km)+ " Medi "+ STRINGA(StazLin[i].KmMedi));
   }
//<<< void CLUSTER_IDX::LinearizzaDistanze    // Linearizza le distanze e carica Stazioni
}
