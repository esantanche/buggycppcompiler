//----------------------------------------------------------------------------
// ML_WRK.HPP : Contiene le strutture intermedie per il caricamento del motore
//----------------------------------------------------------------------------
// Queste strutture sono generate durante il caricamento dei dati del motore
// Sono tutte strutture intermedie di lavoro
//----------------------------------------------------------------------------

#ifndef  HO__ML_WRK
#define  HO__ML_WRK

#include "ML_IN.HPP"
#include "MM_CRIT2.HPP"


// ===========================================
// Queste tabelle per avere periodicita' uniforme a livello di treno virtuale
// ===========================================
// Tabella treni VIRTUALI con periodicita' in chiave
struct MEZZO_VIRTUALE_2 : public MEZZO_VIRTUALE {
   BYTE  ProgressivoPeriodicita      ; // KEY
};

class _export F_MEZZO_VIRTUALE_2 : public FILE_BS {   //    "M2_TRENV.TMP"

   public:

   // Costruttore
   F_MEZZO_VIRTUALE_2(const char* NomeFile,ULONG Size=4096) : FILE_BS(NomeFile,sizeof(MEZZO_VIRTUALE_2),3,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Mezzo_Virtuale,WORD ProgP = 0){
      BUFR Tmp(&Mezzo_Virtuale,sizeof(Mezzo_Virtuale));
      Tmp.Store(&ProgP,sizeof(ProgP));
      return Posiziona(Tmp) && KeyEsatta;
   };

   // Definizione operatori
   MEZZO_VIRTUALE_2&  RecordCorrente(){ return *(MEZZO_VIRTUALE_2*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0);
   virtual void KeySet();     // Imposta KeyCorrente a partire da RecordCorrente

};

// Tabella fermate treni virtuali
struct FERMATE_VIRT_2 : public FERMATE_VIRT {
   BYTE  ProgressivoPeriodicita      ; // KEY
};

class _export F_FERMATE_VIRT_2 : public FILE_BS {  //    "M2_FERMV.TMP"

   public:

   // Costruttore
   F_FERMATE_VIRT_2(const char* NomeFile,ULONG Size=64000) : FILE_BS(NomeFile,sizeof(FERMATE_VIRT_2),5,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Mezzo_Virtuale,BYTE ProgP = 0){
      BUFR Tmp(&Mezzo_Virtuale,sizeof(Mezzo_Virtuale));
      Tmp.Store(&ProgP,sizeof(ProgP));
      return Posiziona(Tmp) && KeyEsatta;
   };

   // Definizione operatori
   FERMATE_VIRT_2&  RecordCorrente(){ return *(FERMATE_VIRT_2*) RecordC; };
   FERMATE_VIRT_2&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(FERMATE_VIRT_2*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0);
   virtual void KeySet();     // Imposta KeyCorrente a partire da RecordCorrente

};

//----------------------------------------------------------------------------
// STAZIONI_MV
//----------------------------------------------------------------------------
// Questa struttura contiene le informazioni addizionali di nodo utilizzate  dall' algoritmo
struct STAZIONE_MV {
   BIT   Id              :13; // Id stazione
   BIT   NumFermate      :13; // Numero di fermate = Peso della stazione / 10
   BIT   NumTransiti     :13; // Numero di transiti
   BIT   NumClusters     :10; // Numero di clusters in relazione con la stazione
   BIT   TipoStazione    : 3; // 1 = Nodo di cambio
                              // 2 = Nodo
                              // 4 = Stazione di instradamento eccezionale
                              // e relative combinazioni binarie
   BIT   ClasseCoincidenza:1; // Classe di coincidenza:
                              // 0 =   Normale (10 Minuti )
                              // 1 =   Esteso  (15 Minuti )
   BIT   NumCollegamenti :11; // Numero di altre stazioni cui e' collegata
   BIT   NumCollCambio   :11; // Numero di stazioni di cambio cui e' collegata
   // Queste definizioni anche su STAZ_CLUSTER (motore)
   BOOL StazioneDiCambio(){return TipoStazione & 1;};
   BOOL StazioneNodale(){return TipoStazione & 2;};
   BOOL IsInstradGrafo(){return TipoStazione & 6;};
};

class F_STAZIONE_MV : public FILE_FIX {   //    "MY_STAZI.TMP"
   public:
   // Costruttore
   F_STAZIONE_MV(const char* NomeFile,ULONG Size=0) : FILE_FIX(NomeFile,sizeof(STAZIONE_MV),Size){};

