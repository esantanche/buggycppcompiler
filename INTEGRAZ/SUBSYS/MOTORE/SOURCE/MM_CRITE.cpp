//----------------------------------------------------------------------------
// MM_CRITE  : Contiene i criteri euristici utilizzati dal MOTORE
//----------------------------------------------------------------------------
// Tutti i criteri sono raggruppati in questo file.
//
// COLLEGAMENTO::CalcolaCosto
//    Calcola il costo di un collegamento (fase iniziale di risoluzione grafo)
//
// T_TRENO::CaricaEValidaTreno
//    Seleziona in base ad alcuni criteri i treni da utilizzare.
//    Carica i dati di t_treno
//
// ELENCO_SOLUZIONI::Semplifica
//    Permette di dichiarare non valide soluzioni in base a varii criteri
//
// DATI_ORARIO_FS::Semplifica
//    Questa routine permette una semplificazione finale basata
//    sull' insieme globale dei criteri. Chiamata a ricerca conclusa
//
// DATI_ORARIO_FS::CriterioDiArrestoSp();
//    Questa routine stabilisce quando e' il momento di considerare
//    conclusa la ricerca.
//
// CLUSTER_PERCORSO::AttesaValida()
//    Ritorna il tempo di attesa di una soluzione potenziale depurato della pausa notturna
//
// CLUSTER_PERCORSO::CaricaTreni(Ora)
//    Il metodo seleziona, tra i treni del primo cluster, un sottoinsieme
//    compatibile con i nodi di cambio ed altri vincoli.
//
// CLUSTER_PERCORSO::CaricaTreni()
//    Idem per i cluster successivi al primo
//
// SOLUZIONE::OkSost()
//    Controlla che siano verificate delle business rules di sostituibilita' da
//    parte di Semplifica()
//
// COLLEGAMENTO_DIRETTO::ControllaAttesaValida()
//    Determina quale sia il massimo limite di attesa accettabile, in funzione
//    del tipo di collegamento
//
//----------------------------------------------------------------------------
//0
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

#include "BASE.hpp"
#include "alfa0.hpp"
#include "MM_CRIT2.HPP"
#include "mm_path.hpp"
#include "mm_combi.hpp"

//----------------------------------------------------------------------------
// Debug opzionale
//----------------------------------------------------------------------------
//#define DBG1                // Mostra quante aggiunte / sostituzioni ho avuto su soluzioni
//#define DBG2                // Segue criterio di arresto
//#define DBG2A               // PROFILER Parziale quando passo in modo esteso
//#define DBG3                // Classificazione delle soluzioni
//#define DBG3A               // Dettagli classificazione soluzione per soluzione
//#define DBG4                // Dettagli caricamento e validazione treno per collegamenti diretti
//#define DBG24               // Compilazione opzionale : Segue caricamento selettivo dei treni
//#define DBG25               // Compilazione opzionale : trace sull' algoritmo di semplificazione
//#define DBG25A              // Compilazione opzionale : trace dettagli algoritmo di semplificazione
//#define DBG25B              // Compilazione opzionale : trace dettagli CalcolaCosto
//#define DBG28               // Compilazione opzionale : mostra soluzioni VALIDE
//#define DBGsempli           // Segue semplificazione finale
//#define DBGconfli           // Segue verifica conflitto
//#define PERCORSI_FISSI 2000 // Criterio di arresto di debug: si ferma quando sono stati generati x percorsi
//#define NO_EXTEND           // Non esegue la fase di ricerca estesa
// Vedere anche il DBG31 su ALFA2.CPP
//----------------------------------------------------------------------------
// Se definita MV_IN_ATTENZIONE limita i trace di DBG25, DBG25A, DBG25B alle
// sole soluzioni che contengono esclusivamente mezzi virtuali compresi tra
// quelli indicati nella corrispondente variabile di environment
// (segnala eventualmente se sostituiti da altre soluzioni ).
//#define MV_IN_ATTENZIONE
// Questa per debug particolari : LASCIA TUTTE LE SOLUZIONI (Tranne le troppo lunghe)
// #define DISABLE_SELEZIONE   // Inibisce Semplifica()
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// COLLEGAMENTO::CalcolaCosto
//----------------------------------------------------------------------------
void __fastcall COLLEGAMENTO::CalcolaCosto(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO::CalcolaCosto"

   // I collegamenti privilegiati hanno costo 0 (metto 1 per aiutare l' algoritmo)
   if(Privilegiato){
      Costo = 1;
   } else {
      Costo = Km + Ctratte + C_MS * PesoTratta + Ctreni * Penalita;
   }

};

