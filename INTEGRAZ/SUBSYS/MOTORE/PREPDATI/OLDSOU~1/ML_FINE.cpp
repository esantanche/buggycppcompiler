//----------------------------------------------------------------------------
// ML_FINE.CPP: Scarico dei dati per ottenere gli archivi del motore
//----------------------------------------------------------------------------
//
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4 // Mettere ad 1 per attivare il debug


#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "ML_IN.HPP"
#include "ML_WRK.HPP"
#include "seq_proc.hpp"
#include "mm_path.hpp"
#include "mm_combi.hpp"
#include "alfa0.hpp"
#include "mm_detta.hpp"
#include "ml_fine.hpp"

#define PGM      "ML_FINE"


//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"
   
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   SetStatoProcesso(PGM, SP_PARTITO);
   
   if(trchse > 1)trchse = 1; // Per evitare trace abissali involontari
   
   DosError(2); // Disabilita Popup di errore
   
   SetPriorita(); // Imposta la priorita'
   
   GESTIONE_ECCEZIONI_ON
   TryTime(0);
   
   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);
   
   
   PROFILER::Clear(FALSE); // Per aprire il grafo
   
   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   
   // APRO il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);
   
   if (CCR_ID::CcrId == NULL) {
      CCR_ID::CcrId = new CCR_ID(PATH_DATI);
   } /* endif */
   
   WORK Work;
   
   int Rc = Work.Body();
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return Rc;
//<<< int main int argc,char *argv    
};
//----------------------------------------------------------------------------
// WORK::Body
//----------------------------------------------------------------------------
int WORK::Body(){
   #undef TRCRTN
   #define TRCRTN "WORK::Body"
   
   ZeroFill(Header);
   
   AllPeriodicita.Get(PATH_OUT "PERIODIC.SV1"); // Gestisce file non esistente o vuoto
   
   Header.TotIdInterni = GRAFO::Gr().TotStazioniGrafo + 1;
   
   // Leggo i dati delle multistazioni
   GEST_MULTI_STA Gest(PATH_CVB,NULL,Staz);
   Gestore = &Gest;
   
   if(Staz.Dim() > MAX_STAZIONI) {
      Bprintf("Superato limite massimo stazioni: correggere dimensioni");
      return 9876;
   };
   
   
   int Rc = MakeClusterStazioni();
   if(Rc) return Rc;
   
   Rc = MakeStazioniCluster();
   if(Rc) return Rc;
   
   // ---------------------------------------------------------
   // Identifico le stazioni di cambio
   // ---------------------------------------------------------
   Header.NumNodiDiCambio = 0;
   TRACESTRING("Identificazione nodi di cambio");
   BS_STAZIONE_F   Stazn(PATH_OUT "MM_STAZN.DB");
   ORD_FORALL(Stazn,k){
      if(Stazn[k].StazioneDiCambio() && Stazn[k].Id){
         StazioniDiCambio.Set(Stazn[k].Id);
         // TRACEVLONG(Stazn[k].Id);
         Header.NumNodiDiCambio ++;
      }
   };
   
   // ---------------------------------------------------------
   // Scarico dei dati di periodicita'
   // ---------------------------------------------------------
   AllPeriodicita.Put(PATH_OUT "PERIODIC.SV1");
   FILE_RW Periodic(PATH_OUT "PERIODIC.DB");
   Periodic.SetSize(0);
   BUFR Buf;
   ORD_FORALL(AllPeriodicita,i)Buf.Store(&AllPeriodicita[i],sizeof(PERIODICITA));
   Periodic.Scrivi(Buf);
   Header.TotPeriodicita = AllPeriodicita.Dim();
   Bprintf("Numero di differenti periodicita':  %i",AllPeriodicita.Dim());
   
   // ---------------------------------------------------------
   // Gestione dell' Header
   // ---------------------------------------------------------
   FIX_COLLEGAMENTO_F CollegamentiOut(PATH_OUT "MM_COLL.DB");
   ZeroFill(Header.TimeStampMotore);
   // Lascio uno spazio extra per i collegamenti poiche' serve per aggiungere Origine e destinazione
   // come nodi di cambio:
   // - 2 Collegamenti (Origine e Dest) Verso tutti i nodi di cambio
   // - Un margine di 512
   Header.NumCollegamenti = CollegamentiOut.Dim() + 2 * Header.NumNodiDiCambio + 512;
   Header.NumStzCluster   = 0;
   ORD_FORALL(Staz_Clus,j){ // Il file contiene TUTTE le relazioni Stazione -> Cluster
      STAZN_CLUS & Scl = Staz_Clus.FixRec(j);
      if(StazioniDiCambio.Test(Scl.Stazione))Header.NumStzCluster ++;
   }
   Header.NumStzCluster   += 512;
   Header.TotalSize = (
      sizeof(Header)                                       +
      Header.TotIdInterni           * sizeof(ID)           +
      (Header.NumNodiDiCambio + 3)  * sizeof(ID)           +
      Header.NumCollegamenti        * sizeof(COLLEGAMENTO) +
      Header.NumStzCluster          * sizeof(STAZN_CLUS)   +
      (Header.NumClusters+1)        * sizeof(CLUSTER)      +
      (Header.NumNodiDiCambio + 3)  * sizeof(NODO_CAMBIO) + // 3: Nodo 0 non valorizzato + origine e destinazione
      Header.TotPeriodicita         * sizeof(PERIODICITA)  );
   Header.Trace("Dati dell' Header",1);
   
   FILE_RW Validita(PATH_OUT "VALIDITA.DB");
   Validita.Posiziona(sizeof( VALIDITA_ORARIO ));
   Validita.Scrivi(VRB(Header)); // Contemporaneamente cambia data ed ora di VALIDITA.DB
   if (Header.NumNodiDiCambio >= MAXNODI_CAMBIO ) {
      Bprintf("ERRORE: Il numero dei nodi di cambio (%i) eccede il limite strutturale di (%i)",Header.NumNodiDiCambio, MAXNODI_CAMBIO );
      Bprintf("Il file VALIDITA.DB viene cancellato per impedire la distribuzione dei dati");
      Validita.SetSize(0);
   } /* endif */
   
   // ---------------------------------------------------------
   // Statistiche
   // ---------------------------------------------------------
   Bprintf2("======================== Statistiche Orario Generato ====================");
   Bprintf2("Archivio Principale Treni (CLUSTAZ.EXT)");
   Bprintf2("   Numero dei Mezzi Virtuali %i (Comprensivo dello split di Periodicit…)", NumeroCLUSTRENO );
   Bprintf2("         * %i Bytes ognuno = %i Bytes",
      (sizeof(CLUSTRENO)-sizeof(INFOSTAZ)),
      NumeroCLUSTRENO * (sizeof(CLUSTRENO)-sizeof(INFOSTAZ))
   );
   Bprintf2("   Numero dei Cluster %i * 2 * Size Testata %i = %i Bytes",
      Header.NumClusters ,
      sizeof(CLUSTERS_STAZ_EXT) - sizeof(CLUSSTAZ),
      Header.NumClusters *(sizeof(CLUSTERS_STAZ_EXT) - sizeof(CLUSSTAZ))
   );
   Bprintf2("   Numero dei dati dei Nodi nella testata %i * %i Bytes = %i Bytes",
      NumeroCLUSSTAZ ,
      sizeof(CLUSSTAZ),
      NumeroCLUSSTAZ * sizeof(CLUSSTAZ)
   );
   Bprintf2("   Numero Fermate e Transiti %i * %i Bytes = %i Bytes",
      NumeroINFOSTAZ,
      sizeof(INFOSTAZ),
      sizeof(INFOSTAZ)  * NumeroINFOSTAZ
   );
   Bprintf2("       Di cui attive %i e dummy %i" ,
      NumStazPiene, NumeroINFOSTAZ - NumStazPiene
   );
   Bprintf2("Dati non strutturati degli identificativi treni, classifiche e servizi: %i Bytes",ExtraBytes);
   Bprintf2("Totale : %i",
      sizeof(INFOSTAZ)  * NumeroINFOSTAZ +
      Header.NumClusters *(sizeof(CLUSTERS_STAZ_EXT) - sizeof(CLUSSTAZ))+
      NumeroCLUSTRENO * (sizeof(CLUSTRENO)-sizeof(INFOSTAZ))+
      NumeroCLUSSTAZ * sizeof(CLUSSTAZ) +
      ExtraBytes
   );
   Bprintf2("Archivio Periodicit… (PERIODIC.DB) : %i Records da %i Bytes = %i Bytes",
      Header.TotPeriodicita ,
      sizeof(PERIODICITA)   ,
      Header.TotPeriodicita * sizeof(PERIODICITA)
   );
   Bprintf2("Archivio Esteso stazioni (MM_STCLU.DB): %i Records da %i Bytes = %i Bytes",
      Stazn.Dim(), sizeof(STAZIONE_F) ,
      Stazn.Dim() * sizeof(STAZIONE_F)
   );
   Bprintf2("Archivio Collegamenti (MM_COLL.DB): %i Records da %i Bytes = %i Bytes",
      Header.NumCollegamenti ,sizeof(COLLEGAMENTO_F),
      Header.NumCollegamenti *sizeof(COLLEGAMENTO_F)
   );
   
   return 0;
   
//<<< int WORK::Body   
}