   // Definizione operatori
   STAZIONE_MV&  RecordCorrente(){ return *(STAZIONE_MV*) RecordC; };
   STAZIONE_MV&  operator [](WORD Indice){ Posiziona(Indice); return *(STAZIONE_MV*) RecordC; };
};

//----------------------------------------------------------------------------
// CLUSTER_MV
//----------------------------------------------------------------------------
// Questa struttura contiene le informazioni di cluster
// Nota: Id1 ed Id2 sono obbligatori, ma non costituiscono necessariamente
// chiave di accesso al cluster: dipende dai criteri di formazione del medesimo.
// In ogni caso per i cluster normali tutti i mezzi del cluster debbono fermare ad Id1 ed Id2
struct CLUSTER_MV {
   BIT   Id                  :13; // Id Cluster: Da 1 in poi
   BIT   TipoCluster         : 4; // 0  = Non definito
                                  // 1  = A dominanza stazioni breve percorrenza (<  250 Km)
                                  // 2  = A dominanza stazioni lunga Percorrenza (>= 250 Km)
                                  // 3 - 9 : Riservati per linearizzazione dei clusters
                                  // 10 = Specializzato Multistazione
                                  // 11 = Traghetti varii
                                  // 12 = Esclusivamente da Carrozze o servizi diretti
                                  // 15 = Splittato (perche' raggiunto il massimo numero di MV o ...)
   BIT   Id1                 :13; // Id prima stazione dominante
   BIT   Id2                 :13; // Id seconda stazione dominante
   BIT   NumeroMezziVirtuali :10;
   BIT   NumeroMezziVirtualiC:10; // Numero mezzi virtuali Concordi
   BIT   ExtraMezziVirtualiC :10; // Numero records mezzi virtuali Concordi aggiunti per uniformare il periodo
   BIT   NumeroMezziVirtualiD:10; // Numero mezzi virtuali Discordi
   BIT   ExtraMezziVirtualiD :10; // Numero records mezzi virtuali Discordi aggiunti per uniformare il periodo
   BIT   NumeroNodiCambio    : 8; // Comprende: stazioni di cambio (debbono fermarvi treni)
   BIT   NumeroNodi          : 8; // Comprende: Stazioni di cambio e TUTTI i nodi e le stazioni di instradamento eccezionali del grafo
   BIT   NumeroStazioni      :10; // Comprende: Nodi di cambio e di instradamento, fermate
   WORD  NumElementiGruppi      ; // Numero di elementi dei gruppi del cluster
};
class F_CLUSTER_MV : public FILE_FIX {   //    "MY_CLUST.TMP"
   public:
   // Costruttore
   F_CLUSTER_MV(const char* NomeFile,ULONG Size=16000) : FILE_FIX(NomeFile,sizeof(CLUSTER_MV),Size){};
   // Definizione operatori
   CLUSTER_MV&  RecordCorrente(){ return *(CLUSTER_MV*) RecordC; };
   CLUSTER_MV&  operator [](WORD Indice){ Posiziona(Indice); return *(CLUSTER_MV*) RecordC; };

