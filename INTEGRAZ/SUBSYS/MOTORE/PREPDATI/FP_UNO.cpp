//----------------------------------------------------------------------------
// FP_UNO.CPP: Caricamento GRAFO e dati polimetriche di CIPRIANO
//----------------------------------------------------------------------------
//#define DBGANOMALE         // traccia stazioni di diramazione anomale
//#define DBGZONE            // traccia appartenenza polimetriche alle zone
//#define ESPANSO            // Out espanso o contratto (dei delta)
//#define TRACERAMI1         // Trace caricamento 1 dei rami
//#define RELAX_ON_CUMUL     // Riduce i warning sulle stazioni cumulative
//#define RELAX_DIST         // Elimina i warning per delta distanza > 2 Km
//#define ESCLUDI_CUMULATIVE // Non include i dati delle polimetriche cumulative
//----------------------------------------------------------------------------
#define LIMITE_ERRORE 3 // Considero OK discordanze fino a LIMITE_ERRORE Km
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4

typedef unsigned long BOOL;    // EMS001 Win

#include "oggetto.h"
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "ALFA0.HPP"
#include "POLIM.HPP"
#include "seq_proc.HPP"
#include "FT_PATHS.HPP"  // Path da utilizzare
#include "scandir.h"
#include "MM_PERIO.HPP"
#include "ft_aux.hpp"

#define MAX_POLIMETRICA 1500
#define PGM      "FP_UNO"


STRINGA Normalizza(const STRINGA & Stazion){
   STRINGA Staz = Stazion;
   Staz.Strip();
   Staz = Staz(0,13);
   Staz.UpCase();
   ORD_FORALL(Staz,i){
      if(Staz[i] == '-')Staz[i] = ' ';
      if(Staz[i] == '(' && i > 0){
         Staz = Staz(0,i-1);
         break;
      }
   }
   Staz.Strip();
   return Staz;
}

STAZIONI * PStazioni;

int Cmp_Staz_RAMO(const void * a, const void * b){
   ID & A = *(ID*)a;
   ID & B = *(ID*)b;

   return (int)(*PStazioni)[A].ProgRamo - (int)(*PStazioni)[B].ProgRamo;
};

