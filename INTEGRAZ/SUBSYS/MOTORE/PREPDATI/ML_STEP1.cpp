//----------------------------------------------------------------------------
// ML_STEP1.CPP: Ricondizionamento distanze in base al grafo
//----------------------------------------------------------------------------
// Modifica i dati delle fermate dei treni
//----------------------------------------------------------------------------
/*

   Il programma prende i dati di base delle fermate e dei treni (files
   .TM0), come forniti dal cliente e pre-controllati dalle routines di
   caricamento, e li modifica nel seguente modo:

   - Controlla ulteriori condizioni (es: le fermate nazionali e
   cumulative debbono essere definite sul grafo) e scarta le fermate che
   non le soddisfano.

   - Elimina le "code" iniziali e finali (cioe' le fermate in sola
   partenza ed i transiti prima della prima fermata in partenza e
   viceversa per la fine).

   - Riaggiusta i progressivi chilometrici delle fermate (sulle tratte
   nazionali) in modo da renderli compatibili con il grafo.

   I dati "filtrati" vengono scaricati come sono (sui files con
   estensione .TM1 ) per essere utilizzati dai programmi di caricamento
   dati su Host.

   Viene contemporaneamente fatta una prima "linearizzazione" dei mezzi
   virtuali che elimina i "loops" (= Fermate duplicate).

   In pratica i mezzi virtuali con fermate duplicate sono splittati in
   due, secondo il seguente schema:

        1           2          3     7
        xÄÄÄÄÄÄÄÄÄÄÄxÄÄÄÄÄÄÄÄÄÄxÄÄÄÄÄxÄÄ
                               ³
               MV1           4ÚxÄ¿
                              xÄÄx
                              5  6


    1           2          3     7        3     7
    xÄÄÄÄÄÄÄÄÄÄÄxÄÄÄÄÄÄÄÄÄÄx  ÚÄÄxÄÄ  +   xÄÄÄÄÄxÄÄ
                           ³  ³           ³
           MV1           4Úx  ³          4xÄ¿  MV2
                          xÄÄxÙ          xÄÄx
                          5  6           5  6


   Come si vede l' insieme dei due mezzi virtuali permette tutte le
   possibili combinazioni O/D possibili nel primo caso non linearizzato.

   I mezzi linearizzati (con fermate, periodicita' eccetera) sono
   copiati sui files con estensione .TMP

   NB: Treni e fermate originali debbono ovviamente essere
   appropriatamente sortati, come per tutti i files BS.

*/
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

// EMS001 Win
typedef unsigned long BOOL;

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "ML_IN.HPP"
#include "seq_proc.hpp"

#define PGM      "ML_STEP1"

struct FER : public FERMATE_VIRT {
   BYTE Situazione; // 0 = Normale
   // 1 = Duplicata 1ø Occorrenza
   // 2 = Duplicata 2ø Occorrenza
   // 3 = Ignorare
   void operator = ( const FERMATE_VIRT & From) { memmove(this, &From, sizeof(FERMATE_VIRT));};
   void operator = ( const FER          & From) { memmove(this, &From, sizeof(FER));};
};

int NumMVScartati;

struct FERMS: public ARRAY_DINAMICA<FER> {

   FERMS(DWORD Size) : ARRAY_DINAMICA<FER>(Size) {};

   void Normalizza();  // Identifica fermate duplicate, Elimina fermate e transiti iniziali e finali inutili

};


// Questa e' una classetta di appoggio che mi serve per poter frazionare il programma
// senza dover passare un milione di parametri globali (che essendo files necessitano di inizializzazione).
struct WORK {

   WORK();

   // Questi sono i files originali
   F_MEZZO_VIRTUALE             TabTv0;
   F_FERMATE_VIRT               TabFv0;
   F_PERIODICITA_FERMATA_VIRT   PerFv0;
   F_INFO_AUX                   Aux0  ;

   // Questa e' la versione filtrata
   F_MEZZO_VIRTUALE             TabTv1;
   F_FERMATE_VIRT               TabFv1;
   F_PERIODICITA_FERMATA_VIRT   PerFv1;
   F_INFO_AUX                   Aux1  ;

   // Questa e' la versione filtrata e linearizzata
   F_MEZZO_VIRTUALE             TabTv2;
   F_FERMATE_VIRT               TabFv2;
   F_PERIODICITA_FERMATA_VIRT   PerFv2;
   F_INFO_AUX                   Aux2  ;

