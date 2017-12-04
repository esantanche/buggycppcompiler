//----------------------------------------------------------------------------
// ML_RANK.CPP: Genera i rank delle stazioni di instradamento in accordo con l' orario attuale
//----------------------------------------------------------------------------
// Formato del file di output (testo):
// Id della stazione    (5 caratteri allineati a destra)
// Codice Instradamento (3 caratteri allineati a destra)
// Rank                 (3 caratteri allineati a destra)   1 : Stazione piu' importante
// Nome Stazione        (35 caratteri allineati a sinistra)
//----------------------------------------------------------------------------
// Il programma non deve essere utilizzato normalemente: serve solo
// a fornire un primo input al CVB, che poi dara' i dati
// via tabella DBF (oppure vedremo)
//----------------------------------------------------------------------------
//
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  4 // Mettere ad 1 per attivare il debug

#define SIZEINSTR   1000

#include "eventi.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_OUT.HPP"
#include "elenco.h"
#include "oggetto.h"
#include "seq_proc.hpp"

#include "FT_PATHS.HPP"  // Path da utilizzare

#define PGM      "ML_RANK"

struct ISTR {
   WORD Id;
   WORD CodIstr;
   WORD NumeroFermate;
};

// Ordinamento inverso per NumeroFermate ed ID
int Cmp_Istr(const void * a, const void * b){
   ISTR &A = *(ISTR*)a;
   ISTR &B = *(ISTR*)b;
   
   if(B.NumeroFermate != A.NumeroFermate) return (int)B.NumeroFermate - (int)A.NumeroFermate;
   return (int)A.Id - (int)B.Id;
};

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

void main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
   { // Per garantire la chiamata dei distruttori dei files
      TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
      TRACEPARAMS(argc,argv);
      TRACEEXE(argv[0]);
      SetStatoProcesso(PGM, SP_PARTITO);
      if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
      
      DosError(2); // Disabilita Popup di errore
      
      GESTIONE_ECCEZIONI_ON
      TryTime(0);

      ISTR Istradamenti[SIZEINSTR];
      ZeroFill(Istradamenti);
      for (int z= 0;z < SIZEINSTR ;z++ ) {
         Istradamenti[z].Id = 59999;
      } /* endfor */
      
      // ----------------------------------------------------------------------
      // Init
      // ----------------------------------------------------------------------
      // Apro gli archivi da utilizzare
      F_STAZIONE_MV      Staz(PATH_OUT "MY_STAZI.TMP");
      STAZIONI           Stazioni(PATH_DATI,"ID_STAZI.DB");
      FILE_RW            Out(PATH_INTERMED "RANK.TXT");
      Out.SetSize(0);

      ORD_FORALL(Staz,i){
            TRACEVLONG(i);
         if(Stazioni[i].CodiceInstradamentoCvb > 0){
            WORD Istr = Stazioni[i].CodiceInstradamentoCvb;
            TRACEVLONG(Istr);
            Istradamenti[Istr].Id            = i    ;
            Istradamenti[Istr].CodIstr       = Istr ;
            Istradamenti[Istr].NumeroFermate = Staz[i].NumFermate ;
         }
      };

      qsort(Istradamenti,SIZEINSTR,sizeof(ISTR),Cmp_Istr);

      for (i = 0;Istradamenti[i].Id != 59999 ; i++ ) {
            Out.printf("%5i%3i%3i%-35s\r\n",
            Istradamenti[i].Id, 
            Istradamenti[i].CodIstr,
            i+1,
            Stazioni[Istradamenti[i].Id].NomeStazione
            );
      } /* endfor */
      
      // ---------------------------------------------------------
      // Fine
      // ---------------------------------------------------------
      TryTime(0);
      GESTIONE_ECCEZIONI_OFF
      TRACETERMINATE;
      
   }
   exit(0);
//<<< void main(int argc,char *argv[]){
}

