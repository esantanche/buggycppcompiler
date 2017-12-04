//========================================================================
// motglue: definizioni che debbono essere allineate tra motore e sua interfaccia esterna
//========================================================================
//
#ifndef HO_MOTGLUE_HPP
#define HO_MOTGLUE_HPP

#ifndef HO_STAZIONI_HPP
#include "id_stazi.hpp"
#endif

#ifndef HO_OGGETTO_H
#include "oggetto.h"
#endif

// Numero massimo di tratte, comprensivo dei trasporti urbani
// Ogni tratta corrisponde ad un mezzo (virtuale) di trasporto
// Se si va oltre le 8 tratte modificare la struttura INFO_NOTE
#define MAX_TRATTE  8

#define  MAX_MMP_BYTES 32 // Numero di bytes da riservare per MM_PERIODICITA: deve essere multiplo di 4

// begin EMS Win
// Inserita questa struttura da MOTGLUE.HPP di \integraz\fasatura
// per compilare FP_ZERO
//#if 0
//----------------------------------------------------------------------------
// Classifiche dei treni
//----------------------------------------------------------------------------
// Questi dati sono aggiornati con il caricamento dei dati del GRAFO
// I codici delle categorie sono parzialmente stabili, nel senso che
// debbono permanere almeno per un orario dopo che la categoria Š stata soppressa,
// e poi possono essere riutilizzati.
// Il codice 0 corrisponde a treni di cui non e' fornita la categoria,
// i codici da 61 a 63 sono riservati per uso del motore con il seguente significato:
// 63 = Collegamento urbano
// 61,62 = Riservato per future espansioni
// Gli altri codici sono caricati su file durante il caricamento dei dati del grafo
// e vengono decodificati dalle relative funzioni.
// NB: I Codici corrispondono univocamente ai caratteri del font con i codici
// da 193 a 255
//----------------------------------------------------------------------------
#define COLLEGAMENTO_URBANO 63
struct _export CLASSIFICA_TRENI {
     BYTE     Classifica                      ; // Solo i valori 0-63 sono validi
     char     Descrizione                 [16]; // Descrizione Classifica
     BIT      TempoRidottoCoincidenza       :1; // * = Regionale, metropolitano ecc.
     BIT      Fumatori                      :1; // * = Si                           
     BIT      Navale                        :1; // * = Traghetto, monocarena ecc
     BIT      SicuramentePrenotabile        :1; // * = Pendolini, IC/EC ...
     BIT      SicuramentePrenotazioneObbli  :1; // * = Pendolini
     BIT      SupplementoIC                 :1; // * IC
     BIT      SupplementoEC                 :1; // * EC / EN ...
     BIT      SupplementoETR                :1; // * ETR, Eurostar
     BIT      TipoConcesso                  :1; // * = Treno di ferrovia concessa
	  BIT      TipoInternazionale            :1; // * = Treno di tipologia non italiana

     static CLASSIFICA_TRENI * Classifiche;
     static void CaricaClassifiche(const STRINGA & PathDati);
     static void LiberaClassifiche();
     // Questa funzioncina d… il carattere del font corrispondente
     // alla classifica Nesima ( 0 = CLASSIFICA NON VALIDA)
     static char FontChar(int i){
        if(i == 0)return 192;
        if(i > 63)return 0;
        if(Classifiche[i].Classifica == i)return 192 + i;
        return 0;
     };
};

inline const CLASSIFICA_TRENI & Classifica(BYTE Classif)
{
	return CLASSIFICA_TRENI::Classifiche[Classif];
};

//#endif
// end EMS Win

//----------------------------------------------------------------------------
// MM_INFO: Struttura generale di informazione su mezzo virtuale viaggiante
//----------------------------------------------------------------------------
// NB: Traghetti e monocarena hanno servizi IMPLICITI
// I posti ponte ecc. sono considerati posti a sedere
// Tutti i bit sono settati a livello di soluzione se sono settati anche su una sola
// delle tratte componenti(a percorso e data fissati).
// Tutti i bit sono settati a livello di tratta se sono settati anche su uno solo
// dei mezzi viaggianti componenti.
// I bit di disuniformita sono settati a livello di tratta se cambiano per data o a seconda
// delle stazioni di partenza / arrivo o a seconda del mezzo viaggiante componente (non visibile)
// Il tipo mezzo e' invece composto per dominanza: cioe' prevale il piu' "Pregiato"
//----------------------------------------------------------------------------
struct  _export MM_INFO {          // Informazioni generali su mezzo virtuale viaggiante

