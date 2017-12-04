//----------------------------------------------------------------------------
// FTPTRENO.CPP: Classe PTRENO
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
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
#include "stddef.h"

#define PGM      "FTPTRENO"
HASH<PTRENO>     PTRENO::HashPt(20000,8000);

static FILE_LOCALITA * PLocal;
static FILE_TABORARI * PTabOr;

//----------------------------------------------------------------------------
// EsaminaNoteGeneriche
//----------------------------------------------------------------------------
// Questa funzione verifica, senza badare ai dettagli, se il treno ha note generiche.
void __fastcall EsaminaNoteGeneriche(TABORARI & TabOrari, MM_INFO & ServiziTreno) {
   #undef TRCRTN
   #define TRCRTN "EsaminaNoteGeneriche"
   static char* Zeroes="00000000000000000000";
   if(! ServiziTreno.HaNoteGeneriche ){
      if(TabOrari.TipoRecord == '3'){
         if(ByteCmp(TabOrari.R3.NoteDiTreno,Zeroes))
            ServiziTreno.HaNoteGeneriche = 1;
      } else if(TabOrari.TipoRecord == '5'){
         if(TabOrari.R5.Flags.Transito != '*'          &&
            TabOrari.R5.Flags.FermataDiServizio != '*' &&
            ByteCmp(TabOrari.R5.NoteDiFermata,Zeroes))
         ServiziTreno.HaNoteGeneriche = 1;
      };
   }
   
   return;
}

//----------------------------------------------------------------------------
// PTRENO::Clear
//----------------------------------------------------------------------------
// Inizializzazione
void PTRENO::Clear(){
   #undef TRCRTN
   #define TRCRTN "PTRENO::Clear"
   memset(this,' ',offsetof(PTRENO,PeriodicitaTreno));
   memset(&this->PeriodicitaTreno,0,sizeof(THIS) - offsetof(PTRENO,PeriodicitaTreno));
   ServiziTreno.Clear();
   ServiziUniformiTreno.Clear();
};
//----------------------------------------------------------------------------
// PTRENO::Get
//----------------------------------------------------------------------------
// Cerca un treno dato l' Id
PTRENO * PTRENO::Get(IDTRENO & IdT){
   #undef TRCRTN
   #define TRCRTN "PTRENO::Get"
   return HashPt.Cerca((PTRENO *)&IdT,10);
};

//----------------------------------------------------------------------------
// PTRENO::GetPerio
//----------------------------------------------------------------------------
// Cerca un treno dato l' Id e ritorna la periodicita'
T_PERIODICITA PTRENO::GetPerio(IDTRENO & IdT){ // Periodicita' del treno
   #undef TRCRTN
   #define TRCRTN "PTRENO::GetPerio"
   PTRENO * Wrk = HashPt.Cerca((PTRENO *)&IdT,10);
   if (Wrk == NULL) {
      return T_PERIODICITA::InLimitiOrario;
   } else {
      return Wrk->PeriodicitaTreno;
   } /* endif */
};
//----------------------------------------------------------------------------
// PTRENO::Get2
//----------------------------------------------------------------------------
// Corregge le periodicita' in base all' ovvia regola che nulla di cio' che e'
// collegato ad un treno puo' essere valido quando il treno non circola.
// In particolare a volte la periodicita' data e' la stessa del treno,
// ed allora preferisco normalizzarla equiparandola all' intero orario
// (che per convenzione tratto come nessuna periodicita').
//----------------------------------------------------------------------------
T_PERIODICITA PTRENO::Get2(IDTRENO & IdT, T_PERIODICITA & Base){ // Periodicita' corretta con quella del treno
   #undef TRCRTN
   #define TRCRTN "PTRENO::GetPerio2"
   T_PERIODICITA Wrk = Base;
   T_PERIODICITA Trn = GetPerio(IdT);
   Wrk &= Trn;
   if (Wrk == Trn) {  // Non e' + restrittiva del treno
      return T_PERIODICITA::InLimitiOrario;
   } else {
      return Wrk;
   } /* endif */
}

