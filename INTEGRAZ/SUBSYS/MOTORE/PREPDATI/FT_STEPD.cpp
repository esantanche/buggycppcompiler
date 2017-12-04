//----------------------------------------------------------------------------
// FT_STEPD.CPP: Caricamento dati delle note e dei servizi
//----------------------------------------------------------------------------
// Carica i formati intermedi
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4
// #pragma option -Od

// EMS001 Win
typedef unsigned long BOOL;

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_IN.HPP"
#include "seq_proc.hpp"
#include "file_t.hpp"

#define PGM      "FT_STEPD"

//----------------------------------------------------------------------------
// Opzioni di debug
//----------------------------------------------------------------------------
//#define DBG1      // Questo mi indica di fare un dump di TUTTO cio' che carico
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Classi di lavoro
//----------------------------------------------------------------------------
struct WORK {

   typedef ARRAY_DINAMICA<INFO_AUX> APPO;

   // Costruttore
   WORK(STRINGA & PathDa , STRINGA & ExtFl) :
   TabOr(PathDa + "TABORARI" + ExtFl)       ,
   Loc(PathDa + "LOCALITA" + ExtFl)         ,
   NoteCommerciali()                        ,
   ServNu()                                 ,
   TabTv(PATH_OUT "M0_TRENV.TM0",2000)      ,
   TabFv(PATH_OUT "M0_FERMV.TM0",2000)      ,
   DatiAusiliari(PATH_OUT "INFO_AUX.TM0")
   {
      TabTv.PermettiScrittura(); // Per scrivere l' informazione dei servizi
      DatiAusiliari.Clear("Dati ausiliari di mezzo virtuale");
   } ;

   // Files
   F_MEZZO_VIRTUALE    TabTv           ; // Mezzi virtuali
   F_FERMATE_VIRT      TabFv           ; // Fermate dei Treni Virtuali
   FILE_TABORARI       TabOr           ; // File Originale Fermate
   FILE_LOCALITA       Loc             ; // File Localit…
   FILE_NOTA_CO        NoteCommerciali ; // Note COmmerciali
   FILE_SERVNU         ServNu          ; // Sevizi non uniformi
   F_INFO_AUX          DatiAusiliari   ; // Dati di out dei servizi ausiliari

   void GestioneNome( MEZZO_VIRTUALE & MvLe , APPO & DatiAux);
   void GestioneClassifica( MEZZO_VIRTUALE & MvLe , APPO & DatiAux);
   void GestioneNoteDiTrenoEFermata( MEZZO_VIRTUALE & MvLe , APPO & DatiAux );
   void GestioneNoteCommerciali( MEZZO_VIRTUALE & MvLe, APPO & DatiAux);
   void GestioneServizi( MEZZO_VIRTUALE & MvLe , APPO & DatiAux);
   BOOL DeterminaEstensioneAux(INFO_AUX & Info, MEZZO_VIRTUALE & MvLe , int NVg); // FALSE se vi sono problemi
   int  ScaricaTestoNote();
   int  ScaricaMezziViaggianti();
   int Riorganizza(APPO & Appo);

   int  Body(int MinMezzo, int MaxMezzo);
//<<< struct WORK
};

//----------------------------------------------------------------------------
// Funzioncine utili
//----------------------------------------------------------------------------
int MinDaMvg(BYTE Mvg){ for(int i = 0; i < 8; i++) if(Mvg & (1 << i)) return i; return 0;};
int MaxDaMvg(BYTE Mvg){ for(int i = 7; i > 0; i--) if(Mvg & (1 << i)) return i; return 0;};

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int  main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"

   int Rc = 0;

   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);

   // Permetto, a fini di trace, di indicare il minimo e massimo mezzo virtuale
   int MinMezzo, MaxMezzo;
   MinMezzo = 0;
   MaxMezzo = 9999999;
   for (int a = 1;a < argc ; a ++ ) {
      STRINGA Tmp(argv[a]);
      Tmp.UpCase();
      if(Tmp[0] == '/' ){
      } else if(MinMezzo == 0){
         MaxMezzo = MinMezzo = Tmp.ToInt();
      } else {
         MaxMezzo = Tmp.ToInt();
      }
   } /* endfor */
   if(MinMezzo != 0 || MaxMezzo != 9999999) Bprintf("Esecuzione limitata ai mezzi virtuali da %i a %i",MinMezzo, MaxMezzo);

   // EMS002 Win SetStatoProcesso(PGM, SP_PARTITO);
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
   // EMS002 Win SetPriorita(); // Imposta la priorita'
   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore

   TryTime(0);

   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");

   // Apro l' archivio decodifica codici CCR
   CCR_ID Ccr_Id(PATH_DATI);
   CCR_ID::CcrId = & Ccr_Id;

   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);

   // APRO il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);

   // Ottengo i dati dei treni
   PTRENO::Restore(); // Parto dai dati caricati l' ultima volta

   STRINGA PathDa(PATH_IN),ExtFl(".T");
   if (getenv("USADATIESTRATTI")){ PathDa=PATH_OUT;ExtFl=".XTR";};

   WORK Work(PathDa,ExtFl);

   // Scarico del file delle note
   Rc = Work.ScaricaTestoNote();

   // Scarico dei mezzi viaggianti
   if(Rc == 0) Rc = Work.ScaricaMezziViaggianti();

   // Elaborazione dati ausiliari
   if(Rc == 0) Rc = Work.Body(MinMezzo, MaxMezzo);


   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------

   TRACESTRING("PROGRAMMA TERMINATO 152");
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;

   return Rc;