 // Avvertenze generali : Non si applica il concetto di uniformita'
      BIT HaNoteGeneriche      :1;  //     Ha delle note generiche associate  (Da TABORARI o alcune di INFOCOMM)
      BIT HaNoteDiVendita      :1;  //     Ha delle note di vendita associate (Da INFOCOMM)

 // Servizi di prenotazione.
      BIT PrenotDisUniforme    :1;  //     Se TRUE cambiano per TRATTA e/o per periodicita'.
      BIT Prenotabilita        :1;  // 3 : Set su tutto cio' che e' prenotabile in TPN, TPF o sistemi HOST non FS
      BIT PrenObbligItalia     :1;  // 4 : Prenotazione obbligatoria in Italia
      BIT PrenObbligEstero     :1;  // 5 : Prenotazione obbligatoria all' estero
      BIT PrenotabileSolo1Cl   :1;  // 6 : Solo la prima classe e' prenotabile

 // Servizi di trasporto (se non uniformi per venderli si deve accedere all' informativa di dettaglio
 // per vedere su quali tratte siano abilitati);
      BIT ServTraspDisUniformi :1; //     Se TRUE cambiano per TRATTA e/o per periodicita'.
      BIT ServizioBase         :1; // 8 : Cuccette, posti a sedere o quant' altro estesa alla generalita' dei passeggeri
		BIT PostiASederePrima    :1; // 9 :
      BIT PostiASedereSeconda  :1; // 10:
      BIT SleeperettePrima     :1; // 11:
      BIT SleeperetteSeconda   :1; // 12:
      BIT CuccettePrima        :1; // 13:
      BIT CuccetteSeconda      :1; // 14:
      BIT VagoniLettoPrima     :1; // 15:
      BIT VagoniLettoSeconda   :1; // 16:
      // Questi quattro di seguito vanno combinati in AND e non in or
      BIT AutoAlSeguito        :1; // 17:
      BIT Invalidi             :1; // 18:
      BIT Biciclette           :1; // 19:
      BIT Animali              :1; // 20:

 // Servizi generici
      BIT ServGenerDisUniformi :1; //     Se TRUE cambiano per TRATTA e/o per periodicita'.
      BIT Ristoro              :1; // 22:
      BIT BuffetBar            :1; // 23: Oppure il nuovo servizio RistoBar
      BIT SelfService          :1; // 24:
      BIT Ristorante           :1; // 25:
      BIT Fumatori             :1; // 26:

      BIT ClassificaDisUniforme:1;  //     Se TRUE vuol dire che la classifica cambia a seconda della tratta
      BIT Disuniforme          :1;  //     TRUE Se ho una qualsiasi disuniformita'
      BIT Filler               :3;  //     Porta a 32 bits : Necessario per il funzionamento dei metodi

		BIT TipoMezzo            :5;  //     Classificazione del mezzo virtuale: Vedi enum TIPO_MEZZO

