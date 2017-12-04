//----------------------------------------------------------------------------
// DU_FERNU: DUMP M1_FERMV.TMP (Periodicit… non uniformi delle fermate) 
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4

#include "FT_PATHS.HPP"  // Path da utilizzare
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ML_IN.HPP"
#include "oggetto.h"
#include "ml_base.hpp"

struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
// Qui va la definizione della struttura
// struct PERIODICITA_FERMATA_VIRT {
   WORD MezzoVirtuale        ; // KEY: Mezzo virtuale
   WORD Progressivo          ; // KEY: Progressivo fermata (in ambito treno virtuale)
   T_PERIODICITA Periodicita ; // Periodicita' di fermata
};
#define NOME "M1_FERMV"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
#define NOME_IN_BSE    PATH_OUT NOME ".TM"
#define NOME_TRACE     NOME ".TRC"
#define BUF_DIM        128000
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

   printf("Utilizzo : %s [MinVirtuale [MaxVirtuale] ]\n"
      " /F0     : Dati come in uscita da FT_STEPB\n"
      " /F1     : Dati non Linearizzati \n"
      " /F2     : Dati Linearizzati\n"
     , argv[0]
   );
   
   int MinMezzo = 0;
   int MaxMezzo = 9999999;
   int Dettaglio = 2;
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

   // TRACEREGISTER2(NULL,NOME,NOME_TRACE);
   T_PERIODICITA::Init(PATH_IN,"DATE.T");

   printf("Analizzo i records di %s scrivendo il risultato su %s\n",(CPSZ)NOME_IN,NOME_OUT);
                                                                                    
   MY_FILE & InFile = * new MY_FILE();                                              
   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN ";                                                                  
   HDR2 = "     ";                                                                  
   FMT  = "%-5u";
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
   // Es: MK1(IdStazioneDestinazione    ,"05","u","Tipo","Rel. ")
   // Es: MK1(NomeStazione              ,"36","s","Nomestazione","")
   MK1(  ,"00","u","","")

//----------------------------------------------------------------------------

   // puts((CPSZ)HDR1);
   // puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);

   FILE * Out;
   Out = fopen(NOME_OUT,"wt");

   printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());

   PERIODICITA Last;
   WORD LastMv = 0;
   int LastIdx = 0;
   ELENCO_S Lst;
   for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
      MY_STRU & Record = InFile[i];
      // Controllo via Nø mezzo virtuale
      if(Record.MezzoVirtuale < MinMezzo)continue;
      if(Record.MezzoVirtuale > MaxMezzo)break;

      if (Record.MezzoVirtuale == LastMv) {
         if (Last == Record.Periodicita) {
            fprintf(Out,"Nø %i eguale\n",Record.Progressivo);
         } else {
            fprintf(Out,"............. MV %i Nø %i ....    CAMBIATA     ..........\n",Record.MezzoVirtuale,Record.Progressivo);
            if((i - LastIdx ) > 15){
            fprintf(Out,"    DA : \n");
            Lst = Last.Decod();
            ORD_FORALL(Lst,i){
               fprintf(Out,"%s\n",(CPSZ)Lst[i]);
            };
            fprintf(Out,"    A : \n");
            };
            Lst = Record.Periodicita.Decod();
            ORD_FORALL(Lst,j){
               fprintf(Out,"%s\n",(CPSZ)Lst[j]);
            };
            Last   = Record.Periodicita;
            LastIdx = i;
         } /* endif */
      } else {
         fprintf(Out,"============= MV %i Nø %i ===============================\n",Record.MezzoVirtuale,Record.Progressivo);
         Last   = Record.Periodicita;
         LastMv = Record.MezzoVirtuale;
         LastIdx = i;
         Lst = Record.Periodicita.Decod();
         ORD_FORALL(Lst,i){
            fprintf(Out,"%s\n",(CPSZ)Lst[i]);
         };
      } /* endif */

   } /* endfor */

   delete &InFile;
   fclose(Out);

   // TRACETERMINATE;
   
   exit(0);
//<<< void main(int argc,char *argv[]){
}

