//----------------------------------------------------------------------------
// File SCANDIR.C
//----------------------------------------------------------------------------
// Esegue un DIR e torna il risultato in un ELENCO_S
//----------------------------------------------------------------------------
// Gestione del trace
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3
//----------------------------------------------------------------------------

#define MODULO_OS2_DIPENDENTE
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS          // Per i mnemonici degli errori
#define INCL_DOS
#define NO_TRC_FOPEN // Per evitare chiamate ricorsive alla DosOpen_TRC ...

#include "scandir.h"
#include <direct.h>
#include <dir.h>

// Commentato extern C per compilare senza NETAPI.LIB #USER
// LAN
// extern "C" {
// #define  INCL_32
// #include <OONETCNS.H>
// #include <WKSTA.H>
// #include <NETERR.H>
// #undef   INCL_32
// }

#define FILE_ATTR  FILE_SYSTEM

// Restituisce l'elenco di tutti i file trovati nella directory e di estensione
// come indicato nel parametro;

ELENCO_S ScanDir(const STRINGA &CompleteFileName)
{
#undef  TRCRTN
#define TRCRTN "ScanDir"

   TRACESTRING2("Files da cercare: ", CompleteFileName);

   ELENCO_S Out;

   APIRET rc;

   HDIR FindHandle = 0xFFFFFFFF;

   FILEFINDBUF3 FindBuffer;

   PFILEFINDBUF3 pFindBuffer = &FindBuffer;

   ULONG FileFindCount = 1;

   struct ffblk ffblk;
   int done;

   done = findfirst((PSZ)(CPSZ)CompleteFileName,&ffblk,0);

   STRINGA Tmp;

   if (!done)
   {
      Tmp = ffblk.ff_name;
      Out += Tmp;
   }
   while (!done)
   {
      done = findnext(&ffblk);
      if (!done)
      {
         Tmp = ffblk.ff_name;
         Out += Tmp;
      }
   }
   TRACESTRING("Files Trovati:");
   Out.Trace();

   return Out;
}

//----------------------------------------------------------------------------
// Funzioncine per i files
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TestFileEsistance : ritorna VERO se il file esiste
// Se vi sono errori ritorna FALSO
//----------------------------------------------------------------------------
BOOL  TestFileExistance (const STRINGA & NomeFile)
{
#undef TRCRTN
#define TRCRTN "TestFileExistance()"    // Nome della routine per il trace

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Variabili locali
//++++++++++++++++++++++++++++++++++++++++++++++++++
   HFILE FileHandle = 0;
   USHORT rc;
   ULONG ActionTaken;

   rc = DosOpen((PSZ)(CPSZ)NomeFile,        // I:Nome del file
                &FileHandle,                // O:Handle se viene trovato
                &ActionTaken,               // O:Indica l' azione fatta da OS2
                0L ,                        // I:Create File Size
                FILE_NORMAL ,               // I:Attributi: non significativi
                OPEN_ACTION_OPEN_IF_EXISTS, // I:Open Action
                OPEN_FLAGS_NO_CACHE |       // I:Open Flags
                OPEN_SHARE_DENYNONE ,
                0L);                        // Reserved

   if (FileHandle) DosClose(FileHandle);

   switch (rc)
   {
      case  NO_ERROR                :
      case  ERROR_SHARING_VIOLATION :
         return TRUE;

      default:
         return FALSE;
   }
}

