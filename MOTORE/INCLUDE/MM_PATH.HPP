//========================================================================
// MM_PATH : classi per l' identificazione del PATH_CAMBI
//========================================================================
// Gestione della prima fase della soluzione: identificazione di una
// serie di stazioni di cambio per spostarsi dalla origine alla
// destinazione.
//========================================================================
// Attenzione: Cambiando i formati dei files dati del motore in modo da
// renderlo incompatibile, modificare la #define ORARIO_FORMATO_ATTUALE
// Vedere la classe VALIDITA_ORARIO
//========================================================================
//
#ifndef HO_MM_PATH_HPP
#define HO_MM_PATH_HPP

// EMS001
// Sostituitsco tutti i __fastcall con __WIN_FASTCALL
#define __WIN_FASTCALL

//------------------------------------------------------------------------
// ADT : Costo di un path
//------------------------------------------------------------------------
/*
   Il COSTO_PATH deve godere della seguenti essenziali proprieta':
   1) A+B > A & A+B > B
   2) Se A > B  =>  A+C  > B+C e C+A > C+B

   Inoltre deve rispecchiare parametri adeguati:
   - deve tenere conto del numero di cambi, privilegiando i cambi tra
     stazioni collegate da cluster multistazioni
   - deve divenire infinito se si supera il limite di distanza in Km
   - deve tener conto del numero di treni che collegano le due stazioni
   - deve tener conto della distanza in Km

   L' ideale sarebbe gestire separatamente le varie componenti e definire un
   operatore di confronto che rispetti le proprieta' 1 e 2.

   Tuttavia cio' non e' semplice, ed operatori del genere ( che esistono:  si
   pensi all' insieme dei parametri come ad un vettore, all' operatore di somma
   come al prodotto ed al confronto come il confronto tra moduli ) hanno difetti
   sia di poca linearita' che di mischiare, ad ogni somma, i varii parametri in
   gioco.

   Risulta piu' conveniente pertanto combinare subito i parametri in una
   funzione costo cosi' strutturata:

   Costo Singolo Collegamento = Km Collegamento + Ctratte + C_Ms*PesoTratta + Ctreni * Penalita

   Ove:

      - Km collegamento sono i Km minimi necessari per il collegamento
      - Ctratte e' un coefficiente di costo per cambio treno.  es:  40 Km o
        15% della distanza minima su grafo tra origine e destinazione.
        oppure 1000 (se si vuole dare massima importanza al numero di cambi:
        esempio collegamenti tra continente e sardegna)
      - C_MS e' un coefficiente di costo aggiuntivo per collegamenti NON urbani.
        Si applica alle tratte che NON hanno collegamenti urbani;
      - Peso Tratta e' 1 per le tratte normali, 0   per le tratte multistazioni (che hanno
        collegamenti urbani)
      - Ctreni e' un coefficiente di pesatura per disponibilita' treni.  Es:
        10 Km o 5% della distanza minima su grafo tra origine e destinazione.
      - Penalita' e' un coefficiente di penalita' collegato al numero di treni
        che effettuano il collegamento. Va da 0 a 7

   Si deve notare che il costo non diviene infinito se si supera il limite di
   distanza in Km: il limite pertanto andra' gestito a programma in una fase
   ulteriore.

   Il costo del collegamento e' rappresentato in modo esatto solo per le stazioni di cambio:
   per le altre si fa una stima approssimata basata sulle connessioni
   stazione-cluster.
*/
//------------------------------------------------------------------------
// Struttura di collegamento tra nodi_cambio:
//------------------------------------------------------------------------
// Rappresenta i collegamenti BIDIREZIONALI con un altro nodo
// Gli orari NON sono bidirezionali, ma si riferiscono al collegamento
// DAL  NODO_CAMBIO corrente AL NODO_CAMBIO puntato da Id.
class  COLLEGAMENTO { // 18 Bytes  * 25000 collegamenti = 450 K bytes

   public:

   WORD CostoTotale();   // EMS002 Win Tolgo l'inline
   void __WIN_FASTCALL CalcolaCosto(); // Ricalcola il costo del collegamento
   ID Eid()const {return Esterno(Id);}; // External Id

