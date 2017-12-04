//----------------------------------------------------------------------------
// FT_STEPB.CPP:
//    Controlla le fermate dai files .T scartando le fermate non accettabili
//    Scrive i dati dei treni e delle fermate nel formato "neutro" usato
//       per il caricamento dati del motore
//    Elimina i mezzi virtuali che non abbiano almeno due fermate valide
//    Identifica e mette in forma normalizzata la periodicita'
//    delle fermate, considerando i passaggi per la mezzanotte.
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2
#pragma option -Od
//----------------------------------------------------------------------------
/*
  
   ALGORITMO: descrizione generale
  
      Scandisce localita' controllando i codici CCR: sono subito scartati
      quelli non accettabili.
  
      Scandisce localita' controllando la corrispondenza tra nomi e codici CCR:
      sono scartati quelli non accettabili.
  
      Sono identificate e scartate le fermate non accettabili
  
      Si determina la periodicita' delle fermate.
  
         Il problema e' esacerbato dal fatto che la periodicita' delle
         fermate e' considerata modificata al passaggio del treno dalla
         mezzanotte:  cio' viene gestito da FS introducendo una nuova
         coppia di records di tipo 3/4 che indicano il cambio di
         periodicita'.
  
         La coppia di record non e' inserita subito ma alla prima stazione
         di fermata ( o forse al cambio di fascicolo: cio' non e' chiaro)
         e' pertanto necessario lavorare con una logica di "trigger" per
         identificare la corretta periodicita' per tutte le fermate.
  
         I dati di output riportano tutte le periodicita' riferite alla
         data di partenza del mezzo virtuale.
  
  
  
   PSEUDOCODIFICA
  
  
      Apro il grafo della rete FS (precaricato dai programmi FP_UNO,
      FP_DUE, FP_TRE in tempi non sinbcronizzati con la fornitura orari)
      e analizzo il file delle multistazioni ( =  collegamenti urbani).
  
      Apro il file delle localita' (LOCALITA.T) fornito da FS e l' array
      di decodifica codici CCR ->  ID delle stazioni (definito in
      ID_STAZI.HPP).
  
      Eseguo una scansione di LOCALITA.T e  scarto i codici CCR anomali
      (non definiti su ID_STAZI).  Viene anche fatto un controllo sui
      nomi per identificare casi in cui il nome della stazione riportato
      su localita.t NON corrisponde al nome su ID_STAZI
  
      Scansione di tutti i treni virtuali per :
         - Espansione periodicita'
         - Scrittura dettaglio fermate del mezzo virtuale.
  
         Per tutti i treni componenti del Mezzo virtuale:
  
           Per tutte le fermate del Mezzo Componente:
  
               I record di tipo 4 e 6 sono utilizzati per caricare le
               periodicita' delle fermate:  Dai records di tipo 4 carico
               la periodicita' corrente (che rimane valida per tutte le
               fermate successive) mentre i records di tipo 6 modificano
               la periodicita' della sola fermata cui si applicano.
  
               Le correzioni di data si applicano solo al successivo
               record di tipo 4:  infatti FS non le applica direttamente
               al cambio della mezzanotte ma solo al cambio fascicolo (o
               alla prima fermata ?).
  
  
               Gestisco le periodicita', corrette per il giorno
               successivo alla partenza del treno e simili amenita'.
  
               Alla fine carico i dati di fermata in una struttura
               temporanea e li scrivo sul file delle fermate.
  
           Le periodicita' delle fermate non sono scritte su file ma
           caricate in un' area temporanea (Pfv) :  Se non sono UNIFORMI
           sono scaricate su un file di appoggio (PerFv).
  
      Infine vengono sortati i files generati.
  
*/
//----------------------------------------------------------------------------
// #define DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
//----------------------------------------------------------------------------
// #define SCRIVI2   // Scrittura del file F2346.TMP contenente solo i records di tipo 2 e 3  ...
// #define SCRIVI3   // Scrittura del file F2346.TMP contenente solo i records di tipo 2 e 3  ...
// #define SCRIVI4   // Scrive anche i records di tipo 4
// #define SCRIVI6   // Scrive anche i records di tipo 6
// #define DETFERM   // Scrive nel dettaglio per quali treni scarto fermate per codici CCR errati
//----------------------------------------------------------------------------
#ifdef PROFILER_ABILITATO
#error "Contrasto tra profiling motore e profiling utility"
#endif
//#define PROFILER_ABILITATO
//----------------------------------------------------------------------------


#include "FT_PATHS.HPP"  // Path da utilizzare
#include "oggetto.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ML_IN.HPP"
#include "elenco.h"
#include "eventi.h"
#include "ctype.h"
#include "seq_proc.hpp"
#include "scandir.h"
#include "MM_BASIC.HPP"


#define PGM   "FT_STEPB"

