//----------------------------------------------------------------------------
// DU_COLLS: DUMP ID_COLLS.TMP
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

typedef unsigned long BOOL;

#include "oggetto.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
//#include "FT_AUX.HPP"
//#include "ML_OUT.HPP"
#include "ML_WRK.HPP"
#include "elenco.h"
//#include "eventi.h"
#include "seq_proc.hpp"
#include "scandir.h"
#include "mm_basic.hpp"
#include <stdio.h>

/*
struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
	BIT  StazionePartenza : 13       ;
	BIT  StazioneArrivo   : 13       ;   // Per ogni coppia di stazioni si hanno due records (con partenza ed arrivo invertiti)
	BIT  PesoTratta       :  1       ;   // 1 Per i collegamenti normali, 0 se la tratta e' multistazione
	BIT  Count            :  5       ;   // Numero dei treni che realizzano il collegamento (max 31);
	BIT  KmMinimi         : 11       ;   // Km minimi necessari per il collegamento
	BIT  OrariPartenza    : NUM_FASCE;   // Ogni BIT rappresenta una fascia oraria
	BIT  OrariArrivo      : NUM_FASCE;   // Ogni BIT rappresenta una fascia oraria
// BIT  OrariArrivo2     : NUM_FASCE;   // Idem , ma considerando i tempi massimi di coincidenza
};
*/
struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
	BIT  StazionePartenza : 13       ;
	BIT  StazioneArrivo   : 13       ;   // Per ogni coppia di stazioni si hanno due records (con partenza ed arrivo invertiti)
	BIT  PesoTratta       :  1       ;   // 1 Per i collegamenti normali, 0 se la tratta e' multistazione
   BIT  Privilegiato     :  1       ;   // 1 = Stessa Stazione o simili (piazzale, civitavecchia e messina marittima ...)
	BIT  Count            :  5       ;   // Numero dei treni che realizzano il collegamento (max 31);
	BIT  KmMinimi         : 12       ;   // Km minimi necessari per il collegamento
	BIT  OrariPartenza    : 10       ;   // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
	BIT  OrariArrivo      : 10       ;   // 10 BIT. I BITS non rappresentano l' ora di arrivo ma
													 // gli orari di partenza VALIDI successivi agli orari di arrivo
													 // considerando 180 minuti di tempo massimo di attesa alla stazione.
	// Questi sono in forma estesa: 32 Fasce da 45 minuti: Rappresentano le ore reali di partenza ed arrivo
	DWORD Partenza32                 ;
	DWORD Arrivo32                   ;
	BIT   TcollMin        :  5       ;   // Tempo di collegamento minimo  (Multipli di 45 minuti max 24 ore)
	BYTE  TcollMax        :  5       ;   // Tempo di collegamento massimo (Multipli di 45 minuti max 24 ore)
};
#define NOME "MM_COLL"
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

   ARRAY_ID Stzs;
	for (int a = 1;a < argc ;a ++ ) {
		STRINGA Tmp(argv[a]);
      if(Tmp.ToInt())Stzs += Tmp.ToInt();
   } /* endfor */

   printf("Analizzo i records di %s scrivendo il risultato su %s\n",NOME_IN,NOME_OUT);
   printf("Parametri opzionali: stazioni specifiche da controllare\n");

   MY_FILE & InFile = * new MY_FILE();                                              
   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN   ";
   HDR2 = "       ";
   FMT  = "%-7u";
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
	MK1( StazionePartenza , "8","u","Id1","");
	MK1( StazioneArrivo   , "8","u","Id2","");
	MK1( PesoTratta       , "8","u","Peso","Tratta");
	MK1( Privilegiato     , "8","u","Priv","");
	MK1( Count            , "8","u","Count","");
	MK1( KmMinimi         , "8","u","Km","");
	MK1( OrariPartenza    , "6","X","Ore","Part");
	MK1( OrariArrivo      , "6","X","Ore","Arr");
	MK1( Partenza32       , "10", "X","Part","32");
	MK1( Arrivo32         , "10", "X","Arr","32");
	MK1( TcollMin         , "6","u","Tcoll","min");
	MK1( TcollMax         , "6","u","Tcoll","max");

// MK1( OrariArrivo2     , "6","X","Ore","Arr2")

	/*
	BIT  StazionePartenza : 13       ;
	BIT  StazioneArrivo   : 13       ;   // Per ogni coppia di stazioni si hanno due records (con partenza ed arrivo invertiti)
	BIT  PesoTratta       :  1       ;   // 1 Per i collegamenti normali, 0 se la tratta e' multistazione
	BIT  Privilegiato     :  1       ;   // 1 = Stessa Stazione o simili (piazzale, civitavecchia e messina marittima ...)
	BIT  Count            :  5       ;   // Numero dei treni che realizzano il collegamento (max 31);
	BIT  KmMinimi         : 12       ;   // Km minimi necessari per il collegamento
	BIT  OrariPartenza    : 10       ;   // 10 BIT, ognuno dei quali e' relativo ad una fascia oraria
	BIT  OrariArrivo      : 10       ;   // 10 BIT. I BITS non rappresentano l' ora di arrivo ma
													 // gli orari di partenza VALIDI successivi agli orari di arrivo
													 // considerando 180 minuti di tempo massimo di attesa alla stazione.
	// Questi sono in forma estesa: 32 Fasce da 45 minuti: Rappresentano le ore reali di partenza ed arrivo
	DWORD Partenza32                 ;
	DWORD Arrivo32                   ;
	BIT   TcollMin        :  5       ;   // Tempo di collegamento minimo  (Multipli di 45 minuti max 24 ore)
	BYTE  TcollMax        :  5       ;   // Tempo di collegamento massimo (Multipli di 45 minuti max 24 ore)
	*/
//};



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

		if(Stzs.Dim() && !(Stzs.Contiene(Record.StazionePartenza) ||Stzs.Contiene(Record.StazioneArrivo)))continue;

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
	MK2( StazionePartenza , "8","u","Id1","")
	MK2( StazioneArrivo   , "8","u","Id2","")
	MK2( PesoTratta       , "8","u","Peso","Tratta")
	MK2( Privilegiato     , "8","u","Priv","")
	MK2( Count            , "8","u","Count","")
	MK2( KmMinimi         , "8","u","Km","")
	MK2( OrariPartenza    , "6","X","Ore","Part")
	MK2( OrariArrivo      , "6","X","Ore","Arr")
	MK2( Partenza32       , "10", "X","Part","32")
	MK2( Arrivo32         , "10", "X","Arr","32")
	MK2( TcollMin         , "6","u","Tcoll","min")
	MK2( TcollMax         , "6","u","Tcoll","max")

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