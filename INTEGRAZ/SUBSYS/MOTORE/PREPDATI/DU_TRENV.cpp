//----------------------------------------------------------------------------
// DU_TRENV: DUMP M0_TRENV.TMP
//----------------------------------------------------------------------------
// Se il trace e' almeno a livello 2 scrive sul trace la periodicita'
// di tutti i mezzi virtuali.
// Sono 12 MBytes
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3
#include "FT_PATHS.HPP"  // Path da utilizzare

//EMS
typedef unsigned long BOOL;

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ML_IN.HPP"
#include "oggetto.h"
#include "ml_base.hpp"

//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
// struct MEZZO_VIRTUALE {
struct MY_STRU {
	WORD  MezzoVirtuale               ; // KEY: Mezzo virtuale
	// Dati riassuntivi a livello di treno virtuale, ottenuti dalla combinazione dei dati
	// dei mezzi viaggianti:
	BIT   TipoMezzoDominante      : 6 ; // Tipo predominante
	BIT   GiornoSuccessivo        : 2 ; // Giorni tra 1ø partenza del MV ed arrivo finale
	BIT   PeriodicitaDisuniformi  : 1 ; // Indica che le periodicita' sono esplose a livello di fermata
	BIT   DaCarrozzeDirette       : 1 ; // Indica che il MV e' stato formato a partire dalle carrozze dirette
	BIT   DaAltriServizi          : 1 ; // Se TRUE vuol dire che esistono servizi diretti che implicano la prosecuzione
	BIT   DaOrigineDestino        : 1 ; // Se TRUE vuol dire che uno dei treni continua nell' altro
	BIT   FittizioLinearizzazione : 1 ; // Indica che il MV e' fittizio: serve solo per linearizzare le fermate di un' altro MV
	BIT   CambiDeboli             : 3 ; // Numero di cambi deboli (vedi FT_STEPA.CPP).
	BIT   InLoop                  : 1 ; // Indica che vi sono treni presenti piu' volte
	BIT   NumeroFermateTransiti   : 10; // Comprende anche i transiti, ma non le fermate scartate
	// NB: Dopo la linearizzazione conta UNA SOLA VOLTA le fermate duplicate
	BIT   NumeroFermateValide     : 10; // NON comprende anche i transiti
	// NB: Dopo la linearizzazione conta UNA SOLA VOLTA le fermate duplicate
	T_PERIODICITA PeriodicitaMV       ; // = OR di tutte le periodicita' componenti
	MM_INFO ServiziUniformi           ; // Servizi uniformi del mezzo virtuale

	// Mezzi viaggianti componenti
	BYTE  NumMezziComponenti          ;
	struct MEZZO_VIAGGIANTE {
		BIT NumeroMezzo            :17 ;
		BIT HaNome                 : 1 ; // Vero se il mezzo viaggiante ha un nome
		BIT CcrStazioneDiCambio    :24 ; // CCR stazione di cambio (0 per l' ultimo treno):
		// Puo' anche non corrispondere ad alcuna stazione !
		// (cambio su stazioni del merci)
		BIT ShiftPartenza          : 2 ; // Indica quanti giorni dopo la partenza del mezzo virtuale ( = del primo mezzo componente) si parte
		// NB:: Il treno potrebbe anche essere partito il giorno prima.
		BIT ShiftDaPTreno          : 2 ; // Indica quanti giorni dopo la partenza del TRENO si PARTE dalla stazione di cambio
		BIT TipoMezzo              : 6 ; // Vedi enum TIPO_MEZZO
		IDTRENO IdentTreno             ;
		char KeyTreno[5]               ;
	} Mv[MAX_MEZZI_REALI];
	// EMS Aggiunto il campo ProgressivoPeriodicita per
	// avere la stessa struttura di M2_TRENV.TMP che mi serve dumpare
	BYTE  ProgressivoPeriodicita      ; // KEY
//<<< struct MY_STRU
};
// EMS
//#define NOME "M0_TRENV"
#define NOME "M2_TRENV"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
#define NOME_OU2       PATH_OUT NOME ".TX2"
#define NOME_IN_BSE    PATH_OUT NOME ".TM"
#define NOME_TRACE     NOME ".TRC"
#define BUF_DIM        64000
//----------------------------------------------------------------------------
STRINGA NOME_IN;

