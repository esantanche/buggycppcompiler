/*static char *SCCSID = "@(#)bsesub.h   6.3 91/05/26";*/
/***************************************************************************\
*
* Module Name: BSESUB.H
*
* OS/2 Base Include File
*
* Copyright (c) 1987 - 1991 International Business Machines Corporation
*
*****************************************************************************
*
* Subcomponents marked with "+" are partially included by default
*   #define:                To include:
*
*   INCL_KBD                KBD
*   INCL_VIO                VIO
*   INCL_MOU                MOU
\***************************************************************************/

#pragma pack(2)

#ifdef INCL_SUB

#define INCL_KBD
#define INCL_VIO
#define INCL_MOU

#endif /* INCL_SUB */

#ifdef INCL_KBD

#define KbdCharIn       Kbd16CharIn
#define KbdClose        Kbd16Close
#define KbdDeRegister   Kbd16DeRegister
#define KbdFlushBuffer  Kbd16FlushBuffer
#define KbdFreeFocus    Kbd16FreeFocus
#define KbdGetCp        Kbd16GetCp
#define KbdGetFocus     Kbd16GetFocus
#define KbdGetHWID      Kbd16GetHWID
#define KbdGetStatus    Kbd16GetStatus
#define KbdOpen         Kbd16Open
#define KbdPeek         Kbd16Peek
#define KbdRegister     Kbd16Register
#define KbdSetCp        Kbd16SetCp
#define KbdSetCustXt    Kbd16SetCustXt
#define KbdSetFgnd      Kbd16SetFgnd
#define KbdSetHWID      Kbd16SetHWID
#define KbdSetStatus    Kbd16SetStatus
#define KbdStringIn     Kbd16StringIn
#define KbdSynch        Kbd16Synch
#define KbdXlate        Kbd16Xlate

typedef unsigned short  HKBD;
typedef HKBD   FAR16PTR PHKBD16;


APIRET16  APIENTRY16    KbdRegister (PSZ16 pszModName, PSZ16 pszEntryPt, ULONG FunMask);

#define KR_KBDCHARIN    0x00000001L
#define KR_KBDPEEK      0x00000002L
#define KR_KBDFLUSHBUFFER       0x00000004L
#define KR_KBDGETSTATUS 0x00000008L
#define KR_KBDSETSTATUS 0x00000010L
#define KR_KBDSTRINGIN  0x00000020L
#define KR_KBDOPEN      0x00000040L
#define KR_KBDCLOSE     0x00000080L
#define KR_KBDGETFOCUS  0x00000100L
#define KR_KBDFREEFOCUS 0x00000200L
#define KR_KBDGETCP     0x00000400L
#define KR_KBDSETCP     0x00000800L
#define KR_KBDXLATE     0x00001000L
#define KR_KBDSETCUSTXT 0x00002000L

#define IO_WAIT                    0
#define IO_NOWAIT                  1

APIRET16  APIENTRY16        KbdDeRegister (void);

/* KBDKEYINFO structure, for KbdCharIn and KbdPeek */

typedef struct _KBDKEYINFO {    /* kbci */
        UCHAR    chChar;
        UCHAR    chScan;
        UCHAR    fbStatus;
        UCHAR    bNlsShift;
        USHORT   fsState;
        ULONG    time;
        }KBDKEYINFO;
typedef KBDKEYINFO FAR16PTR PKBDKEYINFO;

APIRET16  APIENTRY16        KbdCharIn (PKBDKEYINFO pkbci, USHORT fWait, HKBD hkbd);
APIRET16  APIENTRY16        KbdPeek (PKBDKEYINFO pkbci, HKBD hkbd);

/* structure for KbdStringIn() */

typedef struct _STRINGINBUF {   /* kbsi */
        USHORT cb;
        USHORT cchIn;
        } STRINGINBUF;
typedef STRINGINBUF FAR16PTR PSTRINGINBUF;

APIRET16  APIENTRY16        KbdStringIn (PCHAR16 pch, PSTRINGINBUF pchIn, USHORT fsWait,
                             HKBD hkbd);

APIRET16  APIENTRY16        KbdFlushBuffer (HKBD hkbd);

/* KBDINFO.fsMask */

#define KEYBOARD_ECHO_ON                0x0001
#define KEYBOARD_ECHO_OFF               0x0002
#define KEYBOARD_BINARY_MODE            0x0004
#define KEYBOARD_ASCII_MODE             0x0008
#define KEYBOARD_MODIFY_STATE           0x0010
#define KEYBOARD_MODIFY_INTERIM         0x0020
#define KEYBOARD_MODIFY_TURNAROUND      0x0040
#define KEYBOARD_2B_TURNAROUND          0x0080
#define KEYBOARD_SHIFT_REPORT           0x0100

