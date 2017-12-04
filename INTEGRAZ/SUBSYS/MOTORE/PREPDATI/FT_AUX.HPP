//----------------------------------------------------------------------------
// FT_AUX.HPP : Contiene le strutture necessarie per il caricamento dei 
// dati del motore a partire dai files T
//----------------------------------------------------------------------------

#ifndef  HO__ML_BASE
#include "ML_BASE.HPP"
#endif


// -----------------------------------------------------------------------
// Strutture per identificare i mezzi virtuali a partire dalle carrozze dirette (programma FT_STEPA )
// -----------------------------------------------------------------------
// NB: I servizi sono approssimati per eccesso: Va fatto un confronto con i servizi effettivi
// del mezzo virtuale in particolare per quanto riguarda la prima e la seconda classe
// -----------------------------------------------------------------------
// Questa struttura per lavorare in memoria: carica i dati di INFOCOMM
struct  NOTA_SERVIZI {
   IDTRENO IdentTreno                    ; // Identificativo Treno  (Key)
   WORD    IdNota                        ; // Identificatore Nota   (Key). 
   CCR     StazioneInizioServizio        ; // Codice CCR stazione DEL TRENO in cui ha inizio il servizio
   CCR     StazioneFineServizio          ; // Codice CCR stazione DEL TRENO in cui termina il servizio (0 se interessa la sola stazione di partenza)
   IDTRENO TrenoInizioServizioSeMV       ; // E relativo treno (ignorare i *INT , *EST)
   IDTRENO TrenoFineServizioSeMV         ; // E relativo treno (ignorare i *INT , *EST)
   char    ServizioDeiTreniProvenODest   ; // Il record attribuisce un servizio ai treni provenienza e/o destinazione
   T_PERIODICITA Periodicita             ; // Periodicit… (riferita alla partenza del treno e non alla stazione di cambio)
   FULL_INFO Servizi                     ; // Servizi della carrozza diretta desunti dalla nota
   void Init(){ ZeroFill(THIS); Servizi.Clear(); Periodicita.ReSet(); Periodicita.Set(T_PERIODICITA::Inizio_Dati_Caricati, T_PERIODICITA::Fine_Dati_Caricati); };
};
// Questa struttura tiene i dati dei mezzi virtuali identificati in FT_STEPA
struct  COMPOS_MEZZOVI {
   struct MC {
      IDTRENO IdentTreno         ; // Identificativo Treno  (Key)
      CCR     StazioneDiCambio   ; // Verso il successivo: Codice CCR o 00000 se non identificata
      BYTE    ShiftPartenza      ; // Indica quanti giorni dopo la partenza del mezzo virtuale ( = del primo mezzo componente) si parte 
                                   // NB:: Il treno potrebbe anche essere partito il giorno prima.
   } MezziComponenti[MAX_MEZZI_REALI];
   WORD NumMezziComponenti       ;
   WORD MezzoVirtuale            ; // Mezzo virtuale : Identificatore numerico
   BIT  Dupl                   :1; // Se true vuol dire che e' duplicato : non considerare al fine della
                                   // costruzione dei MV, ma considerare per i servizi
   BIT  DaCarrozzeDirette      :1; // Se TRUE vuol dire che esiste una carrozza diretta che da origine al mezzo virtuale
   BIT  DaAltriServizi         :1; // Se TRUE vuol dire che esistono altri servizi che implicano la prosecuzione
   BIT  DaOrigineDestino       :1; // Se TRUE vuol dire che uno dei treni continua nell' altro
   BIT  CambiDeboli            :3; // Numero di cambi deboli (vedi FT_STEPA.CPP).
   BIT  InLoop                 :1; // Indica che vi sono treni presenti piu' volte
   T_PERIODICITA Periodicita     ; // E' la periodicita' con cui circolano TUTTI i mezzi componenti
   FULL_INFO Servizi             ; // Servizi della carrozza diretta
                                   // NB: Il dato comunque al momento non viene utilizzato:
                                   // Ricalcolo i servizi a partire dai servizi treno perchŠ piu' affidabile
                                   // Tuttavia in futuro potrebbe essere necessario: Non e' del tutto esatto 
                                   // infatti che avendo un servizio su due treni in prosecuzione lo abbia anche
                                   // sul mezzi virtuale composito. Il dato potrebbe inoltre tornare
                                   // utile per il caricamento dati host.
   void Clear(){ZeroFill(THIS); Servizi.Clear();};
   STRINGA Ident() { STRINGA Msg; for(int i = 0; i < NumMezziComponenti; i++) Msg += STRINGA(St(MezziComponenti[i].IdentTreno)).Strip()+"-"; Msg[Msg.Dim()-1] = ' ';return Msg;};
};
// Questo file e' sortato per id dei treni in composizione
class  FILE_COMPOS_MEZZOVI : public FILE_FIX {   //    "COMPMZVL.SV1"
   public:
   FILE_COMPOS_MEZZOVI(const STRINGA& NomeFile = PATH_OUT "COMPMZVL.SV1", ULONG BufSize=64000):
   FILE_FIX(NomeFile,sizeof(COMPOS_MEZZOVI),BufSize){};
   COMPOS_MEZZOVI &  operator [](ULONG Indice){ Posiziona(Indice); return *(COMPOS_MEZZOVI*) RecordC; };
   COMPOS_MEZZOVI &  RecordCorrente(){ return *(COMPOS_MEZZOVI*) RecordC; };
};

