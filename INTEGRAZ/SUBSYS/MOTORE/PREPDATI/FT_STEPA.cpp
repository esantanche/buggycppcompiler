//----------------------------------------------------------------------------
// FT_STEPA.CPP:
//    Identifica i mezzi virtuali
//    Identifica i servizi dei treni e dei mezzi virtuali
//    Gestisce i codici che debbono rimanere stabili da una fornitura alla prossima
//
// Il programma richiede un parametro opzionale con i seguenti valori:
//   UPDATE : Cerca di riutilizzare i codici. E' il parametro da usare normalmente
//   DELETE : cancella i dati delle precedenti forniture. i codici sono resettati
//   REUSE  : solo per sviluppo: riusa i precedenti risultati senza rileggere TABORARI
// Di norma il programma va lanciato con UPDATE, nel qual caso controlla
// che i dati siano effettivamente relativi ad una fornitura successiva
// all' ultima caricata.
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2
//#define DBG1  // Trace esteso
//#define DBG2  // Mostra la periodicita' ove opportuno
//#define DBG3  // Trace molto esteso
//#define DBG4  // Fa vedere i casi in cui combino differenti servizi su MV
//#define DBG5  // Scrive sul trace la periodicita' di tutti i mezzi virtuali mostrati
//#define DBG6  // Mostra anche i mezzi virtuali composti di un solo treno
//----------------------------------------------------------------------------
/*
   Particolari:

      - E' introdotto il concetto di TRENO VIRTUALE. I treni virtuali
        sono formati unendo piu' "treni" forniti da FS, sulla base delle
        seguenti informazioni:

         1. Treni Origine - Destinazione (o se si preferisce il termine
           confluenti - defluenti)

         2. Carrozze dirette. L' informazione delle carrozze dirette
           e' presa da NOTEPART.T

         3. Servizi diretti: sempre presi da NOTEPART.T, dovrebbero essere
            salvo errori informazioni ridondanti

   L' elaborazione dei servizi e' principalmente fatta dai metodi della classe
   PTRENO definita sul file FTPTRENO.CPP

   E' previsto che vi siano piu' forniture di dati per ogni orario: il
   programma mantiene (per ogni orario) i codici precedentemente
   assegnati ad alcune entita' (es: periodicita', id dei mezzi virtuali
   eccetera).

   Usare DU_TRENV con l' opzione /T per una analisi di dettaglio della periodicita',
   e DU_SERVI per una analisi di dettaglio dei servizi e delle fermate.

   REGOLA1:
   Nel caso in cui un treno prosegua su un altro a metÖ del percorso,
   sono prodotti due mezzi virtuali: uno che prosegue nel nuovo mezzo
   virtuale, l' altro che completa il percorso.

   REGOLA2:
   La regola 1 NON vale per i mezzi virtuali da carrozze dirette: in
   sostanza non considero accettabile che il viaggiatore debba cambiare
   carrozza sul treno. Vi e' qualche eccezione (per semplicitÖ algoritmica)

   ================================
   ALGORITMO: descrizione generale
   ================================

   ==> "ControllaFornitura"
      Apro il file di validita' orario (DATE.T)

      Controllo formale fornitura (stesso orario e progressivo maggiore
      dell' ultima volta o nuovo orario e progressivo = 0).

      A seconda del valore del parametro (DELETE / REUSE) e della
      effettiva esistenza ed accessibilita' dei files stessi decido se
      riutilizzare i codici preesistenti oppure di cancellare tutti i
      dati dei precedenti caricamenti.

   ==> "PTRENO::Load" (o "PTRENO::Restore" ).
   Il programma inizia con il caricare i dati dei treni, invocando il
   metodo Load() della classe PTRENO .
   Se in modalita' REUSE riutilizzo i dati di un precedente caricamento
   mediante la piu' veloce Restore().

   ==> "CaricaHashTables"
   Poi eseguo una scansione su INFOCOMM, individuando tutte le note che
   indicano servizi diretti od una carrozza diretta.

   Suddivido le note individuate in due categorie, e le raccolgo in due
   Hash tables (Hash ed hash2):

      1) Le note che hanno uno specifico treno origine o destinazione.
         Per tali note dovro' cercare di fare un collegamento con il
         treno indicato.  Vi DEVE essere una qualche nota corrispondente
         sul treno indicato.  Si noti che TUTTE le note di infocomm sono
         gestite in questo modo se hanno indicazione di treni origine /
         destinazione

      2) Le note che non hanno uno specifico treno origine o
         destinazione . Tali note vengono messe in Hash2 e possono o
         meno avere una nota corrispondente. Solo le note che indicano
         carrozze dirette sono gestite in questo modo.

      Le note della prima categoria sono anche raccolte in una lista
      lineare.

   Durante la scansione vengono anche individuati i servizi tipici di
   ogni nota.

   ==> "CompletaHashTables"
   Dopodiche' scandisco la lista lineare (quella delle note di categoria
   "1" ) e per ogni nota controllo e/o aggiusto la congruenza.

   Spiego con un esempio:  Se il treno A ha una nota N che indica la
      prosecuzione sul treno B alla stazione di cambio C, cerchero' il
      treno B sulla Hash delle note di tipo "1" .

      Se trovato (con lo stesso numero di nota, treno origine pari ad A
      e stessa stazione di cambio ) Ok.

      Altrimenti cerco il treno B su Hash2.

      Se trovato ( con lo stesso numero di nota e stessa stazione di
      cambio ) creo dinamicamente una nota complementare a quella di A
      (aggiungendo il treno origine) e la aggiungo alla hash table.

      Altrimenti modifico la nota di A eliminando la prosecuzione sul
      treno B. Se il treno A non ha un treno origine e' tolto dalla hash
      table.

   Ricostruisco la lista lineare (perche' le modifiche erano state
   apportate alla Hash table.

   ==> "GeneraMezziVirtualiDaOrigineDestino"
   Ho gia' controllato, nel metodo PTRENO::Load(), che i mezzi
   provenienza / destinazione siano corretti.

         Con il termine cambio debole, usato di seguito, si intende una
         situazione in cui un treno B origina da un treno A, avendo
         pero' che il treno A non prosegue nel treno B.
         O viceversa.
         Tali situazioni indicano uno scambio PARZIALE di carrozze.

   Scandisco tutta la Hash dei mezzi viaggianti, cercando i mezzi senza
   provenienza e che non siano destinazione di altri treni:  per questi
   concateno ricorsivamente i possibili mezzi destinazione (con cambi
   deboli o forti) fino a generare tutti i possibili virtuali.

      Due Precauzioni:

      - Poiche' vi possono essere dei LOOPS di treni (SI!), cioe'
         situazioni in cui il treno A continua sul treno B che continua
         sul treno C che continua sul treno A, a causa di truch iche FS
         utilizza per rappresentare tratte percorse avanti ed indietro,
         debbo anche aggiungere un LOOP per identificare tali casi e
         caricarli.

      - Vi sono dei treni che entrano in altri mezzi virtuali per una
         parte ridotta del percorso.  Tali situazioni vanno individuate
         e si deve generare un mezzo virtuale in pió

   Registro il numero di cambi deboli .

   La stazione di cambio tra due treni e' identificata con i seguenti
   criteri:

      Cambio forte:  il treno T1 prosegue nel treno T2 che origina da
         T1.  Controllo che l' ultima stazione del treno T1 sia la prima
         del treno T2.  E' la stazione di cambio.

      Cambio debole:  Il treno T1 prosegue nel treno T2.  La stazione di
         cambio e' l' ultima stazione del treno T1.

      Cambio debole:  Il treno T2 origina dal treno T1.  La stazione di
         cambio e' la prima stazione del treno T2.

   ==> "DiramaTrenoPerOD"
   Il metodo concatena ricorsivamente i possibili  mezzi destinazione:
   utilizzato da "GeneraMezziVirtualiDaOrigineDestino"

   ==> "GeneraMezziVirtualiDaCarrozzeDirette"
   A questo punto le note della lista hanno sicuramente dei match sui
   treni origine / prosecuzione nella Hash table.

   Posso quindi procedere a comporre i mezzi virtuali da carrozze
   dirette semplicemente attaccando ad ogni treno il successivo.

   Alcune precauzioni sono necessarie:

      1:  Poiche' vi possono essere piu' note per lo stesso treno vi
         possono anche essere piu' treni destinazione:  percio' il
         completamento e' fatto in con un algoritmo "ricorsivo" (e'
         codificato con un ciclo, ma se si segue la logica si vede che
         e' equivalente ad un algoritmo ricorsivo).

      2:  Per lo stesso motivo mantengo una Hash (Hash3) dei mezzi
         generati, in modo da evitare duplicazioni di mezzi virtuali ove
         un treno avesse piu' note che ne indichino la prosecuzione su
         di un altro treno.

      3:  Bisogna correggere le periodicita' tenendo conto dei cambi di
         data.

   ==> "DiramaTrenoPerCD"
   Il metodo concatena ricorsivamente i mezzi in accordo con le note
   delle carrozze dirette.

   ==> "CorreggiVirtuali"
   Controllo se i virtuali con piu' di un cambio debole siano anche
   generati da carrozze dirette: se no mando una segnalazione.

   Si noti che non elimino i virtuali in quanto il passeggero puo', in
   effetti, spostandosi sul treno usufruire del collegamento (questa ä
   una eccezione alla regola 2).

   Inoltre: Per i virtuali generati SOLO da "altri servizi" controllo se
   per tutti i cambi sia presente un cambio debole o forte (cioe' uno
   dei treni sia origine / destino dell' altro). Se si' elimino il
   virtuale in quanto dovuto ad una semplice imperfezione
   (mando tuttavia un warning).

   ==> "CorreggiServizi"
   Per i mezzi virtuali individuati faccio delle correzioni sui servizi:
   in particolare poiche' debbono avere almeno un servizio di trasporto
   verifico se e' il caso di aggiungere i flags dei posti a sedere.

      Questo in quanto alcuni mezzi hanno delle carrozze dirette con
         soli posti a sedere che si immetto su di un treno, e delle
         carrozze dirette di tipo cuccette o WL che si immettono su di
         un altro: in tali situazioni solo controllando direttamente i
         mezzi virtuali si riesce a comprendere (e non del tutto) cosa
         stia accedendo.

   ==> "Completa"

   Genero gli ID dei mezzi virtuali, cercando di riutilizzare
   quelli gia' assegnati nell' ultimo release dell' orario.

   Sorto e genero i files di output.

   A questo punto i dati sono pronti:  faccio un printout dei dati
   generati per permettere i controlli, e di alcune statistiche.

*/
//----------------------------------------------------------------------------

// EMS001 Win
typedef unsigned long BOOL;

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "oggetto.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ML_IN.HPP"
#include "elenco.h"
// EMS002 Win  #include "eventi.h"
#include "ctype.h"
#include "seq_proc.hpp"
#include "scandir.h"
#include "MM_BASIC.HPP"
#include "stddef.h"


// ed altre anomalie minori
#ifdef DBG4
int HardCheck=1;
#else
int HardCheck=0;
#endif

#define PGM      "FT_STEPA"

//----------------------------------------------------------------------------
// Funzioni
//----------------------------------------------------------------------------
void CaricaHashTables()                         ;
void CompletaHashTables()                       ;
void GeneraMezziVirtualiDaOrigineDestino()      ;
void DiramaTrenoPerOD(PTRENO & Tco)             ;
void GeneraMezziVirtualiDaCarrozzeDirette()     ;
void CorreggiVirtuali()                         ;
void CorreggiServizi()                          ;
void Completa()                                 ;
BOOL NotaCarrozzaDiretta( WORD A)               ; // Identifica se la nota comporta la presenza di una carrozza diretta
int  SortMv( const void *a, const void *b)      ; // Funzioncina di sort dei mezzi virtuali in ordine di treno
int  ControllaFornitura(char * Modo)            ; // Controlla progressivo fornitura


