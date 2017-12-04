//
//   Project: SIPAX
//   File: BITTEST.CPP
//   Author: Ing. Emanuele Maria Santanché
//   Description: Test verifica baco compilatore

typedef unsigned long BOOL;

//#ifdef __SW_BR
//#error definito swbr
//#endif
//#ifdef __NT__
//#error definito nt
//#endif

#include <stdio.h>   ///     *****
// #include "conio.h"    ///  *****
//  **** #include "dos.h"   
#include <mem.h>    /// ****
//  extern void *memset( void *__s, int __c, size_t __n ); //   *****

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

// BIT_STRUCT1 50 bit 6,25  bytes Borland C++ Builder  9 bytes
//                                Watcom               8 bytes
// BIT_STRUCT2 51 bit 6,375 bytes Borland C++ Builder 10 bytes
//                                Watcom               8 bytes

struct BIT_STRUCT1 {  // 50 bit = 6,25 bytes
        BIT        Id                  : 13  ;
        BIT        ClasseCoincidenza   :  1  ;
        BIT        FascePartenza       : 10  ;
        BIT        FasceArrivo         : 10  ;
        BIT        Citta               :  6  ;
        BIT        NumeroCollegamenti  : 10  ;
};

struct BIT_STRUCT2 {  // 51 bit = 6,375 bytes
        BIT        Id                  : 13  ;
        BIT        ClasseCoincidenza   :  1  ;
        BIT        FascePartenza       : 10  ;
        BIT        FasceArrivo         : 10  ;
        BIT        Citta               :  6  ;
        BIT        Dummy               :  1  ;
        BIT        NumeroCollegamenti  : 10  ;
};


int main(int argc,char * argv[]) {

        unsigned int i;
        char * cErr = "*** ERRORE ***";
        char * cNoErr = "";
        char * cErrore_corrente;

        printf("\nFUNZIONA???\n");

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

        TRACEBEGIN "Inizio test n.ro 3 overflow\n" TRACEEND     

        bsProva1.FasceArrivo = 0;
        bsProva1.NumeroCollegamenti = 0;

        for (i=0; i<1000; i++) {
                bsProva1.Citta = i;
                TRACEBEGIN "i = %4d bsProva1.Citta = %4d\n", i, bsProva1.Citta TRACEEND
                if (bsProva1.FasceArrivo != 0 ||
                    bsProva1.NumeroCollegamenti != 0) {
                   TRACEBEGIN "Errore di overflow\n" TRACEEND
                   TRACEBEGIN "   i = %d\n", i TRACEEND
                   TRACEBEGIN "   bsProva1.Citta = %d\n", bsProva1.Citta TRACEEND
                   TRACEBEGIN "   bsProva1.FasceArrivo = %d\n", bsProva1.FasceArrivo TRACEEND
                   TRACEBEGIN "   bsProva1.NumeroCollegamenti = %d\n", bsProva1.NumeroCollegamenti TRACEEND
                }
                if (i<64 && bsProva1.Citta != i) {
                   TRACEBEGIN "Errore di assegnazione a bsProva1.Citta\n" TRACEEND
                   TRACEBEGIN "   i = %d\n", i TRACEEND
                   TRACEBEGIN "   bsProva1.Citta = %d\n", bsProva1.Citta TRACEEND
                }
                if (i<60) {
                   bsProva1.Citta += 3;
                   if (bsProva1.Citta != i+3) {
                      TRACEBEGIN "Errore di assegnazione a bsProva1.Citta == i+3\n" TRACEEND
                      TRACEBEGIN "   i = %d\n", i TRACEEND
                      TRACEBEGIN "   bsProva1.Citta = %d\n", bsProva1.Citta TRACEEND
                   }            
                }
                if (i>4 && i<60) {
                   bsProva1.Citta -= 3;
                   if (bsProva1.Citta != i) {
                      TRACEBEGIN "Errore di assegnazione a bsProva1.Citta == i\n" TRACEEND
                      TRACEBEGIN "   i = %d\n", i TRACEEND
                      TRACEBEGIN "   bsProva1.Citta = %d\n", bsProva1.Citta TRACEEND
                   }            
                   bsProva1.Citta --;
                   if (bsProva1.Citta != i-1) {
                      TRACEBEGIN "Errore di assegnazione a bsProva1.Citta == i-1\n" TRACEEND
                      TRACEBEGIN "   i = %d\n", i TRACEEND
                      TRACEBEGIN "   bsProva1.Citta = %d\n", bsProva1.Citta TRACEEND
                   }            
                }
        }

        return 0;
//<<< void main int argc,char * argv
}

