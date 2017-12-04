//----------------------------------------------------------------------------
// DU_FERMV: DUMP M0_FERMV.TMP
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"

// struct FERMATE_VIRT {
struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
// Qui va la definizione della struttura
   WORD MezzoVirtuale        ; // KEY: Mezzo virtuale
   WORD Progressivo          ; // KEY: Progressivo fermata (in ambito treno virtuale, Parte da 1)
                               // NB: NON viene riordinato dopo la soppressione delle fermate ad opera di ML_STEP1
                               // Perche' e' anche foreign key per le periodicita', le note ed i servizi
   WORD Progressivo2         ; // Come sopra ma escludendo i transiti su stazioni non nodali ne di cambio
                               // Viene riordinato dopo aver individuato le stazioni di cambio
   BIT  CCR               :24; // CCR stazione fermata (come da file .T ) 
                               // Comprensivo del codice Rete se la stazione non e' italiana
   BIT  Id                :13; // Id stazione fermata (Cod DKM/Sipax)
   BIT  TrenoFisico       : 3; // Treno fisico di appartenenza (indice )
   BIT  OraArrivo         :11; // Ora di arrivo   (minuti dalla mezzanotte)
   BIT  OraPartenza       :11; // Ora di partenza (minuti dalla mezzanotte)
   BIT  GiornoSuccessivoArrivo  : 2; // Se arriva alla fermata il giorno successivo alla partenza del treno
   BIT  GiornoSuccessivoPartenza: 2; // Se parte dalla fermata il giorno successivo alla partenza del treno
   BIT  ProgKm            :12; // Progressivo Km 
   BIT  Transito          : 1; // Vero se il treno transita solamente (O fermate di servizio)
   BIT  FermataFacoltativa: 1; // Vero se ???
   // Nota: a differenza dei files T i bit di partenza ed arrivo sono sempre valorizzati.
   BIT  FermataPartenza   : 1; // Vero se la fermata permette di partire
   BIT  FermataArrivo     : 1; // Vero se la fermata permette di arrivare
   BIT  FermataDiServizio : 1; // Vero se la fermata e' esclusivamente di servizio (in tal caso forzo Transito a TRUE)
   BIT  HaNote            : 1; // Vero se la fermata ha delle note 
   BIT  HaNoteLocalita    : 1; // Vero se la STAZIONE di fermata ha delle note
};
#define NOME "M0_FERMV"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
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
//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      
                                                                                    
