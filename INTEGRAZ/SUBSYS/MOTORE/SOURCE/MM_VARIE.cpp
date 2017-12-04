//----------------------------------------------------------------------------
// MM_VARIE.CPP
//----------------------------------------------------------------------------
// Funzioni varie
//----------------------------------------------------------------------------
//
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2
#define PROFILER_ABILITATO

#define MODULO_OS2_DIPENDENTE
#define INCL_DOSPROFILE
#define INCL_DOSMEMMGR
#define INCL_DOSMODULEMGR

#include "id_stazi.hpp"
#include "mm_font.hpp"

//----------------------------------------------------------------------------
// Variabili statiche a gestione particolare: debbono stare prima delle include
//----------------------------------------------------------------------------
// Questa e' l' istance globale per accedere alle stazioni
STAZIONI & Stazioni(*(STAZIONI*)NULL);
// Questa per accedere all' ultimo BEEP
STRINGA _export LastBeep;

#include "mm_varie.hpp"
#include "mm_grafo.hpp"
#include "alfa0.hpp"
#include <stdarg.h>


//----------------------------------------------------------------------------
// Variabili statiche
//----------------------------------------------------------------------------
CLASSIFICA_TRENI * CLASSIFICA_TRENI::Classifiche;

ULONG Time(); // La dichiaro direttamente

#undef TRCRTN
#define TRCRTN "PROFILER"

int Do0(){return 0;};

//----------------------------------------------------------------------------
// Tempo in microsecondi con correzione
//----------------------------------------------------------------------------
DWORD Cronometra(BOOL StartStop,DWORD & Elapsed){
   #undef TRCRTN
   #define TRCRTN "Cronometra()"
   static DWORD Freq;
   static DWORD Delay;
   if(Freq == 0){
      // Aggiusto il delay
      DosTmrQueryFreq(&Freq);
      Freq /= 1000;       // Causa una piccola imprecisione: Altrimenti usare un double ...
      int i = 10;
      do {
         Delay = 0;
         ULONG Tot = 0;
         DWORD Dummy;
         for (int j = 0;j < 30 ;j++ ) {
            Cronometra(TRUE,Dummy);
            Tot += Cronometra(FALSE,Dummy);
            DosSleep(10); // Se no i tempi non vengono precisi
         } /* endfor */
         Delay = Tot / 30;
      } while (i-- > 0 && Delay > 500  ); // Fino a 10 retry (in caso di task switch)
      TRACEVLONG(Freq);
      TRACEVLONG(Delay);
   }
   QWORD Time;
   DosTmrQueryTime(&Time);
   // NB:
   // Per calcolare il deltatime e' sufficente utilizzare la lowword:
   // Se la highword differisce solo di uno il risultato e' infatti comunque corretto
   // Altrimenti siamo comunque fuori con l' accuso
   if (StartStop) {
      Elapsed     =  Time.ulLo;
   } else {
      Elapsed = Time.ulLo - Elapsed; // Se il DELTA time supera ~ 4000 secondi NON va (Viene calcolato modulo 2**32)
      if(Elapsed < 4000000){ // Controllo overflow
         Elapsed = Elapsed * 1000 / Freq;
      } else {
         Elapsed = (Elapsed / Freq) * 1000;
      }
      if(Elapsed > Delay){
         Elapsed -= Delay ;
      } else {
         Elapsed = 0;
      }
   } /* endif */
   return Elapsed;
//<<< DWORD Cronometra BOOL StartStop,DWORD & Elapsed  
};

//----------------------------------------------------------------------------
// PROFILER static data
//----------------------------------------------------------------------------
PROFILER * PROFILER::Buffers;
int        PROFILER::NumProfilers;
DWORD      PROFILER::Freq;
ULONG      PROFILER::Delay;
ULONG      PROFILER::NumCall;
ULONG      PROFILER::TotCall;
ULONG      PROFILER::DelayPer1000Call;
ELENCO_S   PROFILER::Descrizioni     ;   // Descrizioni (per il motore: metodo standard AttivaDescrizioniMotore)
// Funzioncina per correzione frequenze
inline DWORD Elaps(DWORD Elapsed, DWORD Freq){
   if(Elapsed < 4000000){ // Controllo overflow
      Elapsed = Elapsed * 1000 / Freq;
   } else {
      Elapsed = (Elapsed / Freq) * 1000;
   }
}

//----------------------------------------------------------------------------
// PROFILER::Cronometra
//----------------------------------------------------------------------------
void PROFILER::Cronometra(WORD Idx,BOOL StartStop){ // Totalizza i tempi (Pur con tutte le limitazioni)
   #undef TRCRTN
   #define TRCRTN "PROFILER::Cronometra"
   QWORD Time;
   DosTmrQueryTime(&Time);
   PROFILER & Prf = Buffers[Idx];
   if (StartStop) {
      Prf.PrimoTempo  =  Time.ulLo;
   } else {
      ULONG Elapsed = Time.ulLo - Prf.PrimoTempo; // Puo' avere qualche problema in casi particolari
      if(Elapsed > Delay)Prf.TotaleParziale += Elapsed - Delay;
      Prf.PrimoTempo  =  Time.ulLo;  // Per successivi accumuli
      Prf.CallCount ++;
   } /* endif */
};

