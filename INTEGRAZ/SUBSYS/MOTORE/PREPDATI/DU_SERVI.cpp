//----------------------------------------------------------------------------
// DU_SERVI: DUMP dei varii files temporanei di note e servizi
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  100
#include "ft_paths.hpp"

#include "oggetto.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_OUT.HPP"
#include "elenco.h"
#include "eventi.h"
#include "seq_proc.hpp"
#include "file_t.hpp"

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int  main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"

   ARRAY_ID AllNote;

   ELENCO_S Ok ;

   printf(
      " Opzioni:\n"
      " yyy     : Mostra i servizi e le note del treno con nome yyy\n"
      " /F0     : Mostra le fermate (come in uscita da FT_STEP0)   \n"
      " /F1     : Mostra le fermate (Non Linearizzate         0)   \n"
      " /F2     : Mostra le fermate (    Linearizzate         0)   \n"
      " /P      : Mostra la periodicita' in formato esteso         \n"
      "           Si intendono le fermate di tutti i mezzi virtuali che hanno in composizione il treno\n"
      "\n\n"
      "Scrivo il risultato su %sDU_SERVI.TXT\n",PATH_DUMMY );

   int  Dettaglio = -1;
   BOOL PeriodicitaEstesa = FALSE;

   for (int a = 1;a < argc ; a ++ ) {
      STRINGA Tmp(argv[a]);
      if(Tmp[0] == '/' ){
         if(Tmp[1] == 'F' || Tmp[1] == 'f') Dettaglio = Tmp[2] - '0';
         if(Tmp[1] == 'P' || Tmp[1] == 'p') PeriodicitaEstesa = TRUE;
      } else {
         Tmp.Strip();
         Ok += Tmp;
      }
   } /* endfor */

   if(Ok.Dim() == 0){
      printf("Nessun treno specificato!\n");
      return 4;
   }

   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   PERIODICITA::ImpostaProblema( T_PERIODICITA::Inizio_Dati_Caricati, T_PERIODICITA::Inizio_Orario_FS, T_PERIODICITA::Fine_Orario_FS, T_PERIODICITA::Inizio_Dati_Caricati);

   // APRO il grafo
   PROFILER::Clear(FALSE); // Per aprire il grafo
   GRAFO::Grafo   = new GRAFO(PATH_DATI);

   ELENCO_S TipoSrv(
      "0=Di Fermata"
     ,"1=Tutto il MV"
     ,"2=Da stazione a stazione (e tutte le intermedie)"
     ,"3=Da stazione a Stazione (no intermedie)"
     ,"4=Su tutte le stazioni attrezzate"
     ,"5=NON ha il servizio per la data fermata"
   );

   FILE * Out;
   Out = fopen(PATH_DUMMY "DU_SERVI.TXT" ,"wt");

   FILE_SERVIZI_TRENO     AuxServizi(PATH_OUT "S_TRENO.TMP");
   FILE_SERVIZI           ServOut(PATH_OUT "F_SRVTRN.TMP",2048);
   FILE_DETTAGLIO_SERVIZI DetServ(PATH_OUT "F_DETSER.TMP",2048);
   F_IDENT_TRENO          FIdent(PATH_OUT "F_IDENT.TMP",2048);
   F_NOTE_TRENO           NoteT(PATH_OUT "F_NOTE_T.TMP",2048);
   F_NOTE_FERMATA         NoteF(PATH_OUT "F_NOTE_F.TMP",2048);
   F_NOTECOM_TRENO        NoteCom(PATH_OUT "F_NOTECM.TMP",2048);
   FILE_DATI_NOTE         DatiNote(PATH_OUT "NOTEPART.TMP");

   FILE_PERIODICITA_TRENO2 PTr(PATH_OUT "F_PTREN2.TMP", 2048);
   PERIODICITA_TRENO2 PTreno;
   F_MEZZO_VIAGGIANTE FileMV(PATH_OUT "F_MVIAG.TMP", 2048);
   MEZZO_VIAGGIANTE MViag;
   // EMIRO <<<<

   ORD_FORALL(Ok,Tr){
      printf("Elaborazione treno %s\n",(CPSZ)Ok[Tr]);
      int Id = -1;
      fprintf(Out, "=====================================================================\n");
      if (FIdent.Seek(Ok[Tr])) {
         Id = FIdent.RecordCorrente().NumeroMezzo;
         fprintf(Out,"Elaborazione treno %s ID = %i ",(CPSZ)Ok[Tr], Id);
      } else {
         fprintf(Out,"Elaborazione treno %s ID NON IDENTIFICATO\n",(CPSZ)(Ok[Tr]));
      } /* endif */

      // EMIRO >>>>
      // Questo blocco identifica le stazioni "da" "a" del mezzo fisico
      char it[11];
      Ok[Tr].ToFix(it,10); it[10] = '\0';

      if (FileMV.Seek(it)) {
         fprintf(Out," DA %-5s  ", FileMV.RecordCorrente().CCR1);
         fprintf(Out," A  %-5s\n", FileMV.RecordCorrente().CCR2);
      } else {
         fprintf(Out,"\nMezzo Viaggiante %s NON IDENTIFICATO su F_MVIAG.TMP\n",(CPSZ)(Ok[Tr]));
      } /* endif */

      fprintf(Out, "=====================================================================\n");
      T_PERIODICITA PFis;
      if(PeriodicitaEstesa){
         if (PTr.Seek(it)) {
            STRINGA Period;
            PFis = PTr.RecordCorrente().Periodicita;
            PERIODICITA_IN_CHIARO * PerIc = PTr.RecordCorrente().Periodicita.EsplodiPeriodicita();
            if (PerIc) {
               ELENCO_S Tmp =  PerIc->PeriodicitaLeggibile();
               ORD_FORALL(Tmp,t)Period += Tmp[t]+"; ";
               delete PerIc;
            } /* endif */
            fprintf(Out, "%s\n", (CPSZ)Period );

            ELENCO_S Lst = PTr.RecordCorrente().Periodicita.Decod();
            ORD_FORALL(Lst,n){
               fprintf(Out,"%s\n",(CPSZ)Lst[n]);
            };

         } else {
            fprintf(Out,"\nPeriodicita' del Mezzo Viaggiante %s NON IDENTIFICATA su F_PTREN2.TMP\n",(CPSZ)(Ok[Tr]));
         } /* endif */

//<<< if PeriodicitaEstesa
      } /* endif */
      // Vedo i dati dei files definiti su ft_aux.hpp
      int NumMatch ;

      // Servizi treni
      ORD_FORALL(AuxServizi,S1){
         SERVIZI_TRENO & Rec = AuxServizi[S1];
         STRINGA IdTreno = St(Rec.IdentTreno); IdTreno.Strip();
         if(IdTreno != Ok[Tr])continue;
         if(Rec.Uniforme){
            fprintf(Out,"Treno '%10.10s Servizi uniformi %s\n",Rec.IdentTreno,(CPSZ)Rec.Servizi.Decodifica());
         } else {
            fprintf(Out,"Treno %10.10s Servizi NON uniformi %s\n",Rec.IdentTreno,(CPSZ)Rec.Servizi.Decodifica());
         }
      }

      if(Id < 0)continue;

      // Servizi treni da F_SRVTRN
      if(ServOut.Seek(Id)){
         FILE_SERVIZI::R_STRU & Rec = ServOut.RecordCorrente();
         if(Rec.Uniforme){
            fprintf(Out,"Da F_SRVTRN: Servizi uniformi %s\n",(CPSZ)Rec.Servizi.Decodifica());
         } else {
            fprintf(Out,"Da F_SRVTRN: Servizi NON uniformi %s\n",(CPSZ)Rec.Servizi.Decodifica());
         }
      } else {
         fprintf(Out,"Servizi non trovati su file servizi (F_SRVTRN)\n");
      }
      // Periodicita' servizi treni
      if(DetServ.Seek(Id)){
         while (&DetServ.RecordCorrente() && DetServ.RecordCorrente().NumeroMezzoVg == Id){
            DETTAGLIO_SERVIZI & Rec = DetServ.RecordCorrente();
            STRINGA Period;
            PERIODICITA_IN_CHIARO * PerIc = Rec.Periodicita.EsplodiPeriodicita();
            if (PerIc) {
               ELENCO_S Tmp =  PerIc->PeriodicitaLeggibile();
               ORD_FORALL(Tmp,t)Period += Tmp[t]+"; ";
               delete PerIc;
            } /* endif */
            fprintf(Out,"Da F_DETSER: Tipo Servizi %s Servizi %s Da CCR %5.5s a CCR %5.5s Periodicita' %s\n",
               (CPSZ)TipoSrv[Rec.TipoSRV], (CPSZ)Rec.Servizi.Decodifica(), Rec.Ccr1, Rec.Ccr2, (CPSZ)Period);
            if(PeriodicitaEstesa && PFis != Rec.Periodicita){
               ELENCO_S Lst = Rec.Periodicita.Decod();
               ORD_FORALL(Lst,n) fprintf(Out,"%s\n",(CPSZ)Lst[n]);
            } /* endif */
            DetServ.Next();
         } /* endwhile */
      } else {
         fprintf(Out,"Dettaglio servizi non presente su F_DETSER\n");
      }

      // Note di treno
      if(!NoteT.Seek(Id)){
         fprintf(Out,"Il treno non ha note di treno\n");
      } else {
         ARRAY_ID Note;
         while( &NoteT.RecordCorrente()&& NoteT.RecordCorrente().NumeroMezzoVg == Id){
            Note += NoteT.RecordCorrente().NumNota;
            AllNote += Note;
            NoteT.Next();
         }
         fprintf(Out,"Note di treno: %s\n",(CPSZ)Note.ToStringa());
      } /* endif */

      // Note di fermata
      if(!NoteF.Seek(Id)){
         fprintf(Out,"Il treno non ha note di fermata\n");
      } else {
         while(&NoteF.RecordCorrente() && NoteF.RecordCorrente().NumeroMezzoVg == Id){
            fprintf(Out,"Fermata %i Nota: %i\n", NoteF.RecordCorrente().IdFermata , NoteF.RecordCorrente().NumNota   );
            AllNote += NoteF.RecordCorrente().NumNota;
            NoteF.Next();
         }
      } /* endif */

      // Note commerciali
      if(!NoteCom.Seek(Id)){
         fprintf(Out,"Il treno non ha note commerciali \n");
      } else {
         while(&NoteCom.RecordCorrente() && NoteCom.RecordCorrente().NumeroMezzoVg == Id){
            STRINGA Period;
            PERIODICITA_IN_CHIARO * PerIc = NoteCom.RecordCorrente().Periodic.EsplodiPeriodicita();
            if (PerIc) {
               ELENCO_S Tmp =  PerIc->PeriodicitaLeggibile();
               ORD_FORALL(Tmp,t)Period += Tmp[t]+"; ";
               delete PerIc;
            } /* endif */
            fprintf(Out,"Fermate  da %i a %i Nota: %i  Periodicita %s\n", NoteCom.RecordCorrente().StzInizioSrv, NoteCom.RecordCorrente().StzFineSrv , NoteCom.RecordCorrente().NumNota ,(CPSZ)Period );
            AllNote += NoteCom.RecordCorrente().NumNota ;
            if(PeriodicitaEstesa && PFis != NoteCom.RecordCorrente().Periodic){
               ELENCO_S Lst = NoteCom.RecordCorrente().Periodic.Decod();
               ORD_FORALL(Lst,n) fprintf(Out,"%s\n",(CPSZ)Lst[n]);
            } /* endif */
            NoteCom.Next();
         }
      } /* endif */

//<<< ORD_FORALL Ok,Tr
   }

   // A questo punto vedo di stampare il dettaglio dei treni
   if (Dettaglio >= 0) {
      fprintf(Out,"\n\n");
      fprintf(Out,"====================================================================================\n");
      fprintf(Out,"       Stampa dettaglio dei mezzi virtuali con i treni indicati in composizione\n");
      fprintf(Out,"====================================================================================\n");

      char * Crs = "01P"; //
      STRINGA Determ(Crs[Dettaglio]);

      F_MEZZO_VIRTUALE             TabTv(PATH_OUT "M0_TRENV.TM"+ Determ);
      F_FERMATE_VIRT               TabFv(PATH_OUT "M0_FERMV.TM"+ Determ);
      F_PERIODICITA_FERMATA_VIRT   PerFv(PATH_OUT "M1_FERMV.TM"+ Determ);

      ORD_FORALL(TabTv,i){
         MEZZO_VIRTUALE & Mv = TabTv.FixRec(i);
         BOOL Okk = FALSE;
         for (int J = 0; J < Mv.NumMezziComponenti; J++ ) {
            STRINGA Ident = St(Mv.Mv[J].IdentTreno);
            Ident.Strip();
            if (Ok.Contiene(Ident)) {
               Okk = TRUE;
               break;
            } /* endif */
         } /* endfor */
         if (!Okk)continue;
         fprintf(Out,"---------------------------------------------\n");
         fprintf(Out," MV Nø %i  %s %s Fermate / Ft %i %i \n",Mv.MezzoVirtuale, Mv.FittizioLinearizzazione ? "( Fittizio di Linearizzazione )" : "", Mv.DaCarrozzeDirette ? "( Da Carr Dirette )" : "", Mv.NumeroFermateValide, Mv.NumeroFermateTransiti );
         fprintf(Out,"---------------------------------------------\n");
         fprintf(Out,"Periodicita' %s: %s\n",Mv.PeriodicitaDisuniformi ? "disuniforme": "uniforme",(CPSZ)Mv.PeriodicitaMV.InChiaro());
         if(PeriodicitaEstesa){
            ELENCO_S Lst = Mv.PeriodicitaMV.Decod();
            ORD_FORALL(Lst,n) fprintf(Out,"%s\n",(CPSZ)Lst[n]);
         } /* endif */
         fprintf(Out,"Treni in composizione: ");
         for (J = 0; J < Mv.NumMezziComponenti; J++ ) {
            fprintf(Out,"%s ",St(Mv.Mv[J].IdentTreno));
         } /* endfor */
         fprintf(Out,"\n");
         TabFv.Seek(Mv.MezzoVirtuale);
         if(Mv.PeriodicitaDisuniformi)PerFv.Seek(Mv.MezzoVirtuale);
         while (& TabFv.RecordCorrente() && TabFv.RecordCorrente().MezzoVirtuale == Mv.MezzoVirtuale) {
            FERMATE_VIRT & Fer = TabFv.RecordCorrente();
            fprintf(Out,"Fermata Nø %3.3i Id %4.4i Km %3.3i Mf %i Ar %i Par %i Tra %i OrAr %2.2i:%-2.2i  OrPar %2.2i:%-2.2i %s",
               Fer.Progressivo,
               Fer.Id,
               Fer.ProgKm,
               Fer.TrenoFisico,
               Fer.FermataArrivo,
               Fer.FermataPartenza,
               Fer.Transito,
               Fer.OraArrivo/60, Fer.OraArrivo%60,
               Fer.OraPartenza/60, Fer.OraPartenza%60,
               Stazioni[Fer.Id].NomeStazione
            );
            if (Mv.PeriodicitaDisuniformi) {
               PERIODICITA_FERMATA_VIRT & Per = PerFv.RecordCorrente();
               if (&Per) {
                  assert(Per.MezzoVirtuale == Fer.MezzoVirtuale && Per.Progressivo == Fer.Progressivo );
                  fprintf(Out,"  %s",(CPSZ)Per.Periodicita.InChiaro());
                  PerFv.Next();
               } else {
                  assert(&Per != NULL);
               } /* endif */
            } /* endif */
            fprintf(Out,"\n");
            TabFv.Next();
//<<<    while  & TabFv.RecordCorrente   && TabFv.RecordCorrente  .MezzoVirtuale == Mv.MezzoVirtuale
         } /* endwhile */
//<<< ORD_FORALL TabTv,i
      }
//<<< if  Dettaglio >= 0
   } /* endif */

   // Decodifica delle Note
   if (AllNote.Dim() > 0) {
      fprintf(Out,"\n\n ====== Decodifica delle note in tutte le lingue (ID, Lingua, Testo)\n");
      ORD_FORALL(DatiNote,u){
         FILE_DATI_NOTE::FDN_R_STRU & Rec = DatiNote[u];
         if (AllNote.Contiene(Rec.IdNota)) {
            STRINGA Per;
            fprintf(Out, "%i %i %s \n",Rec.IdNota, Rec.Linguaggio, Rec.Testo);
         } /* endif */
      }
   } /* endif */

   fclose(Out);

   return 0;
//<<< int  main int argc,char *argv
}