   WORD    Costo;

   protected:

   BIT     Id            : 12       ;   // Id INTERNO del nodo cui sono collegato
   BIT     Km            : 12       ;   // Km MINIMI con cui e' realizzato il collegamento. Max 3000 Km
   BIT     PesoTratta    :  1       ;   // 1 Per i collegamenti normali, 0 se la tratta e' multistazione
   BIT     Privilegiato  :  1       ;   // 1 = Stessa Stazione o simili (piazzale, civitavecchia e messina marittima ...)
   BIT     Penalita      :  3       ;   // Da 0 a 7
   BIT     OrariPartenza : 10       ;   // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
   BIT     OrariArrivo   : 10       ;   // 10 BIT. I BITS non rappresentano l' ora di partenza ma
                                        // gli orari di partenza VALIDI successivi agli orari di arrivo
                                        // considerando 180 minuti di tempo massimo di attesa alla stazione.
   BIT     TcollMin      :  5       ;   // Tempo di collegamento minimo  (Multipli di 45 minuti)
   BIT     TcollMax      :  5       ;   // Tempo di collegamento massimo (Multipli di 45 minuti)
   DWORD   Partenza32               ;   // Orari di partenza
   DWORD   Arrivo32                 ;   // Orari di arrivo

   static WORD Ctratte ;
   static WORD C_MS    ;
   static WORD Ctreni  ;

   friend class NODO_CAMBIO;
   friend class PATH_CAMBI;

   // EMS002 devo mettere questa dichiarazione di friend esplicita perché
   // la dichiarazione di friend non è bidirezionale, nè gode
   // della proprietà transitiva
   friend class DATI_ORARIO_FS;

};
//------------------------------------------------------------------------
// Grafo di connessione dei nodi di cambio
//------------------------------------------------------------------------
class  NODO_CAMBIO {

   public:

   BIT        Id                  : 13  ;  // Id esterno del nodo di cambio
   BIT        ClasseCoincidenza   :  1  ;  // Classe di coincidenza:
                                           // 0 =   Normale (10 Minuti )
                                           // 1 =   Estesa  (15 Minuti )
   BIT        FascePartenza       : 10  ;  // Dieci  BIT: Ognuno indica che nella fascia relativa parte almeno un treno
   BIT        FasceArrivo         : 10  ;  // Idem per indicare che ARRIVA almeno un treno
   BIT        Citta               :  6  ;  // Se > 0 indica una "citta'" di appartenenza del nodo di cambio
   // begin EMS004 Win Problema assegnazione bit fields
   //BIT        NumeroCollegamenti  : 10  ;  // Solo verso altre stazioni di cambio
   WORD       NumeroCollegamenti   ;  // Solo verso altre stazioni di cambio
   // end EMS004

   COLLEGAMENTO * Collegamenti(){return AllCollegamenti +IdxCollegamenti;};
   COLLEGAMENTO * Collegamenti(int i){return AllCollegamenti + (IdxCollegamenti+i);};
   COLLEGAMENTO * CollegamentoDaId(ID Id);

   static BOOL Risolvi(ID Origine,ID Destinazione); // Risolve il problema del minimum path. FALSE su errore.
                                                    // Gli ID sono id Interni

