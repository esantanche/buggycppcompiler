#include "dummy.h"
//----------------------------------------------------------------------------
// FILE TRC2.H
//----------------------------------------------------------------------------
// Versione per utilizzo con programmi non PM.
//
// I dati vengono scritti sul file definito dalla variabile di environment
// TRCFILE
// Se la variabile non e' definita il trace e' disabilitato.
//
// Sono definite le seguenti macro :
//
// TRACEREGISTER(hwnd,name)
//    Per iniziare il TRACE: hwnd viene ignorato
//    name e' il nome del programma
//    (hwnd: per compatibilita con trc1.h)
// TRACEREGISTER2(hwnd,name,tracefile)
//    Come sopra, ma gli passo il file di trace
// TRACEDLL(Nome)
//    Per ogni DLL mettere NEL PRIMO MODULO DEL PROJECT DOPO IL DEF
//    l' istruzione TRACEDLL("Dll_Name")
//    Il nome della DLL (es: Dll_Name) non deve contenere path od altro
//    Se il trace non e' aperto cerca di aprirlo temporaneamente (verra'
//    poi riaperto da TRACEREGISTER) sul file TRCFILE (variabile di environment)
// TRACEPARAMS(argc,argv)
//    Stampa i parametri di chiamata del programma
// TRACESTRING(data)
//    Scrive il messaggio "data"
// TRACETYPE(data,type)
//    Macro ad hoc per ENELIMG
// TRACESTRING2(data,s1)
// TRACESTRING3(data,s1,s2)
// TRACESTRING4(data,s1,s2,s3)
// TRACESTRING5(data,s1,s2,s3,s4)
//    Scrive i messaggi "data" e "s1","s2" ...
// TRACEINT(data,numb)
//    Scrive il messaggio "data" e l' intero numb
// TRACELONG(data,numb)
//    Scrive il messaggio "data" ed il Long integer numb
// TRACEVRECT(var)
//    Scrive il messaggio "var=" ed il RECTL var
// TRACEPOINTER(data,pointer)
// TRACEPOINTER2(data,pointer1,pointer2)
// TRACEPOINTER2(data,pointer1,pointer2,pointer3)
//    Scrive il messaggio "data" ed i pointers
// TRACEPRTY(data)
//    Come TRACESTRING ed inoltre scrive la priorita' della thread
//    e la memoria disponibile.
// TRACERESET
//    Riazzera l' orologio del trace (indica i millisecondi)
// TRACEHEX(Msg,data,datalen)
// TRACEHEXL(Msg,data,datalen,Livello) // Con specifica livello
//    Mostra il messaggio e fa' un dump esadecimale di datalen caratteri
//    di "data"
// TRACERESTORE
//    Reimposta l' orologio del trace a partire dall' inizio.
// TRACETERMINATE
//    Termina il trace. CHIAMARLO PRIMA DI FINIRE IL PROGRAMMA.
// TRACEMSG  (Commento Handle Msg,mp1,mp2)
// TRACEKMSG  (Commento Handle Msg,mp1,mp2) // Solo messaggi noti
// TRACEKMSGL (Commento Handle Msg,mp1,mp2,Livello) // Idem e specifico livello  di trace
//    Fa' il trace di un messaggio PM
// Specifiche di trc2.h:
// TRACEERR(Cond,Msg)
//    Mostra il messaggio ed il risultato di WinGetLastError
//    se Cond e' vera
// TRACESTYLE(Msg,Style,classe)
//    Mostra lo stile di una finestra PM
// Oltre alle TRACEINT,TRACESTRING,TRACELONG,TRACEPOINTER vi sono le
// corripondenti TRACEVINT,TRACECSTRING ... che impostano da sole il messaggio
// con il nome della variabile
//
// Le macro vanno utilizzate come funzioni: mettere un ';' finale e
// specificare gli apici per le stringhe.
//
//----------------------------------------------------------------------------
// All' inizio di ogni modulo si ridefinisca TRCRTN:
// es:
//
// #define TRCRTN "RoutinDiLettura"
//
// Tale nome compare nei messaggi di trace
//----------------------------------------------------------------------------
#ifndef TRC2_H
#define TRC2_H

