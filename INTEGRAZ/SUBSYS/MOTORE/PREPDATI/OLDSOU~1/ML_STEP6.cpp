//----------------------------------------------------------------------------
// ML_STEP6.CPP: Statistiche e gestione collegamenti
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "ML_IN.HPP"
#include "ML_WRK.HPP"
#include "seq_proc.hpp"
#include "mm_path.hpp"

#define NO_STATS // Non genera statistiche

#define PGM      "ML_STEP6"

int CompareCollF(const void * a,const void * b){
   COLLEGAMENTO_F & A = *(COLLEGAMENTO_F *)a;
   COLLEGAMENTO_F & B = *(COLLEGAMENTO_F *)b;
   if(A.StazionePartenza != B.StazionePartenza)return int(A.StazionePartenza) - int(B.StazionePartenza);
   return int(A.StazioneArrivo) - int(B.StazioneArrivo);
};

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int  main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   SetStatoProcesso(PGM, SP_PARTITO);
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
   SetPriorita(); // Imposta la priorita'
   

   GESTIONE_ECCEZIONI_ON
   
   DosError(2); // Disabilita Popup di errore
   
   TryTime(0);

   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);
   
   // APRO il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);
   if (CCR_ID::CcrId == NULL) {
      CCR_ID::CcrId = new CCR_ID(PATH_DATI);
   } /* endif */
   
   
   // Apro l' archivio Stazioni 
   F_STAZIONE_MV  Fstaz(PATH_OUT "MY_STAZI.TMP");
   F_COLLEGAMENTO1 CollegamentiTraStazioni(PATH_OUT "COLLSTAZ.TMP");
   FIX_COLLEGAMENTO_F CollegamentiOut(PATH_OUT "MM_COLL.DB"); 
   CollegamentiOut.Clear("Collegamenti Stazs di Cambio");
   F_COLLEGAMENTO2 CollegamentiTraStazioniNelCluster(PATH_OUT "COLLCLUS.TMP");

   // Leggo i dati delle multistazioni
   GEST_MULTI_STA Gestore(PATH_CVB,NULL,Fstaz);

   // ---------------------------------------------------------
   // Scarico i dati delle stazioni
   // ---------------------------------------------------------
   BS_STAZIONE_F  FileStaz(PATH_OUT "MM_STAZN.DB");
   FileStaz.Clear("Dati di tutte le stazioni specifici dell' orario");
   ORD_FORALL(Fstaz,Stz1){
      STAZIONE_F Rec;
      Rec.Id                 = Fstaz[Stz1].Id;
      Rec.ClasseCoincidenza  = Fstaz[Stz1].ClasseCoincidenza;
      Rec.Citta              = 0;
      Rec.Peso               = Fstaz[Stz1].NumFermate * 10;
      Rec.TipoStazione       = Fstaz[Stz1].TipoStazione & 3;
      WORD FasceP = 0;
      WORD FasceA = 0;
      CollegamentiTraStazioni.Seek(Rec.Id);
      while (&CollegamentiTraStazioni.RecordCorrente() && CollegamentiTraStazioni.RecordCorrente().StazionePartenza == Rec.Id) {
         FasceP |=  CollegamentiTraStazioni.RecordCorrente().OrariPartenza;
         FasceA |=  CollegamentiTraStazioni.RecordCorrente().OrariVeriArrivo;
         CollegamentiTraStazioni.Next();
      } /* endwhile */
      Rec.FascePartenza      = FasceP;  // Questo per il criterio di arresto
      Rec.FasceArrivo        = FasceA;  // Questo per il criterio di arresto
      FileStaz.AddRecordToEnd(VRB(Rec));
   }
   FileStaz.ReSortFAST();
   // Aggiungo i dati delle citta'
   for (int c = 0;c < Gestore.NumCitta ;c++ ) {
      ORD_FORALL(Gestore.Citta[c],u){
         if(FileStaz.Seek(Gestore.Citta[c][u])){
            FileStaz.RecordCorrente().Citta = c+1;
            FileStaz.ModifyCurrentRecord();
         } else {
            Bprintf("Errore aggiornando i dati per citta' ID = %i",Gestore.Citta[c][u]);
         } /* endif */
      }
   } /* endfor */
   
   // ----------------------------------------------------------------------
   // Statistiche senza clusters
   // ----------------------------------------------------------------------
   // Questa informazione mi serve SEMPRE
   TryTime(0);
   Bprintf("Statistiche");
   int Count[4];
   ZeroFill(Count);
   int Nr3 = Fstaz.NumRecordsTotali();
   for (int l = 0; l < Nr3 ; l++ ) {
      if(Fstaz[l].NumClusters > 0) Count[Fstaz[l].TipoStazione % 4] ++;
   }
   Bprintf("Vi sono %i Nodi non di cambio ,%i Nodi di cambio,  %i stazioni di cambio non nodali, %i fermate", Count[2],Count[3],Count[1],Count[0]);
   
   Bprintf("Scrittura istogrammi collegamenti stazioni di CAMBIO");
   int NumCollC =0,NumCollConC=0;
   HASH<COLLEGAMENTO1> CollStaz(32000);  // Ci metto i soli collegamenti tra stazioni di cambio
   ORD_FORALL(CollegamentiTraStazioni,NumCst){
      COLLEGAMENTO1 Cst = CollegamentiTraStazioni.FixRec(NumCst);
      // TRACESTRING(VRS(Cst.KmMinimi) + " Per collegamento "+ VRS(Cst.StazionePartenza) + VRS(Cst.StazioneArrivo) );
      if(Fstaz[Cst.StazionePartenza].StazioneDiCambio() || Fstaz[Cst.StazioneArrivo].StazioneDiCambio()) NumCollConC ++;
      if(Fstaz[Cst.StazionePartenza].StazioneDiCambio() && 
         Fstaz[Cst.StazioneArrivo].StazioneDiCambio()){
         NumCollC ++;
         // Metto il collegamento nella hash table : Si noti che la chiave e' una singola stazione
         COLLEGAMENTO1 * H1 = CollStaz.Alloca();
         (*H1) = Cst;
         CollStaz.Metti(2); // Solo due bytes per chiave !
         // Copio il collegamento in out
         COLLEGAMENTO_F Out; 
         int MCount = 1;
         int Privilegiato = 0;
         if( Gestore.StazioniMS.Contiene(Cst.StazionePartenza) && Gestore.StazioniMS.Contiene( Cst.StazioneArrivo )){
            FORALL(Gestore.ClustersMultiStazione,i){
               MULTI_STA & Mult = * Gestore.ClustersMultiStazione[i];
               WORD Idx1 = Mult.Stazioni.Posizione(Cst.StazionePartenza);
               WORD Idx2 = Mult.Stazioni.Posizione(Cst.StazioneArrivo);
               if( Idx1 != WORD_NON_VALIDA && Idx2 != WORD_NON_VALIDA){
                  // Controllo che vi sia realmente un collegamento urbano
                  if (Mult.Collegamento(Idx1,Idx2).Minuti) {
                     MCount = 0;
                     // Se il collegamento richiede da 1 a 5 minuti lo considero nella stessa
                     // stazione o comunque tale da non comportare costi addizionali
                     if(Mult.Collegamento(Idx1,Idx2).Minuti <= 5) Privilegiato = 1;

                     // Se il collegamento ha meno di 8  treni lo forzo a 8  treni (perche' ha il multistazione)
                     Top(Cst.Count, 8 );
                     Bottom(Cst.KmMinimi,Mult.Collegamento(Idx1,Idx2).Km);
                     // Inoltre il collegamento pu• avvenire a tutte le ore
                     Cst.OrariPartenza    = 0x3ff;
                     Cst.OrariArrivo      = 0x3ff;
                     Cst.Partenza32       = 0xffffffff;
                     Cst.Arrivo32         = 0xffffffff;
                     // Bprintf3("Collegamento urbano tra stazioni %i => %i : Km %i",Cst.StazionePartenza,Cst.StazioneArrivo,Cst.KmMinimi);
                  } /* endif */
                  break;
               } /* endif */
            }
         }
         Out.StazionePartenza = Chk(Cst.StazionePartenza  , 13         );
         Out.StazioneArrivo   = Chk(Cst.StazioneArrivo    , 13         );
         Out.PesoTratta       = Chk(MCount                ,  1         );
         Out.Privilegiato     = Chk(Privilegiato          ,  1         );
         Out.Count            = Chk(min(Cst.Count,31)     ,  5         );
         Out.KmMinimi         = Chk(Cst.KmMinimi          , 12         );
         Out.OrariPartenza    = Chk(Cst.OrariPartenza     , NUM_FASCE  );
         Out.OrariArrivo      = Chk(Cst.OrariArrivo       , NUM_FASCE  );
         //Out.OrariArrivo2     = Chk(Cst.OrariArrivo2      , NUM_FASCE  );
         Out.Partenza32       = Cst.Partenza32 ;
         Out.Arrivo32         = Cst.Arrivo32   ;
         Out.TcollMin         = Chk(Cst.TcollMin ,5);
         Out.TcollMax         = Chk(Cst.TcollMax ,5);
         CollegamentiOut.AddRecordToEnd(VRB(Out));
      }
   }

   CollegamentiOut.Flush();
   
   Bprintf("Vi sono %i Collegamenti tra stazioni di cambio e %i tra stazioni generiche e stazioni di cambio su %i", NumCollC, NumCollConC, CollegamentiTraStazioni.Dim() );
   
   // Simmetrizzo i collegamenti tra stazioni
   ORD_FORALL(CollegamentiOut, l1){
      COLLEGAMENTO_F & Cst = CollegamentiOut[l1];
      BOOL Esiste = CollegamentiTraStazioni.Seek(Cst.StazioneArrivo,Cst.StazionePartenza);
      if(Esiste){
         COLLEGAMENTO1 & Cst2 = CollegamentiTraStazioni.RecordCorrente();
         if( Gestore.StazioniMS.Contiene(Cst.StazionePartenza) && Gestore.StazioniMS.Contiene( Cst.StazioneArrivo )){
            FORALL(Gestore.ClustersMultiStazione,i){
               MULTI_STA & Mult = * Gestore.ClustersMultiStazione[i];
               WORD Idx1 = Mult.Stazioni.Posizione(Cst.StazionePartenza);
               WORD Idx2 = Mult.Stazioni.Posizione(Cst.StazioneArrivo);
               if( Idx1 != WORD_NON_VALIDA && Idx2 != WORD_NON_VALIDA){
                  if (Mult.Collegamento(Idx1,Idx2).Minuti) {
                     // Aggiorno IN MEMORIA i dati di Cst2, che pero' NON verranno scaricati su disco
                     // Se il collegamento ha meno di 8  treni lo forzo a 8  treni (perche' ha il multistazione)
                     Top(Cst2.Count, 8 );
                     Bottom(Cst2.KmMinimi,Mult.Collegamento(Idx1,Idx2).Km);
                     // Inoltre il collegamento pu• avvenire a tutte le ore
                     Cst2.OrariPartenza    = 0x3ff;
                     Cst2.OrariArrivo      = 0x3ff;
                     Cst2.Partenza32       = 0xffffffff;
                     Cst2.Arrivo32         = 0xffffffff;
                  } /* endif */
                  break;
               } /* endif */
            }
         }
         if(Cst.Count != Cst2.Count || Cst.KmMinimi != Cst2.KmMinimi){
            Bprintf3("Simmetrizzata in modifica relazione tra stazioni: %i => %i", Cst.StazioneArrivo,Cst.StazionePartenza);
            TRACESTRING( VRS(Cst.Count) + VRS(Cst2.Count) + VRS(Cst.KmMinimi) + VRS(Cst2.KmMinimi) );
            Cst.Count    = (Cst.Count + min(31,Cst2.Count) + 1)   / 2; 
            Cst.KmMinimi = (Cst.KmMinimi + Cst2.KmMinimi) / 2;
            TRACESTRING( VRS(Cst.Count) + VRS(Cst.KmMinimi) );
            CollegamentiOut.ModifyCurrentRecord();
         }
      } else {
         COLLEGAMENTO_F Cst2 = Cst;
         Cst2.StazionePartenza = Cst.StazioneArrivo;
         Cst2.StazioneArrivo   = Cst.StazionePartenza;
         Cst2.OrariArrivo      = 0;  // Collegamento non esistente in realta'
         //Cst2.OrariArrivo2   = 0;  // Collegamento non esistente in realta'
         Cst2.OrariPartenza    = 0;  // Collegamento non esistente in realta'
         Cst2.Partenza32       = 0;  // Collegamento non esistente in realta' 
         Cst2.Arrivo32         = 0;  // Collegamento non esistente in realta' 
         Cst2.TcollMin         = 0;  // Collegamento non esistente in realta' 
         Cst2.TcollMax         = 0;  // Collegamento non esistente in realta' 
         if( Gestore.StazioniMS.Contiene(Cst.StazionePartenza) && Gestore.StazioniMS.Contiene(Cst.StazioneArrivo )){
            FORALL(Gestore.ClustersMultiStazione,i){
               MULTI_STA & Mult = * Gestore.ClustersMultiStazione[i];
               WORD Idx1 = Mult.Stazioni.Posizione(Cst.StazionePartenza);
               WORD Idx2 = Mult.Stazioni.Posizione(Cst.StazioneArrivo);
               if( Idx1 != WORD_NON_VALIDA && Idx2 != WORD_NON_VALIDA){
                  if (Mult.Collegamento(Idx1,Idx2).Minuti) {
                     // Aggiorno IN MEMORIA i dati di Cst2, che pero' NON verranno scaricati su disco
                     // Se il collegamento ha meno di 8  treni lo forzo a 8  treni (perche' ha il multistazione)
                     Top(Cst2.Count, 8 );
                     Bottom(Cst2.KmMinimi,Mult.Collegamento(Idx1,Idx2).Km);
                     // Inoltre il collegamento pu• avvenire a tutte le ore
                     Cst2.OrariPartenza    = 0x3ff;
                     Cst2.OrariArrivo      = 0x3ff;
                     Cst2.Partenza32       = 0xffffffff;
                     Cst2.Arrivo32         = 0xffffffff;
                  } /* endif */
                  break;
               } /* endif */
            }
         }
         Bprintf3("Simmetrizzata in aggiunta relazione tra stazioni: %i => %i", Cst.StazioneArrivo,Cst.StazionePartenza);
         CollegamentiOut.AddRecord(VRB(Cst2)); // Uso questo metodo perche' scrive subito su file
      };
   }

   // Adesso aggiungo i collegamenti multistazione su cui non esistono treni
   ORD_FORALL(Gestore.ClustersMultiStazione,K0){
      MULTI_STA & Mult = * Gestore.ClustersMultiStazione[K0];
      if(Mult.Colleg == NULL)continue; // Anomalo
      ORD_FORALL(Mult.Stazioni,K1){
         ORD_FORALL(Mult.Stazioni,K2){
            COLLEG_URBANO & Coll = Mult.Collegamento(K1,K2);
            if (Coll.Minuti) { // Collegamento effettivo
               BOOL Esiste = CollegamentiTraStazioni.Seek( Mult.Stazioni[K1], Mult.Stazioni[K2]) || CollegamentiTraStazioni.Seek( Mult.Stazioni[K2], Mult.Stazioni[K1]);
               if(!Esiste){
                  Bprintf2("Aggiunto collegamento tra Multi-Stazioni %i => %i",Mult.Stazioni[K1], Mult.Stazioni[K2]);
                  COLLEGAMENTO_F Out; 
                  Out.StazionePartenza = Chk(Mult.Stazioni[K1]     , 13         );
                  Out.StazioneArrivo   = Chk(Mult.Stazioni[K2]     , 13         );
                  Out.PesoTratta       = 0;
                  Out.Privilegiato     = Coll.Minuti <= 5;
                  Out.Count            = 8 ; // Considero equivalente a 16 treni al giorno
                  Out.KmMinimi         = Coll.Km;
                  Out.OrariPartenza    = 0xffff;
                  Out.OrariArrivo      = 0xffff;
                  Out.Partenza32       = 0xffffffff;
                  Out.Arrivo32         = 0xffffffff;
                  Out.TcollMin         = 0;
                  Out.TcollMax         = 0;
                  //Out.OrariArrivo2     = 0xffff;
                  CollegamentiOut.AddRecordToEnd(VRB(Out));
               }
            } /* endif */
         }
      }
   };

   CollegamentiOut.Flush();
   CollegamentiOut.ReSort(CompareCollF);
   

   // ----------------------------------------------------------------------
   // Da qui in poi solo se voglio le statistiche
   // ----------------------------------------------------------------------
   #ifndef NO_STATS

   // Adesso calcolo quante seconde connessioni ho tra nodi di cambio, con e
   // senza il filtro delle ore di arrivo.
   DWORD NumStazioni=0,SumStazione=0,SumStazioneC=0;
   DWORD NumSecondeConnessioni=0;
   DWORD NumSecondeConnessioniCondizionate=0;
   ID LastStazione=0;
   ORD_FORALL(CollegamentiTraStazioni,NumCst3){
      COLLEGAMENTO1 & Cst = CollegamentiTraStazioni.FixRec(NumCst3);
      //TRACEVLONG(Cst.StazionePartenza);
      //TRACEVLONG(Cst.StazioneArrivo);
      if(Fstaz[Cst.StazionePartenza].StazioneDiCambio() && 
         Fstaz[Cst.StazioneArrivo].StazioneDiCambio()){
         if(LastStazione != Cst.StazionePartenza){
            Bprintf3("Seconde connessioni per stazione %i: %i condizionate %i",LastStazione,SumStazione,SumStazioneC);
            LastStazione = Cst.StazionePartenza;
            SumStazione=0,SumStazioneC=0;
            NumStazioni ++;
         }
         if(Cst.StazionePartenza == 13){
            TRACEVLONG(Cst.StazionePartenza);
            TRACEVLONG(Cst.StazioneArrivo);
            TRACEVPOINTER(Cst.OrariArrivo);
         }
         //TRACESTRING("Ok Collegamento tra stazioni di cambio");
         // Cerco la seconda stazione come stazione di partenza
         COLLEGAMENTO1 * H1 = CollStaz.Cerca((COLLEGAMENTO1 *)&Cst.StazioneArrivo,2);
         while (H1) {
            //TRACEVPOINTER(H1);
            //TRACEVLONG(H1->StazioneArrivo);
            if(Cst.StazionePartenza == 13){
               TRACEVLONG(H1->StazionePartenza);
               TRACEVLONG(H1->StazioneArrivo);
               TRACEVPOINTER(H1->OrariPartenza);
               TRACEVLONG(Cst.OrariArrivo & H1->OrariPartenza);
            }
            // Seleziono le stazioni di cambio esclusa la precedente stazione stessa
            if(Fstaz[H1->StazioneArrivo].StazioneDiCambio() && 
               H1->StazioneArrivo != Cst.StazionePartenza ){
               NumSecondeConnessioni ++;
               SumStazione ++;
               if(Cst.OrariArrivo & H1->OrariPartenza){
                  NumSecondeConnessioniCondizionate ++;
                  SumStazioneC ++;
               }
            }
            H1 = CollStaz.CercaNext();
         } /* endwhile */
//<<< if Fstaz Cst.StazionePartenza .StazioneDiCambio   && 
      }
//<<< ORD_FORALL CollegamentiTraStazioni,NumCst3  
   }
   Bprintf2("NumStazioni %i NumSecondeConnessioni %i NumSecondeConnessioniCondizionate %i ",
      NumStazioni ,NumSecondeConnessioni ,NumSecondeConnessioniCondizionate);
   
   Bprintf("Numero seconde connessioni/stazione: %i Condizionate: %i",
      NumSecondeConnessioni / NumStazioni , NumSecondeConnessioniCondizionate/ NumStazioni);
   
   // ----------------------------------------------------------------------
   // Statistiche con clusters
   // ----------------------------------------------------------------------
   Bprintf("Statistiche con clusters");
   int NumCollC2 = 0;
   int NumConcordi=0,NumDiscordi=0;
   CollStaz.Clear(); // Ci carico i collegamenti tra stazioni di cambio
   ORD_FORALL(CollegamentiTraStazioni,NumCst9){
      COLLEGAMENTO1 Cst = CollegamentiTraStazioni.FixRec(NumCst9);
      if(Fstaz[Cst.StazionePartenza].StazioneDiCambio() && 
         Fstaz[Cst.StazioneArrivo].StazioneDiCambio()){
         COLLEGAMENTO1 * H1 = CollStaz.Alloca();
         (*H1) = Cst;
         CollStaz.Metti(4); 
      }
   }
   HASH<COLLEGAMENTO2> Coll2(16000);  // Ci metto i soli collegamenti tra stazioni di cambio
   HASH<COLLEGAMENTO3> Coll3(8000); // Ci metto i soli collegamenti tra stazioni di cambio 
   ORD_FORALL(CollegamentiTraStazioniNelCluster,NumCst2){
      COLLEGAMENTO2 & C2  = CollegamentiTraStazioniNelCluster.FixRec(NumCst2);
      if (C2.Concorde ) {
         NumConcordi ++;
      } else {
         NumDiscordi ++;
      } /* endif */
      if(Fstaz[C2.StazionePartenza].StazioneDiCambio() && 
         Fstaz[C2.StazioneArrivo].StazioneDiCambio()){
         NumCollC2 ++;
         // Metto il collegamento nella hash table : Si noti che la chiave e' una singola stazione
         COLLEGAMENTO2 * H2 = Coll2.Alloca();
         (*H2) = C2;
         Coll2.Metti(2); // Solo due bytes per chiave !
         COLLEGAMENTO3 A3; ZeroFill(A3); A3.IdCluster = C2.IdCluster; 
         A3.Stazione = C2.StazionePartenza; A3.Concorde = C2.Concorde;
         COLLEGAMENTO3 * Target = Coll3.Cerca(&A3,5); // Cluster, Stazione e verso
         if (Target == NULL) { Target = Coll3.Alloca(); *Target = A3;  Coll3.Metti(5); } 
         Target ->Count         += C2.Count;
         Target ->OrariPartenza |= C2.OrariPartenza  ;
         Target ->Partenza32    |= C2.Partenza32     ;
         A3.Stazione = C2.StazioneArrivo; A3.Concorde = C2.Concorde;
         Target = Coll3.Cerca(&A3,5); // Cluster, Stazione e verso
         if (Target == NULL) { Target = Coll3.Alloca(); *Target = A3;  Coll3.Metti(5); } 
         Target ->Count         += C2.Count;
         Target ->OrariArrivo   |= C2.OrariArrivo ;
         Target ->OrariArrivo2  |= C2.OrariArrivo2;
         Target ->Arrivo32      |= C2.Arrivo32       ;
      }
//<<< ORD_FORALL CollegamentiTraStazioniNelCluster,NumCst2  
   }
   Bprintf("Vi sono IN TOTALE %i Collegamenti concordi e %i discordi tra stazioni / cluster",NumConcordi,NumDiscordi);
   Bprintf("Vi sono %i Collegamenti tra stazioni di cambio / cluster",NumCollC2);
   
   // Seconde connessioni tenendo conto dei clusters
   // Il filtro delle ore di arrivo e' calcolato sia esattamente (per connessione) che a livello cluster/stazione
   DWORD NumSecondeConnessioniCondizionateCluster=0,SumStazioneC2=0;
   DWORD AncheCondStaz = 0, ClustEStaz = 0;
   NumStazioni=0,SumStazione=0,SumStazioneC=0;
   NumSecondeConnessioni=0;
   NumSecondeConnessioniCondizionate=0;
   LastStazione=0;
   ORD_FORALL(CollegamentiTraStazioniNelCluster,NumCst4){
      COLLEGAMENTO2 C2   = CollegamentiTraStazioniNelCluster.FixRec(NumCst4); // Vado per copia per poter spostare il file
      if(Fstaz[C2.StazionePartenza].StazioneDiCambio() && 
         Fstaz[C2.StazioneArrivo].StazioneDiCambio()){
         if(LastStazione != C2.StazionePartenza){
            Bprintf2("Seconde connessioni per stazione %i: %i condizionate %i Condizionate * Cluster %i",LastStazione,SumStazione,SumStazioneC,SumStazioneC2);
            LastStazione = C2.StazionePartenza;
            SumStazione=0,SumStazioneC=0 ,SumStazioneC2=0;
            NumStazioni ++;
         }
         WORD OrariArrivo = CollStaz.Cerca((COLLEGAMENTO1*)&(C2.StazionePartenza),4)->OrariArrivo;
         if(C2.StazionePartenza == 13){
            TRACEVLONG(C2.StazionePartenza);
            TRACEVLONG(C2.StazioneArrivo);
            TRACEVLONG(C2.IdCluster);
            TRACEVPOINTER(OrariArrivo);
         }
         COLLEGAMENTO3 C3; C3.IdCluster = C2.IdCluster; C3.Stazione = C2.StazioneArrivo; C3.Concorde = C2.Concorde;
         C3 = * Coll3.Cerca(&C3,5); // Completo le informazioni
         // Cerco la seconda stazione come stazione di partenza
         COLLEGAMENTO2 * H2 = Coll2.Cerca((COLLEGAMENTO2 *)&C2.StazioneArrivo,2);
         while (H2) {
            // Seleziono le stazioni di cambio esclusa la precedente stazione stessa
            if(Fstaz[H2->StazioneArrivo].StazioneDiCambio() && 
               H2->StazioneArrivo != C2.StazionePartenza ){
               NumSecondeConnessioni ++;
               SumStazione ++;
               
               COLLEGAMENTO3 K3; K3.IdCluster = H2->IdCluster; K3.Stazione = H2->StazionePartenza;K3.Concorde = H2->Concorde; 
               K3 = * Coll3.Cerca(&K3,5); // Completo le informazioni


               WORD OrariPartenza = CollStaz.Cerca((COLLEGAMENTO1*)&(H2->StazionePartenza),4)->OrariPartenza;

               if(C2.StazionePartenza == 13){
                  TRACEVLONG(H2->StazionePartenza);
                  TRACEVLONG(H2->StazioneArrivo);
                  TRACEVLONG(H2->IdCluster);
                  TRACEVPOINTER(OrariPartenza);
                  TRACEVLONG(OrariArrivo & OrariPartenza);
               }
               
               if(OrariArrivo & OrariPartenza){
                  AncheCondStaz ++;
               }

               if(C2.OrariArrivo & H2->OrariPartenza){
                  NumSecondeConnessioniCondizionate ++;
                  SumStazioneC ++;
               }
               if(C3.OrariArrivo & K3.OrariPartenza){
                  NumSecondeConnessioniCondizionateCluster ++;
                  SumStazioneC2 ++;
                  if(OrariArrivo & OrariPartenza){
                     ClustEStaz++;
                  }
               }
            }
            H2 = Coll2.CercaNext();
         } /* endwhile */
//<<< if Fstaz C2.StazionePartenza .StazioneDiCambio   && 
      }
//<<< ORD_FORALL CollegamentiTraStazioniNelCluster,NumCst4  
   }
   Bprintf2("NumStazioni %i NumSecondeConnessioni %i NumSecondeConnessioniCondizionate %i NumSecondeConnessioniCondizionate da solo cluster %i ",
      NumStazioni ,NumSecondeConnessioni ,NumSecondeConnessioniCondizionate,NumSecondeConnessioniCondizionateCluster);
   
   Bprintf("Numero seconde connessioni/stazione: %i Condizionate: %i Condizionate nel solo cluster %i",
      NumSecondeConnessioni / NumStazioni , 
      NumSecondeConnessioniCondizionate / NumStazioni,
      NumSecondeConnessioniCondizionateCluster/ NumStazioni);
   Bprintf("Numero seconde connessioni/stazione: %i Condizionate a liv stazioni: %i Condizionate nel cluster + stazioni %i",
      NumSecondeConnessioni / NumStazioni , 
      AncheCondStaz/ NumStazioni,
      ClustEStaz/ NumStazioni);
   
   #endif
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
//<<< int  main int argc,char *argv    
}