//----------------------------------------------------------------------------
// Ritorna gli attributi del file specificato
// Se non esiste o se vi sono errori ritorna FALSO
//----------------------------------------------------------------------------
UINT QueryFileAttributes(const STRINGA & NomeFile)
{
#undef TRCRTN
#define TRCRTN "QFileSize()"    // Nome della routine per il trace

   HFILE      FileHandle=0;
   USHORT     rc;
   ULONG      ActionTaken;
   FILESTATUS FileInfo;

   rc=DosOpen(
      (PSZ)(CPSZ)NomeFile,             // Nome del file
      &FileHandle,                     // Handle se viene trovato
      &ActionTaken,                    // Indica l' azione fatta da OS2
      0L,                              // Create File Size
      FILE_NORMAL,                     // Attributi: non significativi
      OPEN_ACTION_OPEN_IF_EXISTS,      // Open Action
      OPEN_FLAGS_NO_CACHE |            // Open Flags
      OPEN_SHARE_DENYNONE,
      0L);                             // Reserved
   if(rc)
   {
      if(FileHandle) DosClose(FileHandle);
      return 0;
   }
   rc=DosQFileInfo(FileHandle, 1, (PBYTE)&FileInfo, sizeof(FileInfo));
   if(FileHandle) DosClose(FileHandle);
   return FileInfo.attrFile;
}


//----------------------------------------------------------------------------
// QueryFileSize : ritorna la dimensione del file
// Ritorna 0 se va tutto bene, altrimenti un return code di errore

USHORT QueryFileSize(const STRINGA & NomeFile, ULONG *Size)
{
#undef TRCRTN
#define TRCRTN "QFileSize()"    // Nome della routine per il trace

   HFILE      FileHandle=0;
   USHORT     rc;
   ULONG      ActionTaken;
   FILESTATUS FileInfo;

   rc=DosOpen(
      (PSZ)(CPSZ)NomeFile,             // Nome del file
      &FileHandle,                     // Handle se viene trovato
      &ActionTaken,                    // Indica l' azione fatta da OS2
      0L,                              // Create File Size
      FILE_NORMAL,                     // Attributi: non significativi
      OPEN_ACTION_OPEN_IF_EXISTS,      // Open Action
      OPEN_FLAGS_NO_CACHE |            // Open Flags
      OPEN_SHARE_DENYNONE,
      0L);                             // Reserved

   TRACEINT("DosOpen, rc = ", rc);
   TRACELONG("Handle dopo DosOpen: ", FileHandle);
   if(rc){
      if(FileHandle) DosClose(FileHandle);
      return rc;
   };
   rc=DosQFileInfo(FileHandle, 1, (PBYTE)&FileInfo, sizeof(FileInfo));
   TRACEINT("DosQFileInfo, rc = ", rc);
   TRACELONG("Handle dopo DosQFileInfo: ", FileHandle);
   if(FileHandle) DosClose(FileHandle);
   TRACELONG("Handle dopo DosClose: ", FileHandle);
   *Size=(!rc ?FileInfo.cbFile :0);
   return rc;
};


//----------------------------------------------------------------------------
// QueryFileStatus : ritorna lo status del file.
// Comprende la data di creazione e dell' ultima modifica, il size
// e gli attributi
// Ritorna 0 se va tutto bene, altrimenti un return code di errore
// Questa routine e' un poco piu' complessa della norma, quindi al
// momento non e' dichiarata export

USHORT QueryFileStatus(CPSZ NomeFile, struct _FILESTATUS3 & FileInfo)
{
#undef TRCRTN
#define TRCRTN "QFileStatus()"    // Nome della routine per il trace

   HFILE      FileHandle=0;
   USHORT     rc;
   ULONG      ActionTaken;

   rc=DosOpen(
      (PSZ)(CPSZ)NomeFile,             // Nome del file
      &FileHandle,                     // Handle se viene trovato
      &ActionTaken,                    // Indica l' azione fatta da OS2
      0L,                              // Create File Size
      FILE_NORMAL,                     // Attributi: non significativi
      OPEN_ACTION_OPEN_IF_EXISTS,      // Open Action
      OPEN_FLAGS_NO_CACHE |            // Open Flags
      OPEN_SHARE_DENYNONE,
      0L);                             // Reserved

   // TRACEINT("DosOpen, rc = ", rc);
   // TRACELONG("Handle dopo DosOpen: ", FileHandle);
   if(rc){
      if(FileHandle) DosClose(FileHandle);
      return rc;
   };
   rc=DosQFileInfo(FileHandle, 1, (PBYTE)&FileInfo, sizeof(FILESTATUS3));
   // TRACEINT("DosQFileInfo, rc = ", rc);
   // TRACELONG("Handle dopo DosQFileInfo: ", FileHandle);
   if(FileHandle) DosClose(FileHandle);
   return rc;
};