STRINGA  * VerificaCCR[99999];

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

   FILE_RW Out(PATH_OUT   PGM ".LOG");
   Out.SetSize(0);
   AddFileToBprintf(&Out);



   ULONG Tm1 = Time();

   ULONG Istogram[MAXPOLI];
   ULONG Scartate         = 0;
   ULONG PerNome          = 0;
   ULONG NonGrafo         = 0;
   ULONG Valide           = 0;
   ULONG VendibiliNoPolim = 0;
   ULONG NonVendibiliPolim= 0;
   ULONG NumPolimetriche  = 0;
   ULONG NonFS            = 0;

   // Gestione dell' instradamento condizionato: i dati di instradamento condizionato forniti
   // da FS consistono in codici di inibizione che, qualora coincidenti tra stazione
   // origine (Nodo1) , Stazione di diramazione , stazione destinazione (oppure tra tre
   // successive stazioni di diramazione) inibiscono l' utilizzo della stazione
   // di diramazione.
   // NB: Sul file delle polimetriche vi sono due serie di codici di inibizione:
   // - I codici di inibizione
   // - I codici di stazione di diramazione inibibile (contrassegnati con *)
   // La coincidenza si intende tra codici di inibizione di origine e destinazione, e codici
   // di stazione inibibile della stazione di diramazione.
   // Nella rappresentazione fatta dal motore metto a fattor comune le stazioni che hanno gli
   // stessi codici e lascio sulla singola stazione una WORD che rappresenta un puntatore
   // all' ISTR_COND (che di per se' e' un insieme).

   ARRAY_ISTR_COND IsCnd(1000);
   ISTR_COND IsC,IsIniB; IsC.Clear();
   IsCnd += IsC; // Elemento vuoto: Lo metto come elemento 0


   // Apro l' archivio stazioni
   STAZIONI Stazioni(PATH_DATI,"ID_STAZI.DB",600000);
   STAZIONI_ALL_PER_NOME StaNomi(PATH_DATI);
   PStazioni = &Stazioni;

   // Usata da varii algoritmi
   STRINGA Linea;

   // ---------------------------------------------------------
   // Mi carico le correzioni al grafo
   // ---------------------------------------------------------
   // Il file KmEccz.MIO deve stare sulla directory corrente
   // Crea sulla directory DEL GRAFO KmEccz.DB
   {
      FILE_RO CorrIn("KmEccz.MIO");
      if(CorrIn.FileSize() == 0){
         Bprintf("Errore: Manca il file KmEccz.MIO");
         exit(999);
      };
      FILE_RW CorrOut(PATH_DATI "MM_KMECZ.DB");
      CorrOut.SetSize(0);

      while (CorrIn.gets(Linea)){
         if(Linea.Dim() == 0 )continue;
         if(Linea[0] == ';') continue ; // Commento
         ELENCO_S Toks = Linea.Tokens(",");
         if(Toks.Dim() < 3)continue   ;
         CORREZIONI_RAMI Correzione;
         Correzione.Id1       = Toks[0].ToInt();
         Correzione.Id2       = Toks[1].ToInt();
         Correzione.KmDaUsare = Toks[2].ToInt();
         CorrOut.Scrivi(VRB(Correzione));
         // Ramo inverso
         Correzione.Id2       = Toks[0].ToInt();
         Correzione.Id1       = Toks[1].ToInt();
         Correzione.KmDaUsare = Toks[2].ToInt();
         CorrOut.Scrivi(VRB(Correzione));
      } /* endwhile */
   }

   // ---------------------------------------------------------
   // Generazione del file con i dati delle polimetriche cumulative
   // ---------------------------------------------------------
   {
      FILE_RW OutP(PATH_INTERMED "POLICUM.TMP");
      OutP.SetSize(0);
      #ifndef ESCLUDI_CUMULATIVE
      Bprintf("Includo dati delle polimetriche cumulative");
      STRINGA Path = PATH_INTERMED;
      F_RETI_CU         Reti(Path+"RETISTRU.TMP");
      F_POLIM_CUMULAT   Cum0(Path+"CUMULAT.TMP") ;
      F_POLIM_CUMULAT_1 Cum1(Path+"CUMULAT1.TMP") ;
      WORD * Distanze = (WORD*) malloc(256000);
      ORD_FORALL(Reti,i2){
         Reti.FixRec(i2);
         RETI_CUMULATIVE & Rete =  Reti.RecordCorrente();
         Bprintf2("OUTPUT: Rete : %i %s",Rete.CodiceRete,Rete.Nome);

         // Ordino le stazioni della Polimetrica:
         // - Prima le stazioni di transito
         // - Poi le altre
         // Le stazioni illegali non sono trasferite
         // Le stazioni che appartengono contemporaneamente ad altre polimetriche
         // cumulative sono promosse a stazioni di diramazione

         ARRAY_ID StazioniPolimetrica ;
         for (int k1 = 0; k1 < 8 ; k1 ++ ) {
            if(Rete.StazioneDitransito[k1])StazioniPolimetrica += Rete.StazioneDitransito[k1];
         } /* endfor */
         int NumTransito = StazioniPolimetrica.Dim();
         memset(Distanze,0,256000);

         Cum0.Seek(Rete.CodiceRete);
         while (&Cum0.RecordCorrente() && Cum0.RecordCorrente().CodiceRete == Rete.CodiceRete) {
            POLIM_CUMULAT & Poli = Cum0.RecordCorrente();
            ID StazioneRete = Poli.Id;
            if(!StazioniPolimetrica.Contiene(StazioneRete)) StazioniPolimetrica += StazioneRete;
            Bprintf3("Stazione in esame : %i %s",StazioneRete,Stazioni.DecodificaIdStazione(StazioneRete));
            // TRACEVSTRING2(StazioniPolimetrica.ToStringa());
            for (int j = 0 ; j< 8; j++ ) {
               if(Rete.StazioneDitransito[j] == 0)continue;
               WORD Km = Poli.Distanze[j];
               if(Km == 0) continue; // Non e' collegato
               int Posx = StazioniPolimetrica.Posizione(StazioneRete);
               int Posy = StazioniPolimetrica.Posizione(Rete.StazioneDitransito[j]);
               if(Rete.NumStazioni*Posx + Posy >= 128000){
                  TRACEVLONG(Posx);
                  TRACEVLONG(Posy);
                  TRACEVLONG(Rete.NumStazioni);
                  TRACEVLONG(Rete.NumStazioni*Posx + Posy );
                  Bprintf("Ecceduto limite strutturale");
                  BEEP;
                  exit(999);
               }
               Distanze[Rete.NumStazioni*Posx + Posy] = Km;
            } /* endfor */
            Cum0.Next();
//<<<    while (&Cum0.RecordCorrente() && Cum0.RecordCorrente().CodiceRete == Rete.CodiceRete) {
         } /* endwhile */

         char Buffer[2048];
         REC1 & Record1 = * (REC1*)Buffer;
         Record1.TipoRecord       = '1';
         sprintf(Record1.Ident,"CUM %i     ",Rete.CodiceRete);
         sprintf(Record1.NumStaz,"%3.3i",StazioniPolimetrica.Dim());
         Record1.Zona[0]='1'; Record1.Zona[1]='5';
         int Tipo = POLIMETRICA_CIP::CUMULATIVA;
         if (Rete.CodiceRete == 98) Tipo = POLIMETRICA_CIP::MARE_CUM;
         sprintf(Record1.Tipologia,"%2.2i",Tipo);
         sprintf(Record1.Descrizione,"%-50s",St(Rete.Nome));
         OutP.printf("%s*\r\n",&Record1);
         OutP.printf("2...../...../...../...../...../...../...../...../...../*\r\n");
         ORD_FORALL(StazioniPolimetrica,p1){
            REC3 & Record3       = * (REC3*)Buffer;
            Record3.TipoRecord   = '3' ;
            ID StazioneRete      = StazioniPolimetrica[p1];
            Bprintf3("Scarico stazione : %i %s",StazioneRete,Stazioni.DecodificaIdStazione(StazioneRete));
            STAZIONI::R_STRU Stz  = Stazioni[StazioneRete] ;
            int CodiceCCr = 0;
            // BOOL DiDiramazione = p1 < NumTransito || (Stz.CCRCumulativo1 > 0 && Stz.CCRCumulativo2 > 0);
            BOOL DiDiramazione = p1 < NumTransito ;
            if (Stz.SocietaConcessa1 == Rete.CodiceRete) {
               CodiceCCr = Stz.CCRCumulativo1;
            } else if (Stz.SocietaConcessa2 == Rete.CodiceRete) {
               CodiceCCr = Stz.CCRCumulativo2;
            } else {
               Bprintf("Incongruenza DSTASPXN-CUMULAT: La stazione %i %s non risulta appartenere alla rete %s", StazioneRete,Stazioni.DecodificaIdStazione(StazioneRete), Rete.Nome );
               // Imposto il codice CCR a 0 in modo da disabilitare la stazione
               sprintf(Record3.CodCCR,"00000");
               sprintf(Record3.NomeStaz,"%-20.20s", "Stazione Disabilitata da ignorare");
               CodiceCCr = -1;
            } /* endif */
            if( CodiceCCr == 0 && Stz.CodiceCCR) CodiceCCr = Stz.CodiceCCR;
            if( CodiceCCr == 0 && Stz.CCRCumulativo1) CodiceCCr = Stz.CCRCumulativo1;
            if( CodiceCCr >= 0){
               sprintf(Record3.CodCCR,"%5.5i",CodiceCCr);
               sprintf(Record3.NomeStaz,"%-20.20s", Stz.NomeStaz20 );
            };
            Record3.Diramazione = DiDiramazione ? '*' : ' ' ;
            Record3.Allacciamento         =' ';
            Record3.Soppressa             =' ';
            Record3.TransitoInternazionale=' ';
            Record3.Impresenziata         =' ';
            Record3.CoincidenzaConAutobus =' ';
            Record3.CasaCantoniera        =' ';
            Record3.TrattaSecondaria      =' ';
            Record3.NonAbilitata          =' ';
            Record3.DiInstradamento       =' ';
            for (int t = 0;t < 10 ;t++ ) {
               Record3.NodiC[t].Insieme[0]='0';
               Record3.NodiC[t].Insieme[1]='0';
               Record3.NodiC[t].Flag      =' ';
            } /* endfor */
            Record3.Filler          = '-';
            Record3.RadiceStazione  = ' ';
            Record3.RadiceStazione2 = ' ';
            Record3.Filler2         = '-';
            char * c = (char*) &Buffer[sizeof(REC3)];
            *c = 0; // Per la prima riga
            ORD_FORALL(StazioniPolimetrica,p2){
               if(p2 >= p1)break;
               c += sprintf(c,"%-4.4i",Distanze[Rete.NumStazioni*p1 + p2]);
               //TRACEVLONG(p1);
               //TRACEVLONG(p2);
               //TRACEVLONG(Distanze[Rete.NumStazioni*p1 + p2]);
            }
            OutP.printf("%s*\r\n",&Record3);
            //TRACESTRING((char*)&Record3);
//<<<    ORD_FORALL(StazioniPolimetrica,p1){
         }
//<<< ORD_FORALL(Reti,i2){
      };
      free(Distanze);
      ORD_FORALL(Cum1,i1){
         POLIM_CUMULAT_1 & Poli = Cum1[i1];
         Bprintf3("Polimetrica collegamento da staz. FS : %i %s",Poli.Id1,Stazioni.DecodificaIdStazione(Poli.Id1));
         Bprintf3("  a stazione cumulativa: %i %s",Poli.Id2,Stazioni.DecodificaIdStazione(Poli.Id2));
         char Buffer[2048];
         REC1 & Record1 = * (REC1*)Buffer;
         Record1.TipoRecord       = '1';
         sprintf(Record1.Ident,"%i->%i       ",Poli.Id1,Poli.Id2);
         sprintf(Record1.NumStaz,"%3.3i",2);
         Record1.Zona[0]='1'; Record1.Zona[1]='5';
         int Tipo = POLIMETRICA_CIP::URBANA_CUM;
         sprintf(Record1.Tipologia,"%2.2i",Tipo);
         sprintf(Record1.Descrizione,"%-50s","Polimetrica urbana collegamento FS -> concessa");
         OutP.printf("%s*\r\n",&Record1);
         OutP.printf("2...../...../...../...../...../...../...../...../...../*\r\n");
         ARRAY_ID StazioniPolimetrica;
         StazioniPolimetrica += Poli.Id1;
         StazioniPolimetrica += Poli.Id2;
         ORD_FORALL(StazioniPolimetrica,p1){
            REC3 & Record3       = * (REC3*)Buffer;
            Record3.TipoRecord   = '3' ;
            ID StazioneRete      = StazioniPolimetrica[p1];
            Bprintf3("Scarico stazione : %i %s",StazioneRete,Stazioni.DecodificaIdStazione(StazioneRete));
            STAZIONI::R_STRU Stz  = Stazioni[StazioneRete] ;
            if (Stz.CodiceCCR) {
               sprintf(Record3.CodCCR,"%5.5i",Stz.CodiceCCR);
            } else if (Stz.CCRCumulativo1) {
               sprintf(Record3.CodCCR,"%5.5i",Stz.CCRCumulativo1);
            } else if (Stz.CCRCumulativo2) {
               sprintf(Record3.CodCCR,"%5.5i",Stz.CCRCumulativo2);
            };
            sprintf(Record3.NomeStaz,"%-20.20s", Stz.NomeStaz20 );
            Record3.Diramazione           ='*';
            Record3.Allacciamento         =' ';
            Record3.Soppressa             =' ';
            Record3.TransitoInternazionale=' ';
            Record3.Impresenziata         =' ';
            Record3.CoincidenzaConAutobus =' ';
            Record3.CasaCantoniera        =' ';
            Record3.TrattaSecondaria      =' ';
            Record3.NonAbilitata          =' ';
            Record3.DiInstradamento       =' ';
            for (int t = 0;t < 10 ;t++ ) {
               Record3.NodiC[t].Insieme[0]='0';
               Record3.NodiC[t].Insieme[1]='0';
               Record3.NodiC[t].Flag      =' ';
            } /* endfor */
            Record3.Filler          = '-';
            Record3.RadiceStazione  = ' ';
            Record3.RadiceStazione2 = ' ';
            Record3.Filler2         = '-';
            char * c = (char*) &Buffer[sizeof(REC3)];
            *c = 0; // Per la prima riga
            ORD_FORALL(StazioniPolimetrica,p2){
               if(p2 >= p1)break;
               c += sprintf(c,"%-4.4i",0);
            }
            OutP.printf("%s*\r\n",&Record3);
//<<<    ORD_FORALL(StazioniPolimetrica,p1){
         }
//<<< ORD_FORALL(Cum1,i1){
      };
      #endif
   }

   // ---------------------------------------------------------
   // Scansione preliminare: per acquisire le polimetriche cumulative
   // ---------------------------------------------------------
   BOOL HoCumulativo = TRUE;
   {
      // FILE_RO Poli(PATH_POLIM "POLIMETR.UFF");
      FILE_RO Poli(PATH_POLIM "POLIMETR.CED"); // Versione modificata
      FILE_RO Poli2(PATH_INTERMED "POLICUM.TMP");
      FILE_RW PoliOut(PATH_POLIM "POLIMETR.TMP");
      PoliOut.SetSize(0);
      if(Poli2.FileHandle() == 0 || Poli2.FileSize() == 0 ){
         Bprintf("====================================================================");
         Bprintf("===    NON SONO PRESENTI LE POLIMETRICHE DEL SERVIZIO CUMULATIVO ===");
         Bprintf("====================================================================");
         HoCumulativo = FALSE;
      }

      while(Poli.gets(Linea,4096)  ){ // Concatena i files
         if(Linea[0] == '4')continue; // Ignoro i records di tipo 4
         if(Linea[0] == '1' && Linea.Dim() >= sizeof(REC1) + 49){
            Bprintf2("Descrizione piu' lunga di 50 caratteri: %s",(CPSZ)Linea);
            Linea = Linea( 0, sizeof(REC1) + 48 )+"*";
         };
         PoliOut.printf("%s\r\n",(CPSZ)Linea);
      }
      while(Poli2.gets(Linea,4096) ){ // Concatena i files
         if(Linea[0] == '4')continue; // Ignoro i records di tipo 4
         PoliOut.printf("%s\r\n",(CPSZ)Linea);
      }
   }



   // ---------------------------------------------------------
   // Gestione delle stazioni di instradamento eccezionale
   // ---------------------------------------------------------
   // A regime non ve ne debbono essere, possono essere usate per
   // gestire situazioni transitorie.
   ARRAY_ID StazioniEccezionaliDiInstradamento;
   // Le carico dal file StzIsEcc.MIO
   if(TestFileExistance(PATH_POLIM "StzIsEcc.MIO")){
      FILE_RO Ecc(PATH_POLIM "StzIsEcc.MIO");
      while(Ecc.gets(Linea,80)){
         int Id = Linea.ToInt();
         if(Id)StazioniEccezionaliDiInstradamento += Id;
      }
   }


   TRACESTRING("Allocazione Aree");
   POLIMETRICA_CIP * Polimetriche = new POLIMETRICA_CIP[TOPPOLI];
   STAZ_POLI_CIP   * Staz         = new STAZ_POLI_CIP[Stazioni.Dim()];

   memset(Staz, 0, sizeof(STAZ_POLI_CIP  ) *Stazioni.Dim()); // EMS003 Win

   TRACEVLONG(sizeof(POLIMETRICA_CIP) *TOPPOLI       );
   TRACEVLONG(Stazioni.Dim()); // EMS Win
   TRACEVLONG(sizeof(WORD));
   TRACEVLONG(sizeof(STAZ_POLI_CIP  ) *Stazioni.Dim());

   FILE_RO Poli(PATH_POLIM "POLIMETR.TMP");


   // Apro l' archivio decodifica codici CCR
   CCR_ID Ccr_Id(PATH_DATI);

   Bprintf3("Prima scansione: Carico i dati come presenti in archivio (in forma triangolare)");
   POLIMETRICA_CIP * Polim = NULL;
   while(Poli.gets(Linea,4096)){
      if(Linea.Last() == '*'){
         Linea[Linea.Dim()-1] = 0;
      } else {
         Bprintf("Errore: record anomalo, fermo l' applicazione");
         Bprintf("Record: %s",(CPSZ)Linea);
         BEEP;
         exit (999);
      }
      if (Linea[0] == '1') {
         Bprintf("%s",(CPSZ)Linea);
         REC1 & Rec1 = *(REC1*)(CPSZ)Linea;
         POLIMETRICA_CIP & Polimetrica = Polimetriche[++NumPolimetriche];
         Polim = &Polimetrica;
         Polimetrica.IdPolimetrica   = NumPolimetriche;
         Polimetrica.Zona            = It(Rec1.Zona);
         strcpy(Polimetrica.Nome,(CPSZ)STRINGA(St(Rec1.Ident)).Strip());
         strcpy(Polimetrica.Descrizione,(CPSZ)STRINGA(Rec1.Descrizione).Strip());
         Polimetrica.TipoPolimetrica = It(Rec1.Tipologia);
         //if (!strcmp(Polimetrica.Descrizione,"Tavola di allacciamento")) {
         //   Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::ALLACCIAMENTO;
         //} else if (!strcmp(Polimetrica.Descrizione,"Tavola di diramazione")) {
         //   Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::DIRAMAZIONE;
         //} else if(Polimetrica.Zona == 15){
         //   Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::CUMULATIVA;
         //   if (!strcmp(Polimetrica.Descrizione,"Tirrenia")) {
         //      Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::MARE_CUM;
         //   }
         //} else {
         //   Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::LOCALE;
         //   if (!strcmp(Polimetrica.Descrizione,"Da Civitavecchia Marittima a Golfo Aranci")) {
         //      Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::MARE_FS;
         //   }
         //} /* endif */
//<<< if (Linea[0] == '1') {
      } else if (Linea[0] == '2') {
         Bprintf3("%s",(CPSZ)Linea);
         REC2 & Rec2 = *(REC2*)(CPSZ)Linea;
         for (int i = 0;i < 9 ;i ++ ) {
            REC2::RADICI & Rad = Rec2.Radici[i];
            if (memcmp(Rad.CCR_Radice,".....",5)) {
               int Ccr = It(Rad.CCR_Radice);
               int Result = Ccr_Id.CercaCCR(Ccr);
               if (!Result) {
                  Bprintf("ERRORE GRAVE: Codice CCR illegale SU RADICE : %i Polimetrica %s",Ccr,Polim->Descrizione);
                  BEEP;
                  ReturnCode = 123;
               } /* endif */
               Polim->Radici += Result;
            } /* endif */
         } /* endfor */
      } else if (Linea[0] == '3') {
         Bprintf("%s",(CPSZ)Linea);    // EMS004 Win
         REC3 & Rec3 = *(REC3*)(CPSZ)Linea;
         int Ccr = It(Rec3.CodCCR);
         char * Distanze = (char*)(CPSZ)Linea + sizeof(REC3);

         // Verifico univocita' corrispondenze codici ccr <-> Nomi
         STRINGA NomeStaz(St(Rec3.NomeStaz));
         NomeStaz.Strip();
         STRINGA NomeStaz2 = NomeStaz;
         NomeStaz2.UpCase();
         if (VerificaCCR[Ccr]) {
            if( *VerificaCCR[Ccr] != NomeStaz2 && Ccr){
               Out.printf("Warning: il codice CCR %i e' utilizzato sia per la stazione '%s' che per '%s'\r\n",
                  Ccr,(CPSZ)*VerificaCCR[Ccr],(CPSZ)NomeStaz);
            }
         } else {
            VerificaCCR[Ccr] = new STRINGA(NomeStaz2);
         } /* endif */
         STRINGA NOMESTAZ(Normalizza(NomeStaz));

         // ..............................................
         // Identicazione stazione corrispondente al CCR
         // ..............................................
         int Result = Ccr_Id.CercaCCR(Ccr);
         if(Result == 0){      // Non identificata
            TRACEINT("Non trovato codice CCR su stazione "+NomeStaz,Ccr);
            // Provo a vedere se la identifico per nome
            StaNomi.Posiziona(NOMESTAZ);
            STRINGA Nome_Stazione(Normalizza(StaNomi.RecordCorrente().NomeStazione));
            if( NOMESTAZ == Nome_Stazione) {
               // Ok: Found
               Result = StaNomi.RecordCorrente().IdStazione;
               Polim->Stazioni += Result;
               Bprintf2("Assegnata per nome (perche' il CCR %i non esiste) la stazione %i %s",Ccr,Result,StaNomi.RecordCorrente().NomeStazione);
               PerNome ++;
               Valide ++;
            } else {
               TRACESTRING("NOT Found: First = '"+Nome_Stazione+"' Da '"+NomeStaz+"'");
               if(Result < Stazioni.Dim() - 1){
                  StaNomi.Next();
                  Nome_Stazione = Normalizza(StaNomi.RecordCorrente().NomeStazione);
               }
               if( NOMESTAZ == Nome_Stazione) {
                  // Ok: Found
                  Result = StaNomi.RecordCorrente().IdStazione;
                  Polim->Stazioni += Result;
                  Bprintf2("Assegnata per nome (perche' il CCR %i non esiste) la stazione %i %s",Ccr,Result,StaNomi.RecordCorrente().NomeStazione);
                  PerNome ++;
                  Valide ++;
               } else {
                  TRACESTRING("NOT Found: Second = '"+Nome_Stazione+"' Da "+NomeStaz+"'");
                  Bprintf("Scartata (perche' non presente in DSTASPXN o senza codice DKM) stazione con codice ccr %i %s",Ccr,(CPSZ)NomeStaz);
                  Polim->Stazioni += 0;
                  Result = 0;
                  Scartate ++;
               }
//<<<       if( NOMESTAZ == Nome_Stazione) {
            } /* endif */
//<<<    if(Result == 0){      // Non identificata
         } else if(Result < 0){      // Non presente sul grafo
            Bprintf("WARNING: Non presente su grafo stazione con codice ccr %i %s",Ccr,(CPSZ)NomeStaz);
            NonGrafo ++;
            Result = Ccr_Id[Ccr];
            // Provo a vedere se il nome corrisponde
            if(NomeStaz == STRINGA(Stazioni[Result].NomeStazione)(0,19)) {
               TRACESTRING("I nomi corrispondono : OK comunque");
            } else {
               Bprintf2("I nomi NON corrispondono : controllare");
            } /* endif */
            Polim->Stazioni += Result;
            Valide ++;
         } else {
            Polim->Stazioni += Result;
            Valide ++;
         }
         // if(Result == 1) Bprintf2("Codice CCR che mi ha dato result = 1: %i Staz. %s",Ccr,(CPSZ)NomeStaz);
         int NumPolim;
         if(Result){ // Se la stazione e' valida
            if(Polim->Radici.Dim() == 0)Polim->Radici += Result; // Polimetrica senza rami secondari
            // Identifico la polimetrica
            for (NumPolim=0; NumPolim < MAXPOLI ; ++NumPolim ) {
               if(Staz[Result].Polimetriche[NumPolim].Id == NumPolimetriche) {
                  Bprintf("%s",(CPSZ)STRINGA("Errore: la stazione ")+NomeStaz+" compare piu' volte sulla polimetrica "+Polim->Descrizione);
                  BEEP;
                  Result = 0; // Inibisco la stazione
                  Polim->Stazioni.Last() = 0; // Elimino dall' elenco stazioni
                  break;
               } else if(Staz[Result].Polimetriche[NumPolim].Id == 0) {
                  Staz[Result].Polimetriche[NumPolim].Id = NumPolimetriche;
                  Staz[Result].Polimetriche[NumPolim].Tipo = Polim->TipoPolimetrica;
                  break;
               } /* endif */
            } /* endfor */
         }

         // ..............................................
         // Elaborazione dati della stazione
         // ..............................................
         if(Result){ // Se la stazione e' valida

            // Identificazione dati di instradamento condizionale
            IsIniB.Clear(); IsC.Clear();
            for (int x=0;x < 10  ; x++ ) {
               int Is = It(Rec3.NodiC[x].Insieme);
               if (Is) {
                  if (Rec3.NodiC[x].Flag == '*'){
                     IsIniB.Set(Is-1);
                  } else {
                     IsC.Set(Is-1);
                  } /* endif */
               } /* endif */
            } /* endfor */
            FORALL(IsCnd,x1){
               if(IsCnd[x1] == IsC)break;
            };
            if(x1 < 0){
               x1 = IsCnd.Dim();
               IsCnd += IsC; // Nuovo elemento
            }
            FORALL(IsCnd,x2){
               if(IsCnd[x2] == IsIniB)break;
            };
            if(x2 < 0){
               x2 = IsCnd.Dim();
               IsCnd += IsIniB; // Nuovo elemento
            }
            if (Staz[Result].IsCnd == 0 ) Staz[Result].IsCnd       = x1;
            if (Staz[Result].IsCndInibit == 0 ) Staz[Result].IsCndInibit = x2;

            // Verifica della relazione stazione -> Polimetrica
            if (NumPolim >= MAXPOLI ) {
               // EMS004 Win aggiunto trace di NumPolim
               Bprintf("Errore: La stazione %s e' presente su %i >= %i Polimetriche", (CPSZ)NomeStaz,NumPolim,MAXPOLI);
               Bprintf("Result = %i", Result);
               BEEP;
               exit(999);
            } /* endif */
            if (Polim->TipoPolimetrica == POLIMETRICA_CIP::ALLACCIAMENTO) {
               if(Rec3.Allacciamento == '*' ){
                  Staz[Result].Allacciamento          = TRUE;
               } else {
                  STRINGA Msg = "Stazione NON di allacciamento("+NomeStaz+") su polimetrica di allacciamento "+Polim->Nome+" "+STRINGA(Polim->Descrizione);
                  Bprintf("%s",(CPSZ)Msg);
               }
            } else {
               if(Rec3.Allacciamento == '*' && ! Staz[Result].Allacciamento ){
                  STRINGA Msg = "Stazione di allacciamento ANOMALA ("+NomeStaz+") perche' dichiarata su polimetrica NORMALE (= NON di allacciamento) "+Polim->Nome+" "+STRINGA(Polim->Descrizione);
                  ERRSTRING(Msg);
                  #ifdef DBGANOMALE
                  Out.printf("%s\r\n",(CPSZ)Msg);
                  #endif
               }
//<<<       if (Polim->TipoPolimetrica == POLIMETRICA_CIP::ALLACCIAMENTO) {
            } /* endif */
            if (Polim->TipoPolimetrica == POLIMETRICA_CIP::DIRAMAZIONE ||
               Polim->TipoPolimetrica == POLIMETRICA_CIP::ALLACCIAMENTO) {
               if(Rec3.Diramazione == '*' ){
                  Staz[Result].Diramazione          = TRUE;
                  Staz[Result].DiramazioneAnomala   = FALSE;
               } else {
                  STRINGA Msg = "Stazione NON di diramazione ("+NomeStaz+") su polimetrica di diramazione "+Polim->Nome+" "+STRINGA(Polim->Descrizione);
                  ERRSTRING(Msg);
                  Out.printf("%s\r\n",(CPSZ)Msg);
               }
            } else if(Rec3.Diramazione == '*') {
               if (Polim->TipoPolimetrica == POLIMETRICA_CIP::CUMULATIVA  ||
                  Polim->TipoPolimetrica == POLIMETRICA_CIP::URBANA_CUM   ||
                  Polim->TipoPolimetrica == POLIMETRICA_CIP::MARE_CUM  ) {
                  Staz[Result].Diramazione = TRUE; // Ok : promossa a stazione di diramazione
               } else if( ! Staz[Result].Diramazione ){
                  STRINGA Msg = "Stazione di diramazione ANOMALA ("+NomeStaz+") perche' dichiarata su polimetrica NORMALE ( = NON di diramazione ) "+Polim->Nome+" "+STRINGA(Polim->Descrizione);
                  Staz[Result].DiramazioneAnomala   = TRUE;
                  ERRSTRING(Msg);
                  #ifdef DBGANOMALE
                  Out.printf("%s\r\n",(CPSZ)Msg);
                  #endif
               } /* endif */
//<<<       if (Polim->TipoPolimetrica == POLIMETRICA_CIP::DIRAMAZIONE ||
            } /* endif */

            // Dati della stazione desunti dalla Polimetrica
            if(Rec3.Soppressa               == '*' ) Staz[Result].Soppressa              = TRUE;
            if(Rec3.TransitoInternazionale  == '*' ) Staz[Result].TransitoInternazionale = TRUE;
            if(Rec3.Impresenziata           == '*' ) Staz[Result].Impresenziata          = TRUE;
            if(Rec3.CoincidenzaConAutobus   == '*' ) Staz[Result].CoincidenzaConAutobus  = TRUE;
            if(Rec3.CasaCantoniera          == '*' ) Staz[Result].CasaCantoniera         = TRUE;
            if(Rec3.TrattaSecondaria        != ' ' ) Staz[Result].TrattaSecondaria       = TRUE;
            if (Polim->TipoPolimetrica == POLIMETRICA_CIP::ALLACCIAMENTO && ! Staz[Result].Allacciamento ) {
               BEEP;
               exit(998);
            } /* endif */

            // Dati della relazione stazione -> Polimetrica
            // Radice
            BYTE & NmRad = Staz[Result].Polimetriche[NumPolim].Livello;
            if (Rec3.RadiceStazione == ' ') {
               NmRad = 0;
            } else {
               NmRad = Rec3.RadiceStazione - '1';
            } /* endif */
            if (NmRad >= Polim->Radici.Dim() ) {
               Bprintf("Stazione Invertita: compare prima della radice, Stz %i %s Polim %s",Result,(CPSZ)NomeStaz,Polim->Descrizione);
               TRACEVLONG(NmRad);
               TRACEVLONG(Polim->Radici.Dim());
            } /* endif */
            // Distanze
            ARRAY_ID & ADistanze =Staz[Result].Polimetriche[NumPolim].Distanze;
            int NumStazioni = 0;
            while (*Distanze) {
               NumStazioni ++;
               if (Polim->TipoPolimetrica == POLIMETRICA_CIP::MARE_FS) {
                  // Per Civitavecchia - Golfo aranci forzo 300 KM
                  ADistanze += 300;
               } else {
                  ADistanze += StringToInt(Distanze,4);
               } /* endif */
               Distanze +=4;
            } /* endwhile */
            if(NumStazioni != Polim->Stazioni.Dim()-1){
               Bprintf("Errore: La stazione %s ha %i distanze invece di %i sulla polimetrica %s",(CPSZ)NomeStaz,NumStazioni,Polim->Stazioni.Dim(),(CPSZ)Polim->Nome);
               BEEP;
               exit(999);
            };
            //if(Polim->TipoPolimetrica == POLIMETRICA_CIP::DIRAMAZIONE){
            //   Staz[Result].Zone |= Polim->Zone;
            //};
//<<<    if(Result){ // Se la stazione e' valida
         }
//<<< if (Linea[0] == '1') {
      } else {
         Bprintf("Errore: record di tipo non riconoscibile, fermo l' applicazione");
         BEEP;
         exit (999);
      } /* endif */
//<<< while(Poli.gets(Linea,4096)){
   }

   Out.printf("Vi sono %i differenti insiemi di stazioni di instradamento condizionale\r\n",IsCnd.Dim());
   // Printout delle combinazioni
   //ORD_FORALL(IsCnd,xx) {
   //   Out.printf("IsCnd[%i] = %s\n",xx,(CPSZ)IsCnd[xx]);
   //} /* endfor */

   ZeroFill(Istogram);
   Bprintf("Controllo di TUTTE le stazioni");
   Bprintf(".......................................................");
   Bprintf("LE STAZIONI NON VENDIBILI, SE NON NODALI NE' TERMINALI,");
   Bprintf("SARANNO ELIMINATE DA GRAFO E POLIMETRICHE");
   Bprintf(".......................................................");
   ORD_FORALL(Stazioni,t){
      if(! (HoCumulativo || Stazioni[t].Fs()))continue; // Evito segnalazioni se non ho il cumulativo
      int NumPolim = Staz[t].NumeroPolimetriche();
      if (NumPolim == 0) {
         if (Stazioni[t].Vendibile()){
            VendibiliNoPolim ++; // Nessuna polimetrica definita
            Bprintf("La stazione vendibile %i %s NON compare su alcuna polimetrica",t,Stazioni.DecodificaIdStazione(t));
         }
      } else {
         Istogram[NumPolim-1]++;
         if (!Stazioni[t].Vendibile() ){
            NonVendibiliPolim ++;
            Bprintf2("La stazione NON vendibile %i %s compare su una o piu' polimetriche",t,Stazioni.DecodificaIdStazione(t));
         }
      } /* endif */
   } /* endfor */

   Bprintf(".......................................................");
   Bprintf("Fine prima scansione: passati %i secondi",(Time()-Tm1)/100);
   Bprintf("Numero Polimetriche: %i",NumPolimetriche);
   Bprintf("Numero Stazioni/Polimetrica Scartate: %i",Scartate);
   // Bprintf("Numero Stazioni/Polimetrica Scartate perche' NON FS : %i",NonFS);
   Bprintf("Numero Stazioni/Polimetrica assegnate Per Nome: %i",PerNome);
   Bprintf("Numero Stazioni/Polimetrica senza riscontro su grafo: %i",NonGrafo);
   Bprintf("Numero Stazioni/Polimetrica valide: %i",Valide);
   Bprintf("Numero Stazioni FS vendibili non presenti su polimetrica: %i",VendibiliNoPolim);
   Bprintf("Numero Stazioni FS non vendibili presenti su polimetrica: %i",NonVendibiliPolim);
   for (int p = 0;p <MAXPOLI ;p++ ) if(Istogram[p])Out.printf("Numero Stazioni con %i Polimetriche : %i\r\n",p+1,Istogram[p]);
   Bprintf(".......................................................");

   // Statistiche
   ELENCO_S Legenda;
   Legenda += "Tutte le polimetriche"                           ;
   Legenda += "Polimetriche Cumulative"                         ;
   Legenda += "Polimetriche Cumulative con instradamento"       ;
   Legenda += "Polimetriche Cumulative mare"                    ;
   Legenda += "Polimetriche urbane collegamento con cumulativo" ;
   Legenda += "Polimetriche locali"                             ;
   Legenda += "Polimetriche di diramazione"                     ;
   Legenda += "Polimetriche di allacciamento"                   ;
   Legenda += "Polimetriche mare FS"                            ;

   for (BYTE Tipo_P=0;Tipo_P <=8 ;Tipo_P ++ ) {
      int Tot = 0;
      ORD_FORALL(Stazioni,t) Tot += Staz[t].NumeroPolimetriche(Tipo_P);
      if (Tot == 0)continue;
      Out.printf("%s\r\n","");
      Out.printf("%s\r\n","====================================================");
      Out.printf("Statistiche per: %s\r\n",(CPSZ)Legenda[Tipo_P]);
      Out.printf("%s\r\n","====================================================");
      {
         ZeroFill(Istogram);
         Tot = 0;
         ORD_FORALL(Stazioni,t){
            if(!Staz[t].Allacciamento)continue;
            int NumPolim = Staz[t].NumeroPolimetriche(Tipo_P);
            if (NumPolim > 0) Istogram[NumPolim-1]++;
            Tot += NumPolim;
         }
         if (Tot) {
            Out.printf("%s\r\n"," Statistiche per SOLE stazioni di ALLACCIAMENTO");
            for (int p = 0;p <MAXPOLI ;p++ ) if(Istogram[p])Out.printf("Numero Stazioni con %i Polimetriche : %i\r\n",p+1,Istogram[p]);
         }
      }
      {
         ZeroFill(Istogram);
         Tot = 0;
         ORD_FORALL(Stazioni,t){
            if(!Staz[t].Diramazione)continue;
            int NumPolim = Staz[t].NumeroPolimetriche(Tipo_P);
            if (NumPolim > 0) Istogram[NumPolim-1]++;
            Tot += NumPolim;
         }
         if (Tot) {
            Out.printf("%s\r\n"," Statistiche per SOLE stazioni di DIRAMAZIONE");
            for (int p = 0;p <MAXPOLI ;p++ ) if(Istogram[p])Out.printf("Numero Stazioni con %i Polimetriche : %i\r\n",p+1,Istogram[p]);
         }
      }
      {
         ZeroFill(Istogram);
         Tot = 0;
         ORD_FORALL(Stazioni,t){
            if(!Staz[t].DiramazioneAnomala)continue;
            int NumPolim = Staz[t].NumeroPolimetriche(Tipo_P);
            if (NumPolim > 0) Istogram[NumPolim-1]++;
            Tot += NumPolim;
         }
         if (Tot) {
            Out.printf("%s\r\n"," Statistiche per SOLE stazioni di DIRAMAZIONE ANOMALA");
            for (int p = 0;p <MAXPOLI ;p++ ) if(Istogram[p])Out.printf("Numero Stazioni con %i Polimetriche : %i\r\n",p+1,Istogram[p]);
         }
      }
      {
         ZeroFill(Istogram);
         Tot = 0;
         ORD_FORALL(Stazioni,t){
            // if(Staz[t].DiramazioneAnomala)continue;
            if(Staz[t].Diramazione)continue;
            if(Staz[t].Allacciamento)continue;
            int NumPolim = Staz[t].NumeroPolimetriche(Tipo_P);
            if (NumPolim > 0) Istogram[NumPolim-1]++;
            Tot += NumPolim;
         }
         if (Tot) {
            Out.printf("%s\r\n"," Statistiche per stazioni NORMALI");
            for (int p = 0;p <MAXPOLI ;p++ ) if(Istogram[p])Out.printf("Numero Stazioni con %i Polimetriche : %i\r\n",p+1,Istogram[p]);
         }
      }
//<<< for (BYTE Tipo_P=0;Tipo_P <=8 ;Tipo_P ++ ) {
   } /* endfor */
   Out.printf("%s\r\n","====================================================");
   Out.printf("%s\r\n","");
   Out.printf("%s\r\n","Stazioni di DIRAMAZIONE che appartengono ad UNA SOLA polimetrica");
   {
      ORD_FORALL(Stazioni,t){
         if(!Staz[t].Diramazione)continue;
         int NumPolim = Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::LOCALE) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::SOLO_LOCALE) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::CUMULATIVA) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::URBANA_CUM) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::MARE_CUM) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::URBANA_CUM) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::MARE_FS);
         if (NumPolim ==1)Out.printf("%s\r\n",Stazioni[t].NomeStazione);
      }
   }
   Out.printf("%s\r\n","");
   Out.printf("%s\r\n","Stazioni di DIRAMAZIONE che NON sono NODI (non e' un errore )");
   {
      ORD_FORALL(Stazioni,t){
         if(!Staz[t].Diramazione)continue;
         if (Stazioni[t].TipoStazione != ST_NODO)Out.printf("%s\r\n",Stazioni[t].NomeStazione);
      }
   }
   Out.printf("%s\r\n","");
   Out.printf("%s\r\n","Stazioni NORMALI che appartengono a piu' di una polimetrica locale");
   {
      ORD_FORALL(Stazioni,t){
         if(Staz[t].Diramazione)continue;
         if(Staz[t].Allacciamento)continue;
         int NumPolim = Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::LOCALE) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::SOLO_LOCALE) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::CUMULATIVA) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::URBANA_CUM) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::MARE_CUM) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::URBANA_CUM) +
         Staz[t].NumeroPolimetriche(POLIMETRICA_CIP::MARE_FS);
         if (NumPolim >1)Out.printf("%s\r\n",Stazioni[t].NomeStazione);
      }
   }
   Out.printf("%s\r\n","");

   // ---------------------------------------------------------
   // Qui potrei inserire un controllo incrociato con il grafo
   // ---------------------------------------------------------

   // ---------------------------------------------------------
   // Completamento della matrice: da triangolare a quadrata
   // ---------------------------------------------------------
   for (int i1 = 0; i1 <= NumPolimetriche ; i1 ++) {
      POLIMETRICA_CIP & Polimetrica = Polimetriche[i1];
      int TargetDim = Polimetrica.Stazioni.Dim();
      ORD_FORALL(Polimetrica.Stazioni,i2){
         if(Polimetrica.Stazioni[i2] != 0 ) {
            STAZ_POLI_CIP::LEGAMI & Legami = Staz[Polimetrica.Stazioni[i2]].DaIdPolim(i1);
            if(&Legami != NULL){
               ARRAY_ID & Distanze =  Legami.Distanze;
               if(Distanze.Dim() != i2){
                  Bprintf("%s",(CPSZ)STRINGA("Anomalia: Matrice distanze NON e' triangolare polimetrica ")+STRINGA(Polimetrica.Descrizione)+" stazione "+Stazioni[Polimetrica.Stazioni[i2]].NomeStazione);
                  ERRSTRING(" L' anomalia e' probabilmente dovuta a confusione sui codici");
                  TRACEVLONG(Distanze.Dim());
                  TRACEVLONG(i2);
                  TRACEVSTRING2(Stazioni[Polimetrica.Stazioni[i2]].NomeStazione);
                  BEEP;
                  ReturnCode = 123;
               }
               Distanze += 0; // Collegamento stazione con se stessa = 0 Km
               for (int i4 = Distanze.Dim(); i4 <TargetDim ; i4++) {
                  if(Polimetrica.Stazioni[i4] == 0 ) { // Stazione NON valida
                     Distanze += 0;
                  } else {
                     STAZ_POLI_CIP::LEGAMI & Leg2 = Staz[Polimetrica.Stazioni[i4]].DaIdPolim(i1);
                     if(&Leg2 == NULL){
                        Bprintf("??? Legami (2) inconsistenti");
                        BEEP;
                        exit (999);
                     }
                     Distanze += Leg2.Distanze[i2];
                  }
               } /* endfor */
//<<<       if(&Legami != NULL){
            } else {
               Bprintf("??? Legami inconsistenti");
               BEEP;
               exit (999);
            }
//<<<    if(Polimetrica.Stazioni[i2] != 0 ) {
         } /* endif */
//<<< ORD_FORALL(Polimetrica.Stazioni,i2){
      }
//<<< for (int i1 = 0; i1 <= NumPolimetriche ; i1 ++) {
   } /* endfor */
   Out.printf("Fine completamento matrice: passati %i secondi\r\n",(Time()-Tm1)/100);

   // ---------------------------------------------------------
   // Elimino stazioni non valide dalla polimetrica
   // ---------------------------------------------------------
   // ATTENZIONE: VENGONO ELIMINATE ANCHE LE STAZIONI NON VENDIBILI
   // PURCHE' NON NODI NE STAZIONI TERMINALI
   // ---------------------------------------------------------
   for (int j1 = 1; j1 <= NumPolimetriche ; j1 ++) {
      POLIMETRICA_CIP & Polimetrica = Polimetriche[j1];
      FORALL(Polimetrica.Stazioni,i2){
         STAZIONI::R_STRU & Stazione = Stazioni[Polimetrica.Stazioni[i2]];
         if(Stazione.IdStazione &&
            !( Stazione.Vendibile()                       ||
               Stazione.TipoStazione == ST_NODO           ||
               Stazione.TipoStazione == ST_TERMINALE      ||
               Staz[Polimetrica.Stazioni[i2]].Diramazione ||
               Stazione.CodiceInstradamentoCvb > 0 )){
            // Elimino anche questa stazione da polimetrica e grafo
            Bprintf2("Eliminata stazione Nø %i NON VENDIBILE %s da polimetrica %s %s",i2,Stazione.NomeStazione,Polimetrica.Nome,Polimetrica.Descrizione);
            Polimetrica.Stazioni[i2] = 0;
         }
      }
      FORALL(Polimetrica.Stazioni,j2){
         ID IdStaz = Polimetrica.Stazioni[j2];
         if(IdStaz != 0 ){
            STAZ_POLI_CIP & Stazio = Staz[IdStaz];
            STAZ_POLI_CIP::LEGAMI & Legami = Stazio.DaIdPolim(Polimetrica.IdPolimetrica);
            if(Legami.Distanze.Dim() != Polimetrica.Stazioni.Dim()){
               Bprintf("Anomalia di caricamento polimetrica "+STRINGA(Polimetrica.Descrizione));
               ERRSTRING(" L' anomalia e' probabilmente dovuta a confusione sui codici");
               TRACEVLONG(Legami.Distanze.Dim());
               TRACEVLONG(Polimetrica.Stazioni.Dim());
               TRACEVLONG(j1);
               TRACEVLONG(j2);
               TRACEVSTRING2(Stazioni[Polimetrica.Stazioni[j2]].NomeStazione);
            };
            FORALL(Legami.Distanze,j3){
               if (Polimetrica.Stazioni[j3]==0) {
                  Legami.Distanze -= j3;
               } /* endif */
            }
         };
      }
      FORALL(Polimetrica.Stazioni,jj2){
         ID IdStaz = Polimetrica.Stazioni[jj2];
         if(IdStaz == 0 ){
            Bprintf3("Elimino stazione Nø %i dalla polimetrica %s",jj2,Polimetrica.Descrizione);
            Polimetrica.Stazioni -= jj2;
         };
      }
//<<< for (int j1 = 1; j1 <= NumPolimetriche ; j1 ++) {
   } /* endfor */
   Out.printf("Fine Eliminazioni stazioni non valide dalle polimetriche %i secondi\r\n",(Time()-Tm1)/100);

   // ---------------------------------------------------------
   // Generazione grafo (senza polimetriche)
   // ---------------------------------------------------------
   BUFR DatiRami;
   {
      FILE_RO Rami(PATH_INTERMED "RAMI.TMP");
      Rami.Leggi(Rami.FileSize(),DatiRami);
   }
   ETAB_RAMO * ETAB_Rami = (ETAB_RAMO*) DatiRami.Dati;
   int NumRami = DatiRami.Dim() / sizeof(ETAB_RAMO);
   ARRAY_ID  * RamiStaz  = new ARRAY_ID[NumRami + 1];

   int E_Pv=0,E_Pp=0,E_Dc=0,E_Di=0,E_Dd=0,E_Sr = 0;

   GRAFO Grafo(NUSTR);
   //TRACEPOINTER("GG 995 GRAFO::Grafo",GRAFO::Grafo);
   //TRACEVLONG(Stazioni.Dim());
   Grafo.StazioniGrafo = new STAZ_FS[Stazioni.Dim()];
   memset(Grafo.StazioniGrafo,0,sizeof(STAZ_FS)*Stazioni.Dim());
   Grafo.TotStazioniGrafo = 1  ; // L' elemento 0 e' vuoto
   Grafo.Rami = new RAMO[NumRami + 1];
   memset(Grafo.Rami,0,sizeof(RAMO)*(NumRami + 1));
   Grafo.TotRami = 1  ; // L' elemento 0 e' un ramo non valido !
   RAMO * RamiSimmetrici = new RAMO[2*NumRami];
   do {
      RAMO & Ramo = Grafo.Rami[Grafo.TotRami];
      ETAB_RAMO & ETAB_Ramo = ETAB_Rami[Grafo.TotRami-1];
      Ramo.IdRamo  = Chk(ETAB_Ramo.IdRamo,10)  ;
      Ramo.IdStaz1 = Chk(ETAB_Ramo.St1,13)     ;
      Ramo.IdStaz2 = Chk(ETAB_Ramo.St2,13)     ;
      Ramo.KmRamo  = Chk(ETAB_Ramo.Km,9)       ;
      Ramo.Mare    = Chk(ETAB_Ramo.Mare,1)     ;
      Ramo.Concesso= Chk(ETAB_Ramo.Concesso,1) ;
      Ramo.CodConcessa= Chk(ETAB_Ramo.SocCon,7) ;
      RamiSimmetrici[2*Grafo.TotRami -2] = Ramo;
      RamiSimmetrici[2*Grafo.TotRami -1] = Ramo;
      // Inversione
      RamiSimmetrici[2*Grafo.TotRami -1].IdStaz1 = ETAB_Ramo.St2 ;
      RamiSimmetrici[2*Grafo.TotRami -1].IdStaz2 = ETAB_Ramo.St1 ;
      #ifdef TRACERAMI1
      TRACESTRING("----------------");
      TRACEVLONG(Ramo.IdRamo );
      TRACEVLONG(Ramo.IdStaz1);
      TRACEVLONG(Ramo.IdStaz2);
      TRACEVLONG(Ramo.KmRamo );
      TRACEVLONG(Ramo.Mare    );
      TRACEVLONG(Ramo.Concesso);
      TRACEVLONG(Ramo.CodConcessa);
      #endif
      Grafo.TotRami ++;
//<<< do {
   } while ( Grafo.TotRami <= NumRami ); /* enddo */
   qsort(RamiSimmetrici,NumRami * 2,sizeof(RAMO),Cmp_RAMO);
   int PSimmetrici=0;

   //TRACEPOINTER("GG 1034",GRAFO::Grafo);

   Grafo.Polim = new POLIMETRICA[MAX_POLIMETRICA];
   memset(Grafo.Polim,0,sizeof(POLIMETRICA)*MAX_POLIMETRICA);
   Grafo.TotPolim = 0  ;
   Grafo.FilePolim=NULL;
   // Identifico le stazioni di instradamento con una prima scansione
   // impostando i relativi rami
   ORD_FORALL(Stazioni,s0){
      STAZIONI::R_STRU & Stazione = Stazioni[s0];
      if(Stazione.IdStazione &&
         Stazione.CodiceInstradamentoCvb > 0 &&
         Stazione.TipoStazione != ST_NODO &&
         Stazione.TipoStazione != ST_TERMINALE){
         RAMO Key;
         Key.IdStaz1 = Stazione.IdStazione1;
         Key.IdStaz2 = Stazione.IdStazione2;
         void *P = bsearch(&Key,Grafo.Rami,Grafo.TotRami,sizeof(RAMO),Cmp_RAMO);
         if(P != NULL){
            RAMO & Ram = * (RAMO*)P;
            if(Ram.IdRamo != 0){
               // Instradamento e' impostato per stazioni di instradamento interne
               Ram.Instradamento =1;
               TRACESTRING("====>>  Impostato Instradamento per STAZIONE INTERNA DI RAMO ");
               TRACEVLONG(Ram.IdRamo);
               TRACEVLONG(Ram.IdStaz1);
               TRACEVLONG(Ram.IdStaz2);
            }
         }
      }
//<<< ORD_FORALL(Stazioni,s0){
   }
   ORD_FORALL(Stazioni,s){
      STAZIONI::R_STRU & Stazione = Stazioni[s];
      STAZ_POLI_CIP    & Stazn    = Staz[s]    ;
      if(Stazione.IdStazione && (
            Stazione.Vendibile()
            || Stazione.TipoStazione == ST_NODO
            || Stazione.TipoStazione == ST_TERMINALE
            || Stazn.Diramazione
            || Stazione.CodiceInstradamentoCvb > 0
         ) ){
         // Altrimenti lascio il record vuoto
         Grafo.TotStazioniGrafo = s+1;
         STAZ_FS & Stz = Grafo.StazioniGrafo[s];
         Stz.NumeroPrimiVicini   =0;
         Stz.NumeroConcesse      =0;
         Stz.PDati               =0;
         Stz.Distanza1           =0;
         Stz.Nodo1               =0;
         Stz.Progressivo         =0;
         Stz.Terminale           =0;
         Stz.IdRamo              =0; // Viene impostato in un secondo scan
         Stz.Id                  = Chk(Stazione.IdStazione,13);
         Stz.NumeroPolimetriche  = Chk(Stazn.NumeroPolimetriche(),4);
         Stz.CCR                 = Chk(Stazione.CodiceCCR,17);
         Stz.IsCnd               = Chk(Stazn.IsCnd,7);
         Stz.Km1                 =0;
         Stz.ProgRamo            =0;
         Stz.Nodale              =0;
         memmove(Stz.IdentIstr,Stazione.Nome7,7);
         if (Stazione.TipoStazione == ST_NODO) {
            Stz.Nodale = 1;
            Stz.InstrGrafo = 1;
         } else {
            Stz.InstrGrafo = 0;
            if(StazioniEccezionaliDiInstradamento.Contiene(Stz.Id)){
               Out.printf("Impostato flag di instradamento necessario per stazione %i %s \r\n",Stz.Id,Stazione.NomeStazione);
               Stz.InstrGrafo = 1;
            }
            RAMO Key;
            if (Stazione.TipoStazione == ST_TERMINALE ) {
               Stz.Terminale = 1;
               Key.IdStaz1 = Stazione.IdStazione1;
               Key.IdStaz2 = Stazione.IdStazione ;
            } else {
               Key.IdStaz1 = Stazione.IdStazione1;
               Key.IdStaz2 = Stazione.IdStazione2;
            }
            void *P = bsearch(&Key,Grafo.Rami,Grafo.TotRami,sizeof(RAMO),Cmp_RAMO);
            if(P == NULL){
               Stz.IdRamo= 0;
               STRINGA Msg;
               Msg = "Stazione "+STRINGA(Stazione.IdStazione)+" "+STRINGA(Stazione.NomeStazione)+
               (Stazione.Aperta ? " Aperta":" Chiusa")+ (Stazione.Fittizia ? " Fittizia":" Reale")+
               (Stazione.Fs() ? " FS" : " NON FS")+
               " NON IDENTIFICATO RAMO "+
               STRINGA(Key.IdStaz1)+ "->"+STRINGA(Key.IdStaz2);
               Bprintf("%s",(CPSZ)Msg);
            } else {
               RAMO & Ram = * (RAMO*)P;
               Stz.IdRamo=  (RAMO*)P - Grafo.Rami;
               if(Stz.IdRamo==0){
                  #ifdef RELAX_ON_CUMUL
                  if(Stazione.Fs()){
                     #endif
                     STRINGA Msg;
                     Msg = "Stazione "+STRINGA(Stazione.IdStazione)+" "+STRINGA(Stazione.NomeStazione)+
                     (Stazione.Aperta ? " Aperta":" Chiusa")+ (Stazione.Fittizia ? " Fittizia":" Reale")+
                     (Stazione.Fs() ? " FS" : " NON FS")+
                     " NON appartiene ad alcun ramo";
                     if (Stazione.Fittizia) {
                        Bprintf3("%s",(CPSZ)Msg); // Solo sul trace
                     } else {
                        Bprintf("%s",(CPSZ)Msg);
                     } /* endif */
                     #ifdef RELAX_ON_CUMUL
                  }
                  #endif
               } else {
                  if (Stazione.TipoStazione != ST_TERMINALE ){
                     RamiStaz[Stz.IdRamo] += Stz.Id;
                  }
                  Stz.Km1 = Stazione.Distanza1;
                  if(Stazione.Vendibile() && Stazione.Distanza1 + Stazione.Distanza2 != Ram.KmRamo){
                     STRINGA Msg;
                     Msg = "Stazione "+STRINGA(Stazione.IdStazione)+" "+STRINGA(Stazione.NomeStazione)+
                     " Appartiene a ramo " +
                     STRINGA(Key.IdStaz1)+ "->"+STRINGA(Key.IdStaz2)+
                     " ha somma Km1 e Km2 = "+STRINGA(Stazione.Distanza1 + Stazione.Distanza2)+
                     " invece di "+STRINGA(((RAMO*)P)->KmRamo);
                     Bprintf("%s",(CPSZ)Msg);
                  };
                  //if(!((RAMO*)P)->Aperto){
                  //   STRINGA Msg;
                  //   Msg = "Stazione "+STRINGA(Stazione.IdStazione)+" "+STRINGA(Stazione.NomeStazione)+
                  //   (Stazione.Aperta ? " Aperta":" Chiusa")+ (Stazione.Fittizia ? " Fittizia":" Reale")+
                  //   (Stazione.Fs() ? " FS" : " NON FS")+
                  //   " Appartiene a ramo chiuso " +
                  //   STRINGA(Key.IdStaz1)+ "->"+STRINGA(Key.IdStaz2);
                  //   TRACESTRING(Msg);
                  //   Out.printf("%s\r\n",(CPSZ)Msg);
                  //}
//<<<          if(Stz.IdRamo==0){
               }
//<<<       if(P == NULL){
            } /* endif */
//<<<    if (Stazione.TipoStazione == ST_NODO) {
         } /* endif */
         Stz.StazioneFS          = Stazione.Fs()             ;
         Stz.DiDiramazione       = Stazn.Diramazione         ;
         Stz.DiAllacciamento     = Stazn.Allacciamento       ;
         Stz.DiInstradamento     = Stazione.CodiceInstradamentoCvb != 0;
         Stz.DiTransito          = 0;
         Stz.Vendibile           = Stazione.Vendibile()        ;
         Stz.Tariffe.Set(Stazione.TariffaRegione ,Stazione.Prima_Estensione ,Stazione.Seconda_Estensione ,Stazione.Terza_Estensione);
         if (Stazione.Sarda()) {
            Stz.Abilitazione   = STAZ_FS::SARDEGNA;
         } else if (Stazione.Siciliana()){
            Stz.Abilitazione   = STAZ_FS::SICILIA;
         } else {
            Stz.Abilitazione   = STAZ_FS::CONTINENTE;
         } /* endif */

         // Adesso carico i dati estesi della stazione
         // I dati comprendono:
         //   - NumeroPrimiVicini  PRIMO_V
         //   - NumeroPolimetriche P_POLI
         //   - NumeroConcesse     D_CONCESSE
         //   - Eventuali dati di instradamento
         // NB: i primi vicini sono caricati solo per le stazioni nodali
         Stz.PDati = Grafo.ExtStazioniGrafo.Dim();
         while (PSimmetrici < 2*NumRami &&
            RamiSimmetrici[PSimmetrici].IdStaz1 == Stz.Id ) {
            if (Stazione.TipoStazione == ST_NODO && IsNodo(RamiSimmetrici[PSimmetrici].IdStaz2) ) {
               Stz.NumeroPrimiVicini = Chk(Stz.NumeroPrimiVicini + 1,4);
               STAZ_FS::PRIMO_V Pv;
               RAMO & Ram = Grafo.Rami[RamiSimmetrici[PSimmetrici].IdRamo];
               Pv.Id             = Chk(RamiSimmetrici[PSimmetrici].IdStaz2 ,13);
               Pv.Distanza       = Chk(Ram.KmRamo ,9)   ;
               Pv.Mare           = Ram.Mare             ;
               Pv.Concesso       = Ram.Concesso         ;
               Pv.Instradamento  = Ram.Instradamento    ;
               Pv.IdRamo         = Chk(Ram.IdRamo,10)   ;
               Grafo.ExtStazioniGrafo.Store(&Pv,sizeof(Pv));
               E_Pv ++;
            };

            PSimmetrici ++;
         } /* endwhile */
         for (int j = 0;j < Stz.NumeroPolimetriche ;j++ ) {
            STAZ_FS::P_POLI Pp;
            Pp.IdPoli      =  Chk(Stazn.Polimetriche[j].Id ,10) ;
            Pp.TipoPoli    =  Chk(Stazn.Polimetriche[j].Tipo,4) ;
            WORD Indice = Polimetriche[Stazn.Polimetriche[j].Id].Stazioni.Posizione(Stz.Id);
            if (Indice == WORD_NON_VALIDA) {
               Bprintf(" ========== ARCHIVI INCONSISTENTI ========");
               BEEP;
               DosBeep(1200,1200);
               exit(999);
            } /* endif */
            Pp.IdxStazione = Chk(Indice,9);
            Grafo.ExtStazioniGrafo.Store(&Pp,sizeof(Pp));
            E_Pp ++;
         } /* endfor */
         if(Stazione.StazioneCumulativo){
            if(Stazione.SocietaConcessa1){
               Stz.NumeroConcesse ++;
               STAZ_FS::D_CONCESSE Dc;
               ZeroFill(Dc);
               Dc.CCRCumulativo   = Chk(Stazione.CCRCumulativo1,17)  ;
               Dc.SocietaConcessa = Chk(Stazione.SocietaConcessa1,7) ;
               Grafo.ExtStazioniGrafo.Store(&Dc,sizeof(Dc));
               E_Dc ++;
            }
            if(Stazione.SocietaConcessa2){
               Stz.NumeroConcesse ++;
               STAZ_FS::D_CONCESSE Dc;
               ZeroFill(Dc);
               Dc.CCRCumulativo   = Chk(Stazione.CCRCumulativo2,17)  ;
               Dc.SocietaConcessa = Chk(Stazione.SocietaConcessa2,7) ;
               Grafo.ExtStazioniGrafo.Store(&Dc,sizeof(Dc));
               E_Dc ++;
            }
         }
         if(Stz.DiInstradamento){
            STAZ_FS::D_INSTR Di;
            Di.CodiceInstr = Stazione.CodiceInstradamentoCvb;
            Di.Rank        = 1000; // Vuol dire che non e' definito
            Grafo.ExtStazioniGrafo.Store(&Di,sizeof(Di));
            E_Di ++;
         }
         if(Stz.DiDiramazione){
            STAZ_FS::D_DIRAM Dd;
            Dd.IsCndInibit  = Chk(Stazn.IsCndInibit,7);
            Grafo.ExtStazioniGrafo.Store(&Dd,sizeof(Dd));
            E_Dd ++;
         }
//<<< if(Stazione.IdStazione && (
      };

//<<< ORD_FORALL(Stazioni,s){
   }

   // Adesso carico i dati estesi dei rami
   for (int r = 1; r < Grafo.TotRami ; r++) {
      Grafo.Rami[r].PDati = Grafo.ExtRami.Dim();
      Grafo.Rami[r].NumStazioni = RamiStaz[r].Dim();
      if(RamiStaz[r].Dim() >= 64){
         Bprintf("Errore : Troppe stazioni su di un ramo");
         exit(999);
      };
      if(RamiStaz[r].Dim() > 0){
         // Eseguo un SORT delle stazioni del ramo basato sul progressivi ramo
         qsort(&RamiStaz[r][0], RamiStaz[r].Dim(), sizeof(ID),Cmp_Staz_RAMO);
         ORD_FORALL(RamiStaz[r],i){
            RAMO::STAZIONE_R Sr;
            STAZ_FS & Stz = Grafo.StazioniGrafo[RamiStaz[r][i]];
            Sr.Id        =  Chk(Stz.Id,13)       ;
            Sr.Instrad   =  Stz.DiInstradamento  ;
            Sr.Diram     =  Stz.DiDiramazione    ;
            Sr.Vendibile =  Stz.Vendibile        ;
            Stz.ProgRamo =  Chk(i+1,6); // Comincia da 1 e prosegue
            Grafo.ExtRami.Store(&Sr,sizeof(Sr));
            E_Sr ++;
         }
      };
//<<< for (int r = 1; r < Grafo.TotRami ; r++) {
   } /* endfor */

   Bprintf("Fine generazione grafo : passati %i secondi",(Time()-Tm1)/100);
   Bprintf("Dati estesi Grafo: Rami %i Bytes",Grafo.ExtStazioniGrafo.Dim());
   //TRACEPOINTER("GRAFO::Grafo & Grafo - GRAFO::Grafo", GRAFO::Grafo);
   //TRACEPOINTER("GRAFO::Grafo & Grafo - Grafo", &Grafo);
   TRACEVLONG(E_Pv);
   TRACEVLONG(E_Pp);
   TRACEVLONG(E_Dc);
   TRACEVLONG(E_Di);
   TRACEVLONG(E_Dd);
   TRACEVLONG(E_Sr);

   //TRACEPOINTER("GG 1303",GRAFO::Grafo);

   // ---------------------------------------------------------
   // Carico il rank delle stazioni di instradamento
   // ---------------------------------------------------------
   // Questo e' il file con i rank delle stazioni
   FILE_RO  Ranks(PATH_INTERMED "RANK.TXT");
   while(Ranks.gets(Linea,128)){
      RANK_FILE & Rec = *(RANK_FILE*)(CPSZ)Linea;
      TRACEVSTRING2(Linea);
      //TRACEPOINTER("GG 1313",GRAFO::Grafo);
      if (Grafo[It(Rec.Id)].DiInstradamento) {
         //TRACEPOINTER("GG 1315",GRAFO::Grafo);
         Grafo[It(Rec.Id)].Instradamento().Rank = It(Rec.Rank);
         TRACESTRING("Assegnato Rank "+STRINGA(Grafo[It(Rec.Id)].Instradamento().Rank)+
            " a stazione "+STRINGA(It(Rec.Id))+ " "+
            Stazioni[It(Rec.Id)].NomeStazione);
      } else {
         Bprintf("Errore assegnando rank a stazione %i",It(Rec.Id));
         BEEP;
         ReturnCode = 123;
      } /* endif */
   };

   // ---------------------------------------------------------
   // Identifico le stazioni di transito
   // ---------------------------------------------------------
   {
      Bprintf2("Identifico le stazioni di transito");
      F_RETI_CU Reti( PATH_INTERMED "RETISTRU.TMP");
      ORD_FORALL(Reti,i2){
         Reti.FixRec(i2);
         RETI_CUMULATIVE & Rete =  Reti.RecordCorrente();
         // Bprintf2("OUTPUT: Rete : %i %s",Rete.CodiceRete,Rete.Nome);
         for (int i = 0;i < 8 ;i++ ) {
            if( Rete.StazioneDitransito[i] ){
               Grafo[Rete.StazioneDitransito[i]].DiTransito = 1;
            }
         } /* endfor */
      }
   }

   // ---------------------------------------------------------
   // Dump e completamento dati
   // ---------------------------------------------------------
   #ifdef ESPANSO
   FILE_RW Delta(PATH_POLIM "POLIMETR.DTA");
   #else
   FILE_RW Delta(PATH_POLIM "POLIMETR.DTB");
   #endif
   Delta.SetSize(0);
   Grafo.Polim = new POLIMETRICA[Stazioni.Dim()];
   memset(Grafo.Polim,0,sizeof(POLIMETRICA)*NumPolimetriche+1);
   Grafo.TotPolim = 1  ; // Ha gia' l' elemento 0
   char * BufrTriang = new char[64000];
   for (j1 = 1; j1 <= NumPolimetriche ; j1 ++) {
      BOOL WarnDist = FALSE;
      int NumWarn[400];
      ZeroFill(NumWarn);
      int WarnPoli=0;
      POLIMETRICA_CIP & Polimetrica = Polimetriche[j1];
      POLIMETRICA     & Poli = Grafo.Polim[j1];
      Bprintf3("Elaborazione Polimetrica %s",Polimetrica.Descrizione);
      STAZ_POLI_CIP & First = Staz[Polimetrica.Stazioni[0]];
      STAZ_POLI_CIP::LEGAMI & Leg1 = First.DaIdPolim(Polimetrica.IdPolimetrica);
      if (&Leg1 == NULL) {
         ERRSTRING("Leg1 NON identificato!");
         Bprintf("Polimetrica %s ha la prima stazione non identificabile: scarto tutta la polimetrica",Polimetrica.Descrizione);
         BEEP;
         ReturnCode = 123;
         continue;
      } /* endif */

      int Kp=0;
      ORD_FORALL(Leg1.Distanze,u)Top(Kp,Leg1.Distanze[u]);

      Poli.IdPoli       = Chk(Polimetrica.IdPolimetrica,10)   ;
      Poli.KmPoli       = Chk(Kp,11);
      Poli.TipoPoli     = Chk(Polimetrica.TipoPolimetrica,4) ;
      Poli.NumStazioni  = Chk(Polimetrica.Stazioni.Dim(),9)  ;
      Poli.NumStazDiram = 0;
      Poli.NumBits      = 0;
      Poli.SocietaConcessa = 0;
      Poli.Zone = 1 << Polimetrica.Zona;
      switch (Polimetrica.TipoPolimetrica) {
      case POLIMETRICA_CIP::ALLACCIAMENTO:
         Poli.Zone = 0xffff; // Tutti i bits set: collega tutte le zone
         break;
      case POLIMETRICA_CIP::CUMULATIVA:
      case POLIMETRICA_CIP::MARE_CUM:
         Poli.SocietaConcessa = StringToInt(Polimetrica.Nome + 4,2);
         break;
      default:
         break;
      } /* endswitch */
      // Polimetrica.Zone.ToFix(&Poli.Zone,sizeof(Poli.Zone)*8);
      Poli.Base         = 0;
      Poli.PDati        = Chk(Grafo.ExtPolim.Dim(),20) ;
      BUFR BufrDiram;
      memset(BufrTriang,0,64000);
      int Size=0; // Size del buffer triangolare
      ORD_FORALL(Polimetrica.Stazioni,j2){
         if(Staz[Polimetrica.Stazioni[j2]].Diramazione){
            Poli.NumStazDiram++;
            POLIMETRICA::PO_DIRA Pd;
            Pd.Id = Chk(Polimetrica.Stazioni[j2],13);
            Pd.Idx= Chk(j2,9);
            BufrDiram.Store(&Pd,sizeof(Pd));
         }
      }
      if(Polimetrica.TipoPolimetrica != POLIMETRICA_CIP::LOCALE &&
         Polimetrica.TipoPolimetrica != POLIMETRICA_CIP::SOLO_LOCALE &&
         Polimetrica.TipoPolimetrica != POLIMETRICA_CIP::MARE_FS){
         // Polimetrica Matriciale
         int MaxKm = 0;
         ORD_FORALL(Polimetrica.Stazioni,j2){
            POLIMETRICA::PO_STAZ Ps;
            Ps.Id = Chk(Polimetrica.Stazioni[j2],13);
            Ps.Km = Chk(Leg1.Distanze[j2],11);
            Ps.Livello= 0;
            Ps.Radice = 0;
            Ps.Diram   = Chk(Staz[Ps.Id].Diramazione,1);
            Grafo.ExtPolim.Store(&Ps,sizeof(Ps));
            STAZ_POLI_CIP & Stazio = Staz[Ps.Id];
            STAZ_POLI_CIP::LEGAMI & Legami = Stazio.DaIdPolim(Polimetrica.IdPolimetrica);
            for (int j3 = 1; j3 < j2 ; j3 ++ )MaxKm = max(MaxKm,Legami.Distanze[j3]);
         }
         for (int NumBits = 1; (1 << NumBits) <= MaxKm ; NumBits ++ );
         Delta.printf("Polim matriciale ID %i MaxKm %i NumBits %i %s %s\r\n",j1,MaxKm,NumBits,Polimetrica.Nome,Polimetrica.Descrizione);
         Bprintf2("Polim matriciale ID %i MaxKm %i NumBits %i %s %s",j1,MaxKm,NumBits,Polimetrica.Nome,Polimetrica.Descrizione);
         Poli.NumBits = NumBits;
         ORD_FORALL(Polimetrica.Stazioni,jj2){
            STAZ_POLI_CIP::LEGAMI & Legami = Staz[Polimetrica.Stazioni[jj2]].DaIdPolim(Polimetrica.IdPolimetrica);
            for (int j3 = 1; j3 < jj2 ; j3 ++ ) {
               if(!SetTrgBits(BufrTriang,Size,jj2-2,j3-1,NumBits,Legami.Distanze[j3])){
                  TRACESTRING("ID Polimetrica Matriciale:"+STRINGA(Polimetrica.IdPolimetrica)+
                     " Stazione ID: "+STRINGA(Polimetrica.Stazioni[jj2])+
                     " Stazione2 ID: "+STRINGA(Polimetrica.Stazioni[j3])+
                     " Distanza : "+STRINGA(Legami.Distanze[j3]));
               }
            }
         }
         Grafo.ExtPolim.Store(BufrDiram);
         Grafo.ExtPolim.Store(BufrTriang,Size);
//<<< if(Polimetrica.TipoPolimetrica != POLIMETRICA_CIP::LOCALE &&
      } else {
         int MinKm =  127; // Max valore che entra in un signed char
         int MaxKm = -128; // Max valore che entra in un signed char
         Delta.printf("ID %i %s %s\r\n",j1,Polimetrica.Nome,Polimetrica.Descrizione);
         Delta.printf("Radici: %s\r\n",(CPSZ)Polimetrica.Radici.ToStringa());
         Bprintf2("ID %i %s %s",j1,Polimetrica.Nome,Polimetrica.Descrizione);
         Bprintf2("Radici: %s",(CPSZ)Polimetrica.Radici.ToStringa());
         ORD_FORALL(Polimetrica.Stazioni,j2){
            ID IdStaz = Polimetrica.Stazioni[j2];
            if(IdStaz == 0 ){
               Delta.printf(" **********\r\r\n");
               Bprintf("Anomalia: Doveva essere stata eliminata");
               BEEP;
               exit(999);
            };
            Delta.printf(" %5i %5i ",Stazioni[IdStaz].CodiceCCR,IdStaz);
            int IdxRad = 0;
            STAZ_POLI_CIP & Stazio = Staz[IdStaz];
            STAZ_POLI_CIP::LEGAMI & Legami = Stazio.DaIdPolim(Polimetrica.IdPolimetrica);
            if(j2 == 0){
               Delta.printf(" |0| ");
            } else {
               IdxRad = Polimetrica.IdxRadice(Legami.Livello);
               Delta.printf(" |%1i| ",Legami.Livello);
               for (int j3 = 1; j3 < j2 ; j3 ++ ) {
                  int Predizione;
                  STAZ_POLI_CIP::LEGAMI & Legami3 = Staz[Polimetrica.Stazioni[j3]].DaIdPolim(Polimetrica.IdPolimetrica);
                  int IdxRad3 = Polimetrica.IdxRadice(Legami3.Livello);
                  if(Legami.Livello == Legami3.Livello){
                     // Stesso ramo. Predizione = Differenza tra le distanze dall' origine
                     Predizione = Leg1.Distanze[j2] - Leg1.Distanze[j3];
                  } else if(Legami3.Livello > Legami.Livello){
                     // Predizione = Differenza tra le distanze dall' origine di stazione3 e di
                     // radice3, piu' distanza effettiva di radice3 da stazione2
                     Predizione = Leg1.Distanze[j3] - Leg1.Distanze[IdxRad3] + Legami.Distanze[IdxRad3];
                  } else {
                     // Predizione = Differenza tra le distanze dall' origine di stazione2 e di
                     // radice2, piu' distanza effettiva di radice2 da stazione3
                     Predizione = Leg1.Distanze[j2] - Leg1.Distanze[IdxRad] + Legami3.Distanze[IdxRad];
                  }
                  int DeltaKm = Legami.Distanze[j3] - Predizione;
                  MinKm = min(MinKm, DeltaKm);
                  MaxKm = max(MaxKm, DeltaKm);
                  #ifdef ESPANSO
                  Delta.printf("%4i ",DeltaKm);
                  #else
                  if (abs(DeltaKm) <= LIMITE_ERRORE) {
                     Delta.printf(".");
                  } else {
                     Delta.printf("*");
                     if(Legami.Livello == Legami3.Livello){
                        Bprintf3("Relazione %i -> %i KM Reali %i Previsione %i Delta %i Dist1 = %i Dist2 = %i", IdStaz,Polimetrica.Stazioni[j3],Legami.Distanze[j3],Predizione,Leg1.Distanze[j2],Leg1.Distanze[j3]);
                     } else {
                        Bprintf3("Relazione %i -> %i KM Reali %i Previsione %i Delta %i Dist1 = %i DistRdx = %i RealeDaRadice %i", IdStaz,Polimetrica.Stazioni[j3],Legami.Distanze[j3],Predizione,Leg1.Distanze[j2], Legami3.Distanze[IdxRad]);
                     } /* endif */
                  } /* endif */
                  #endif
                  #ifndef RELAX_DIST
                  if (WarnPoli < 30 && abs(DeltaKm)>LIMITE_ERRORE) {
                     WarnPoli ++;
                     if (!WarnDist) {
                        Bprintf2("Distanze sospette su polimetrica %s %s",Polimetrica.Nome,Polimetrica.Descrizione);
                        WarnDist = TRUE;
                     } /* endif */
                     if((NumWarn[j2]) <  2 && (NumWarn[j3]) <  2){
                        Bprintf2("Sospetta distanza di Km %i (plausibili: %i) tra stazioni %s",
                           Legami.Distanze[j3],Predizione,
                           Stazioni[Polimetrica.Stazioni[j2]].NomeStazione);
                        Bprintf2(" e %s", Stazioni[Polimetrica.Stazioni[j3]].NomeStazione);
                        NumWarn[j2]++; NumWarn[j3]++;
                        if(NumWarn[j2] == 2){
                           Bprintf2("Inibiti ulteriori warning su %s",
                              Stazioni[Polimetrica.Stazioni[j2]].NomeStazione);
                        }
                        if(NumWarn[j3] == 2){
                           Bprintf2("Inibiti ulteriori warning su %s",
                              Stazioni[Polimetrica.Stazioni[j3]].NomeStazione);
                        }
                     };
                     if(WarnPoli == 30)Out.printf("%s\r\n","Inibito ogni ulteriore warning sulla polimetrica");
//<<<             if (WarnPoli < 30 && abs(DeltaKm)>LIMITE_ERRORE) {
                  }
                  #endif
//<<<          for (int j3 = 1; j3 < j2 ; j3 ++ ) {
               } /* endfor */
//<<<       if(j2 == 0){
            }

            POLIMETRICA::PO_STAZ Ps;
            Ps.Id = Chk(Polimetrica.Stazioni[j2],13);
            Ps.Km = Chk(Leg1.Distanze[j2],11);
            Ps.Livello= Chk(Legami.Livello,3);
            Ps.Radice = Chk(IdxRad,9);
            Ps.Diram   = Chk(Staz[Ps.Id].Diramazione,1);
            Grafo.ExtPolim.Store(&Ps,sizeof(Ps));
            Delta.printf("\r\n");
//<<<    ORD_FORALL(Polimetrica.Stazioni,j2){
         }

         // EMS005 VA
         if (MinKm < 0)
            ERRINT("ERRORE MinKm",MinKm);
         Poli.Base         = ChkS(MinKm,9);
         int Diff = MaxKm - MinKm;
         for (int NumBits = 1; (1 << NumBits) <= Diff  ; NumBits ++ );
         Poli.NumBits = NumBits;
         Delta.printf("MinKm %i MaxKm %i Diff %i NumBits %i\r\r\n",MinKm,MaxKm,Diff,NumBits);

         // Scarico dei dati della polimetrica sulle strutture del grafo
         ORD_FORALL(Polimetrica.Stazioni,jj2){
            ID IdStaz = Polimetrica.Stazioni[jj2];
            if(!Grafo.StazioniGrafo[IdStaz].Nodale  &&
               Grafo.StazioniGrafo[IdStaz].DiDiramazione &&
               jj2 != 0 && jj2 != Polimetrica.Stazioni.Dim() -1){
               Bprintf2("La stazione di diramazione %i non e' nodale, ne appare ad un estremo di polimetrica:",IdStaz);
               Bprintf2("  Stazione %s Polimetrica %s %s",Stazioni[IdStaz].NomeStazione,Polimetrica.Nome,Polimetrica.Descrizione);
            }
            int IdxRad = 0;
            if(jj2 > 0){
               STAZ_POLI_CIP & Stazio = Staz[IdStaz];
               STAZ_POLI_CIP::LEGAMI & Legami = Stazio.DaIdPolim(Polimetrica.IdPolimetrica);
               IdxRad = Polimetrica.IdxRadice(Legami.Livello);
               for (int j3 = 1; j3 < jj2 ; j3 ++ ) {
                  int Predizione;
                  STAZ_POLI_CIP::LEGAMI & Legami3 = Staz[Polimetrica.Stazioni[j3]].DaIdPolim(Polimetrica.IdPolimetrica);
                  int IdxRad3 = Polimetrica.IdxRadice(Legami3.Livello);
                  if(Legami.Livello == Legami3.Livello){
                     // Stesso ramo. Predizione = Differenza tra le distanze dall' origine
                     Predizione = Leg1.Distanze[jj2] - Leg1.Distanze[j3];
                  } else if(Legami3.Livello > Legami.Livello){
                     // Predizione = Differenza tra le distanze dall' origine di stazione3 e di
                     // radice3, piu' distanza effettiva di radice3 da stazione2
                     Predizione = Leg1.Distanze[j3] - Leg1.Distanze[IdxRad3] + Legami.Distanze[IdxRad3];
                  } else {
                     // Predizione = Differenza tra le distanze dall' origine di stazione2 e di
                     // radice2, piu' distanza effettiva di radice2 da stazione3
                     Predizione = Leg1.Distanze[jj2] - Leg1.Distanze[IdxRad] + Legami3.Distanze[IdxRad];
                  }
                  int DeltaKm = Legami.Distanze[j3]- Predizione - MinKm;
                  int IdxElem = ((jj2 -2)*(jj2 -1) / 2) + j3 - 1;
                  if(!SetBits(BufrTriang,Size,IdxElem,NumBits,DeltaKm)){
                     TRACESTRING("ID Polimetrica locale :"+STRINGA(Polimetrica.IdPolimetrica)+
                        " Stazione ID: "+STRINGA(Polimetrica.Stazioni[jj2])+
                        " Stazione2 ID: "+STRINGA(Polimetrica.Stazioni[j3])+
                        " Distanza : "+STRINGA(Legami.Distanze[j3])+
                        " Predizione: "+STRINGA(Predizione)
                     );
                  }
//<<<          for (int j3 = 1; j3 < jj2 ; j3 ++ ) {
               } /* endfor */
//<<<       if(jj2 > 0){
            }
//<<<    ORD_FORALL(Polimetrica.Stazioni,jj2){
         }
         Grafo.ExtPolim.Store(BufrDiram);
         Grafo.ExtPolim.Store(BufrTriang,Size);
//<<< if(Polimetrica.TipoPolimetrica != POLIMETRICA_CIP::LOCALE &&
      }
//<<< for (j1 = 1; j1 <= NumPolimetriche ; j1 ++) {
   } /* endfor */
   Out.printf("Fine dump dati e scrittura dati polimetriche: passati %i secondi\r\n",(Time()-Tm1)/100);

   // ---------------------------------------------------------
   // Scarico sui files
   // ---------------------------------------------------------
   {
      FILE_RW Grafo1DB(PATH_DATI "MM_GRAF1.DB");
      FILE_RW Grafo1EX(PATH_DATI "MM_GRAF1.EXT");
      FILE_RW Grafo2DB(PATH_DATI "MM_GRAF2.DB");
      FILE_RW Grafo2EX(PATH_DATI "MM_GRAF2.EXT");
      F_POLIM Grafo3DB(PATH_DATI "MM_GRAF3.DB");
      FILE_RW Grafo3EX(PATH_DATI "MM_GRAF3.EXT");
      FILE_RW Grafo4DB(PATH_DATI "MM_GRAF4.DB");

      Grafo1DB.SetSize(0);
      Grafo1EX.SetSize(0);
      Grafo2DB.SetSize(0);
      Grafo2EX.SetSize(0);
      Grafo3DB.Clear("Dati delle polimetriche FS");
      Grafo3EX.SetSize(0);
      Grafo4DB.SetSize(0);
      Grafo1DB.Scrivi(Grafo.StazioniGrafo,Grafo.TotStazioniGrafo * sizeof(STAZ_FS));
      Grafo1EX.Scrivi(Grafo.ExtStazioniGrafo);
      Grafo2DB.Scrivi(Grafo.Rami,Grafo.TotRami * sizeof(RAMO));
      Grafo2EX.Scrivi(Grafo.ExtRami);
      Grafo3EX.Scrivi(Grafo.ExtPolim);
      Grafo4DB.Scrivi(&IsCnd[0],IsCnd.Dim() * sizeof(ISTR_COND));

      for (j1 = 0; j1 <= NumPolimetriche ; j1 ++) {
         POLIMETRICA_CIP & Polimetrica = Polimetriche[j1];
         POLIMETRICA     & Poli = Grafo.Polim[j1];
         POLIMETRICA_SU_FILE Pol;
         ZeroFill(Pol);
         (POLIMETRICA &)Pol = Poli;
         strcpy(Pol.Nome,Polimetrica.Nome);
         strcpy(Pol.Descrizione,Polimetrica.Descrizione);
         Grafo3DB.AddRecordToEnd(BUFR(&Pol,sizeof(Pol)));
      }
   }
   Out.printf("Fine scarico dati su file: passati %i secondi\r\n",(Time()-Tm1)/100);


   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   Bprintf("Fine");
   delete [] Polimetriche;
   delete [] Staz        ;
   delete [] RamiStaz    ;
   delete    BufrTriang  ;
   TRACESTRING("Deallocate le aree di programma");

   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;

   return ReturnCode;
//<<< int main(int argc,char *argv[]){
}

