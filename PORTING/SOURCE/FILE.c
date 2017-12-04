//
//   Project: SIPAX
//   File: FILE.C
//   Author: Ing. Emanuele Maria Santanché
//   Description: riscrittura delle API OS/2 di gestione files
//   per la piattaforma Windows (Win32s)
//   Note: se questo sorgente viene incluso in un progetto che
//   realizza un eseguibile, il main, prima di usarne le funzioni
//   deve chiamare la funzione Inizializza_utilizzo_modulo_FILE
//   Se questo sorgente viene usato per realizzare una DLL,
//   la funzione DllEntryPoint deve chiamare la funzione
//   Inizializza_utilizzo_modulo_FILE in corrispondenza dell'attachment
//   da parte di un processo
//   Va inoltre chiamata la funzione Concludi_utilizzo_modulo_FILE
//   al termine dell'eseguibile oppure su detach di un processo

///////////////////////////////////////////////////////
//  Scheletro di funzione DllEntryPoint da usare per
//  inizializzare questo modulo

/*
BOOL APIENTRY DllEntryPoint(HINSTANCE hinstDll, DWORD fdwReason,
                                                LPVOID lpvReserved)
{

   if (dwReason == DLL_PROCESS_ATTACH) {
      if (Inizializza_utilizzo_modulo_FILE() != NO_ERROR)
         return FALSE;
   }
   else
   if (dwReason == DLL_PROCESS_DETACH) {
      if (Concludi_utilizzo_modulo_FILE() != NO_ERROR)
         return FALSE;
   }
    .
    .
    .
}
*/

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3

#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_DOSMISC
#define INCL_DOSMODULEMGR
#define INCL_DOSPROFILE

#define ERROR_RICEVUTO_WM_QUIT 10000L

// EMS VA indrodotta define _cdecl
#define _cdecl
#include "windows.h"    //Windows

// definizioni prelevate da os2def.h che non è
// possibile includere tutto perché ci sono molte definizioni
// in conflitto con Windows (winbase.h)
typedef unsigned long  APIRET;
typedef unsigned long  LHANDLE;
typedef PVOID *PPVOID;
typedef LHANDLE PID;            /* pid  */
typedef LHANDLE TID;            /* tid  */
//typedef LHANDLE HMODULE;        /* hmod */
typedef HMODULE *PHMODULE;
typedef PID *PPID;
typedef TID *PTID;
typedef int ( APIENTRY _PFN)  ();
typedef _PFN *PFN;
typedef struct _QWORD          /* qword */
{
   ULONG   ulLo;
   ULONG   ulHi;
} QWORD;
typedef QWORD *PQWORD;
#define BOOL32 BOOL
#define PBOOL32 PBOOL
#define CPSZ const char *
// fine definizioni estratte da os2def.h
extern "C"
{
#include "bsedos.h"     //OS2  questo header è personalizzato per Windows
}
#include "bseerr.h"     //OS2
#include "stdio.h"
#include <stdlib.h>     // per _searchenv e _searchstr
// EMS VA non serve #include <dir.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <time.h> // EMS VA
#include <share.h>
//#include "std.h"
#include "trc2.h"

FILE * Fprova;
#define TRACERAPIDA(stringa)  \
   Fprova = fopen("PROVAMIf.TRC", "a"); \
   fprintf(Fprova, "%s\n", stringa);        \
   fclose(Fprova);
char cErrMsg[80];