   int Seek(WORD Tgt1,WORD Tgt2, WORD TipoC){ // Torna -1 se non li trova
       for(WORD i = 0; i < NumRecords ; i++){
          CLUSTER_MV& Rec = THIS[i];
          if(Rec.Id1 == Tgt1 && Rec.Id2 == Tgt2 && Rec.TipoCluster == TipoC )return i;
       };
       return -1;
   }
};

//----------------------------------------------------------------------------
// CLUSTER_STAZIONE
//----------------------------------------------------------------------------
// Questa struttura contiene le relazioni tra cluster e stazioni
// Sono compresi i nodi reali (per l' instradamento) e le fermate
// Per le stazioni dei cluster multistazione la distanza e' 0
struct CLUSTER_STAZIONE {
   WORD   IdCluster              ; // KEY Id Cluster: Da 1 in poi
   WORD   IdStazione             ; // KEY Id stazione
   BIT   Id1                   :13;// Id prima stazione dominante
   BIT   Id2                   :13;// Id seconda stazione dominante
   BIT   TipoStazione          : 3;// 1 = Nodo di cambio
                                   // 2 = Nodo
                                   // 4 = Stazione di instradamento eccezionale
                                   // e relative combinazioni binarie
   short  Distanza               ; // Distanza Linearizzata in Km dalla stazione dominante 1 del cluster
                                   // E' calcolata dall' algoritmo di linearizzazione in ML_STEP2
   short  DistanzaMediana        ; // Media tra il minimo e massimo valore della distanza in Km dalla stazione dominante 1 del cluster
   WORD   NumMzvFerm             ; // Numero di mezzi virtuali che FERMANO alla stazione nel cluster
                                   // Puo' essere 0 se e' solo NODO di instradamento
   WORD   Prog                   ; // Progressivo stazione in ambito cluster
                                   // Parte da 0
                                   // E' assegnato ordinando per:
                                   //    Nodo di cambio / Stazione normale
                                   //    Distanza
   WORD   Prog2                  ; // Progressivo stazione in ambito cluster Parte da 0
                                   // E' assegnato ordinando per  Distanza
   GRUPPO Gruppo                 ; // Gruppo di appartenenza della stazione (Nel cluster)

};

class F_CLUSTER_STAZIONE : public FILE_BS {   //    "WRK1.TMP"

   public:

   // Costruttore
   F_CLUSTER_STAZIONE(const char* NomeFile,ULONG Size=6400) : FILE_BS(NomeFile,sizeof(CLUSTER_STAZIONE),4,Size,0,FALSE){ Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Cluster){ return Posiziona(BUFR(&Cluster,sizeof(Cluster))) && KeyEsatta;};
   BOOL Seek(WORD Cluster,WORD Stazione){ ARRAY_ID Target; Target += Cluster;Target += Stazione; return Posiziona((BUFR&)Target) && KeyEsatta;};

   // Definizione operatori
   CLUSTER_STAZIONE&  RecordCorrente(){ return *(CLUSTER_STAZIONE*) RecordC; };
   CLUSTER_STAZIONE&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(CLUSTER_STAZIONE*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return Compare2Word_D(Key1,Key2,DimConfronto);};

};
// Posso accedere anche come un file FIX
class F_CLUSTER_STAZIONE_FIX : public FILE_FIX {   //    "CLUSSTAZ.TMP"
   public:
   // Costruttore
   F_CLUSTER_STAZIONE_FIX(const char* NomeFile,ULONG Size=6400) : FILE_FIX(NomeFile,sizeof(CLUSTER_STAZIONE),Size){};
   // Definizione operatori
   CLUSTER_STAZIONE&  RecordCorrente(){ return *(CLUSTER_STAZIONE*) RecordC; };
   CLUSTER_STAZIONE&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(CLUSTER_STAZIONE*) RecordC; };
};
// E questo e' l' indice ausiliario di accesso in ordine di stazione
struct STAZIONE_CLUSTER {
   WORD    IdStazione             ; // KEY Id stazione
   WORD    IdCluster              ; // Id Cluster: Da 1 in poi
};

class F_STAZIONE_CLUSTER : public FILE_BS {   //    "CLUSSTAZ.IX1"

   public:

   // Costruttore
   F_STAZIONE_CLUSTER(const char* NomeFile,ULONG Size=6400) : FILE_BS(NomeFile,sizeof(STAZIONE_CLUSTER),4,Size,0,FALSE){ Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Stazione){ return Posiziona(BUFR(&Stazione,sizeof(Stazione))) && KeyEsatta;};

   // Definizione operatori
   STAZIONE_CLUSTER&  RecordCorrente(){ return *(STAZIONE_CLUSTER*) RecordC; };
   STAZIONE_CLUSTER&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(STAZIONE_CLUSTER*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return Compare2Word_D(Key1,Key2,DimConfronto);};
};