//----------------------------------------------------------------------------
// PTRENO: Classe per catalogare i dati dei treni
//----------------------------------------------------------------------------
// NB: L' HASH accede per id treno, il file per numero treno (-1 per avere il corretto numero record).
// Cio' permette di usare lo stesso file per entrambi i tipi di accessi.
struct PTRENO {
   IDTRENO       IdTreno                           ;     // Identificativo treno ( a 10 caratteri ) : Univoco
   char          KeyTreno                      [5] ;     // Identificativo treno ( a 5  caratteri ) : Univoco per i prenotabili
   CCR           CcrDa                             ;     // Codice della prima stazione del treno
   CCR           CcrA                              ;     // Codice dell'ultima stazione del treno
   IDTRENO       TProv                             ;     // Eventuale Treno provenienza
   IDTRENO       TDestinazione                     ;     // Eventuale Treno destinazione
   char          NomeMezzoViaggiante          [20] ;     // Opzionale
   T_PERIODICITA PeriodicitaTreno                  ;     // Periodicita' (alla partenza)
   FLAGS_TRENO   Flags                             ;     // Flags di classe e supplementi
   MM_INFO       ServiziTreno                      ;     // Servizi del treno & Tipo mezzo codificato 
   MM_INFO       ServiziUniformiTreno              ;     // Servizi UNIFORMI del treno & Tipo mezzo codificato  
   WORD          OraPartenzaTreno                  ;     // E' l' ora di partenza alla prima stazione (anche se solo transito)
   BIT           PLocalita                     :20 ;     // Punta all' inizio del treno nell' archivio Localita'
   BIT           PTaborariR2                   :20 ;     // Punta al record 2 in TabOrari
   BIT           PInfocomm                     :20 ;     // Punta al record 1 in INFOCOMM
   BIT           NumeroTreno                   :17 ;     // Posizione progressiva in TABORARI+1. E' la chiave di accesso al file
   BIT           TipoCambio                    : 3 ;     // 0 = Nessuna prosecuzione
                                                         // 1 = Treno prosegue nel successivo
                                                         // 2 = Treno termina e si inserisce a meta' del successivo
                                                         // 3 = Treno prosegue e successivo inizia
                                                         // 4 = Scambio carrozze ad X (NON GESTITO) od altri errori
                                                         // 6 = Cambio da carrozze dirette
                                                         // 7 = Non Noto
   BIT           NumProvenienza                :  4 ;    // Numero di treni per i quali il treno corrente e' treno provenienza
   BIT           NumDestinazione               :  4 ;    // Numero di treni per i quali il treno corrente e' treno destinazione
   BIT           NumProvenienzaDiTesta         :  4 ;    // .. L' inserimento avviene in effetti in prosecuzione
   BIT           NumDestinazioneDiTesta        :  4 ;    // .. L' inserimento avviene in effetti in prosecuzione 
   BIT           NumVirtuali                   :  5 ;    // Numero dei virtuali in cui compare il treno.
   