void main(int argc,char *argv[]){                                                   
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          

   // TRACEREGISTER2(NULL,NOME,PATH_OUT NOME_TRACE);

   int MinMezzo, MaxMezzo;
   printf("Utilizzo: DU_FERMV MinMezzo [MaxMezzo] [Opzioni]\n"
      " /F0     : Dati come in uscita da FT_STEP0\n"
      " /F1     : Dati non Linearizzati \n"
      " /F2     : Dati Linearizzati\n"
   );
   int Dettaglio = 2;
   MinMezzo = 0;
   MaxMezzo = 9999999;

   for (int a = 1;a < argc ; a ++ ) {
      STRINGA Tmp(argv[a]);
      Tmp.UpCase();
      if(Tmp[0] == '/' ){
         if(Tmp[1] == 'F' ) Dettaglio = Tmp[2] - '0'; 
      } else if(MinMezzo == 0){
         MaxMezzo = MinMezzo = Tmp.ToInt();
      } else {
         MaxMezzo = Tmp.ToInt();
      }
   } /* endfor */

   char * Crs = "01P"; 
   NOME_IN =  NOME_IN_BSE ;
   NOME_IN += Crs[Dettaglio];

   if(MaxMezzo == 9999999)exit(999);

   printf("Analizzo i records di %s scrivendo il risultato su %s\n",(CPSZ)NOME_IN,NOME_OUT);
   printf("Analisi ristretta ai mezzi virtuali da %i a %i\n",MinMezzo, MaxMezzo);

   STAZIONI Stazioni(PATH_DATI,"ID_STAZI.DB",0);
                                                                                    
   MY_FILE & InFile = * new MY_FILE();                                              
   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN    ";
   HDR2 = "        ";
   FMT  = "%-8u";
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
   MK1( MezzoVirtuale      , "6","u","Mezzo","Virt")
   MK1( Progressivo        , "5","u","Prog","Fer.")
   MK1( Progressivo2       , "5","u","Pr 2","Fer.")
   MK1( Id                 , "6","u","Id","")
   MK1( CCR                , "8","u","CCR","")
   MK1( TrenoFisico        , "6","u","Treno","Fis.")
   MK1( OraArrivo          , "5","u","Ora","Arr.")
   MK1( OraPartenza        , "5","u","Ora","Par.")
   MK1( GiornoSuccessivoArrivo   , "3","u","Gs","A")
   MK1( GiornoSuccessivoPartenza , "3","u","Gs","P")
   MK1( ProgKm             , "6","u","Km","")
   MK1( Transito           , "3","u","Tr","")
   MK1( FermataFacoltativa , "4","u","Fac","")
   MK1( FermataPartenza    , "3","u", "P","")
   MK1( FermataArrivo      , "3","u", "A","")
   MK1( FermataDiServizio  , "4","u", "Srv","")
   MK1( HaNote             , "3","u","Nf","")
   MK1( HaNoteLocalita     , "3","u","Nl","")
   MK1( NomeStazione       ,"36","s","Nome Stazione","")

//----------------------------------------------------------------------------

   // puts((CPSZ)HDR1);
   // puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);

   FILE * Out;
   Out = fopen(NOME_OUT,"wt");

   printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());

   int Init = -1;

   for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
      MY_STRU & Record = InFile[i];

      // Controllo via Nø mezzo virtuale
      if(Record.MezzoVirtuale < MinMezzo)continue;
      if(Record.MezzoVirtuale > MaxMezzo)break;

      if(Record.ProgKm > 3000)printf("> 3000 Km Trenv %i\n",Record.MezzoVirtuale);

      // Controllo via record range
      //if( i < MinMezzo)continue;
      //if(MaxMezzo -- < 0)break;

      if(Init == -1)Init = i % NUMRIGHE_DUMP;
      if (i % NUMRIGHE_DUMP == Init) {
         if(i != 0)fprintf(Out,"\n");
         fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
         // TRACESTRING(HDR1);
         // TRACESTRING(HDR2);
      } /* endif */
      char Buf[500];
      sprintf(Buf,(CPSZ)FMT,i
//----------------------------------------------------------------------------
// Parte specifica: Macro definizione variabili da stampare
//----------------------------------------------------------------------------
   MK2( MezzoVirtuale      , "6","u","Mezzo","Virt")
   MK2( Progressivo        , "5","u","Prog","Ferm")
   MK2( Progressivo2       , "5","u","Pr 2","Ferm")
   MK2( Id                 , "6","u","Id","")
   MK2( CCR                , "8","u","CCR","")
   MK2( TrenoFisico        , "6","u","Treno","Fis.")
   ,  100*(Record.OraArrivo /60) + Record.OraArrivo %60
   ,  100*(Record.OraPartenza /60) + Record.OraPartenza %60
   MK2( GiornoSuccessivoArrivo   , "3","u","Gs","A")
   MK2( GiornoSuccessivoPartenza , "3","u","Gs","P")
   MK2( ProgKm             , "6","u","Km","")
   MK2( Transito           , "3","u","Tr","")
   MK2( FermataFacoltativa , "3","u","Fac","")
   MK2( FermataPartenza    , "3","u", "P","")
   MK2( FermataArrivo      , "3","u", "A","")
   MK2( FermataDiServizio  , "4","u", "Srv","")
   MK2( HaNote             , "3","u","Nf","")
   MK2( HaNoteLocalita     , "3","u","Nl","")
   ,Stazioni[Record.Id].NomeStazione
//----------------------------------------------------------------------------

      );
      fprintf(Out,"%s\n",Buf);
      // TRACESTRING(Buf);
      // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));



   } /* endfor */

   delete &InFile;
   fclose(Out);

   // TRACETERMINATE;
   
   exit(0);
//<<< void main(int argc,char *argv[]){
}