//----------------------------------------------------------------------------
// PROFILER::AttivaDescrizioniMotore
//----------------------------------------------------------------------------
// Attiva le descrizioni standard del motore
void  PROFILER::AttivaDescrizioniMotore()  {
   #undef TRCRTN
   #define TRCRTN "PROFILER::AttivaDescrizioniMotore"
   
   Descrizioni +=  "Memoria Totale Immobilizzata dal Motore       ";  //  0
   Descrizioni +=  "Memoria Immobilizzata dal Grafo               ";  //  1
   Descrizioni +=  "Memoria Immobilizzata dal Risolutore Orario   ";  //  2
   // Inner Loops
   Descrizioni +=  "Count chiamate STAZ_FS::PrimiVicini           ";  //  3
   Descrizioni +=  "Count chiamate identifica ramo                ";  //  4
   Descrizioni +=  "Count inner loop PosDaId o DistDaId           ";  //  5
   // Inner Loops Fine
   // Questi tempi sono significativi solo in mono thread, ancuni non sono calcolati in multi thread
   Descrizioni +=  "Tempo complessivo di generazione PATHS        ";  //  6
   Descrizioni +=  "Tempo per Valorizzazione soluzioni         (@)";  //  7
   Descrizioni +=  "Tempo complessivo di run Motore            ($)";  //  8
   Descrizioni +=  "Tempo Consolidamento (Collega soluz. al grafo)";  //  9
   Descrizioni +=  "Tempo attivita' iniziali e finali          (*)";  // 10
   Descrizioni +=  "Tempo di esecuzione misurato su PROVA6        ";  // 11
   Descrizioni +=  "Tempo complessivo di Combina                 $";  // 12
   Descrizioni +=  "Tempo complessivo di Semplifica (non finale) $";  // 13
   Descrizioni +=  "Tempo di creazione e distruzione IPOTESI     $";  // 14
   Descrizioni +=  "Tempo per ottenere i dati (I/O + Elab) (%)   $";  // 15
   Descrizioni +=  "Tempo per Risolvi Grafo (cicli no rami dir.) @";  // 16
   #ifdef ENABLE_INNER
   Descrizioni +=  "Tempo complessivo di I/O clusters            %";  // 17
   #else
   Descrizioni +=  "Numero complessivo GetCluster                 ";  // 17
   #endif
   Descrizioni +=  "Tempo complessivo di I/O FISICO Clusters     %";  // 18
   // Questi anche in multi thread
   Descrizioni +=  "Tempo di caricamento orario                  *";  // 19
   // ....
   // Inner Loops
   Descrizioni +=  "Count inner loop di tutte le macro SCAN       ";  // 20
   Descrizioni +=  "Count inner loop Tariffe regionali            ";  // 21
   Descrizioni +=  "Count inner loop identifica nodi vincolanti   ";  // 22
   Descrizioni +=  "Count inner loop Valorizza                    ";  // 23
   Descrizioni +=  "Count inner loop di Risolvi                   ";  // 24
   Descrizioni +=  "Count inner loop di RisolviPolimetriche       ";  // 25
   Descrizioni +=  "Count confronti di sort_function_NodoAlfa     ";  // 26
   Descrizioni +=  "Count inner loop di GeneraDelta               ";  // 27
   Descrizioni +=  "Count inner loop di Estendi                   ";  // 28
   Descrizioni +=  "Count inner loop di PuntiDiDinstradamento     ";  // 29
   Descrizioni +=  "Count inner loop di CompletaPercorso          ";  // 30
   // Questi sono dati per test estemporanei
   Descrizioni +=  "Numero di soluzioni generate da Ipotesi       ";  // 31
   Descrizioni +=  "Numero di soluzioni alternative               ";  // 32
   Descrizioni +=  "Numero di PATHs Generati (totali)             ";  // 33
   Descrizioni +=  "Numero di PATHs Generati(che accedono ai dati)";  // 34
   Descrizioni +=  "Numero di PATHs Generati(che tentano la Soluz)";  // 35
   Descrizioni +=  "Numero di PATHs che tentano e non trovano sol.";  // 36

//<<< void  PROFILER::AttivaDescrizioniMotore     
};
//----------------------------------------------------------------------------
// PROFILER::Clear
//----------------------------------------------------------------------------
void PROFILER::Clear(BOOL Parziale)        { // Azzera i tempi totali o parziali
   #undef TRCRTN
   #define TRCRTN "PROFILER::Clear"
   if (Buffers == NULL) {
      if(Descrizioni.Dim() ==0) AttivaDescrizioniMotore(); // Di default sono quelle del motore
      
      NumProfilers = Descrizioni.Dim();
      Buffers = new PROFILER[NumProfilers];
      for (int i = 0; i <  NumProfilers; i++ ) {
         Buffers[i].Descrizione = Descrizioni[i];
      } /* endfor */
      
      // Aggiusto il delay
      DosTmrQueryFreq(&Freq);
      Freq /= 1000;
      i = 10;
      do {
         Delay = 0;
         for (int j = 0;j < 30 ;j++ ) {
            Cronometra(Do0(),TRUE);
            Cronometra(Do0(),FALSE);
            DosSleep(10); // Se no i tempi non vengono precisi
         } /* endfor */
         Delay = Buffers[0].TotaleParziale / 30;
         Buffers[0].TotaleParziale = 0;
         Buffers[0].CallCount      = 0;
      } while (i-- > 0 && Delay > 500  ); // Fino a 10 retry (in caso di task switch)
      TRACEVLONG(Delay);
      QWORD Time1,Time2;
      
      int Sum = Do0();
      DosTmrQueryTime(&Time1);
      for (int K = 0 ; K < 100000 ; K++ ) {
         Sum += K + Do0();
      } /* endfor */
      DosTmrQueryTime(&Time2);
      ULONG SoloLoop = (Time2.ulLo - Time1.ulLo);
      //TRACEVLONG(SoloLoop);
      
      DosTmrQueryTime(&Time1);
      for (K = 0 ; K < 100000 ; K++ ) {
         Sum += K;
         PROFILER::Conta(Do0());
      } /* endfor */
      DosTmrQueryTime(&Time2);
      Buffers[0].TotaleParziale = 0;
      NumCall = 0;
      ULONG ConConta = (Time2.ulLo - Time1.ulLo) ;
      //TRACEVLONG(ConConta);
      DelayPer1000Call= (ConConta - SoloLoop)/100;
      TRACELONG("Tempo stimato per 1000 operazioni in microsecondi:", DelayPer1000Call);
      
//<<< if  Buffers == NULL   
   } /* endif */
   if (Parziale) {
      for (int i = 0;i < NumProfilers ; i++) {
         Buffers[i].TotaleParziale = 0;
         Buffers[i].PrimoTempo     = 0;
         Buffers[i].CallCount      = 0;
      } /* endfor */
      NumCall = 0;
   } else {
      for (int i = 0;i < NumProfilers ; i++) {
         Buffers[i].TotaleTotale    = 0;
         Buffers[i].CallCountTotale = 0;
      } /* endfor */
      TotCall = 0;
   } /* endif */
   
//<<< void PROFILER::Clear BOOL Parziale           // Azzera i tempi totali o parziali
};
void PROFILER::Clear(WORD Idx , BOOL Parziale)        { // Azzera i tempi totali o parziali
   if (Parziale) {
      Buffers[Idx].TotaleParziale = 0;
      Buffers[Idx].PrimoTempo     = 0;
      Buffers[Idx].CallCount      = 0;
   } else {
      Buffers[Idx].TotaleTotale   = 0;
      Buffers[Idx].CallCountTotale= 0;
   } /* endif */
};
//----------------------------------------------------------------------------
// PROFILER::Acquire
//----------------------------------------------------------------------------
void PROFILER::Acquire(BOOL ClearP)        { // Somma i dati di parziale in Totale Opzionalmente azzera Parziale
   #undef TRCRTN
   #define TRCRTN "PROFILER::Acquire"
   for (int i = 0;i < NumProfilers ; i++) {
      Buffers[i].TotaleTotale    += Buffers[i].TotaleParziale;
      Buffers[i].CallCountTotale += Buffers[i].CallCount;
   } /* endfor */
   if(ClearP){
      for (int i = 0;i < NumProfilers ; i++) {
         Buffers[i].TotaleParziale = 0;
         Buffers[i].CallCount      = 0;
      } /* endfor */
   }
   TotCall += NumCall;
   NumCall = 0;
};