#ifndef INCL_DOSDEVIOCTL  /* following constants defined in bsedev.h */

/* KBDINFO.fsState/KBDKEYINFO.fsState/KBDTRANS.fsState */

#define KBDSTF_RIGHTSHIFT       0x0001
#define KBDSTF_LEFTSHIFT        0x0002
#define KBDSTF_CONTROL          0x0004
#define KBDSTF_ALT              0x0008
#define KBDSTF_SCROLLLOCK_ON    0x0010
#define KBDSTF_NUMLOCK_ON       0x0020
#define KBDSTF_CAPSLOCK_ON      0x0040
#define KBDSTF_INSERT_ON        0x0080
#define KBDSTF_LEFTCONTROL      0x0100
#define KBDSTF_LEFTALT          0x0200
#define KBDSTF_RIGHTCONTROL     0x0400
#define KBDSTF_RIGHTALT         0x0800
#define KBDSTF_SCROLLLOCK       0x1000
#define KBDSTF_NUMLOCK          0x2000
#define KBDSTF_CAPSLOCK         0x4000
#define KBDSTF_SYSREQ           0x8000

#endif  /* INCL_DOSDEVIOCTL */

/* KBDINFO structure, for KbdSet/GetStatus */
typedef struct _KBDINFO {       /* kbst */
        USHORT cb;
        USHORT fsMask;
        USHORT chTurnAround;
        USHORT fsInterim;
        USHORT fsState;
        }KBDINFO;
typedef KBDINFO FAR16PTR PKBDINFO;

APIRET16  APIENTRY16    KbdSetStatus (PKBDINFO pkbdinfo, HKBD hkbd);
APIRET16  APIENTRY16    KbdGetStatus (PKBDINFO pkbdinfo, HKBD hdbd);

APIRET16  APIENTRY16    KbdSetCp (USHORT usReserved, USHORT pidCP, HKBD hdbd);
APIRET16  APIENTRY16    KbdGetCp (ULONG ulReserved, PUSHORT16 pidCP, HKBD hkbd);

APIRET16  APIENTRY16    KbdOpen (PHKBD16 PHKBD16);
APIRET16  APIENTRY16    KbdClose (HKBD hkbd);

APIRET16  APIENTRY16    KbdGetFocus (USHORT fWait, HKBD hkbd);
APIRET16  APIENTRY16    KbdFreeFocus (HKBD hkbd);

APIRET16  APIENTRY16    KbdSynch (USHORT fsWait);

APIRET16  APIENTRY16    KbdSetFgnd(VOID);

/* structure for KbdGetHWID() */
typedef struct _KBDHWID {       /* kbhw */
        USHORT cb;
        USHORT idKbd;
        USHORT usReserved1;
        USHORT usReserved2;
        } KBDHWID;
typedef KBDHWID FAR16PTR PKBDHWID;

APIRET16  APIENTRY16    KbdGetHWID (PKBDHWID pkbdhwid, HKBD hkbd);
APIRET16  APIENTRY16    KbdSetHWID (PKBDHWID pkbdhwid, HKBD hkbd);

/* KBDTRANS.fbStatus */

#define KBDTRF_SHIFT_KEY_IN             0x01
#define KBDTRF_CONVERSION_REQUEST       0x20
#define KBDTRF_FINAL_CHAR_IN            0x40
#define KBDTRF_INTERIM_CHAR_IN          0x80

/* structure for KbdXlate() */
typedef struct _KBDTRANS {      /* kbxl */
        UCHAR      chChar;
        UCHAR      chScan;
        UCHAR      fbStatus;
        UCHAR      bNlsShift;
        USHORT     fsState;
        ULONG      time;
        USHORT     fsDD;
        USHORT     fsXlate;
        USHORT     fsShift;
        USHORT     sZero;
        } KBDTRANS;
typedef KBDTRANS FAR16PTR PKBDTRANS;

APIRET16  APIENTRY16    KbdXlate (PKBDTRANS pkbdtrans, HKBD hkbd);
APIRET16  APIENTRY16    KbdSetCustXt (PUSHORT16 usCodePage, HKBD hkbd);

#endif /* INCL_KBD */

#ifdef INCL_VIO

