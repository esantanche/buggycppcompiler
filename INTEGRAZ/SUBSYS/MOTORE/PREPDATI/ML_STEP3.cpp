//----------------------------------------------------------------------------
// ML_STEP3.CPP:
//  - Identificazione dei nodi di cambio
//  - Identificazione progressivo stazioni nell' ambito del cluster
//  - Imposta Progressivo2 nel file delle fermate
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

#define PGM      "ML_STEP3"

int Ord_Cluster_Distanza(const void * a,const void * b){
   CLUSTER_STAZIONE & A = *( CLUSTER_STAZIONE *) a;
   CLUSTER_STAZIONE & B = *( CLUSTER_STAZIONE *) b;
   if (A.IdCluster != B.IdCluster) return (int)A.IdCluster  - (int)B.IdCluster ;
   // Le stazioni non nodali ne di cambio vengono COMUNQUE poste alla fine
   if (A.TipoStazione && !B.TipoStazione) return -1;
   if (B.TipoStazione && !A.TipoStazione) return 1;

   return  (int)A.Distanza - (int)B.Distanza ;
};
int Ord_Cluster_SoloDistanza(const void * a,const void * b){
   CLUSTER_STAZIONE & A = *( CLUSTER_STAZIONE *) a;
   CLUSTER_STAZIONE & B = *( CLUSTER_STAZIONE *) b;
   if (A.IdCluster != B.IdCluster) return (int)A.IdCluster  - (int)B.IdCluster ;
   return  (int)A.Distanza - (int)B.Distanza ;
};