   void TraceId(const STRINGA& Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   protected :


   WORD       IdxClusters               ;  // Appartenenza a clusters (solo se vi sono fermate). I clusters sono sortati per Id
   BYTE       NumeroClusters            ;
   WORD       IdxCollegamenti           ;  // Collegamenti con altri nodi (Indice in AllCollegamenti)
                                           // Dopo l' invocazione di Estendi sono sortati per distanza dalla destinazione

   // Queste variabili per l' algoritmo di generazione dei paths
   BIT        Stato     :2              ;  // 2 = UnLabeled 1 = Labeled 0 = Scanned
   BIT        Esteso    :1              ;  // Vero se ho eseguito il sort dei Primi Vicini
   BIT        HeapPos   :BITS_ND_CAMBIO ;  // Utilizzato dall' heap
   BIT        Ponte     :1              ;  // Vero se il nodo di cambio e' un nodo ponte
   WORD       Costo1                    ;  // Costo Complessivo fino alla stazione di destinazione
   ID         Nodo1                     ;  // Id interno del primo vicino corrispondente

   enum STATUS { SCANNED=0, LABELED=1, UNLABELED=2};
   void Estendi();              // Esegue il SORT dei primi vicini
   ID Iid()const { return Interno(Id);}; // Internal Id

   // Questi dati sono caricati di volta in volta da DATI_ORARIO_FS
   static NODO_CAMBIO        * NodiDiCambio;
   static int                  TotNodiDiCambio;
   static struct STAZN_CLUS  * StzClusters    ; // Relazioni tra stazioni e clusters
   static COLLEGAMENTO       * AllCollegamenti;

   friend class COLLEGAMENTO_DIRETTO;
   friend WORD COLLEGAMENTO::CostoTotale();
   friend class PATH_CAMBI;
   friend class HEAP<NODO_CAMBIO>;
   friend class HEAP2<NODO_CAMBIO>;

   // EMS005 devo mettere questa dichiarazione di friend esplicita perché
   // la dichiarazione di friend non è bidirezionale, nè gode
   // della proprietà transitiva
   friend class DATI_ORARIO_FS;
   friend class SOLUZIONE;

};

inline WORD COLLEGAMENTO::CostoTotale(){ return Costo + NODO_CAMBIO::NodiDiCambio[Id].Costo1; };

//------------------------------------------------------------------------
// NODO_DI_CAMBIO_ESTESO
//------------------------------------------------------------------------
struct NODO_DI_CAMBIO_ESTESO {
   BIT   Id         :13 ;  // Id del nodo di cambio (Interno)
   BIT   Diramabile : 1 ;  // Indica se si puo' diramare al nodo. Un nodo che ha diramato non puo' piu' diramare
   BIT   Livello    :10 ;  // E' l' indice ( nell' array dei collegamenti ) che punta al nodo successivo.
                           // Il percorso piu' breve ha tutti 0 in Livello, i percorsi
                           // diramati hanno valori via via successivi.
   void SetNonDiramabile(ID Id1,WORD Livello1){Id = Id1; Livello = Livello1;Diramabile = 0;};
   void SetDiramabile(ID Id1,WORD Livello1){Id = Id1; Livello = Livello1;Diramabile = 1;};
   void SetDiramabile(ID Id1){Id = Id1; Livello = 0;Diramabile = 1;};
   ID Eid()const {return Esterno(Id);}; // External Id
};
//------------------------------------------------------------------------
// Relazioni Stazioni -> Cluster
//------------------------------------------------------------------------
struct STAZN_CLUS  { // Questo e' un sommario dei collegamenti per stazione / Cluster
   BIT  Stazione       : 13  ; // Key (3 Bytes Totali) E' l' ID Esterno su file, interno quando e' caricato in memoria
   BIT  IdCluster      : 11  ; // Key
   BYTE Progressivo          ; // Progressivo stazione nell' ambito del cluster
   GRUPPO Gruppi             ; // Gruppi della stazione nel cluster
   // Questi dati sono relativi al verso concorde (C) o discorde (D) : Si puo' individuare in base ai progressivi
   BIT  OrariPartenzaC : 10  ; // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
   BIT  OrariPartenzaD : 10  ; // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
   BIT  OrariArrivoC   : 10  ; // 10 BIT. I BITS non rappresentano l' ora di arrivo ma
                               // gli orari di partenza VALIDI successivi agli orari di arrivo
                               // considerando 180 minuti di tempo massimo di attesa alla stazione.
   BIT  OrariArrivoD   : 10  ; // 10 BIT. I BITS non rappresentano l' ora di partenza ma ...
};
//------------------------------------------------------------------------
// COMBINA_CLUSTER
//------------------------------------------------------------------------
#define  COMBINA_CLUSTER_SIZE 32 // Max 32 clusters uniscono due nodi (se no ignoro)
struct COMBINA_CLUSTER {
   BYTE NumClusters;      // Numero di cluster presenti
   BYTE P_Cluster;        // Variabile di algoritmo
   struct CU{
      BYTE   Concorde;
      ID     IdCluster;
      WORD   OrariPartenza;
      WORD   OrariArrivo  ;
   } ClustersUtilizzabili[COMBINA_CLUSTER_SIZE];
   STRINGA CluCorrente()const {return STRINGA(ClustersUtilizzabili[P_Cluster].Concorde ? " ":"#")+STRINGA(ClustersUtilizzabili[P_Cluster].IdCluster);};
};

//------------------------------------------------------------------------
// PATH_ILLEGALE
//------------------------------------------------------------------------
// Strutturina per tenere un PATH che si e' rivelato illegale
struct PATH_ILLEGALE {
   ID Nodi[MAX_TRATTE+1];  // Path (i nodi non usati sono posti a zero)
   void __WIN_FASTCALL Set(const class  PATH_CAMBI & From);
};

//------------------------------------------------------------------------
// PATHS sul grafo di connessione dei nodi di cambio
//------------------------------------------------------------------------
// Si noti che ho scelto di immagazzinare meno dati: ogni volta che diramo debbo ricalcolare
// tutto per identificare la migliore diramazione successiva.
// Sono possibili dei tradeoff tra calcoli da eseguire e dati da immagazzinare.
// Per ridurre i dati conservati ho scelto di trasferire la diramabilita' dal
// PATH generante al generato: pertanto un nodo puo' diramare solo una
// volta da un percorso. L' unico drawback e' un trace meno leggibile.
//------------------------------------------------------------------------
class PATH_CAMBI {