/* Include Header Files */

#define VR(x) #x " = ",x

//----------------------------------------------------------------------------
// Area globale
//----------------------------------------------------------------------------


#ifndef TRCEXTERN
   extern unsigned short _export trchse;
#endif

//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
// Definizione dei valori da confrontare dentro i moduli con TRACELEVEL
// 6  Trace completo (anche stringhe ed elenchi varii)
// 5  Trace dei moduli VIEW/PM (tutti)
// 4  Trace Normale + VIEW/PM principale (Messaggi su coda eventi, PM_MAPPA e OGG_PM )
// 3  Trace Normale
// 2  Trace Ridotto, per seguire il flusso principale (per sviluppo)
// 1  Trace Minimale (Essenzialmente solo applicativo)
//----------------------------------------------------------------------------
#if defined(TRACELEVEL) && TRACELEVEL >=  LIVELLO_DI_TRACE_DEL_PROGRAMMA
#define OKTRACE
#endif

//----------------------------------------------------------------------------
// Inizializzatore delle DLL
//----------------------------------------------------------------------------
struct initzDLL { _export initzDLL(char *  NomeModulo); };
// Questa macro deve essere sempre definita ed invocata per le DLL
// Modifica Montagna: registro su variabile publica il nome della DLL
// Questo e' utilizzato da fopen_TRC eccetera
#define TRACEDLL(Nome) static initzDLL DLL_Alarmer(Nome); \
        char * NomeDllCorrente = Nome;
void _export TRACEEXE(char * NomeExe); // Idem per gli exe (li cerca sul PATH)

//----------------------------------------------------------------------------
// Questa macro serve per includere in ogni modulo un' area statica di informazione
//----------------------------------------------------------------------------
// trchse: serve per il trace
// Info: nome del file e data di compilazione
// Espande (Per un file Test.c compilato il 1/8/1991) a:
// unsigned short trchse;
// static char Info[] = "Info:" "Test.c" "1/8/1991"
//----------------------------------------------------------------------------

#define TRACEVARIABLES  static const char Info[] ="Info: " __FILE__ " " __DATE__ " ";

void  _export DecodWStyle(char * Buf,LONG* Style, USHORT Classe);
const char *  _export Decod_VK(USHORT Vk);

#if ! defined(TRCRTN)
#define TRCRTN "???"
#endif


#define TRV1 static const char TRCR[]=TRCRTN
#define TRV2(_trc_a) static const char TRCR2[]= _trc_a " = "

// Gli errori debbono sempre essere tradotti anche a trace disabilitato

#define ERRINT(data,numb)  {if(trchse){TRV1;PrtInt(TRCR,(CPSZ)data,(int)numb);}}
#define ERRSTRING(data) {if(trchse){TRV1;PrtText(TRCR,(CPSZ)data);}}
#define TRACEERR(Er,data)    {if(trchse && Er){TRV1;PrtErr(TRCR,(CPSZ)data);}}

extern "C" {
   void _export PrtInt  (const char * R,const char * Messaggio,int Val);
   void _export PrtErr  (CPSZ R,CPSZ Messaggio);
   void _export PrtText (CPSZ R,CPSZ Messaggio);
};



// Definizione routines che poi verranno chiamate
extern "C" {
   void _export PrtInit (CPSZ Name);
   void _export PrtOpen (CPSZ Name,CPSZ  FileName);
   void _export PrtStop (void);
   void _export PrtLong (CPSZ R,CPSZ Messaggio,unsigned long rc);
   void _export PrtRectl(CPSZ R,CPSZ Messaggio,RECTL rec);
   void _export PrtText2(CPSZ R,CPSZ Messaggio, CPSZ s1);
   void _export PrtText3(CPSZ R,CPSZ Messaggio, CPSZ s1,CPSZ s2);
   void _export PrtText4(CPSZ R,CPSZ Messaggio, CPSZ s1,CPSZ s2,CPSZ s3);
   void _export PrtText5(CPSZ R,CPSZ Messaggio, CPSZ s1,CPSZ s2,CPSZ s3,CPSZ s4);
   void _export PrtPrty (CPSZ R,CPSZ Messaggio);
   void _export PrtHex  (CPSZ R,CPSZ Messaggio,CPSZ  data,unsigned short len);
   void _export PrtProf ();
   void cdecl _export PrtParms(CPSZ R,int argc,PSZ*  argv);
   void _export PrtPointer(CPSZ r,CPSZ Messaggio,void * P);
   void _export PrtPointer2(CPSZ r,CPSZ Messaggio,void * P,void * P2);
   void _export PrtPointer3(CPSZ r,CPSZ Messaggio,void * P,void * P2,void * P3);
   void _export PrtMsg(CPSZ R,CPSZ Messaggio,void * Win,unsigned short message,long mp1, long mp2,BOOL k);
   char * _export DecMsg(unsigned short m); // Funzione decodifica messaggi
   void _export PrtWStyle(CPSZ R,CPSZ  Messaggio,long Style,const char * Classe);
   void _export PrtClear(); // Vuota il FILE di trace
   void _export PrtCircular(ULONG MaxSize); // Rende il file di trace un buffer circolare di circa MaxSiz bytes
}