//----------------------------------------------------------------------------
// PROFILER::GetTotale
//----------------------------------------------------------------------------
ELENCO_S PROFILER::GetTotale(BOOL Parziale){ // Ottengo i dati (Come elenco di stringhe)
   #undef TRCRTN
   #define TRCRTN "PROFILER::GetTotale"
   ELENCO_S Out;
   for (int i = 0;i < NumProfilers ; i++) {
      Out += GetTotale(i,Parziale);
   } /* endfor */
   return Out;
};
STRINGA PROFILER::GetTotale(WORD Idx,BOOL Parziale){ // Ottengo i dati (Come elenco di stringhe)
   STRINGA Out;
   if (Parziale) {
      if(Buffers[Idx].TotaleParziale== 0){
         Out = "#";
      } else {
         char Buf[500];
         if( Buffers[Idx].CallCount > 1){
            DWORD Tm1 = Elaps(Buffers[Idx].TotaleParziale,Freq);
            DWORD Med = Tm1/Buffers[Idx].CallCount;
            sprintf(Buf,"%s : %7i.%-3i Su %5i Chiamate Tempo Medio = %3i.%-3i", Buffers[Idx].Descrizione, Tm1/1000, Tm1%1000, Buffers[Idx].CallCount, Med/1000, Med%1000);
            Out = Buf;
         } else if( Buffers[Idx].CallCount == 1){
            DWORD Tm1 = Elaps(Buffers[Idx].TotaleParziale,Freq);
            sprintf(Buf,"%s : %7i.%-3i Su %5i Chiamata", Buffers[Idx].Descrizione, Tm1/1000, Tm1%1000, 1);
            Out = Buf;
         } else {
            sprintf(Buf,"%s : %7i ", Buffers[Idx].Descrizione,Buffers[Idx].TotaleParziale);
            Out = Buf;
         }
      }
   } else {
      if(Buffers[Idx].TotaleTotale  == 0){
         Out = "#";
      } else {
         char Buf[500];
         if( Buffers[Idx].CallCountTotale > 1){
            DWORD Tm1 = Elaps(Buffers[Idx].TotaleTotale,Freq);
            DWORD Med = Tm1/Buffers[Idx].CallCountTotale;
            sprintf(Buf,"%s : %7i.%-3i Su %5i Chiamate Tempo Medio = %3i.%-3i", Buffers[Idx].Descrizione, Tm1/1000, Tm1%1000, Buffers[Idx].CallCountTotale, Med/1000, Med%1000);
            Out = Buf;
         } else if( Buffers[Idx].CallCountTotale == 1){
            DWORD Tm1 = Elaps(Buffers[Idx].TotaleTotale,Freq);
            sprintf(Buf,"%s : %7i.%-3i Su %5i Chiamata", Buffers[Idx].Descrizione, Tm1/1000, Tm1%1000, 1);
            Out = Buf;
         } else {
            sprintf(Buf,"%s : %7i ", Buffers[Idx].Descrizione,Buffers[Idx].TotaleTotale);
            Out = Buf;
         }
      }
//<<< if  Parziale   
   } /* endif */
   return Out;
//<<< STRINGA PROFILER::GetTotale WORD Idx,BOOL Parziale   // Ottengo i dati  Come elenco di stringhe 
};
//----------------------------------------------------------------------------
// PROFILER::Trace
//----------------------------------------------------------------------------
void  PROFILER::Trace(BOOL Parziale,int Livello) { // Scrive i dati sul trace
   #undef TRCRTN
   #define TRCRTN "PROFILER::Trace"
   if (Livello > trchse)return;
   ULONG CallC=0;
   ERRSTRING("===================================================");
   if (Parziale) {
      for (int i = 0;i < NumProfilers ; i++)CallC += Buffers[i].CallCount;
      ULONG Tme = ((CallC * Delay) + (NumCall/1000*DelayPer1000Call)) / Freq;
      ERRSTRING("Dati parziali (Tempo utilizzato per la raccolta dati: "+STRINGA(Tme)+" msec)");
   } else {
      for (int i = 0;i < NumProfilers ; i++)CallC += Buffers[i].CallCountTotale;
      ULONG Tme = ((CallC * Delay) + (NumCall/1000*DelayPer1000Call)) / Freq;
      ERRSTRING("Totale accumulato (Tempo utilizzato per la raccolta dati: "+STRINGA(Tme)+" msec)");
   } /* endif */
   ERRSTRING("===================================================");
   ELENCO_S Tmp = GetTotale(Parziale);
   ORD_FORALL(Tmp,i)if(Tmp[i] != STRINGA("#"))ERRSTRING(Tmp[i]);
   ERRSTRING("===================================================");
};
void  PROFILER::Trace(WORD Idx, BOOL Parziale,int Livello) { // Scrive i dati sul trace
   if (Livello > trchse)return;
   ERRSTRING(GetTotale(Idx,Parziale));
};
//----------------------------------------------------------------------------
// PROFILER::PutStdOut
//----------------------------------------------------------------------------
void  PROFILER::PutStdOut(BOOL Parziale)   { // Scrive i dati sullo standard output
   #undef TRCRTN
   #define TRCRTN "PROFILER::PutStdOut"
   ULONG CallC=0;
   puts("===================================================");
   if (Parziale) {
      for (int i = 0;i < NumProfilers ; i++)CallC += Buffers[i].CallCount;
      ULONG Tme = ((CallC * Delay) + (NumCall/1000*DelayPer1000Call)) / 1000;
      printf("Dati parziali (Tempo utilizzato per la raccolta dati: %i msec)\n",Tme);
   } else {
      for (int i = 0;i < NumProfilers ; i++)CallC += Buffers[i].CallCountTotale;
      ULONG Tme = ((CallC * Delay) + (NumCall/1000*DelayPer1000Call)) / 1000;
      printf("Totale accumulato (Tempo utilizzato per la raccolta dati: %i msec)\n",Tme);
   } /* endif */
   puts("===================================================");
   ELENCO_S Tmp = GetTotale(Parziale);
   ORD_FORALL(Tmp,i)if(Tmp[i] != STRINGA("#"))puts((CPSZ)Tmp[i]);
   puts("===================================================");
   fflush(stdout);
};