//----------------------------------------------------------------------------
// Funzione per trovare un file
//----------------------------------------------------------------------------
// Il file deve stare su una delle directory indicate da "Directories"
// (se e' una stringa indica una variabile di environment).
//
// Se CurrentDir e' vera cerca anche sulla directory corrente
//
// Ritorna il nome COMPLETO del file (path , nome ed estensione)
//----------------------------------------------------------------------------
STRINGA CercaFile(
   const STRINGA& NomeFile       // Nome del file con estensioni
  ,const ELENCO_S & Directories  // Dir su cui cercarlo
  ,BOOL CurrentDir)              // Indica se includere la directory corrente
{
// Se il file ha gia' un PATH allora esegue un TestFileExistance e
// ritorna NUSTR se non esiste, lo stesso nome fornito altrimenti
// In caso contrario lo cerca sul PATH

   STRINGA PATH,FILE,EXT;

// Il PATH Non e' fornito
   if (AnalizeFileName(NomeFile,PATH,FILE,EXT)) {
      STRINGA Path;
      ORD_FORALL(Directories,i)Path += Directories[i] + (STRINGA)";" ;
      char Result[256];
      USHORT Control = (CurrentDir ? 0x05 : 0x04);
      int rc = DosSearchPath( Control
                             ,(PSZ)(CPSZ)Path
                             ,(PSZ)(CPSZ)NomeFile
                             ,(PSZ)Result
                             ,sizeof(Result) );
      if (rc) return NUSTR;
      return STRINGA(Result);
   } else {
// Se esiste va' bene il nome originario, altrimenti ritorno NULL
      if(TestFileExistance(PATH+FILE+"."+EXT))return NomeFile;
      return NUSTR;
   };
};

// Versione con variabile di environment
STRINGA CercaFile(
   const STRINGA& NomeFile       // Nome del file con estensioni
  ,const STRINGA & Directories   // Dir su cui cercarlo (Default = PATH)
  ,BOOL CurrentDir               // Indica se includere la directory corrente
){
   STRINGA PATH,FILE,EXT;
   if (AnalizeFileName(NomeFile,PATH,FILE,EXT)) {
      char Result[256];
      USHORT Control = (CurrentDir ? 0x07 : 0x06);
      int rc=DosSearchPath(
         Control
        ,(PSZ)(CPSZ)Directories
        ,(PSZ)(CPSZ)NomeFile
        ,(PSZ)Result
        ,sizeof(Result));
      if(rc)return NUSTR;
      return STRINGA(Result);
   } else {
   // Se esiste va' bene il nome originario, altrimenti ritorno NULL
      if(TestFileExistance(PATH+FILE+"."+EXT))return NomeFile;
      return NUSTR ;
   };
};

//----------------------------------------------------------------------------
// Analizza il nome di un file.
//----------------------------------------------------------------------------
// Se qualcuna delle variabili di output e' NULL non ritorna la
// componente corrispondente
// Se PATH non e' fornito (cioe' non identificabile univocamente)
// vi imposta il path corrente.
// In tal caso ritorna TRUE per avvisare che ha impostato lui il path
//----------------------------------------------------------------------------
// Nota: il path contiene il ':' od il '\' finale
//----------------------------------------------------------------------------

