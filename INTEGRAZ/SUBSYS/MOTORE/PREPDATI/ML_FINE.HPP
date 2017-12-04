//----------------------------------------------------------------------------
// ML_FINE.HPP: Definizioni per ML_FINE.CPP
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Classe di lavoro
//----------------------------------------------------------------------------
struct WORK {
   
   // Costruttore
   WORK():
   Clust(PATH_OUT "MY_CLUST.TMP")           ,
   ClusStaz(PATH_OUT "CLUSSTAZ.TMP",2000)   ,
   IxClusStaz(PATH_OUT "CLUSSTAZ.IX1",2000) ,
   ClusMezv(PATH_OUT "CLUSMEZV.TMP",2000)   ,
   IxClusMezv(PATH_OUT "CLUSMEZV.IX1",2000) ,
   TabTv2(PATH_OUT "M2_TRENV.TMP",2000)     ,
   TabFv(PATH_OUT "M0_FERMV.TMP",2000)      ,
   TabFv2(PATH_OUT "M2_FERMV.TMP",2000)     ,
   Staz(PATH_OUT "MY_STAZI.TMP")            ,
   DatiAusiliari(PATH_OUT "INFO_AUX.TMP")   ,
   Clusta_Db(PATH_OUT)                      ,
   Clusta_Ext(PATH_OUT "CLUSSTAZ.EXT")      ,
   Staz_Clus(PATH_OUT "MM_STCLU.DB")        ,
   // Variabili da inizializzare 
   OutC(400000)  , OutD(400000)             ,
   ExtraC(20000) , ExtraD(20000)            ,
   ServC(40000)  , ServD(40000)             ,
   StazioniDiCambio(65000)                  ,
   NumeroCLUSTERS_STAZ    (0)               ,  
   NumeroSTAZ_CLUSTERS    (0)               ,  
   NumeroCLUSSTAZ         (0)               ,     
   NumeroCLUSTRENO        (0)               ,     
   NumeroINFOSTAZ         (0)               ,     
   NumeroURBANI           (0)               ,     
   NumeroCLUSTER_MULTI    (0)               ,     
   NumeroACRONIMI         (0)               ,     
   ExtraBytes             (0)               ,     
   TotalSizeExpected      (0)               ,     
   NumFermServizio        (0)               ,  
   NumStazPiene           (0)               ,  
   NumTransiti            (0)               ,  
   NumTransitiDaIgnorare  (0)               ,
   OffSet                 (0)               ,
   OffSetMultiStazione    (0)               ,
   NumCla                 (0)               , 
   NumSrv                 (0)               , 
   NumErrSrv              (0)               , 
   MaxDelta               (-10000000 )      ,
   MinDelta               ( 10000000 )      
   {
      Clusta_Db.Clear("Indice dati dei clusters");
      Clusta_Ext.SetSize(0);
      Gestore = NULL;
      Staz_Clus.Clear("Relazioni Stazioni -> Clusters");
   } ;
   
   // Files di Output ( a parte le periodicit… )
   CLUSTA             Clusta_Db       ; // CLUSSTAZ.DB
   FILE_RW            Clusta_Ext      ; // CLUSSTAZ.EXT
   // Files di Input
   F_CLUSTER_MV       Clust           ; // Dati dei cluster
   F_CLUSTER_STAZIONE ClusStaz        ; // Relazioni cluster - Stazioni
   F_STAZIONE_CLUSTER IxClusStaz      ; // Relativo indice ausiliario
   F_MEZZOV_CLUSTER   ClusMezv        ; // Relazioni Cluster - Mezzi virtuali
   F_CLUSTER_MEZZOV   IxClusMezv      ; // Relativo indice ausiliario
   F_MEZZO_VIRTUALE_2 TabTv2          ; // Dati dei treni virtuali (periodicit… rese uniformi)
   F_FERMATE_VIRT     TabFv           ; // Dati delle fermate virtuali
   F_FERMATE_VIRT_2   TabFv2          ; // Dati delle fermate virtuali (periodicit… rese uniformi)
   F_STAZIONE_MV      Staz            ; // Informazioni sui nodi dei cluster
   F_INFO_AUX         DatiAusiliari   ; // Dati dei servizi
   F_STAZN_CLUS       Staz_Clus       ; // Relazioni tra Stazioni e cluster
   
