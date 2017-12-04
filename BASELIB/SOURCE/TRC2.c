
//----------------------------------------------------------------------------
// FILE TRC2.C
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

// EMS001
typedef unsigned long BOOL;

#ifndef OKTRACE
#define OKTRACE
#endif
#define NO_TRC_FOPEN // Per evitare chiamate ricorsive alla DosOpen_TRC ...
#define ANCHE_CALL_STACK  // Fa vedere la situazione dello stack su ogni riga di trace

// EMS002 12.5.97 sostituito globalmente HFILE in HFILEOS2

// EMS003 ridefinisco qui queste define che si trovano in bsedos.h ma che
// lì ho dovuto commentare perché definite anche in winbase.h
// Preferisco questa soluzione a quella di includere winbase.h perché
// quest'ultimo può interferire con altre definizioni in questo file
#define FILE_BEGIN      0x0000   /* Move relative to beginning of file */
#define FILE_CURRENT    0x0001   /* Move relative to current fptr position */
#define FILE_END        0x0002   /* Move relative to end of file */

//----------------------------------------------------------------------------
// Modulo dipendente da OS2
//----------------------------------------------------------------------------
#define MODULO_OS2_DIPENDENTE
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#define INCL_DOSDATETIME
#define INCL_DOSINFOSEG
#define INCL_DOSMEMMGR
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSSEMAPHORES

#define TRCEXTERN
// EMS004 VA la keyword _export viene scritta _Export dal comiplatore VisualAge
unsigned short _Export trchse;
unsigned short _Export trchmem;

#include "std.h"

#include <ctype.h>
#include <time.h>
// EMS005 VA non serve #include <scandir.h>
// #include <windows.h>   // EMS005 VA

// begin EMS006 VA

typedef unsigned long DWORD;

#define DECLSPEC_IMPORT __declspec(dllimport)

#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define WINAPIV     __cdecl
#define APIENTRY    WINAPI
#define APIPRIVATE  __stdcall
#define PASCAL      __stdcall
#else
#define CALLBACK
#define WINAPI
#define WINAPIV
#define APIENTRY    WINAPI
#define APIPRIVATE
#define PASCAL      pascal
#endif

#if !defined(_KERNEL32_)
#define WINBASEAPI DECLSPEC_IMPORT
#else
#define WINBASEAPI
#endif

typedef char * LPSTR;

#define GetCurrentDirectory GetCurrentDirectoryA

WINBASEAPI DWORD __stdcall GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer);

WINBASEAPI DWORD __stdcall GetLastError(VOID);

// end EMS006 VA

#include <myalloc.h>
#include <oggetto.h>

// Questa funzione decodifica gli RC per gli errori su file piu' comuni
// Formato " Rc = ... (ErroreInChiaroSeNoto)"
// Decodifica anche alcuni errori OS2 di altro tipo
char * _export DecodificaRcOS2(int Rc);

void TraceLogControl(); // Tronca il file di log se troppo grande

#define MSGSIZE 512
#define LENFILE 0L

static ULONG BytesCircolareAttivo = 0;
TRACEVARIABLES;

enum TIPI_APERTURA { Da_TRACEREGISTER, Da_TRACEDLL, Da_TRACEREGISTER_SCND };


static HFILEOS2 FileHandle=NULL;
static HFILEOS2  Lpt1FileHandle=NULL;

static BOOL   Lpt1Enabled=FALSE;
static USHORT Lpt1Tid;

static HFILEOS2  LogFileHandle=NULL;
static BOOL   LogEnabled=FALSE;
static USHORT LogTid;
static char Trch_Programma[100]={"PGMNONSO"};
static BOOL  ReqClose   = FALSE;
static TIPI_APERTURA TipoOpen = Da_TRACEREGISTER;

const char * TempoFile(const char * NomeFile);

static Conteggi[10];
void FineConta();

//----------------------------------------------------------------------------
// Montagna: supporto monitoring load delle DLL
//----------------------------------------------------------------------------
initzDLL::initzDLL(char *  NomeModulo){
};

//----------------------------------------------------------------------------
// Montagna: supporto monitoring del main (= Exe)
//----------------------------------------------------------------------------
void TRACEEXE(char *  Nome){
};

