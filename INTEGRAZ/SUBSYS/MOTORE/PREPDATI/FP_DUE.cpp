//----------------------------------------------------------------------------
// FP_DUE.CPP: Controlli incrociati dati grafo - polimetriche
// Se i controlli falliscono ha un return code di 123
//----------------------------------------------------------------------------
//#define DUMP_STAZIONI          // Dump dei dati delle stazioni
//#define DUMP_GRAFO             // Dump dei primi vicini delle stazioni
//#define DUMP_RAMI              // Dump dei RAMI
//#define DUMP_POLIMETRICHE      // Dump dei dati delle polimetriche
#define CONTROLLA_SIMMETRIA    // Controlla che le relazioni Nodi<->Primi vicini siano simmetriche
#define CONTROLLA_POLIMETRICHE // Controlla che le distanze sulle polimetriche corrispondano
#define CONTROLLA_POLICUM      // Controlli su polimetriche cumulative
#define CONTROLLA_DIRAMAZIONE  // Controlla che le distanze sulle polimetriche di diramazione corrispondano alla risoluzione del grafo
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4

typedef unsigned long BOOL;    // EMS001 Win

#include "oggetto.h"
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "ALFA0.HPP"
#include "seq_proc.HPP"
#include "polim.HPP"
#include <time.h>
#include <i_motore.hpp>

#include "FT_PATHS.HPP"  // Path da utilizzare

#define PGM      "FP_DUE"

STRINGA Add(char * a,char*b){
   char Buf[100];
   sprintf(Buf,a,b);
   return STRINGA(Buf);
};

