#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

typedef unsigned long BOOL;

#include <oggetto.h>
#include "base.hpp"
#include "CHNTEMPL.HPP"
#include "mm_basic.hpp"

struct PROVA {
   char Key[8];
   int  Value ;
};

HASH<PROVA> Hash(4);

void PutHash(char * Key, int Val){
   PROVA * Prov = Hash.Cerca((PROVA *)Key,8);
   if ( Prov != NULL) {
      Bprintf("La chiave %8.8s esisteva gi… con valore %i new val = %i", Key,Prov->Value,Val);
      Prov->Value = Val;
   } else {
      Prov = Hash.Alloca();
      memcpy(Prov->Key,Key,8);
      Prov->Value = Val;
      Bprintf("La chiave %8.8s viene aggiunta con valore %i ", Key, Val);
      Hash.Metti(8);
   } /* endif */
}

int main(int argc, char *argv[], char *envp[]) {

	TRACEREGISTER2(NULL,"","TTT.TRC");


   PutHash("UNO*****", 1 );

	PutHash("DUE*****", 2 );

	PutHash("TRE*****", 3 );
	PutHash("QUATTRO*", 4 );
	PutHash("CINQUE**", 5 );
	PutHash("SEI*****", 6 );
	PutHash("SETTE***", 7 );
	PutHash("OTTO****", 8 );
	PutHash("NOVE****", 9 );
	PutHash("DIECI***", 10);
	PutHash("UNDICI**", 11);
	PutHash("DODICI**", 12);
	PutHash("CINQUE**",75 );
	PutHash("DIECI***",710);
	PutHash("DODICI**",712);
	PutHash("DUE*****",72 );
	PutHash("NOVE****",79 );
	PutHash("OTTO****",78 );
	PutHash("QUATTRO*",74 );
	PutHash("SEI*****",76 );
	PutHash("SETTE***",77 );
	PutHash("TRE*****",73 );
	PutHash("UNDICI**",711);

	PutHash("UNO*****",71 );

	PutHash("UNO*****", 1 );
	PutHash("DUE*****", 2 );
	PutHash("TRE*****", 3 );
	PutHash("QUATTRO*", 4 );
	PutHash("CINQUE**", 5 );
	PutHash("SEI*****", 6 );
	PutHash("SETTE***", 7 );
	PutHash("OTTO****", 8 );
	PutHash("NOVE****", 9 );
	PutHash("DIECI***", 10);
	PutHash("UNDICI**", 11);
	PutHash("DODICI**", 12);


	TRACETERMINATE;
	return 0;
}