//----------------------------------------------------------------------------
// T_TRENO::CaricaEValidaTreno
//----------------------------------------------------------------------------
BOOL __fastcall T_TRENO::CaricaEValidaTreno(CLU_BUFR & Buffer,T_CLUST & Clu ){ // Valida e carica i dati di T_TRENO
   #undef TRCRTN
   #define TRCRTN "T_TRENO::CaricaEValidaTreno"

   Valido = FALSE ;
   BOOL InAttenzione = DATI_ORARIO_FS::AutoDiagnostica;
   ID Da,A;
   MM_INFO InfoNU;

   if(InAttenzione ){
      InAttenzione = DATI_ORARIO_FS::Target.TrenoInAttenzione(Buffer.IdTrenoCorrente(Clu.PNodoIn));
      if(InAttenzione ){
         Da = Buffer.Nodi[Clu.PNodoIn ].IdNodo;
         A  = Buffer.Nodi[Clu.PNodoOut].IdNodo;
         WORD P1 = WORD_NON_VALIDA;
         WORD P2 = WORD_NON_VALIDA;
         if( Da == Orario.EIdOrigine()){
            P1 = 0;
         } else {
            P1 = DATI_ORARIO_FS::StazioniTarget.Posizione(Da);
            if(P1 != WORD_NON_VALIDA ) P1 ++;
         }
         if( A == Orario.EIdDestinazione()){
            P2 = DATI_ORARIO_FS::StazioniTarget.Dim() + 1;
         } else {
            P2 = DATI_ORARIO_FS::StazioniTarget.Posizione(A);
            if(P2 != WORD_NON_VALIDA ) P2 ++;
         }
         InAttenzione = P2 == P1 + 1;
         // InAttenzione = P2 != WORD_NON_VALIDA && P1 != WORD_NON_VALIDA; // Per debug della funzionalit… di trace selettivo
      };
//<<< if InAttenzione
   };

   MM_INFO  & Info  =  Buffer.Treno->InfoUniforme;
   INFOSTAZ & Fin   =  Buffer.Nodo(Clu.PNodoIn);  // Fermata di partenza
   INFOSTAZ & Fout  =  Buffer.Nodo(Clu.PNodoOut); // Fermata di arrivo

   if( ! (
         Fin.Partenza  &&
         Fout.Arrivo   &&
         Fin.ProgressivoStazione <  Fout.ProgressivoStazione
      )){
      if (InAttenzione) {
         int Prg = Buffer.ProgTreno();
         Bprintf3(" Scartato (per fermate o senso di circolazione) treno in attenzione %5.5s Cluster %i Da %i a %i" , Buffer.IdTrenoCorrente(Clu.PNodoIn), Buffer.Dat->IdCluster, Da,A );
         TRACEVINT(Fin.Partenza);
         TRACEVINT(Fout.Arrivo);
         TRACEVINT(Fin.ProgressivoStazione);
         TRACEVINT(Fout.ProgressivoStazione);
         TRACEVINT(Clu.PNodoIn );
         TRACEVINT(Clu.PNodoOut);
         TRACEVINT(Buffer.Dat->IdCluster);
         TRACEVINT(Da);
         TRACEVINT(A );
         Buffer.Dump(Da,A ,TRUE);
         Buffer.FindTreno(Prg);
         TRACESTRING(" ============= TRENO SCARTATO !! =======");
      } /* endif */
      return FALSE; // Non ferma oppure senso di percorrenza errato
//<<< if  !
   };

   RidottoTempoCoincP       =  Buffer.RidottoTempoCoinc(Clu.PNodoIn);
   // Non e' pi— utilizzato
   // RidottoTempoCoincA       =  Buffer.RidottoTempoCoinc(Clu.PNodoOut);

   LettiOCuccette   =  Info.LettiOCuccette(); // Dipenderebbe dalle stazioni di salita e discesa, ma faccio finta che sia uniforme
   OraPartenza      =  Fin.OraPartenza    ;
   OraArrivo        =  Fout.OraArrivo     ;
   Navale           =  Info.Navale();

   if (T_TRENO::PerDirettiPrenotabili) {
      ForzoCircolaSempre();
      Valido = TRUE;
      return TRUE;
   } else {
      // La circolazione e' riferita alla data di partenza, quindi la calcolo solo a partire da Fin;
      GiorniCircolazione = Buffer.Treno->Per().Circola4(Fin.GiornoSuccessivoPartenza);
   } /* endif */

   // Questa e' una la logica di selezione dei treni: Determina se alcuni treni vadano filtrati
   // NB: non filtro eliminando il treno (per biechi motivi di debug) ma inibendolo
   Valido  |= Info.PostiASedereSeconda;
   Valido  |= Info.PostiASederePrima  ;
   Valido  |= Navale                  ; // D• per scontato che i mezzi navali non cambino classifica
   // Do' per scontato che comunque il treno debba avere qualche servizio,
   // in particolare letti o cuccette
   // Se percorro almeno MM_CR.LimiteSoloLettiCucc Km pertanto accetto sempre il treno
   int Km = Buffer.PercorsiKm(Clu.PNodoIn, Clu.PNodoOut) ;
   Valido  |= Km >= MM_CR.LimiteSoloLettiCucc;

   // Questo flag mi dice che se il treno e' a prenotazione obbligatoria non debbo usarlo
   // Se e' impostato debbo ricalcolare i servizi usando la vera tratta in esame
   BOOL NoPrObb = !Navale && Buffer.Treno->KmLimiteTrattaCorta >= MM_CR.Lim2Prenobb && Km < MM_CR.LimitePrenobb;

   // Non e' corretto usare InfoUniforme se ho limitazioni.
   // Pertanto  cui prima di filtrare il treno accedo ai servizi .
   // Faccio riferimento alle tratte ignorando la periodicit…
   if(!Valido || NoPrObb ){
      InfoNU  =  Info;
      Buffer.DeterminaServizi(Clu.PNodoIn, Clu.PNodoOut, -1 , InfoNU);
      Valido  |= InfoNU.PostiASederePrima  ;
      Valido  |= InfoNU.PostiASedereSeconda;
      // Non testo navale perchŠ se son qui il mezzo non pu• essere navale
      // Non testo i Km perchŠ se son qui i Km sono <= MM_CR.LimiteSoloLettiCucc (sopra tale limite Š tutto valido)

      // Per i treni a prenotazione obbligatoria scarto le tratte che non
      // coprono almeno MM_CR.LimPrenobb Km
      if(NoPrObb && InfoNU.PrenObbligItalia ) Valido = FALSE;
      #ifdef DBG4
      if(!Valido)Bprintf3("Inibito treno, %5.5s Km %i Posti 1ø %i 2ø %i PrenObb %i",
         Buffer.IdTrenoCorrente(Clu.PNodoIn)
        ,Km
        ,InfoNU.PostiASederePrima
        ,InfoNU.PostiASedereSeconda
        ,InfoNU.PrenObbligItalia
      );
      #endif
   }

   if (InAttenzione) {
      if (!Valido){
         Bprintf3(" Inibito treno in attenzione %5.5s Cluster %i Da %i a %i Circolazione %x Km %i Posti 1ø %i 2ø %i PrenObb %i",
            Buffer.IdTrenoCorrente(Clu.PNodoIn), Buffer.Dat->IdCluster,
            Da, A, GiorniCircolazione, Km, InfoNU.PostiASederePrima,
            InfoNU.PostiASedereSeconda, InfoNU.PrenObbligItalia
         ) ;
      } else if( GiorniCircolazione == 0){
         Bprintf3(" Non circola nei giorni prescelti treno in attenzione %5.5s Cluster %i Da %i a %i Circolazione %x", Buffer.IdTrenoCorrente(Clu.PNodoIn), Buffer.Dat->IdCluster, Da,A,GiorniCircolazione );
      } else {
         Bprintf3(" Attivato treno in attenzione %5.5s Cluster %i Da %i a %i Circolazione %x", Buffer.IdTrenoCorrente(Clu.PNodoIn), Buffer.Dat->IdCluster, Da,A,GiorniCircolazione );
      }
   }

   // Si noti che NON filtro via per GiorniCircolazione == 0 in quanto il caso
   // e' non troppo frequente e non ritengo di averne vantaggio, rendendo invece pi—
   // difficile il DEBUG : Invece in seguito sorter• ed eliminer• tutti
   // i collegamenti non validi

   // Per velocizzare: Forzo i giorni circolazione a 0
   if(!Valido) {
      GiorniCircolazione = 0;
   };

   // Non devo mettere Valido = FALSE se GiorniCircolazione==0 : crea problemi di verifica SW
   // Tanto i test sono fatti tutti su GiorniCircolazione;

   return TRUE;
//<<< BOOL __fastcall T_TRENO::CaricaEValidaTreno CLU_BUFR & Buffer,T_CLUST & Clu    // Valida e carica i dati di T_TRENO
}