  // L' ordine e' per predominanza: cioe' su treni virtuali misti
  // viene considerata come tipologia dominante quella con valore piu' alto
  // dell' enum
  // ***************************************
  // Attenzione
  // ***************************************
  // Introducendo nuovi tipi mezzo navali, o sempre prenotabili modificare le routine definite
  // di seguito e ricompilare (In particolare il caricamento dati)
  enum TIPO_MEZZO {
     NON_NOTO              , // Non ho ulteriori informazioni
     MEZZO_FITTIZIO        , // Non noto ne vendibile.
     COLLEGAMENTO_URBANO   , // Di cluster multistazioni
     AUTOBUS               ,
     FUNIVIA               ,
     TRENO_Metropolitano   ,
     TRENO_Diretto         ,
     TRENO_Regionale       ,
     TRENO_Espresso        ,
     TRENO_Treno           , // Per le ferrovie concesse
     TRENO_Locale          , // Per le ferrovie concesse
     TRENO_Accelerato      , // Per le ferrovie concesse
     TRENO_Direttissimo    , // Per le ferrovie concesse
     TRENO_Interregionale  ,
     TRENO_Intercity       ,
     TRENO_Eurocity        ,
     TRENO_Euronight       ,
     TRENO_Pendolino       ,
     MONOCARENA            ,
	  TRAGHETTO             ,
     ALISCAFO              ,
     TRENO_CIS             ,
     TRENO_ETR500          ,
	  TRENO_Eurostar        ,
     TRENO_InterCityExpres ,
     TRENO_InterCityNight  ,
     TRENO_TGV             ,
     TAPPO_CLASSIFICHE        // Primo valore non valido
     };

     static ELENCO_S Decod_TIPO_MEZZO;
     static TIPO_MEZZO EncodTipoMezzo(STRINGA & Categoria); // Anche categoria puo' essere MODIFICATA se e' vuota (Viene messo NON_NOTO)
     static STRINGA    DecodTipoMezzo(USHORT Categoria);

     // ............................................
     // Queste api classificano in parte i tipi mezzo.
     // ............................................
     // Attenzione: si tenga presente che, ove la classifica del mezzo non fosse uniforme, 
     // questi metodi potrebbero fornire un risultato errato.
     BOOL Regionale()             ;
     BOOL Navale()                ;
     BOOL LettiOCuccette()        ;
     BOOL SicuramentePrenotabile();
     BOOL SupplementoIC()         ;
     BOOL SupplementoETR()        ;
     BOOL QualitaEsteri()         ;

     // ............................................
     // Accesso ai servizi via numero
     // ............................................
     // Dalla versione di fine Aprile 97 i servizi possono essere acceduti anche via 
     // un "numero" inferiore a 31
     // Non per tutti i numeri e' definito un servizio.
	  // E' un modo ALTERNATIVO per interpretare e manipolare i dati di MM_INFO
     // specialmente nei programmi di caricamento dati
     // I numeri corrispondono a quanto sopra indicato 
     // ............................................
     static BOOL IsServizio(BYTE Num);         // Indica se esiste il servizio Num
     BOOL TestServizio(BYTE Num) const;        // Indica se ho il servizio Num; Torna FALSE se non e' definito il servizio corrispondente al numero 
     void SetServizio(BYTE Num);               // Set del servizio Num; Non fa nulla se non e' definito il servizio corrispondente al numero
     void ReSetServizio(BYTE Num);             // ReSet del servizio Num; Non fa nulla se non e' definito il servizio corrispondente al numero

     // ............................................
     // Funzioni di decodifica
     // ............................................
     // Decodificano sia i servizi sia le informazioni generali (es: Ho Note generiche) 
     STRINGA Categoria(){return DecodTipoMezzo(TipoMezzo);};
     static STRINGA Categoria(int TipoMezzo){return DecodTipoMezzo(TipoMezzo);};
     // Per questo metodo si puo' scegliere una forma corta ( acronimo) o lunga
     static const char * DecServizio(BYTE Num, BOOL Lunga); // Torna NULL per Num fuori range
     // I metodi seguenti decodificano TUTTI i servizi definiti, ed altre informazioni
     // Questo metodo usa la sola forma corta
     STRINGA Decodifica(BOOL AncheClassifica=TRUE); // Tutti i servizi + opzionalmente la classifica
     // Questo metodo usa la sola forma lunga
     ELENCO_S DecodificaServizi();                  // Tutti i servizi e le informazioni generali

     // ............................................
     // Alcuni metodi per manipolare MM_INFO
     // ............................................
     void Clear(){*(DWORD*)this = 0; TipoMezzo = 0;};
     BOOL operator == ( MM_INFO & B){ return (*(DWORD*)this == *(DWORD*)&B) && (TipoMezzo == B.TipoMezzo ); } ;