//----------------------------------------------------------------------------
// Aree di appoggio
//----------------------------------------------------------------------------
// Su queste strutture carico (in memoria) le note lette da INFOCOMM
static HASH<NOTA_SERVIZI> Hash(512,1024)     ; // Queste sono le note delle carrozze dirette specifiche
static HASH<NOTA_SERVIZI> Hash2(512,1024)    ; // Queste sono le note delle carrozze dirette generiche
static HASH<COMPOS_MEZZOVI>Hash3(10000,10000); // Queste sono i dati finali: Per evitare duplicazioni
// begin EMS003 Win
//static ARRAY_DINAMICA<NOTA_SERVIZI> Arr(1024); // Queste sono le note delle carrozze dirette specifiche  (lista lineare)
static ARRAY_DINAMICA<NOTA_SERVIZI> Arr(2048); // Queste sono le note delle carrozze dirette specifiche  (lista lineare)// E queste sono variabili di lavoro
// end EMS003
static NOTA_SERVIZI  WrkNota;
static PTRENO * TrenoCorrente=NULL;
static BOOL Internazionale;
static int TLen = sizeof( WrkNota.IdentTreno );
static int KLen = sizeof( WrkNota.IdentTreno ) + sizeof(WrkNota.IdNota);

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------
int main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"

   int Rc = 0;

   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);

   // EMS004 Win SetStatoProcesso(PGM, SP_PARTITO);
   if (trchse > 2) trchse = 2; // Per evitare trace abissali involontari

   SetPriorita(); // Imposta la priorita'

   // Aggiungo il file di logging alle destinazioni di stampa
   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);

   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");

   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore

   TryTime(0);
   // Verifica progressivo fornitura e , se OK, carica i relativi dati
   if(argc <  2 ){
      Rc = ControllaFornitura("NO_PARM");
   } else {
      Rc = ControllaFornitura(argv[1]);
   };
   if(Rc)return Rc;


   // Caricamento HashTables
   CaricaHashTables();

   // Metto a posto le indicazioni discordanti sulle note in modo che corrispondano sui
   // differenti treni
   CompletaHashTables();

   // Compongo i mezzi virtuali
   GeneraMezziVirtualiDaOrigineDestino();
   GeneraMezziVirtualiDaCarrozzeDirette();

   // Segnalazione dei virtuali "garibaldini"
   CorreggiVirtuali();

   // Correzione finale dei servizi
   CorreggiServizi();

   // Scarico su file ed analisi risultati
   Completa();

   TRACESTRING("Il programma Ë terminato !!!!!!!!!!!!!!!");
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;

   return 0;
//<<< int main int argc,char *argv
};

//----------------------------------------------------------------------------
// ControllaFornitura
//----------------------------------------------------------------------------
int  ControllaFornitura(char * Modalita){
   #undef TRCRTN
   #define TRCRTN "ControllaFornitura"

   enum MODO { UPDATE , DELETE , REUSE } Modo;

   if(!stricmp(Modalita,"REUSE")){         // Accetto i vecchi dati con la stessa versione
      Modo = REUSE;
   } else if(!stricmp(Modalita,"DELETE")){ // Ricarico i dati da capo
      Modo = DELETE;
   } else if(!stricmp(Modalita,"UPDATE")){ // Nuova versione stesso orario
      Modo = UPDATE;
   } else {
      Bprintf("Errore: Parametro richiesto : modalita' operativa");
      Bprintf(" valori possibili: DELETE, REUSE, UPDATE");
      BEEP;
      return 999;
   }

   // Controlli con precedente fornitura
   if(Modo != DELETE && TestFileExistance(PATH_OUT "VALIDITA.DB")){
      BUFR Tmp;
      FILE_RO OldValidita(PATH_OUT "VALIDITA.DB");
      OldValidita.Leggi(OldValidita.FileSize(),Tmp);
      VALIDITA_ORARIO & OldVal = *(VALIDITA_ORARIO*)Tmp.Dati;
      if( !(OldVal.NomeOrario == T_PERIODICITA::NomeOrario) ){ // Cambiato orario
         if (T_PERIODICITA::ProgressivoFornitura != 1) {
            Bprintf("Cambio di orario: Da: '%s' A  '%s'",OldVal.NomeOrario,(CPSZ)T_PERIODICITA::NomeOrario);
            Bprintf("Progressivo fornitura dovrebbe essere 1 invece e' %i",T_PERIODICITA::ProgressivoFornitura);
            BEEP;
            return 100;
         } /* endif */
      } else {
         if (Modo == REUSE){ // OK accetto il progressivo fornitura qualunque esseo sia
         } else if (T_PERIODICITA::ProgressivoFornitura == 1) {
            Bprintf("Stesso orario: '%s'",(CPSZ)T_PERIODICITA::NomeOrario);
            Bprintf("Progressivo fornitura NON dovrebbe essere 1 invece lo e'");
            BEEP;
            return 101;
         } else if (T_PERIODICITA::ProgressivoFornitura <= OldVal.ProgressivoFornitura) {
            Bprintf("Stesso orario: '%s'",(CPSZ)T_PERIODICITA::NomeOrario);
            Bprintf("Progressivo fornitura (%i) dovrebbe essere maggiore del precedente (%i)", T_PERIODICITA::ProgressivoFornitura,OldVal.ProgressivoFornitura);
            BEEP;
            return 102;
         } /* endif */
      } /* endif */
//<<< if Modo != DELETE && TestFileExistance PATH_OUT "VALIDITA.DB"
   } else {  // Ricreo tutto daccapo: modalita' DELETE oppure non ho trovato validita.db
      system("Del " PATH_OUT "*.DB " PATH_OUT "*.EXT " PATH_OUT "*.TM? " PATH_OUT "*.SV1 >NUL 2>&1");
      // Metto in PERIODIC.SV1 le periodicitÖ standard
      ARRAY_T_PERIODICITA AllPeriodicita;
      // Di seguito metto i valori "standard"
      // In particolare il valore 0 deve corrispondere a "InLimitiOrario"
      // Perche' puï essere usato nei test interni al motore (e' un modo rapido)
      AllPeriodicita += T_PERIODICITA::InLimitiOrario;
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[2 ]; // 2: Si effettua sempre
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[3 ]; // 3: Si effettua nei giorni lavorativi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[5 ]; // 5: Si effettua nei festivi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[8 ]; // 8: Si effettua il Sabato e nei giorni festivi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[29]; //29: Si effettua nei pre-festivi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[31]; //31: Si effettua nei post-festivi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[6 ]; // 6: Si effettua nei giorni scolastici
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[11]; //11: Si effettua nei giorni non scolastici
      AllPeriodicita.Put(PATH_OUT "PERIODIC.SV1");
      Modo = DELETE;
      // Scarico la struttura VALIDITA
      VALIDITA_ORARIO Val;
      Val.FormatoDeiDati                      = ORARIO_FORMATO_ATTUALE                ;
      Val.Inizio                              = T_PERIODICITA::Inizio_Dati_Caricati   ;
      Val.Fine                                = T_PERIODICITA::Fine_Dati_Caricati     ;
      Val.InizioUfficiale                     = T_PERIODICITA::Inizio_Orario_FS       ;
      Val.FineUfficiale                       = T_PERIODICITA::Fine_Orario_FS         ;
      Val.InizioOraLegale                     = T_PERIODICITA::Inizio_Ora_Legale      ;
      Val.FineOraLegale                       = T_PERIODICITA::Fine_Ora_Legale        ;
      Val.OraInizioOraLegale                  = T_PERIODICITA::Ora_Inizio_Ora_Legale  ;
      Val.OraFineOraLegale                    = T_PERIODICITA::Ora_Fine_Ora_Legale    ;
      Val.ProgressivoFornitura                = T_PERIODICITA::ProgressivoFornitura   ;
      Val.CircolaSempre                       = T_PERIODICITA::InLimitiOrario         ;
      Val.CircolaNeiLavorativi                = T_PERIODICITA::PeriodicitaPerCodice[3];
      Val.CircolaNeiFestivi                   = T_PERIODICITA::PeriodicitaPerCodice[5];
      Val.CircolaNeiLavorativiEsclusoIlSabato = T_PERIODICITA::PeriodicitaPerCodice[4];
      Val.CircolaTuttiIGiorniEsclusoIlSabato  = T_PERIODICITA::PeriodicitaPerCodice[7];
      Val.CircolaIlSabatoENeiFestivi          = T_PERIODICITA::PeriodicitaPerCodice[8];

      // Queste sono periodicitÖ di uso frequente, che preferisco siano associate a indici
      // bassi (semplifica il debug).
      // E' ESSENZIALE che all' indice 0 vi sia "circola sempre"
      AllPeriodicita += T_PERIODICITA::InLimitiOrario;
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[2 ]; // 2: Si effettua sempre
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[3 ]; // 3: Si effettua nei giorni lavorativi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[4 ]; // 3: Si effettua nei giorni lavorativi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[5 ]; // 5: Si effettua nei festivi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[6 ]; // 6: Si effettua nei giorni scolastici
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[7 ]; // 7: Si effettua dalla Domenica al Venerdç
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[8 ]; // 8: Si effettua il Sabato e nei giorni festivi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[11]; //11: Si effettua nei giorni non scolastici
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[29]; //29: Si effettua nei pre-festivi
      AllPeriodicita += T_PERIODICITA::PeriodicitaPerCodice[31]; //31: Si effettua nei post-festivi
      ZeroFill(Val.NomeOrario);
      strcpy(Val.NomeOrario ,(CPSZ)T_PERIODICITA::NomeOrario);
      FILE_RW Validita(PATH_OUT "VALIDITA.DB");
      Validita.SetSize(0);
      Validita.Scrivi(&Val,sizeof(VALIDITA_ORARIO));
//<<< if Modo != DELETE && TestFileExistance PATH_OUT "VALIDITA.DB"
   } /* endif */

   // Per prima cosa carico i dati di tutti i treni.
   if(Modo != REUSE){ // Ricarico i dati da capo
      TRACESTRING("In ft_stepa prima di PTRENO::LOad");
      PTRENO::Load();
      TRACESTRING("Usciamo da Load");
   } else {
      PTRENO::Restore(); // Parto dai dati caricati l' ultima volta
      ITERA(PTRENO::HashPt,Tr,PTRENO){
      Tr.NumVirtuali =0; // Questa variabile deve essere reimpostata
      } END_ITERA
   }
   return 0;
//<<< int  ControllaFornitura char * Modalita
};


//----------------------------------------------------------------------------
// NotaCarrozzaDiretta
//----------------------------------------------------------------------------
BOOL NotaCarrozzaDiretta( WORD Nota ) {
   #undef TRCRTN
   #define TRCRTN "NotaCarrozzaDiretta"
   switch (Nota) {
   case 4:
   case 7:
   case 30:
   case 54:
      return TRUE;
   default:
      return FALSE;
   } /* endswitch */
};
//----------------------------------------------------------------------------
// SortMv
//----------------------------------------------------------------------------
// Questa funzione sorta i mezzi virtuali in ordine di TRENO
int SortMv( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "SortMv"
   return memcmp(a,b,offsetof( COMPOS_MEZZOVI , Periodicita));
};

