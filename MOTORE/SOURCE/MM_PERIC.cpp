//========================================================================
// MM_PERIA.CPP : Classi per il caricamento iniziale dei files "t"
//========================================================================
// Componente Assembler
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 1

// EMS001
typedef unsigned long BOOL;

// Disattivo le informazioni di debug
#pragma option -y- -v-

#include "BASE.hpp"
#include "alfa0.hpp"
#include "mm_crit2.hpp"

//#define DBGMASKPA
//#define OKMAIN

// EMS002 #pragma inline  // Per il metodo GiornoPrecedente();
// EMS Win #pragma option -Od

#define INIZIO_NOTTE_45       1  // Primo blocco di 45 minuti notturno
#define FINE_NOTTE_45         7  // Ultimo blocco di 45 minuti notturno
#define MASK_NOTTE_45      0xfe  // Maschera ore notturne
#define PRIMO_GIORNO_45   0x100  // Primo blocco del giorno = blocco da settare in partenza per arrivi notturni
#if (INIZIO_NOTTE != 60) || (FINE_NOTTE != 360)
#error "Errore: Dipendenza del codice assembler da orari inizio e fine NOTTE"
#endif

struct AdHoc{ DWORD dw1; DWORD dw2; DWORD dw3; DWORD dw4; DWORD dw5; DWORD dw6; DWORD dw7; DWORD dw8; };


//----------------------------------------------------------------------------
// MaskPartenzeCompatibili
//----------------------------------------------------------------------------
DWORD MaskPartenzeCompatibili(
   DWORD OrariPartenza,
   DWORD OrariArrivo  ,
   BYTE MinPercorrenza,
   BYTE MaxPercorrenza,
   BYTE Margine
){
   #undef TRCRTN
   #define TRCRTN "MaskPartenzeCompatibili"
/*
   Questo metodo e' utilizzato per identificare rapidamente eventuali
      incompatibilit… di orario in un PATH.

   In pratica OrariPartenza e' una rappresentazione delle ore di
      partenza in formato binario (ogni bit rappresenta 45 minuti, in
      base alla formula 1 << (Ora/45) ).

   MinPercorrenza rappresenta (in unit… di 45 minuti) il tempo minimo di
      percorrenza.

   MaxPercorrenza il massimo

   La funzione opera ruotando gli OrariPartenza di MinPercorrenza Bytes,
      in modo da avere gli orari di arrivo Minimi.

   Ogni bit settato e' "trascinato" di Margine2 posizioni,
      in modo da coprire tutta la fascia di incertezza sul tempo di
      arrivo + il tempo accettabile di coincidenza.

   Il tutto e' replicato, per le ore che precedono la fascia notturna,
      con un margine di tolleranza aumentato di 5 ore (l' ampiezza della
      fascia notturna)

   Si ritorna la combinazione dei due risultati.

*/
   DWORD Ex_EDI, Ex_EAX, Ex_EBX, Ex_ECX;
   BYTE Ex_CL;

   assert(MaxPercorrenza >= MinPercorrenza);

   // Identifico il minimo orario di arrivo possibile
   Ex_EDI = OrariPartenza;
   Ex_CL  = MinPercorrenza;
   /*
   #include <stdlib.h>
   unsigned long _lrotl(unsigned long val, int count);
   unsigned long _lrotr(unsigned long val, int count);

   Description

   Rotates an unsigned long integer value to the left or right.
   _lrotl rotates the given val to the left count bits. _lrotr rotates the given val to the right count bits.
   */
   //asm ROL EDI,CL
   Ex_EDI = _lrotl(Ex_EDI, Ex_CL);

   Ex_EAX = Ex_EDI ;
   // Identifico tutti gli orari di arrivo possibili
   for (int i = MaxPercorrenza - MinPercorrenza ; i > 0; i-- ) {
      //asm ROL EDI,1
      Ex_EDI = _lrotl(Ex_EDI, 1);

      Ex_EAX |= Ex_EDI;
   } /* endfor */
   // Li limito agli orari in cui so che vi sono realmente degli arrivi
   Ex_EAX &= OrariArrivo;
   Ex_EDI = Ex_EAX;

   #ifdef DBGMASKPA
   int PossibiliArrivi = Ex_EAX;
   #endif

   // Se ho arrivi nelle ore notturne valido tutte le ore notturne
   // successive all' ora di arrivo
   Ex_EBX = Ex_EDI & MASK_NOTTE_45;
   if(Ex_EBX){
      Ex_EBX += Ex_EBX;     // Equivale a << 1 ma pi— veloce
      Ex_ECX  = Ex_EBX;
      Ex_EBX += Ex_EBX;
      Ex_ECX |= Ex_EBX;
      Ex_EBX += Ex_EBX;
      Ex_ECX |= Ex_EBX;
      Ex_EBX += Ex_EBX;
      Ex_ECX |= Ex_EBX;
      Ex_EBX += Ex_EBX;
      Ex_ECX |= Ex_EBX;
      Ex_EBX += Ex_EBX;
      Ex_ECX |= Ex_EBX;
      Ex_ECX &= MASK_NOTTE_45   ;
      Ex_EAX |= Ex_ECX            ;
      Ex_EDI &= ~MASK_NOTTE_45  ; // Perche' ne ho gia' tenuto conto
      Ex_EDI |= PRIMO_GIORNO_45 ;
      Ex_EAX |= PRIMO_GIORNO_45 ;
   }
   for (i = Margine ; i > 0; i-- ) {
      //asm ROL EDI,1
      Ex_EDI = _lrotl(Ex_EDI, 1);

      if(Ex_EDI & MASK_NOTTE_45){
         Ex_EDI |= PRIMO_GIORNO_45 ;
         Ex_EAX |= MASK_NOTTE_45   ;   // Perche' a questo punto tutta la notte e' OK
      }
      Ex_EAX |= Ex_EDI ;
   } /* endfor */

   #ifdef DBGMASKPA
   int Result = Ex_EAX;
   Bprintf3("Part %8.8X Arr %8.8X MinP %i MaxP %i Margine %i PossibiliArrivi %8.8X Ris %8.8X",
      OrariPartenza, OrariArrivo, MinPercorrenza , MaxPercorrenza ,Margine , PossibiliArrivi,Result );
   Ex_EAX = Result ;
   #endif
   return Ex_EAX;

//<<< DWORD MaskPartenzeCompatibili
};

