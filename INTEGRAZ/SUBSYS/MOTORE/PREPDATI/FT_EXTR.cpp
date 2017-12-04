//----------------------------------------------------------------------------
// FT_EXTR: Estrazione pezzi selezionati di Taborari e localita
//----------------------------------------------------------------------------
// Estrae solo i records relativi ad un insieme selezionato di TRENI
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_IN.HPP"
#include "seq_proc.hpp"
#include "file_t.hpp"
#include "ctype.h"

#define PGM      "FT_EXTR"

const char Spaces[] = "                                                                                ";

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int main(int argc,char *argv[]){                                                   
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
   
   TRACEREGISTER2(NULL,PGM, PATH_OUT PGM ".TRC");
   trchse = 2; // Forzo il tracelevel a 2
   GESTIONE_ECCEZIONI_ON
   
   BIN_ELENCO_S Ok ;
   ELENCO_S Ok2 ;      // Ricerche parziali
   ARRAY_ID CCRs;      // Ricerche per stazioni di fermata (via CCR)
   ELENCO_S Nomi;      // Ricerche per stazioni di fermata (via Nome)
   BOOL ShowLineNum = 0;
   char Buf1[256];
   int Count5 = 0;
   int SoloInfocomm = 0;


   printf(
   " Opzioni:\n"
   " /L      : mostra i numeri di linea sui files originali (e su taborari conteggio records di tipo 5)\n"
   " /S      : Estrae solo infocomm\n"
   " /Nnnn   : mostra nnn linee dopo ogni mezzo che soddisfa uno dei seguenti criteri\n"
   " $xxx    : mostra tutti i mezzi che hanno nome che inizia con xxx\n"
   " %CCR    : mostra tutti i mezzi che FERMANO alla stazione con codice CCR (necessarie 2 scansioni)\n"
   " #zzz    : mostra tutti i mezzi che FERMANO alla stazione con nome CONTENENTE zzz (case insensitive, necessarie 2 scansioni)\n"
   " yyy     : Mostra il mezzo con nome yyy\n"
   " @file   : File contenente la lista dei treni (Commenti iniziano per ';') \n\n");

   int LineeDaMostrare = 0;



   BOOL DaFile = FALSE;
   STRINGA NomeFile;
   for (int a = 1;a < argc ; a ++ ) {
      STRINGA Tmp(argv[a]);
      if(Tmp[0] == '/' ){
         if(Tmp[1] == 'L' || Tmp[1] == 'l') ShowLineNum = 1;
         if(Tmp[1] == 'S' || Tmp[1] == 's') SoloInfocomm= 1;
         if(Tmp[1] == 'N' || Tmp[1] == 'n') LineeDaMostrare = Tmp(2,999).ToInt();
      } else if(Tmp[0] == '$' ){
         Ok2 += Tmp(1,999);
      } else if(Tmp[0] == '%' ){
         CCRs += Tmp(1,999).ToInt();
      } else if(Tmp[0] == '#' ){
         Nomi += Tmp(1,999).Strip().UpCase();
      } else if(Tmp[0] == '@' ){
         DaFile = TRUE;
         NomeFile = Tmp(1, Tmp.Dim());
      } else {
         Tmp.Pad(10);
         Ok += Tmp;
      }
   } /* endfor */
   

   if (DaFile) {
      FILE_RO ListaTreni(NomeFile);
      STRINGA Linea;
      while (ListaTreni.gets(Linea)) {
         if(Linea[0] == ';')continue; // Commento
         if(Linea[0] == '/')continue; // Commento
         Ok += Linea.Pad(10);
      } /* endwhile */
   } 

   // Oppure se setto Compositi mi include automaticamente tutti i treni compositi
   // BOOL Compositi = TRUE;
   BOOL Compositi = FALSE;
   
   // Se setto Literals a true mi include tutti i treni che hanno delle lettere (a-z,A-Z)
   // nel Nome
   // BOOL Literals = TRUE;
   BOOL Literals = FALSE;
   
   
   {
      FILE_RW OutL(PATH_OUT "LOCALITA.XTR"); OutL.SetSize(0);
      FILE_RW OutT(PATH_OUT "TABORARI.XTR"); OutT.SetSize(0);
      FILE_RW OutI(PATH_OUT "INFOCOMM.XTR"); OutI.SetSize(0);
      
      // Apro l' archivio LOCALITA'
      FILE_LOCALITA Loc(PATH_IN "LOCALITA.T");
      
      // Apro l' archivio TABORARI
      FILE_TABORARI TabOr(PATH_IN "TABORARI.T");
      
      // Apro l' archivio INFOCOMM
      FILE_INFOCOMM InfCo(PATH_IN "INFOCOMM.T");
      
      if( CCRs.Dim() || Nomi.Dim() ){ // Debbo identificare per codice CCR o per nomi
         int j = 0;
         ORD_FORALL(TabOr,k){ 
            if(k % 10000 == 0) printf("Scansione accoppiata TABORARI / LOCALITA': Record Nø %i\r",k);
            TABORARI & To = TabOr[k];
            if(To.TipoRecord ==  '5'){
               if (To.R5.Flags.Transito == '*') {
                  j++;          // E' solo un transito
               } else {
                  LOCALITA & Lo = Loc[j];
                  j++;
                  if(CCRs.Contiene(Lo.Codice.Cod())){
                     Ok += St(Lo.IdentTreno);
                  } else if(Nomi.Dim()){
                     STRINGA Citta = St(Lo.Descrizione);
                     Citta.UpCase();
                     FORALL(Nomi,i){
                        if(Citta.Pos(Nomi[i]) >= 0){
                           Ok += St(Lo.IdentTreno);
                        }
                     }
                  }
               } /* endif */
            }
         }
         puts("");
      }

      BOOL Composito = FALSE;
      if(Compositi){ // Ricerca generica di tutti i treni compositi
         ORD_FORALL(TabOr,k){ // 1ø Scansione TABORARI
            if(k % 10000 == 0){
               TRACEINT("1ø scansione TABORARI: Record",k);
               printf("1ø scansione TABORARI: Record %i     \r",k);
            }
            TABORARI & To = TabOr[k];
            if(To.TipoRecord ==  '2'){
               Composito = memcmp(To.R2.IdentTrenoProvenienza,Spaces,20);
               if (Composito) {
                  STRINGA Ident1(St(To.R2.IdentTrenoProvenienza));
                  STRINGA Ident2(St(To.R2.IdentTrenoDestinazione));
                  if(Ident1 != STRINGA("          ") && !Ok.Contiene(Ident1)) Ok += Ident1;
                  if(Ident2 != STRINGA("          ") && !Ok.Contiene(Ident2)) Ok += Ident2;
               } /* endif */
            } else if(To.TipoRecord ==  '3'){
               if(Composito){
                  STRINGA Ident(St(To.R3.IdentTreno));
                  if(!Ok.Contiene(Ident)) Ok += Ident;
                  Composito = FALSE;
               }
            }
//<<<    ORD_FORALL(TabOr,k){ // 1ø Scansione TABORARI
         } /* endfor */
         puts("");
//<<< if(Compositi){ // Ricerca generica di tutti i treni compositi
      }
      
      Ok.Sort();
      BOOL Write = FALSE;
      TABORARI R2;
      ELENCO_S Ok3;
      int LineR2;
      int Trigger = 10000000;
      if(!SoloInfocomm) ORD_FORALL(TabOr,j){ // 2ø Scansione TABORARI
         if(j % 10000 == 0){
            TRACEINT("TABORARI: Record",j);
            printf("TABORARI: Record %i       \r",j);
         }
         TABORARI & To = TabOr[j]; 
         int Tr = To.TipoRecord - '0';
         if(Tr == 2){
            memcpy(&R2,&To,sizeof(TABORARI));
            Write = FALSE;
            LineR2 = j;
         } else if( ! Write && Tr == 3){ // il !Write per i record 3 duplicati
            STRINGA Ident(St(To.R3.IdentTreno));
            if(Ok.Bin_Contiene(Ident)){
               if(ShowLineNum) OutT.Scrivi(Buf1, sprintf(Buf1,"%6.6u %6.6u:",LineR2,Count5));
               OutT.Scrivi(BUFR(&R2,sizeof(TABORARI)));
               if(ShowLineNum) OutT.Scrivi(Buf1, sprintf(Buf1,"%6.6u %6.6u:",j,Count5));
               OutT.Scrivi(BUFR(&To,sizeof(TABORARI)));
               Write = TRUE;
               Trigger = 0;
            } else {
               if(Literals){
                  char * P = St(To.R3.IdentTreno);
                  for(int l=0; l < 10; P++,l++){
                     if(isalpha(*P)){
                        if(ShowLineNum) OutT.Scrivi(Buf1, sprintf(Buf1,"%6.6u %6.6u:",LineR2,Count5));
                        OutT.Scrivi(BUFR(&R2,sizeof(TABORARI))); 
                        if(ShowLineNum) OutT.Scrivi(Buf1, sprintf(Buf1,"%6.6u %6.6u:",j,Count5));
                        OutT.Scrivi(BUFR(&To,sizeof(TABORARI)));
                        Write = TRUE;
                        if(!Ok3.Contiene(Ident))Ok3 += Ident;
                        Trigger = 0;
                        break;
                     }
                  }
               }
               ORD_FORALL(Ok2,i){
                  if(!memcmp((CPSZ)Ident,(CPSZ)Ok2[i],Ok2[i].Dim())){
                     if(ShowLineNum) OutT.Scrivi(Buf1, sprintf(Buf1,"%6.6u %6.6u:",LineR2,Count5));
                     OutT.Scrivi(BUFR(&R2,sizeof(TABORARI))); 
                    if(ShowLineNum) OutT.Scrivi(Buf1, sprintf(Buf1,"%6.6u %6.6u:",j,Count5));
                     OutT.Scrivi(BUFR(&To,sizeof(TABORARI)));
                     Write = TRUE;
                     if(!Ok3.Contiene(Ident))Ok3 += Ident;
                     Trigger = 0;
                     break; // Per evitare scritture multiple
                  }
               }
//<<<       if(Ok.Bin_Contiene(Ident)){
            }
//<<<    if(Tr == 2){
         } else if(Write) {
               if(ShowLineNum) OutT.Scrivi(Buf1, sprintf(Buf1,"%6.6u %6.6u:",j,Count5));
               OutT.Scrivi(BUFR(&To,sizeof(TABORARI)));
               Trigger = 0;
         }
         if(Trigger > 0 && Trigger <= LineeDaMostrare){
            if(ShowLineNum) OutT.Scrivi(Buf1, sprintf(Buf1,"%6.6u %6.6u:",j,Count5));
            OutT.Scrivi(BUFR(&To,sizeof(TABORARI)));
         }
         Trigger ++;

         if(Tr == 5)Count5 ++;
//<<< ORD_FORALL(TabOr,j){ // 2ø Scansione TABORARI
      } /* endfor */
      puts("");
      
      Ok += Ok3;
      TRACEVLONG(Ok.Dim());
      Ok.Sort();
      
      Trigger = 10000000;
      if(!SoloInfocomm) ORD_FORALL(Loc,i){
         if(i % 10000 == 0){
            TRACEINT("LOCALITA': Record",i);
            printf("LOCALITA': Record %i      \r",i);
         }
         LOCALITA & Lo = Loc[i];
         // if(Lo.TestRecord != 'A') continue; // Bad
         STRINGA Ident(St(Lo.IdentTreno));
         if(Ok.Bin_Contiene(Ident)){
            if(ShowLineNum) OutL.Scrivi(Buf1, sprintf(Buf1,"%6.6u:",i));
            OutL.Scrivi(BUFR(&Lo,sizeof(LOCALITA)));
            Trigger = 0;
         }
         if(Trigger > 0 && Trigger <= LineeDaMostrare){
            if(ShowLineNum) OutL.Scrivi(Buf1, sprintf(Buf1,"%6.6u:",i));
            OutL.Scrivi(BUFR(&Lo,sizeof(LOCALITA)));
         }
         Trigger ++;
      }
      puts("");
       
      STRINGA Ident;
      Write = FALSE;
      ORD_FORALL(InfCo,x){
         if(x % 1000 == 0){
            TRACEINT("INFOCOMM': Record",x);
            printf("INFOCOMM': Record %i      \r",x);
         }
         INFOCOMM & Inf = InfCo[x];
         int Tr = Inf.TipoRecord - '0';
         switch (Tr){
            case 1:
               {
                  Ident = St(Inf.R1.IdentTreno);
                  if(Ok.Bin_Contiene(Ident)){
                     if(ShowLineNum) OutI.Scrivi(Buf1, sprintf(Buf1,"%6.6u:",x));
                     OutI.Scrivi(BUFR(&Inf,sizeof(INFOCOMM)));
                     Write = TRUE;
                  }
                  else
                     Write = FALSE;
               }
               break;
            case 2:
               if (Write) {
                  OutI.Scrivi(BUFR(&Inf,sizeof(INFOCOMM)));
               } 
               break;
            case 3:
               if (Write) {
                  OutI.Scrivi(BUFR(&Inf,sizeof(INFOCOMM)));
               } 
               break;
         }

      }
   };


   
   TRACESTRING("Pronto a finire");
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
//<<< void main(int argc,char *argv[]){                                                   
}

