//----------------------------------------------------------------------------
// ML_STEP7.CPP: Split dei mezzi virtuali in base alla periodicita
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "ML_IN.HPP"
#include "ML_WRK.HPP"
#include "seq_proc.hpp"
#include "alfa0.hpp"

#define PGM      "ML_STEP7"

struct PERIODICITA_FERMATE {
   T_PERIODICITA Periodicita ; // Periodicita
   SET   Fermate;     // Identifica quali tra le stazioni siano attive nella periodicita' specificata
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
   
   // ---------------------------------------------------------
   // Init
   // ---------------------------------------------------------
   F_CLUSTER_MV       Clust(PATH_OUT "MY_CLUST.TMP");
   Clust.PermettiScrittura();
   F_MEZZO_VIRTUALE   TabTv(PATH_OUT "M0_TRENV.TMP");
   F_MEZZOV_CLUSTER   ClusMezv(PATH_OUT "CLUSMEZV.TMP");
   F_FERMATE_VIRT     TabFv(PATH_OUT "M0_FERMV.TMP");
   F_MEZZO_VIRTUALE_2 TabT2(PATH_OUT "M2_TRENV.TMP");
   F_FERMATE_VIRT_2   TabF2(PATH_OUT "M2_FERMV.TMP");
   F_PERIODICITA_FERMATA_VIRT TabFvBis(PATH_OUT "M1_FERMV.TMP");
   TabT2.Clear();
   TabF2.Clear();
   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   ORD_FORALL(Clust,c){
      Clust[c].ExtraMezziVirtualiC = 0;
      Clust[c].ExtraMezziVirtualiD = 0;
      Clust.ModifyCurrentRecord();
   };
   
   // ---------------------------------------------------------
   // Ciclo di copia
   // ---------------------------------------------------------
   MEZZO_VIRTUALE_2 OutTv2;
   FERMATE_VIRT_2   OutFv2;
   PERIODICITA_FERMATE * Pf = new PERIODICITA_FERMATE[400];
   PERIODICITA_FERMATA_VIRT * PfV = new PERIODICITA_FERMATA_VIRT[1024];
   
   int NumTreniSplit=0,TotOut=0,MaxOut=0;
   
