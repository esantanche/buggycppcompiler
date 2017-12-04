#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 1
// Le periodicita' di base sono mostrate a livello di trace 2


#include "STRINGA.H"

#define PGM      "TE"

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

void main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
   
   { // Per garantire la chiamata dei distruttori dei files
      
      TRACEREGISTER2(NULL,PGM, PGM ".TRC");
      TRACEPARAMS(argc,argv);
      TRACEEXE(argv[0]);

      STRINGA Uno("Prova1");
      char *Due = "Prova3 Questo va in abend ! tttttttttttttttttttttttttttttttttttttttttttttttttttt";
      strcpy((char*)(const char*)Uno,Due);
      printf("Uno = %s\n",(CPSZ)Uno);
      printf("Due = %s\n",Due);

      // ---------------------------------------------------------
      // Fine
      // ---------------------------------------------------------
      TRACETERMINATE;
      
   }
   
   exit(0);
}


