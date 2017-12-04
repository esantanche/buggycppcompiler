//----------------------------------------------------------------------------
// BASE.cpp
//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3

// EMS001
typedef unsigned long BOOL;
// EMS002 cambio HFILE in HFILEOS2

#define INVALID_HANDLE_VALUE (void *)-1   // EMS003

#define MODULO_OS2_DIPENDENTE
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS          // Per i mnemonici degli errori
// EMS004 tolgo define INCL_DOS che non serve e provoca errori con Windows
//#define INCL_DOS
#define INCL_DOSSEMAPHORES   /* Semaphore values */

//#define DBG   // Per debug : dati file (normalmente OK, si attiva solo ai livelli alti)
//#define DBG2  // Per debug : particolari della ricerca dicotomica ed altro
//#define DBG3  // Per debug : particolari della fase di sort/add del FILE_BS
//#define DBG4  // Istruzioni di controllo riscrittura dei files

#include <string.h>     // EMS005 VA
#include <direct.h>
// EMS005 VA #include <dir.h>
#include <stdlib.h>
#include <stdarg.h>
#include "std.h"
#include "BASE.HPP"
#include "file_rw.HPP"
// EMS005 VA #include "scandir.H"
#include <time.h>
#include <sys\utime.h>

#ifdef OKTRACE
#define TESTTRACE(a) _Abil2(a)  // trace  condizionato
#else
#define TESTTRACE(a) FALSE
#endif

const char* FNO = "Errore: file non aperto";

#define max(a,b) ( (a) > (b) ? (a) : (b) )
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#define min3(a,b,c) ( (a) < (b) ? min(a,c) : min(b,c) )

//----------------------------------------------------------------------------
// Questo e' l' Header che viene messo sui file FIX e sui file BS:
// E' opzionale, e la sua presenza e' riconosciuta dai primi 4 bytes
// (0XFAFBFCFD)
// Se presente viene controllata la dimensione del record e della chiave.
//----------------------------------------------------------------------------
#define HDR_OK 0XFAFBFCFD
struct FIX_HEADER {
   DWORD   Hdr;                    // Deve essere 0XFAFBFCFD
   WORD    Tipo;                   // 1= FIX 2=BS
   ULONG   RecLen;                 // Record Length
   ULONG   KeyLen;                 // Key Length
   ULONG   HdrLen;                 // Lunghezza totale dell' Header
   char    DescrizioneHeader[40];  // Descrizione del record
   // Qui ci starebbe bene la descrizione dei campi ma per ora no
};


//----------------------------------------------------------------------------
// F_DTM Struttura per rappresentare Data ed ora dei files
//----------------------------------------------------------------------------
// Occupa 4 Bytes

//----------------------------------------------------------------------------
// F_DTM::TimeStamp
//----------------------------------------------------------------------------
STRINGA F_DTM::TimeStamp(){
   #undef TRCRTN
   #define TRCRTN "F_DTM::TimeStamp"
   return Data() + " " + Ora();
};

//----------------------------------------------------------------------------
// F_DTM::Ora
//----------------------------------------------------------------------------
STRINGA F_DTM::Ora(){
   #undef TRCRTN
   #define TRCRTN "F_DTM::Ora"
   char Tmp[64];
   sprintf(Tmp,"%2.2u:%2.2u:%2.2u",Ore,Minuti,Secondi());
   return STRINGA(Tmp);
}

//----------------------------------------------------------------------------
// F_DTM::Data
//----------------------------------------------------------------------------
STRINGA F_DTM::Data(){
   #undef TRCRTN
   #define TRCRTN "F_DTM::Data"
   char Tmp[64];
   sprintf(Tmp,"%2.2u/%2.2u/%4.4u",Giorno,Mese,Anno());
   return STRINGA(Tmp);
}
//----------------------------------------------------------------------------
// Bisestile
//----------------------------------------------------------------------------
inline BOOL Bisestile(int Anno){
   #undef TRCRTN
   #define TRCRTN "Bisestile"
// Bisestile: anno divisibile per 4, non divisibile per 100 oppure divisibile per 400
   return (!(Anno % 4)) && ((Anno % 100) || !(Anno % 400) );
};

static char limitiMese[12]={31,29,31,30,31,30,31,31,30,31,30,31};

//----------------------------------------------------------------------------
// ++
//----------------------------------------------------------------------------
F_DTM & F_DTM::operator ++(){
   #undef TRCRTN
   #define TRCRTN "++"
   Giorno ++; // Modulo 32 !
   if (Giorno == 0 || Giorno > limitiMese[Mese-1] || (Giorno == 29 && Mese == 2 && !Bisestile(Anno())) ) {
      Giorno = 1;
      Mese ++;
   } /* endif */

   if(Mese > 12){
      Mese = 1;
      _Anno ++;
   };
   return THIS;
};

//----------------------------------------------------------------------------
// --
//----------------------------------------------------------------------------
F_DTM & F_DTM::operator --(){
   #undef TRCRTN
   #define TRCRTN "--"
   Giorno --;
   if (Giorno == 0 ) {
      Mese --;
      if(Mese == 0){
         _Anno --;
         Mese = 12;
         Giorno = 31;
      } else {
         Giorno = limitiMese[Mese - 1];
         if (Giorno == 29 && Mese == 2 && !Bisestile(Anno()))Giorno --;
      } /* endif */
   } /* endif */
   return THIS;
};
//----------------------------------------------------------------------------
// F_DTM::Adesso
//----------------------------------------------------------------------------
void F_DTM::Adesso(){
   #undef TRCRTN
   #define TRCRTN "F_DTM::Adesso"
   DATETIME Now;
   DosGetDateTime(&Now);
   Sec2       = Now.seconds / 2;
   Minuti     = Now.minutes    ;
   Ore        = Now.hours      ;
   Giorno     = Now.day        ;
   Mese       = Now.month      ;
   _Anno      = Now.year - 1980;
};
//----------------------------------------------------------------------------
// F_DTM::AddMinuti
//----------------------------------------------------------------------------
F_DTM& F_DTM::AddMinuti(USHORT Mn){ // Aggiunge un dato numero di minuti alla data ed ora
   #undef TRCRTN
   #define TRCRTN "F_DTM::AddMinuti"

   ULONG  Minutes = Minuti ;
   Minutes += Mn ;
   Minuti = Minutes % 60;
   if(Minutes > 60){
      ULONG Hours = Ore;
      Hours += Minutes / 60;
      Ore = Hours % 24;
      for (ULONG Days = Hours / 24; Days > 0  ;Days -- ) {
         ++THIS;
      } /* endfor */
   }
   return THIS;
}

// Il costruttore esegue la ricerca su directory
//----------------------------------------------------------------------------
// @RO_INFO_LIST
//----------------------------------------------------------------------------
RO_INFO_LIST::RO_INFO_LIST(const STRINGA & PatternDaCercare, BOOL AncheHidden, BOOL AncheDirectories){
   #undef TRCRTN
   #define TRCRTN "@RO_INFO_LIST"

/* EMS006 VA RO_INFO_LIST::RO_INFO_LIST() non serve a nessuno e mi da problemi di compilazione
   TRACESTRING("Ricerca su pattern:"+PatternDaCercare);
   Pattern = PatternDaCercare;
   AnalizeFileName(PatternDaCercare,Path);
   Size = 0;
   Dati = 0;

   HDIR          FindHandle=1;
   BYTE *        FindBuffer = (BYTE*)malloc(10000); // Leggo 10 k per volta
   USHORT        NumeroBuffersAllocati = 0;
   BYTE          * BufferPool[100]; // Max 1 mega ram
   ULONG         FindCount = 300; // Leggo max 300 files per volta
   ULONG         Attributi = FILE_ARCHIVED | FILE_READONLY; // Include sempre i files archiviati e readonly
   if(AncheHidden)Attributi |= FILE_SYSTEM | FILE_HIDDEN;
   if(AncheDirectories)Attributi |= FILE_DIRECTORY;
   Rc = DosFindFirst((PSZ)(CPSZ)PatternDaCercare, &FindHandle, Attributi, (PVOID) FindBuffer,10000, &FindCount, 1);
   if(Rc == ERROR_NO_MORE_FILES){
      TRACESTRING("Nessun file trovato");
      free(FindBuffer);
      DosFindClose(FindHandle);
      return;
   }
   //TRACEVLONG(Rc);
   //TRACEVLONG(FindCount);
   if(!Rc){
      Size += FindCount;
      FILEFINDBUF3 * Cursore = (FILEFINDBUF3 *)FindBuffer;
      for(int k = 0; k < FindCount; k++) Cursore = (FILEFINDBUF3 *)((BYTE*)Cursore + Cursore->oNextEntryOffset);
      if(((BYTE*)Cursore - FindBuffer) < (10000 - sizeof(FILEFINDBUF3)))Rc = ERROR_NO_MORE_FILES;
      for (BufferPool[NumeroBuffersAllocati++] = FindBuffer;
         Rc == 0 && NumeroBuffersAllocati< 100 ;
         BufferPool[NumeroBuffersAllocati++] = FindBuffer ) {
         // TRACEVLONG(NumeroBuffersAllocati);
         if(Rc)break; // Errore !
         FindBuffer = (BYTE*)malloc(10000); // Leggo 10 k per volta
         FindCount = 300;
         Rc = DosFindNext(FindHandle, (PVOID) FindBuffer, 10000, &FindCount);
         Size += FindCount;
         //TRACEVLONG(Rc);
         //TRACEVLONG(FindCount);
         for(int k = 0; k < FindCount; k++) Cursore = (FILEFINDBUF3 *)((BYTE*)Cursore + Cursore->oNextEntryOffset);
         if((BYTE*)Cursore - FindBuffer < (10000 - sizeof(FILEFINDBUF3)))Rc = ERROR_NO_MORE_FILES;
      } //
   };
   DosFindClose(FindHandle);

   if(Rc && Rc != ERROR_NO_MORE_FILES){ // Errore
      ERRSTRING("Errore nella ricerca: "+STRINGA(DecodificaRcOS2(Rc)));
      Size = 0;
   } else {
      Rc = 0;
      // Alloco l' area di output
      if(Size)Dati = new RO_INFO[Size];
      Size = 0;
      for (int i = 0;i < NumeroBuffersAllocati ;i++ ) {
         FILEFINDBUF3 * Cursore = (FILEFINDBUF3 *)BufferPool[i];
         do {
            if(Cursore->achName[0] != 0){ // Per i buffers con 0 files
               // TRACESTRING("File["+STRINGA(Size)+"] = "+STRINGA(Cursore->achName));
               // TRACEVLONG(Cursore->oNextEntryOffset);
               FILEFINDBUF3 & Bf= *Cursore;
               RO_INFO & Out = Dati[Size++];
               strncpy(Out.Nome,Bf.achName,40);
               Out.Nome[39]=0;
               Out.Size = Bf.cbFile;
               memmove (&Out.Data,&Bf.ftimeLastWrite,2);
               memmove (((char*)&Out.Data)+2,&Bf.fdateLastWrite,2);
               Out.Hidden     = Bf.attrFile & FILE_HIDDEN;
               Out.System     = Bf.attrFile & FILE_SYSTEM;
               Out.ReadOnly   = Bf.attrFile & FILE_READONLY;
               Out.Archived   = Bf.attrFile & FILE_ARCHIVED;
               Out.Directory  = Bf.attrFile & FILE_DIRECTORY;
               // Out.Data.Sec2    = Bf.ftimeLastWrite.twosecs ;
               // Out.Data.Minuti  = Bf.ftimeLastWrite.minutes ;
               // Out.Data.Ore     = Bf.ftimeLastWrite.hours   ;
               // Out.Data.Giorno  = Bf.fdateLastWrite.day     ;
               // Out.Data.Mese    = Bf.fdateLastWrite.month   ;
               // Out.Data._Anno   = Bf.fdateLastWrite.year    ;
             }
             if(!Cursore->oNextEntryOffset)break;
             Cursore = (FILEFINDBUF3 *)((BYTE*)Cursore + Cursore->oNextEntryOffset);
         } while (TRUE);
      }
   };
   TRACESTRING("Trovati "+STRINGA(Size)+" Files");
   for (int i = 0;i < NumeroBuffersAllocati ;i++ ) {
      free(BufferPool[i]);
   }
*/
};
//----------------------------------------------------------------------------
// ~RO_INFO_LIST
//----------------------------------------------------------------------------
RO_INFO_LIST::~RO_INFO_LIST(){
   #undef TRCRTN
   #define TRCRTN "~RO_INFO_LIST"
   if(Dati) delete Dati;
};
//----------------------------------------------------------------------------
// FW_SName
//----------------------------------------------------------------------------
int FW_SName( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "FW_SName"
   RO_INFO & A = *(RO_INFO*) a;
   RO_INFO & B = *(RO_INFO*) b;
   return strcmp(A.Nome,B.Nome);
};
//----------------------------------------------------------------------------
// FW_SData
//----------------------------------------------------------------------------
int FW_SData( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "FW_SData"
   RO_INFO & A = *(RO_INFO*) a;
   RO_INFO & B = *(RO_INFO*) b;
   if(A.Data > B.Data)return 1;
   if(A.Data == B.Data)return 0;
   return -1;
};
//----------------------------------------------------------------------------
// FW_SSize
//----------------------------------------------------------------------------
int FW_SSize( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "FW_SSize"
   RO_INFO & A = *(RO_INFO*) a;
   RO_INFO & B = *(RO_INFO*) b;
   return int(A.Size) - int(B.Size);
};
//----------------------------------------------------------------------------
// RO_INFO_LIST::SortNome
//----------------------------------------------------------------------------
void RO_INFO_LIST::SortNome(){
   #undef TRCRTN
   #define TRCRTN "RO_INFO_LIST::SortNome"
   if(Dati)qsort(Dati,Size,sizeof(RO_INFO),FW_SName);
};
//----------------------------------------------------------------------------
// RO_INFO_LIST::SortData
//----------------------------------------------------------------------------
void RO_INFO_LIST::SortData(){
   #undef TRCRTN
   #define TRCRTN "RO_INFO_LIST::SortData"
   if(Dati)qsort(Dati,Size,sizeof(RO_INFO),FW_SData);
};
//----------------------------------------------------------------------------
// RO_INFO_LIST::SortSize
//----------------------------------------------------------------------------
void RO_INFO_LIST::SortSize(){
   #undef TRCRTN
   #define TRCRTN "RO_INFO_LIST::SortSize"
   if(Dati)qsort(Dati,Size,sizeof(RO_INFO),FW_SSize);
};