//----------------------------------------------------------------------------
// ELENCO_SOLUZIONI::Semplifica()
//----------------------------------------------------------------------------
// Permette di dichiarare non valide soluzioni in base a varii criteri
// Le soluzioni che rimangono valide sono aggiunte all' elenco delle soluzioni
// Le altre sono cancellate
// In pratica fa un controllo comparato tra la soluzione e tutte le
// soluzioni gia' disponibili, in modo da vedere se vi siano delle soluzioni
// assolutamente preferibili alla soluzione.
// Al momento della chiamata di Semplifica le soluzioni sono tariffate
// (anche con le polimetriche)
// Comunque controllo che siano verificate le condizioni stabilite da OkSost
// Sono gestiti correttamente i percorsi di piu' giorni
// Le soluzioni che superano il criterio vengono spostate sulle soluzioni
// della DATI_ORARIO_FS.
//----------------------------------------------------------------------------
BOOL ELENCO_SOLUZIONI::Semplifica(){
   #undef TRCRTN
   #define TRCRTN "ELENCO_SOLUZIONI::Semplifica()"

   #ifdef DBG1
   int NumAggiunte = 0, NumSostituite = 0;
   #endif


   for (int i = Dim()-1;i >=0 ;i -- ) { // Controllo tutte le soluzioni
      SOLUZIONE & Sol = *THIS[i];

      #ifdef DBG25A
      if(Sol.InAttenzione) Sol.Trace("Soluzione DA VALIDARE: ",2);
      #endif

      if (T_TRENO::PerDirettiPrenotabili && !Sol.InfoSoluzione.Prenotabilita) {
         #ifdef DBG25
         Sol.Trace("Eliminata soluzione NON prenotabile");
         #endif
         if( Sol.IsTargetAutoChk ) Sol.Trace(" Eliminata soluzione NON prenotabile",1);
         Sol.Valida = FALSE; // Troppo lunga
      } /* endif */

      // I percorsi marittimi o sul cumulativo possono portare a percorsi piu' corti
      // del percorso minimo sul grafo: pertanto confronto anche con questo
      WORD Lung2 = max(Sol.Km, Orario.DistanzaMinima);
      Lung2 = max(50, (Lung2 * MM_CR.LIM_DIST/100));
      if (Lung2 < Orario.DistanzaLimite) Orario.DistanzaLimite = Lung2;
      if (Sol.Km > Orario.DistanzaLimite && Sol.IdTraghetto == 0){
         #ifdef DBG25
         if(Sol.InAttenzione)Sol.Trace("Soluzione troppo lunga ("+STRINGA(Sol.Km)+" Km): NON VALIDA",2);
         #endif
         if( Sol.IsTargetAutoChk ) Sol.Trace(" Soluzione troppo lunga ("+STRINGA(Sol.Km)+" Km): NON VALIDA",1);
         Sol.Valida = FALSE; // Troppo lunga
      } /* endif */

      #ifndef DISABLE_SELEZIONE
      // ..................................
      // Controllo se debbo sostituire una soluzione di DATI_ORARIO_FS
      // ..................................
      if(Sol.Valida){

         int SolIniziale = Sol.OraPartenza();

         FORALL(Orario.Soluzioni,j){

            SOLUZIONE & Sol2 = *Orario.Soluzioni[j];
            int Sol2Iniziale = Sol2.OraPartenza();

            #ifdef DBG25A
            if(Sol.InAttenzione && Sol2.InAttenzione) Sol2.Trace("Alternativa in esame");
            #endif


            // Calcolo parametri utilizzati dai criteri di decisione
            int DeltaTime = (int)Sol.TempoTotaleDiPercorrenza - (int) Sol2.TempoTotaleDiPercorrenza;

            //-------------------------------------------------------------------------------------
            // Calcolo il tempo di sovrapposizione tra i due percorsi e, per analogia, anche un
            // tempo di sovrapposizione "virtuale" che tiene in conto l' elemento "Costo".
            // Per determinare se le soluzioni sono compatibili tengo in conto sia il tempo di
            // sovrapposizione virtuale che quello reale: il criterio e' che basta una delle due
            // condizioni per rendere le due soluzioni compatibili
            // Inoltre: Se la soluzione che parte DOPO parte dopo l' arrivo della
            // prima soluzione, debbo valutare i tempi di sovrapposizione facendo
            // riferimento alla prima soluzione spostata di un giorno.
            // In questo caso e' opportuno NON considerare se la prima soluzione
            // circoli o meno il giorno dopo (altrimenti si avrebbe una dipendenza
            // dall' ordine con cui vengono identificate le soluzioni).
            //-------------------------------------------------------------------------------------
            // Calcolo sovrapposizione: Supponendo che Sol parta prima di Sol2 (altrimenti caso simmetrico)
            //       =========================================            Sol
            //             -----------------------------------------      Sol2
            //
            //       |.....|                                   TempoTrascorso(SolIniziale,Sol2Iniziale)
            //
            //             |*********************************| Sol.CostoAssoluto - TempoTrascorso(SolIniziale,Sol2Iniziale)
            //                                                 Sol.TempoTotaleDiPercorrenza - TempoTrascorso(SolIniziale,Sol2Iniziale)
            // oppure
            //       =========================================            Sol
            //             ---------------------------                    Sol2
            //       |.....|*************************|         Sol2.CostoAssoluto
            //                                                 Sol2.TempoTotaleDiPercorrenza
            // Oppure
            //      ==========================
            //                                  -----------------------------
            //      |...........................|   Identifico perche' Sol.TempoTotaleDiPercorrenza - TempoTrascorso(SolIniziale,Sol2Iniziale) <= 0
            //
            //     In questo caso valuto il tempo di sovrapposizione al giorno dopo
            //
            //      ===                           ===          Sol
            //           -------------------------------       Sol2
            //           |........................| |..|
            //
            //
            //
            //-------------------------------------------------------------------------------------
            int  TempoDiSovrapposizioneVirtuale  = 0;
            int  TempoDiSovrapposizioneReale     = 0;
            WORD DeltaInizio;
            if (SolIniziale <= Sol2Iniziale) {
               DeltaInizio = TempoTrascorso(SolIniziale,Sol2Iniziale);
               if (Sol.TempoTotaleDiPercorrenza > DeltaInizio ) {
                  TempoDiSovrapposizioneReale    = min((Sol.TempoTotaleDiPercorrenza - DeltaInizio), Sol2.TempoTotaleDiPercorrenza);
                  TempoDiSovrapposizioneVirtuale = min((Sol.CostoAssoluto.MinutiEquivalenti - DeltaInizio), Sol2.CostoAssoluto.MinutiEquivalenti);
               } else {
                  // In questo caso valuto il tempo di sovrapposizione al giorno dopo
                  // E' sufficiente ricalcolare DeltaInizio e considerare Sol2 in
                  // partenza prima di Sol
                  DeltaInizio =  1440 - DeltaInizio;
                  TempoDiSovrapposizioneReale    = min((Sol2.TempoTotaleDiPercorrenza - DeltaInizio), Sol.TempoTotaleDiPercorrenza);
                  TempoDiSovrapposizioneVirtuale = min((Sol2.CostoAssoluto.MinutiEquivalenti - DeltaInizio), Sol.CostoAssoluto.MinutiEquivalenti);
               } /* endif */
            } else {
               DeltaInizio = TempoTrascorso(Sol2Iniziale,SolIniziale);
               if (Sol2.TempoTotaleDiPercorrenza > DeltaInizio ) {
                  TempoDiSovrapposizioneReale    = min((Sol2.TempoTotaleDiPercorrenza - DeltaInizio), Sol.TempoTotaleDiPercorrenza);
                  TempoDiSovrapposizioneVirtuale = min((Sol2.CostoAssoluto.MinutiEquivalenti - DeltaInizio), Sol.CostoAssoluto.MinutiEquivalenti);
               } else {
                  // In questo caso valuto il tempo di sovrapposizione al giorno dopo
                  // E' sufficiente ricalcolare DeltaInizio e considerare Sol2 in
                  // partenza prima di Sol
                  DeltaInizio =  1440 - DeltaInizio;
                  TempoDiSovrapposizioneReale    = min((Sol.TempoTotaleDiPercorrenza - DeltaInizio), Sol2.TempoTotaleDiPercorrenza);
                  TempoDiSovrapposizioneVirtuale = min((Sol.CostoAssoluto.MinutiEquivalenti - DeltaInizio), Sol2.CostoAssoluto.MinutiEquivalenti);
               } /* endif */
//<<<       if  SolIniziale <= Sol2Iniziale
            } /* endif */

            // -------------------------------------------------------------------------------------------------------
            // Determino se una delle due soluzioni sia preferibile all' altra
            // -------------------------------------------------------------------------------------------------------
            // Imposto tale informazione in "Situazione" con i seguenti codici:
            // 0 = Nessuna e' preferibile
            // 1 = Sol preferibile a Sol2
            // 2 = Sol2 preferibile a Sol
            // In questo punto la stima e' speculativa: debbo controllare la compatibilita'
            // -------------------------------------------------------------------------------------------------------
            int Situazione;
            // Questa e' una stima che e' effettivamente valida solo se le due soluzioni
            // sono compatibili. Ma per sapere se sono compatibili debbo fare prima la stima
            if( Sol.CostoAssoluto !=  Sol2.CostoAssoluto ){
               // Privilegio la soluzione a minimo costo assoluto
               Situazione = Sol.CostoAssoluto < Sol2.CostoAssoluto ? 1 : 2;
            } else if( Sol.Costo !=  Sol2.Costo ){
               // Privilegio la soluzione a minimo costo
               Situazione = Sol.Costo < Sol2.Costo ? 1 : 2;
            } else if( Sol.TempoTotaleDiAttesa !=  Sol2.TempoTotaleDiAttesa ){
               // Scelgo quella con MAGGIORE tempo di attesa
               // Perche' presumibilmente usa treni migliori
               Situazione = Sol.TempoTotaleDiAttesa > Sol2.TempoTotaleDiAttesa  ? 1 : 2;
            } else {
               // Scelgo quella gia' presente
               Situazione = 2;
            }

            // -------------------------------------------------------------------------------------------------------
            // Identificazione compatibilita' (= Se ha senso che una sostituisca l' altra)
            // -------------------------------------------------------------------------------------------------------
            // Le due soluzioni hanno orari "compatibili" se il tempo di sovrapposizione e'
            // >= Costo soluzione preferibile - il margine significativo
            int LimiteVirtuale,LimiteReale;
            if (Situazione == 1) {
               LimiteVirtuale = Sol.CostoAssoluto.MinutiEquivalenti - MM_CR.DeltaTimeSignificativo;
               LimiteReale    = Sol.TempoTotaleDiPercorrenza - MM_CR.DeltaTimeSignificativo;
            } else {
               LimiteVirtuale = Sol2.CostoAssoluto.MinutiEquivalenti - MM_CR.DeltaTimeSignificativo;
               LimiteReale    = Sol2.TempoTotaleDiPercorrenza - MM_CR.DeltaTimeSignificativo;
            } /* endif */
            BOOL  Compatibili = TempoDiSovrapposizioneVirtuale > 0 &&  TempoDiSovrapposizioneVirtuale >= LimiteVirtuale;
            Compatibili      |= TempoDiSovrapposizioneReale    > 0 &&  TempoDiSovrapposizioneReale    >= LimiteReale;
            // Euristic:
            // Due soluzioni che utilizzano lo stesso traghetto sono sempre compatibili
            // se ho piu' di due ore di differenza di tempo di percorrrenza
            Compatibili      |= Sol.IdTraghetto && (Sol.IdTraghetto == Sol2.IdTraghetto ) && DeltaTime > 120;
            // TRACESTRING( VRS(Compatibili) + VRS(Sol.IdTraghetto) + VRS(Sol2.IdTraghetto) );
            if(!Compatibili){
            };
            // -------------------------------------------------------------------------------------------------------
            // NB: L' identificare se le soluzioni siano compatibili (cioe' se una possa sostituire l' altra)
            // e' delicata: se mi baso solo sui costi assoluti lascio fuori dei casi, se mi baso sui tempi
            // ho comunque dei casi anomali. Pertanto ho messo un test polarizzato (mirato alla soluzione preferibile)
            // il che' e' abbastanza una schifezza perche' e' una definizione "quasi" circolare.
            // Comunque cosi' funziona bene: se si cambia qualunque cosa fare dei test ESTESI
            // controllando in particolare una gran mole di dati: suggerisco di lanciare i test prima e dopo
            // la modifica (non accoppiata ad altre) e vedere che succede
            // -------------------------------------------------------------------------------------------------------
            // LA QUALITA' DELLE SOLUZIONI DIPENDE IN MISURA CRITICA DA QUESTA LOGICA
            // -------------------------------------------------------------------------------------------------------

            // Se le due soluzioni usano lo stesso insieme di treni una delle due deve essere assolutamente scartata  (considero solo le tratte reali)
            BOOL Identici = FALSE;
            if (Sol.NumeroTratte == Sol2.NumeroTratte) {
               for (int tr = Sol.NumeroTratte -1; tr >= 0; tr --) {
                  // Qui dovrei usare una memcompare: Ma faccio un truccaccio per andare piu' veloce
                  struct MY_S { // 6 Bytes comunque messi per poter definire l' operatore !=
                     DWORD A; WORD B;
                     BOOL operator  != (MY_S x) {return A != x.A || B != x.B;};
                  };
                  if ((*(MY_S*)Sol.Tratte[tr].IdTreno) != (*(MY_S*)Sol2.Tratte[tr].IdTreno)) break;
               } /* endfor */
               Identici = tr < 0;
            } /* endif */
            // Identifico i casi in cui una delle due soluzioni deve sempre scartarne un' altra:
            //    Se usano gli stessi treni (fisici)
            //    Se partono ed arrivano alla stessa ora: Sono variazioni sul tema e (esclusi i traghetti) debbo sempre scartarne una
            BOOL Absolute = Identici || ((DeltaTime == 0 && SolIniziale == Sol2Iniziale) && !( Sol.IdTraghetto && Sol2.IdTraghetto && Sol.IdTraghetto != Sol2.IdTraghetto) ) ;

            // Trace dei parametri identificati
            #ifdef DBG25A
            if(Sol.InAttenzione && Sol2.InAttenzione){
               TRACESTRING(VRO(Sol.TempoTotaleDiPercorrenza) + VRO(Sol2.TempoTotaleDiPercorrenza) + VRS(Sol.CostoAssoluto) + VRS(Sol2.CostoAssoluto) + VRS(LimiteVirtuale) + VRS(LimiteReale) );
               TRACESTRING(VRS(DeltaTime) + VRS(Absolute) + VRS(Identici) + VRS(Compatibili) + VRS(DeltaInizio) + VRS(TempoDiSovrapposizioneVirtuale)+VRS(TempoDiSovrapposizioneReale));;
            }
            #endif


            if (Identici) {
               // DeltaTime puo' essere diverso da 0 per i treni con percorsi circolari
               if (DeltaTime < 0) {
                  Situazione =1;
               } else if (DeltaTime > 0) {
                  Situazione =2;
               } else {
                  // Cerco di basarmi sul tempo di attesa: altrimenti sono proprio equivalenti
                  // Privilegio (per i percorsi circolari) il massimo tempo di attesa
                  Situazione = Sol.TempoTotaleDiAttesa > Sol2.TempoTotaleDiAttesa  ? 1 : 2;
               } /* endif */
            } else if(!Compatibili){
               Situazione = 0; // Non preferisco nessuna
            }

            // Adesso verifico se effettivamente la sostituzione possa
            // avere luogo
            if(Situazione){
               if(Situazione == 1){
                  if(Absolute || Sol.OkSost(Sol2)){
                     #ifdef DBG25A
                     if(Sol2.InAttenzione){
                        if(!Sol.InAttenzione) Sol.Trace("±±± Soluzione Preferibile :");
                        Sol2.Trace("±±±  Soluzione che viene sostituita :");
                     }
                     #endif
                     if( Sol2.IsTargetAutoChk ){
                        DATI_ORARIO_FS::Target.Imposta(Sol);
                        Sol.CalcolaCosto(); // Per fare un trace
                     }

                     #ifdef DBG1
                     NumSostituite ++;
                     #endif

                     Orario.Soluzioni.Elimina(j);
                     Orario.NuoveSoluzioni = TRUE; // per il criterio di arresto

                  } else {
                     #ifdef DBG25A
                     if(Sol2.InAttenzione && Sol.InAttenzione ) TRACESTRING("±±±  Soluzione accettata insieme a soluzione precedente causa OkSost");
                     #endif
//<<<             if Absolute || Sol.OkSost Sol2
                  }
                  // Non faccio un break perche' potrebbe sostituire piu' di una soluzione
//<<<          if Situazione == 1
               } else {
                  if(Absolute || Sol2.OkSost(Sol)){
                     #ifdef DBG25A
                     if(Sol.InAttenzione){
                        if(!Sol2.InAttenzione) Sol2.Trace("±±± Alternativa Preferibile :");
                        TRACESTRING("±±±  Soluzione cassata");
                     }
                     #endif
                     if( Sol.IsTargetAutoChk ){
                        DATI_ORARIO_FS::Target.Imposta(Sol2);
                        Sol2.CalcolaCosto(); // Per fare un trace
                     }
                     Sol.Valida = FALSE;
                     break;
                  } else {
                     #ifdef DBG25A
                     if(Sol2.InAttenzione && Sol.InAttenzione ) TRACESTRING("±±±  Soluzione accettata causa OkSost");
                     #endif
                  }
               }
//<<<       if Situazione
            }

//<<<    FORALL Orario.Soluzioni,j
         }
//<<< if Sol.Valida
      }


      #endif
      // ..................................
      // Metto la soluzione nell' elenco di soluzioni di DATI_ORARIO_FS
      // ..................................
      if(Sol.Valida){
         #ifdef DBG1
         NumAggiunte ++;
         #endif
         Orario.Soluzioni.SpostaSoluzione(THIS,i);
         Orario.NuoveSoluzioni = TRUE; // per il criterio di arresto
      }
      #ifdef DBG28
      if(Sol.Valida)Sol.Trace("Semplifica() Soluzione valida",2);
      #endif
//<<< for  int i = Dim  -1;i >=0 ;i --     // Controllo tutte le soluzioni
   }

   Clear(); // Cancello le soluzioni non valide

   #if  defined(DBG25) || defined(DBG28)
   TRACESTRING("Soluzioni valide Nø "+STRINGA(Orario.Soluzioni.Dim())+ " Percorsi Nø"+ STRINGA(Orario.Grafo.PercorsiGrafo.Dim()));
   #endif
   #ifdef DBG1
   TRACESTRING(" Aggiunte "+STRINGA(NumAggiunte)+" soluzioni, sostituendo "+STRINGA(NumSostituite)+" soluzioni precedenti");
   #endif

   return TRUE;
//<<< BOOL ELENCO_SOLUZIONI::Semplifica
};