//----------------------------------------------------------------------------
// ARRAY_ID::Trace
//----------------------------------------------------------------------------
void ARRAY_ID::Trace(struct GRAFO& Grafo,const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::Trace"
   if(Livello > trchse)return;
   PrtText("",(CPSZ)Messaggio);
   ORD_FORALL(THIS,i){
      STRINGA Msg;
      ID Id = THIS[i];
      Msg = "Id["+ STRINGA(i)+ "] ";
      Msg += " =";
      Grafo.TraceId(Msg,Id,Livello);
   };
   PrtText("","");
};
//----------------------------------------------------------------------------
// ChkSegnala
//----------------------------------------------------------------------------
// Questo e' un bound check error per le routines di caricamento: NON e' disabilitato al run time
void ChkSegnala(UINT i, UINT n,int Linea){
   printf("Ecceduto limite strutturale, assegnato valore %i a campo di %i BITS (Max Valido %i) ,  Linea %i\n",i,n,(1 << n)-1, Linea);
   BEEP;
   exit(999);
}

//----------------------------------------------------------------------------
// Maschere servizi
//----------------------------------------------------------------------------
DWORD MaskPrenotazione; // Servizi di prenotazione
DWORD MaskTrasporto   ; // Servizi di trasporto
DWORD MaskGenerici    ; // Servizi generici
DWORD MaskTariffe     ; // Servizi che si traducono in tariffe
DWORD MaskAnd         ; // BIT che vanno combinati in AND
DWORD MaskServizi     ; // Tutti i servizi
DWORD MaskLettiCucc   ; // Letti e cuccette
DWORD MaskUniformita  ;
DWORD MaskDueStaz     ; // Sono i servizi attivi solo se lo sono ad inizio e fine tratta contemporaneamente
class MASK_INITIALIZER {MASK_INITIALIZER(); static MASK_INITIALIZER Minit1;};
MASK_INITIALIZER MASK_INITIALIZER::Minit1; // Istanza statica: Solo per chiamare il costruttore

