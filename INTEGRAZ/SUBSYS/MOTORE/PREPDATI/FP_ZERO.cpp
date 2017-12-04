//----------------------------------------------------------------------------                             c
// FP_ZERO.CPP: Carico i dati delle eccezioni tariffarie e delle classifiche
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

//----------------------------------------------------------------------------
//#define NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
//----------------------------------------------------------------------------

// begin EMS001 Win

typedef unsigned long BOOL;

// end EMS Win

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "oggetto.h"
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "ALFA0.HPP"
#include "POLIM.HPP"
#include "seq_proc.HPP"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "motglue.hpp"
#include "file_t.hpp"

#include "FT_PATHS.HPP"  // Path da utilizzare

#define PGM      "FP_ZERO"


//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"

   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   // EMS002 Win SetStatoProcesso(PGM, SP_PARTITO);

   FILE_RW Out(PATH_OUT   PGM ".LOG");
   Out.SetSize(0);
   AddFileToBprintf(&Out);

   PROFILER::Clear(FALSE);
   PROFILER::Clear(TRUE);

   TRACESTRING("prima di GRAFO::Grafo   = new GRAFO(PATH_DATI);");
   GRAFO::Grafo   = new GRAFO(PATH_DATI);
   GRAFO & Grafo = *GRAFO::Grafo;

   // Apro l' archivio decodifica codici CCR
   TRACESTRING("PRima di CCR_ID Ccr_Id(PATH_DATI);");
   CCR_ID Ccr_Id(PATH_DATI);

   if (CCR_ID::CcrId == NULL) {
      CCR_ID::CcrId = & Ccr_Id;
   } /* endif */

   int Rc = 0;

   TRACESTRING("Prima di FILE_RO Eccz1(PATH_CVB TRATTCOM.TXT);");
   FILE_RO Eccz1(PATH_CVB "TRATTCOM.TXT");
   TRACESTRING("Prima di FILE_RO Eccz2(PATH_CVB ASSORB3.TXT);");
   FILE_RO Eccz2(PATH_CVB "ASSORB3.TXT");
   TRACESTRING("Prima di FILE_RO Eccz3(PATH_CVB VEND_REL.TXT);");
   FILE_RO Eccz3(PATH_CVB "VEND_REL.TXT");
   TRACESTRING("Prima di FILE_RO Eccz4(PATH_CVB BIGECC.TXT);");
   FILE_RO Eccz4(PATH_CVB "BIGECC.TXT");
   TRACESTRING("Prima di FILE_RO Eccz56(PATH_CVB INIB_OD.TXT);");
   FILE_RO Eccz56(PATH_CVB "INIB_OD.TXT");
   TRACESTRING("Prima di F_ECCZ_NORM Ecz(PATH_DATI MM_ECZNR.DB);");
   F_ECCZ_NORM Ecz(PATH_DATI "MM_ECZNR.DB");
   TRACESTRING("DOPO Apertura di MM_ECZNR.DB");
   Ecz.Clear();
   TRACESTRING("DOPO Clear di Ecz");
   STRINGA Linea;
   Bprintf2("=================================================");
   Bprintf2( "Eccezioni normativa: TRATTE che non debbono essere tariffate se percorse avanti ed indietro");
   while (Eccz1.gets(Linea)) {
      TRACESTRING(Linea);
      if(Linea[0] == '*') continue ; // Commento
      ELENCO_S Analize = Linea.Tokens("/");
      if(Analize.Dim() < 2){
         Bprintf("Errore : Non riuscito ad interpretare linea: '%s'",(CPSZ)Linea);
         continue;
      } /* endif */
      ECCEZ_NORMATIVA Rec;
      ZeroFill(Rec);
      Rec.Tipo  = 1;
      Rec.Id1   = Ccr_Id[Analize[0].ToInt()];
      Rec.Id2   = Ccr_Id[Analize[1].ToInt()];
      Rec.ValeEsatto = Analize[2] != NUSTR && Analize[2][0] == '=';
      STRINGA DaStz= Stazioni[Rec.Id1].NomeStazione;
      Bprintf2("Da %i %s a %i %s",Rec.Id1,(CPSZ)DaStz,Rec.Id2,Stazioni[Rec.Id2].NomeStazione);
      if (Rec.Id1 == 0 ) {
         Bprintf("Errore : Non riuscito ad identificare Id1=%i %s , linea: '%s'",Rec.Id1,Stazioni[Rec.Id1].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      if (!IsNodo(Rec.Id1) ) {
         Bprintf("Errore : Id1=%i %s non e' nodo, linea: '%s'",Rec.Id1,Stazioni[Rec.Id1].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      if (Rec.Id2 == 0 ) {
         Bprintf("Errore : Non riuscito ad identificare Id2 = %i %s, linea: '%s'",Rec.Id2,Stazioni[Rec.Id2].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      if (!Grafo[Rec.Id2].Vendibile) {
         Bprintf("Warning: Stazione di inversione %i %s non vendibile, linea: '%s'",Rec.Id2,Stazioni[Rec.Id2].NomeStazione,(CPSZ)Linea);
         // continue;
      } /* endif */
      ARRAY_ID Percorso;  // Metto in forma normale
      Percorso += Rec.Id1;
      Percorso += Rec.Id2;
      PERCORSO_GRAFO Pg(Percorso);
      if (Pg.Nodi.Dim() > MAXECC ) {
         Bprintf("Errore : Superato il limite di %i stazioni del percorso, linea: '%s'",MAXECC,(CPSZ)Linea);
         Pg.Nodi.Trace(Stazioni,"Percorso troppo lungo: ");
         continue;
      } /* endif */
      ORD_FORALL(Pg.Nodi,i){
         ID Id = Pg.Nodi[i];
         Rec.Percorso[Rec.DimPercorso  ++] = Id;
      }
      Rec.Trace("Percorso");
      Ecz.AddRecordToEnd(VRB(Rec));
//<<< while  Eccz1.gets Linea
   }
   Bprintf2("=================================================");
   Bprintf2( "Eccezioni normativa: Stazioni da assorbire in stazioni piu' importanti o simili");
   while (Eccz2.gets(Linea)) {
      TRACESTRING(Linea);
      if(Linea[0] == '*') continue ; // Commento
      ELENCO_S Analize = Linea.Tokens(";");
      if(Analize.Dim() != 2){
         Bprintf("Errore : Non riuscito ad interpretare linea: '%s'",(CPSZ)Linea);
         continue;
      } /* endif */
      ECCEZ_NORMATIVA Rec;
      ZeroFill(Rec);
      Rec.Tipo  = 2;
      ID StazionePrin = Analize[0].ToInt();
      if ( StazionePrin == 0 ) {
         Bprintf("Errore : Non riuscito ad identificare Stazione Principale %i %s, linea: '%s'",StazionePrin,Stazioni[StazionePrin].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      Analize = Analize[1].Tokens(",");
      if(Analize.Dim() < 2){
         Bprintf("Errore : Non riuscito ad interpretare linea: '%s'",(CPSZ)Linea);
         continue;
      } /* endif */
      Rec.Id1   = Analize[0].ToInt();
      Rec.Id2 = 0;
      ARRAY_ID Percorso;
      Bprintf2("Percorso: Stazione principale = %i",StazionePrin);
      ORD_FORALL(Analize,j){
         ID Id = Analize[j].ToInt();
         Percorso += Id;
         Bprintf2(" --- %i %s",Id,Stazioni[Id].NomeStazione);
      }
      PERCORSO_GRAFO Pg(Percorso); // Metto in forma normale
      if (Pg.Nodi.Dim() > MAXECC ) {
         Bprintf("Errore : Superato il limite di %i stazioni del percorso, linea: '%s'",MAXECC,(CPSZ)Linea);
         Pg.Nodi.Trace(Stazioni,"Percorso troppo lungo: ");
         continue;
      } /* endif */
      ORD_FORALL(Pg.Nodi,i){
         ID Id = Pg.Nodi[i];
         if(Id == StazionePrin)Rec.Id2= i; // Notare che non va bene se e' il primo nodo (tale e' il comportamento corretto)
         Rec.Percorso[Rec.DimPercorso  ++] = Id;
      }
      if(Rec.Id2 == 0){
         Bprintf("Errore : La stazione %i deve comparire nel percorso e non deve essere la prima, linea: '%s'",StazionePrin,(CPSZ)Linea);
         continue;
      } /* endif */
      Rec.Trace("Percorso");
      Ecz.AddRecordToEnd(VRB(Rec));
//<<< while  Eccz2.gets Linea
   } /* endwhile */

   Bprintf2("=================================================");
   Bprintf2("Eccezioni normativa: Stazioni tariffabili solo su alcune relazioni");
   while (Eccz3.gets(Linea)) {
      TRACESTRING(Linea);
      if(Linea[0] == '*') continue ; // Commento
      ELENCO_S Analize = Linea.Tokens("/");
      if(Analize.Dim() < 2){
         Bprintf("Errore : Non riuscito ad interpretare linea: '%s'",(CPSZ)Linea);
         continue;
      } /* endif */
      ECCEZ_NORMATIVA Rec;
      ZeroFill(Rec);
      Rec.Tipo  = 3;
      Rec.Id1   = Ccr_Id[Analize[0].ToInt()];
      Rec.Id2   = Ccr_Id[Analize[1].ToInt()];
      STRINGA DaStz= Stazioni[Rec.Id1].NomeStazione;
      Bprintf2("Percorso Da %i %s a %i %s",Rec.Id1,(CPSZ)DaStz,Rec.Id2,Stazioni[Rec.Id2].NomeStazione);
      ORD_FORALL(Analize,j){
         ID Id = Ccr_Id[Analize[j].ToInt()];
         Bprintf2(" --- %i %s",Id,Stazioni[Id].NomeStazione);
      }
      if (Rec.Id1 == 0 ) {
         Bprintf("Errore : Non riuscito ad identificare Id1=%i %s, linea: '%s'",Rec.Id1,Stazioni[Rec.Id1].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      if (!Grafo[Rec.Id1].Vendibile) {
         Bprintf("Errore : Stazione Origine non vendibile =%i %s, linea: '%s'",Rec.Id1,Stazioni[Rec.Id1].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      if (Rec.Id2 == 0 ) {
         Bprintf("Errore : Non riuscito ad identificare Id2=%i %s, linea: '%s'",Rec.Id2,Stazioni[Rec.Id2].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      if (!Grafo[Rec.Id2].Vendibile) {
         Bprintf("Errore : Stazione Destinazione non vendibile =%i %s, linea: '%s'",Rec.Id2,Stazioni[Rec.Id2].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      ARRAY_ID Percorso;  // Metto in forma normale
      Percorso += Rec.Id1;
      Percorso += Rec.Id2;
      PERCORSO_GRAFO Pg(Percorso);
      if (Pg.Nodi.Dim() > MAXECC ) {
         Bprintf("Errore : Superato il limite di %i stazioni del percorso, linea: '%s'",MAXECC,(CPSZ)Linea);
         Percorso.Trace(Stazioni,"Percorso Originale");
         Pg.Nodi.Trace(Stazioni,"Percorso troppo lungo: ");
         continue;
      } /* endif */
      ORD_FORALL(Pg.Nodi,i){
         ID Id = Pg.Nodi[i];
         Rec.Percorso[Rec.DimPercorso  ++] = Id;
      }
      Rec.Trace("Percorso");
      Ecz.AddRecordToEnd(VRB(Rec));
//<<< while  Eccz3.gets Linea
   }
   Bprintf2("=================================================");
   Bprintf2( "Eccezioni normativa: eccezioni tariffarie maggiori");
   ELENCO_S Analize, Analize2;
   while (Eccz4.gets(Linea)) {
      TRACESTRING(Linea);
      if(Linea[0] == '*') continue ; // Commento
      if (Analize.Dim() == 0) {
         Analize = Linea.Tokens(" ;");
         if(Analize.Dim() < 2) {
            Bprintf("Errore : Non riuscito ad interpretare linea: '%s'",(CPSZ)Linea);
            exit(999);
         } /* endif */
         continue;
      } else if (Analize2.Dim() == 0) {
         Analize2 = Linea.Tokens(" ;");
         if(Analize2.Dim() < 2) {
            Bprintf("Errore : Non riuscito ad interpretare linea: '%s'",(CPSZ)Linea);
            exit(999);
         } /* endif */
         continue;
      } /* endif */
      ECCEZ_NORMATIVA Rec;
      ZeroFill(Rec);
      Rec.Tipo  = 4;
      Rec.Id1   = Analize[0].ToInt();
      Rec.Id2 = 0;
      Rec.ValeInizio  = Linea.Pos("I") >= 0;
      Rec.ValeFine    = Linea.Pos("F") >= 0;
      Rec.ValeInMezzo = Linea.Pos("M") >= 0;
      Rec.ValeEsatto  = Linea.Pos("E") >= 0;
      Rec.ValeInverso = Linea.Pos("D") < 0;
      ARRAY_ID Percorso;
      STRINGA Msg;
      if(Rec.ValeInizio   ) Msg += "ValeInizio ";
      if(Rec.ValeFine     ) Msg += "ValeFine ";
      if(Rec.ValeInMezzo  ) Msg += "ValeInMezzo ";
      if(Rec.ValeEsatto   ) Msg += "ValeEsatto ";
      if(Rec.ValeInverso  ) Msg += "ValeInverso ";
      TRACESTRING(Msg);
      Bprintf2("Percorso: %s", (CPSZ)Msg);
      ORD_FORALL(Analize,j){
         ID Id = Analize[j].ToInt();
         Percorso += Id;
         Bprintf2(" --- %i %s",Id,Stazioni[Id].NomeStazione);
      }
      PERCORSO_GRAFO Pg(Percorso); // Metto in forma normale
      if (Pg.Nodi.Dim() > MAXECC ) {
         Bprintf("Errore : Superato il limite di %i stazioni del percorso, linea: '%s'",MAXECC,(CPSZ)Linea);
         Pg.Nodi.Trace(Stazioni,"Percorso troppo lungo: ");
         continue;
      } /* endif */
      ORD_FORALL(Pg.Nodi,i){
         ID Id = Pg.Nodi[i];
         Rec.Id2= Id;
         Rec.Percorso[Rec.DimPercorso  ++] = Id;
      }
      Rec.Trace("Percorso");
      ECCEZ_NORMATIVA Rec2;
      ZeroFill(Rec2);
      Rec2.Tipo  = 4;
      Bprintf2("Percorso da sostituire: ");
      Percorso.Clear();
      ORD_FORALL(Analize2,j2){
         ID Id = Analize2[j2].ToInt();
         Percorso += Id;
         Bprintf2(" --- %i %s",Id,Stazioni[Id].NomeStazione);
      }
      if (Percorso.Dim() > MAXECC ) {
         Bprintf("Errore : Superato il limite di %i stazioni del percorso, linea: '%s'",MAXECC,(CPSZ)Linea);
         Percorso.Trace(Stazioni,"Percorso troppo lungo: ");
         continue;
      } /* endif */
      ORD_FORALL(Percorso,i2){
         ID Id = Percorso[i2];
         Rec2.Id2= Id;
         Rec2.Percorso[Rec2.DimPercorso  ++] = Id;
      }
      Rec2.Trace("Percorso che sostituisco:");
      Ecz.AddRecordToEnd(VRB(Rec));
      Ecz.AddRecordToEnd(VRB(Rec2));
      Analize.Clear();
      Analize2.Clear();
//<<< while  Eccz4.gets Linea
   } /* endwhile */
   Bprintf2("=================================================");
   Bprintf2( "Eccezioni normativa: Coppie O/D da inibire");
   ARRAY_DINAMICA<ECCEZ_NORMATIVA> Tipi6;
   while (Eccz56.gets(Linea)) {
      TRACESTRING(Linea);
      if(Linea[0] == '*') continue ; // Commento
      if(Linea[0] == ';') continue ; // Commento
      ELENCO_S Analize = Linea.Tokens("/");
      if(Analize.Dim() < 2){
         Bprintf("Errore : Non riuscito ad interpretare linea: '%s'",(CPSZ)Linea);
         continue;
      } /* endif */
      ECCEZ_NORMATIVA Rec;
      ZeroFill(Rec);
      Rec.Id1   = Ccr_Id[Analize[0].ToInt()];
      if (Rec.Id1 == 0 ) {
         Bprintf("Errore : Non riuscito ad identificare Id1=%i %s , linea: '%s'",Rec.Id1,Stazioni[Rec.Id1].NomeStazione,(CPSZ)Linea);
         continue;
      } /* endif */
      if (Analize[1][0] == 'S' && Analize[1][3] == 'G' ) {
         Rec.Tipo  = 6;
         Rec.Id2   = (1000 * Analize[1](1,2).ToInt()) + Analize[1](4,5).ToInt() ;
         if (Rec.Id2 == 0 ) {
            Bprintf("Errore : Nome gruppo illegale, gruppo %s", (CPSZ)Analize[1]);
            continue;
         } /* endif */
         Bprintf2("Inibizione Gruppo %i Stazione %i %s",Rec.Id2,Rec.Id1,Stazioni[Rec.Id1].NomeStazione);
         Tipi6 += Rec;
         continue;
      } else {
         Rec.Tipo  = 5;
         Rec.Id2   = Ccr_Id[Analize[1].ToInt()];
         if(Rec.Id2 && Rec.Id2 < Rec.Id1){
            int Tmp = Rec.Id1;
            Rec.Id1 = Rec.Id2;
            Rec.Id2 = Tmp;
         }
         STRINGA DaStz= Stazioni[Rec.Id1].NomeStazione;
         if(Rec.Id2 == 0) Bprintf2("Inibizione O/D %i %s Verso tutte le altre stazioni",Rec.Id1,(CPSZ)DaStz,Rec.Id2,Stazioni[Rec.Id2].NomeStazione);
         else Bprintf2("Inibizione O/D %i %s a %i %s",Rec.Id1,(CPSZ)DaStz,Rec.Id2,Stazioni[Rec.Id2].NomeStazione);
         if (Rec.Id2 == 0 && Analize[1][0] != '*' ) {
            Bprintf("Errore : Non riuscito ad identificare Id2 = %i %s, linea: '%s'",Rec.Id2,Stazioni[Rec.Id2].NomeStazione,(CPSZ)Linea);
            continue;
         } /* endif */
//<<< if  Analize 1  0  == 'G'
      } /* endif */
      Ecz.AddRecordToEnd(VRB(Rec));
//<<< while  Eccz56.gets Linea
   }
   ORD_FORALL(Tipi6, T6) Ecz.AddRecordToEnd(VRB(Tipi6[T6]));

   Bprintf2("=================================================");
   Bprintf2("Caricamento delle classifiche");
   FILE_FISSO<CATEGORIE> Classifiche(PATH_IN "CATEGOR.T");
   FILE_FISSO<CLASSIFICA_TRENI> Classifiche2(PATH_DATI "MM_CLASF.DB");
   Classifiche2.Clear("File Classifiche");

   ORD_FORALL(Classifiche,Cl){
      CATEGORIE & Cat = Classifiche[Cl];
      CLASSIFICA_TRENI Cat2;
      ZeroFill(Cat2);
      Cat2.Classifica                   = It(Cat.Codice) ;
      assert(Cat2.Classifica && Cat2.Classifica <= 28);
      STRINGA  Class(St(Cat.Categoria));
      Class.Strip();
      strcpy(Cat2.Descrizione, (CPSZ)Class);
      Cat2.TempoRidottoCoincidenza      = Cat.TempoRidottoCoincidenza      != ' ';
      Cat2.Fumatori                     = Cat.Fumatori                     != ' ';
      Cat2.Navale                       = Cat.Navale                       != ' ';
      Cat2.SicuramentePrenotabile       = Cat.SicuramentePrenotabile       != ' ';
      Cat2.SicuramentePrenotazioneObbli = Cat.SicuramentePrenotazioneObbli != ' ';
      Cat2.SupplementoIC                = Cat.SupplementoIC                != ' ';
      Cat2.SupplementoEC                = Cat.SupplementoEC                != ' ';
      Cat2.SupplementoETR               = Cat.SupplementoETR               != ' ';
      Cat2.TipoConcesso                 = Cat.TipoConcesso                 != ' ';
      Cat2.TipoInternazionale           = Cat.TipoInternazionale           != ' ';
      Classifiche2.AddRecordToEnd(VRB(Cat2));
      Bprintf2("Caricata classifica %s ",Cat2.Descrizione);
   };

   Bprintf2("=================================================");

   TRACETERMINATE;

   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   return Rc;
//<<< int main int argc,char *argv
}