//----------------------------------------------------------------------------
// DATI_ORARIO_FS::Semplifica(void)
//----------------------------------------------------------------------------
// Questa routine permette una semplificazione finale basata
// sull' insieme globale dei criteri
// Vengono anche corrette alcune anomalie delle soluzioni trovate
// ed identificata la periodicit… di ognuna di esse
//----------------------------------------------------------------------------
BOOL DATI_ORARIO_FS::Semplifica(void)
{
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::Semplifica"

   if (ElaborazioneInterrotta) return TRUE;

   for (int i = Soluzioni.Dim()-1;i >=0 ;i -- ) { // Controllo tutte le soluzioni
      SOLUZIONE & Sol = *Soluzioni[i];
      if (Sol.Km > DistanzaLimite && Sol.IdTraghetto == 0){
         #ifdef DBG25
         if(Sol.InAttenzione){
            Sol.Trace("Soluzione troppo lunga ("+STRINGA(Sol.Km)+" Km): NON VALIDA",2);
         }
         #endif
         if( Sol.IsTargetAutoChk ) Sol.Trace(" Soluzione troppo lunga ("+STRINGA(Sol.Km)+" Km): DA ELIMINARE",1);
         Soluzioni.Elimina(i);
      } else {
         Sol.Semplifica(); // Semplifica la soluzione, eliminandone eventuali anomalie per treni e stazioni di cambio
         if(!Sol.Valida){
            if( Sol.IsTargetAutoChk ) Sol.Trace(" Eliminata soluzione da SEMPLIFICA (errori interni algoritmo)",1);
            Soluzioni.Elimina(i); // Semplifica pu• invalidare la soluzione per errori di algoritmo
         }
      } /* endif */
   }


   #ifdef DBGsempli
   TRACESTRING("Soluzioni valide Nø "+STRINGA(Soluzioni.Dim())+" Instradamenti Nø"+
      STRINGA(Grafo.Instradamenti.Dim())+" Percorsi Nø"+
      STRINGA(Grafo.PercorsiGrafo.Dim()));
   #endif

   AutoDiagnostica = FALSE;

   return TRUE;
//<<< BOOL DATI_ORARIO_FS::Semplifica void
}