//----------------------------------------------------------------------------
// FILE_RO
//----------------------------------------------------------------------------
FILE_RO::FILE_RO(const char * NomeFile){
   #undef TRCRTN
   #define TRCRTN "@FILE_RO"

   BOOL RwFlag = FALSE;
   rc = 0;
   ULONG ActionTaken;
   Handle = NULL;
   if(RwFlag){
      OpenAction = OPEN_ACTION_OPEN_IF_EXISTS|OPEN_ACTION_CREATE_IF_NEW;
      OpenFlags  = OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYWRITE ;  // Open Flags OS2
      TRACESTRINGL("=======================================================",1);
      TRACESTRINGL("=======================================================",1);
      TRACESTRINGL("        ATTENZIONE: FILE "+STRINGA(NomeFile)+" APERTO IN SCRITTURA!",1);
      TRACESTRINGL("=======================================================",1);
      TRACESTRINGL("=======================================================",1);
   } else {
      OpenAction = OPEN_ACTION_OPEN_IF_EXISTS;                         // Open Action OS2
      OpenFlags  = OPEN_SHARE_DENYNONE | OPEN_FLAGS_RANDOMSEQUENTIAL ; // Open Flags OS2
   }
   Nome = NomeFile;
   FSize = 0;
   Open();
   TRACESTRING("====================>>");
   TRACESTRINGL("Apertura file "+Nome+STRINGA(" Handle=")+STRINGA(Handle)+
      " Size="+STRINGA(FileSize())+" Timestamp="+FTms.TimeStamp(),1);
//<<< FILE_RO::FILE_RO(const char * NomeFile,BOOL RwFlag){
};

//----------------------------------------------------------------------------
// FILE_RO::Open
//----------------------------------------------------------------------------
WORD FILE_RO::Open(){
   #undef TRCRTN
   #define TRCRTN "FILE_RO::Open"

   rc = 0;
   if(Handle) return (WORD)0;
   ULONG ActionTaken;

   rc= DosOpen((PSZ)(CPSZ)Nome, // Nome del file
      &Handle,       // Handle se viene trovato
      &ActionTaken,  // Indica l' azione fatta da OS2
      0L ,           // Create File Size
      FILE_NORMAL ,  // Attributi: non significativi
      OpenAction,OpenFlags,  // Parametri di apertura
      0L);           // Reserved


   switch (rc)
   {
   case NO_ERROR                :
      // TRACELONG("Aperto file "+Nome+STRINGA(" Handle="),Handle);
      break;
   case ERROR_OPEN_FAILED:
      ERRSTRING("WARNING (File non preesistente) aprendo file "+Nome+DecodificaRcOS2(rc));
      break;
   default:
      ERRSTRING("Errore (Ri)aprendo file "+Nome+DecodificaRcOS2(rc));
      BEEP;
      // begin EMS007 VA l'handle invalido deve valere INVALID_HANDLE_VALUE e non NULL
      // Handle = NULL;
      Handle = (HFILEOS2) INVALID_HANDLE_VALUE; // Segnala file illegale
      // end EMS007 VA
      break;
   }
   return rc;
//<<< WORD FILE_RO::Open(){
};

//----------------------------------------------------------------------------
// FILE_RO::Close
//----------------------------------------------------------------------------
void FILE_RO::Close(){ // Esegue la Close
   #undef TRCRTN
   #define TRCRTN "FILE_RO::Close"

   rc = 0;
   if(Handle){
      rc =  DosClose(Handle);
      Handle = NULL;
      switch (rc)
      {
      case NO_ERROR  :
         // TRACELONG("Chiuso file "+Nome+STRINGA(" Handle="),Handle);
         break;
      default:
         ERRSTRING("Errore Chiudendo file "+Nome+DecodificaRcOS2(rc));
         BEEP;
         break;
      }

   }
   return;
//<<< void FILE_RO::Close(){ // Esegue la Close
};

//----------------------------------------------------------------------------
// ~FILE_RO
//----------------------------------------------------------------------------
FILE_RO::~FILE_RO(){ // Esegue la Close
   #undef TRCRTN
   #define TRCRTN "~FILE_RO"
   if(Handle){
      TRACELONG("Chiudo il file "+Nome+STRINGA(" Handle="),Handle);
      Close();
   };
};

//----------------------------------------------------------------------------
// FILE_RO::FileSize
//----------------------------------------------------------------------------
ULONG FILE_RO::FileSize(){ //
   #undef TRCRTN
   #define TRCRTN "FILE_RO::FileSize"

   rc = 0;
   if(FSize)return FSize;
   HFILEOS2 OldHandle = Handle;
   if(!Handle)Open();
   if(!FSize){
      FILESTATUS FileInfo;
      TRACEVLONG(Handle);
      rc=DosQueryFileInfo(Handle, 1, (PBYTE)&FileInfo, sizeof(FileInfo));
      TRACELONG("rc da DosQueryFileInfo",rc);
      if (rc) {
         memset(&FTms,0,sizeof(FTms));
         FSize= 0;
      } else {
         FSize= FileInfo.cbFile;
         memmove (&FTms,&FileInfo.ftimeLastWrite,2);
         memmove (((char*)&FTms)+2,&FileInfo.fdateLastWrite,2);
         // FTms.Sec2    = FileInfo.ftimeLastWrite.twosecs ;
         // FTms.Minuti  = FileInfo.ftimeLastWrite.minutes ;
         // FTms.Ore     = FileInfo.ftimeLastWrite.hours   ;
         // FTms.Giorno  = FileInfo.fdateLastWrite.day     ;
         // FTms.Mese    = FileInfo.fdateLastWrite.month   ;
         // FTms._Anno   = FileInfo.fdateLastWrite.year    ;
      } /* endif */
      //   TRACELONG("Size del file "+Nome+STRINGA(" ="),FSize);
      TRACELONG("LA FSize: ",FSize);
   }
   if(!OldHandle)Close();
   return FSize;
//<<< ULONG FILE_RO::FileSize(){ //
};
//----------------------------------------------------------------------------
// FILE_RO::FileTms
//----------------------------------------------------------------------------
F_DTM FILE_RO::FileTms(){
   #undef TRCRTN
   #define TRCRTN "FILE_RO::FileTms"

   if(FSize==0)FileSize(); // Legge anche l' ora
   return FTms;
};

//----------------------------------------------------------------------------
// FILE_RO::Posiziona
//----------------------------------------------------------------------------
BOOL  FILE_RO::Posiziona(ULONG Posizione){  // Posiziona il file (FALSE se errore)
   #undef TRCRTN
   #define TRCRTN "FILE_RO::Posiziona"
   ULONG   ulMoveType = 0; // Dall' inizio
   ULONG   NewPointer;

   rc = 0;
   #ifdef DBG
   if(TESTTRACE(7))TRACELONG(Nome + STRINGA(": Posizionamento al Byte "),Posizione); // Trace a livello 7
   #endif
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return FALSE;
   };

   rc = DosSetFilePtr(Handle, Posizione, ulMoveType, &NewPointer);

   if(rc){
      ERRSTRING("Errore posizionando il file "+ Nome+DecodificaRcOS2(rc));
      ERRINT("Posizione richiesta=",Posizione);
      ERRINT("Posizione effettiva=",NewPointer);
      // BEEP;
   };

   return !rc;
//<<< BOOL  FILE_RO::Posiziona(ULONG Posizione){  // Posiziona il file (FALSE se errore)
};
//----------------------------------------------------------------------------
// FILE_RO::Posiziona
//----------------------------------------------------------------------------
BOOL  FILE_RO::Posiziona(){  // Posiziona il file alla fine
   #undef TRCRTN
   #define TRCRTN "FILE_RO::Posiziona (Alla fine)"

   rc = 0;
   #ifdef DBG
   if(TESTTRACE(5))TRACESTRING(Nome + STRINGA("Posizionamento a fine file"));
   #endif
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return FALSE;
   };

   ULONG   ulMoveType = 2; // Alla fine
   ULONG   NewPointer;

   rc = DosSetFilePtr(Handle, 0, ulMoveType, &NewPointer);

   if(rc){
      ERRSTRING("Errore posizionando il file "+ Nome+DecodificaRcOS2(rc));
      ERRINT("Posizione effettiva=",NewPointer);
   };

   return !rc;
//<<< BOOL  FILE_RO::Posiziona(){  // Posiziona il file alla fine
};

//----------------------------------------------------------------------------
// FILE_RO::Leggi
//----------------------------------------------------------------------------
// Legge i dati e li mette in un BUFR (allocato con new)
// in caso di errore : ritorna NULL
BUFR * FILE_RO::Leggi(ULONG NumBytes){                 // Legge sequenzialmente
   #undef TRCRTN
   #define TRCRTN "FILE_RO::Leggi"

   BUFR * Out = new BUFR(NumBytes); // Alloco un nuovo buffer
   BOOL Result = Leggi(Out->Dati,NumBytes,Out->Length);
   if((!Result) || rc){
      delete Out;
      Out = NULL;
   }
   return Out;
};
BUFR * FILE_RO::Leggi(ULONG NumBytes,ULONG Posizione){ // Posiziona e legge
   if(!Posiziona(Posizione))return NULL;
   return Leggi(NumBytes);
};
BOOL FILE_RO::Leggi(ULONG NumBytes,BUFR & Out){                 // Legge sequenzialmente
   if(Out.Alloc < NumBytes)Out.ReDim(NumBytes);
   return Leggi(Out.Dati,NumBytes,Out.Length);
};
BOOL FILE_RO::Leggi(ULONG NumBytes,ULONG Posizione,BUFR & Out){ // Posiziona e legge
   if(!Posiziona(Posizione))return NULL;
   return Leggi(NumBytes,Out);
};
// Legge i dati e li mette in un' area esistente.
// Ritorna opzionalmente il numero di bytes letti
BOOL FILE_RO::Leggi( void * Dati, ULONG NumBytes, ULONG & PBytesLetti){
   rc = 0;
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return FALSE;
   };

   ULONG BytesLetti;
   rc = DosRead(Handle, Dati,NumBytes,&BytesLetti);
   if(&PBytesLetti) PBytesLetti = BytesLetti;
   if(rc){
      ERRSTRING("Errore leggendo il file "+ Nome+DecodificaRcOS2(rc));
      return FALSE;
   } else {
      #ifdef DBG
      TRACEHEXL(Nome + STRINGA(" Letti dati : Segue Dump "),Dati,BytesLetti,7);
      #endif
   };
   if(BytesLetti < NumBytes){
      TRACESTRINGL("Fine file leggendo il file "+ Nome+DecodificaRcOS2(rc),1);
      TRACEINTL("Letti solo bytes nø:",BytesLetti,1);
   };
   return TRUE;
}
//----------------------------------------------------------------------------
// FILE_RO::PosizioneCorrente
//----------------------------------------------------------------------------
ULONG FILE_RO::PosizioneCorrente(){
   #undef TRCRTN
   #define TRCRTN "FILE_RO::PosizioneCorrente()"
   ULONG  Pos;
   rc = 0;
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return 0;
   };
   rc = DosSetFilePtr ( Handle,  0, 1 , &Pos);
   if(rc){
      ERRSTRING("Errore file "+ Nome+DecodificaRcOS2(rc));
      return 0;
   };
   return Pos;
};