//----------------------------------------------------------------------------
// MEZZOV_CLUSTER
//----------------------------------------------------------------------------
// Questa struttura contiene le relazioni tra Mezzo virtuale e cluster
struct MEZZOV_CLUSTER {
   WORD    MezzoVirtuale          ; // KEY: Mezzo virtuale
   WORD    IdCluster              ; // Id Cluster: Da 1 in poi
   WORD    OraPartenza            ; // Ora di partenza  del mezzo dalla stazione dominante (o meglio: Max tra ora partenza ed ora arrivo)
   BIT   Id1                   :13; // Id prima stazione dominante
   BIT   Id2                   :13; // Id seconda stazione dominante
   BIT   TipoCluster           : 4; // Vedi CLUSTER_MV
   BIT     Distanza1           :12; // Progressivo Km della stazione dominante 1
   BIT     Distanza2           :12; // Progressivo Km della stazione dominante 2
   BIT     VersoConcorde       : 1; // 1 = Concorde (Distanza2 > Distanza1) 0 = Discorde
   GRUPPO  Gruppo                 ; // Gruppo del mezzo virtuale
};


class F_MEZZOV_CLUSTER : public FILE_BS {   //    "CLUSMEZV.TMP"

   public:

   // Costruttore
   F_MEZZOV_CLUSTER(const char* NomeFile,ULONG Size=16000) : FILE_BS(NomeFile,sizeof(MEZZOV_CLUSTER),2,Size,0,FALSE){ Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD MezzoV){ return Posiziona(BUFR(&MezzoV,sizeof(MezzoV))) && KeyEsatta;};

   // Definizione operatori
   MEZZOV_CLUSTER&  RecordCorrente(){ return *(MEZZOV_CLUSTER*) RecordC; };
   MEZZOV_CLUSTER&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(MEZZOV_CLUSTER*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return CompareWord(Key1,Key2);};

};

// E questo e' l' indice ausiliario di accesso in ordine di cluster verso ed ora di passaggio dal nodo dominante
struct CLUSTER_MEZZOV {
   WORD    IdCluster              ; // Id Cluster: Da 1 in poi
   BYTE    VersoConcorde          ; // 1 = Concorde (Distanza2 > Distanza1) 0 = Discorde
   WORD    OraPartenza            ; // Ora di partenza  del mezzo dalla stazione dominante (o meglio: Max tra ora partenza ed ora arrivo)
   WORD    MezzoVirtuale          ; // Mezzo virtuale
};
class F_CLUSTER_MEZZOV : public FILE_BS {   //    "CLUSMEZV.IX1"
   public:

   // Costruttore
   F_CLUSTER_MEZZOV(const char* NomeFile,ULONG Size=4096) : FILE_BS(NomeFile,sizeof(CLUSTER_MEZZOV),7,Size,0,FALSE){ Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD IdCluster){ return Posiziona(BUFR(&IdCluster,sizeof(IdCluster))) && KeyEsatta;};

   // Definizione operatori
   CLUSTER_MEZZOV&  RecordCorrente(){ return *(CLUSTER_MEZZOV*) RecordC; };
   CLUSTER_MEZZOV&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(CLUSTER_MEZZOV*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0);
};

// ==================================================
// Strutture di lavoro per caricamento dati motore
// ==================================================
// Sono compresi i nodi reali (per l' instradamento) e le fermate
struct CLUSTER_STAZIONE_2 {
   WORD   IdCluster              ; // KEY Id Cluster: Da 1 in poi
   WORD   IdStazione             ; // KEY Id stazione
   short  Distanza               ; // Distanza Linearizzata in Km dalla stazione dominante 1 del cluster
                                   // Calcolata dall' algoritmo di linearizzazione in ml_step2
   short  DistanzaMediana        ; // Distanza Mediana in Km dalla stazione dominante 1 del cluster
   BIT    Ferma               :1 ;
};

class F_CLUSTER_STAZIONE_2 : public FILE_FIX {   //    "WRK1.TMP"

   public:

   // Costruttore
   F_CLUSTER_STAZIONE_2(const char* NomeFile) : FILE_FIX(NomeFile,sizeof(CLUSTER_STAZIONE_2),64000){};

   // Definizione operatori
   CLUSTER_STAZIONE_2&  RecordCorrente(){ return *(CLUSTER_STAZIONE_2*) RecordC; };
   CLUSTER_STAZIONE_2&  operator [](ULONG Indice){Posiziona(Indice); return *(CLUSTER_STAZIONE_2*) RecordC; };

};

//------------------------------------------------------------------------
// Funzioni di gestione FASCE ORARIE
//------------------------------------------------------------------------
// Queste funzioni impostano i bit delle fasce orarie coinvolte
// La fascia di coincidenza va interpretata come l' insieme delle fasce di partenza
// compastibili con una data ora d' arrivo. Si considera un tempo di coincidenza
// minimo di 5 minuti ed un tempo di attesa massimo passato come parametro
// Le ore notturne dall' una alle sei NON sono considerate.
// NB: DEFINITE ANCHE IN MM_PATH.HPP
//------------------------------------------------------------------------

