#include "dummy.h"
//----------------------------------------------------------------------------
// Funzioni di base
//----------------------------------------------------------------------------

#ifndef HO_MINMAX_H
#define HO_MINMAX_H        // Indica che questo file Š gi… stato incluso;

#include <os2def.h>
#include <math.h>

// begin EMS001 VA commento funzioni inline min, max
/*
inline const int _export  min(int a,int b)
{
   if(a>b)return b;
   return a;
}

inline const USHORT _export min(USHORT a,USHORT b)
{
   if(a>b)return b;
   return a;
}

inline const long  _export min(long a,long b)
{
   if(a>b)return b;
   return a;
}

inline const int  _export max(int a,int b)
{
   if(a<b)return b;
   return a;
}

inline const USHORT _export max(USHORT a,USHORT b)
{
   if(a<b)return b;
   return a;
}

inline const long  _export max(long a,long b)
{
   if(a<b)return b;
   return a;
}
*/
// end EMS001 VA

//----------------------------------------------------------------------------
// Confronto tra due RECTL
//----------------------------------------------------------------------------

// Confronta ritorna il massimo delta tra le coordinate
inline ULONG _export Confronta(const RECTL & A, const RECTL & B)
{
   ULONG Delta = labs(A.xLeft - B.xLeft);
   Delta = max(Delta,labs(A.xRight - B.xRight));
   Delta = max(Delta,labs(A.yBottom - B.yBottom));
   Delta = max(Delta,labs(A.yTop - B.yTop));
   return Delta;
}

//----------------------------------------------------------------------------
// RECTL da coordinate
//----------------------------------------------------------------------------


inline RECTL _export Retl(LONG Xl, LONG Yb, LONG Xr, LONG Yt)
{
   RECTL A;
   A.xLeft    = Xl;
   A.xRight   = Xr;
   A.yBottom  = Yb;
   A.yTop     = Yt;
   return A;
}

#endif