#define VioCheckCharType        Vio16CheckCharType
#define VioDeRegister   Vio16DeRegister
#define VioEndPopUp     Vio16EndPopUp
#define VioGetAnsi      Vio16GetAnsi
#define VioGetBuf       Vio16GetBuf
#define VioGetConfig    Vio16GetConfig
#define VioGetCp        Vio16GetCp
#define VioGetCurPos    Vio16GetCurPos
#define VioGetCurType   Vio16GetCurType
#define VioGetFont      Vio16GetFont
#define VioGetMode      Vio16GetMode
#define VioGetPhysBuf   Vio16GetPhysBuf
#define VioGetState     Vio16GetState
#define VioModeUndo     Vio16ModeUndo
#define VioModeWait     Vio16ModeWait
#define VioPopUp        Vio16PopUp
#define VioPrtSc        Vio16PrtSc
#define VioPrtScToggle  Vio16PrtScToggle
#define VioReadCellStr  Vio16ReadCellStr
#define VioReadCharStr  Vio16ReadCharStr
#define VioRedrawSize   Vio16RedrawSize
#define VioRegister     Vio16Register
#define VioSavRedrawUndo        Vio16SavRedrawUndo
#define VioSavRedrawWait        Vio16SavRedrawWait
#define VioScrLock      Vio16ScrLock
#define VioScrUnLock    Vio16ScrUnLock
#define VioScrollDn     Vio16ScrollDn
#define VioScrollLf     Vio16ScrollLf
#define VioScrollRt     Vio16ScrollRt
#define VioScrollUp     Vio16ScrollUp
#define VioSetAnsi      Vio16SetAnsi
#define VioSetCp        Vio16SetCp
#define VioSetCurPos    Vio16SetCurPos
#define VioSetCurType   Vio16SetCurType
#define VioSetFont      Vio16SetFont
#define VioSetMode      Vio16SetMode
#define VioSetState     Vio16SetState
#define VioShowBuf      Vio16ShowBuf
#define VioWrtCellStr   Vio16WrtCellStr
#define VioWrtCharStr   Vio16WrtCharStr
#define VioWrtCharStrAtt        Vio16WrtCharStrAtt
#define VioWrtNAttr     Vio16WrtNAttr
#define VioWrtNCell     Vio16WrtNCell
#define VioWrtNChar     Vio16WrtNChar
#define VioWrtTTY       Vio16WrtTTY

typedef unsigned short      HVIO;
typedef HVIO    FAR16PTR    PHVIO16;

APIRET16  APIENTRY16    VioRegister (PSZ16 pszModName, PSZ16 pszEntryName, ULONG flFun1,
                             ULONG flFun2);

/* first parameter registration constants   */
#define VR_VIOGETCURPOS         0x00000001L
#define VR_VIOGETCURTYPE        0x00000002L
#define VR_VIOGETMODE           0x00000004L
#define VR_VIOGETBUF            0x00000008L
#define VR_VIOGETPHYSBUF        0x00000010L
#define VR_VIOSETCURPOS         0x00000020L
#define VR_VIOSETCURTYPE        0x00000040L
#define VR_VIOSETMODE           0x00000080L
#define VR_VIOSHOWBUF           0x00000100L
#define VR_VIOREADCHARSTR       0x00000200L
#define VR_VIOREADCELLSTR       0x00000400L
#define VR_VIOWRTNCHAR          0x00000800L
#define VR_VIOWRTNATTR          0x00001000L
#define VR_VIOWRTNCELL          0x00002000L
#define VR_VIOWRTTTY            0x00004000L
#define VR_VIOWRTCHARSTR        0x00008000L

#define VR_VIOWRTCHARSTRATT     0x00010000L
#define VR_VIOWRTCELLSTR        0x00020000L
#define VR_VIOSCROLLUP          0x00040000L
#define VR_VIOSCROLLDN          0x00080000L
#define VR_VIOSCROLLLF          0x00100000L
#define VR_VIOSCROLLRT          0x00200000L
#define VR_VIOSETANSI           0x00400000L
#define VR_VIOGETANSI           0x00800000L
#define VR_VIOPRTSC             0x01000000L
#define VR_VIOSCRLOCK           0x02000000L
#define VR_VIOSCRUNLOCK         0x04000000L
#define VR_VIOSAVREDRAWWAIT     0x08000000L
#define VR_VIOSAVREDRAWUNDO     0x10000000L
#define VR_VIOPOPUP             0x20000000L
#define VR_VIOENDPOPUP          0x40000000L
#define VR_VIOPRTSCTOGGLE       0x80000000L

/* second parameter registration constants  */
#define VR_VIOMODEWAIT  0x00000001L
#define VR_VIOMODEUNDO  0x00000002L
#define VR_VIOGETFONT   0x00000004L
#define VR_VIOGETCONFIG 0x00000008L
#define VR_VIOSETCP     0x00000010L
#define VR_VIOGETCP     0x00000020L
#define VR_VIOSETFONT   0x00000040L
#define VR_VIOGETSTATE  0x00000080L
#define VR_VIOSETSTATE  0x00000100L