class  MY_FILE : public FILE_FIX {
   public:
   MY_FILE():
   FILE_FIX(NOME_IN,sizeof(MY_STRU),BUF_DIM){};
   MY_STRU &  operator [](ULONG Indice){ Posiziona(Indice); return *(MY_STRU*) RecordC; };
};

STRINGA Add(char * a,char*b){
   char Buf[100];
   sprintf(Buf,a,b);
   return STRINGA(Buf);
};
#define MK1(_a,_b,_c,_d,_e) FMT += "%-" _b _c ;HDR1 += Add("%-" _b "s",_d) ; HDR2 += Add("%-" _b "s",_e);
#define MK2(_a,_b,_c,_d,_e) ,Record._a
#define MK3(_n,_a,_b,_c,_d,_e) ,Record.Mv[_n]._a
//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int  main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"

	char cMsg[1000]; //EMS

   TRACEREGISTER2(NULL,NOME,PATH_DUMMY NOME_TRACE);
   
   #define Tr(_a) TRACEVLONG(sizeof(_a))
	Tr(MEZZO_VIRTUALE);
   Tr(MY_STRU);
   
   int MinMezzo, MaxMezzo;
   BOOL AnalisiPeriodicita = TRUE;
   BOOL AnalisiPEstesa     = FALSE;
   BOOL SoloDisuniformi    = FALSE;
	BOOL NoteEServizi       = FALSE;
	sprintf(cMsg,"Utilizzo : %s [MinVirtuale [MaxVirtuale] ]\n"
		" /F0     : Dati come in uscita da FT_STEPB\n"
		" /F1     : Dati non Linearizzati \n"
		" /F2     : Dati Linearizzati\n"
		" /P      : Disabilita analisi periodicita'\n"
		" /T      : Analisi periodicita' ESTESA \n"
		" /D      : Solo virtuali con periodicit… disuniforme\n"
		" /S      : Mostra note e Servizi\n"
		" /Nxxxx  : Solo virtuali che hanno in composizione il treno xxxx\n"
     , argv[0]
   );

   int Dettaglio = 2;
   
   MinMezzo = 0;
   MaxMezzo = 9999999;
   
   ELENCO_S Componenti;
   
   for (int a = 1;a < argc ; a ++ ) {
      STRINGA Tmp(argv[a]);
      Tmp.UpCase();
      if(Tmp[0] == '/' ){
         if(Tmp[1] == 'F' ) Dettaglio = Tmp[2] - '0';
         if(Tmp[1] == 'P' ) AnalisiPeriodicita = FALSE;
         if(Tmp[1] == 'T' ) AnalisiPEstesa     = TRUE;
			if(Tmp[1] == 'D' ) SoloDisuniformi    = TRUE;
         if(Tmp[1] == 'S' ) NoteEServizi       = TRUE;
         if(Tmp[1] == 'N' ) Componenti += Tmp(2,999).Strip();
      } else if(MinMezzo == 0){
         MaxMezzo = MinMezzo = Tmp.ToInt();
      } else {
         MaxMezzo = Tmp.ToInt();
      }
   } /* endfor */
   AnalisiPEstesa &= AnalisiPeriodicita ;
   
   
   char * Crs = "01P";
   NOME_IN =  NOME_IN_BSE ;
   NOME_IN += Crs[Dettaglio];
	FILE * Out;
   
   if(MaxMezzo == 9999999 && ! SoloDisuniformi && Componenti.Dim() == 0){
      printf("Analizzo i records di %s scrivendo il risultato su %s\n",(CPSZ)NOME_IN ,NOME_OUT);
      Out = fopen(NOME_OUT,"wt");
   } else {
      printf("Analizzo i records di %s scrivendo il risultato su %s\n",(CPSZ)NOME_IN,NOME_OU2);
      if(MaxMezzo != 9999999)printf("Analisi ristretta ai mezzi virtuali da %i a %i\n",MinMezzo, MaxMezzo);
		if(SoloDisuniformi )printf("Analisi ristretta ai mezzi virtuali con periodicit… disumiforme\n");
      if(Componenti.Dim()){
         STRINGA Mv;
         ORD_FORALL(Componenti,i)Mv += Componenti[i] +',';
         Mv[Mv.Dim() -1] = ' ';
         printf("Analisi ristretta ai mezzi virtuali contenenti il/i treni %s\n",(CPSZ)Mv);
      };
      Out = fopen(NOME_OU2,"wt");
   };
   
   if(NoteEServizi) printf("Abilitata analisi servizi\n");
	if(AnalisiPEstesa)printf("Abilitata analisi periodicita' ESTESA\n");
   if(AnalisiPeriodicita && ! AnalisiPEstesa) printf("Abilitata analisi periodicita'\n");
   
   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   
   PTRENO::Restore(); // Parto dai dati caricati l' ultima volta
   
   MY_FILE & InFile = * new MY_FILE();
   
   STAZIONI Stazioni(PATH_DATI);
   STRINGA PeriodFermate(PATH_OUT "M1_FERMV.TM");
   F_PERIODICITA_FERMATA_VIRT PerioF( PeriodFermate + STRINGA(Crs[Dettaglio]));
   STRINGA DatiFermate(PATH_OUT "M0_FERMV.TM");
   F_FERMATE_VIRT TabFv( DatiFermate + STRINGA(Crs[Dettaglio]));

	TRACESTRING("Sono dopo l'apertura di TabFv");

   STRINGA HDR1,HDR2,FMT,FMT0,FMT1,FMT2;
   HDR1 = "RecN  ";
   HDR2 = "      ";
   FMT  = "%-6u";
   
   //----------------------------------------------------------------------------
   // Parte specifica: Macro definizione formato di stampa
   //----------------------------------------------------------------------------
   // MK1(  ,"00","u","","")
   MK1( MezzoVirtuale , "6"    ,"u","Mez.","Virt.")
   MK1( NomeMezzoVirtuale      ,"21","s","Nome","Mez. Virt.")
   MK1( TipoMezzoDominante     , "3","u","T","M")
   MK1( GiornoSuccessivo       , "3","u","G","S")
	MK1( PeriodicitaDisuniformi , "3","u","Pr","Du")
   MK1( DaCarrozzeDirette      , "3","u","Da","CD")
   MK1( DaAltriServizi         , "3","u","Da","AS")
   MK1( DaOrigineDestino       , "3","u","Da","OD")
   MK1( FittizioLinearizzazione, "3","u","Fi","Ln")
   MK1( CambiDeboli            , "3","u","Ca","De")
   MK1( InLoop                 , "4","u","In","Lp")
   MK1( NumeroFermateTransiti  , "5","u","Num","F/T")
   MK1( NumeroFermateValide    , "5","u","Num","Fval")
   MK1( PeriodicitaMV          , "4","s","Ha","Per.")

   FMT0 = FMT;
   FMT.Clear();
   MK1( NumeroMezzo       , "7","u","Numero","Mezzo")
   MK1( IdentMezzo        ,"11","s","Nome","Mezzo")
   MK1( Iden5             , "6","s","Iden","Mezzo")
   MK1( HaNome            , "3","u","Ha","Nm")
   MK1( ShiftPartenza     , "3","u","Sh","Pa")
   MK1( ShiftDaPTreno     , "3","u","Sh","Tr")
	MK1( CcrStazioneDiCambio, "9","u","Stazione","cambio 1")
   
   FMT1 = FMT;
   // Queste solo per l' header
   MK1( NumeroMezzo       , "7","u","Numero","Mezzo")
   MK1( IdentMezzo        ,"11","s","Nome","Mezzo")
   MK1( Iden5             , "6","s","Iden","Mezzo")
   MK1( HaNome            , "3","u","Ha","Nm")
   MK1( ShiftPartenza     , "3","u","Sh","Pa")
   MK1( ShiftDaPTreno     , "3","u","Sh","Tr")
   MK1( CcrStazioneDiCambio, "9","u","Stazione","cambio 2")
   MK1( NumeroMezzo       , "7","u","Numero","Mezzo")
   MK1( IdentMezzo        ,"11","s","Nome","Mezzo")
   MK1( Iden5             , "6","s","Iden","Mezzo")
   MK1( HaNome            , "3","u","Ha","Nm")
   MK1( ShiftPartenza     , "3","u","Sh","Pa")
   MK1( ShiftDaPTreno     , "3","u","Sh","Tr")
   MK1( CcrStazioneDiCambio, "9","u","Stazione","cambio 3")
   MK1( NumeroMezzo       , "7","u","Numero","Mezzo")
   MK1( IdentMezzo        , "11","s","Nome","Mezzo")
   MK1( Iden5             , "6","s","Iden","Mezzo")
   MK1( HaNome            , "3","u","Ha","Nm")
   MK1( ShiftPartenza     , "3","u","Sh","Pa")
   MK1( ShiftDaPTreno     , "3","u","Sh","Tr")
   MK1( CcrStazioneDiCambio, "9","u","Stazione","cambio 4")
   
   FMT.Clear();
   MK1( Periodicita       ,"3","s","Periodicita' e Servizi uniformi","");
   FMT2 = FMT;
   FMT  = FMT0 + FMT1 + FMT2;
	// fprintf(Out,"%s\n",(CPSZ)FMT); // Per debug
   
   //----------------------------------------------------------------------------
   
   // puts((CPSZ)HDR1);
   // puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);
   
   printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());
   // fprintf(Out,"Periodicita' Orario: %s\n",(CPSZ)T_PERIODICITA::InOrario.InChiaro());

   int k = 0;
   for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
      MY_STRU & Record = InFile[i];
      if( Record.MezzoVirtuale < MinMezzo  || Record.MezzoVirtuale > MaxMezzo ) continue;
      if( SoloDisuniformi && !Record.PeriodicitaDisuniformi) continue;
      
      if(Componenti.Dim()){
         for (int Mc = 0;Mc < Record.NumMezziComponenti ; Mc++) {
            STRINGA Mzc = Record.Mv[Mc].IdentTreno;
            if (Componenti.Contiene(Mzc.Strip()))break;
         } /* endfor */
         if (Mc >= Record.NumMezziComponenti) continue; // Non contiene il treno
      }
      
      if(MaxMezzo != 9999999) printf("Processing %i\r",Record.MezzoVirtuale);
      if (k % NUMRIGHE_DUMP == 0) {
         if(k != 0)fprintf(Out,"\n");
         fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
         // TRACESTRING(HDR1);
         // TRACESTRING(HDR2);
      } /* endif */
      k++;
      
      STRINGA Period;
      char * HaPeriodicita = "No";
      if(Record.PeriodicitaMV  != T_PERIODICITA::InOrario || Record.PeriodicitaDisuniformi ){
         HaPeriodicita = "Si";
         if (AnalisiPeriodicita){
            Period = Record.PeriodicitaMV.InChiaro();
         }
      }
      
      STRINGA Servizi = Record.ServiziUniformi.Decodifica(TRUE);
      
		STRINGA NomeVirtuale;
		PTRENO * pTreno;
		IDTRENO idtTemp;
		for (int j = 0;j < Record.NumMezziComponenti ;j++ ) {
			if(Record.Mv[j].HaNome){
				// EMS Cambio per far funzionare sotto Windows
				idtTemp = Record.Mv[j].IdentTreno;
				pTreno = NULL;
				pTreno = PTRENO::Get(idtTemp);
				if (pTreno != NULL)
					NomeVirtuale = St(pTreno->NomeMezzoViaggiante);
				else
					TRACESTRING("ERRORE: pTreno == NULL 313");
				break;
         }
      } /* endfor */
      
      char Buf[2048];
      int Offset = sprintf(Buf,(CPSZ)FMT0,i
         
         //----------------------------------------------------------------------------
         // Parte specifica: Macro definizione variabili da stampare
			//----------------------------------------------------------------------------
         MK2( MezzoVirtuale , "6","u","Mez.","Virt.")
        ,(CPSZ)NomeVirtuale
         MK2( TipoMezzoDominante, "3","u","T","M")
         MK2( GiornoSuccessivo  , "3","u","G","S")
         MK2( PeriodicitaDisuniformi , "3","u","Pr","Du")
         MK2( DaCarrozzeDirette      , "3","u","Da","CD")
         MK2( DaAltriServizi         , "3","u","Da","AS")
         MK2( DaOrigineDestino       , "3","u","Da","OD")
         MK2( FittizioLinearizzazione, "3","u","Fi","Ln")
         MK2( CambiDeboli            , "3","u","Ca","De")
         MK2( InLoop                 , "3","u","In","Lp")
         MK2( NumeroFermateTransiti,"5","u","Num","F/T")
         MK2( NumeroFermateValide  ,"5","u","Num","Fval")
        ,HaPeriodicita
      );
      for ( j = 0;j < 4 ;j++ ) {
         Offset += sprintf(Buf+Offset,(CPSZ)FMT1
            MK3(j, NumeroMezzo  , "7","u","Numero","Mezzo")
           ,(CPSZ)Record.Mv[j].IdentTreno
           ,St(Record.Mv[j].KeyTreno)
            MK3(j, HaNome             , "3","u","Ha","Nm")
            MK3(j, ShiftPartenza      , "3","u","Sh","Pa")
            MK3(j, ShiftDaPTreno      , "3","u","Sh","Tr")
            MK3(j, CcrStazioneDiCambio, "9","u","Stazione","di cambio")
         );
      } /* endfor */
      
      Offset += sprintf(Buf+Offset,(CPSZ)FMT2,(CPSZ)(Servizi + " " +Period));
      
      //----------------------------------------------------------------------------
      fprintf(Out,"%s\n",Buf);
      // TRACESTRING(Buf);
      // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
      
      // Sul trace mando la periodicita' del treno
      if( Record.PeriodicitaDisuniformi ){
         fprintf(Out,"===============  PERIODICITA' DISUNIFORME  =========================\n");
      } else {
         fprintf(Out,"===============  PERIODICITA' UNIFORME     =========================\n");
      } /* endif */
      STRINGA Msg;
      if(AnalisiPEstesa || NoteEServizi ){
         Msg +="Mezzo virtuale "+STRINGA(Record.MezzoVirtuale)+ " ";
         Msg += NomeVirtuale + " Mezzi Viaggianti:";
         for (int mc = 0;mc < Record.NumMezziComponenti ; mc++ ) {
            Msg += " ";
            Msg += St(Record.Mv[mc].IdentTreno);
         } /* endfor */
		}
      
      if (NoteEServizi) {
         FILE_SERVNU  ServiziTreno;
         FILE_NOTA_CO NoteTreno   ;
         STRINGA NomeAux =PATH_OUT "INFO_AUX.TM" + STRINGA(Crs[Dettaglio]);
         F_INFO_AUX  Aux(NomeAux);
         if (Aux.Rc() == 0 && Aux.Seek(Record.MezzoVirtuale)) {
            fprintf(Out,"ANALISI DATI AUSILIARI MEZZO VIRTUALE %s\n",(CPSZ)Msg);
            while (&Aux.RecordCorrente() && Aux.RecordCorrente().MezzoVirtuale == Record.MezzoVirtuale) {
               fprintf(Out, "   %s\n",(CPSZ)Aux.RecordCorrente().ToStringa());
               Aux.Next();
            } /* endwhile */
         } /* endif */
         fprintf(Out,"ANALISI SERVIZI %s\n",(CPSZ)Msg);
         for (int mc = 0;mc < Record.NumMezziComponenti ; mc++ ) {
            MY_STRU::MEZZO_VIAGGIANTE & Mc = Record.Mv[mc];
            PTRENO & Treno = * PTRENO::Get(Mc.IdentTreno);
            STRINGA ServiziU = Treno.ServiziUniformiTreno.Decodifica(TRUE);
            MM_INFO Tmp = Treno.ServiziTreno;
            Tmp.NotIn(Treno.ServiziUniformiTreno, TRUE);
            STRINGA ServiziD = Tmp.Decodifica(FALSE);
            if(!Tmp.EmptyServizi())ServiziD = "Non Uniformi "+ServiziD;
            fprintf(Out,"Mezzo componente %s servizi Uniformi %s %s \n",St(Mc.IdentTreno) ,(CPSZ)ServiziU ,(CPSZ)ServiziD );
            if(ServiziTreno.Seek(Mc.IdentTreno)){
               while (&ServiziTreno.RecordCorrente() && ServiziTreno.RecordCorrente().IdTreno == Mc.IdentTreno) {
                  fprintf(Out, "   %s\n",(CPSZ)ServiziTreno.RecordCorrente().Servizi.ToStringa());
                  ServiziTreno.Next();
               } /* endwhile */
            };
         } /* endfor */
         fprintf(Out,"ANALISI NOTE %s\n",(CPSZ)Msg);
         for (mc = 0;mc < Record.NumMezziComponenti ; mc++ ) {
            MY_STRU::MEZZO_VIAGGIANTE & Mc = Record.Mv[mc];
            PTRENO & Treno = * PTRENO::Get(Mc.IdentTreno);
            if(NoteTreno.Seek(Mc.IdentTreno)){
               while (&NoteTreno.RecordCorrente() && NoteTreno.RecordCorrente().IdTreno == Mc.IdentTreno) {
                  fprintf(Out, "   %s\n",(CPSZ)NoteTreno.RecordCorrente().Nota.ToStringa());
                  NoteTreno.Next();
               } /* endwhile */
            };
         } /* endfor */
         
//<<< if  NoteEServizi   
      } /* endif */
      
      if (AnalisiPEstesa) {
         STRINGA PathDa(PATH_IN),ExtFl(".T");
         if (getenv("USADATIESTRATTI")){ PathDa=PATH_OUT;ExtFl=".XTR";};
			FILE_TABORARI TabOr(PathDa + "TABORARI" + ExtFl);
         fprintf(Out, "ANALISI ESTESA PERIODICITA': %s\n",(CPSZ)Msg);
         fprintf(Out, "%s\n",(CPSZ)Period);
         ELENCO_S Per = Record.PeriodicitaMV.Decod();
         ORD_FORALL(Per,i)fprintf(Out,"%s\n",(CPSZ)Per[i]);
         for (int mc = 0;mc < Record.NumMezziComponenti ; mc++ ) {
            MY_STRU::MEZZO_VIAGGIANTE & Mc = Record.Mv[mc];
            fprintf(Out,"\nAnalisi periodicita' mezzo componente %s\n",St(Mc.IdentTreno));
            PTRENO & Treno = * PTRENO::Get(Mc.IdentTreno);
            ELENCO_S Per = Treno.PeriodicitaTreno.Decod();
            ORD_FORALL(Per,j)fprintf(Out,"%s\n",(CPSZ)Per[j]);
            fprintf(Out,"Dati ottenuti direttamente da TabOrari:\n");
            int Nrec5 = 1;
            for (TabOr[Treno.PTaborariR2+1]; &TabOr.RecordCorrente() && TabOr.RecordCorrente().TipoRecord != '2';TabOr.Next() ) {
               TABORARI & To = TabOr.RecordCorrente();
               if (To.TipoRecord == '3') {
                  fprintf(Out,"   Tipo Record 3\n");
               } else if (To.TipoRecord == '5') {
                  Nrec5 ++;
               } else if (To.TipoRecord == '4') {
                  for (int i = 0;i < 5  ;i++ ) {
                     STRINGA Descr = To.R4.Periodicita[i].Analisi();
                     if (Descr != NUSTR) {
                        fprintf(Out,"   Prog Rec= %i   %s\n",Nrec5,(CPSZ)Descr);
                     } /* endif */
                  } /* endfor */
               } /* endif */
            } /* endfor */
//<<<    for  int mc = 0;mc < Record.NumMezziComponenti ; mc++    
         } /* endfor */
         if( Record.PeriodicitaDisuniformi ){
            // Ora mostro l' analisi della periodicit… dai dati delle fermate
            if (PerioF.Seek(Record.MezzoVirtuale)) {
               T_PERIODICITA LastP;
               LastP.ReSet() ;
               while (& PerioF.RecordCorrente()) {
                  PERIODICITA_FERMATA_VIRT & Ferm = PerioF.RecordCorrente();
                  if(Ferm.MezzoVirtuale != Record.MezzoVirtuale )break;
                  if(Ferm.Periodicita != LastP){
                     ELENCO_S Per = Ferm.Periodicita.Decod();
                     TabFv.Seek(Ferm.MezzoVirtuale, Ferm.Progressivo);
                     FERMATE_VIRT & Fe = TabFv.RecordCorrente();
                     fprintf(Out, "Periodicit… alla fermata MezzoCon %i progressivo %i ID =%i GSA %i GSP %i %s:\n",Fe.TrenoFisico ,Ferm.Progressivo,Fe.Id, Fe.GiornoSuccessivoArrivo , Fe.GiornoSuccessivoPartenza , Stazioni[Fe.Id].NomeStazione );
                     ORD_FORALL(Per,j)fprintf(Out,"%s\n",(CPSZ)Per[j]);
                     LastP = Ferm.Periodicita ;
                  }
                  PerioF.Next();
               } /* endwhile */
            } else {
					fprintf(Out,"ERRORE: Non trovata periodicit… delle fermate del mezzo virtuale %i\n", Record.MezzoVirtuale);
            } /* endif */
//<<<    if  Record.PeriodicitaDisuniformi   
         }
//<<< if  AnalisiPEstesa   
      } /* endif */
//<<< for  ULONG i= 0;i < InFile.NumRecordsTotali   ;i++    
   } /* endfor */
   TRACESTRING("====================================================================");
   
   printf("\n");
   delete &InFile;
   fclose(Out);
   
   TRACETERMINATE;
   
   return 0;
//<<< int  main int argc,char *argv    
}
