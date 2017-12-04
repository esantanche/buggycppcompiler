//----------------------------------------------------------------------------
// FILE UTENTE.C
//----------------------------------------------------------------------------
// Contiene le funzioni ridefinibili dall' utente e le personalizzazioni di progetto
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
#define OKTRACE

// EMS001
typedef unsigned long BOOL;

#include "oggetto.H"
// EMS002 VA non serve #include "scandir.h"
#include <stringa.h>  // EMS003 VA
// EMS004 VA #include <dir.h>

STRINGA _export CommentoBeep;

//----------------------------------------------------------------------------
// Funzione per fare un BEEP generico di attenzione
//----------------------------------------------------------------------------
// Chiamare in caso di errori INTERNI o simili
// Puo' essere completamente ridefinito dall' utente
//----------------------------------------------------------------------------
void _export Beep( char * File, int Line, USHORT TLev )
{
#undef TRCRTN
#define TRCRTN "BEEP "

	static NUMBEEP = 0;
	if (NUMBEEP ++ > 100) return;
	TRACESTRING("께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께");
	TRACESTRING("께께께께께께께께       BEEP: Probabile errore     께께께께께께");
	TRACESTRING("께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께께");
	if(getenv("OK_BEEP") && !stricmp(getenv("OK_BEEP"),"SI")){
		if (NUMBEEP < 5) { DosBeep((ULONG)660,(ULONG)600); }
      else { TRACESTRING("Non emessa segnalazione acustica"); }
   } else {
      NUMBEEP ; //++;
   };
   TRACESTRING("File sorgente: " +STRINGA(File)+" Linea "+STRINGA(Line));
   if(CommentoBeep != EMPTYSTR)TRACEVSTRING2(CommentoBeep);
}

//----------------------------------------------------------------------------
// Funzione per fare un BEEP per errore su carattere
//----------------------------------------------------------------------------
void ErroreSuCarattere()
{
   DosBeep((ULONG)440,(ULONG)100);
}

