//========================================================================
// Dati privati interfaccia con il motore
//========================================================================
// Consistono negli oggetti che interfacciano il motore ed i precaricati
//========================================================================
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3
#define MODULO_OS2_DIPENDENTE

// EMS001
typedef unsigned long BOOL;

#define IN_MOTORE_CPP
// 1G

#include "BASE.hpp"
#include "elenco.h"
#include "file_rw.hpp"
#include "mm_basic.hpp"
#include "motglue.hpp"
#include "i_datimo.hpp"
#include "i_motore.hpp"
#include "oggetto.h"
#include "alfa0.hpp"


//----------------------------------------------------------------------------
// INSTRPRE
//----------------------------------------------------------------------------
void GET_INSTRPRE::Cerca(ID Da,ID A,INSTRPRE & Out){
   #undef TRCRTN
   #define TRCRTN "GET_INSTRPRE::Cerca"
   ARRAY_ID Key;
   Key +=A;
   Key +=Da;
   if(FuoriRange((BUFR&)Key)) { // Non sta sicuramente su file
      Ok =  FALSE;
   } else {
      FILE_BS::Posiziona((BUFR&)Key);
      if(KeyEsatta){
         Out = *(INSTRPRE*)RecordC;
         Ok =  TRUE;
      } else {
         Ok = FALSE;
      }
   } /* endif */
};
void GET_INSTRPRE::Cerca(ID A,INSTRPRE & Out){
   ARRAY_ID Key;
   Key +=A;
   if(FuoriRange((BUFR&)Key)) { // Non sta sicuramente su file
      Ok =  FALSE;
   } else {
      FILE_BS::Posiziona((BUFR&)Key);
      if(KeyEsatta){
         Out = *(INSTRPRE*)RecordC;
         Ok =  TRUE;
      } else {
         Ok = FALSE;
      }
   } /* endif */
};
void GET_INSTRPRE::CercaNext(INSTRPRE & Out){
   #undef TRCRTN
   #define TRCRTN "GET_INSTRPRE::CercaNext"
   Next();
   if(RecordC == NULL){
      Ok = FALSE;
   } else {
      INSTRPRE & Corrente = *(INSTRPRE*)RecordC;
      if(Corrente.IdStazionePartenza == Out.IdStazionePartenza  &&
         Corrente.IdStazioneDestinazione == Out.IdStazioneDestinazione) {
         Out = Corrente;
         Ok =  TRUE;
      } else {
         Ok =  FALSE;
      } /* endif */
   } /* endif */
};
void GET_INSTRPRE::CercaNextDestinazione(INSTRPRE & Out){
   #undef TRCRTN
   #define TRCRTN "GET_INSTRPRE::CercaNextDestinazione"
   Next();
   if(RecordC == NULL){
      Ok = FALSE;
   } else {
      INSTRPRE & Corrente = *(INSTRPRE*)RecordC;
      if(Corrente.IdStazioneDestinazione == Out.IdStazioneDestinazione) {
         Out = Corrente;
         Ok =  TRUE;
      } else {
         Ok =  FALSE;
      } /* endif */
   } /* endif */
};
void INSTRPRE::Inverti(){        // Inverte Origine e destinazione
   #undef TRCRTN
   #define TRCRTN "INSTRPRE::Inverti()"
   INSTRPRE Tmp = THIS;
   IdStazionePartenza     = Tmp.IdStazioneDestinazione;
   IdStazioneDestinazione = Tmp.IdStazionePartenza;
   for (BYTE j = 0; j < MAX_INSTRPRE && Stazioni[j] ; j++ );
   // j = Numero effettivo delle stazioni
   for (BYTE i = 0; i < j ; i++ ) {
      Stazioni[i] = Tmp.Stazioni[j-i-1];
   };
}
int  INSTRPRE::NumeroStazioniInstradamento(){// Ritorna il numero di "Stazioni" > 0
   for (int j = 0; j < MAX_INSTRPRE && Stazioni[j] ; j++ );
   return j;
};
//----------------------------------------------------------------------------
// MARECUM
//----------------------------------------------------------------------------
void TRAVERSATA_MARE_CON_DESCR::Set(TRAVERSATA_MARE & From){
   #undef TRCRTN
   #define TRCRTN "TRAVERSATA_MARE_CON_DESCR::Set"
   *(TRAVERSATA_MARE*)this = From;
   STAZ_FS & Imbarco = (*GRAFO::Grafo)[IdStazioneImbarco];
   STAZ_FS & Sbarco = (*GRAFO::Grafo)[IdStazioneSbarco];
   strcpy(Descrizione,Imbarco.Nome7());
   strcat(Descrizione,"->");
   strcat(Descrizione,Sbarco.Nome7());
};
// Il metodo seguente torna FALSE se non trova la relazione
// Altrimenti aggiorna Out
void GET_MARECUM::Cerca(ID A,TRAVERSATA_MARE & Out){
   #undef TRCRTN
   #define TRCRTN "GET_MARECUM::Cerca"
   ARRAY_ID Key;
   Key +=A;
   if(FuoriRange((BUFR&)Key)) { // Non sta sicuramente su file
      Ok =  FALSE;
   } else {
      FILE_BS::Posiziona((BUFR&)Key);
      if(KeyEsatta){
         Out = *(TRAVERSATA_MARE*)RecordC;
         Ok =  TRUE;
      } else {
         Ok = FALSE;
      }
   } /* endif */
}
void GET_MARECUM::CercaNext(TRAVERSATA_MARE & Out){
   #undef TRCRTN
   #define TRCRTN "GET_MARECUM::CercaNextA"
   Next();
   if(RecordC == NULL){
      Ok = FALSE;
   } else {
      TRAVERSATA_MARE & Corrente = *(TRAVERSATA_MARE*)RecordC;
      if(Corrente.IdStazioneSbarco == Out.IdStazioneSbarco) {
         Out = Corrente;
         Ok =  TRUE;
      } else {
         Ok = FALSE;
      } /* endif */
   } /* endif */
};
//----------------------------------------------------------------------------
// RELCUMUL
//----------------------------------------------------------------------------
// Il metodo seguente torna FALSE se non trova la relazione
// Altrimenti aggiorna Out
void GET_RELCUMUL::Cerca(ID A,RELCUMUL & Out){
   #undef TRCRTN
   #define TRCRTN "GET_RELCUMUL::Cerca"
   ARRAY_ID Key;
   Key +=A;
   if(FuoriRange((BUFR&)Key)) { // Non sta sicuramente su file
      Ok =  FALSE;
   } else {
      FILE_BS::Posiziona((BUFR&)Key);
      if(KeyEsatta){
         Out = *(RELCUMUL*)RecordC;
         Ok =  TRUE;
      } else {
         Ok = FALSE;
      } /* endif */
   } /* endif */
}
void GET_RELCUMUL::CercaNext(RELCUMUL & Out){
   #undef TRCRTN
   #define TRCRTN "GET_RELCUMUL::CercaNextA"
   Next();
   if(RecordC == NULL){
      Ok = FALSE;
   } else {
      RELCUMUL & Corrente = *(RELCUMUL*)RecordC;
      if(Corrente.IdStazioneDestinazione == Out.IdStazioneDestinazione) {
         Out = Corrente;
         Ok =  TRUE;
      } else {
         Ok = FALSE;
      } /* endif */
   } /* endif */
};