   MEZZO_VIRTUALE               Mv;                             // Questo e' il mezzo virtuale cui appartengono le fermate
   FERMS                        Fermate;                        // Queste sono le fermate
   ARRAY_DINAMICA<PERIODICITA_FERMATA_VIRT> PeriodicitaFermate; // Se le fermate sono periodiche
   ARRAY_DINAMICA<INFO_AUX>     Aux    ;                        // Area di appoggio
   int                          NextVirtuale;                   // Questo per definire i mezzi virtuali fittizi
   BOOL                         Duplicate   ;                   // Vero se il mezzo virtuale ha fermate duplicate
   ELENCO_S                     MezziDuplicati;

   // Questi per evitare segnalazioni multiple
   ARRAY_ID StazioniScartate;
   ARRAY_ID StazioniNulle;
   ARRAY_ID TransitiScartati;


   // Questa routine carica i dati di un mezzo virtuale: torna FALSE se non ci riesce
   BOOL CaricaVirtuale();

   // Questa routine effettua il prefiltraggio delle fermate
   // di un singolo mezzo virtuale in base a qualche condizione
   // e la normalizzazione delle stesse
   // Inoltre ricalcola i Km per le fermate nazionali in base al grafo
   void  Prefilter();

   // Questa routine scarica la versione NON linearizzata del mezzo virtuale
   BOOL Scarica();

   // Questa routine scarica la versione linearizzata del mezzo virtuale
   void ScaricaLinearizzato();

   // Queste sono le operazioni conclusive
   void Terminate();

//<<< struct WORK
};


//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int  main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"


   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   // EMS002 Win SetStatoProcesso(PGM, SP_PARTITO);
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari

   // EMS002 Win SetPriorita(); // Imposta la priorita'

   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore

   TryTime(0);

   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);

   // APRO il grafo
   PROFILER::Clear(FALSE); // Per aprire il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);

   // Apro l' archivio stazioni
   STAZIONI Stazioni(PATH_DATI,"ID_STAZI.DB",640000); // Lo metto tutto in memoria

   WORK Work; // Oggetto di lavoro

   // ---------------------------------------------------------
   // Scansione
   // ---------------------------------------------------------
   Bprintf("Scansione Fermate");
   while (Work.CaricaVirtuale()) {
      TryTime(Work.Mv.MezzoVirtuale);
      Work.Prefilter();
      BOOL Ok = Work.Scarica();  // Torna FALSE se non ha due fermate
      if(Ok)Work.ScaricaLinearizzato();
   } /* endwhile */

   Work.Terminate();

   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------

   TRACESTRING("PROGRAMMA TERMINATO");

   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;

   // fclose(Out);

   return 0;