     // Questo metodo dati i servizi di due tratte li combina in un insieme di servizi "combinati"
	  // La combinazione e' fatta in modo significativo dal punto di vista ferroviario
     // (Es: Handicap in AND , WL in OR )
     void CombinaTratta(const MM_INFO & From); 

     // Questi metodi vanno utilizzati solo in algoritmi specifici: infatti manipolano i 
     // servizi in modo puramente binario.
     BOOL Empty();                              // Non ha alcun servizio ne' note ne' niente ....
     BOOL EmptyServizi();                       // Non ha alcun servizio
     void operator &= (const MM_INFO & From);   // Combinazione in AND
     void operator |= (const MM_INFO & From);   // Combinazione in OR
     void NotIn(const MM_INFO & From,BOOL AzzeraNonSrv) ;   
     // Lascio solo i servizi NON presenti in From
     // Posso resettare oppure lasciati inalterati i BIT non corrispondenti a servizi
};

#if sizeof(MM_INFO) != (sizeof(DWORD) + 1)
#error "Attenzione: rivedere i metodi di MM_INFO"
#endif

//----------------------------------------------------------------------------
// LINGUA : Lingua correntemente utilizzata per le note
//----------------------------------------------------------------------------
// Impostata in MM_RETE_FS
enum LINGUA { Francese = 1, Inglese = 2, Italiano = 3, Spagnolo = 4, Tedesco = 5 };

//----------------------------------------------------------------------------
// INFO_NOTE
//----------------------------------------------------------------------------
// NB: La struttura di note e' applicabile nell' ambito di una data soluzione, e per
// la data della richiesta. Non sono disponibili informazioni sulle note ad altre date
// o per altre tratte degli stessi mezzi virtuali (anche se spesso le note
// saranno comunque applicabili).
//----------------------------------------------------------------------------
struct INFO_NOTE {

    // Ritorna: 0 = NON Applicabile, 1 = Nota generica, 2 = Nota di vendita
    // Le note di vendita stanno prima delle altre.
    int ApplicabileATratta(int NumTratta, int NumNota);

    ELENCO_S Note;
    BYTE     NumNoteDiVendita  ; // Numero delle note di vendita (ESCLUSE LE BANALI)
    LINGUA   Linguaggio        ; // Linguaggio in cui ho fornito le note (solo 1 per volta)

    private:
    BUFR TratteApplicabili ; // Notare che l' uso di BUFR limita le tratte ad 8 !
    friend class SOLUZIONE;
};

//----------------------------------------------------------------------------
// TRAVERSATE_MARE
//----------------------------------------------------------------------------
struct TRAVERSATA_MARE {
   WORD IdStazioneSbarco      ;     // Key
   WORD IdStazioneImbarco     ;     // Key
   WORD KmMare                ;
   WORD CodSocietaMare        ;
   WORD CodLineaMare          ;
   BYTE IstatRegioneImbarco   ;     // Codici Istat Regionali : ENUM CODICI_ISTAT_REGIONE
   BYTE IstatRegioneSbarco    ;     // Codici Istat Regionali   ENUM CODICI_ISTAT_REGIONE

   void Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

} ;

class TRAVERSATA_MARE_CON_DESCR:  public TRAVERSATA_MARE{
   public:
   char Descrizione[17];
   void Set(TRAVERSATA_MARE & From);
};

   // Elenco dinamico di TRAVERSATA_MARE_CON_DESCR
   typedef TRAVERSATA_MARE_CON_DESCR * P_TM;
   #define DIN_ARRAY ELENCO_TRAVERSATE_MARE_CON_DESCR
   #define CLAS P_TM
   #define DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
   #define CopiaMembro(_CLAS) (Beep(__FILE__,__LINE__,1),(P_TM)NULL) // Vieto la copia
   #include "dinarray.h"

//----------------------------------------------------------------------------
// TIPOPERIODICITA
//----------------------------------------------------------------------------
enum TIPOPERIODICITA {
   NON_PERIODICO             =0 ,
   GIORNI_FERIALI               ,
   GIORNI_FESTIVI               ,
   LAVORATIVI_ESCLUSO_SABATO    ,
   ESCLUSO_SABATO               ,
   SABATO_E_FESTIVI             ,
   PERIODICITA_COMPLESSA         
};

