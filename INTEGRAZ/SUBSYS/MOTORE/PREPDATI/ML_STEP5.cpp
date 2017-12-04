//----------------------------------------------------------------------------
// Caricamento dati delle note e nomi dei treni
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

// EMS001 Win
typedef unsigned long BOOL;

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "ML_IN.HPP"
#include "ML_WRK.HPP"
#include "seq_proc.hpp"
#include "mm_basic.hpp"
#include "scandir.h"
#include "mm_detta.hpp"

#define PGM      "ML_STEP5"

SET NoteUtilizzate;
WORD MaxNota = 0;
int NumErr = 0;

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int  main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"

   int Rc = 0;

   // ---------------------------------------------------------
   // Init
   // ---------------------------------------------------------
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   /* EMS002 Win
   SetStatoProcesso(PGM, SP_PARTITO);
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
   SetPriorita(); // Imposta la priorita'
   */

   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore
   TryTime(0);

   FILE_RW OutF(PATH_OUT PGM ".OUT");
   OutF.SetSize(0);
   AddFileToBprintf(&OutF);

   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");

   ARRAY_T_PERIODICITA AllPeriodicita;

   AllPeriodicita.Get(PATH_OUT "PERIODIC.SV1"); // Gestisce file non esistente o vuoto

   PROFILER::Clear(FALSE); // Per aprire il grafo
   // APRO il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);

   // ---------------------------------------------------------
   // Scarico del file dei testi delle note
   // ---------------------------------------------------------
   FILE_DATI_NOTE TestiNote(PATH_OUT "NOTEPART.TMP");
   IDX_TESTI_NOTE OutNote(PATH_OUT "MM_NOTE.DB");
   FILE_RW        OutNote2(PATH_OUT "MM_NOTE.EXT");
   OutNote.Clear(); OutNote2.SetSize(0);
   int LastNota     = -1;
   int LastLanguage = -1;
   IDX_TESTI_NOTE::ITN_R_STRU OutRec;  // EMS003 VA
   STRINGA   Out;

   ORD_FORALL(TestiNote,i){
      FILE_DATI_NOTE::FDN_R_STRU Rec = TestiNote[i];  // EMS004 VA
      if ( Rec.IdNota != LastNota || Rec.Linguaggio != LastLanguage) {
         if (LastNota != -1){
            OutNote.AddRecordToEnd(VRB(OutRec));
            OutNote2.Scrivi((CPSZ)Out,(ULONG)Out.Dim()+1);
         }
         OutRec.IdNota           =    Rec.IdNota;
         OutRec.Linguaggio       =    Rec.Linguaggio;
         OutRec.DimDati          =    0;
         OutRec.OffsetDati       =    OutNote2.FileSize();
         OutRec.IdxPeriodicita   =    AllPeriodicita.Indice(Rec.Periodicita); // converto le periodicita' dalla forma estesa ad indice
         LastNota     = Rec.IdNota    ;
         LastLanguage = Rec.Linguaggio;
         Out.Clear();
      } /* endif */
      if(Out.Dim())Out += " ";
      Out += Rec.Testo; // Gia' fatto uno strip
      OutRec.DimDati  = Out.Dim()+1;
   }
   if (LastNota != -1){
      OutNote.AddRecordToEnd(VRB(OutRec));
      OutNote2.Scrivi((CPSZ)Out,(ULONG)Out.Dim()+1);
   }
   OutNote.ReSortFAST();
   AllPeriodicita.Put(PATH_OUT "PERIODIC.SV1");  // Aggiorno il file con le varie periodicita'

   TRACEVLONG(OutNote.FileSize());
   TRACEVLONG(OutNote.Dim());
   TRACEVLONG(OutNote2.FileSize());
   TRACEVLONG(AllPeriodicita.Dim());


   // Carico da file le note eccezionali, poco importanti. In pratica sono quelle gia' implicite nei servizi
   ARRAY_ID Eccezioni;
   {
      FILE_RO F_Eccezioni(PATH_IN "NOTE_BAN.DAT");
      STRINGA Linea;
      while(F_Eccezioni.gets(Linea)) {
         Linea.Strip();
         if(Linea[0] == '*')continue; // Commento
         ELENCO_S Toks = Linea.Strip().Tokens(" ");
         ID Nota = Toks[0].ToInt();
         if(Nota){
            BOOL Ignorare    = Toks[1] != NUSTR && Toks[1][0] == '!';
            BOOL DueStaz     = Toks[1] != NUSTR && Toks[1][0] == 'õ';
            BOOL DueStazIgn  = Toks[1] != NUSTR && Toks[1][0] == '?';
            if(Ignorare){
               Eccezioni += Nota + 20000;
            } else if(DueStaz ){
               Eccezioni += Nota + 40000;
            } else if(DueStazIgn ){
               Eccezioni += Nota + 40000;
               Eccezioni += Nota;
            } else {
               Eccezioni += Nota;
            };

            NoteUtilizzate.Set(Nota);
            Top(MaxNota,Nota);
         }
      }
   }

   // ---------------------------------------------------------
   // Scarico dei files delle note di mezzo virtuale
   // ---------------------------------------------------------
   F_DETTAGLI_MV   Dettagli(PATH_OUT "MM_DETMV.DB");
   FILE_RW         Dettagli2(PATH_OUT "MM_DETMV.EXT");
   F_INFO_AUX      DatiAux(PATH_OUT "INFO_AUX.TMP" );
   F_MEZZO_VIAGG   MezziVg(PATH_OUT "MZVIAG.TMP" );
   F_FERMATE_VIRT  TabFv(PATH_OUT "M0_FERMV.TMP",2000);

   Dettagli.Clear();
   Dettagli2.SetSize(0);

   BUFR OutBuf;  // Dati di output
   BUFR OutBuf2; // Dati di output (Note non in evidenza)

   ARRAY_DINAMICA<INFO_AUX> Wrk; // Area di appoggio
   int LastMv = -1;

   int NumNomi = 0,SzNomi = 0;
   int NumNote = 0,SzNote = 0;
   int NumOut  = 0;
   int NumMv   = 0;
   int NumNot  = 0;
   int NumSrv  = 0;
   int NumNom  = 0;
   int NumCla  = 0;

   Bprintf("Vi sono %i records originali di dati ausiliari",DatiAux.Dim());

   ORD_FORALL(DatiAux,Idx){
      INFO_AUX & Aux = DatiAux.FixRec(Idx);
      switch (Aux.Tipo) {
      case AUX_NOME:       NumNom ++; break;
      case AUX_CLASSIFICA: NumCla ++; break;
      case AUX_NOTA:       NumNot ++; break;
      case AUX_SERVIZIO:   NumSrv ++; break;
      } /* endswitch */

      if ( Aux.MezzoVirtuale == LastMv ) { // Aggiungo il record all' array in memoria
         Wrk += Aux;
      } /* endif */

      // Preparazione dei dati a cambio di mezzo virtuale od a fine file
      if (Aux.MezzoVirtuale != LastMv || (Idx == DatiAux.Dim() -1)) {
         NumMv ++;
         // 1ø Scansione: scarico dei nomi
         FORALL(Wrk, i1){
            INFO_AUX & InfoAux = Wrk[i1];
            if(InfoAux.Tipo != AUX_NOME )continue;
            if(InfoAux.Info <=0 || InfoAux.Info > MezziVg.Dim() ){
               assert(InfoAux.Info > 0 && InfoAux.Info <= MezziVg.Dim() );
               continue;
            };
            Bprintf3("Scarico Nome, Id Treno = %i Mv %i, Mvg %i",InfoAux.Info, LastMv, InfoAux.Mvg);
            MEZZO_VIAGG & MVg = MezziVg[InfoAux.Info - 1];
            STRINGA Nome(St(MVg.NomeMezzoViaggiante));
            Nome.Strip();
            assert2(Nome.Dim() > 0, InfoAux.Info);
            NOME_MV Tmp;
            Tmp.Tipo     = NOME_MEZZO_VIRTUALE;
            Tmp.NumBytes = sizeof(NOME_MV) + Nome.Dim() ;
            Tmp.Mvg      = InfoAux.Mvg ;
            OutBuf.Store(&Tmp, sizeof(Tmp) -1);
            OutBuf.Store((CPSZ)Nome, Nome.Dim() + 1);
            NumNomi ++;
            SzNomi += Tmp.NumBytes;
         };

         // 2ø Scansione: scarico delle note
         // NB: Tolgo l' evidenza alle note BANALI
         FORALL(Wrk, i2){
            INFO_AUX & InfoAux = Wrk[i2];
            if(InfoAux.Tipo != AUX_NOTA ) continue;
            if(Eccezioni.Contiene(InfoAux.Id))InfoAux.Info = 0; // Le note banali non sono mai in evidenza
            NoteUtilizzate.Set(InfoAux.Id);
            if(Eccezioni.Contiene(InfoAux.Id + 20000))continue; // Nota da ignorare (non visualizzare)
            Top(MaxNota,InfoAux.Id);
            BOOL DueStaz = Eccezioni.Contiene(InfoAux.Id + 40000);
            if (InfoAux.DiFermata) {
               // Nota di fermata, senza periodicit…
               FERMATE_VIRT & Fv = TabFv.CercaCcrDiMv(InfoAux.MezzoVirtuale, InfoAux.Da);
               if(&Fv){
                  NOTA_FV Tmp;
                  Tmp.Tipo       = NOTA_FERMATA    ;
                  Tmp.Evidenza   = InfoAux.Info    ;
                  Tmp.DueStaz    = DueStaz         ;
                  Tmp.PrgFermata = Fv.Progressivo2 ;
                  Tmp.IdNota     = InfoAux.Id      ;
                  NumNote ++;
                  SzNote += sizeof(Tmp);
                  if (Tmp.Evidenza) {
                     OutBuf.Store(VRB(Tmp))           ;
                  } else {
                     OutBuf2.Store(VRB(Tmp))           ;
                  } /* endif */
               } else {
                  Bprintf2("Ignorata nota %i ed altre relative a fermata non identificabile CCR %i MV %i",InfoAux.Id,InfoAux.Da, LastMv);
                  // Disabilito tutte le note di questa stessa fermata
                  FORALL(Wrk,w){
                     INFO_AUX & Clr = Wrk[w];
                     if(InfoAux.Tipo != AUX_NOTA ) continue;
                     if(!Clr.DiFermata) continue;
                     if(Clr.Da != InfoAux.Da) continue;
                     Clr.Tipo = AUX_TAPPO  ; // Per non riutilizzare il dato
                  };
                  NumErr ++;
//<<<          if &Fv
               }
//<<<       if  InfoAux.DiFermata
            } else if (InfoAux.DiMvg) {
               // Nota di treno
               NOTA_MV Tmp;
               Tmp.Tipo           = NOTA_MZV            ;
               Tmp.Evidenza       = InfoAux.Info        ;
               Tmp.DueStaz        = DueStaz             ;
               Tmp.Mvg            = InfoAux.Mvg         ;
               Tmp.IdxPeriodicita = AllPeriodicita.Indice(InfoAux.Periodicita);
               Tmp.IdNota         = InfoAux.Id          ;
               NumNote ++;
               SzNote += sizeof(Tmp);
               if (Tmp.Evidenza) {
                  OutBuf.Store(VRB(Tmp))           ;
               } else {
                  OutBuf2.Store(VRB(Tmp))           ;
               }
            } else {
               // Nota complessa
               // Innanzitutto debbo individuare le fermate
               BYTE PrgDa=0,PrgA=0,PrgMaxDa=0;
               FERMATE_VIRT & FvDa    = TabFv.CercaCcrDiMv(InfoAux.MezzoVirtuale, InfoAux.Da   );
               if(& FvDa   ) PrgDa    = FvDa.Progressivo2 ;
               FERMATE_VIRT & FvA     = TabFv.CercaCcrDiMv(InfoAux.MezzoVirtuale, InfoAux.A    );
               if(& FvA    ) PrgA     = FvA.Progressivo2 ;
               FERMATE_VIRT & FvMaxDa = TabFv.CercaCcrDiMv(InfoAux.MezzoVirtuale, InfoAux.MaxDa);
               if(& FvMaxDa) PrgMaxDa = FvMaxDa.Progressivo2 ;
               // Si noti che i progressivi sono a 0 anche se la fermata esiste ma e' un transito (non nodale)

               if(PrgDa > 0 && ((InfoAux.TipoAUX != 3 && InfoAux.TipoAUX != 4) || PrgA > 0)){
                  NOTA_COMPLEX Tmp;
                  Tmp.Tipo           = NOTA_COMPLESSA      ;
                  Tmp.Evidenza       = InfoAux.Info        ;
                  Tmp.DueStaz        = DueStaz             ;
                  Tmp.TipoNota       = InfoAux.TipoAUX     ;
                  Tmp.IdxPeriodicita = AllPeriodicita.Indice(InfoAux.Periodicita);
                  Tmp.ProgDa         = PrgDa               ;
                  Tmp.ProgMaxDa      = PrgMaxDa            ;
                  Tmp.ProgA          = PrgA                ;
                  Tmp.IdNota         = InfoAux.Id          ;
                  NumNote ++;
                  SzNote += sizeof(Tmp);
                  if (Tmp.Evidenza) {
                     OutBuf.Store(VRB(Tmp))           ;
                  } else {
                     OutBuf2.Store(VRB(Tmp))           ;
                  }
               } else {
                  NumErr ++;
                  Bprintf2("Ignorata nota %i ed altre relative a tratta non identificabile CCR %i->%i MV %i", InfoAux.Id, InfoAux.Da, InfoAux.A, InfoAux.MezzoVirtuale);
                  // Disabilito tutte le note analoghe
                  FORALL(Wrk,w){
                     INFO_AUX & Clr = Wrk[w];
                     if(Clr.Tipo != AUX_NOTA )continue;
                     if(Clr.DiFermata)        continue;
                     if(Clr.DiMvg)            continue;
                                         if(Clr.Da != InfoAux.Da) continue;
                     if(Clr.A  != InfoAux.A ) continue;
                     if(Clr.Id != InfoAux.Id) continue;
                     Clr.Tipo = AUX_TAPPO  ; // Per non riutilizzare il dato
                  };
//<<<          if PrgDa > 0 && PrgA > 0
               }
//<<<       if  InfoAux.DiFermata
            } /* endif */
//<<<    FORALL Wrk, i2
         };

         // Unisco note banali e non
         OutBuf.Store(OutBuf2);
         OutBuf2.Clear();

         // Scarico dei dati
         DETTAGLI_MV Rec1;
         Rec1.IdMezzoVirtuale = LastMv;
         Rec1.DimDati         = OutBuf.Dim();
         Rec1.OffsetDati      = Dettagli2.FileSize();
         if(OutBuf.Dim()){
            Dettagli.AddRecordToEnd(VRB(Rec1));
            Dettagli2.Scrivi(OutBuf);
         };

         // Reset
         Wrk.Clear();
         OutBuf.Clear();
         LastMv = Aux.MezzoVirtuale ;
         Wrk += Aux;
//<<< if  Aux.MezzoVirtuale != LastMv ||  Idx == DatiAux.Dim   -1
      }

//<<< ORD_FORALL DatiAux,Idx
   }

   Bprintf("Scritti %i nomi di treni per %i Bytes, e %i note per %i Bytes", NumNomi , SzNomi  , NumNote , SzNote );
   Bprintf("Dati Originali:");
   Bprintf("   Numero dei mezzi virtuali con dati ausiliari: %i",NumMv);
   Bprintf("   Numero dei dati di tipo \"Nome\"            : %i",NumNom);
   Bprintf("   Numero dei dati di tipo \"Classifica\"      : %i",NumCla);
   Bprintf("   Numero dei dati di tipo \"Nota\"            : %i",NumNot);
   Bprintf("   Numero dei dati di tipo \"Servizi\"         : %i",NumSrv);

   Dettagli.ReSortFAST();

   // Scarico su AllNote.out le note utilizzate
   FILE_RW AllNote(PATH_OUT "ALLNOTE.Out");
   AllNote.SetSize(0);
   STRINGA Buffer,Linea;
   Buffer = "Elenco delle note Utilizzate : \r\n ";
   for (i = 0;i <= MaxNota ; i++) {
      if(NoteUtilizzate.Test(i)){
         Linea += STRINGA(i);
         if (Linea.Dim() > 70) {
            Linea += "\r\n ";
            Buffer += Linea;
            Linea.Clear();
         } else {
            Linea += ", ";
         } /* endif */
      } /* endif */
   } /* endfor */
   Linea += "\r\n";
   Buffer += Linea;
   AllNote.Scrivi((CPSZ)Buffer,Buffer.Dim());

   if(NumErr)Bprintf("Vi sono state %i anomalie",NumErr);

   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------

   TRACESTRING("PROGRAMMA TERMINATO");

   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;

   return Rc;

//<<< int  main int argc,char *argv
};