APIRET16  APIENTRY16        VioDeRegister (void);

APIRET16  APIENTRY16        VioGetBuf (PULONG16 pLVB, PUSHORT16 pcbLVB, HVIO hvio);

APIRET16  APIENTRY16        VioGetCurPos (PUSHORT16 pusRow, PUSHORT16 pusColumn, HVIO hvio);
APIRET16  APIENTRY16        VioSetCurPos (USHORT usRow, USHORT usColumn, HVIO hvio);

/* structure for VioSet/GetCurType() */
typedef struct _VIOCURSORINFO { /* vioci */
        USHORT   yStart;
        USHORT   cEnd;
        USHORT   cx;
        USHORT   attr;
        } VIOCURSORINFO;
typedef VIOCURSORINFO FAR16PTR PVIOCURSORINFO;

APIRET16  APIENTRY16        VioGetCurType (PVIOCURSORINFO pvioCursorInfo, HVIO hvio);
APIRET16  APIENTRY16        VioSetCurType (PVIOCURSORINFO pvioCursorInfo, HVIO hvio);

/* VIOMODEINFO.color constants */

#define COLORS_2        0x0001
#define COLORS_4        0x0002
#define COLORS_16       0x0004

#pragma pack(1)

/* structure for VioSet/GetMode() */
typedef struct _VIOMODEINFO {   /* viomi */
        USHORT cb;
        UCHAR  fbType;
        UCHAR  color;
        USHORT col;
        USHORT row;
        USHORT hres;
        USHORT vres;
        UCHAR  fmt_ID;
        UCHAR  attrib;
        ULONG  buf_addr;
        ULONG  buf_length;
        ULONG  full_length;
        ULONG  partial_length;
        PCHAR16    ext_data_addr;
        } VIOMODEINFO;
typedef VIOMODEINFO FAR16PTR PVIOMODEINFO;

#pragma pack(2)

#define VGMT_OTHER      0x01
#define VGMT_GRAPHICS   0x02
#define VGMT_DISABLEBURST       0x04

APIRET16  APIENTRY16        VioGetMode (PVIOMODEINFO pvioModeInfo, HVIO hvio);
APIRET16  APIENTRY16        VioSetMode (PVIOMODEINFO pvioModeInfo, HVIO hvio);

/* structure for VioGetPhysBuf() */

typedef struct _VIOPHYSBUF {    /* viopb */
        PBYTE    pBuf;
        ULONG    cb;
        SEL      asel[1];
        } VIOPHYSBUF;
typedef VIOPHYSBUF FAR16PTR PVIOPHYSBUF;

APIRET16  APIENTRY16    VioGetPhysBuf (PVIOPHYSBUF pvioPhysBuf, USHORT usReserved);

APIRET16  APIENTRY16    VioReadCellStr (PCHAR16 pchCellStr, PUSHORT16 pcb, USHORT usRow,
                                USHORT usColumn, HVIO hvio);
APIRET16  APIENTRY16    VioReadCharStr (PCHAR16 pchCellStr, PUSHORT16 pcb, USHORT usRow,
                                USHORT usColumn, HVIO hvio);
APIRET16  APIENTRY16    VioWrtCellStr (PCHAR16 pchCellStr, USHORT cb, USHORT usRow,
                               USHORT usColumn, HVIO hvio);
APIRET16  APIENTRY16    VioWrtCharStr (PCHAR16 pchStr, USHORT cb, USHORT usRow,
                               USHORT usColumn, HVIO hvio);

APIRET16  APIENTRY16    VioScrollDn (USHORT usTopRow, USHORT usLeftCol,
                             USHORT usBotRow, USHORT usRightCol,
                             USHORT cbLines, PBYTE16 pCell, HVIO hvio);
APIRET16  APIENTRY16    VioScrollUp (USHORT usTopRow, USHORT usLeftCol,
                             USHORT usBotRow, USHORT usRightCol,
                             USHORT cbLines, PBYTE16 pCell, HVIO hvio);
APIRET16  APIENTRY16    VioScrollLf (USHORT usTopRow, USHORT usLeftCol,
                             USHORT usBotRow, USHORT usRightCol,
                             USHORT cbCol, PBYTE16 pCell, HVIO hvio);
APIRET16  APIENTRY16    VioScrollRt (USHORT usTopRow, USHORT usLeftCol,
                             USHORT usBotRow, USHORT usRightCol,
                             USHORT cbCol, PBYTE16 pCell, HVIO hvio);