//----------------------------------------------------------------------------
// WORK::MakeClusterStazioni
//----------------------------------------------------------------------------
// Alla fine di questa routine:
// - Ho generato CLUSTAZ.EXT e CLUSTAZ.DB
// Sia per i cluster normali che per le multistazione
//----------------------------------------------------------------------------
int WORK::MakeClusterStazioni(){
   #undef TRCRTN
   #define TRCRTN "WORK::MakeClusterStazioni"
   
   OffSet=0; // Riazzero
   
   // Scansione di tutti i cluster
   Bprintf("Scansione dei clusters e scarico dati");
   
   ORD_FORALL(Clust,k0){
      if(k0 == 0)continue; // Il cluster 0 e' fittizio
      TryTime(k0);
      CLUSTER_MV & Clu = Clust[k0];
      
      TRACESTRING("Caricamento dei dati del cluster: " VRS(Clu.Id));
      
      // Inizializzazione
      OutC.Clear()  ; OutD.Clear()  ;
      ExtraC.Clear(); ExtraD.Clear();
      ServC.Clear() ; ServD.Clear() ;
      NumeroCLUSTERS_STAZ ++;
      MV_Estesi  = -1 ; // Reset per dire che non ho dati estesi validi
      
      // Anticipo il caricamento della testata sui buffers, ma i dati veri li potro'
      // scrivere solo alla fine del caricamento del cluster
      OutC.Store(&Clusta_Ext_Rec1,sizeof(Clusta_Ext_Rec1) - sizeof(CLUSSTAZ));  // Testata (dati non significativi)
      OutD.Store(&Clusta_Ext_Rec1,sizeof(Clusta_Ext_Rec1) - sizeof(CLUSSTAZ));  // Testata (dati non significativi)
      
      // Carico le stazioni del cluster
      int Rc = CaricaStazioniDelcluster(Clu);
      if(Rc) return Rc;
      
      // Carico i mezzi virtuali del cluster
      Rc = CaricaMezziVirtualiDelcluster(Clu);
      if(Rc) return Rc;
      
      // Scrivo i dati del Cluster
      Rc = ScrivoDatiCluster(Clu);
      if(Rc) return Rc;
      
//<<< ORD_FORALL Clust,k0  
   };
   
   Bprintf("MaxDelta = %i, MinDelta = %i",MaxDelta,MinDelta);
   
   Bprintf("Scansione dei clusters MULTISTAZIONE e scarico dati");
   OffSetMultiStazione = NumeroCLUSTERS_STAZ + 1;
   TRACEVLONG(OffSetMultiStazione);
   
   int Rc = CaricaEScriviDatiMultistazioni();
   if(Rc) return Rc;
   
   Clusta_Db.Flush();
   
   // ...............................................
   // Controlli finali
   // ...............................................
   TotalSizeExpected =
   NumeroCLUSTERS_STAZ * 2 * (sizeof( CLUSTERS_STAZ_EXT ) - sizeof(CLUSSTAZ)) - // Doppia testata
   NumeroCLUSTER_MULTI * (sizeof( CLUSTERS_STAZ_EXT ) - sizeof(CLUSSTAZ)) + // Tranne che per i multistazione
   sizeof(CLUSSTAZ)  * NumeroCLUSSTAZ +                       // Dati dei nodi in testa ad ogni cluster
   (sizeof(CLUSTRENO)- sizeof(INFOSTAZ)) * NumeroCLUSTRENO +  // Dati generali del treno
   sizeof(INFOSTAZ)  * NumeroINFOSTAZ +                       // Dati di dettaglio del treno
   sizeof(COLLEG_URBANO) * NumeroURBANI+                      // Dati delle matrici di interscambio
   4 * NumeroACRONIMI +                                       // Dati degli acronimi
   ExtraBytes ;                                               // Dati degli identificativi treni e servizi
   
   // La dimensione del file dovrebbe essere data dalla somma dei contributi: Ora scrivo e controllo
   TRACEVLONGL(NumeroCLUSTERS_STAZ  ,1);
   TRACEVLONGL(NumeroCLUSSTAZ       ,1);
   TRACEVLONGL(NumeroCLUSTRENO      ,1);
   TRACEVLONGL(NumeroINFOSTAZ       ,1);
   TRACEVLONGL(NumeroURBANI         ,1);
   TRACEVLONGL(NumeroACRONIMI       ,1);
   TRACEVLONGL(ExtraBytes           ,1);
   TRACEVLONGL(Clusta_Db.FileSize() ,1);
   TRACEVLONGL(Clusta_Db.Dim()      ,1);
   TRACEVLONGL(Clusta_Ext.FileSize(),1);
   TRACEVLONGL(TotalSizeExpected    ,1);
   TRACEVLONGL(NumFermServizio      ,1);
   TRACEVLONGL(NumStazPiene         ,1);
   TRACEVLONGL(NumTransiti          ,1);
   TRACEVLONGL(NumTransitiDaIgnorare,1);
   TRACEVLONGL((NumeroINFOSTAZ - NumStazPiene) ,1);
   ERRINT("Bytes nei nodi delle TESTATE del .EXT:",sizeof(CLUSSTAZ)  * NumeroCLUSSTAZ);
   ERRINT("Bytes nei dati generali dei treni:",(sizeof(CLUSTRENO)- sizeof(INFOSTAZ)) * NumeroCLUSTRENO);
   ERRINT("Bytes nel dettaglio fermate:",sizeof(INFOSTAZ)  * NumeroINFOSTAZ);
   ERRINT("  di cui per stazioni vuote:",sizeof(INFOSTAZ)  * (NumeroINFOSTAZ - NumStazPiene) );
   ERRINT("Bytes nelle matrici interscambio:",sizeof(COLLEG_URBANO) * NumeroURBANI+4*NumeroACRONIMI);
   ERRINT("Bytes negli identificativi treni e nei servizi:",ExtraBytes);
   if(TotalSizeExpected != Clusta_Ext.FileSize()){
      ERRSTRING("Errore: Clusta_Ext.FileSize() e TotalSizeExpected dovrebbero coincidere");
      puts("Errore: Clusta_Ext.FileSize() e TotalSizeExpected dovrebbero coincidere");
      BEEP;
      return 99;
   } /* endif */
   if(Clusta_Db.Dim() != NumeroCLUSTERS_STAZ){
      ERRSTRING("Errore: Clusta_Db.Dim() e NumeroCLUSTERS_STAZ dovrebbero coincidere");
      puts("Errore: Clusta_Db.Dim() e NumeroCLUSTERS_STAZ dovrebbero coincidere");
      BEEP;
      return 99;
   } /* endif */
   Bprintf("Dati estesi: Generati %i servizi e %i classifiche", NumSrv , NumCla );
   if(NumErrSrv)Bprintf("Vi sono stati %i errori caricando i servizi", NumErrSrv );
   return 0;
//<<< int WORK::MakeClusterStazioni   
};

