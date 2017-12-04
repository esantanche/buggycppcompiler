//----------------------------------------------------------------------------
// DU_COLL2: DUMP COLLCLUS.TMP
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
// Qui va la definizione della struttura
   ID   StazionePartenza   ; // Key
   ID   StazioneArrivo     ; // Key
   ID   IdCluster          ; // Key
   BYTE Concorde           ; // Indica se il collegamento e' concorde o discorde con il cluster
   WORD Count              ; // Numero dei treni che realizzano il collegamento
   WORD OrariPartenza      ; 
   WORD OrariArrivo        ; 
   WORD OrariArrivo2       ;
};
#define NOME "COLLCLUS"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
#define NOME_IN        PATH_OUT NOME ".TMP"
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

   if(argc < 2){
      printf("Utilizzo: DU_COLL2 Stazione [... Stazione]");
      printf(" DU_COLL2 0: Dump dei collegamenti di TUTTE le stazioni di cambio con ALTRE stazioni di cambio ");
      return 99;
   };

   printf("Analizzo i records di %s scrivendo il risultato su %s\n",NOME_IN,NOME_OUT);

   ARRAY_ID Stzs;
   for (int a = 1;a < argc ;a ++ ) {
      STRINGA Tmp(argv[a]);
      Stzs += Tmp.ToInt();
   } /* endfor */
                                                                                    
   MY_FILE & InFile = * new MY_FILE();                                              
   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN   ";
   HDR2 = "       ";
   FMT  = "%-7u";
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
   MK1( StazionePartenza , "8","u","Id1","")
   MK1( StazioneArrivo   , "8","u","Id2","")
   MK1( IdCluster        , "8","u","Id ","Cluster")
   MK1( Concorde         , "8","u","Concor.",""   )
   MK1( Count            , "8","u","Count","")
   MK1( OrariPartenza    , "6","X","Ore","Part")
   MK1( OrariArrivo      , "6","X","Ore","Arr")
   MK1( OrariArrivo2     , "6","X","Ore","Arr2")

//----------------------------------------------------------------------------

   // puts((CPSZ)HDR1);
   // puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);

   FILE * Out;
   Out = fopen(NOME_OUT,"wt");

   F_STAZIONE_MV  Fstaz(PATH_OUT "MY_STAZI.TMP");

   printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());

   int j = 0;
   for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
      MY_STRU & Record = InFile[i];
      if(Stzs[0] == 0){
         if(!Fstaz[Record.StazionePartenza].StazioneDiCambio() || !Fstaz[Record.StazioneArrivo].StazioneDiCambio())continue;
      } else {
         if(!(Stzs.Contiene(Record.StazionePartenza) ||Stzs.Contiene(Record.StazioneArrivo)))continue;
      } /* endif */
      if (j % NUMRIGHE_DUMP == 0) {
         if(j != 0)fprintf(Out,"\n");
         fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
         // TRACESTRING(HDR1);
         // TRACESTRING(HDR2);
         j++;
      } /* endif */
      char Buf[500];
      sprintf(Buf,(CPSZ)FMT,i
//----------------------------------------------------------------------------
// Parte specifica: Macro definizione variabili da stampare
//----------------------------------------------------------------------------
   MK2( StazionePartenza , "8","u","Id1","")
   MK2( StazioneArrivo   , "8","u","Id2","")
   MK2( IdCluster        , "8","u","Id ","Cluster")
   MK2( Concorde         , "8","u","Concor.",""   )
   MK2( Count            , "8","u","Count","")
   MK2( OrariPartenza    , "6","X","Ore","Part")
   MK2( OrariArrivo      , "6","X","Ore","Arr")
   MK2( OrariArrivo2     , "6","X","Ore","Arr2")
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