   public:

   NODO_DI_CAMBIO_ESTESO Nodi[MAX_TRATTE+1];  // Percorso
   BYTE                  NumNodi;             // Numero nodi definiti
   BYTE                  Illegale;            // 1 = Ha LOOP; 2 = >MAX_TRATTE cambi; 3 = Troppi Km; 4 = Troppi cambi
                                              // 5 = Km superiori a DistanzaLimite, 6 = Orari incompatibili
                                              // 7 = Simile a PATH precedente senza soluzioni
                                              // 8 = Due stazioni della stessa citta' non consecutive
                                              // 9 = Gestito da un precedente Ponte
                                              // 10= Due tratte mare
   // BYTE                  Attenzione ;      // Per prove estemporanee: Indica che il percorso avrebbe dovuto essere filtrato
   WORD                  Km;                  // Km del percorso
   WORD                  Progressivo;         // Progressivo percorso


   PATH_CAMBI();              // Genera il primo percorso (Il minimo)
   PATH_CAMBI(PATH_CAMBI & From); // Genera un percorso diramando da un percorso precedente
                                  // Il percorso da cui dirama e' aggiornato perche' il nodo di diramazione
                                  //  e' reso NON piu' diramabile e perche' si ricalcola NextCosto.

   static PATH_CAMBI * DiramaNext(); // Genera il prossimo percorso (o NULL se non e' possibile)
                              // La diramazione e' automaticamente aggiunta alla lista dei percorsi
                              // diramabili, se e' diramabile. Il percorso corrente e' tolto dalla
                              // stessa lista se non e' piu' diramabile.

   void __WIN_FASTCALL OrariCompatibili()     ; // Aggiorna Illegale se l' insieme di orari di cambio alle stazioni NON e' compatibile
                                            // Il path e' troncato al nodo che genera l' incompatibilita' (in modo da evitare diramazioni ai nodi successivi)
   void __WIN_FASTCALL CompatibilePrecedenti(); // Aggiorna Illegale se so gia', in base a IPOTESI precedenti, di NON avere soluzioni

   void __WIN_FASTCALL DichiaraIncompatibile(BYTE PosNodoDiCambio); // Mi dice che al nodo indicato NON riesco a trovare treni ADEGUATI verso il nodo successivo

   BOOL PuoAncoraDiramare(){return MiglioreDiramazione != 0XFF;};

   operator STRINGA() const; // Elenco compatto dei nodi e nomi 7
   operator const char *() const{return (CPSZ)STRINGA(THIS);};

