#include "dummy.h"

// EMS ?C?C?C #define INCL_BASE
#define INCL_DOSPROCESS

// begin EMS
#include <windows.h>
// EMS ?C?C? #include <os2.h>
//EMS
typedef unsigned long  APIRET;
typedef unsigned long  LHANDLE;
#define PBOOL32 PBOOL
typedef PVOID *PPVOID;
typedef LHANDLE PID;            /* pid  */
typedef LHANDLE TID;            /* tid  */
typedef PID *PPID;
typedef TID *PTID;

#define INCL_NOCOMMON
extern "C" {
#include <bsedos.h>
}
// end EMS

#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include "HTAPI.h"
#include "asfsdat8.h"
#include "asconfig.h"

//extern "C"
//{
#include "ipcfscli.h"
//}

RISPOSTA Engine_Request(char **, char *, ConfORARIO);
char *Output_and_Free(RISPOSTA, char*, char *, ConfORARIO);
void Write_Q5_Log(char *);



char *Search_Orario(char *query, ConfORARIO config)
{
  char *page;
  RISPOSTA risposta;
  char *req;

// Write_Q5_Log("ER");
   risposta = Engine_Request(&req,query,config);

   if (risposta.retcode == 2)
   {
// Write_Q5_Log("OF1");
      page = Output_and_Free(risposta,req,query,config);
      return page;
   }

   int iCount = 0;
   ULONG ulRc;
   do
   {
     ulRc = ipcClientBegin( );
     if ( ulRc == IPCFS_SERVERNOTSTARTED )
     {
       DosSleep(500L);
     }
   } while( (++iCount < 8) && (ulRc == IPCFS_SERVERNOTSTARTED) );
   if ( ulRc != IPCFS_OK )
   {
// Write_Q5_Log("OF2");
      risposta.retcode = 2;
      risposta.Dati.errori.motore = 1;
      page = Output_and_Free(risposta,req,query,config);
      return page;
   }

   ULONG Len;
   ipcClientWrite( req, (ULONG)strlen(req));

   char *shmPun;
   ulRc = ipcClientGetReadArea( &shmPun, &Len );
    //ulRc = IPCFS_ERROR;    // EMS prove ?C?C?C?C?C
   if (ulRc == IPCFS_ERROR)
    {
      ipcClientEnd( );
      risposta.retcode = 2;
      risposta.Dati.errori.motore = 1;
// Write_Q5_Log("OF3");
      page = Output_and_Free(risposta,req,query,config);
      return page;      // ERROR HTML message
    } /* endif */

   *(shmPun+Len)='\0';

   /*
   Attenzione: la variabile risposta deve essere quella
   proveniente da Check_Input()
   */

   // Write_Q5_Log("PR");
   Parse_Risposta(shmPun,&risposta);

   ipcClientEnd( );

// Write_Q5_Log("OF4");
      page = Output_and_Free(risposta,req,query,config);
      return page;

}