//<<< int  main int argc,char *argv
};
//----------------------------------------------------------------------------
// WORK::Body
//----------------------------------------------------------------------------
int WORK::Body(int MinMezzo, int MaxMezzo){
   #undef TRCRTN
   #define TRCRTN "WORK::Body"
   int NumOut  = 0;
   int NumMv   = 0;
   int NumNot  = 0;
   int NumSrv  = 0;
   int NumNom  = 0;
   int NumCla  = 0;

   APPO DatiAux(250); // Tutti i dati di appoggio

   // Ora scandisco i mezzi virtuali, e carico i dati per ogni mezzo virtuale e tipologia
   ORD_FORALL(TabTv,NMv){
      TryTime(NMv);
      MEZZO_VIRTUALE & MvLe = TabTv.FixRec(NMv);

      DatiAux.Clear();

      if(MvLe.MezzoVirtuale < MinMezzo ||  MvLe.MezzoVirtuale > MaxMezzo ) continue;


      GestioneNome( MvLe , DatiAux);

      GestioneClassifica(MvLe , DatiAux);

      GestioneNoteDiTrenoEFermata( MvLe , DatiAux);

      GestioneNoteCommerciali( MvLe , DatiAux);

      GestioneServizi( MvLe , DatiAux);

      TabTv.ModifyCurrentRecord(); // Ho modificato Servizi Uniformi

      NumOut += DatiAux.Dim()   ;  // Prima della riorganizzazione
      Riorganizza(DatiAux);

      if(DatiAux.Dim()){
         NumMv   ++;
         FORALL(DatiAux,i){
            switch (DatiAux[i].Tipo) {
            case AUX_NOME:       NumNom ++; break;
            case AUX_CLASSIFICA: NumCla ++; break;
            case AUX_NOTA:       NumNot ++; break;
            case AUX_SERVIZIO:   NumSrv ++; break;
            } /* endswitch */
         }
      }

      // Scarico dei dati su File
      ORD_FORALL(DatiAux,Aux)DatiAusiliari.AddRecordToEnd(VRB(DatiAux[Aux]));
//<<< ORD_FORALL TabTv,NMv
   };

   DatiAusiliari.ReSortFAST();

   Bprintf("Generati %i records di informazioni ausiliarie, da compattare",NumOut);
   Bprintf("Compattati in %i records finali",DatiAusiliari.Dim());
   Bprintf("Numero dei mezzi virtuali con dati ausiliari: %i",NumMv);
   Bprintf("Numero dei dati di tipo \"Nome\"            : %i",NumNom);
   Bprintf("Numero dei dati di tipo \"Classifica\"      : %i",NumCla);
   Bprintf("Numero dei dati di tipo \"Nota\"            : %i",NumNot);
   Bprintf("Numero dei dati di tipo \"Servizi\"         : %i",NumSrv);
   return 0;
//<<< int WORK::Body int MinMezzo, int MaxMezzo
};
//----------------------------------------------------------------------------
// WORK::GestioneNome
//----------------------------------------------------------------------------
void WORK::GestioneNome( MEZZO_VIRTUALE & MvLe , APPO & DatiAux ){
   #undef TRCRTN
   #define TRCRTN "WORK::GestioneNome"

   for (int NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ) if(MvLe.Mv[NVg].HaNome)break;
   if (NVg >= MvLe.NumMezziComponenti) return ; // Il mezzo virtuale NON ha nome
   TRACESTRING(VRS(MvLe.MezzoVirtuale) + " Ha Nomi");

   // Nomi dei treni
   ELENCO_S Nomi  ;
   ARRAY_DINAMICA<DWORD> Id_Nomi; // Id Corrispondenti ai nomi (0 se non ha nome)
   // ....................
   // Ciclo dei mezzi viaggianti
   // ....................
   for (NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ) {
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
      PTRENO & Treno = * PTRENO::Get(MvGi.IdentTreno);
      assert(&Treno != NULL);
      if (MvGi.HaNome) {
         STRINGA Nome = St(Treno.NomeMezzoViaggiante);
         Nome.Strip();
         assert(Nome.Dim() > 0);
         int Pos = Nomi.CercaPerNome(Nome);
         Nomi    += Nome;
         if (Pos < 0) {
            Id_Nomi += Treno.NumeroTreno ;
         } else {
            Id_Nomi += Id_Nomi[Pos];
            // Riutilizzo l' id del treno precedente: hanno lo stesso nome!
            // Cio' al fine di far risultare uniforme il tutto (se lo Š)
            TRACESTRING(VRS(NVg) + " Riutilizzo nome " VRS(Pos));
         } /* endif */
         assert(Id_Nomi.Last() > 0);
         INFO_AUX InfoNome;
         ZeroFill(InfoNome);
         InfoNome.MezzoVirtuale = MvLe.MezzoVirtuale;
         InfoNome.Tipo          = AUX_NOME          ;
         InfoNome.Id            = 0                 ;
         InfoNome.Info          = Id_Nomi[NVg]      ;
         InfoNome.Shift         = 0                 ;        // Non applicabile
         InfoNome.Periodicita   = T_PERIODICITA::InLimitiOrario ;  // Non applicabile
         InfoNome.DiFermata     = 0                 ;
         InfoNome.DiMvg         = 1                 ;
         InfoNome.Mvg           = 1 << NVg          ;        // Indica il mezzo viaggiante
         InfoNome.TipoAUX       = 0                 ;
         InfoNome.MaxDa         = 0                 ;
         if(DeterminaEstensioneAux(InfoNome, MvLe , NVg)) DatiAux += InfoNome;
         TRACESTRING("Scaricato" VRS(MvLe.MezzoVirtuale) + VRS(NVg) + VRS(Id_Nomi[NVg]) + VRS(Nome) );
//<<< if  MvGi.HaNome
      } else {
         Nomi    += "";
         Id_Nomi += 0;
      } /* endif */
//<<< for  NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++
   } /* endfor */
//<<< void WORK::GestioneNome  MEZZO_VIRTUALE & MvLe , APPO & DatiAux
};

