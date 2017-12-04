//----------------------------------------------------------------------------
// ML_STEPX.CPP: Controllo e statistiche
//----------------------------------------------------------------------------
// - Identifico treni con tratte invertite rispetto alla norma
// - Conteggio note di infocomm
// - Conteggio numero differenti PERIODICITA
//----------------------------------------------------------------------------
// #define CONTA_NOTE_PER
#define IDENTIFICA_INVERTITI
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2
#include "FT_PATHS.HPP"  // Path da utilizzare

#include "eventi.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_OUT.HPP"
#include "elenco.h"
#include "oggetto.h"
#include "seq_proc.hpp"
#include "file_t.hpp"


#define PGM      "ML_STEPX"



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
   
   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore
   TryTime(0);
   
   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   
   // FILE * Out;
   // Out = fopen(PATH_OUT PGM ".OUT","wt");
   
   // ---------------------------------------------------------
   // - Conteggio note di infocomm
   // - Conteggio numero differenti PERIODICITA
   // ---------------------------------------------------------
   #ifdef CONTA_NOTE_PER
   FILE_INFOCOMM InfoComm(PATH_IN  "INFOCOMM.T");
   ARRAY_T_PERIODICITA AllPeriodicitaInfoComm;
   int CntTreni = 0, CntNote = 0;
   T_PERIODICITA Per;
   char LastTr;
   ORD_FORALL(InfoComm,Ic){
      InfoComm.Posiziona(Ic);
      //STRINGA IdentTreno;
      if (InfoComm.RecordCorrente().TipoRecord == '1') {
         //IdentTreno = St(InfoComm.RecordCorrente().R1.IdentTreno);
         CntTreni ++;
         Per.Set();
      } else if (InfoComm.RecordCorrente().TipoRecord == '2') {
         CntNote ++;
      } else if (InfoComm.RecordCorrente().TipoRecord == '3') {
         Per.ComponiPeriodicita(5,InfoComm.RecordCorrente().R3.Periodicita,(LastTr != '3'));
      } else {
         printf("Records illegali in INFOCOMM \n");
         BEEP;
         exit(999);
      } /* endif */
      LastTr = InfoComm.RecordCorrente().TipoRecord;
      if(!AllPeriodicitaInfoComm.Contiene(Per)){
         AllPeriodicitaInfoComm += Per;
      } /* endif */
   }
   printf("INFOCOMM : vi sono %i differenti valori di periodicita' per %i note commerciali su %i treni\n",
      AllPeriodicitaInfoComm.Dim(),CntNote, CntTreni);
   
   // ---------------------------------------------------------
   // - Conteggio note di NOTEPART
   // - Conteggio numero differenti PERIODICITA
   // ---------------------------------------------------------
   FILE_NOTEPART NotePart(PATH_IN  "NOTEPART.T");
   ARRAY_T_PERIODICITA AllPeriodicitaNotePart;
   CntNote = 0;
   ORD_FORALL(NotePart,Np){
      NotePart.Posiziona(Np);
      if (NotePart.RecordCorrente().TipoRecord == '1') {
         CntNote ++;
         Per.Set();
      } else if (NotePart.RecordCorrente().TipoRecord == '3') {
         Per.ComponiPeriodicita(5,NotePart.RecordCorrente().R3.Periodicita,(LastTr != '3'));
      } /* endif */
      LastTr = NotePart.RecordCorrente().TipoRecord;
      if(!AllPeriodicitaNotePart.Contiene(Per)){
         AllPeriodicitaNotePart += * new T_PERIODICITA(Per);
      } /* endif */
   }
   printf("NOTEPART : vi sono %i differenti valori di periodicita' per %i note\n",
      AllPeriodicitaNotePart.Dim(),CntNote);
   
   // ---------------------------------------------------------
   // - Conteggio numero differenti PERIODICITA
   // ---------------------------------------------------------
   F_MEZZO_VIRTUALE_2 TabTv2(PATH_OUT "M2_TRENV.TMP");
   ARRAY_T_PERIODICITA AllPeriodicita;
   ORD_FORALL(TabTv2,t){
      TabTv2.Posiziona(t);
      T_PERIODICITA & Per = TabTv2.RecordCorrente().PeriodicitaMV;
      // Per.Trace("Prova, MV = "+STRINGA(TabTv2.RecordCorrente().MezzoVirtuale));
      if(!AllPeriodicita.Contiene(Per)){
         AllPeriodicita += * new T_PERIODICITA(Per);
      } /* endif */
   }
   printf("Mezzi virtuali : vi sono %i differenti valori di periodicita' per %i treni\n", AllPeriodicita.Dim(),TabTv2.Dim());
   #endif
   
   
   // ---------------------------------------------------------
   // Scandisco i treni virtuali ed identico le sequenze fuori ordine
   // ---------------------------------------------------------
   #ifdef IDENTIFICA_INVERTITI
   // Apro treni virtuali e relative fermate, e relazione con i clusters
   F_FERMATE_VIRT   TabFv(PATH_OUT "M0_FERMV.TM2");
   F_MEZZO_VIRTUALE TabTv(PATH_OUT "M0_TRENV.TMP");
   F_MEZZOV_CLUSTER ClusMezv(PATH_OUT "CLUSMEZV.TMP");
   // Apro relazione tra clusters e stazioni
   F_CLUSTER_STAZIONE ClusStaz(PATH_OUT "CLUSSTAZ.TMP");
   STAZIONI Stazioni(PATH_DATI);
   
   MEZZOV_CLUSTER Mv_Clu;
   USHORT LastMV = 0;
   int    LastKm = 0;
   int    LastKmMv = 0;
   int    LastId = 0;
   int NumTreniConFermateInvertite = 0;
   int FermateInvertite = 0;
   BOOL HoFermateInvertite = FALSE;
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
            if(ClusStaz.RecordCorrente().Distanza < LastKm){
               STRINGA Stz1(Stazioni[LastId].NomeStazione);
               STRINGA Stz2(Stazioni[Fermata.Id].NomeStazione);
               TRACESTRING("MV "+STRINGA(LastMV)+" stazioni fuori sequenza: "+
                  STRINGA(LastId)+" "+Stz1+" ("+STRINGA(LastKmMv)+"/"+STRINGA(LastKm)+") -> "+
                  STRINGA(Fermata.Id)+" "+Stz2+" ("+STRINGA(Fermata.ProgKm)+"/"+STRINGA(ClusStaz.RecordCorrente().Distanza)+")"); 
               FermateInvertite ++ ;
               HoFermateInvertite = TRUE ;
            }
         } else {
            if(ClusStaz.RecordCorrente().Distanza > LastKm){
               STRINGA Stz1(Stazioni[LastId].NomeStazione);
               STRINGA Stz2(Stazioni[Fermata.Id].NomeStazione);
               TRACESTRING("MVDISC "+STRINGA(LastMV)+" stazioni fuori sequenza: "+
                  STRINGA(LastId)+" "+Stz1+" ("+STRINGA(LastKmMv)+"/"+STRINGA(LastKm)+") -> "+
                  STRINGA(Fermata.Id)+" "+Stz2+" ("+STRINGA(Fermata.ProgKm)+"/"+STRINGA(ClusStaz.RecordCorrente().Distanza)+")"); 
               FermateInvertite ++ ;
               HoFermateInvertite = TRUE ;
            }
         } /* endif */
