#include "dummy.h"

#define  INCL_BASE

#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES

// begin EMS
// EMS ?C?C #include <os2.h>
#define INCL_DOSPROCESS
#include <windows.h>
typedef unsigned long  APIRET;
typedef unsigned long  LHANDLE;
typedef LHANDLE PID;            /* pid  */
typedef PID *PPID;
typedef LHANDLE TID;            /* tid  */
typedef TID *PTID;
#define BOOL32 BOOL
#define PBOOL32 PBOOL
typedef PVOID *PPVOID;
#define INCL_NOCOMMON
extern "C" {
#include <bsedos.h>
}
// end EMS

#include "ipcfscli.h"
#include <stdio.h>
#include <stddef.h>
#include <memory.h>


//#define IPCMUTEXWAIT SEM_INDEFINITE_WAIT
#define IPCMUTEXWAIT 60000
//#define IPCEVENTWAIT SEM_INDEFINITE_WAIT
#define IPCEVENTWAIT 30000


#define IPCFSNAME      "\\SHAREMEM\\IPCFS"
#define IPCFSMUXSEM    "\\SEM32\\IPCFSMUX"
#define IPCFSC2SSEM    "\\SEM32\\IPCFSC2S"
#define IPCFSS2CSEM    "\\SEM32\\IPCFSS2C"

#define ERR_THROW( err )  goto err;
#define ERR_CATCH( err )  err:


#define STS_SERVERWAITTRANSACTION  0
#define STS_SERVERINPROGRESS       1



#define CLIENT_CYCLES    200


char   *pIpcBuf = NULL;
ULONG  *pIpcLen = 0;
char * pIpcSts = NULL;

HEV  hevC2S  = (HEV)0;
HEV  hevS2C  = (HEV)0;
HMTX htmxCgi = (HMTX)0;


ULONG ipcClientBegin( )
{
 APIRET  rc;
 char   *p = NULL;
 int i;
 ULONG  ulRet;

 rc = DosOpenMutexSem( IPCFSMUXSEM, &htmxCgi );
 if ( ( rc != 0 ) )
 {
   ulRet = IPCFS_ERROR;
   ERR_THROW(ERR1);
 } /* endif */


 rc = DosRequestMutexSem( htmxCgi, IPCMUTEXWAIT );
 if ( ( rc != 0 ) && ( rc != 105 ) )
 {
   ulRet = IPCFS_ERROR;
   ERR_THROW(ERR2);
 } /* endif */


 rc = DosGetNamedSharedMem( (PPVOID)&p, IPCFSNAME, PAG_WRITE );
 if ( rc != 0 )
 {
   ulRet = IPCFS_SERVERNOTSTARTED;
   ERR_THROW(ERR3);
 } /* endif */


 rc = DosOpenEventSem( IPCFSC2SSEM, &hevC2S );
 if ( rc != 0 )
 {
   ulRet = IPCFS_ERROR;
   ERR_THROW(ERR4);
 } /* endif */


 rc = DosOpenEventSem( IPCFSS2CSEM, &hevS2C );
 if ( rc != 0 )
 {
   ulRet = IPCFS_ERROR;
   ERR_THROW(ERR5);
 } /* endif */


 pIpcSts = (char *)p;
 pIpcLen = (PULONG)((char *)p + 1);
 pIpcBuf = p + sizeof(ULONG) + sizeof(char);

// pIpcLen = (PULONG)p;
// pIpcBuf = p + sizeof(ULONG);
 *pIpcLen = 0L;

 for (i = 0; i < CLIENT_CYCLES; i++)
 {
   if (*pIpcSts != STS_SERVERWAITTRANSACTION)
   {
     DosSleep(50L);
   }
   else
   {
     i = 0;
     break;
   }
 }
 if (i == CLIENT_CYCLES)
 {
   ulRet = IPCFS_ERROR;
   ERR_THROW(ERR6);
 }



 return IPCFS_OK;

 ERR_CATCH(ERR6)
    DosCloseEventSem( hevS2C);
 ERR_CATCH(ERR5)
    DosCloseEventSem( hevC2S);
 ERR_CATCH(ERR4)
 ERR_CATCH(ERR3)
    DosReleaseMutexSem( htmxCgi );
 ERR_CATCH(ERR2)
    DosCloseMutexSem( htmxCgi );
 ERR_CATCH(ERR1)
  return  ulRet;
}



ULONG ipcClientFirstWrite( PCHAR pcBuf, ULONG ulBufLen )
{
 APIRET  rc;
 ULONG   ulCount;

 if (hevC2S == (HEV)0)
 {
   return IPCFS_CLIENTNOTSTARTED;
 } /* endif */




 /*
  *   reset this semaphore so that the server gets informed
  *   that it a new buffer is ready for it.
  */
#ifdef DEBUG
//   printf("Step 16\r\n");
#endif
 rc = DosResetEventSem( hevS2C, &ulCount );
 if ( rc == ERROR_INVALID_HANDLE)
 {
//   printf("error %X: ResetEvS2C\r\n", (ULONG)rc);
   return IPCFS_ERROR;
 } /* endif */


 memcpy( pIpcBuf, pcBuf, ulBufLen );
 *pIpcLen = ulBufLen;


 return IPCFS_OK;

}