//----------------------------------------------------------------------------
// WORK::GestioneClassifica
//----------------------------------------------------------------------------
void WORK::GestioneClassifica( MEZZO_VIRTUALE & MvLe , APPO & DatiAux ){
   #undef TRCRTN
   #define TRCRTN "WORK::GestioneClassifica"

   if(MvLe.NumMezziComponenti == 1)return; // E' per definizione uniforme
   int LastClassifica = 0;
   for (int NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ){
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
      PTRENO & Treno = * PTRENO::Get(MvGi.IdentTreno);
      if (NVg == 0) {
         LastClassifica = Treno.ServiziTreno.TipoMezzo ;
      } else if(Treno.ServiziTreno.TipoMezzo != LastClassifica){
         LastClassifica = -1;
         break;
      } /* endif */
   }
   if(LastClassifica >= 0){
      assert2(LastClassifica == MvLe.TipoMezzoDominante, MvLe.MezzoVirtuale);
      return ;  // E' di fatto uniforme
   }

   // ....................
   // Ciclo dei mezzi viaggianti
   // ....................
   for (NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ) {
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
      PTRENO & Treno = * PTRENO::Get(MvGi.IdentTreno);
      assert(&Treno != NULL);
      INFO_AUX InfoClas;
      ZeroFill(InfoClas);
      InfoClas.MezzoVirtuale = MvLe.MezzoVirtuale;
      InfoClas.Tipo          = AUX_CLASSIFICA    ;
      InfoClas.Id            = Treno.ServiziTreno.TipoMezzo ;
      InfoClas.Info          = 0                 ;
      InfoClas.Shift         = 0                 ;        // Non applicabile
      InfoClas.Periodicita   = T_PERIODICITA::InLimitiOrario ;  // Non applicabile
      InfoClas.DiFermata     = 0                 ;
      InfoClas.DiMvg         = 1                 ;
      InfoClas.Mvg           = 1 << NVg          ;        // Indica il mezzo viaggiante
      InfoClas.TipoAUX       = 0                 ;
      InfoClas.MaxDa         = 0                 ;
      if(DeterminaEstensioneAux(InfoClas, MvLe , NVg)) DatiAux += InfoClas;
   } /* endfor */
//<<< void WORK::GestioneClassifica  MEZZO_VIRTUALE & MvLe , APPO & DatiAux
};
//----------------------------------------------------------------------------
// WORK::GestioneNoteDiTrenoEFermata
//----------------------------------------------------------------------------
void WORK::GestioneNoteDiTrenoEFermata( MEZZO_VIRTUALE & MvLe , APPO & DatiAux ){
   #undef TRCRTN
   #define TRCRTN "WORK::GestioneNoteDiTrenoEFermata"

   // ....................
   // Ciclo dei mezzi viaggianti
   // ....................
   for (int NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ){
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
      PTRENO & Treno = * PTRENO::Get(MvGi.IdentTreno);
      assert(&Treno != NULL);
      if(!Treno.ServiziTreno.HaNoteGeneriche)continue; // Non ha note
      TABORARI *To = &TabOr[Treno.PTaborariR2+1];
      assert(To->TipoRecord == '3');
      ARRAY_ID NoteTreno;
      ARRAY_ID NoteFermata;
      int PLoc = Treno.PLocalita;
      DWORD CCR_Out = MvGi.CcrStazioneDiCambio ;
      DWORD CCR_In  = (NVg == 0) ? 0 : MvLe.Mv[NVg-1].CcrStazioneDiCambio ;
      BOOL OkIn  = CCR_In == 0;
      BOOL OkOut = TRUE;
      while (OkOut && To && (To->TipoRecord != '2')) {
         switch (To->TipoRecord) {   // Gestione differenti tipi record
         case '3':       // record di tipo 3
            {
               // ..............................
               // Determino le note di treno
               // ..............................
               for (int i = 0 ;i < 5 ;i++ ) {
                  int Nota = It(To->R3.NoteDiTreno[i]);
                  if(Nota && !NoteTreno.Contiene(Nota)){
                     NoteTreno += Nota;
                     INFO_AUX InfoNote;
                     ZeroFill(InfoNote);
                     InfoNote.MezzoVirtuale = MvLe.MezzoVirtuale;
                     InfoNote.Tipo          = AUX_NOTA          ;
                     InfoNote.Id            = Nota              ;
                     InfoNote.Info          = 0                 ;        // Non e' una nota commerciale
                     InfoNote.Shift         = 0                 ;        // Non applicabile
                     InfoNote.Periodicita   = T_PERIODICITA::InLimitiOrario ;  // Non applicabile
                     InfoNote.DiFermata     = 0                 ;
                     InfoNote.DiMvg         = 1                 ;
                     InfoNote.Mvg           = 1 << NVg          ;        // Indica il mezzo viaggiante
                     InfoNote.TipoAUX       = 0                 ;
                     InfoNote.MaxDa         = 0                 ;
                     if(DeterminaEstensioneAux(InfoNote, MvLe , NVg)) DatiAux += InfoNote;
                  }
               } /* endfor */
//<<<    case '3':       // record di tipo 3
            }
            break;
//<<<    switch  To->TipoRecord      // Gestione differenti tipi record
         case '5':
            {
               // ..............................
               // Note di fermata
               // ..............................
               // Elaborazione dei records di tipo 5
               LOCALITA & Lo = Loc[PLoc]; // Sono in corrispondenza 1 ad 1
               PLoc ++;
               if(Lo.TestRecord != 'A') break; // Record Illegale
               assert(!memcmp(Lo.CodiceSuQuadro,To->R5.Localita,3));
               ULONG Ccr = Lo.Codice.Cod2();
               OkIn  |= (Ccr == CCR_In );
               OkOut &= (Ccr != CCR_Out);
               if (IgnoraFermata(Ccr))break;
               if (!OkIn)break;
               ID Id = CCR_ID::CcrId->Id(Ccr);
               if(Id == 0) break; // Ignoro la fermata
               // Determino le note di fermata
               for (int i = 0 ;i < 5 ;i++ ) {
                  int Nota = It(To->R5.NoteDiFermata[i]);
                  if(Nota ){
                     INFO_AUX InfoNote;
                     ZeroFill(InfoNote);
                     InfoNote.MezzoVirtuale = MvLe.MezzoVirtuale;
                     InfoNote.Tipo          = AUX_NOTA          ;
                     InfoNote.Id            = Nota              ;
                     InfoNote.Info          = 0                 ;        // Non e' una nota commerciale
                     InfoNote.Shift         = 0                 ;        // Non applicabile
                     InfoNote.Periodicita   = T_PERIODICITA::InLimitiOrario ;  // Non applicabile
                     InfoNote.DiFermata     = 1                 ;
                     InfoNote.DiMvg         = 0                 ;
                     InfoNote.Mvg           = 0 << NVg          ;        // Indica il mezzo viaggiante
                     InfoNote.TipoAUX       = 1                 ;
                     InfoNote.Da            = Ccr               ;
                     InfoNote.A             = 0                 ;
                     InfoNote.MaxDa         = 0                 ;
                     if(DeterminaEstensioneAux(InfoNote, MvLe , NVg)) DatiAux += InfoNote;
                  }
               } /* endfor */
//<<<    case '5':
            }
            break;
//<<<    switch  To->TipoRecord      // Gestione differenti tipi record
         }
         TabOr.Next();
         To = & TabOr.RecordCorrente() ;
//<<< while  OkOut && To &&  To->TipoRecord != '2'
      }
//<<< for  int NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++
   }
//<<< void WORK::GestioneNoteDiTrenoEFermata  MEZZO_VIRTUALE & MvLe , APPO & DatiAux
};