APIRET16  APIENTRY16    VioWrtNAttr (PBYTE16 pAttr, USHORT cb, USHORT usRow,
                             USHORT usColumn, HVIO hvio);
APIRET16  APIENTRY16    VioWrtNCell (PBYTE16 pCell, USHORT cb, USHORT usRow,
                             USHORT usColumn, HVIO hvio);
APIRET16  APIENTRY16    VioWrtNChar (PCHAR16 pchChar, USHORT cb, USHORT usRow,
                             USHORT usColumn, HVIO hvio);
APIRET16  APIENTRY16    VioWrtTTY (PCHAR16 pch, USHORT cb, HVIO hvio);
APIRET16  APIENTRY16    VioWrtCharStrAtt (PCHAR16 pch, USHORT cb, USHORT usRow,
                                  USHORT usColumn, PBYTE16 pAttr, HVIO hvio);

#define VCC_SBCSCHAR               0
#define VCC_DBCSFULLCHAR           1
#define VCC_DBCS1STHALF            2
#define VCC_DBCS2NDHALF            3

APIRET16  APIENTRY16    VioCheckCharType (PUSHORT16 pType, USHORT usRow,
                                  USHORT usColumn, HVIO hvio);

APIRET16  APIENTRY16    VioShowBuf (USHORT offLVB, USHORT cb, HVIO hvio);


#define ANSI_ON                    1
#define ANSI_OFF                   0

APIRET16  APIENTRY16    VioSetAnsi (USHORT fAnsi, HVIO hvio);
APIRET16  APIENTRY16    VioGetAnsi (PUSHORT16 pfAnsi, HVIO hvio);

APIRET16  APIENTRY16    VioPrtSc (HVIO hvio);
APIRET16  APIENTRY16    VioPrtScToggle (HVIO hvio);

#define VSRWI_SAVEANDREDRAW        0
#define VSRWI_REDRAW               1

#define VSRWN_SAVE                 0
#define VSRWN_REDRAW               1

#define UNDOI_GETOWNER             0
#define UNDOI_RELEASEOWNER         1

#define UNDOK_ERRORCODE            0
#define UNDOK_TERMINATE            1

APIRET16  APIENTRY16    VioRedrawSize (PULONG16 pcbRedraw);
APIRET16  APIENTRY16    VioSavRedrawWait (USHORT usRedrawInd, PUSHORT16 pNotifyType,
                                  USHORT usReserved);
APIRET16  APIENTRY16    VioSavRedrawUndo (USHORT usOwnerInd, USHORT usKillInd,
                                  USHORT usReserved);

#define VMWR_POPUP                 0
#define VMWN_POPUP                 0

APIRET16  APIENTRY16    VioModeWait (USHORT usReqType, PUSHORT16 pNotifyType,
                             USHORT usReserved);
APIRET16  APIENTRY16    VioModeUndo (USHORT usOwnerInd, USHORT usKillInd,
                             USHORT usReserved);

#define LOCKIO_NOWAIT              0
#define LOCKIO_WAIT                1

#define LOCK_SUCCESS               0
#define LOCK_FAIL                  1

APIRET16  APIENTRY16    VioScrLock (USHORT fWait, PUCHAR16 pfNotLocked, HVIO hvio);
APIRET16  APIENTRY16    VioScrUnLock (HVIO hvio);

#define VP_NOWAIT       0x0000
#define VP_WAIT         0x0001
#define VP_OPAQUE       0x0000
#define VP_TRANSPARENT  0x0002

APIRET16  APIENTRY16    VioPopUp (PUSHORT16 pfWait, HVIO hvio);
APIRET16  APIENTRY16    VioEndPopUp (HVIO hvio);

/* VIOCONFIGINFO.adapter constants */

#define DISPLAY_MONOCHROME      0x0000
#define DISPLAY_CGA             0x0001
#define DISPLAY_EGA             0x0002
#define DISPLAY_VGA             0x0003
#define DISPLAY_8514A           0x0007

/* VIOCONFIGINFO.display constants */

#define MONITOR_MONOCHROME      0x0000
#define MONITOR_COLOR           0x0001
#define MONITOR_ENHANCED        0x0002
#define MONITOR_8503            0x0003
#define MONITOR_851X_COLOR      0x0004
#define MONITOR_8514            0x0009

/* structure for VioGetConfig() */