//----------------------------------------------------------------------------
// DATI_ORARIO_FS::NewCriterioDiArresto();
//----------------------------------------------------------------------------
// Questa routine stabilisce quando e' il momento di considerare
// conclusa la ricerca.
// Al momento ha una logica brutale:
// Sono impostate due grandezza, BASE e DELTA (= BASE /2)
// Vengono generate ipotesi BASE volte.
// Si riazzera NuoveSoluzioni
// Vengono generate altre  DELTA ipotesi
// Se si hanno NuoveSoluzioni si aggiunge DELTA a base, si imposta Delta a BASE/3 e si ricicla
// finche' o non si hanno piu' nuove soluzioni o finche' si raggiunge un massimo numero di cicli.
// Oltre che per le nuove soluzioni la stessa logica si applica se ho meno di 4 soluzioni
//
// Alla fine si inibiscono gli I/O e si fa' continuare la routines per un numero di cicli
// legato ai cicli gia' fatti , usando solo i dati che gia' stanno in cache
//
// NB: Deve girare sul thread che trova le soluzioni, o genera abend sporadici
//----------------------------------------------------------------------------
BOOL DATI_ORARIO_FS::NewCriterioDiArresto(const int i)
{
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::NewCriterioDiArresto();"

   if(ElaborazioneInterrotta){
      ERRSTRING("Interruzione Elaborazione OK.");
      return FALSE;
   }

   if(i == 0){
      DifficoltaProblema  = DIFFICILE;
      Base = MM_CR.BASE;
      Delta = Base / 2;
      Base  += Delta;
   };

   if(i < MM_CR.DIRETTO) return TRUE;  // Nø Min assoluto di cicli
   if(i > MM_CR.MAXPT){
      ERRSTRING("Stop per raggiunto massimo paths generabili");
      return FALSE; // Nø Max assoluto di cicli
   }


   if( i == MM_CR.DIRETTO ){
      #ifdef DBG2
      TRACESTRING(" i == MM_CR.DIRETTO per " VRS(i));
      #endif
      Classifica();
      if (DifficoltaProblema == DIRETTO_FACILE) return FALSE; // BASTA COSI'
   }

   if (DifficoltaProblema == FACILE) {
      IPOTESI::UsaDatiInCache = i > Base       ; // Lavoro in modo ESTESO se ho almeno generato il numero minimo di PATHs
      // Per le situazioni in cui vado da sardegna al continente o viceversa non passo mai ad un utilizzo diretto
      // dei dati in cache perche' perdo troppe soluzioni
      if (GestioneParticolareTraghetti ) IPOTESI::UsaDatiInCache = FALSE;
      return i <  Base * MM_CR.EXT_IPO  / 10   ; // Mi fermo dopo aver generato le estensioni
   } /* endif */

   // Se arriva qui vuol dire che il problema e' stato catalogato come DIFFICILE
   // (oppure e' nella fase iniziale in cui per default il problema e' catalogato come DIFFICILE)

   if( i == MM_CR.BASE){   // E' il primo punto di decisione
      #ifdef DBG2
      TRACESTRING(" i == MM_CR.BASE per " VRS(i));
      #endif
      Classifica();            // Riclassifico il problema
      NuoveSoluzioni = FALSE;  // Per vedere se verranno generate nuove soluzioni
      return TRUE;             // Per ora posso tornare senza problemi
   }

   if(IPOTESI::UsaDatiInCache ){  // Sono nella fase di estensione
      #ifdef DBG2
      if (i == Extend)  TRACESTRING(" i == Extend per " VRS(Extend));
      #endif
      return i < Extend;    // Continuo fino a che la fase non sia finita
   }

   if( i == Base ){ // E' un successivo punto di decisione
      #ifdef DBG2
      TRACESTRING(" i == Base per " VRS(i));
      #endif
      if(NuoveSoluzioni || Soluzioni.Dim() < 4){
         NuoveSoluzioni = FALSE;
         Base += Delta;
         Delta = Base /3;
         #ifdef DBG2
         TRACESTRING(" Impostato " VRS(Base));
         #endif
      } else {
         #ifdef NO_EXTEND
         return FALSE;
         #else
         Extend = i * MM_CR.EXT_IPO / 10;
         IPOTESI::UsaDatiInCache = TRUE  ; // Lavoro in modo ESTESO
         // Per le situazioni in cui vado da sardegna al continente o viceversa non passo mai ad un utilizzo diretto
         // dei dati in cache perche' perdo troppe soluzioni
         if (GestioneParticolareTraghetti ) IPOTESI::UsaDatiInCache = FALSE;
         #ifdef DBG2
         TRACESTRING(" Impostato " VRS(Extend));
         #endif
         #ifdef DBG2A
         PROFILER::Trace(TRUE,1);
         PROFILER::Acquire(TRUE)         ; // Somma i dati di parziale in Totale; Opzionalmente azzera Parziale
         #endif
         #endif
//<<< if NuoveSoluzioni || Soluzioni.Dim   < 4
      }
//<<< if  i == Base    // E' un successivo punto di decisione
   } else if( i >= MM_CR.MAX_IPO){
      #ifdef NO_EXTEND
      return FALSE;
      #else
      Extend = i * MM_CR.EXT_IPO / 10;
      IPOTESI::UsaDatiInCache = TRUE;
      // Per le situazioni in cui vado da sardegna al continente o viceversa non passo mai ad un utilizzo diretto
      // dei dati in cache perche' perdo troppe soluzioni
      if (GestioneParticolareTraghetti ) IPOTESI::UsaDatiInCache = FALSE;
      #ifdef DBG2
      TRACESTRING(" i == MM_CR.MAX_IPO per " VRS(MM_CR.MAX_IPO));
      TRACESTRING(" Impostato " VRS(Extend));
      #endif
      #ifdef DBG2A
      PROFILER::Trace(TRUE,1);
      PROFILER::Acquire(TRUE)         ; // Somma i dati di parziale in Totale; Opzionalmente azzera Parziale
      #endif
      #endif
   }

   return TRUE;
//<<< BOOL DATI_ORARIO_FS::NewCriterioDiArresto const int i
}
//----------------------------------------------------------------------------
// Ordinamento delle soluzioni per costo
//----------------------------------------------------------------------------
int MM_Sort_Costo( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "MM_Sort_Costo()"
   SOLUZIONE * A = *(SOLUZIONE **) a;
   SOLUZIONE * B = *(SOLUZIONE **) b;
   return int(A->Costo.MinutiEquivalenti) - int(B->Costo.MinutiEquivalenti);
};
//----------------------------------------------------------------------------
// DATI_ORARIO_FS::Classifica()
//----------------------------------------------------------------------------
// Classifica le soluzioni, operando con la seguente logica:
//
//   - Sorta le soluzioni per Costo
//   - Identifica il limite di costo accettabile per soluzioni buone e per soluzioni accettabili
//     Nel costo NON vanno comprese le percorrenze notturne
//   - Elimina le soluzioni non accettabili
// Inoltre determina se l' insieme complessivo di soluzioni e' soddisfacente, e
// ricataloga il livello di risoluzione complessivo del problema.
//----------------------------------------------------------------------------
void DATI_ORARIO_FS::Classifica(){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::Classifica"

   int NumSoluzioni =  Soluzioni.Dim();
   #ifdef DBG3
   TRACEVLONG(NumSoluzioni);
   WORD NumBuone = 0;
   #endif

   if(NumSoluzioni <= 1){
      if(NumSoluzioni) Soluzioni[0]->Buona = TRUE; // Quando non si ha altro tutto fa brodo
      return;
   };

   if(MultiThread) DOE1; // I Sort vanno semaforizzati per problemi Borland
   qsort(&Soluzioni[0],Soluzioni.Dim(),sizeof(void*),MM_Sort_Costo);
   if(MultiThread) DOE2;

   WORD CostoTipico = Soluzioni[0]->Costo.MinutiEquivalenti;
   if(NumSoluzioni > 4 ) CostoTipico  = Soluzioni[1]->Costo.MinutiEquivalenti;
   if(NumSoluzioni > 10 ) CostoTipico = Soluzioni[2]->Costo.MinutiEquivalenti;
   if(NumSoluzioni > 20 ) CostoTipico = Soluzioni[4]->Costo.MinutiEquivalenti;
   WORD LimiteCostoTraghetti = 60000; // Very High
   ORD_FORALL(Soluzioni, IdxTr)if(Soluzioni[IdxTr]->IdTraghetto){
      LimiteCostoTraghetti = Soluzioni[IdxTr]->Costo.MinutiEquivalenti + 480; // 8 Ore fisse di margine massimo
      break;
   }

   WORD LimiteCostoPerSoluzioneBuona = CostoTipico + 45;           // 45 minuti di margine
   Top(LimiteCostoPerSoluzioneBuona, CostoTipico * 12 / 10);       // Oppure il 20%
   WORD LimiteCostoPerSoluzioneAccettabile = CostoTipico + 180;    // 3 ore di margine
   Top(LimiteCostoPerSoluzioneAccettabile, CostoTipico * 15 / 10); // Oppure il 50%
   Bottom(LimiteCostoPerSoluzioneAccettabile, CostoTipico + 600);  // Con un limite di 10 ore.
   // Questo per non favorire troppo le soluzioni notturne
   WORD LimiteCostoAssolutoPerSoluzioneAccettabile = max( CostoTipico *2, LimiteCostoPerSoluzioneAccettabile);
   // Questo per i livelli alti di ricerca
   WORD LimiteCostoAssurdo = max( CostoTipico *25 / 10 , LimiteCostoPerSoluzioneAccettabile * 15 / 10);

   // Il numero di fasce su cui posso avere una soluzione NON PUO sicuramente superare
   // il numero di fasce coperte dai treni in partenza dall' origine, o in arrivo alla
   // destinazione. E' un limite approssimato, che non copre tutti i casi, ove fallisca
   // la ricerca andra' avanti piu' del necessario.
   WORD FascePartenza = NODO_CAMBIO::NodiDiCambio[IdOrigine()].FascePartenza;
   BYTE NumFasceSoddisfacentiP = NumeroBits( FascePartenza);
   if(NumFasceSoddisfacentiP >= 6) NumFasceSoddisfacentiP --;
   if(NumFasceSoddisfacentiP >= 3) NumFasceSoddisfacentiP --;
   WORD FasceArrivo   = NODO_CAMBIO::NodiDiCambio[IdDestinazione()].FascePartenza;
   BYTE NumFasceSoddisfacentiA = NumeroBits( FasceArrivo  );
   if(NumFasceSoddisfacentiA >= 6) NumFasceSoddisfacentiA --;
   if(NumFasceSoddisfacentiA >= 3) NumFasceSoddisfacentiA --;

   #ifdef DBG3A
   TRACESTRING(
      VRO(CostoTipico                               )  +" ("+STRINGA(CostoTipico                               )+")"+
      VRO(LimiteCostoPerSoluzioneBuona              )  +" ("+STRINGA(LimiteCostoPerSoluzioneBuona              )+")"+
      VRO(LimiteCostoPerSoluzioneAccettabile        )  +" ("+STRINGA(LimiteCostoPerSoluzioneAccettabile        )+")"+
      VRO(LimiteCostoAssolutoPerSoluzioneAccettabile)  +" ("+STRINGA(LimiteCostoAssolutoPerSoluzioneAccettabile)+")");
   TRACESTRING(VRS(FascePartenza) + VRS(FasceArrivo  )+ VRS(NumFasceSoddisfacentiP) + VRS(NumFasceSoddisfacentiA));
   #endif
   #define DURATA_NOTTE ( FINE_NOTTE - INIZIO_NOTTE )

   WORD FasceP = 0, FasceA =0 , FasceDP = 0, FasceDA = 0;
   for (int i = NumSoluzioni-1; i > 0  ;i-- ) {
      SOLUZIONE & Soluzione = *Soluzioni[i];
      WORD Nt = Soluzione.NumeroTratteReali; // I collegamenti diretti ed i traghetti sono sempre soluzioni BUONE
      WORD PercNotte;
      if ( Soluzione.OraPartenza() >= Soluzione.OraArrivo()  ){
         PercNotte = DURATA_NOTTE;
      } else if (Soluzione.OraArrivo() <= INIZIO_NOTTE || Soluzione.OraPartenza() >= FINE_NOTTE) {
         PercNotte = 0;
      } else {
         PercNotte = TempoTrascorso( max(Soluzione.OraPartenza(), INIZIO_NOTTE) , min(FINE_NOTTE, Soluzione.OraArrivo()) );
      };
      // Nello scarto delle soluzioni troppo lunghe non debbo tener conto delle ore notturne (a questo punto pero' penalizzo le attese)
      BOOL CostoProibitivo = Soluzione.Costo.MinutiEquivalenti  > LimiteCostoPerSoluzioneAccettabile && (
         (Soluzione.Costo.MinutiEquivalenti + Soluzione.TempoTotaleDiAttesa - PercNotte) > LimiteCostoPerSoluzioneAccettabile ||
         Soluzione.Costo.MinutiEquivalenti > LimiteCostoAssolutoPerSoluzioneAccettabile
      );
      #ifdef DBG3A
      TRACESTRING( VRS(CostoProibitivo) + VRS(Soluzione.Costo.MinutiEquivalenti) );
      TRACESTRING( VRO(Soluzione.OraPartenza()) + VRO(Soluzione.OraArrivo()) + VRS(PercNotte) + VRS(Soluzione.Costo.MinutiEquivalenti) + VRS(Soluzione.TempoTotaleDiAttesa) );
      #endif
      if (Nt > 1 && ((MM_CR.EliminaTroppoCostose && CostoProibitivo)  || Soluzione.Costo.MinutiEquivalenti > LimiteCostoAssurdo )) {
         if ((Soluzione.IdTraghetto == 0) || (Soluzione.Costo.MinutiEquivalenti > LimiteCostoTraghetti)) {
            #ifdef DBG3A
            Soluzione.Trace("Soluzione DA ELIMINARE: ");
            #endif
            Soluzioni.Elimina(i);
         } else {
            Soluzione.Buona = FALSE;
            #ifdef DBG3A
            Soluzione.Trace("Soluzione traghetto ACCETTABILE ma non buona: ");
            #endif
         } /* endif */
      } else if (Nt > 1 && Soluzione.Costo.MinutiEquivalenti > LimiteCostoPerSoluzioneBuona) {
         Soluzione.Buona = FALSE;
         #ifdef DBG3A
         Soluzione.Trace("Soluzione ACCETTABILE ma non buona: ");
         #endif
      } else {
         Soluzione.Buona = TRUE;
         #ifdef DBG3
         NumBuone ++;
         #endif

         if (Soluzione.NumeroTratteReali <= 3) {  // Se ho al massimo due cambi
            // Individuo la fascia oraria di appartenenza
            FasceP |= FasciaOraria( Soluzione.OraPartenza() );
            FasceA |= FasciaOraria( Soluzione.OraArrivo()   );
         }
         if (Soluzione.NumeroTratteReali == 1) {  // Se e' un collegamento diretto
            // Individuo la fascia oraria di appartenenza
            FasceDP |= FasciaOraria( Soluzione.OraPartenza() );
            FasceDA |= FasciaOraria( Soluzione.OraArrivo()   );
         } /* endif */
         #ifdef DBG3A
         Soluzione.Trace("Soluzione BUONA: ");
         #endif
//<<< if  Nt > 1 &&   MM_CR.EliminaTroppoCostose && CostoProibitivo   || Soluzione.Costo.MinutiEquivalenti > LimiteCostoAssurdo
      } /* endif */
//<<< for  int i = NumSoluzioni-1; i > 0  ;i--
   } /* endfor */

   FasceP &= FascePartenza ; // Lascio solo le coincidenze
   FasceA &= FasceArrivo   ; // Lascio solo le coincidenze
   FasceDP &= FascePartenza ; // Lascio solo le coincidenze
   FasceDA &= FasceArrivo   ; // Lascio solo le coincidenze
   BYTE NumFasceSoddisfatteP  = NumeroBits( FasceP  );
   BYTE NumFasceSoddisfatteA  = NumeroBits( FasceA  );
   BYTE NumFasceSoddisfatteDP = NumeroBits( FasceDP );
   BYTE NumFasceSoddisfatteDA = NumeroBits( FasceDA );

   BOOL SoluzioniSoddisfacenti  = NumFasceSoddisfatteP  >= NumFasceSoddisfacentiP || NumFasceSoddisfatteA  >= NumFasceSoddisfacentiA ;
   BOOL SoluzioniSoddisfacentiD = NumFasceSoddisfatteDP >= NumFasceSoddisfacentiP || NumFasceSoddisfatteDA >= NumFasceSoddisfacentiA ;

   DifficoltaProblema  = DIFFICILE;
   if (SoluzioniSoddisfacenti ) DifficoltaProblema  = FACILE ;
   if (SoluzioniSoddisfacentiD) DifficoltaProblema  = DIRETTO_FACILE ;

   #ifdef DBG3
   Bprintf("Numero soluzioni Buone %i ed accettabili %i su %i totali",NumBuone , Soluzioni.Dim() - NumBuone, Soluzioni.Dim());
   TRACESTRING( VRS(SoluzioniSoddisfacenti) + VRS(NumFasceSoddisfacentiP) + VRS(NumFasceSoddisfatteP) + VRS(NumFasceSoddisfacentiA) + VRS(NumFasceSoddisfatteA)  );
   TRACESTRING( VRS(SoluzioniSoddisfacentiD) + VRS(NumFasceSoddisfacentiP) + VRS(NumFasceSoddisfatteDP) + VRS(NumFasceSoddisfacentiA) + VRS(NumFasceSoddisfatteDA)  );
   TRACEVLONG( DifficoltaProblema );
   #endif

//<<< void DATI_ORARIO_FS::Classifica
};