   // Questi sono dei flags che indicano la presenza di particolari note
   BIT           HaNote                        :  1 ;    // Ha delle note su INFOCOMM
   BIT           HaNote_ServiziODiInstradamento:  1 ;    // Ha delle note che indicano servizi o instradamento ( ... )
   BIT           HaNote_Altre                  :  1 ;    // Ha altri tipi di note ( ... )
   BIT           HaNota_02_Ealtre              :  1 ;    // Ha la nota 02 (Invalidi, specifica)
   BIT           HaNota_04                     :  1 ;    // Ha la nota 04 (Comporta WL )
   BIT           HaNota_07                     :  1 ;    // Ha la nota 07 (Carrozze dirette )
   BIT           HaNota_08                     :  1 ;    // Ha la nota 08 (Invalidi, generica)
   BIT           HaNota_28                     :  1 ;    // Ha la nota 28 (Comporta posti a sedere di sola 2ø classe)
   BIT           HaNota_30                     :  1 ;    // Ha la nota 30 (Comporta auto al seguito)
   BIT           HaNota_54                     :  1 ;    // Ha la nota 54 (Comporta cuccette 2ø )
   BIT           HaNota_55                     :  1 ;    // Ha la nota 55 (Comporta non posti a sedere ... )
   BIT           HaNote_53_ed_altre            :  1 ;    // Ha una delle note che indicano la mancanza dei posti a sedere
   BIT           HaNote_Altre_AutoAlSeguito    :  1 ;    // Ha delle note oltre la 30 che comportano auto al seguito
   BIT           HaNote_Altre_WL               :  1 ;    // Ha delle note oltre la 4  che comportano WL
   BIT           HaNote_Altre_CC               :  1 ;    // Ha delle note oltre la 54 che comportano cuccette
                                                      
   void Clear();                                      
                                                      
   FULL_INFO Servizio(class FILE_INFOCOMM & InfoComm)     ;     // Identifica i servizi corrispondenti alla nota correntemente puntata su INFOCOMM

   // NB: I due metodi seguenti aprono i files localita e taborari. Saranno chiusi dalla funzione Save()
   // o da una chiamata a CercaCCR con CCR < 0
   short int OraPartenza(const CCR & Fermata)       ;     // Torna l' ora di partenza da una data fermata
                                                          // -1 se la fermata non appartiene al treno o se il treno
                                                          // non ammette viaggiatori in partenza dalla fermata
                                                          // NB: Se Fermata == CcrDa torna l' ora di partenza
                                                          // del treno anche se la stazione non e' una fermata.
   short int OraArrivo(const CCR & Fermata)         ;     // Torna l' ora di arrivo da una data fermata
                                                          // -1 se la fermata non appartiene al treno o se il treno
                                                          // non ammette viaggiatori in arrivo alla fermata

   // Motodi di classe                                
   static PTRENO * Get(IDTRENO & IdT)               ;     // Ritorna tutti i dati
   static T_PERIODICITA GetPerio(IDTRENO & IdT)     ;     // Periodicita' del treno
   static T_PERIODICITA Get2(IDTRENO & IdT,          
                             T_PERIODICITA & Base)  ;     // La periodicita' 'Base' viene corretta con quella del treno
   static void Load()                               ;     // Carica la hash a partire dai files .T
   static void Save()                               ;     // Salva i dati della hash su di un file temporaneo
   static void Restore()                            ;     // Ristora i dati della hash a partire dal file temporaneo