//----------------------------------------------------------------------------
// @MASK_INITIALIZER
//----------------------------------------------------------------------------
MASK_INITIALIZER::MASK_INITIALIZER(){
   #undef TRCRTN
   #define TRCRTN "@MASK_INITIALIZER"
   MM_INFO Tmp; Tmp.Clear();
   Tmp.Prenotabilita    =  Tmp.PrenObbligItalia   =  
   Tmp.PrenObbligEstero =  Tmp.PrenotabileSolo1Cl =1;
   MaskPrenotazione = *(DWORD*)&Tmp;
   Tmp.Clear();
   Tmp.ServizioBase        = Tmp.LimitiDiAmmissione  =1;
   Tmp.PostiASederePrima   = Tmp.PostiASedereSeconda =1;
   Tmp.CuccettePrima       = Tmp.CuccetteSeconda     =1;
   Tmp.VagoniLettoPrima    = Tmp.VagoniLettoSeconda  =1;
   Tmp.AutoAlSeguito       = Tmp.Invalidi            =1;
   Tmp.Biciclette          = Tmp.Animali             =1;
   MaskTrasporto = *(DWORD*)&Tmp;
   Tmp.Clear();
   Tmp.Ristoro= Tmp.RistoBar= Tmp.SelfService= Tmp.Ristorante = Tmp.Fumatori =1;
   MaskGenerici  = *(DWORD*)&Tmp;
   Tmp.Clear();
   Tmp.PostiASederePrima   = Tmp.PostiASedereSeconda =1;
   Tmp.CuccettePrima       = Tmp.CuccetteSeconda     =1;
   MaskLettiCucc = *(DWORD*)&Tmp;
   Tmp.Clear();
   Tmp.Supplemento   = Tmp.TrenoVerde = 1;
   MaskTariffe = *(DWORD*)&Tmp;
   *(DWORD*)&Tmp = 0xffffffff; // Set all 1
   Tmp.AutoAlSeguito  = Tmp.Invalidi = Tmp.Biciclette = Tmp.Animali = 0;
   MaskAnd =  *(DWORD*)&Tmp;
   Tmp.Clear();
   Tmp.AutoAlSeguito  = Tmp.Invalidi = 1;
   Tmp.PostiASederePrima   = Tmp.PostiASedereSeconda =1;
   MaskDueStaz  = *(DWORD*)&Tmp;
   *(DWORD*)&Tmp = 0xffffffff; // Set all 1
   Tmp.HaNoteGeneriche        = Tmp.HaNoteDiVendita     = 0;
   Tmp.PrenotDisUniforme      = Tmp.ServTraspDisUniformi= 0;
   Tmp.ServGenerDisUniformi   = Tmp.Filler              = 0;
   Tmp.ClassificaDisUniforme  = Tmp.Disuniforme         = 0;
   MaskServizi  =  *(DWORD*)&Tmp;
   Tmp.Clear();
   Tmp.ServTraspDisUniformi  = Tmp.ServGenerDisUniformi = 1;
   Tmp.ClassificaDisUniforme = Tmp.PrenotDisUniforme    = 1;
   MaskUniformita =  *(DWORD*)&Tmp;
   assert(MaskServizi == (MaskPrenotazione | MaskTrasporto | MaskGenerici | MaskTariffe)) ;
//<<< MASK_INITIALIZER::MASK_INITIALIZER   
};
//----------------------------------------------------------------------------
// Descrizioni dei servizi
//----------------------------------------------------------------------------
#undef TRCRTN
#define TRCRTN "Descrizioni dei servizi"
static PCHAR NomiCorti[] = {
   "Pr"         ,
   "PrObIt"     ,
   "PrObEs"     ,
   "Pr1ø"       ,
   "S.B."       ,
   "LmAms"      ,
   "1ø"         ,
   "2ø"         ,
   "Cuc1ø"      ,
   "Cuc2ø"      ,
   "WL1ø"       ,
   "WL2ø"       ,
   "AUto"       ,
   "Inv"        ,
   "Bic"        ,
   "Anm"        ,
   "Ri.ro"      ,
   "Ri.Bar"     ,
   "Ss"         ,
   "Ri.te"      ,
   "Fum"        ,
   "SUP"        ,
   "VERDE"      ,
   (PCHAR)NULL  ,
   (PCHAR)NULL  ,
   "NoGe"       ,
   "NoVe"       ,
   "PrDis"      ,
   "TraDis"     ,
   "GenDis"     ,
   "ClaDis"     ,
   "Dis"        
//<<< static PCHAR NomiCorti   =  
};
static PCHAR NomiLunghi[] = {
   "Prenotabile"          ,
   "Prenotazione obbligatoria in Italia",
   "Prenotazione obbligatoria all' estero",
   "Prenotabile solo in 1ø classe",
   "Servizio Base (Posto a sedere / Cuccetta)" ,
   "Limiti di Ammissione",
   "Posti a sedere di prima classe",
   "Posti a sedere di seconda classe",
   "Cuccette di prima classe",
   "Cuccette di seconda classe",
   "Vagoni letto di prima classe",
   "Vagoni letto di seconda classe",
   "Vagone auto al seguito",
   "Trasporto invalidi"    ,
   "Trasporto biciclette"  ,
   "Trasporto animali"     ,
   "Ristoro"               ,
   "Servizio RistoBar"     ,
   "Carrozza Self Service" ,
   "Servizio ristorante"   ,
   "Carrozze fumatori"     ,
   "Pagamento di supplemento",
   "Treno VERDE"           ,
   (PCHAR)NULL             ,
   (PCHAR)NULL             ,
   "Ha note generiche"     ,
   "Ha note di vendita"    ,
   "Prenotabilit… disuniforme" ,
   "Servizi di trasporto disuniformi",
   "Servizi generici disuniformi",
   "Classifica disuniforme",
   "Ha delle disuniformit…"
//<<< static PCHAR NomiLunghi   =  
};
//----------------------------------------------------------------------------
// MM_INFO::DecServizio
//----------------------------------------------------------------------------
const char * MM_INFO::DecServizio(BYTE Num, BOOL Lunga) {
   #undef TRCRTN
   #define TRCRTN "MM_INFO::DecServizio"
   if (Num >= 32)return NULL;
   if (Lunga) {
      return NomiLunghi[Num];
   } else {
      return NomiCorti[Num];
   } /* endif */
};

//----------------------------------------------------------------------------
// MM_INFO::IsServizio
//----------------------------------------------------------------------------
// Indica se esiste il servizio Num
BOOL MM_INFO::IsServizio(BYTE Num) {
   #undef TRCRTN
   #define TRCRTN "MM_INFO::IsServizio"
   if (Num >= 32)return FALSE;
   DWORD Msk = 1 << Num;
   return Msk & MaskServizi;
};

