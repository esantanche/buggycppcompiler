/****************************** Module Header ******************************\
*
* Module Name: BSE.H
*
* This file includes the definitions necessary for writing Base OS/2 applications.
*
* Copyright (c) 1987, 1992  IBM Corporation
*
*
*
* ===========================================================================
*
* The following symbols are used in this file for conditional sections.
*
*   INCL_BASE      -  ALL of OS/2 Base
*   INCL_DOS       -  OS/2 DOS Kernel
*   INCL_SUB       -  OS/2 VIO/KBD/MOU
*   INCL_DOSERRORS -  OS/2 Errors         - only included if symbol defined
*   INCL_ORDINALS  -  OS/2 Ordinals       - only included if symbol defined
\***************************************************************************/

#ifdef __IBMC__
#pragma checkout( suspend )
   #ifndef __CHKHDR__
      #pragma checkout( suspend )
   #endif
#pragma checkout( resume )
#endif

#ifndef __BSE__
#define __BSE__

#define INCL_BASEINCLUDED

/* if INCL_BASE defined then define all the symbols */

#ifdef INCL_BASE
   #define INCL_DOS
   #define INCL_SUB
   #define INCL_DOSERRORS
#endif /* INCL_BASE */

#include <bsedos.h>       /* Base definitions */


#ifdef INCL_DOSDEVIOCTL
   #include <bsedev.h>    /* Structures and constants for DosDevIOCtl */
#endif /* INCL_DOSDEVIOCTL */

#include <bsesub.h>       /* VIO/KBD/MOU definitions */
#include <bseerr.h>       /* Base error code definitions */

#ifdef INCL_ORDINALS
#include <bseord.h>     /* ordinals */
#endif /* INCL_ORDINALS */

#endif /* __BSE__ */

#ifdef __IBMC__
#pragma checkout( suspend )
   #ifndef __CHKHDR__
      #pragma checkout( resume )
   #endif
#pragma checkout( resume )
#endif