//----------------------------------------------------------------------------
// Impostazione corretta abilitazione
//----------------------------------------------------------------------------
#ifndef LIVELLO_DI_TRACE_DEL_PROGRAMMA
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 1
#endif

//----------------------------------------------------------------------------
// Per eseguire il trace del CALL STACK
//----------------------------------------------------------------------------
// Si puo' chiamare la funzione o la macro TRACECALLSTACK / TRACECALLSTACKL
// Chiamando la funzione si puo' anche indicare la profondita'
// con cui risalire nello stack
void _export TraceCall(       // Chiedo di fare un trace del CALL_STACK
   const char* Rtn,           // Routine chiamante
   const char* Msg,           // Messaggio da mostrare
   int Depth = 4  ,           // Profondita' di chiamate da mostrare
   USHORT TrcLev= LIVELLO_DI_TRACE_DEL_PROGRAMMA );
//----------------------------------------------------------------------------
/* Macros */

#if defined(OKTRACE)

#define _Abil (trchse >= LIVELLO_DI_TRACE_DEL_PROGRAMMA ) // A livello fisso
#define _Abil2(_TLev) (trchse >= _TLev )                  // A livello variabile per istruzione

#define TRACECLEAR                    PrtClear()
#define TRACECIRCULAR(Size)           PrtCircular(Size)
#define TRACEREGISTER(hwnd,name)      PrtInit(name)
#define TRACEREGISTER2(hwnd,name,fil) PrtOpen(name,fil)

