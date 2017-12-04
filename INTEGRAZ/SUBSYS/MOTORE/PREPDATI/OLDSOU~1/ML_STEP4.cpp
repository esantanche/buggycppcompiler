//----------------------------------------------------------------------------
// ML_STEP4.CPP: Completamento dati.
//----------------------------------------------------------------------------
// - Impostazione classe coincidenza per tutte le stazioni
// - Conteggio Nodi e stazioni di fermata per i cluster
// - Impostazione dei GRUPPI
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4


#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "ML_IN.HPP"
#include "ML_WRK.HPP"
#include "seq_proc.hpp"
#include "mm_basic.hpp"
#include "scandir.h"
#include "alfa0.hpp"

#define PGM      "ML_STEP4"

// Classi per l' individuazione dei gruppi
struct FERMATE_TRENO {
   WORD  IdTreno;
   WORD  IdCluster;
   BYTE  NumFermate;
   BYTE  ElemGruppo;  // Elemento del gruppo cui corrisponde il SET (255 = Non Noto)
   SET   Fermate;     // Identifica in quali tra le stazioni DI FERMATA del cluster fermi il treno
   WORD  CardFerm;    // Numero di fermate
   FERMATE_TRENO(){IdTreno = IdCluster = 0;NumFermate=0;ElemGruppo=255;CardFerm = 0;};
   void ReSize(BYTE Nf){NumFermate = Nf;Fermate.Realloc(NumFermate);};
};
FERMATE_TRENO * Ft;     // Dati reali
WORD         * IdxFt;  // Indice ausiliario: Ordinati per cluster
int NumValidi;
// Ordina IdxFt per Cluster e -(Nø fermate del treno)
// Si noti che ordinando per numeri di elementi si ordina anche per ordine di
// inclusione insiemistica (il che permette poi di passare ai gruppi).
int sort_fermate( const void *a, const void *b){
   FERMATE_TRENO & A = Ft[*(WORD*)a];
   FERMATE_TRENO & B = Ft[*(WORD*)b];
   int Out = (int)A.IdCluster - (int)B.IdCluster;
   if(Out == 0){
      Out = (int)B.CardFerm - (int)A.CardFerm;
   }
   return Out;
};
void SortFermate(){
   qsort(IdxFt,NumValidi,sizeof(WORD),sort_fermate);
};

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
   SetStatoProcesso(PGM, SP_PARTITO);
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
   SetPriorita(); // Imposta la priorita'
   
   GESTIONE_ECCEZIONI_ON
   
   DosError(2); // Disabilita Popup di errore
   
   TryTime(0);
   
   
   // ----------------------------------------------------------------------
   // Init
   // ----------------------------------------------------------------------
   // Imposto le stazioni
   Bprintf("Inizializzazione :");
   Bprintf("     Stazioni ");
   F_STAZIONE_MV  Fstaz(PATH_OUT "MY_STAZI.TMP");
   // APRO il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);
   
   if (CCR_ID::CcrId == NULL) {
      CCR_ID::CcrId = new CCR_ID(PATH_DATI);
   } /* endif */
   
   // Leggo i dati delle multistazioni
   GEST_MULTI_STA Gestore(PATH_CVB,NULL,Fstaz);
   
   Fstaz.PermettiScrittura();
   ORD_FORALL(Fstaz,Idx1){
      Fstaz[Idx1].ClasseCoincidenza = 0;
      Fstaz.ModifyCurrentRecord();
   };
   Bprintf("     Clusters ");
   F_CLUSTER_MV   Clust(PATH_OUT "MY_CLUST.TMP");
   Clust.PermettiScrittura();
   ORD_FORALL(Clust,Idx2){
      Clust[Idx2].NumeroNodi         = 0;
      Clust[Idx2].NumeroNodiCambio   = 0;
      Clust[Idx2].NumeroStazioni     = 0;
      Clust[Idx2].NumElementiGruppi  = 0;
      Clust.ModifyCurrentRecord();
   }
   Bprintf("     Clusters<->Stazioni ");
   F_CLUSTER_STAZIONE ClusStaz(PATH_OUT "CLUSSTAZ.TMP");
   ClusStaz.PermettiScrittura();
   ORD_FORALL(ClusStaz,Idx3){
      ClusStaz.FixRec(Idx3).Gruppo.Clear();
      ClusStaz.ModifyCurrentRecord();
   }
   Bprintf("     Clusters<->Mezzi virtuali ");
   F_MEZZOV_CLUSTER ClusMezv(PATH_OUT "CLUSMEZV.TMP");
   ClusMezv.PermettiScrittura();
   ORD_FORALL(ClusMezv,Idx4){
      ClusMezv.FixRec(Idx4).Gruppo.Clear();
      ClusMezv.ModifyCurrentRecord();
   }
   
   // ----------------------------------------------------------------------
   // Impostazione classe coincidenza per tutte le stazioni
   // ----------------------------------------------------------------------
   TryTime(0);
   puts("Impostazione classe coincidenza stazioni eccezionali");
   ORD_FORALL(Gestore.StazioniCoincidenzaEstesa,i){
      Fstaz[Gestore.StazioniCoincidenzaEstesa[i]].ClasseCoincidenza = 1;
      Fstaz.ModifyCurrentRecord();
   } /* endfor */
   
   // ----------------------------------------------------------------------
   // Conteggio Nodi e stazioni di fermata per i cluster
   // ----------------------------------------------------------------------
   TryTime(0);
   puts("Conteggio Nodi e stazioni di fermata per i cluster");
   int MaxFermate=0,MaxNodi=0,MaxCambio = 0;
   int ClusterMaxSt = 0;
   ORD_FORALL(ClusStaz,j){
      CLUSTER_STAZIONE & Cl_St =  ClusStaz.FixRec(j);
      CLUSTER_MV       & Clu   =   Clust[Cl_St.IdCluster];
      if(!Fstaz[Cl_St.IdStazione].IsInstradGrafo() && Cl_St.NumMzvFerm == 0)continue;
      Clu.NumeroStazioni ++;
      if(Clu.NumeroStazioni > MaxFermate){
          MaxFermate=Clu.NumeroStazioni;
          ClusterMaxSt = Cl_St.IdCluster ;
      };
      
      if(Fstaz[Cl_St.IdStazione].IsInstradGrafo()  || Fstaz.RecordCorrente().StazioneDiCambio() ){
         Clu.NumeroNodi++;
         Top(MaxNodi,Clu.NumeroNodi);
         // La stazione e' di cambio solo se Tipostazione & 1
         // Inoltre la stazione deve avere dei treni che fermano sul cluster
         if(Fstaz.RecordCorrente().StazioneDiCambio() && Cl_St.NumMzvFerm> 0){
            Clu.NumeroNodiCambio++;
            Top(MaxCambio,Clu.NumeroNodiCambio);
         }
      }
      Clust.ModifyCurrentRecord();
   }
   printf("MaxFermate = %i , MaxNodi = %i, MaxCambio = %i\n",MaxFermate,MaxNodi,MaxCambio);
   if(MaxFermate > MAX_STAZIONI_IN_CLUSTER){
      Bprintf("Errore: Ecceduto limite strutturale di 256 fermate al cluster Id=%i .",ClusterMaxSt);
      exit (999);
   }
   
   // ----------------------------------------------------------------------
   // Scandisco i treni e determino i gruppi
   // ----------------------------------------------------------------------
   puts("Scandisco i treni e determino i gruppi");
   TryTime(0);
   // Apro treni virtuali e relative fermate
   F_MEZZO_VIRTUALE TabTv(PATH_OUT "M0_TRENV.TMP");
   F_FERMATE_VIRT TabFv(PATH_OUT "M0_FERMV.TMP");
   int DimTv = TabFv.FixRec(TabFv.Dim() - 1).MezzoVirtuale + 1;
   Ft = new FERMATE_TRENO[DimTv];
   IdxFt = new WORD[DimTv];
   memset(IdxFt,0,DimTv * sizeof(WORD));
   NumValidi = 0;
   int LastMV      = 0;
   MEZZOV_CLUSTER Mv_Clu;
   // Questa scansione ha il compito di individuare le fermate, nell' ambito del cluster, in cui
   // ferma il treno, e di metterle in una forma adatta a formare i gruppi
   ORD_FORALL(TabFv,f){
      TryTime(f);
      FERMATE_VIRT & Fermata = TabFv.FixRec(f);
      if(Fermata.Transito)continue; // Non ferma
      
      // Trovo il treno virtuale ed il relativo cluster
      if (Fermata.MezzoVirtuale != LastMV) {
         if(LastMV){
            TRACESTRING2("Treno Virt Nø "+STRINGA(LastMV)+" Fermate ("+STRINGA(Ft[LastMV].Fermate.Cardinalita())+") :",Ft[LastMV].Fermate.ToHex());
            Ft[LastMV].CardFerm = Ft[LastMV].Fermate.Cardinalita();
         }
         LastMV = Fermata.MezzoVirtuale ;
         TabTv.Seek(LastMV);
         if(TabTv.RecordCorrente().NumeroFermateValide < 2) continue;
         ClusMezv.Seek(Fermata.MezzoVirtuale);
         Mv_Clu  =  ClusMezv.RecordCorrente();
         // Imposto il record di definizione treno
         Ft[LastMV].ReSize(Clust[Mv_Clu.IdCluster].NumeroStazioni);
         Ft[LastMV].IdTreno   = LastMV;
         Ft[LastMV].IdCluster = Mv_Clu.IdCluster;
         IdxFt[NumValidi ++]  = LastMV;
      }
      ClusStaz.Seek(Mv_Clu.IdCluster,Fermata.Id);
      Ft[LastMV].Fermate.Set(ClusStaz.RecordCorrente().Prog); // Insieme delle fermate del treno
//<<< ORD_FORALL TabFv,f  
   }
   TRACESTRING2("Treno Virt Nø "+STRINGA(LastMV)+" Fermate ("+STRINGA(Ft[LastMV].Fermate.Cardinalita())+") :",Ft[LastMV].Fermate.ToHex());
   Ft[LastMV].CardFerm = Ft[LastMV].Fermate.Cardinalita();
   puts("Rianalisi dei dati e preparazione dei gruppi");
   TryTime(0);
   SortFermate();
   WORD LastCluster = 0;
   WORD LastIdx     = 0;
   WORD NumGruppo   = 0;
   WORD MaxElGrup   = 0; // Massimo numero di elementi di un gruppo
   for ( i = 0;i < NumValidi ; i++ ) { // Ogni ciclo gestisco un treno
      TryTime(i);
      FERMATE_TRENO & FermateInEsame = Ft[IdxFt[i]];
      TRACESTRING("Fermate: Treno = "+STRINGA(FermateInEsame.IdTreno)+",Cluster = "+STRINGA(FermateInEsame.IdCluster)+ " Fermate: "+FermateInEsame.Fermate.ToHex());
      if(LastCluster != FermateInEsame.IdCluster){ // Primo treno di un altro cluster
         LastCluster =  FermateInEsame.IdCluster;
         LastIdx   = i;
         NumGruppo = 0;
         FermateInEsame.ElemGruppo = NumGruppo ++;       // Indica che e' un elemento del gruppo
         Clust[FermateInEsame.IdCluster].NumElementiGruppi  = NumGruppo;
         Clust.ModifyCurrentRecord();
         Top(MaxElGrup,NumGruppo);
      } else {
         // Controllo tutti i gruppi precedentemente definiti per vedere se il treno
         // appartiene ad un gruppo gia' definito
         BYTE NextGruppo = 0;
         for (int j = LastIdx; j < i ; j++ ) {
            FERMATE_TRENO & FermateDaConfrontare = Ft[IdxFt[j]];
            // Questo confronto evita test inutili: il primo insieme con un dato ElemGruppo
            // include tutti gli altri con lo stesso valore (per definizione di ElemGruppo)
            if(FermateDaConfrontare.ElemGruppo == NextGruppo){
               NextGruppo ++;
               if(FermateDaConfrontare.Fermate >= FermateInEsame.Fermate){ // Ok: Incluso
                  FermateInEsame.ElemGruppo = FermateDaConfrontare.ElemGruppo;
                  TRACESTRING("Ok fermate appartengono ad elemento Nø "+STRINGA(NextGruppo -1)+" del gruppo");
                  break; // Fine: identificato elemento del gruppo di appartenenza
               } else {
                  TRACESTRING("Fermate NON appartengono ad elemento Nø "+STRINGA(NextGruppo -1)+" del gruppo");
               } /* endif */
            } /* endif */
         } /* endfor */
         if (j == i) { // Non e' incluso in nessuno dei set precedenti !
            FermateInEsame.ElemGruppo = NumGruppo ++;
            Clust[FermateInEsame.IdCluster].NumElementiGruppi  = NumGruppo;
            Clust.ModifyCurrentRecord();
            Top(MaxElGrup,NumGruppo);
         } /* endif */
//<<< if LastCluster != FermateInEsame.IdCluster   // Primo treno di un altro cluster
      };
      TRACEVLONG(FermateInEsame.ElemGruppo);
//<<< for   i = 0;i < NumValidi ; i++     // Ogni ciclo gestisco un treno
   } /* endfor */
   TryTime(0);
   printf("Numero massimo di elementi di un gruppo : %i\n",MaxElGrup);
   if (MaxElGrup > MAXGRUPPO) {
      puts(
         "Il numero massimo di elementi di un gruppo eccede la dimensione\n"
         "attualmente gestibile: e' necessario modificare la definizione della classe\n"
         "GRUPPO aumentandone la dimensione o splittare i cluster.\n"
      );
      BEEP;
      exit(999);
   } /* endif */
   puts("Controaggiorno i dati dei gruppi per i treni");
   for ( i = 0;i < NumValidi ; i++ ) {
      TryTime(i);
      FERMATE_TRENO & FermateInEsame = Ft[IdxFt[i]];
      ClusMezv.Seek(FermateInEsame.IdTreno);
      ClusMezv.RecordCorrente().Gruppo.Set(FermateInEsame.ElemGruppo);
      ClusMezv.ModifyCurrentRecord();
   }
   TryTime(0);
   puts("Controaggiorno i dati dei gruppi per le stazioni");
   LastCluster = 0;
   BYTE  NextGruppo = 0;
   DWORD P_ClusStaz = 0;
   int Limite_ClusStaz = ClusStaz.Dim();
   for ( i = 0;i < NumValidi ; i++ ) {
      TryTime(i);
      FERMATE_TRENO & FermateInEsame = Ft[IdxFt[i]];
      if(LastCluster != FermateInEsame.IdCluster){ // Prima fermata di un altro cluster
         LastCluster =  FermateInEsame.IdCluster;
         NextGruppo = 0;
         ClusStaz.Seek(FermateInEsame.IdCluster);
         P_ClusStaz = ClusStaz.NumRecordCorrente();
      };
      if(FermateInEsame.ElemGruppo == NextGruppo){ // Per evitare di considerare piu' volte le stazioni dello stesso gruppo
         NextGruppo ++;
         // Imposto il gruppo in tutte le stazioni del cluster incluse nel set
         for (int m = P_ClusStaz; m < Limite_ClusStaz ; m++ ) {
            CLUSTER_STAZIONE & Cs = ClusStaz.FixRec(m);
            if (Cs.IdCluster != LastCluster) break;
            if (FermateInEsame.Fermate.Test(Cs.Prog))Cs.Gruppo.Set(FermateInEsame.ElemGruppo);
            ClusStaz.ModifyCurrentRecord();
         } /* endfor */
      } /* endif */
   }
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   return 0;
//<<< int  main int argc,char *argv    
}