//----------------------------------------------------------------------------
//  SOLUZIONE::OkSost
//----------------------------------------------------------------------------
// Controlla che siano verificate le seguenti condizioni di sostituibilita':
//   - Collegamenti diretti non sono sostituibili
//   - Considero i "costi" aggiuntivi per soluzione.
//     Una soluzione con costo maggiore non puo' sostituirne una con costo minore
//     (La affianca invece)
//   - Una soluzione con mezzo navale puo' essere sostituita solo da un' altra
//     soluzione che possiede le stesso mezzo navale
//----------------------------------------------------------------------------
BOOL SOLUZIONE::OkSost(SOLUZIONE & Sol2){    // Controlla se la soluzione possa sostituire Sol2
   #undef TRCRTN
   #define TRCRTN "SOLUZIONE::OkSost()"

   #ifdef DBG25A
   if(InAttenzione && Sol2.InAttenzione) TRACESTRING(VRS(Costo) + VRS(Sol2.Costo) + VRS(Sol2.NumeroTratteReali ));
   #endif

   // ---------------------------------------------------------------------------------------
   // Non si puo' sostituire una soluzione con una sola tratta
   // ---------------------------------------------------------------------------------------
   if(Sol2.NumeroTratte == 1) return FALSE;

   // ---------------------------------------------------------------------------------------
   // Analoga, ma tiene in condiderazione le tratte urbane
   // Il criterio e' il seguente: ammetto la sostituzione solo se anche la soluzione
   // che sostituisce ha solo una tratta (escluse le tratte urbane)
   // ---------------------------------------------------------------------------------------
   if(Sol2.NumeroTratteReali == 1 && NumeroTratteReali != 1)return FALSE;

   // ---------------------------------------------------------------------------------------
   // Una soluzione con mezzo navale puo' essere sostituita solo da un' altra
   // soluzione che possiede le stesso mezzo navale
   // ---------------------------------------------------------------------------------------
   if( Sol2.IdTraghetto && (Sol2.IdTraghetto != IdTraghetto ))return FALSE;

   // ---------------------------------------------------------------------------------------
   // A questo punto la sostituzione potrebbe avere luogo.
   // Faccio dei controlli ulteriori per vedere se, prendendo in considerazione altri criteri
   // oltre al tempo di percorrenza, la seconda soluzione diventerebbe preferibile rispetto
   // alla prima. Se cosi' fosse le tengo entrambi
   // ---------------------------------------------------------------------------------------
   return Costo <  Sol2.Costo;

//<<< BOOL SOLUZIONE::OkSost SOLUZIONE & Sol2      // Controlla se la soluzione possa sostituire Sol2
}
//----------------------------------------------------------------------------
// COLLEGAMENTO_DIRETTO::ControllaAttesaValida()
//----------------------------------------------------------------------------
BOOL __fastcall COLLEGAMENTO_DIRETTO::ControllaAttesaValida(){
   #undef TRCRTN
   #define TRCRTN "COLLEGAMENTO_DIRETTO::ControllaAttesaValida()"
   WORD AttesaV = AttesaValida(LastOraArrivo, OraLimite) + AttesaValida(OraLimite, Treni[TrenoCorrente].OraPartenza);
   if(Treni[TrenoCorrente].Navale || (Last && Last->Navale)){
      return AttesaV <= MAX_ATTESA_TRAGHETTI;  // Mai piu' di dodici ore
   } else {
      return AttesaV <= MM_CR.MaxAttesa ;
   }
}
//----------------------------------------------------------------------------
// CattivoUsoMezzoDiQualita
//----------------------------------------------------------------------------
// Questo classificatore e' utilizzato dal motore per identificare le tratte con cattivo utilizzo
// dei mezzi di qualita' .
// Al momento il concetto di mezzo di qualita' coincide con il concetto di "prenotabilita' intrinseca",
// ma non e' detto rimanga cosi'
//----------------------------------------------------------------------------
int  __fastcall CattivoUsoMezzoDiQualita(MM_SOLUZIONE::TRATTA_TRENO & Tratta){
   #undef TRCRTN
   #define TRCRTN  "CattivoUsoMezzoDiQualita"

   // Questo raddoppia le penalizzazioni per Km < 50
   // if(Tratta.InfoTreno.DiQualita())return Tratta.TrattaCorta;
   if(Tratta.InfoTreno.DiQualita() && Tratta.TrattaCorta) return 1;
   return 0;
}
//----------------------------------------------------------------------------
// SOLUZIONE::CalcolaCosto"
//----------------------------------------------------------------------------
//  Calcola un "Costo" equivalente di soluzione (che viene usato per inibire la sostituzione)
//  - Il costo base e' il tempo di percorrenza
//  - Ad ogni cambio faccio corrispondere un costo di MinEqvCambio minuti
//  - Ad ogni MinEqvKm Km di percorso in piu' faccio corrispondere un costo di un minuto
//  - I cambi effettuati tra l' una e le sei del mattino comportano una penalizzazione
//    perche' disturbano tutti i passeggeri e perche' son scomodi purche'
//    il viaggio sia iniziato prima dell' una.
//    Escluse le tratte urbane (perchŠ non vincolanti come orario).
//    Se entrambi i treni NON hanno cuccette o VagonLit la penalizzazione e'
//    di MM_CR.CostoCambioNotturno altrimenti del doppio
//  - Si considera SICURO un tempo di attesa di MM_CR.CAMBIO_SICURO minuti
//    Escludendo i cambi VERSO le multistazioni (ma non DALLE multistazioni)
//    e le multistazioni iniziali si considera come penalizzazione la differenza
//    tra MM_CR.CAMBIO_SICURO ed il tempo effettivo di attesa
//  - Uso in alternativa un secondo criterio analogo al precedente (ma meno stringente)
//    che genera un costo (quindi additivo). Usa la variabile MM_CR.CAMBIO_SICURO2
//  - L' utilizzo di un treno a supplemento per una tratta inferiore alla meta' del
//    percorso complessivo del mezzo virtuale (o MM_CR.Lim1PenSuppl Km )
//    comporta MM_CR.NOQUAL minuti di penalizzazione.
//    Tale penalizzazione Š raddoppiata se il percorso e' inferiore a Lim2PenSuppl
//    Se ho due o pi— treni a supplemento consecutivi per applicare il criterio si
//    sommano le percorrenze delle tratte consecutive (pi— o meno).
//    Ignoro eventuali tratte urbane intermedie
//  - L' utilizzo di una tratta urbana comporta una penalizzazione di MM_CR.URBANO minuti
//  - Se la multistazione ha un tempo di coincidenza <= 5 minuti lo considero
//    un cambio nella stessa stazione, e NON aggiungo la penalizzazione n‚ il costo della tratta!
//  - Vi e' anche un fattore addizionale, che entra in gioco solo quando il costo di due
//    soluzioni e' quasi eguale: questo fattore tiene in conto il numero di "connessioni"
//    che una stazione di cambio ha con altre stazioni di cambio. Tende a far scegliere
//    come nodo di cambio (a parita' di altri fattori) delle stazioni da cui partono
//    treni verso un maggior numero di destinazioni.
// Quando si e' indicato il termine "penalizzazione" si intende che non e' cumulativo
// su piu' tratte mentre il costo lo Š.
// Oltre al costo si calcola anche un "CostoAssoluto" che tiene conto di solo parte dei precedenti
// fattori e che viene usato per forzare la sostituzione.
// I fattori considerati (con un differente peso) sono:
//  - Tempo
//  - Numero cambi
//  - Cattivo uso dei treni a supplemento
//  - Uso di tratte urbane
//  - Indice di connessione della stazione di cambio
//----------------------------------------------------------------------------
void __fastcall SOLUZIONE::CalcolaCosto(){
   #undef TRCRTN
   #define TRCRTN "SOLUZIONE::CalcolaCosto"

   if (DATI_ORARIO_FS::AutoDiagnostica) {
      IsTargetAutoChk = DATI_ORARIO_FS::Target.Eqv(THIS);
   } /* endif */

   // Comverto in minuti equivalenti i fattori di disagio del maggior numero di
   // cambi e della maggiore percorrenza chilometrica, ed altri criteri sul modo in cui
   // eseguo i cambi.
   Costo  = TempoTotaleDiPercorrenza; // Questa e' la base
   CostoAssoluto = TempoTotaleDiPercorrenza; // Questa e' la base
   Costo += MM_CR.MinEqvCambio * NumeroTratte ;
   Costo += Km / MM_CR.MinEqvKm;
   CostoAssoluto += MM_CR.MinEqvCambio2 * NumeroTratte ;

   // Queste penalizzazioni comportano criteri complessi:
   // - Gestione dei cambi notturni
   // - Cambio sicuro
   // - Utilizzo di un treno a supplemento per una piccola parte del suo percorso
   // Si noti che le singole penalizzazioni non sono cumulate su piu' tratte (si prende la maggiore)
   // Vi' inoltre una seconda penalizzazione per cambio (meno stringente) che viene cumulata su piu' tratte
   WORD PenalizzCambio      = 0;
   WORD Penalizz2Cambio     = 0;
   WORD PenalizzNotturna    = 0;
   WORD PenalizzSuppl       = 0;
   WORD IndiceConnes        = 0;
   WORD PenalizzUrbana      = Tratte[0].IdMezzoVirtuale == 0 ? MM_CR.URBANO : 0;
   WORD OraPvera = OraPartenzaVera();
   BOOL PartoPrimaDellaNotte = OraPvera <= INIZIO_NOTTE || OraPvera > FINE_NOTTE;
   WORD OraLastArrivo          ;
   WORD ApplicaSupplemento  =999;
   WORD KmSuppl = 0;
   BOOL LastCambioStessaStazione = FALSE;
   for (int i = 0; i < NumeroTratte ; i++ ) {
      TRATTA_TRENO & Tratta = Tratte[i];
      if (Tratta.IdMezzoVirtuale == 0 ){
         // Non penalizzo i cambi notturni su multistazione (potrei anche farlo ma
         //    cosi' gira meglio per prendere l' ultimo treno della notte.
         //    Se il cambio Š realmente a met… notte penalizzo alla tratta
         //    precedente o successiva ).
         // Analogamente per i tempi di attesa in arrivo (perche' fittizi)

         // Vedo se siamo nella stessa stazione o nel relativo piazzale
         // Il criterio e' che e' la stessa stazione se il tempo di spostamento
         // e' di 5 minuti o meno
         // Analogamente la penalit… va eliminata per le stazioni marittime
         if(Tratta.NoPenalita){
            // Elimino il costo della tratta !
            Costo         -= MM_CR.MinEqvCambio   ;
            CostoAssoluto -= MM_CR.MinEqvCambio2  ;
            LastCambioStessaStazione = TRUE;
         } else {
            PenalizzUrbana = MM_CR.URBANO;
            LastCambioStessaStazione = FALSE;
         }
      } else {
         if(i > 0){ // Queste sono le penalit… associate ai cambi
            WORD TempoAttesa = TempoTrascorso( OraLastArrivo, Tratta.OraIn);
            if (!LastCambioStessaStazione) { // Altrimenti il tempo di cambio non Š critico
               if(TempoAttesa < MM_CR.CAMBIO_SICURO)  Top(PenalizzCambio, (MM_CR.CAMBIO_SICURO - TempoAttesa ));
               if(TempoAttesa < MM_CR.CAMBIO_SICURO2) Penalizz2Cambio +=  MM_CR.CAMBIO_SICURO2 - TempoAttesa ;
            } /* endif */
            if(PartoPrimaDellaNotte) { // Debbo penalizzare i cambi notturni
               if( (Tratta.OraIn > INIZIO_NOTTE && Tratta.OraIn < FINE_NOTTE) || (Tratta.OraOut > INIZIO_NOTTE && Tratta.OraOut < FINE_NOTTE) ) {
                  if (Tratta.InfoTreno.LettiOCuccette()) {
                     PenalizzNotturna = MM_CR.CostoCambioNotturno + MM_CR.CostoCambioNotturno ; // Due ore
                  } else {
                     Top(PenalizzNotturna,MM_CR.CostoCambioNotturno) ;  // Un Ora
                  } /* endif */
               }
            } /* endif */
         } /* endif */

         if( Tratta.Km > 0 ){ // E queste sono le penalit… associate ai treni
            // L' anomalia potrebbe sorgere in futuro per treni cui non
            // so associare ad un chilometraggio.
            if (Tratta.InfoTreno.Supplemento) {
               KmSuppl += Km ;
               // Combino in modo che se una delle tratte consecutive non Š penalizzata
               // posso sicuramente prendere l' insieme delle due tratte.
               Bottom(ApplicaSupplemento, Tratta.TrattaCorta);
            } else {
               if(ApplicaSupplemento && ApplicaSupplemento != 999 ){
                  if( KmSuppl < MM_CR.Lim1PenSuppl ){
                     if( ApplicaSupplemento >= 2 && KmSuppl < MM_CR.Lim2PenSuppl ){
                        Top(PenalizzSuppl, MM_CR.NOQUAL + MM_CR.NOQUAL );
                     } else  {
                        Top(PenalizzSuppl, MM_CR.NOQUAL );
                     }
                  }
               }
            } /* endif */
            ApplicaSupplemento  =999;
            KmSuppl = 0;
//<<<    if  Tratta.Km > 0    // E queste sono le penalit… associate ai treni
         } /* endif */

         LastCambioStessaStazione = FALSE;

//<<< if  Tratta.IdMezzoVirtuale == 0
      } /* endif */

      if(i > 0){ // Queste sono penalit… associate ai cambi
         // Adesso aggiungo l' eventuale costo dovuto al (ridotto) numero di collegamenti della stazione di cambio
         WORD Nc = NODO_CAMBIO::NodiDiCambio[Interno(Tratta.IdStazioneIn)].NumeroCollegamenti;
         if(Nc)IndiceConnes += 32000 / Nc;
      } /* endif */

      OraLastArrivo = Tratte[i].OraOut;

//<<< for  int i = 0; i < NumeroTratte ; i++
   } /* endfor */

   if(ApplicaSupplemento && ApplicaSupplemento != 999 ){
      if( KmSuppl < MM_CR.Lim1PenSuppl ){
         if( ApplicaSupplemento >= 2 && KmSuppl < MM_CR.Lim2PenSuppl ){
            Top(PenalizzSuppl, MM_CR.NOQUAL + MM_CR.NOQUAL );
         } else  {
            Top(PenalizzSuppl, MM_CR.NOQUAL );
         }
      }
   }

   // Considero le penalizzazioni nel costo complessivo
   Costo += max(PenalizzCambio,Penalizz2Cambio)     ;
   Costo += PenalizzNotturna   ;
   Costo += PenalizzSuppl      ;
   Costo += PenalizzUrbana     ;
   Costo.Set(IndiceConnes);

   // E nel costo assoluto
   CostoAssoluto += PenalizzSuppl;
   CostoAssoluto += PenalizzUrbana;
   CostoAssoluto.Set(IndiceConnes);
   // Inibito penalizzazione cambio: in realt… crea problemi perchŠ favorisce soluzioni,
   // sullo stesso percorso, in cui prendo una coincidenza con un treno che parte dopo.
   // Ma ci• Š illogico perchŠ se perdo il primo treno posso sempre prendere il secondo.
   // CostoAssoluto += Penalizz2Cambio;

   // Gestione dei mezzi virtuali su cui voglio un' attenzione particolare
   InAttenzione = TRUE;
   #ifdef MV_IN_ATTENZIONE
   InAttenzione = FALSE;
   static ARRAY_ID MvFiltro;
   static BOOL AllMV=FALSE;
   if(MvFiltro.Dim() == 0){
      STRINGA MVA(getenv("MV_IN_ATTENZIONE"));
      if (MVA == "*") {
         AllMV = TRUE;
      } else {
         ELENCO_S MVV = MVA.Tokens(" ,;");
         FORALL(MVV,i)if(MVV[i].ToInt()) MvFiltro += MVV[i].ToInt();
      } /* endif */
      if(MvFiltro.Dim() == 0) MvFiltro += 65000; // Non valido
   }
   if(!AllMV) for (i = 0; InAttenzione && i < NumeroTratte ; i++ ) {  // Per tutte le tratte
      TRATTA_TRENO & Tratta = Tratte[i];
      if ( Tratta.IdMezzoVirtuale  == 0)continue; // Non vale
      FORALL(MvFiltro,j)if ( Tratta.IdMezzoVirtuale == MvFiltro[j] ) break;
      InAttenzione = j >= 0;
   }
   #endif

   #ifdef DBG25B
   if( InAttenzione ){
      STRINGA Msg=IdSol();
      ERRSTRING( Msg + STRINGA(VRS(Costo)) + VRS(CostoAssoluto) + VRS(NumeroTratte ) + VRS( Km) + VRS(TempoTotaleDiPercorrenza) );
      ERRSTRING( "            " VRS(PenalizzCambio) + VRS(Penalizz2Cambio) + VRS(PenalizzNotturna) + VRS(PenalizzSuppl) + VRS(PenalizzUrbana) );
   }
   #endif

   if( IsTargetAutoChk ){
      if (!DATI_ORARIO_FS::Target.NumSostituzioni){
         if( IsTargetAutoChk == 1){
            Trace(" Trovata soluzione TARGET :",1);
         } else {
            Trace(" Trovata soluzione ANALOGA al TARGET :",1);
         } /* endif */
      } else {
         Trace(" Soluzione TARGET sostituita da :",1);
      } /* endif */
      STRINGA Msg=IdSol();
      ERRSTRING( Msg );
      ERRSTRING( VRS(Costo) + VRS(CostoAssoluto) + VRS(NumeroTratte ));
      ERRSTRING(+ VRS( Km) + VRS(TempoTotaleDiPercorrenza) );
      ERRSTRING( VRS(PenalizzCambio) + VRS(Penalizz2Cambio) );
      ERRSTRING( VRS(PenalizzNotturna) + VRS(PenalizzSuppl) );
      ERRSTRING( VRS(PenalizzUrbana) );
   }
//<<< void __fastcall SOLUZIONE::CalcolaCosto
};