//----------------------------------------------------------------------------
// CaricaHashTables
//----------------------------------------------------------------------------
// Suddivido le note nelle due categorie indicate in cima e le
// metto nelle hash tables e nella lista lineare.
// Inoltre faccio dei controlli base di congruenza.
//----------------------------------------------------------------------------
void CaricaHashTables(){
   #undef TRCRTN
   #define TRCRTN "CaricaHashTables"

   STRINGA PathDa(PATH_IN),ExtFl(".T");
   if (getenv("USADATIESTRATTI")){ PathDa=PATH_OUT;ExtFl=".XTR";};
   FILE_INFOCOMM InfoComm(PathDa + "INFOCOMM" + ExtFl);

   // Scansione INFOCOMM
   TryTime(0);
   Bprintf("%s",TRCRTN);
   IDTRENO LastSegnalato1 = NESSUN_TRENO, LastSegnalato2 = NESSUN_TRENO;
   BOOL Inibit = FALSE;  // Inibisce le note palesemente in errore
   ORD_FORALL(InfoComm,i1){
      INFOCOMM & Ic = InfoComm[i1];

      char LastTipoR;

      if (Ic.TipoRecord == '1' || Ic.TipoRecord == '2') {
         WrkNota.Periodicita = PTRENO::Get2( WrkNota.IdentTreno, WrkNota.Periodicita); // Normalizzo rispetto circolazione del treno
         #ifdef DBG2
         if( !(WrkNota.Periodicita == T_PERIODICITA::InLimitiOrario)){
            WrkNota.Periodicita.Trace(STRINGA("Periodicita' della carrozza del treno: ")+St(WrkNota.IdentTreno));
         };
         #endif
         // Scarico dei dati della precedente nota: Ammetto chiave duplicata nelle Hash
         if (Inibit){
            Bprintf("Causa errori ignorata nota %i del treno %s",WrkNota.IdNota,St(WrkNota.IdentTreno));
            Inibit = FALSE;
         } else if (WrkNota.ServizioDeiTreniProvenODest == '*') {
            NOTA_SERVIZI  & Tg = * Hash.Alloca();
            ByteCopyR(Tg, WrkNota);
            Arr += Tg;
            #ifdef DBG3
            TRACESTRING( "       Hash.Metti: " VRS(Tg.IdNota) + VRS(Tg.IdentTreno) + VRS(Tg.TrenoInizioServizioSeMV) + VRS(Tg.TrenoFineServizioSeMV) + VRS(Tg.TrenoInizioServizioSeMV) + VRS(Tg.TrenoFineServizioSeMV));
            #endif
            Hash.Metti(KLen);
         } else if(NotaCarrozzaDiretta(WrkNota.IdNota)) {
            NOTA_SERVIZI  & Tg = * Hash2.Alloca();
            ByteCopyR(Tg, WrkNota);
            #ifdef DBG3
            TRACESTRING( "       Hash2.Metti: " VRS(Tg.IdNota) + VRS(Tg.IdentTreno) + VRS(Tg.TrenoInizioServizioSeMV) + VRS(Tg.TrenoFineServizioSeMV) + VRS(Tg.TrenoInizioServizioSeMV) + VRS(Tg.TrenoFineServizioSeMV));
            #endif
            Hash2.Metti(KLen);
         };
//<<< if  Ic.TipoRecord == '1' || Ic.TipoRecord == '2'
      };

      if (Ic.TipoRecord == '1') {
         WrkNota.Init();
         WrkNota.IdentTreno = Ic.R1.IdentTreno;
         TrenoCorrente = PTRENO::Get(WrkNota.IdentTreno);
      } else if (Ic.TipoRecord == '2'){
         WrkNota.IdNota = It(Ic.R2.Nota);
         WrkNota.ServizioDeiTreniProvenODest = Ic.R2.ServizioDeiTreniProvenODest ;
         WrkNota.StazioneInizioServizio = Ic.R2.StazioneInizioServizio;
         WrkNota.StazioneFineServizio   = Ic.R2.StazioneFineServizio    ;
         WrkNota.Servizi = TrenoCorrente->Servizio(InfoComm); // Identifica i servizi specifici della nota
         WrkNota.Servizi.HaNoteDiVendita = 0; // L' informazione non mi serve ed anzi mi crea problemi
         // Ovvia regola: Non posso avere un servizio di mezzo virtuale se non ho un servizio (permette correzioni pragmatiche ...)
         WrkNota.Servizi &= TrenoCorrente->ServiziTreno;
         WrkNota.TrenoInizioServizioSeMV  = Ic.R2.TrenoInizioServizioSeMV ;
         WrkNota.TrenoFineServizioSeMV    = Ic.R2.TrenoFineServizioSeMV   ;
         #ifdef DBG3
         TRACESTRING( VRS(WrkNota.IdNota) + VRS(WrkNota.IdentTreno) + VRS(WrkNota.StazioneFineServizio) + VRS(Ic.R2.StazioneFineServizio) );
         TRACESTRING( "       " VRS(WrkNota.TrenoInizioServizioSeMV) + VRS(WrkNota.TrenoFineServizioSeMV) + VRS(Ic.R2.TrenoInizioServizioSeMV) + VRS(Ic.R2.TrenoFineServizioSeMV  ) );
         #endif
         if (WrkNota.IdentTreno == WrkNota.TrenoInizioServizioSeMV) {
            Inibit = TRUE;
            Bprintf2("Errore: Nota %i Treno %s ha se stesso come treno inizio servizio",WrkNota.IdNota,St(WrkNota.IdentTreno));
         } else if (WrkNota.IdentTreno == WrkNota.TrenoFineServizioSeMV) {
            Inibit = TRUE;
            Bprintf2("Errore: Nota %i Treno %s ha se stesso come treno fine servizio",WrkNota.IdNota,St(WrkNota.IdentTreno));
         } /* endif */
         // Faccio un minimo di controlli
         if (Ic.R2.ServizioDeiTreniProvenODest == '*') {
            if( Ic.R2.TrenoInizioServizioSeMV == NESSUN_TRENO &&
               Ic.R2.TrenoFineServizioSeMV == NESSUN_TRENO
            ){
               if(WrkNota.IdentTreno != LastSegnalato1 && (NotaCarrozzaDiretta(WrkNota.IdNota) || HardCheck))
                  Bprintf2("Errore: Nota/e su Mezzo %s manca ERRONEAMENTE dei treni provenienza o destinazione" ,St(WrkNota.IdentTreno));
               WrkNota.ServizioDeiTreniProvenODest = ' ';
               LastSegnalato1 = WrkNota.IdentTreno;
            }
         } else {
            if( Ic.R2.TrenoInizioServizioSeMV != NESSUN_TRENO ||
               Ic.R2.TrenoFineServizioSeMV != NESSUN_TRENO
            ){
               if(WrkNota.IdentTreno != LastSegnalato2 && (NotaCarrozzaDiretta(WrkNota.IdNota) || HardCheck))
                  Bprintf2("Errore: Nota/e su Mezzo %s ha ERRONEAMENTE treni provenienza o destinazione" ,St(WrkNota.IdentTreno));
               WrkNota.ServizioDeiTreniProvenODest = '*';
               LastSegnalato2 = WrkNota.IdentTreno;
            }
         } /* endif */

         // Controllo effettiva esistenza dei treni origine / destinazione
         if ( WrkNota.TrenoInizioServizioSeMV != NESSUN_TRENO &&
            PTRENO::Get(WrkNota.TrenoInizioServizioSeMV) == NULL
         ){
            Internazionale |= strstr(St(WrkNota.TrenoInizioServizioSeMV), "*INT")!= NULL;
            Internazionale |= strstr(St(WrkNota.TrenoInizioServizioSeMV), "*EST")!= NULL;
            #ifdef DBG1
            Internazionale = FALSE; // Per mostralo sempre
            #endif
            if(!Internazionale)Bprintf2("Errore: Nota N¯ %i del treno %s indica il treno fine servizio inesistente %s",WrkNota.IdNota, St(WrkNota.IdentTreno), St(WrkNota.TrenoInizioServizioSeMV) );
            WrkNota.TrenoInizioServizioSeMV = NESSUN_TRENO ;
         };
         if ( WrkNota.TrenoFineServizioSeMV != NESSUN_TRENO &&
            PTRENO::Get(WrkNota.TrenoFineServizioSeMV) == NULL
         ){
            Internazionale |= strstr(St(WrkNota.TrenoFineServizioSeMV), "*INT")!= NULL;
            Internazionale |= strstr(St(WrkNota.TrenoFineServizioSeMV), "*EST")!= NULL;
            #ifdef DBG1
            Internazionale = FALSE; // Per mostralo sempre
            #endif
            if(!Internazionale)Bprintf2("Errore: Nota N¯ %i del treno %s indica il treno fine servizio inesistente %s",WrkNota.IdNota, St(WrkNota.IdentTreno), St(WrkNota.TrenoFineServizioSeMV) );
            WrkNota.TrenoFineServizioSeMV = NESSUN_TRENO ;
         };
         // Questo per tener conto della precedente correzione
         if( Ic.R2.TrenoInizioServizioSeMV == NESSUN_TRENO &&
            Ic.R2.TrenoFineServizioSeMV == NESSUN_TRENO
         ) WrkNota.ServizioDeiTreniProvenODest = ' ';
//<<< if  Ic.TipoRecord == '1'
      } else if (Ic.TipoRecord == '3') {
         WrkNota.Periodicita.ComponiPeriodicita(5,Ic.R3.Periodicita,LastTipoR != '3');
      } /* endif */
      LastTipoR = Ic.TipoRecord;
//<<< ORD_FORALL InfoComm,i1
   }
   if (TrenoCorrente != NULL ){
      WrkNota.Periodicita = PTRENO::Get2( WrkNota.IdentTreno, WrkNota.Periodicita); // Normalizzo rispetto circolazione del treno
      // Scarico dei dati della precedente nota: Ammetto chiave duplicata nelle Hash
      if (Inibit){
         Bprintf("Causa errori ignorata nota %i del treno %s",WrkNota.IdNota,St(WrkNota.IdentTreno));
         Inibit = FALSE;
      } else if (WrkNota.ServizioDeiTreniProvenODest == '*') {
         NOTA_SERVIZI  & Target = * Hash.Alloca();
         ByteCopyR(Target, WrkNota);
         Arr += Target;
         Hash.Metti(KLen);
      } else if(NotaCarrozzaDiretta(WrkNota.IdNota)) {
         NOTA_SERVIZI  & Target = * Hash2.Alloca();
         ByteCopyR(Target, WrkNota);
         Hash2.Metti(KLen);
      };
   };
//<<< void CaricaHashTables
};
//----------------------------------------------------------------------------
// CompletaHashTables
//----------------------------------------------------------------------------
// Adesso vado a completare la Hash Table con i treni generici, e controllo la completezza.
// Alla fine di questo blocco ogni nota nella Hash table avra' i corretti treni origine e/o destinazione
// (esistenti) e la corretta stazione di cambio.
//----------------------------------------------------------------------------
void CompletaHashTables(){
   #undef TRCRTN
   #define TRCRTN "CompletaHashTables"

   TryTime(0);
   Bprintf("%s",TRCRTN);
   WrkNota.Init();
   FORALL(Arr,i2){
      TRACEINT("Indice ciclo principale i2=",i2);
      NOTA_SERVIZI  Arr2 = Arr[i2]; // Lavoro per copia perche' debbo poi modificare l' Hash Table
      BOOL Modificato = FALSE;
      if ( Arr2.TrenoInizioServizioSeMV != NESSUN_TRENO ) {
         WrkNota.IdentTreno = Arr2.TrenoInizioServizioSeMV;
         WrkNota.IdNota = Arr2.IdNota;
         for( NOTA_SERVIZI  * Wrk = Hash.Cerca( &WrkNota, KLen); Wrk; Wrk = Hash.CercaNext()) {
            #ifdef DBG3
            TRACESTRING( "In Hash 701: " VRS(WrkNota.IdentTreno) + VRS(WrkNota.IdNota));
            TRACESTRING( VRS(Arr2.IdentTreno) + VRS(Arr2.TrenoInizioServizioSeMV) + VRS(Arr2.StazioneFineServizio) + VRS(Arr2.StazioneInizioServizio));
            TRACESTRING( VRS(Wrk->IdentTreno) + VRS(Wrk->TrenoFineServizioSeMV) + VRS(Wrk->StazioneFineServizio) + VRS(Wrk->StazioneInizioServizio) );
            #endif
            if( Arr2.IdentTreno !=  Wrk->TrenoFineServizioSeMV   )continue; // Not Found
            if( Arr2.TrenoInizioServizioSeMV !=  Wrk->IdentTreno )continue; // Not Found
            if( Arr2.StazioneInizioServizio !=  Wrk->StazioneFineServizio ){
               Bprintf("Errore: Nota N¯ %i La stazione di cambio non corrisponde treno %s da treno provenienza %s", Arr2.IdNota, St(Arr2.IdentTreno), St(Arr2.TrenoInizioServizioSeMV) );
               continue;
            }
            break; // Ok
         }
         if(Wrk == NULL){ // Non trovato
            // Vedo se lo trovo tra i treni con carrozze dirette generiche
            for( Wrk = Hash2.Cerca( &WrkNota, KLen); Wrk; Wrk = Hash2.CercaNext()) {
               #ifdef DBG3
               TRACESTRING("In Hash2 717: " VRS(Arr2.StazioneFineServizio) + VRS(Wrk->StazioneInizioServizio) );
               #endif
               if( Arr2.StazioneInizioServizio !=  Wrk->StazioneFineServizio )continue; // Not Found
               break; // Ok
            }
            if(Wrk != NULL) { // Trovato
               // Creo una nuova nota equivalente
               NOTA_SERVIZI  & Wrk2 = * Hash.Alloca();
               BlankFill(Wrk2);
               Wrk2.IdentTreno = Arr2.TrenoInizioServizioSeMV;
               Wrk2.TrenoFineServizioSeMV = Arr2.IdentTreno             ;
               Wrk2.StazioneFineServizio = Arr2.StazioneInizioServizio ;
               Wrk2.IdNota = Arr2.IdNota;
               Wrk2.Periodicita = Wrk->Periodicita;
               Wrk2.Servizi     = Wrk->Servizi    ;
               Hash.Metti(KLen); // Notare che non la metto anche nell' array
               #ifdef DBG1
               Bprintf2("Aggiunta nota EQV (1) N¯ %i tra treni %s e %s a stazione %s",Arr2.IdNota, St(Arr2.TrenoInizioServizioSeMV), St(Arr2.IdentTreno), St(Arr2.StazioneInizioServizio) );
               #endif
            } else {
               Bprintf2("Note senza corrispondenza: Nota N¯ %i treno provenienza %s del treno %s a stazione di cambio %s", Arr2.IdNota, St(Arr2.TrenoInizioServizioSeMV) , St(Arr2.IdentTreno), St(Arr2.StazioneInizioServizio) );
               BlankFill(Arr2.TrenoInizioServizioSeMV );
               Modificato = TRUE;
            }
            #ifdef DBG1
//<<<    if Wrk == NULL   // Non trovato
         } else {
            Bprintf2("Ok identificazione cambio (1) Nota N¯ %i tra treni %s e %s a stazione %s", Arr2.IdNota, St(Arr2.TrenoInizioServizioSeMV), St(Arr2.IdentTreno), St(Arr2.StazioneInizioServizio) );
            #endif
         }
//<<< if   Arr2.TrenoInizioServizioSeMV != NESSUN_TRENO
      } /* endif */

      if ( Arr2.TrenoFineServizioSeMV != NESSUN_TRENO ) {
         WrkNota.IdentTreno = Arr2.TrenoFineServizioSeMV ;
         WrkNota.IdNota = Arr2.IdNota;
         for( NOTA_SERVIZI  * Wrk = Hash.Cerca( &WrkNota,KLen); Wrk; Wrk = Hash.CercaNext()) {
            #ifdef DBG3
            TRACESTRING( "In Hash 755: " VRS(WrkNota.IdentTreno) + VRS(WrkNota.IdNota));
            TRACESTRING( VRS(Arr2.IdentTreno) + VRS(Arr2.TrenoFineServizioSeMV) + VRS(Arr2.StazioneFineServizio) + VRS(Arr2.StazioneInizioServizio));
            TRACESTRING( VRS(Wrk->IdentTreno) + VRS(Wrk->TrenoInizioServizioSeMV) + VRS(Wrk->StazioneFineServizio) + VRS(Wrk->StazioneInizioServizio) );
            #endif
            if( Arr2.IdentTreno !=  Wrk->TrenoInizioServizioSeMV )continue ; // Not Found
            if( Arr2.TrenoFineServizioSeMV !=  Wrk->IdentTreno   )continue ; // Not Found
            if( Arr2.StazioneFineServizio !=  Wrk->StazioneInizioServizio ){
               Bprintf("Errore: Nota N¯ %i La stazione di cambio non corrisponde treno %s a treno destinazione %s", Arr2.IdNota, St(Arr2.IdentTreno), St(Arr2.TrenoFineServizioSeMV) );
               continue;
            }
            break ; // Ok
         } /* endwhile */
         if(Wrk == NULL){ // Non trovato
            // Vedo se lo trovo tra i treni con carrozze dirette generiche
            for( Wrk = Hash2.Cerca( &WrkNota,KLen); Wrk; Wrk = Hash2.CercaNext()) {
               #ifdef DBG3
               TRACESTRING("In Hash2 771: " VRS(Arr2.StazioneFineServizio) + VRS(Wrk->StazioneInizioServizio) );
               #endif
               if( Arr2.StazioneFineServizio !=  Wrk->StazioneInizioServizio )continue; // Not Found
               break ; // Ok
            } /* endwhile */
            if(Wrk != NULL) { // Trovato
               // Creo una nuova nota equivalente
               NOTA_SERVIZI  & Wrk2 = * Hash.Alloca();
               BlankFill(Wrk2);
               Wrk2.IdentTreno              = Arr2.TrenoFineServizioSeMV  ;
               Wrk2.TrenoInizioServizioSeMV = Arr2.IdentTreno           ;
               Wrk2.StazioneInizioServizio  = Arr2.StazioneFineServizio ;
               Wrk2.IdNota = Arr2.IdNota;
               Wrk2.Periodicita = Wrk->Periodicita;
               Wrk2.Servizi     = Wrk->Servizi    ;
               Hash.Metti(KLen);
               #ifdef DBG1
               Bprintf2("Aggiunta nota EQV (1) N¯ %i tra treni %s e %s a stazione %s",Arr2.IdNota, St(Arr2.TrenoFineServizioSeMV), St(Arr2.IdentTreno), St(Arr2.StazioneFineServizio) );
               #endif
            } else {
               Bprintf2("Note senza corrispondenza: Nota N¯ %i treno destinazione %s del treno %s a stazione di cambio %s", Arr2.IdNota, St(Arr2.TrenoFineServizioSeMV) , St(Arr2.IdentTreno), St(Arr2.StazioneFineServizio) );
               BlankFill(Arr2.TrenoFineServizioSeMV );
               Modificato = TRUE;
            }
//<<<    if Wrk == NULL   // Non trovato
         } else if( Arr2.StazioneFineServizio !=  Wrk->StazioneInizioServizio ){
            Bprintf2("Errore di identificazione stazione di cambio Nota N¯ %i tra treni %s e %s", Arr2.IdNota, St(Arr2.TrenoFineServizioSeMV), St(Arr2.IdentTreno) );
            #ifdef DBG1
         } else {
            Bprintf2("Ok identificazione cambio (1) Nota N¯ %i tra treni %s e %s a stazione %s", Arr2.IdNota, St(Arr2.TrenoFineServizioSeMV), St(Arr2.IdentTreno), St(Arr2.StazioneFineServizio) );
            #endif
         }
//<<< if   Arr2.TrenoFineServizioSeMV != NESSUN_TRENO
      } /* endif */
      if (Modificato) {
         // Cerco la nota del servizio diretto nella Hash Table
         WrkNota.IdentTreno = Arr[i2].IdentTreno ;
         WrkNota.IdNota = Arr2.IdNota;
         for( NOTA_SERVIZI  * Wrk = Hash.Cerca( &WrkNota, KLen); Wrk; Wrk = Hash.CercaNext()) {
            TRACESTRING("IN ciclo incriminato");
            TRACEPOINTER("Wrk=",Wrk);
            if( Arr[i2].TrenoFineServizioSeMV !=  Wrk->TrenoFineServizioSeMV )continue;
            if( Arr[i2].TrenoInizioServizioSeMV !=  Wrk->TrenoInizioServizioSeMV )continue;
            if( Arr[i2].StazioneFineServizio !=  Wrk->StazioneFineServizio )continue;
            if( Arr[i2].StazioneInizioServizio !=  Wrk->StazioneInizioServizio )continue;
            break;
         } /* endwhile */
         if(Wrk == NULL){ // Non trovato
            Bprintf("WARNING: Mancata corrispondenza tra Hash Table e lista lineare per treno %s",St(Arr2.IdentTreno));
            BEEP;
         } else if (
            Arr2.TrenoInizioServizioSeMV == NESSUN_TRENO &&
            Arr2.TrenoFineServizioSeMV == NESSUN_TRENO
         ) {
            TRACESTRING("Entro in ramo NESSUN_TRENO");
            // Elimino la nota del servizio diretto dalla Hash Table (Si noti che non la metto nella lista dei treni con CD generiche)
            Hash.DeleteCurrent();
            if(HardCheck)Bprintf("Eliminata nota errata N¯ %i del treno %s",Arr2.IdNota,St(Arr2.IdentTreno));
         } else {
            TRACESTRING("Ultimo ramo");
            // Modifico la nota del servizio diretto nella Hash Table
            *Wrk = Arr2;
            if(HardCheck)Bprintf("Modificata nota errata N¯ %i del treno %s Ora ha provenienza %s e destinazione %s",Arr2.IdNota,St(Arr2.IdentTreno), St(Arr2.TrenoInizioServizioSeMV), St(Arr2.TrenoFineServizioSeMV) );
         }
//<<< if  Modificato
      } /* endif */
//<<< FORALL Arr,i2
   }

   // Aggiorno la lista lineare
   Arr.Clear();

   ITERA(Hash, NotaCorr, NOTA_SERVIZI ){
   Arr += NotaCorr;
   } END_ITERA
//<<< void CompletaHashTables
};
//----------------------------------------------------------------------------
// WRK_OD   ...   Classe di utilizzo locale
//----------------------------------------------------------------------------
// WRK_OD : Rappresenta una struttura incompleta
// E' utilizzata per comporre i mezzi virtuali a partire da origine / destino
struct WRK_OD : public COMPOS_MEZZOVI {
   PTRENO Treno;
   void Clear(){ COMPOS_MEZZOVI::Clear(); Treno.Clear(); };
   void ConcatenaTreno(PTRENO & B);
};
//----------------------------------------------------------------------------
// WRK_OD::ConcatenaTreno
//----------------------------------------------------------------------------
void WRK_OD::ConcatenaTreno(PTRENO & NewTreno){
   #undef TRCRTN
   #define TRCRTN "WRK_OD::ConcatenaTreno"

   #ifdef DBG1
   TRACESTRING("In elaborazione "+STRINGA(Ident())+" Prosegue con " + STRINGA(St(NewTreno.IdTreno)));
   #endif
   MC & Mc = MezziComponenti[NumMezziComponenti];
   NewTreno.NumVirtuali ++;
   if (NumMezziComponenti == 0) {
      Mc.ShiftPartenza = 0;
      Periodicita = NewTreno.PeriodicitaTreno ;
      // Non altero i servizi ...
      DaOrigineDestino = 1; // Se TRUE vuol dire che uno dei treni continua nell' altro
   } else if(NumMezziComponenti >= MAX_MEZZI_REALI) {
      Bprintf("Errore: Raggiunto limite N¯ Mezzi virtuale: %s",(CPSZ)Ident());
   } else {
      MC & McLast = MezziComponenti[NumMezziComponenti-1];
      // Debbo determinare se il cambio e' debole o forte
      BOOL Forte = FALSE;
      if (Treno.TDestinazione == NewTreno.IdTreno) {
         McLast.StazioneDiCambio = Treno.CcrA;
         if(Treno.IdTreno == NewTreno.TProv){
            Forte = TRUE;
            assert(Treno.CcrA == NewTreno.CcrDa );
         };
      } else {
         McLast.StazioneDiCambio = NewTreno.CcrDa;
         assert(Treno.IdTreno == NewTreno.TProv);
      } /* endif */
      if(!Forte)CambiDeboli ++;

      // Vedo se ho avuto il passaggio per la mezzanotte
      int OraLast;
      if (NumMezziComponenti == 1) {
         OraLast = Treno.OraPartenza(Treno.CcrDa);
      } else {
         OraLast = Treno.OraPartenza( MezziComponenti[NumMezziComponenti-2].StazioneDiCambio );
      }
      int OraNew = NewTreno.OraPartenza( McLast.StazioneDiCambio );
      if (OraLast < 0 || OraNew < 0) { // Incongruenze
         TRACESTRING( VRO(OraLast) +  VRO(OraNew) + VRS(Ident()) + VRS(NewTreno.IdTreno) + VRS(McLast.StazioneDiCambio));
         Mc.ShiftPartenza = McLast.ShiftPartenza ;
      } else if ( OraLast > OraNew ) {
         Mc.ShiftPartenza = McLast.ShiftPartenza + 1;
      } else {
         Mc.ShiftPartenza = McLast.ShiftPartenza ;
      } /* endif */
      int ShiftVero =  Mc.ShiftPartenza ;
      if( OraNew >= 0 && OraNew < NewTreno.OraPartenzaTreno) ShiftVero -- ; // Cambio di data sul secondo treno
      // TRACESTRING( VRO(OraLast) +  VRO(OraNew) + VRS(Ident())+ VRS(ShiftVero) + VRS(NewTreno.IdTreno) + VRS(McLast.StazioneDiCambio));

      T_PERIODICITA PWrk = NewTreno.PeriodicitaTreno;
      // Vedo se la periodicita' shiftata e' equivalente alla periodicita' di partenza:
      // Se non sono equivalenti utilizzo la nuova periodicitÖ
      if( ! Periodicita.ConfrontaShiftando(PWrk, ShiftVero) ){
         // Qui PWrk e' stato alterato: side effect di ConfrontaShiftando
         if (NewTreno.PeriodicitaTreno != T_PERIODICITA::InOrario) { // Altrimenti non altera la periodicita' perche' circola sempre
            if (Periodicita == T_PERIODICITA::InOrario) { // Il nuovo treno impone la periodicita'
               Periodicita = PWrk ;
            } else {
               if( Forte ){
                  TRACESTRING(VRS(ShiftVero) + VRS(McLast.ShiftPartenza ) );
                  TRACESTRING( VRO(OraLast) +  VRO(OraNew) + VRS(Ident()) + VRS(NewTreno.OraPartenzaTreno) + VRS(NewTreno.IdTreno));
                  Bprintf2("WARNING : Differente periodicita' in cambio forte tra treni %s e %s", St(McLast.IdentTreno), St(NewTreno.IdTreno));
                  NewTreno.PeriodicitaTreno.Trace("Periodicita 2¯ Treno");
                  if(PWrk != NewTreno.PeriodicitaTreno)PWrk.Trace("Pwrk (Corretta cambio data)");
                  Periodicita.Trace("Periodicita mezzo virtuale");
               }
               Periodicita &= PWrk ; // Circolazione COMPLESSIVA del mezzo virtuale
            }
         } /* endif */
      } /* endif */
//<<< if  NumMezziComponenti == 0
   } /* endif */
   Mc.IdentTreno = NewTreno.IdTreno;
   Mc.StazioneDiCambio = NESSUNA_STAZIONE;
   Treno = NewTreno;
   NumMezziComponenti ++;

   // Vedo se ho treni duplicati
   if (NumMezziComponenti> 1) {
      ELENCO_S Treni;
      for (int i = 0; i < NumMezziComponenti ; i++ ) {
         STRINGA Idt=(STRINGA)MezziComponenti[i].IdentTreno;
         if (Treni.Contiene(Idt)) {
            InLoop = 1;
            Bprintf2("WARNING: Il mezzo virtuale %s contiene due volte lo stesso treno",(CPSZ)Ident());
            break;
         } else {
            Treni += Idt;
         } /* endif */
      } /* endfor */
   } /* endif */

//<<< void WRK_OD::ConcatenaTreno PTRENO & NewTreno
};
//----------------------------------------------------------------------------
// DiramaTrenoPerOD
//----------------------------------------------------------------------------
void DiramaTrenoPerOD(PTRENO & Tco){
   #undef TRCRTN
   #define TRCRTN "DiramaTrenoPerOD"
   ARRAY_DINAMICA<WRK_OD> Appoggio(100); // Area di appoggio: necessaria perche' i treni in prosecuzione possono ramificare
   WRK_OD Mv;
   Mv.Clear();
   Mv.ConcatenaTreno(Tco);
   Appoggio.Clear();
   Appoggio += Mv;
   while (Appoggio.Dim()) { // Continuo finche' non ho esaurito tutte le possibili combinazioni
      Mv = Appoggio.Last();
      TRACESTRING(VRS(Mv.Ident()));
      Appoggio.Elimina(Appoggio.Dim() -1);
      if(Mv.NumMezziComponenti > MAX_MEZZI_REALI) { // Ero in Loop ?
         Mv.NumMezziComponenti = MAX_MEZZI_REALI;
         Mv.InLoop=TRUE;
      };
      if(Mv.InLoop){
         *Hash3.Alloca() = (COMPOS_MEZZOVI)Mv;
         int KeySize = Mv.NumMezziComponenti * sizeof(COMPOS_MEZZOVI::MC);
         Hash3.Metti(KeySize);
         continue;
      }
      if (Mv.Treno.TDestinazione == NESSUN_TRENO ) {
         // REGOLA1:
         // Genero un mezzo virtuale, a meno che non che vi siano mezzi in prosecuzione (di testa)
         if (Mv.Treno.NumProvenienzaDiTesta == 0  ){
            #ifdef DBG1
            TRACESTRING("Scarico "+STRINGA(Mv.Ident()));
            #endif
            Mv.Periodicita &= T_PERIODICITA::InLimitiOrario ;
            int KeySize = Mv.NumMezziComponenti * sizeof(COMPOS_MEZZOVI::MC);
            assert(Mv.NumMezziComponenti >= 1);
            COMPOS_MEZZOVI * PrecedenteRecord = Hash3.Cerca(&Mv,KeySize);
            if (PrecedenteRecord == NULL) {   // Va inserito ex novo nella hash table
               PrecedenteRecord = Hash3.Alloca();
               *PrecedenteRecord = (COMPOS_MEZZOVI)Mv;
               Hash3.Metti(KeySize);
            } else {
               // Ignoro i servizi: di fatto non li considero poiche' essendo
               // questi mezzi virtuali derivati dai treni li gestisco SEMPRE come
               // combinazioni di servizi di treno.
               PrecedenteRecord->Periodicita |=  Mv.Periodicita;
            } /* endif */
         } /* endif */
//<<< if  Mv.Treno.TDestinazione == NESSUN_TRENO
      } else { // Genero la prosecuzione per cambio debole o forte
         WRK_OD Mv2 = Mv;
         Mv2.ConcatenaTreno(*PTRENO::Get(Mv.Treno.TDestinazione)); // Ricorsivita'
         Appoggio += Mv2;
      }
      if (Mv.Treno.NumProvenienza > 0 ) {
         // Test per velocizzare : altrimenti dovrei scandire ogni volta.
         // Cosi' lo faccio solo se ho cambi deboli
         if (Mv.Treno.NumProvenienza > 1 ||
            Mv.Treno.TDestinazione == NESSUN_TRENO ||
            PTRENO::Get(Mv.Treno.TDestinazione)->TProv != Mv.Treno.IdTreno
         ) {
            int i = 0;
            ITERA(PTRENO::HashPt, WrkT, PTRENO){
               if (WrkT.TProv == Mv.Treno.IdTreno) {
                  // Se WrkT.IdTreno == Mv.Treno.TDestinazione ho un cambio forte e non
                  // sarebbe necessario inserirlo: ma poiche' la hash mi gestisce le
                  // duplicazioni faccio prima a non filtrare
                  WRK_OD Mv2 = Mv;
                  Mv2.ConcatenaTreno(WrkT); // Ricorsivita'
                  Appoggio += Mv2;
                  i ++;

               } /* endif */
            } END_ITERA
            assert3(i == Mv.Treno.NumProvenienza , { TRACESTRING(VRS(i) + VRS(Mv.Treno.NumProvenienza) + VRS(Mv.Treno.IdTreno) + VRS(Mv.Ident()) ) });
         } /* endif */
//<<< if  Mv.Treno.NumProvenienza > 0
      } /* endif */
//<<< while  Appoggio.Dim      // Continuo finche' non ho esaurito tutte le possibili combinazioni
   }
//<<< void DiramaTrenoPerOD PTRENO & Tco
};
//----------------------------------------------------------------------------
// GeneraMezziVirtualiDaOrigineDestino
//----------------------------------------------------------------------------
void GeneraMezziVirtualiDaOrigineDestino(){
   #undef TRCRTN
   #define TRCRTN "GeneraMezziVirtualiDaOrigineDestino"
   TryTime(0);
   Bprintf("%s",TRCRTN);

   ITERA(PTRENO::HashPt, Tco, PTRENO){
      #ifdef DBG3
      TRACESTRING( VRS(Tco.IdTreno) + VRS(Tco.TProv) + VRS(Tco.NumDestinazione) + VRS(Tco.NumDestinazioneDiTesta) + VRS(Tco.NumProvenienza) + VRS(Tco.NumProvenienzaDiTesta));
      #endif
      if (Tco.TProv != NESSUN_TRENO)continue; // Il mezzo ha una provenienza (cambio debole o forte)

      // REGOLA1:
      // Modificato: Genero il MV anche se il treno ä destinazione di altri,
      // se vi sono solo inserimenti a metÖ percorso
      // if (Tco.NumDestinazione > 0  )continue; // Il mezzo e' destinazione di altri treni (Sicuramente cambio debole)
      if (Tco.NumDestinazioneDiTesta > 0  )continue; // Cambio di testa

      DiramaTrenoPerOD(Tco);
   } END_ITERA

   // Adesso aggiungo tutti i treni che appartengono ad un loop: li identifico
   // perche' non sono ancora stati caricati.
   ITERA(PTRENO::HashPt, Tloop, PTRENO){
      if (Tloop.NumVirtuali > 0  )continue;
      if (Tloop.TProv != NESSUN_TRENO)continue; // Il mezzo ha una provenienza
      #ifdef DBG3
      TRACESTRING( VRS(Tloop.IdTreno) + VRS(Tloop.TProv) + VRS(Tloop.NumDestinazione) + VRS(Tloop.NumProvenienza));
      #endif
      DiramaTrenoPerOD(Tloop);
   } END_ITERA

//<<< void GeneraMezziVirtualiDaOrigineDestino
};
//----------------------------------------------------------------------------
// WRK_CARR ...   Classe di utilizzo locale
//----------------------------------------------------------------------------
// WRK_CARR: Rappresenta una struttura incompleta, mantiene i dati dell' ultima nota.
// E' utilizzata per comporre i mezzi virtuali a partire dalle carrozze dirette
struct WRK_CARR : public COMPOS_MEZZOVI {
   NOTA_SERVIZI  Nota;
   void Clear(){ COMPOS_MEZZOVI::Clear(); ZeroFill(Nota);};
   void ConcatenaNota(NOTA_SERVIZI & B);
};
//----------------------------------------------------------------------------
// WRK_CARR::ConcatenaNota
//----------------------------------------------------------------------------
void WRK_CARR::ConcatenaNota(NOTA_SERVIZI & B){
   #undef TRCRTN
   #define TRCRTN "WRK_CARR::ConcatenaNota"

   MC & Mc = MezziComponenti[NumMezziComponenti];
   T_PERIODICITA PWrk = B.Periodicita;
   if (PWrk == T_PERIODICITA::InOrario) { // Vuol dire che ha la stessa periodicita' del treno
      PWrk = PTRENO::Get(B.IdentTreno)->PeriodicitaTreno;
   } /* endif */
   if (NumMezziComponenti == 0) {
      Mc.ShiftPartenza = 0;
      Periodicita = PWrk;
      Servizi     = B.Servizi     ;
      Servizi.DiMezzoVirtuale = TRUE;
      if( NotaCarrozzaDiretta( B.IdNota )) {
         DaCarrozzeDirette =1;  // Se TRUE vuol dire che esiste una carrozza diretta che da' origine al mezzo virtuale
      } else {
         DaAltriServizi    =1;  // Se TRUE vuol dire che esistono altri servizi che implicano la prosecuzione
      } /* endif */
   } else {
      assert3(
         (  Nota.IdNota                ==  B.IdNota              &&
            Nota.TrenoFineServizioSeMV ==  B.IdentTreno          &&
            B.TrenoInizioServizioSeMV  ==  Nota.IdentTreno       &&
            Nota.StazioneFineServizio  ==  B.StazioneInizioServizio
         ) ,{
            TRACESTRING( VRS(Ident()) + VRS(Nota.IdNota) + VRS(B.IdNota));
            TRACESTRING( VRS(Nota.IdentTreno)+ VRS(Nota.TrenoFineServizioSeMV)+ VRS(Nota.StazioneFineServizio));
            TRACESTRING( VRS(B.IdentTreno)+ VRS(B.TrenoInizioServizioSeMV)+ VRS(B.StazioneInizioServizio));
         } );
      MC & McLast = MezziComponenti[NumMezziComponenti-1];
      McLast.StazioneDiCambio = B.StazioneInizioServizio;
      PTRENO & LastTreno = *PTRENO::Get(McLast.IdentTreno);
      // Vedo se ho avuto il passaggio per la mezzanotte
      int OraLast;
      if (NumMezziComponenti == 1) {
         OraLast = LastTreno.OraPartenza(LastTreno.CcrDa);
      } else {
         OraLast = LastTreno.OraPartenza( MezziComponenti[NumMezziComponenti-2].StazioneDiCambio );
      }
      PTRENO & NewTreno = *PTRENO::Get(B.IdentTreno);
      int OraNew = NewTreno.OraPartenza( McLast.StazioneDiCambio );
      if (OraLast < 0 || OraNew < 0) { // Incongruenze
         TRACESTRING( VRO(OraLast) +  VRO(OraNew) + VRS(Ident()) + VRS(Nota.IdNota) + VRS(B.IdentTreno) + VRS(McLast.StazioneDiCambio));
         Mc.ShiftPartenza = McLast.ShiftPartenza ;
      } else if ( OraLast > OraNew ) {
         Mc.ShiftPartenza = McLast.ShiftPartenza + 1;
      } else {
         Mc.ShiftPartenza = McLast.ShiftPartenza ;
      } /* endif */
      // ShiftPartenza e' lo shift della stazione di cambio riferito alla data di partenza del mezzo virtuale
      // ShiftVero e' lo shift da applicare alla periodicitÖ delle nota, e deve tener conto anche
      // del fatto che vi potrebbe essere uno shift tra la data di partenza del secondo treno e
      // la stazione di cambio .  Cio' perche' la periodicitÖ delle note e' riferita alla
      // partenza del treno
      int ShiftVero =  Mc.ShiftPartenza ;
      if( OraNew >= 0 && OraNew < NewTreno.OraPartenzaTreno) ShiftVero -- ; // Cambio di data sul secondo treno

      // Vedo se la periodicita' shiftata e' equivalente alla periodicita' di partenza:
      // Se non sono equivalenti utilizzo la nuova periodicitÖ
      if( ! Periodicita.ConfrontaShiftando(PWrk, ShiftVero) ){
         // Qui PWrk e' stato alterato: side effect di ConfrontaShiftando
         if (Periodicita == T_PERIODICITA::InOrario) { // Il nuovo treno impone la periodicita'
            Periodicita = PWrk ;
         } else {
            TRACESTRING(VRS(ShiftVero) + VRS(McLast.ShiftPartenza ) );
            TRACESTRING( VRO(OraLast) +  VRO(OraNew) + VRS(Ident()) + VRS(NewTreno.OraPartenzaTreno) + VRS(Nota.IdNota) + VRS(B.IdentTreno));
            Bprintf2("WARNING: Differente periodicita' su carrozza diretta tra treni %s e %s", St(McLast.IdentTreno), St(B.IdentTreno));
            PWrk.Trace("Periodicita 2¯ Treno (corretta cambio data)");
            Periodicita.Trace("Periodicita mezzo virtuale prima di combinarla con 2¯ treno");
            T_PERIODICITA PWrk2 = B.Periodicita;
            if(Periodicita.ConfrontaShiftando(PWrk2, Mc.ShiftPartenza) ){
               Bprintf2("         Probabilmente la periodicitÖ della nota e' stata erroneamente ");
               Bprintf2("         riferita alla stazione di cambio invece che alla stazione di");
               Bprintf2("         partenza del treno");
            }
            Periodicita &= PWrk ; // Circolazione COMPLESSIVA del mezzo virtuale
         } /* endif */
      } /* endif */
      // Il confronto deve essere limitato alla componente base (MM_INFO)
      if(!((MM_INFO&)Servizi == (MM_INFO&)B.Servizi)){
         // Ho visto che di solito non e' una anomalia, ma rispecchia il fatto che solo una parte delle carrozze e' trasferita
         // Pertanto restringo i servizi a quelli comuni ai due treni
         // Eccezione: Se il primo insieme di servizi e' vuoto lo sovrascrivo
         #if defined(DBG1) || defined(DBG4)
         Bprintf2("Potenziale anomalia su servizi: Treni %s %s Nota %i Servizi finora: %s Servizi di %s : %s",(CPSZ)Ident(),St(B.IdentTreno),B.IdNota ,(CPSZ)Servizi.Decodifica(FALSE),St(B.IdentTreno),(CPSZ)B.Servizi.Decodifica(FALSE));
         #endif
         if (Servizi.Empty()) {
            Servizi = B.Servizi;
         } else {
            Servizi &= B.Servizi;
         } /* endif */
      };

//<<< if  NumMezziComponenti == 0
   } /* endif */
   Mc.IdentTreno = B.IdentTreno;
   Mc.StazioneDiCambio = NESSUNA_STAZIONE;
   Nota = B;
   NumMezziComponenti ++;
   // Vedo se ho treni duplicati
   if (NumMezziComponenti> 1) {
      ELENCO_S Treni;
      for (int i = 0; i < NumMezziComponenti ; i++ ) {
         STRINGA Idt=(STRINGA)MezziComponenti[i].IdentTreno;
         if (Treni.Contiene(Idt)) {
            InLoop = 1;
            Bprintf("WARNING: Il mezzo virtuale %s contiene due volte lo stesso treno",(CPSZ)Ident());
            break;
         } else {
            Treni += Idt;
         } /* endif */
      } /* endfor */
   } /* endif */
//<<< void WRK_CARR::ConcatenaNota NOTA_SERVIZI & B
};
//----------------------------------------------------------------------------
// GeneraMezziVirtualiDaCarrozzeDirette
//----------------------------------------------------------------------------
// Adesso compongo i mezzi virtuali da carrozze dirette: basta una singola scansione
// Compongo a partire dal primo treno aggiungendo i treni in prosecuzione
// ---------------------------------------------------------------------------
void GeneraMezziVirtualiDaCarrozzeDirette(){
   #undef TRCRTN
   #define TRCRTN "GeneraMezziVirtualiDaCarrozzeDirette"
   TryTime(0);
   Bprintf("%s",TRCRTN);
   ARRAY_DINAMICA<WRK_CARR> Appoggio(100); // Area di appoggio: necessaria perche' i treni in prosecuzione possono ramificare
   FORALL(Arr,i3){
      NOTA_SERVIZI  & Arr3 = Arr[i3];
      if(Arr3.TrenoInizioServizioSeMV != NESSUN_TRENO )continue; // E' un treno prosecuzione
      if(Arr3.TrenoFineServizioSeMV   == NESSUN_TRENO )continue; // Non ha treno prosecuzione
      WrkNota.IdNota = Arr3.IdNota; // Per le ricerche nella hash

      WRK_CARR Mv;
      Mv.Clear();
      Mv.ConcatenaNota(Arr3);
      Appoggio.Clear();
      Appoggio += Mv;

      while (Appoggio.Dim()) { // Continuo finche' non ho esaurito tutte le possibili combinazioni
         Mv = Appoggio.Last();
         Appoggio.Elimina(Appoggio.Dim() -1);
         if(Mv.NumMezziComponenti > MAX_MEZZI_REALI) { // Ero in Loop ?
            Mv.NumMezziComponenti = MAX_MEZZI_REALI;
            Mv.InLoop=TRUE;
         };
         if(Mv.InLoop){
            *Hash3.Alloca() = (COMPOS_MEZZOVI)Mv;
            int KeySize = Mv.NumMezziComponenti * sizeof(COMPOS_MEZZOVI::MC);
            Hash3.Metti(KeySize);
            continue;
         }
         if (Mv.Nota.TrenoFineServizioSeMV == NESSUN_TRENO ) { // Non ha treno prosecuzione: Scarico in out
            #ifdef DBG1
            TRACESTRING("Scarico "+STRINGA(Mv.Ident())+" IdNota = "+STRINGA(Mv.Nota.IdNota));
            #endif
            Mv.Periodicita &= T_PERIODICITA::InLimitiOrario ;
            int KeySize = Mv.NumMezziComponenti * sizeof(COMPOS_MEZZOVI::MC);
            assert(Mv.NumMezziComponenti > 1);
            COMPOS_MEZZOVI * PrecedenteRecord = Hash3.Cerca(&Mv,KeySize);
            if (PrecedenteRecord == NULL) {   // Va inserito ex novo nella hash table
               PrecedenteRecord = Hash3.Alloca();
               *PrecedenteRecord = (COMPOS_MEZZOVI)Mv;
               Hash3.Metti(KeySize);
            } else {
               if( PrecedenteRecord->Servizi.Empty() && !PrecedenteRecord->DaCarrozzeDirette){
                  PrecedenteRecord->Periodicita |=  Mv.Periodicita; // Non e' del tutto corretto ma ... Ok
                  PrecedenteRecord->Servizi = Mv.Servizi;
               } else if (PrecedenteRecord->Periodicita ==  Mv.Periodicita  ) { // Basta combinare i servizi
                  #ifdef DBG1
                  if( !(PrecedenteRecord->Servizi == Mv.Servizi)) TRACESTRING(((Mv.Ident()+" Combino servizi diversi su piu' MV: Precedenti: "+PrecedenteRecord->Servizi.Decodifica(FALSE)+" con "+Mv.Servizi.Decodifica(FALSE))));
                  #endif
                  PrecedenteRecord->Servizi |= Mv.Servizi;
               } else if( PrecedenteRecord->Servizi == Mv.Servizi){     // Basta combinare le periodicita'
                  PrecedenteRecord->Periodicita |=  Mv.Periodicita;
               } else {
                  Mv.Dupl = TRUE;
                  PrecedenteRecord = Hash3.Alloca();
                  *PrecedenteRecord = (COMPOS_MEZZOVI)Mv;
                  Hash3.Metti(KeySize);
               } /* endif */
               PrecedenteRecord->DaCarrozzeDirette |= Mv.DaCarrozzeDirette ;
               PrecedenteRecord->DaAltriServizi    |= Mv.DaAltriServizi    ;
//<<<       if  PrecedenteRecord == NULL      // Va inserito ex novo nella hash table
            } /* endif */
//<<<    if  Mv.Nota.TrenoFineServizioSeMV == NESSUN_TRENO     // Non ha treno prosecuzione: Scarico in out
         } else { // Genero le prosecuzioni
            #ifdef DBG1
            TRACESTRING("In elaborazione "+STRINGA(Mv.Ident())+" IdNota = "+STRINGA(Mv.Nota.IdNota)+" Prosegue con " + STRINGA(St(Mv.Nota.TrenoFineServizioSeMV)));
            #endif
            // Cerco i treni / nota prosecuzione potenziali nella Hash Table
            WrkNota.IdentTreno = Mv.Nota.TrenoFineServizioSeMV;
            int NumFound = 0;
            for( NOTA_SERVIZI  *  Next = Hash.Cerca( &WrkNota, KLen); Next ; Next = Hash.CercaNext()) {
               #ifdef DBG1
               TRACESTRING( VRS(WrkNota.IdNota) + VRS(Mv.Ident()) + VRS(St(Next->IdentTreno)) + VRS(St(Mv.Nota.TrenoFineServizioSeMV)));
               TRACESTRING(( STRINGA(" ...") + VRS(St(Mv.Nota.StazioneFineServizio)) + VRS(St(Next->StazioneInizioServizio))+ VRS(St(Mv.Nota.IdentTreno)) + VRS(St(Next->TrenoInizioServizioSeMV))) );
               #endif
               if(  Mv.Nota.TrenoFineServizioSeMV  !=  Next->IdentTreno              )continue; // Not really Found
               if(  Mv.Nota.IdentTreno             !=  Next->TrenoInizioServizioSeMV )continue; // Not really Found
               if(  Mv.Nota.StazioneFineServizio   !=  Next->StazioneInizioServizio  )continue; // Not really Found
               NumFound ++;
               WRK_CARR Mv2 = Mv;
               #ifdef DBG1
               TRACESTRING("  Concateno nota di treno " + STRINGA(St(Next->IdentTreno))+" che prosegue con "+STRINGA(St(Next->TrenoFineServizioSeMV)));
               #endif
               Mv2.ConcatenaNota(*Next); // Ricorsivita'
               Appoggio += Mv2;
            } /* endwhile */
            if (NumFound == 0){
               Bprintf("Errore di algoritmo nota %i: Non trovato treno %s in prosecuzione a %s (%s)",WrkNota.IdNota, St(Mv.Nota.TrenoFineServizioSeMV), St(Mv.Nota.IdentTreno),(CPSZ)Mv.Ident());
               exit(99);
            } /* endif */
//<<<    if  Mv.Nota.TrenoFineServizioSeMV == NESSUN_TRENO     // Non ha treno prosecuzione: Scarico in out
         } /* endif */
//<<< while  Appoggio.Dim      // Continuo finche' non ho esaurito tutte le possibili combinazioni
      }
//<<< FORALL Arr,i3
   } /* endwhile */
//<<< void GeneraMezziVirtualiDaCarrozzeDirette
};
//----------------------------------------------------------------------------
// CorreggiVirtuali
//----------------------------------------------------------------------------
// Controllo se i virtuali con piu' di un cambio debole siano anche
// generati da carrozze dirette: se no mando una segnalazione.
//
// Si noti che non elimino i virtuali in quanto il passeggero puo', in
// effetti, spostandosi sul treno usufruire del collegamento.
//
// Inoltre: Per i virtuali generati SOLO da "altri servizi" controllo se
// per tutti i cambi sia presente un cambio debole o forte (cioe' uno
// dei treni sia origine / destino dell' altro). Se si' elimino il
// virtuale in quanto dovuto ad una semplice imperfezione
// (mando tuttavia un warning).
//
// Infine: Conto il numero di virtuali che compete a ciascun treno
//----------------------------------------------------------------------------
void CorreggiVirtuali(){
   #undef TRCRTN
   #define TRCRTN "CorreggiVirtuali"
   TryTime(0);
   Bprintf("%s",TRCRTN);
   ITERA(Hash3,Mv,COMPOS_MEZZOVI){
      if (Mv.CambiDeboli > 1 && ! Mv.DaCarrozzeDirette) {
         Bprintf2("INFO:    Il mezzo virtuale %s ha %i cambi deboli: Mi aspetterei indicazione di carrozze dirette",(CPSZ)Mv.Ident(),Mv.CambiDeboli);
      } /* endif */
      if (!Mv.DaCarrozzeDirette && !Mv.DaOrigineDestino ) {
         for (int i = Mv.NumMezziComponenti-1; i > 0; i-- ) {
            IDTRENO & T1 = Mv.MezziComponenti[i-1].IdentTreno;
            IDTRENO & T2 = Mv.MezziComponenti[i].IdentTreno;
            // Vedo se ho un cambio da origine-destino
            if (T1 != PTRENO::Get(T2)->TProv && T2 != PTRENO::Get(T1)->TDestinazione) {
               break ; // Il cambio NON e' da origine destino
            } /* endif */
         } /* endfor */
         if(i == 0){
            // Il mezzo virtuale e' inutile perche' l' informazione e' gia' contenuta in
            // altro mezzo virtuale.
            Bprintf("INFO:    Cancellato MV %s da servizi di altro tipo perche' ha solo mezzi in origine/destinazione",(CPSZ)Mv.Ident());
            Hash3.DeleteITERATO(Mv);
         };
      } /* endif */
   } END_ITERA
   ITERA(PTRENO::HashPt,Tr,PTRENO){
   Tr.NumVirtuali =0; // Questa variabile deve essere reimpostata
   } END_ITERA
   ITERA(Hash3,Mv1,COMPOS_MEZZOVI){
      for (int i = Mv1.NumMezziComponenti; i > 0; i-- ) {
         IDTRENO & T1 = Mv1.MezziComponenti[i-1].IdentTreno;
         PTRENO::Get(T1)->NumVirtuali ++;
      }
   } END_ITERA
//<<< void CorreggiVirtuali
};
//----------------------------------------------------------------------------
// CorreggiServizi
//----------------------------------------------------------------------------
// Correzione finale dei servizi
// Poiche' se ho delle carrozze posti a sedere che vanno su di un treno, e
// altre carrozze che vanno su di un altro, non riesco a capire che le note
// 7 indicavano dei posti a sedere, adesso scandisco tutti i mezzi virtuali
// e se non hanno servizi di trasporto ne deduco che debbono avere posti a sedere
// Quindi aggiungo i servizi "posto a sedere" ai treni.
// ---------------------------------------------------------------------------
void CorreggiServizi(){
   #undef TRCRTN
   #define TRCRTN "CorreggiServizi"
   TryTime(0);
   Bprintf("%s",TRCRTN);
   FILE_SERVNU * FServNu =NULL;
   ITERA(Hash3,MezzoVirt,COMPOS_MEZZOVI) {
      if( MezzoVirt.DaCarrozzeDirette                && // altrimenti e' normale non abbia tutti i servizi
         MezzoVirt.NumMezziComponenti            > 1 &&
         MezzoVirt.Servizi.PostiASederePrima    == 0 &&
         MezzoVirt.Servizi.PostiASedereSeconda  == 0 &&
         MezzoVirt.Servizi.CuccettePrima        == 0 &&
         MezzoVirt.Servizi.CuccetteSeconda      == 0 &&
         MezzoVirt.Servizi.VagoniLettoPrima     == 0 &&
         MezzoVirt.Servizi.VagoniLettoSeconda   == 0){
         // Debbo determinare la classe: Bisogna vedere i flags di classe
         for (int i = 0; i < MezzoVirt.NumMezziComponenti; i++ ) {
            PTRENO & Treno = * PTRENO::Get(MezzoVirt.MezziComponenti[i].IdentTreno);
            if (Treno.HaNota_28) {
               MezzoVirt.Servizi.PostiASedereSeconda  = 1;
               MezzoVirt.Servizi.ServizioBase         = 1;
            } else {
               if(Treno.Flags.SecondaClasse == '*') MezzoVirt.Servizi.PostiASedereSeconda  = 1;
               if(Treno.Flags.SecondaClasse == '*') MezzoVirt.Servizi.ServizioBase         = 1;
               if(Treno.Flags.PrimaClasse   == '*') MezzoVirt.Servizi.PostiASederePrima    = 1;
            } /* endif */
         } /* endfor */
         if ( MezzoVirt.Servizi.PostiASederePrima || MezzoVirt.Servizi.PostiASedereSeconda ){
            Bprintf2("Aggiunti servizi posto a sedere IN MODO DEDUTTIVO su Mezzo Virt %s",(CPSZ)MezzoVirt.Ident());
            // Debbo aggiungere il servizio a tutti i treni componenti !
            for (int i = 0; i < MezzoVirt.NumMezziComponenti ; i++ ) {
               PTRENO & Treno = * PTRENO::Get(MezzoVirt.MezziComponenti[i].IdentTreno);
               if (
                  (!Treno.ServiziUniformiTreno.PostiASederePrima && MezzoVirt.Servizi.PostiASederePrima ) ||
                  (!Treno.ServiziUniformiTreno.PostiASedereSeconda && MezzoVirt.Servizi.PostiASedereSeconda )
               ){
                  Bprintf2("   .... E sul treno componente %s",St(Treno.IdTreno));
                  SERVNU ServNu;
                  ServNu.Clear();
                  ServNu.Servizi.PostiASederePrima   = MezzoVirt.Servizi.PostiASederePrima;
                  ServNu.Servizi.PostiASedereSeconda = MezzoVirt.Servizi.PostiASedereSeconda;
                  ServNu.Servizi.ServizioBase        = MezzoVirt.Servizi.ServizioBase;
                  ServNu.Servizi.InProsecuzione      = 1;
                  ServNu.Servizi.PeriodicitaServizi  = MezzoVirt.Periodicita;
                  if (i == 0) {                                        // Primo Mezzo
                     ServNu.Servizi.Da = Treno.CcrDa;
                     ServNu.Servizi.A  = MezzoVirt.MezziComponenti[i+1].StazioneDiCambio;
                  } else if (i == MezzoVirt.NumMezziComponenti - 1) {  // Ultimo Mezzo
                     ServNu.Servizi.Da = MezzoVirt.MezziComponenti[i-1].StazioneDiCambio;
                     ServNu.Servizi.A  = Treno.CcrA;
                  } else {                                             // Intermedi
                     ServNu.Servizi.Da = MezzoVirt.MezziComponenti[i-1].StazioneDiCambio;
                     ServNu.Servizi.A  = MezzoVirt.MezziComponenti[i].StazioneDiCambio;
                  } /* endif */
                  if (ServNu.Servizi.Uniforme()) {
                     Treno.ServiziTreno |= ServNu.Servizi; // Correggo i servizi UNIFORMI di treno
                  } else if (ServNu.Servizi.Da == Treno.CcrDa && ServNu.Servizi.A == Treno.CcrA && ServNu.Servizi.MaxDa.Empty() ) {
                     ServNu.Servizi.TipoSRV = 0; // 0 = Per tutto il mezzo viaggiante
                     if(FServNu == NULL)FServNu = new FILE_SERVNU;
                     FServNu->AddRecordToEnd(VRB(ServNu));
                  } else {
                     ServNu.Servizi.TipoSRV = 3; // 3 = Da stazione a stazione e tutte le intermedie
                     if(FServNu == NULL)FServNu = new FILE_SERVNU;
                     FServNu->AddRecordToEnd(VRB(ServNu));
                  } /* endif */
//<<<          if
               }
//<<<       for  int i = 0; i < MezzoVirt.NumMezziComponenti ; i++
            } /* endfor */
//<<<    if   MezzoVirt.Servizi.PostiASederePrima || MezzoVirt.Servizi.PostiASedereSeconda
         } /* endif */
//<<< if  MezzoVirt.DaCarrozzeDirette                && // altrimenti e' normale non abbia tutti i servizi
      } /* endif */
//<<< ITERA Hash3,MezzoVirt,COMPOS_MEZZOVI
   } END_ITERA;
   if(FServNu) {
      FServNu->ReSortFAST();
      delete FServNu ;
   }
//<<< void CorreggiServizi
};