//----------------------------------------------------------------------------
// MM_PERIODICITA : Struttura per la gestione della periodicita' (su base giorno)
//----------------------------------------------------------------------------
// Attenzione: opera solo nell' ambito dell' orario CORRENTE!

class _export MM_PERIODICITA {

   public:

   // Torna TRUE se il treno circola quel dato giorno
   BOOL Circola(class SDATA & Giorno);

   const char * Circolazione_STRINGA(); // Rimane valida fino alla prossima chiamata forma LMmGVSD

   // Questo metodo torna una serie di righe presentabili con la periodicita'.
   // Giorni della settimana nella forma: LMmGVSD
   // Con testata: Include due linee di testata
   // Doublespace: 2 caratteri per giorno
   // Points: puntini per facilitare la lettura. Di solito inutile se DoubleSpace = TRUE
   ELENCO_S Decod(BOOL ConTestata=TRUE,BOOL DoubleSpace=FALSE,BOOL Points=TRUE);

   void  _export Trace(STRINGA Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);


   // Non accedere direttamente
   protected:
   BYTE Dati[MAX_MMP_BYTES]; // Ogni bit rappresenta un giorno dall' inizio del periodo

};


//----------------------------------------------------------------------------
// DATI_TARIFFAZIONE
//----------------------------------------------------------------------------
// Questa struttura contiene i dati necessari per la tariffazione.
// Potrebbe essere estesa in futuro per tenere conto di zone tariffarie
// o tariffe O/D
//----------------------------------------------------------------------------
struct DATI_TARIFFAZIONE {

   // Servizio interno
   WORD KmReali     ;
   WORD KmAggiuntivi;
   WORD KmMare      ;
   BYTE Diretta     ; // Vero se seguo la via piu' breve

   // Cumulativo (e Tirrenia)
   WORD KmConcessi1 ;
   WORD KmConcessi2 ;
   BYTE CodConcessione1 ; // Codice societa' concessa
   WORD CodLinea1 ;       // Codice linea societa' concessa
   BYTE CodConcessione2 ; // Codice societa' concessa
   WORD CodLinea2 ;       // Codice linea societa' concessa

   // Stazioni di transito FS
   ID   TransitoFS1;      // Prima stazione di transito FS
   ID   TransitoFS2;      // Seconda stazione di transito FS
   ID   TransitoFS3;      // Terza stazione di transito FS

   // Tariffe regionali
   BYTE CodiceTariffaRegionale; // Per abbonamenti regionali
                                // 0    = Applicare tariffa nazionale
                                // 0xFF = NON SO (disabilitare vendita abbonamenti)
   //
   TRAVERSATA_MARE_CON_DESCR Andata;

   enum STATO {
      TARIFFA_NON_CALCOLATA = 0 ,
      TARIFFA_VALIDA_SOLO_PER_INFORMATIVA,
      TARIFFA_VALIDA            ,
      TRATTE_ILLEGALI           ,  // Chiesta tariffa parziale su errato intervallo tratte o comunque con parametri errati
      TRATTE_NON_TARIFFABILI    ,  // Alcune tratte non sono tariffabili
      SOLO_CUMULATIVO           ,  // Ha servizio cumulativo MA NON tratte FS
      TRE_CUMULATIVO               // Ha tre o piu' tratte cumulative
   } Stato;

   // Questo e' il codice CCR della destinazione: lo passo direttamente perche'
   // sui cumulativi a volte debbo prendere uno dei codici alternativi.
   // Per tutte le altre stazioni si puo' accedere alla stazione per ID e chiedere
   // il codice CCR (se necessario)
   DWORD CodiceCCRDestinazione;
   // Idem per origine
   DWORD CodiceCCROrigine;

