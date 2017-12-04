//----------------------------------------------------------------------------
// DU_STAFS: DUMP Stazioni del grafo
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "oggetto.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_OUT.HPP"
#include "elenco.h"
#include "eventi.h"
#include "seq_proc.hpp"
#include "scandir.h"
#include "mm_basic.hpp"
#include "mm_grafo.hpp"

#define NOME "DU_STAFS"

#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
#define NOME_IN        PATH_OUT NOME ".TMP"
#define NOME_TRACE     NOME ".TRC"
#define BUF_DIM        128000

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

   // TRACEREGISTER2(NULL,NOME,NOME_TRACE);

   printf("Analizzo le stazioni del grafo");
                                                                                    
      // APRO il grafo
      PROFILER::Clear(FALSE); // Per aprire il grafo
      GRAFO::Grafo   = new GRAFO(PATH_DATI);
      GRAFO & Grafo = GRAFO::Gr();

   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN ";                                                                  
   HDR2 = "     ";                                                                  
   FMT  = "%-5u";
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
   MK1( Id                 , "5","u","Id","")
   MK1( StazioneFS         , "3","u","FS","")
   MK1( NumeroConcesse     , "4","u","Nø","Con")
   MK1( NumeroPrimiVicini  , "4","u","Nø","PV")
   MK1( NumeroPolimetriche , "4","u","Nø","Pol")
   MK1( IdRamo             , "5","u","Id","Ramo")
   MK1( DiDiramazione      , "4","u","Di","Dir")
   MK1( Nodale             , "4","u","Nod","")
   MK1( Terminale          , "4","u","Ter","")
   MK1( DiAllacciamento    , "4","u","All","")
   MK1( DiInstradamento    , "4","u","Ist","")
   MK1( DiTransito         , "4","u","Trn","")
   MK1( Vendibile          , "4","u","Vnd","")
   MK1( InstrGrafo         , "6","u","Istr","Grafo")
   MK1( Abilitazione       , "4","u","Abl","")
   MK1( CCR                , "6","u","CCR","")
   MK1( Km1                , "5","u","Km1","")
   MK1( ProgRamo           , "4","u","Prg","Ram")
   MK1( IsCnd              , "3","u","I","C")
   MK1( IdentIstr          , "7","s","Nome7","")

//----------------------------------------------------------------------------

   // puts((CPSZ)HDR1);
   // puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);

   FILE * Out;
   Out = fopen(NOME_OUT,"wt");

   printf("Debbo analizzare %i stazioni\n",GRAFO::Gr().TotStazioniGrafo);

   for (ULONG i= 0;i < GRAFO::Gr().TotStazioniGrafo;i++ ) {
      STAZ_FS & Record = GRAFO::Gr()[i];
      if (i % NUMRIGHE_DUMP == 0) {
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
   MK2( Id                 , "5","u","Id","")
   MK2( StazioneFS         , "3","u","FS","")
   MK2( NumeroConcesse     , "4","u","Nø","Con")
   MK2( NumeroPrimiVicini  , "4","u","Nø","PV")
   MK2( NumeroPolimetriche , "4","u","Nø","Pol")
   MK2( IdRamo             , "5","u","Id","Ramo")
   MK2( DiDiramazione      , "4","u","Di","Dir")
   MK2( Nodale             , "4","u","Nod","")
   MK2( Terminale          , "4","u","Ter","")
   MK2( DiAllacciamento    , "4","u","All","")
   MK2( DiInstradamento    , "4","u","Ist","")
   MK2( DiTransito         , "4","u","Trn","")
   MK2( Vendibile          , "4","u","Vnd","")
   MK2( InstrGrafo         , "6","u","Istr","Grafo")
   MK2( Abilitazione       , "4","u","Abl","")
   MK2( CCR                , "6","u","CCR","")
   MK2( Km1                , "5","u","Km1","")
   MK2( ProgRamo           , "4","u","Prg","Ram")
   MK2( IsCnd              , "3","u","I","C")
   MK2( IdentIstr          , "7","s","Nome7","")
//----------------------------------------------------------------------------

      );
      fprintf(Out,"%s\n",Buf);
      // TRACESTRING(Buf);
      // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
   } /* endfor */

   fclose(Out);

   // TRACETERMINATE;
   
   exit(0);
//<<< void main(int argc,char *argv[]){
}