//----------------------------------------------------------------------------
// FILE_RO   - Funzioni analoghe a quelle del c
//----------------------------------------------------------------------------

// begin EMS008 VA elimino funzione FILE_RO::scanf inutile e non compilabile
/*
//----------------------------------------------------------------------------
// FILE_RO::scanf
//----------------------------------------------------------------------------
int  FILE_RO::scanf(const char * Format, ...){
   #undef TRCRTN
   #define TRCRTN "FILE_RO::scanf"
   BUFR * Buf = gets();
   if (Buf == NULL || Buf->Dim() == 0) {
      TRACESTRINGL("Causa Errore o fine file Non posso leggere dati",1);
      return 0;
   } /* endif *
   va_list  argptr;
   int cnt;
   va_start(argptr, Format);
   cnt = vsscanf(Buf->Dati, Format, argptr);
   va_end(argptr);
   return(cnt);
};
*/
// end EMS008 VA

//----------------------------------------------------------------------------
// FILE_RO::gets
//----------------------------------------------------------------------------
char  * FILE_RO::gets(char * Buffer, int BufferDim){ // Legge i dati alla posizione attuale
   #undef TRCRTN
   #define TRCRTN "FILE_RO::gets"
   rc = 0;
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return NULL;
   };
   ULONG Pos = PosizioneCorrente();
   ULONG  Le;

   rc = DosRead(Handle, Buffer,BufferDim-1,&Le);
   if(rc){
      ERRSTRING("Errore leggendo il file "+ Nome+DecodificaRcOS2(rc));
      return NULL;
   } else if(Le == 0 || *Buffer == 0x1A ){ // EOF
      return NULL;
   } else {
      #ifdef DBG
      TRACEHEXL(Nome + STRINGA(" Letti dati : Segue Dump "),Buffer,Le,6);
      #endif
   };
   char * c = strchr(Buffer,'\n');
   int Dim1;
   if (c == NULL)c = Buffer+Le; // Non trovato fine linea
   Dim1 = c - Buffer;
   if(*(c-1) == 0x1a)c--;
   if(*(c-1) == '\r')c--;
   *c = 0;
   Posiziona(Pos+Dim1+1);
   return Buffer;
//<<< char  * FILE_RO::gets(char * Buffer, int BufferDim){ // Legge i dati alla posizione attuale
};
BOOL FILE_RO::gets(STRINGA & Linea,int MaxLinea){
// Legge una linea alla posizione attuale e la torna come stringa; Torna FALSE ad EOF od errore
   BUFR * Tmp = gets(MaxLinea);
   if (Tmp) {
      Linea = STRINGA((char*)Tmp->Dati);
      delete Tmp;
      return TRUE;
   } else {
      Linea = "";
      return FALSE;
   } /* endif */
};
BUFR * FILE_RO::gets(int MaxLinea){ // Legge i dati alla posizione attuale
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return NULL;
   };
   ULONG Pos = PosizioneCorrente();
   BUFR * Buf = new BUFR(MaxLinea); // Alloco un nuovo buffer
   int rc = DosRead(Handle, Buf->Dati,Buf->Alloc,&Buf->Length);
   if(rc){
      ERRSTRING("Errore leggendo il file "+ Nome+DecodificaRcOS2(rc));
      delete Buf;
      return NULL;
   };
   if (Buf->Dim() == 0 || (*Buf)[0] == 0x1A ){ // EOF
      TRACESTRINGL("Arrivato alla fine del file "+Nome,1);
      delete Buf;
      return NULL;
   } /* endif */
   // EMS009 12.5.97 inserito cast (char *)
   char * c = strchr((char *)Buf->Dati,'\n');
   int Dim1;
   // EMS009 VA inserito cast char *
   if (c == NULL)c = (char *) (Buf->Dati + Buf->Dim());  // Non trovato fine linea
   // EMS009 VA inserito cast char *
   Dim1 = c - ((char *) Buf->Dati);
   int Dim2 = Dim1;
   if(*(c-1) == 0x1a)Dim2--,c--;
   if(*(c-1) == '\r')Dim2--,c--;
   *c = 0;
   Buf->ReDim(Dim2);
   Posiziona(Pos+Dim1+1);
   return Buf;
//<<< BUFR * FILE_RO::gets(){ // Legge i dati alla posizione attuale
};

//----------------------------------------------------------------------------
// FILE_RW
//----------------------------------------------------------------------------
FILE_RW::FILE_RW(const char * NomeFile,BOOL Lazy): FILE_RO(){ // Esegue direttamente la Open
   #undef TRCRTN
   #define TRCRTN "@FILE_RW"

   Handle = NULL;
   OpenAction = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;  // Open Action OS2
   OpenFlags  = OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE      ;  // Open Flags OS2
   if(!Lazy) OpenFlags |= OPEN_FLAGS_WRITE_THROUGH;
   Nome = NomeFile;
   FSize = 0;
   Open();
   TRACELONG("Apertura file "+Nome+STRINGA(" Handle="),Handle);
};

//----------------------------------------------------------------------------
// FILE_RW::Scrivi
//----------------------------------------------------------------------------
BOOL FILE_RW::Scrivi(const BUFR & In){        // Alla posizione corrente
   #undef TRCRTN
   #define TRCRTN "FILE_RW::Scrivi"

   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return FALSE;
   };
   return Scrivi(In.Dati,In.Length);
};
BOOL FILE_RW::Scrivi(const void * Dati, ULONG Dim){        // Alla posizione corrente
   #undef TRCRTN
   #define TRCRTN "FILE_RW::Scrivi"

   rc = 0;
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return FALSE;
   };
   ULONG  BytesWritten;

   #ifdef DBG
   TRACEHEXL(Nome + STRINGA(" Sto per scrivere dati : Segue Dump "),(void*)Dati,Dim,7);
   #endif
   rc = DosWrite(Handle, (void*)Dati, Dim, &BytesWritten);
   FSize = 0; // Potrebbe essere cambiata
   if(rc){
      ERRSTRING("Errore scrivendo il file "+ Nome+DecodificaRcOS2(rc));
      return FALSE;
   };
   if(Dim > BytesWritten ){
      ERRSTRING("Scritti solo parzialmente i dati:");
      ERRINT("Richiesti bytes nø:",Dim);
      TRACEVLONGL(BytesWritten,1);
      return FALSE;
   };
   return TRUE;
};
//----------------------------------------------------------------------------
// FILE_RW::SetSize
//----------------------------------------------------------------------------
BOOL FILE_RW::SetSize(ULONG NewSize){
   #undef TRCRTN
   #define TRCRTN "FILE_RW::SetSize"
   rc = 0;
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return FALSE;
   };
   #ifdef DBG
   TRACELONG(Nome + STRINGA(" Sto per impostare il size del file a:"),NewSize);
   #endif
   rc = DosSetFileSize(Handle, NewSize);
   if(rc){
      ERRSTRING("Errore impostando dimensione del file "+ Nome+DecodificaRcOS2(rc));
      ERRINT("NewSize   =",NewSize   );
      ERRINT("FileSize()=",FileSize());
   };
   FSize= 0;
   FileSize();

   ULONG Pos = PosizioneCorrente();
   if (Pos >= FSize ) {
      Posiziona(); // Va alla fine del file
   } /* endif */

   return !rc;
};
//----------------------------------------------------------------------------
// FILE_RW::SetTms
//----------------------------------------------------------------------------
BOOL FILE_RW::SetTms(F_DTM Tms){
   #undef TRCRTN
   #define TRCRTN "FILE_RW::SetTms"

// EMS010 VA non avendo le funzioni di libreria per la modifica
// dell'ora ultima modifica di un file ovvero non potendo implementare la
// DosSetFileInfo, vado a modificare direttamente questa funzione

#ifdef __WINDOWS__

   struct tm tmData_e_ora_da_impostare;
   time_t timetData_e_ora_da_impostare;
   struct utimbuf utbBuffer_per_funzione_utime;

   tmData_e_ora_da_impostare.tm_sec   = Tms.Sec2 * 2;
   tmData_e_ora_da_impostare.tm_min   = Tms.Minuti;
   tmData_e_ora_da_impostare.tm_hour  = Tms.Ore;
   tmData_e_ora_da_impostare.tm_mday  = Tms.Giorno;
   tmData_e_ora_da_impostare.tm_mon   = Tms.Mese - 1;
   tmData_e_ora_da_impostare.tm_year  = Tms._Anno + 80;
   tmData_e_ora_da_impostare.tm_wday  = 0;
   tmData_e_ora_da_impostare.tm_yday  = 0;
   tmData_e_ora_da_impostare.tm_isdst = 0;

   timetData_e_ora_da_impostare = mktime(&tmData_e_ora_da_impostare);

   if (timetData_e_ora_da_impostare == -1) {
      char cMsg[200];
      sprintf(cMsg, "%02d/%02d/%04d %02d:%02d:%02d wday=%d yday=%d isdst=%d",
            tmData_e_ora_da_impostare.tm_mday,
            tmData_e_ora_da_impostare.tm_mon + 1,
            tmData_e_ora_da_impostare.tm_year + 1900,
            tmData_e_ora_da_impostare.tm_hour,
            tmData_e_ora_da_impostare.tm_min,
            tmData_e_ora_da_impostare.tm_sec,
            tmData_e_ora_da_impostare.tm_wday,
            tmData_e_ora_da_impostare.tm_yday,
            tmData_e_ora_da_impostare.tm_isdst);
      ERRSTRING("Errore in conversione data ora");
      ERRSTRING(cMsg);
      BEEP;
      return FALSE;
   }

   utbBuffer_per_funzione_utime.actime  = 0;
   utbBuffer_per_funzione_utime.modtime = timetData_e_ora_da_impostare;

   STRINGA Nome_con_drive = "C:"+Nome;
   if (utime((char *)((const char *) Nome_con_drive), &utbBuffer_per_funzione_utime) == -1) {
      ERRINT("utime fallita errno=", errno);
      ERRSTRING(Nome_con_drive);
      return FALSE;
   }

   return TRUE;

#else // ifdef __WINDOWS__

   rc = 0;
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return FALSE;
   };
   FILESTATUS  FileInfoBuf;     /* File info buffer */
   DATETIME    DateTimeBuf;     /* Date/Time buffer */

   rc = DosQueryFileInfo(Handle, 1, &FileInfoBuf, sizeof(FILESTATUS));
   if (rc != 0){
      ERRINT("Errore DosQueryFileInfo File "+Nome+" RC = ",rc);
      BEEP;
      return FALSE;
   };
   FileInfoBuf.fdateLastWrite.year    = Tms._Anno;
   FileInfoBuf.fdateLastWrite.month   = Tms.Mese;
   FileInfoBuf.fdateLastWrite.day     = Tms.Giorno;
   FileInfoBuf.ftimeLastWrite.hours   = Tms.Ore;
   FileInfoBuf.ftimeLastWrite.minutes = Tms.Minuti;
   FileInfoBuf.ftimeLastWrite.twosecs = Tms.Sec2;
   rc = DosSetFileInfo(Handle, 1, &FileInfoBuf, sizeof(FILESTATUS));
   if (rc != 0){
      ERRINT("Errore DosSetFileInfo File "+Nome+" RC = ",rc);
      BEEP;
      return FALSE;
   };
   return TRUE;

#endif  // ifdef __WINDOWS__