//----------------------------------------------------------------------------
// WORK::ScrivoDatiCluster
//----------------------------------------------------------------------------
// Scrive CLUSTAZ.EXT e CLUSTAZ.DB
//----------------------------------------------------------------------------
int  WORK::ScrivoDatiCluster(CLUSTER_MV & Clu){
   #undef TRCRTN
   #define TRCRTN "WORK::ScrivoDatiCluster"
   Clusta_Db_Rec.IdCluster         =  Chk(Clu.Id ,11)         ;
   Clusta_Db_Rec.TipoCluster       =  Chk(Clu.TipoCluster,4)  ;
   Clusta_Db_Rec.OffSetBytes       =  Chk(OffSet,23)               ;
   Clusta_Db_Rec.Id1               =  Chk(Clu.Id1,13)              ;
   Clusta_Db_Rec.Id2               =  Chk(Clu.Id2,13)              ;
   Clusta_Db_Rec.DimDatiC          =  Chk(
      sizeof(CLUSTERS_STAZ_EXT) - sizeof(CLUSSTAZ) +                      // Testata
      (sizeof(CLUSSTAZ) * Clu.NumeroStazioni)      +                      // Dati delle stazioni
      ((sizeof(CLUSTRENO) + (sizeof(INFOSTAZ) * (Clu.NumeroStazioni-1)))  // Dati dei treni
         *     (Clu.NumeroMezziVirtualiC + Clu.ExtraMezziVirtualiC))
      + ExtraC.Dim() + ServC.Dim()
     ,19);
   Clusta_Db_Rec.DimDatiD          =  Chk(
      sizeof(CLUSTERS_STAZ_EXT) - sizeof(CLUSSTAZ) +                      // Testata
      (sizeof(CLUSSTAZ) * Clu.NumeroStazioni)      +                      // Dati delle stazioni
      ((sizeof(CLUSTRENO) + (sizeof(INFOSTAZ) * (Clu.NumeroStazioni-1)))  // Dati dei treni
         *     (Clu.NumeroMezziVirtualiD + Clu.ExtraMezziVirtualiD))
      + ExtraD.Dim() + ServD.Dim()
     ,19);
   
   TRACEVLONG(Clusta_Db_Rec.IdCluster        );
   TRACEVLONG(Clusta_Db_Rec.TipoCluster      );
   TRACEVLONG(Clusta_Db_Rec.Id1              );
   TRACEVLONG(Clusta_Db_Rec.Id2              );
   TRACEVLONG(Clusta_Db_Rec.OffSetBytes      );
   TRACEVLONG(Clusta_Db_Rec.DimDatiC         );
   TRACEVLONG(Clusta_Db_Rec.DimDatiD         );
   TRACEVLONG(ExtraC.Dim()                   );
   TRACEVLONG(ExtraD.Dim()                   );
   TRACEVLONG(ServC.Dim()                    );
   TRACEVLONG(ServD.Dim()                    );
   TRACEVLONG(Clu.NumeroStazioni             );
   TRACEVLONG(Clu.NumeroMezziVirtualiC       );
   TRACEVLONG(Clu.ExtraMezziVirtualiC        );
   TRACEVLONG(Clu.NumeroMezziVirtualiD       );
   TRACEVLONG(Clu.ExtraMezziVirtualiD        );
   
   // Aggiorno i due records di testata e scrivo i relativi files.
   Clusta_Db.AddRecordToEnd(VRB(Clusta_Db_Rec));
   Header.NumClusters ++;
   Clusta_Ext_Rec1.DimDati         =  Clusta_Db_Rec.DimDatiC;
   Clusta_Ext_Rec1.IdCluster       =  Clu.Id                ;
   Clusta_Ext_Rec1.TotStazioni     =  Clu.NumeroStazioni    ;
   Clusta_Ext_Rec1.NumeroNodi      =  Clu.NumeroNodi        ;
   Clusta_Ext_Rec1.NumeroTreni     =  Clu.NumeroMezziVirtualiC + Clu.ExtraMezziVirtualiC;
   Clusta_Ext_Rec1.OffsetIdTreni   =  OutC.Dim()            ;
   OutC += ExtraC;
   Clusta_Ext_Rec1.OffsetSrvTreni  =  OutC.Dim();
   OutC += ServC ;
   memmove(&OutC[0], &Clusta_Ext_Rec1 , sizeof(Clusta_Ext_Rec1) - sizeof(CLUSSTAZ));  // Copio testata (avevo gia' riservato l' area)
   
   Clusta_Ext_Rec1.DimDati         =  Clusta_Db_Rec.DimDatiD;
   Clusta_Ext_Rec1.NumeroTreni     =  Clu.NumeroMezziVirtualiD + Clu.ExtraMezziVirtualiD;
   Clusta_Ext_Rec1.OffsetIdTreni   =  OutD.Dim();
   OutD += ExtraD;
   Clusta_Ext_Rec1.OffsetSrvTreni  =  OutD.Dim();
   OutD += ServD ;
   memmove(&OutD[0], &Clusta_Ext_Rec1 , sizeof(Clusta_Ext_Rec1) - sizeof(CLUSSTAZ));  // Copio testata (avevo gia' riservato l' area)
   
   ExtraBytes += ExtraC.Dim() + ExtraD.Dim();
   ExtraBytes += ServC.Dim()  + ServD.Dim() ;
   OffSet += Clusta_Db_Rec.DimDatiC;
   OffSet += Clusta_Db_Rec.DimDatiD;
   
   
   // Controllo generale dimensione delle aree
   if (OutC.Dim() != Clusta_Db_Rec.DimDatiC) {
      Bprintf("Errata dimensione dei dati: %i invece di %i",OutC.Dim(),Clusta_Db_Rec.DimDatiC);
      TRACEVLONGL(OutC.Dim(),1);
      TRACEVLONGL(Clusta_Db_Rec.DimDatiC,1);
      TRACESTRING("Composizione dei dati:");
      TRACEVLONG(sizeof(CLUSTERS_STAZ_EXT) - sizeof(CLUSSTAZ));
      TRACEVLONG(sizeof(CLUSSTAZ) * Clu.NumeroStazioni);
      TRACEVLONG((sizeof(CLUSTRENO)) + (sizeof(INFOSTAZ) * (Clu.NumeroStazioni-1)));
      TRACEVLONG((sizeof(CLUSTRENO)) + (sizeof(INFOSTAZ) * (Clu.NumeroStazioni-1)) * (Clu.NumeroMezziVirtualiC + Clu.ExtraMezziVirtualiC));
      TRACEVLONG(ExtraC.Dim());
      TRACEVLONG(ServC.Dim());
      return 31;
   } /* endif */
   if (OutD.Dim() != Clusta_Db_Rec.DimDatiD) {
      Bprintf("Errata dimensione dei dati: %i invece di %i",OutD.Dim(),Clusta_Db_Rec.DimDatiD);
      TRACEVLONGL(OutD.Dim(),1);
      TRACEVLONGL(Clusta_Db_Rec.DimDatiD,1);
      TRACESTRING("Composizione dei dati:");
      TRACEVLONG(sizeof(CLUSTERS_STAZ_EXT) - sizeof(CLUSSTAZ));
      TRACEVLONG(sizeof(CLUSSTAZ) * Clu.NumeroStazioni);
      TRACEVLONG((sizeof(CLUSTRENO)) + (sizeof(INFOSTAZ) * (Clu.NumeroStazioni-1)));
      TRACEVLONG((sizeof(CLUSTRENO)) + (sizeof(INFOSTAZ) * (Clu.NumeroStazioni-1)) * (Clu.NumeroMezziVirtualiC + Clu.ExtraMezziVirtualiC));
      TRACEVLONG(ExtraD.Dim());
      TRACEVLONG(ServD.Dim());
      return 32;
   } /* endif */
   Clusta_Ext.Scrivi(OutC);
   Clusta_Ext.Scrivi(OutD);
   return 0;
//<<< int  WORK::ScrivoDatiCluster CLUSTER_MV & Clu  
};
//----------------------------------------------------------------------------
// WORK::CaricaStazioniDelcluster
//----------------------------------------------------------------------------
// Alla fine di questa routine:
// - Ho caricato IDToProg che trascodifica da ID delle stazioni a progressivo nell' ambito del cluster
// - Ho identificato i due nodi dominanti
// - Ho caricato Clusta_Ext_Rec2 che e' un' array con i dati delle
//   stazioni del cluster
//----------------------------------------------------------------------------
int WORK::CaricaStazioniDelcluster( CLUSTER_MV & Clu){
   #undef TRCRTN
   #define TRCRTN "WORK::CaricaStazioniDelcluster"
   
   // Init
   P1 = P2=-1;
   IDToProg.SetDimAndInitialize(MAX_STAZIONI,0xff); // Uso il valore 255: 0 e' valido
   ZeroFill(Clusta_Ext_Rec2);  // Azzero i dati delle stazioni nel cluster
   int Num = 0;
   
   // .......................................................................
   // Scandisco le STAZIONI del cluster e per ogni stazione scarico un record
   // di tipo CLUSSTAZ
   // I dati vanno su entrambi i buffers
   // Le stazioni sono scandite in ordine di progressivo
   // .......................................................................
   // Questo ciclo identifica le stazioni del cluster
   for (ClusStaz.Seek(Clu.Id);
      &ClusStaz.RecordCorrente() != NULL &&
      ClusStaz.RecordCorrente().IdCluster == Clu.Id;
      ClusStaz.Next()
   ){
      
      CLUSTER_STAZIONE & Cs = ClusStaz.RecordCorrente();
      TRACEVLONG(Cs.IdStazione);
      
      if(Cs.IdStazione == Clu.Id1)P1 = Cs.Prog;
      if(Cs.IdStazione == Clu.Id2)P2 = Cs.Prog;
      
      // Conversione da Id Stazione a progressivo
      // Uso Prog, che ha il significato di "indice della stazione nel cluster"
      IDToProg[Cs.IdStazione] = Cs.Prog;
      if(Cs.Prog >= Clu.NumeroStazioni || Cs.Prog >= MAX_STAZIONI_IN_CLUSTER){
         Bprintf("Cluster %i Errore su stazione %i : Progressivo (%i) fuori range",Cs.IdCluster,Cs.IdStazione,Cs.Prog);
         return 234;
      }
      
      // Carico i dati riassuntivi della stazione nel cluster
      Clusta_Ext_Rec2[Cs.Prog].Fermata              =   (Cs.NumMzvFerm > 0) ? 1 : 0;
      Clusta_Ext_Rec2[Cs.Prog].ClasseCoincidenza    =   Chk(Staz[Cs.IdStazione].ClasseCoincidenza,2);
      Clusta_Ext_Rec2[Cs.Prog].IdNodo               =   Chk(Cs.IdStazione,13);
      Clusta_Ext_Rec2[Cs.Prog].MediaKm              =   ChkS(Cs.DistanzaMediana,13);
      Clusta_Ext_Rec2[Cs.Prog].Gruppi               =   Cs.Gruppo;
      CLUSSTAZ & Q = Clusta_Ext_Rec2[Cs.Prog];
      IFTRC(Bprintf3(" Stazione Prog = %i Id = %i Fermata %i ClasseCoincidenza %i Distanza %i", Cs.Prog, Q.IdNodo, Q.Fermata, Q.ClasseCoincidenza, Q.MediaKm );)
      Num ++;
//<<< for  ClusStaz.Seek Clu.Id ;
   };
   
   if(Num != Clu.NumeroStazioni){
      Bprintf("Cluster %i Errore : Ha %i stazioni e non %i\n",Clu.Id,Num,Clu.NumeroStazioni);
      return 234;
   }
   if(P1 < 0){
      Bprintf("Cluster %i Errore : non trovato primo nodo dominante\n",Clu.Id);
      return 234;
   }
   if(P2 < 0){
      Bprintf("Cluster %i Errore : non trovato secondo nodo dominante\n",Clu.Id);
      return 234;
   }
   OutC.Store(&Clusta_Ext_Rec2,sizeof(CLUSSTAZ)*Num);  // Dati delle stazioni
   OutD.Store(&Clusta_Ext_Rec2,sizeof(CLUSSTAZ)*Num);  // Dati delle stazioni
   NumeroCLUSSTAZ += Num*2;
   return 0;
//<<< int WORK::CaricaStazioniDelcluster  CLUSTER_MV & Clu  
};

