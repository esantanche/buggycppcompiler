//----------------------------------------------------------------------------
// DU_CLUMV: DUMP CLUSMEZV.TMP
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
#include "FT_PATHS.HPP"  // Path da utilizzare

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "oggetto.h"
#include "motglue.HPP"
#include "ALFA0.HPP"
#include "mm_perio.hpp"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ML_IN.HPP"
#include "ml_base.hpp"

//struct MEZZOV_CLUSTER {
struct MY_STRU {
   //----------------------------------------------------------------------------
   // Parte specifica: Definizione del records su file
   //----------------------------------------------------------------------------
   // Qui va la definizione della struttura
   WORD    MezzoVirtuale          ; // KEY: Mezzo virtuale
   WORD    IdCluster              ; // Id Cluster: Da 1 in poi
   WORD    OraPartenza            ; // Ora di partenza  del mezzo dalla stazione dominante (o meglio: Max tra ora partenza ed ora arrivo)
   BIT   Id1                   :13; // Id prima stazione dominante
   BIT   Id2                   :13; // Id seconda stazione dominante
   BIT   TipoCluster           : 4; // Vedi CLUSTER_MV
   BIT     Distanza1           :12; // Progressivo Km della stazione dominante 1
   BIT     Distanza2           :12; // Progressivo Km della stazione dominante 2
   BIT     VersoConcorde       : 1; // 1 = Concorde (Distanza2 > Distanza1) 0 = Discorde
   GRUPPO  Gruppo                 ; // Gruppo del mezzo virtuale
};
// Indice ausiliario
//struct CLUSTER_MEZZOV {
struct MY_STRU2 {
   WORD    IdCluster              ; // Id Cluster: Da 1 in poi
   BYTE    VersoConcorde          ; // 1 = Concorde (Distanza2 > Distanza1) 0 = Discorde
   WORD    OraPartenza            ; // Ora di partenza  del mezzo dalla stazione dominante (o meglio: Max tra ora partenza ed ora arrivo)
   WORD    MezzoVirtuale          ; // Mezzo virtuale
};
// struct MEZZO_VIRTUALE {
struct MY_STRU3 {
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
   } Mv[MAX_MEZZI_REALI]             ;