//<<< BOOL FILE_RW::SetTms(F_DTM Tms){
};
//----------------------------------------------------------------------------
// FILE_RW::Rename
//----------------------------------------------------------------------------
BOOL FILE_RW::Rename(const char* NomeFile){
   #undef TRCRTN
   #define TRCRTN "FILE_RW::Rename"

   rc = 0;
   HFILEOS2 OldHandle = Handle;
   if(Handle)Close();

   rc = DosMove((PSZ)(CPSZ)Nome, (PSZ)NomeFile);

   if (rc != 0) {
      ERRINT("Errore DosMove File "+Nome+" -> "+STRINGA(NomeFile)+" RC = ",rc);
      BEEP;
      return FALSE;
   }

   Nome = NomeFile;
   if(OldHandle){
      Open();
      // Controllo che la Open abbia avuto successo
      if(!Handle){
         ERRSTRING("Errore: Rename OK ma successiva apertura fallita!");
         return FALSE;
      }
   }

   return TRUE;
//<<< BOOL FILE_RW::SetTms(F_DTM Tms){
};
//----------------------------------------------------------------------------
// FILE_RW   - Funzioni analoghe a quelle del c
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// FILE_RW::printf
//----------------------------------------------------------------------------
int  FILE_RW::printf(const char * Format, ...){    // Scrive alla posizione corrente;
   #undef TRCRTN
   #define TRCRTN "FILE_RW::printf"

   rc = 0;
   if(!Handle){
      ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
      BEEP;
      return 0;
   };

   va_list argptr;
   int cnt;
   char Buffer[1024];
   va_start(argptr, Format);
   cnt = vsprintf(Buffer, Format, argptr);
   va_end(argptr);
   if(cnt > 0){
      ULONG  BytesWritten;

      #ifdef DBG
      TRACEHEXL(Nome + STRINGA(" Sto per scrivere dati : Segue Dump "),Buffer,cnt,7);
      #endif
      rc = DosWrite(Handle, Buffer,cnt, &BytesWritten);
      FSize = 0; // Potrebbe essere cambiata
      if(rc){
         ERRSTRING("Errore scrivendo il file "+ Nome+DecodificaRcOS2(rc));
         cnt=0;
      };
      if(cnt > BytesWritten ){
         ERRSTRING("Scritti solo parzialmente i dati:");
         ERRINT("Richiesti bytes nø:",cnt);
         TRACEVLONGL(BytesWritten,1);
      };
   };
   return(cnt);
//<<< int  FILE_RW::printf(const char * Format, ...){    // Scrive alla posizione corrente;
};
//----------------------------------------------------------------------------
// FILE_FIX
//----------------------------------------------------------------------------
FILE_FIX::FILE_FIX( const char* NomeFile,ULONG RecLen,ULONG PgSize):
FILE_RO(NomeFile)
, RecordLength  (RecLen)
, PageSize      (RecLen * ((PgSize+RecLen-1)/RecLen))
{
   #undef TRCRTN
   #define TRCRTN "@FILE_FIX"
   if(PgSize == 0) { // Il buffer deve essere dimensionato in modo da tenere in memoria tutto il file
      PgSize = FileSize();
      TRACEVLONG(PgSize);
      if(PgSize == 0)PgSize = 4096; // Ad evitare abends stupidi se il file non esiste
      ULONG & Pgs = ( ULONG &)PageSize;
      TRACEVLONG(PgSize);
      Pgs =     RecLen * ((PgSize+RecLen-1)/RecLen);
      TRACEVLONG(PageSize);
   }

   BOOL RwFlag = FALSE;
   TRACEVLONG(RecordLength);
   TRACEVLONG(PageSize);
   FileBuffer.ReDim(PageSize); // Per i files piu' corti di una pagina (cui appendo records)
   // Adesso controllo se sia presente un Header
   FileHeader = NULL; HeaderSize = 0;
   BUFR * Dati = NULL;
   Riscritto = FALSE;
   if(FileSize())Dati = FILE_RO::Leggi((ULONG)sizeof(FIX_HEADER));
   if(Dati) {
      TRACESTRING("SONO ENTRATO !!!!!!!!!!!!!!!!!");
      FIX_HEADER & Header = *(FIX_HEADER*)(Dati->Dati);
      if (Header.Hdr == HDR_OK) {
         TRACESTRING("Header Presente, DescrizioneFile='"+STRINGA(Header.DescrizioneHeader)+"'");
         FileHeader = new FIX_HEADER;
         memmove(FileHeader,Dati->Dati,sizeof(FIX_HEADER));
         HeaderSize = Header.HdrLen;
         if(Header.RecLen != RecLen) {
            ERRSTRING("Errata lunghezza del record");
            ERRINT("  Il programma si aspetta",RecLen);
            ERRINT("  Il file ha un LRECL di ",Header.RecLen);
            BEEP;
         } /* endif */
      } else {
         TRACESTRING("Header Assente");
      } /* endif */
      delete Dati;
   } else {
      TRACESTRING("Header Assente");
   } /* endif */
   NumRecords = ((FileSize()-HeaderSize) /RecLen);    // Numero di records del file
   TRACEVLONG(NumRecords);
   // Ora accedo ai dati e chiudo il file se posso caricarlo in memoria, se no
   // invalido il buffer
   totalmenteInMemoria=FALSE;
   LONG NRecsInBuffer = PageSize / RecordLength;
   if (NRecsInBuffer >= NumRecords) {
      InvalidateBuffer();
      Posiziona(0);    // Con questo leggo il file
      if(!RwFlag){
         TRACESTRING("Chiudo il file "+Nome+STRINGA(" Perche' puo' essere contenuto interamente nel buffer"));
         totalmenteInMemoria=TRUE;
         Close() ; // Chiudo direttamente l' handle
      }
   } else {
      InvalidateBuffer();
   }
};
//----------------------------------------------------------------------------
// FILE_FIX::ModificaPageSize
//----------------------------------------------------------------------------
void FILE_FIX::ModificaPageSize(ULONG NewPageSize){
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::ModificaPageSize"

   InvalidateBuffer(); // Scarico dei dati

   ULONG & Pgs = ( ULONG &)PageSize;
   Pgs = RecordLength * ((NewPageSize+RecordLength-1)/RecordLength);
   TRACEVLONG(RecordLength);
   TRACEVLONG(NewPageSize);
   TRACEVLONG(PageSize);
   FileBuffer.ReDim(PageSize); // Per i files piu' corti di una pagina (cui appendo records)
   if(totalmenteInMemoria)Open();
   // Ora accedo ai dati e chiudo il file se posso caricarlo in memoria, se no
   // invalido il buffer
   totalmenteInMemoria=FALSE;
   LONG NRecsInBuffer = PageSize / RecordLength;
   if (NRecsInBuffer >= NumRecords) {
      InvalidateBuffer();
      Posiziona(0);    // Con questo leggo il file
      TRACESTRING("Chiudo il file "+Nome+STRINGA(" Perche' puo' essere contenuto interamente nel buffer"));
      totalmenteInMemoria=TRUE;
      Close() ; // Chiudo direttamente l' handle
   } else {
      InvalidateBuffer();
   }
   EndSort();  // Per ricaricare i files BS
};

//----------------------------------------------------------------------------
// ~FILE_FIX
//----------------------------------------------------------------------------
FILE_FIX::~FILE_FIX(){ // Dealloca l' area per l' header
   #undef TRCRTN
   #define TRCRTN "~FILE_FIX"
   InvalidateBuffer(); // Per eventuali riscritture pending
   if(FileHeader)delete FileHeader;
};
//----------------------------------------------------------------------------
// FILE_FIX::DescrizioneSuFile
//----------------------------------------------------------------------------
STRINGA FILE_FIX::DescrizioneSuFile(){
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::DescrizioneSuFile"
   if (FileHeader == NULL) {
      return EMPTYSTR;
   } else {
      return STRINGA(FileHeader->DescrizioneHeader);
   } /* endif */
};
//----------------------------------------------------------------------------
// FILE_FIX::InvalidateBuffer
//----------------------------------------------------------------------------
void FILE_FIX::InvalidateBuffer(){
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::InvalidateBuffer"

   if (Riscritto) { // Debbo scaricare fisicamente su file
      if (Handle == 0 && totalmenteInMemoria) Open();
      if (Handle == 0) { // ???
         ERRSTRING("Errore: il file non e' aperto !");
         BEEP;
      } else {
         TRACESTRING("Scarico fisico su file buffer records da "+STRINGA(FirstIdInBuffer)+" a "+STRINGA(LastIdInBuffer));
         FILE_RW & ThisRW = *(FILE_RW*)this;
         ULONG BytesToWrite = (LastIdInBuffer - FirstIdInBuffer + 1)* RecordLength;
         if (BytesToWrite != FileBuffer.Length) {
            ERRSTRING("Scarico fisico su file buffer records da "+STRINGA(FirstIdInBuffer)+" a "+STRINGA(LastIdInBuffer));
            ERRSTRING("BytesToWrite = "+STRINGA(BytesToWrite)+ " FileBuffer.Length = "+STRINGA(FileBuffer.Length));
            BEEP;
         } /* endif */
         #ifdef DBG3
         TRACEHEXL("Dati dei records che sono riscritti:",FileBuffer.Dati,FileBuffer.Length,5);
         #endif
         ThisRW.Posiziona(HeaderSize + FirstIdInBuffer * RecordLength);
         ThisRW.Scrivi(FileBuffer);  // Scrive alla posizione corrente;
      } /* endif */
      Riscritto=FALSE;
   } /* endif */
   if(!totalmenteInMemoria){
      FirstIdInBuffer = 0xffffffff;
      LastIdInBuffer =0;
      NumRec = 0;
      RecordC = NULL;
   }
//<<< void FILE_FIX::InvalidateBuffer(){
}; // Forza l' I/O