//----------------------------------------------------------------------------
// EMS007 VA pascal -> _Pascal
// EMSb001 tolgo completamente keyword _Pascal perché non compatibile con Win32s
void TrcAbnd (USHORT j){
   //---------------------------------------------------------------------------
   if(FileHandle){
      if(ReqClose){
         PrtText(" ---->","Chiusura effettiva del Trace");
      } else {
         PrtText(" ---->","TrcAbnd : Fine Trace (Su Abend ?)");
      };
      FineConta();
      Mem_Ctrl();
      trchse=0; // Disabilito il trace
      DosClose(FileHandle);
      FileHandle = NULL;
   };
   if(Lpt1FileHandle){
      DosClose(Lpt1FileHandle);
      Lpt1FileHandle = NULL;
   }
   if(LogFileHandle){
      DosClose(LogFileHandle);
      LogFileHandle = NULL;
   }
   DosExitList (MAKEULONG(3,0),NULL);
};

//----------------------------------------------------------------------------
void PrtInit (CPSZ Name){
   //----------------------------------------------------------------------------

   char *FileName= getenv("TRCFILE");

   if(TipoOpen == Da_TRACEREGISTER && FileName==NULL){
      if(trchse)PrtStop();        // Chiudo il trace se gia' attivo
      if(trchse)PrtStop();        // Per avere una chiusura effettiva
      return;
   };
   PrtOpen(Name,FileName);
};

//----------------------------------------------------------------------------
void PrtOpen (CPSZ Name,CPSZ  FileName){
   //----------------------------------------------------------------------------

   DATETIME Now;
   DosGetDateTime(&Now);

   Mem_Reset();

   /* begin EMS ?C?C?C?C?C temporaneo
   {
   char cFile_trace[50];
   sprintf(cFile_trace, "D:\\EMS\\TRACE\\%s.TRC", Name);
   FILE * Ftrace = fopen(cFile_trace,"w");
   if (Ftrace)
      fprintf(Ftrace,"T-INIZIO TRACE ***********************\n");
      fclose(Ftrace);
   }
   */ //end EMS

   if (TipoOpen != Da_TRACEREGISTER_SCND) {
      ULONG act;
      char File [80]; char * ext;
      // #BRD Tolte le dichiarative PLINFOSEG LS=NULL; e SEL selg,sell; che venivano
      // utilizzate sotto nella DosGetInfoBlock.

      if(trchse)PrtStop();        // Chiudo il trace se gia' attivo
      if(trchse)PrtStop();        // Per avere una chiusura effettiva

      char * Trl = getenv("TRACELEVEL");
      if( getenv("TRACECIRCOLARE"))BytesCircolareAttivo = atoi(getenv("TRACECIRCOLARE"));
      LONG TLev;
      char Buf[20];
      if(Trl == NULL){
         TLev = 1; // Minimo livello
      } else {
         TLev = strtol(Trl,(char **)&Buf,10);
      };

      strcpy(Trch_Programma,Name);
      trchse=TLev;

      strcpy(File,FileName);
      ext = File +strlen(File) -4;

      // begin EMS ?C?C?C  temporaneo
      /*
      if(DosOpen((PSZ)File,&FileHandle,&act , LENFILE ,0L,
               OPEN_ACTION_REPLACE_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW,
               OPEN_ACCESS_READWRITE |
               OPEN_SHARE_DENYWRITE  |
               OPEN_FLAGS_NO_LOCALITY
              , NULL )){
         trchse=0;
      };
      */
      // end EMS

//<<< if (TipoOpen != Da_TRACEREGISTER_SCND) {
   } else if(strcmp(Trch_Programma,Name)){
      PrtText(" ----> CAUSA APERTURA PRIMA PARTENZA DEL MAIN NON UTILIZZO IL FILE DI TRACE INDICATO = ",Name);
   } /* endif */

   switch (TipoOpen) {
   case Da_TRACEREGISTER:
      PrtText(" -----------------------------------------------------","");
      PrtText(" ---->",Trch_Programma);
      PrtText(" -----------------------------------------------------","");
      TipoOpen = Da_TRACEREGISTER_SCND;
      DosExitList (MAKEULONG(1,0), (PFNTHREAD)TrcAbnd);
      break;
   case Da_TRACEDLL:
      PrtText(" -----------------------------------------------------","");
      PrtText(" ---->  Apertura anticipata : Main non ancora partito. DLL = ",Trch_Programma);
      PrtText(" -----------------------------------------------------","");
      TipoOpen = Da_TRACEREGISTER_SCND;
      break;
   case Da_TRACEREGISTER_SCND:
      strcpy(Trch_Programma,Name);
      PrtText(" -----------------------------------------------------","");
      PrtText(" ----> PARTENZA DEL MAIN       ",Trch_Programma);
      PrtText(" -----------------------------------------------------","");
      DosExitList (MAKEULONG(1,0), (PFNTHREAD)TrcAbnd);
      break;
//<<< switch (TipoOpen) {
   } /* endswitch */

   char MsgStart[100];
   sprintf(MsgStart,"Inizio Trace : %i/%i/%i %i:%i %i.%i\r\n", (int)Now.day, (int)Now.month,
         (int)Now.year, (int)Now.hours, (int)Now.minutes, (int)Now.seconds, (int)Now.hundredths );

   PrtLong(" ---->","Livello di trace : ",trchse);
   PrtText(" ---->",MsgStart);
   TraceLogControl(); // Tronca il file di log se troppo grande
//<<< void PrtOpen (CPSZ Name,CPSZ  FileName){
};

   //----------------------------------------------------------------------------
   // Gestione dei pointers
   //----------------------------------------------------------------------------