   void _export Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
   int Lunghezza()const {return KmReali + KmAggiuntivi + KmMare + KmConcessi1 + KmConcessi2 + Andata.KmMare ;};
   STRINGA StatoInChiaro(); // Decodifica stato

};
struct DATI_TARIFFAZIONE_2 : public DATI_TARIFFAZIONE {
   ARRAY_ID FormaStandard; // Forma standard del percorso
   // Il primo ID e' l' ID della stazione di origine
   // L' ultimo ID e' l' ID della stazione di destinazione
   // Gli altri sono i "nodi" di instradamento.
   // Potrebbero essere delle stazioni non nodali  ma solo se
   // strettamente necessarie per rappresentare l' instradamento.
   ARRAY_DW IstradamentoCVB; // Serie di codici di instradamento 
   // I codici riportati sono codici CCR, non id interni
   // I codici delle stazioni italiane non comprendono il codice rete
   
   STRINGA FormaStandardStampabile; // E' stampabile sul biglietto
   void Set(DATI_TARIFFAZIONE & From){*(DATI_TARIFFAZIONE *)this = From;};

   void _export Clear();
   void _export Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
};

//----------------------------------------------------------------------------
// SDATA
//----------------------------------------------------------------------------
// E' la struttura di base per le date.
// Viene subclassata all' interno del motore
struct _export SDATA {
   USHORT Anno;
   BYTE   Giorno;
   BYTE   Mese;

   operator STRINGA()const;
   operator const char*()const{ return (const char *)STRINGA(THIS);};

   BOOL operator >  (const SDATA & b)const;
   BOOL operator >= (const SDATA & b)const;
   BOOL operator <  (const SDATA & b)const;
   BOOL operator <= (const SDATA & b)const;
   BOOL operator == (const SDATA & b)const;
   BOOL operator != (const SDATA & b)const;

   int  operator - (const SDATA & b)const; // Differenza tra due date

   int Giuliano()const; // Giuliano = giorni dall' inizio dell' anno

   BYTE  GiornoDellaSettimana(); // 0 = Lunedi ...

   SDATA & operator ++(); // Giorno successivo
   SDATA & operator --(); // Giorno precedente

   BOOL DaGGMMAAAA(const STRINGA & From); // Converte da formato "GG/MM/AAAA" . Torna FALSE su errori banali

   BOOL Bisestile()const; // Torna TRUE se bisestile

 };

//----------------------------------------------------------------------------
// TIPO_RICERCA: Profondita' di ricerca del MOTORE
//----------------------------------------------------------------------------
  enum TIPO_RICERCA {
  Minima,         // Ricerca FAST
  Media,          // Ricerca estesa
  TanteSol,       // Ricerca molto estesa
  CasiDifficili   // Per ora eguale a TanteSol
  };


// Stampa l' ora in formato hh:mm,  e' rientrante solo 10 volte ****
const char * _export ORA(short int Ora);

//----------------------------------------------------------------------------
// MM_SOLUZIONE e relativo elenco dinamico
//----------------------------------------------------------------------------
struct _export MM_SOLUZIONE {

   struct TRATTA_TRENO {
      WORD IdMezzoVirtuale; // Id del mezzo virtuale (0 se multistazione)
      char IdTreno[6];      // Id del treno (numero) corrispondente al mezzo virtuale a StazioneIn : "" se collegamento urbano
      MM_INFO InfoTreno;    // Informativa del mezzo virtuale (relativamente alla tratta specifica)
      WORD IdStazioneIn;    // Id della stazione in cui si sale sul treno
      WORD IdStazioneOut;   // Id della stazione in cui si scende dal treno
      WORD OraIn;           // Orario di partenza dalla stazione in cui si sale sul treno
      WORD OraOut;          // Orario di arrivo nella stazione in cui si scende dal treno
      char IdentOut[8];     // Identificatore della stazione di uscita (8 caratteri)
      STRINGA * NomeSpecifico;// Solo per alcuni treni (Michelangelo ...) altrimenti NULL
                              // Il dato e' disponibile SOLO dopo aver chiamato GetNote()

      ~TRATTA_TRENO(){if(NomeSpecifico){ delete NomeSpecifico; }; };