//----------------------------------------------------------------------------
// FILE_FIX::Posiziona
//----------------------------------------------------------------------------
BOOL FILE_FIX::Posiziona(ULONG Id){
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::Posiziona()"

   // Speedup: di solito si accede piu' volte allo stesso record
   if(Id == NumRec && Id)return TRUE;

   if(Id > NumRecords ){
      ERRINT("Errore posizionamento del file "+ Nome+STRINGA(" Id richiesto="),Id);
      ERRINT("NumRecords=",NumRecords);
      BEEP;
      RecordC = NULL;
      NumRec = Id;
      return FALSE;
   };

   // Modifica di amedeo: evita un BEEP scandendo sequenzialmente il file
   if(Id == NumRecords && ( Id > 0 || !(OpenFlags & OPEN_ACCESS_READWRITE))  ){
      TRACEINTL("Raggiunta la fine del file in  "+ Nome+STRINGA(" Id richiesto="),Id,1);
      RecordC = NULL;
      NumRec = Id;
      return FALSE;
   };

   if ( Id < FirstIdInBuffer || Id > LastIdInBuffer ) { // Refresh FileBuffer
      if(totalmenteInMemoria){
         ERRSTRING("Errore: il buffer dovrebbe contenere l' intero file!");
         ERRINT("Id ",Id );
         ERRINT("FirstIdInBuffer",FirstIdInBuffer);
         ERRINT("LastIdInBuffer ",LastIdInBuffer );
         BEEP;
         return FALSE;
      };
      LONG Posizione = Id * RecordLength;
      LONG NumBytes  =  ((Posizione + PageSize ) > (NumRecords * RecordLength)) ? // Fine File
      ((NumRecords - Id ) * RecordLength) : // Restanti records
      PageSize                              // Una pagina
      ;
      #ifdef DBG
      if((TESTTRACE(4)&& !LimitaTrace) ||TESTTRACE(5))
      TRACELONG(Nome + STRINGA(" Posizionamento con accesso fisico al record Nø"),Id);
      #endif
      if(!Handle){
         ERRSTRING("Errore: Il file "+Nome+STRINGA(" e' chiuso!"));
         InvalidateBuffer();
         BEEP;
         return FALSE;
      };
      if(Riscritto)InvalidateBuffer(); // Se ho un aggiornamento pending
      if(!Leggi(NumBytes, Posizione + HeaderSize,  FileBuffer)){
         InvalidateBuffer();
         ERRSTRING("Errore durante I/O fisico");
         BEEP; // Gestione errore minimale
         return FALSE;
      };
      FirstIdInBuffer = Id;
      LastIdInBuffer = Id + (FileBuffer.Length) / RecordLength - 1;
      #ifdef DBG2
      TRACELONG(Nome+STRINGA("Posizione assoluta (per Eb2)"),Posizione + HeaderSize);
      TRACEVLONG(NumBytes);
      TRACEVLONG(FirstIdInBuffer);
      TRACEVLONG(LastIdInBuffer);
      #endif
//<<< if (Id < FirstIdInBuffer || Id > LastIdInBuffer ) { // Refresh FileBuffer
   } else {
      #ifdef DBG
      if((TESTTRACE(4)&& !LimitaTrace) ||TESTTRACE(5))
      TRACELONG(Nome + STRINGA(" Posizionamento logico al record Nø"),Id);
      #endif
   };
   #ifdef DBG2
   if (!LimitaTrace) {
      LONG Posizione = Id * RecordLength;
      TRACELONG(Nome+STRINGA("Posizione assoluta (per Eb2)"),Posizione + HeaderSize);
   } /* endif */
   #endif

   NumRec = Id;
   FileBuffer.Pointer = ((Id-FirstIdInBuffer) * RecordLength);
   if (FileBuffer.Length == 0) { // Per i files vuoti
      RecordC = NULL;
   } else {
      RecordC = &FileBuffer[FileBuffer.Pointer];
   } /* endif */
   #ifdef DBG2
   if (!LimitaTrace) {
      TRACEVLONG(FileBuffer.Pointer);
   } /* endif */
   #endif
   // TRACEHEX("Record Posizionato",RecordC,RecordLength);
   return TRUE;
//<<< BOOL FILE_FIX::Posiziona(ULONG Id){
};
//----------------------------------------------------------------------------
// FILE_FIX::PermettiScrittura()
//----------------------------------------------------------------------------
BOOL FILE_FIX::PermettiScrittura(){       // Prerequisito per le altre
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::PermettiScrittura()"
   if(Riscritto)InvalidateBuffer(); // Se viene chiamata due volte
   totalmenteInMemoria = FALSE;
   Close();
   OpenAction = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;  // Open Action OS2
   OpenFlags  = OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE      ;  // Open Flags OS2
   WORD Rc = Open();
   TRACELONG("RiApertura file in modalita' RW "+Nome+STRINGA(" Handle="),Handle);
   if(Rc){ // Tento di ripristinare la precedente situazione (ma non e' garantito)
      OpenAction = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_CREATE_IF_NEW;  // Open Action OS2
      OpenFlags  = OPEN_SHARE_DENYNONE | OPEN_FLAGS_RANDOMSEQUENTIAL ; // Open Flags OS2
      ERRSTRING("Errore nella Open : Retry per ripristino almeno apertura normale");
      Open();
      return FALSE;
   }
   return TRUE;
};
//----------------------------------------------------------------------------
// FILE_FIX::Clear()
//----------------------------------------------------------------------------
BOOL FILE_FIX::Clear(const STRINGA & Descrizione,BOOL ConHeader){       // Prerequisito per le altre
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::Clear()"
   TRACESTRING("PRima di permettiScrittura");
   if(!PermettiScrittura())return FALSE;
   TRACESTRING("DOpo permettiscrittura");
   Riscritto = FALSE; // Ignoro eventuali modifiche pending
   FILE_RW & ThisRW = *(FILE_RW*)this;
   ThisRW.Posiziona(0);
   ThisRW.SetSize(0);
   if(FileHeader)delete FileHeader;
   if(ConHeader){
      STRINGA Descr(Descrizione);
      if(Descrizione.Dim() > 39)Descr = Descrizione(0,38);
      if(Descrizione == NUSTR)Descr = "Header creato da Clear()";
      FileHeader = new FIX_HEADER;
      FIX_HEADER &  Header = *FileHeader;
      Header.Hdr   = HDR_OK;
      Header.Tipo  = 1;
      Header.RecLen = RecordLength;
      Header.KeyLen = 0;
      Header.HdrLen = sizeof(FIX_HEADER);
      HeaderSize = sizeof(FIX_HEADER);
      memset(Header.DescrizioneHeader,0,sizeof(Header.DescrizioneHeader));
      strcpy(Header.DescrizioneHeader , (CPSZ)Descr);
      ThisRW.Scrivi(FileHeader, sizeof(FIX_HEADER));
   } else {
      FileHeader=NULL;
   };
   NumRecords = 0;
   InvalidateBuffer(); // Forzo scarico del buffer
   TRACESTRING("Prima di filesize");
   ERRINT("FileSize()",FileSize());
   TRACESTRING("Dopo di filesize");
   return TRUE;
//<<< BOOL FILE_FIX::Clear(const STRINGA & Descrizione){       // Prerequisito per le altre
};
//----------------------------------------------------------------------------
// FILE_FIX::PutHeader()
//----------------------------------------------------------------------------
BOOL FILE_FIX::PutHeader(
      int     Tipo,                   // 1= FIX 2=BS
      int     RecLen,                 // Record Length
      int     KeyLen,                 // Key Length
      const  char * Descr){           // Descrizione del file (MAX 39 caratteri + NULL)
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::PutHeader()"

   if(!(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
      ERRSTRING("Errore: il file "+Nome+" deve essere riaperto con PermettiScrittura() o Clear()");
      BEEP;
      return FALSE;
   };
   if(FileHeader == NULL && (Tipo < 0 || RecLen < 0 || KeyLen < 0 || Descr == NULL)){
      ERRSTRING("Errore sul file "+Nome+" : Si possono utilizzare dei defaults solo se il file ha GIA' un header");
      BEEP;
      return FALSE;
   };
   InvalidateBuffer(); // Forzo scarico del buffer
   FIX_HEADER Hdr;
   ZeroFill(Hdr);
   Hdr.Hdr           =  HDR_OK    ;
   Hdr.HdrLen        =  sizeof(FIX_HEADER) ;
   if(Tipo   >0    ){ Hdr.Tipo   = Tipo;           } else { Hdr.Tipo   = FileHeader->Tipo   ; }
   if(RecLen >0    ){ Hdr.RecLen = RecLen;         } else { Hdr.RecLen = FileHeader->RecLen ; }
   if(KeyLen >0    ){ Hdr.KeyLen = KeyLen;         } else { Hdr.KeyLen = FileHeader->KeyLen ; }
   if(Descr != NULL){
      strncpy(Hdr.DescrizioneHeader,Descr,39);
   } else {
      strncpy(Hdr.DescrizioneHeader,FileHeader->DescrizioneHeader,39);
   }

   FILE_RW & ThisRW = *(FILE_RW*)this;
   if(FileHeader == NULL){ // Posso scrivere al posto dell' Header esistente
      FileHeader = (FIX_HEADER*) malloc(sizeof(FIX_HEADER));
      HeaderSize =0;
   }
   // Procedo sempre leggendo e riscrivendo tutto il file, in modo da gestire correttamente i
   // cambi di formato interno dell' header (eventuali campi futuri).
   ThisRW.Posiziona(HeaderSize);
   BUFR * Buf = ThisRW.Leggi(ThisRW.FileSize()-HeaderSize);
   memcpy(FileHeader,&Hdr,sizeof(FIX_HEADER));
   if(!ThisRW.SetSize(0)){BEEP;return FALSE;};
   ThisRW.Scrivi(FileHeader, sizeof(FIX_HEADER));
   ThisRW.Scrivi(*Buf);
   delete Buf;
   HeaderSize = sizeof(FIX_HEADER);
   return TRUE;
};
//----------------------------------------------------------------------------
//  FILE_FIX::ModifyRecord()
//----------------------------------------------------------------------------
// Se il file e' di tipo FILE_BS ed e' cambiata la chiave poi si dovra' chiamare Sort
BOOL FILE_FIX::ModifyRecord(ULONG NumeroRecord,const BUFR & NewRec){
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::ModifyRecord()"
   // M1 //  if(totalmenteInMemoria || !(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
   if(!(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
      ERRSTRING("Errore: il file "+Nome+" deve essere riaperto con PermettiScrittura() o Clear()");
      BEEP;
      return FALSE;
   };
   if(NewRec.Length != RecordLength){
      ERRSTRING("Errore: errata dimensione del record fornito, file "+ Nome);
      ERRSTRING("Dimensione fornita "+STRINGA(NewRec.Length)+" Dimensione del record "+STRINGA(RecordLength));
      TRACEHEXL("Dati che avrei dovuto riscrivere:",NewRec.Dati,NewRec.Length,1);
      BEEP;
      return FALSE;
   };

   if ( this->Posiziona(NumeroRecord) && RecordC != NULL){
      TRACELONG("Trovato record in posizione ",NumRecordCorrente());
      Riscritto = TRUE;// Bufferizzo l' output
      // FILE_RW & ThisRW = *(FILE_RW*)this;
      // #ifdef DBG3
      // TRACEHEXL("Dati del record che viene riscritto:",NewRec.Dati,NewRec.Length,5);
      // #endif
      // ThisRW.Posiziona(HeaderSize + NumeroRecord * RecordLength);
      // ThisRW.Scrivi(NewRec);  // Scrive alla posizione corrente;
      memcpy(RecordC,NewRec.Dati,RecordLength);
      return TRUE;
   } else {
      ERRSTRING("Errore: non possibile posizionamento, file "+ Nome+ " al record "+STRINGA(NumeroRecord));
      BEEP;
      return FALSE;
   } /* endif */
//<<< BOOL FILE_FIX::ModifyRecord(ULONG NumeroRecord,const BUFR & NewRec){
};
//----------------------------------------------------------------------------
// FILE_FIX::ModifyCurrentRecord
//----------------------------------------------------------------------------
BOOL FILE_FIX::ModifyCurrentRecord(){
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::ModifyCurrentRecord"
   if(!(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
      ERRSTRING("Errore: il file "+Nome+" deve essere riaperto con PermettiScrittura() o Clear()");
      BEEP;
      return FALSE;
   };
   Riscritto = TRUE;// Bufferizzo l' output
   return TRUE;
};
//----------------------------------------------------------------------------
// FILE_FIX::Flush
//----------------------------------------------------------------------------
void FILE_FIX::Flush(){
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::Flush"
   InvalidateBuffer();
}; // Forza lo scarico del buffer
//----------------------------------------------------------------------------
//  FILE_FIX::ToEnd()
//----------------------------------------------------------------------------
BOOL FILE_FIX::ToEnd(){
   if(NumRecords){
      return Posiziona(NumRecords - 1);
   } else {
      return FALSE;
   };
};
//----------------------------------------------------------------------------
// FILE_FIX::AddRecordToEnd
//----------------------------------------------------------------------------
BOOL FILE_FIX::AddRecordToEnd(const BUFR & NewRec){   // Poi si deve chiamare ReSort
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::AddRecordToEnd"
   ToEnd();
   return AddRecord(NewRec.Dati,NewRec.Length);
};
BOOL FILE_FIX::AddRecordToEnd(const void * Dati, ULONG Dim){   // Poi si deve chiamare ReSort
   ToEnd();
   return AddRecord(Dati,Dim);
};
//----------------------------------------------------------------------------
// FILE_FIX::AddRecord
//----------------------------------------------------------------------------
BOOL FILE_FIX::AddRecord(const BUFR & NewRec){   // Poi si deve chiamare ReSort
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::AddRecord"
   return AddRecord(NewRec.Dati,NewRec.Length);
};
BOOL FILE_FIX::AddRecord(const void * Dati, ULONG Dim){   // Poi si deve chiamare ReSort
   // M1 //  if(totalmenteInMemoria || !(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
   if(!(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
      ERRSTRING("Errore: il file "+Nome+" deve essere riaperto con PermettiScrittura() o Clear()");
      BEEP;
      return FALSE;
   }
   if(Dim != RecordLength){
      ERRSTRING("Errore: errata dimensione del record fornito, file "+ Nome);
      ERRSTRING("Dimensione fornita "+STRINGA(Dim)+" Dimensione del record "+STRINGA(RecordLength));
      TRACEHEXL("Dati che avrei dovuto riscrivere:",Dati,Dim,1);
      BEEP;
      return FALSE;
   };
   // AddRecord NON modifica il posizionamento logico del file

   ULONG RecordsInBuffer = PageSize / RecordLength;

   // Se il file e' vuoto aggiorno solo il buffer.
   if(NumRecords == 0 ){
         InvalidateBuffer(); FileBuffer.Clear();
         LastIdInBuffer = FirstIdInBuffer = 0;
         FileBuffer.Store(Dati,Dim); // Concatena il nuovo record
         NumRec = 0; // Mi posiziono al primo record
         RecordC = &FileBuffer[0]; // causa eventuale riallocazione aggiorno RecordC
         Riscritto = TRUE; // Indico che dovro' scaricare i dati
   // Se sono posizionato alla fine del file e ho spazio aggiorno solo il buffer.
   } else if(NumRecords == NumRec + 1  &&  (NumRecords % RecordsInBuffer) > 0){
      // Se il buffer non e' valido lo rileggo
      if(NumRec < FirstIdInBuffer || NumRec > LastIdInBuffer ){
         Posiziona(NumRec);
      }
      FileBuffer.Store(Dati,Dim); // Concatena il nuovo record
      LastIdInBuffer ++;
      RecordC = &FileBuffer[FileBuffer.Pointer]; // causa eventuale riallocazione aggiorno RecordC
      // Si noti che non ho spostato la posizione logica sul file
      Riscritto = TRUE; // Indico che dovro' scaricare i dati
   } else {  // Scrivo sul file fisico ma non sul buffer
      if(Riscritto) InvalidateBuffer(); // Per eventuali scritture precedenti
      // Riscrivo fisicamente il file
      FILE_RW & ThisRW = *(FILE_RW*)this;
      #ifdef DBG
      TRACEHEXL("Dati del record aggiunto alla fine del file:",Dati,Dim,6);
      #endif
      ThisRW.Posiziona();      // Si posiziona fisicamente alla fine del file
      ThisRW.Scrivi(Dati,Dim); // Scrive alla posizione corrente;
   } /* endif */

   FSize = 0;              // Invalido dimensione del file
   NumRecords ++;
   #ifdef DBG4
   if ( NumRecords != (FileSize()- HeaderSize) / RecordLength){
      ERRINT("Numero Records",NumRecords);
      ERRINT("(FileSize()",FileSize());
      ERRINT("(FileSize()- HeaderSize) / RecordLength",(FileSize()- HeaderSize) / RecordLength);
   } /* endif */
   #endif
   return TRUE;
};
//----------------------------------------------------------------------------
// FILE_FIX::ReSort(FILE_SORT_FUNC SortFunction);
//----------------------------------------------------------------------------
BOOL FILE_FIX::ReSort(FILE_SORT_FUNC SortFunction,BOOL ConHeader){ // Esegue un sort del file e lo riscrive (Torna FALSE su errore)
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::ReSort(SortFunction)"

   // M1 //  if(totalmenteInMemoria || !(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
   if(!(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
      ERRSTRING("Errore: il file "+Nome+" deve essere riaperto con PermettiScrittura() o Clear()");
      BEEP;
      return FALSE;
   }

   InvalidateBuffer(); // Per forzare eventuale scrittura del buffer corrente

   NumRecords= (FileSize()- HeaderSize) / RecordLength;    // Numero di records del file

   STRINGA Descr("Header Automatico del ReSort");
   if(FileHeader)Descr = FileHeader->DescrizioneHeader;

   // Leggo i records a seguire l' Header
   BUFR * SortBuf = Leggi(NumRecords * RecordLength , HeaderSize);

   qsort(SortBuf->Dati,NumRecords,RecordLength,SortFunction);

   Clear(Descr,ConHeader); // Vuoto il file
   FILE_RW & ThisRW = *(FILE_RW*)this;
   ThisRW.Scrivi(*SortBuf);
   delete(SortBuf);

   NumRecords= (FileSize()- HeaderSize) / RecordLength;    // Numero di records del file

   EndSort();
   return TRUE;
//<<< BOOL FILE_FIX::ReSort(FILE_SORT_FUNC SortFunction){ // Esegue un sort del file e lo riscrive (Torna FALSE su errore)
};
//----------------------------------------------------------------------------
// FILE_FIX::EndSort
//----------------------------------------------------------------------------
void FILE_FIX::EndSort(){
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::EndSort"
   #undef TRCRTN
   #define TRCRTN "FILE_FIX::EndSort()"
   InvalidateBuffer();
};

//----------------------------------------------------------------------------
// FILE_BS
//----------------------------------------------------------------------------
FILE_BS::FILE_BS(const char* NomeFile,ULONG RecLen,ULONG KeyLen,ULONG PgSize,
   ULONG ExtraLen,BOOL Inizializza):
FILE_FIX(NomeFile,RecLen,PgSize)
,KeyCorrente   (KeyLen + sizeof(ULONG)+ExtraLen)
,KeyLength     (KeyLen)
,ExtraLength   (ExtraLen)
,SizeSlotIndice(2*(KeyLen + sizeof(ULONG)+ExtraLen))
,NumSlotIndice ( ( PgSize ? ((FileSize()+PgSize-1)/PgSize) : 1) ) // Prima Approssimazione
,DatiIndice( (2*(KeyLen + sizeof(ULONG)+ExtraLen)) * ( PgSize ? ((FileSize()+PgSize-1)/PgSize) : 1)) // Potrebbe essere poi riallocato
{
   #undef TRCRTN
   #define TRCRTN "@FILE_BS"
   TRACEVLONG(KeyLength);
   TRACEVLONG(SizeSlotIndice);
   TRACELONG("Spazio riservato in memoria:",RecLen+2*ExtraLen+KeyLen+
      SizeSlotIndice * NumSlotIndice + PageSize);
   if( (SizeSlotIndice * NumSlotIndice) -(3 * PageSize) > 2000  ||
      PageSize - (3 * SizeSlotIndice * NumSlotIndice) > 2000){
      TRACESTRING("      I BUFFERS SONO SBILANCIATI:");
      TRACELONG("     Page Buffer: ",PageSize);
      TRACELONG("     Index Buffer: ", SizeSlotIndice * NumSlotIndice);
   };

   // Adesso controllo se l' Header corrisponde
   if(FileHeader){
      FIX_HEADER & Header = *FileHeader;
      if (Header.Tipo != 2) {
         ERRINT("Errore: aperto come BS un file di tipo",Header.Tipo);
         // ErroreToQualita(35,Codice,Errore);
         BEEP;
      } /* endif */
      if (Header.KeyLen != KeyLength) {
         // ErroreToQualita(35,Codice,Errore);
         ERRSTRING("Errata lunghezza della chiave del record");
         ERRINT("  Il programma si aspetta",KeyLen);
         ERRINT("  Il file ha una Key Length di",Header.KeyLen);
         BEEP;
      } /* endif */
   } /* endif */

   NumSlotIndice=0; // Ora li debbo contare esattamente

   if (!totalmenteInMemoria && (FileHandle() == NULL))return; // Apertura in errore
   if(Inizializza)Init();
};

//----------------------------------------------------------------------------
// FILE_BS::Init
//----------------------------------------------------------------------------
// Caricamento della tabella di indicizzazione
// Per ogni pagina si carichera' in memoria la chiave + Id corrispondente +
//    extrabytes (per sottoclassi) + ultima chiave della pagina
void FILE_BS::Init(){   // Puo' essere chiamato nel qual caso ricarica tutto
   #undef TRCRTN
   #define TRCRTN "FILE_BS::Init"
   TRACESTRING2("InizioRoutineCaricamento per",Nome);
   ULONG IdNext = 0;
   ULONG NPage  = 0;
   BUFR LastKey;
   BOOL Test = TRUE;
   BOOL FirstTime = TRUE;
   NumSlotIndice=0; // Ora li debbo contare esattamente
   DatiIndice.Clear();
   LimitaTrace=TRUE;

   do {
      InvalidateBuffer();
      if(NumRecords == 0)break; // Evita l' I/O Fisico
      if(!Posiziona(IdNext))break;          // Esegue l' I/O Fisico
      DatiIndice.Store(KeyCorrente);        // Chiave Id ed Extrabytes
      NumSlotIndice++;

      #ifdef DBG
      if(TESTTRACE(6)) {   // Se il tracelevel e' almeno 6
         TRACELONG(Nome + STRINGA(" Pagina "),NPage++);
         TRACEHEX("Prima Key della pagina",KeyCorrente.Dati,KeyCorrente.Length);
         TRACEHEX("RecordCorrispondente",RecordC,RecordLength);
      };
      #endif

      if(Test && !FirstTime){           // Test chiavi in ordine crescente (esclusa prima chiave)
         ULONG DimConfronto = min3(LastKey.Length,KeyCorrente.Length,KeyLength);
         if(this->Compare(LastKey.Dati,KeyCorrente.Dati, DimConfronto)>0){
            ERRSTRING("Errore: errata sequenza del file BS "+Nome+STRINGA(" Le chiavi del file non sono in ordine"));
            TRACEHEXL("LastKey.Dati = ",LastKey.Dati,LastKey.Length,1);
            TRACEHEXL("KeyCorrente.Dati = ",KeyCorrente.Dati,KeyCorrente.Length,1);
            // ErroreToQualita(35,Codice,Errore);
            BEEP;
            Test = FALSE; // Per evitare BEEP a ripetizione
         };
      };
      FirstTime = FALSE;
      BUFR::Swap(LastKey,KeyCorrente); // Piu' veloce di una assegnazione. KeyCorrente non e' piu' valida

      // Identifico prossima pagina da leggere ed ultima chiave della pagina
      IdNext = LastIdInBuffer + 1;  // Id da leggere
      // Identifico l' ultima chiave della pagina
      if(NumRecords && !Posiziona(LastIdInBuffer))break; // Errore
      DatiIndice.Store(KeyCorrente); // Aggiungo ultima chiave della pagina (+ Id + Extrabytes)
      #ifdef DBG
      TRACEHEXL("Ultima Key della pagina",KeyCorrente.Dati,KeyCorrente.Length,6);
      #endif
      if(Test){           // Test chiavi in ordine crescente (esclusa prima chiave)
         ULONG DimConfronto = min3(LastKey.Length,KeyCorrente.Length,KeyLength);
         if(this->Compare(LastKey.Dati,KeyCorrente.Dati, DimConfronto)>0){
            ERRSTRING("Errore: errata sequenza del file BS "+Nome+STRINGA(" Le chiavi del file non sono in ordine"));
            TRACEHEXL("LastKey.Dati = ",LastKey.Dati,LastKey.Length,1);
            TRACEHEXL("Ultima Key della pagina: KeyCorrente.Dati = ",KeyCorrente.Dati,KeyCorrente.Length,1);
            // this->Compare(LastKey.Dati,KeyCorrente.Dati, DimConfronto) ;// Per debug
            // ErroreToQualita(35,Codice,Errore);
            BEEP;
            Test = FALSE; // Per evitare BEEP a ripetizione
         };
      };
      BUFR::Swap(LastKey,KeyCorrente); // Piu' veloce di una assegnazione. KeyCorrente non e' piu' valida
//<<< do {
   } while (LastIdInBuffer < NumRecords - 1 );
   #ifdef DBG
   TRACEHEXL("Ultima Key del file:",LastKey.Dati,LastKey.Length,6);
   TRACEVLONG(NumSlotIndice);
   #endif
   if(DatiIndice.Length != (NumSlotIndice * SizeSlotIndice)){
      ERRSTRING("Errore: Mancata corrispondenza tra dimensione e Numero Slot");
      ERRINT("DatiIndice.Length",DatiIndice.Length);
      ERRINT("DatiIndice.Dim() ",DatiIndice.Dim() );
      ERRINT("SizeSlotIndice   ",SizeSlotIndice   );
      ERRINT("NumSlotIndice    ",NumSlotIndice    );
      BEEP;
   };
   #ifdef DBG
   TRACESTRING("====================<<");
   #endif
   LimitaTrace=FALSE;
//<<< void FILE_BS::Init(){   // Puo' essere chiamato nel qual caso ricarica tutto
};

//----------------------------------------------------------------------------
// FILE_BS:: FuoriRange
//----------------------------------------------------------------------------
// Questo metodo mi dice subito se una data Key e' fuori range
// (cioe' minore della chiave minima o maggiore della massima)
// Cio' permette di ridurre il numero degli accessi fisici
// Se il metodo torna TRUE la chiave NON si trova certamente su FILE.
// (Piu' precisamente : -1 = Minore di tutte le chiavi, +1 = Maggiore ...)
// Altrimenti si deve accedere per vedere se esiste
// Il metodo non cambia la posizione od altri dati del file
//----------------------------------------------------------------------------
int FILE_BS::FuoriRange(const BUFR& Key)const {
   #undef TRCRTN
   #define TRCRTN "FILE_BS::FuoriRange"
   if (!Key.Length){
      ERRSTRING("Chiesto Test con chiave di lunghezza nulla");
      return FALSE;
   };

   BYTE * IdxMem = DatiIndice.Dati;
   BYTE * Target = Key.Dati;

   // Determino se e' un confronto parziale (= Num Bytes da confrontare)
   ULONG DimConfronto = min(Key.Length,KeyLength);

   // Per evitare warnings in compilazione: mi sono scordato di dichiarare
   // const la funzione compare
   FILE_BS * Fil = (FILE_BS*)this;

   // Test < minima Key
   if(Fil->Compare(IdxMem , Target, DimConfronto) > 0)return -1;


   if(Fil->Compare(IdxMem +
      (SizeSlotIndice*(NumSlotIndice-1)+SizeSlotIndice/2) // Ultima chiave del file
      , Target, DimConfronto)<0)return 1;

   return 0;
};


//----------------------------------------------------------------------------
// FILE_BS:: Posiziona
//----------------------------------------------------------------------------
// Posizionamento per chiave (anche parziale)
// Si posiziona al PRIMO record con Key <= della chiave (parziale) data.
// Ritorna FALSE in caso di errore. (Normalmente mai)
BOOL FILE_BS::Posiziona(const BUFR & Key){
   #undef TRCRTN
   #define TRCRTN "FILE_BS::Posiziona(Key)"

   KeyEsatta = FALSE;

   if (!Key.Length){
      ERRSTRING("Chiesto posizionamento con chiave di lunghezza nulla");
      RecordC = NULL;   // Non ho piu' record corrente
      NumRec = NumRecords + 1; // Dopo fine file
      return FALSE;
   };


   if(NumRecords == 0){ // File vuoto
      RecordC = NULL;   // Non ho piu' record corrente
      NumRec = 0;
      return FALSE;
   };

   BYTE * IdxMem = DatiIndice.Dati;
   BYTE * Target = Key.Dati;

   // Determino se e' un confronto parziale (= Num Bytes da confrontare)
   ULONG DimConfronto = min(Key.Length,KeyLength);

   if(TESTTRACE(4) && (TESTTRACE(5)|| ! LimitaTrace)){
      TRACEHEX(Nome + STRINGA(" Posizionamento alla chiave: "),Key.Dati, Key.Length);
      TRACEVLONGL(DimConfronto,6);
   };

   ULONG i           ; // Id in esame
   LONG  Res         ; // Risultato ultimo confronto
   LONG  MinId       ; // Minimo  Id Valido (nella ricerca dicotomica)
   LONG  MaxId       ; // Massimo Id Valido (nella ricerca dicotomica)
   LONG  TargetId    ; // Risultato

   #ifdef DBG2
   TRACESTRING("Fase 1");
   #endif
   // Ricerca Dicotomica nell' indice in memoria
   // Se non trova la chiave deve tornare l' indice corrispondente
   // alla chiave inferiore
   MinId = 0;
   MaxId = NumSlotIndice-1;
   do {
      i = (MinId + MaxId ) /2;
      Res=this->Compare(IdxMem +(SizeSlotIndice*i), Target, DimConfronto);
      #ifdef DBG2
      TRACEVLONG(i);
      TRACELONG("Key < = > Target:",Res);
      TRACEVLONG(MinId);
      TRACEVLONG(MaxId);
      if(TESTTRACE(4) && (TESTTRACE(5)|| ! LimitaTrace)){
         TRACEHEX(Nome + STRINGA(" Dati Inizio slot: "),IdxMem +(SizeSlotIndice*i), DimConfronto);
      };
      #endif
      if(Res < 0 ){
         MinId = i ;     // Il valore minimo non puo' essere scartato
      } else if(Res > 0 ){
         MaxId = i - 1;  // Il valore massimo puo' essere scartato
      } else {           // Non posso scartare ne' l' uno ne l' altro
         // Dovendo posizionarmi al primo record con chiave data, posso
         // scartare i valori superiori a quello di confronto
         MaxId = i;
      };
      if(MinId == MaxId - 1) { // Posizione di stallo: Decido ed esco
         Res=this->Compare(IdxMem +(SizeSlotIndice*MaxId), Target, DimConfronto);
         #ifdef DBG2
         TRACESTRING("Stallo: Finisco ricerca");
         TRACELONG("Key < = > Target:",Res);
         TRACEVLONG(MinId);
         TRACEVLONG(MaxId);
         if(TESTTRACE(4) && (TESTTRACE(5)|| ! LimitaTrace)){
            TRACEHEX(Nome + STRINGA(" Dati Inizio slot: "),IdxMem +(SizeSlotIndice*MaxId), DimConfronto);
         };
         #endif
         if(Res <  0) {
            MinId = MaxId;      // Target = MaxId
         } else if(Res >  0) {
            MaxId = MinId;      // Target = MinId
         } else { // Debbo confrontare anche con MinId
            Res=this->Compare(IdxMem +(SizeSlotIndice*MinId), Target, DimConfronto);
            if(Res < 0) {
               MinId = MaxId;   // Target = MaxId
            } else {
               MaxId = MinId;   // Target = MinId
            };
         };
      };

//<<< do {
   } while (MinId < MaxId ); // Trovato

   // In situazioni di chiave che si estende tra due pagine adiacenti
   // il record potrebbe stare sulla pagina precedente: controllo.
   if(MinId > 0){
      // Confronto con ultima key record precedente
      Res=this->Compare(IdxMem +(SizeSlotIndice*(MinId-1)) +SizeSlotIndice/2, Target, DimConfronto);
      if(Res >= 0){
         //      TRACESTRING("Correzione: La chiave inizia sulla pagina precedente");
         MinId --;
      };
   };
   memmove(&TargetId , IdxMem +(SizeSlotIndice * MinId) + KeyLength,sizeof(ULONG));
   #ifdef DBG2
   TRACELONG("Fine Fase 1 : MinId=",MinId);
   TRACEVLONG(TargetId);
   #endif


   // Ora carico la pagina da file e ripeto
   // Tralascio se ho gia' la pagina esatta in memoria
   if(FirstIdInBuffer != TargetId)InvalidateBuffer(); // Non serve
   if(!Posiziona(TargetId)){ ERRSTRING("Interrotta la ricerca del record"); return FALSE;}; // Accedo per record Number
   if(TargetId != FirstIdInBuffer){ // Situazione Anomala
      TRACEVLONG(TargetId);
      TRACEVLONG(FirstIdInBuffer);
      BEEP;
      RecordC = NULL;   // Non ho piu' record corrente
      NumRec = NumRecords + 1; // Dopo fine file
      return FALSE;
   };

   // Ricerca Dicotomica nella pagina caricata
   // Se non trova la chiave deve posizionarsi alla chiave inferiore

   MinId = TargetId;
   MaxId = LastIdInBuffer;

   do {
      i = (MinId + MaxId ) /2;
      if(!Posiziona(i)){ ERRSTRING("Interrotta la ricerca del record"); return FALSE;}; // Non dovrebbe comportare accesso al file
      Res=this->Compare(KeyCorrente.Dati, Target, DimConfronto);
      #ifdef DBG2
      TRACEVLONG(i);
      TRACELONG("Key < = > Target:",Res);
      TRACEHEX("KeyCorrente: ",KeyCorrente.Dati, KeyCorrente.Length);
      TRACEVLONG(MinId);
      TRACEVLONG(MaxId);
      TRACEHEX("Record Corrente (o Primi 50 Bytes)" ,RecordC, min(RecordLength,50));
      TRACESTRING("-------------------");
      #endif
      if(Res <0 ){
         MinId = i ;     // Il valore minimo non puo' essere scartato
      } else if(Res > 0 ){
         MaxId = i - 1;  // Il valore massimo puo' essere scartato
      } else { // Found
         MaxId = i;
      };
      if(MinId == MaxId - 1) { // Posizione di stallo: Decido ed esco
         if(!Posiziona(MaxId)){ ERRSTRING("Interrotta la ricerca del record"); return FALSE;}; // Non dovrebbe comportare accesso al file
         Res=this->Compare(KeyCorrente.Dati, Target, DimConfronto);
         #ifdef DBG2
         TRACESTRING("Stallo: Finisco ricerca");
         TRACELONG("Key < = > Target:",Res);
         TRACEHEX("KeyCorrente: ",KeyCorrente.Dati, KeyCorrente.Length);
         TRACEVLONG(MinId);
         TRACEVLONG(MaxId);
         TRACEHEX("Record Corrente (o Primi 50 Bytes)" ,RecordC, min(RecordLength,50));
         TRACESTRING("-------------------");
         #endif
         if(Res <  0) {
            MinId = MaxId;      // Target = MaxId
         } else if(Res >  0) {
            MaxId = MinId;      // Target = MinId
         } else { // Debbo confrontare anche con MinId
            if(!Posiziona(MinId)){ ERRSTRING("Interrotta la ricerca del record"); return FALSE;}; // Non dovrebbe comportare accesso al file
            Res=this->Compare(KeyCorrente.Dati, Target, DimConfronto);
            if(Res < 0) {
               MinId = MaxId;   // Target = MaxId
            } else {
               MaxId = MinId;   // Target = MinId
            };
         };
//<<< if(MinId == MaxId - 1) { // Posizione di stallo: Decido ed esco
      };
//<<< do {
   } while (MinId < MaxId  ); /* enddo */
   TargetId = MinId;
   #ifdef DBG2
   TRACEVLONG(TargetId);
   TRACEHEX(Nome + STRINGA(" Key effettiva: "),KeyCorrente.Dati, KeyCorrente.Length);
   #endif
   if(!Posiziona(TargetId)){ ERRSTRING("Interrotta la ricerca del record"); return FALSE;}; // Non dovrebbe comportare accesso al file
   // luciano : Forza un posizionamento fisico
   if( OpenFlags & OPEN_ACCESS_READWRITE)FILE_RO::Posiziona(TargetId*RecordLength+HeaderSize);
   // Determino se la chiave e' esatta
   KeyEsatta = this->Compare(KeyCorrente.Dati, Target, DimConfronto) == 0;
   #ifdef DBG
   if(TESTTRACE(4) && (TESTTRACE(5)|| ! LimitaTrace)){
      TRACEVLONG(KeyEsatta);
      if(!KeyEsatta){
         TRACEHEX(Nome + STRINGA(" Key effettiva: "),KeyCorrente.Dati, KeyLength);
      };
   };
   #endif
   return TRUE;
//<<< BOOL FILE_BS::Posiziona(const BUFR & Key){
};

//----------------------------------------------------------------------------
// FILE_BS:: KeySet (virtuale)
//----------------------------------------------------------------------------
void FILE_BS::KeySet(){
   #undef TRCRTN
   #define TRCRTN "FILE_BS::KeySet"
   // Di default copia i primi KeyLength bytes
   KeyCorrente.Clear();
   //luciano
   if (NumRecords == 0)return;
   KeyCorrente.Store(RecordC,KeyLength);
   KeyCorrente.Store(&NumRec,sizeof(ULONG)); // Aggiungo indice
   this->ExtraSet();
   KeyCorrente.Pointer=0;
   #ifdef DBG
   TRACEHEXL(Nome + STRINGA(" Caricata Chiave"),KeyCorrente.Dati,KeyCorrente.Length,7);
   #endif
};
//----------------------------------------------------------------------------
// FILE_BS:: Compare(virtuale)
//----------------------------------------------------------------------------
int FILE_BS::Compare(const void * Key1, const void* Key2,ULONG DimConfronto){
   // Confronto tra due chiavi
   if(!DimConfronto)DimConfronto = KeyLength;
   return memcmp(Key1,Key2,DimConfronto);
};

//----------------------------------------------------------------------------
// FILE_BS::Clear()
//----------------------------------------------------------------------------
BOOL FILE_BS::Clear(const STRINGA & Descrizione,BOOL ConHeader){    // Prerequisito per le altre
   #undef TRCRTN
   #define TRCRTN "FILE_BS::Clear()"
   if(!FILE_FIX::Clear(Descrizione,ConHeader))return FALSE;
   if(ConHeader){
      FileHeader->Tipo   = 2;
      FileHeader->KeyLen = KeyLength   ;
      FILE_RW & ThisRW = *(FILE_RW*)this;
      ThisRW.Posiziona(0);
      ThisRW.Scrivi(FileHeader, sizeof(FIX_HEADER));
   };
   EndSort(); // Per ricaricare in modo corretto le chiavi
   return TRUE;
};


//----------------------------------------------------------------------------
//  FILE_BS::ModifyRecord()
//----------------------------------------------------------------------------
// Se e' cambiata la chiave poi si dovra' chiamare Sort
BOOL FILE_BS::ModifyRecord(const BUFR& Chiave,const BUFR & NewRec){
   #undef TRCRTN
   #define TRCRTN "FILE_BS::ModifyRecord()"
   BUFR Key(Chiave); // Per evitare problemi nel caso in cui chiave fosse KeyCorrente
   if ( this->Posiziona(Key)){
      if(!KeyEsatta) {
         ERRSTRING("Errore: posso modificare un record solo se viene fornita la chiave esatta, file "+ Nome);
         TRACEHEXL(Nome + STRINGA(" Key Richiesta: "),Key.Dati, Key.Length,1);
         TRACEHEXL(Nome + STRINGA(" Key effettiva: "),KeyCorrente.Dati, KeyCorrente.Length,1);
         BEEP;
         return FALSE;
      } else {
         return FILE_FIX::ModifyRecord(NumRecordCorrente(),NewRec);
      };
   } else {
      ERRSTRING("Errore: non possibile posizionamento, file "+ Nome);
      TRACEHEXL(Nome + STRINGA(" Key Richiesta: "),Key.Dati, Key.Length,1);
      TRACEHEXL(Nome + STRINGA(" Key effettiva: "),KeyCorrente.Dati, KeyCorrente.Length,1);
      BEEP;
      return FALSE;
   } /* endif */
//<<< BOOL FILE_BS::ModifyRecord(const BUFR& Chiave,const BUFR & NewRec){
};

//----------------------------------------------------------------------------
// FILE_BS:: ReSort(){
//----------------------------------------------------------------------------
static FILE_BS * SORT_File;
int FILE_BS::sort_function( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "FILE_BS::sort_function()"
   BUFR BKey;
   ULONG A = *(ULONG*)a;
   ULONG B = *(ULONG*)b;
   SORT_File->Posiziona(B);
   BUFR::Swap(BKey,SORT_File->KeyCorrente);
   SORT_File->Posiziona(A);
   ULONG DimConfronto = min3(BKey.Length,SORT_File->KeyCorrente.Length,SORT_File->KeyLength);
   return SORT_File->Compare(SORT_File->KeyCorrente.Dati,BKey.Dati,DimConfronto);
};
//----------------------------------------------------------------------------
// FILE_BS::sort_function2
//----------------------------------------------------------------------------
int FILE_BS::sort_function2( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "FILE_BS::sort_function2"
   return SORT_File->Compare(a,b,SORT_File->KeyLength);
};
//----------------------------------------------------------------------------
// FILE_BS::ReSortFAST
//----------------------------------------------------------------------------
BOOL FILE_BS::ReSortFAST(){ // Esegue un sort del file completamente in memoria
   #undef TRCRTN
   #define TRCRTN "FILE_BS::ReSortFAST"
   if(!(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
      ERRSTRING("Errore: il file "+Nome+" deve essere riaperto con PermettiScrittura() o Clear()");
      BEEP;
      return FALSE;
   }
   static HMTX  hmtx   ; /* Mutex semaphore handle */
   if(hmtx == NULL){
      LONG rc;
      ULONG   flAttr=0;    /* Creation attributes */
      BOOL32  fState=0;    /* Initial state of the mutex semaphore */
      rc = DosCreateMutexSem(NULL, &hmtx, flAttr, fState);
      if(rc){
         ERRINT("Errore nella creazione del semaforo: Routine non rientrante",rc);
         BEEP;
      }
   }
   if(hmtx)DosRequestMutexSem(hmtx, -1);
   SORT_File = this;
   ReSort(FILE_BS::sort_function2);
   if(hmtx)DosReleaseMutexSem(hmtx);
   return TRUE;
};
// Non funziona molto bene: eseguo comunque un resort FAST
#ifndef OLD_SORT
BOOL FILE_BS::ReSort(){ // Esegue un sort del file e lo riscrive (Torna FALSE su errore)
   return ReSortFAST();
};
#else
//----------------------------------------------------------------------------
// FILE_BS::ReSort
//----------------------------------------------------------------------------
BOOL FILE_BS::ReSort(){ // Esegue un sort del file e lo riscrive (Torna FALSE su errore)
   #undef TRCRTN
   #define TRCRTN "FILE_BS::ReSort"

   // M1 //  if(totalmenteInMemoria || !(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
   if(!(OpenFlags & OPEN_ACCESS_READWRITE)){ // E' aperto in modo read only
      ERRSTRING("Errore: il file "+Nome+" deve essere riaperto con PermettiScrittura() o Clear()");
      BEEP;
      return FALSE;
   }

   InvalidateBuffer(); // Per forzare eventuale scrittura del buffer corrente

   FSize = 0;
   *(LONG*)&NumRecords= (FileSize()- HeaderSize) /RecordLength;    // Numero di records del file

   SORT_File = this;

   // Ordine[i] contiene il numero di record (nel file originario)
   // che dovra' andare nella posizione I dopo il sort
   // Siti[i] contiene il numero di record ove e' finito il record Iesimo
   // del file originario (momento per momento)
   ULONG * Ordine = new ULONG[NumRecords];
   ULONG * Siti   = new ULONG[NumRecords];
   ULONG * Orig   = new ULONG[NumRecords];
   for (int i = 0;i < NumRecords ;i++ ){
      Ordine[i] = Siti[i] = Orig[i] = i; }; // Carico con ordine attuale
   // Sort per impostare Ordine
   qsort(Ordine,NumRecords, sizeof(ULONG),FILE_BS::sort_function);
   #ifdef DBG3
   TRACESTRING("Contenuti di ORDINE");
   for (i = 0;i < NumRecords ;i++ ){
      TRACESTRING("ORDINE["+STRINGA(i)+"]="+STRINGA(Ordine[i]));
   } /* endfor */
   #endif

   // Ora scambio i records fuori ordine
   TRACESTRING("Ordinamento del file");
   FILE_RW & ThisRW = *(FILE_RW*)this;
   BUFR Buf1,Buf2; ULONG I_Buf;
   for (i = 0;i < NumRecords ;i++ ){
      ULONG PosRec = Siti[Ordine[i]];
      #ifdef DBG3
      TRACESTRING("--------------------- Record "+ STRINGA(i)+" <--> "+STRINGA(PosRec));
      #endif
      if(PosRec == i)continue; // Record gia' ordinato
      // ------------------------------------
      // Siti[x] contiene la posizione in cui si trova adesso il record che originariamente era in posizione x
      // Orig[x] contiene la posizione in cui si trovava originariamente il record che adesso e' in posizione x
      // Ordine[i] indica ove si trovava originariamente il record che debbo mettere in posizione i
      // ------------------------------------
      // Per mettore il record in posizione i:
      // Il record si trovava originariamente in posizione Ordine[i] ma
      // durante l' algoritmo viene spostato.
      // La posizione corretta e' pertanto Siti[Ordine[i]] = PosRec
      // Ne risulta:
      //    Siti[Ordine[i]] = i;
      //    Orig[i] = Ordine[i];
      // Metto inoltre il record che si trova nella posizione i in Posrec
      // Ne risulta:
      //    Siti[Orig[i]] = PosRec;
      //    Orig[PosRec] = Orig[i]; (Prima della modifica di Orig[i])
      // ------------------------------------
      // Mi posiziono sul record che deve andare in posizione i e leggo su Buf1
      Leggi(RecordLength,HeaderSize + (RecordLength*PosRec),Buf1);
      // Mi posiziono sul record in posizione i e leggo su Buf2
      Leggi(RecordLength,HeaderSize+(RecordLength*i),Buf2);
      #ifdef DBG3
      TRACEHEX("Dati del record in posizione (Prima dell' inversione)"+STRINGA(PosRec),Buf1.Dati,Buf1.Length);
      TRACEHEX("Dati del record in posizione (Prima dell' inversione)"+STRINGA(i),Buf2.Dati,Buf2.Length);
      #endif

      // Swap dei buffers e riscrittura
      BUFR::Swap(Buf1,Buf2);
      ThisRW.FILE_RW::Posiziona(HeaderSize+(RecordLength*i));
      ThisRW.Scrivi(Buf2);                // Scrive alla posizione corrente;
      ThisRW.FILE_RW::Posiziona(HeaderSize+(RecordLength*PosRec));
      ThisRW.Scrivi(Buf1);                // Scrive alla posizione corrente;

      // Aggiorno Siti ed Orig
      #ifdef DBG3
      //    TRACESTRING(" Siti[Ordine[i]]=i      : Siti["+STRINGA(Ordine[i])+"] = "+STRINGA(i));
      //    TRACESTRING(" Siti[Siti[i]] = PosRec : Siti["+STRINGA(Siti[i])+"] = "+STRINGA(PosRec));
      TRACESTRING(" Siti[Orig[i]] = PosRec : Siti["+STRINGA(Orig[i]  )+"] = "+STRINGA(PosRec));
      TRACESTRING(" Orig[PosRec] = Orig[i] : Orig["+STRINGA(PosRec   )+"] = "+STRINGA(Orig[i]));
      TRACESTRING(" Siti[Ordine[i]] = i    : Siti["+STRINGA(Ordine[i])+"] = "+STRINGA(i     ));
      TRACESTRING(" Orig[i] = Ordine[i]    : Orig["+STRINGA(i        )+"] = "+STRINGA(Ordine[i]));
      #endif
      Siti[Orig[i]] = PosRec;
      Orig[PosRec] = Orig[i];
      Siti[Ordine[i]] = i;
      Orig[i] = Ordine[i];
      // .......
      //    Siti[Ordine[i]] = i; // E' stato messo a posto
      //    Siti[Siti[i]] = PosRec;    // E' stato spostato per appoggiarlo da qualche parte

//<<< for (i = 0;i < NumRecords ;i++ ){
   } /* endfor */
   TRACESTRING("Eseguito il sort");

   // libero le risorse
   delete Ordine;
   delete Siti;

   EndSort();

   return TRUE;
//<<< BOOL FILE_BS::ReSort(){ // Esegue un sort del file e lo riscrive (Torna FALSE su errore)
};
#endif

//----------------------------------------------------------------------------
// FILE_BS::EndSort
//----------------------------------------------------------------------------
void FILE_BS::EndSort(){
   #undef TRCRTN
   #define TRCRTN "FILE_BS::EndSort"
   InvalidateBuffer();
   Init();
};

//----------------------------------------------------------------------------
// FILE_KEY_TEXT
//----------------------------------------------------------------------------
FILE_KEY_TEXT::FILE_KEY_TEXT( const char* NomeFile,ULONG RecLen,ULONG KeyLen,ULONG PageSize,
   ULONG ExtraLen,BOOL Inizializza):
FILE_BS(NomeFile,RecLen,KeyLen+1,PageSize,ExtraLen,FALSE){
   if(Inizializza)Init();
};

const char *  FILE_KEY_TEXT::ChiaveCorrente()  // NULL Terminated
{ return (CPSZ) FILE_KEY_TEXT::KeyCorrente.Dati; };

//----------------------------------------------------------------------------
// FILE_KEY_TEXT::Posiziona
//----------------------------------------------------------------------------
BOOL FILE_KEY_TEXT::Posiziona(const STRINGA& ChiaveParziale) {
   #undef TRCRTN
   #define TRCRTN "FILE_KEY_TEXT::Posiziona"
   if (!ChiaveParziale.Dim()){
      ERRSTRING("Errore: Richiesto posizionamento su chiave di lunghezza 0");
      return FALSE;
   };
   TRACEVSTRING2(ChiaveParziale);
   BUFR Target; Target.Store((CPSZ)ChiaveParziale,ChiaveParziale.Dim());
   return FILE_BS::Posiziona(Target);
};
//----------------------------------------------------------------------------
// FILE_KEY_TEXT::Compare
//----------------------------------------------------------------------------
int  FILE_KEY_TEXT::Compare(const void * Key1,const void* Key2,ULONG DimConfronto){
   #undef TRCRTN
   #define TRCRTN "FILE_KEY_TEXT::Compare"
   if(!DimConfronto)DimConfronto = KeyLength;
   return strnicmp((char*)Key1,(char*)Key2,DimConfronto);
};
//----------------------------------------------------------------------------
// FILE_KEY_TEXT::KeySet
//----------------------------------------------------------------------------
void FILE_KEY_TEXT::KeySet(){
   #undef TRCRTN
   #define TRCRTN "FILE_KEY_TEXT::KeySet"
   // Copia i primi KeyLength bytes ed aggiunge uno zero finale
   KeyCorrente.Clear();
   KeyCorrente.Store(RecordC,KeyLength);
   KeyCorrente[KeyLength-1] = '\0'; // terminatore di stringa
   KeyCorrente.Store(&NumRec,sizeof(ULONG)); // Aggiungo indice
   this->ExtraSet();
   KeyCorrente.Pointer=0;
   #ifdef DBG
   TRACEHEXL(Nome + STRINGA(" Caricata Chiave"),KeyCorrente.Dati,KeyCorrente.Length,7);
   #endif
};

//----------------------------------------------------------------------------
// FILE_BS::ControllaSequenza
//----------------------------------------------------------------------------
// Questa funzione controlla la sequenza del file
// Ritorna TRUE se la sequenza e' corretta
// Il risultato viene anche scritto sul trace a tracelevel 1
#undef LIVELLO_DI_TRACE_DEL_PROGRAMMA
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
BOOL FILE_BS::ControllaSequenza(){
   #undef TRCRTN
   #define TRCRTN "FILE_BS::ControllaSequenza()"
   int NRec = NumRecordsTotali();
   BUFR LastKey;
   LimitaTrace=TRUE;
   for (ULONG i=0;i < NRec ;i++ ) {
      if(!this->Posiziona(i)){
         ERRINT(Nome + STRINGA("Errore nel posizionamento al record numero "),i);
         return FALSE;
      };
      if(i>0) {
         if (this->Compare(LastKey.Dati,KeyCorrente.Dati,KeyLength) > 0) {
            ERRSTRING(Nome + STRINGA(" Errore nel confronto : record Nø ")+STRINGA(i-1)+STRINGA(" e' > record Nø ")+STRINGA(i));
            TRACEHEXL(Nome + STRINGA(" Chiave del record Nø ")+STRINGA(i-1),LastKey.Dati,LastKey.Length,1);
            TRACEHEXL(Nome + STRINGA(" Chiave del record Nø ")+STRINGA(i),KeyCorrente.Dati,KeyCorrente.Length,1);
            return FALSE;
         } /* endif */
      } /* endif */
      LastKey = KeyCorrente;
   } /* endfor */
   ERRSTRING(":::::::::::::::::: Controllo OK  :::::::::::::::::::::  "+Nome);
   LimitaTrace=FALSE;
   return TRUE;
//<<< BOOL FILE_BS::ControllaSequenza(){
};
//----------------------------------------------------------------------------
// Dummy1
//----------------------------------------------------------------------------
// Questa funzione fa' caricare in SIPAX.DLL il supporto run time del formatter float
// Serve ad evitare abend con il compilatore  BORLAND
void Dummy1(FILE * Out){
   #undef TRCRTN
   #define TRCRTN "Dummy1"
   fprintf(Out,"%f\n",35.53);
   float x;
   fscanf(Out,"%f ",&x);
};