//<<< struct MY_STRU3  
};
// struct FERMATE_VIRT {
struct MY_STRU4 {
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
#define NOME "CLUSMEZV"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
#define NOME_IN        PATH_OUT NOME ".TMP"
#define NOME_IDX       PATH_OUT NOME ".IX1"
#define NOME_MV        PATH_OUT "M0_TRENV.TMP"
#define NOME_FV        PATH_OUT "M0_FERMV.TMP"
#define NOME_TRACE     NOME ".TRC"
#define BUF_DIM        128000
//----------------------------------------------------------------------------

class  MY_FILE : public FILE_FIX {
   public:
   MY_FILE():
   FILE_FIX(NOME_IN,sizeof(MY_STRU),BUF_DIM){};
   MY_STRU &  operator [](ULONG Indice){ Posiziona(Indice); return *(MY_STRU*) RecordC; };
};
class  MY_FILE2: public FILE_FIX {
   public:
   MY_FILE2():
   FILE_FIX(NOME_IDX,sizeof(MY_STRU2),BUF_DIM){};
   MY_STRU2 &  operator [](ULONG Indice){ Posiziona(Indice); return *(MY_STRU2*) RecordC; };
};
class  MY_FILE3 : public FILE_FIX {
   public:
   MY_FILE3():
   FILE_FIX(NOME_MV,sizeof(MY_STRU3),BUF_DIM){};
   MY_STRU3 &  operator [](ULONG Indice){ Posiziona(Indice); return *(MY_STRU3*) RecordC; };
};
class  MY_FILE4 : public FILE_FIX {
   public:
   MY_FILE4():
   FILE_FIX(NOME_FV,sizeof(MY_STRU4),BUF_DIM){};
   MY_STRU4 &  operator [](ULONG Indice){ Posiziona(Indice); return *(MY_STRU4*) RecordC; };
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

void main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"
   // TRACEREGISTER2(NULL,NOME,NOME_TRACE);
   FILE * Out;
   Out = fopen(NOME_OUT,"wt");
   int Cluster = 0;
   ARRAY_ID TreniV;
   {
      printf("Analizzo i records di %s scrivendo il risultato su %s\n",NOME_IN,NOME_OUT);
      if(argc > 1){
         Cluster = atoi(argv[1]);
         printf("Analisi ristretta al cluster %i con scarico dei dati dei treni \n",Cluster);
      }
      
      MY_FILE & InFile = * new MY_FILE();
      STRINGA HDR1,HDR2,FMT;
      HDR1 = "RecN  ";
      HDR2 = "      ";
      FMT  = "%-6u";
      
      //----------------------------------------------------------------------------
      // Parte specifica: Macro definizione formato di stampa
      //----------------------------------------------------------------------------
      MK1( MezzoVirtuale , "6","u","Mz","Virt")
      MK1( IdCluster     , "6","u","Id","Clust")
      MK1( Id1           , "6","u","Id1","")
      MK1( Id2           , "6","u","Id2","")
      MK1( Distanza1     , "6","u","D1","")
      MK1( Distanza2     , "6","u","D2","")
      MK1( TipoCluster   , "6","u","Tipo","Clust")
      MK1( VersoConcorde , "6","u","Verso","1=con.")
      MK1( OraPartenza   , "6","u","Ora","Part.")
      MK1( Gruppo        ,"40","s","Gruppo","")
      
      //----------------------------------------------------------------------------
      
      
      printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());
      
      
      int j = 0;
      for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
         MY_STRU & Record = InFile[i];
         if(Cluster && Record.IdCluster != Cluster) continue;
         if (j % NUMRIGHE_DUMP == 0) {
            if(j != 0)fprintf(Out,"\n");
            fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
            // TRACESTRING(HDR1);
            // TRACESTRING(HDR2);
         } /* endif */
         j++ ;
         if(Cluster) TreniV += Record.MezzoVirtuale;
         char Buf[1024];
         sprintf(Buf,(CPSZ)FMT,i
            //----------------------------------------------------------------------------
            // Parte specifica: Macro definizione variabili da stampare
            //----------------------------------------------------------------------------
            // Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
            MK2( MezzoVirtuale ," 6","u","Mz","Virt")
            MK2( IdCluster     , "6","u","Id","Clust")
            MK2( Id1           , "6","u","Id1","")
            MK2( Id2           , "6","u","Id2","")
            MK2( Distanza1     , "6","u","D1","")
            MK2( Distanza2     , "6","u","D2","")
            MK2( TipoCluster   , "6","u","Tipo","Clust")
            MK2( VersoConcorde , "6","u","Verso","1=con.")
            MK2( OraPartenza   , "6","u","Ora","Part.")
           ,(CPSZ)(STRINGA)Record.Gruppo
            //----------------------------------------------------------------------------
            
         );
         fprintf(Out,"%s\n",Buf);
         // TRACESTRING(Buf);
         // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
//<<< for  ULONG i= 0;i < InFile.NumRecordsTotali   ;i++    
      } /* endfor */
      
      delete &InFile;
//<<< ARRAY_ID TreniV;
   }
   
   // INDICE
   {
      printf("Analizzo i records dell' indice %s scrivendo il risultato su %s\n",NOME_IDX,NOME_OUT);
      MY_FILE2 & InFile = * new MY_FILE2();
      STRINGA HDR1,HDR2,FMT;
      HDR1 = "RecN  ";
      HDR2 = "      ";
      FMT  = "%-6u";
      
      //----------------------------------------------------------------------------
      // Parte specifica: Macro definizione formato di stampa
      //----------------------------------------------------------------------------
      MK1( IdCluster     ," 6","u","Id","Clust")
      MK1( VersoConcorde ," 6","u","Verso","1=con.")
      MK1( OraPartenza   ," 6","u","Ora","Part.")
      MK1( MezzoVirtuale ," 6","u","Mz","Virt")
      
      //----------------------------------------------------------------------------
      
      printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());
      
      int j = 0;
      for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
         MY_STRU2 & Record = InFile[i];
         if(Cluster && Record.IdCluster != Cluster) continue;
         if (j % NUMRIGHE_DUMP == 0) {
            if(j != 0)fprintf(Out,"\n");
            fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
            // TRACESTRING(HDR1);
            // TRACESTRING(HDR2);
         } /* endif */
         j++;
         char Buf[500];
         sprintf(Buf,(CPSZ)FMT,i
            //----------------------------------------------------------------------------
            // Parte specifica: Macro definizione variabili da stampare
            //----------------------------------------------------------------------------
            MK2( IdCluster     ," 6","u","Id","Clust")
            MK2( VersoConcorde ," 6","u","Verso","1=con.")
            MK2( OraPartenza   ," 6","u","Ora","Part.")
            MK2( MezzoVirtuale ," 6","u","Mz","Virt")
            //----------------------------------------------------------------------------
            
         );
         fprintf(Out,"%s\n",Buf);
         // TRACESTRING(Buf);
         // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