void PrtPointer (CPSZ R,CPSZ  Messaggio, void * p) {
   char  Msg[512];
   sprintf(Msg,"%s %p (%lu)",Messaggio,p,p);
   PrtText(R,Msg);
};
void PrtPointer2(CPSZ R,CPSZ Messaggio,void * p,void * p2){
   char  Msg[512];
   sprintf(Msg,"%s %p %p",Messaggio,p,p2);
   PrtText(R,Msg);
};
void PrtPointer3(CPSZ R,CPSZ Messaggio,void * p,void * p2,void * p3){
   char  Msg[512];
   sprintf(Msg,"%s %p %p %p",Messaggio,p,p2,p3);
   PrtText(R,Msg);
};

   //----------------------------------------------------------------------------
void PrtStop (){
      //----------------------------------------------------------------------------
   if(!trchse)return;

   return; //EMS?C?C?C??C?C?C
   if(ReqClose){    // Seconda richiesta di chiusura: chiudo davvero
      PrtText(" ---->","Chiusura effettiva del Trace");
      FineConta();
      Mem_Ctrl();
      DosClose(FileHandle);
      FileHandle = NULL;
      trchse=0; // Disabilito il trace
      ReqClose = FALSE;
      DosExitList (MAKEULONG(2,0),NULL);
   } else {
      PrtText(" ---->","Richiesta Fine Trace (continuo per permettere DEBUG)");
      FineConta();
      Mem_Ctrl();
      ReqClose = TRUE;
   };

};

   //  *----------------------------------------------------------------*
void PrtPrty (CPSZ R,CPSZ  Messaggio) {
      //  *----------------------------------------------------------------*
   USHORT i_prty;
      //#BRD Vedi sotto
      //   ULONG  MemAv;
      //   ULONG  MemHeap;
      //   ULONG  MemStack;


      // #BRD Non esiste piu' la DosGetPrty bisogna usare la DosGetInfoBlock per
      // ottenere il livello di priorita' del thread lal posto di DosGetPrty(2,&i_prty,0);

   PPIB PS = NULL;
   PTIB TS = NULL;
   DosGetInfoBlocks (&TS, &PS);
   i_prty = TS->tib_ptib2->tib2_ulpri;

      // #BRD Eliminato DosMemAvail che non e' piu' supportato in OS/2 2.0
      //   DosMemAvail(&MemAv);
      //    MemHeap=(ULONG)_memavl();
      //    MemStack=(ULONG)stackavail();

   PrtText(R,Messaggio);
   PrtInt (R,"---> Priority ",i_prty);
      //   PrtLong(R,"---> Total Processor Memory available",MemAv);
      //    PrtLong(R,"---> Heap            Memory available",MemHeap);
      //    PrtLong(R,"---> Stack           Memory available",MemStack);

//<<< void PrtPrty (CPSZ R,CPSZ  Messaggio) {
};



   //  *----------------------------------------------------------------*
void PrtLong (CPSZ R,CPSZ  Messaggio, ULONG rc) {
      //  *----------------------------------------------------------------*

   char  C_rc[17],Msg[MSGSIZE];

   strcpy(Msg,Messaggio);
   strcat(Msg," ");
   ltoa(rc,C_rc,10);
   strcat(Msg,C_rc);
   strcat(Msg," (0x");
   ltoa(rc,C_rc,16);
   strcat(Msg,C_rc);
   strcat(Msg,")");

   PrtText(R,(CPSZ )Msg);
}

   //  *----------------------------------------------------------------*
