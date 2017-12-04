//----------------------------------------------------------------------------
// CHNTEMPL.CPP
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

#include "std.h"
#include <chntempl.hpp>
#include "oggetto.h"
#include <stdarg.h>
#include "FILE_RW.HPP"

//----------------------------------------------------------------------------
// StringToInt
//----------------------------------------------------------------------------
ULONG  _export StringToInt(const char *a, BYTE NumCaratteri){
   #undef TRCRTN
   #define TRCRTN "StringToInt"
   if(NumCaratteri <= 0){ BEEP;};
   ULONG Tot = 0;
   for (int i = 0 ;i < NumCaratteri ;i++ ) {
      char C = a[i];
      if(C >='0' && C <= '9'){
         Tot = (10 * Tot) + (C - '0');
      }
   } /* endfor */
   return Tot;
};

//----------------------------------------------------------------------------
// StringFrFix
//----------------------------------------------------------------------------
char *  _export StringFrFix( const char *a, BYTE NumCaratteri){
   #undef TRCRTN
   #define TRCRTN "StringFrFix"
   static char Buf[2600];
   static int Idx = 0;
   if(NumCaratteri <= 0){ BEEP;};
   char * Tmp = Buf + (256 * Idx);
   Idx = (++Idx) %10;
   memmove(Tmp,a,NumCaratteri);
   Tmp[NumCaratteri]=0;
   return Tmp;
};

//----------------------------------------------------------------------------
// AsgnFromInt
//----------------------------------------------------------------------------
void _export AsgnFromInt(char * To, int From, int NumChar){
   #undef TRCRTN
   #define TRCRTN "AsgnFromInt"
   if(NumChar <= 0 || NumChar >= 256 ){ BEEP;};
   char * c = To + NumChar -1 ;
   while (c >= To) {
      int Quoziente = From / 10;
      *c = '0' + (From - (10*Quoziente));
      c -- ;
      From = Quoziente;
   } /* endwhile */
};

//----------------------------------------------------------------------------
// Questa funzioncina scrive gli stessi dati su piu' destinazioni
// Funziona come una sprintf, aggiunge da sola i CR-LF finali
// Scrive automaticamente su STDOUT e sul trace (a TRACELEVEL 1)
//----------------------------------------------------------------------------
ELENCO Files_RW;
int  _export Bprintf(const char * Format, ...){
   #undef TRCRTN
   #define TRCRTN "Bprintf"
   va_list argptr;
   int cnt;
   char Buffer[4096];
   va_start(argptr, Format);
   cnt = vsprintf(Buffer, Format, argptr);
   va_end(argptr);
   if(cnt > 0){
      printf("%s\n",Buffer);
      if(trchse)PrtText("",Buffer);
      ORD_FORALL(Files_RW,j){
         FILE_RW * Fil = (FILE_RW*)Files_RW[j];
         Fil->printf("%s\r\n",Buffer);
      }
   };
   return(cnt);
//<<< int  _export Bprintf const char * Format, ...  
};
int  _export Bprintf2(const char * Format, ...){
   #undef TRCRTN
   #define TRCRTN "Bprintf2"
   va_list argptr;
   int cnt;
   char Buffer[4096];
   va_start(argptr, Format);
   cnt = vsprintf(Buffer, Format, argptr);
   va_end(argptr);
   if(cnt > 0){
      if(trchse)PrtText("",Buffer);
      ORD_FORALL(Files_RW,j){
         FILE_RW * Fil = (FILE_RW*)Files_RW[j];
         Fil->printf("%s\r\n",Buffer);
      }
   };
   return(cnt);
//<<< int  _export Bprintf2 const char * Format, ...  
};
int  _export Bprintf3(const char * Format, ...){
   #undef TRCRTN
   #define TRCRTN "Bprintf3"
   va_list argptr;
   int cnt;
   char Buffer[4096];
   va_start(argptr, Format);
   cnt = vsprintf(Buffer, Format, argptr);
   va_end(argptr);
   if(cnt > 0){
      if(trchse)PrtText("",Buffer);
   };
   return(cnt);
};
//----------------------------------------------------------------------------
// AddFileToBprintf
//----------------------------------------------------------------------------
void _export AddFileToBprintf(FILE_RW * Out){
   if(Out){
      Files_RW += Out;
   } else {
      Files_RW.Clear();
   }
};