#define TRACEPROFILE(data)   {if(_Abil){TRV1;    PrtProf (TRCR,data);}}
#define TRACEINT(data,numb)  {if(_Abil){TRV1;    PrtInt  (TRCR,(CPSZ)data,(int)numb);}}
#define TRACEINTL(data,numb,Tl) {if(_Abil2(Tl)){TRV1;    PrtInt  (TRCR,(CPSZ)data,(int)numb);}}
#define TRACELONG(data,numb) {if(_Abil){TRV1;    PrtLong (TRCR,(CPSZ)data,(unsigned long)numb);}}
#define TRACETYPE(data,type) {if(_Abil){TRV1;    PrtType (TRCR,(CPSZ)data,type);}}
#define TRACESTRING(data)    {if(_Abil){TRV1;    PrtText (TRCR,(CPSZ)data);}}
#define TRACESTRINGL(data,Tl)  {if(_Abil2(Tl)){TRV1;    PrtText (TRCR,(CPSZ)data);}}
#define TRACESTRING2(data,s1)          {if(_Abil){TRV1; PrtText2(TRCR,(CPSZ)data,(CPSZ)s1);}}
#define TRACESTRING3(data,s1,s2)       {if(_Abil){TRV1; PrtText3(TRCR,(CPSZ)data,(CPSZ)s1,(CPSZ)s2);}}
#define TRACESTRING4(data,s1,s2,s3)    {if(_Abil){TRV1; PrtText4(TRCR,(CPSZ)data,(CPSZ)s1,(CPSZ)s2,(CPSZ)s3);}}
#define TRACESTRING5(data,s1,s2,s3,s4) {if(_Abil){TRV1; PrtText5(TRCR,(CPSZ)data,(CPSZ)s1,(CPSZ)s2,(CPSZ)s3,(CPSZ)s4);}}
#define TRACEPOINTER(data,p)        {if(_Abil){TRV1; PrtPointer(TRCR,(CPSZ)data,(void *)p);}}
#define TRACEPOINTER2(data,p,p2)    {if(_Abil){TRV1; PrtPointer2(TRCR,(CPSZ)data,(void *)p,(void *)p2);}}
#define TRACEPOINTER3(data,p,p2,p3) {if(_Abil){TRV1; PrtPointer3(TRCR,(CPSZ)data,(void *)p,(void *)p2,(void *)p3);}}
#define TRACEPRTY(data)      {if(_Abil){TRV1;   PrtPrty (TRCR,(CPSZ)data);}}
#define TRACEHEX(Msg,data,len) {if(_Abil){TRV1; PrtHex (TRCR,Msg,(CPSZ)data,len);}}
#define TRACEHEXL(Msg,data,len,Tl) {if(_Abil2(Tl)){TRV1; PrtHex (TRCR,Msg,(CPSZ)data,len);}}
#define TRACERESET           {if(_Abil){TRV1;   PrtText(NULL,NULL);}}  // Resetta l'orologio
#define TRACERESTORE         {if(_Abil){TRV1;   PrtText(NULL,(CPSZ )1L);}}  // Restore orologio dall' inizio
#define TRACEPARAMS(argc,argv) {if(_Abil){TRV1; PrtParms(TRCR,(int)argc,(PSZ*)argv);}}
#define TRACETERMINATE       {if(_Abil){TRV1;   PrtStop();}}
#define TRACEVINT(numb)      {if(_Abil){TRV1;TRV2(#numb);  PrtInt  (TRCR,TRCR2,(int)numb);}}
#define TRACEVINTL(numb,Tl)  {if(_Abil2(Tl)){TRV1;TRV2(#numb);  PrtInt  (TRCR,TRCR2,(int)numb);}}
#define TRACEVLONG(numb)     {if(_Abil){TRV1;TRV2(#numb);  PrtLong (TRCR,TRCR2,(unsigned long)numb);}}
#define TRACEVLONGL(numb,Tl) {if(_Abil2(Tl)){TRV1;TRV2(#numb);  PrtLong (TRCR,TRCR2,(unsigned long)numb);}}
#define TRACERECTL(Msg,rect) {if(_Abil){TRV1;   PrtRectl(TRCR,(CPSZ)Msg,rect);}}
#define TRACEVRECTL(rect)    {if(_Abil){TRV1;TRV2(#rect);  PrtRectl(TRCR,TRCR2,rect);}}
#define TRACEVSTRING2(st1)   {if(_Abil){TRV1;TRV2(#st1); PrtText2(TRCR,TRCR2,st1);}}
#define TRACEVPOINTER(p)        {if(_Abil){TRV1;TRV2(#p); PrtPointer(TRCR,TRCR2,(void *)p);}}
#define TRACEVPOINTER2(p,p2)    {if(_Abil){TRV1;TRV2(#p "," #p2); PrtPointer2(TRCR,TRCR2,(void *)p,(void *)p2);}}
#define TRACEVPOINTER3(p,p2,p3) {if(_Abil){TRV1;TRV2(#p "," #p2 "," #p3); PrtPointer3(TRCR,TRCR2,(void *)p,(void *)p2,(void *)p3);}}
#define IFTRC(x)             if(_Abil){x;};
#define TRACEVBUF(x)     {if(_Abil){TRV1;char trccopy[61];strncpy(trccopy,x,min(sizeof(x),60));trccopy[min(sizeof(x),60)]=0;PrtText2(TRCR,(CPSZ)#x " = ",trccopy);}}
#define TRACEMSG(Cmnt,Win,m,m1,m2)    {if(_Abil){TRV1;PrtMsg(TRCR,(CPSZ)Cmnt,(void *)Win,(unsigned short)m,(long)m1,(long)m2,TRUE);}}
#define TRACEKMSG(Cmnt,Win,m,m1,m2)   {if(_Abil){TRV1;PrtMsg(TRCR,(CPSZ)Cmnt,(void *)Win,(unsigned short)m,(long)m1,(long)m2,FALSE);}}
#define TRACEKMSGL(Cmnt,Win,m,m1,m2,Tl) {if(_Abil2(Tl)){TRV1;PrtMsg(TRCR,(CPSZ)Cmnt,(void *)Win,(unsigned short)m,(long)m1,(long)m2,FALSE);}}
#define TRACESTYLE(Cmnt,Style,Cls)    {if(_Abil){TRV1;PrtWStyle(TRCR,(CPSZ)Cmnt,(long)Style,(const char*)Cls);}}