#define TRACEBEGIN \
   Fprova = fopen("PROVAMIf.TRC", "a"); \
   fprintf(Fprova,

#define TRACEEND  );  \
   fclose(Fprova);

//?C mettere dei trace per dettagliare gli errori ??
//?C definire tutti i TRCRTN per tutte le funzioni
//?C scrivere testing effettuato per ogni funzione
//?C controllare tutti i commenti iniziali che dicono come ho fatto l'implementazione

///////////////////////////////////////////////////////////////
//     Definizioni

typedef struct _STRUTTURA_TLS {
   ULONG ulMaschera_filtro_attributi;
   TIB   tibInfo_thread;
   TIB2  tib2Info_specifiche_thread;
} STRUTTURA_TLS;

typedef STRUTTURA_TLS *LPSTRUTTURA_TLS;


//////////////////////////////////////////////////////
// Prototipi funzioni private

APIRET Ultimo_errore(void);
APIRET Cerca_file_seguenti(PVOID  pfindbuf,
                                               ULONG  cbBuf,
                                               LPWIN32_FIND_DATA pffdDati_file_trovato,
                                               BOOL fElaborare_primo_file_trovato,
                                               PULONG pulNumero_files_trovati,
                                               ULONG ulMaschera_filtro_attributi,
                                               HDIR hdir);

// I prototipi delle seguenti funzioni si trovano in bsedos.h
// (personalizzato per Windows)
// APIRET Inizializza_utilizzo_modulo_FILE(void);
// APIRET Concludi_utilizzo_modulo_FILE(void);

APIRET Salva_maschera_filtro_attributi_in_tls(ULONG ulMaschera_filtro_attributi);
APIRET Ottieni_maschera_filtro_attributi_da_tls(ULONG *ulMaschera_filtro_attributi);
APIRET Ottieni_puntatore_a_struttura_tls(LPSTRUTTURA_TLS *plpStruttura_tls);

// Fine Prototipi funzioni private
///////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
// Variabili private del modulo

DWORD dwIndice_tls = 0xFFFFFFFF;
ULONG ulNumero_processi_attaccati = 0;

/////////////////////////////////////////////////////////
//    DosGetDateTime
/////////////////////////////////////////////////////////

// La funzione ritorna un valore per il campo weekday di
// cui non è garantita la compatibilità del valore
// Il campo timezone non viene restituito
// Codici errore: ERROR_INVALID_DATA se pdt == NULL

APIRET APIENTRY DosGetDateTime(PDATETIME pdt)
{

   #undef TRCRTN
   #define TRCRTN "DosGetDateTime"

   /*
   typedef struct _DATETIME      //
           {
           UCHAR   hours;
           UCHAR   minutes;
           UCHAR   seconds;
           UCHAR   hundredths;
           UCHAR   day;
           UCHAR   month;
           USHORT  year;
           SHORT   timezone;
           UCHAR   weekday;
           } DATETIME;
   typedef DATETIME *PDATETIME;
   */

   /*
   typedef struct _SYSTEMTIME {  //
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
   } SYSTEMTIME;
   */

   SYSTEMTIME stData_ora_attuale;

   GetSystemTime(&stData_ora_attuale);

   // check pdt non nullo
   if (pdt != NULL) {
      pdt->hours      = stData_ora_attuale.wHour;
      pdt->minutes    = stData_ora_attuale.wMinute;
      pdt->seconds    = stData_ora_attuale.wSecond;
      pdt->hundredths = stData_ora_attuale.wMilliseconds / 10;
      pdt->day        = stData_ora_attuale.wDay;
      pdt->month      = stData_ora_attuale.wMonth;
      pdt->year       = stData_ora_attuale.wYear;
      // Non ho verificato se la convenzione del giorno della settimana
      // è uguale in Windows e in OS/2 perché comunque per il porting
      // dell'orario FS questo campo non serve
      pdt->weekday    = stData_ora_attuale.wDayOfWeek;
      pdt->timezone   = 0; // Non fornisco questo campo
   } else {
      ERRSTRING("pdt==NULL");
      return ERROR_INVALID_DATA;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//    DosFindFirst
///////////////////////////////////////////////////////

// La funzione non riporta i campi cbFileAlloc e cchName
// Non viene correttamente fornita la dimensione di file
// più grandi di 4Gb
// Il campo cFileName viene copiato in achName troncandolo
// Il campo cAlternateFileName non può essere copiato nella
// struttura OS/2 che non lo prevede
// Il parametro ulInfoLevel viene ignorato

// ?E questa funzione potrebbe utilizzare la findfirst invece
// della FindFirstFile. La findfirst sarebbe preferibile alla
// FindFirstFile perché portabile su DOS, Win16, Win32, OS/2
// mentre la FindFirstFile è solo Win32 (e Win32s).
// La findfirst comunque non è ANSI C

APIRET APIENTRY  DosFindFirst(PSZ    pszFileSpec,
                                                  PHDIR  phdir,
                                                  ULONG  flAttribute,
                                                  PVOID  pfindbuf,
                                                  ULONG  cbBuf,
                                                  PULONG pcFileNames,
                                                  ULONG  ulInfoLevel)
{

// ?E metto una trace di avvertimento se scopro che ulInfoLevel
// è diverso da 1 ? (che è il solo valore usato ?)

/*
HANDLE FindFirstFile(lpszSearchFile, lpffd)

LPCTSTR lpszSearchFile;                  // address of name of the file to search for
LPWIN32_FIND_DATA lpffd;                 // address of returned information

typedef struct _WIN32_FIND_DATA { // wfd
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
    DWORD    dwReserved0;
    DWORD    dwReserved1;
    TCHAR    cFileName[ MAX_PATH ];        // MAX_PATH = 260
    TCHAR    cAlternateFileName[ 14 ];
} WIN32_FIND_DATA;

typedef struct _FILEFINDBUF3                 // findbuf3
              {
     ULONG   oNextEntryOffset;            // new field          // 4
     FDATE   fdateCreation;                                     // 2
     FTIME   ftimeCreation;                                     // 2
     FDATE   fdateLastAccess;                                   // 2
     FTIME   ftimeLastAccess;                                   // 2
     FDATE   fdateLastWrite;                                    // 2
     FTIME   ftimeLastWrite;                                    // 2
     ULONG   cbFile;                                            // 4
     ULONG   cbFileAlloc;                                       // 4
     ULONG   attrFile;                    // widened field      // 4
     UCHAR   cchName;            // Non utilizzato              // 1
     CHAR    achName[CCHMAXPATHCOMP];                           // 256
     } FILEFINDBUF3;                                            // tot byte 285 word 286 dword 300
typedef FILEFINDBUF3 *PFILEFINDBUF3;

HDIR è infine un unsigned long (usare per metterci un pointer ad un'area allocata
   con tutti i dati per la funzione)
*/

/*  Campo flAttribute
   ULONG Attributi = FILE_ARCHIVED | FILE_READONLY; // Include sempre i files archiviati e readonly
   if(AncheHidden)Attributi |= FILE_SYSTEM | FILE_HIDDEN;
   if(AncheDirectories)Attributi |= FILE_DIRECTORY;
   #define FILE_NORMAL     0x0000   0x0080 WIN
   #define FILE_READONLY   0x0001
   #define FILE_HIDDEN     0x0002
   #define FILE_SYSTEM     0x0004
   #define FILE_DIRECTORY  0x0010
   #define FILE_ARCHIVED   0x0020


*/

   WIN32_FIND_DATA ffdDati_file;         // address of returned information
   //HANDLE hHandle_ricerca;
   APIRET ulUltimo_errore;
   APIRET ulReturn_code;
   //ULONG ulNumero_files_trovati=0;
   //ULONG ulOffset_primo_byte_libero=0;
   //SYSTEMTIME stOra_data_file;
   ULONG ulMaschera_filtro_attributi = (~flAttribute) & 0x0000003F;
   WIN32_FIND_DATA ffdDati_file_trovato;
   LPSTRUTTURA_TLS lpStruttura_tls;

   // Archivio la maschera filtro attributi nel Tls per l'uso
   // da parte delle funzioni DosFindNext e DosFindClose
   // in modo da evitare l'uso di variabili globali

   *phdir = (HDIR) INVALID_HANDLE_VALUE;

   ulReturn_code =
         Ottieni_puntatore_a_struttura_tls(&lpStruttura_tls);
   if (ulReturn_code != NO_ERROR) {
      ERRINT("Ottieni_puntatore_a_struttura_tls", ulReturn_code);
      return ulReturn_code;
   }

   lpStruttura_tls->ulMaschera_filtro_attributi = ulMaschera_filtro_attributi;

   *phdir = (HDIR) FindFirstFile(pszFileSpec,    //lpszSearchFile
                                                     &ffdDati_file_trovato); //lpffd

   if (*phdir == (HDIR) INVALID_HANDLE_VALUE) {
      ulUltimo_errore = Ultimo_errore();
      // Cambio codice d'errore da ERROR_FILE_NOT_FOUND a
      // ERROR_NO_MORE_FILES perché il chiamante (FILE_RW.CPP)
      // testa solo quest'ultimo
      if (ulUltimo_errore == ERROR_FILE_NOT_FOUND)
         ulUltimo_errore = ERROR_NO_MORE_FILES;
      if (ulUltimo_errore != ERROR_NO_MORE_FILES)
         ERRINT("FindFirstFile", ulUltimo_errore);
      return ulUltimo_errore;
   }

   ulReturn_code = Cerca_file_seguenti(pfindbuf,    //PVOID  pfindbuf,
                                                           cbBuf,       //ULONG  cbBuf,
                                                           &ffdDati_file_trovato,    //LPWIN32_FIND_DATA pffdDati_file_trovato,
                                                           TRUE,                     //BOOL fElaborare_primo_file_trovato,
                                                           pcFileNames,              //PULONG pulNumero_files_trovati,
                                                           ulMaschera_filtro_attributi,    //ULONG ulMaschera_filtro_attributi,
                                                           *phdir);                         //HDIR hdir,
                                                           //BOOL * pfRicerca_terminata)

   ERRINT("Cerca_file_seguenti", ulReturn_code);
   return ulReturn_code;
}

///////////////////////////////////////////////////////
//    DosFindNext
///////////////////////////////////////////////////////

// Vedere DosFindFirst

APIRET APIENTRY DosFindNext(HDIR   hDir,
                                                PVOID  pfindbuf,
                                                ULONG  cbfindbuf,
                                                PULONG pcFilenames)
{
   WIN32_FIND_DATA ffdDati_file_trovato;
   ULONG ulMaschera_filtro_attributi;
   APIRET ulReturn_code;
   LPSTRUTTURA_TLS lpStruttura_tls;

   if (hDir == (HDIR) INVALID_HANDLE_VALUE) {
      ERRSTRING("hDir == (HDIR) INVALID_HANDLE_VALUE");
      return ERROR_INVALID_DATA;
   }

   ulReturn_code =
         Ottieni_puntatore_a_struttura_tls(&lpStruttura_tls);
   if (ulReturn_code != NO_ERROR) {
      ERRINT("Ottieni_puntatore_a_struttura_tls", ulReturn_code);
      return ulReturn_code;
   }

   ulMaschera_filtro_attributi = lpStruttura_tls->ulMaschera_filtro_attributi;

   ulReturn_code = Cerca_file_seguenti(pfindbuf,    //PVOID  pfindbuf,
                                                           cbfindbuf,       //ULONG  cbBuf,
                                                           &ffdDati_file_trovato,    //LPWIN32_FIND_DATA pffdDati_file_trovato,
                                                           FALSE,                     //BOOL fElaborare_primo_file_trovato,
                                                           pcFilenames,              //PULONG pulNumero_files_trovati,
                                                           ulMaschera_filtro_attributi,    //ULONG ulMaschera_filtro_attributi,
                                                           hDir);                         //HDIR hdir,

   ERRINT("Cerca_file_seguenti", ulReturn_code);
   return ulReturn_code;
}

///////////////////////////////////////////////////////
//    DosFindClose
///////////////////////////////////////////////////////

APIRET APIENTRY DosFindClose(HDIR hDir)
{
   APIRET ulUltimo_errore;

   if (!FindClose((HANDLE) hDir)) {
      ulUltimo_errore = Ultimo_errore();
      ERRINT("FindClose", ulUltimo_errore);
      return ulUltimo_errore;
   }
   else
      return NO_ERROR;
}

///////////////////////////////////////////////////////
//      Cerca_file_seguenti
///////////////////////////////////////////////////////

APIRET Cerca_file_seguenti(PVOID  pfindbuf,
                                               ULONG  cbBuf,
                                               LPWIN32_FIND_DATA pffdDati_file_trovato,
                                               BOOL fElaborare_primo_file_trovato,
                                               PULONG pulNumero_files_trovati,
                                               ULONG ulMaschera_filtro_attributi,
                                               HDIR hdir)
                                               //BOOL * pfRicerca_terminata)
{
   BOOL fElaborare_file = fElaborare_primo_file_trovato;
   BOOL fRicerca_terminata = FALSE;
   ULONG ulOffset_primo_byte_libero = 0;
   PFILEFINDBUF3 pStruttura_file_corrente = (PFILEFINDBUF3) pfindbuf;
   APIRET ulUltimo_errore;
   ULONG ulMax_numero_files_da_cercare;
   SYSTEMTIME stOra_data_file;

   if ((cbBuf <= sizeof(FILEFINDBUF3)) ||
       pfindbuf == NULL ||
       pulNumero_files_trovati == NULL) {
      ERRSTRING("Invalid data 1");
      return ERROR_INVALID_DATA;
   }

   ulMax_numero_files_da_cercare = *pulNumero_files_trovati;

   if (ulMax_numero_files_da_cercare == 0) {
      ERRSTRING("Invalid data 2");
      return ERROR_INVALID_DATA;
   }

   *pulNumero_files_trovati = 0;

   do {

      // Se il file ha uno o più attributi non ammessi dalla maschera
      // flAttribute, non lo prendo in considerazione.
      // Per es. il file può avere l'attibuto hidden mentre la maschera
      // flAttribute non lo contiene: il file non va considerato

      if (((pffdDati_file_trovato->dwFileAttributes & ulMaschera_filtro_attributi) == 0) &&
          fElaborare_file) {

         // Copio i dati del file nel buffer
         (*pulNumero_files_trovati) ++;

         /*
         typedef struct _FILEFINDBUF3                 // findbuf3
           {
         ULONG   oNextEntryOffset;            // new field          // 4
         FDATE   fdateCreation;                                     // 2
         FTIME   ftimeCreation;                                     // 2
         FDATE   fdateLastAccess;                                   // 2
         FTIME   ftimeLastAccess;                                   // 2
         FDATE   fdateLastWrite;                                    // 2
         FTIME   ftimeLastWrite;                                    // 2
         ULONG   cbFile;                                            // 4
         ULONG   cbFileAlloc;                                       // 4
         ULONG   attrFile;                    // widened field      // 4
         UCHAR   cchName;            // Non utilizzato              // 1
         CHAR    achName[CCHMAXPATHCOMP];                           // 256
         } FILEFINDBUF3;                                            // tot byte 285 word 286 dword 300
         typedef FILEFINDBUF3 *PFILEFINDBUF3;
         */
         /*
         typedef struct _FTIME           // ftime
              {
              UINT   twosecs : 5;
              UINT   minutes : 6;
              UINT   hours   : 5;
         } FTIME;
         typedef FTIME *PFTIME;
         */

         /*typedef struct _SYSTEMTIME {  // st
          WORD wYear;
          WORD wMonth;
          WORD wDayOfWeek;
          WORD wDay;
          WORD wHour;
          WORD wMinute;
          WORD wSecond;
          WORD wMilliseconds;
         } SYSTEMTIME;
         */

         /*
         BOOL FileTimeToSystemTime(lpft, lpst)
         CONST FILETIME * lpft;          // address of file time to convert
         LPSYSTEMTIME lpst;              // address of converted system time
         */

         /*
         typedef struct _FDATE           // fdate
              {
              UINT   day     : 5;
              UINT   month   : 4;
              UINT   year    : 7;
              } FDATE;
         typedef FDATE   *PFDATE;
         */

         ulOffset_primo_byte_libero += sizeof(FILEFINDBUF3);
         pStruttura_file_corrente->oNextEntryOffset = ulOffset_primo_byte_libero;

         FileTimeToSystemTime(&(pffdDati_file_trovato->ftCreationTime), &stOra_data_file);

         pStruttura_file_corrente->fdateCreation.day     = stOra_data_file.wDay;
         pStruttura_file_corrente->fdateCreation.month   = stOra_data_file.wMonth;
         pStruttura_file_corrente->fdateCreation.year    = stOra_data_file.wYear - 1980;
         pStruttura_file_corrente->ftimeCreation.twosecs = stOra_data_file.wSecond / 2;
         pStruttura_file_corrente->ftimeCreation.minutes = stOra_data_file.wMinute;
         pStruttura_file_corrente->ftimeCreation.hours   = stOra_data_file.wHour;

         FileTimeToSystemTime(&(pffdDati_file_trovato->ftLastAccessTime), &stOra_data_file);

         pStruttura_file_corrente->fdateLastAccess.day     = stOra_data_file.wDay;
         pStruttura_file_corrente->fdateLastAccess.month   = stOra_data_file.wMonth;
         pStruttura_file_corrente->fdateLastAccess.year    = stOra_data_file.wYear - 1980;
         pStruttura_file_corrente->ftimeLastAccess.twosecs = stOra_data_file.wSecond / 2;
         pStruttura_file_corrente->ftimeLastAccess.minutes = stOra_data_file.wMinute;
         pStruttura_file_corrente->ftimeLastAccess.hours   = stOra_data_file.wHour;

         FileTimeToSystemTime(&(pffdDati_file_trovato->ftLastWriteTime), &stOra_data_file);

         pStruttura_file_corrente->fdateLastWrite.day     = stOra_data_file.wDay;
         pStruttura_file_corrente->fdateLastWrite.month   = stOra_data_file.wMonth;
         pStruttura_file_corrente->fdateLastWrite.year    = stOra_data_file.wYear - 1980;
         pStruttura_file_corrente->ftimeLastWrite.twosecs = stOra_data_file.wSecond / 2;
         pStruttura_file_corrente->ftimeLastWrite.minutes = stOra_data_file.wMinute;
         pStruttura_file_corrente->ftimeLastWrite.hours   = stOra_data_file.wHour;

         pStruttura_file_corrente->cbFile = pffdDati_file_trovato->nFileSizeLow;
         pStruttura_file_corrente->cbFileAlloc = 0;
         // la maschera 0x0000003F serve ad ottenere 0x00000000 che significa
         // file normale in OS/2, da 0x00000080 che significa file normale in
         // Windows
         pStruttura_file_corrente->attrFile = pffdDati_file_trovato->dwFileAttributes & 0x0000003F;
         pStruttura_file_corrente->cchName  = 0;
         memcpy(pStruttura_file_corrente->achName,
                pffdDati_file_trovato->cFileName,
                CCHMAXPATHCOMP);
         (pStruttura_file_corrente->achName)[CCHMAXPATHCOMP-1] = '\0';

         // Il prossimo file trovato non entrerebbe nel buffer per cui esco dal ciclo
         if ((ulOffset_primo_byte_libero + sizeof(FILEFINDBUF3)) > cbBuf) break;

         pStruttura_file_corrente = (PFILEFINDBUF3)
               (((BYTE *) pfindbuf) + ulOffset_primo_byte_libero);

      } // endif (ffdDati_file.dwFileAttributes & ulMaschera_filtro_attributi) && fElaborare_file

      // esco se ho trovato il numero max di files
      if (*pulNumero_files_trovati == ulMax_numero_files_da_cercare) break;

      // next
      if (!FindNextFile((HANDLE) hdir, pffdDati_file_trovato)) {
         ulUltimo_errore = Ultimo_errore();
         if (ulUltimo_errore == ERROR_NO_MORE_FILES) {
            fRicerca_terminata = TRUE;
            // esco perché non ci sono più files da trovare
            break;
         } else {
            ERRINT("FindNextFile", ulUltimo_errore);
            return  ulUltimo_errore;
         }
      }

      fElaborare_file = TRUE;

   } while (TRUE); // il ciclo termina su: buffer pieno, raggiunto max n.ro files,
                   // non ci sono più files

   if (fRicerca_terminata)
      return ERROR_NO_MORE_FILES;
   else
      return NO_ERROR;
};


///////////////////////////////////////////////////////
//       DosOpen
///////////////////////////////////////////////////////

//  Uso la sopen per avere un handle di file che mi permette
//  di utilizzare tutte le API che mi servono, cosa che non
//  potrei fare con le API Win32

//  Flags supportati:

//  OPEN_ACTION_OPEN_IF_EXISTS
//  OPEN_ACTION_CREATE_IF_NEW

//  OPEN_ACCESS_READWRITE
//  OPEN_ACCESS_WRITEONLY
//  OPEN_ACCESS_READONLY

//  OPEN_SHARE_DENYWRITE
//  OPEN_SHARE_DENYREADWRITE
//  OPEN_SHARE_DENYNONE
//  OPEN_SHARE_DENYREAD

//  Se non viene riconosciuto nessun flag di share,
//  per default si imposta l'uso esclusivo (OPEN_SHARE_DENYREADWRITE)

//  Questi flags non sono più supportati perchè ho dovuto usare
//  la sopen invece della CreateFile per incompatibilità degli handle

//  OPEN_FLAGS_WRITE_THROUGH
//  OPEN_FLAGS_NO_CACHE
//  OPEN_FLAGS_SEQUENTIAL
//  OPEN_FLAGS_RANDOM
//  OPEN_FLAGS_RANDOMSEQUENTIAL

// Ignoro i parametri pulAction, peaop2, cbFile, ulAttribute

// ?E implementare parametri cdFile e ulAttribute

#if 0
APIRET APIENTRY DosOpen(PSZ    pszFileName,
                                            PHFILEOS2 pHf,
                                            PULONG pulAction,
                                            ULONG  cbFile,
                                            ULONG  ulAttribute,
                                            ULONG  fsOpenFlags,
                                            ULONG  fsOpenMode,
                                            PEAOP2 peaop2)
{
   #undef TRCRTN
   #define TRCRTN "DosOpen"

   int access;
   int shflag;
   int mode;
   int iHandle_del_file;
   static BOOL bPrima_volta = TRUE;

   TRACEBEGIN "trchse=%d  bPrima_volta=%d  pszFileName=%s  livtrace=%d\n",
               trchse, bPrima_volta,
               pszFileName, LIVELLO_DI_TRACE_DEL_PROGRAMMA TRACEEND

   if (bPrima_volta)
      bPrima_volta = FALSE;
   else {
      sprintf(cErrMsg, "Apertura del file %s", pszFileName);
      TRACESTRING(cErrMsg);
   }

   //sprintf(cErrMsg, "trchse=%d",trchse);
   //TRACERAPIDA(cErrMsg);
   //TRACEBEGIN "\n", bPrima_volta TRACEEND

   *pHf = (HFILEOS2) INVALID_HANDLE_VALUE;

   /*
   OpenAction = OPEN_ACTION_OPEN_IF_EXISTS|OPEN_ACTION_CREATE_IF_NEW;
   OpenFlags  = OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYWRITE ;  // Open Flags OS2
   OpenFlags  = OPEN_SHARE_DENYNONE   | OPEN_FLAGS_RANDOMSEQUENTIAL ; // Open Flags OS2
                OPEN_SHARE_DENYREADWRITE  OPEN_FLAGS_WRITE_THROUGH

   OpenAction

   OPEN_ACTION_OPEN_IF_EXISTS|OPEN_ACTION_CREATE_IF_NEW -->  fdwCreate=OPEN_ALWAYS
   OPEN_ACTION_OPEN_IF_EXISTS  -->                             fdwCreate=OPEN_EXISTING
   OPEN_ACTION_CREATE_IF_NEW  -->                                 fdwCreate=CREATE_NEW

   #define OPEN_ACTION_FAIL_IF_EXISTS     0x0000  /* ---- ---- ---- 0000
   #define OPEN_ACTION_OPEN_IF_EXISTS     0x0001  /* ---- ---- ---- 0001
   #define OPEN_ACTION_REPLACE_IF_EXISTS  0x0002  /* ---- ---- ---- 0010

   /*     this nibble applies if file does not exist           xxxx
   #define OPEN_ACTION_FAIL_IF_NEW        0x0000  /* ---- ---- 0000 ----
   #define OPEN_ACTION_CREATE_IF_NEW      0x0010  /* ---- ---- 0001 ----

   OpenFlags

   OPEN_ACCESS_READWRITE  -->   fdwAccess=GENERIC_READ | GENERIC_WRITE
   OPEN_ACCESS_WRITEONLY  -->   fdwAccess=GENERIC_WRITE
   OPEN_ACCESS_READONLY   -->   fdwAccess=GENERIC_READ

   OPEN_SHARE_DENYWRITE      -->  fdwShareMode=FILE_SHARE_READ
   OPEN_SHARE_DENYREADWRITE  -->  fdwShareMode=0
   OPEN_SHARE_DENYNONE       -->  fdwShareMode=FILE_SHARE_WRITE | FILE_SHARE_READ
   OPEN_SHARE_DENYREAD       -->  fdwShareMode=FILE_SHARE_WRITE

   fdwAttrsAndFlags;

   OPEN_FLAGS_WRITE_THROUGH-->FILE_FLAG_WRITE_THROUGH
   OPEN_FLAGS_NO_CACHE-->FILE_FLAG_NO_BUFFERING
   OPEN_FLAGS_SEQUENTIAL-->FILE_FLAG_SEQUENTIAL_SCAN
   OPEN_FLAGS_RANDOM-->FILE_FLAG_RANDOM_ACCESS
   OPEN_FLAGS_RANDOMSEQUENTIAL-->FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_RANDOM_ACCESS

   * = non supportato in questa implementazione
    #define OPEN_ACCESS_READONLY           0x0000  /* ---- ---- ---- -000
    #define OPEN_ACCESS_WRITEONLY          0x0001  /* ---- ---- ---- -001
    #define OPEN_ACCESS_READWRITE          0x0002  /* ---- ---- ---- -010

    #define OPEN_SHARE_DENYREADWRITE       0x0010  /* ---- ---- -001 ----
    #define OPEN_SHARE_DENYWRITE           0x0020  /* ---- ---- -010 ----
    #define OPEN_SHARE_DENYREAD            0x0030  /* ---- ---- -011 ----
    #define OPEN_SHARE_DENYNONE            0x0040  /* ---- ---- -100 ----

   *#define OPEN_FLAGS_NOINHERIT           0x0080  /* ---- ---- 1--- ----
   *#define OPEN_FLAGS_NO_LOCALITY         0x0000  /* ---- -000 ---- ----

    #define OPEN_FLAGS_SEQUENTIAL          0x0100  /* ---- -001 ---- ----
    #define OPEN_FLAGS_RANDOM              0x0200  /* ---- -010 ---- ----
    #define OPEN_FLAGS_RANDOMSEQUENTIAL    0x0300  /* ---- -011 ---- ----
    #define OPEN_FLAGS_NO_CACHE            0x1000  /* ---1 ---- ---- ----

   *#define OPEN_FLAGS_FAIL_ON_ERROR       0x2000  /* --1- ---- ---- ----
    #define OPEN_FLAGS_WRITE_THROUGH       0x4000  /* -1-- ---- ---- ----
   *#define OPEN_FLAGS_DASD                0x8000  /* 1--- ---- ---- ----
   *#define OPEN_FLAGS_NONSPOOLED          0x00040000

   */

   if (fsOpenFlags & OPEN_ACTION_CREATE_IF_NEW) {
      access = O_CREAT;
      mode = S_IREAD | S_IWRITE;
   } else {
      access = 0;
      mode = 0;
   }

   switch (fsOpenFlags & 0x00000003) {
      case 0:      //OPEN_ACTION_FAIL_IF_EXISTS
         // Se non posso crearlo se non esiste e devo fallire
         // se esiste, non posso fare niente: non va bene.
         if (access == 0) {
            printf("Azioni di apertura incompatibili\n");
            printf("fsOpenFlags = 0x%x\n", fsOpenFlags);
            return ERROR_INVALID_DATA;
         }
         access |= O_EXCL;
         break;
      case 2:      //OPEN_ACTION_REPLACE_IF_EXISTS
         access |= O_TRUNC;
         break;
   }

   //O_RDONLY  Open for reading only.
   //O_WRONLY  Open for writing only.
   //O_RDWR Open for reading and writing.

   // Modalità di accesso
   switch (fsOpenMode & 0x00000003) {
      case OPEN_ACCESS_READWRITE:
         access |= O_RDWR; //            Open for reading and writing.
         break;
      case OPEN_ACCESS_WRITEONLY:
         access |= O_WRONLY;  //         Open for writing only.
         break;
      case OPEN_ACCESS_READONLY:
         access |= O_RDONLY;   //        Open for reading only.
         break;
      default:
         printf("Invalid data in Modalità di accesso\n");
         printf("fsOpenMode = 0x%x\n", fsOpenMode);
         return ERROR_INVALID_DATA;
   }

   access |= O_BINARY; // EMS ?C?C?C va bene questa modalità ?

   //SH_COMPAT    Sets compatibility mode.
   //SH_DENYRW    Denies read/write access
   //SH_DENYWR    Denies write access
   //SH_DENYRD    Denies read access
   //SH_DENYNONE  Permits read/write access
   //SH_DENYNO    Permits read/write access

   // Modalità di condivisione
   switch (fsOpenMode & 0x00000070) {
      case OPEN_SHARE_DENYWRITE:
         shflag = SH_DENYWR;  //         Denies write access
         break;
      case OPEN_SHARE_DENYREADWRITE:
         shflag = SH_DENYRW;  //         Denies read/write access
         break;
      case OPEN_SHARE_DENYNONE:
    // EMS VA shflag = SH_DENYNONE;  //  Permits read/write access
    shflag = SH_DENYNO;  //              Permits read/write access
         break;
      case OPEN_SHARE_DENYREAD:
         shflag = SH_DENYRD;             // Denies read access
         break;
      default:
         shflag = SH_DENYRW;
   }

   /*
   // Modalità di bufferizzazione delle operazioni di lettura/scrittura
   if (fsOpenFlags & OPEN_FLAGS_NO_CACHE)
      fdwAttrsAndFlags = FILE_FLAG_NO_BUFFERING; //OPEN_FLAGS_NO_CACHE-->FILE_FLAG_NO_BUFFERING
   else
      switch (fsOpenFlags & 0x00000300) {
         case OPEN_FLAGS_SEQUENTIAL:  //OPEN_FLAGS_SEQUENTIAL-->FILE_FLAG_SEQUENTIAL_SCAN
            fdwAttrsAndFlags = FILE_FLAG_SEQUENTIAL_SCAN;
            break;
         case OPEN_FLAGS_RANDOM:  //OPEN_FLAGS_RANDOM-->FILE_FLAG_RANDOM_ACCESS
            fdwAttrsAndFlags = FILE_FLAG_RANDOM_ACCESS;
            break;
         case OPEN_FLAGS_RANDOMSEQUENTIAL:  //OPEN_FLAGS_RANDOMSEQUENTIAL-->FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_RANDOM_ACCESS
            fdwAttrsAndFlags = FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_RANDOM_ACCESS;
            break;
         default:
            fdwAttrsAndFlags = FILE_FLAG_NO_BUFFERING;
      }


   // E' richiesto l'accesso diretto al file: si tratta di una modalità intermedia
   // tra un uso intenso del buffering (richiesto dai flags FILE_FLAG_SEQUENTIAL_SCAN,
   // FILE_FLAG_RANDOM_ACCESS ecc.) e una completa assenza del buffering
   // (flag FILE_FLAG_NO_BUFFERING)
   if (fsOpenFlags & OPEN_FLAGS_WRITE_THROUGH)
      fdwAttrsAndFlags |= FILE_FLAG_WRITE_THROUGH;
   */

   if (mode != 0) {
      iHandle_del_file = sopen(pszFileName, access, shflag, mode);
      if (iHandle_del_file == -1) {
         ERRINT("sopen 1", errno);
         //printf("Errore sopen 1 errno=%d\n", errno);
         return errno;
      }
   } else {
      iHandle_del_file = sopen(pszFileName, access, shflag);
      if (iHandle_del_file == -1) {
         ERRINT("sopen 2", errno);
         //printf("Errore sopen 2 errno=%d\n", errno);
         return errno;
      }
   }

   // EMS ?C?C?C
   //Fprova = fopen("PROVA.TRC","at");
   //fprintf(Fprova, "Sono qui 824\n");
   //fprintf(Fprova, "iHandle_del_file=%x\n", iHandle_del_file);
   //fclose(Fprova);

   *pHf = (HFILEOS2) iHandle_del_file;

   /*
   Read/write flags

   You can use only one of the following flags:

   O_RDONLY Open for reading only.
   O_WRONLY Open for writing only.
   O_RDWR   Open for reading and writing.

   Other access flags

   You can use any logical combination of the following flags:

   O_NDELAY Not used; for UNIX compatibility.
   O_APPEND If set, the file pointer is set to the end of the file prior to each write.
   O_CREA   If the file exists, this flag has no effect. If the file does not exist, the file is created,
   and the bits of mode are used to set the file attribute bits as in chmod.
   O_TRUNC  If the file exists, its length is truncated to 0. The file attributes remain unchanged.
   O_EXCL   Used only with O_CREAT. If the file already exists, an error is returned.
   O_BINARY This flag can be given to explicitly open the file in binary mode.
   O_TEXT   This flag can be given to explicitly open the file in text mode.

   O_NOINHERIT The file is not passed to child programs.

   S_IWRITE    Permission to write
   S_IREAD     Permission to read
   S_IREAD|S_IWRITE                      Permission to read/write

   shflag specifies the type of file-sharing allowed on the file path. Symbolic constants
   for shflag are defined in share.h.

   Value of shflag                       What it does

   SH_COMPAT   Sets compatibility mode.
   SH_DENYRW   Denies read/write access
   SH_DENYWR   Denies write access
   SH_DENYRD   Denies read access
   SH_DENYNONE Permits read/write access
   SH_DENYNO   Permits read/write access
   */

   /*
   *pHf = (HFILEOS2) CreateFile(pszFileName, //lpszName,
                                                    fdwAccess, //fdwAccess,
                                            fdwShareMode, //fdwShareMode,
                                            NULL, //lpsa,
                                            fdwCreate, //fdwCreate,
                                            fdwAttrsAndFlags, //fdwAttrsAndFlags,
                                            NULL); //hTemplateFile)
   */

   /*
   LPCTSTR lpszName;                              // address of name of the file
   DWORD fdwAccess;                               // access (read-write) mode
   DWORD fdwShareMode;                         // share mode
   LPSECURITY_ATTRIBUTES lpsa;           // address of security descriptor
   DWORD fdwCreate;                                  // how to create
   DWORD fdwAttrsAndFlags;                     // file attributes
   HANDLE hTemplateFile;                       // handle of file with attrs. to copy
   */

   return NO_ERROR;
}

#else

typedef enum {
   NON_DEFINITO,
   R,
   W,
   RW
} ACCESSO;

APIRET APIENTRY DosOpen(PSZ    pszFileName,
                                            PHFILEOS2 pHf,
                                            PULONG pulAction,
                                            ULONG  cbFile,
                                            ULONG  ulAttribute,
                                            ULONG  fsOpenFlags,
                                            ULONG  fsOpenMode,
                                            PEAOP2 peaop2)
{
   #undef TRCRTN
   #define TRCRTN "DosOpen"

   int mode;
   int iHandle_del_file;
   static BOOL bPrima_volta = TRUE;
   ACCESSO Accesso = NON_DEFINITO;
   char cAzione1[3];
   char cAzione2[3];
   BOOL bProseguire_su_successo; //=Eseguire_seconda_azione_su_fallimento_della_prima;
   FILE * fFile;

   TRACEBEGIN "NEW trchse=%d  bPrima_volta=%d  pszFileName=%s  livtrace=%d\n",
               trchse, bPrima_volta,
               pszFileName, LIVELLO_DI_TRACE_DEL_PROGRAMMA TRACEEND

   if (bPrima_volta)
      bPrima_volta = FALSE;
   else {
      sprintf(cErrMsg, "NEW Apertura del file %s", pszFileName);
      TRACESTRING(cErrMsg);
   }

   //sprintf(cErrMsg, "trchse=%d",trchse);
   //TRACERAPIDA(cErrMsg);
   //TRACEBEGIN "\n", bPrima_volta TRACEEND

   *pHf = (HFILEOS2) INVALID_HANDLE_VALUE;

   if ( (fsOpenFlags & 0x00000003) == OPEN_ACTION_FAIL_IF_EXISTS )
      return ERROR_INVALID_DATA;

   if ( ((fsOpenMode & 0x00000070) == OPEN_SHARE_DENYWRITE) ||
        ((fsOpenMode & 0x00000070) == OPEN_SHARE_DENYNONE) ||
        ((fsOpenMode & 0x00000070) == OPEN_SHARE_DENYREAD) ) {
      //printf("Invalid data in Modalità di accesso\n");
      //printf("fsOpenMode = 0x%x\n", fsOpenMode);
      //return ERROR_INVALID_DATA;
      //?C?C? trace per dire che la modalità di share non è supportata
   }

   switch (fsOpenMode & 0x00000003) {
      case OPEN_ACCESS_READWRITE:
         //access |= O_RDWR; //            Open for reading and writing.
         Accesso = RW;
         break;
      case OPEN_ACCESS_WRITEONLY:
         //access |= O_WRONLY;  //         Open for writing only.
         Accesso = W;
         break;
      case OPEN_ACCESS_READONLY:
         //access |= O_RDONLY;   //        Open for reading only.
         Accesso = R;
         break;
      default:
         //printf("Invalid data in Modalità di accesso\n");
         //printf("fsOpenMode = 0x%x\n", fsOpenMode);
         return ERROR_INVALID_DATA;
   }

   #define SETAZ(az1,az2) { strcpy(cAzione1, az1); strcpy(cAzione2, az2); }
   #define ERRORE return ERROR_INVALID_DATA;

   if (fsOpenFlags & OPEN_ACTION_CREATE_IF_NEW) {
      if ( (fsOpenFlags & 0x00000003) == OPEN_ACTION_REPLACE_IF_EXISTS ) {
         // SE NON C'E': CREARE
         // SE C'E'    : AZZERARE
         // AZIONI:
         // R:   ERRORE
         // W:   w
         // RW:  w+
         switch (Accesso) {
            case R:
               ERRORE;
            case W:
               SETAZ("w",""); break;
            case RW:
               SETAZ("w+",""); break;
         }
      } else {  //(fsOpenFlags & 0x00000003) == OPEN_ACTION_REPLACE_IF_EXISTS
         // SE NON C'E': CREARE
         // SE C'E'    : APRIRE
         // AZIONI:
         // R:   ERRORE
         // W:   a
         // RW:  r+ se non va w+
         switch (Accesso) {
            case R:
               ERRORE
            case W:
               SETAZ("a",""); break;
            case RW:
               SETAZ("r+","w+"); bProseguire_su_successo = FALSE; break;
         }
      } //endif (fsOpenFlags & 0x00000003) == OPEN_ACTION_REPLACE_IF_EXISTS
   } else {  //(fsOpenFlags & OPEN_ACTION_CREATE_IF_NEW)
      if ( (fsOpenFlags & 0x00000003) == OPEN_ACTION_REPLACE_IF_EXISTS ) {
         // SE NON C'E': FALLIRE
         // SE C'E'    : AZZERARE
         // AZIONI:
         // R:   ERRORE
         // W:   r e se va w
         // RW:  r e se va w+
         switch (Accesso) {
            case R:
               ERRORE
            case W:
               SETAZ("r","w"); bProseguire_su_successo = TRUE; break;
            case RW:
               SETAZ("r","w+"); bProseguire_su_successo = TRUE; break;
         }
      } else { //(fsOpenFlags & 0x00000003) == OPEN_ACTION_REPLACE_IF_EXISTS
         // SE NON C'E': FALLIRE
         // SE C'E'    : APRIRE
         // AZIONI:
         // R:   r
         // W:   r e se va a
         // RW:  r+
         switch (Accesso) {
            case R:
               SETAZ("r",""); break;
            case W:
               SETAZ("r","a"); bProseguire_su_successo = TRUE; break;
            case RW:
               SETAZ("r+",""); break;
         }
      } //endif (fsOpenFlags & 0x00000003) == OPEN_ACTION_REPLACE_IF_EXISTS
   }  // endif (fsOpenFlags & OPEN_ACTION_CREATE_IF_NEW)

   // Modalità di accesso

   //access |= O_BINARY; // EMS ?C?C?C va bene questa modalità ?

   // assert (*cAzione1 != '\0');

   strcat(cAzione1, "b");
   fFile = fopen(pszFileName, cAzione1);
   if (fFile != NULL && bProseguire_su_successo && *cAzione2) {
      fclose(fFile);
      strcat(cAzione2, "b");
      fFile = fopen(pszFileName, cAzione2);
      if (fFile == NULL)
         return ERROR_OPEN_FAILED;
   }
   else
   if (fFile == NULL && !bProseguire_su_successo && *cAzione2)  {
      fclose(fFile);
      strcat(cAzione2, "b");
      fFile = fopen(pszFileName, cAzione2);
      if (fFile == NULL)
         return ERROR_OPEN_FAILED;
   }
   else
   if (fFile == NULL) {
      return ERROR_OPEN_FAILED;
   }

   *pHf = (HFILEOS2) fFile;

   return NO_ERROR;
};

#endif

///////////////////////////////////////////////////////
//        DosClose
///////////////////////////////////////////////////////

#if 0
APIRET APIENTRY DosClose(HFILEOS2 hFile)
{
   #undef TRCRTN
   #define TRCRTN "DosClose"

   if (close((int) hFile) == -1) {
      ERRINT("close", errno);
      return errno;
   }
   else
      return NO_ERROR;

   //if (!CloseHandle((HANDLE) hFile))
   // return Ultimo_errore();
   //else
   // return NO_ERROR;
}
#else
APIRET APIENTRY DosClose(HFILEOS2 hFile)
{
   #undef TRCRTN
   #define TRCRTN "DosClose"

   int iReturn_code;

   if (hFile == (HFILEOS2) INVALID_HANDLE_VALUE)
      return ERROR_INVALID_DATA;

   fclose((FILE *) hFile);

   //iReturn_code = ferror((FILE *) hFile);

   hFile = (HFILEOS2) INVALID_HANDLE_VALUE;

   return NO_ERROR; //errno; //iReturn_code;
}
#endif

///////////////////////////////////////////////////////
//       DosQueryFileInfo
///////////////////////////////////////////////////////

// Prima dell'utilizzo di questa funzione il file va aperto
// con la DosOpen (che si converte in una sopen)

// Viene ritornato errore (ERROR_INVALID_DATA) se il puntatore
// pInfo è nullo o se cbInfoBuf non coincide con la size
// della struttura FILESTATUS

// La funzione non ritorna i seguenti campi della struttura FILESTATUS:
// cbFileAlloc
// fdateCreation
// ftimeCreation
// fdateLastAccess
// ftimeLastAccess
// attrFile

// Il parametro ulInfoLevel non è supportato. Viene tornato errore
// se il valore è diverso da 1 che è l'unico valore utilizzato
// nelle chiamate a questa funzione. Se il valore è diverso,
// significa che sono state codificate altre chiamate per cui
// anche questa implementazione che ignora il parametro
// ulInfoLevel va rivista

APIRET APIENTRY DosQueryFileInfo(HFILEOS2 hf,
                                                     ULONG ulInfoLevel,
                                                     PVOID pInfo,
                                                     ULONG cbInfoBuf)
{
   #undef TRCRTN
   #define TRCRTN "DosQueryFileInfo"

   /*
   typedef struct _FILESTATUS      // fsts
   {
     FDATE  fdateCreation;
     FTIME  ftimeCreation;
     FDATE  fdateLastAccess;
     FTIME  ftimeLastAccess;
     FDATE  fdateLastWrite;        // richiesto
     FTIME  ftimeLastWrite;        // richiesto
     ULONG  cbFile;                // richiesto
     ULONG  cbFileAlloc;
     USHORT attrFile;
   } FILESTATUS;
   */

   /*
   struct ftime {
      unsigned ft_tsec: 5;      /* two seconds *
      unsigned ft_min: 6;       /* minutes *
      unsigned ft_hour: 5;      /* hours *
      unsigned ft_day: 5;       /* days *
      unsigned ft_month: 4;     /* months *
      unsigned ft_year: 7;      /* year - 1980*
   };
   */

   PFILESTATUS pfsFileInfo = (PFILESTATUS) pInfo;
   struct stat statInfo_file; // EMS VA
   // EMS VA struct ftime ftimeData_ora_ultima_scrittura;
   long lSize_del_file;
   struct tm *ptmData_ora_ultima_scrittura;

   if (pfsFileInfo == NULL ||
       cbInfoBuf != sizeof(FILESTATUS) ||
       hf == (HFILEOS2) INVALID_HANDLE_VALUE ||
       ulInfoLevel != 1) {
      ERRSTRING("Invalid data");
      return ERROR_INVALID_DATA;
   }

   // Begin EMS VA
   /*st_size st_mtime
   if (getftime((int) hf, &ftimeData_ora_ultima_scrittura) == -1) {
      ERRINT("getftime", errno);
      return errno;
   }

   lSize_del_file = filelength((int) hf);
   */

   TRACEVLONG(INVALID_HANDLE_VALUE);
   TRACEVLONG(hf);

   if (fstat(fileno((FILE *) hf), &statInfo_file) == -1) {
      ERRINT("fstat", errno);
      return errno;
   }

   lSize_del_file = statInfo_file.st_size;

   // end EMS VA

   if (lSize_del_file == -1) {
      ERRSTRING("lSize_del_file == -1");
      return errno;
   }

   // EMS VA
   ptmData_ora_ultima_scrittura = localtime(&(statInfo_file.st_mtime));

   memset(pfsFileInfo, 0, sizeof(FILESTATUS));

   // begin EMS VA
   //pfsFileInfo->fdateLastWrite.day      = ftimeData_ora_ultima_scrittura.ft_day;
   //pfsFileInfo->fdateLastWrite.month    = ftimeData_ora_ultima_scrittura.ft_month;
   //pfsFileInfo->fdateLastWrite.year     = ftimeData_ora_ultima_scrittura.ft_year;
   //pfsFileInfo->ftimeLastWrite.twosecs  = ftimeData_ora_ultima_scrittura.ft_tsec;
   //pfsFileInfo->ftimeLastWrite.minutes  = ftimeData_ora_ultima_scrittura.ft_min;
   //pfsFileInfo->ftimeLastWrite.hours    = ftimeData_ora_ultima_scrittura.ft_hour;

   /*
   struct tm {
         int tm_sec;      /* seconds after the minute [0-61]        *
         int tm_min;      /* minutes after the hour [0-59]          *
         int tm_hour;     /* hours since midnight [0-23]            *
         int tm_mday;     /* day of the month [1-31]                *
         int tm_mon;      /* months since January [0-11]            *
         int tm_year;     /* years since 1900                       *
         int tm_wday;     /* days since Sunday [0-6]                *
         int tm_yday;     /* days since January 1 [0-365]           *
         int tm_isdst;    /* Daylight Saving Time flag              *
   };
   */

   pfsFileInfo->fdateLastWrite.day      = ptmData_ora_ultima_scrittura->tm_mday;
   pfsFileInfo->fdateLastWrite.month    = ptmData_ora_ultima_scrittura->tm_mon+1;
   pfsFileInfo->fdateLastWrite.year     = ptmData_ora_ultima_scrittura->tm_year-80;
   pfsFileInfo->ftimeLastWrite.twosecs  = ptmData_ora_ultima_scrittura->tm_sec / 2;
   pfsFileInfo->ftimeLastWrite.minutes  = ptmData_ora_ultima_scrittura->tm_min;
   pfsFileInfo->ftimeLastWrite.hours    = ptmData_ora_ultima_scrittura->tm_hour;
   // end EMS VA

   pfsFileInfo->cbFile = lSize_del_file;

   // la maschera 0x0000003F serve ad ottenere 0x00000000, che significa
   // file normale in OS/2, da 0x00000080 che significa file normale in
   // Windows

   //pfsFileInfo->attrFile = fiInfo_file.dwFileAttributes & 0x0000003F;

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//             DosSetFilePtr
///////////////////////////////////////////////////////

// Per il parametro method sono supportati solo i valori
// 0, 1 e 2
// Non vengono supportati files di dimensione superiore
// ai 4 Gb
// Uso ancora API Win32 perché ho usato la CreateFile per
// aprire il file per cui devo usare solo funzioni
// che trattano lo stesso tipo di handle di file

APIRET APIENTRY DosSetFilePtr(HFILEOS2 hFile, LONG ib, ULONG method, PULONG ibActual)
{
   #undef TRCRTN
   #define TRCRTN "DosSetFilePtr"

   // method==0 setta la posizione data
   // method==1 ritorna la posizione corrente
   // method==2 va a fine file

   int iReturn_code;

   /*
   #include <io.h>
   long lseek(int handle, long offset, int fromwhere);
   SEEK_CUR Current file pointer position
   SEEK_END End-of-file
   SEEK_SET File beginning
   */

   if (ibActual == NULL ||
       method > 2 ||
       hFile == (HFILEOS2) INVALID_HANDLE_VALUE) {
      ERRSTRING("Invalid data");
      return ERROR_INVALID_DATA;
   }

   switch (method) {
      case 0:    //setta la posizione data
         iReturn_code = fseek((FILE *)hFile, ib, SEEK_SET);
         break;
      case 1:    //ritorna la posizione corrente
         iReturn_code = fseek((FILE *)hFile,  0, SEEK_CUR);
         break;
      case 2:    //va a fine file
         iReturn_code = fseek((FILE *)hFile,  0, SEEK_END);
         break;
   }

   if (iReturn_code != 0) {
      iReturn_code = ferror((FILE *)hFile);
      ERRINT("fseek", iReturn_code);
      return iReturn_code;
   } else {
      *ibActual = ftell((FILE *)hFile);
      return NO_ERROR;
   }
}

///////////////////////////////////////////////////////
//              DosRead
///////////////////////////////////////////////////////

// Non vengono supportate operazioni asincrone di I/O (lpOverlapped
// non viene specificato e il file non viene aperto (vedi DosOpen)
// in modo FILE_FLAG_OVERLAPPED)

// Non posso usare la funzione read perché utilizza un tipo di file handle
// non compatibile con quello Win32 ritornato dalla CreateFile (vedi DosOpen)

APIRET APIENTRY DosRead(HFILEOS2 hFile, PVOID pBuffer, ULONG cbRead, PULONG pcbActual)
{
   #undef TRCRTN
   #define TRCRTN "DosRead"

   int iNumero_bytes_letti;
   int iCodice_errore;

   /*
   ReadFile (Win32):
   BOOL ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped)

   HANDLE hFile;  // handle of file to read
   LPVOID lpBuffer;                      // address of buffer that receives data
   DWORD nNumberOfBytesToRead;           // number of bytes to read
   LPDWORD lpNumberOfBytesRead;          // address of number of bytes read
   LPOVERLAPPED lpOverlapped;            // address of structure for data
   */

   if (pcbActual == NULL ||
       hFile == (HFILEOS2) INVALID_HANDLE_VALUE) {
      ERRSTRING("Invalid data");
      return ERROR_INVALID_DATA;
   }

   if (pBuffer == NULL || cbRead == 0) {
      // 30.6.97 Modifica effettuata per non restituire errore
      // in caso di richiesta di lettura di zero bytes con buffer
      // vuoto
      *pcbActual = 0;
      return NO_ERROR;
   }

   iNumero_bytes_letti = fread(pBuffer, 1, cbRead, (FILE *) hFile);

   iCodice_errore = ferror((FILE *) hFile);
   if (iCodice_errore) {
      ERRINT("Errore fread", iCodice_errore);
      return iCodice_errore;
   }

   *pcbActual = iNumero_bytes_letti;

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//              DosWrite
///////////////////////////////////////////////////////

// Non vengono supportate operazioni asincrone di I/O (lpOverlapped
// non viene specificato e il file non viene aperto (vedi DosOpen)
// in modo FILE_FLAG_OVERLAPPED

// Non posso usare la funzione write perché utilizza un tipo di file handle
// non compatibile con quello Win32 ritornato dalla CreateFile (vedi DosOpen)

APIRET APIENTRY DosWrite(HFILEOS2 hFile, PVOID pBuffer, ULONG cbWrite, PULONG pcbActual)
{
   #undef TRCRTN
   #define TRCRTN "DosWrite"

   int iNumero_bytes_scritti;
   int iError_code;

   if (pcbActual == NULL ||
       hFile == (HFILEOS2) INVALID_HANDLE_VALUE) {
      ERRSTRING("Invalid data");
      //TRACERAPIDA("DosWrite-invalida data");
      return ERROR_INVALID_DATA;
   }

   if (pBuffer == NULL || cbWrite == 0) {
      // 30.6.97 Modifica effettuata per non restituire errore
      // in caso di richiesta di lettura di zero bytes con buffer
      // vuoto
      *pcbActual = 0;
      return NO_ERROR;
   }

   iNumero_bytes_scritti =
         fwrite(pBuffer, 1, cbWrite, (FILE *)hFile);

   //iNumero_bytes_scritti = write((int) hFile, pBuffer, cbWrite);
   /*
   if (iNumero_bytes_scritti == -1) {
      ERRINT("write", errno);
      //sprintf(cErrMsg, "DosWrite errno=%d",errno);
      //TRACERAPIDA(cErrMsg);
      return errno;
   }
   */

   iError_code = ferror((FILE *)hFile);

   if (iError_code == 0) {
      fflush((FILE *)hFile);
      *pcbActual = iNumero_bytes_scritti;
      return NO_ERROR;
   } else {
      return iError_code;
   }

   //_flushall();  // ?C?C?C

   //sprintf(cErrMsg, "DosWrite n.ro bytes scritti = %d",iNumero_bytes_scritti);
   //TRACERAPIDA(cErrMsg);

}

///////////////////////////////////////////////////////
//            DosSetFileSize
///////////////////////////////////////////////////////

APIRET APIENTRY DosSetFileSize(HFILEOS2 hFile, ULONG cbSize)
{
   #undef TRCRTN
   #define TRCRTN "DosSetFileSize"

   if (chsize(fileno((FILE *) hFile), cbSize) == -1) {
      ERRINT("chsize", errno);
      return errno;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosSetFileInfo
///////////////////////////////////////////////////////

// La funzione si limita a impostare data e ora dell'
// ultima scrittura.

// Il parametro ulInfoLevel non è supportato. Viene tornato errore
// se il valore è diverso da 1 che è l'unico valore utilizzato
// nelle chiamate a questa funzione. Se il valore è diverso,
// significa che sono state codificate altre chiamate per cui
// anche questa implementazione che ignora il parametro
// ulInfoLevel va rivista

// A seguito del trasferimento del progetto in ambiente VisualAge,
// la funzione non è più implementata e la funzione FILE_RW::SetTms che
// la chiamava è stata ristrutturata.

APIRET APIENTRY DosSetFileInfo(HFILEOS2 hf,
                               ULONG ulInfoLevel,
                               PVOID pInfoBuf,
                               ULONG cbInfoBuf)
{
   #undef TRCRTN
   #define TRCRTN "DosSetFileInfo"

   /*
   struct ftime ftimeData_ora_ultima_scrittura;
   PFILESTATUS pfsFileInfo = (PFILESTATUS) pInfoBuf;

   if (pfsFileInfo == NULL ||
       cbInfoBuf != sizeof(FILESTATUS) ||
       hf == (HFILEOS2) INVALID_HANDLE_VALUE ||
       ulInfoLevel != 1) {
      ERRSTRING("Invalid data");
      return ERROR_INVALID_DATA;
   }

   ftimeData_ora_ultima_scrittura.ft_tsec   = (pfsFileInfo->ftimeLastWrite).twosecs;
   ftimeData_ora_ultima_scrittura.ft_min    = (pfsFileInfo->ftimeLastWrite).minutes;
   ftimeData_ora_ultima_scrittura.ft_hour   = (pfsFileInfo->ftimeLastWrite).hours;
   ftimeData_ora_ultima_scrittura.ft_day    = (pfsFileInfo->fdateLastWrite).day;
   ftimeData_ora_ultima_scrittura.ft_month  = (pfsFileInfo->fdateLastWrite).month;
   ftimeData_ora_ultima_scrittura.ft_year   = (pfsFileInfo->fdateLastWrite).year;

   if (setftime((int) hf, &ftimeData_ora_ultima_scrittura) == -1) {
      ERRINT("setftime", errno);
      return errno;
   }
   */

   return ERROR_CALL_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////
//               DosMove
///////////////////////////////////////////////////////

// Portabilità funzione rename: tutto tranne UNIX (anche ANSI)

APIRET APIENTRY DosMove(PSZ pszOld, PSZ pszNew)
{
   #undef TRCRTN
   #define TRCRTN "DosMove"

   if (rename((const char *) pszOld, (const char *) pszNew)) {
      ERRINT("rename", errno);
      return errno;
   }
   else
      return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosOpenMutexSem
///////////////////////////////////////////////////////

APIRET APIENTRY DosOpenMutexSem (PSZ pszName, PHMTX phmtx)
{


   return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosCreateMutexSem
///////////////////////////////////////////////////////

APIRET APIENTRY DosCreateMutexSem (PSZ pszName, PHMTX phmtx, ULONG flAttr, BOOL32 fState)
{

   if (phmtx != NULL) *phmtx = 0xFFFFFFFF;

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosRequestMutexSem
///////////////////////////////////////////////////////

APIRET APIENTRY DosRequestMutexSem (HMTX hmtx, ULONG ulTimeout)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosReleaseMutexSem
///////////////////////////////////////////////////////

APIRET APIENTRY DosReleaseMutexSem (HMTX hmtx)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                DosCloseMutexSem
///////////////////////////////////////////////////////

APIRET APIENTRY DosCloseMutexSem (HMTX hmtx)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosSearchPath
///////////////////////////////////////////////////////

// Il bit relativo agli errori di rete (SEARCH_IGNORENETERRS)
// non viene supportato

APIRET APIENTRY DosSearchPath(ULONG flag, PSZ pszPathOrName,
                                                  PSZ pszFilename, PBYTE pBuf, ULONG cbBuf)
{
   #undef TRCRTN
   #define TRCRTN "DosSearchPath"

   // EMS VA funzione ristrutturata per assenza delle funzioni _searchenv
   // e _searchstr

#if 0

   char cPath_completo_file[_MAX_PATH];
   char *cContenuto_var_ambiente;
   char *cContenuto_var_ambiente_temp;
   LPTSTR cPuntatore_parte_nome_del_path;
   //DWORD dwLunghezza_di_buffer_necessaria;
   APIRET aErrore;

   /*
   #define SEARCH_PATH           0x0000
   #define SEARCH_CUR_DIRECTORY  0x0001
   #define SEARCH_ENVIRONMENT    0x0002
   #define SEARCH_IGNORENETERRS  0x0004
   */
   /*

   cerca path dato oppure usa suo crit di ricerca
   non cerca dir corrente

   DWORD SearchPath(lpszPath, lpszFile, lpszExtension, cchReturnBuffer, lpszReturnBuffer, plpszFilePart)
   LPCTSTR lpszPath;                     // address of the search path
   LPCTSTR lpszFile;                     // address of the filename
   LPCTSTR lpszExtension;                // address of the extension
   DWORD cchReturnBuffer;                // size, in characters, of the buffer
   LPTSTR lpszReturnBuffer;              // address of buffer for found filename
   LPTSTR *plpszFilePart;                // address of pointer to file part
   */
   /*

   cerca dir corrente
   cerca var ambiente

   #include <stdlib.h>
   void _searchenv(const char *file, const char *varname, char *buf);
   */

   /*

   cerca dir corrente
   cerca path dato

   #include <stdlib.h>
   void _searchstr(const char *file, const char *ipath, char *buf);
   */

   /*  per cercare nella dir corrente:
   void _searchstr(const char *file, const char *ipath = "", char *buf);*/

   /*
   PATH dato + Dir corrente = _searchstr
   PATH dato                = SearchPath / _searchstr con atra ricerca in dir corr
   Var Env   + Dir corrente = _searchenv
   Var Env                  = SearchPath + getenv / _searchenv con altra ricerca in dir corr
   */

   /*#include <stdlib.h>
   char *getenv(const char *name);
   torna NULL se non trova la variabile: dare errore
   */

   //#define SEARCH_PATH           0x0000
   //#define SEARCH_CUR_DIRECTORY  0x0001
   //#define SEARCH_ENVIRONMENT    0x0002
   //#define SEARCH_IGNORENETERRS  0x0004

   aErrore = NO_ERROR;

   if (pszFilename == NULL || pszPathOrName == NULL) {
      ERRSTRING("invalid data 1");
      return ERROR_INVALID_DATA;
   }
   if (pszFilename[0] == '\0' ||
       (!(flag & SEARCH_CUR_DIRECTORY) && pszPathOrName[0] == '\0')) {
      ERRSTRING("invalid data 1");
      return ERROR_INVALID_DATA;
   }

   if (flag & SEARCH_CUR_DIRECTORY) {

      SearchPath(".",         //lpszPath,
                 pszFilename,                     //lpszFile,
                 NULL,                            //lpszExtension,
                 cbBuf,                           //cchReturnBuffer,
                 // EMS VA cast (LPSTR)
                 (LPSTR) pBuf,                            //lpszReturnBuffer,
                 &cPuntatore_parte_nome_del_path); //plpszFilePart)

      aErrore = Ultimo_errore();

      if (aErrore == NO_ERROR) {
         // Abbiamo trovato il file, possiamo uscire

      }

   }

   if (flag & SEARCH_ENVIRONMENT) {
      // Devo cercare nella data variabile di environment
      if (flag & SEARCH_CUR_DIRECTORY) {
         // Devo cercare nella directory corrente e nella var di ambiente data
         // uso la _searchenv

         _searchenv(pszFilename,            //const char *file
                    pszPathOrName,          //const char *varname
                    cPath_completo_file);   //char *buf

         // Se non trova il file, cPath_completo_file == ""

      } else {
         // Devo cercare solo nella var di ambiente data
         // uso la SearchPath ottenendo il contenuto della var di ambiente
         // con la getenv

         //#include <stdlib.h>

         cContenuto_var_ambiente_temp = getenv(pszPathOrName);

         if (cContenuto_var_ambiente_temp[0] != '\0') {

            cContenuto_var_ambiente = (char *) malloc(strlen(cContenuto_var_ambiente_temp)+1);
            strcpy(cContenuto_var_ambiente, cContenuto_var_ambiente_temp);

            //dwLunghezza_di_buffer_necessaria =
            SearchPath(cContenuto_var_ambiente,         //lpszPath,
                       pszFilename,                     //lpszFile,
                       NULL,                            //lpszExtension,
                       cbBuf,                           //cchReturnBuffer,
                       // EMS VA cast (LPSTR)
                       (LPSTR) pBuf,                            //lpszReturnBuffer,
                       &cPuntatore_parte_nome_del_path); //plpszFilePart)

            aErrore = Ultimo_errore();

         } else {

            pBuf[0] = '\0';

         }

      }
   } else {
      // Devo cercare nel path dato
      if (flag & SEARCH_CUR_DIRECTORY) {
         // Devo cercare nella directory corrente e nel path dato
         // uso la _searchstr

         _searchstr(pszFilename,     //const char *file,
                    pszPathOrName,     //const char *ipath = "",
                    cPath_completo_file);    //char *buf);*/

         // Se non trova il file, cPath_completo_file == ""

      } else {
         // Devo cercare solo il path dato
         // uso la SearchPath

         //dwLunghezza_di_buffer_necessaria =
         SearchPath(pszPathOrName,                        //lpszPath,
                    pszFilename,                     //lpszFile,
                    NULL,                            //lpszExtension,
                    cbBuf,                           //cchReturnBuffer,
                    pBuf,                            //lpszReturnBuffer,
                    &cPuntatore_parte_nome_del_path); //plpszFilePart)

         aErrore = Ultimo_errore();

      }
   }

   if (aErrore != NO_ERROR) {
      ERRINT("SearchPath", aErrore);
      return aErrore;
   }

   /*
   if (flag & SEARCH_CUR_DIRECTORY) {
      // Se ho usato _searchstr o _searchenv, devo copiare
      // cPath_completo_file in pBuf ponendo attenzione alla lunghezza
      // del buffer, perché le _searchstr e _searchenv non lo fanno
      // mentre SearchPath lo fa

      if (cbBuf > strlen(cPath_completo_file)) {
         strcpy(pBuf, cPath_completo_file);
      } else {
         memcpy(pBuf, cPath_completo_file, cbBuf);
         pBuf[cbBuf-1] = '\0';
         ERRINT("Buffer overflow lungh data:", cbBuf);
         return ERROR_BUFFER_OVERFLOW;
      }

   }
   */

   return NO_ERROR;
#endif

   return ERROR_CALL_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////
//              DosQueryPathInfo
///////////////////////////////////////////////////////

// I parametri pInfoBuf e cbInfoBuf vengono ignorati
// La funzione viene usata nella libreria solo per
// verificare l'esistenza di date directory per
// cui non servono altre info che non forniamo

APIRET APIENTRY DosQueryPathInfo(PSZ   pszPathName,
                                                     ULONG ulInfoLevel,
                                                     PVOID pInfoBuf,
                                                     ULONG cbInfoBuf)
{
   #undef TRCRTN
   #define TRCRTN "DosQueryPathInfo"

   HANDLE hHandle_ricerca;
   WIN32_FIND_DATA ffdInfo_su_path_dato; // address of returned information
   APIRET ulUltimo_errore;

   /*
   HANDLE FindFirstFile(lpszSearchFile, lpffd)

   LPCTSTR lpszSearchFile;                  // address of name of the file to search for
   LPWIN32_FIND_DATA lpffd;              // address of returned information
   */

   hHandle_ricerca = FindFirstFile(pszPathName,              //lpszSearchFile,
                                                       &ffdInfo_su_path_dato);    //lpffd)

   if (hHandle_ricerca == INVALID_HANDLE_VALUE) {
      ulUltimo_errore = Ultimo_errore();
      ERRINT("FindFirstFile", ulUltimo_errore);
      return ulUltimo_errore;
   }
   else {
      if (!FindClose(hHandle_ricerca)) {
         ulUltimo_errore = Ultimo_errore();
         ERRINT("FindClose", ulUltimo_errore);
         return ulUltimo_errore;
      }
      else
         return NO_ERROR;
   }

   // ?E può in futuro servire ritornare i dati sul file/directory
   // vedere funzione Cerca_file_seguenti
}

///////////////////////////////////////////////////////
//              DosCreateDir
///////////////////////////////////////////////////////

// il parametro peaop2 viene ignorato

APIRET APIENTRY DosCreateDir(PSZ pszDirName, PEAOP2 peaop2)
{
   #undef TRCRTN
   #define TRCRTN "DosCreateDir"

   // EMS VA elimino la funzione perché non usata
#if 0

   if (mkdir(pszDirName)) {
      ERRINT("mkdir", errno);
      return errno;
   }
   else
      return NO_ERROR;

#endif

   return ERROR_CALL_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////
//               DosGetInfoBlocks
///////////////////////////////////////////////////////

APIRET APIENTRY DosGetInfoBlocks(PTIB *pptib, PPIB *pppib)
{
   #undef TRCRTN
   #define TRCRTN "DosGetInfoBlocks"

   /* TIB secondo OS2
   struct tib_s                        // TIB Thread Information Block
   {
       PVOID   tib_pexchain;           // Head of exception handler chain
       PVOID   tib_pstack;             // Pointer to base of stack
       PVOID   tib_pstacklimit;        // Pointer to end of stack
       PTIB2   tib_ptib2;              // Pointer to system specific TIB
       ULONG   tib_version;            // Version number for this TIB structure
       ULONG   tib_ordinal;            // Thread ordinal number
   };
   struct tib2_s
   {
       ULONG   tib2_ultid;             // Thread I.D.
       ULONG   tib2_ulpri;             // Thread priority
       ULONG   tib2_version;           // Version number for this structure
       USHORT  tib2_usMCCount;         // Must Complete count
       USHORT  tib2_fMCForceFlag;      // Must Complete force flag
   };
   */

   /* Win32 puro
   DWORD GetPriorityClass(hProcess)
   HANDLE hProcess;                      // handle to the process
   */

   /*
   int GetThreadPriority(hThread)
   HANDLE hThread;                       // handle to thread
   */

   /*
   typedef struct _STRUTTURA_TLS {
      ULONG ulMaschera_filtro_attributi;
      TIB   tibInfo_thread;
      TIB2  tib2Info_specifiche_thread;
   } STRUTTURA_TLS;
   */

   LPSTRUTTURA_TLS lpStruttura_tls;
   int iPriorita_thread;
   APIRET ulReturn_code;

   ulReturn_code =
         Ottieni_puntatore_a_struttura_tls(&lpStruttura_tls);
   if (ulReturn_code != NO_ERROR) {
      ERRINT("Ottieni_puntatore_a_struttura_tls", ulReturn_code);
      return ulReturn_code;
   }

   memset(&(lpStruttura_tls->tibInfo_thread), 0, sizeof(TIB));
   memset(&(lpStruttura_tls->tib2Info_specifiche_thread), 0, sizeof(TIB2));

   (lpStruttura_tls->tibInfo_thread).tib_ptib2 =
         &(lpStruttura_tls->tib2Info_specifiche_thread);

   iPriorita_thread = GetThreadPriority(GetCurrentThread());

   if (iPriorita_thread == THREAD_PRIORITY_ERROR_RETURN) {
      ulReturn_code = Ultimo_errore();
      ERRINT("GetThreadPriority", ulReturn_code);
      return ulReturn_code;
   }

   (lpStruttura_tls->tib2Info_specifiche_thread).tib2_ulpri =
         iPriorita_thread;

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//             DosEnterCritSec
///////////////////////////////////////////////////////

APIRET APIENTRY DosEnterCritSec(VOID)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//            DosExitCritSec
///////////////////////////////////////////////////////

APIRET APIENTRY DosExitCritSec(VOID)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//            DosQueryCurrentDir
///////////////////////////////////////////////////////

APIRET APIENTRY DosQueryCurrentDir(ULONG disknum, PBYTE pBuf, PULONG pcbBuf)
{
   #undef TRCRTN
   #define TRCRTN "DosQueryCurrentDir"

/* EMS VA
   char cDirectory[_MAX_DIR]; // va definito __WIN32__

   if (pBuf == NULL) {
      ERRSTRING("Invalid data");
      return ERROR_INVALID_DATA;
   }

   if (getcurdir(disknum, cDirectory)) {
      ERRINT("getcurdir", errno);
      return errno;
   }

   if (*pcbBuf > strlen(cDirectory)) {
      strcpy(pBuf, cDirectory);
   } else {
      memcpy(pBuf, cDirectory, *pcbBuf);
      pBuf[*pcbBuf-1] = '\0';
      // Restituisco la lunghezza di buffer minima necessaria
      *pcbBuf = strlen(cDirectory)+1;
      ERRINT("Buffer overflow lungh data:", *pcbBuf);
      return ERROR_BUFFER_OVERFLOW;
   }

   return NO_ERROR;
*/
// EMS VA
   return ERROR_CALL_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////
//            DosQueryCurrentDisk
///////////////////////////////////////////////////////

// Non restituisco la mappa logica dei drive (plogical)

APIRET APIENTRY DosQueryCurrentDisk(PULONG pdisknum, PULONG plogical)
{
   #undef TRCRTN
   #define TRCRTN "DosQueryCurrentDisk"

/*
   //#include <dir.h>
   //int getdisk(void);

   //if (pdisknum == NULL || plogical == NULL)
   // return ERROR_INVALID_DATA;

   if (pdisknum == NULL) {
      ERRSTRING("Invalid data");
      return ERROR_INVALID_DATA;
   }

   *pdisknum = getdisk() + 1;
   //*plogical = 0;

   return NO_ERROR;
*/
// EMS VA
   return ERROR_CALL_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////
//               DosSleep
///////////////////////////////////////////////////////

APIRET APIENTRY DosSleep(ULONG msec)
{
   #undef TRCRTN
   #define TRCRTN "DosSleep"

   DWORD dwStart_time;

   dwStart_time = GetTickCount();

   while (GetTickCount() - dwStart_time < msec);

   // La Sleep non funziona su piattaforma Win32s !!!
   //Sleep(msec);

   return NO_ERROR;
}


///////////////////////////////////////////////////////
//               Sleep_window
///////////////////////////////////////////////////////

APIRET APIENTRY Sleep_window(ULONG msec)
{
   #undef TRCRTN
   #define TRCRTN "Sleep_window"

   MSG msgMessaggio_coda;
   DWORD dwStart_time;

   dwStart_time = GetTickCount();

   while (GetTickCount() - dwStart_time < msec) {

      if (PeekMessage(&msgMessaggio_coda, NULL, 0, 0, PM_REMOVE)) {
         TranslateMessage(&msgMessaggio_coda);
         DispatchMessage(&msgMessaggio_coda);
      }

   }

   return GetTickCount() - dwStart_time; //NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosBeep
///////////////////////////////////////////////////////

APIRET APIENTRY DosBeep(ULONG freq, ULONG dur)
{
   #undef TRCRTN
   #define TRCRTN "DosBeep"

   APIRET ulUltimo_errore;

   if (!Beep(freq, dur)) {
      ulUltimo_errore = Ultimo_errore();
      ERRINT("Beep", ulUltimo_errore);
      return ulUltimo_errore;
   }
   else
      return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosExitList
///////////////////////////////////////////////////////

// Non implementata

APIRET APIENTRY DosExitList(ULONG ordercode, PFNEXITLIST pfn)
{

   //return ERROR_CALL_NOT_IMPLEMENTED;
   return NO_ERROR;
}

///////////////////////////////////////////////////////
//              DosError
///////////////////////////////////////////////////////

APIRET APIENTRY DosError(ULONG error)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//              DosTmrQueryFreq
///////////////////////////////////////////////////////

APIRET APIENTRY DosTmrQueryFreq(PULONG pulTmrFreq)
{
   if (pulTmrFreq != NULL)
      *pulTmrFreq = 1000000L;

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//              DosTmrQueryTime
///////////////////////////////////////////////////////

APIRET APIENTRY DosTmrQueryTime(PQWORD pqwTmrTime)
{
   //typedef struct _QWORD          /* qword */
   //{
   // ULONG   ulLo;
   // ULONG   ulHi;
   //} QWORD;
   //typedef QWORD *PQWORD;
   /* INC */

   if (pqwTmrTime != NULL) {
      pqwTmrTime->ulLo = GetTickCount();
      pqwTmrTime->ulHi = 0L;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                DosLoadModule
///////////////////////////////////////////////////////

APIRET APIENTRY  DosLoadModule(PSZ pszName, ULONG cbName, PSZ pszModname, PHMODULE phmod)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                DosFreeModule
///////////////////////////////////////////////////////

APIRET APIENTRY  DosFreeModule(HMODULE hmod)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosQueryModuleName
///////////////////////////////////////////////////////

APIRET APIENTRY  DosQueryModuleName(HMODULE hmod, ULONG cbName, PCHAR pch)
{
   if ((cbName > 0) && pch != NULL) *pch = '\0';

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosCreateEventSem
///////////////////////////////////////////////////////

// I parametri flAttr e fState vengono ignorati
// Il parametro *phev viene impostato al numero di messaggio
// assegnato da Windows al messaggio di nome pszName che
// registriamo

APIRET APIENTRY  DosCreateEventSem (PSZ pszName, PHEV phev, ULONG flAttr, BOOL32 fState)
{
   #undef TRCRTN
   #define TRCRTN "DosCreateEventSem"

   PSZ pcCarattere_corrente;
   APIRET ulUltimo_errore;
   char cNome_semaforo[81];

   //??CCC?C?C BOOL SetMessageQueue(cMessagesMax)

   //UINT RegisterWindowMessage(lpsz)
   //LPCTSTR lpsz;                       /* address of message string  */

   if (pszName == NULL || phev == NULL) {
      TRACESTRING("Parametri invalidi (pszName == NULL || phev == NULL)");
      return ERROR_INVALID_DATA;
   }

   if (strlen(pszName) < 1 || strlen(pszName) > 80) {
      TRACESTRING("Parametri invalidi (strlen(pszName) < 1 || strlen(pszName) > 80)");
      return ERROR_INVALID_DATA;
   }

   strcpy(cNome_semaforo, pszName);

   for (pcCarattere_corrente = cNome_semaforo; *pcCarattere_corrente; pcCarattere_corrente++)
      if (*pcCarattere_corrente == '\\') *pcCarattere_corrente = '_';

   *phev = RegisterWindowMessage(cNome_semaforo);

   if (*phev == 0) {
      ulUltimo_errore = Ultimo_errore();
      TRACEINT("RegisterWindowMessage",ulUltimo_errore);
      return ulUltimo_errore;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                 DosOpenEventSem
///////////////////////////////////////////////////////

APIRET APIENTRY  DosOpenEventSem (PSZ pszName, PHEV phev)
{
   #undef TRCRTN
   #define TRCRTN "DosOpenEventSem"

   char cNome_semaforo[81];
   char * pcCarattere_corrente;

   if (pszName == NULL || phev == NULL) {
      TRACESTRING("Parametri invalidi (pszName == NULL || phev == NULL)");
      return ERROR_INVALID_DATA;
   }

   if (strlen(pszName) < 1 || strlen(pszName) > 80) {
      TRACESTRING("Parametri invalidi (strlen(pszName) < 1 || strlen(pszName) > 80)");
      return ERROR_INVALID_DATA;
   }

   strcpy(cNome_semaforo, pszName);

   for (pcCarattere_corrente = cNome_semaforo; *pcCarattere_corrente; pcCarattere_corrente++)
      if (*pcCarattere_corrente == '\\') *pcCarattere_corrente = '_';

   *phev = RegisterWindowMessage(cNome_semaforo);

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//               DosCloseEventSem
///////////////////////////////////////////////////////

APIRET APIENTRY  DosCloseEventSem (HEV hev)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                DosResetEventSem
///////////////////////////////////////////////////////

APIRET APIENTRY  DosResetEventSem (HEV hev, PULONG pulPostCt)
{

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                DosPostEventSem
///////////////////////////////////////////////////////

APIRET APIENTRY  DosPostEventSem (HEV hev)
{
   #undef TRCRTN
   #define TRCRTN "DosPostEventSem"

   APIRET ulUltimo_errore;

   //?C?C
   //BOOL PostMessage(hwnd, uMsg, wParam, lParam)

   //HWND hwnd;   /* handle of destination window */
   //UINT uMsg;   /* message to post     */
   //WPARAM wParam;                      /* first message parameter */
   //LPARAM lParam;                      /* second message parameter   */

   if (!PostMessage(HWND_BROADCAST, hev, 0, 0)) {
      ulUltimo_errore = Ultimo_errore();
      TRACEINT("PostMessage failed", ulUltimo_errore);
      return ulUltimo_errore;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                DosWaitEventSem
///////////////////////////////////////////////////////

// ulTimeout non gestito
// se arriva un messaggio WM_QUIT la funzione esce

APIRET APIENTRY DosWaitEventSem (HEV hev, ULONG ulTimeout)
{
   #undef TRCRTN
   #define TRCRTN "DosWaitEventSem"

   MSG msgMessaggio_coda;
   BOOL bEvento_ricevuto = FALSE;

   /*
   BOOL GetMessage(lpmsg, hwnd, uMsgFilterMin, uMsgFilterMax)

   LPMSG lpmsg;   /* address of structure with message  *
   HWND hwnd;  /* handle of window       *
   UINT uMsgFilterMin;                   /* first message  *
   UINT uMsgFilterMax;                   /* last message   *
   */

   /*
   typedef struct tagMSG {
      HWND   hwnd;
      UINT   message;
      WPARAM wParam;
      LPARAM lParam;
      DWORD  time;
      POINT  pt;
   } MSG;
   */

   while (GetMessage(&msgMessaggio_coda, NULL, 0, 0)) {
      if (msgMessaggio_coda.message == hev) {
         bEvento_ricevuto = TRUE;
         break;
      }
      else
      if (msgMessaggio_coda.message == WM_CLOSE)
         break;
      TranslateMessage(&msgMessaggio_coda);
      DispatchMessage(&msgMessaggio_coda);
   }

   if (!bEvento_ricevuto)
      return ERROR_RICEVUTO_WM_QUIT;

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                DosQueryEventSem
///////////////////////////////////////////////////////

APIRET APIENTRY  DosQueryEventSem (HEV hev, PULONG pulPostCt)
{

   return ERROR_CALL_NOT_IMPLEMENTED;
}


///////////////////////////////////////////////////////
//                 DosGetNamedSharedMem
///////////////////////////////////////////////////////

APIRET APIENTRY DosGetNamedSharedMem(PPVOID ppb, PSZ pszName, ULONG flag)
{
   #undef TRCRTN
   #define TRCRTN "DosGetNamedSharedMem"

   HANDLE hFile_mapping;
   PSZ pcCarattere_corrente;
   APIRET aUltimo_errore;
   char cNome_shared_memory[81];

   //?C
   /*
   HANDLE OpenFileMapping(dwDesiredAccess, bInheritHandle, lpName)

   DWORD dwDesiredAccess;                /* access mode *
   BOOL bInheritHandle;                     /* inherit flag *
   LPCTSTR lpName;                             /* address of name of file-mapping object *
   */

   if (ppb == NULL || pszName == NULL) {
      TRACESTRING("Dati invalidi (ppb == NULL || pszName == NULL)");
      return ERROR_INVALID_DATA;
   }

   if (strlen(pszName) < 1 || strlen(pszName) > 80) {
      TRACESTRING("Parametri invalidi (strlen(pszName) < 1 || strlen(pszName) > 80)");
      return ERROR_INVALID_DATA;
   }

   strcpy(cNome_shared_memory, pszName);

   // Devo convertire tutti i caratteri "\" del nome della shared memory
   // in "_" perché la funzione CreateFileMapping non li accetta
   for (pcCarattere_corrente = cNome_shared_memory; *pcCarattere_corrente; pcCarattere_corrente++)
      if (*pcCarattere_corrente == '\\') *pcCarattere_corrente = '_';

   hFile_mapping = OpenFileMapping(FILE_MAP_WRITE, FALSE, pszName);

   if (hFile_mapping == NULL) {
      aUltimo_errore = Ultimo_errore();
      TRACEINT("OpenFileMapping GetLastError:", aUltimo_errore);
      return aUltimo_errore;
   }

   *ppb = MapViewOfFile(hFile_mapping, FILE_MAP_WRITE, 0, 0, 0);

   if (*ppb == NULL) {
      aUltimo_errore = Ultimo_errore();
      TRACEINT("MapViewOfFile GetLastError:", aUltimo_errore);
      CloseHandle(hFile_mapping);
      return aUltimo_errore;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//                DosAllocSharedMem
///////////////////////////////////////////////////////

// Supporto solo i flags: PAG_COMMIT e PAG_WRITE

APIRET APIENTRY DosAllocSharedMem(PPVOID ppb, PSZ pszName, ULONG cb, ULONG flag)
{
   #undef TRCRTN
   #define TRCRTN "DosAllocSharedMem"

   HANDLE hFile_mapping;
   DWORD dwFlags_creazione;
   APIRET aUltimo_errore;
   PSZ pcCarattere_corrente;
   char cNome_shared_memory[101];

   /*
   HANDLE CreateFileMapping(hFile, lpsa, fdwProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpszMapName)

   HANDLE hFile;  /* handle of file to map  *
   LPSECURITY_ATTRIBUTES lpsa;           /* optional security attributes  *
   DWORD fdwProtect;                     /* protection for mapping object *
   DWORD dwMaximumSizeHigh;              /* high-order 32 bits of object size   *
   DWORD dwMaximumSizeLow;               /* low-order 32 bits of object size *
   LPCTSTR lpszMapName;                  /* name of file-mapping object   *
   */

   if (ppb == NULL || pszName == NULL) {
      TRACESTRING("Dati invalidi (ppb == NULL || pszName == NULL)");
      return ERROR_INVALID_DATA;
   }

   // Devo convertire tutti i caratteri "\" del nome della shared memory
   // in "_" perché la funzione CreateFileMapping non li accetta

   memcpy(cNome_shared_memory, pszName, 100);
   cNome_shared_memory[100] = '\0';
   for (pcCarattere_corrente = cNome_shared_memory; *pcCarattere_corrente; pcCarattere_corrente++)
      if (*pcCarattere_corrente == '\\') *pcCarattere_corrente = '_';

   //dwFlags_creazione = 0L;
   if (flag & PAG_COMMIT) dwFlags_creazione = SEC_COMMIT;
   if (flag & PAG_WRITE) dwFlags_creazione |= PAGE_READWRITE;

   hFile_mapping = CreateFileMapping((HANDLE) 0xFFFFFFFF,
                                                          NULL,
                                                          dwFlags_creazione,
                                                          0L,
                                                          cb,
                                                          pszName);

   if (hFile_mapping == NULL) {
      aUltimo_errore = Ultimo_errore();
      TRACEINT("CreateFileMapping GetLastError:", aUltimo_errore);
      return aUltimo_errore;
   }

   /*
   LPVOID MapViewOfFile(hMapObject, fdwAccess, dwOffsetHigh, dwOffsetLow, cbMap)

   HANDLE hMapObject;                    /* file-mapping object to map into address space   *
   DWORD fdwAccess;                      /* access mode *
   DWORD dwOffsetHigh;                   /* high-order 32 bits of file offset   *
   DWORD dwOffsetLow;                    /* low-order 32 bits of file offset *
   DWORD cbMap;   /* number of bytes to map *
   */

   *ppb = MapViewOfFile(hFile_mapping, FILE_MAP_WRITE, 0, 0, 0);

   if (*ppb == NULL) {
      aUltimo_errore = Ultimo_errore();
      TRACEINT("MapViewOfFile GetLastError:", aUltimo_errore);
      CloseHandle(hFile_mapping);
      return aUltimo_errore;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//              DosFreeMem
///////////////////////////////////////////////////////

APIRET APIENTRY DosFreeMem(PVOID pb)
{
   // ?K

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//         Inizializza_utilizzo_modulo_FILE
///////////////////////////////////////////////////////

APIRET Inizializza_utilizzo_modulo_FILE(void)
{
   // Questo flag ci dice se stiamo usando le Win32s su Windows 3.1,
   // oppure se siamo invece su piattaforma Win32 (NT o 95)
   BOOL fWin32s = GetVersion() & 0x80000000;
   LPSTRUTTURA_TLS pStruttura_tls;
   APIRET ulReturn_code;

   if (ulNumero_processi_attaccati == 0 || !fWin32s) {
      // L'allocazione di un Tls (Thread Local Storage) va fatta,
      // per la piattaforma Windows 3.1 con Win32s, solo per il
      // primo processo che si aggancia alla DLL. Per la piattaforma
      // Win32 (Win95 e WinNT) l'allocazione va fatta invece
      // per ogni processo.
      dwIndice_tls = TlsAlloc();
      if (dwIndice_tls == 0xFFFFFFFF) {
         ulReturn_code = Ultimo_errore();
         ERRINT("TlsAlloc", ulReturn_code);
         return ulReturn_code;
      }
      pStruttura_tls = (LPSTRUTTURA_TLS) malloc(sizeof(STRUTTURA_TLS));
      memset(pStruttura_tls, 0, sizeof(STRUTTURA_TLS));
      if (!TlsSetValue(dwIndice_tls,                   // dwTlsIndex,
                                           (LPVOID) pStruttura_tls)) {   // lpvTlsValue)
         ulReturn_code = Ultimo_errore();
         TlsFree(dwIndice_tls);
         dwIndice_tls = 0xFFFFFFFF;
         ERRINT("TlsSetValue", ulReturn_code);
         return ulReturn_code;
      }
   }
   ulNumero_processi_attaccati ++;

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//        Concludi_utilizzo_modulo_FILE
///////////////////////////////////////////////////////

APIRET Concludi_utilizzo_modulo_FILE(void)
{
   // Questo flag ci dice se stiamo usando le Win32s su Windows 3.1,
   // oppure se siamo invece su piattaforma Win32 (NT o 95)
   BOOL fWin32s = GetVersion() & 0x80000000;
   STRUTTURA_TLS *pStruttura_tls;
   APIRET ulReturn_code;

   // Se il numero di processi attaccati è già zero, il fatto che
   // sia stata chiamata questa funzione indica che qualcosa non va
   if (ulNumero_processi_attaccati > 0 &&
       dwIndice_tls != 0xFFFFFFFF)
      ulNumero_processi_attaccati --;
   else {
      ERRSTRING("N.ro processi attaccati = 0, chiamata"
                " Concludi_utilizzo_modulo_FILE invalida");
      return ERROR_CAN_NOT_COMPLETE;
   }

   if (ulNumero_processi_attaccati == 0 || !fWin32s) {
      pStruttura_tls =
            (STRUTTURA_TLS *) TlsGetValue(dwIndice_tls);
      ulReturn_code = Ultimo_errore();
      if (ulReturn_code != NO_ERROR) {
         ERRINT("TlsGetValue", ulReturn_code);
         return ulReturn_code;
      }

      free(pStruttura_tls);

      if (!TlsFree(dwIndice_tls)) {
         ulReturn_code = Ultimo_errore();
         ERRINT("TlsFree", ulReturn_code);
         return ulReturn_code;
      }
      dwIndice_tls = 0xFFFFFFFF;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//      Ottieni_puntatore_a_struttura_tls
///////////////////////////////////////////////////////

APIRET Ottieni_puntatore_a_struttura_tls(LPSTRUTTURA_TLS *plpStruttura_tls)
{
   APIRET ulReturn_code;

   *plpStruttura_tls = (STRUTTURA_TLS *) TlsGetValue(dwIndice_tls);

   ulReturn_code = Ultimo_errore();
   if (ulReturn_code != NO_ERROR) {
      ERRINT("TlsGetValue", ulReturn_code);
      return ulReturn_code;
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////
//      Ottieni_maschera_filtro_attributi_da_tls
///////////////////////////////////////////////////////
/*
APIRET Ottieni_maschera_filtro_attributi_da_tls(ULONG *ulMaschera_filtro_attributi)
{
   STRUTTURA_TLS *pStruttura_tls;
   APIRET ulReturn_code;

   pStruttura_tls = (STRUTTURA_TLS *) TlsGetValue(dwIndice_tls);

   ulReturn_code = Ultimo_errore();
   if (ulReturn_code != NO_ERROR) return ulReturn_code;

   *ulMaschera_filtro_attributi =
         pStruttura_tls->ulMaschera_filtro_attributi;

   return NO_ERROR;
}
*/

/////////////////////////////////////////////////////
//    Ultimo_errore
/////////////////////////////////////////////////////

// Questa funzione elimina la word più significativa
// del codice d'errore in modo da ottenere un codice
// d'errore compatibile con la codifica OS/2

APIRET Ultimo_errore(void)
{

   return (GetLastError() & 0x0000FFFF);
}

#if 0
///////////////////////////////////////////////////////
//       DosOpen
///////////////////////////////////////////////////////

//  Ho dovuto usare la CreateFile perché è l'unica API
//  che mi permette di tradurre da OS2 a Windows dei flag
//  un pò particolari tipo OPEN_FLAGS_WRITE_THROUGH,
//  OPEN_FLAGS_NO_CACHE, ecc.

//  Flags supportati:

//  OPEN_ACTION_OPEN_IF_EXISTS
//  OPEN_ACTION_CREATE_IF_NEW

//  OPEN_ACCESS_READWRITE
//  OPEN_ACCESS_WRITEONLY
//  OPEN_ACCESS_READONLY

//  OPEN_SHARE_DENYWRITE
//  OPEN_SHARE_DENYREADWRITE
//  OPEN_SHARE_DENYNONE
//  OPEN_SHARE_DENYREAD

//  Se non viene riconosciuto nessun flag di share,
//  per default si imposta l'uso esclusivo (OPEN_SHARE_DENYREADWRITE)

//  OPEN_FLAGS_WRITE_THROUGH
//  OPEN_FLAGS_NO_CACHE
//  OPEN_FLAGS_SEQUENTIAL
//  OPEN_FLAGS_RANDOM
//  OPEN_FLAGS_RANDOMSEQUENTIAL

// Ignoro i parametri pulAction, peaop2, cdFile, ulAttribute

// ?E implementare parametri cdFile e ulAttribute

/*

APIRET APIENTRY DosOpen(PSZ    pszFileName,
                                            PHFILEOS2 pHf,
                                            PULONG pulAction,
                                            ULONG  cbFile,
                                            ULONG  ulAttribute,
                                            ULONG  fsOpenFlags,
                                            ULONG  fsOpenMode,
                                            PEAOP2 peaop2)
{
   DWORD fdwAccess;
   DWORD fdwShareMode;
   DWORD fdwCreate;
   DWORD fdwAttrsAndFlags;


   /*
   OpenAction = OPEN_ACTION_OPEN_IF_EXISTS|OPEN_ACTION_CREATE_IF_NEW;
   OpenFlags  = OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYWRITE ;  // Open Flags OS2
   OpenFlags  = OPEN_SHARE_DENYNONE   | OPEN_FLAGS_RANDOMSEQUENTIAL ; // Open Flags OS2
                OPEN_SHARE_DENYREADWRITE  OPEN_FLAGS_WRITE_THROUGH

   OpenAction

   OPEN_ACTION_OPEN_IF_EXISTS|OPEN_ACTION_CREATE_IF_NEW -->  fdwCreate=OPEN_ALWAYS
   OPEN_ACTION_OPEN_IF_EXISTS  -->                             fdwCreate=OPEN_EXISTING
   OPEN_ACTION_CREATE_IF_NEW  -->                                 fdwCreate=CREATE_NEW

   #define OPEN_ACTION_FAIL_IF_EXISTS     0x0000  /* ---- ---- ---- 0000
   #define OPEN_ACTION_OPEN_IF_EXISTS     0x0001  /* ---- ---- ---- 0001
   #define OPEN_ACTION_REPLACE_IF_EXISTS  0x0002  /* ---- ---- ---- 0010

   /*     this nibble applies if file does not exist           xxxx
   #define OPEN_ACTION_FAIL_IF_NEW        0x0000  /* ---- ---- 0000 ----
   #define OPEN_ACTION_CREATE_IF_NEW      0x0010  /* ---- ---- 0001 ----

   OpenFlags

   OPEN_ACCESS_READWRITE  -->   fdwAccess=GENERIC_READ | GENERIC_WRITE
   OPEN_ACCESS_WRITEONLY  -->   fdwAccess=GENERIC_WRITE
   OPEN_ACCESS_READONLY   -->   fdwAccess=GENERIC_READ

   OPEN_SHARE_DENYWRITE      -->  fdwShareMode=FILE_SHARE_READ
   OPEN_SHARE_DENYREADWRITE  -->  fdwShareMode=0
   OPEN_SHARE_DENYNONE       -->  fdwShareMode=FILE_SHARE_WRITE | FILE_SHARE_READ
   OPEN_SHARE_DENYREAD       -->  fdwShareMode=FILE_SHARE_WRITE

   fdwAttrsAndFlags;

   OPEN_FLAGS_WRITE_THROUGH-->FILE_FLAG_WRITE_THROUGH
   OPEN_FLAGS_NO_CACHE-->FILE_FLAG_NO_BUFFERING
   OPEN_FLAGS_SEQUENTIAL-->FILE_FLAG_SEQUENTIAL_SCAN
   OPEN_FLAGS_RANDOM-->FILE_FLAG_RANDOM_ACCESS
   OPEN_FLAGS_RANDOMSEQUENTIAL-->FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_RANDOM_ACCESS

   * = non supportato in questa implementazione
    #define OPEN_ACCESS_READONLY           0x0000  /* ---- ---- ---- -000
    #define OPEN_ACCESS_WRITEONLY          0x0001  /* ---- ---- ---- -001
    #define OPEN_ACCESS_READWRITE          0x0002  /* ---- ---- ---- -010

    #define OPEN_SHARE_DENYREADWRITE       0x0010  /* ---- ---- -001 ----
    #define OPEN_SHARE_DENYWRITE           0x0020  /* ---- ---- -010 ----
    #define OPEN_SHARE_DENYREAD            0x0030  /* ---- ---- -011 ----
    #define OPEN_SHARE_DENYNONE            0x0040  /* ---- ---- -100 ----

   *#define OPEN_FLAGS_NOINHERIT           0x0080  /* ---- ---- 1--- ----
   *#define OPEN_FLAGS_NO_LOCALITY         0x0000  /* ---- -000 ---- ----

    #define OPEN_FLAGS_SEQUENTIAL          0x0100  /* ---- -001 ---- ----
    #define OPEN_FLAGS_RANDOM              0x0200  /* ---- -010 ---- ----
    #define OPEN_FLAGS_RANDOMSEQUENTIAL    0x0300  /* ---- -011 ---- ----
    #define OPEN_FLAGS_NO_CACHE            0x1000  /* ---1 ---- ---- ----

   *#define OPEN_FLAGS_FAIL_ON_ERROR       0x2000  /* --1- ---- ---- ----
    #define OPEN_FLAGS_WRITE_THROUGH       0x4000  /* -1-- ---- ---- ----
   *#define OPEN_FLAGS_DASD                0x8000  /* 1--- ---- ---- ----
   *#define OPEN_FLAGS_NONSPOOLED          0x00040000



   // Modalità di apertura
   if ((fsOpenMode & OPEN_ACTION_OPEN_IF_EXISTS) &&
       (fsOpenMode & OPEN_ACTION_CREATE_IF_NEW))
      fdwCreate = OPEN_ALWAYS;
   else
   if (fsOpenMode & OPEN_ACTION_OPEN_IF_EXISTS)
      fdwCreate = OPEN_EXISTING;
   else
   if (fsOpenMode & OPEN_ACTION_CREATE_IF_NEW)
      fdwCreate = CREATE_NEW;
   else
      return ERROR_INVALID_DATA;

   // Modalità di accesso
   switch (fsOpenFlags & 0x00000003) {
      case OPEN_ACCESS_READWRITE:   //OPEN_ACCESS_READWRITE  -->   fdwAccess=GENERIC_READ | GENERIC_WRITE
         fdwAccess = GENERIC_READ | GENERIC_WRITE;
         break;
      case OPEN_ACCESS_WRITEONLY:  //OPEN_ACCESS_WRITEONLY  -->   fdwAccess=GENERIC_WRITE
         fdwAccess = GENERIC_WRITE;
         break;
      case OPEN_ACCESS_READONLY:  //OPEN_ACCESS_READONLY   -->   fdwAccess=GENERIC_READ
         fdwAccess = GENERIC_READ;
         break;
      default:
         return ERROR_INVALID_DATA;
   }

   // Modalità di condivisione
   switch (fsOpenFlags & 0x00000070) {
      case OPEN_SHARE_DENYWRITE: //OPEN_SHARE_DENYWRITE--> fdwShareMode=FILE_SHARE_READ
         fdwShareMode = FILE_SHARE_READ;
         break;
      case OPEN_SHARE_DENYREADWRITE: //OPEN_SHARE_DENYREADWRITE -->  fdwShareMode=0
         fdwShareMode = 0;
         break;
      case OPEN_SHARE_DENYNONE:   //OPEN_SHARE_DENYNONE --> fdwShareMode=FILE_SHARE_WRITE | FILE_SHARE_READ
         fdwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ;
         break;
      case OPEN_SHARE_DENYREAD:   //OPEN_SHARE_DENYREAD       -->  fdwShareMode=FILE_SHARE_WRITE
         fdwShareMode = FILE_SHARE_WRITE;
         break;
      default:
         fdwShareMode = 0;
   }

   // Modalità di bufferizzazione delle operazioni di lettura/scrittura
   if (fsOpenFlags & OPEN_FLAGS_NO_CACHE)
      fdwAttrsAndFlags = FILE_FLAG_NO_BUFFERING; //OPEN_FLAGS_NO_CACHE-->FILE_FLAG_NO_BUFFERING
   else
      switch (fsOpenFlags & 0x00000300) {
         case OPEN_FLAGS_SEQUENTIAL:  //OPEN_FLAGS_SEQUENTIAL-->FILE_FLAG_SEQUENTIAL_SCAN
            fdwAttrsAndFlags = FILE_FLAG_SEQUENTIAL_SCAN;
            break;
         case OPEN_FLAGS_RANDOM:  //OPEN_FLAGS_RANDOM-->FILE_FLAG_RANDOM_ACCESS
            fdwAttrsAndFlags = FILE_FLAG_RANDOM_ACCESS;
            break;
         case OPEN_FLAGS_RANDOMSEQUENTIAL:  //OPEN_FLAGS_RANDOMSEQUENTIAL-->FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_RANDOM_ACCESS
            fdwAttrsAndFlags = FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_RANDOM_ACCESS;
            break;
         default:
            fdwAttrsAndFlags = FILE_FLAG_NO_BUFFERING;
      }

   // E' richiesto l'accesso diretto al file: si tratta di una modalità intermedia
   // tra un uso intenso del buffering (richiesto dai flags FILE_FLAG_SEQUENTIAL_SCAN,
   // FILE_FLAG_RANDOM_ACCESS ecc.) e una completa assenza del buffering
   // (flag FILE_FLAG_NO_BUFFERING)
   if (fsOpenFlags & OPEN_FLAGS_WRITE_THROUGH)
      fdwAttrsAndFlags |= FILE_FLAG_WRITE_THROUGH;

   *pHf = (HFILEOS2) CreateFile(pszFileName, //lpszName,
                                            fdwAccess, //fdwAccess,
                                            fdwShareMode, //fdwShareMode,
                                            NULL, //lpsa,
                                            fdwCreate, //fdwCreate,
                                            fdwAttrsAndFlags, //fdwAttrsAndFlags,
                                            NULL); //hTemplateFile)

   /*
   LPCTSTR lpszName;                              // address of name of the file
   DWORD fdwAccess;                               // access (read-write) mode
   DWORD fdwShareMode;                         // share mode
   LPSECURITY_ATTRIBUTES lpsa;           // address of security descriptor
   DWORD fdwCreate;                                  // how to create
   DWORD fdwAttrsAndFlags;                     // file attributes
   HANDLE hTemplateFile;                       // handle of file with attrs. to copy


   return Ultimo_errore();
}
*/

/*
///////////////////////////////////////////////////////
//       DosQueryFileInfo
///////////////////////////////////////////////////////

// Prima dell'utilizzo di questa funzione il file va aperto
// con la DosOpen (che si converte in una CreateFile)

// Viene ritornato errore (ERROR_INVALID_DATA) se il puntatore
// pInfo è nullo o se cbInfoBuf non coincide con la size
// della struttura FILESTATUS

// La funzione non ritorna il campo cbFileAlloc della struttura
// FILESTATUS (lo imposta a zero)

// Il parametro ulInfoLevel non è supportato. Viene tornato errore
// se il valore è diverso da 1 che è l'unico valore utilizzato
// nelle chiamate a questa funzione. Se il valore è diverso,
// significa che sono state codificate altre chiamate per cui
// anche questa implementazione che ignora il parametro
// ulInfoLevel va rivista

APIRET APIENTRY DosQueryFileInfo(HFILEOS2 hf,
                                                     ULONG ulInfoLevel,
                                                     PVOID pInfo,
                                                     ULONG cbInfoBuf)
{
   /*
   typedef struct _BY_HANDLE_FILE_INFORMATION { // bhfi
     DWORD    dwFileAttributes;
     FILETIME ftCreationTime;
     FILETIME ftLastAccessTime;
     FILETIME ftLastWriteTime;
     DWORD    dwVolumeSerialNumber;
     DWORD    nFileSizeHigh;
     DWORD    nFileSizeLow;
     DWORD    nNumberOfLinks;
     DWORD    nFileIndexHigh;
     DWORD    nFileIndexLow;
   } BY_HANDLE_FILE_INFORMATION;

   HANDLE hFile;  // handle of file
   LPBY_HANDLE_FILE_INFORMATION lpFileInformation;   // address of structure

   typedef struct _FILESTATUS      // fsts
   {
     FDATE  fdateCreation;
     FTIME  ftimeCreation;
     FDATE  fdateLastAccess;
     FTIME  ftimeLastAccess;
     FDATE  fdateLastWrite;        // richiesto
     FTIME  ftimeLastWrite;        // richiesto
     ULONG  cbFile;                // richiesto
     ULONG  cbFileAlloc;
     USHORT attrFile;
   } FILESTATUS;
   *

   PFILESTATUS pfsFileInfo = (PFILESTATUS) pInfo;
   BY_HANDLE_FILE_INFORMATION fiInfo_file;
   SYSTEMTIME stOra_data_file;

   if (pfsFileInfo == NULL ||
       cbInfoBuf != sizeof(FILESTATUS) ||
       hf == (HFILEOS2) INVALID_HANDLE_VALUE ||
       ulInfoLevel != 1)
      return ERROR_INVALID_DATA;

   //?C ma questa GetFileInformationByHandle funziona ?

   if (!GetFileInformationByHandle((HANDLE) hf, &fiInfo_file))
      return Ultimo_errore();

   FileTimeToSystemTime(&(fiInfo_file.ftCreationTime), &stOra_data_file);

   pfsFileInfo->fdateCreation.day       = stOra_data_file.wDay;
   pfsFileInfo->fdateCreation.month     = stOra_data_file.wMonth;
   pfsFileInfo->fdateCreation.year      = stOra_data_file.wYear;
   pfsFileInfo->ftimeCreation.twosecs   = stOra_data_file.wSecond / 2;
   pfsFileInfo->ftimeCreation.minutes   = stOra_data_file.wMinute;
   pfsFileInfo->ftimeCreation.hours     = stOra_data_file.wHour;

   FileTimeToSystemTime(&(fiInfo_file.ftLastAccessTime), &stOra_data_file);

   pfsFileInfo->fdateLastAccess.day     = stOra_data_file.wDay;
   pfsFileInfo->fdateLastAccess.month   = stOra_data_file.wMonth;
   pfsFileInfo->fdateLastAccess.year    = stOra_data_file.wYear;
   pfsFileInfo->ftimeLastAccess.twosecs = stOra_data_file.wSecond / 2;
   pfsFileInfo->ftimeLastAccess.minutes = stOra_data_file.wMinute;
   pfsFileInfo->ftimeLastAccess.hours   = stOra_data_file.wHour;

   FileTimeToSystemTime(&(fiInfo_file.ftLastWriteTime), &stOra_data_file);

   pfsFileInfo->fdateLastWrite.day      = stOra_data_file.wDay;
   pfsFileInfo->fdateLastWrite.month    = stOra_data_file.wMonth;
   pfsFileInfo->fdateLastWrite.year     = stOra_data_file.wYear;
   pfsFileInfo->ftimeLastWrite.twosecs  = stOra_data_file.wSecond / 2;
   pfsFileInfo->ftimeLastWrite.minutes  = stOra_data_file.wMinute;
   pfsFileInfo->ftimeLastWrite.hours    = stOra_data_file.wHour;

   pfsFileInfo->cbFile = fiInfo_file.nFileSizeLow;
   pfsFileInfo->cbFileAlloc = 0;

   // la maschera 0x0000003F serve ad ottenere 0x00000000, che significa
   // file normale in OS/2, da 0x00000080 che significa file normale in
   // Windows

   pfsFileInfo->attrFile = fiInfo_file.dwFileAttributes & 0x0000003F;

   return NO_ERROR;
}
*/

/*
///////////////////////////////////////////////////////
//             DosSetFilePtr
///////////////////////////////////////////////////////

// Per il parametro method sono supportati solo i valori
// 0, 1 e 2
// Non vengono supportati files di dimensione superiore
// ai 4 Gb
// Uso ancora API Win32 perché ho usato la CreateFile per
// aprire il file per cui devo usare solo funzioni
// che trattano lo stesso tipo di handle di file

APIRET APIENTRY DosSetFilePtr(HFILEOS2 hFile, LONG ib, ULONG method, PULONG ibActual)
{
   // method==0 setta la posizione data
   // method==1 ritorna la posizione corrente
   // method==2 va a fine file

   /*
   DWORD SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod)

   HANDLE hFile;  // handle of file
   LONG lDistanceToMove;                 // number of bytes to move file pointer
   PLONG lpDistanceToMoveHigh;           // address of high-order word of distance to move
   DWORD dwMoveMethod;                   // how to move

   FILE_BEGIN  The starting point is zero or the beginning of the file. If FILE_BEGIN is specified, DistanceToMove is interpreted as
   an unsigned location for the new file pointer.
   FILE_CURRENT   The current value of the file pointer is the starting point.
   FILE_END The current end-of-file position is the starting point.
   *

   if (ibActual == NULL ||
       method > 2 ||
       hFile == (HFILEOS2) INVALID_HANDLE_VALUE)
      return ERROR_INVALID_DATA;

   switch (method) {
      case 0:    //setta la posizione data
         *ibActual = SetFilePointer((HANDLE) hFile, ib, NULL, FILE_BEGIN);
         break;
      case 1:    //ritorna la posizione corrente
         *ibActual = SetFilePointer((HANDLE) hFile,  0, NULL, FILE_CURRENT);
         break;
      case 2:    //va a fine file
         *ibActual = SetFilePointer((HANDLE) hFile,  0, NULL, FILE_END);
         break;
   }

   if (*ibActual == 0xFFFFFFFF)
      return Ultimo_errore();
   else
      return NO_ERROR;
}
*/
#endif



