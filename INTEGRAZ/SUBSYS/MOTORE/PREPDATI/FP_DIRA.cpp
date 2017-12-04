//----------------------------------------------------------------------------                             c
// FP_zero.CPP: Determinazione livelli diramazione da polimetriche di CIPRIANO
//----------------------------------------------------------------------------
// Il programma e' utilizzato nel primo rilascio, per convertire i dati
// digitalizzati delle polimetriche in una forma piu' "friendly" (che poi sara'
// quella utilizzata).
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "oggetto.h"
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "ALFA0.HPP"
#include "POLIM.HPP"
#include "seq_proc.HPP"
#include "FT_PATHS.HPP"  // Path da utilizzare
#include "scandir.h"

#define MAX_POLIMETRICA 1000
#define PGM      "FP_DIRA"

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
   
   
   
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   SetStatoProcesso(PGM, SP_PARTITO);
   
   
   GESTIONE_ECCEZIONI_ON
   
   STRINGA Linea;
   
   FILE_RO Poli(PATH_POLIM "POLIMETR.IBM");
   FILE_RW PoliOut(PATH_POLIM "POLIMETR.IN");

   int KmLast=0;
   int LastSecondaria = FALSE;
   int ThisSecondaria = FALSE;
   int LivelloSecondaria = 0;
   BOOL Go = FALSE;
   
   while(Poli.gets(Linea,4096)){
      LastSecondaria =  ThisSecondaria;
      if (Linea[0] == '1') {
         LastSecondaria = FALSE;
         LivelloSecondaria = 0;
         KmLast=0;
         Go = Linea[1] < '0' ||  Linea[1] >  '9';
      } else if (Go) {
         // Polimetrica di allacciamento o diramazione: ignorare
      } else if (Linea[0] == '2') {
         REC2 & Rec2 = *(REC2*)(CPSZ)Linea;
         ThisSecondaria = Rec2.TrattaSecondaria == '*';
         char * Distanze = (char*)(CPSZ)Linea + sizeof(REC2);
         int Km1 =  StringToInt(Distanze,4);
         if( ThisSecondaria ){
            if(!LastSecondaria || Km1 < KmLast)LivelloSecondaria ++;
            Rec2.TrattaSecondaria = '0' + LivelloSecondaria;
         }
         KmLast = Km1;
      }
      PoliOut.printf("%s\r\n",(CPSZ)Linea);
   } 
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TRACESTRING("Fine");
   
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
//<<< int main(int argc,char *argv[]){
}

