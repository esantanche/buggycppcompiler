//
//   Project: SIPAX
//   File: BITTEST.CPP
//   Author: Ing. Emanuele Maria Santanché
//   Description: Test verifica baco compilatore

typedef unsigned long BOOL;

#include "stdio.h"
#include "conio.h"
#include "dos.h"
#include "mem.h"

FILE * Fprova;

#define TRACEBEGIN  \
	Fprova = fopen("BITTEST.TRC", "at"); \
	fprintf(Fprova,

#define TRACEEND  \
	);  \
	fclose(Fprova);

#undef TRCRTN
#define TRCRTN "Main"

#define BIT unsigned int

struct BIT_STRUCT1 {
	BIT        Id                  : 13  ;
	BIT        ClasseCoincidenza   :  1  ;
	BIT        FascePartenza       : 10  ;
	BIT        FasceArrivo         : 10  ;
	BIT        Citta               :  6  ;
	BIT        NumeroCollegamenti  : 10  ;
};

struct BIT_STRUCT2 {
	BIT        Id                  : 13  ;
	BIT        ClasseCoincidenza   :  1  ;
	BIT        FascePartenza       : 10  ;
	BIT        FasceArrivo         : 10  ;
	BIT        Citta               :  6  ;
	BIT		  Dummy               :  1  ;
	BIT        NumeroCollegamenti  : 10  ;
};


int main(int argc,char * argv[]) {

	int i;
	char * cErr = "*** ERRORE ***";
	char * cNoErr = "";
	char * cErrore_corrente;

	TRACEBEGIN "Inizio BITTEST.CPP\n" TRACEEND

	BIT_STRUCT1 bsProva1;

	TRACEBEGIN "Inizio test n.ro 1 struttura BIT_STRUCT1\n" TRACEEND

	TRACEBEGIN "sizeof(BIT_STRUCT1) = %i\n",sizeof(BIT_STRUCT1) TRACEEND

	memset(&bsProva1, 0L, sizeof(BIT_STRUCT1));

	for (i=0;i<32;i++) {
		bsProva1.NumeroCollegamenti = i;
		if (bsProva1.NumeroCollegamenti == i)
			cErrore_corrente = cNoErr;
		else
			cErrore_corrente = cErr;
		TRACEBEGIN "i = %5i   bsProva1.NumeroCollegamenti = %5i   %s\n",
				i,bsProva1.NumeroCollegamenti,cErrore_corrente TRACEEND
	}

	for (i=32;i<1024;i+=32) {
		bsProva1.NumeroCollegamenti = i;
		if (bsProva1.NumeroCollegamenti == i)
			cErrore_corrente = cNoErr;
		else
			cErrore_corrente = cErr;
		TRACEBEGIN "i = %5i   bsProva1.NumeroCollegamenti = %5i   %s\n",
				i,bsProva1.NumeroCollegamenti,cErrore_corrente TRACEEND
	}

	BIT_STRUCT2 bsProva2;

	TRACEBEGIN "Inizio test n.ro 2 struttura BIT_STRUCT2\n" TRACEEND

	TRACEBEGIN "sizeof(BIT_STRUCT2) = %i\n",sizeof(BIT_STRUCT2) TRACEEND

	memset(&bsProva2, 0L, sizeof(BIT_STRUCT2));

	for (i=0;i<32;i++) {
		bsProva2.NumeroCollegamenti = i;
		if (bsProva2.NumeroCollegamenti == i)
			cErrore_corrente = cNoErr;
		else
			cErrore_corrente = cErr;
		TRACEBEGIN "i = %5i   bsProva2.NumeroCollegamenti = %5i   %s\n",
				i,bsProva2.NumeroCollegamenti,cErrore_corrente TRACEEND
	}

	for (i=32;i<1024;i+=32) {
		bsProva2.NumeroCollegamenti = i;
		if (bsProva2.NumeroCollegamenti == i)
			cErrore_corrente = cNoErr;
		else
			cErrore_corrente = cErr;
		TRACEBEGIN "i = %5i   bsProva2.NumeroCollegamenti = %5i   %s\n",
				i,bsProva2.NumeroCollegamenti,cErrore_corrente TRACEEND
	}

	return 0;
//<<< void main int argc,char * argv
}