void PrtInt (CPSZ R,CPSZ  Messaggio,int rc) {
      //  *----------------------------------------------------------------*

   char  C_rc[17],Msg[MSGSIZE];

   strcpy(Msg,Messaggio);
   strcat(Msg," ");
   itoa(rc,C_rc,10);
   strcat(Msg,C_rc);
   strcat(Msg," (0x");
   itoa(rc,C_rc,16);
   strcat(Msg,C_rc);
   strcat(Msg,")");

   PrtText(R,(CPSZ )Msg);
}
   //  *----------------------------------------------------------------*
void PrtText2(CPSZ R,CPSZ Messaggio, CPSZ s1) {
      //  *----------------------------------------------------------------*

   char  Msg[MSGSIZE];
   sprintf(Msg,"%s '%s' ",Messaggio,s1);
   PrtText(R,(CPSZ )Msg);
};
   //  *----------------------------------------------------------------*
void PrtText3(CPSZ R,CPSZ Messaggio, CPSZ s1,CPSZ s2) {
      //  *----------------------------------------------------------------*

   char  Msg[MSGSIZE];
   sprintf(Msg,"%s '%s' '%s' ",Messaggio,s1,s2);
   PrtText(R,(CPSZ )Msg);
};
   //  *----------------------------------------------------------------*
void PrtText4(CPSZ R,CPSZ Messaggio, CPSZ s1,CPSZ s2,CPSZ s3) {
      //  *----------------------------------------------------------------*

   char  Msg[MSGSIZE];
   sprintf(Msg,"%s '%s' '%s' '%s' ",Messaggio,s1,s2,s3);
   PrtText(R,(CPSZ )Msg);
};

//  *----------------------------------------------------------------*
//  *----------------------------------------------------------------*
void PrtText5(CPSZ R,CPSZ Messaggio, CPSZ s1,CPSZ s2,CPSZ s3,CPSZ s4) {

   char  Msg[MSGSIZE];
   sprintf(Msg,"%s '%s' '%s' '%s' '%s' ",Messaggio,s1,s2,s3,s4);
   PrtText(R,(CPSZ)Msg);
};