//----------------------------------------------------------------------------
// WORK::GestioneServizi
//----------------------------------------------------------------------------
void WORK::GestioneServizi( MEZZO_VIRTUALE & MvLe , APPO & DatiAux){
   #undef TRCRTN
   #define TRCRTN "WORK::GestioneServizi"

   BOOL HoDisuniformi = FALSE;

   // .................................
   // Gestione dei servizi non uniformi
   // .................................
   for (int NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ) {
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
      if(ServNu.Seek(MvGi.IdentTreno)){
         while ( & ServNu.RecordCorrente() && ServNu.RecordCorrente().IdTreno == MvGi.IdentTreno ) {
            FULL_INFO & Serv =  ServNu.RecordCorrente().Servizi;
            INFO_AUX InfoServ;
            ZeroFill(InfoServ);
            InfoServ.MezzoVirtuale = MvLe.MezzoVirtuale;
            InfoServ.Tipo          = AUX_SERVIZIO      ;
            InfoServ.Info          = 0                 ;
            InfoServ.Shift         = MvGi.ShiftPartenza - MvGi.ShiftDaPTreno; // Shift effettivamente applicato
            InfoServ.Periodicita   = Serv.PeriodicitaServizi;
            InfoServ.Periodicita.ShiftMe(InfoServ.Shift);
            InfoServ.DiFermata     = Serv.TipoSRV == 1 && InfoServ.Periodicita == T_PERIODICITA::InOrario; // le note ed i servizi di fermata non debbono avere periodicit…
            InfoServ.DiMvg         = Serv.TipoSRV == 0 ;
            if (InfoServ.DiMvg) {
               InfoServ.Mvg        = 1 << NVg          ;        // Indica il mezzo viaggiante
            } else {
               InfoServ.Mvg        = 0                 ;
            } /* endif */
            InfoServ.TipoAUX       = Serv.TipoSRV      ;
            InfoServ.Da            = Serv.Da.Cod2()    ;
            InfoServ.A             = Serv.A.Cod2()     ;
            InfoServ.MaxDa         = Serv.MaxDa.Cod2() ;
            if(DeterminaEstensioneAux(InfoServ, MvLe , NVg)){
               // Adesso debbo splittare MM_INFO in tanti servizi elementari
               for (int S = 0;S < 32 ; S++ ) {
                  if(Serv.TestServizio(S)){
                     InfoServ.Id = S;
                     DatiAux += InfoServ;
                     HoDisuniformi = TRUE;
                  }
               } /* endfor */
            }
            ServNu.Next();
//<<<    while   & ServNu.RecordCorrente   && ServNu.RecordCorrente  .IdTreno == MvGi.IdentTreno
         } /* endwhile */
//<<< if ServNu.Seek MvGi.IdentTreno
      }
//<<< for  int NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++
   } /* endfor */

   // .................................
   // Gestione dei servizi uniformi (a livello di treno)
   // .................................
   MM_INFO ServiziUniformi;
   ServiziUniformi.Clear();
   for (NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ) {
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
      PTRENO & Treno = * PTRENO::Get(MvGi.IdentTreno);
      assert(&Treno != NULL);
      if (NVg == 0) {
         ServiziUniformi  = Treno.ServiziUniformiTreno;
      } else {
         ServiziUniformi &= Treno.ServiziUniformiTreno;
      } /* endif */
   }
   // Ora ServiziUniformi contiene servizi uniformi a livello di mezzo virtuale

   TRACESTRING(VRS(MvLe.MezzoVirtuale) + VRS(ServiziUniformi.Decodifica(TRUE)));

   // I servizi uniformi a livello di treno ma non di mezzo virtuale vanno
   // trasformati in  servizi non uniformi
   for (NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ) {
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
      PTRENO & Treno = * PTRENO::Get(MvGi.IdentTreno);
      assert(&Treno != NULL);
      MM_INFO ServiziNonUniformi;
      ServiziNonUniformi = Treno.ServiziUniformiTreno;
      ServiziNonUniformi.NotIn(ServiziUniformi, TRUE);
      if (!ServiziNonUniformi.EmptyServizi()){
         TRACESTRING(VRS(MvLe.MezzoVirtuale) + VRS(MvGi.IdentTreno) +VRS(ServiziNonUniformi.Decodifica(FALSE)));
         INFO_AUX InfoServ;
         ZeroFill(InfoServ);
         InfoServ.MezzoVirtuale = MvLe.MezzoVirtuale;
         InfoServ.Tipo          = AUX_SERVIZIO      ;
         InfoServ.Info          = 0                 ;
         InfoServ.Shift         = MvGi.ShiftPartenza - MvGi.ShiftDaPTreno; // Shift effettivamente applicato
         InfoServ.Periodicita   = T_PERIODICITA::InLimitiOrario ; // Non applicabile
         InfoServ.Periodicita.ShiftMe(InfoServ.Shift);
         InfoServ.DiFermata     = 0                 ;
         InfoServ.DiMvg         = 1                 ;
         InfoServ.Mvg           = 1 << NVg          ;        // Indica il mezzo viaggiante
         InfoServ.TipoAUX       = 0                 ;        // Di mezzo viaggiante
         InfoServ.MaxDa         = 0                 ;
         if(DeterminaEstensioneAux(InfoServ, MvLe , NVg)){
            // Adesso debbo splittare MM_INFO in tanti servizi elementari
            for (int S = 0;S < 32 ; S++ ) {
               if(ServiziNonUniformi.TestServizio(S)){
                  InfoServ.Id = S;
                  DatiAux += InfoServ;
                  HoDisuniformi = TRUE;
               }
            } /* endfor */
         } /* endif */
//<<< if  !ServiziNonUniformi.EmptyServizi
      } /* endif */
//<<< for  NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++
   }
   ServiziUniformi.Disuniforme |= HoDisuniformi;
   MvLe.ServiziUniformi = ServiziUniformi;

//<<< void WORK::GestioneServizi  MEZZO_VIRTUALE & MvLe , APPO & DatiAux
};
//----------------------------------------------------------------------------
// WORK::GestioneNoteCommerciali
//----------------------------------------------------------------------------
void WORK::GestioneNoteCommerciali( MEZZO_VIRTUALE & MvLe , APPO & DatiAux){
   #undef TRCRTN
   #define TRCRTN "WORK::GestioneNoteCommerciali"

   // ....................
   // Ciclo dei mezzi viaggianti
   // ....................
   for (int NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++ ) {
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
      if(NoteCommerciali.Seek(MvGi.IdentTreno)){
         while ( & NoteCommerciali.RecordCorrente() && NoteCommerciali.RecordCorrente().IdTreno == MvGi.IdentTreno ) {
            FULL_NOTA & Nota =  NoteCommerciali.RecordCorrente().Nota;
            INFO_AUX InfoNote;
            ZeroFill(InfoNote);
            InfoNote.MezzoVirtuale = MvLe.MezzoVirtuale;
            InfoNote.Tipo          = AUX_NOTA          ;
            InfoNote.Id            = Nota.CodiceNota   ;
            InfoNote.Info          = 1                 ;        // E' una nota commerciale
            InfoNote.Shift         = MvGi.ShiftPartenza - MvGi.ShiftDaPTreno; // Shift effettivamente applicato
            InfoNote.Periodicita   = Nota.PeriodicitaNota ;
            InfoNote.Periodicita.ShiftMe(InfoNote.Shift);
            InfoNote.DiFermata     = Nota.TipoNOT == 1 && InfoNote.Periodicita == T_PERIODICITA::InOrario; // le note ed i servizi di fermata non debbono avere periodicit…
            InfoNote.DiMvg         = Nota.TipoNOT == 0 ;
            if (InfoNote.DiMvg) {
               InfoNote.Mvg        = 1 << NVg          ;        // Indica il mezzo viaggiante
            } else {
               InfoNote.Mvg        = 0                 ;
            } /* endif */
            InfoNote.TipoAUX       = Nota.TipoNOT      ;
            InfoNote.Da            = Nota.Da.Cod2()    ;
            InfoNote.A             = Nota.A.Cod2()     ;
            InfoNote.MaxDa         = Nota.MaxDa.Cod2() ;
            if(DeterminaEstensioneAux(InfoNote, MvLe , NVg)) DatiAux += InfoNote;
            NoteCommerciali.Next();
//<<<    while   & NoteCommerciali.RecordCorrente   && NoteCommerciali.RecordCorrente  .IdTreno == MvGi.IdentTreno
         } /* endwhile */
//<<< if NoteCommerciali.Seek MvGi.IdentTreno
      }
//<<< for  int NVg =  0; NVg < MvLe.NumMezziComponenti ; NVg++
   } /* endfor */
//<<< void WORK::GestioneNoteCommerciali  MEZZO_VIRTUALE & MvLe , APPO & DatiAux
};