// begin EMS001 VA elimino i __fastcall
WORD FasciaOraria(WORD Ora);
WORD FasceDiCoincidenza(WORD OraDiArrivo, WORD MaxTempoDiCoincidenza);
BYTE NumFasce(WORD Fasce);
// end EMS001

// begin EMS002 VA il corpo della funzione Fascia32 viene portato in ft_lib.cpp causa
// errori di duplicazione
DWORD  Fascia32(WORD Ora);
//DWORD  Fascia32(WORD Ora){ return 1 << (Ora / 45);};
// end EMS002

// Queste per i traghetti (tempo massimo di coincidenza a

// ==================================================
// Collegamenti tra stazioni BIDIREZIONALI
// ==================================================
struct DETT_COLL {
   // Parametri riassuntivi dei collegamenti
   WORD Count           ; // Numero dei treni che realizzano il collegamento
   WORD TcollMin        ; // Tempo di collegamento minimo
   WORD TcollMax        ; // Tempo di collegamento massimo
   WORD KmMinimi        ; // Km minimi necessari per il collegamento
   WORD KmMassimi       ; // Km massimi necessari per il collegamento
   // Questi sono gli orari del collegamento da StazionePartenza a StazioneArrivo
   WORD OrariPartenza   ; // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
   WORD OrariVeriArrivo ; // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
   WORD OrariArrivo     ; // 10 BIT. I BITS non rappresentano l' ora di arrivo ma
                          // gli orari di partenza VALIDI successivi agli orari di arrivo
                          // considerando 180 minuti di tempo massimo di attesa alla stazione.
   WORD OrariArrivo2    ; // Idem , ma considerando i tempi massimi di coincidenza possibili
   // Questi sono in forma estesa: 32 Fasce da 45 minuti: Rappresentano le ore reali di partenza ed arrivo
   DWORD Partenza32     ;
   DWORD Arrivo32       ;

   void Compute(WORD OraPartenza,WORD OraArrivo, WORD Km){
        OrariPartenza         = FasciaOraria(OraPartenza);
        OrariVeriArrivo       = FasciaOraria(OraArrivo);
        OrariArrivo           = FasceDiCoincidenza(OraArrivo,MAX_ATTESA_MINIMA);  // 180 minuti
        OrariArrivo2          = FasceDiCoincidenza(OraArrivo,MAX_ATTESA_MASSIMA); // 360 minuti
        Partenza32            = Fascia32(OraPartenza);
        Arrivo32              = Fascia32(OraArrivo);
        TcollMin = TcollMax   = (32 + (OraArrivo/45) -(OraPartenza/45) ) & 0x1f ;
        KmMinimi = KmMassimi  = Km;
   }
   void Compute2(WORD OraPartenza,WORD OraArrivo,WORD Km){ // per i traghetti
   // NB: Per i traghetti il tempo massimo di coincidenza e' di 12 ore
        OrariPartenza         = FasciaOraria(OraPartenza);
        OrariVeriArrivo       = FasciaOraria(OraArrivo);
        OrariArrivo           = FasceDiCoincidenza(OraArrivo,MAX_ATTESA_TRAGHETTI );
        OrariArrivo2          = FasceDiCoincidenza(OraArrivo,MAX_ATTESA_TRAGHETTI );
        Partenza32            = Fascia32(OraPartenza);
        Arrivo32              = Fascia32(OraArrivo);
        TcollMin = TcollMax   = (32 + (OraArrivo/45) -(OraPartenza/45) ) & 0x1f ;
        KmMinimi = KmMassimi  = Km;
   }
   void operator +=( DETT_COLL & b){
        Count            += b.Count             ;
        OrariPartenza    |= b.OrariPartenza     ;
        OrariVeriArrivo  |= b.OrariVeriArrivo   ;
        OrariArrivo      |= b.OrariArrivo       ;
        OrariArrivo2     |= b.OrariArrivo2      ;
        Partenza32       |= b.Partenza32        ;
        Arrivo32         |= b.Arrivo32          ;
        Top(TcollMax    ,b.TcollMax)            ;
        Top(KmMassimi   ,b.KmMassimi)           ;
        Bottom(TcollMin ,b.TcollMin)            ;
        Bottom(KmMinimi ,b.KmMinimi)            ;
   };
};
struct COLL_KEY1 {
   ID   StazionePartenza;
   ID   StazioneArrivo  ; // Per ogni coppia di stazioni si hanno due records (con partenza ed arrivo invertiti)
};
struct COLLEGAMENTO1 : public COLL_KEY1 , DETT_COLL {