#else

#define TRACECLEAR                       {} // Trace istruction ignored
#define TRACECIRCULAR(Size)              {} // Trace istruction ignored 
#define TRACEREGISTER(hwnd,name)         {} // Trace istruction ignored
#define TRACEREGISTER2(hwnd,name,fil)    {} // Trace istruction ignored

#define TRACEPROFILE(data)               {} // Trace istruction ignored
#define TRACEINT(data,numb)              {} // Trace istruction ignored
#define TRACEINTL(data,numb,Tl)          {} // Trace istruction ignored
#define TRACELONG(data,numb)             {} // Trace istruction ignored
#define TRACETYPE(data,type)             {} // Trace istruction ignored
#define TRACESTRING(data)                {} // Trace istruction ignored
#define TRACESTRINGL(data,Tl)            {} // Trace istruction ignored
#define TRACESTRING2(data,s1)            {} // Trace istruction ignored
#define TRACESTRING3(data,s1,s2)         {} // Trace istruction ignored
#define TRACESTRING4(data,s1,s2,s3)      {} // Trace istruction ignored
#define TRACESTRING5(data,s1,s2,s3,s4)   {} // Trace istruction ignored
#define TRACEPOINTER(data,p)             {} // Trace istruction ignored
#define TRACEPOINTER2(data,p,p2)         {} // Trace istruction ignored
#define TRACEPOINTER3(data,p,p2,p3)      {} // Trace istruction ignored
#define TRACEPRTY(data)                  {} // Trace istruction ignored
#define TRACEHEX(Msg,data,len)           {} // Trace istruction ignored
#define TRACEHEXL(Msg,data,len,Tl)       {} // Trace istruction ignored
#define TRACERESET                       {} // Trace istruction ignored
#define TRACERESTORE                     {} // Trace istruction ignored
#define TRACEPARAMS(argc,argv)           {} // Trace istruction ignored
#define TRACETERMINATE                   {} // Trace istruction ignored
#define TRACEVINT(numb)                  {} // Trace istruction ignored
#define TRACEVINTL(numb,Tl)              {} // Trace istruction ignored
#define TRACEVLONG(numb)                 {} // Trace istruction ignored
#define TRACEVLONGL(numb,Tl)             {} // Trace istruction ignored
#define TRACERECTL(Msg,rect)             {} // Trace istruction ignored
#define TRACEVRECTL(rect)                {} // Trace istruction ignored
#define TRACEVSTRING2(st1)               {} // Trace istruction ignored
#define TRACEVPOINTER(p)                 {} // Trace istruction ignored
#define TRACEVPOINTER2(p,p2)             {} // Trace istruction ignored
#define TRACEVPOINTER3(p,p2,p3)          {} // Trace istruction ignored
#define IFTRC(x)                         {} // Trace istruction ignored
#define TRACEVBUF(x)                     {} // Trace istruction ignored
#define TRACEMSG(Cmnt,Win,m,m1,m2)       {} // Trace istruction ignored
#define TRACEKMSG(Cmnt,Win,m,m1,m2)      {} // Trace istruction ignored
#define TRACEKMSGL(Cmnt,Win,m,m1,m2,Tl)  {} // Trace istruction ignored
#define TRACESTYLE(Cmnt,Style,Clas)      {} // Trace istruction ignored
#define _Abil FALSE                         // Trace istruction ignored
#define _Abil2 FALSE                        // Trace istruction ignored
#endif

#define TRACECALLSTACK(data)             {} // Trace istruction ignored
#define TRACECALLSTACKL(data,Tl)         {} // Trace istruction ignored

#endif