   // Imposta Illegale se completando il percorso genera un LOOP o supera il numero delle stazioni accettabili ecc.
   void __WIN_FASTCALL CompletaPercorso();   // Completa il percorso

   void __WIN_FASTCALL CalcolaCostoDiramazione();

   void * operator new(size_t Size){return Allocator.Get();};
   void operator delete(void * Item){Allocator.Free((PATH_CAMBI*)Item);};
   void Put(){HeapPercorsi.Insert(this);};

   void ResetTentativi();

   static ULONG NumPaths; // Numero dei paths esaminati

   static void ImpostaProblema(ID EOrigine, ID EDestinazione, DATI_ORARIO_FS * Orar); // Sono Id esterni, Imposta anche per le classi collegate

   static BOOL RelazioneInUnaSolaCitta(){return NodoC1 == NodoC2 && NodoC1 != 0xff;}; // Indica che non posso usare i collegamenti urbani

   static BYTE           IsTargetAutoChk ;    // Utilizzata per autodiagnostica

   private:

   BOOL IsTarget(); // Per autodiagnostica

   static BYTE NodoC1       ; // Nodo-citta' della stazione origine oppure 0xff
   static BYTE NodoC2       ; // Nodo-citta' della stazione destinazione oppure 0xff

   BYTE  MiglioreDiramazione; // Indice del nodo cui diramare. 0xff se non posso piu' diramare dal percorso
   WORD  NextCosto          ; // Costo del path che avrei alla prossima diramazione (Diramando a MiglioreDiramazione);
                              // 0xFFFF se non posso piu' diramare


   static ID Origine      ; // Id Interno dell' Origine
   static ID Destinazione ; // Id Interno della destinazione

   static WORD MinKm      ; // Minimi Km di un percorso CHE DIA ORIGINE A SOLUZIONI NON SCARTATE
   static WORD LimKm      ; // Limite Km dei percorsi validi

   // Per ora non pongo limiti sul numero dei cambi

   static ALLOCATOR<PATH_CAMBI> Allocator;
   static HEAP<PATH_CAMBI>      HeapPercorsi;
   static WORD                  LastCosto; // Costo dell' ultimo percorso completato
   static COMBINA_CLUSTER       IdxClusters[MAX_TRATTE];    // Puntamento ai clusters utilizzabili sul path corrente
   static BOOL                  StopCombinazione;           // Indica che non ho altre combinazioni
   static HASH<PATH_ILLEGALE>   HashIllegali;

   friend class DATI_ORARIO_FS;
   friend class CLU_BUFR;
   friend inline int HEAP<PATH_CAMBI>::Confronta( PATH_CAMBI * Uno, PATH_CAMBI * Due);

};

inline int HEAP<PATH_CAMBI>::Confronta( PATH_CAMBI * Uno, PATH_CAMBI * Due){  // Per la heap
   return int(Uno->NextCosto) - int(Due->NextCosto);
};

//------------------------------------------------------------------------
// Funzione di gestione FASCE ORARIE
//------------------------------------------------------------------------
WORD __WIN_FASTCALL FasciaOraria(WORD Ora);
WORD __WIN_FASTCALL FasceDiCoincidenza(WORD OraDiArrivo, WORD MaxTempoDiCoincidenza);

//------------------------------------------------------------------------
// Queste classi solo per il caricamento dati da file
//------------------------------------------------------------------------
struct COLLEGAMENTO_F {
   BIT  StazionePartenza : 13       ;
   BIT  StazioneArrivo   : 13       ;   // Per ogni coppia di stazioni si hanno due records (con partenza ed arrivo invertiti)
   BIT  PesoTratta       :  1       ;   // 1 Per i collegamenti normali, 0 se la tratta e' multistazione
   BIT  Privilegiato     :  1       ;   // 1 = Stessa Stazione o simili (piazzale, civitavecchia e messina marittima ...)
   BIT  Count            :  5       ;   // Numero dei treni che realizzano il collegamento (max 31);
   BIT  KmMinimi         : 12       ;   // Km minimi necessari per il collegamento
   BIT  OrariPartenza    : 10       ;   // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
   BIT  OrariArrivo      : 10       ;   // 10 BIT. I BITS non rappresentano l' ora di arrivo ma
                                        // gli orari di partenza VALIDI successivi agli orari di arrivo
                                        // considerando 180 minuti di tempo massimo di attesa alla stazione.
   // Questi sono in forma estesa: 32 Fasce da 45 minuti: Rappresentano le ore reali di partenza ed arrivo
   DWORD Partenza32                 ;
   DWORD Arrivo32                   ;
   BIT   TcollMin        :  5       ;   // Tempo di collegamento minimo  (Multipli di 45 minuti max 24 ore)
   BYTE  TcollMax        :  5       ;   // Tempo di collegamento massimo (Multipli di 45 minuti max 24 ore)
};

class FIX_COLLEGAMENTO_F : public FILE_FIX {    // "MM_COLL.DB"
   public:
   FIX_COLLEGAMENTO_F(const char* NomeFile,ULONG Size=4096) : FILE_FIX(NomeFile,sizeof(COLLEGAMENTO_F),Size){};
   COLLEGAMENTO_F&  RecordCorrente(){ return *(COLLEGAMENTO_F*) RecordC; };
   COLLEGAMENTO_F&  operator [](DWORD Indice){Posiziona(Indice) ;return *(COLLEGAMENTO_F*) RecordC; };
};
class F_STAZN_CLUS : public FILE_BS { // "MM_STCLU.DB"