//----------------------------------------------------------------------------
// PTRENO::OraPartenza
//----------------------------------------------------------------------------
// Torna l' ora di partenza da una data fermata
// -1 se la fermata non appartiene al treno o se il treno non ammette
// viaggiatori in partenza dalla fermata
//----------------------------------------------------------------------------
short int PTRENO::OraPartenza(const CCR & Fermata){
   #undef TRCRTN
   #define TRCRTN "PTRENO::OraPartenza"
   
   if (Fermata == CcrDa) return OraPartenzaTreno; // Eccezione: anche se e' solo un transito
   
   if(PLocal == NULL){
      PLocal = new FILE_LOCALITA( PATH_IN "LOCALITA.T");
      PTabOr = new FILE_TABORARI( PATH_IN "TABORARI.T");
   };
   FILE_LOCALITA & Local = *PLocal;
   FILE_TABORARI & TabOr = *PTabOr;
   for ( int i = PLocalita; i < Local.Dim() ; i++ ) {
      LOCALITA & Loc = Local[i];
      if(Loc.IdentTreno != IdTreno) break; // Next Treno
      if (Loc.Codice == Fermata) {
         for ( int j = PTaborariR2+1; j < TabOr.Dim() ; j++ ) {
            TABORARI & To = TabOr[j];
            if(To.TipoRecord == '2') break; // Next Treno
            if(To.TipoRecord != '5') continue;
            if(ByteCmp(To.R5.Localita,Loc.CodiceSuQuadro))continue;
            if (To.R5.Flags.FermataViaggInArrivo   == '*') return -1;
            return To.R5.Partenza.MinMz() ;
         } /* endfor */
         Bprintf("Errore interno algoritmo: Disallineamento localita' / taborari");
         BEEP;
         return -1;
      } /* endif */
   } /* endfor */
   return -1;
//<<< short int PTRENO::OraPartenza const CCR & Fermata  
};
//----------------------------------------------------------------------------
// PTRENO::OraArrivo
//----------------------------------------------------------------------------
// Torna l' ora di arrivo da una data fermata
// -1 se la fermata non appartiene al treno o se il treno non ammette
// viaggiatori in arrivo alla fermata
//----------------------------------------------------------------------------
short int PTRENO::OraArrivo(const CCR & Fermata){
   #undef TRCRTN
   #define TRCRTN "PTRENO::OraArrivo"
   if(PLocal == NULL){
      PLocal = new FILE_LOCALITA( PATH_IN "LOCALITA.T");
      PTabOr = new FILE_TABORARI( PATH_IN "TABORARI.T");
   };
   FILE_LOCALITA & Local = *PLocal;
   FILE_TABORARI & TabOr = *PTabOr;
   for ( int i = PLocalita; i < Local.Dim() ; i++ ) {
      LOCALITA & Loc = Local[i];
      if(Loc.IdentTreno != IdTreno) break; // Next Treno
      if (Loc.Codice == Fermata) {
         for ( int j = PTaborariR2+1; j < TabOr.Dim() ; j++ ) {
            TABORARI & To = TabOr[j];
            if(To.TipoRecord == '2') break; // Next Treno
            if(To.TipoRecord != '5') continue;
            if(ByteCmp(To.R5.Localita,Loc.CodiceSuQuadro))continue;
            if (To.R5.Flags.FermataViaggInPartenza == '*') return -1;
            return To.R5.Arrivo.MinMz() ;
         } /* endfor */
         Bprintf("Errore interno algoritmo: Disallineamento localita' / taborari");
         BEEP;
         return -1;
      } /* endif */
   } /* endfor */
   return -1;
//<<< short int PTRENO::OraArrivo const CCR & Fermata  
};

//----------------------------------------------------------------------------
// PTRENO::CercaCCR
//----------------------------------------------------------------------------
BOOL PTRENO::CercaCCR(int Ccr, int & PLoc, int & PTab){
   #undef TRCRTN
   #define TRCRTN "PTRENO::CercaCCR"
   if(Ccr < 0){
      if(PLocal != NULL){
         delete PLocal;
         delete PTabOr;
         PLocal = NULL;
         PTabOr = NULL;
      };
   }
   if(Ccr <= 0)return FALSE;
   if(PLocal == NULL){
      PLocal = new FILE_LOCALITA( PATH_IN "LOCALITA.T");
      PTabOr = new FILE_TABORARI( PATH_IN "TABORARI.T");
   };
   FILE_LOCALITA & Local = *PLocal;
   FILE_TABORARI & TabOr = *PTabOr;
   for ( int i = PLocalita; i < Local.Dim() ; i++ ) {
      LOCALITA & Loc = Local[i];
      if(Loc.IdentTreno != IdTreno) break; // Next Treno
      if (Loc.Codice.Cod2() == Ccr) {
         for ( int j = PTaborariR2+1; j < TabOr.Dim() ; j++ ) {
            TABORARI & To = TabOr[j];
            if(To.TipoRecord == '2') break; // Next Treno
            if(To.TipoRecord != '5') continue;
            if(ByteCmp(To.R5.Localita,Loc.CodiceSuQuadro))continue;
            PLoc = i;
            PTab = j;
            return TRUE;
         } /* endfor */
         Bprintf("Errore interno algoritmo: Disallineamento localita' / taborari");
         BEEP;
         return FALSE;
      } /* endif */
   } /* endfor */
   return FALSE;
//<<< BOOL PTRENO::CercaCCR int Ccr, int & PLoc, int & PTab  
};