   printf("Debbo elaborare %i mezzi virtuali\n",TabTv.Dim());
   ORD_FORALL(TabTv,y){
      TryTime(y);
      MEZZO_VIRTUALE & Mzv = TabTv.FixRec(y);
      if(Mzv.NumeroFermateValide < 2){
         continue;
      };
      if(Mzv.PeriodicitaDisuniformi){
         NumTreniSplit++;
         WORD MezzoVirtuale = Mzv.MezzoVirtuale;
         TRACEINT("Estensione periodicita' per mezzo virtuale",Mzv.MezzoVirtuale);
         TRACEVLONG(Mzv.NumeroFermateTransiti);
         // -------------------------------------
         // Esplosione della periodicita'
         // -------------------------------------
         int NumPf = 0;
         TabFvBis.Seek(MezzoVirtuale);
         int i=0;
         while (&TabFvBis.RecordCorrente() && TabFvBis.RecordCorrente().MezzoVirtuale == MezzoVirtuale) {
            PfV[i] = TabFvBis.RecordCorrente();
            if(TabFv.Seek(PfV[i].MezzoVirtuale,PfV[i].Progressivo)){
               // PfV[i].Periodicita.Trace("Fermata["+STRINGA(i)+"].Periodicita MV="+STRINGA(PfV[i].MezzoVirtuale));
               i++;
            } else {
               Bprintf3("Ignorata periodicita' di fermata soppressa MV %i Progressivo %i",PfV[i].MezzoVirtuale,PfV[i].Progressivo);
            } /* endif */
            TabFvBis.Next();
         } /* endwhile */
         if (i != Mzv.NumeroFermateTransiti) {
            ERRSTRING("Errore interno P1 : " VRS(i) + VRS(Mzv.NumeroFermateTransiti));
            ERRSTRING("Mezzo virtuale: " +STRINGA(MezzoVirtuale));
            BEEP;
         } /* endif */
         int OffSetMax = PERIODICITA::FineOrarioFS - PERIODICITA::InizioOrario;
         for (int OffSet = 0; OffSet <= OffSetMax ; OffSet ++ ) {
            SET FermateDelGiorno;
            FermateDelGiorno.Realloc(i);
            for (int k = 0;k < i  ;k ++ ) {
               if (PfV[k].Periodicita.Circola(OffSet)) {
                  FermateDelGiorno.Set(k);
               } /* endif */
            } /* endfor */
            // TRACESTRING("Fermate Del Giorno: "+FermateDelGiorno.ToHex());
            if(!FermateDelGiorno.Empty()){ // Se ho almeno una fermata in quel giorno
               for (k = 0;k < NumPf ; k ++ ) { // Controllo se e' una delle periodicita' gia' identificate
                  if(FermateDelGiorno == Pf[k].Fermate)break;
               } /* endfor */
               if (k == NumPf) {
                  NumPf ++;
                  Pf[k].Periodicita.ReSet();
                  Pf[k].Fermate = FermateDelGiorno;
               } /* endif */
               Pf[k].Periodicita.Set(OffSet);
               // TRACESTRING("Pf["+STRINGA(k)+"].Fermate: "+Pf[k].Fermate.ToHex());
            };
//<<<    for  int OffSet = 0; OffSet <= OffSetMax ; OffSet ++    
         } /* endfor */
         TRACELONG("Numero differenti periodicita': ",NumPf);
         TotOut += NumPf;
         Top(MaxOut,NumPf);
         int NumAnomalie=0;
         for (int t = 0;t < NumPf ;t++ ) {
            Pf[t].Periodicita.Trace("Pf["+STRINGA(t)+"].Periodicita");
            TRACESTRING("MV "+STRINGA(MezzoVirtuale)+" Pf["+STRINGA(t)+"].Fermate: "+Pf[t].Fermate.ToHex());
            if(Pf[t].Fermate.Cardinalita() < 2){
               printf("Anomalia su MV Nø %i : vi sono dei giorni in cui risultano meno di due fermate\n",MezzoVirtuale);
               TRACESTRING("Anomalia : vi sono dei giorni in cui risultano meno di due fermate");
               NumAnomalie ++;
            }
         } /* endfor */
         ClusMezv.Seek(MezzoVirtuale);
         if (ClusMezv.RecordCorrente().VersoConcorde) {
            Clust[ClusMezv.RecordCorrente().IdCluster].ExtraMezziVirtualiC += NumPf -1 -NumAnomalie;
         } else {
            Clust[ClusMezv.RecordCorrente().IdCluster].ExtraMezziVirtualiD += NumPf -1 -NumAnomalie;
         } /* endif */
         Clust.ModifyCurrentRecord();
         
         int ProgPer=0;
         for (int k = 0; k < NumPf; k++) {
            //if( MezzoVirtuale == 1619 ||MezzoVirtuale == 7201 ){
            //   TRACEVLONG(MezzoVirtuale);
            //   TRACEVLONG(k);
            //   TRACEVLONG(Pf[k].Fermate.Cardinalita());
            //}
            if( Pf[k].Fermate.Cardinalita() < 2)continue; // Anomalia
            TabFv.Seek(MezzoVirtuale);
            memmove(&OutTv2,&Mzv,sizeof(MEZZO_VIRTUALE));
            OutTv2.ProgressivoPeriodicita = ProgPer ;
            OutTv2.PeriodicitaMV = Pf[k].Periodicita;
            TabT2.AddRecordToEnd(VRB(OutTv2));
            
            i=0;
            while (&TabFv.RecordCorrente()&& TabFv.RecordCorrente().MezzoVirtuale == MezzoVirtuale) {
               memmove(&OutFv2,&TabFv.RecordCorrente(),sizeof(FERMATE_VIRT));
               OutFv2.ProgressivoPeriodicita = ProgPer ;
               if (!Pf[k].Fermate.Test(i++)) {
                  if(OutFv2.FermataPartenza || OutFv2.FermataArrivo)OutFv2.Transito = 1; // Trasformo in transito
                  OutFv2.FermataPartenza  =0;
                  OutFv2.FermataArrivo    =0;
               } /* endif */
               TabF2.AddRecordToEnd(VRB(OutFv2));
               TabFv.Next();
            } /* endwhile */
            ProgPer ++;
//<<<    for  int k = 0; k < NumPf; k++   
         } /* endfor */
         // -------------------------------------
         
//<<< if Mzv.PeriodicitaDisuniformi  
      } else {
         // TRACEINT("Copio in out il treno virtuale",Mzv.MezzoVirtuale);
         // TRACEVLONG(TabTv.NumRecordCorrente());
         // Copio direttamente in out
         memmove(&OutTv2,&Mzv,sizeof(MEZZO_VIRTUALE));
         OutTv2.ProgressivoPeriodicita = 0;
         
         TabT2.AddRecordToEnd(VRB(OutTv2));
         
         WORD MezzoVirtuale = Mzv.MezzoVirtuale;
         TabFv.Seek(MezzoVirtuale);
         while (&TabFv.RecordCorrente()&& TabFv.RecordCorrente().MezzoVirtuale == MezzoVirtuale) {
            memmove(&OutFv2,&TabFv.RecordCorrente(),sizeof(FERMATE_VIRT));
            OutFv2.ProgressivoPeriodicita = 0;
            TabF2.AddRecordToEnd(VRB(OutFv2));
            TabFv.Next();
         } /* endwhile */
      } /* endif */
      
//<<< ORD_FORALL TabTv,y  
   };
   
   printf("Numero di MV con periodicita' disuniformi %i Generano %i MV maxSplit : %i treni\n" ,NumTreniSplit,TotOut,MaxOut);
   
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
//<<< int  main int argc,char *argv    
}

