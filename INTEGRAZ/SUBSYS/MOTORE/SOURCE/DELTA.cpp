//----------------------------------------------------------------------------
// Programma di confronto tra due outputs del motore
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "stdio.h"
#include <scandir.h>
#include <program.hpp>

#undef TRCRTN
#define TRCRTN "Delta"


int main(int argc,char * argv[]) {
   
   if(argc < 2){
      printf("Utilizzo: %s FileProve \n",argv[0]);
      printf("   Con FileProve che non deve avere estensione\n");
      printf("Oppure  : %s FileNew.Ext FileOld.Ext\n",argv[0]);
      printf("   La prima forma equivale a : %s FileProve.OUT FileProve.V1\n",argv[0]);
      return 99;
   };
   
   STRINGA F1,F2;
   if(argc < 3){
      F1 = STRINGA(argv[1]) + ".OUT";
      F2 = STRINGA(argv[1]) + ".V1";
   } else {
      F1 = STRINGA(argv[1]);
      F2 = STRINGA(argv[2]);
   } /* endif */
   
   if(!TestFileExistance(F1)){
      printf("Errore: non esiste il file %s\n",(CPSZ)F1);
      return 12;
   }
   if(!TestFileExistance(F2)){
      printf("Errore: non esiste il file %s\n",(CPSZ)F2);
      return 13;
   }
   
   printf("Paragone di due outputs del motore, su files %s e %s\n",(CPSZ)F1,(CPSZ)F2);
   printf("Ci puo' mettere un po' , aspettare con pazienza\n");
   printf("Inizio depurazione degli outputs dai tempi di esecuzione\n");
   
   {
      FILE * Nuovo;
      if((Nuovo=fopen("NUOVO.TMP","w"))==NULL){
         printf("Impossibile aprire il file %s","NUOVO.TMP");
         return 99;
      };
      FILE * Vecchio;
      if((Vecchio=fopen("VECCHIO.TMP","w"))==NULL){
         printf("Impossibile aprire il file %s","VECCHIO.TMP");
         return 99;
      };
      
      STRINGA Linea;
      ELENCO_S Ignora;
      Ignora += "Tempo di esecuzione";
      Ignora += "Tempo di Esecuzione";
      Ignora += "Tempo Totale";
      Ignora += "Profondit… ricerca";
      
      
      char Buf[512];
      FILE * f1;
      if((f1=fopen((CPSZ)F1,"r"))==NULL){
         printf("Impossibile aprire il file %s",(CPSZ)F1);
         return 99;
      };
      FILE * f2;
      if((f2=fopen((CPSZ)F2,"r"))==NULL){
         printf("Impossibile aprire il file %s",(CPSZ)F2);
         return 99;
      };
      
      while (fgets(Buf,sizeof(Buf),f1)){
         Linea = Buf;
         BOOL Stop = FALSE;
         ORD_FORALL(Ignora,i){
            if(Linea.Pos(Ignora[i]) >= 0){
               Stop = TRUE;
               break;
            }
         }
         if(Stop)continue;
         fprintf(Nuovo,"%s",(CPSZ)Linea);
      }
      while (fgets(Buf,sizeof(Buf),f2)){
         Linea = Buf;
         BOOL Stop = FALSE;
         ORD_FORALL(Ignora,i){
            if(Linea.Pos(Ignora[i]) >= 0){
               Stop = TRUE;
               break;
            }
         }
         if(Stop)continue;
         fprintf(Vecchio,"%s",(CPSZ)Linea);
      }
      fclose(f1);
      fclose(f2);
      fclose(Nuovo);
      fclose(Vecchio);
   }
   printf("Depurati i dati dai tempi di esecuzione, attivo il programma di confronto\n");
   ELENCO_S Comandi(
      "PROGRAMMA=GFC.EXE"                                    ,
      "COMANDO= Nuovo.tmp vecchio.tmp"                       ,
      "TITOLO=Paragone tra due output del motore"            ,
      "SESSIONE=SI"                                          ,
      "VISIBILE=MASSIMIZZATO"                                ,
      "DIPENDENTE=SI"                                        ,
      "WAIT=SI"
   );
   //ELENCO_S Comandi(
   //   "PROGRAMMA=SUPERC2.EXE"                                ,
   //   "COMANDO= Nuovo.tmp vecchio.tmp ROVR LTYP=L"           ,
   //   "TITOLO=Paragone tra due output del motore"            ,
   //   "SESSIONE=SI"                                          ,
   //   "VISIBILE=MASSIMIZZATO"                                ,
   //   "DIPENDENTE=SI"                                        ,
   //   "WAIT=SI"
   //);
   PROGRAMMA Pgm("Compare");
   Pgm.Run(FALSE,Comandi);
   remove("NUOVO.TMP"); // Camcello i temporanei
   remove("VECCHIO.TMP"); // Camcello i temporanei
   return 0;
//<<< int main int argc,char * argv     
}