//----------------------------------------------------------------------------
// MM_INFO::TestServizio
//----------------------------------------------------------------------------
// Indica se ho il servizio Num; Torna FALSE se non e' definito il servizio corrispondente al numero
BOOL MM_INFO::TestServizio(BYTE Num) const {
   #undef TRCRTN
   #define TRCRTN "MM_INFO::TestServizio"
   if (Num >= 32)return FALSE;
   DWORD Msk = 1 << Num;
   return Msk & MaskServizi & (*(DWORD*)this) ;
};

//----------------------------------------------------------------------------
// MM_INFO::SetServizio
//----------------------------------------------------------------------------
// Set del servizio Num; Non fa nulla se non e' definito il servizio corrispondente al numero
void MM_INFO::SetServizio(BYTE Num) {
   #undef TRCRTN
   #define TRCRTN "MM_INFO::SetServizio"
   if (Num >= 32)return;
   DWORD Msk = 1 << Num;
   (*(DWORD*)this) |= (Msk & MaskServizi);
};
//----------------------------------------------------------------------------
// MM_INFO::ReSetServizio
//----------------------------------------------------------------------------
void MM_INFO::ReSetServizio(BYTE Num) {
   #undef TRCRTN
   #define TRCRTN "MM_INFO::ReSetServizio"
   // ReSet del servizio Num; Non fa nulla se non e' definito il servizio corrispondente al numero
   if (Num >= 32)return;
   DWORD Msk = 1 << Num;
   (*(DWORD*)this) &= ~(Msk & MaskServizi);
};
//----------------------------------------------------------------------------
// MM_INFO::EncodTipoMezzo
//----------------------------------------------------------------------------
BYTE MM_INFO::EncodTipoMezzo(const STRINGA & Categ){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::EncodTipoMezzo"

   if (CLASSIFICA_TRENI::Classifiche == NULL){
      ERRSTRING("Non inizializzate le classifiche");
      BEEP;
      return 0;
   };
   STRINGA Categoria(Categ);
   Categoria.Strip();
   for (int i = 0;i <=  COLLEGAMENTO_URBANO  ; i++) {
      if (Categoria == CLASSIFICA_TRENI::Classifiche[i].Descrizione ) return i;
   } /* endfor */
   return 0xff;
};

//----------------------------------------------------------------------------
// MM_INFO::DecodTipoMezzo
//----------------------------------------------------------------------------
STRINGA  MM_INFO::DecodTipoMezzo(BYTE i){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::DecodTipoMezzo"
   if ( i <= COLLEGAMENTO_URBANO) {
      return CLASSIFICA_TRENI::Classifiche[i].Descrizione ;
   } else {
      return NUSTR;
   } /* endif */
};

//----------------------------------------------------------------------------
// MM_INFO::Decodifica
//----------------------------------------------------------------------------
STRINGA MM_INFO::Decodifica(BOOL AncheClassifica){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::Decodifica"
   STRINGA Out;
   if(AncheClassifica)Out += DecodTipoMezzo(TipoMezzo);
   BOOL HaNote = HaNoteGeneriche || HaNoteDiVendita;
   if(HaNote) Out += " Ha Note";
   if(Disuniforme){
      Out += "(";
      if(PrenotDisUniforme       ) Out += " Pren.";
      if(ServTraspDisUniformi    ) Out += " Trasp.";
      if(ServGenerDisUniformi    ) Out += " Gener.";
      if(ClassificaDisUniforme   ) Out += " Class.";
      Out += " Disun.)";
   }
   for (int i = 0; i < 32 ; i++ ) {
      if(TestServizio(i)){
         Out += ' ';
         Out += NomiCorti[i];
      }
   } /* endfor */
   return Out;
//<<< STRINGA MM_INFO::Decodifica BOOL AncheClassifica  
}
//----------------------------------------------------------------------------
// MM_INFO::DecodificaServizi
//----------------------------------------------------------------------------
ELENCO_S MM_INFO::DecodificaServizi(){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::DecodificaServizi"
   
   ELENCO_S Out;
   for (int i = 0; i < 32 ; i++ ) {
      if( 1 << i & (*(DWORD*)this)) Out += NomiLunghi[i];
   }
   return Out;
}

//----------------------------------------------------------------------------
// MM_INFO::CombinaTratta
//----------------------------------------------------------------------------
// Questo metodo dati i servizi di due tratte li combina in un insieme di servizi "combinati"
void MM_INFO::CombinaTratta(const MM_INFO & From){ // Combina le due tratte
   #undef TRCRTN
   #define TRCRTN "MM_INFO::CombinaTratta"
   
   DWORD A,B;
   A = *(DWORD*)&From;
   B = *(DWORD*)this;
   MM_INFO Out = THIS;
   DWORD & DOut = *(DWORD*)&Out;
   // I bits vanno combinati in OR
   DOut |= A;
   // Alcuni bits vanno combinati in AND: adesso correggo
   DOut &= (A & B) | MaskAnd  ;
   // Ed infine controllo l' uniformita' di alcuni flags
   if((A & MaskPrenotazione) ^ (B & MaskPrenotazione))Out.PrenotDisUniforme    = 1;
   if((A & MaskTrasporto   ) ^ (B & MaskTrasporto   ))Out.ServTraspDisUniformi = 1;
   if((A & MaskGenerici    ) ^ (B & MaskGenerici    ))Out.ServGenerDisUniformi = 1;
   if(TipoMezzo != From.TipoMezzo){
      Out.TipoMezzo = max(TipoMezzo,From.TipoMezzo);
      Out.ClassificaDisUniforme = 1;
   } else {
      Out.TipoMezzo = TipoMezzo;
   }
   Out.Disuniforme = (DOut & MaskUniformita ) == 0 ? 0 : 1 ;
   THIS = Out;
//<<< void MM_INFO::CombinaTratta const MM_INFO & From   // Combina le due tratte
};
//----------------------------------------------------------------------------
// MM_INFO::operator|=
//----------------------------------------------------------------------------
void MM_INFO::operator|= (const MM_INFO & From){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::operator|="
   DWORD &A = *(DWORD*)&From;
   DWORD &B = *(DWORD*)this ;
   B |= A;
};