//----------------------------------------------------------------------------
// WORK::ScaricaTestoNote
//----------------------------------------------------------------------------
int WORK::ScaricaTestoNote(){
   #undef TRCRTN
   #define TRCRTN "WORK::ScaricaTestoNote"

   #define MAXLINGUA 11 // Max 10 Lingue gestite
   struct TESTI {
      BYTE     Lingua;
      ELENCO_S Testo;
      void Init(BYTE i){ Lingua = i; Testo.Clear();};
   };
   TESTI Testi[MAXLINGUA];
   FILE_DATI_NOTE   TestoNote(PATH_OUT "NOTEPART.TMP");
   FILE_NOTEPART Note(PATH_IN "NOTEPART.T");

   TestoNote.Clear("Testo delle note");

   T_PERIODICITA Per;
   char LastTr;
   // begin EMS003 VA rinomino struttura
   //FILE_DATI_NOTE::R_STRU Nota;
   FILE_DATI_NOTE::FDN_R_STRU Nota;
   // end EMS
   int i;
   Bprintf("Caricamento NOTEPART");
   ORD_FORALL(Note,n1){
      TryTime(n1);
      NOTEPART & Rec = Note[n1];
      switch (Rec.TipoRecord) {

      case '1':      // Gestione flags
         if (n1 != 0) {
            for (i= 0; i < MAXLINGUA;i++ ){
               ELENCO_S & Testo = Testi[i].Testo;
               if(Testo.Dim() == 0)continue;
               Nota.Linguaggio = Testi[i].Lingua;
               Nota.Progressivo = 0;
               ORD_FORALL(Testo,t){
                  ZeroFill(Nota.Testo);
                  strcpy(Nota.Testo,(CPSZ)Testo[t]);
                  TestoNote.AddRecordToEnd(BUFR(&Nota,sizeof(Nota)));
                  Nota.Progressivo ++;
               }
            } /* endfor */
         } /* endif */
         Per = T_PERIODICITA::InLimitiOrario;
         for (i= 0; i < MAXLINGUA;i++ )Testi[i].Init(i);
         Nota.IdNota = It(Rec.Nota);
         Nota.Progressivo = 0;
         Nota.Periodicita = Per;

         break;

//<<< switch  Rec.TipoRecord
      case '2':      // Testo della nota
         assert(Rec.R2.IdentificativoLingua -'0' < MAXLINGUA);
         Testi[Rec.R2.IdentificativoLingua-'0'].Testo += STRINGA(St(Rec.R2.Testo)).Strip();
         break;

      case '3':      // Periodicita' della nota
         Per.ComponiPeriodicita(5,Rec.R3.Periodicita,(LastTr != '3'));
         break;

      default:
         Bprintf("Tipo record errato su NOTEPART: STOP");
         BEEP;
         exit(999);
         break;
      } /* endswitch */
      LastTr = Rec.TipoRecord;
//<<< ORD_FORALL Note,n1
   }
   for (i= 0; i < MAXLINGUA;i++ ){
      ELENCO_S & Testo = Testi[i].Testo;
      if(Testo.Dim() == 0)continue;
      Nota.Linguaggio = Testi[i].Lingua;
      Nota.Progressivo = 0;
      ORD_FORALL(Testo,t){
         ZeroFill(Nota.Testo);
         strcpy(Nota.Testo,(CPSZ)Testo[t]);
         TestoNote.AddRecordToEnd(BUFR(&Nota,sizeof(Nota)));
         Nota.Progressivo ++;
      }
   } /* endfor */
   TestoNote.Flush();
   return 0;
//<<< int WORK::ScaricaTestoNote
};