   public:

   // Costruttore
   F_STAZN_CLUS(const char* NomeFile,ULONG Size=2000) : FILE_BS(NomeFile,sizeof(STAZN_CLUS),3,Size,0,FALSE){ Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Stazione){ return Posiziona(BUFR(&Stazione,sizeof(Stazione))) && KeyEsatta;};

   // Definizione operatori
   STAZN_CLUS&  RecordCorrente(){ return *(STAZN_CLUS*) RecordC; };
   STAZN_CLUS&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(STAZN_CLUS*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){
      STAZN_CLUS  & A = *(STAZN_CLUS *) Key1;
      STAZN_CLUS  & B = *(STAZN_CLUS *) Key2;
      if(A.Stazione != B.Stazione || DimConfronto < 3) return (int)A.Stazione - (int)B.Stazione;
      return (int)A.IdCluster - (int)B.IdCluster ;
   }
};
// Questi sono i dati delle stazioni dipendenti da orario
// Sono definiti per tutte le stazioni su cui fermano treni
struct STAZIONE_F {
   WORD Id                        ;  // ID Esterno
   BIT  FascePartenza        : 10 ;  // Dieci  BIT: Ognuno indica che nella fascia relativa parte almeno un treno
                                     // Le fasce sono le stesse utilizzate nel caricamento dati
   BIT  FasceArrivo          : 10 ;  // Idem per indicare che ARRIVA almeno un treno
   BIT  ClasseCoincidenza    : 1  ;
   BIT  Citta                : 6  ;  // Se > 0 indica una "citta'" di appartenenza della stazione
   BIT  Peso                 : 14 ;  // Numero fermate nella stazione * 10
   BIT  TipoStazione         : 2  ;  // 2 = Nodo 1 = Nodo di cambio, 0 = stazione

   BOOL StazioneDiCambio(){return TipoStazione & 1;};
   BOOL StazioneNodale(){return TipoStazione & 2;};
};

class BS_STAZIONE_F : public FILE_BS  {   //"MM_STAZN.DB"
   public:
   BS_STAZIONE_F(const char* NomeFile,ULONG Size=4096) : FILE_BS(NomeFile,sizeof(STAZIONE_F),2,Size,0,FALSE){Init();};
   STAZIONE_F&  RecordCorrente(){ return *(STAZIONE_F*) RecordC; };
   STAZIONE_F&  operator [](DWORD Indice){Posiziona(Indice) ;return *(STAZIONE_F*) RecordC; };
   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(ID Stazione){ BUFR Wrk; Wrk.Store(Stazione); return Posiziona(Wrk) && KeyEsatta;};
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto ){ return CompareWord(Key1,Key2);};
};

#endif
