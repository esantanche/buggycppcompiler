//----------------------------------------------------------------------------
// DU_MYSTA: DUMP MY_STAZI.TMP
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "BASE.HPP"
#include "FILE_RW.HPP"

// struct STAZIONE_MV {
struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
// Qui va la definizione della struttura
   BIT   Id              :13; // Id stazione
   BIT   NumFermate      :13; // Numero di fermate = Peso della stazione
   BIT   NumTransiti     :13; // Numero di transiti
   BIT   NumClusters     :10; // Numero di clusters in relazione con la stazione
   BIT   TipoStazione    : 3; // ...
   BIT   ClasseCoincidenza:1; // Classe di coincidenza: 
                              // 0 =   Normale (10 Minuti )
                              // 1 =   Esteso  (15 Minuti )
   BIT   NumCollegamenti :11; // Numero di altre stazioni cui e' collegata
   BIT   NumCollCambio   :11; // Numero di stazioni di cambio cui e' collegata
};
#define NOME "MY_STAZI"
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
                                                                                    
void main(int argc,char *argv[]){                                                   
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          

   // TRACEREGISTER2(NULL,NOME,NOME_TRACE);

   printf("Analizzo i records di %s scrivendo il risultato su %s\n",NOME_IN,NOME_OUT);
                                                                                    
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
   MK1(Id                 , "8","u","Id","")
   MK1(NumFermate         , "8","u","Num"   ,"Fermate")
   MK1(NumTransiti        ,"10","u","Num"   ,"Transiti")
   MK1(NumClusters        ,"10","u","Num"   ,"Clusters")
   MK1(TipoStazione       ,"10","u","Tipo"  ,"Stazione")
   MK1(ClasseCoincidenza  ,"10","u","Classe","Coincid.")
   MK1(NumCollegamenti    , "8","u","Num"   ,"Coll"    )
   MK1(NumCollCambio      , "8","u","Num"   ,"CollCamb")

//----------------------------------------------------------------------------

   // puts((CPSZ)HDR1);
   // puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);

   FILE * Out;
   Out = fopen(NOME_OUT,"wt");
   printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());

   for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
      MY_STRU & Record = InFile[i];
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
   MK2(Id                 , "8","u","Id","")
   MK2(NumFermate         , "8","u","Num"   ,"Fermate")
   MK2(NumTransiti        ,"10","u","Num"   ,"Transiti")
   MK2(NumClusters        ,"10","u","Num"   ,"Clusters")
   MK2(TipoStazione       ,"10","u","Tipo"  ,"Stazione")
   MK2(ClasseCoincidenza  ,"10","u","Classe","Coincid.")
   MK2(NumCollegamenti    , "8","u","Num"   ,"Coll"    )
   MK2(NumCollCambio      , "8","u","Num"   ,"CollCamb")
//----------------------------------------------------------------------------

      );
      fprintf(Out,"%s\n",Buf);
      // TRACESTRING(Buf);
      // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
   } /* endfor */

   delete &InFile;
   fclose(Out);

   // TRACETERMINATE;
   
   exit(0);
//<<< void main(int argc,char *argv[]){
}