//<<< if LastMV  
      };
      int KmDaCluster;
      if (Mv_Clu.VersoConcorde) {
         KmDaCluster = Mv_Clu.Distanza1 + ClusStaz.RecordCorrente().Distanza;
      } else {
         KmDaCluster = Mv_Clu.Distanza1 - ClusStaz.RecordCorrente().Distanza;
      }
      if (abs(KmDaCluster - Fermata.ProgKm) > 255) {
         Bprintf("Discordanza eccessiva a fermata %i del cluster %i Mezzo virtuale = %i",Fermata.Id, Mv_Clu.IdCluster, Fermata.MezzoVirtuale);
         Bprintf("KmDaCluster %i Fermata.ProgKm %i Distanza1 %i DistanzaClu %i", KmDaCluster ,Fermata.ProgKm ,Mv_Clu.Distanza1 ,ClusStaz.RecordCorrente().Distanza);
      } /* endif */
      LastMV = Fermata.MezzoVirtuale ;
      LastKm = ClusStaz.RecordCorrente().Distanza;
      LastKmMv = Fermata.ProgKm;
      LastId = Fermata.Id;
//<<< ORD_FORALL TabFv,f  
   }
   Bprintf("Numero MV con fermate invertite: %i per %i fermate", NumTreniConFermateInvertite , FermateInvertite );
   #endif
   
   
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   // fclose(Out);
   return 0;
//<<< int  main int argc,char *argv    
}