//  *----------------------------------------------------------------*
// PrtText
//  *----------------------------------------------------------------*
void PrtText (CPSZ R,CPSZ Messaggio) {
      #define TRIGGER 100L   // Delta time per lasciare una riga bianca
   char  Msg[MSGSIZE],CentSec[25],CentSec2[25];

   static ULONG RefTime=0;
   static ULONG LastTime=0;
   static BOOL  Ora_Formattata = getenv("ORA_FORMATTATA") != NULL;
   union TME {
      char  Fmt[12];
      struct {
         USHORT HH;
         char Fill1;
         USHORT MM;
         char Fill2;
         USHORT SS;
         char Fill3;
         USHORT CC;
         char EndString;
      }hhmm;
   };
   static TME Tme;
   static USHORT Decod100[100];

   int BytesWritten,len,i;

   // begin EMS ?C?C?C?C?C temporaneo
   {
   char cFile_trace[50];
   sprintf(cFile_trace, "D:\\EMS\\TRACE\\%s.TRC", Trch_Programma);
   FILE * Ftrace = fopen(cFile_trace,"a");
   if (Ftrace)
      if (((unsigned int)R) < 10 && ((unsigned int)Messaggio) < 10)
         fprintf(Ftrace, "T-??\n");
      else
      if (((unsigned int)R) < 10 && ((unsigned int)Messaggio) > 10)
         fprintf(Ftrace, "T-?-%s\n",Messaggio);
      else
      if (((unsigned int)R) > 10 && ((unsigned int)Messaggio) < 10)
         fprintf(Ftrace, "T-%s-?\n",R,Messaggio);
      else
         fprintf(Ftrace, "T-%s-%s\n",R,Messaggio);
      fclose(Ftrace);
   }
   return;
   // end EMS

   if (trchse == 0 || (FileHandle == NULL && Lpt1FileHandle == NULL && LogFileHandle == NULL ))return;

   ULONG OraCorrente = TimePart();  // Ora corrente in centesimi di secondo dalla partenza del programma

   if (!Ora_Formattata) {
      ltoa(OraCorrente - RefTime, CentSec, 10);
      strcat(CentSec,"0");
   } else {
      if(OraCorrente == 0) {
         for (i = 0;i < 100 ;i++ ) {
            Decod100[i] = 256*('0'+(i%10))+('0'+(i/10)); // Carico la tabella di decodifica
         } /* endfor */
         memset(&Tme,'*',sizeof(Tme));
         Tme.hhmm.Fill1 = ':';
         Tme.hhmm.Fill2 = ':';
         Tme.hhmm.Fill3 = '.';
         Tme.hhmm.EndString = 0;
      };
      if(OraCorrente == 0 || OraCorrente != LastTime ) {
         DATETIME Now;
         DosGetDateTime(&Now);
         Tme.hhmm.HH= Decod100[Now.hours];
         Tme.hhmm.MM= Decod100[Now.minutes];
         Tme.hhmm.SS= Decod100[Now.seconds];
         Tme.hhmm.CC= Decod100[Now.hundredths];
      }
      strcpy(CentSec,Tme.Fmt);
   } /* endif */

   if(R==NULL) {
      // Se messaggio e' anch' esso NULL vuol dire che ho chiesto un reset, altrimenti
      // ho chiesto un restore
      RefTime = (Messaggio == NULL ? OraCorrente : 0 );
      return;
   };

      // Dopo TRIGGER centesimi di secondo senza TRACE lascio una riga bianca
   if(OraCorrente - LastTime > TRIGGER)
      { // Lascio una riga bianca
      strcpy(Msg,"\r\n");
   } else {
      strcpy(Msg,"");
   };
   LastTime = OraCorrente;
   strcat(Msg,Trch_Programma);
   strcat(Msg," ");
   strncat(Msg,CentSec,25);
   strcat(Msg,"-");
   strcat(Msg," ");
   strcat(Msg,R);
   strcat(Msg,": ");
   int BytesTestata = strlen(Msg);
   if(Messaggio != NULL) {
      strncat(Msg,Messaggio,220);
   } else {
      if(Lpt1FileHandle){
         strcat(Msg,"Programma: ");
         strcat(Msg,Trch_Programma);
      } else{
         strcat(Msg,"(null)");
      }
   }
   Msg[252]=0;
   strcat(Msg,"\r\n");
   len=strlen(Msg);

   if( FileHandle && BytesCircolareAttivo ) {
      // Gestione del file di trace "Circolare"
      ULONG   NewPointer;
      int rc = DosSetFilePtr(FileHandle, 0 , FILE_CURRENT, &NewPointer);
      if( (rc == 0) && (NewPointer > BytesCircolareAttivo) ) {
         // Lascio meta' del file o 150K, a seconda di quale e' piu' piccolo
         ULONG   BytesRead = 150000;
         if( BytesCircolareAttivo < 2*BytesRead  ) BytesRead = BytesCircolareAttivo / 2;
         char * Buffer = (char*) malloc(BytesRead );
         DosSetFilePtr(FileHandle, -BytesRead , FILE_CURRENT, &NewPointer);
         rc = DosRead ( FileHandle,  Buffer,  BytesRead ,&BytesRead);
         if ((rc == 0) || (rc == ERROR_MORE_DATA) ) {
            PrtClear();
            rc = DosWrite ( FileHandle,  Buffer,  BytesRead, &BytesRead);
         } else {
            // Riposiziono all' inizio e gestisco veramente in modo circolare
            DosSetFilePtr(FileHandle, 0 , FILE_BEGIN, &NewPointer);
         } /* endif */
         free(Buffer);
      }
   }

   if(FileHandle) DosWrite (FileHandle, Msg, len, (ULONG *)&BytesWritten);
   // begin EMS ?C?C?C?C?C temporaneo
   {
   FILE * Ftrace = fopen("D:\\EMS\\DAEMON\\EXE\\TRACEDAE.TRC","a");
   if (Ftrace)
      fprintf(Ftrace,"T2-%s",Msg);
      fclose(Ftrace);
   }
   // end EMS
   if(Lpt1FileHandle && Lpt1Enabled ){
      for (int NumSkipLpt1 = BytesTestata; NumSkipLpt1 < len  ; NumSkipLpt1+= 78 ) {
         if(len-NumSkipLpt1 > 78){
            DosWrite (Lpt1FileHandle, Msg+NumSkipLpt1, 78, (ULONG *)&BytesWritten);
            DosWrite (Lpt1FileHandle, "\r\n", 2 , (ULONG *)&BytesWritten);
         } else {
            DosWrite (Lpt1FileHandle, Msg+NumSkipLpt1, len-NumSkipLpt1, (ULONG *)&BytesWritten);
         }
      } /* endfor */
   };
   if(LogFileHandle && LogEnabled ){
      DosWrite (LogFileHandle, Msg+BytesTestata, len-BytesTestata, (ULONG *)&BytesWritten);
   };
   return;
//<<< void PrtText (CPSZ R,CPSZ Messaggio) {
}
//  *----------------------------------------------------------------*
// PrtClircular
//  *----------------------------------------------------------------*
void PrtCircular(ULONG Size)
{
   BytesCircolareAttivo = Size;
}