typedef struct _VIOCONFIGINFO { /* vioin */
        USHORT  cb;
        USHORT  adapter;
        USHORT  display;
        ULONG   cbMemory;
        USHORT  Configuration;
        USHORT  VDHVersion;
        USHORT  Flags;
        ULONG   HWBufferSize;
        ULONG   FullSaveSize;
        ULONG   PartSaveSize;
        USHORT  EMAdaptersOFF;
        USHORT  EMDisplaysOFF;
        } VIOCONFIGINFO;
typedef VIOCONFIGINFO FAR16PTR PVIOCONFIGINFO;


#define VIO_CONFIG_CURRENT         0
#define VIO_CONFIG_PRIMARY         1
#define VIO_CONFIG_SECONDARY       2

APIRET16  APIENTRY16    VioGetConfig (USHORT usConfigId, PVIOCONFIGINFO pvioin,
                              HVIO hvio);

/* structure for VioGet/SetFont() */
typedef struct _VIOFONTINFO {   /* viofi */
        USHORT  cb;
        USHORT  type;
        USHORT  cxCell;
        USHORT  cyCell;
        PVOID   pbData;
        USHORT  cbData;
        } VIOFONTINFO;
typedef VIOFONTINFO FAR16PTR PVIOFONTINFO;

#define VGFI_GETCURFONT            0
#define VGFI_GETROMFONT            1

APIRET16  APIENTRY16    VioGetFont (PVIOFONTINFO pviofi, HVIO hvio);
APIRET16  APIENTRY16    VioSetFont (PVIOFONTINFO pviofi, HVIO hvio);

APIRET16  APIENTRY16    VioGetCp (USHORT usReserved, PUSHORT16 pIdCodePage, HVIO hvio);
APIRET16  APIENTRY16    VioSetCp (USHORT usReserved, USHORT idCodePage, HVIO hvio);

typedef struct _VIOPALSTATE {   /* viopal */
        USHORT  cb;
        USHORT  type;
        USHORT  iFirst;
        USHORT  acolor[1];
        }VIOPALSTATE;
typedef VIOPALSTATE FAR16PTR PVIOPALSTATE;

typedef struct _VIOOVERSCAN {   /* vioos */
        USHORT  cb;
        USHORT  type;
        USHORT  color;
        }VIOOVERSCAN;
typedef VIOOVERSCAN FAR16PTR PVIOOVERSCAN;

typedef struct _VIOINTENSITY {  /* vioint */
        USHORT  cb;
        USHORT  type;
        USHORT  fs;
        }VIOINTENSITY;
typedef VIOINTENSITY FAR16PTR PVIOINTENSITY;

typedef struct _VIOCOLORREG {  /* viocreg */
        USHORT  cb;
        USHORT  type;
        USHORT  firstcolorreg;
        USHORT  numcolorregs;
        PCHAR16     colorregaddr;
        }VIOCOLORREG;
typedef VIOCOLORREG FAR16PTR PVIOCOLORREG;

typedef struct _VIOSETULINELOC {  /* viouline */
        USHORT  cb;
        USHORT  type;
        USHORT  scanline;
        }VIOSETULINELOC;
typedef VIOSETULINELOC FAR16PTR PVIOSETULINELOC;

typedef struct _VIOSETTARGET {  /* viosett */
        USHORT  cb;
        USHORT  type;
        USHORT  defaultalgorithm;
        }VIOSETTARGET;
typedef VIOSETTARGET FAR16PTR PVIOSETTARGET;

APIRET16  APIENTRY16    VioGetState (PVOID pState, HVIO hvio);
APIRET16  APIENTRY16    VioSetState (PVOID pState, HVIO hvio);

#endif /* INCL_VIO */

#ifdef INCL_MOU

#define MouClose        Mou16Close
#define MouDeRegister   Mou16DeRegister
#define MouDrawPtr      Mou16DrawPtr
#define MouFlushQue     Mou16FlushQue
#define MouGetDevStatus Mou16GetDevStatus
#define MouGetEventMask Mou16GetEventMask
#define MouGetNumButtons        Mou16GetNumButtons
#define MouGetNumMickeys        Mou16GetNumMickeys
#define MouGetNumQueEl  Mou16GetNumQueEl
#define MouGetPtrPos    Mou16GetPtrPos
#define MouGetPtrShape  Mou16GetPtrShape
#define MouGetScaleFact Mou16GetScaleFact
#define MouGetThreshold Mou16GetThreshold
#define MouInitReal     Mou16InitReal
#define MouOpen         Mou16Open
#define MouReadEventQue Mou16ReadEventQue
#define MouRegister     Mou16Register
#define MouRemovePtr    Mou16RemovePtr
#define MouSetDevStatus Mou16SetDevStatus
#define MouSetEventMask Mou16SetEventMask
#define MouSetPtrPos    Mou16SetPtrPos
#define MouSetPtrShape  Mou16SetPtrShape
#define MouSetScaleFact Mou16SetScaleFact
#define MouSetThreshold Mou16SetThreshold
#define MouSynch        Mou16Synch
 