   // Aree lavoro
   CLUSTERS_STAZ          Clusta_Db_Rec;
   CLUSTERS_STAZ_EXT      Clusta_Ext_Rec1;
   CLUSSTAZ               Clusta_Ext_Rec2[MAX_STAZIONI_IN_CLUSTER];
   CLUSTRENO              Clusta_Ext_Rec3;
   INFOSTAZ               Clusta_Ext_Rec4[MAX_STAZIONI_IN_CLUSTER];
   // Variabili di lavoro
   ARRAY_T_PERIODICITA    AllPeriodicita ; // Elenco di tutte le periodicit…, da codificare
   int                    P1,P2          ; // Puntatori alle stazioni dominanti del cluster corrente
   DATI_ORARIO_FS::HEADER Header         ; // Dati generali dell' orario

   // Questi sono BUFR utilizzati per tenere (temporaneamente) i dati dei MV del cluster
   // Outx  : Dati base
   // Extrax: Identificatori mezzi viaggianti
   // Servx : Dati dei servizi , classifiche ecc.
   BUFR OutC,ExtraC,OutD,ExtraD,ServC,ServD;
   
   // Questo mi serve per identificare le stazioni di cambio
   SET StazioniDiCambio;

   // Questa e' la tabella per identificare le stazioni nel cluster
   BUFR IDToProg              ;

   // Gestore Multistazioni
   GEST_MULTI_STA * Gestore   ;

   // Totalizzatori per tipologia aree scritte
   int NumeroCLUSTERS_STAZ    ;
   int NumeroSTAZ_CLUSTERS    ;
   int NumeroCLUSSTAZ         ;
   int NumeroCLUSTRENO        ;
   int NumeroINFOSTAZ         ;
   int NumeroURBANI           ;
   int NumeroCLUSTER_MULTI    ;
   int NumeroACRONIMI         ;
   int ExtraBytes             ;
   int TotalSizeExpected      ;
   int NumFermServizio        ; // Numero stazioni non di transito ne di fermata caricate sui dati
   int NumStazPiene           ; // Numero stazioni di transito o di fermata caricate sui dati
   int NumTransiti            ; // Numero stazioni di transito
   int NumTransitiDaIgnorare  ;
   int OffSet                 ;
   int OffSetMultiStazione    ;
   int MaxDelta               ;
   int MinDelta               ;

   // Queste variabili servono per riutilizzare gli stessi dati estesi
   // per differenti periodicit… del mezzo virtuale
   int  MV_Estesi             ; // Id del mezzo virtuale per cui ho i dati estesi (riazzerato ad ogni cluster)
   int  MV_Estesi_IdTreni     ; // Valore da caricare nell' offset degli Id treni (o -1 se non valido)
   int  MV_Estesi_Servizi     ; // Valore da caricare nell' offset dei servizi    (o -1 se non valido)
MM_INFO MV_Estesi_LastInfo    ; // Per i flags di disuniformita
   int  NumCla                ; // Numero delle "classifiche" scaricate
   int  NumSrv                ; // Numero dei "servizi" scaricati
   int  NumErrSrv             ; // Numero degli errori caricando i "servizi" 

   // Metodi di classe
   int  Body();
   int  MakeClusterStazioni();
   int  CaricaStazioniDelcluster( CLUSTER_MV & Clu);
   int  CaricaMezziVirtualiDelcluster( CLUSTER_MV & Clu);
   int  CaricaDatiEstesiMV(BUFR & Extra, BUFR & Serv );
   int  CaricaDatiDiFermataMV(CLUSTER_MV & Clu, int ProgressivoPeriodicita );
   int  ScrivoDatiCluster(CLUSTER_MV & Clu);
   int  CaricaEScriviDatiMultistazioni();
   int  MakeStazioniCluster();
   
//<<< struct WORK  
};

//----------------------------------------------------------------------------
// Prototipi funzioni
//----------------------------------------------------------------------------
// Ordinamento dei MV nell' ambito di un cluster per MV e progressivo periodicita'
int Sort_Treni( const void *a, const void *b);

//----------------------------------------------------------------------------
// Funzioncina
//----------------------------------------------------------------------------
// Questa funzione incrementa controllando che non vi sia overflow
int IncrNB(int From, int NumBits){
   int Appo = From;
   Appo ++;
   return Chk(Appo,NumBits);
};