      // ................................................................
      // Dati interni per algoritmi: possono cambiare senza preavviso
      // Non usare piu' gli shift : ho fornito dei metodi in MM_SOLUZIONE
      // ................................................................
      BIT  IdCluster      :11;// ID del cluster di appartenza del treno
      BIT  P_MezzoViagIn  : 3;// Mezzo viaggiante
      BIT  P_MezzoViagOut : 3;// Mezzo viaggiante
      BIT  Concorde       : 1;// Verso del cluster
      BIT _GiorniShiftP   : 4;// Indica quanti giorni dopo la partenza della SOLUZIONE si ha la partenza della tratta.
                              // NB: Dal punto di vista del passeggero: ignora i giorni di partenza dei MV 
                              // (che possono essere antecedenti) e considero solo i giorni effettivi in cui prendo i mezzi
      BIT _GiorniShiftA   : 4;// Indica quanti giorni dopo la partenza della SOLUZIONE si ha l' arrivo della tratta. Stesso NB.
      BIT _SuccPartenzaMV : 1;// 1 Se la partenza di tratta si ha il giorno successivo alla data di partenza del mezzo virtuale della tratta.
      BIT  TrattaCorta    : 2;// Vero se la percorrenza della tratta non raggiunge 1/2 percorrenza del treno o 250 Km
      BIT  NoPenalita     : 1;// Vero se sulla tratta NON debbo applicare le normali penalit… (in particolare per collegamenti urbani)
                              // 2 se la tratta non raggiunge i 50 Km
      BYTE OfsIn;             // Punta alla stazione In : indice nei nodi del cluster
      BYTE OfsOut;            // Punta alla stazione Out: indice nei nodi del cluster
      BYTE OfsTreno;          // Punta al treno : indice nei treni del cluster (Solo se non multistazione)
      BYTE PrgIn;             // Punta alla stazione In : progressivo in ambito TRENO VIRTUALE
      BYTE PrgOut;            // Punta alla stazione Out: progressivo in ambito TRENO VIRTUALE
      // Fine dati interni per algoritmi
      // ................................................................
   };

   BYTE   NumeroTratte;
   MM_INFO InfoSoluzione          ; // Informativa generale della soluzione
   WORD   TempoTotaleDiAttesa;      // Tempo di attesa totale alle stazioni intermedie
   WORD   TempoTotaleDiPercorrenza; // Tempo totale di percorrenza
   char   IdentPartenza[8];         // Identificatore della stazione di partenza
   TRATTA_TRENO Tratte[MAX_TRATTE]; // Massimo MAX_TRATTE tratte
   WORD   Ordine;                   // Viene utilizzato per il sort finale


   // ....................................................
   // Informazioni su date ed ore di partenza e simili
   // ....................................................
   // Sono fornite sia considerando che escludendo le multistazioni
   // Di norma per l' utente NON si debbono considerare le multistazioni
   // NB: La data della richiesta ( = DATA Corrente su RETE_FS ) non deve 
   // essere cambiata altrimenti i dati non saranno corretti. 
   // ....................................................
   SDATA DataPartenza(){ return DataPartenza(0);};
   WORD  OraPartenza() { return Tratte[0].OraIn;};
   SDATA DataArrivo()  { return DataArrivo(NumeroTratte-1);}; 
   WORD  OraArrivo()   { return Tratte[NumeroTratte-1].OraOut;};
   // Queste sono corretta per la tratta iniziale/finale in servizio urbano 
   BYTE  Vera1(){ return Tratte[0].IdMezzoVirtuale ? 0 : 1;};
   BYTE  VeraN(){ return Tratte[NumeroTratte - 1].IdMezzoVirtuale ? NumeroTratte - 1 : NumeroTratte - 2;};
   SDATA DataPartenzaVera(){ return DataPartenza(Vera1());};
   WORD  OraPartenzaVera() { return Tratte[Vera1()].OraIn;};
   SDATA DataArrivoVera()  { return DataArrivo(VeraN());};
   WORD  OraArrivoVera()   { return Tratte[VeraN()].OraOut;};      
   // Queste sono le corrispondenti per tratta 
   SDATA DataPartenza(BYTE NTratta)             ; // Giorno di partenza della tratta ( = giorno in cui si sale sul treno)
   SDATA DataArrivo(BYTE NTratta)               ; // Giorno di arrivo della tratta ( = giorno in cui si scende dal treno)
   SDATA DataPartenzaMezzoVirtuale(BYTE NTratta); // Giorno in cui parte il mezzo virtuale della tratta . 
                                                  // Puo' essere antecedente al giorno di partenza della tratta.