   void Clear(){ZeroFill(THIS); TcollMin = KmMinimi = 9999; };

   void SetKey(ID Id1, ID Id2){
      Clear()                 ;
      Count             = 1   ;
      StazionePartenza  = Id1 ;
      StazioneArrivo    = Id2 ;
   };

};

struct COLL_KEY2 {
   ID   StazionePartenza   ; // Key
   ID   StazioneArrivo     ; // Key
   ID   IdCluster          ; // Key
   BYTE Concorde           ; // Indica se il collegamento e' concorde (1) o discorde (0) con il cluster
};
struct COLLEGAMENTO2 : public COLL_KEY2 , DETT_COLL {

   void Clear(){ZeroFill(THIS);TcollMin = KmMinimi = 9999; };

   void SetKey(ID Id1, ID Id2, ID Clu, BYTE C){
      Clear()                 ;
      Count             = 1   ;
      StazionePartenza  = Id1 ;
      StazioneArrivo    = Id2 ;
      IdCluster         = Clu ;
      Concorde          = C   ;
   };
   void SetKey(COLLEGAMENTO1 & b , ID Clu, BYTE C){
      Clear()                 ;
      Count             = 1   ;
      StazionePartenza  = b.StazionePartenza ;
      StazioneArrivo    = b.StazioneArrivo   ;
      IdCluster         = Clu ;
      Concorde          = C   ;
   };

};
struct COLL_KEY3 {
   ID   IdCluster       ; // KEY
   ID   Stazione        ; // KEY
   BYTE Concorde        ; // KEY Indica se mi riferisco ai collegamenti concordi o discordi con il cluster
};
struct COLLEGAMENTO3 : public COLL_KEY3 , DETT_COLL {
// Questo e' un sommario dei collegamenti per stazione / Cluster
};

class F_COLLEGAMENTO1 : public FILE_BS {   //    "COLLSTAZ.TMP"

   public:

   // Costruttore
   F_COLLEGAMENTO1(const char* NomeFile,ULONG Size=4096) : FILE_BS(NomeFile,sizeof(COLLEGAMENTO1),4,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Id1 ){ BUFR Wrk; Wrk.Store(Id1); return Posiziona(Wrk) && KeyEsatta;};
   BOOL Seek(WORD Id1, WORD Id2){ BUFR Wrk; Wrk.Store(Id1); Wrk.Store(Id2); return Posiziona(Wrk) && KeyEsatta;};

   // Definizione operatori
   COLLEGAMENTO1&  RecordCorrente(){ return *(COLLEGAMENTO1*) RecordC; };
   COLLEGAMENTO1&  FixRec(DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(COLLEGAMENTO1*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return Compare2Word_D(Key1,Key2,DimConfronto);};

};

class F_COLLEGAMENTO2 : public FILE_BS {   //    "COLLCLUS.TMP"

   public:

   // Costruttore
   F_COLLEGAMENTO2(const char* NomeFile,ULONG Size=4096) : FILE_BS(NomeFile,sizeof(COLLEGAMENTO2),6,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Id1 ){ BUFR Wrk; Wrk.Store(Id1); return Posiziona(Wrk) && KeyEsatta;};
   BOOL Seek(WORD Id1  ,WORD Id2 ){ BUFR Wrk; Wrk.Store(Id1);Wrk.Store(Id2); return Posiziona(Wrk) && KeyEsatta;};
   BOOL Seek(WORD Id1, WORD Id2,WORD Clust){ BUFR Wrk; Wrk.Store(Id1);Wrk.Store(Id2); Wrk.Store(Clust); return Posiziona(Wrk) && KeyEsatta;};