#ifdef OKMAIN
int main(int argc, char *argv[], char *envp[]) {
   int Mgn = atoi(argv[1]);

   MaskPartenzeCompatibili( 0x40408, 0x1010400 , 6,  7, 6);
   /*
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   1, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   2, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   3, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   4, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   5, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   6, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   7, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   8, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,   9, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  10, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  11, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  12, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  13, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  14, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  15, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  16, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  17, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  18, Mgn);
   MaskPartenzeCompatibili( 0x04000000, 0xffff, 1,  19, Mgn);
   */
   return 0;
};
#endif

//----------------------------------------------------------------------------
// PERIODICITA::GiornoPrecedente
//----------------------------------------------------------------------------
void PERIODICITA::GiornoPrecedente(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::GiornoPrecedente"
   // Son partito da questo codice, lo ho assemblato e modificato
   // In modo da gestire uno shift multiword attraverso il Carry Bit
   // Si noti che poiche' il 486 e' little endian lo shift e' a DESTRA.
   // e debbo partire dall' offset piu' alto (28) e poi scendere
   // struct AdHoc{ DWORD dw1; DWORD dw2; DWORD dw3; DWORD dw4; DWORD dw5; DWORD dw6; DWORD dw7; DWORD dw8; };
   // AdHoc & Data = *(AdHoc*)Dati;
   // Data.dw8 >>= 1;
   // Data.dw7 >>= 1;
   // Data.dw6 >>= 1;
   // Data.dw5 >>= 1;
   // Data.dw4 >>= 1;
   // Data.dw3 >>= 1;
   // Data.dw2 >>= 1;
   // Data.dw1 >>= 1;

   // Codice generato dal compilatore
   // asm {
   //     MOV EDI,dword ptr [EBP+8]
   //     SHR dword ptr [EDI+28],1
   //     SHR dword ptr [EDI+24],1
   //     SHR dword ptr [EDI+20],1
   //     SHR dword ptr [EDI+16],1
   //     SHR dword ptr [EDI+12],1
   //     SHR dword ptr [EDI+ 8],1
   //     SHR dword ptr [EDI+ 4],1
   //     SHR dword ptr [EDI   ],1
   //  }

   // Codice modificato a mano
   //asm {
   //    MOV EDI,dword ptr [EBP+8]
   //    SAR dword ptr [EDI+28],1   // Shift Right with carry
   //    RCR dword ptr [EDI+24],1   // Rotate Right with carry
   //    RCR dword ptr [EDI+20],1
   //    RCR dword ptr [EDI+16],1
   //    RCR dword ptr [EDI+12],1
   //    RCR dword ptr [EDI+ 8],1
   //    RCR dword ptr [EDI+ 4],1
   //    RCR dword ptr [EDI   ],1
   //}

   AdHoc & Data = *(AdHoc*)Dati;

   Data.dw1 >>= 1;
   if (Data.dw2 & 0x00000001) Data.dw1 |= 0x80000000;
   Data.dw2 >>= 1;
   if (Data.dw3 & 0x00000001) Data.dw2 |= 0x80000000;
   Data.dw3 >>= 1;
   if (Data.dw4 & 0x00000001) Data.dw3 |= 0x80000000;
   Data.dw4 >>= 1;
   if (Data.dw5 & 0x00000001) Data.dw4 |= 0x80000000;
   Data.dw5 >>= 1;
   if (Data.dw6 & 0x00000001) Data.dw5 |= 0x80000000;
   Data.dw6 >>= 1;
   if (Data.dw7 & 0x00000001) Data.dw6 |= 0x80000000;
   Data.dw7 >>= 1;
   if (Data.dw8 & 0x00000001) Data.dw7 |= 0x80000000;
   Data.dw8 >>= 1;

//<<< void PERIODICITA::GiornoPrecedente
   return;
}
//----------------------------------------------------------------------------
// PERIODICITA::GiornoSeguente
//----------------------------------------------------------------------------
void PERIODICITA::GiornoSeguente(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::GiornoSeguente"
   //asm {
   //   MOV EDI,dword ptr [EBP+8]
   //   SAL dword ptr [EDI+ 0],1   // Shift Left with carry
   //   RCL dword ptr [EDI+ 4],1   // Rotate Left with carry
   //   RCL dword ptr [EDI+ 8],1
   //   RCL dword ptr [EDI+12],1
   //   RCL dword ptr [EDI+16],1
   //   RCL dword ptr [EDI+20],1
   //   RCL dword ptr [EDI+24],1
   //   RCL dword ptr [EDI+28],1
   //}
   //struct AdHoc{ DWORD dw1; DWORD dw2; DWORD dw3; DWORD dw4; DWORD dw5; DWORD dw6; DWORD dw7; DWORD dw8; };

   AdHoc & Data = *(AdHoc*)Dati;

   Data.dw8 <<= 1;
   if (Data.dw7 & 0x80000000) Data.dw8 |= 0x00000001;
   Data.dw7 <<= 1;
   if (Data.dw6 & 0x80000000) Data.dw7 |= 0x00000001;
   Data.dw6 <<= 1;
   if (Data.dw5 & 0x80000000) Data.dw6 |= 0x00000001;
   Data.dw5 <<= 1;
   if (Data.dw4 & 0x80000000) Data.dw5 |= 0x00000001;
   Data.dw4 <<= 1;
   if (Data.dw3 & 0x80000000) Data.dw4 |= 0x00000001;
   Data.dw3 <<= 1;
   if (Data.dw2 & 0x80000000) Data.dw3 |= 0x00000001;
   Data.dw2 <<= 1;
   if (Data.dw1 & 0x80000000) Data.dw2 |= 0x00000001;
   Data.dw1 <<= 1;

   return;
}


// Truccaccio da memorizzare: salto indiretto
//_EAX = Indice * 4;
//asm {
//  LEA  EBX,$ + 11
//  ADD  EBX,EAX
//  MOV  EAX,EDI
//  JMP  EBX
//};