//<<< for  ULONG i= 0;i < InFile.NumRecordsTotali   ;i++    
      } /* endfor */
      
      delete &InFile;
//<<< // INDICE
   }
   
   //----------------------------------------------------------------------------
   // Questo e' lo scarico dei mezzi virtuali
   //----------------------------------------------------------------------------
   if (Cluster) {
      PTRENO::Restore(); // Parto dai dati caricati l' ultima volta
      
      MY_FILE3 & InFile = * new MY_FILE3();
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
      FMT  = FMT0 + FMT1;
      // fprintf(Out,"%s\n",(CPSZ)FMT); // Per debug
      
      //----------------------------------------------------------------------------
      
      // puts((CPSZ)HDR1);
      // puts((CPSZ)HDR2);
      // puts((CPSZ)FMT);
      
      printf("Scarico dati dei MV: Debbo analizzare %i records\n",InFile.NumRecordsTotali());
      
      int k = 0 ;
      for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
         MY_STRU3 & Record = InFile[i];
         if(!TreniV.Contiene(Record.MezzoVirtuale)) continue;
         
         if (k % NUMRIGHE_DUMP == 0) {
            if(k != 0)fprintf(Out,"\n");
            fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
            // TRACESTRING(HDR1);
            // TRACESTRING(HDR2);
         } /* endif */
         k++;
         
         char * HaPeriodicita = "No";
         if(Record.PeriodicitaMV  != T_PERIODICITA::InOrario || Record.PeriodicitaDisuniformi ){
            HaPeriodicita = "Si";
         }
         
         STRINGA Servizi = Record.ServiziUniformi.Decodifica(TRUE);
         
         STRINGA NomeVirtuale;
         for (int j = 0;j < Record.NumMezziComponenti ;j++ ) {
            if(Record.Mv[j].HaNome){
               PTRENO & Treno = *PTRENO::Get(Record.Mv[j].IdentTreno);
               NomeVirtuale = St(Treno.NomeMezzoViaggiante);
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
         
         //----------------------------------------------------------------------------
         fprintf(Out,"%s\n",Buf);
         
//<<< for  ULONG i= 0;i < InFile.NumRecordsTotali   ;i++    
      }
//<<< if  Cluster   
   } /* endif */

   if (Cluster) {
   STAZIONI Stazioni(PATH_DATI,"ID_STAZI.DB",0);
                                                                                    
   MY_FILE4 & InFile = * new MY_FILE4();
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

   printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());

   int Init = -1;

   int k = 0;
   for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
      MY_STRU4 & Record = InFile[i];

      if(!TreniV.Contiene(Record.MezzoVirtuale)) continue;

      if(Record.ProgKm > 3000)printf("> 3000 Km Trenv %i\n",Record.MezzoVirtuale);

      // Controllo via record range
      //if( i < MinMezzo)continue;
      //if(MaxMezzo -- < 0)break;

      if(Init == -1)Init = i % NUMRIGHE_DUMP;
      if (k % NUMRIGHE_DUMP == Init) {
         if(k != 0)fprintf(Out,"\n");
         fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
         // TRACESTRING(HDR1);
         // TRACESTRING(HDR2);
      } /* endif */
      k++;
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
      // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU4));



   } /* endfor */

   delete &InFile;
   } /* endif */
   
   
   fclose(Out);
   
   // TRACETERMINATE;
   
   exit(0);
//<<< void main int argc,char *argv    
}