//  *----------------------------------------------------------------*
// PrtClear
//  *----------------------------------------------------------------*
void PrtClear(){
   // EMS ?C?C?C?C?C?C togliere
   return;
   DosEnterCritSec();
   if(FileHandle) DosSetFileSize(FileHandle, 0); // Clear del file
   ULONG   NewPointer;
   DosSetFilePtr(FileHandle, 0 , FILE_BEGIN, &NewPointer);
   DosExitCritSec();
};

void cdecl PrtParms(CPSZ R,int argc,PSZ  argv[]){

   // EMS008 VA introdotto cast (const char *)
   if (!strcmp((const char *)Trch_Programma,"PGMNONSO") || Trch_Programma[0] == 0) {
      // EMS008 VA introdotto cast (const char *)
      if(argc > 0)strcpy(Trch_Programma,(const char *) argv[0]);
   } /* endif */
   int i;
   PrtText (R,"=================================================================");
   PrtText (R,"===     PARAMETRI DI CHIAMATA DEL PROGRAMMA                   ===");
   PrtText (R,"=================================================================");
   PrtInt  (R,"Numero di parametri: ",argc);

   // Abilito automaticamente il trace per l' exe se non e' gia' impostato
   // EMS008 VA introdotto cast (char *)
   TRACEEXE((char *)argv[0]);

   // EMS008 VA introdotto cast (const char *)
   for (i=0;i<argc;i++)PrtText(R,(const char *)argv[i]);
   char CD[250];
   strcpy(CD,"Directory Corrente: ");
   char  * DirPath = CD + strlen(CD);
   ULONG  DriveNumber;
   ULONG  LogicalDriveMap;
   ULONG  DirPathLen = 248 - strlen(CD);

   // EMS009 VA cambio il sorgente a causa nella non disponibilità di una funzione
   // di libreria che mi permetta di implementare la DosQueryCurrentDir

#ifdef __WINDOWS__

   GetCurrentDirectory(DirPathLen,  //DWORD nBufferLength,// size, in characters, of directory buffer
                       DirPath      //LPTSTR lpBuffer // address of buffer for current directory
                       );

   APIRET aErrore = (GetLastError() & 0x0000FFFF);

   if (aErrore != NO_ERROR) {
      strcpy(DirPath, "NON TROVATA");
      ERRINT("GetCurrentDirectory",aErrore);
   }

#else

   DosQueryCurrentDisk(&DriveNumber,  &LogicalDriveMap);
   // EMS010 VA introdotto cast (PBYTE)
   DosQueryCurrentDir ( DriveNumber,  (PBYTE)(DirPath+2), &DirPathLen);
   DirPath[0]='A'+ DriveNumber -1;
   DirPath[1]='\\';

#endif

   PrtText (R,CD);
   PrtText (R,"=================================================================");
};