//----------------------------------------------------------------------------
// WORK::ScaricaMezziViaggianti
//----------------------------------------------------------------------------
int WORK::ScaricaMezziViaggianti(){
   #undef TRCRTN
   #define TRCRTN "WORK::ScaricaMezziViaggianti"
   FILE_PTRENO Treni;
   F_MEZZO_VIAGG Treni2(PATH_OUT "MZVIAG.TMP");
   Treni2.Clear("Mezzi Viaggianti");
   ORD_FORALL(Treni,i){
      PTRENO & MezzoViaggiante                      = Treni[i];
      MEZZO_VIAGG MezzoViaggiante2;
      MezzoViaggiante2.NumeroTreno                  = MezzoViaggiante.NumeroTreno            ;
      MezzoViaggiante2.TipoMezzo                    = MezzoViaggiante.ServiziTreno.TipoMezzo ;
      MezzoViaggiante2.IdTreno                      = MezzoViaggiante.IdTreno                ;
      MezzoViaggiante2.Servizi                      = MezzoViaggiante.ServiziTreno           ;
      MezzoViaggiante2.ServiziUniformi              = MezzoViaggiante.ServiziUniformiTreno   ;
      ByteCopy(MezzoViaggiante2.KeyTreno            , MezzoViaggiante.KeyTreno )             ;
      ByteCopy(MezzoViaggiante2.NomeMezzoViaggiante , MezzoViaggiante.NomeMezzoViaggiante )  ;
      Treni2.AddRecordToEnd(VRB(MezzoViaggiante2));
   }
   return 0;
};
//----------------------------------------------------------------------------
// WORK::Riorganizza
//----------------------------------------------------------------------------
/*
   Riorganizzazione dei dati in formato finale

   Questa funzione viene chiamata per ricompattare tutti i dati relativi
   ad un unico mezzo virtuale.

   Si opera nel seguente modo:

   - I dati sono compattati per tratta:

      + Per ogni dato si cerca di aggregargli un secondo dato, a parit…
         di periodicit…

      + Uno dei due dati Š rimosso (quello con indice maggiore)

      + Ripeto il ciclo finchŠ non posso piu' fare accorpamenti

   - I dati sono compattati per periodicit… con la stessa logica

   Non cerco di compattare per TipoAux == 2 (soppresso alla fermata indicata)
   perchŠ il gioco non vale la candela

   Non cerco di compattare per TipoAux == 4 (esattamente da fermata a
   fermata) perchŠ al momento nell' orario non ho tali casi tranne che
   per treni singoli .

   Comunque non Š chiaro neanche come potrebbero essere forniti i dati per
   indicare una tale situazione

   Per TipoAux = 0 permetto anche accorpamenti misti con TipoAux = 3,
   purchŠ vi sia coincidenza con la stazione di cambio.

   Infine controllo e modifico i codici CCR in modo che siano sempre validi

*/
//----------------------------------------------------------------------------
int WORK::Riorganizza(APPO & Appo){
   #undef TRCRTN
   #define TRCRTN "WORK::Riorganizza"

   if(Appo.Dim() == 0) return 0;
   WORD NMvLe = Appo[0].MezzoVirtuale;
   BOOL Loop;
   do {     // Compattazione di intervallo
      Loop = FALSE;
      FORALL(Appo,i){
         INFO_AUX & Corr = Appo[i];
         assert( Corr.MezzoVirtuale == NMvLe );
         assert( (Corr.TipoAUX == 0) == Corr.DiMvg );
         FORALL(Appo,j){
            if(i == j) continue;
            INFO_AUX & Alt  = Appo[j];
            // Verifico se sia possibile compattarli
            if(Alt.Tipo != Corr.Tipo) continue;
            if(Alt.Id   != Corr.Id  ) continue;
            if(Alt.Info != Corr.Info) continue;
            INFO_AUX Composito;
            Composito.MezzoVirtuale = NMvLe                   ;
            Composito.Tipo          = Corr.Tipo               ;
            Composito.Id            = Corr.Id                 ;
            Composito.Info          = Corr.Info               ;
            // Verifico che la periodicit… corrisponda
            if (Corr.Shift <= Alt.Shift) {
               if(!Corr.Periodicita.ConfrontaShiftata(Alt.Periodicita, Alt.Shift - Corr.Shift))continue;
               Composito.Periodicita   = Corr.Periodicita        ;
               Composito.Shift         = Corr.Shift              ;
            } else {
               if(!Alt.Periodicita.ConfrontaShiftata(Corr.Periodicita, Corr.Shift - Alt.Shift))continue;
               Composito.Periodicita   = Alt.Periodicita        ;
               Composito.Shift         = Alt.Shift              ;
            } /* endif */
            // Ok: Qui corrispondono tipi periodicit… ecc: debbo verificare se le
            // tipologie di dati permettano l' accorpamento
            if(Corr.TipoAUX == 0 && Alt.TipoAUX == 0){
               // Preparo i dati per un' accorpamento di interi treni
               Composito.DiFermata     = 0                       ;
               Composito.DiMvg         = 1                       ;
               Composito.Mvg           = Corr.Mvg | Alt.Mvg      ;
               Composito.TipoAUX       = 0                       ;
               // Questi dati vengono gestiti solo in vista di una eventuale trasformazione
               // dell' informazione in tipo 3: si veda anche il commento poco pi— avanti
               // Si noti che non verifico se vi siano buchi tra i due treni
               if( MinDaMvg(Corr.Mvg ) <= MinDaMvg(Alt.Mvg )){
                  Composito.Da         = Corr.Da                 ;
               } else {
                  Composito.Da         = Alt.Da                  ;
               }
               if( MaxDaMvg(Corr.Mvg ) >= MaxDaMvg(Alt.Mvg )){
                  Composito.A          = Corr.A                  ;
               } else {
                  Composito.A          = Alt.A                   ;
               }
               Composito.MaxDa         = 0                       ;
            } else if(
               // La combinazione 0/0 e' gi… stata trattata: rimangono 3/3, 3/0, 0/3
               ( Corr.TipoAUX == 3 || Corr.TipoAUX == 0 ) &&
               (  Alt.TipoAUX == 3 ||  Alt.TipoAUX == 0 ) ){
               // Per le combinazioni 0/3 e 3/0 si trasformano i dati da dati di treno a dati di intervallo
               // Si noti che cos facendo potrei riempire eventuali "buchi" : tuttavia cio' non
               // mi sembra troppo scorretto dal punto di vista ferroviario (se avessi un passeggero
               // gi… a bordo sostanzialmente potrebbe usufruire del servizio) e comunque si verifica
               // solo se il mezzo virtuale ha almeno 4 mezzi componenti: in pratica Š improbabile
               if(Corr.A != Alt.Da) continue;
               Composito.DiFermata     = 0                       ;
               Composito.DiMvg         = 0                       ;
               Composito.Mvg           = 0                       ;
               Composito.TipoAUX       = 3                       ;
               Composito.Da            = Corr.Da                 ;
               Composito.A             = Alt.A                   ;
               if(Alt.MaxDa == 0 ){
                  Composito.MaxDa      = Corr.MaxDa              ;
               } else {
                  Composito.MaxDa      = Alt.MaxDa               ;
                  if(Corr.MaxDa != 0 && Corr.MaxDa != Alt.MaxDa && Corr.MaxDa != Corr.A ){
                     TabTv.Seek(NMvLe);
                     MEZZO_VIRTUALE & MvLe = TabTv.RecordCorrente();
                     int Idx = MvLe.TrDaCambio(Corr.A);
                     Bprintf("Errore: Cambio tra treni %s e %s con differente stazione massima di prenotazione:", (CPSZ)MvLe.Mv[Idx].IdentTreno, (CPSZ)MvLe.Mv[Idx+1].IdentTreno );
                     ID Id = CCR_ID::CcrId->Id(Corr.MaxDa);
                     Bprintf("        Il treno %s e' prenotabile fino alla stazione Cod CCR %i %s", (CPSZ)MvLe.Mv[Idx].IdentTreno, Corr.MaxDa, Stazioni.DecodificaIdStazione(Id));
                     Id = CCR_ID::CcrId->Id(Alt.MaxDa);
                     Bprintf("        Il treno %s e' prenotabile fino alla stazione Cod CCR %i %s", (CPSZ)MvLe.Mv[Idx+1].IdentTreno, Alt.MaxDa, Stazioni.DecodificaIdStazione(Id));
                  }
               }
//<<<       if Corr.TipoAUX == 0 && Alt.TipoAUX == 0
            } else {
               continue; // Non compatto
            } /* endif */

            TRACESTRING(" Accorpati tratta per" VRS(i) + VRS(j));
            Corr.Trace("Info I Comp Tratta");
            Alt.Trace("Info J Comp Tratta");
            // Sostituisco il dato con indice piu' basso ed elimino quello con indice
            // piu' alto (anche se e' il corrente)
            if (i < j) {
               Appo[i] = Composito;
               Appo.Elimina(j);
            } else {
               Appo[j] = Composito;
               Appo.Elimina(i);
               break; // Perche' non e' pi— valido l' iesimo dato
            } /* endif */
            Loop = TRUE; // Mi fermo solo quando non posso pi— far modifiche
//<<<    FORALL Appo,j
         }
//<<< FORALL Appo,i
      }
//<<< do       // Compattazione di intervallo
   } while ( Loop ); /* enddo */

   do {     // Compattazione di periodo
      Loop = FALSE;
      FORALL(Appo,i){
         INFO_AUX & Corr = Appo[i];
         FORALL(Appo,j){
            if(i == j) continue;
            INFO_AUX & Alt  = Appo[j];
            // Verifico se sia possibile compattarli
            if(Alt.Tipo    != Corr.Tipo   ) continue;
            if(Alt.Id      != Corr.Id     ) continue;
            if(Alt.Info    != Corr.Info   ) continue;
            if(Alt.TipoAUX != Corr.TipoAUX) continue;
            if(Alt.Mvg     != Corr.Mvg    ) continue;
            if(Alt.Da      != Corr.Da     ) continue;
            if(Alt.A       != Corr.A      ) continue;
            if(Alt.MaxDa   != Corr.MaxDa  ) continue;
            assert(Alt.DiFermata == Corr.DiFermata);
            assert(Alt.DiMvg     == Corr.DiMvg    );
            assert(Alt.Shift     == Corr.Shift    );
            TRACESTRING(" Accorpati periodo, per" VRS(i) + VRS(j));
            Corr.Trace("Info I Comp Periodo");
            Alt.Trace("Info J Comp Periodo");
            // Sostituisco il dato con indice piu' basso ed elimino quello con indice
            // piu' alto (anche se e' il corrente)
            if (i < j) {
               Appo[i].Periodicita |= Appo[j].Periodicita;
               Appo.Elimina(j);
            } else {
               Appo[j].Periodicita |= Appo[i].Periodicita;
               Appo.Elimina(i);
               break; // Perche' non e' pi— valido l' iesimo dato
            } /* endif */
            Loop = TRUE; // Mi fermo solo quando non posso pi— far modifiche
//<<<    FORALL Appo,j
         }
//<<< FORALL Appo,i
      }
//<<< do       // Compattazione di periodo
   } while ( Loop ); /* enddo */

   // Adesso faccio un controllo finale: Le stazioni Da A e MaxDa debbono essere
   // identificabili sul treno e corrispondere a fermate valide.
   // Ove cos non fosse do' errore
   FORALL(Appo,i){
      INFO_AUX & Info = Appo[i];
      if(Info.Da != 0 && !IgnoraFermata(Info.Da)){
         if(CCR_ID::CcrId->Id(Info.Da) == 0 ){
            Info.Trace("Info:");
            Bprintf2("Errore: Mezzo Virtuale %i la stazione 'Da' ha codice CCR %i non valido", Info.MezzoVirtuale, Info.Da);
         } else {
            FERMATE_VIRT & FvDa = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Info.Da );
            if (! &FvDa) {
               Info.Trace("Info:");
               Bprintf("Errore: Mezzo Virtuale %i la stazione 'Da' con CCR %i non e' stazione del treno", Info.MezzoVirtuale, Info.Da);
            } /* endif */
         }
      }
      if(Info.MaxDa != 0 && !IgnoraFermata(Info.MaxDa)){
         if(CCR_ID::CcrId->Id(Info.MaxDa) == 0 ){
            Info.Trace("Info:");
            Bprintf2("Errore: Mezzo Virtuale %i la stazione 'MaxDa' ha codice CCR %i non valido", Info.MezzoVirtuale, Info.MaxDa);
         } else {
            FERMATE_VIRT & FvMaxDa = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Info.MaxDa );
            if (! &FvMaxDa) {
               Info.Trace("Info:");
               Bprintf("Errore: Mezzo Virtuale %i la stazione 'MaxDa' con CCR %i non e' stazione del treno", Info.MezzoVirtuale, Info.MaxDa);
            } /* endif */
         }
      }
      if(Info.A != 0 && !IgnoraFermata(Info.A)){
         if(CCR_ID::CcrId->Id(Info.A) == 0){
            Info.Trace("Info:");
            Bprintf2("Errore: Mezzo Virtuale %i la stazione 'A' ha codice CCR %i non valido", Info.MezzoVirtuale, Info.A);
         } else {
            FERMATE_VIRT & FvA = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Info.A );
            if (! &FvA) {
               Info.Trace("Info:");
               Bprintf("Errore: Mezzo Virtuale %i la stazione 'A' con CCR %i non e' stazione del treno", Info.MezzoVirtuale, Info.A);
            } /* endif */
         }
      }
