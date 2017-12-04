//----------------------------------------------------------------------------
// DU_CLUST: DUMP CLUSSTAZ.TMP
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

typedef unsigned long BOOL;

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ALFA0.HPP"
#include "ID_STAZI.HPP"

// struct CLUSTER_STAZIONE {
struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
// Qui va la definizione della struttura
   WORD   IdCluster              ; // KEY Id Cluster: Da 1 in poi
   WORD   IdStazione             ; // KEY Id stazione
   BIT   Id1                   :13;// Id prima stazione dominante
   BIT   Id2                   :13;// Id seconda stazione dominante
   BIT   TipoStazione          : 3;// 1 = Nodo di cambio
                                   // 2 = Nodo 
                                   // 4 = Stazione di instradamento eccezionale
                                   // e relative combinazioni binarie
   short  Distanza               ; // Distanza in Km dalla stazione dominante 1 del cluster
											  // E' calcolata dall' algoritmo di linearizzazione in ML_STEP2
	short DistanzaMediana    ;
   WORD   NumMzvFerm             ; // Numero di mezzi virtuali che FERMANO alla stazione nel cluster
											  // Puo' essere 0 se e' solo NODO di instradamento
   WORD   Prog                   ; // Progressivo stazione in ambito cluster 
                                   // Parte da 0
                                   // E' assegnato ordinando per:
                                   //    Nodo di cambio / Stazione normale
                                   //    Distanza 
   WORD   Prog2                  ; // Progressivo stazione in ambito cluster Parte da 0
                                   // E' assegnato ordinando per  Distanza 
   GRUPPO Gruppo                 ; // Gruppo di appartenenza della stazione (Nel cluster)
};

// Indice Ausiliario
// struct STAZIONE_CLUSTER {
struct MY_STRU2 {
   WORD    IdStazione             ; // KEY Id stazione
   WORD    IdCluster              ; // Id Cluster: Da 1 in poi   
};
#define NOME "CLUSSTAZ"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
#define NOME_IN        PATH_OUT NOME ".TMP"
#define NOME_IDX       PATH_OUT NOME ".IX1"
#define NOME_TRACE     NOME ".TRC"
#define BUF_DIM        128000
//----------------------------------------------------------------------------

class  MY_FILE : public FILE_FIX {
   public:
   MY_FILE():
	FILE_FIX(NOME_IN,sizeof(MY_STRU),BUF_DIM){};
   MY_STRU &  operator [](ULONG Indice){ Posiziona(Indice); return *(MY_STRU*) RecordC; };
};
class  MY_FILE2 : public FILE_FIX {
   public:
   MY_FILE2():
	FILE_FIX(NOME_IDX,sizeof(MY_STRU2),BUF_DIM){};
   MY_STRU2 &  operator [](ULONG Indice){ Posiziona(Indice); return *(MY_STRU2*) RecordC; };
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
   FILE * Out;
   Out = fopen(NOME_OUT,"wt");
   {
   printf("Analizzo i records di %s scrivendo il risultato su %s\n",NOME_IN,NOME_OUT);
   printf(" Usare lo switch /I per dumpare anche i dati da stazione a cluster\n");

   MY_FILE & InFile = * new MY_FILE();                                              
   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN  ";
   HDR2 = "      ";
   FMT  = "%-6u";

   STAZIONI Stazioni(PATH_DATI,"ID_STAZI.DB",0);
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
	// Es: MK1(IdStazioneDestinazione    ,"05","u","Tipo","Rel. ")
   // Es: MK1(NomeStazione              ,"36","s","Nomestazione","")
   MK1( IdCluster   , "8","u","IdClust","")
	MK1( IdStazione  , "8","u","IdStazi","")
   MK1( TipoStazione, "5","u","Tipo","Staz")
   MK1( Prog        , "5","u","Prog","")
   MK1( Prog2       , "5","u","Prog","2")
   MK1( Id1         , "6","u","Id1","")
   MK1( Id2         , "6","u","Id2","")
	MK1( Distanza    ,"10","i","Distanza","")
	MK1( DistanzaMediana    ,"10","i","DistanzaM","")
	MK1( NumMzvFerm  ,"10","u","NumMzvFer","")
   MK1( Gruppo      ,"40","s","Gruppo   ","")
	MK1( NomeStazione,"36","s","NomeStazione","")

//----------------------------------------------------------------------------

	// puts((CPSZ)HDR1);
	// puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);


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
   MK2( IdCluster   , "8","u","IdClust","")
   MK2( IdStazione  , "8","u","IdStazi","")
   MK2( TipoStazione, "5","u","Tipo","Staz")
   MK2( Prog        , "5","u","Prog","")
   MK2( Prog2       , "5","u","Prog","2")
   MK2( Id1         , "6","u","Id1","")
   MK2( Id2         , "6","u","Id2","")
	MK2( Distanza    ,"10","u","Distanza","")
	MK2( DistanzaMediana    ,"10","u","DistanzaM","")
	MK2( NumMzvFerm  ,"10","u","NumMzvFer","")
	,(CPSZ)((STRINGA)Record.Gruppo)
	,Stazioni[Record.IdStazione].NomeStazione
//	,Stazioni[(Record.IdStazione >= 0 ? Record.IdStazione].NomeStazione
//----------------------------------------------------------------------------

		);
		fprintf(Out,"%s\n",Buf);
		// TRACESTRING(Buf);
		// TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
	} /* endfor */

	delete &InFile;
	}

 // INDICE
	if(argc > 1 && ! stricmp(argv[1],"/I")) {
	printf("Analizzo i records dell' indice %s scrivendo il risultato su %s\n",NOME_IDX,NOME_OUT);
	MY_FILE2 & InFile = * new MY_FILE2();
	STRINGA HDR1,HDR2,FMT;
	HDR1 = "RecN  ";
	HDR2 = "      ";
	FMT  = "%-6u";

//----------------------------------------------------------------------------
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
	MK1( IdStazione  ," 8","u","IdStazi","")
	MK1( IdCluster   ," 8","u","IdClust","")

//----------------------------------------------------------------------------

	printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());

	for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
		MY_STRU2 & Record = InFile[i];
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
	MK2( IdStazione  ," 8","u","IdStazi","")
	MK2( IdCluster   ," 8","u","IdClust","")
//----------------------------------------------------------------------------

		);
		fprintf(Out,"%s\n",Buf);
		// TRACESTRING(Buf);
		// TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
	} /* endfor */

	delete &InFile;
	}

	fclose(Out);

	// TRACETERMINATE;

	exit(0);
//<<< void main(int argc,char *argv[]){
}