//----------------------------------------------------------------------------
// PTRENO::Load
//----------------------------------------------------------------------------
/*
   Questo metodo carica i dati dei treni.
   
   Viene fatta una prima scansione dei dati dei treni ( TabOrari +
   InfoComm) per caricare i dati di tutti i treni.
   
   La periodicita' viene presa dai records di tipo 4 che seguono il
   PRIMO record di tipo 3.
   
   Sono identificati (dal record 2 di taborari) eventuali treni
   provenienza o destinazione (che saranno utilizzati poi per formare i
   mezzi virtuali non da carrozza diretta).

   Viene identificata la presenza di eventuali note di treno o di
   fermata(da record 3 e 5 di taborari)

   Controllo nella Hash table che i treni origine e destinazione siano
   validi, scartando i treni non esistenti.
   
   Poi scandisco InfoComm per individuare i servizi:  per ogni nota
   scandita chiamo "HaNota" per catalogare la nota, e Servizio() per
   vedere quali servizi implica la presenza della nota.
   
   I servizi cosi' individuati vengono impostati tra i servizi del
   treno (variabile ServiziTreno).
   
   A cambio treno chiamo "FineNote" per correggere i servizi in base ad
   alcune regole (es: i treni esteri non si prenotano) e per aggiungere
   i servizi non legati alla presenza di note.

*/
//----------------------------------------------------------------------------
void PTRENO::Load(){
   #undef TRCRTN
   #define TRCRTN "PTRENO::Load"
   
   CLASSIFICA_TRENI::CaricaClassifiche(PATH_DATI);
   // Apro l' archivio TABORARI, Localita' ed INFOCOMM
   STRINGA PathDa(PATH_IN),ExtFl(".T");
   if (getenv("USADATIESTRATTI")){ PathDa=PATH_OUT;ExtFl=".XTR";};
   FILE_TABORARI TabOr(PathDa + "TABORARI" + ExtFl);
   FILE_LOCALITA Loc(PathDa + "LOCALITA" + ExtFl);
   FILE_INFOCOMM InfoComm(PathDa + "INFOCOMM" + ExtFl);
   
   // Preparo l' archivio Servizi
   FILE_SERVNU FServNu(64000);
   FServNu.Clear("Servizi non uniformi dei treni");
   // Preparo l' archivio Note Commerciali
   FILE_NOTA_CO FNotaCom(64000);
   FNotaCom.Clear("Note commerciali (per treno)");
   
   
   Bprintf("Scansione TABORARI");
   int Cont3=0, Cont5 = 0, LastTr = 0;
   PTRENO Treno;
   BOOL PrimoRec5 = TRUE;
   Treno.Clear();
   int NumeroTreno = 0;
   ELENCO_S CategorieNonNote ;
   ORD_FORALL(TabOr,j){
      TABORARI & To = TabOr[j];
      int Tr = To.TipoRecord - '0'; if (Tr < 2 || Tr > 7)Tr = 0;
      switch (Tr) {   // Gestione differenti tipi record
      case 2:  // Record di tipo 2
         if (Cont3){
            Treno.PeriodicitaTreno &= T_PERIODICITA::InLimitiOrario;
            Treno.CcrA  = NESSUNA_STAZIONE;
            PTRENO & Pt = * HashPt.Alloca();
            Pt = Treno;
            HashPt.Metti(sizeof(IDTRENO));
         };
         Cont3 = 0;
         Treno.Clear();
         Treno.TProv         = To.R2.IdentTrenoProvenienza   ; // Identificativo Treno Provenienza
         Treno.TDestinazione = To.R2.IdentTrenoDestinazione  ; // Identificativo Treno Destinazione
         Treno.PTaborariR2   = j;
         Treno.NumeroTreno   = ++ NumeroTreno;
         PrimoRec5 = TRUE;
         break;
      case 3:       // Record di tipo 3
         {
            EsaminaNoteGeneriche(To, Treno.ServiziTreno);
            if ( Cont3 ++ ) continue;
            Treno.IdTreno  = To.R3.IdentTreno;
            ByteCopy(Treno.KeyTreno, To.R3.KeyTreno);
            ByteCopy(Treno.NomeMezzoViaggiante, To.R3.Denominazione);
            Treno.Flags    = To.R3.Flags;
            BYTE Tipom = MM_INFO::EncodTipoMezzo(STRINGA(St(To.R3.Categoria)));
            if (Tipom == 0xff) {
               Treno.ServiziTreno.TipoMezzo = 0;
               if (!CategorieNonNote.Contiene(St(To.R3.Categoria)) ){
                  CategorieNonNote += St(To.R3.Categoria);
                  Bprintf("Errore: categoria non nota %s del treno %s",St(To.R3.Categoria), St(Treno.IdTreno) );
               } /* endif */
            } else {
               Treno.ServiziTreno.TipoMezzo = Tipom;
            } /* endif */
            Treno.ServiziUniformiTreno.TipoMezzo = Treno.ServiziTreno.TipoMezzo ;
            Treno.ServiziUniformiTreno.HaNoteGeneriche = Treno.ServiziTreno.HaNoteGeneriche ;
         }
         break;
//<<< switch  Tr      // Gestione differenti tipi record
      case 4:
         if ( Cont3 != 1 ) continue;
         Treno.PeriodicitaTreno.ComponiPeriodicita(5,To.R4.Periodicita,(LastTr != 4));
         break;
      case 5:
         EsaminaNoteGeneriche(To, Treno.ServiziTreno);
         Treno.ServiziUniformiTreno.HaNoteGeneriche = Treno.ServiziTreno.HaNoteGeneriche ;
         if (PrimoRec5) {
            PrimoRec5 = FALSE;
            Treno.PLocalita  = Cont5 ;
            Treno.OraPartenzaTreno = To.R5.Partenza.MinMz(); // Anche se e' solo un transito
         } /* endif */
         Cont5 ++;
         break;
      } /* endswitch */
      LastTr=Tr;
//<<< ORD_FORALL TabOr,j  
   } /* endfor */
   if (Cont3){
      Treno.PeriodicitaTreno &= T_PERIODICITA::InLimitiOrario;
      Treno.CcrA  = NESSUNA_STAZIONE;
      PTRENO & Pt = * HashPt.Alloca();
      Pt = Treno;
      HashPt.Metti(sizeof(IDTRENO));
   };
   
   Bprintf("Scansione LOCALITA");
   PTRENO * PTr=NULL;
   ORD_FORALL(Loc,k){
      LOCALITA & Lo = Loc[k];
      if (PTr == NULL || PTr->IdTreno != Lo.IdentTreno) {
         PTr = Get(Lo.IdentTreno);
         assert2(PTr != NULL,Lo.IdentTreno);
         assert(k == PTr->PLocalita);
         PTr->CcrDa = Lo.Codice;
      } /* endif */
      PTr->CcrA = Lo.Codice;
   } /* endfor */
   
   // Controllo nella Hash table che i treni origine e destinazione siano validi, scartando i treni non esistenti.
   ITERA(HashPt, Tco, PTRENO ){
      // TRACESTRING( VRS(Tco.IdTreno) + VRS(Tco.TProv) + VRS(Tco.TDestinazione));
      BOOL Internazionale;
      if ( Tco.TProv != NESSUN_TRENO){
         PTr = Get(Tco.TProv);
         if(PTr == NULL ){
            Internazionale  = strstr(St(Tco.TProv), "*INT")!= NULL;
            Internazionale |= strstr(St(Tco.TProv), "*EST")!= NULL;
            if(!Internazionale)Bprintf("Errore: Treno provenienza %s del treno %s e' inesistente",St(Tco.TProv), St(Tco.IdTreno));
            Tco.TProv = NESSUN_TRENO ;
         } else {
            PTr->NumProvenienza ++;
            if (PTr->CcrA  == Tco.CcrDa ) PTr->NumProvenienzaDiTesta  ++;
         };
      };
      if ( Tco.TDestinazione != NESSUN_TRENO ){
         PTr = Get(Tco.TDestinazione) ;
         if (PTr == NULL ){
            Internazionale  = strstr(St(Tco.TDestinazione), "*INT")!= NULL;
            Internazionale |= strstr(St(Tco.TDestinazione), "*EST")!= NULL;
            if(!Internazionale)Bprintf("Errore: Treno destinazione %s del treno %s e' inesistente",St(Tco.TDestinazione), St(Tco.IdTreno));
            Tco.TDestinazione = NESSUN_TRENO ;
         } else {
            PTr->NumDestinazione ++;
            if (PTr->CcrDa == Tco.CcrA) PTr->NumDestinazioneDiTesta  ++;
         } /* endif */
      };
//<<< ITERA HashPt, Tco, PTRENO   
   } END_ITERA;
   
   // Scansione INFOCOMM: Identifico le note definite per ogni treno
   Bprintf("Scansione INFOCOMM");
   PTRENO * TrenoCorrente = NULL;
   // Prima scansione: Mi serve solo per individuare le note possedute dai treni
   ORD_FORALL(InfoComm,i1){
      INFOCOMM & Ic = InfoComm[i1];
      if (Ic.TipoRecord == '1') TrenoCorrente = PTRENO::Get(Ic.R1.IdentTreno);
//<<< if   Tco.TDestinazione != NESSUN_TRENO   
      else if (Ic.TipoRecord == '2') TrenoCorrente->HaNota(It(Ic.R2.Nota));
   };
   
   
   ARRAY_DINAMICA<SERVNU> TServNu(100);
   TrenoCorrente = NULL;
   ORD_FORALL(InfoComm,i2){
      INFOCOMM & Ic = InfoComm[i2];
      if (Ic.TipoRecord == '2'){
         FULL_INFO Serv = TrenoCorrente->Servizio(InfoComm); // Identifica i servizi della nota
         NOTA_CO Not;
         Not.Clear();
         Not.IdTreno = TrenoCorrente->IdTreno;
         Not.Nota.CodiceNota      = It(Ic.R2.Nota) ;
         Not.Nota.DiMezzoVirtuale = 0      ;
         // Gli altri dati sono gia' stati calcolati in Serv pertanto mi limito a copiarli
         Not.Nota.TipoNOT         = Serv.TipoSRV;
         Not.Nota.Da              = Serv.Da;
         Not.Nota.A               = Serv.A;
         Not.Nota.MaxDa           = Serv.MaxDa;
         Not.Nota.InProsecuzione  = Serv.InProsecuzione;
         Not.Nota.PeriodicitaNota = Serv.PeriodicitaServizi;
         BOOL AggiungiNota = Not.Nota.CodiceNota != 8    ;
         AggiungiNota |= !TrenoCorrente->HaNota_02_Ealtre;
         // La nota 8 deve essere visibile solo se non ho la nota 2 ...
         if(AggiungiNota)FNotaCom.AddRecordToEnd(VRB(Not));
         TrenoCorrente->ServiziTreno |= Serv;
         if(Serv.Empty()){
            // OK : E' probabilmente la nota 7 che indica la carrozza diretta. Non debbo fare nulla
         } else if (Serv.Uniforme()) {
            TrenoCorrente->ServiziUniformiTreno |= Serv;
         } else {
            SERVNU ServNu;
            ServNu.Clear();
            ServNu.IdTreno = TrenoCorrente->IdTreno;
            ServNu.Servizi = Serv;
            ServNu.Servizi.HaNoteDiVendita = 0; // L' informazione non mi serve ed anzi mi crea problemi
            TServNu += ServNu;
         } /* endif */
//<<< if  Ic.TipoRecord == '2'  
      } else if (Ic.TipoRecord == '1' || i2 == InfoComm.Dim() -1) {
         if(TrenoCorrente){
            TrenoCorrente->FineNote()   ;   // Completa la determinazione dei servizi del treno
            FORALL(TServNu,i){
               SERVNU & ServNu = TServNu[i];
               // Ovvia regola: Non posso avere un servizio disuniforme se non ho un servizio (permette correzioni pragmatiche ...)
               ServNu.Servizi &= TrenoCorrente->ServiziTreno;
               // Scarico solo i servizi non uniformi
               // NB: I flags di disuniformita' non sono impostati in quanto su tale
               // file sono riportati SOLO i servizi non uniformi
               ServNu.Servizi.NotIn(TrenoCorrente->ServiziUniformiTreno,FALSE);
               if (!ServNu.Servizi.EmptyServizi()) {
                  FServNu.AddRecordToEnd(VRB(ServNu));
                  TrenoCorrente->ServiziUniformiTreno.HaNoteDiVendita = 1;
               }
            }
         }
         if (Ic.TipoRecord == '1' ){
            TServNu.Clear();
            TrenoCorrente = PTRENO::Get(Ic.R1.IdentTreno) ;
            TrenoCorrente->PInfocomm = i2                 ;   // Identificatore treno
         }
//<<< if  Ic.TipoRecord == '2'  
      }
//<<< ORD_FORALL InfoComm,i2  
   };
   
   // A questo punto i treni che non hanno note di vendita non hanno ancora servizi
   // Scandisco percio' i treni e carico i default
   ITERA(HashPt, MVg, PTRENO ){
      if(!MVg.ServiziUniformiTreno.HaNoteDiVendita){
         MVg.FineNote();   // Completa la determinazione dei servizi del treno
      }
   } END_ITERA;
   
   // Sort Finale
   FServNu.ReSortFAST();
   FNotaCom.ReSortFAST();
   
//<<< void PTRENO::Load   
};

