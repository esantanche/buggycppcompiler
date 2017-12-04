#include "stdio.h"
#include "stdlib.h"
#include "conio.h"
// EMS VA #include "dos.h"
#include <oggetto.h>
#include <scandir.h>
// EMS ?C?C?C?C?C?C #include <glue.h>
#include "id_stazi.hpp"
#include "MOTGLUE.HPP"
#include "I_MOTORE.hpp"
#include "alfa0.hpp"
#include "myalloc.h"
#include "stdarg.h"


void AsEHtml( USHORT usErrIdet,
              USHORT usErrIn,  ELENCO_S StazioniPossibiliDa,
              USHORT usErrOut, ELENCO_S StazioniPossibiliA,
              USHORT usErrSol = 0 )
{

char bufint[20];


 char *p;


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 ipcServerFirstWrite( p = "2\001", strlen(p) );

 ipcServerNextWrite( p = "0\001", strlen(p) );                       // motore

 if (0 == usErrIdet) ipcServerNextWrite( p = "0\001", strlen(p) );   // richiesta
 else ipcServerNextWrite( p = "2\001", strlen(p) );                  // richiesta

 if (3 == usErrIn) ipcServerNextWrite( p = "1\001", strlen(p) );     // stazione_p
 else if (2 == usErrIn) ipcServerNextWrite( p = "2\001", strlen(p) );
 else ipcServerNextWrite( p = "0\001", strlen(p) );

 if (3 == usErrOut) ipcServerNextWrite( p = "1\001", strlen(p) );    // stazione_a
 else if (2 == usErrOut) ipcServerNextWrite( p = "2\001", strlen(p) );
 else ipcServerNextWrite( p = "0\001", strlen(p) );

 ipcServerNextWrite( p = "0\001", strlen(p) );                       // data

 ipcServerNextWrite( p = "0\001", strlen(p) );                       // ora

 if (0 == usErrSol) ipcServerNextWrite( p = "0\001", strlen(p) );    // soluzioni
 else ipcServerNextWrite( p = "1\001", strlen(p) );

 if (2 == usErrIn) {
    sprintf(bufint,"%d",StazioniPossibiliDa.Dim());
    ipcServerNextWrite( p = bufint, strlen(p) );
    ipcServerNextWrite( p = "\001", strlen(p) );
    for (int i = 0; i < StazioniPossibiliDa.Dim(); i++ )
    {
      char *pStaz = (char *)(CPSZ)StazioniPossibiliDa[i];
      ipcServerNextWrite( p = pStaz, strlen(p) );
      ipcServerNextWrite( p = "\001", strlen(p) );
    }
 }

 if (2 == usErrOut) {
    sprintf(bufint,"%d",StazioniPossibiliA.Dim());
    ipcServerNextWrite( p = bufint, strlen(p) );
    ipcServerNextWrite( p = "\001", strlen(p) );
    for (int i = 0; i < StazioniPossibiliA.Dim(); i++ )
    {
      char *pStaz = (char *)(CPSZ)StazioniPossibiliA[i];
      ipcServerNextWrite( p = pStaz, strlen(p) );
      ipcServerNextWrite( p = "\001", strlen(p) );
    }
 }

}

