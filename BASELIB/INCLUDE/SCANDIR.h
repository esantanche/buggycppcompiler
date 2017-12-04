#include "dummy.h"
//----------------------------------------------------------------------------
// Funzioncine  per lavorare con i files
//----------------------------------------------------------------------------

#ifndef HO_SCANDIR_H
#define HO_SCANDIR_H   // Indica che questo file e' stato incluso

#ifndef HO_STD_H
#include <std.h>
#endif

#ifndef HO_ELENCO_S_H
#include <elenco_s.h>
#endif

//----------------------------------------------------------------------------
// Ricerca di files in directories
//----------------------------------------------------------------------------
// EMS001 VA ELENCO_S _export ScanDir(const STRINGA& CompleteFileName);

//----------------------------------------------------------------------------
// TestFileEsistance : ritorna VERO se il file esiste
//----------------------------------------------------------------------------
// Se vi sono errori ritorna FALSO
BOOL  _export TestFileExistance (const STRINGA & NomeFile);

/* EMSv VA
//----------------------------------------------------------------------------
// Ritorna gli attributi del file specificato
//----------------------------------------------------------------------------
// Se non esiste o se vi sono errori ritorna 0
UINT _export QueryFileAttributes(const STRINGA & NomeFile);

//----------------------------------------------------------------------------
// QueryFileSize : ritorna la dimensione del file
//----------------------------------------------------------------------------
// Ritorna 0 se va tutto bene, altrimenti un return code di errore
USHORT _export QueryFileSize(const STRINGA & NomeFile, ULONG *Size) ;

//----------------------------------------------------------------------------
// QueryFileStatus : ritorna lo status del file.
// Comprende la data di creazione e dell' ultima modifica, il size
// e gli attributi
//----------------------------------------------------------------------------
// Ritorna 0 se va tutto bene, altrimenti un return code di errore
// Questa routine al momento non e' dichiarata export, lo sara'
// non appena possibile. Essendo utilizzata da trc2 debbo
// utilizzare un CPSZ (per non legare il trace alle stringhe).
USHORT QueryFileStatus(CPSZ NomeFile, struct _FILESTATUS3 & Stato);


//----------------------------------------------------------------------------
// Analizza il nome di un file.
//----------------------------------------------------------------------------
// Se qualcuna delle variabili di output e' NULL non ritorna la
// componente corrispondente
//----------------------------------------------------------------------------
// Se PATH non e' fornito vi imposta il path corrente.
// Nel caso in cui il PATH non fosse stato ESPLICITAMENTE FORNITO
// ritorna TRUE.
//----------------------------------------------------------------------------
BOOL _export AnalizeFileName(const STRINGA& FileName,STRINGA& Path=NUSTR ,STRINGA& File =NUSTR, STRINGA& Ext=NUSTR);


//----------------------------------------------------------------------------
// CercaFile
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
STRINGA _export CercaFile(
   const STRINGA& NomeFile       // Nome del file con estensioni
  ,const ELENCO_S & Directories  // Dir su cui cercarlo
  ,BOOL CurrentDir  = TRUE       // Indica se includere la directory corrente
);
// Versione con variabile di environment
STRINGA _export CercaFile(
   const STRINGA& NomeFile                // Nome del file con estensioni
  ,const STRINGA & Directories = "PATH"   // Dir su cui cercarlo (Default = PATH)
  ,BOOL CurrentDir             =  TRUE    // Indica se includere la directory corrente
);

//----------------------------------------------------------------------------
// CreatePath
//----------------------------------------------------------------------------
// Funzione per creare un path
// Il Path da creare deve essere passato come parametro, se il path non esiste
// la funzione lo crea i codici di errore ritornati sono quelli della funzione
// DosCreateDir
APIRET _export CreatePath( const STRINGA & );

//----------------------------------------------------------------------------
// GetReqName
//----------------------------------------------------------------------------
// Funzione per ottenere il nome dell'utente collegato
VOID _export GetReqName  ( STRINGA & );
*/

#endif