// Utile macro
#define DICAMBIO(_a,_b) if(! (Fstaz[_a].TipoStazione & 1)){              \
   Fstaz[_a].TipoStazione |= 1;                                       \
   Fstaz.ModifyCurrentRecord();      \
}
   // TRACESTRING("Stazione ID="+STRINGA(_a)+" Di CAMBIO perche' " _b);  \

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
   /* EMS002 Win
   SetStatoProcesso(PGM, SP_PARTITO);
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
   SetPriorita(); // Imposta la priorita'
   */

   GESTIONE_ECCEZIONI_ON

   DosError(2); // Disabilita Popup di errore

   TryTime(0);

   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);

   // Faccio una analisi delle multistazioni
   PROFILER::Clear(FALSE); // Per aprire il grafo

   // APRO il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);
   if (CCR_ID::CcrId == NULL) {
      CCR_ID::CcrId = new CCR_ID(PATH_DATI);
   } /* endif */


   STRINGA Linea;
   ARRAY_ID StazioniEccezionaliDiInstradamento;
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


   // FILE * Out;
   // Out = fopen(PATH_OUT PGM ".OUT","wt");

   // Apro l' archivio Stazioni in output
   F_STAZIONE_MV  Fstaz(PATH_OUT "MY_STAZI.TMP");

   Fstaz.PermettiScrittura();

   // Leggo i dati delle multistazioni
   GEST_MULTI_STA Gestore(PATH_CVB,NULL,Fstaz);


   // ----------------------------------------------------------------------
   // Init
   // ----------------------------------------------------------------------
   // Imposto le stazioni non nodali
   for(int Idx = MAXNODI+1; Idx < Fstaz.NumRecordsTotali(); Idx ++){
      Fstaz[Idx].TipoStazione    = 0;
      Fstaz[Idx].NumCollCambio   = 0;
      Fstaz.ModifyCurrentRecord();
   };
   // Imposto le stazioni nodali
   for(Idx = 1; Idx <= MAXNODI ; Idx ++){
      Fstaz[Idx].TipoStazione   |= 2;
      Fstaz[Idx].NumCollCambio   = 0;
      Fstaz.ModifyCurrentRecord();
   };
   // Imposto le stazioni Eccezionali di instradamento
   ORD_FORALL(StazioniEccezionaliDiInstradamento,Idx1){
      Fstaz[StazioniEccezionaliDiInstradamento[Idx1]].TipoStazione |= 4;
      Fstaz.ModifyCurrentRecord();
      // Bprintf("impostato tipostazione a 4 per %i",StazioniEccezionaliDiInstradamento[Idx1]);
   };

   // ----------------------------------------------------------------------
   // Le multistazioni sono stazioni di cambio
   // ----------------------------------------------------------------------
   ORD_FORALL(Gestore.StazioniMS,Ms){
      ID Id = Gestore.StazioniMS[Ms];
      DICAMBIO(Id, "MULTISTAZIONE");
   }

   // ----------------------------------------------------------------------
   // La prima e l' ultima fermata di ogni treno sono stazioni di cambio
   // Tutti i nodi in cui fermi almeno un treno sono stazioni di cambio
   // ----------------------------------------------------------------------
   //Bprintf("Scansione Treni : Promuove prima ed ultima fermata a nodo di cambio");
   // Apro treni virtuali e relative fermate
   F_MEZZO_VIRTUALE TabTv(PATH_OUT "M0_TRENV.TMP");
   F_FERMATE_VIRT TabFv(PATH_OUT "M0_FERMV.TMP");
   int LastMV      = 0;
   int LastFermata = 0;
   int Nr = TabFv.NumRecordsTotali();
   for (int f = 0; f < Nr ; f++ ) {
      TryTime(f);
      FERMATE_VIRT & Fermata = TabFv.FixRec(f);
      if(Fermata.Transito)continue; // Non ferma

      // Trovo il treno virtuale ed il relativo cluster
      if (Fermata.MezzoVirtuale != LastMV) {
         LastMV = Fermata.MezzoVirtuale ;
         TabTv.Seek(LastMV);

         if(TabTv.RecordCorrente().NumeroFermateValide < 2 )continue;

         DICAMBIO(Fermata.Id, "PRIMA_FERMATA");
         DICAMBIO(LastFermata, "ULTIMA_FERMATA");
      }
      LastFermata = Fermata.Id;

      // Tutti i nodi in cui fermi almeno un treno sono stazioni di cambio
      if(IsNodo(Fermata.Id)){
         DICAMBIO(Fermata.Id,"NODO E FERMA");
      }

//<<< for  int f = 0; f < Nr ; f++
   }
   TabTv.Seek(LastMV);
   if(TabTv.RecordCorrente().NumeroFermateValide >= 2){
      DICAMBIO(LastFermata, "ULTIMA_FERMATA");
   }

   // ----------------------------------------------------------------------
   // Le fermate dominanti dei cluster sono stazioni di cambio
   // ----------------------------------------------------------------------
   F_CLUSTER_MV   Clust(PATH_OUT "MY_CLUST.TMP");
   //Bprintf("Scansione Cluster : Promuove fermate dominanti a nodo di cambio");
   int Nr2 = Clust.NumRecordsTotali();
   for (int k = 0; k < Nr2 ; k++ ) {
      TryTime(k);
      CLUSTER_MV & Clus = Clust[k];
      DICAMBIO(Clus.Id1, "DOMINANTE 1");
      DICAMBIO(Clus.Id2, "DOMINANTE 2");
   }

   // ----------------------------------------------------------------------
   // Statistiche
   // ----------------------------------------------------------------------
   TryTime(0);
   Bprintf("Raccolta Statistiche");
   int Count[4];
   ZeroFill(Count);
   int Nr3 = Fstaz.NumRecordsTotali();
   for (int l = 0; l < Nr3 ; l++ ) {
      if(Fstaz[l].NumClusters > 0) Count[Fstaz[l].TipoStazione % 4] ++;
   }
   Bprintf("Vi sono %i Nodi non di cambio ,%i Nodi di cambio,  %i stazioni di cambio non nodali, %i fermate", Count[2],Count[3],Count[1],Count[0]);

   F_COLLEGAMENTO1 CollegamentiTraStazioni(PATH_OUT "COLLSTAZ.TMP");
   Bprintf("Scrittura istogrammi collegamenti stazioni di CAMBIO");
   int NumCollC =0;
   ORD_FORALL(CollegamentiTraStazioni,NumCst){
      COLLEGAMENTO1 & Cst = CollegamentiTraStazioni.FixRec(NumCst);
      BOOL Ok1 = FALSE;
      if(Fstaz[Cst.StazionePartenza].StazioneDiCambio()){
         Fstaz[Cst.StazioneArrivo].NumCollCambio ++;
         Fstaz.ModifyCurrentRecord();
         Ok1 = TRUE;
      }
      if(Fstaz[Cst.StazioneArrivo].StazioneDiCambio()){
         Fstaz[Cst.StazionePartenza].NumCollCambio ++;
         Fstaz.ModifyCurrentRecord();
         if(Ok1){
            NumCollC ++;
         }
      }
   }
   Bprintf("Vi sono %i Collegamenti UNIDIREZIONALI tra stazioni di cambio",NumCollC);
   int NumCollC2 = 0;
   F_COLLEGAMENTO2 CollegamentiTraStazioniNelCluster(PATH_OUT "COLLCLUS.TMP");
   ORD_FORALL(CollegamentiTraStazioniNelCluster,NumCst2){
      COLLEGAMENTO2 & Cst = CollegamentiTraStazioniNelCluster.FixRec(NumCst2);
      if(Fstaz[Cst.StazionePartenza].StazioneDiCambio() &&
            Fstaz[Cst.StazioneArrivo].StazioneDiCambio()){
         NumCollC2 ++;
      }
   }
   Bprintf("Vi sono %i Collegamenti UNIDIREZIONALI tra stazioni di cambio / cluster",NumCollC2);

   ORD_FORALL(Fstaz,z)Fstaz[z].NumCollCambio /= 2;

   // Istogramma fino a 2048 collegamenti
   WORD Counts[2048]; int i;
   ZeroFill(Counts);
   for(i= Fstaz.NumRecordsTotali()-1; i > 0 ; i--){
      if(!Fstaz[i].StazioneDiCambio())continue;
      Counts[Fstaz[i].NumCollCambio] ++;
      if(Fstaz[i].NumCollCambio <= 1 ){
         //Bprintf2("La stazione di cambio %i %s NON e' collegata ad almeno altre 2 stazioni di cambio ",i, Stazioni[i].NomeStazione);
         // La situazione e' regolare: permette i cambi alle stazioni intermedie
      }
   } /* endfor */
   Bprintf2("============== DUMP DEI COLLEGAMENTI BIDIREZIONALI TRA SOLE STAZIONI DI CAMBIO ==============");
   //for(i=0;i < 2048; i++)if(Counts[i])Bprintf2("Vi sono %i Stazioni con %i Collegamenti bidirezionali ",Counts[i],i);
   Bprintf2("============== DUMP DEI COLLEGAMENTI BIDIREZIONALI TRA SOLE STAZIONI DI CAMBIO : FINE ==============");
   int Total=0; for(i=0;i < 2048; i++)Total += Counts[i]; TRACEVLONG(Total);

   Fstaz.Flush();

   // ----------------------------------------------------------------------
   // Aggiorno F_CLUSTER_STAZIONE
   // ----------------------------------------------------------------------
   // NOTA: L' apertura del file e' fatta come un file FIX e non come un file BS
   TRACESTRING("Caricamento progressivi stazione nell' ambito del cluster");
   //puts("Caricamento progressivi stazione nell' ambito del cluster");
   TryTime(0);
   F_CLUSTER_STAZIONE_FIX ClusStaz(PATH_OUT "CLUSSTAZ.TMP");
   ClusStaz.PermettiScrittura();
   // Aggiorno i nodi di cambio
   ORD_FORALL(ClusStaz,Idx2){
      CLUSTER_STAZIONE & Cz = ClusStaz.FixRec(Idx2);
      Cz.TipoStazione = Fstaz[Cz.IdStazione].TipoStazione;
      ClusStaz.ModifyCurrentRecord();
      //TRACESTRING("Stazione "+STRINGA(Cz.IdStazione)+" Cluster "+STRINGA(Cz.IdCluster)+" Tipo "+STRINGA(Cz.TipoStazione));
   };
   ClusStaz.Flush();
   TryTime(0);

   ClusStaz.ReSort(Ord_Cluster_Distanza);
   ClusStaz.Flush();

   TRACESTRING("prima dell'ORD_FORALL ClusStaz,Idx3");

   int Prog = 0;
   int LastCluster = 0;
   ORD_FORALL(ClusStaz,Idx3){
      CLUSTER_STAZIONE & Cz = ClusStaz.FixRec(Idx3);
      if(Cz.IdCluster != LastCluster){
         LastCluster = Cz.IdCluster;
         Prog = 0;
      }
      if( Cz.TipoStazione > 1 || Cz.NumMzvFerm){
         Cz.Prog = Prog ++;
      } else {
         Cz.Prog = 999;
      }
      ClusStaz.ModifyCurrentRecord();
   }
   ClusStaz.ReSort(Ord_Cluster_SoloDistanza);
   ClusStaz.Flush();

   Prog = 0; LastCluster = 0;
   ORD_FORALL(ClusStaz,Idx4){
      CLUSTER_STAZIONE & Cz = ClusStaz.FixRec(Idx4);
      if(Cz.IdCluster != LastCluster){
         LastCluster = Cz.IdCluster;
         Prog = 0;
      }
      Cz.Prog2 = Prog ++;
      ClusStaz.ModifyCurrentRecord();
   }
   ClusStaz.Flush();

   // Adesso ripristino il corretto ordine di chiave
   ClusStaz.ReSort(Compare2Word);
   ClusStaz.Flush();

   TRACESTRING("prima del PutHeader 316");

   // Ed ora aggiusto l' header in modo che corrisponda ad un file BS con chiave di 4 bytes
   ClusStaz.PutHeader(2,-1,4);
   // ----------------------------------------------------------------------
   // Aggiungo Progressivo2 a F_FERMATE_VIRT
   // ----------------------------------------------------------------------
   LastMV      = 0;
   LastFermata = 0;
   TabFv.PermettiScrittura();
   Nr = TabFv.NumRecordsTotali();
   TRACESTRING("Aggiungo Progressivo2 a F_FERMATE_VIRT ");
   //puts("Aggiungo Progressivo2 a F_FERMATE_VIRT ");
   int Prog2=0;
   for (f = 0; f < Nr ; f++ ) {
      TryTime(f);
      FERMATE_VIRT & Fermata = TabFv.FixRec(f);

      // Trovo il treno virtuale ed il relativo cluster
      if (Fermata.MezzoVirtuale != LastMV) {
         Prog2=0;
         LastMV = Fermata.MezzoVirtuale ;
      }
      if( !Fermata.Transito || Fstaz[Fermata.Id].TipoStazione != 0 ){
         Fermata.Progressivo2 = ++ Prog2;
         TabFv.ModifyCurrentRecord();
      }
   }


   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------

   TRACESTRING("PROGRAMMA TERMINATO");

   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;

   return 0;
//<<< int  main int argc,char *argv
};