typedef unsigned short  HMOU;
typedef HMOU    FAR16PTR   PHMOU;

APIRET16  APIENTRY16    MouRegister (PSZ pszModName, PSZ pszEntryName, ULONG flFuns);

#define MR_MOUGETNUMBUTTONS     0x00000001L
#define MR_MOUGETNUMMICKEYS     0x00000002L
#define MR_MOUGETDEVSTATUS      0x00000004L
#define MR_MOUGETNUMQUEEL       0x00000008L
#define MR_MOUREADEVENTQUE      0x00000010L
#define MR_MOUGETSCALEFACT      0x00000020L
#define MR_MOUGETEVENTMASK      0x00000040L
#define MR_MOUSETSCALEFACT      0x00000080L
#define MR_MOUSETEVENTMASK      0x00000100L
#define MR_MOUOPEN              0x00000800L
#define MR_MOUCLOSE             0x00001000L
#define MR_MOUGETPTRSHAPE       0x00002000L
#define MR_MOUSETPTRSHAPE       0x00004000L
#define MR_MOUDRAWPTR           0x00008000L
#define MR_MOUREMOVEPTR         0x00010000L
#define MR_MOUGETPTRPOS         0x00020000L
#define MR_MOUSETPTRPOS         0x00040000L
#define MR_MOUINITREAL          0x00080000L
#define MR_MOUSETDEVSTATUS      0x00100000L

APIRET16  APIENTRY16    MouDeRegister (void);

APIRET16  APIENTRY16    MouFlushQue (HMOU hmou);

#define MHK_BUTTON1     0x0001
#define MHK_BUTTON2     0x0002
#define MHK_BUTTON3     0x0004

/* structure for MouGet/SetPtrPos() */
typedef struct _PTRLOC {    /* moupl */
        USHORT row;
        USHORT col;
        } PTRLOC;
typedef PTRLOC FAR16PTR PPTRLOC;

APIRET16  APIENTRY16    MouGetPtrPos (PPTRLOC pmouLoc, HMOU hmou);
APIRET16  APIENTRY16    MouSetPtrPos (PPTRLOC pmouLoc, HMOU hmou);

/* structure for MouGet/SetPtrShape() */
typedef struct _PTRSHAPE {  /* moups */
        USHORT cb;
        USHORT col;
        USHORT row;
        USHORT colHot;
        USHORT rowHot;
        } PTRSHAPE;
typedef PTRSHAPE FAR16PTR PPTRSHAPE;

APIRET16  APIENTRY16    MouSetPtrShape (PBYTE16 pBuf, PPTRSHAPE pmoupsInfo, HMOU hmou);
APIRET16  APIENTRY16    MouGetPtrShape (PBYTE16 pBuf, PPTRSHAPE pmoupsInfo, HMOU hmou);

/* MouGetDevStatus/MouSetDevStatus device status constants */

#define MOUSE_QUEUEBUSY         0x0001
#define MOUSE_BLOCKREAD         0x0002
#define MOUSE_FLUSH             0x0004
#define MOUSE_UNSUPPORTED_MODE  0x0008
#define MOUSE_DISABLED          0x0100
#define MOUSE_MICKEYS           0x0200

APIRET16  APIENTRY16    MouGetDevStatus (PUSHORT16 pfsDevStatus, HMOU hmou);

APIRET16  APIENTRY16    MouGetNumButtons (PUSHORT16 pcButtons, HMOU hmou);
APIRET16  APIENTRY16    MouGetNumMickeys (PUSHORT16 pcMickeys, HMOU hmou);

/* MouReadEventQue */

#define MOU_NOWAIT      0x0000
#define MOU_WAIT        0x0001

/* structure for MouReadEventQue() */
typedef struct _MOUEVENTINFO {  /* mouev */
        USHORT fs;
        ULONG  time;
        SHORT row;
        SHORT col;
        }MOUEVENTINFO;
typedef MOUEVENTINFO FAR16PTR PMOUEVENTINFO;

APIRET16  APIENTRY16    MouReadEventQue (PMOUEVENTINFO pmouevEvent, PUSHORT16 pfWait,
                                 HMOU hmou);

/* structure for MouGetNumQueEl() */
typedef struct _MOUQUEINFO {    /* mouqi */
        USHORT cEvents;
        USHORT cmaxEvents;
        } MOUQUEINFO;