//----------------------------------------------------------------------------
// WORK::CaricaMezziVirtualiDelcluster
//----------------------------------------------------------------------------
// Alla fine di questa routine:
// - Ho caricato i dati dei mezzi virtuali nei buffer
// - Per ogni mezzo virtuale ho caricato il dettaglio delle stazioni
// - Ho sortato l' area per cui i mezzi virtuali sono ordinati per ID e progressivo
//----------------------------------------------------------------------------
int  WORK::CaricaMezziVirtualiDelcluster( CLUSTER_MV & Clu){
   #undef TRCRTN
   #define TRCRTN "WORK::CaricaMezziVirtualiDelcluster"
   
   // A questo punto so quanto spazio occupano i dati di testata e
   // delle stazioni: Registro i puntatori all' inizio dell' area dei mezzi virtuali
   // che utilizzer• nel successivo Sort
   int DimC_Treni = OutC.Dim();
   int DimD_Treni = OutD.Dim();
   
   int IdxVirtuale = 0;
   
   // Scandisco i mezzi virtuali del cluster e per ogni MV scarico un record
   // di tipo CLUSTRENO, le informazioni di fermata ed i dati estesi
   
   for (IxClusMezv.Seek(Clu.Id);
      &IxClusMezv.RecordCorrente() != NULL && IxClusMezv.RecordCorrente().IdCluster == Clu.Id;
      IxClusMezv.Next()
   ) {
      if (++IdxVirtuale >  MAX_VIRTUALI_IN_CLUSTER) {
         Bprintf("Errore: ecceduto (causa split periodicita') il limite MV sul cluster %i",Clu.Id);
         BEEP;
         return 4;
      } /* endif */
      
      WORD Mv = IxClusMezv.RecordCorrente().MezzoVirtuale;
      TRACEVLONG(IxClusMezv.RecordCorrente().MezzoVirtuale);
      
      if(!TabTv2.Seek(Mv,0)){
         Bprintf("Errore: non trovato su TabTv2 il mezzo virtuale %i (Cluster %i)",Mv,Clu.Id);
         BEEP;
         return 4;
      };
      // Debbo ignorare i mezzi virtuali con meno di due fermate valide
      if(TabTv2.RecordCorrente().NumeroFermateValide < 2){
         continue;
      };
      while (&TabTv2.RecordCorrente() && TabTv2.RecordCorrente().MezzoVirtuale == Mv) {
         int ProgressivoPeriodicita = TabTv2.RecordCorrente().ProgressivoPeriodicita;
         // TRACEVLONG(TabTv2.RecordCorrente().ProgressivoPeriodicita);
         
         if(!ClusMezv.Seek(Mv)){
            Bprintf("Errore: non trovato su ClusMezv il mezzo virtuale %i (Cluster %i)",Mv,Clu.Id);
            BEEP;
            return 4;
         };
         if(!TabFv2.Seek(Mv,ProgressivoPeriodicita)){
            Bprintf("Errore: non trovato su TabFv2 il mezzo virtuale %i ProgPer %i (Cluster %i)",Mv,ProgressivoPeriodicita,Clu.Id);
            BEEP;
            return 4;
         };
         
         // I mezzi virtuali con verso concorde debbono essere messi su OutC
         // I mezzi virtuali con verso discorde debbono essere messi su OutD
         // Analogamente per identificatori treni e servizi
         BUFR & Out   = ClusMezv.RecordCorrente().VersoConcorde ? OutC   : OutD  ;
         BUFR & Extra = ClusMezv.RecordCorrente().VersoConcorde ? ExtraC : ExtraD;
         BUFR & Serv  = ClusMezv.RecordCorrente().VersoConcorde ? ServC  : ServD ;
         
         
         // Predispongo i dati generali del mezzo virtuale
         // NB : Uso un solo Bit per i progressivi periodicit…
         Clusta_Ext_Rec3.IdMezv                 = Chk(ClusMezv.RecordCorrente().MezzoVirtuale,17);
         Clusta_Ext_Rec3.PeriodicitaUniforme    = !TabTv2.RecordCorrente().PeriodicitaDisuniformi;
         Clusta_Ext_Rec3.ProgressivoPeriodicita = ProgressivoPeriodicita ? 1:0              ;
         Clusta_Ext_Rec3.IdxPeriodicita         = AllPeriodicita.Indice(TabTv2.RecordCorrente().PeriodicitaMV );
         Clusta_Ext_Rec3.InfoUniforme           = TabTv2.RecordCorrente().ServiziUniformi ;
         Clusta_Ext_Rec3.Gruppo                 = ClusMezv.RecordCorrente().Gruppo          ;
         
         // Carico i dati estesi del mezzo virtuale
         int Rc = CaricaDatiEstesiMV(Extra,Serv);
         if(Rc) return Rc;
         
         // Carico i dati di fermata del mezzo virtuale
         Rc = CaricaDatiDiFermataMV(Clu, ProgressivoPeriodicita);
         if(Rc) return Rc;
         
         // Aggiungo i dati di fermata all' area di Output
         Out.Store(&Clusta_Ext_Rec3,sizeof(Clusta_Ext_Rec3) - sizeof(INFOSTAZ));
         NumeroCLUSTRENO ++;
         Out.Store(&Clusta_Ext_Rec4,sizeof(INFOSTAZ)* Clu.NumeroStazioni);
         NumeroINFOSTAZ +=Clu.NumeroStazioni;
         TabTv2.Next();
//<<< while  &TabTv2.RecordCorrente   && TabTv2.RecordCorrente  .MezzoVirtuale == Mv   
      }
//<<< for  IxClusMezv.Seek Clu.Id ;
   }
   
   // Sort dell' area dei treni
   TRACESTRING("Sort Dei Treni concordi per il cluster Nø "+STRINGA(Clu.Id)+ " Numero treni:"+STRINGA(Clu.NumeroMezziVirtualiC + Clu.ExtraMezziVirtualiC));
   if(Clu.NumeroMezziVirtualiC > 0){
      qsort(& OutC[DimC_Treni],
         (Clu.NumeroMezziVirtualiC + Clu.ExtraMezziVirtualiC)
        ,(sizeof(CLUSTRENO) + (sizeof(INFOSTAZ) * (Clu.NumeroStazioni-1)))
        ,Sort_Treni);
   }
   
   TRACESTRING("Sort Dei Treni discordi per il cluster Nø "+STRINGA(Clu.Id)+ " Numero treni:"+STRINGA(Clu.NumeroMezziVirtualiD + Clu.ExtraMezziVirtualiD));
   if(Clu.NumeroMezziVirtualiD > 0){
      qsort(& OutD[DimD_Treni],
         (Clu.NumeroMezziVirtualiD + Clu.ExtraMezziVirtualiD)
        ,(sizeof(CLUSTRENO) + (sizeof(INFOSTAZ) * (Clu.NumeroStazioni-1)))
        ,Sort_Treni);
   }
   return 0;
//<<< int  WORK::CaricaMezziVirtualiDelcluster  CLUSTER_MV & Clu  
}
//----------------------------------------------------------------------------
// WORK::CaricaDatiEstesiMV
//----------------------------------------------------------------------------
// Gestisco i dati estesi del mezzo virtuale
// Alla fine di questa routine
// - Ho impostato i flags che dicono se e dove ho caricato i dati estesi
//   del mezzo virtuale
// - Ho impostato i puntamenti ai dati estesi nei dati del mezzo virtuale
// - Ho caricato i dati estesi nei due buffers
// Si noti che utilizzo GLI STESSI dati estesi per le differenti periodicit…
// dei mezzi virtuali
//----------------------------------------------------------------------------
int WORK::CaricaDatiEstesiMV( BUFR & Extra, BUFR & Serv ){
   #undef TRCRTN
   #define TRCRTN "WORK::CaricaDatiEstesiMV"
   
   extern DWORD MaskPrenotazione; // Servizi di prenotazione                
   extern DWORD MaskTrasporto   ; // Servizi di trasporto                   
   extern DWORD MaskGenerici    ; // Servizi generici                       

   ZeroFill( Clusta_Ext_Rec3.Viaggiante);
   
   int NumMezziComponenti = TabTv2.RecordCorrente().NumMezziComponenti ;
   
   // ......................................................................
   // Vedo se sia possibile riutilizzare i dati di un precedente progressivo
   // ......................................................................
   if (MV_Estesi == TabTv2.RecordCorrente().MezzoVirtuale) { // Riutilizzo dati: e' sempre lo stesso MV
      TRACESTRING("Riutilizzo dati estesi del virtuale "+STRINGA(MV_Estesi)+ " Progressivo "+STRINGA(TabTv2.RecordCorrente().ProgressivoPeriodicita));
      assert((TabTv2.RecordCorrente().NumMezziComponenti > 1) == (MV_Estesi_IdTreni >= 0));
      if (MV_Estesi_IdTreni >= 0) {
         Clusta_Ext_Rec3.Viaggiante.Multipli.Test       = 0;
         Clusta_Ext_Rec3.Viaggiante.Multipli.Numero     = NumMezziComponenti;
         Clusta_Ext_Rec3.Viaggiante.Multipli.OffsetDati = MV_Estesi_IdTreni ;
      } else {
         memmove(Clusta_Ext_Rec3.Viaggiante.IdMezzoViaggiante,TabTv2.RecordCorrente().Mv[0].KeyTreno,sizeof(ID_VIAG));
      } /* endif */
      if (MV_Estesi_Servizi > 0) {
         Clusta_Ext_Rec3.OffsetServizi = MV_Estesi_Servizi;
         Clusta_Ext_Rec3.InfoUniforme  = MV_Estesi_LastInfo; // Deve avere lo stesso valore !
      } /* endif */
   } else { // I dati vanno caricati
      
      
      MV_Estesi = TabTv2.RecordCorrente().MezzoVirtuale ;
      MV_Estesi_IdTreni = -1 ; // Non valido
      MV_Estesi_Servizi = -1 ; // Non valido
      TRACESTRING("Carico dati estesi del virtuale "+STRINGA(MV_Estesi)+ " Progressivo "+STRINGA(TabTv2.RecordCorrente().ProgressivoPeriodicita) );
      TRACESTRING("Viaggiante[0]= "+STRINGA(TabTv2.RecordCorrente().Mv[0].IdentTreno)+ " Servizi Uniformi: "+ TabTv2.RecordCorrente().ServiziUniformi.Decodifica());
      
      // Gli id dei mezzi viaggianti vanno caricati solo se ho pi— di un mezzo componente
      if (TabTv2.RecordCorrente().NumMezziComponenti == 1) {
         memmove(Clusta_Ext_Rec3.Viaggiante.IdMezzoViaggiante,TabTv2.RecordCorrente().Mv[0].KeyTreno,sizeof(ID_VIAG));
      } else {
         Clusta_Ext_Rec3.Viaggiante.Multipli.Test       = 0;
         Clusta_Ext_Rec3.Viaggiante.Multipli.Numero     = NumMezziComponenti;
         Clusta_Ext_Rec3.Viaggiante.Multipli.OffsetDati = Extra.Dim();
         MV_Estesi_IdTreni = Clusta_Ext_Rec3.Viaggiante.Multipli.OffsetDati ;
         STRINGA Msg;
         for (int ii=0;ii < NumMezziComponenti ;ii ++ ) {
            Extra.Store( TabTv2.RecordCorrente().Mv[ii].KeyTreno,sizeof(ID_VIAG));
            Msg += St(TabTv2.RecordCorrente().Mv[ii].KeyTreno);
            Msg += ",";
         } /* endfor */
         TRACESTRING("Caricati gli Id di " +STRINGA(TabTv2.RecordCorrente().NumMezziComponenti) +" Mezzi viaggianti: " +Msg);
      } /* endif */
      
      Clusta_Ext_Rec3.OffsetServizi = Serv.Dim();
      SRV_HDR SrvHdr;
      ZeroFill(SrvHdr)
      
      // Preimposto le classifiche in modo uniforme
      BYTE Classifiche[MVG];
      for (int i = 0 ; i < NumMezziComponenti ; i++ ) {
         Classifiche[i] = Clusta_Ext_Rec3.InfoUniforme.TipoMezzo;
      } /* endfor */
      TRACESTRING("Classifica Uniforme: "+ STRINGA(Clusta_Ext_Rec3.InfoUniforme.TipoMezzo) +" " + MM_INFO::DecodTipoMezzo(Clusta_Ext_Rec3.InfoUniforme.TipoMezzo));
      
      ARRAY_DINAMICA<SERV>  WrkSrv ; // Area di appoggio Servizi
      ARRAY_DINAMICA<INFO_AUX> Wrk ; // Area di appoggio DATI ORIGINALI
      BUFR                    Appo ; // Vi metto i dati temporaneamente
      
      // ...........................
      // Carico l' area di appoggio
      // ...........................
      for ( DatiAusiliari.Seek(MV_Estesi);
         &DatiAusiliari.RecordCorrente() &&
         DatiAusiliari.RecordCorrente().MezzoVirtuale == MV_Estesi ;
         DatiAusiliari.Next()
      ){
         INFO_AUX & Aux = DatiAusiliari.RecordCorrente();
         if( Aux.Tipo == AUX_CLASSIFICA || Aux.Tipo == AUX_SERVIZIO  ) Wrk += Aux;
      } /* endfor */
      
      // ...........................
      // Carico i dati delle classifiche
      // ...........................
      BOOL HoClassifiche = FALSE;
      FORALL(Wrk,w0){
         INFO_AUX & InfoAux = Wrk[w0];
         if (InfoAux.Tipo != AUX_CLASSIFICA)continue;
         assert(InfoAux.Id < 32);
         assert(InfoAux.DiMvg != 0);
         assert(InfoAux.Mvg   != 0);
         for (int i = 0 ; i < NumMezziComponenti ; i++ ) {
            if((1 << i) & InfoAux.Mvg){
               Classifiche[i] = InfoAux.Id ;
               if(Clusta_Ext_Rec3.InfoUniforme.TipoMezzo != InfoAux.Id ){
                  HoClassifiche = TRUE;
                  TRACESTRING("Impostata classifica " +STRINGA(Classifiche[i]) +" " +MM_INFO::DecodTipoMezzo(Classifiche[i]) +" per il mezzo componente Nø "+STRINGA(i));
                  NumCla ++ ;
               }
            }
         } /* endfor */
         InfoAux.Tipo = AUX_TAPPO  ; // Per non riutilizzare il dato
      };
      if(HoClassifiche){
         SrvHdr.NumClassifiche = NumMezziComponenti ;
         Appo.Store(&Classifiche, TabTv2.RecordCorrente().NumMezziComponenti );
         Clusta_Ext_Rec3.InfoUniforme.ClassificaDisUniforme = TRUE;
      }
      
      // .....................................
      // Carico i dati dei servizi di fermata
      // .....................................
      FORALL(Wrk,w1){
         INFO_AUX & InfoAux = Wrk[w1];
         if(InfoAux.Tipo != AUX_SERVIZIO) continue;
         if(!InfoAux.DiFermata) continue;
         FERMATE_VIRT & Fv = TabFv.CercaCcrDiMv(InfoAux.MezzoVirtuale, InfoAux.Da);
         if(&Fv){
            SRV_FERM Tmp;
            Tmp.ProgFermata = Fv.Progressivo2 ;
            Appo.Store(&Tmp, sizeof(Tmp) - sizeof(SERV) );
            WrkSrv.Clear();
            SrvHdr.GruppiFermata = IncrNB(SrvHdr.GruppiFermata  ,7);
            STRINGA Msg;
            // Carico tutti i servizi di questa stessa fermata
            FORALL(Wrk,w){
               INFO_AUX & Aux = Wrk[w];
               if(Aux.Tipo != AUX_SERVIZIO) continue;
               if(!Aux.DiFermata          ) continue;
               if(Aux.Da   != InfoAux.Da  ) continue;
               // Si noti che i dati di fermata non hanno periodicit…
               SERV Srv; Srv.Servizio = Aux.Id ; Srv.Continua = TRUE;
               Msg += MM_INFO::DecServizio(Srv.Servizio,FALSE);
               WrkSrv += Srv;
               NumSrv ++;
               if( 1 << Srv.Servizio && MaskPrenotazione) Clusta_Ext_Rec3.InfoUniforme.PrenotDisUniforme    = TRUE;
               if( 1 << Srv.Servizio && MaskTrasporto   ) Clusta_Ext_Rec3.InfoUniforme.ServTraspDisUniformi = TRUE;
               if( 1 << Srv.Servizio && MaskGenerici    ) Clusta_Ext_Rec3.InfoUniforme.ServGenerDisUniformi = TRUE;
               Aux.Tipo = AUX_TAPPO  ; // Per non riutilizzare il dato
            };
            TRACESTRING("Caricati servizi per fermata CCR "+STRINGA(InfoAux.Da)+" Prog Nø " +STRINGA(Tmp.ProgFermata) +" Servizi "+Msg);
            WrkSrv.Last().Continua = FALSE;
            Appo.Store((BUFR&)WrkSrv);
//<<<    if &Fv  
         } else {
            Bprintf2("Ignorati servizi relativi a fermata non identificabile CCR %i MV %i", InfoAux.Da, InfoAux.MezzoVirtuale);
            // Disabilito tutti i servizi di questa stessa fermata
            FORALL(Wrk,w){
               INFO_AUX & Clr = Wrk[w];
               if(Clr.Tipo != AUX_SERVIZIO) continue;
               if(!Clr.DiFermata          ) continue;
               if(Clr.Da   != InfoAux.Da  ) continue;
               Clr.Tipo = AUX_TAPPO  ; // Per non riutilizzare il dato
            };
            NumErrSrv ++;
         }
//<<< FORALL Wrk,w1  
      }
      
      // .....................................
      // Carico i dati dei servizi di treno
      // .....................................
      FORALL(Wrk,w2){
         INFO_AUX & InfoAux = Wrk[w2];
         if(InfoAux.Tipo != AUX_SERVIZIO) continue;
         if(!InfoAux.DiMvg) continue;
         // Servizio di treno
         SRV_TRENO Tmp;
         Tmp.Mvg            = InfoAux.Mvg         ;
         Tmp.IdxPeriodicita = AllPeriodicita.Indice(InfoAux.Periodicita);
         Appo.Store(&Tmp, sizeof(Tmp) - sizeof(SERV) );
         WrkSrv.Clear();
         SrvHdr.GruppiTreno = IncrNB(SrvHdr.GruppiTreno    ,7);
         // Carico tutti i servizi di questi stessi treni
         STRINGA Msg;
         FORALL(Wrk,w){
            INFO_AUX & Aux = Wrk[w];
            if(Aux.Tipo        != AUX_SERVIZIO        ) continue;
            if(!Aux.DiMvg                             ) continue;
            if(Aux.Mvg         != InfoAux.Mvg         ) continue;
            if(Aux.Periodicita != InfoAux.Periodicita ) continue;
            SERV Srv; Srv.Servizio = Aux.Id ; Srv.Continua = TRUE;
            Msg += MM_INFO::DecServizio(Srv.Servizio,FALSE);
            WrkSrv += Srv;
            NumSrv ++;
               if( 1 << Srv.Servizio && MaskPrenotazione) Clusta_Ext_Rec3.InfoUniforme.PrenotDisUniforme    = TRUE;
               if( 1 << Srv.Servizio && MaskTrasporto   ) Clusta_Ext_Rec3.InfoUniforme.ServTraspDisUniformi = TRUE;
               if( 1 << Srv.Servizio && MaskGenerici    ) Clusta_Ext_Rec3.InfoUniforme.ServGenerDisUniformi = TRUE;
            Aux.Tipo = AUX_TAPPO  ; // Per non riutilizzare il dato
         };
         TRACESTRING("Caricati servizi per treni "+STRINGA(InfoAux.Mvg)+" Servizi "+Msg);
         WrkSrv.Last().Continua = FALSE;
         Appo.Store((BUFR&)WrkSrv);
//<<< FORALL Wrk,w2  
      }
      
      // .....................................
      // Carico i dati dei servizi complessi
      // .....................................
      FORALL(Wrk,w3){
         INFO_AUX & InfoAux = Wrk[w3];
         if(InfoAux.Tipo != AUX_SERVIZIO) continue;
         if(InfoAux.DiFermata           ) continue;
         if(InfoAux.DiMvg               ) continue;
         // Servizio complesso
         // Innanzitutto debbo individuare le fermate
         BYTE PrgDa=0,PrgA=0,PrgMaxDa=0;
         FERMATE_VIRT & FvDa    = TabFv.CercaCcrDiMv(InfoAux.MezzoVirtuale, InfoAux.Da   );
         if(& FvDa   ) PrgDa    = FvDa.Progressivo2 ;
         FERMATE_VIRT & FvA     = TabFv.CercaCcrDiMv(InfoAux.MezzoVirtuale, InfoAux.A    );
         if(& FvA    ) PrgA     = FvA.Progressivo2 ;
         FERMATE_VIRT & FvMaxDa = TabFv.CercaCcrDiMv(InfoAux.MezzoVirtuale, InfoAux.MaxDa);
         if(& FvMaxDa) PrgMaxDa = FvMaxDa.Progressivo2 ;
         // Si noti che i progressivi sono a 0 anche se la fermata esiste ma e' un transito (non nodale)
         if(PrgDa > 0 && ((InfoAux.TipoAUX != 3 && InfoAux.TipoAUX != 4) || PrgA > 0)){
            SRV_CMPLX Tmp;
            Tmp.Tipo           = InfoAux.TipoAUX     ;
            Tmp.IdxPeriodicita = AllPeriodicita.Indice(InfoAux.Periodicita);
            Tmp.ProgDa         = PrgDa               ;
            Tmp.ProgMaxDa      = PrgMaxDa            ;
            Tmp.ProgA          = PrgA                ;
            Appo.Store(&Tmp, sizeof(Tmp) - sizeof(SERV) );
            WrkSrv.Clear();
            SrvHdr.GruppiComplessi = IncrNB(SrvHdr.GruppiComplessi,7);
            // Carico tutti i servizi di questi stessi treni
            STRINGA Msg;
            FORALL(Wrk,w){
               INFO_AUX & Aux = Wrk[w];
               if(Aux.Tipo        != AUX_SERVIZIO        ) continue;
               if(Aux.DiMvg                              ) continue;
               if(Aux.DiFermata                          ) continue;
               if(Aux.TipoAUX     != InfoAux.TipoAUX     ) continue;
               if(Aux.Periodicita != InfoAux.Periodicita ) continue;
               if(Aux.Da          != InfoAux.Da          ) continue;
               if(Aux.MaxDa       != InfoAux.MaxDa       ) continue;
               if(Aux.A           != InfoAux.A           ) continue;
               SERV Srv; Srv.Servizio = Aux.Id ; Srv.Continua = TRUE;
               Msg += MM_INFO::DecServizio(Srv.Servizio,FALSE);
               WrkSrv += Srv;
               if( 1 << Srv.Servizio && MaskPrenotazione) Clusta_Ext_Rec3.InfoUniforme.PrenotDisUniforme    = TRUE;
               if( 1 << Srv.Servizio && MaskTrasporto   ) Clusta_Ext_Rec3.InfoUniforme.ServTraspDisUniformi = TRUE;
               if( 1 << Srv.Servizio && MaskGenerici    ) Clusta_Ext_Rec3.InfoUniforme.ServGenerDisUniformi = TRUE;
               NumSrv ++;
               Aux.Tipo = AUX_TAPPO  ; // Per non riutilizzare il dato
            };
            WrkSrv.Last().Continua = FALSE;
            Appo.Store((BUFR&)WrkSrv);
            TRACESTRING("Caricati servizi complessi Tratta (CCR) " +STRINGA(InfoAux.Da) +"->" +STRINGA(InfoAux.A) +" Prog. " + STRINGA(PrgDa)+"->" + STRINGA(PrgA) +" Indice Periodicit… "+STRINGA(Tmp.IdxPeriodicita) +" Servizi "+Msg);
            
//<<<    if PrgDa > 0 &&   InfoAux.TipoAUX != 3 && InfoAux.TipoAUX != 4  || PrgA > 0   
         } else {
            NumErrSrv ++;
            Bprintf2("Ignorati servizi relativi a tratta non identificabile CCR %i->%i MV %i", InfoAux.Da, InfoAux.A, InfoAux.MezzoVirtuale);
            // Disabilito tutti i servizi analoghi
            FORALL(Wrk,w){
               INFO_AUX & Clr = Wrk[w];
               if(Clr.Tipo != AUX_SERVIZIO) continue;
               if(Clr.DiFermata           ) continue;
               if(Clr.DiMvg               ) continue;
               if(Clr.Da   != InfoAux.Da  ) continue;
               if(Clr.A    != InfoAux.A   ) continue;
               if(Clr.Id   != InfoAux.Id  ) continue;
               Clr.Tipo = AUX_TAPPO  ; // Per non riutilizzare il dato
            };
         }
         
//<<< FORALL Wrk,w3  
      }
      if (
         SrvHdr.NumClassifiche   ||
         SrvHdr.GruppiFermata    ||
         SrvHdr.GruppiTreno      ||
         SrvHdr.GruppiComplessi
      ) {
         Clusta_Ext_Rec3.InfoUniforme.Disuniforme = 1;
         MV_Estesi_Servizi = Clusta_Ext_Rec3.OffsetServizi ;
         MV_Estesi_LastInfo = Clusta_Ext_Rec3.InfoUniforme  ; // Salvo 
         Serv.Store(VRB(SrvHdr));
         Serv.Store(Appo);
         TRACESTRING("Caricati "+ STRINGA(Appo.Dim() + sizeof(SrvHdr))+" Bytes Ad offset " + STRINGA(Clusta_Ext_Rec3.OffsetServizi) );
         // TRACEHEX("Dati caricati:",&Serv[Clusta_Ext_Rec3.OffsetServizi],Appo.Dim() + sizeof(SrvHdr));
      } else {
         Clusta_Ext_Rec3.OffsetServizi = 0; // Non definiti
         Clusta_Ext_Rec3.InfoUniforme.Disuniforme = 0;
         TRACESTRING("Nessun Servizio Caricato !");
      } /* endif */
      TRACESTRING( VRS(SrvHdr.NumClassifiche ) + VRS(SrvHdr.GruppiFermata  ) + VRS(SrvHdr.GruppiTreno    ) + VRS(SrvHdr.GruppiComplessi) );
//<<< if  MV_Estesi == TabTv2.RecordCorrente  .MezzoVirtuale    // Riutilizzo dati: e' sempre lo stesso MV
   }
   return 0;
//<<< int WORK::CaricaDatiEstesiMV  BUFR & Extra, BUFR & Serv   
};
//----------------------------------------------------------------------------
// WORK::CaricaDatiDiFermataMV
//----------------------------------------------------------------------------
// Gestisco i dati di fermata del mezzo virtuale
// Alla fine di questa routine
// - Ho caricato i dati di fermata nel buffer temporaneo
// - Ho aggiornato LimiteTrattaCorta
// Si noti che utilizzo GLI STESSI dati estesi per le differenti periodicit…
// dei mezzi virtuali
//----------------------------------------------------------------------------
int  WORK::CaricaDatiDiFermataMV(CLUSTER_MV & Clu, int ProgressivoPeriodicita ){
   #undef TRCRTN
   #define TRCRTN "WORK::CaricaDatiDiFermataMV"
   
   // Il limite di tratta corta va calcolato in base alle fermate effettive del MV
   int LimiteTrattaCorta  = 0;
   
   WORD Mv = TabTv2.RecordCorrente().MezzoVirtuale;
   
   if(!TabFv2.Seek(Mv,ProgressivoPeriodicita)){
      Bprintf("Errore interno algoritmo: non trovate le fermate del MV %i Progressivo periodicit… %i", TabTv2.RecordCorrente().MezzoVirtuale,ProgressivoPeriodicita);
      BEEP;
      return 4;
   };
   
   // Scandisco le fermate del treno e memorizzo i relativi dati
   ZeroFill(Clusta_Ext_Rec4);
   while (
      &TabFv2.RecordCorrente() != NULL            &&
      TabFv2.RecordCorrente().MezzoVirtuale == Mv &&
      TabFv2.RecordCorrente().ProgressivoPeriodicita == ProgressivoPeriodicita
   ) {
      int Idx = IDToProg[TabFv2.RecordCorrente().Id];
      if(Idx != 0xff){ // Controllo che non sia un transito DA IGNORARE
         if(Idx  >= Clu.NumeroStazioni || Idx >= MAX_STAZIONI_IN_CLUSTER){
            TRACEVLONGL(TabFv2.RecordCorrente().Id,1);
            TRACEVLONGL(IDToProg[TabFv2.RecordCorrente().Id],1);
            TRACEVLONGL(Clu.NumeroStazioni,1);
            BEEP;
            Bprintf("Cluster %i Errore su fermata %i : Progressivo fuori range",Clu.Id,TabFv2.RecordCorrente().Id);
            return 234;
         }
         Clusta_Ext_Rec4[Idx].Partenza                = TabFv2.RecordCorrente().FermataPartenza && ! TabFv2.RecordCorrente().Transito;
         Clusta_Ext_Rec4[Idx].Arrivo                  = TabFv2.RecordCorrente().FermataArrivo   && ! TabFv2.RecordCorrente().Transito;
         Clusta_Ext_Rec4[Idx].TransitaOCambiaTreno    = TabFv2.RecordCorrente().Transito ;
         Clusta_Ext_Rec4[Idx].GiornoSuccessivoArrivo  = Chk(TabFv2.RecordCorrente().GiornoSuccessivoArrivo,1)  ;
         Clusta_Ext_Rec4[Idx].GiornoSuccessivoPartenza= Chk(TabFv2.RecordCorrente().GiornoSuccessivoPartenza,1);
         Clusta_Ext_Rec4[Idx].P_MezzoViaggiante       = Chk(TabFv2.RecordCorrente().TrenoFisico,3)             ;
         Clusta_Ext_Rec4[Idx].ProgressivoStazione     = Chk(TabFv2.RecordCorrente().Progressivo2,8)            ;
         Clusta_Ext_Rec4[Idx].OraArrivo               = Chk(TabFv2.RecordCorrente().OraArrivo,11)              ;
         Clusta_Ext_Rec4[Idx].OraPartenza             = Chk(TabFv2.RecordCorrente().OraPartenza,11)            ;
         if( Clusta_Ext_Rec4[Idx].Partenza ||
            Clusta_Ext_Rec4[Idx].Arrivo  ||
            Clusta_Ext_Rec4[Idx].TransitaOCambiaTreno
         ){
            NumStazPiene ++;
            if(Clusta_Ext_Rec4[Idx].TransitaOCambiaTreno) NumTransiti ++;
         } else {
            NumFermServizio ++;
         }
         // Imposto i Km
         int KmCorretti,Delta;
         if (ClusMezv.RecordCorrente().VersoConcorde) {
            KmCorretti = TabFv2.RecordCorrente().ProgKm - ClusMezv.RecordCorrente().Distanza1;
         } else {
            KmCorretti = ClusMezv.RecordCorrente().Distanza1 - TabFv2.RecordCorrente().ProgKm;
         } /* endif */
         Delta = KmCorretti - Clusta_Ext_Rec2[Idx].MediaKm;
         // Per i transiti forzo i Km a 0 (Tanto non sono usati) come anche gli orari di arrivo e partenza ecc
         // NB: I km non sono linearizzati in modo esatto per i transiti quindi preferisco azzerarli
         if(Clusta_Ext_Rec4[Idx].TransitaOCambiaTreno){
            Delta = 0;
            Clusta_Ext_Rec4[Idx].Partenza                = 0;
            Clusta_Ext_Rec4[Idx].Arrivo                  = 0;
            Clusta_Ext_Rec4[Idx].GiornoSuccessivoArrivo  = 0;
            Clusta_Ext_Rec4[Idx].GiornoSuccessivoPartenza= 0;
            Clusta_Ext_Rec4[Idx].P_MezzoViaggiante       = 0;
            Clusta_Ext_Rec4[Idx].OraArrivo               = 0;
            Clusta_Ext_Rec4[Idx].OraPartenza             = 0;
         } else {
            Top( LimiteTrattaCorta, TabFv2.RecordCorrente().ProgKm / 2); // Le fermate incidono sul limite di tratta corta
            // Debbo vedere se e' una delle stazioni di cambio
            MEZZO_VIRTUALE & MvLe = TabTv2.RecordCorrente();
            for (int i = 0 ; i < MvLe.NumMezziComponenti -1;i++ ) {
               if(TabFv2.RecordCorrente().CCR == MvLe.Mv[i].CcrStazioneDiCambio){
                  Clusta_Ext_Rec4[Idx].TransitaOCambiaTreno = 1; // Cambia Treno
               }
            } /* endfor */
         }
         Top(MaxDelta,Delta);
         Bottom(MinDelta,Delta);
         if(abs(Delta) > 255){
            STRINGA Msg;
            Bprintf("Delta %i : ",Delta);
            Bprintf("  %s",(CPSZ)(VRS(TabFv2.RecordCorrente().Id)));
            Bprintf("  %s",(CPSZ)(VRS(ClusMezv.RecordCorrente().IdCluster)));
            Bprintf("  %s",(CPSZ)(VRS(ClusMezv.RecordCorrente().MezzoVirtuale)));
            Bprintf("  %s",(CPSZ)(VRS(TabFv2.RecordCorrente().ProgKm)));
            Bprintf("  %s",(CPSZ)(VRS(ClusMezv.RecordCorrente().Distanza1)));
            Bprintf("  %s",(CPSZ)(VRS(KmCorretti )));
            Bprintf("  %s",(CPSZ)(VRS(Clusta_Ext_Rec2[Idx].MediaKm )));
            Bprintf("  %s",(CPSZ)(VRS(ClusStaz.RecordCorrente().Distanza )));
            Bprintf("  %s",(CPSZ)(VRS(ClusStaz.RecordCorrente().DistanzaMediana )));
         }
         Clusta_Ext_Rec4[Idx].DeltaKm  = ChkS(Delta,9);
//<<< if Idx != 0xff   // Controllo che non sia un transito DA IGNORARE
      } else {
         NumTransitiDaIgnorare ++;
      };
      TabFv2.Next();
//<<< while  
   } /* endwhile */
   // Aggiungo il limite di tratta corta
   Clusta_Ext_Rec3.KmLimiteTrattaCorta = min(250,LimiteTrattaCorta);
   return 0;
//<<< int  WORK::CaricaDatiDiFermataMV CLUSTER_MV & Clu, int ProgressivoPeriodicita   
};
//----------------------------------------------------------------------------
// WORK::CaricaEScriviDatiMultistazioni
//----------------------------------------------------------------------------
// Alla fine di questa routine:
// - Ho generato CLUSTAZ.EXT e CLUSTAZ.DB
// - Li ho scritti su file
// Per i soli cluster multistazione
//----------------------------------------------------------------------------
int  WORK::CaricaEScriviDatiMultistazioni(){
   #undef TRCRTN
   #define TRCRTN "WORK::CaricaEScriviDatiMultistazioni"
   ORD_FORALL(Gestore->ClustersMultiStazione,K0){
      MULTI_STA & Mult = * Gestore->ClustersMultiStazione[K0];
      
      OutC.Clear(); OutD.Clear();
      NumeroCLUSTERS_STAZ ++;
      NumeroCLUSTER_MULTI ++;
      int NumeroCluster = NumeroCLUSTERS_STAZ;
      
      Clusta_Db_Rec.IdCluster         =  Chk(NumeroCluster,11)        ;
      Clusta_Db_Rec.TipoCluster       =  CLUSTER_MULTISTAZIONE        ;
      Clusta_Db_Rec.OffSetBytes       =  Chk(OffSet ,23)              ;
      Clusta_Db_Rec.Id1               =  Chk(Mult.Stazioni[0],13)     ;
      Clusta_Db_Rec.Id2               =  Clusta_Db_Rec.Id1            ;
      Clusta_Db_Rec.DimDatiC          =  Chk(
         sizeof(CLUSTERS_STAZ_EXT) - sizeof(CLUSSTAZ) +                      // Testata
         (sizeof(CLUSSTAZ) * Mult.Stazioni.Dim())     +                      // Dati delle stazioni
         Mult.Stazioni.Dim()*Mult.Stazioni.Dim()*sizeof(COLLEG_URBANO)+      // Matrice di interscambio
         Mult.Stazioni.Dim()*sizeof(ACRONIMI_CITTA) , 19);                   // Acronimi multistazioni
      Clusta_Db_Rec.DimDatiD          =  0;
      TRACEVLONG(Clusta_Db_Rec.IdCluster        );
      TRACEVLONG(Clusta_Db_Rec.TipoCluster      );
      TRACEVLONG(Clusta_Db_Rec.OffSetBytes      );
      TRACEVLONG(Clusta_Db_Rec.DimDatiC         );
      TRACEVLONG(Clusta_Db_Rec.DimDatiD         );
      TRACEVLONG(Mult.Stazioni.Dim());
      
      // Aggiorno i due records di testata e scrivo i relativi files.
      Clusta_Db.AddRecordToEnd(VRB(Clusta_Db_Rec));
      Header.NumClusters ++;
      Clusta_Ext_Rec1.DimDati         =  Clusta_Db_Rec.DimDatiC          ;
      Clusta_Ext_Rec1.IdCluster       =  Clusta_Db_Rec.IdCluster         ;
      Clusta_Ext_Rec1.TotStazioni     =  Mult.Stazioni.Dim()             ;
      Clusta_Ext_Rec1.NumeroNodi      =  Mult.Stazioni.Dim()             ;
      Clusta_Ext_Rec1.NumeroTreni     =  0;
      Clusta_Ext_Rec1.OffsetIdTreni   =  0;    // Non significativo
      Clusta_Ext_Rec1.OffsetSrvTreni  =  0;    // Non significativo
      OutC.Store(&Clusta_Ext_Rec1,sizeof(Clusta_Ext_Rec1) - sizeof(CLUSSTAZ));  // Testata
      
      OffSet += Clusta_Db_Rec.DimDatiC;
      
      // Scandisco le STAZIONI del cluster e per ogni stazione scarico un record di tipo CLUSSTAZ
      ORD_FORALL(Mult.Stazioni,K1){
         
         Clusta_Ext_Rec2[K1].Fermata              =   1;
         Clusta_Ext_Rec2[K1].ClasseCoincidenza    =   Staz[Mult.Stazioni[K1]].ClasseCoincidenza;
         Clusta_Ext_Rec2[K1].IdNodo               =   Mult.Stazioni[K1];
         Clusta_Ext_Rec2[K1].MediaKm              =   0;
         Clusta_Ext_Rec2[K1].Gruppi.Clear();
         Clusta_Ext_Rec2[K1].Gruppi.Set(0);
         
      }
      OutC.Store(&Clusta_Ext_Rec2,sizeof(CLUSSTAZ)*Mult.Stazioni.Dim());  // Dati delle stazioni
      NumeroCLUSSTAZ += Mult.Stazioni.Dim();
      OutC.Store(Mult.Colleg,Mult.Stazioni.Dim()*Mult.Stazioni.Dim()*sizeof(COLLEG_URBANO));
      NumeroURBANI += Mult.Stazioni.Dim()*Mult.Stazioni.Dim();
      
      // Ora scarico gli acronimi
      ORD_FORALL(Mult.Stazioni,K2){
         int Pos = Gestore->StazioniMS.Posizione(Mult.Stazioni[K2]);
         if(Pos < 0){
            Bprintf("Acronimi multistazioni inconsistenti");
            return 999;
         };
         //Bprintf("Caricato acronimo %s per stazione %i %s",(CPSZ)Gestore->Acronimi[Pos],Gestore->StazioniMS[Pos],Stazioni.DecodificaIdStazione(Gestore->StazioniMS[Pos]));
         OutC.Store((CPSZ)Gestore->Acronimi[Pos],4);
      }
      
      NumeroACRONIMI += Mult.Stazioni.Dim();
      
      // Controllo generale dimensione delle aree
      if (OutC.Dim() != Clusta_Db_Rec.DimDatiC) {
         Bprintf("Errata dimensione dei dati: %i invece di %i",OutC.Dim(),Clusta_Db_Rec.DimDatiC);
         TRACEVLONGL(OutC.Dim(),1);
         TRACEVLONGL(Clusta_Db_Rec.DimDatiC,1);
         return 31;
      } /* endif */
      Clusta_Ext.Scrivi(OutC);
//<<< ORD_FORALL Gestore->ClustersMultiStazione,K0  
   };
   return 0;
//<<< int  WORK::CaricaEScriviDatiMultistazioni   
};
//----------------------------------------------------------------------------
// Sort_Treni
//----------------------------------------------------------------------------
// Ordinamento dei MV nell' ambito di un cluster per MV e progressivo periodicita'
int Sort_Treni( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "Sort_Treni"
   CLUSTRENO & A = *(CLUSTRENO*) a;
   CLUSTRENO & B = *(CLUSTRENO*) b;
   if(A.IdMezv == B.IdMezv){
      return (int)A.ProgressivoPeriodicita - (int)B.ProgressivoPeriodicita;
   } else {
      return (int)A.IdMezv - (int)B.IdMezv ;
   }
};