//----------------------------------------------------------------------------
// PTRENO::FineNote
//----------------------------------------------------------------------------
// Completa la determinazione dei servizi del treno
//   - Aggiunge i servizi che non dipendono dalle note
//   - Applica delle regole di business
//----------------------------------------------------------------------------
void PTRENO::FineNote(){
   #undef TRCRTN
   #define TRCRTN "PTRENO::FineNote"
   
   
   if (ServiziTreno.Navale() ) {
      // I traghetti non hanno prima o seconda classe
      ServiziTreno.PostiASederePrima    = 0;
      ServiziTreno.PostiASedereSeconda  = 0;
   } else if(HaNota_28) {
      ServiziTreno.PostiASederePrima    = 0;
      ServiziTreno.PostiASedereSeconda  = 1;
   } else if (HaNote_53_ed_altre || (HaNota_55 && HaNote_ServiziODiInstradamento) ) {
      ServiziTreno.PostiASederePrima    = 0;
      ServiziTreno.PostiASedereSeconda  = 0;
   } else if(Flags.PrimaClasse  != '*'){
      if(Flags.SecondaClasse != '*'){
         Bprintf("Anomalia: Mancanza di flags sia di prima che di seconda classe per il treno %s",(CPSZ)IdTreno);
      } else {
         ServiziTreno.PostiASederePrima    = 0;
         ServiziTreno.PostiASedereSeconda  = 1;
         ServiziUniformiTreno.PostiASedereSeconda  = 1;
      }
   } else {
      if(Flags.SecondaClasse != '*'){
         ServiziTreno.PostiASederePrima    = 1;
         ServiziUniformiTreno.PostiASederePrima    = 1;
         ServiziTreno.PostiASedereSeconda  = 0;
      } else {
         ServiziTreno.PostiASederePrima    = 1;
         ServiziUniformiTreno.PostiASederePrima    = 1;
         ServiziTreno.PostiASedereSeconda  = 1;
         ServiziUniformiTreno.PostiASedereSeconda  = 1;
      } /* endif */
//<<< if  ServiziTreno.Navale      
   } /* endif */
   
   const CLASSIFICA_TRENI & Cla = Classifica(ServiziTreno.TipoMezzo);
   ServiziTreno.Fumatori          =      Cla.Fumatori                    ;
   ServiziTreno.Prenotabilita    |=      Cla.SicuramentePrenotabile      ;
   ServiziTreno.Prenotabilita    |=      Cla.SicuramentePrenotazioneObbli;
   ServiziTreno.PrenObbligItalia |=      Cla.SicuramentePrenotazioneObbli;
   ServiziTreno.Supplemento      |=      Cla.SupplementoIC               ;
   ServiziTreno.Supplemento      |=      Cla.SupplementoEC               ;
   ServiziTreno.Supplemento      |=      Cla.SupplementoETR              ;
   ServiziTreno.PrenotDisUniforme&=     !Cla.SicuramentePrenotabile      ;
   ServiziUniformiTreno.Fumatori          =      Cla.Fumatori                    ;
   ServiziUniformiTreno.Prenotabilita    |=      Cla.SicuramentePrenotabile      ;
   ServiziUniformiTreno.Prenotabilita    |=      Cla.SicuramentePrenotazioneObbli;
   ServiziUniformiTreno.PrenObbligItalia |=      Cla.SicuramentePrenotazioneObbli;
   ServiziUniformiTreno.Supplemento      |=      Cla.SupplementoIC               ;
   ServiziUniformiTreno.Supplemento      |=      Cla.SupplementoEC               ;
   ServiziUniformiTreno.Supplemento      |=      Cla.SupplementoETR              ;
   
   if (HaNota_08 && ! HaNota_02_Ealtre) {
      ServiziTreno.Invalidi = 1;
      ServiziUniformiTreno.Invalidi = 1;
   } /* endif */
   
   ServiziTreno.ServizioBase = ServiziTreno.PostiASedereSeconda | ServiziTreno.CuccetteSeconda | ServiziTreno.Navale() ;
   ServiziUniformiTreno.ServizioBase = ServiziUniformiTreno.PostiASedereSeconda | ServiziUniformiTreno.CuccetteSeconda | ServiziTreno.Navale();
   
   // Controlli generali
   if( HaNote_Altre_AutoAlSeguito && ! HaNota_30 )
      Bprintf("Anomalia: Il treno %s ha un carro auto al seguito ma non ha la nota 30 ! Servizi: %s",St(IdTreno),(CPSZ)ServiziTreno.Decodifica());
   else assert3(HaNota_30 == ServiziTreno.AutoAlSeguito , Bprintf("%s",St(IdTreno)));
   
   if( HaNote_Altre_WL            && ! HaNota_04 )
      Bprintf("Anomalia: Il treno %s ha una carrozza WL ma non ha la nota 4 ! Servizi: %s",St(IdTreno),(CPSZ)ServiziTreno.Decodifica());
   else assert3(HaNota_04 == (ServiziTreno.VagoniLettoPrima || ServiziTreno.VagoniLettoSeconda), Bprintf("%s",St(IdTreno)));
   
   if( HaNote_Altre_CC            && ! HaNota_54 )
      Bprintf("Anomalia: Il treno %s ha una carrozza cuccette ma non ha la nota 54 ! Servizi: %s",St(IdTreno),(CPSZ)ServiziTreno.Decodifica());
   else assert3(HaNota_54 == ServiziTreno.CuccetteSeconda, Bprintf("%s",St(IdTreno)));
   
   ServiziTreno.Disuniforme = ServiziTreno.ServTraspDisUniformi || ServiziTreno.ServGenerDisUniformi || ServiziTreno.PrenotDisUniforme;
   
   // Ovvia regola: Non poso avere un servizio uniforme se non ho un servizio (permette correzioni pragmatiche ...)
   ServiziUniformiTreno &= ServiziTreno;
   
//<<< void PTRENO::FineNote   
};