BOOL AnalizeFileName(const STRINGA& FileName,
                           STRINGA& Path,
                           STRINGA& File,
                           STRINGA& Ext)
{
#undef  TRCRTN
#define TRCRTN "AnalizeFileName"

   STRINGA PATH ;
   STRINGA FILE ;
   STRINGA EXT  ;
   ELENCO_S Seps;

   STRINGA FName;

   BOOL PathIdentificabile;
   char Currentpath[150];
   TRACESTRINGL("Filename = "+FileName,6);


// Se FileName inizia con \ o indica la ROOT o e' un UNC
   if (FileName[0] == '\\')
   {
      PathIdentificabile = TRUE;
      if (FileName[1] == '\\')
      {  TRACESTRINGL("UNC",6);
         FName = FileName;
      } else {  // ROOT DIRECTORY del drive corrente
         TRACESTRINGL("ROOT",6);
         char Drive = 'A'+ getdisk();
         FName = STRINGA(Drive) +":"+FileName;
      }
   }
// Se FileName inizia con un punto
   else if (FileName[0] == '.')
   {
   // Directory corrente
      if (FileName[1] == '\\')
      {
         TRACESTRINGL("CURDIR",6);
         PathIdentificabile = TRUE;
         FName  = getcwd(Currentpath,150);
         FName +=  FileName(1, FileName.Dim());
      }
   // Se FileName inizia con due punti si deve sostituire la directory precedente
      else if (FileName(0,2) == "..\\")
      {
         PathIdentificabile = TRUE;
         STRINGA Tmp = getcwd(Currentpath,150);
         TRACESTRINGL("PREVDIR",6);
         ELENCO_S Toks= Tmp.Tokens("\\");
         if (Toks.Dim() < 2 ) {
            ERRSTRING("Errore : Impossibile analizzare il file '"+FileName+"'");
            FName = FileName(3, FileName.Dim());
            TRACESTRING("Sostuisco il file: '"+FName+"'");
            BEEP;
         } else {
            for (int i = 0;i < Toks.Dim()-1  ; i++) { FName += Toks[i]+"\\"; }
            FName += FileName(3, FileName.Dim());
         }
      } else {
      // Se FileName inizia con un punto e' un' estensione (di file o directory)
      // Aggiungo in testa la directory corrente completamente qualificata;
         TRACESTRINGL("SOLOEXT",6);
         PathIdentificabile = FALSE;
         FName   = getcwd(Currentpath,150);
         FName  +=  '\\';
         FName  +=  FileName;
      }
   }
// Se FileName inizia con un drive
// Se e' indicata la root directory OK
   else if (FileName[1] == ':') {
      if (FileName[2] == '\\') {
         TRACESTRINGL("DRIVEROOT",6);
         PathIdentificabile = TRUE;
         FName  =  FileName;

      } else {
      // Se FileName inizia con un drive ma non con la root directory del drive
      // si deve sostituire la directory corrente del drive completamente qualificata;
         TRACESTRINGL("DRIVECUR",6);
         PathIdentificabile = TRUE;
         char Drive = 1+ ((FileName[0] >= 'A') ? (FileName[0] - 'a') : (FileName[0] - 'A')) ;
         getcurdir(Drive,Currentpath);
         FName  =  FileName(0,1);
         FName  += Currentpath;
         FName  +=  '\\';
         FName  +=  FileName(2, FileName.Dim());
      }
   } else {
   // Il nome non specifica drive ne path ne ...
   // Completo
      TRACESTRINGL("INCOMPLETO",6);
      PathIdentificabile = FALSE;
      FName   = getcwd(Currentpath,150);
      if(FName.Last()!= '\\')FName  +=  '\\';
      FName  +=  FileName;
   }

   TRACESTRINGL("Nome completo dopo le sostituzioni:"+FName,6);

   ELENCO_S Toks = FName.Tokens(":\\.", &Seps);

// Se l' ultimo separatore e' un '.' allora corrisponde all' estensione
   int LastSep = Seps.Dim()-1;
   int LastTok = Toks.Dim()-1;
// Vuol dire che la stringa NON termina con un separatore
   if (Seps[LastSep] == NUSTR)
   {
   // Il file non ha estensione
      if (LastSep == 0 || Seps[LastSep-1] != STRINGA("."))
      {
         if (&Ext  != NULL) Ext.Clear();
         LastSep --;
      }
      else
      {
         if (&Ext  != NULL) Ext = Toks[LastTok];
         LastTok --;
         LastSep -= 2;
      }
   }
// Il file non ha estensione
   else if (Seps[LastSep] == ".")
   {
      if (&Ext != NULL) Ext.Clear();
         LastSep --;
   }

// Determino il filename (puo' avere dei . all' interno)
   while (LastSep < LastTok)
   {
      FILE = Toks[LastTok] + FILE;
      LastTok -- ;
      if (Seps[LastSep] == ".")
      {
         FILE = Seps[LastSep] + FILE;
         LastSep --;
      }
   }

   if (&File != NULL) File = FILE;

// Il resto e' il path
   for (int i = 0; i <= LastTok ;i++ ) { PATH +=  Toks[i] + Seps[i]; }

   if (&Path != NULL) Path = PATH;

   TRACESTRINGL("PATH: "+PATH,6);
   TRACESTRINGL("FILE: "+FILE,6);
   TRACESTRINGL("EXT:  "+EXT,6 );
   TRACEVLONGL(PathIdentificabile,6);

   return !PathIdentificabile;
}