//----------------------------------------------------------------------------
// MM_INFO::operator&=
//----------------------------------------------------------------------------
void MM_INFO::operator&= (const MM_INFO & From){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::operator&="
   DWORD &A = *(DWORD*)&From;
   DWORD &B = *(DWORD*)this ;
   B &= A;
};

//----------------------------------------------------------------------------
// MM_INFO::NotIn
//----------------------------------------------------------------------------
// Lascio solo i servizi NON presenti in From.
// Posso resettare oppure lasciati inalterati i BIT non corrispondenti a servizi
void MM_INFO::NotIn(const MM_INFO & From ,BOOL AzzeraNonSrv){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::NotIn"
   DWORD A = *(DWORD*)&From;
   DWORD &B = *(DWORD*)this ;
   A = ~A;
   if (AzzeraNonSrv) {
      // Pongo a 0 tutto cio' che non e' un servizio
      A &= MaskServizi;
   } else {
      // Pongo a 1 tutto cio' che non e' un servizio
      A |= ~MaskServizi;
   } /* endif */
   B &= A;
};

//----------------------------------------------------------------------------
// MM_INFO::Empty
//----------------------------------------------------------------------------
BOOL MM_INFO::Empty(){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::Empty"
   DWORD &A = *(DWORD*)this;
   return A == 0;
};

//----------------------------------------------------------------------------
// MM_INFO::EmptyServizi
//----------------------------------------------------------------------------
BOOL MM_INFO::EmptyServizi(){
   #undef TRCRTN
   #define TRCRTN "MM_INFO::EmptyServizi"
   DWORD  A = *(DWORD*)this;
   A &= MaskServizi  ;
   return A == 0;
};
//----------------------------------------------------------------------------
// MM_INFO::DecodificaPerFontStandard
//----------------------------------------------------------------------------
// Questo metodo decodifica le informazioni in accordo a quanto necessario 
// secondo il font standard. L' output e' fornito su tre stringhe:
// - La prima ha un carattere che indica la classifica (2 se disuniforme)
// - La seconda ha un carattere per ogni servizio
// - La terza riporta le informazioni ausiliarie (sempre una per carattere)
//----------------------------------------------------------------------------
void MM_INFO::DecodificaPerFontStandard( STRINGA & Classif, STRINGA & Servizi, STRINGA & InfoAux) const {
   #undef TRCRTN
   #define TRCRTN "MM_INFO::DecodificaPerFontStandard"

   Classif.Clear();
   Servizi.Clear();
   InfoAux.Clear();

   if(ClassificaDisUniforme) Classif += CLDISUNIFORME;
   Classif += CLASSIFICA_TRENI::FontChar(Classifica().Classifica);

   if (PostiASederePrima ){
      if(PostiASedereSeconda ) Servizi += POSTI12A     ;
      else Servizi += POSTI1A     ;
   } else if (PostiASedereSeconda ) Servizi += POSTI2A ;

   if (VagoniLettoPrima   ){
      if (VagoniLettoSeconda ) Servizi += LETTI12A     ;
      else Servizi += LETTI1A     ;
   } else if (VagoniLettoSeconda ) Servizi += LETTI2A  ;

   if (PrenObbligItalia   ) Servizi += PRENOBBLIGITA;
   else if (Prenotabilita ) Servizi += PRENOTABILE;

   if (CuccetteSeconda    ) Servizi += CUCC2A      ;
   if (AutoAlSeguito      ) Servizi += AUTO        ;
   if (Invalidi           ) Servizi += INVALIDI    ;
   if (Biciclette         ) Servizi += BICI        ;
   if (Animali            ) Servizi += ANIMALI     ;
   if (Ristoro            ) Servizi += RISTORO     ;
   if (RistoBar           ) Servizi += RISTOBAR    ;
   if (SelfService        ) Servizi += SELFSERVICE ;
   if (Ristorante         ) Servizi += RISTORANTE  ;
   if (TrenoVerde         ) Servizi += TRENO_VERDE ;

   // if (Fumatori           ) Servizi += FUMATORI   ;
   // if (!Fumatori          ) Servizi += NOFUMATORI ;

   if (!ServizioBase ) InfoAux += NOSERVBASE;
   if (Supplemento && Classifica().SupplementoIC ) InfoAux += SUPPLEMENTO_IC;
   if (Supplemento && Classifica().SupplementoEC ) InfoAux += SUPPLEMENTO_IC;
   if (Supplemento && Classifica().SupplementoETR) InfoAux += SUPPLEMENTO_ES;
};

//----------------------------------------------------------------------------
// FontPeriodicita
//----------------------------------------------------------------------------
// Decodifica sencondo il font standard
STRINGA FontPeriodicita(TIPOPERIODICITA TipoPeriodicita){
   #undef TRCRTN
   #define TRCRTN "FontPeriodicita"
   STRINGA Pp ;
   switch (TipoPeriodicita) {
   case GIORNI_FERIALI           :
      Pp += PP_FERIALI;
      break;
   case GIORNI_FESTIVI           :
      Pp += PP_FESTIVI;
      break;
   case LAVORATIVI_ESCLUSO_SABATO:
      Pp += PP_LAVNOSAB;
      break;
   case ESCLUSO_SABATO           :
      Pp += PP_NOSAB;
      break;
   case SABATO_E_FESTIVI         :
      Pp += PP_SABFESTIVI ;
      break;
   case PERIODICITA_COMPLESSA    :
      Pp += PP_COMPLEX;
      break;
   } /* endswitch */
   return Pp;
}
//----------------------------------------------------------------------------
// GetTimestampDll
//----------------------------------------------------------------------------
F_DTM _export GetTimestampDll(const char * NomeDLL){
   #undef TRCRTN
   #define TRCRTN "GetTimestampDll"
   F_DTM  Tms;
   ZeroFill(Tms);
   int rc;
   char DllName[256];
   char Msg[256];
   HMODULE   Idll = 0;
   ULONG Len = 256;
   rc = DosLoadModule(Msg,sizeof(Msg),(char*)NomeDLL,&Idll);
   if(rc){
      ERRINT("Errore in DosLoadModule, non trovata "+STRINGA(NomeDLL)+".DLL , RC = ",rc);
      BEEP;
   } else {
      rc = DosQueryModuleName ( Idll, Len,DllName);
      if(rc){
         ERRINT("Errore in DosQueryModuleName su DLL "+STRINGA(NomeDLL)+".DLL, RC = ",rc);
         BEEP;
      } else {
         FILE_RO Dll(DllName);
         Tms = Dll.FileTms();
      }
      DosFreeModule(Idll);
   }
   return Tms;
//<<< F_DTM _export GetTimestampDll const char * NomeDLL  
};