//<<< FORALL Appo,i
   }

   // Eventuale DUMP del contenuto
   #ifdef DBG1
   ORD_FORALL(Appo,Tr) Appo[Tr].Trace("Dump:");
   #endif
   return 0;
//<<< int WORK::Riorganizza APPO & Appo
}
//----------------------------------------------------------------------------
// WORK::DeterminaEstensioneAux
//----------------------------------------------------------------------------
// Questo metodo corregge le stazioni Da ed A dei servizi:
// Se Info e' di tipo Treno Da ed A sono la prima e l' ultima stazione del treno
// oppure le stazioni di cambio del mezzo virtuale.
// Se Info e' di tipo Da-A controlla che Da-A siano entro i limiti delle stazioni di cambio
// e se del caso sostituisce con le stazioni di cambio
// Analogamente per MaxDa
// In caso di anomalie ritorna FALSE
//----------------------------------------------------------------------------
BOOL WORK::DeterminaEstensioneAux(INFO_AUX & Info, MEZZO_VIRTUALE & MvLe , int NVg){
   #undef TRCRTN
   #define TRCRTN "WORK::DeterminaEstensioneAux"
   MEZZO_VIRTUALE::MEZZO_VIAGGIANTE MvGi = MvLe.Mv[NVg];
   PTRENO & Treno = * PTRENO::Get(MvGi.IdentTreno);
   int MinDa,MaxA; // CCR degli estremi utilizzabili della tratta
   if (NVg  == 0){
      MinDa = Treno.CcrDa.Cod2()      ;
   } else {
      MinDa = MvLe.Mv[NVg-1].CcrStazioneDiCambio ;
   }
   if (NVg  == MvLe.NumMezziComponenti-1){
      MaxA  = Treno.CcrA.Cod2() ;
   } else {
      MaxA  = MvLe.Mv[NVg].CcrStazioneDiCambio    ;
   }
   if (Info.TipoAUX == 0) { // Informazione di treno: Imposto Da ed A ai limiti della tratta
      Info.Da = MinDa;
      Info.A  = MaxA ;
   } else if (Info.TipoAUX == 1 ){ // Di fermata
      // Debbo controllare che la fermata appartenga al mezzo virtuale
      FERMATE_VIRT & FvDa = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Info.Da );
      if(&FvDa == NULL) return FALSE;
   } else if (Info.TipoAUX == 2 ){ // Escluso fermata: Equivale ad una informazione di treno
      Info.Da = MinDa;
      Info.A  = MaxA ;
   } else if (Info.TipoAUX == 3 || Info.TipoAUX == 4 ) { // Informazione Da-A
      // Controllo che Da ed A gi… forniti non siano oltre gli estremi della tratta
      if (Info.Da != MinDa ){
         FERMATE_VIRT & FvDa = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Info.Da );
         if(& FvDa == NULL){ // Devo andare alle fermate dei treni per capire se e' OK
            BOOL HoMinDa = FALSE, HoMaxA = FALSE , PutNext = FALSE;
            for ( int j = Treno.PLocalita ; j < Loc.Dim() ; j++ ) {
               LOCALITA & Lo = Loc[j];
               if(Lo.IdentTreno != MvGi.IdentTreno) break; // Cambiato treno
               int Ccr = Lo.Codice.Cod2();
               if ( Ccr == MinDa  ) HoMinDa = TRUE;
               if ( Ccr == MaxA   ) HoMaxA  = TRUE;
               if (PutNext && ! HoMaxA ) {  // Sto cercando una fermata sostitutiva
                  FERMATE_VIRT & FvSubst = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Ccr );
                  if (&FvSubst && FvSubst.FermataPartenza) {
                     Info.Da = Ccr;
                     j = -1; // Per indicare che ho sostituito
                     break;
                  } /* endif */
               } else if ( Ccr == Info.Da){
                  if (HoMaxA) {              // Dopo l' intervallo
                     return FALSE;
                  } else if (HoMinDa ){      // Interna all' intervallo
                     // Debbo sostituire la prima stazione valida
                     PutNext = TRUE;
                  } else {                   // Prima dell' intervallo
                     // Sostituisco la prima stazione della tratta
                     Info.Da = MinDa;
                     j = -1; // Per indicare che ho sostituito
                     break;
                  } /* endif */
               }
//<<<       for   int j = Treno.PLocalita ; j < Loc.Dim   ; j++
            } /* endfor */
            if (j >= 0) { // Non sono riuscito a sostituire
               return FALSE;
            } /* endif */
//<<<    if & FvDa == NULL   // Devo andare alle fermate dei treni per capire se e' OK
         } else if(FvDa.TrenoFisico != NVg){ // E' di un' altro treno!
            if(FvDa.TrenoFisico < NVg ){ // Precedente
               Info.Da = MinDa;
            } else {
               return FALSE;
            }
         }