//----------------------------------------------------------------------------
// PTRENO::Servizio
//----------------------------------------------------------------------------
// Ritorna il/i servizi implicati dalla presenza di una data nota, per
// un dato treno.
//----------------------------------------------------------------------------
// NB: deve caricare comunque i dati generali di FULL_INFO, perche' utilizzati anche
// dalle note
FULL_INFO PTRENO::Servizio(FILE_INFOCOMM & InfoComm){
   #undef TRCRTN
   #define TRCRTN "PTRENO::Servizio"
   
   INFOCOMM & Ic = InfoComm.RecordCorrente();
   assert(Ic.TipoRecord == '2');
   
   ID Nota = It(Ic.R2.Nota);
   
   FULL_INFO Wrk;
   Wrk.Clear();
   Wrk.PeriodicitaServizi = T_PERIODICITA::InLimitiOrario;
   Wrk.Da       = Ic.R2.StazioneInizioServizio ;
   Wrk.A        = Ic.R2.StazioneFineServizio ;
   Wrk.MaxDa    = Ic.R2.UltimaStazioneSalitaServ;
   if (Wrk.Da.Empty()) {                          // E' abbastanza anomalo
      if (Wrk.A.Empty()) {                        // Ok lo posso considerare un servizio di tutto il treno
         Wrk.TipoSRV = 0;                         // Tutto il mezzo viaggiante
      } else {
         Bprintf("Caso anomalo!");
         BEEP;
      };
   } else if (Wrk.A.Empty()) {
      Wrk.TipoSRV = 1;                            // Tipo servizio relativo alla sola stazione di partenza
      assert(Wrk.MaxDa.Empty());
   } else if ( CcrDa == Wrk.Da && CcrA == Wrk.A && Wrk.MaxDa.Empty() ){
      Wrk.TipoSRV = 0;                            // Tutto il mezzo viaggiante
   } else {
      Wrk.TipoSRV = 3;                            // Da stazione a stazione e tutte le intermedie
   }
   Wrk.InProsecuzione =  (
      Ic.R2.TrenoInizioServizioSeMV != NESSUN_TRENO &&
      Get(Ic.R2.TrenoInizioServizioSeMV) != NULL
   ) || (
      Ic.R2.TrenoFineServizioSeMV != NESSUN_TRENO &&
      Get(Ic.R2.TrenoFineServizioSeMV) != NULL  );
   
   // Per la periodicita' debbo scandire i records di tipo 3
   char LastTr = Ic.TipoRecord;
   ULONG P1 = InfoComm.NumRecordCorrente();
   for ( InfoComm.Next(); &InfoComm.RecordCorrente()!= NULL && InfoComm.RecordCorrente().TipoRecord == '3'; InfoComm.Next() ) {
      INFOCOMM & Rec = InfoComm.RecordCorrente();
      Wrk.PeriodicitaServizi.ComponiPeriodicita(5,Rec.R3.Periodicita,(LastTr != '3'));
      LastTr = Rec.TipoRecord;
   } /* endfor */
   InfoComm.Posiziona(P1);
   // Se non e' significativa ignoro la periodicita'
   if (Wrk.PeriodicitaServizi == PeriodicitaTreno) Wrk.PeriodicitaServizi = T_PERIODICITA::InOrario;
   
   Wrk.HaNoteDiVendita = 1;
   
   // Queste sono le informazioni generali del treno
   switch (Nota) {
      // ------  Servizi diretti
   case 4:
      Wrk.VagoniLettoPrima    = Flags.PrimaClasse   == '*';
      Wrk.VagoniLettoSeconda  = Flags.SecondaClasse == '*';
      Wrk.Prenotabilita = 1;
      assert(Flags.PrimaClasse   == '*' || Flags.SecondaClasse == '*' );
      break;
   case 7:
      // Di per se non individua nessun servizio, ma solo una carrozza diretta
      // Tuttavia in assenza di altre note indica posti a sedere
      if ( HaNote_53_ed_altre || HaNota_55 || HaNota_04 || HaNota_28
         || HaNota_30 || HaNota_54 || HaNote_Altre_AutoAlSeguito
         || HaNote_Altre_WL || HaNote_Altre_CC ) {
         // Do Nothing
      } else {
         if(Flags.SecondaClasse == '*') Wrk.PostiASedereSeconda  = 1;
         if(Flags.PrimaClasse   == '*') Wrk.PostiASederePrima    = 1;
      }
      break;
   case 30:
      Wrk.AutoAlSeguito = 1;
      Wrk.Prenotabilita = 1;
      Wrk.TipoSRV       = 4;  // Da stazione a Stazione ma non le intermedie
      assert(Wrk.MaxDa.Empty());
      break;
//<<< switch  Nota   
   case 54:
      Wrk.CuccetteSeconda     = 1;
      Wrk.Prenotabilita = 1;
      break;
      // ------  Altre note particolari sui servizi
   case 28:
      Wrk.PostiASedereSeconda  = 1;
      break;
   case 57:
      if(HaNota_28 ){
         ServiziTreno.PostiASedereSeconda  = 1;
      } else {
         if(Flags.PrimaClasse != '*'  ) ServiziTreno.PostiASederePrima    = 1;
         if(Flags.SecondaClasse != '*') ServiziTreno.PostiASedereSeconda  = 1;
      } /* endif */
      Wrk.VagoniLettoPrima    = Flags.PrimaClasse   == '*';
      Wrk.VagoniLettoSeconda  = Flags.SecondaClasse == '*';
      Wrk.Prenotabilita = 1;
      break;
      // ------  Auto al seguito
   case   41:
//<<< switch  Nota   
   case   42:
   case   43:
   case   44:
   case   62:
   case  131:
   case  151:
   case  152:
   case  153:
   case  154:
   case  155:
   case  156:
   case  157:
   case  158:
   case  159:
   case  160:
   case  728:
      Wrk.AutoAlSeguito = 1;
      Wrk.Prenotabilita = 1;
      Wrk.TipoSRV       = 4;  // Da stazione a Stazione ma non le intermedie
      assert(Wrk.MaxDa.Empty());
      break;
      // ------  Biciclette
//<<< switch  Nota   
   case    3:
      Wrk.Biciclette = 1;
      break;
   case   23:
      Wrk.TipoSRV       = 4;  // Da stazione a Stazione ma non le intermedie
      assert(Wrk.MaxDa.Empty());
      Wrk.Biciclette = 1;
      break;
   case 1230:
      Wrk.Biciclette = 1;
      Wrk.TipoSRV       = 2; // 2 = Soppresso alla fermata indicata
      assert(Wrk.MaxDa.Empty());
      break;
      // ------  Handicap
   case    2:
   case   21:
   case 1226:
      Wrk.Invalidi = 1;
      break;
   case 8:
      if (!HaNota_02_Ealtre) {
         Wrk.Invalidi = 1;
      } /* endif */
      break;
      // ------  Servizi Varii
//<<< switch  Nota   
   case   14:
      Wrk.Ristorante = 1;
      break;
   case   15:
   case   27:
      Wrk.RistoBar  = 1;
      break;
   case 16:
      Wrk.SelfService = 1;
      break;
   case 17:
      Wrk.Ristoro = 1;
      break;
   case 80:
      Wrk.TrenoVerde = 1;
      break;
      // ------  Prenotazione
   case   12:
   case   31:
      Wrk.PrenObbligItalia    = 1;
      Wrk.Prenotabilita       = 1;
      break;
//<<< switch  Nota   
   case   34:
   case   37:
      Wrk.PrenObbligEstero    = 1;
      Wrk.Prenotabilita       = 1;
      break;
   case   29:
      Wrk.PrenotabileSolo1Cl  = 1;
      Wrk.Prenotabilita       = 1;
      break;
   case   13:
   case   48:
   case   49:
   case 1192:
   case 1193:
      Wrk.Prenotabilita       = 1;
      break;
      // ------  Le altre note non implicano DIRETTAMENTE servizi
   } /* endswitch */
   return Wrk;
//<<< FULL_INFO PTRENO::Servizio FILE_INFOCOMM & InfoComm  
};

