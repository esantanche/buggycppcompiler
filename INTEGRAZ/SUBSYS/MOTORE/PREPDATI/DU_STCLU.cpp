//----------------------------------------------------------------------------
// DU_STCLU: DUMP MM_STCLU.DB 
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

struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
   BIT  Stazione       : 13  ; // Key
   BIT  IdCluster      : 11  ; // Key
   BYTE Progressivo          ; // Progressivo stazione nell' ambito del cluster
   GRUPPO Gruppi             ; // Gruppi della stazione nel cluster
   // Questi dati sono relativi al verso concorde (C) o discorde (D) : Si puo' individuare in base ai progressivi
   BIT  OrariPartenzaC : 10  ; // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
   BIT  OrariPartenzaD : 10  ; // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
   BIT  OrariArrivoC   : 10  ; // 10 BIT. I BITS non rappresentano l' ora di partenza ma
                               // gli orari di partenza VALIDI successivi agli orari di arrivo
                               // considerando 180 minuti di tempo massimo di attesa alla stazione.
   BIT  OrariArrivoD   : 10  ; // 10 BIT. I BITS non rappresentano l' ora di partenza ma ...
};
#define NOME "MM_STCLU"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
#define NOME_IN        PATH_OUT NOME ".DB"
#define NOME_TRACE     NOME ".TRC"
#define BUF_DIM        128000
//----------------------------------------------------------------------------

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
                                                                                    
int main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          

   // TRACEREGISTER2(NULL,NOME,NOME_TRACE);

   printf("Analizzo i records di %s scrivendo il risultato su %s\n",NOME_IN,NOME_OUT);

   MY_FILE & InFile = * new MY_FILE();                                              
   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN   ";
   HDR2 = "       ";
   FMT  = "%-7u";
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
   MK1( Stazione         , "8","u","Staz","")
   MK1( IdCluster        , "8","u","Cluster","")
   MK1( Progressivo      , "6","u","Prog","")
   MK1( Gruppi           ,"10","s","Gruppi","")
   MK1( OrariPartenzaC   , "8","X","Ore","PartC")
   MK1( OrariPartenzaD   , "8","X","Ore","PartD")
   MK1( OrariArrivoC     , "8","X","Ore","ArrC")
   MK1( OrariArrivoD     , "8","X","Ore","ArrD")
//----------------------------------------------------------------------------

   // puts((CPSZ)HDR1);
   // puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);

   FILE * Out;
   Out = fopen(NOME_OUT,"wt");

   printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());

   int j = 0;
   for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
      MY_STRU & Record = InFile[i];

      if (j % NUMRIGHE_DUMP == 0) {
         if(j != 0)fprintf(Out,"\n");
         fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
         // TRACESTRING(HDR1);
         // TRACESTRING(HDR2);
         j++;
      } /* endif */
      char Buf[500];
      sprintf(Buf,(CPSZ)FMT ,i
//----------------------------------------------------------------------------
// Parte specifica: Macro definizione variabili da stampare
//----------------------------------------------------------------------------
   MK2( Stazione         , "8","u","Staz","")
   MK2( IdCluster        , "8","u","Cluster","")
   MK2( Progressivo      , "6","u","Prog","")
   ,(CPSZ)((STRINGA)Record.Gruppi)
   MK2( OrariPartenzaC   , "8","X","Ore","PartC")
   MK2( OrariPartenzaD   , "8","X","Ore","PartD")
   MK2( OrariArrivoC     , "8","X","Ore","ArrC")
   MK2( OrariArrivoD     , "8","X","Ore","ArrD")
//----------------------------------------------------------------------------

      );
      fprintf(Out,"%s\n",Buf);
      // TRACESTRING(Buf);
      // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
   } /* endfor */

   delete &InFile;
   fclose(Out);

   // TRACETERMINATE;
   
   return 0;
//<<< void main(int argc,char *argv[]){
}