ELENCO_S NomiGenerici("TORRE","PIAZZA","MARITTIMA","CAMPO");
const char Zeroes[] = "00000000000000000000000000000000000000000000000000000000000000000000000000000000";

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
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
   SetPriorita(); // Imposta la priorita'
   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore
   TryTime(0);
   
   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);
   
   ELENCO_S ErroriSignificativi;
   
   PROFILER::Descrizioni += "Tempo di accesso ai mezzi virtuali";
   PROFILER::Descrizioni += "Tempo di accesso alle fermate     ";
   PROFILER::Descrizioni += "Tempo di accesso ai dati dei treni";
   PROFILER::Descrizioni += "Tempo di accesso a localita'      ";
   PROFILER::Descrizioni += "Tempo scrittura MV                ";
   PROFILER::Descrizioni += "Tempo scrittura FV                ";
   PROFILER::Descrizioni += "Tempo scrittura FV non uniformi   ";
   PROFILER::Descrizioni += "Tempo totale (per ciclo)          ";
   PROFILER::Descrizioni += "Tempo di SORT                     ";
   PROFILER::Descrizioni += "Tempo di controllo finale sequenza";
   
   // Caricamento delle stazioni per cui FS ha utilizzato sinonimi (diciamo cosi' )
   ARRAY_ID Sinonimi;
   {
      FILE_RO FSinonimi(PATH_IN "SINONIMI.T");
      STRINGA Linea;
      while (FSinonimi.gets(Linea)) {
         if(Linea[0] == ';')continue;
         Sinonimi += Linea.ToInt();
      } /* endwhile */
   }
   
   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   T_PERIODICITA LimitiOrario = T_PERIODICITA::InLimitiOrario; // Amedeo: Modifica
   
   BIN_ELENCO_S TreniConFermateDaScartare; // Per anomalie su Localita'
   
   // APRO il grafo
   PROFILER::Clear(FALSE); // Per aprire il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);
   
   // Apro l' archivio decodifica codici CCR
   CCR_ID Ccr_Id(PATH_DATI);
   
   if (CCR_ID::CcrId == NULL) CCR_ID::CcrId = & Ccr_Id;
   
   // Faccio una analisi delle multistazioni in modo da segnalare eventuali anomalie
   Bprintf("Controllo dei dati delle multistazioni (matrice tempi di interscambio)");
   GEST_MULTI_STA Gestore(PATH_CVB,&Out);
   // E faccio un dump
   Gestore.Trace("Dati delle multistazioni:");
   
   // Apro l' archivio LOCALITA'
   STRINGA PathDa(PATH_IN),ExtFl(".T");
   if (getenv("USADATIESTRATTI")){ PathDa=PATH_OUT;ExtFl=".XTR";};
   FILE_LOCALITA Loc(PathDa + "LOCALITA" + ExtFl);
   
   // Apro il file dei records 2 e 3
   #if  defined(SCRIVI2) || defined(SCRIVI3) ||  defined(SCRIVI4) || defined(SCRIVI6)
   FILE_RW F23(PATH_OUT "F2346.TMP");
   F23.SetSize(0);
   #endif
   
   // Identifica i codici CCR anomali e li mette in una hash table
   HASH<DWORD> CcrSegnalati(500);
   // Questo e' l' elenco delle coppie CCR / Nome stazione approvati
   struct TESTNOME {
      DWORD Ccr;
      char Nome[20];
   };
   HASH<TESTNOME> NomiOk(4096);
   
   TryTime(0);
   // ---------------------------------------------------------
   // Scansione archivio localita'
   // ---------------------------------------------------------
   Bprintf2("=============================================================");
   Bprintf2("Identificazione delle stazioni senza corrispettivo da CVB");
   Bprintf2("=============================================================");
   Bprintf2("Criteri Utilizzati: ");
   Bprintf2("  - I codici CCR debbono esistere tra quelli forniti dal CVB:");
   Bprintf2("  - Il codice non deve essere relativo al servizio cumulativo");
   Bprintf2("  - Viene fatto un controllo sommario sulla corrispondenza delle descrizioni delle stazioni:");
   Bprintf2("    1) La prima lettera deve corrispondere");
   Bprintf2("    2) Oppure debbono avere una parola 'significativa' di almeno 5 lettere in comune");
   Bprintf2("    Data la limitatezza del controllo non e' probabile identifichi TUTTI i casi anomali" );
   Bprintf2("    NB: Alcuni casi anomali sono catalogati in un file \"SINONIMI\" e " );
   Bprintf2("    gestiti a programma.                                              " );
   Bprintf2("=============================================================");
   Bprintf("Scansione Localita'");
   ULONG Nr = Loc.NumRecordsTotali();
   Bprintf("LOCALITA' : records totali = %i di %i Bytes",Nr,sizeof(Loc.RecordCorrente()));
   int Ok  =0;
   int Bad =0;
   int Scartate = 0;
   STRINGA LastTreno;
   Loc.ModificaPageSize(64000); // 64000 Bytes: efficace scansione sequential
   for (int i = 0 ;i < Nr ; i++ ) { // Scansione Localita'
      TryTime(i);
      if(Loc[i].TestRecord != 'A'){
         Bad ++;
         if (!TreniConFermateDaScartare.Contiene(LastTreno)) TreniConFermateDaScartare += LastTreno;
         continue;
      } else {
         LastTreno = Loc.RecordCorrente().IdentTreno;
      }
      Ok ++;
      ULONG Ccr = Loc.RecordCorrente().Codice.Cod();
      if (IgnoraFermata(Ccr))continue;
      int Result = Ccr_Id.Id(Ccr);
      if(Result == 0){      // Non identificata
         if(CcrSegnalati.Cerca(&Ccr,4) == NULL){
            Bprintf2("Non Identificata Stazione codice CCR %u  %s",Ccr,St(Loc.RecordCorrente().Descrizione));
            *CcrSegnalati.Alloca() = Ccr;
            CcrSegnalati.Metti(sizeof(DWORD));
            Scartate ++;
         } /* endif */
      } else if(toupper(Stazioni[Result].NomeStazione[0]) != toupper(Loc.RecordCorrente().Descrizione[0])){
         TESTNOME Tst;
         Tst.Ccr = Ccr;
         memcpy(Tst.Nome,Loc.RecordCorrente().Descrizione, sizeof(Tst.Nome));
         if(NomiOk.Cerca(&Tst,sizeof(Tst))== NULL && CcrSegnalati.Cerca(&Ccr,4) == NULL){ // Non OK e non segnalato
            // Divido i nomi in Tokens, e cerco un eguaglianza tra le localita'
            ELENCO_S Primo   = STRINGA(Stazioni.RecordCorrente().NomeStazione).UpCase().Tokens(" .;-'");
            ELENCO_S Secondo = STRINGA(St(Loc.RecordCorrente().Descrizione)).UpCase().Tokens(" .;-'");
            ORD_FORALL(Primo,s){
               if(Primo[s].Dim() >= 5 && !NomiGenerici.Contiene(Primo[s]) && Secondo.Contiene(Primo[s])){
                  break; // OK i nomi sono piu' o meno corrispondenti
               }
            }
            if(s >= Primo.Dim()){ // Non lo ho trovato
               if(Sinonimi.Contiene(Ccr)){
                  Bprintf2("Ok PER SINONIMI Stazione codice CCR %i Nome su LOCALITA '%s' nome da CVB '%s'",Ccr,St(Loc.RecordCorrente().Descrizione),Stazioni.RecordCorrente().NomeStazione);
                  memcpy(NomiOk.Alloca(), &Tst, sizeof(Tst));
                  NomiOk.Metti(sizeof(Tst));
               } else {
                  Bprintf2("SCARTATA Stazione codice CCR %i Nome su LOCALITA '%s' nome da CVB '%s'",Ccr,St(Loc.RecordCorrente().Descrizione),Stazioni.RecordCorrente().NomeStazione);
                  Scartate ++;
               } /* endif */
               *CcrSegnalati.Alloca() = Ccr;
               CcrSegnalati.Metti(sizeof(DWORD));
            } else {
               memcpy(NomiOk.Alloca(), &Tst, sizeof(Tst));
               NomiOk.Metti(sizeof(Tst));
            }
//<<<    if NomiOk.Cerca &Tst,sizeof Tst  == NULL && CcrSegnalati.Cerca &Ccr,4  == NULL   // Non OK e non segnalato
         } /* endif */
//<<< if Result == 0        // Non identificata
      }
//<<< for  int i = 0 ;i < Nr ; i++     // Scansione Localita'
   } /* endfor */
   if (Scartate) {
      Bprintf2("øøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøø");
      Bprintf("ATTENZIONE: Scartate %i stazioni per mancata corrispondenza con anagrafiche CVB",Scartate);
      Bprintf2("øøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøø");
   } /* endif */
   
   
   if (TreniConFermateDaScartare.Dim()) {
      
      Bprintf("øøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøø");
      Bprintf("ATTENZIONE: I seguenti treni hanno dei records NON VALIDI in LOCALITA.T ");
      Bprintf("øøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøøø");
      TreniConFermateDaScartare.Sort();
      STRINGA Riga;
      ORD_FORALL(TreniConFermateDaScartare, i){
         if(Riga.Dim())Riga += ", ";
         Riga += "'" + TreniConFermateDaScartare[i]+"'";
         if(Riga.Dim() > 68){
            Bprintf("%s",(CPSZ)Riga);
            Riga.Clear();
         }
      };
      if(Riga.Dim()) Bprintf("%s",(CPSZ)Riga);
   } /* endif */
   
   TryTime(0);
   
   // Apro l' archivio TABORARI
   FILE_TABORARI TabOr(PathDa + "TABORARI" + ExtFl);
   
   // Modifico la pagesize di localita perche' adesso accedo in modo random
   Loc.ModificaPageSize(2000); // 2000 Bytes: efficace scansione Random->sequential
   // Modifico la pagesize di taborari per accesso random
   TabOr.ModificaPageSize(2000); // 2000 Bytes: efficace scansione Random->sequential
   int Nr2 = TabOr.Dim();
   
   // ---------------------------------------------------------
   // Costruzione tabelle di output
   // ---------------------------------------------------------
   {
      Bprintf2("=============================================================");
      Bprintf("Costruzione Tabelle di Output");
      Bprintf2("=============================================================");
      PTRENO::Restore(); // Parto dai dati caricati l' ultima volta
      // Apro il file dei mezzi virtuali
      FILE_COMPOS_MEZZOVI MezziVirtuali;
      
      // Costruzione della tabella di output dei mezzi virtuali
      F_MEZZO_VIRTUALE TabTv(PATH_OUT "M0_TRENV.TM0");
      F_FERMATE_VIRT   TabFv(PATH_OUT "M0_FERMV.TM0");
      F_PERIODICITA_FERMATA_VIRT PerFv(PATH_OUT "M1_FERMV.TM0");
      TabTv.Clear("Tabella Dati Mezzi Virtuali");             // Rendo R/W
      TabFv.Clear("Tabella Fermate Mezzi Virtuali");          // Rendo R/W
      PerFv.Clear("Tabella Periodicita' Fermate M.V.");       // Rendo R/W
      TRACESTRING("Inizio costruzione Tabelle di Output");
      int Maxerr =0;
      int MaxProg = 0;
      int MaxFermVal = 0;
      int NumScarti,NumWrongCcr,NumTransitiScartatiCCR;
      int TotScarti=0, TotScartiFermate=0, TotTreniAffettiFermate=0, TotTreniAffettiTransiti=0;
      int TotMvDisabled =0;
      int Num1Ferm = 0;
      // Queste periodicit… NON sono corrette per riportarle alla data di partenza del MV
      T_PERIODICITA PeriodicitaFermata ; // E' la periodicit… di fermata
      // Queste periodicit… sono corrette per riportarle alla data di partenza del MV
      T_PERIODICITA PeriodicitaCorrente; // E' la periodicit… "standard" indicata dai records di tipo 3 e 4
      // Trigger mi indica che sono al primo record di tipo 5 successivo ad un cambio periodicit…
      // e che pertanto debbo aggiornare PeriodicitaCorrente;
      int  Trigger;
      PERIODICITA_FERMATA_VIRT *Pfv = new PERIODICITA_FERMATA_VIRT[1024];
      
      ORD_FORALL(MezziVirtuali,Tv){
         
         PROFILER::Cronometra(7,TRUE) ;
         int GiornoSuccessivoArrivo  = 0 ; // Della fermata corrente, rispetto alla partenza mezzo virtuale
         int GiornoSuccessivoPartenza= 0 ; // Della fermata corrente, rispetto alla partenza mezzo virtuale
         int LastPartenza            = -1; // Ultima ora di partenza, reset a cambio giorno, -1 se non sono partito
         Trigger = 0; // 0 = Non triggered
         PROFILER::Cronometra(0,TRUE) ;
         COMPOS_MEZZOVI TrenoVirtuale = MezziVirtuali[Tv]; // Vado per copia perche' modifico i dati
         PROFILER::Cronometra(0,FALSE);
         if(TrenoVirtuale.Dupl) continue; // Ignoro il mezzo virtuale duplicato (va utilizzato per i soli servizi)
         MEZZO_VIRTUALE Mvr;
         FERMATE_VIRT  FermataCorrente;
         int Prog=0,FermateValide=0;
         int LastCambio = 0; // Salva codice CCr della precedente stazione di cambio
         int NewCambio  = 0; // Codice CCR della prossima stazione di cambio
         int KmMezzo  = 0;
         BOOL OkLast5 ; // Mi dice se ho accettato l' ultimo record di tipo 5
         
         TryTime(Tv);
         
         NumScarti=NumWrongCcr=NumTransitiScartatiCCR=0;
         ZeroFill(Mvr);
         ZeroFill(FermataCorrente);
         
         Mvr.MezzoVirtuale          = TrenoVirtuale.MezzoVirtuale             ;
         Mvr.NumMezziComponenti     = TrenoVirtuale.NumMezziComponenti        ;
         Mvr.PeriodicitaDisuniformi = FALSE                                   ;
         Mvr.DaCarrozzeDirette      = TrenoVirtuale.DaCarrozzeDirette         ;
         Mvr.DaAltriServizi         = TrenoVirtuale.DaAltriServizi            ;
         Mvr.DaOrigineDestino       = TrenoVirtuale.DaOrigineDestino          ;
         Mvr.CambiDeboli            = TrenoVirtuale.CambiDeboli               ;
         Mvr.InLoop                 = TrenoVirtuale.InLoop                    ;
         Mvr.FittizioLinearizzazione= 0;
         
         for (int i = 0; i < TrenoVirtuale.NumMezziComponenti; i++ ){
            COMPOS_MEZZOVI::MC & Mc = TrenoVirtuale.MezziComponenti[i];
            PROFILER::Cronometra(2,TRUE);
            PTRENO & Treno = *PTRENO::Get(Mc.IdentTreno);
            PROFILER::Cronometra(2,FALSE);
            // TABORARI R3 = TabOr[Treno.PTaborariR2 + 1];
            // assert(R3.TipoRecord == '3' );
            Mvr.Mv[i].NumeroMezzo         = Chk(Treno.NumeroTreno,17);
            Mvr.Mv[i].CcrStazioneDiCambio = Chk(Mc.StazioneDiCambio.Cod(),24);
            assert2(i == (TrenoVirtuale.NumMezziComponenti-1) || Mvr.Mv[i].CcrStazioneDiCambio != 0,VRS(i)+VRS(TrenoVirtuale.MezzoVirtuale)+VRS(Mvr.Mv[i].CcrStazioneDiCambio));
            if (i < (TrenoVirtuale.NumMezziComponenti-1) && Ccr_Id.Id(Mc.StazioneDiCambio.Cod()) == 0 ) {
               STRINGA T1(TrenoVirtuale.MezziComponenti[i].IdentTreno);
               STRINGA T2(TrenoVirtuale.MezziComponenti[i+1].IdentTreno);
               if (TrenoVirtuale.Servizi.Prenotabilita) {
                  Bprintf("=====================================================");
                  Bprintf("Mezzo virtuale %i Cambio treni %s->%s a stazione CCR %s non valido",
                     TrenoVirtuale.MezzoVirtuale, (CPSZ)T1, (CPSZ)T2, St(Mc.StazioneDiCambio));
                  Bprintf("Errore grave: il mezzo e' prenotabile");
                  Bprintf("=====================================================");
               } else {
                  Bprintf2("Mezzo virtuale %i Cambio treni %s->%s a stazione CCR %s non valido",
                     TrenoVirtuale.MezzoVirtuale, (CPSZ)T1, (CPSZ)T2, St(Mc.StazioneDiCambio));
               } /* endif */
            } /* endif */
            Mvr.Mv[i].ShiftPartenza    = Mc.ShiftPartenza;
            Mvr.Mv[i].ShiftDaPTreno    = 0               ;
            Mvr.Mv[i].IdentTreno       = Treno.IdTreno;
            Mvr.Mv[i].TipoMezzo        = Treno.ServiziTreno.TipoMezzo;
            memcpy(Mvr.Mv[i].KeyTreno,Treno.KeyTreno, sizeof(Treno.KeyTreno));
            
            // Dati riassuntivi a livello di mezzo virtuale
            Top(Mvr.TipoMezzoDominante ,Treno.ServiziTreno.TipoMezzo);
            if(Treno.NomeMezzoViaggiante[0] != ' ') Mvr.Mv[i].HaNome = TRUE;
//<<<    for  int i = 0; i < TrenoVirtuale.NumMezziComponenti; i++   
         }
         
         // L' ultima periodicit… corrente mi serve perche' nei files
         // .T al cambio di periodicit… la periodicit… risultante e' la
         // periodicit… in partenza della prossima fermata .
         // Il motore invece si aspetta la combinazione della periodicit…
         // in partenza e della periodicit… in arrivo.
         T_PERIODICITA PeriodicitaInArrivo;
         PeriodicitaInArrivo.ReSet();
         
         // Ora scarico le fermate del mezzo virtuale
         // Nel mentre gestisco le periodicita'
         
         for (i = 0; i < TrenoVirtuale.NumMezziComponenti; i++ ){
            int ShiftDaPTreno     = 0 ; // Indica quanti giorni dopo la partenza del TRENO si PARTE dalla stazione di cambio
            int PartenzaTreno     = -1;
            COMPOS_MEZZOVI::MC & Mc = TrenoVirtuale.MezziComponenti[i];
            PROFILER::Cronometra(2,TRUE);
            PTRENO & Treno = *PTRENO::Get(Mc.IdentTreno);
            PROFILER::Cronometra(2,FALSE);
            NewCambio = Mc.StazioneDiCambio.Cod();
            // TRACESTRING( VRS(TrenoVirtuale.MezzoVirtuale) + VRS(i) + VRS(TrenoVirtuale.NumMezziComponenti) + VRS(NewCambio) );
            assert(i < (TrenoVirtuale.NumMezziComponenti-1) || NewCambio == 0);
            
            // DopoSkipIniziale ed SkipFinale servono per gestire i treni in prosecuzione.
            // Se veri fanno skippare opportunamente delle fermate del treno
            // DopoSkipIniziale se falso fa skippare fermate fino a che si trova la stazione di cambio
            // SkipFinale se vero fa skippare tutte le fermate successive del treno
            // Se tutto e' OK alla fine sono tutti e due veri
            BOOL DopoSkipIniziale = (LastCambio == 0) ;
            BOOL SkipFinale = FALSE ;
            
            int KmOrigineTratta = -1;
            int LastKm   = 0;
            int LastTr=0; // Tipo del precedente record
            int Count5 = Treno.PLocalita;
            for (int j = Treno.PTaborariR2 + 1 ;  j < Nr2 ; j++ ) {   // Scansione TABORARI
               PROFILER::Cronometra(1,TRUE);
               TABORARI & To = TabOr[j];
               PROFILER::Cronometra( 1 , FALSE );
               int Tr = To.TipoRecord - '0';
               if(Tr == 2 )break; // Arrivato al prossimo treno
               switch (Tr) {   // Gestione differenti tipi record
               case 3:
                  break;
               case 4:
                  // Combino le periodicita' : si noti che utilizzo il tipo
                  // dell' ultimo record (LastTr) per capire se debbo continuare a comporre
                  // la periodicita' o se debbo pulire il campo PeriodicitaCorrente
                  // prima di impostarvi la periodicita' letta da file.
                  PeriodicitaCorrente.ComponiPeriodicita(5,To.R4.Periodicita,(LastTr != 4));
                  #ifdef DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
                  PeriodicitaCorrente.Trace("Per Treno da Rec4 prima di shift: MV Nø"+STRINGA(Tv) + " Treno Nø "+STRINGA(i)+" LastTr = "+STRINGA(LastTr));
                  #endif
                  Trigger = 1; // 1 = devo shiftare periodicit…
                  break;
               case 6:
                  if (OkLast5) {
                     PeriodicitaFermata.ComponiPeriodicita(5,To.R4.Periodicita,(LastTr != 6));
                     #ifdef DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
                     PeriodicitaFermata.Trace("Per Fermata da Rec6 no : MV Nø"+STRINGA(Tv) + " Treno Nø "+STRINGA(i)+ " Prog "+STRINGA(Prog)+ " LastTr = "+STRINGA(LastTr));
                     #endif
                     
                     if (GiornoSuccessivoPartenza) Bprintf3("Record di tipo 6 su giorno succ MV Nø %i MC Nø %i",Tv,i);
                     T_PERIODICITA PeriodicitaOut = PeriodicitaFermata; // Devo correggere per lo shift
                     PeriodicitaOut.ShiftMe(GiornoSuccessivoPartenza);
                     
                     // Montagna: non sono sicuro che sia corretto fare l' AND
                     // In realta' temo che a volte il record 6 vada messo in AND ed a volte no
                     PeriodicitaOut &= PeriodicitaCorrente;
                     
                     #ifdef DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
                     PeriodicitaOut.Trace(" Corretta: " VRS(GiornoSuccessivoPartenza) );
                     #endif
                     Pfv[Prog].Periodicita  = PeriodicitaOut ;
                  } /* endif */
                  
                  break;
//<<<          switch  Tr      // Gestione differenti tipi record
               case 5:
                  {
                     PROFILER::Cronometra( 3 , TRUE  );
                     LOCALITA & Lo = Loc[Count5]; // Sono in corrispondenza 1 ad 1
                     PROFILER::Cronometra( 3 , FALSE );
                     Count5 ++; OkLast5 = FALSE;
                     
                     if(Lo.TestRecord != 'A'){
                        // Record da scartare
                        NumScarti ++;
                        break; // Non lo posso utilizzare
                     }
                     
                     int OraA = To.R5.Arrivo.MinMz();
                     int OraP = To.R5.Partenza.MinMz();
                     //  Bprintf3("Info MV Nø %i Treno %s Prog = %i OraArrivo = %i LastPartenza = %i DopoSkipIniziale = %i SkipFinale = %i",
                     //     Mvr.MezzoVirtuale , (CPSZ)Treno.IdTreno,Prog+1, 100*(OraA/60)+OraA%60,100*(LastPartenza/60)+LastPartenza %60, DopoSkipIniziale ,SkipFinale);
                     
                     // Vedo se la fermata e' relativa ad un nuovo giorno, rispetto alla partenza treno
                     if( !DopoSkipIniziale){
                        if( OraP >= PartenzaTreno ){
                           PartenzaTreno = OraP;
                        } else if(OraP == 0 ){
                           // Ignoro tutti i casi in cui non parte ed anche i cambi a mezzanotte esatta
                        } else { // Non entro in merito alla plausibilit…
                           ShiftDaPTreno ++;
                           PartenzaTreno = OraP;
                        }
                     }
                     // Vedo se la fermata e' relativa ad un nuovo giorno
                     if(LastPartenza >=0 && DopoSkipIniziale && !SkipFinale){
                        // Ignoro la stazione iniziale del mezzo virtuale e lo skip iniziale dei mezzi
                        if(OraA >= LastPartenza){
                           // Ok
                        } else if(LastPartenza - OraA >  1200){ // OK cambio data plausibile (max 4 ore tra due fermate)
                           GiornoSuccessivoArrivo ++;
                        } else if(Treno.ServiziTreno.Navale()){ // OK cambio data plausibile (traghetto)
                           GiornoSuccessivoArrivo ++;
                        } else {
                           Bprintf2("Orario di arrivo probabilmente errato MV Nø %i Treno %s Prog = %i OraArrivo = %i LastPartenza = %i",
                              Mvr.MezzoVirtuale , (CPSZ)Treno.IdTreno,Prog+1, 100*(OraA/60)+OraA%60,100*(LastPartenza/60)+LastPartenza %60);
                           Bprintf2("La fermata viene ignorata");
                           break;
                           // GiornoSuccessivoArrivo ++ ; // Comunque non posso rifiutare il dato
                        };
                     }
                     if((DopoSkipIniziale || Lo.Codice.Cod() == LastCambio) &&
                           !(SkipFinale || Lo.Codice.Cod()  == NewCambio)){
                        
                        // Aggiorno SOLO se non e' l' ultimo record del treno: altrimenti avrei risultati anomali
                        if( OraP >= LastPartenza){
                           LastPartenza = OraP;
                        } else if(OraP == 0 && i == TrenoVirtuale.NumMezziComponenti -1 && Lo.Codice == Treno.CcrA) {
                           // E' l' ultima fermata dell' ultimo mezzo virtuale: In realta' non ferma
                        } else if(LastPartenza - OraP >  1200){ // OK cambio data plausibile (max 4 ore tra due fermate)
                           GiornoSuccessivoPartenza ++;
                           LastPartenza = OraP; // Reset
                        } else if(Treno.ServiziTreno.Navale()){ // OK cambio data plausibile (traghetto)
                           GiornoSuccessivoPartenza ++;
                           LastPartenza = OraP; // Reset
                        } else { // Ignoro i casi di cambio di mezzo viaggiante, in cui e' corretto che l' ora di partenza possa essere successiva all' ora di arrivo corrente
                           STRINGA T1(Mc.IdentTreno);
                           Bprintf2("Orario di partenza probabilmente errato MV Nø %i Treno %s Prog = %i OraPartenza = %i LastPartenza = %i",
                              Mvr.MezzoVirtuale , (CPSZ)T1 ,Prog+1, 100*(OraP/60)+OraP%60,100*(LastPartenza/60)+LastPartenza %60);
                           Bprintf2("La fermata viene ignorata");
                           break;
                           // GiornoSuccessivoPartenza ++;
                        }

                        // ---------------  A Montagna ------------------------------------
                        // Adesso ricalcolo la periodicit… corrente ( = Periodicit… in partenza dalla fermata)
                        // ----------------------------------------------------------------
                        if (Trigger == 1) {
                           PeriodicitaCorrente.ShiftMe(GiornoSuccessivoPartenza);
                           #ifdef DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
                           if(GiornoSuccessivoPartenza)PeriodicitaCorrente.Trace("Periodicit… treno corretta per cambio data: MV Nø"+STRINGA(Tv) + " Treno Nø "+STRINGA(i)+ " Prog "+STRINGA(Prog));
                           #endif
                           Trigger = 2; // 2 = debbo aggiustare periodicit… fermata
                        }
//<<<                if  DopoSkipIniziale || Lo.Codice.Cod   == LastCambio  &&
                     }
                     
                     
                     if(Lo.IdentTreno != Treno.IdTreno || memcmp(Lo.CodiceSuQuadro,To.R5.Localita,3)){
                        Bprintf2("Non corrisponde Record 5 Mezzo Virtuale Nø %i Mezzo Componente %s Prog = %i Count5 =%i ", Mvr.MezzoVirtuale , St(Treno.IdTreno),Prog+1,Count5-1);
                        Bprintf2(" Su Loc: %s",St(Lo.IdentTreno));
                        Bprintf2(" Lo.CodiceSuQuadro: %s",St(Lo.CodiceSuQuadro));
                        Bprintf2(" To.R5.Localita: %s",St(To.R5.Localita));
                        if(Maxerr++ >= 100) exit(997);
                        NumScarti   ++;
                     } else if(!DopoSkipIniziale ) {  // Skip iniziale dei records
                        if( Lo.Codice.Cod() == LastCambio){
                           DopoSkipIniziale = TRUE; // Found
                           if(KmOrigineTratta < 0)KmOrigineTratta = It(Lo.KmDaOrigine);
                           LastKm = It(Lo.KmDaOrigine);
                           if(Prog == 0){
                              TRACEINT("Anomalia mezzo virtuale Nø",Mvr.MezzoVirtuale);
                              TRACEVINT(i);
                              TRACEVINT(Prog);
                              TRACEVSTRING2(St(Mc.IdentTreno));
                           } else {
                              // Correggo l' ora della partenza aggiornando l' ultimo record
                              // E' necessario perche' potrebbe essere stata staccata una
                              // carrozza che parte prima del resto del treno
                              // Inoltre aggiorno l' id del treno
                              PROFILER::Cronometra( 5 , TRUE  );
                              TabFv.FixRec(TabFv.Dim()-1);
                              TabFv.RecordCorrente().OraPartenza = Chk(To.R5.Partenza.MinMz(),11);
                              TabFv.RecordCorrente().GiornoSuccessivoPartenza = GiornoSuccessivoPartenza;
                              TabFv.ModifyCurrentRecord();
                              PROFILER::Cronometra( 5 , FALSE );
                              // Correggo anche la periodicit… della fermata
                              // Questo perche' la periodicit… deve tener conto sia della periodicit…
                              // in arrivo (gi… considerata sul precedente treno) che della periodicit… in partenza
                              Pfv[Prog].Periodicita  |= PeriodicitaCorrente;
                           };
//<<<                   if  Lo.Codice.Cod   == LastCambio  
                        };
//<<<                if Lo.IdentTreno != Treno.IdTreno || memcmp Lo.CodiceSuQuadro,To.R5.Localita,3   
                     } else if(SkipFinale) {  // Skip finale dei records
                        // Skip
                     } else {
                        if(KmOrigineTratta < 0)KmOrigineTratta = It(Lo.KmDaOrigine);
                        LastKm = It(Lo.KmDaOrigine);
                        ULONG Ccr = Lo.Codice.Cod();
                        if(Ccr == NewCambio)SkipFinale = TRUE; // Found
                        int Result = Ccr_Id.Id(Ccr);
                        if(Result > 0){      // Controllo utilizzo appropriato codice CCR
                           if(CcrSegnalati.Cerca(&Ccr,4) != NULL){
                              char Buf[512];
                              if(To.R5.Flags.Transito == '*'){
                                 // Per ora NON do segnalazioni di errore sui transiti ma solo un trace
                                 sprintf(Buf,"Treno %s Ignorato transito %s causa codice CCR ANOMALO %i (Probabile errore!)",St(Mc.IdentTreno),Stazioni[Result].NomeStazione,Ccr);
                              } else {
                                 sprintf(Buf,"Treno %s Ignorata fermata %s causa codice CCR ANOMALO %i. Probabile errore.",St(Mc.IdentTreno),Stazioni[Result].NomeStazione,Ccr);
                                 // Bprintf2("%s",Buf);
                              }
                              Result = 0; // Ignoro fermata
                              TRACESTRING(Buf);
                           }
                        }
                        if(Result > 0){      // identificata e presente sul grafo
                           
                           #if defined(DOPPIAF)
                           char Buf[512];
                           sprintf(Buf,"Mezzo Virt. %i Treno %s Ha fermata/transito duplicato %s CCR %i",TrenoVirtuale.MezzoVirtuale,St(Mc.IdentTreno),Stazioni[Result].NomeStazione,Ccr);
                           TRACESTRING(Buf);
                           Bprintf2("%s",Buf);
                           #endif
                           
                           ZeroFill(FermataCorrente); // Init
                           OkLast5 = TRUE;
                           // if(To.R5.Flags.Transito != '*')ControlloDoppieFermate += Result;
                           if(Ccr == NewCambio)TRACESTRING("Mezzo virtuale "+STRINGA(Mvr.MezzoVirtuale)+" Tratta["+STRINGA(i)+"] Stazione di cambio : "+STRINGA(Result)+" '"+STRINGA(Stazioni[Result].NomeStazione)+"'");
                           FermataCorrente.MezzoVirtuale           = Mvr.MezzoVirtuale       ;
                           FermataCorrente.Progressivo             = ++ Prog                 ;
                           FermataCorrente.Progressivo2            = 0                       ;
                           FermataCorrente.Id                      = Chk(Result,13)          ;
                           FermataCorrente.CCR                     = Chk(Ccr,24)             ;
                           // TRACESTRING(VRS(Mvr.MezzoVirtuale) + VRS(FermataCorrente.Progressivo) + VRS(FermataCorrente.CCR) + VRS(FermataCorrente.Id));
                           if( SkipFinale  && i < TrenoVirtuale.NumMezziComponenti -1 ){   // Ultima fermata del treno (ma non ultimo treno)
                              FermataCorrente.TrenoFisico             = Chk(i+1,3)           ;
                           } else {
                              FermataCorrente.TrenoFisico             = Chk(i,3)             ;
                           }
                           FermataCorrente.OraArrivo               = Chk(To.R5.Arrivo.MinMz(),11) ;
                           FermataCorrente.OraPartenza             = Chk(To.R5.Partenza.MinMz(),11);
                           FermataCorrente.GiornoSuccessivoArrivo  = Chk(GiornoSuccessivoArrivo,1)  ;
                           FermataCorrente.GiornoSuccessivoPartenza= Chk(GiornoSuccessivoPartenza,1);
                           FermataCorrente.ProgKm                  = Chk(KmMezzo  +  LastKm - KmOrigineTratta,12);
                           FermataCorrente.Transito                = To.R5.Flags.Transito               == '*';
                           FermataCorrente.FermataFacoltativa      = To.R5.Flags.FermataFacoltativa     == '*';
                           FermataCorrente.FermataPartenza         = To.R5.Flags.FermataViaggInArrivo   != '*';
                           FermataCorrente.FermataArrivo           = To.R5.Flags.FermataViaggInPartenza != '*';
                           FermataCorrente.FermataDiServizio       = To.R5.Flags.FermataDiServizio      == '*';
                           if(FermataCorrente.FermataDiServizio){ // La gestisco come un transito
                              FermataCorrente.Transito                = 1;
                           }
                           if(FermataCorrente.Transito){
                              FermataCorrente.FermataPartenza         = 0;
                              FermataCorrente.FermataArrivo           = 0;
                           }
                           FermataCorrente.HaNote                  = memcmp(To.R5.NoteDiFermata,Zeroes,20)!= 0;
                           FermataCorrente.HaNoteLocalita          = memcmp(Lo.NoteDiLocalita,Zeroes,20)  != 0;
                           
                           // La prima fermata del mezzo virtuale e' di sola partenza, l' ultima di solo arrivo
                           static int LastMVwritten;
                           if (LastMVwritten != Mvr.MezzoVirtuale) {
                              if (TabFv.Dim()) {
                                 PROFILER::Cronometra( 5 , TRUE  );
                                 TabFv.FixRec(TabFv.Dim()-1);
                                 TabFv.RecordCorrente().FermataPartenza = 0;
                                 TabFv.ModifyCurrentRecord();
                                 PROFILER::Cronometra( 5 , FALSE );
                              }
                              FermataCorrente.FermataArrivo          = 0;
                              LastMVwritten = Mvr.MezzoVirtuale;
                           } /* endif */
                           
                           PROFILER::Cronometra( 5 , TRUE  );
                           TabFv.AddRecordToEnd(&FermataCorrente,sizeof(FermataCorrente));
                           PROFILER::Cronometra( 5 , FALSE );
                           if(!FermataCorrente.Transito)FermateValide ++;
                           
                           Pfv[Prog].MezzoVirtuale= FermataCorrente.MezzoVirtuale;
                           Pfv[Prog].Progressivo  = FermataCorrente.Progressivo  ;
                           
                           // ---------------  A Montagna ------------------------------------
                           // Adesso ricalcolo la periodicit… corrente, tenendo conto
                           // che la periodicit… della fermata deve essere la
                           // combinazione della periodicit… di partenza
                           // (= PeriodicitaCorrente) e di quella di arrivo
                           // Tale correzione verr… meno se vi sono dei successivi records di tipo 6
                           // ----------------------------------------------------------------
                           Pfv[Prog].Periodicita   = PeriodicitaCorrente;
                           if (Trigger) {
                              Pfv[Prog].Periodicita  |= PeriodicitaInArrivo;
                              #ifdef DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
                              if(!PeriodicitaInArrivo.Empty()){
                                 PeriodicitaInArrivo.Trace("Per in arrivo MV Nø"+STRINGA(Tv) + " Treno Nø "+STRINGA(i)+" LastTr = "+STRINGA(LastTr));
                                 Pfv[Prog].Periodicita.Trace(" Combinazione periodicit… di partenza ed arrivo Prog " + STRINGA(Prog)+ VRS(GiornoSuccessivoPartenza) );
                              }
                              #endif
                           }
                           
//<<<                   if Result > 0        // identificata e presente sul grafo
                        } else {
                           NumScarti   ++;
                           NumWrongCcr ++;
                           if(FermataCorrente.Transito) NumTransitiScartatiCCR ++;
                        }
                        // ---------------  A Montagna ------------------------------------
                        // Adesso salvo la periodicit… in arrivo
                        // ----------------------------------------------------------------
                        if (Trigger) {
                           PeriodicitaInArrivo = PeriodicitaCorrente;
                           #ifdef DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
                           // PeriodicitaInArrivo.Trace("Salvata periodicit… in arrivo: MV Nø"+STRINGA(Tv) + " Treno Nø "+STRINGA(i)+ " Prog "+STRINGA(Prog));
                           #endif
                           Trigger = 0;
                        }
//<<<                if Lo.IdentTreno != Treno.IdTreno || memcmp Lo.CodiceSuQuadro,To.R5.Localita,3   
                     }
                     break;
//<<<          case 5:
                  }
//<<<          switch  Tr      // Gestione differenti tipi record
               } /* endswitch */
               LastTr=Tr;
//<<<       for  int j = Treno.PTaborariR2 + 1 ;  j < Nr2 ; j++       // Scansione TABORARI
            } /* endfor */
            
            if(KmOrigineTratta >= 0 )KmMezzo  += LastKm - KmOrigineTratta; // Km accumulati nelle tratte precedenti
            
            if(!DopoSkipIniziale){
               Bprintf2("P2: Mancata corrispondenza stazione di cambio (CCR %i) su treno virtuale Nø %i Mezzo viaggiante %s",LastCambio,TrenoVirtuale.MezzoVirtuale,St(Mc.IdentTreno));
            }
            if(!SkipFinale && NewCambio != 0){
               Bprintf2("P3: Mancata corrispondenza stazione di cambio (CCR %i) su treno virtuale Nø %i Mezzo viaggiante %s",NewCambio,TrenoVirtuale.MezzoVirtuale,St(Mc.IdentTreno));
            }
            TotScarti += NumScarti;
            TotScartiFermate +=  NumScarti - NumTransitiScartatiCCR;
            
            if(NumScarti){
               char Buf[500];
               sprintf(Buf,"Scartate %i fermate %i Transiti del mezzo virtuale Nø %i Treno %s: %i per problemi su codice CCR",NumScarti-NumTransitiScartatiCCR,NumTransitiScartatiCCR,TrenoVirtuale.MezzoVirtuale,St(Mc.IdentTreno),NumWrongCcr)
               ;
               TRACESTRING(Buf);
               #ifdef DETFERM      // Scrive nel dettaglio per quali treni scarto fermate per codici CCR errati
               if(NumScarti > NumTransitiScartatiCCR)Bprintf2("%s",Buf);
               #endif
               if(NumScarti > NumTransitiScartatiCCR) TotTreniAffettiFermate ++;
               TotTreniAffettiTransiti ++;
            };
            
            Mvr.Mv[i].ShiftDaPTreno    = ShiftDaPTreno   ;
            LastCambio = NewCambio;
//<<<    for  i = 0; i < TrenoVirtuale.NumMezziComponenti; i++   
         } /* endfor */
         
         
         if(Prog > MaxProg)MaxProg = Prog;
         if(FermateValide > MaxFermVal)MaxFermVal = FermateValide;
         
         Top(Mvr.GiornoSuccessivo ,Chk(GiornoSuccessivoArrivo,2));
         Mvr.NumeroFermateTransiti = Chk(Prog,10) ;
         Mvr.NumeroFermateValide   = Chk(FermateValide,10);
         if(FermateValide < 2){
            Bprintf2("Il treno virtuale Nø %i (Treno %s) non ha almeno 2 fermate valide, e viene annullato",TrenoVirtuale.MezzoVirtuale,St(TrenoVirtuale.MezziComponenti[0].IdentTreno));
            TotMvDisabled ++;
         }
         
         // Controllo uniformita e preparazione periodicita' composita
         Mvr.PeriodicitaMV = Pfv[1].Periodicita;
         #ifdef DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
         Mvr.PeriodicitaMV.Trace("Controllo uniformit… : Periodicit… iniziale MV Nø"+STRINGA(Tv));
         #endif
         for (int k = 2; k <= Prog;k ++ ) {
            if (Pfv[k].Periodicita != Pfv[k-1].Periodicita ) {
               Mvr.PeriodicitaDisuniformi = TRUE;
               Mvr.PeriodicitaMV |= Pfv[k].Periodicita;
               #ifdef DEBUGPERIOD // Mostra come forma le periodicit… nei dettagli
               Pfv[k].Periodicita.Trace("Cambiata periodicit… a fermata Nø "+STRINGA(k));
               #endif
               break;
            } /* endif */
         } /* endfor */
         
         // Scarico delle periodicita' del treno
         if(Mvr.PeriodicitaDisuniformi){
            for (int k = 1; k <= Prog;k ++ ) {
               PROFILER::Cronometra( 6 , TRUE  );
               PerFv.AddRecordToEnd(&Pfv[k],sizeof(PERIODICITA_FERMATA_VIRT));
               PROFILER::Cronometra( 6 , FALSE );
            } /* endfor */
         }
         
         if(Prog > 0 ){
            PROFILER::Cronometra( 4 , TRUE  );
            TabTv.AddRecordToEnd(VRB(Mvr));
            PROFILER::Cronometra( 4 , FALSE );
            Num1Ferm ++;
         } /* endif */
         
         PROFILER::Cronometra(7,FALSE);
//<<< ORD_FORALL MezziVirtuali,Tv  
      };
      
      {  // Aggiornamento ultima fermata ultimo MV
         TabFv.FixRec(TabFv.Dim()-1);
         TabFv.RecordCorrente().FermataPartenza = 0;
         TabFv.ModifyCurrentRecord();
      } /* endif */
      
      
      TryTime(0);
      Bprintf2("Numero totale Fermate scartate: %i\ su %i treni (reali)",TotScartiFermate,TotTreniAffettiFermate);
      Bprintf2("Numero totale Fermate e transiti scartati: %i su %i treni (reali)",TotScarti,TotTreniAffettiTransiti);
      Bprintf2("Numero totale mezzi virtuali disabilitati: %i su %i",TotMvDisabled,MezziVirtuali.Dim());
      Bprintf2("Numero records scritti ( = mezzi virtuali con almeno 1 fermata) : %i ",Num1Ferm);
      
      delete Pfv;
      
      TRACESTRING("Inizio Sort");
      PROFILER::Cronometra( 8 , TRUE  );
      TabTv.ReSortFAST();
      TabFv.ReSortFAST();
      PerFv.ReSortFAST();
      PROFILER::Cronometra( 8 , FALSE );
      
      TRACESTRING("Fine Sort");
      
      // Le fermate ed i mezzi virtuali dovrebbero essere gia' sortati
      // Ma ripeto il sort per eseguire le corrette inizializzazioni
      
      PROFILER::Cronometra( 9 , TRUE  );
      TRACESTRING("Inizio controllo sequenza mezzi virtuali");
      if(!TabTv.ControllaSequenza()){
         Bprintf2( "Errore di sequenza in output sui treni virtuali");
         TRACESTRING("Errore di sequenza in output sui treni virtuali: Accidenti !");
      };
      TRACESTRING("Inizio controllo sequenza fermate virtuali");
      if(!TabFv.ControllaSequenza()){
         Bprintf2( "Errore di sequenza in output su fermate treni virtuali");
         TRACESTRING("Errore di sequenza in output su fermate treni virtuali: Accidenti !");
      };
      TRACESTRING("Fine controlli sequenza");
      PROFILER::Cronometra( 9 , FALSE );
      
//<<< // ---------------------------------------------------------
   }
   
   PROFILER::PutStdOut(TRUE);
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
//<<< int main int argc,char *argv    
}
