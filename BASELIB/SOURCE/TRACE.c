//----------------------------------------------------------------------------
// FILE TRACE.C
//----------------------------------------------------------------------------

#define THISOGG (*(OGGETTO*)this)


//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

// EMS001
typedef unsigned long BOOL;

#include "oggetto.h"
#include "elenco.h"
#include "elenco_s.h"

//----------------------------------------------------------------------------
void OGGETTO::Trace(USHORT TrcLev){
//----------------------------------------------------------------------------
#undef TRCRTN
#define TRCRTN "OGGETTO::Trace"
   if(trchse <  TrcLev)return;

   if(this == NULL)return;
   if(!this->EsisteAncora())return;
   STRINGA Valx = this->Valore();
   if(Valx == NUSTR ){
      STRINGA Msg;
       Msg  = " -->"     + NomeClasse() ;
       Msg += " "        + Nome         ;
       Msg += "("        + STRINGA((void*)this) ;
       Msg += ") Type "  + STRINGA(TipoOggetto) ;
       TRACESTRING(Msg);
   } else {
      STRINGA Msg;
       Msg  = " -->"     + NomeClasse()         ;
       Msg += " "        + Nome                 ;
       Msg += "("        + STRINGA((void*)this) ;
       Msg += ") Type "  + STRINGA(TipoOggetto) ;
       Msg += " Valore " + Valx                 ;
       TRACESTRING(Msg);
   }
}

//----------------------------------------------------------------------------
// Trace ELENCO
//----------------------------------------------------------------------------
void ELENCO::Trace(USHORT TrcLev)const{
#undef TRCRTN
#define TRCRTN "ELENCO::Trace"
   if(trchse <  TrcLev)return;
   TRACESTRING( "Elenco PVOID " + STRINGA((void*)this) +
                " Elementi  "   + STRINGA(Dim())  );
   ORD_FORALL(*this,i){
      TRACEPOINTER(" --> ",THIS[i]);
   };
};

//----------------------------------------------------------------------------
// Trace ELENCO_s
//----------------------------------------------------------------------------
void ELENCO_S::Trace(USHORT TrcLev)const{
#undef TRCRTN
#define TRCRTN "ELENCO_S::Trace"
   if(trchse <  TrcLev)return;

   STRINGA Msg;
   Msg  =  "Elenco Stringhe " + STRINGA((void*)this)  ;
   Msg +=  " Numero Stringhe " + STRINGA(NumStringhe) ;
   TRACESTRING(Msg);
   ORD_FORALL(*this,i){
      TRACESTRING2("Contiene Stringa ", THIS[i] );
   };
};
//----------------------------------------------------------------------------
// Trace ELENCO::Oggetti
//----------------------------------------------------------------------------
void ELENCO_Oggetti::Trace(USHORT TrcLev)const {
#undef TRCRTN
#define TRCRTN "ELENCO_Oggetti::Trace"

   if(trchse < TrcLev)return;

// if(this == NULL) return;
//
// // Questo lo abbiamo commentato perche' il BORLAND dava i numeri. Chissa perche' ?
// STRINGA Msg;
// Msg  = "Elenco Oggetti " + STRINGA((void*)this);
// Msg += " Numero Oggetti " + STRINGA(Dim());
// Msg += " Array "          + STRINGA((void*)ArrayOggetti) ;
// Msg += " ArrayAssoc "     + STRINGA((void*)ArrayAssociati);
// Msg += " "+STRINGA((int)HoAssociati) ;
// TRACESTRING(Msg);
// ORD_FORALL(*this,i){
//     THIS[i].Trace();
//     if(HoAssociati==Relaz_ID)TRACELONG("Atom Associato:",ArrayNums[i]);
// }
};
