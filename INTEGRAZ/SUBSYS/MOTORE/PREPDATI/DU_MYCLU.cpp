//----------------------------------------------------------------------------
// DU_MYCLU: DUMP MY_CLUST.TMP
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"

// struct CLUSTER_MV {
struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
// Qui va la definizione della struttura
   BIT   Id                  :13; // Id Cluster: Da 1 in poi
   BIT   TipoCluster         : 4; // 0  = Non definito
                                  // 1  = A dominanza stazioni breve percorrenza (<  250 Km) 
                                  // 2  = A dominanza stazioni lunga Percorrenza (>= 250 Km) 
                                  // 3 - 9 : Riservati per linearizzazione dei clusters
                                  // 10 = Specializzato Multistazione
                                  // 11 = Traghetti varii
                                  // 12 = Esclusivamente da Carrozze o servizi diretti
                                  // 15 = Splittato (perche' raggiunto il massimo numero di MV o ...)
   BIT   Id1                 :13; // Id prima stazione dominante
   BIT   Id2                 :13; // Id seconda stazione dominante
   BIT   NumeroMezziVirtuali :10;
   BIT   NumeroMezziVirtualiC:10; // Numero mezzi virtuali Concordi
   BIT   ExtraMezziVirtualiC :10; // Numero records mezzi virtuali Concordi aggiunti per uniformare il periodo
   BIT   NumeroMezziVirtualiD:10; // Numero mezzi virtuali Discordi
   BIT   ExtraMezziVirtualiD :10; // Numero records mezzi virtuali Discordi aggiunti per uniformare il periodo
   BIT   NumeroNodiCambio    : 8; // Comprende: stazioni di cambio (debbono fermarvi treni)
   BIT   NumeroNodi          : 8; // Comprende: Stazioni di cambio e TUTTI i nodi e le stazioni di instradamento eccezionali del grafo
   BIT   NumeroStazioni      :10; // Comprende: Nodi di cambio e di instradamento, fermate
   WORD  NumElementiGruppi      ; // Numero di elementi dei gruppi del cluster
};
#define NOME "MY_CLUST"
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

   STAZIONI Stazioni(PATH_DATI);
                                                                                    
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
   MK1( Id                   , "8","u","Id    ","")
   MK1( TipoCluster          , "8","u","Tipo  ","Cluster")
   MK1( Id1                  , "6","u","Id1   ","")
   MK1( Id2                  , "6","u","Id2   ","")
   MK1( NumeroMezziVirtuali  ,"10","u","Mezzi","Virtuali")
   MK1( NumeroMezziVirtualiC , "5","u","MV","Conc")
   MK1( ExtraMezziVirtualiC  , "5","u","Xtra","Conc")
   MK1( NumeroMezziVirtualiD , "5","u","MV","Disc")
   MK1( ExtraMezziVirtualiD  , "5","u","Xtra","Disc")
   MK1( NumeroNodi           , "6","u","Nodi","")
   MK1( NumeroNodiCambio     , "7","u","Nodi","Cambio")
   MK1( NumeroStazioni       ,"10","u","Stazioni","")
   MK1( NumElementiGruppi    , "8","u","NumElem","Gruppi")
   MK1( Id1                  ,"36","s","Staz1 ","")
   MK1( Id2                  ,"36","s","Staz2 ","")

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

      char Staz1[50],Staz2[50];
      strcpy(Staz1,Stazioni[Record.Id1].NomeStazione);
      strcpy(Staz2,Stazioni[Record.Id2].NomeStazione);

      char Buf[500];
      sprintf(Buf,(CPSZ)FMT,i
//----------------------------------------------------------------------------
// Parte specifica: Macro definizione variabili da stampare
//----------------------------------------------------------------------------
   MK2( Id                   , "8","u","Id    ","")
   MK2( TipoCluster          , "8","u","Tipo  ","Cluster")
   MK2( Id1                  , "6","u","Id1   ","")
   MK2( Id2                  , "6","u","Id2   ","")
   MK2( NumeroMezziVirtuali  ,"10","u","Mezzi","Virtuali")
   MK2( NumeroMezziVirtualiC , "5","u","MV","Conc")
   MK2( ExtraMezziVirtualiC  , "5","u","Xtra","Conc")
   MK2( NumeroMezziVirtualiD , "5","u","MV","Disc")
   MK2( ExtraMezziVirtualiD  , "5","u","Xtra","Disc")
   MK2( NumeroNodi           , "6","u","Nodi","")
   MK2( NumeroNodiCambio     , "7","u","Nodi","Cambio")
   MK2( NumeroStazioni       ,"10","u","Stazioni","")
   MK2( NumElementiGruppi    , "8","u","NumElem","Gruppi")
   ,Staz1
   ,Staz2
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