//<<< int  main int argc,char *argv
}
//----------------------------------------------------------------------------
// @WORK
//----------------------------------------------------------------------------
WORK::WORK() :
TabTv0(PATH_OUT "M0_TRENV.TM0"),
TabTv1(PATH_OUT "M0_TRENV.TM1"),
TabTv2(PATH_OUT "M0_TRENV.TMP"),
TabFv0(PATH_OUT "M0_FERMV.TM0"),
TabFv1(PATH_OUT "M0_FERMV.TM1"),
TabFv2(PATH_OUT "M0_FERMV.TMP"),
PerFv0(PATH_OUT "M1_FERMV.TM0"),
PerFv1(PATH_OUT "M1_FERMV.TM1"),
PerFv2(PATH_OUT "M1_FERMV.TMP"),
Aux0(PATH_OUT "INFO_AUX.TM0" ),
Aux1(PATH_OUT "INFO_AUX.TM1" ),
Aux2(PATH_OUT "INFO_AUX.TMP" ),
Fermate(500),
Aux(200),
PeriodicitaFermate(500)
{
   #undef TRCRTN
   #define TRCRTN "@WORK"

   TabTv1.Clear(TabTv0.DescrizioneSuFile());
   TabTv2.Clear(TabTv0.DescrizioneSuFile());
   TabFv1.Clear(TabFv0.DescrizioneSuFile());
   TabFv2.Clear(TabFv0.DescrizioneSuFile());
   PerFv1.Clear(PerFv0.DescrizioneSuFile());
   PerFv2.Clear(PerFv0.DescrizioneSuFile());
   Aux1.Clear(Aux0.DescrizioneSuFile());
   Aux2.Clear(Aux0.DescrizioneSuFile());

   // Ora vedo quale sia il minimo ID che posso utilizzare per i virtuali fittizi
   TabTv0.FixRec(TabTv0.Dim() - 1);
   NextVirtuale=TabTv0.RecordCorrente().MezzoVirtuale;
   NextVirtuale += 500; // Lascio un poco di margine che non fa male
   NextVirtuale = NextVirtuale - (NextVirtuale% 1000) + 1000; // Prossimo multiplo di 1000
   TabTv0.FixRec(0); // Riposiziono all' inizio
};
//----------------------------------------------------------------------------
// WORK::CaricaVirtuale
//----------------------------------------------------------------------------
// Questa routine carica i dati di un mezzo virtuale: torna FALSE se non ci riesce (fine file)
BOOL WORK::CaricaVirtuale(){
   #undef TRCRTN
   #define TRCRTN "WORK::CaricaVirtuale"
   Fermate.Clear();
   PeriodicitaFermate.Clear();
   Aux.Clear();
   Duplicate    = FALSE;                   // Vero se il mezzo virtuale ha fermate duplicate: Viene valorizzato da Prefilter
   if(&TabTv0.RecordCorrente() == NULL)return FALSE;
   Mv = TabTv0.RecordCorrente();
   TabTv0.Next();                       // Per la prossima volta

   // Carico le fermate
   TabFv0.Seek(Mv.MezzoVirtuale);
   while (&TabFv0.RecordCorrente() && TabFv0.RecordCorrente().MezzoVirtuale == Mv.MezzoVirtuale) {
      FER Fermata;
      Fermata = TabFv0.RecordCorrente();
      Fermata.Situazione = 0;
      Fermate += Fermata;
      TabFv0.Next();
   } /* endwhile */

   // Eventualmente carico le periodicita'
   if (Mv.PeriodicitaDisuniformi) {
      PerFv0.Seek(Mv.MezzoVirtuale);
      while (&PerFv0.RecordCorrente() && PerFv0.RecordCorrente().MezzoVirtuale == Mv.MezzoVirtuale) {
         PeriodicitaFermate += PerFv0.RecordCorrente();
         PerFv0.Next();
      } /* endwhile */
   } /* endif */

   // E le informazioni ausiliarie
   if(Aux0.Seek(Mv.MezzoVirtuale)){
      while (&Aux0.RecordCorrente() && Aux0.RecordCorrente().MezzoVirtuale == Mv.MezzoVirtuale) {
         Aux += Aux0.RecordCorrente();
         Aux0.Next();
      } /* endwhile */
      }

      return TRUE;
   };
   //----------------------------------------------------------------------------
   // WORK::Prefilter
   //----------------------------------------------------------------------------
   // Questa routine effettua il prefiltraggio delle fermate
   // di un singolo mezzo virtuale in base a qualche condizione
   // e la normalizzazione delle stesse
   // Inoltre ricalcola i Km per le fermate nazionali in base al grafo
   void  WORK::Prefilter(){
      #undef TRCRTN
      #define TRCRTN "WORK::Prefilter"

      GRAFO & Grafo = GRAFO::Gr();

      // Per prima cosa elimino le fermate ed i transiti non definiti sul grafo o senza indicazioni
      // di partenza o arrivo o transito
      ORD_FORALL(Fermate,i0){
         FER & Fermata = Fermate[i0];
         // Ignoro le stazioni NON presenti sul grafo.
         if(Grafo[Fermata.Id].Id == 0){
            if(Fermata.FermataPartenza || Fermata.FermataArrivo ){
               if(!StazioniScartate.Contiene(Fermata.Id)){
                  StazioniScartate += Fermata.Id;
                  Bprintf("Scartata fermata/e su stazione non su grafo (probabilmente rimossa perche' non vendibile) : ID = %i %s ",Fermata.Id,Stazioni[Fermata.Id].NomeStazione);
               };
               Bprintf3("Scartata fermata non Identificata Mv Nø %i Treno %10.10s ID = %i %s", Fermata.MezzoVirtuale, (CPSZ)Mv.Mv[Fermata.TrenoFisico].IdentTreno, Fermata.Id,Stazioni[Fermata.Id].NomeStazione);
            } else {
               if(!TransitiScartati.Contiene(Fermata.Id)){
                  TransitiScartati += Fermata.Id;
                  Bprintf3("Scartato transito/i su stazione non su grafo (probabilmente rimossa perche' non vendibile) : ID = %i %s ",Fermata.Id,Stazioni[Fermata.Id].NomeStazione);
               };
            } /* endif */
            Fermata.Situazione = 3;
         } else if(!(Fermata.FermataPartenza || Fermata.FermataArrivo || Fermata.Transito) ){
            if(!StazioniNulle.Contiene(Fermata.Id)){
               StazioniNulle += Fermata.Id;
               printf("Scartata fermata ne' partenza ne' arrivo ne' Transito: ID = %i %s\n",Fermata.Id,Stazioni[Fermata.Id].NomeStazione);
            };
            Bprintf3("Scartata fermata ne' partenza ne' arrivo ne' Transito: Mv Nø %i Treno %10.10s ID = %i %s", Fermata.MezzoVirtuale, (CPSZ)Mv.Mv[Fermata.TrenoFisico].IdentTreno, Fermata.Id,Stazioni[Fermata.Id].NomeStazione);
            Fermata.Situazione = 3;
//<<<    if Grafo Fermata.Id .Id == 0
         }
//<<< ORD_FORALL Fermate,i0
      }

      //// Poi vedo se per caso vi sono mezzi fisici duplicati: se si cancello le occorrenze dopo la prima
      //ELENCO_S MezziFisici;
      //for (int J = 0; J < Mv.NumMezziComponenti; J++ ) {
      //   STRINGA Ident = St(Mv.Mv[J].IdentTreno);
      //   if (MezziFisici.Contiene(Ident)) {
      //      Bprintf("Scarto fermate su treno fisico duplicato Mv Nø %i Treno %10.10s", Mv.MezzoVirtuale, (CPSZ)Ident);
      //      FORALL(Fermate, i){
      //         FER & Fermata = Fermate[i];
      //         if(Fermata.TrenoFisico == J)Fermata.Situazione = 3; // Disabilito la fermata
      //      }
      //   } /* endif */
      //   MezziFisici += Ident;
      //} /* endfor */

      // Ignoro le fermate all' inizio del treno fino alla prima fermata che permette la salita.
      ORD_FORALL(Fermate, i1){
         FER & Fermata = Fermate[i1];
         if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
         if(Fermata.FermataPartenza) break   ; // Ok prima fermata valida in partenza
         Bprintf3("Scartata stazione non partenza all' inizio del treno: Mv Nø %i Treno %10.10s ID = %i %s", Fermata.MezzoVirtuale, (CPSZ)Mv.Mv[Fermata.TrenoFisico].IdentTreno, Fermata.Id,Stazioni[Fermata.Id].NomeStazione);
         Fermata.Situazione               = 3; // Disabilito la fermata
      }

      // Poi ignoro le fermate alla fine del treno fino all' ultima fermata che permette la discesa
      FORALL(Fermate, i2){
         FER & Fermata = Fermate[i2];
         if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
         if(Fermata.FermataArrivo) break     ; // Ok ultima fermata valida in arrivo
         Bprintf3("Scartata stazione non arrivo alla fine del treno: Mv Nø %i Treno %10.10s ID = %i %s", Fermata.MezzoVirtuale, (CPSZ)Mv.Mv[Fermata.TrenoFisico].IdentTreno, Fermata.Id,Stazioni[Fermata.Id].NomeStazione);
         Fermata.Situazione               = 3; // Disabilito la fermata
      }

      // Se vi sono fermate duplicate "degeneri" (= 2 fermate di seguito eguali) le accorpo in una sola fermata
      // Capita spesso a causa dello scarto di fermate o transiti non validi
      FER * LastFermata = NULL;
      ORD_FORALL(Fermate, i10){
         FER & Fermata = Fermate[i10];
         if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
         if(LastFermata != NULL && Fermata.Id == LastFermata->Id ){
            if(LastFermata->FermataArrivo){ // La precedente e' valida
               if(Fermata.FermataPartenza){ // Ora di arrivo della precedente, e partenza dell' attuale
                  LastFermata->FermataPartenza = 1;
                  LastFermata->OraPartenza = Fermata.OraPartenza ;
               }
               Fermata.Situazione               = 3; // Disabilito la fermata
               LastFermata = NULL;
            } else if(Fermata.FermataPartenza){ // Annullo la precedente perche' l' attuale e' meglio
               LastFermata->Situazione  = 3;
               LastFermata = NULL;
            } else if(LastFermata->FermataPartenza){ // Annullo la corrente
               Fermata.Situazione               = 3; // Disabilito la fermata
               LastFermata = NULL;
            }
            Bprintf3("Scartata stazione duplicata 'degenere' : Mv Nø %i Treno %10.10s ID = %i %s", Fermata.MezzoVirtuale, (CPSZ)Mv.Mv[Fermata.TrenoFisico].IdentTreno, Fermata.Id,Stazioni[Fermata.Id].NomeStazione);
         } else {
            LastFermata = &Fermata;
         }
//<<< ORD_FORALL Fermate, i10
      }

      // Poi identifico le fermate duplicate if any
      IdDuplicato(0,TRUE); // Reset controllo id duplicati (Vedere base.hpp)
      ORD_FORALL(Fermate,i3){
         FER & Fermata = Fermate[i3];
         if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
         if(IdDuplicato(Fermata.Id)){
            Fermata.Situazione = 2;
            if(Fermata.FermataPartenza || Fermata.FermataArrivo)Duplicate = TRUE; // Non setto per i soli transiti
         }
      }
      if(Duplicate){
         IdDuplicato(0,TRUE); // Reset controllo id duplicati (Vedere base.hpp)
         FORALL(Fermate,i31){
            FER & Fermata = Fermate[i31];
            if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
            if(IdDuplicato(Fermata.Id)) Fermata.Situazione = 1;
         }
      }


      // Infine ricalcolo i KM per le sole fermate valide
      BOOL TrovataPrimaValida = FALSE;
      Grafo.AbilitaCumulativo = TRUE;
      ORD_FORALL(Fermate,i4){
         FER & Fermata = Fermate[i4];
         if (TrovataPrimaValida) {
            if (Fermata.Situazione == 3) { // Ignoro la fermata anomala
               Fermata.ProgKm  = LastFermata->ProgKm;
            } else {
               DIST Km = Grafo.DistanzaTra(LastFermata->Id, Fermata.Id);
               if (Km == BigDIST) {  // La fermata NON e' COMUNQUE valida
                  Bprintf("Scartata fermata ANOMALA con ID = %i Investigare",Fermata.Id);
                  Fermata.Situazione = 3;
                  Fermata.ProgKm  = LastFermata->ProgKm;
               } else if(Km == 0) { // Non ammetto due stazioni con la stessa distanza
                  Fermata.ProgKm =  LastFermata->ProgKm + 1;
                  LastFermata = &Fermata;
               } else {
                  Fermata.ProgKm =  LastFermata->ProgKm + Km;
                  LastFermata = &Fermata;
               } /* endif */
            } /* endif */
         } else {
            Fermata.ProgKm  = 0;
            if (Fermata.Situazione != 3) { // Ignoro la fermata anomala
               TrovataPrimaValida = TRUE;
               LastFermata = &Fermata;
            }
//<<<    if  TrovataPrimaValida
         }
//<<< ORD_FORALL Fermate,i4
      }
//<<< void  WORK::Prefilter
   };


   //----------------------------------------------------------------------------
   // WORK::Scarica
   //----------------------------------------------------------------------------
   // Questa routine scarica la versione NON linearizzata del mezzo virtuale
   BOOL WORK::Scarica(){
      #undef TRCRTN
      #define TRCRTN "WORK::Scarica"


      assert((!Mv.PeriodicitaDisuniformi) || Fermate.Dim() == PeriodicitaFermate.Dim());

      // Scarico su M0_FERMV  tutte le fermate valide
      // Eventualmente carico le periodicita'
      int NumFermTransiti = 0; // Fermate e transiti
      int NumFerm         = 0; // Solo fermate
      // Prima Conto
      ORD_FORALL(Fermate,i0){
         FER & Fermata = Fermate[i0];
         if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
         NumFermTransiti ++;
         if(Fermata.FermataPartenza || Fermata.FermataArrivo )NumFerm ++;
      }
      if(NumFerm < 2){
         Bprintf2("Scartato il mezzo virtuale %i perche' non ha due fermate valide",Mv.MezzoVirtuale);
         NumMVScartati ++;
         return FALSE; // Non valido
      }
      // Aggiungo il mezzo virtuale
      Mv.NumeroFermateTransiti = Chk(NumFermTransiti,10) ;
      Mv.NumeroFermateValide   = Chk(NumFerm ,10) ;
      TabTv1.AddRecordToEnd(VRB(Mv));

      // E le fermate
      ORD_FORALL(Fermate,i){
         FER & Fermata = Fermate[i];
         if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
         TabFv1.AddRecordToEnd(&Fermata, sizeof(FERMATE_VIRT));
         if (Mv.PeriodicitaDisuniformi) PerFv1.AddRecordToEnd(VRB(PeriodicitaFermate[i]));
      }

      // E le informazioni ausiliarie
      ORD_FORALL(Aux,a){
         Aux1.AddRecordToEnd(&Aux[a], sizeof(INFO_AUX));
      }

      return TRUE;

//<<< BOOL WORK::Scarica
   }

   //----------------------------------------------------------------------------
   // WORK::ScaricaLinearizzato
   //----------------------------------------------------------------------------
   // Questa routine scarica la versione linearizzata del mezzo virtuale
   void WORK::ScaricaLinearizzato(){
      #undef TRCRTN
      #define TRCRTN "WORK::ScaricaLinearizzato"

      assert((!Mv.PeriodicitaDisuniformi) || Fermate.Dim() == PeriodicitaFermate.Dim());

      // Scarico su M0_FERMV  tutte le fermate NON DUPLICATE (o la prima occorrenza)
      // Contemporaneamente conto il numero delle fermate e dei transiti
      // Si noti che per scartare eventuali transiti finali (seguiti da una fermata duplicata) scrivo le
      // stazioni al contrario: poi faro' un sort
      // Eventualmente carico le periodicita'
      int NumFermTransiti = 0; // Fermate e transiti
      int NumFerm         = 0; // Solo fermate
      BOOL HoPassatoLaUltimaDuplicata = FALSE;
      FORALL(Fermate,i){
         FER & Fermata = Fermate[i];
         if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
         if(Fermata.Situazione == 2) continue; // Ignoro le duplicate
         if(Fermata.FermataPartenza || Fermata.FermataArrivo ){
            NumFerm ++;
            HoPassatoLaUltimaDuplicata = TRUE;
         } else if(!HoPassatoLaUltimaDuplicata)continue; // Altrimenti finirei con un transito
         NumFermTransiti ++;
         TabFv2.AddRecordToEnd(&Fermata, sizeof(FERMATE_VIRT));
         if (Mv.PeriodicitaDisuniformi) PerFv2.AddRecordToEnd(VRB(PeriodicitaFermate[i]));
      }

      // Aggiungo il mezzo virtuale
      Mv.NumeroFermateTransiti = Chk(NumFermTransiti,10) ;
      Mv.NumeroFermateValide   = Chk(NumFerm ,10) ;
      TabTv2.AddRecordToEnd(VRB(Mv));

      // E le informazioni ausiliarie
      ORD_FORALL(Aux,a){
         Aux2.AddRecordToEnd(&Aux[a], sizeof(INFO_AUX));
      }

      // Se ho fermate duplicate (no solo transiti):
      if(Duplicate){
         char Buf[500];
         sprintf(Buf,"Mezzo virtuale %i ha fermate duplicate: viene splittato con mezzo virtuale %i",Mv.MezzoVirtuale, NextVirtuale);
         // Bprintf2("%s",Buf);
         MezziDuplicati += Buf;
         int OffsetKm = -1;

         // Scarico su M0_FERM2  tutte le fermate del secondo mezzo
         // - Tutte le fermate Normali dopo la prima fermata duplicata
         // - Tutte le seconde occorrenze
         // Contemporaneamente conto il numero delle fermate e dei transiti
         int NumFermTransiti = 0; // Fermate e transiti
         int NumFerm         = 0; // Solo fermate
         int HoPassatoLaPrimaDuplicata = 0;
         ORD_FORALL(Fermate,i){
            FER Fermata = Fermate[i]; // Lavoro per copia per poter modificare l' ID del mezzo virtuale
            if(Fermata.Situazione == 3) continue; // Ignoro le fermate non valide
            if(Fermata.Situazione == 1){
               if(Fermata.FermataPartenza || Fermata.FermataArrivo )Top(HoPassatoLaPrimaDuplicata,1);
               continue; // Ignoro le duplicate
            }
            if( !HoPassatoLaPrimaDuplicata && Fermata.Situazione == 0) continue; // E le fermate prima della prima duplicata
            if(Fermata.FermataPartenza ){
               if(OffsetKm <0)OffsetKm = Fermata.ProgKm;
               NumFerm ++;
               HoPassatoLaPrimaDuplicata = 2;
            } else if(Fermata.FermataArrivo ){
               if(HoPassatoLaPrimaDuplicata<2)continue; // Non posso partire con una fermata di solo arrivo
               NumFerm ++;
               HoPassatoLaPrimaDuplicata = 2;
            } else if(HoPassatoLaPrimaDuplicata<2)continue; // Altrimenti partirei con un transito
            NumFermTransiti ++;
            Fermata.MezzoVirtuale = NextVirtuale;
            Fermata.ProgKm -= OffsetKm;
            TabFv2.AddRecordToEnd(&Fermata, sizeof(FERMATE_VIRT));
            if (Mv.PeriodicitaDisuniformi){
               PERIODICITA_FERMATA_VIRT PerF = PeriodicitaFermate[i]; // Lavoro per copia per poter modificare l' ID del mezzo virtuale
               PerF.MezzoVirtuale = NextVirtuale;
               PerFv2.AddRecordToEnd(VRB(PerF));
            }
//<<<    ORD_FORALL Fermate,i
         }

         // Creo un nuovo mezzo virtuale
         // NB: Alcuni dei mezzi componenti potrebbero non avere fermate: E' OK
         MEZZO_VIRTUALE MvDuplicato  = Mv;
         MvDuplicato.MezzoVirtuale = NextVirtuale;
         MvDuplicato.FittizioLinearizzazione = 1;

         // Aggiungo il mezzo virtuale
         MvDuplicato.NumeroFermateTransiti = Chk(NumFermTransiti,10) ;
         MvDuplicato.NumeroFermateValide   = Chk(NumFerm ,10) ;
         TabTv2.AddRecordToEnd(VRB(MvDuplicato));

         // E le informazioni ausiliarie
         ORD_FORALL(Aux,a){
            INFO_AUX AuxBis = Aux[a];
            AuxBis.MezzoVirtuale = NextVirtuale;
            Aux2.AddRecordToEnd(VRB(AuxBis));
         }

         // Aggiorno NextVirtuale
         NextVirtuale ++;

//<<< if Duplicate
      }
//<<< void WORK::ScaricaLinearizzato
   }
   // Queste sono le operazioni conclusive
   void WORK::Terminate(){
      // ---------------------------------------------------------
      // Sort finale
      // ---------------------------------------------------------
      Bprintf("Sort Finale");
      TabTv1.ReSortFAST();
      TabFv1.ReSortFAST();
      PerFv1.ReSortFAST();
      TabTv2.ReSortFAST();
      TabFv2.ReSortFAST();
      PerFv2.ReSortFAST();
      Aux1.ReSortFAST();
      Aux2.ReSortFAST();

      // Scrivo in modo leggibile alla fine i MV su cui ho avuto split
      if (MezziDuplicati.Dim()){
         Bprintf2("============================================================================================");
         Bprintf2("    Elenco dei Mezzi virtuali splittati per linearizzazione: ");
         Bprintf2("============================================================================================");
         ORD_FORALL( MezziDuplicati , i ) Bprintf2("%s",(CPSZ)MezziDuplicati[i]);
         Bprintf("Splittati %i Mezzi virtuali duplicati per linearizzazione",MezziDuplicati.Dim());
      }
      Bprintf("Scartati ulteriori %i Mezzi virtuali perchŠ non hanno almeno due fermate valide", NumMVScartati);

      Bprintf2("TabTv0 : %i Records",TabTv0.Dim());
      Bprintf2("TabFv0 : %i Records",TabFv0.Dim());
      Bprintf2("PerFv0 : %i Records",PerFv0.Dim());
      Bprintf2("Aux0   : %i Records",Aux0  .Dim());
      Bprintf2("TabTv1 : %i Records",TabTv1.Dim());
      Bprintf2("TabFv1 : %i Records",TabFv1.Dim());
      Bprintf2("PerFv1 : %i Records",PerFv1.Dim());
      Bprintf2("Aux1   : %i Records",Aux1  .Dim());
      Bprintf2("TabTv2 : %i Records",TabTv2.Dim());
      Bprintf2("TabFv2 : %i Records",TabFv2.Dim());
      Bprintf2("PerFv2 : %i Records",PerFv2.Dim());
      Bprintf2("Aux2   : %i Records",Aux2  .Dim());

//<<< void WORK::Terminate
   };