APIRET CreatePath( const STRINGA& Path )
{
   APIRET      rc;
   STRINGA     AppPath;
   FILESTATUS3 FileStatus3;
   ELENCO_S    Seps;
   ELENCO_S    Toks = Path.Tokens("\\",&Seps);

#undef TRCRTN
#define TRCRTN "@SCANDIR CREATEPATH"

   /* check drive */

   AppPath       += Toks[0];
   STRINGA Drive  = Toks[0]+Seps[0];

   TRACESTRING2("Check del drive ", Drive );

   if( (rc = DosQueryPathInfo( (PSZ)(CPSZ)Drive , 1,
            &FileStatus3,
            sizeof( FileStatus3 ) )) != 0 ) {
      TRACESTRING2("Errore drive non trovato", Drive );
      return ERROR_INVALID_DRIVE;
   };

   /* creazione directories */
   for( USHORT i = 1; i < Toks.Dim(); i++) {
      AppPath += Seps[i-1] + Toks[i];

      TRACESTRING2("Check del path ", AppPath );

      rc = DosQueryPathInfo( (PSZ)(CPSZ)AppPath, 1, &FileStatus3,
         sizeof( FileStatus3 ) );
      if( rc == ERROR_PATH_NOT_FOUND || rc == ERROR_FILE_NOT_FOUND ) {

         TRACESTRING2("Creazione del path ", AppPath );

         if( (rc = DosCreateDir( (PSZ)(CPSZ)AppPath , NULL )) != 0 ) {

            TRACESTRING2("Errore Creazione Path", AppPath );

            return rc;
         }
      } else if ( rc != NO_ERROR ) {
         return rc;
      }
   };
   return NO_ERROR;
};

VOID GetReqName(STRINGA & szRequester)

{
   struct wksta_info_10 * PWKSTAINFO;
   USHORT         rc;
   USHORT         nn;
   CHAR           Buffer[256];

// Commentato per compilare senza NETAPI.LIB #USER
//   PWKSTAINFO = (struct wksta_info_10 *)Buffer;

//   rc = NetWkstaGetInfo( (const char *)NULL, 10, (CHAR *)Buffer  ,
//                         sizeof( Buffer ), (USHORT *)&nn );

//   szRequester = "";
//   if (rc == NERR_Success) {
//       szRequester = (const CHAR *)PWKSTAINFO->wki10_username;
//   };
// Aggiunto per compilare senza NETAPI.LIB #USER
   szRequester = "USER";

   return;
};