//----------------------------------------------------------------------------
// PTRENO::HaNota
//----------------------------------------------------------------------------
// Registra la presenza di una nota nelle variabili HaNota...
//----------------------------------------------------------------------------
void    PTRENO::HaNota(ID Nota) {
   #undef TRCRTN
   #define TRCRTN "PTRENO::HaNota"
   
   
   HaNote = 1;
   switch (Nota) {
      // ------  Handicap
   case    2:
   case   21:
   case 1226:
      HaNota_02_Ealtre = 1;
      break;
   case 8:
      HaNota_08 = 1;
      break;
      // ------  Servizi diretti
   case    4:
      HaNota_04 = 1;
      break;
   case    7:
      HaNota_07 = 1;
      break;
   case   30:
      HaNota_30 = 1;
      break;
   case   54:
      HaNota_54 = 1;
      break;
      // ------  Posti a sedere : Mancanza
//<<< switch  Nota   
   case   55:
      HaNota_55 = 1;
      HaNote_Altre_CC = 1;
      HaNote_Altre_WL = 1;
      break;
   case   56:
   case   58:
      HaNote_Altre_CC = 1;
      HaNote_Altre_WL = 1;
      HaNote_53_ed_altre = 1 ;
      break;
   case   53:
      HaNote_Altre_WL = 1;
      HaNote_53_ed_altre = 1 ;
      break;
   case  173:
   case  232:
      HaNote_53_ed_altre = 1 ;
      HaNote_Altre_CC = 1;
      break;
      // ------  Altre note particolari sui posti a sedere, letti e cuccette
//<<< switch  Nota   
   case   57:
      HaNote_Altre_WL = 1;
      break;
   case   28:
      HaNota_28 = 1;
      break;
      // ------  Auto al seguito
   case   60:
   case   61:
   case   62:
      HaNote_Altre_CC = 1;
      HaNote_Altre_WL = 1;
      HaNote_Altre_AutoAlSeguito = 1;
      break;
   case   41:
   case   42:
   case   43:
   case   44:
   case  131:
   case  151:
   case  152:
//<<< switch  Nota   
   case  153:
   case  154:
   case  155:
   case  156:
   case  157:
   case  158:
   case  159:
   case  160:
   case  728:
      HaNote_Altre_AutoAlSeguito = 1;
      break;
      // ------  Biciclette
   case    3:
   case   23:
   case 1230:
      // ------  Prenotazione
   case   12:
   case   13:
   case   29:
   case   31:
   case   34:
//<<< switch  Nota   
   case   37:
   case   48:
   case   49:
   case 1192:
   case 1193:
      // ------  Servizi Varii
   case   14:
   case   15:
   case   16:
   case   17:
   case   27:
   case   80:
      // ------  Note di Instradamento
   case 1177:
   case 1178:
      break;
   default:
      HaNote_Altre = 1;
      return;  // Per non settare HaNote_ServiziODiInstradamento
   } /* endswitch */
   HaNote_ServiziODiInstradamento = 1;
//<<< void    PTRENO::HaNota ID Nota   
};