ULONG ipcClientNextWrite( PCHAR pcBuf, ULONG ulBufLen )
{
 APIRET  rc;
 ULONG   ulCount;

 if (hevC2S == (HEV)0)
 {
    return IPCFS_CLIENTNOTSTARTED;
 } /* endif */


 memcpy( pIpcBuf + *pIpcLen, pcBuf, ulBufLen );
 *pIpcLen += ulBufLen;


 return IPCFS_OK;

}



ULONG ipcClientLastWrite( PCHAR pcBuf, ULONG ulBufLen )
{
 APIRET  rc;
 ULONG   ulCount;

 if (hevC2S == (HEV)0)
 {
   return IPCFS_CLIENTNOTSTARTED;
 } /* endif */


 memcpy( pIpcBuf + *pIpcLen, pcBuf, ulBufLen );
 *pIpcLen += ulBufLen;

 /*
  *   post this semaphore so that the server can inform us
  *   that it has consumed our buffer and a response is there
  */
#ifdef DEBUG
//   printf("Step 17\r\n");
#endif
 rc = DosPostEventSem( hevC2S );
 if ( rc != NO_ERROR)
 {
#ifdef DEBUG
//   printf("error %X: PostEvC2S\r\n", (ULONG)rc);
#endif
   return IPCFS_ERROR;
 } /* endif */

 return IPCFS_OK;

}





ULONG ipcClientWrite( PCHAR pcBuf, ULONG ulBufLen )
{
 APIRET  rc;
 ULONG   ulCount;

 if (hevC2S == (HEV)0)
 {
   return IPCFS_CLIENTNOTSTARTED;
 } /* endif */




 /*
  *   reset this semaphore so that the server gets informed
  *   that it a new buffer is ready for it.
  */
#ifdef DEBUG
//   printf("Step 6\r\n");
#endif
 rc = DosResetEventSem( hevS2C, &ulCount );
 if ( rc == ERROR_INVALID_HANDLE)
 {
#ifdef DEBUG
//   printf("error %X: ResetEvS2C\r\n", (ULONG)rc);
#endif
   return IPCFS_ERROR;
 } /* endif */


 memcpy( pIpcBuf, pcBuf, ulBufLen );
 *pIpcLen = ulBufLen;


 /*
  *   post this semaphore so that the server can inform us
  *   that it has consumed our buffer and a response is there
  */
#ifdef DEBUG
//   printf("Step 7\r\n");
#endif
 rc = DosPostEventSem( hevC2S );
 if ( rc != NO_ERROR)
 {
#ifdef DEBUG
//   printf("error %X: PostEvC2S\r\n", (ULONG)rc);
#endif
   return IPCFS_ERROR;
 } /* endif */

 DosSleep(10L);

 return IPCFS_OK;

}



ULONG ipcClientGetReadArea( PCHAR *ppcBuf, PULONG pulBufLen )
{

 APIRET  rc;

 /*
  *   wait on this semaphore for the server to produce
  *   a response for us.
  */
 rc = DosWaitEventSem( hevS2C, IPCEVENTWAIT );
 if ( rc != NO_ERROR)
 {
   return IPCFS_ERROR;
 } /* endif */

 *ppcBuf = pIpcBuf;
 *pulBufLen = *pIpcLen;

 return IPCFS_OK;

}







ULONG ipcClientRead( PCHAR pcBuf, PULONG pulBufLen )
{
 APIRET  rc;

 /*
  *   wait on this semaphore for the server to produce
  *   a response for us.
  */
#ifdef DEBUG
//   printf("Step 8\r\n");
#endif
 rc = DosWaitEventSem( hevS2C, IPCEVENTWAIT );
 if ( rc != NO_ERROR)
 {
#ifdef DEBUG
//   printf("error %X: WaitEvS2C\r\n", (ULONG)rc);
#endif
   return IPCFS_ERROR;
 } /* endif */

 memcpy( pcBuf, pIpcBuf, *pIpcLen );
 *pulBufLen = *pIpcLen;

 return IPCFS_OK;

}








ULONG ipcClientQuery( PULONG pulBufLen )
{
 APIRET  rc;

 if ( pIpcLen == NULL)
 {
   return IPCFS_CLIENTNOTSTARTED;
 } /* endif */

 rc = DosWaitEventSem( hevS2C, IPCEVENTWAIT );
 if ( rc != NO_ERROR)
 {
#ifdef DEBUG
//   printf("error %X: WaitEvS2C\r\n", (ULONG)rc);
#endif
   return IPCFS_ERROR;
 } /* endif */

 *pulBufLen = *pIpcLen;

 return IPCFS_OK;
}






ULONG ipcClientEnd( )
{

 DosCloseEventSem( hevS2C);
 DosCloseEventSem( hevC2S);
 DosReleaseMutexSem( htmxCgi );
 DosCloseMutexSem( htmxCgi );

 return 0;

}