   static HASH<PTRENO>   HashPt                     ;


   // Questo metodo scandisce Localita e TabOrari ed identifica, se possibile, 
   // la stazione con un dato CCR.
   // Sono tornati i numeri di record in Localita' e Taborari, se trovato il CCR
   // altrimenti la funzione torna FALSE
   // I files sono aperti alla prima chiamata, per chiuderli chiamare il metodo
   // con CCR < 0 ;
   BOOL CercaCCR(int CCR, int & PLoc, int & PTab);

   private:

   void    HaNota(ID Nota)                          ;     // Registra la presenza di una nota
   void    FineNote()                               ;     // Completa la determinazione dei servizi del treno

};

// Questo file e' scritto dal metodo Save() di PTRENO
// Uso un file FIX e non un file BS perche' la chiave e' semplicemente eguale al numero record + 1
struct FILE_PTRENO  : public FILE_FIX  {                // "PTRENO.TMP"
   FILE_PTRENO(int Size = 2048): FILE_FIX(PATH_OUT  "PTRENO.TMP",sizeof(PTRENO),Size){}; // Record piccoli per permettere accessi random veloci
   PTRENO&  operator [](int Rec){ Posiziona(Rec); return *(PTRENO*) RecordC; }; 
   PTRENO&  RecordCorrente(){ return *(PTRENO*) RecordC; };
};

//----------------------------------------------------------------------------
// Questra struttura contiene i servizi, non uniformi, di treno
//----------------------------------------------------------------------------
// Sono salvati nel caricamento di PTRENO e poi completati da FT_STEPA.
// NB: I flags di disuniformita' non sono impostati in quanto sul
// file sono riportati per definizione SOLO i servizi non uniformi
struct SERVNU {
   IDTRENO       IdTreno                          ;     // Identificativo treno ( a 10 caratteri ) : Univoco
   FULL_INFO     Servizi                          ;     // Servizi
   void Clear(){ BlankFill(IdTreno); Servizi.Clear();};
};
struct FILE_SERVNU : public FILE_BS  {                // "SERVNU.TMP"
   FILE_SERVNU(int Size=2048): FILE_BS(PATH_OUT  "SERVNU.TMP",sizeof(SERVNU),sizeof(IDTRENO),Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(IDTRENO & IdTreno){ BUFR Wrk; Wrk.Store(IdTreno,sizeof(IdTreno)); return Posiziona(Wrk) && KeyEsatta;};
   SERVNU&  FixRec(int Rec){ Posiziona(Rec); return *(SERVNU*) RecordC; };
   SERVNU&  RecordCorrente(){ return *(SERVNU*) RecordC; };
};

//----------------------------------------------------------------------------
// Questra struttura contiene le note commerciali di treno
//----------------------------------------------------------------------------
// Sono salvate nel caricamento di PTRENO 
struct NOTA_CO {
   IDTRENO       IdTreno                          ;     // Identificativo treno ( a 10 caratteri ) : Univoco
   FULL_NOTA     Nota                             ;     // Nota commerciale
   void Clear(){ BlankFill(IdTreno); Nota.Clear();};
};
struct FILE_NOTA_CO : public FILE_BS  {                // "NOTA_CO.TMP"
   FILE_NOTA_CO(int Size=2048): FILE_BS(PATH_OUT  "NOTA_CO.TMP",sizeof(NOTA_CO),sizeof(IDTRENO),Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(IDTRENO & IdTreno){ BUFR Wrk; Wrk.Store(IdTreno,sizeof(IdTreno)); return Posiziona(Wrk) && KeyEsatta;};
   NOTA_CO&  FixRec(int Rec){ Posiziona(Rec); return *(NOTA_CO*) RecordC; };
   NOTA_CO&  RecordCorrente(){ return *(NOTA_CO*) RecordC; };
};