//----------------------------------------------------------------------------
// WORK::MakeStazioniCluster
//----------------------------------------------------------------------------
// Alla fine di questa routine:
// - Ho generato le relazioni tra stazioni e cluster
// Sia per i cluster normali che per le multistazione
// ---------------------------------------------------------------------------
int WORK::MakeStazioniCluster(){
   #undef TRCRTN
   #define TRCRTN "WORK::MakeStazioniCluster"
   
   OffSet=0; // Riazzero
   
   Bprintf("Scansione delle stazioni e scarico dati : %i Stazioni",Staz.Dim());
   ORD_FORALL(Staz,k1){
      TryTime(k1);
      STAZIONE_MV & Stazione = Staz[k1];
      TRACESTRING(" ================ Stazione Id = "+STRINGA(Stazione.Id)+" =====================");
      NumeroSTAZ_CLUSTERS ++;
      BOOL MultiStaz = FALSE;
      int IdClusterMS =0;
      
      MultiStaz = Gestore->StazioniMS.Contiene(Stazione.Id);
      if (MultiStaz){
         ORD_FORALL(Gestore->ClustersMultiStazione,K0){
            MULTI_STA & Mult = *Gestore->ClustersMultiStazione[K0];
            if (Mult.Stazioni.Contiene(Stazione.Id)){
               IdClusterMS = OffSetMultiStazione + K0;
               break;
            }
         }
         if (IdClusterMS == 0){
            BEEP;
            Bprintf("Problemi con i multistazione! Dati incoerenti ID Stazione %i",Stazione.Id);
         }
         TRACELONG("ID Cluster Multistazione: ", IdClusterMS);
      }
      TRACEVLONG(Stazione.NumClusters);
//<<< ORD_FORALL Staz,k1  
   };
   
   
   // ---------------------------------------------------------
   // Relazioni Stazioni -> Cluster:
   // ---------------------------------------------------------
   F_COLLEGAMENTO2 CollegamentiTraStazioniNelCluster(PATH_OUT "COLLCLUS.TMP");
   HASH<STAZN_CLUS> HashCS(13,6000);
   Bprintf("Genero i collegamenti estesi tra stazioni: %i Collegamenti",CollegamentiTraStazioniNelCluster.Dim());
   ORD_FORALL(CollegamentiTraStazioniNelCluster,NumCst8){
      TryTime(NumCst8);
      COLLEGAMENTO2 & C2  = CollegamentiTraStazioniNelCluster.FixRec(NumCst8);
      STAZN_CLUS * CS ,R1;
      ZeroFill(R1);
      R1.IdCluster = C2.IdCluster;
      R1.Stazione  = C2.StazionePartenza;
      CS = HashCS.Cerca(&R1,3);
      if (CS == NULL) {
         CS = HashCS.Alloca();
         *CS = R1;
         HashCS.Metti(3);
      } /* endif */
      if (C2.Concorde) {
         CS->OrariPartenzaC |= C2.OrariPartenza;
      } else {
         CS->OrariPartenzaD |= C2.OrariPartenza;
      } /* endif */
      R1.Stazione  = C2.StazioneArrivo;
      CS = HashCS.Cerca(&R1,3);
      if (CS == NULL) {
         CS = HashCS.Alloca();
         *CS = R1;
         HashCS.Metti(3);
      } /* endif */
      if (C2.Concorde) {
         CS->OrariArrivoC |= C2.OrariArrivo;
      } else {
         CS->OrariArrivoD |= C2.OrariArrivo;
      } /* endif */
//<<< ORD_FORALL CollegamentiTraStazioniNelCluster,NumCst8  
   }
   Bprintf("Match con i dati delle relazioni Cluster -> Stazioni : %i controlli",ClusStaz.Dim());
   ORD_FORALL(ClusStaz,Cs1){
      TryTime(Cs1);
      CLUSTER_STAZIONE & CluSt = ClusStaz.FixRec(Cs1);
      if(CluSt.NumMzvFerm == 0)continue;
      STAZN_CLUS * CS ,R1;
      R1.IdCluster = CluSt.IdCluster;
      R1.Stazione  = CluSt.IdStazione;
      CS = HashCS.Cerca(&R1,3);
      if (CS) {
         CS->Progressivo = Chk(CluSt.Prog2,8);
         CS->Gruppi      = CluSt.Gruppo;
      } else {
         // Qualche disallineamento e' possibile a causa delle fermate di sola partenza o solo arrivo
         Bprintf2("Warning: Mancata rispondenza tra files per Cluster %i Stazione %i",CluSt.IdCluster,CluSt.IdStazione);
      } /* endif */
   };
   Bprintf("Aggiungo le relazioni dovute ai cluster multistazione: %i Relazioni",Gestore->ClustersMultiStazione.Dim());
   ORD_FORALL(Gestore->ClustersMultiStazione,M0){
      TryTime(M0);
      MULTI_STA & Mult = * Gestore->ClustersMultiStazione[M0];
      if(Mult.Colleg == NULL)continue; // Anomalo
      ORD_FORALL(Mult.Stazioni,K1){
         STAZN_CLUS & Rec = * HashCS.Alloca();
         Rec.Stazione       = Mult.Stazioni[K1]; // Key (3 Bytes Totali) E' l' ID Esterno su file, interno quando e' caricato in memoria
         Rec.IdCluster      = OffSetMultiStazione + M0; // Key
         Rec.Progressivo    = 1; // Progressivo stazione nell' ambito del cluster
         Rec.Gruppi.Clear(); Rec.Gruppi.Set(0);
         Rec.OrariPartenzaC = 0xffff;
         Rec.OrariPartenzaD = 0xffff;
         Rec.OrariArrivoC   = 0xffff;
         Rec.OrariArrivoD   = 0xffff;
         assert(HashCS.Cerca(&Rec,3) == NULL); // Non deve essere gia' presente
         HashCS.Metti(3);
         TRACESTRING("Aggiunta relazione tra stazione "+STRINGA(Rec.Stazione)+ " e cluster MS "+ STRINGA(Rec.IdCluster));
      }
   };
   Bprintf("Scarico dei dati, %i elementi",HashCS.NumeroElementiInHash());
   
   int T = 0;
   ITERA(HashCS,StzC,STAZN_CLUS){
      Staz_Clus.AddRecordToEnd(VRB(StzC));
      TryTime(T++);
   } END_ITERA
   Bprintf("Sort delle relazioni");
   Staz_Clus.ReSortFAST();
   return 0;
//<<< int WORK::MakeStazioniCluster   
}