   // Giorni della settimana in cui posso avere la soluzione
   // Rimane valida fino alla prossima chiamata forma LMMGVSD
   const char * Circolazione_STRINGA(){ return Per.Circolazione_STRINGA(); };
   MM_PERIODICITA  Per;             // Periodicita' della soluzione trovata, relativa ai giorni di partenza della soluzione.
                                    // Indica cioe' i giorni in cui si puo' partire, NON i giorni di partenza dei mezzi virtuali.
                                    // Per sapere se con la soluzione si puo' partire in una data X
                                    // si utilizzi la chiamata: Per.Circola(X) ove X e' in formato SDATA.
   BYTE        SuccessivoRichiesta; // 1 se la partenza avviene il giorno successivo alla data per cui e' effettuata la richiesta.
                                    // Si ha per definizione: 
                                    //  se 0 Per.Circola(DataRichiesta)   = TRUE
                                    //  se 1 Per.Circola(DataRichiesta-1) = TRUE
   TIPOPERIODICITA TipoPeriodicita; // Tipo periodicita'  (come da Orario)



   // Puntatore ad una struttura INFO_NOTE . Inizialmente NULL
   // La struttura NON deve essere distrutta dall' applicativo: ci pensa il distruttore di MM_SOLUZIONE
   INFO_NOTE * InfoNote;

   char AcronimoStazionePartenza[4];
   char AcronimoStazioneArrivo  [4];
   BYTE   NumeroTratteReali;        // Esclude le multistazioni : e' il numero di treni, autobus eccetera
                                    // effettivamente utilizzati dal viaggiatore

   ~MM_SOLUZIONE(){if(InfoNote)delete InfoNote;};
    void * operator new(size_t Size);
   void operator delete(void * Item);

   struct MM_TRENO & Treno();  // Accedera' ai dati del treno; La struttura e' da determinare
   void Trace(const STRINGA& Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   // Questo metodo fornisce informazioni di secondo livello, valorizzando InfoNote.
   // In caso di errore InfoNote rimane NULL; Se non vi sono note InfoNote viene
   // valorizzato ma gli elenchi sono vuoti.
   // NB: La data della richiesta ( = DATA Corrente su RETE_FS ) non deve essere cambiata
   // altrimenti i dati non saranno corretti. La lingua puo' invece essere cambiata e, ripetendo
   // la richiesta, si otterranno le note nel nuovo linguaggio.
   void GetNote();

   void StoreTo(    BUFR & To  );
   void ReStoreFrom(BUFR & From);

   // Informazioni per secondi contatti

   // Questo metodo identifica le fermate/transiti lungo il percorso.
   // Se richiesti i transiti vengono forniti i soli transiti su stazioni nodali
   // NB: Per ora non fornisco gli orari, ma sarebbe possibile se necessario
   BOOL IdentificaStazioni(ARRAY_ID & NodiDiTransito, BOOL AncheTransiti = FALSE);

   // Ritorna i dati di tariffazione per tratte parziali (contigue) della soluzione
   // In caso di errore imposta il campo Stato dei DATI_TARIFFAZIONE
   // Le tratte vanno da 0 a NumeroTratte - 1, gli estremi sono inclusi.
   DATI_TARIFFAZIONE_2 KmParziali(BYTE PrimaTratta, BYTE SecondaTratta, class MM_I_PERCORSO * Percorso);

   // Questa forma funziona tra due qualunque FERMATE del percorso.
   // Nel caso il treno ripassi pi— volte da una stessa stazione verr… scelta
   // la fermata che comporta un minore chilometraggio complessivo.
   DATI_TARIFFAZIONE_2 KmParziali(ID Da, ID A);

//<<< struct MM_SOLUZIONE {
};

#endif