//----------------------------------------------------------------------------
// Completa
//----------------------------------------------------------------------------
// Scarico su file ed analisi risultati
void Completa(){
   #undef TRCRTN
   #define TRCRTN "Completa"

   TryTime(0);
   Bprintf("%s",TRCRTN);

   // Apro il file dei mezzi virtuali
   FILE_COMPOS_MEZZOVI MezziVirtuali;

   // .......................................
   // Riutilizzo dei codici di mezzo virtuale
   // .......................................
   {  // Blocco per allocare solo temporaneamente le aree

      // Copio il file su di una HASH con i vecchi codici dei mezzi virtuali
      HASH<COMPOS_MEZZOVI>OldMv(20000,MezziVirtuali.Dim() + 100) ; // Questi per l' assegnazione compatibile degli ID ai mezzi virtuali.
      // E questo e' il set che indica quali ID siano utilizzati
      SET IdUtilizzati(max(20000,MezziVirtuali.Dim() + 1000));
      ORD_FORALL(MezziVirtuali,m){
         COMPOS_MEZZOVI & Mv = MezziVirtuali[m];
         if(Mv.Dupl)continue;
         if(Mv.MezzoVirtuale == 0)continue;
         COMPOS_MEZZOVI * Sav = OldMv.Alloca();
         *Sav = Mv;
         int KeySize = Mv.NumMezziComponenti * sizeof(COMPOS_MEZZOVI::MC);
         OldMv.Metti(KeySize);
      }
      // Copio i dati dei NUOVI MV su di una lista lineare e sorto, in modo
      // da avere i NUOVI id ordinati in modo logico: per leggibilita'
      int Stima = (Hash3.AreaAllocata() / sizeof(COMPOS_MEZZOVI)) - MezziVirtuali.Dim() + 3000;
      ARRAY_DINAMICA<COMPOS_MEZZOVI> Arr(Stima);
      ITERA(Hash3,Mv0,COMPOS_MEZZOVI){
         // L' ultimo mezzo non deve cambiare
         assert3(Mv0.MezziComponenti[Mv0.NumMezziComponenti-1].StazioneDiCambio == NESSUNA_STAZIONE,Mv0.MezzoVirtuale);
         int KeySize = Mv0.NumMezziComponenti * sizeof(COMPOS_MEZZOVI::MC);
         COMPOS_MEZZOVI * MvOld = OldMv.Cerca(&Mv0,KeySize);
         if (MvOld) {     // Esisteva: riutilizzo il codice
            Mv0.MezzoVirtuale = MvOld->MezzoVirtuale;
            IdUtilizzati.Set(Mv0.MezzoVirtuale);
         } else if(!Mv0.Dupl){  // Non e' necessario per i duplicati
            Arr += Mv0; // Aggiungo alla lista lineare
         }
      } END_ITERA
      Arr.Sort(SortMv);

      // Adesso assegno i codici ai nuovi mezzi virtuali
      int NextId = 1;
      ORD_FORALL(Arr,a){
         int KeySize = Arr[a].NumMezziComponenti * sizeof(COMPOS_MEZZOVI::MC);
         COMPOS_MEZZOVI & Mv = *Hash3.Cerca(&Arr[a],KeySize);
         assert(&Mv != NULL);
         // Cerco l' id da utilizzare
         while (IdUtilizzati.Test(NextId))NextId ++;
         Mv.MezzoVirtuale = NextId;
         IdUtilizzati.Set(Mv.MezzoVirtuale);
         // Continuo per tutti i mezzi virtuali duplicati con lo stesso codice
         for (COMPOS_MEZZOVI * PMv = Hash3.CercaNext() ; PMv ; PMv = Hash3.CercaNext()) {
            PMv->MezzoVirtuale = Mv.MezzoVirtuale ;
         } /* endfor */
      };
   }

   // ---------------
   // Scarico su file
   // ---------------
   Bprintf("Salvo i dati di tutti i treni.");
   MezziVirtuali.Clear("Composizione Mezzi Virtuali");
   TryTime(0);
   ITERA(Hash3,Mv1,COMPOS_MEZZOVI){
   MezziVirtuali.AddRecordToEnd(VRB(Mv1));
   } END_ITERA
   PTRENO::Save();
   // Resort
   MezziVirtuali.ReSort(SortMv);

   // Questa scansione fa degli ulteriori controlli e raccoglie dati statistici
   // Non si considerano i mezzi virtuali Duplicati
   Bprintf("Raccolta dati statistici");
   TryTime(0);
   int NumVirtuali      = 0;
   int NumVirtualiOD    = 0;
   int NumVirtualiCD    = 0;
   int NumVirtualiAS    = 0;
   int NumVirtuali2Piu  = 0;
   int NumVirtualiSing  = 0;
   int NumVirtualiDeboli= 0;
   ITERA(Hash3,Mv,COMPOS_MEZZOVI){
      if (!Mv.Dupl) {
         NumVirtuali ++;
         if(Mv.DaCarrozzeDirette        ) NumVirtualiCD     ++;
         if(Mv.DaAltriServizi           ) NumVirtualiAS     ++;
         if(Mv.DaOrigineDestino         ) NumVirtualiOD     ++;
         if(Mv.CambiDeboli              ) NumVirtualiDeboli ++;
         if(Mv.NumMezziComponenti >=   2) NumVirtuali2Piu   ++;
         if(Mv.NumMezziComponenti ==   1) NumVirtualiSing   ++;
      } /* endif */
      if (Mv.NumMezziComponenti < 2)continue; // Mostro solo quelli con piu' di un treno
      if (Mv.Dupl) continue                 ; // Non segnalo anomalia sui duplicati: e' normale che i servizi abbiano caratteristiche differenti
      if (!(Mv.DaCarrozzeDirette || Mv.DaOrigineDestino)) {
         Bprintf("WARNING : Mezzo virtuale %s non deriva da carrozze dirette ne da treni Origine/Destinazione",(CPSZ)Mv.Ident());
      } /* endif */
   } END_ITERA

   Bprintf2("---------------------------------------------------------");
   Bprintf2("Elenco dei mezzi virtuali generati e mezzi componenti:");
   Bprintf2("---------------------------------------------------------");
   #ifndef DBG6
   Bprintf2("Sono omessi i mezzi composti di un solo treno");
   #endif
   TryTime(0);
   ORD_FORALL(MezziVirtuali,i4){
      COMPOS_MEZZOVI &Mv  = MezziVirtuali[i4];
      #ifndef DBG6
      if (Mv.NumMezziComponenti < 2)continue; // Mostro solo quelli con piu' di un treno
      #endif
      STRINGA SMv;
      for (int i = 0;i < Mv.NumMezziComponenti; i++ ) {
         COMPOS_MEZZOVI::MC &Mc = Mv.MezziComponenti[i];
         SMv += St(Mc.IdentTreno);
         BOOL Ok = i < Mv.NumMezziComponenti-1;
         if(!Ok){
            PTRENO & Tren = * PTRENO::Get(Mc.IdentTreno);
            Ok = Tren.CcrA != Mc.StazioneDiCambio; // Non deve essere la stazione finale del treno
            Ok &= Mc.StazioneDiCambio != NESSUNA_STAZIONE;   // Deve essere significativo
         }
         if (Ok){
            SMv += " @(";
            SMv += St(Mc.StazioneDiCambio);
            SMv += ") ";
         } /* endif */
      } /* endfor */
      SMv += " Mv N¯ " + STRINGA(Mv.MezzoVirtuale);
      if (Mv.Dupl) SMv += " Duplicato";
      if (Mv.Periodicita != T_PERIODICITA::InLimitiOrario )SMv += " Periodico";
      SMv += " Da:";
      if(Mv.DaCarrozzeDirette )SMv += " CD";
      if(Mv.DaAltriServizi    )SMv += " AS";
      if(Mv.DaOrigineDestino  )SMv += " OD";
      if(Mv.CambiDeboli )SMv += " Cambi Deb. N¯ "+STRINGA(Mv.CambiDeboli);
      SMv += " Servizi: ";
      SMv += Mv.Servizi.Decodifica(FALSE);
      Bprintf2("%s",(CPSZ)SMv);
      #if defined(DBG2) || defined(DBG5)
      //      if (Mv.Periodicita != T_PERIODICITA::InLimitiOrario) {
      SMv = " Periodicita' di: "+Mv.Ident();
      Mv.Periodicita.Trace(SMv);
      //      } /* endif */
      #endif
//<<< ORD_FORALL MezziVirtuali,i4
   }
   Bprintf("Statistiche su mezzi virtuali: ");
   Bprintf("N¯ records su file                                     = %i",MezziVirtuali.Dim());
   Bprintf("N¯ Virtuali di qualunque tipo                          = %i",NumVirtuali      );
   Bprintf("N¯ Virtuali da indicazione dei mezzi origine/destinaz. = %i",NumVirtualiOD    );
   Bprintf("N¯ Virtuali da carrozze dirette                        = %i",NumVirtualiCD    );
   Bprintf("N¯ Virtuali da servizi diretti                         = %i",NumVirtualiAS    );
   Bprintf("N¯ Virtuali con 2 o piu' mezzi viaggianti              = %i",NumVirtuali2Piu  );
   Bprintf("N¯ Virtuali composti da un solo treno                  = %i",NumVirtualiSing  );
   Bprintf("N¯ Virtuali che hanno almeno un cambio debole          = %i",NumVirtualiDeboli);
//<<< void Completa
};
