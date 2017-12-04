#include "dummy.h"
//----------------------------------------------------------------------------
// FILE MYALLOC.H
//----------------------------------------------------------------------------
// Routines utilizzate per tenere sotto controllo l' allocazione della memoria
// Compilando myalloc.C si sostituiscono le routines malloc,calloc,free,e realloc
// e si abilita il controllo dell' allocazione di memoria.
// Compilando Myalloc.NO si lascia la normale gestione delle malloc.
//----------------------------------------------------------------------------

#ifndef HO_MYALLOC_H
#define HO_MYALLOC_H                   // Indica che questo file e' stato incluso

#ifndef HO_STRINGA_H                // Includo sempre le librerie standard del progetto
#include <stringa.h>
#endif

void _export Mem_Reset();           // Tutte le malloc effettuate fino a questo momento possono NON
                                    // avere una free corrispondente senza che cio' comporti una segnalazione.

void _export Mem_Reset(void * x);   // L' indirizzo 'x' puo' NON avere una free corrispondente senza
                                    // che cio' comporti una segnalazione.

void _export Mem_Ctrl();            // Dump di tutte le malloc effettuate fino a quel momento senza
                                    // una corrispondente free.
                                    // le aree mostrate sono flaggate in modo che alla prossima chiamata
                                    // non vengano nuovamente mostrate.
                                    // VIENE IN OGNI CASO CHIAMATA ALLA FINE DEL PROGRAMMA


BOOL _export  TestAlloc(void * __block);
ULONG _export TestAllAlloc();

#ifndef TRCEXTERN
   extern unsigned short _export trchmem;
#endif

#define MEMIGNORE(_x) if(trchmem)Mem_Reset(_x)


#endif