#define MK1(_a,_b,_c,_d,_e) FMT += "%-" _b _c ;HDR1 += Add("%-" _b "s",_d) ; HDR2 += Add("%-" _b "s",_e);
#define MK2(_a,_b,_c,_d,_e) ,Staz._a

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"

   int ReturnCode = 0;

   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   // EMS002 Win SetStatoProcesso(PGM, SP_PARTITO);


   GESTIONE_ECCEZIONI_ON

   // ULONG Tm1 = Time();
   PROFILER::Clear(FALSE);
   PROFILER::Clear(TRUE);

   MM_RETE_FS & Rete = * MM_RETE_FS::CreaRete(PATH_DATI,NUSTR,NUSTR);
   TRACESTRING("Creata la RETE FS");

   GRAFO & Grafo = GRAFO::Gr();

   // Apro l' archivio decodifica codici CCR
   CCR_ID Ccr_Id(PATH_DATI);

   FILE_RW Out(PATH_OUT   PGM ".LOG");
   Out.SetSize(0);
   AddFileToBprintf(&Out);

   char datebuf[9], timebuf[9];
   _strdate(datebuf);
   _strtime(timebuf);
   Out.printf("FP_DUE: Attivato il %s alle %s\r\n",datebuf,timebuf);
   int i,i1,i2,i3;

   // ---------------------------------------------------------
   // Dump (opzionale) delle stazioni del grafo
   // ---------------------------------------------------------
   #ifdef DUMP_STAZIONI
   Out.printf("===============================\r\n");
   Out.printf("===   DUMP DELLE STAZIONI   ===\r\n");
   Out.printf("===============================\r\n");
   STRINGA HDR1,HDR2,FMT;
   HDR1 = "RecN   ";
   HDR2 = "       ";
   FMT  = "%-7u";
   MK1( Id                ,"05","u","Id","Staz")
   MK1( CCR               ,"06","u","CCR","")
   MK1( Km1               ,"05","u","Km1","")
   MK1( ProgRamo          ,"05","u","Prog","Ramo")
   MK1( StazioneFS        ,"03","u","FS","")
   MK1( Vendibile         ,"05","u","Vend","")
   MK1( Nodale            ,"05","u","Nodo","")
   MK1( Terminale         ,"05","u","Term","")
   MK1( DiAllacciamento   ,"05","u","Allc","")
   MK1( DiInstradamento   ,"05","u","Istr","")
   MK1( DiDiramazione     ,"05","u","Di","Diram")
   MK1( DiTransito        ,"05","u","Tran","")
   MK1( IdRamo            ,"05","u","Id","Ramo")
   MK1( IdentIstr         ,"08","s","Nome7","")
   MK1( Tariffe.TariffaRegione    ,"04","u","Tar","Reg")
   MK1( Tariffe.Prima_Estensione  ,"03","u","E1","")
   MK1( Tariffe.Seconda_Estensione,"03","u","E2","")
   MK1( Tariffe.Terza_Estensione  ,"03","u","E3","")
   MK1( NumeroConcesse    ,"04","u","Num","Con")
   MK1( NumeroPrimiVicini ,"04","u","Num","PV")
   MK1( NumeroPolimetriche,"04","u","Num","Pol")
   MK1( InstrGrafo        ,"05","u","Istr","Graf")
   MK1( Abilitazione      ,"05","u","Abil","")
   MK1( IsCnd             ,"05","u","Istr","Cond")
   FMT += "\r\n";

   for (i1= 1;i1< Grafo.TotStazioniGrafo ;i1++ ) {
      if (i1 % NUMRIGHE_DUMP == 1) {
         if(i1 != 1)Out.printf("\r\n");
         Out.printf("%s\r\n%s\r\n",(CPSZ)HDR1,(CPSZ)HDR2);
      } /* endif */
      STAZ_FS & Staz = Grafo.StazioniGrafo[i1];
      Out.printf((CPSZ)FMT ,i
      MK2( Id                ,"05","u","Id","Staz")
      MK2( CCR               ,"06","u","CCR","")
      MK2( Km1               ,"05","u","Km1","")
      MK2( ProgRamo          ,"05","u","Prog","Ramo")
      MK2( StazioneFS        ,"03","u","FS","")
      MK2( Vendibile         ,"05","u","Vend","")
      MK2( Nodale            ,"05","u","Nodo","")
      MK2( Terminale         ,"05","u","Term","")
      MK2( DiAllacciamento   ,"05","u","Allc","")
      MK2( DiInstradamento   ,"05","u","Istr","")
      MK2( DiDiramazione     ,"05","u","Di","Diram")
      MK2( DiTransito        ,"05","u","Tran","")
      MK2( IdRamo            ,"05","u","Id","Ramo")
      ,St(Staz.IdentIstr)
      MK2( Tariffe.TariffaRegione    ,"04","u","Tar","Reg")
      MK2( Tariffe.Prima_Estensione  ,"03","u","E1","")
      MK2( Tariffe.Seconda_Estensione,"03","u","E2","")
      MK2( Tariffe.Terza_Estensione  ,"03","u","E3","")
      MK2( NumeroConcesse    ,"04","u","Num","Con")
      MK2( NumeroPrimiVicini ,"04","u","Num","PV")
      MK2( NumeroPolimetriche,"04","u","Num","Pol")
      MK2( InstrGrafo        ,"05","u","Istr","Graf")
      MK2( Abilitazione      ,"05","u","Abil","")
      MK2( IsCnd             ,"05","u","Istr","Cond")
      );
   } /* endfor */
   Out.printf("\r\n");
   #endif

   // ---------------------------------------------------------
   // Dump (opzionale) del grafo
   // ---------------------------------------------------------
   #ifdef DUMP_GRAFO
   for (i1= 1;i1< Grafo.TotStazioniGrafo ;i1++ ) {
      STAZ_FS & Staz = Grafo.StazioniGrafo[i1];
      if(Staz.NumeroPrimiVicini == 0)continue;
      Out.printf("===============================\r\n");
      Out.printf("Stazione ID %i: %s\r\n", i1,Stazioni[i1].NomeStazione);
      Out.printf("===============================\r\n");
      for (i2= 0;i2< Staz.NumeroPrimiVicini ; i2++) {
         ID S2 = Staz.PrimiVicini()[i2].Id;
         Out.printf("CCR %5i ID %4i Km %5i Mare %i Concesso %i Instr %i IdRamo %i %s\r\n",
            Stazioni[S2].CodiceCCR,S2,
            Staz.PrimiVicini()[i2].Distanza      ,
            Staz.PrimiVicini()[i2].Mare          ,
            Staz.PrimiVicini()[i2].Concesso      ,
            Staz.PrimiVicini()[i2].Instradamento ,
            Staz.PrimiVicini()[i2].IdRamo        ,
            Stazioni[S2].NomeStazione);
      } /* endfor */
   } /* endfor */
   Out.printf("\r\n");
   #endif
   // ---------------------------------------------------------
   // Dump (opzionale) dei RAMI del grafo
   // ---------------------------------------------------------
   #ifdef DUMP_RAMI
   Out.printf("Numero totale di rami del grafo: %i\r\n",Grafo.TotRami);
   for (i2= 1;i2< Grafo.TotRami ;i2++ ) {
      RAMO & Ramo = Grafo.Rami[i2];
      Out.printf("===============================\r\n");
      Out.printf("Ramo ID %i: %i->%i %s->", i2,Ramo.IdStaz1,Ramo.IdStaz2,Stazioni[Ramo.IdStaz1].NomeStazione);
      Out.printf("%s\r\n",Stazioni[Ramo.IdStaz2].NomeStazione);
      Out.printf("===============================\r\n");
      Out.printf("KmRamo        %i\r\n",Ramo.KmRamo        );
      Out.printf("Mare          %i\r\n",Ramo.Mare          );
      Out.printf("Concesso      %i\r\n",Ramo.Concesso      );
      Out.printf("CodConcessa   %i\r\n",Ramo.CodConcessa   );
      Out.printf("LineaConcessa %i\r\n",Ramo.LineaConcessa );
      Out.printf("Instradamento %i\r\n",Ramo.Instradamento );
      Out.printf("NumStazioni   %i\r\n",Ramo.NumStazioni   );
      for (i= 0;i< Ramo.NumStazioni ; i++) {
         Out.printf("Staz Nø %2i: Id        %i\r\n",i,Ramo.StazioniDelRamo()[i].Id        );
         Out.printf("Staz Nø %2i: Instrad   %i\r\n",i,Ramo.StazioniDelRamo()[i].Instrad   );
         Out.printf("Staz Nø %2i: Diram     %i\r\n",i,Ramo.StazioniDelRamo()[i].Diram     );
         Out.printf("Staz Nø %2i: Vendibile %i\r\n",i,Ramo.StazioniDelRamo()[i].Vendibile );
      } /* endfor */
   } /* endfor */
   Out.printf("\r\n");
   #endif
   // ---------------------------------------------------------
   // Dump (opzionale) delle polimetriche
   // ---------------------------------------------------------
   #ifdef DUMP_POLIMETRICHE
   for (i = 1;i < Grafo.TotPolim ;i++ ) {
      POLIMETRICA & Pol = Grafo.Polim[i];
      Out.printf("===============================\r\n");
      Out.printf("Polimetrica ID %i: Nø %s %s\r\n",
         i, (*Grafo.FilePolim)[i].Nome,
         (*Grafo.FilePolim)[i].Descrizione);
      Out.printf("===============================\r\n");
      Out.printf("IdPoli        %i\r\n",Pol.IdPoli       );
      Out.printf("KmPoli        %i\r\n",Pol.KmPoli       );
      Out.printf("TipoPoli      %i\r\n",Pol.TipoPoli     );
      Out.printf("NumStazioni   %i\r\n",Pol.NumStazioni  );
      Out.printf("NumStazDiram  %i\r\n",Pol.NumStazDiram );
      Out.printf("NumBits       %i\r\n",Pol.NumBits      );
      Out.printf("PDati         %i\r\n",Pol.PDati        );
      Out.printf("Zone          %p\r\n",Pol.Zone         );
      Out.printf("Base          %i\r\n",Pol.Base         );
      for (i2= 0;i2< Pol.NumStazioni ; i2++) {
         POLIMETRICA::PO_STAZ & Stz = Pol.StazioniDellaPolimetrica()[i2];
         ID Id2 = Stz.Id;
         Out.printf("%5i %4i ",Grafo.StazioniGrafo[Id2].CCR,Id2);
         Out.printf("|%3i| ",Stz.Radice);
         //if(Stz.Last0){
         //   Out.printf("%3i ",i2 - Stz.Last0);
         //} else {
         //   Out.printf("%3i ", Stz.Last0);
         //}
         for (i3= 0; i3 < i2 ; i3++) {
            ID Id3 = Pol.StazioniDellaPolimetrica()[i3].Id;
            Out.printf(" %4i",Pol.DistanzaTra(Id2,Id3));
         } /* endfor */
         Out.printf("\r\n");
      } /* endfor */

//<<< for (i = 1;i < Grafo.TotPolim ;i++ ) {
   } /* endfor */
   Out.printf("\r\n");
   #endif

   // ---------------------------------------------------------
   // Controllo simmetria
   // ---------------------------------------------------------
   #ifdef CONTROLLA_SIMMETRIA
   Bprintf("===============================");
   Bprintf("Controllo simmetria grafo ");
   Bprintf("===============================");
   for (i1= 1;i1< Grafo.TotStazioniGrafo ;i1++ ) {
      STAZ_FS & Staz = Grafo.StazioniGrafo[i1];
      if(Staz.NumeroPrimiVicini == 0)continue;
      for (i2= 0;i2< Staz.NumeroPrimiVicini ; i2++) {
         ID S2 = Staz.PrimiVicini()[i2].Id;
         STAZ_FS & Pv = Grafo.StazioniGrafo[S2];
         if (Pv.PosDaId(i1) == WORD_NON_VALIDA) {
            Bprintf2("Asimmetria: la stazione % i ha come primo vicino %i, ma non viceversa",i1,Pv.Id);
            ReturnCode = 123;
         } /* endif */
      } /* endfor */
   } /* endfor */
   Out.printf("\r\n");
   #endif

   // ---------------------------------------------------------
   // Controllo (opzionale) delle polimetriche
   // ---------------------------------------------------------
   #ifdef CONTROLLA_POLIMETRICHE
   {
      Bprintf("===============================");
      Bprintf("Controllo distanze polimetriche");
      Bprintf("===============================");
      FILE_RO Poli(PATH_POLIM "POLIMETR.TMP");
      ARRAY_ID StazioniPoli;
      int IdPoli=0;
      BOOL HdrOk;
      STRINGA Linea,Ident,Descriz;


      while(Poli.gets(Linea,4096)){
         // TRACESTRING(Linea);
         Linea[Linea.Dim()-1]=0;
         if (Linea[0] == '1') {
            IdPoli ++;
            HdrOk = FALSE;
            REC1 & Rec1 = *(REC1*)(CPSZ)Linea;
            Ident = St(Rec1.Ident);
            Descriz = Rec1.Descrizione;
            Ident.Strip();
            Descriz.Strip();
            StazioniPoli.Clear();
         } else if (Linea[0] == '3') {
            POLIMETRICA & Pol = Grafo.Polim[IdPoli];
            REC3 & Rec3 = *(REC3*)(CPSZ)Linea;
            int Ccr = It(Rec3.CodCCR);
            TRACESTRING("Entro in int IdStaz = Ccr_Id.CercaCCR(Ccr);");
            int IdStaz = Ccr_Id.CercaCCR(Ccr);
            TRACESTRING("Esco da  int IdStaz = Ccr_Id.CercaCCR(Ccr);");
            char * Distanze = (char*)(CPSZ)Linea + sizeof(REC3);
            ORD_FORALL(StazioniPoli,i){
               ID Id = StazioniPoli[i];
               if(!Pol.Contiene(Id))continue; // E' una stazione che era stata scartata
               if(!Pol.Contiene(IdStaz))continue; // E' una stazione che era stata scartata
               int Dist = StringToInt(Distanze+(4*i),4);
               int DistPol = Pol.DistanzaTra(Id,IdStaz);
               if(DistPol == 9999 )DistPol =0;
               if(DistPol == Dist )continue;
               if(Dist == 0 )continue;
               // Eccezione: distanze forzate da 0 ad 1 Km
               if(Dist == 1 && DistPol == 0)continue;
               ReturnCode = 123;
               if(!HdrOk ){
                  Bprintf2("===============================");
                  Bprintf("Polimetrica ID %i: Nø %s %s Errore nell' assegnazione distanze ", IdPoli, (CPSZ)Ident,(CPSZ)Descriz);
                  Bprintf2("===============================");
                  HdrOk = TRUE;
               }
               Out.printf(" %5i Km invece di %5i tra stazioni ", Dist, DistPol );
               Out.printf(" %i %s e ",Id, Stazioni.DecodificaIdStazione(Id));
               Out.printf(" %i %s \r\n",IdStaz, Stazioni.DecodificaIdStazione(IdStaz));
            }
            StazioniPoli += IdStaz;
//<<<    if (Linea[0] == '1') {
         }
//<<< while(Poli.gets(Linea,4096)){
      }
   }
   Out.printf("\r\n");
   #endif

   TRACESTRING("FP_DUE 335");

   // ---------------------------------------------------------
   // Controllo (opzionale) delle polimetriche cumulative
   // ---------------------------------------------------------
   #ifdef CONTROLLA_POLICUM
   {

   Bprintf("===============================");
   Bprintf("Controllo distanze polimetriche cumulative");
   Bprintf("===============================");

   STRINGA Path = PATH_INTERMED;
   F_RETI_CU         Reti(Path+"RETISTRU.TMP");
   F_POLIM_CUMULAT   Cum1(Path+"CUMULAT.TMP") ;
   // F_POLIM_CUMULAT_2 Cum2(Path+"CUMULAT2.TMP");

   ORD_FORALL(Reti,i1){
      Reti.FixRec(i1);
      RETI_CUMULATIVE & Rete =  Reti.RecordCorrente();
      Bprintf("Rete : %i %s",Rete.CodiceRete,Rete.Nome);
      Cum1.Seek(Rete.CodiceRete);
      while (&Cum1.RecordCorrente() && Cum1.RecordCorrente().CodiceRete == Rete.CodiceRete) {
         POLIM_CUMULAT & Poli = Cum1.RecordCorrente();
            ID StazioneRete = Poli.Id;
            if (Stazioni[StazioneRete].SocietaConcessa1 == Rete.CodiceRete) {
               // Ok
               Bprintf3("Stazione in esame : %i %s",StazioneRete,Stazioni.DecodificaIdStazione(StazioneRete));
            } else if (Stazioni[StazioneRete].SocietaConcessa2 == Rete.CodiceRete) {
               // Ok
               Bprintf3("Stazione in esame : %i %s",StazioneRete,Stazioni.DecodificaIdStazione(StazioneRete));
            } else {
               Bprintf3("Scartata stazione non appartenente alla rete : %i %s",StazioneRete,Stazioni.DecodificaIdStazione(StazioneRete));
               continue;
               // Cum1.ModifyCurrentRecord();
            } /* endif */
            for (int j = 0 ; j< 8; j++ ) {
               WORD Km = Poli.Distanze[j];
               if(Km == 0) continue; // Non e' collegato
               Bprintf3(" -> Transito %i %s %i Km\n",Rete.StazioneDitransito[j],Stazioni.DecodificaIdStazione(Rete.StazioneDitransito[j]),Poli.Distanze[j]);
               ARRAY_ID Nodi;
               Nodi += Rete.StazioneDitransito[j];
               Nodi += StazioneRete;
               Grafo.AbilitaCumulativo = 1 ; // Abilita cumulativo ed interno
               BOOL Ok = TRUE;
               PERCORSO_GRAFO P1; Ok &= P1.Set(Nodi);
               Grafo.AbilitaCumulativo = 99; // Abilita solo cumulativo
               PERCORSO_GRAFO P2; Ok &= P2.Set(Nodi);
               // Bprintf2("Percorso cumulativo, Stazione Id %i Transito Id %i %i Km invece di %i" ,StazioneRete,Rete.StazioneDitransito[j],P2.Len(),Poli.Distanze[j]);
               if(abs(P2.Len() - Poli.Distanze[j]) > 3){
                  char Stz1[40];
                  strcpy(Stz1,Stazioni.DecodificaIdStazione(StazioneRete));
                  //Bprintf2("Percorso cumulativo non segue percorso minimo, Stazione Id %i %s Transito Id %i %s : %i Km invece di %i",
                  Bprintf2("Stazione Id %i %s Transito Id %i %s : %i Km invece di %i",
                  StazioneRete, Stz1, Rete.StazioneDitransito[j],
                  Stazioni.DecodificaIdStazione(Rete.StazioneDitransito[j]),
                  P2.Len(),Poli.Distanze[j]);
               }
               if(!(P1.Nodi == P2.Nodi)){
                  TRACESTRING("--------------------------------------");
                  TRACESTRING("Necessita' di stazione/i vincolanti per percorso cumulativo:");
                  P1.Nodi.Trace(Stazioni,"P1, Km = "+STRINGA(P1.Len()));
                  P2.Nodi.Trace(Stazioni,"P2, Km = "+STRINGA(P2.Len()));
                  Grafo.AbilitaCumulativo = 1 ; // Abilita cumulativo ed interno
                  PERCORSO_GRAFO P3; Ok &= P3.Set(P2.Nodi);
                  P3.Nodi.Trace(Stazioni,"P3, Km = "+STRINGA(P3.Len()));
                  ARRAY_ID A1,A2;
                  Ok &= P3.DeterminaStazioniVincolanti(A1,A2);
                  STRINGA Org = Stazioni.DecodificaIdStazione(Rete.StazioneDitransito[j]);
                  if (Ok) {
                     Bprintf3("Percorso cumulativo da %s a %s",(CPSZ)Org,Stazioni.DecodificaIdStazione(StazioneRete));
                     if(A1.Dim() > 1)Bprintf3(" Stazioni vincolanti Nø %i: ",A1.Dim());
                     ORD_FORALL(A1,k){
                        if(A1[k] == A2[k]){
                           Bprintf3("   Stazione vincolante %i %s", P3.Nodi[A1[k]], Stazioni.DecodificaIdStazione(P3.Nodi[A1[k]]));
                        } else {
                           Bprintf3("   Una stazione vincolante a scelta tra:");
                           for (int l = A2[k];l <= A1[k] ;l++ ) {
                              Bprintf3("      -> %i %s", P3.Nodi[l], Stazioni.DecodificaIdStazione(P3.Nodi[l]));
                           } /* endfor */
                        } /* endif */
                     }
                  } else {
                     Bprintf("ERRORE: Percorso cumulativo non completabile da %s a %s",(CPSZ)Org,Stazioni.DecodificaIdStazione(StazioneRete));
                  } /* endif */
//<<<          if(!(P1.Nodi == P2.Nodi)){
               };
//<<<       for (int j = 0 ; j< 8; j++ ) {
            } /* endfor */
         Cum1.Next();
//<<< while (&Cum1.RecordCorrente() && Cum1.RecordCorrente().CodiceRete == Rete.CodiceRete) {
      } /* endwhile */

//<<< ORD_FORALL(Reti,i1){
   };
   }
   #endif

   // ---------------------------------------------------------
   // Controllo (opzionale) delle polimetriche di diramazione
   // ---------------------------------------------------------
   #ifdef CONTROLLA_DIRAMAZIONE
   {

      extern BOOL Si_Diramazion;
      BOOL Old_Diram = Si_Diramazion;
      Si_Diramazion = FALSE;
      Bprintf("===============================");
      Bprintf("Controllo corrispondenza distanze polimetriche normali / di diramazione ");
      Bprintf("===============================");
      int TotControlli = 0, TotErr1=0, TotErrori = 0, TotNr = 0;
      ARRAY_ID Percorso;
      Grafo.AbilitaCumulativo = 0; // Solo percorsi su rete FS
      for (int i = 0; i < Grafo.TotPolim; i++ ) {
         POLIMETRICA & Pol = Grafo.Polim[i];
         if (Pol.TipoPoli != POLIMETRICA::DIRAMAZIONE  && Pol.TipoPoli != POLIMETRICA::ALLACCIAMENTO )continue;
         // if (Pol.TipoPoli != POLIMETRICA::DIRAMAZIONE )continue;
         char * Ident   = (*Grafo.FilePolim)[i].Nome;
         char * Descriz = Grafo.FilePolim->RecordCorrente().Descrizione;
         BOOL HdrOk = FALSE;
         POLIMETRICA::PO_STAZ *  Stz = Pol.StazioniDellaPolimetrica();
         int NumControlli = 0, NumErr1=0, NumErrori = 0, NumNr = 0;
         for (int j = 0; j < Pol.NumStazioni ; j++ ) {
            ID Da = Stz[j].Id;
            if(!Grafo[Da].Vendibile)continue;   // Altrimenti ViaggioLibero() va in errore
            for (int k = j+1; k < Pol.NumStazioni ; k++ ) {
               ID A  = Stz[k].Id;
               if(!Grafo[A].Vendibile)continue; // Altrimenti ViaggioLibero() va in errore

               // Questi sono casi particolari in cui ho delle polimetriche ad esclusivo uso locale
               if(Da ==  173 && A == 142 )continue; // Arquata Scrivia        - Tortona
               if(Da ==  40  && A == 135 )continue; // Maccarese-Fregene      - Roma S. Pietro
               if(Da ==  251 && A == 252 )continue; // Eccellente             - Rosarno
               if(Da ==  238 && A == 187 )continue; // Firenze Campo Di Marte - Firenze Rifredi

               TRACESTRING(" Da = "+STRINGA(Da)+ " A = "+STRINGA(A)+ " Polimetrica "+STRINGA(i)+" "+STRINGA(Ident)+" "+STRINGA(Descriz));
               int KmDaDiramazione = Pol.DistanzaTra(Da,A);
               Percorso.Clear();Percorso += Da; Percorso += A;
               TRACESTRING("Entro in MM_PERCORSO * Perc = Rete.ViaggioLibero(Percorso);");
               Percorso.Trace(Stazioni, "Trace percorso FP_DUE 476", 1);
               MM_PERCORSO * Perc = Rete.ViaggioLibero(Percorso);
               TRACESTRING("Esco  in MM_PERCORSO * Perc = Rete.ViaggioLibero(Percorso);");
               if(Perc && Perc->DatiTariffazione.Stato == DATI_TARIFFAZIONE::TARIFFA_VALIDA){ // Altrimenti e' in errore
                  NumControlli ++;
                  int KmDaPolimetriche = Perc->DatiTariffazione.KmReali;
                  int DeltaKm = KmDaDiramazione - KmDaPolimetriche;
                  if(DeltaKm != 0){
                     if (abs(DeltaKm) <= 1) {
                        NumErr1 ++;
                     } else {
                        NumErrori ++;
                     } /* endif */
                     if(!HdrOk ){
                        Bprintf2("===============================");
                        Bprintf2("Polimetrica ID %i: Nø %s %s Mancate corrispondenze nelle distanze ", Pol.IdPoli, Ident,Descriz);
                        Bprintf2("===============================");
                        HdrOk = TRUE;
                     }
                     Out.printf(" %3i:  %5i Km invece di %5i tra stazioni ",DeltaKm, KmDaPolimetriche,KmDaDiramazione);
                     Out.printf(" %i %s e "  ,Da, Stazioni.DecodificaIdStazione(Da));
                     Out.printf(" %i %s \r\n",A , Stazioni.DecodificaIdStazione(A ));
                  }
                  delete Perc;
               } else {
                  if(!HdrOk ){
                     Bprintf2("===============================");
                     Bprintf2("Polimetrica ID %i: Nø %s %s Mancate corrispondenze nelle distanze ", Pol.IdPoli, Ident,Descriz);
                     Bprintf2("===============================");
                     HdrOk = TRUE;
                  }
                  NumNr ++;
                  Out.printf(" Relazione NON risolubile (%5i Km)  tra stazioni ", KmDaDiramazione);
                  Out.printf(" %i %s e "  ,Da, Stazioni.DecodificaIdStazione(Da));
                  Out.printf(" %i %s \r\n",A , Stazioni.DecodificaIdStazione(A ));
               }
            } /* endfor */
//<<<    for (int j = 0; j < Pol.NumStazioni ; j++ ) {
         } /* endfor */
         Bprintf("%i controlli %i Non risolubili %i Diff1Km e %i errori su polimetrica ID %i: Nø %s %s ",NumControlli, NumNr, NumErr1, NumErrori, Pol.IdPoli, Ident,Descriz);
         TotControlli += NumControlli;
         TotErr1      += NumErr1     ;
         TotErrori    += NumErrori   ;
         TotNr        += NumNr       ;
//<<< for (int i = 0; i < Grafo.TotPolim; i++ ) {
      } /* endfor */
      Bprintf("TOTALE: %i Controlli %i Non risolubili %i Diff1Km e %i errori ",TotControlli, TotNr, TotErr1, TotErrori);
      Out.printf("\r\n");
      Si_Diramazion = Old_Diram ;
   }
   #endif

   PROFILER::Trace(TRUE,1);

   // ---------------------------------------------------------
   // Controlli incrociati
   // ---------------------------------------------------------
   // 1: Scandisco le polimetriche LOCALI:
   //    - Verifico che le stazioni non nodali appartengano a rami
   //      i cui nodi stanno sulla polimetrica
   //      (considerando come se fossero nodi le stazioni di diramazione)
   //    - Identifico i nodi vincolanti della polimetrica
   // 2: Scandisco i rami, verifico che le stazioni (non di diramazione)
   //    contigue appartengano alla stessa polimetrica

   _strdate(datebuf);
   _strtime(timebuf);
   Out.printf("FP_DUE: Termina il %s alle %s\r\n",datebuf,timebuf);

   // ---------------------------------------------------------
   // Genera il file di instradamenti complessi per il cumulativo.
   // ---------------------------------------------------------
   F_POLIM_CUMULAT_2 Cum2(PATH_INTERMED "CUMULAT2.TMP") ;
   F_ISTRECC IstrEcc(PATH_DATI "MM_ISCU2.DB");
   IstrEcc.Clear();
   ORD_FORALL(Cum2,z){
      ARRAY_ID Perc;
      POLIM_CUMULAT_2 & Pol =Cum2.FixRec(z);
      Perc+=Pol.Transito;
      for (int i = 0;i < 4  ;i++ ) {
         if(Pol.Instradamento[i])Perc += Pol.Instradamento[i];
      } /* endfor */
      Perc += Pol.Id;
      Perc.Trace(Stazioni,"Percorso da dati polimetriche:");
      GRAFO::Gr().AbilitaCumulativo = TRUE;
      PERCORSO_GRAFO Pe(Perc);
      Pe.Trace("Percorso in esame:");
      PERCORSO_INSTRADAMENTI * Pi = PERCORSO_INSTRADAMENTI::CreaPercorso();
      // Determino i punti di instradamento del percorso
      Pi->DeterminaPuntiDiInstradamento(Pe);
      Pi->StazioniDiInstradamento.Trace(Stazioni,"Stazioni di instradamento");
      ISTRADAMENTO_ECCEZIONALE_CUM Iec;
      ZeroFill(Iec);
      Iec.StzVincolante[0] = Pol.Transito ;
      ORD_FORALL(Pi->StazioniDiInstradamento,k){
         Iec.StzVincolante[k+1] = Pi->StazioniDiInstradamento[k];
      }
      Iec.StzVincolante[k+1] = Pol.Id;
      ID IdPolimetrica = 0;
      for (int y = 0;y < Grafo.TotPolim ;y++ ) {
         if( Grafo.Polim[y].TipoPoli == POLIMETRICA::CUMULATIVA && Grafo.Polim[y].SocietaConcessa == Pol.CodiceRete){
            IdPolimetrica  = Grafo.Polim[y].IdPoli;
            break;
         }
      } /* endfor */
      Iec.Polimetrica  = IdPolimetrica;
      Iec.Verso        = 0 ;
      Iec.Km           = Pol.Distanza ;
      IstrEcc.AddRecordToEnd(BF1(Iec));

      ZeroFill(Iec);
      Iec.StzVincolante[0] = Pol.Id ;
      int To = 1;
      FORALL(Pi->StazioniDiInstradamento,k1){
         Iec.StzVincolante[To] = Pi->StazioniDiInstradamento[k1];
         To ++;
      }
      Iec.StzVincolante[To] = Pol.Transito;
      Iec.Polimetrica  = IdPolimetrica;
      Iec.Verso        = 1 ;
      Iec.Km           = Pol.Distanza ;
      IstrEcc.AddRecordToEnd(BF1(Iec));
      delete Pi;
   }
   IstrEcc.ReSortFAST();
   IstrEcc.Flush();

   // ---------------------------------------------------------
   // Genera il file di instradamenti con codici regionali particolari
   // ---------------------------------------------------------
   FILE_RO Eccz(PATH_CVB "REG_ECCZ.TXT");
   F_ISTRECC_REG IstrEcc_REG(PATH_DATI "MM_ECZRG.DB");
   IstrEcc_REG.Clear();
   IstrEcc_REG.ReSortFAST();
   STRINGA Linea;
   while (Eccz.gets(Linea)) {
      TRACESTRING(Linea); // EMS
      if(Linea[0]=='*')continue;
      ELENCO_S Tokens = Linea.Tokens(";");
      if(Tokens.Dim() < 3)continue;       // Non plausibile
      if(Tokens[0].ToInt() == 0)continue; // Non ha codice regione
      BOOL Ok = TRUE;
      ARRAY_ID Nodi;
      for (int i = 1;i < Tokens.Dim() ;i++ ) {
         if(Ccr_Id.CercaCCR(Tokens[i].ToInt()) <= 0){
            Ok = FALSE;
            break;
         }
         Nodi += Ccr_Id.CercaCCR(Tokens[i].ToInt());
      } /* endfor */
      if(!Ok)continue; // Dati Illegali
      Grafo.AbilitaCumulativo = 1 ;
      // Converto i nodi in forma STANDARD
      PERCORSO_GRAFO P1;
      Ok = P1.Set(Nodi);
      if(!Ok)continue; // Non risolto sul grafo
      Nodi = P1.Nodi;
      ISTRADAMENTO_ECCEZIONALE_REG Iec;
      ZeroFill(Iec);
      TRACEINT("Relazione eccezionale, regione ",Tokens[0].ToInt());
      Iec.CodiceRegione = Tokens[0].ToInt();
      ORD_FORALL(Nodi,k){
         Iec.NodiStd[Iec.NumeroNodi++]= Nodi[k];
         TRACESTRING("Stazione ID = " +STRINGA(Nodi[k])+" " +Stazioni.DecodificaIdStazione(Nodi[k]));
      } /* endfor */
      IstrEcc_REG.AddRecordToEnd(BF1(Iec));
      ZeroFill(Iec);
      Iec.CodiceRegione = Tokens[0].ToInt();
      FORALL(Nodi,k1){
         Iec.NodiStd[Iec.NumeroNodi++]= Nodi[k1];
      } /* endfor */
      IstrEcc_REG.AddRecordToEnd(BF1(Iec));
   }
   IstrEcc_REG.ReSortFAST();
   IstrEcc_REG.Flush();

   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TRACESTRING("Fine");

   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;

   return ReturnCode;
//<<< int main(int argc,char *argv[]){
}

