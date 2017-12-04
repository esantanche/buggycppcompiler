//----------------------------------------------------------------------------
// ML_STEP2.HPP: Fase iniziale algoritmo preparazione dati del motore
//----------------------------------------------------------------------------
/*
   Sono definite le seguenti strutture intermedie di lavoro: 

   - Staz2
      E' una array di STAZIONE_MV che contiene le informazioni di 
      massima sulle stazioni: le informazioni sono accumulate qui e poi 
      scaricate su file

   - ClustersIdx
      E' un indice (HASH-TABLE) dei dati dei cluster. Contiene i dati 
      fondamentali (Necessari per identificare l' appartenenza di un MV 
      ad un cluster) ed un puntamento a file.

   - CollStaz
   - CollClustStaz
      Sono HASH-TABLES utilizzate per tenere traccia dei collegamenti 
      diretti tra stazioni. 
      Sono caricate in memoria ( dal metodo CaricaCollegamenti ) e poi 
      trasferite su file.
      I collegamenti stessi sono definiti su ML_OUT.HPP e sono piccole 
      classi che definiscono gli operatori base per costruire il 
      collegamento a partire dal dato elementare del treno




   Il tutto e' posto in una classe di lavoro WORK per comodita' 

*/
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------      
// Strutture usate dall' algoritmo
//----------------------------------------------------------------------------      
// Questa struttura serve per l' identificazione dei nodi dominanti,
// soggetti a vincoli di distanza.
// Inoltre serve come struttura intermedia per i collegamenti ed il calcolo
// dei dati riassuntivi (orario partenza ed arrivo) per collegamenti e clusters
struct FERM {
   WORD  Id;
   WORD  ProgKm;
   int   Peso;
   short int OraPartenza; // -1  se non parte
   short int OraArrivo;   // -1  se non arriva
};

struct STZ {
   ID    Id;
   WORD  dKm;   // Km dalla precedente stazione (0 per la prima)
};

struct STZ2 {
   ID    Id;
   short int Km;       // Km (Linearizzati) dalla stazione dominante del cluster
   short int KmMedi;   // Km (Mediani)      dalla stazione dominante del cluster
};

// Questa struttura contiene i "Percorsi Logici" dei MV contenuti in un cluster
struct PERCORSO_LOGICO {
   WORD NumeroStazioni;
   WORD OffsetDominante;
   PERCORSO_LOGICO * Next; // Sono gestiti in linked list
   STZ  Stazioni[1]   ; // Segue un' array di NumeroStazioni ID

   // Il metodo confronta ritorna: 
   // 0  = I due percorsi logici sono eguali
   // 1  = Il percorso logico contiene B
   // 2  = B Contiene il percorso logico
   // 3  = I percorsi logici sono diversi ma compatibili
   // 4  = I percorsi logici sono incompatibili
   int Confronta( PERCORSO_LOGICO & B);
   WORD Dim(){return NumeroStazioni;};
};


// Indice cluster
struct CLUSTER_IDX {
   ID   Id1;
   ID   Id2;
   BYTE Tipo;
   int  NumeroRecord; // == IdCluster o -1 se non assegnato
   WORD UsageCount;
   LISTA< PERCORSO_LOGICO> PercorsiLogici;
   WORD NumStazioni;
   STZ2 * StazLin;
   void LinearizzaDistanze(); // Linearizza le distanze e carica Stazioni
   int Cerca(ID Id); // Cerca una stazione in StazLin e torna l' indice (o -1 se non trovata)
};

//----------------------------------------------------------------------------      
// Gestore Fermate
//----------------------------------------------------------------------------      
// Questa classe gestisce l' insieme delle fermate di un mezzo virtuale
struct ARRAY_FERM : public BUFR {
   FERM& operator[] (UINT Indice){
      #ifdef BOUND_CHECK
      if(Indice >= Dim() )BoundCheckError(Indice,Dim());
      #endif
      return * ( ((FERM*)this->Dati) +Indice);
   };
   ULONG Dim(){return this->Length / sizeof(FERM);};
   ARRAY_FERM(ULONG i=400);
   void operator+= (FERM& Ferm){BUFR::Store(&Ferm,sizeof(FERM));};
   BUFR::Clear;  // Rendo accessibile
   
   // Identifica il cluster
   // Torna l' Id = il numero del record del cluster su file
   // Se il cluster non esiste lo aggiunge, altrimenti aggiorna il file
   // Attenzione: Poiche' aggiorna i dati, DEVE essere chiamata una sola volta per MV.
   int IdentificaCluster(HASH<CLUSTER_IDX> & ClustersIdx, F_CLUSTER_MV & Clust,WORD LastMV, MEZZO_VIRTUALE & MvLe); 
   
   // A partire dall' insieme delle fermate genera tutti i collegamenti diretti possibili e 
   // li carica sulle strutture di Hash
   void CaricaCollegamenti( HASH<COLLEGAMENTO1> & Coll1, HASH<COLLEGAMENTO2> & Coll2 );

   // Identifica i casi di linearizzazione incompatibile
   BOOL LinearizzazioneIncompatibile(CLUSTER_IDX & Cluster);


   short Nodo1,Nodo2,Distanza1,Distanza2;   // Nodi dominanti
   int   PercorrenzaTotale;
   int   IdMezzo    ;                       // Identificatore Mezzo Virtuale
   WORD  OraP1;
   WORD  IdCluster ;                        // Id del cluster di appartenza
   int   TipoCluster;                       // E relativo tipo 
   BYTE  Concorde  ;                        // Indica se il treno ha un senso concorde o discorde con il cluster
   BYTE  Navale    ;                        // Indica se utilizza un mezzo navale
   PERCORSO_LOGICO & PercorsoCorrente;      // Area di lavoro

//<<< struct ARRAY_FERM : public BUFR  
};

//----------------------------------------------------------------------------      
// Classe di lavoro
//----------------------------------------------------------------------------      
struct WORK {

   // Metodi
   WORK();                                                      // Costruttore
   void ContaFermate();                                         // Conta quanti treni fermano ad una data stazione
   void ScandisciTreni();                                       // Ciclo sui treni e preparazione dati
   void ScaricaRelazioni();                                     // Scarico relazioni da memoria a file
   int  ScandisciTreni2();                                      // Altro ciclo sui treni e preparazione relazioni stazione <-> Cluster con distanze linearizzate
   int  IdentificaTreniNonLinearizzati();                       // Controllo che la linearizzazione sia completa

   // Strutture in memoria
   STAZIONE_MV *       Staz2 ;                                  // Dati stazioni
   HASH<COLLEGAMENTO1> CollStaz;                                // Collegamenti diretti tra stazioni               
   HASH<COLLEGAMENTO2> CollClustStaz;                           // Collegamenti diretti tra stazioni / Per cluster 
   HASH<CLUSTER_IDX>   ClustersIdx;                             // Catalogo cluster

   // Files di input
   F_MEZZO_VIRTUALE    TabTv;                                   // Mezzi virtuali
   F_FERMATE_VIRT      TabFv;                                   // Fermate dei Treni Virtuali

   // Files di Output
   F_CLUSTER_MV        Clust;                                   // Clusters
   F_CLUSTER_STAZIONE  ClusStaz;                                // Relazioni Clusters -> Stazioni
   F_MEZZOV_CLUSTER    ClusMezv;                                // Relazioni Clusters -> Mezzi virtuali
   F_COLLEGAMENTO1     CollegamentiTraStazioni;                 // Collegamenti diretti tra stazioni
   F_COLLEGAMENTO2     CollegamentiTraStazioniNelCluster;       // Collegamenti diretti tra stazioni / Per cluster

};