char * CtoS(char a) {static char b[2];b[1]='\0';b[0]=a;return b;};
//----------------------------------------------------------------------------
// PrtHex
//----------------------------------------------------------------------------
void PrtHex  (CPSZ R,CPSZ  Messaggio,CPSZ  data,unsigned short len){
      #define CHARPERRIGA 16
   char Buf[512];
   CPSZ ToHex="0123456789ABCDEF";
   USHORT NRiga,NTot;
   USHORT Col,Riga;
   char * P1;
   char * P2;
   char * PBUF;
   UCHAR C;

   if(data == NULL)return;

   if(Messaggio != NULL){
      sprintf(Buf,"DUMP di variabile esadecimale lunga %u: ",len);
      strcat(Buf,Messaggio);
      PrtPointer(R,Buf,(void*)data);
   };

   NTot=0;NRiga=(len+CHARPERRIGA-1)/CHARPERRIGA;

   for (Riga=0;Riga < NRiga;Riga++){
      memset(Buf,' ',sizeof(Buf));
      PBUF=Buf + sprintf(Buf,"%p",data+(Riga * CHARPERRIGA));
      *PBUF=' ';
      for (Col=0;Col < CHARPERRIGA; Col++){
         P1 = PBUF + (Col*3) + 2;
         C  = (UCHAR)data[NTot];
         *P1= ToHex[C/0x10];P1++;
         *P1= ToHex[C%0x10];
         P2 = PBUF + (CHARPERRIGA*3) + Col + 6;
         *P2= isprint(C) ? C : '.';
         if(++NTot>=len)break;
      };
      *(++P2)='\0';
      PrtText(R,Buf);
   };

//<<< void PrtHex  (CPSZ R,CPSZ  Messaggio,CPSZ  data,unsigned short len){
};


//----------------------------------------------------------------------------
// Questa funzioncina mi conteggia i passaggi fino a dieci punti nel programma
//----------------------------------------------------------------------------
void Conta(USHORT i){Conteggi[i]++;};
void FineConta(){
   for (int i = 0;i < 10 ;i++ ) {
      if(Conteggi[i]){
         ERRSTRING("Conteggi["+STRINGA(i)+"] = "+STRINGA(Conteggi[i]));
      };
   } /* endfor */
};