//----------------------------------------------------------------------------
// Assert: attenzione anche a tracelevel 0
//----------------------------------------------------------------------------
// E' BOOL Per il modo in cui e' stata scritta la macro (assert(...) puo' essere messa in un if() )
BOOL _export SegnalaAssert(char * Rtn,const char* Asserzione, char * File, int Line){
   #undef TRCRTN
   #define TRCRTN "SegnalaAssert"
   
   #ifdef BEEP_A_TRC0
   #define Bp Bprintf
   #else
   #define Bp Bprintf2
   #endif
   
   static NUMASSERT=0;
   static MAXASSERT= getenv("MAXASSERT") == NULL ? 40 : atoi(getenv("MAXASSERT"));
   // Per non ingorgare i trace NON segnalo mai piu' di MAXASSERT assert
   if (NUMASSERT >  MAXASSERT )return TRUE;
   int TRCHSE = trchse;
   if(trchse == 0)trchse = 1;
   
   Bp("%s:²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²",Rtn);
   Bp("%s:²²²²²²²²²²²²²²²²    ASSERT FALLITA: ERRORE   ²²²²²²²²²²²²²²²²²",Rtn);
   Bp("%s:²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²²",Rtn);
   Bp("%s:Asserzione: \"%s\" File: %s Linea %i",Rtn,Asserzione,File,Line);
   if(&Orario){
      Bp( "%s:Relazione %i %s ==> %i %s",Rtn ,Orario.EIdOrigine() ,GRAFO::Gr()[Orario.EIdOrigine()].Nome7() ,Orario.EIdDestinazione() ,GRAFO::Gr()[Orario.EIdDestinazione()].Nome7() );
   }
   
   if (NUMASSERT ++ < 5) { DosBeep((ULONG)1200,(ULONG)600); }
   TraceCall(Rtn,"Call Stack di chiamata della Assert",6,1);
   trchse = TRCHSE;
   
   #undef Bp
   return TRUE;
//<<< BOOL _export SegnalaAssert char * Rtn,const char* Asserzione, char * File, int Line  
};
//----------------------------------------------------------------------------
// Funzioncina per leggere una serie di coefficienti da environment
//----------------------------------------------------------------------------
void GetEnvArray(int * Array, int NumEle, char * EnvName){
   #undef TRCRTN
   #define TRCRTN "GetEnvArray"
   if(getenv(EnvName)){
      STRINGA Tmp = getenv(EnvName);
      ELENCO_S Coeff = Tmp.Tokens(",;/");
      for (int i = 0; i < NumEle; i++ ) {
         Array[i] = Coeff[i].ToInt();
      } /* endfor */
   }
};

//----------------------------------------------------------------------------
// MM_INFO::LettiOCuccette
//----------------------------------------------------------------------------
BOOL MM_INFO::LettiOCuccette() const {
   #undef TRCRTN
   #define TRCRTN "MM_INFO::LettiOCuccette"
   return Bool( *(DWORD*)this  & MaskLettiCucc);
};

//----------------------------------------------------------------------------
// CLASSIFICA_TRENI::CaricaClassifiche
//----------------------------------------------------------------------------
void CLASSIFICA_TRENI::CaricaClassifiche(const STRINGA & PathDati){
   #undef TRCRTN
   #define TRCRTN "CLASSIFICA_TRENI::CaricaClassifiche"
   if (Classifiche) return;
   STRINGA PathDati2 = PathDati ;
   if(PathDati2.Last() != '\\' ) PathDati2 += '\\';
   FILE_FISSO<CLASSIFICA_TRENI> FClassifiche(PathDati2 + "MM_CLASF.DB");
   Classifiche = new CLASSIFICA_TRENI[COLLEGAMENTO_URBANO+1];
   memset(Classifiche,0, (COLLEGAMENTO_URBANO+1) * sizeof(CLASSIFICA_TRENI) );
   ORD_FORALL(FClassifiche,i){
      Classifiche[FClassifiche[i].Classifica] = FClassifiche[i] ;
   }
   strcpy(Classifiche[0].Descrizione,"NON NOTO");
   strcpy(Classifiche[COLLEGAMENTO_URBANO ].Descrizione,"Colleg. Urbano");
   Classifiche[COLLEGAMENTO_URBANO ].Classifica = COLLEGAMENTO_URBANO ;
};

//----------------------------------------------------------------------------
// CLASSIFICA_TRENI::LiberaClassifiche
//----------------------------------------------------------------------------
void CLASSIFICA_TRENI::LiberaClassifiche(){
   #undef TRCRTN
   #define TRCRTN "CLASSIFICA_TRENI::LiberaClassifiche"
   if (!Classifiche) return;
   delete [] Classifiche ;
   Classifiche = NULL;
};