   // Definizione operatori
   COLLEGAMENTO2&  RecordCorrente(){ return *(COLLEGAMENTO2*) RecordC; };
   COLLEGAMENTO2&  FixRec(DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(COLLEGAMENTO2*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){
      short * KEY1 = ((short *)Key1);
      short * KEY2 = ((short *)Key2);
      if ( KEY1[0] != KEY2[0] || DimConfronto < 3) return KEY1[0] - KEY2[0];
      if ( KEY1[1] != KEY2[1] || DimConfronto < 5) return KEY1[1] - KEY2[1];
      return KEY1[2] - KEY2[2];
   };
};

struct PERIODICITA_TRENO2 {
    IDTRENO IdentTreno                ; // Key
    T_PERIODICITA Periodicita;
    CCR   CcrDa                       ; // Codice della prima stazione
    CCR   CcrA                        ; // Codice dell'ultima stazione
};
// un mezzo viaggiante puo' avere uno o piu' records che definiscono la periodicita per tratta.

class  FILE_PERIODICITA_TRENO2 : public FILE_BS {   //    "F_PTREN2.TMP"
   public:
   FILE_PERIODICITA_TRENO2(const char* NomeFile,ULONG Size) : FILE_BS(NomeFile,sizeof(PERIODICITA_TRENO2),10,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(IDTRENO & Mezzo_Vg ){ BUFR Wrk; Wrk.Store(&Mezzo_Vg,10); return Posiziona(Wrk) && KeyEsatta;};

   // Definizione operatori
   PERIODICITA_TRENO2&  RecordCorrente(){ return *(PERIODICITA_TRENO2*) RecordC; };
   PERIODICITA_TRENO2&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(PERIODICITA_TRENO2*) RecordC; };

   // L' operatore di confronto di default e' OK perche' non ho problemi di case sensitivity

};

// ========================================================
// Strutture di lavoro per caricamento dati orario su TPF
// ========================================================
struct COPPIA_OD {
   ID    StazioneOrigineID;         // codici DKM
   ID    StazioneDestinazioneID;
   DWORD StazioneOrigineCCR;        // codici CCR
   DWORD StazioneDestinazioneCCR;
};

class  FILE_COPPIA_OD : public FILE_BS {
   public:
   FILE_COPPIA_OD(const char* NomeFile,ULONG Size) : FILE_BS(NomeFile,sizeof(COPPIA_OD), 4,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(ID StazOrg, ID StazDest){ BUFR Wrk; Wrk.Store(StazOrg); Wrk.Store(StazDest); return Posiziona(Wrk) && KeyEsatta;};

   // Definizione operatori
   COPPIA_OD&  RecordCorrente(){ return *(COPPIA_OD*) RecordC; };
   COPPIA_OD&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(COPPIA_OD*) RecordC; };
   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * a,const void* b,ULONG DimConfronto=0){
      COPPIA_OD & A = *(COPPIA_OD*)a;
      COPPIA_OD & B = *(COPPIA_OD*)b;
      if(DimConfronto < 3 || A.StazioneOrigineID != B.StazioneOrigineID) return int(A.StazioneOrigineID) - int(B.StazioneOrigineID);
      return int(A.StazioneDestinazioneID) - int(B.StazioneDestinazioneID);
   };

};

struct PERIODICITA_CARROZZA  {
   IDTRENO IdentTreno                  ; // Key
   char    Segmento                    ; // Segmento di appartenenza
   WORD    Numero                      ; // Numero della carrozza
   T_PERIODICITA Periodicita           ; // Periodicita'
};


class  FILE_PERIODICITA_CARROZZE  : public FILE_BS {
   public:
   // Costruttore
   FILE_PERIODICITA_CARROZZE(const char* NomeFile,ULONG Size) : FILE_BS(NomeFile,sizeof(PERIODICITA_CARROZZA),10,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek( IDTRENO & IdTreno ){ BUFR Wrk; Wrk.Store(&IdTreno,10); return Posiziona(Wrk) && KeyEsatta;};

   // Definizione operatori
   PERIODICITA_CARROZZA&  RecordCorrente(){ return *(PERIODICITA_CARROZZA*) RecordC; };
   PERIODICITA_CARROZZA&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(PERIODICITA_CARROZZA*) RecordC; };

   // Metodi virtuali da ridefinire
//   virtual int  Compare(const void * a,const void* b,ULONG DimConfronto=0){
//   };
};


//----------------------------------------------------------------------------
#endif