// Queste routines attivano la stampa del trace.
// Utili per raccogliere informazioni di abend e casi particolari
// Funzionano anche se il tracelevel e' 0.
void _export TracePrintEnable(int OpenTid){
   if(Lpt1FileHandle == NULL && OpenTid){
      Lpt1Tid = OpenTid;
      // Apro il file per leggere le caratteristiche ed i dati
      USHORT     rc;
      ULONG      ActionTaken;
      // EMS011 VA introdotto cast (PSZ)
      rc=DosOpen(
            (PSZ) "LPT1",                          // Nome del file
            &Lpt1FileHandle,                 // Handle se viene trovato
            &ActionTaken,                    // Indica l' azione fatta da OS2
            0L,                              // Create File Size
            FILE_NORMAL,                     // Attributi: non significativi
            OPEN_ACTION_OPEN_IF_EXISTS,      // Open Action
            OPEN_FLAGS_NO_CACHE |            // Open Flags
            OPEN_SHARE_DENYNONE,
            0L);                             // Reserved
   };
   Lpt1Enabled = TRUE;
};
void _export TracePrintDisable(BOOL Close){
   if(Close ){
      if(Lpt1FileHandle != NULL)DosClose(Lpt1FileHandle);
      Lpt1FileHandle = NULL;
      Lpt1Tid = 0;
   }
   Lpt1Enabled = FALSE;
};
// Queste routines attivano la scrittura sul LOG del trace
// Inoltre alzano il tracelevel se era a 0
// Utili per raccogliere informazioni di abend e casi particolari
static BOOL Settato_trchse = FALSE;
int TraceLogOpen(){
   #undef TRCRTN
   #define TRCRTN "TraceLog.."
   int rc = 0;
   if(LogFileHandle == NULL){
      // La directory sta nella variabile di environment "SIPAX_LOG_DIRECTORY"
      // Se non impostata il default e' "C:\"
      // Il nome del file e' sempre "PROBLEMI.LOG"
      STRINGA NomeFile = getenv("SIPAX_LOG_DIRECTORY");
      if(NomeFile == NUSTR)NomeFile = "C:\\";
      if(NomeFile.Last() != '\\' )NomeFile += "\\";
      NomeFile += "PROBLEMI.LOG";
      // Apro il file per leggere le caratteristiche ed i dati
      ULONG ActionTaken;
      // Provo ad aprire per sei secondi max (potrebbe essere in uso da altri processi)
      ULONG Initialt = TimePart();
      while (TimePart() < Initialt + 600){
         rc=DosOpen(
               (PSZ)(CPSZ)NomeFile,             // Nome del file
               &LogFileHandle,                  // Handle se viene trovato
               &ActionTaken,                    // Indica l' azione fatta da OS2
               0L,                              // Create File Size
               FILE_NORMAL,                     // Attributi: non significativi
               OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW,  // Open Action OS2
               OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYWRITE          ,  // Open Flags OS2
               0L);                             // Reserved
         if(rc == 0)break;
         DosSleep(100); // Aspetto un poco
      } /* endfor */
      if(rc){
         ERRSTRING("Errore aprendo "+NomeFile+DecodificaRcOS2(rc));
         LogFileHandle = NULL;
      } else {
         ULONG   NewPointer;
         DosSetFilePtr(LogFileHandle, 0 , FILE_END, &NewPointer);
      }
   }
   return rc;
};
// Questa routine tranca il file di log: e' chiamata all' apertura del trace o del lock monitor,
// ma non durante la gestione di un' eccezione (il che vuol dire che se non si usa mai
// il trace ne' dei semafori il log potrebbe aumentare senza fine: ma e' quasi impossibile).
void TraceLogControl(){
   if(STRINGA(getenv("PROBLEMI_LOG_CIRCOLARE"))=="NO")return;
   int rc = TraceLogOpen();
   if(rc == 0){
      // Se il file e' piu' grande di 250000 bytes lascio solo 100 k bytes
      ULONG   NewPointer;
      rc = DosSetFilePtr(LogFileHandle, 0 , FILE_CURRENT, &NewPointer);
      if( (rc == 0) && (NewPointer > 250000) ) {
         ULONG   BytesRead = 100000;
         char * Buffer = (char*) malloc(BytesRead);
         DosSetFilePtr(LogFileHandle, -BytesRead , FILE_CURRENT, &NewPointer);
         rc = DosRead ( LogFileHandle,  Buffer,  BytesRead, &BytesRead);
         if ((rc == 0) || (rc == ERROR_MORE_DATA) ) {
            DosSetFileSize(LogFileHandle, 0); // Clear del file
            DosSetFilePtr(LogFileHandle, 0 , FILE_BEGIN, &NewPointer);
            rc = DosWrite ( LogFileHandle,  Buffer,  BytesRead, &BytesRead);
         } else {
            // Cerco di gestirlo veramente in modo circolare riposizionandolo all' inizio
            DosSetFilePtr(LogFileHandle, 0 , FILE_BEGIN, &NewPointer);
         } /* endif */
         free(Buffer);
      }
      if(LogFileHandle != NULL)DosClose(LogFileHandle);
      LogFileHandle = NULL;
   }
};
void _export TraceLogEnable(int OpenTid){
   if (trchse == 0) {
      Settato_trchse = TRUE;
      trchse = 1;
   } /* endif */
   if(LogFileHandle == NULL && OpenTid){
      LogTid = OpenTid;
      int rc = TraceLogOpen();
      if(rc == 0){
          // Scrivo la data
          DATETIME Now;
          DosGetDateTime(&Now);
          char Buffer[100];
          ULONG  BytesRead = sprintf(Buffer,"\r\n\r\nInizio LOGGING alle : %i/%i/%i %i:%i %i.%i Programma: %s\r\n", (int)Now.day, (int)Now.month, (int)Now.year, (int)Now.hours, (int)Now.minutes, (int)Now.seconds, (int)Now.hundredths,Trch_Programma );
          rc = DosWrite ( LogFileHandle,  Buffer,  BytesRead, &BytesRead);
      }
   };
   LogEnabled = LogFileHandle != NULL;
};
void _export TraceLogDisable(BOOL Close){
   if (Settato_trchse) {
      Settato_trchse = FALSE;
      trchse = 0;
   } /* endif */
   if(Close ){
      if(LogFileHandle != NULL)DosClose(LogFileHandle);
      LogFileHandle = NULL;
      LogTid = 0;
   }
   LogEnabled = FALSE;
};

void TraceCall(      // Chiedo di fare un trace del CALL_STACK
   const char* Rtn,  // Routine chiamante
   const char* Msg,  // Messaggio da mostrare
   int Depth ,       // Profondita' di chiamate da mostrare
   USHORT TrcLev ){
};

// ---------------------------------------------------------------
// Aggiunte per compatibilita… con OOLIB
// ---------------------------------------------------------------
void _export StartMot(){ };
APIRET APIENTRY  DosRequestMutexSem_TRC (ULONG hmtx, ULONG ulTimeout){
   APIRET Rc = DosRequestMutexSem (hmtx, ulTimeout);
   return Rc;
};