typedef MOUQUEINFO FAR16PTR PMOUQUEINFO;

APIRET16  APIENTRY16    MouGetNumQueEl (PMOUQUEINFO qmouqi, HMOU hmou);

/* MouGetEventMask/MouSetEventMask events */

#define MOUSE_MOTION                0x0001
#define MOUSE_MOTION_WITH_BN1_DOWN  0x0002
#define MOUSE_BN1_DOWN              0x0004
#define MOUSE_MOTION_WITH_BN2_DOWN  0x0008
#define MOUSE_BN2_DOWN              0x0010
#define MOUSE_MOTION_WITH_BN3_DOWN  0x0020
#define MOUSE_BN3_DOWN              0x0040

APIRET16  APIENTRY16    MouGetEventMask (PUSHORT16 pfsEvents, HMOU hmou);
APIRET16  APIENTRY16    MouSetEventMask (PUSHORT16 pfsEvents, HMOU hmou);


/* structure for MouGet/SetScaleFact() */
typedef struct _SCALEFACT { /* mousc */
        USHORT rowScale;
        USHORT colScale;
        } SCALEFACT;
typedef SCALEFACT FAR16PTR PSCALEFACT;

APIRET16  APIENTRY16    MouGetScaleFact (PSCALEFACT pmouscFactors, HMOU hmou);
APIRET16  APIENTRY16    MouSetScaleFact (PSCALEFACT pmouscFactors, HMOU hmou);

APIRET16  APIENTRY16    MouOpen (PSZ16 pszDvrName, PHMOU phmou);
APIRET16  APIENTRY16    MouClose (HMOU hmou);

/* structure for MouRemovePtr() */
typedef struct _NOPTRRECT { /* mourt */
        USHORT row;
        USHORT col;
        USHORT cRow;
        USHORT cCol;
        } NOPTRRECT;
typedef NOPTRRECT FAR16PTR PNOPTRRECT;

APIRET16  APIENTRY16    MouRemovePtr (PNOPTRRECT pmourtRect, HMOU hmou);

APIRET16  APIENTRY16    MouDrawPtr (HMOU hmou);

#define MOU_NODRAW      0x0001
#define MOU_DRAW        0x0000
#define MOU_MICKEYS     0x0002
#define MOU_PELS        0x0000

APIRET16  APIENTRY16    MouSetDevStatus (PUSHORT16 pfsDevStatus, HMOU hmou);
APIRET16  APIENTRY16    MouInitReal (PSZ16);

APIRET16  APIENTRY16    MouSynch(USHORT pszDvrName);

typedef struct _THRESHOLD {     /* threshold */
        USHORT Length;          /* Length Field            */
        USHORT Level1;          /* First movement level    */
        USHORT Lev1Mult;        /* First level multiplier  */
        USHORT Level2;          /* Second movement level   */
        USHORT lev2Mult;        /* Second level multiplier */
} THRESHOLD, *PTHRESHOLD;

APIRET16  APIENTRY16    MouGetThreshold(PTHRESHOLD pthreshold, HMOU hmou);
APIRET16  APIENTRY16    MouSetThreshold(PTHRESHOLD pthreshold, HMOU hmou);

#endif /* INCL_MOU */


/***    Input Method Profiler Services
 *
 *      IMPSetAIMProfile
 *      IMPQueryAIMProfile
 *      IMPResetAIMProfile
 */

typedef struct _AIMParms {  /* aimp */
          ULONG  Length;
          ULONG  Reserved;
          ULONG  AIM_Errors;
          USHORT AIM_Active;
          USHORT AIM_TimeOut;
          ULONG  AIM_FKAccept;
          ULONG  AIM_FKRate;
          ULONG  AIM_FKDelay;
          } AIMParms;

typedef AIMParms *PAIMParms;

#pragma pack()

#define  IMPSetAIMProfile     IMP32SetAIMProfile
#define  IMPQueryAIMProfile   IMP32QueryAIMProfile
#define  IMPResetAIMProfile   IMP32ResetAIMProfile

/***    IMPSetAIMProfile - Alter AIM profile values
 *
 */

USHORT  APIENTRY IMP32SetAIMProfile( PAIMParms pAIMParms );


/***    IMPQueryAIMProfile - Query AIM profile values
 *
 */

USHORT  APIENTRY IMP32QueryAIMProfile( PAIMParms pAIMParms );

/***    IMPResetAIMProfile - Reset AIM profile values
 *
 */

VOID  APIENTRY IMP32ResetAIMProfile(void);

#pragma checkout( suspend )
   #ifndef __CHKHDR__
      #pragma checkout( resume )
   #endif
#pragma checkout( resume )