//----------------------------------------------------------------------------
// PTRENO_sort_function
//----------------------------------------------------------------------------
int PTRENO_sort_function( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "PTRENO_sort_function"
   PTRENO & A = *(PTRENO*)a;
   PTRENO & B = *(PTRENO*)b;
   return (int)A.NumeroTreno - (int)B.NumeroTreno ;
};

//----------------------------------------------------------------------------
// PTRENO::Save
//----------------------------------------------------------------------------
// Salva i dati della hash su di un file
//----------------------------------------------------------------------------
void PTRENO::Save(){
   #undef TRCRTN
   #define TRCRTN "PTRENO::Save"
   
   if(PLocal != NULL){
      delete PLocal;
      delete PTabOr;
      PLocal = NULL;
      PTabOr = NULL;
   };
   
   FILE_PTRENO File(64000);
   File.Clear("Dati dei treni");
   ITERA(HashPt, Rec, PTRENO) {
      File.AddRecordToEnd(VRB(Rec));
   } END_ITERA
   File.ReSort(PTRENO_sort_function,TRUE);
};
//----------------------------------------------------------------------------
// PTRENO::Restore
//----------------------------------------------------------------------------
// Ristora i dati della hash a partire dal file salvato da Save()
//----------------------------------------------------------------------------
void PTRENO::Restore(){
   #undef TRCRTN
   #define TRCRTN "PTRENO::Restore"
   CLASSIFICA_TRENI::CaricaClassifiche(PATH_DATI);
   HashPt.Clear();
   FILE_PTRENO File(64000);
   ORD_FORALL(File,i){
      PTRENO & Pt = * HashPt.Alloca();
      Pt = File[i];
      HashPt.Metti(sizeof(Pt.IdTreno));
   };
};