//<<< if  Info.Da != MinDa
      } /* endif */
      if (Info.A  != MaxA  ){
         FERMATE_VIRT & FvA = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Info.A );
         if(&FvA == NULL){ // Devo andare alle fermate dei treni per capire se e' OK
            BOOL HoMaxA = FALSE, HoMinDa = FALSE ;
            int LastCcrValido = 0;
            for ( int j = Treno.PLocalita ; j < Loc.Dim() ; j++ ) {
               LOCALITA & Lo = Loc[j];
               if(Lo.IdentTreno != MvGi.IdentTreno) break; // Cambiato treno
               int Ccr = Lo.Codice.Cod2();
               if ( Ccr == MaxA    ) HoMaxA  = TRUE;
               if ( Ccr == Info.Da ) HoMinDa = TRUE;  // Piu' corretto testare con Info.Da
               if (HoMinDa) {  // Sto cercando una fermata sostitutiva
                  FERMATE_VIRT & FvSubst = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Ccr );
                  if (&FvSubst && FvSubst.FermataArrivo) LastCcrValido = Ccr;
               };
               if ( Ccr == Info.A){
                  if (!HoMinDa) {            // Prima dell' intervallo
                     return FALSE;
                  } else if (!HoMaxA ){      // Interna all' intervallo
                     if (LastCcrValido) {
                        Info.A = LastCcrValido;
                        j = -1; // Per indicare che ho sostituito
                        break;
                     } else {
                        return FALSE;
                     } /* endif */
                  } else {                   // Dopo  l' intervallo
                     // Sostituisco l' ultima stazione della tratta
                     Info.A = MaxA;
                     j = -1; // Per indicare che ho sostituito
                     break;
                  } /* endif */
               }
//<<<       for   int j = Treno.PLocalita ; j < Loc.Dim   ; j++
            } /* endfor */
            if (j >= 0) { // Non sono riuscito a sostituire
               return FALSE;
            } /* endif */
//<<<    if &FvA == NULL   // Devo andare alle fermate dei treni per capire se e' OK
         } else if(FvA.TrenoFisico != NVg){ // E' di un' altro treno!
            if(FvA.TrenoFisico > NVg ){ // Seguente (si noti che non pu• essere la stazione di cambio
               Info.A = MaxA;
            } else {
               return FALSE;
            }
         }
//<<< if  Info.A  != MaxA
      } /* endif */
      // E adesso controllo se per caso non vi sia un limite di prenotazione, per cui
      // debbo comunque inibire
      if (Info.MaxDa != 0 && Info.MaxDa != MaxA ){
         FERMATE_VIRT & FvMaxDa = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Info.MaxDa );
         if(&FvMaxDa == NULL){ // Devo andare alle fermate dei treni per capire se e' OK
            BOOL HoMaxA = FALSE, HoMinDa = FALSE ;
            int LastCcrValido = 0;
            for ( int j = Treno.PLocalita ; j < Loc.Dim() ; j++ ) {
               LOCALITA & Lo = Loc[j];
               if(Lo.IdentTreno != MvGi.IdentTreno) break; // Cambiato treno
               int Ccr = Lo.Codice.Cod2();
               if ( Ccr == MaxA    ) HoMaxA  = TRUE;
               if ( Ccr == Info.Da ) HoMinDa = TRUE;
               if (HoMinDa) {  // Sto cercando una fermata sostitutiva
                  FERMATE_VIRT & FvSubst = TabFv.CercaCcrDiMv(Info.MezzoVirtuale, Ccr );
                  if (&FvSubst && FvSubst.FermataPartenza) LastCcrValido = Ccr;
               };
               if ( Ccr == Info.MaxDa){
                  if (!HoMinDa) {            // Prima dell' intervallo
                     return FALSE;
                  } else if (!HoMaxA ){      // Interna all' intervallo
                     if (LastCcrValido) {
                        Info.MaxDa = LastCcrValido;
                        j = -1; // Per indicare che ho sostituito
                        break;
                     } else {
                        return FALSE;
                     } /* endif */
                  } else {                   // Dopo  l' intervallo
                     // Sostituisco l' ultima stazione della tratta
                     Info.MaxDa = MaxA;
                     j = -1; // Per indicare che ho sostituito
                     break;
                  } /* endif */
               }
//<<<       for   int j = Treno.PLocalita ; j < Loc.Dim   ; j++
            } /* endfor */
            if (j >= 0) { // Non sono riuscito a sostituire
               return FALSE;
            } /* endif */
//<<<    if &FvMaxDa == NULL   // Devo andare alle fermate dei treni per capire se e' OK
         } else if(FvMaxDa.TrenoFisico != NVg){ // E' di un' altro treno!
            if(FvMaxDa.TrenoFisico > NVg ){ // Seguente (si noti che non pu• essere la stazione di cambio
               Info.MaxDa = MaxA;
            } else {
               return FALSE;
            }
         }
//<<< if  Info.MaxDa  != MaxA
      } /* endif */
//<<< if  Info.TipoAUX == 0    // Informazione di treno: Imposto Da ed A ai limiti della tratta
   } else {
      Bprintf("Errore: TipoAux non gestito !");
      BEEP;
      return FALSE;
   } /* endif */
   return TRUE  ;
//<<< BOOL WORK::DeterminaEstensioneAux INFO_AUX & Info, MEZZO_VIRTUALE & MvLe , int NVg
}
