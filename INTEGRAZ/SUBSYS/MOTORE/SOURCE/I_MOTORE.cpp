//=======================================================================
// Interfaccia con il motore
//========================================================================
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3
#define LtrcPerc      2  // Livello di TRACE per i percorsi trovati (sul precaricato)
#define LtrcIDS       2  // Livello di TRACE per TRACEID

#define MODULO_OS2_DIPENDENTE
#define INCL_DOSFILEMGR
#define INCL_DOSSEMAPHORES   /* Semaphore values */
// #define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS

//----------------------------------------------------------------------------
// Debug opzionale
//----------------------------------------------------------------------------
//#define DBGSOL              // Per scrivere dettagli sul trace a livello di trace LtrcPerc
//#define CONTROLLA_SEQUENZE  // Per far controllare l' ordine dei files utilizzati
//#define DBGPRECA            // Debug sui precaricati
//#define DBGCACHE            // Dettaglio costruzione cache (Store e ReStore)
//#define DBGCACHE2           // Dati generali uso cache
//----------------------------------------------------------------------------
// La cache viene cancellata ogni volta che si chiude la biglietteria

//----------------------------------------------------------------------------
// Controllo funzionamento
//----------------------------------------------------------------------------
#define  PARTEINFORMATORE      // Abilita gli algoritmi di risoluzione orario
//#define  PARTEPRECARICATO      // Abilita l' utilizzo di tabelle precaricate
//#define  ABILITA_MULTI_THREAD  // Abilita l' utilizzo parallelo di piu' threads
#define  ABILITA_CACHE         // Abilita la CACHE delle soluzioni
//----------------------------------------------------------------------------

#define IN_MOTORE_CPP
// 1G

#include "BASE.hpp"
#include "alfa0.hpp"
#include "mm_basic.hpp"
#include "i_datimo.hpp"
#include "i_motore.hpp"
#include "scandir.h"
#include "MM_CRIT2.HPP"
#include "mm_grafo.hpp"
#include <stdarg.h>
#include <stddef.h>

HMTX    hmtx          ; // Semaforo richieste motore (Funziona per tutte le richieste)

//----------------------------------------------------------------------------
// Questa e' la dimensione massima che puo' essere salvata su CACHE:
// risposte che eccedono questa dimensione non sono salvate
static int SlotSize = ( getenv("CACHE_SLOT_SIZE") == NULL) ? 10000  : atoi(getenv("CACHE_SLOT_SIZE")) ;


//----------------------------------------------------------------------------
// Area statica
//----------------------------------------------------------------------------
MM_RETE_FS * MM_RETE_FS::Rete;        // Rete FS (una sola per volta)
// Questa variabile mi dice se si debba lavorare con i precaricati o con le polimetriche informatizzate
#ifdef PARTEPRECARICATO
BOOL _export UsaIPrecaricati = TRUE;
#else
BOOL _export UsaIPrecaricati = FALSE;
#endif


#ifndef NO_SYNC_AP
extern void StartMot(); // Per sincronizzarsi con autoplay
#endif

//----------------------------------------------------------------------------
// Funzioni di ordinamento soluzioni
//----------------------------------------------------------------------------
// Ordinamento delle soluzioni nell' ambito di un percorso
int MM_sort_functionSol( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "MM_sort_functionSol()"
   MM_SOLUZIONE * A = *(MM_SOLUZIONE **) a;
   MM_SOLUZIONE * B = *(MM_SOLUZIONE **) b;
   // Criterio: e' il motore che ha impostato un livello di
   // desiderabilita' della soluzione nella variabile Ordine.
   return int(A->Ordine) - int(B->Ordine);
};
// Ordinamento dei percorsi nell' ambito di un elenco percorsi
// Gestisce sia percorsi da precaricato che da motore
int MM_sort_functionPerc( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "MM_sort_functionPerc()"
   MM_PERCORSO_E_SOLUZIONI * A = *(MM_PERCORSO_E_SOLUZIONI **) a;
   MM_PERCORSO_E_SOLUZIONI * B = *(MM_PERCORSO_E_SOLUZIONI **) b;
   //------------------------------------------------------------------------
   // Criteri di ordinamento dei percorsi trovati dal motore:
   //------------------------------------------------------------------------
   // 05) - I percorsi dei precaricati vengono prima di quelli del motore
   // 10) - Un percorso non cumulativo precede un percorso cumulativo
   // 20) - Un percorso senza traversata marittima precede un percorso con
   //       traversata marittima
   // 30) - Un percorso con traversata marittima FS precede un percorso con
   //       traversata marittima Tirrenia
   // Tutto il resto e' ordinato per ora di partenza della prima soluzione
   // se ottenuto dal motore, altrimenti:
   // 40) - Due percorsi non cumulativi sono ordinati per preferenza
   //       commerciale
   // 50) - I percorsi con traversata Tirrenia sono ordinati per Lunghezza
   //       totale di percorrenza
   // 60) - Un percorso cumulativo "doppio" segue un cumulativo "semplice"
   // 80) - Due percorsi con differenti Km1 Concessi sono ordinati per Km1
   //       Concessi
   // 90) - A parita' di Km1 Concessi due percorsi cumulativi sono ordinati per
   //       ID del primo, secondo e terzo punto di transito
   // A0) - A parita' di punti di transito sono ordinati per preferenza
   //       commerciale della tratta FS
   // B0) - Eventuali casi anomali rimanenti sono ordinati in base alla
   //       lunghezza totale di percorrenza
   //------------------------------------------------------------------------
   //
   // 05) I percorsi dei precaricati vengono prima dei percorsi del motore
   if( A->Percorso->DaPrecaricato != B->Percorso->DaPrecaricato ) return int(A->Percorso->DaPrecaricato) - int(B->Percorso->DaPrecaricato);
   DATI_TARIFFAZIONE  & A_Dt = A->Percorso->DatiTariffazione;
   DATI_TARIFFAZIONE  & B_Dt = B->Percorso->DatiTariffazione;
   BOOL    A_Cumulativo = A_Dt.KmConcessi1 || A_Dt.Andata.KmMare;
   BOOL    B_Cumulativo = B_Dt.KmConcessi1 || B_Dt.Andata.KmMare;
   BOOL    A_TraMar     = A_Dt.KmMare      || A_Dt.Andata.KmMare;
   BOOL    B_TraMar     = B_Dt.KmMare      || B_Dt.Andata.KmMare;
   BOOL    A_Tirrenia   = A_Dt.Andata.KmMare > 0;
   BOOL    B_Tirrenia   = B_Dt.Andata.KmMare > 0;
   BOOL    A_Doppio     = A_Dt.KmConcessi2 > 0;
   BOOL    B_Doppio     = B_Dt.KmConcessi2 > 0;
   if( A_Cumulativo && ! B_Cumulativo) return  1;     // 10) B Preferito ad A
   if(!A_Cumulativo &&   B_Cumulativo) return -1;     // 10) A Preferito a B
   if( A_TraMar     && ! B_TraMar    ) return  1;     // 20) B Preferito ad A
   if(!A_TraMar     &&   B_TraMar    ) return -1;     // 20) A Preferito a B
   if( A_Tirrenia   && ! B_Tirrenia  ) return  1;     // 30) B Preferito ad A
   if(!A_Tirrenia   &&   B_Tirrenia  ) return -1;     // 30) A Preferito a B
   
   if( !A->Percorso->DaPrecaricato ){
      // Accedo ai percorsi instradamenti
      PERCORSO_INSTRADAMENTI & PiA = *( PERCORSO_INSTRADAMENTI *)A->Percorso->Reserved;
      PERCORSO_INSTRADAMENTI & PiB = *( PERCORSO_INSTRADAMENTI *)B->Percorso->Reserved;
      
      // Li ordino per ora di partenza se possibile
      if(&PiA && &PiB) return (int)PiA.MinPartenza - (int)PiB.MinPartenza;
      
   } else {
      
      // A e B hanno la stessa tipologia di percorsi
      if(A_Cumulativo){         // Percorsi Cumulativi
         
         // 50) - I percorsi con traversata Tirrenia sono ordinati per Lunghezza totale di percorrenza
         if( A_Tirrenia && A_Dt.Lunghezza() != B_Dt.Lunghezza()) // 50)
            return int(A_Dt.Lunghezza()) - int(B_Dt.Lunghezza());
         
         if( A_Doppio && !B_Doppio)        return  1;     // 60) B Preferito ad A
         if(!A_Doppio &&  B_Doppio)        return -1;     // 60) A Preferito a  B
         
         if(A_Dt.KmConcessi1 != B_Dt.KmConcessi1)          // 80)
            return int(A_Dt.KmConcessi1) - int(B_Dt.KmConcessi1);
         
         if(A_Dt.TransitoFS1 != B_Dt.TransitoFS1)          // 90)
            return int(A_Dt.TransitoFS1) - int(B_Dt.TransitoFS1);
         if(A_Dt.TransitoFS2 != B_Dt.TransitoFS2)          // 90)
            return int(A_Dt.TransitoFS2) - int(B_Dt.TransitoFS2);
         if(A_Dt.TransitoFS3 != B_Dt.TransitoFS3)          // 90)
            return int(A_Dt.TransitoFS3) - int(B_Dt.TransitoFS3);
         
         if(A->Percorso->PrefComm != B->Percorso->PrefComm)                // A0)
            return int(A->Percorso->PrefComm) - int(B->Percorso->PrefComm);
         
//<<< if A_Cumulativo           // Percorsi Cumulativi
      } else {                     // Percorsi servizio interno
         if(A->Percorso->PrefComm != B->Percorso->PrefComm)                // 40)
            return int(A->Percorso->PrefComm) - int(B->Percorso->PrefComm);
      };
//<<< if  !A->Percorso->DaPrecaricato   
   };
   // Altrimenti utilizzo la lunghezza complessiva come elemento di preferenza
   return int(A_Dt.Lunghezza()) - int(B_Dt.Lunghezza());    // B0)
//<<< int MM_sort_functionPerc  const void *a, const void *b  
};

//----------------------------------------------------------------------------
// Funzioni utilizzate internamente per impostare i percorsi
//----------------------------------------------------------------------------
void MM_I_PERCORSO::Inverti(){
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::Inverti"
   if(this == NULL) return;
   FormaStandard.Mirror();
   IstradamentoCVB.Mirror();
   ELENCO_S Tmp = FormaStandardStampabile.Tokens("*");
   FormaStandardStampabile="";
   FORALL(Tmp,i){
      FormaStandardStampabile+=Tmp[i];
      if(i != 0)FormaStandardStampabile+="*";
   }
   DWORD Tmp2 = DatiTariffazione.CodiceCCRDestinazione      ;
   DatiTariffazione.CodiceCCRDestinazione = DatiTariffazione.CodiceCCROrigine;
   DatiTariffazione.CodiceCCROrigine      = Tmp2            ;
   if (DatiTariffazione.Andata.KmMare) { // Ho anche una traversata Tirrenia
      Tmp2                      = DatiTariffazione.Andata.IdStazioneSbarco   ;
      DatiTariffazione.Andata.IdStazioneSbarco   = DatiTariffazione.Andata.IdStazioneImbarco  ;
      DatiTariffazione.Andata.IdStazioneImbarco  = Tmp2                      ;
      char Tmp3[7];
      memcpy(Tmp3,DatiTariffazione.Andata.Descrizione,7);
      memcpy(DatiTariffazione.Andata.Descrizione,DatiTariffazione.Andata.Descrizione + 9,7);
      memcpy(DatiTariffazione.Andata.Descrizione + 9,Tmp3,7);
   } /* endif */
//<<< void MM_I_PERCORSO::Inverti   
};
void MM_I_PERCORSO::Set(PERCORSO_INSTRADAMENTI & Instr){
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::Set"
   
   if(this == NULL) return;
   
   
   DatiTariffazione = Instr.DatiTariffazione;
   PrefComm = 0;
   
   FormaStandard += Instr.Origine;
   ORD_FORALL(Instr.StazioniDiInstradamento,i){
      STAZ_FS & Staz = (*GRAFO::Grafo)[Instr.StazioniDiInstradamento[i]];
      if (Staz.StazioneFS){             // Se e' una stazione FS
         FormaStandard += Staz.Id;
         IstradamentoCVB += Staz.CCR;
         FormaStandardStampabile += Staz.Nome7();
         FormaStandardStampabile += '*';
      } else if (Staz.Concesse()[0].SocietaConcessa == 98){ // Oppure un punto di imbarco Tirrenia
         FormaStandard += Staz.Id;
         IstradamentoCVB += Staz.Concesse()[0].CCRCumulativo;
         FormaStandardStampabile += Staz.Nome7();
         FormaStandardStampabile += '*';
      } /* endif */
   }
   FormaStandard += Instr.Destinazione;
   
   // SE VUOTA SCRIVO DIRETTA
   if (!FormaStandardStampabile.Dim()) { FormaStandardStampabile="DIRETTA*"; }
   // PERDO L'* FINALE
   FormaStandardStampabile = FormaStandardStampabile(0,FormaStandardStampabile.Dim()-2);
   
   Reserved = &Instr;
//<<< void MM_I_PERCORSO::Set PERCORSO_INSTRADAMENTI & Instr  
};
//----------------------------------------------------------------------------
// MM_I_PERCORSO::Set
//----------------------------------------------------------------------------
void MM_I_PERCORSO::Set(INSTRPRE& Instradamento){
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::Set"
   if(this == NULL) return;
   FormaStandard += Instradamento.IdStazionePartenza;
   Reserved = NULL;
   DatiTariffazione.Stato   = DATI_TARIFFAZIONE::TARIFFA_VALIDA ;
   // Montagna: 25/7/1996 Fix per evitare la redistribuzione degli instradamenti regionali
   if (Instradamento.CodiceTariffaRegionale == 0 ) Instradamento.CodiceTariffaRegionale = 0xFF;
   Add(Instradamento);
};
void MM_I_PERCORSO::Set(struct RELCUMUL& TrattaCum){
   if(this == NULL) return;
   Reserved = NULL;
   FormaStandard += TrattaCum.IdStazioneTransitoFS;
   DatiTariffazione.Stato   = DATI_TARIFFAZIONE::TARIFFA_VALIDA ;
   Add(TrattaCum);
};
void MM_I_PERCORSO::Set(TRAVERSATA_MARE & TrattaTraghetto){
   if(this == NULL) return;
   Reserved = NULL;
   FormaStandard += TrattaTraghetto.IdStazioneImbarco;
   DatiTariffazione.Stato   = DATI_TARIFFAZIONE::TARIFFA_VALIDA ;
   Add(TrattaTraghetto);
};

void MM_I_PERCORSO::Add(struct RELCUMUL& TrattaCumulativo){     // Aggiunge una seconda tratta presa da instradamento cumulativo
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::Add"
   #ifdef  PARTEPRECARICATO
   if(this == NULL) return;
   
   #ifdef DBGPRECA            // Debug sui precaricati
   TrattaCumulativo.Trace("Tratta cumulativa:");
   #endif
   if (FormaStandard.Last() != TrattaCumulativo.IdStazioneTransitoFS) {
      ERRINT("Errore : Tratte non consecutive !, Id nodo di transito=",TrattaCumulativo.IdStazioneTransitoFS);
      ERRINT("         Stazione finale del percorso attuale:",FormaStandard.Last());
      BEEP;
   } /* endif */
   if(DatiTariffazione.CodConcessione1){
      ERRSTRING("Errore: Inserita piu' di una tratta cumulativa in un percorso");
      BEEP;
      return; // Non ricopro
   };
   STAZ_FS & Fs1 = (*GRAFO::Grafo)[TrattaCumulativo.IdStazioneTransitoFS]; // Prima stazione di transito FS
   if (!Fs1.DiInstradamento) {
      ERRSTRING("Warning:  Stazione di transito che non e' stazione di instradamento");
   } /* endif */
   // Aggiungo primo nodo di transito all' instradamento
   FormaStandardStampabile += '*';
   FormaStandardStampabile += Fs1.Nome7();
   // Cumulativo Semplice
   CodCumCVB       = TrattaCumulativo.CodiceCVB      ;   // Codice CVB della relazione cumulativa
   DatiTariffazione.KmConcessi1     = TrattaCumulativo.KmConcessi1    ;
   DatiTariffazione.CodConcessione1 = TrattaCumulativo.CodConcessione1;   // Codice societa' concessa
   DatiTariffazione.CodLinea1       = TrattaCumulativo.CodLinea1      ;   // Codice linea societa' concessa
   DatiTariffazione.TransitoFS1     = TrattaCumulativo.IdStazioneTransitoFS;
   if(TrattaCumulativo.TipoRelazione){   // Relazione composita
      if(TrattaCumulativo.IdStazioneTransitoFS2){
         //STAZIONI::R_STRU & Fs2 = Stazioni[TrattaCumulativo.IdStazioneTransitoFS2];
         TRACEID(TrattaCumulativo.IdStazioneTransitoFS2);
         DatiTariffazione.TransitoFS2     = TrattaCumulativo.IdStazioneTransitoFS2;
         FormaStandard += DatiTariffazione.TransitoFS2;
      };
      if(TrattaCumulativo.IdStazioneTransitoFS3){
         //STAZIONI::R_STRU & Fs3 = Stazioni[TrattaCumulativo.IdStazioneTransitoFS3];
         TRACEID(TrattaCumulativo.IdStazioneTransitoFS3);
         DatiTariffazione.TransitoFS3     = TrattaCumulativo.IdStazioneTransitoFS3;
         FormaStandard += DatiTariffazione.TransitoFS3;
      };
      if (TrattaCumulativo.KmFs){
         DatiTariffazione.KmReali   += TrattaCumulativo.KmFs   ;
      };
      if (TrattaCumulativo.KmConcessi2) {
         DatiTariffazione.KmConcessi2     = TrattaCumulativo.KmConcessi2    ;
         DatiTariffazione.CodConcessione2 = TrattaCumulativo.CodConcessione2;
         DatiTariffazione.CodLinea2       = TrattaCumulativo.CodLinea2      ;
      } /* endif */
//<<< if TrattaCumulativo.TipoRelazione     // Relazione composita
   };
   FormaStandard += TrattaCumulativo.IdStazioneDestinazione;
   if (TrattaCumulativo.CodiceTariffaRegionale == 0xFF){ // Debbo calcolarlo con il motore
      // Al momento non gestito, ne' credo lo gestiro' visto che mnacano dei dati
      DatiTariffazione.CodiceTariffaRegionale = 0xFF; // Non valido
   } else if (DatiTariffazione.CodiceTariffaRegionale == 0xFE){ // = Deve essere ancora impostato
      DatiTariffazione.CodiceTariffaRegionale = TrattaCumulativo.CodiceTariffaRegionale;
   } else if (DatiTariffazione.CodiceTariffaRegionale == TrattaCumulativo.CodiceTariffaRegionale){
      // E' sempre la stessa regione : OK
   } else {
      DatiTariffazione.CodiceTariffaRegionale   = 0; // Non usare tariffe regionali
   } /*endif */
   #else
   ERRSTRING("Spiacente: la gestione dei precaricati e' disabilitata");
   TrattaCumulativo.Trace("Tratta cumulativa:");
   BEEP;
   #endif
//<<< void MM_I_PERCORSO::Add struct RELCUMUL& TrattaCumulativo       // Aggiunge una seconda tratta presa da instradamento cumulativo
};
void MM_I_PERCORSO::Add(struct TRAVERSATA_MARE & TrattaTraghetto){     // Aggiunge una seconda tratta presa da Tirrenia
   #ifdef  PARTEPRECARICATO
   if(this == NULL) return;
   #ifdef DBGPRECA            // Debug sui precaricati
   TrattaTraghetto.Trace("Tratta traghetto:");
   #endif
   if (FormaStandard.Last() != TrattaTraghetto.IdStazioneImbarco) {
      ERRINT("Errore : Tratte non consecutive !, Id nodo di imbarco=",TrattaTraghetto.IdStazioneImbarco);
      ERRINT("         Stazione finale del percorso attuale:",FormaStandard.Last());
      BEEP;
   } /* endif */
   if(DatiTariffazione.Andata.KmMare){
      ERRSTRING("Errore: Inserita piu' di una tratta mare tirrenia in un percorso");
      BEEP;
      return; // Non ricopro
   };
   FormaStandard += TrattaTraghetto.IdStazioneSbarco;
   DatiTariffazione.Andata.Set(TrattaTraghetto);
   #else
   ERRSTRING("Spiacente: la gestione dei precaricati e' disabilitata");
   TrattaTraghetto.Trace("Tratta traghetto:");
   BEEP;
   #endif
//<<< void MM_I_PERCORSO::Add struct TRAVERSATA_MARE & TrattaTraghetto       // Aggiunge una seconda tratta presa da Tirrenia
};

void MM_I_PERCORSO::Add(struct INSTRPRE& TrattaInterno){     // Aggiunge una seconda tratta presa da instradamento cumulativo
   #ifdef  PARTEPRECARICATO
   if(this == NULL) return;
   #ifdef DBGPRECA            // Debug sui precaricati
   TrattaInterno.Trace("Tratta interna:");
   #endif
   if (FormaStandard.Last() != TrattaInterno.IdStazionePartenza) {
      ERRINT("Errore : Tratte non consecutive !, Id Stazione inizio tratta FS=",TrattaInterno.IdStazionePartenza);
      ERRINT("         Stazione finale del percorso attuale:",FormaStandard.Last());
      BEEP;
   } /* endif */
   if(PrefComm== 0)PrefComm = TrattaInterno.Ordine;
   int j = TrattaInterno.NumeroStazioniInstradamento();
   if(j > 0 && IstradamentoCVB.Dim()) {
      ERRSTRING("Errore : Tratte FS non consecutive hanno entrambe stazioni di instradamento");
      ERRINT("         Stazione finale del percorso attuale:",FormaStandard.Last());
      BEEP;
   } /* endif */
   for (BYTE i = 0; i < j ; i++ ) {
      STAZ_FS & Staz = (*GRAFO::Grafo)[TrattaInterno.Stazioni[i]];
      if (Staz.StazioneFS){             // Se e' una stazione FS
         if(Staz.Id == TrattaInterno.IdStazioneDestinazione) continue; // Ho alcune relazioni anomale
         FormaStandard += Staz.Id;
         IstradamentoCVB += Staz.CCR;
         if(FormaStandardStampabile.Dim())FormaStandardStampabile += '*';
         FormaStandardStampabile += Staz.Nome7();
      } /* endif */
   } /* endfor */
   FormaStandard += TrattaInterno.IdStazioneDestinazione          ;
   DatiTariffazione.KmReali                 += TrattaInterno.KmReali               ;
   DatiTariffazione.KmMare                  += TrattaInterno.KmMare                ;
   // Montagna: Da Giugno 1997 NON debbo pi— considerare i Km aggiuntivi
   DatiTariffazione.KmAggiuntivi         += TrattaInterno.KmAggiuntivi          ;
   // DatiTariffazione.KmReali                 += TrattaInterno.KmAggiuntivi          ;
   if (TrattaInterno.CodiceTariffaRegionale == 0xFF){ // Debbo calcolarlo con il motore
      ARRAY_ID Nodi;
      Nodi += TrattaInterno.IdStazionePartenza;
      for (i = 0; i < j ; i++ ) {
         if(TrattaInterno.Stazioni[i] == TrattaInterno.IdStazioneDestinazione) continue; // Ho alcune relazioni anomale
         Nodi += TrattaInterno.Stazioni[i];
      };
      Nodi += TrattaInterno.IdStazioneDestinazione;
      PERCORSO_GRAFO Percorso;
      if(Percorso.Set(Nodi)){
         TrattaInterno.CodiceTariffaRegionale = GRAFO::Grafo->CodiceTariffaRegionale(Percorso,FALSE);
      } else {
         TrattaInterno.CodiceTariffaRegionale = 0xFF;
      }
   }
   if (TrattaInterno.CodiceTariffaRegionale == 0xFF){ // Non sono riuscito a determinarlo
      DatiTariffazione.CodiceTariffaRegionale = 0xFF; // Non valido
   } else if (DatiTariffazione.CodiceTariffaRegionale == 0xFE){ // = Deve essere ancora impostato
      DatiTariffazione.CodiceTariffaRegionale = TrattaInterno.CodiceTariffaRegionale;
   } else if (DatiTariffazione.CodiceTariffaRegionale == TrattaInterno.CodiceTariffaRegionale){ // E' sempre la stessa regione
   } else {
      DatiTariffazione.CodiceTariffaRegionale   = 0; // Non usare tariffe regionali
   } /* endif */
   
   #else
   ERRSTRING("Spiacente: la gestione dei precaricati e' disabilitata");
   TrattaInterno.Trace("Tratta interna:");
   BEEP;
   #endif
//<<< void MM_I_PERCORSO::Add struct INSTRPRE& TrattaInterno       // Aggiunge una seconda tratta presa da instradamento cumulativo
};
void MM_I_PERCORSO::ImpostaCCRDestinazione(ID IdOrigine, ID IdDestinazione){
   // NB: Versione per precaricati
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::ImpostaCCRDestinazione"
   
   STAZ_FS & Origine = (*GRAFO::Grafo)[IdOrigine];
   STAZ_FS & Destinazione = (*GRAFO::Grafo)[IdDestinazione];
   
   // Identifico i codici CCR da utilizzare per origine e destinazione
   // le tratte mare non influiscono sulla logica
   DatiTariffazione.CodiceCCROrigine  = Origine.CCR;
   
   if (DatiTariffazione.KmConcessi1 == 0) {
      // Percorso Interno
      DatiTariffazione.CodiceCCRDestinazione = Destinazione.CCR;
      
   } else if (DatiTariffazione.KmConcessi2 == 0 && DatiTariffazione.TransitoFS2 != 0) {
      // Cumulativo doppio con seconda tratta FS
      DatiTariffazione.CodiceCCRDestinazione = Destinazione.CCR;
      
   } else {
      BYTE CodSOC;
      if (DatiTariffazione.KmConcessi2 > 0){
         // Percorso cumulativo doppio con seconda tratta concessa
         CodSOC = DatiTariffazione.CodConcessione2;
      } else {
         // Percorso cumulativo semplice
         CodSOC = DatiTariffazione.CodConcessione1;
      }
      STAZ_FS::D_CONCESSE * Conc = Destinazione.Concesse();
      DatiTariffazione.CodiceCCRDestinazione = 0;
      SCAN_NUM(Conc,Destinazione.NumeroConcesse,DatiConcessi,STAZ_FS::D_CONCESSE){
         if(DatiConcessi.SocietaConcessa == CodSOC){
            DatiTariffazione.CodiceCCRDestinazione = DatiConcessi.CCRCumulativo;
            break;
         }
      } ENDSCAN ;
      if(DatiTariffazione.CodiceCCRDestinazione ==0){
         DatiTariffazione.CodiceCCRDestinazione = Destinazione.CCR;
         ERRSTRING("Non identificata la societa' cumulativa: metto primo codice CCR valido");
         ERRINT("Societa' cumulativa non trovata: ",CodSOC);
         ERRINT("Codice CCR forzato: ",DatiTariffazione.CodiceCCRDestinazione);
         BEEP;
      }
//<<< if  DatiTariffazione.KmConcessi1 == 0   
   } /* endif */
//<<< void MM_I_PERCORSO::ImpostaCCRDestinazione ID IdOrigine, ID IdDestinazione  
};

//----------------------------------------------------------------------------
// MM_I_PERCORSO::PrintPercorso
//----------------------------------------------------------------------------
int  Fprintf(int Handle,const char * Format, ...){    // Scrive alla posizione corrente;
   #undef TRCRTN
   #define TRCRTN "Fprintf"
   int rc = 0;
   if(!Handle){ BEEP; return 0; };
   
   va_list argptr;
   int cnt;
   char Buffer[1024];
   va_start(argptr, Format);
   cnt = vsprintf(Buffer, Format, argptr);
   va_end(argptr);
   if(cnt > 0){
      ULONG  BytesWritten;
      
      rc = DosWrite(Handle, Buffer,cnt, &BytesWritten);
      if(rc){
         ERRSTRING("Errore scrivendo il file ");
         cnt=0;
      };
      if(cnt > BytesWritten ){
         ERRSTRING("Scritti solo parzialmente i dati:");
         ERRINT("Richiesti bytes nø:",cnt);
         TRACEVLONGL(BytesWritten,1);
      };
   };
   return(cnt);
//<<< int  Fprintf int Handle,const char * Format, ...      // Scrive alla posizione corrente;
}
void _export MM_I_PERCORSO::PrintPercorso(int Out,ARRAY_ID & Richiesta){
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::PrintPercorso"
   
   PERCORSO_POLIMETRICHE & Pp = *( PERCORSO_POLIMETRICHE *)Reserved;
   
   if (&Richiesta) {
      Fprintf(Out,"====================   Percorso Richiesto ======================\r\n");
      Fprintf(Out,"%s\r\n",(CPSZ)Richiesta.ToStringa(Stazioni));
      Fprintf(Out,"%s\r\n",(CPSZ)Richiesta.ToStringa());
   } /* endif */
   Fprintf(Out,"===============  Dettaglio tariffazione del percorso ===========\r\n");
   Fprintf(Out,"Origine: %s\r\n", Stazioni.DecodificaIdStazione(FormaStandard[0]));
   Fprintf(Out,"Destinazione: %s\r\n", Stazioni.DecodificaIdStazione(FormaStandard.Last()));
   Fprintf(Out,"Instradamenti: %s\r\n", (CPSZ)FormaStandardStampabile);
   // Fprintf(Out,"PercorsoCompleto: %s\r\n", (CPSZ)FormaStandard.ToStringa(Stazioni));
   if(&Pp){
      ARRAY_ID VincoliAssoluti2;
      ORD_FORALL(Pp.VincoliAssoluti,i)VincoliAssoluti2 += Pp.VincoliAssoluti[i].Id;
      Fprintf(Out,"Vincoli Assoluti: %s\r\n", (CPSZ)VincoliAssoluti2.ToStringa(Stazioni));
   }
   Fprintf(Out,"Km Fs =%i, Fs Add. = %i, Cod. Reg. = %i, CCR Da = %i, CCR A = %i\r\n",
      DatiTariffazione.KmReali, DatiTariffazione.KmAggiuntivi , DatiTariffazione.CodiceTariffaRegionale ,
      DatiTariffazione.CodiceCCROrigine, DatiTariffazione.CodiceCCRDestinazione);
   Fprintf(Out,"================================================================\r\n");
   
   if(! DaPrecaricato && &Pp && (Pp.Stato != PERCORSO_POLIMETRICHE::TARIFFABILE) ){
      
      STRINGA Motivo;
      switch (Pp.Stato) {
      case PERCORSO_POLIMETRICHE::NO_ISTRADAMENTO:
         Motivo = "Non e' stato possibile identificare i punti di instradamento";
         break;
      case PERCORSO_POLIMETRICHE::NON_VALORIZZATO:
         Motivo = "Errore interno dell' algoritmo di tariffazione";
         break;
      case PERCORSO_POLIMETRICHE::NO_PERCORSO_POLIMETRICHE:
         Motivo = "Non e' stato possibile collegare origine, destinazione ed instradamenti via polimetriche";
         break;
      case PERCORSO_POLIMETRICHE::SOLO_CUMULATIVO:
         Motivo = "Percorso solo cumulativo, privo di tratta FS";
         break;
      case PERCORSO_POLIMETRICHE::TRE_CUMULATIVO:
         Motivo = "Percorso per cui necessitano 3 o piu' ferrovie concesse";
         break;
      case PERCORSO_POLIMETRICHE::NON_TARIFFABILE:
         Motivo = "Il percorso contiene una tratta non tariffabile a normativa FS o non vendibile in stazione";
         break;
      case PERCORSO_POLIMETRICHE::TROPPE_POLIMETRICHE:
      default:
         Motivo = "Errore interno dell' algoritmo di tariffazione";
         break;
//<<< switch  Pp.Stato   
      } /* endswitch */
      Fprintf(Out,"Percorso non TARIFFABILE\r\n  Motivo: %s\r\n",(CPSZ)Motivo);
//<<< if ! DaPrecaricato && &Pp &&  Pp.Stato != PERCORSO_POLIMETRICHE::TARIFFABILE    
   } else {
      
      if (DaPrecaricato) {
         Fprintf(Out,"\r\n Dati da relazione precaricata.\r\n");
      } else {
         Fprintf(Out,"\r\nPolimetriche di tariffazione:\r\n");
         for (int i = 0;i < Pp.NumeroTratte ;i++ ) {
            PERCORSO_POLIMETRICHE::TRATTA & Tratta = Pp.Tratte[i];
            Fprintf(Out,"Da %s ",(CPSZ)Stazioni[Tratta.IdStazioneIngresso].NomeStazione);
            Fprintf(Out,"A %s\r\n",(CPSZ)Stazioni[Tratta.IdStazioneUscita].NomeStazione);
            Fprintf(Out,"  %i Km su Polimetrica %s ",Tratta.Km,
               (*GRAFO::Gr().FilePolim)[Tratta.IdPolimetrica].Nome);
            Fprintf(Out,"%s\r\n\r\n",(*GRAFO::Gr().FilePolim)[Tratta.IdPolimetrica].Descrizione);
         } /* endfor */
      } /* endif */
      
      if(DatiTariffazione.Stato != DATI_TARIFFAZIONE::TARIFFA_VALIDA) Fprintf(Out,"%s\n",(CPSZ)DatiTariffazione.StatoInChiaro());
      #define P(A,B) if(DatiTariffazione.B)Fprintf(Out,A,DatiTariffazione.B);
      #define S(A,B) if(DatiTariffazione.B)Fprintf(Out,A,Stazioni.DecodificaIdStazione(DatiTariffazione.B));
      if(DatiTariffazione.Lunghezza() != DatiTariffazione.KmReali)Fprintf(Out,"Lunghezza Totale  %i Km\r\n", DatiTariffazione.Lunghezza());
      // P("Km FS         %i Km\r\n", KmReali        );
      // P("Km FS Aggiuntivi %i Km\r\n", KmAggiuntivi   );
      P("Via DIRETTA (= Percorso piu' breve ) \r\n", Diretta        );
      P("Km Mare       %i Km\r\n", KmMare         );
      P("Km Concessi1  %i Km\r\n", KmConcessi1    );
      P("CodConcessione1 %i \r\n", CodConcessione1);
      P("CodLinea1     %i \r\n"  , CodLinea1      );
      S("Transito FS1  %s\r\n"   , TransitoFS1    );
      P("Km Concessi2  %i Km\r\n", KmConcessi2    );
      P("CodConcessione2 %i \r\n", CodConcessione2);
      P("CodLinea2     %i \r\n"  , CodLinea2      );
      S("Transito FS2  %s\r\n"   , TransitoFS2    );
      S("Transito FS3  %s\r\n"   , TransitoFS3    );
      P("CodiceTariffaRegionale %i \r\n", CodiceTariffaRegionale );
      if(DatiTariffazione.Andata.KmMare){
         S("Imbarco       %s\r\n"   , Andata.IdStazioneImbarco);
         S("Sbarco        %s\r\n"   , Andata.IdStazioneSbarco );
         P("Km Mare       %i \r\n"  , Andata.KmMare        );
         P("SocietaMare   %i \r\n"  , Andata.CodSocietaMare);
         P("LineaMare     %i \r\n"  , Andata.CodLineaMare  );
      } ;
      #undef P
      #undef S
//<<< if ! DaPrecaricato && &Pp &&  Pp.Stato != PERCORSO_POLIMETRICHE::TARIFFABILE    
   }
   Fprintf(Out,"\r\n");
//<<< void _export MM_I_PERCORSO::PrintPercorso int Out,ARRAY_ID & Richiesta  
}
void _export MM_I_PERCORSO::ShortPrintPercorso(int Out){
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::ShortPrintPercorso"
   
   PERCORSO_POLIMETRICHE & Pp = *( PERCORSO_POLIMETRICHE *)Reserved;
   
   if(Pp.Stato != PERCORSO_POLIMETRICHE::TARIFFABILE){
      Fprintf(Out,"Tratta non TARIFFABILE\r\n");
   } else {
      for (int i = 0;i < Pp.NumeroTratte ;i++ ) {
         PERCORSO_POLIMETRICHE::TRATTA & Tratta = Pp.Tratte[i];
         Fprintf(Out,"Da %s ",(CPSZ)Stazioni[Tratta.IdStazioneIngresso].NomeStazione);
         Fprintf(Out,"A %s\r\n",(CPSZ)Stazioni[Tratta.IdStazioneUscita].NomeStazione);
         Fprintf(Out,"  %i Km su Polimetrica %s ",Tratta.Km,
            (*GRAFO::Gr().FilePolim)[Tratta.IdPolimetrica].Nome);
         Fprintf(Out,"%s\r\n",(*GRAFO::Gr().FilePolim)[Tratta.IdPolimetrica].Descrizione);
      } /* endfor */
   }
}

//----------------------------------------------------------------------------
// CACHE_MOTORE
//----------------------------------------------------------------------------
#ifdef ABILITA_CACHE         // Abilita la CACHE delle soluzioni
int CacheSz(){ return ( getenv("CACHE_SIZE") == NULL) ? 200  : atoi(getenv("CACHE_SIZE")) ;};
#else
int CacheSz(){ return 1;};
#endif
CACHE_MOTORE::CACHE_MOTORE(const STRINGA & Dir) :
CACHE<RISPOSTA_MOTORE>(CacheSz(),sizeof(RICHIESTA_MOTORE::KEY))  ,
File(Dir +"CACHEMOT.ORE")
{
   #undef TRCRTN
   #define TRCRTN "@CACHE_MOTORE"
   
   int CacheSize = CacheSz();
   Attiva = CacheSize > 1;
   CacheDir = Dir;
   TRACEVLONGL(CacheSize,1);
   TRACEVLONGL(SlotSize,1);
   
   if (Attiva) {  // Riservo spazio e carico la free list  (margine di 32 elementi )
      File.SetSize((CacheSize+ 32) * SlotSize);
      Slots = new FREE_SLOT[CacheSize+32];
      for (int i = CacheSize + 32 - 1 ;i >= 0 ; i-- ) {  // In ordine inverso altrimenti comincia dall' ultimo slot
         Slots[i].NumSlot = i;
         FreeSlots.Push(Slots + i);
      } /* endfor */
   } else {
      Slots = NULL;
   } /* endif */
}
CACHE_MOTORE::~CACHE_MOTORE(){
   #undef TRCRTN
   #define TRCRTN "~CACHE_MOTORE"
   if (Attiva) {
      FreeSlots.Clear();
      delete [] Slots;
   } /* endif */
   File.Close();
   remove(CacheDir + "CACHEMOT.ORE");
}
// Questo metodo cerca su Cache i dati della richiesta: se li trova
// torna un MM_ELENCO_PERCORSO_E_SOLUZIONI (allocato ex novo), altrimenti
// torna NULL.
// NB: Vengono anche ristorati i percorsi del GRAFO
MM_ELENCO_PERCORSO_E_SOLUZIONI * CACHE_MOTORE::GetSol( RICHIESTA_MOTORE & Richiesta, DATI_ORARIO_FS & Orar){
   #undef TRCRTN
   #define TRCRTN "CACHE_MOTORE::GetSol"
   
   if(!Attiva)return NULL;
   RISPOSTA_MOTORE * Risp = Get( (RISPOSTA_MOTORE *) &Richiesta);
   RISPOSTA_MOTORE & Risposta = *Risp;
   if(Risp== NULL){
      #ifdef DBGCACHE2
      Richiesta.Trace("Non trovato su Cache");
      // Hash.Istogram("Situazione della cache");
      #endif
      return NULL;  // NON trovato su cache
   } else {
      #ifdef DBGCACHE2
      Richiesta.Trace("Trovato su Cache");
      #endif
   };
   
   GRAFO &Grafo = GRAFO::Gr();
   SDATA Data;
   Data.Anno     =  Richiesta.KeyAssoluta._Anno + 1980  ;
   Data.Giorno   =  Richiesta.KeyAssoluta.Giorno ;
   Data.Mese     =  Richiesta.KeyAssoluta.Mese   ;
   WORD Ora      =  Richiesta.DataEdOraRichiesta.Ore * 60 + Richiesta.DataEdOraRichiesta.Minuti;
   if(!Orar.ImpostaProblema(Richiesta.KeyAssoluta.From,Richiesta.KeyAssoluta.To,Data)){
      ERRSTRING("Errore impostando il problema");
      return NULL;
   };
   #ifdef DBGCACHE2
   TRACESTRING("Leggo la soluzione da Slot = "+STRINGA(Risposta.NumSlot)+" Size = "+STRINGA(Risposta.SlotDim));
   #endif
   BUFR Buf; Buf.Pointer = 0;
   File.Leggi(Risposta.SlotDim,Risposta.NumSlot * SlotSize, Buf);
   if (Buf.Dim() == 0) {
      ERRSTRING("Non trovati i dati");
      return NULL;
   } /* endif */
   DWORD Ctrl;
   Buf.ReStore(Ctrl);
   if (Ctrl != 0xf1f2f3f4) {
      TRACESTRING(" Cache Corrotta Inizio");
      return NULL;
   } /* endif */
   RISPOSTA_MOTORE Controllo;
   Buf.ReStore(VRB(Controllo));
   if (memcmp(&Richiesta.KeyAssoluta,&Controllo.KeyAssoluta,sizeof(Controllo.KeyAssoluta))) {
      ERRSTRING("I dati letti da CACHE non corrispondono alla richiesta ");
      BEEP;
      return NULL;
   } /* endif */
   MM_ELENCO_PERCORSO_E_SOLUZIONI * EPerc = new MM_ELENCO_PERCORSO_E_SOLUZIONI("Soluzione da CACHE",MM_RETE_FS::Rete);
   USHORT NumInstradamenti;
   Buf.ReStore(NumInstradamenti);
   for (USHORT i = 0;i < NumInstradamenti ; i++) {
      Grafo.Instradamenti += new PERCORSO_POLIMETRICHE();
      Grafo.Instradamenti[i]->ReStoreFrom(Buf);
   } /* endfor */
   EPerc->ReStoreFrom(Buf);
   Buf.ReStore(Ctrl);
   if (Ctrl != 0xf4f3f2f1) {
      TRACESTRING(" Cache Corrotta Fine");
      return NULL;
   } /* endif */
   SDATA GiornoSeguente = Data;  ++ GiornoSeguente ;
   // Elimino le eventuali soluzioni non valide
   FORALL((*EPerc),i1){
      MM_ELENCO_SOLUZIONE & Sols = * (*EPerc)[i1]->Soluzioni;
      if (&Sols != NULL) {
         FORALL(Sols,i2){
            if(Sols[i2]->OraPartenza() < Ora ){
               if(Sols[i2]->Per.Circola(GiornoSeguente)){
                  Sols[i2]->SuccessivoRichiesta = TRUE;
               } else  {
                  Sols.Elimina(i2);
               }
            }
         }
      } /* endif */
      if(Sols.Dim() == 0)EPerc->Elimina(i1);
   }
   EPerc->Sort(Ora);
   return EPerc;
//<<< MM_ELENCO_PERCORSO_E_SOLUZIONI * CACHE_MOTORE::GetSol  RICHIESTA_MOTORE & Richiesta, DATI_ORARIO_FS & Orar  
};

// Questo metodo mette su Cache la soluzione. In caso di errore torna FALSE
// Se i dati superano la dimensione dello slot torna TRUE ma non salva
BOOL CACHE_MOTORE::PutSol(RICHIESTA_MOTORE & Richiesta, MM_ELENCO_PERCORSO_E_SOLUZIONI * EPerc, DATI_ORARIO_FS & Orar){
   #undef TRCRTN
   #define TRCRTN "CACHE_MOTORE::PutSol"
   if(!Attiva)return TRUE;
   if( EPerc->Dim() == 0)return FALSE;
   if( (*EPerc)[0]->Soluzioni == NULL)return FALSE;
   if( (*EPerc)[0]->Soluzioni->Dim() ==0)return FALSE;
   if( Orar.ElaborazioneInterrotta )return FALSE;
   #ifdef DBGCACHE2
   TRACESTRING("Salvataggio su Cache: Num Percorsi = "+STRINGA(EPerc->Dim()));
   #endif
   GRAFO &Grafo = GRAFO::Gr();
   RISPOSTA_MOTORE Risposta ;
   memcpy(&Risposta, &Richiesta, sizeof(Richiesta)); // Copia anche gli Spare Bits.
   BOOL OkRisposta = TRUE;
   BUFR Buf(SlotSize),Buf2(SlotSize);
   DWORD Ctrl = 0xf1f2f3f4;
   Buf.Store(Ctrl);
   Buf.Store(VRB(Risposta));
   ELENCO Instrads;
   EPerc->StoreTo(Buf2,Instrads);     // Salvataggio percorsi : Come side effect riempie Instrads
   USHORT NumInstradamenti= Instrads.Dim();
   Buf.Store(NumInstradamenti);
   ORD_FORALL(Instrads,i){
      int Idx = Grafo.Instradamenti.CercaPerPuntatore(Instrads[i]);
      if (Idx < 0) { // Solo per check interno: non e' possibile
         OkRisposta = FALSE;
         BEEP;
         break;
      } /* endif */
      Grafo.Instradamenti[Idx]->StoreTo(Buf);
   } /* endfor */
   Buf.Store(Buf2);
   Ctrl = 0xf4f3f2f1;
   Buf.Store(Ctrl);
   if(Buf.Dim() > SlotSize){
      #ifdef DBGCACHE2
      TRACESTRING("La dimensione dei dati "+STRINGA(Buf.Dim())+" eccede la dimensione dello slot: "+STRINGA(SlotSize)+" I dati non vengono salvati");
      #endif
   } else if(OkRisposta){
      Risposta.DataEdOraMaxUtilizzabile = Risposta.DataEdOraRichiesta;
      WORD Ora  = Risposta.DataEdOraRichiesta.Ore * 60 + Risposta.DataEdOraRichiesta.Minuti;
      // Identifico la minima ora di partenza dopo l' ora della richiesta
      WORD MinTrasc = 9999;
      ORD_FORALL((*EPerc),Pr){
         Bottom(MinTrasc,TempoTrascorso(Ora,(*(*EPerc)[Pr]->Soluzioni)[0]->OraPartenza()));
      }
      Risposta.DataEdOraMaxUtilizzabile.AddMinuti(MinTrasc+240); // Rimangono validi per 4 ore DOPO LA PRIMA PARTENZA
      Risposta.SlotDim = Buf.Dim();
      
      // Metto la risposta in cache
      GetSlot(&Risposta);
      memmove(&Buf[4],VRB(Risposta)); // Aggiorno i dati della risposta
      File.Posiziona(Risposta.NumSlot * SlotSize);
      File.Scrivi(Buf);
      
      #ifdef DBGCACHE2
      Risposta.Trace("Salvataggio su file di cache ");
      // Hash.Istogram("Situazione della cache");
      // ITERA(Hash, Risp, RISPOSTA_MOTORE ){
      //    Risp.Trace("Dati In Cache: ");
      // } END_ITERA
      #endif
      
//<<< if Buf.Dim   > SlotSize  
   } else {
      TRACESTRING("Problemi scrivendo su Cache");
   }
   return OkRisposta;
//<<< BOOL CACHE_MOTORE::PutSol RICHIESTA_MOTORE & Richiesta, MM_ELENCO_PERCORSO_E_SOLUZIONI * EPerc, DATI_ORARIO_FS & Orar  
}
//----------------------------------------------------------------------------
// RICHIESTA_MOTORE
//----------------------------------------------------------------------------
void  RICHIESTA_MOTORE::Trace(const STRINGA&  Msg, int Livello ){
   #undef TRCRTN
   #define TRCRTN "RICHIESTA_MOTORE::Trace"
   if(Livello > trchse)return;
   TRACESTRING(Msg + STRINGA(" ")+
      + " From "+ STRINGA(KeyAssoluta.From)
      + " To "+ STRINGA(KeyAssoluta.To)
      + " Livello "+ STRINGA(KeyAssoluta.Livello)
      + " Giorno "+ STRINGA(KeyAssoluta.Giorno)
      + " Mese "+ STRINGA(KeyAssoluta.Mese)
      + " _Anno "+ STRINGA(KeyAssoluta._Anno)
      + " Data "+DataEdOraRichiesta.TimeStamp()
   );
};
//----------------------------------------------------------------------------
// RISPOSTA_MOTORE
//----------------------------------------------------------------------------
void  RISPOSTA_MOTORE::Trace(const STRINGA&  Msg, int Livello ){
   #undef TRCRTN
   #define TRCRTN "RISPOSTA_MOTORE::Trace"
   if(Livello > trchse)return;
   TRACESTRING(Msg + STRINGA(" ")+
      + " Slot "+ STRINGA(NumSlot)
      + " Size "+ STRINGA(SlotDim)
      + " From "+ STRINGA(KeyAssoluta.From)
      + " To "+ STRINGA(KeyAssoluta.To)
      + " Livello "+ STRINGA(KeyAssoluta.Livello)
      + " Giorno "+ STRINGA(KeyAssoluta.Giorno)
      + " Mese "+ STRINGA(KeyAssoluta.Mese)
      + " _Anno "+ STRINGA(KeyAssoluta._Anno)
      + " Data "+DataEdOraRichiesta.TimeStamp()
      + " Data MAX "+DataEdOraMaxUtilizzabile.TimeStamp()
   );
};
BOOL RISPOSTA_MOTORE::InitForCache(RISPOSTA_MOTORE * Key, void * PCache){
   #undef TRCRTN
   #define TRCRTN "RISPOSTA_MOTORE::InitForCache()"
   
   BOOL Risp = TRUE;
   CACHE_MOTORE * Cache = ( CACHE_MOTORE * ) PCache ;
   DataEdOraRichiesta         = Key->DataEdOraRichiesta        ;
   DataEdOraMaxUtilizzabile   = Key->DataEdOraMaxUtilizzabile  ;
   SlotDim                    = Key->SlotDim                   ;
   // Cerco uno slot utilizzabile da Free List
   FREE_SLOT *  Slot = Cache->FreeSlots.Pop();
   if(Slot == NULL){
      ERRSTRING("??? : Free list vuota: Probabile abend a seguire! ");
      BEEP;
      NumSlot = 0;
      Risp = FALSE;
   } else {
      NumSlot = Slot->NumSlot;
   }
   Key->NumSlot  = NumSlot  ; // Mando indietro il numero di file da utilizzare
   return Risp;
//<<< BOOL RISPOSTA_MOTORE::InitForCache RISPOSTA_MOTORE * Key, void * PCache  
}
void RISPOSTA_MOTORE::RemoveFromCache( void * PCache){
   #undef TRCRTN
   #define TRCRTN "RISPOSTA_MOTORE::RemoveFromCache()"
   CACHE_MOTORE * Cache = ( CACHE_MOTORE * ) PCache ;
   Cache->FreeSlots.Push(Cache->Slots + NumSlot);
}

//----------------------------------------------------------------------------
// RETE_FS
//----------------------------------------------------------------------------
MM_RETE_FS * MM_RETE_FS::CreaRete(
   const  STRINGA& PathE
  ,const STRINGA& PathInCorso
  ,const STRINGA& PathProssimo
  ,const STRINGA& PathCacheGrafo
  ,const STRINGA& PathFastOrarioCorrente
  ,const STRINGA& PathFastOrarioSeguente
) { return new MM_RETE_FS( PathE ,PathInCorso ,PathProssimo ,PathCacheGrafo ,PathFastOrarioCorrente ,PathFastOrarioSeguente );};

MM_RETE_FS::MM_RETE_FS(
   const STRINGA& PathE                   // Path su cui stanno i files del GRAFO
  ,const STRINGA& PathInCorso             // Path su cui sta l'orario in corso
  ,const STRINGA& PathProssimo            // Path su cui sta l'orario prossimo
  ,const STRINGA& PathCacheGrafo          // Path su cui stanno i files di cache (default = Path)
  ,const STRINGA& PathFastOrarioCorrente  // Path su cui stanno i files di fast start dell' orario corrente (default = PathInCorso)
  ,const STRINGA& PathFastOrarioSeguente  // Path su cui stanno i files di fast start dell' orario seguente (default = PathProssimo)
) : OGGETTO("MM_RETE_FS",ALTRI,NULL,TRUE),
CacheMotore((PathCacheGrafo == NUSTR) ? PathE : PathCacheGrafo )
{
   #undef TRCRTN
   #define TRCRTN "@MM_RETE_FS"
   TRACESTRING("Inizio");
   
   LONG rc;
   ULONG   flAttr=0;    /* Creation attributes */
   BOOL32  fState=0;    /* Initial state of the mutex semaphore */
   
   PROFILER::Clear(FALSE);
   PROFILER::Clear(TRUE);
   
   ClassName = "MM_RETE_FS";
   Linguaggio = Italiano;
   
   TRACEVSTRING2(Path);
   TipoRicerca=Minima;
   GRAFO::Grafo         = NULL;
   DatiOrarioA           = NULL;
   OrarioInCorso          = NULL;
   OrarioProssimo         = NULL;
   Instradamenti        = NULL;
   RelazioniCumulative  = NULL;
   InstradamentiCum     = NULL;
   RelazioniTirrenia    = NULL;
   
   // Default: scarto le soluzioni non tariffabili solo su internet
   ScartaSoluzioniNonTariffabili  = (getenv("ORARI_IN_MEMORIA") != NULL);
   
   
   if (Rete) {
      ERRSTRING("Errore: e' possibile creare un solo oggetto di classe MM_RETE_FS per volta");
      BEEP;
      delete this;
      return;
   } else {
      Rete = this;
      Path = PathE;
      OrarioCorrente = PathInCorso ;
      OrarioSeguente = PathProssimo;
      if ( PathFastOrarioCorrente ) {
         PathFast1 = PathFastOrarioCorrente;
      } else {
         PathFast1 = PathInCorso;
      } /* endif */
      if ( PathFastOrarioSeguente ) {
         PathFast2 = PathFastOrarioSeguente;
      } else {
         PathFast2 = PathProssimo;
      } /* endif */
   } /* endif */
   
   CLASSIFICA_TRENI::CaricaClassifiche(Path);
   GRAFO::Grafo   = new GRAFO(Path);
   
   #ifdef  PARTEPRECARICATO
   if(TestFileExistance(Path+"ID_ISTRA.DB")){
      Instradamenti       = new GET_INSTRPRE(Path);
      
      RelazioniCumulative = new GET_RELCUMUL(Path);
      InstradamentiCum    = new GET_INSTRPRE(Path,"ID_ISCTR.DB");
      RelazioniTirrenia   = new GET_MARECUM(Path);
   }
   
   TRACEVPOINTER(Instradamenti);
   TRACEVPOINTER(RelazioniCumulative);
   TRACEVPOINTER(InstradamentiCum);
   TRACEVPOINTER(RelazioniTirrenia);
   
   #if defined(CONTROLLA_SEQUENZE)  && defined(OKTRACE)
   BOOL Ok = TRUE;
   Ok &= Instradamenti->ControllaSequenza(); // Solo per debug
   Ok &= RelazioniCumulative->ControllaSequenza(); // Solo per debug
   Ok &= InstradamentiCum->ControllaSequenza(); // Solo per debug
   Ok &= RelazioniTirrenia->ControllaSequenza(); // Solo per debug
   if(!Ok){
      ERRSTRING("Errore: una delle verifiche ha rilevato un file non sequenzialmente ordinato");
      BEEP;
   };
   #endif
   #endif
   
   
   #ifdef PARTEINFORMATORE
   char * NoMotore= getenv("NO_MOTORE");
   if(NoMotore == NULL || stricmp(NoMotore,"YES")){
      if (VALIDITA_ORARIO::OrarioValido( PathProssimo)) {
         OrarioProssimo = new DATI_ORARIO_FS(PathProssimo,PathFast2);
      } else {
         ERRSTRING("NON TROVATO O NON VALIDO ORARIO SEGUENTE");
      } /* endif */
      if (VALIDITA_ORARIO::OrarioValido( PathInCorso)) {
         OrarioInCorso  = new DATI_ORARIO_FS(PathInCorso,PathFast1);
         // Imposto l' orario valido = orario in corso
         InizioValiditaOrario = OrarioInCorso->ValiditaOrario.InizioUfficiale;
         FineValiditaOrario   = OrarioInCorso->ValiditaOrario.FineUfficiale;
         DatiOrarioA = OrarioInCorso;
      } else {
         ERRSTRING("NON TROVATO O NON VALIDO ORARIO CORRENTE");
         // Imposto l' orario valido = orario prossimo
         if (OrarioProssimo) {
            InizioValiditaOrario = OrarioProssimo->ValiditaOrario.InizioUfficiale;
            FineValiditaOrario   = OrarioProssimo->ValiditaOrario.FineUfficiale;
            DatiOrarioA = OrarioProssimo;
         } /* endif */
      } /* endif */
      
      TRACEVPOINTER(GRAFO::Grafo);
      TRACEVPOINTER(DatiOrarioA);
      TRACEVPOINTER(OrarioInCorso);
      TRACEVPOINTER(OrarioProssimo);
//<<< if NoMotore == NULL || stricmp NoMotore,"YES"   
   } else {
      TRACESTRING("Test precaricati: non caricate le tabelle del motore");
   };
   #endif
   
   flAttr=0;    /* Creation attributes */
   fState=0;    /* Initial state of the mutex semaphore */
   rc= DosCreateMutexSem(NULL, &hmtx, flAttr, fState);
   if (rc) {
      ERRINT("Errore creando semaforo rc =",rc);
      ERRSTRING("La rete verra' comunque creata, salvo malfunzioni dovute a conflitti nell' accesso risorse");
      BEEP;
   } /* endif */
   TRACESTRING("Fine");
//<<< CacheMotore  PathCacheGrafo == NUSTR  ? PathE : PathCacheGrafo  
};

//----------------------------------------------------------------------------
// ~RETE_FS
//----------------------------------------------------------------------------
MM_RETE_FS::~MM_RETE_FS(){
   #undef TRCRTN
   #define TRCRTN "~MM_RETE_FS"
   APIRET rc;
   CLASSIFICA_TRENI::LiberaClassifiche();
   if(hmtx)DosRequestMutexSem(hmtx, -1);
   FORALL(OggettiDellaComponente,i)delete &OggettiDellaComponente[i];
   Free();
   if(Instradamenti      ) delete Instradamenti      ; Instradamenti       = NULL;
   if(RelazioniCumulative) delete RelazioniCumulative; RelazioniCumulative = NULL;
   if(OrarioInCorso        ) delete OrarioInCorso        ; OrarioInCorso         = NULL;
   if(OrarioProssimo       ) delete OrarioProssimo       ; OrarioProssimo        = NULL;
   if(GRAFO::Grafo       ) delete GRAFO::Grafo       ; GRAFO::Grafo        = NULL;
   if(hmtx)DosReleaseMutexSem(hmtx);
   if(hmtx)DosCloseMutexSem(hmtx);
   hmtx = NULL;
   Rete = NULL;
};

//----------------------------------------------------------------------------
// Lingua correntemente in uso
//----------------------------------------------------------------------------
// Questa routine e' in pratica un metodo di rete_FS, per questo e' messa qui.
// Ho usato una funzione per motivi di visibilita' all' interno del motore
//----------------------------------------------------------------------------
BYTE __fastcall LinguaInUso(){
   #undef TRCRTN
   #define TRCRTN "LinguaInUso"
   return (BYTE)MM_RETE_FS::Rete->Linguaggio;
};

//----------------------------------------------------------------------------
//  SetOrarioDelGiorno
//----------------------------------------------------------------------------
// Questo metodo mette in uso l'orario corrispondente ad un dato giorno
// per la successiva richiesta; Torna TRUE su errore
BOOL MM_RETE_FS::SetOrarioDelGiorno(SDATA & Day){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::SetOrarioDelGiorno"
   if (OrarioInCorso && Day >= OrarioInCorso->ValiditaOrario.InizioUfficiale && Day <= OrarioInCorso->ValiditaOrario.FineUfficiale ) {
      // Imposto l' orario valido = orario in corso
      DatiOrarioA = OrarioInCorso;
      DataCorrente = Day;
      DatiOrarioA->Activate(); // Attivo i Dati Orario
      InizioValiditaOrario = OrarioInCorso->ValiditaOrario.InizioUfficiale;
      FineValiditaOrario   = OrarioInCorso->ValiditaOrario.FineUfficiale;
      return FALSE;
   } else if ( OrarioProssimo && Day >= OrarioProssimo->ValiditaOrario.InizioUfficiale && Day <= OrarioProssimo->ValiditaOrario.FineUfficiale) {
      // Imposto l' orario valido = orario prossimo
      DatiOrarioA = OrarioProssimo;
      DataCorrente = Day;
      DatiOrarioA->Activate(); // Attivo i Dati Orario
      InizioValiditaOrario = OrarioProssimo->ValiditaOrario.InizioUfficiale;
      FineValiditaOrario   = OrarioProssimo->ValiditaOrario.FineUfficiale;
      return FALSE;
   } else {
      ERRSTRING("Errore : la data "+STRINGA(Day)+" non appartiene ne' all' orario corrente ne al seguente");
      ERRSTRING("Non sono state cambiate le impostazioni di orario");
      return TRUE;
   } /* endif */
//<<< BOOL MM_RETE_FS::SetOrarioDelGiorno SDATA & Day  
};

//----------------------------------------------------------------------------
// Viaggio Libero
//----------------------------------------------------------------------------
// Questo crea un PERCORSO a partire da un insieme di stazioni toccate in un viaggio libero.
// Dovra' essere poi cancellato con un delete
MM_PERCORSO * MM_RETE_FS::ViaggioLibero(ARRAY_ID & NodiDelViaggio,BOOL AbilitaCumulativo){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::ViaggioLibero()"
   
   NodiDelViaggio.Trace(GRAFO::Gr(),"Richiesto viaggio libero");
   ARRAY_ID Nodi = NodiDelViaggio;
   MM_PERCORSO * Perc = NULL;
   if(!GRAFO::Gr()[NodiDelViaggio[0]].Vendibile || !GRAFO::Gr()[NodiDelViaggio.Last()].Vendibile){
      NodiDelViaggio.Trace(GRAFO::Gr(),"Richiesta Errata: Origine o Destinazione non vendibile",1);
      //BEEP;
      return Perc;
   }
   if(NodiDelViaggio[0] == NodiDelViaggio.Last()){
      NodiDelViaggio.Trace(GRAFO::Gr(),"Richiesta Errata: Viaggi circolari non ammessi",1);
      //BEEP;
      return Perc;
   }
   DosRequestMutexSem(hmtx, -1);
   // NON DARE FREE: CREA PROBLEMI
   GRAFO::Gr().AbilitaCumulativo = AbilitaCumulativo;
   NodiDelViaggio.Trace(*GRAFO::Grafo,TRCRTN);
   PERCORSO_GRAFO & Perco = *new PERCORSO_GRAFO;
   if (Perco.SetUtilizzandoPolimetriche(NodiDelViaggio)) {
      Perco.LunghezzaTariffabile();
      if(Perco.Istr == NULL){
         ERRSTRING("Errore non creato percorso ISTRADAMENTI");
         delete &Perco;
      } else {
         GRAFO::Gr().PercorsiGrafo += &Perco;
         Perc = new MM_PERCORSO("Viaggio Libero",this);
         Perc->DaPrecaricato = FALSE;
         Perc->Set(*Perco.Istr);
         Perc->Trace("Percorso da viaggio libero");
      }
   } else {
      ERRSTRING("Errore non creato percorso");
      delete &Perco;
   } /* endif */
   DosReleaseMutexSem(hmtx);
   TRACEVPOINTER(Perc);
   return Perc;
//<<< MM_PERCORSO * MM_RETE_FS::ViaggioLibero ARRAY_ID & NodiDelViaggio,BOOL AbilitaCumulativo  
};

//----------------------------------------------------------------------------
// M300_RichiestaPerRelazione
//----------------------------------------------------------------------------
// Questo metodo crea un MM_ELENCO_PERCORSO_E_SOLUZIONI
//
// Il metodo puo' gestire le seguenti combinazioni
// 1  - Una tratta del servizio interno da origine a destinazione o viceversa
// 2  - Una tratta del servizio interno da origine ad un transito +
//       una tratta cumulativa semplice o doppia
// 3  - Una tratta del servizio interno da origine ad un transito mare
//       Tirrenia + tratto mare Tirrenia
// 4  - Partenza da stazione transito mare Tirrenia, tratto mare Tirrenia,
//      tratto interno FS
// 5  - Partenza da stazione transito mare Tirrenia, tratto mare Tirrenia,
//      tratto interno FS, tratto cumulativo semplice o doppio
// 6  - Partenza da stazione FS, tratta interna, tratto mare Tirrenia,
//      tratto interno FS
// 7  - Partenza da stazione FS, tratta interna, tratto mare Tirrenia,
//      tratto cumulativo semplice o doppio
//      es: Porto Torres-Cagliari-Civitavecchia (ammette servizio cumulativo)
// 8  - Partenza da stazione FS, tratta interna, tratto mare Tirrenia,
//      tratto interno FS, tratto cumulativo semplice o doppio
// C  - Tratte cumulative da tutti i possibili transiti fino  a destinazione
//      (Per fornirne un elenco)
//
// Nonche' tutte le combinazioni ottenute invertendo Origine e Destinazione
//
// Si noti che le tratte del cumulativo Doppie possono terminare su di una
// stazione FS
//
// Non e' stata gestita la vendita con combinazione automatica di due relazioni
// cumulative semplici (anche se sarebbe in linea di massima possibile)
// in quanto non gestito dall' attuale M300
//
//----------------------------------------------------------------------------
// Manca la combinazione Tirrenia * Cumulativo perche' non e' vendibile
// non avendo tratte FS.
//
// Il caso 7 non si verifica attualmente, ma potrebbe verificarsi in futuro.
//----------------------------------------------------------------------------
// Modifica 25/11/1996: Simmetrizzato: Calcolo sempre sia le soluzioni dirette
// che le inverse, poi scelgo.
//----------------------------------------------------------------------------
// Modifica  7/12/1996: Esclusi i percorsi del tipo 1 (che non essendo ottenuti
// tramite combinazioni sono sempre sicuramente valide) tutte le altre
// combinazioni sono soggette alla regola che non devono eccedere il 100%
// della lunghezza della combinazione / percorso piu' breve (o 100 Km).
// I valori esatti sono controllati dalla var di environment ESCLUSIONE_PRECARICATI
// NB: Andando dal continente in Sardegna o viceversa non considero nella
// logica le combinazioni "Escluso Mare" in quanto molto piu' corte delle altre.
// Inoltre il limite percentuale e' elevato (al minimo) al 100%, anche se
// la variabile di environment ha un valore inferiore.
//----------------------------------------------------------------------------
// La composizione delle singole tratte per ottenere il risultato risulta
// dal seguente schema:
//
//                      1  2  3  4  5  6  7  8    File      Da        A
//   TrattaInterno      x  x  x        x  x  x  ID_ISTRA  Origine
//   TrattaTirrenia           x  x  x  x  x  x  ID_MACUM
//   TrattaInterno2              x  x  x     x  ID_ISCTR  Imbarco
//   TrattaCumulativo      x        x     x  x  ID_ISCUM            Destinazione
//
//   File      Indice            Contiene
//   ID_ISTRA  Dest. Orig. Pref. Tutte le relazioni FS da Origine/i ammessa/e
//   ID_MACUM  Imbarco,Sbarco    Tutte le tratte mare tirrenia (Bidirezionale)
//   ID_ISCTR  Dest. Orig. Pref. Tutte le relazioni FS da stazioni di imbarco
//                               (sia continente che Sardegna)
//   ID_ISCUM  Destinazione      Tutte le relazioni del cumulativo dai transiti
//                                  FS (sia continente che Sardegna)
//
// Valgono i seguenti vincoli:
//
// - Debbo avere almeno una tratta del servizio interno
// - Non posso avere piu' di una tratta del servizio cumulativo
// - TrattaInterno  e TrattaInterno2 debbono stare una in SARDEGNA
//   e l' altra no, e sono accedute rispettivamente tramite
//   ID_ISTRA ed ID_ISCTR (potrebbero esserci altri tratti FS
//   nascosti all' interno delle relazioni del cumulativo)
// - Per gli instradamenti cumulativi il numero di instradaamenti
//   sulle tratte del servizio interno non deve superare 4 (o la
//   SOMMA se ho due tratte interne). Il vincolo deriva dai records CVB.
// - Se ho due tratte interne una non puo' avere instradamenti
//   Per il momento e' banalmente sodddisfatta perche' RELFSARD non
//   ha instradamenti, ma il problema si potrebbe ripresentare
//   se venissero posti instradamenti nelle tratte interne di future
//   stazioni FS SARDE (improbabile)
//----------------------------------------------------------------------------
MM_ELENCO_PERCORSO_E_SOLUZIONI * MM_RETE_FS::M300_RichiestaPerRelazione(ID Da,ID A,
   MM_ELENCO_PERCORSO_E_SOLUZIONI * AggiungiA){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::M300_RichiestaPerRelazione()"
   
   MM_ELENCO_PERCORSO_E_SOLUZIONI * Out=NULL;
   
   if(Instradamenti == NULL){
      ReasonCode = RIC_BAD_SETTINGS;
      return Out; // Non ho caricato alcunche'
   }
   #ifdef  PARTEPRECARICATO
   
   static DWORD ValoreSoluzioni;
   ValoreSoluzioni = 0;
   
   // ------------------------------------------------------------------------------
   // INIT
   // ------------------------------------------------------------------------------
   // Determino se e' la chiamata originale o la fase di percorso invertito
   if(AggiungiA == NULL){   // Altrimenti sto gestendo il percorso inverso
      DosRequestMutexSem(hmtx, -1);
      Out = new MM_ELENCO_PERCORSO_E_SOLUZIONI("Percorsi Predeterminati",this);
   } else {
      Out = AggiungiA;
   } /* endif */
   // Accedo a stazione Origine e Destinazione per caratterizzarle
   STAZIONI::R_STRU Origine      = Stazioni[Da]; // Lavora per copia
   STAZIONI::R_STRU Destinazione = Stazioni[A] ; // Lavora per copia
   TRACEVPOINTER3(Instradamenti,GRAFO::Grafo,DatiOrarioA);
   TRACEID_L(1,Da);
   TRACEID_L(1,A);
   TRACEVLONG(Origine.Fs());
   TRACEVLONG(Origine.Sarda());
   TRACEVLONG(Origine.TransitoMareTirrenia());
   TRACEVLONG(Destinazione.Fs());
   TRACEVLONG(Destinazione.Sarda());
   TRACEVLONG(Destinazione.DestinazioneCumulativa());
   TRACEVLONG(Destinazione.TransitoMareTirrenia());
   // Variabili di lavoro
   INSTRPRE         TrattaInterno    ;
   INSTRPRE         TrattaInterno2   ;
   RELCUMUL         TrattaCumulativo ;
   TRAVERSATA_MARE  TrattaTirrenia   ;
   
   if(Da == 0 && Destinazione.DestinazioneCumulativa()){
      // ------------------------------------------------------------------------------
      // C  - una tratta cumulativa semplice o doppia
      // ------------------------------------------------------------------------------
      TRACESTRING("Relazioni tipo C:");
      
      for(RelazioniCumulative->Cerca(A,TrattaCumulativo);
         RelazioniCumulative->Ok;
         RelazioniCumulative->CercaNext(TrattaCumulativo)){
         
         TRACEID(TrattaCumulativo.IdStazioneTransitoFS);
         
         MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
         Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
         Perco.Percorso = new MM_I_PERCORSO();
         Perco.Percorso->Set(TrattaCumulativo);
         Perco.Percorso->ImpostaCCRDestinazione(Da,A);
         Perco.Percorso->Trace("C: Tratta Cumulativo",LtrcPerc);
         (*Out) += &Perco;
      } /* endfor */
   } else {
      
      BOOL Rifiuta = TRUE; // Se TRUE Non accetto la relazione
      if(Da == A){
         ReasonCode = RIC_DA_IS_A;
      } else if(!Origine.Vendibile()){
         ReasonCode = RIC_BAD_DA;
      } else if(!Destinazione.Vendibile()){
         ReasonCode = RIC_BAD_A;
      } else {
         Rifiuta = FALSE;
      };
      if(Rifiuta){
         TRACESTRING("Relazione rifiutata");
         if(AggiungiA == NULL){   // Altrimenti sto gestendo il percorso inverso
            DosReleaseMutexSem(hmtx);
         }
         return Out;                   // Non accetto la relazione
      };
      
      // ------------------------------------------------------------------------------
      // 1  - Una tratta del servizio interno da origine a destinazione o viceversa
      // ------------------------------------------------------------------------------
      // Se la destinazione o l' origine non sono del servizio interno SKIP
      if (Origine.Fs() && Destinazione.Fs()){
         TRACESTRING("Relazioni tipo 1:");
         
         for(Instradamenti->Cerca(Da,A,TrattaInterno);
            Instradamenti->Ok;
            Instradamenti->CercaNext(TrattaInterno)){
            
            MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
            Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
            Perco.Percorso = new MM_I_PERCORSO();
            Perco.Percorso->Set(TrattaInterno);
            Perco.Percorso->ImpostaCCRDestinazione(Da,A);
            Perco.Percorso->Trace("1: Percorso servizio interno",LtrcPerc);
            (*Out) += &Perco;
            ValoreSoluzioni += 1000;  // Molto bene
         } /* endfor */
      }  /* endif */
      
      
      // ------------------------------------------------------------------------------
      // 2  - Una tratta del servizio interno da origine ad un transito +
      //       una tratta cumulativa semplice o doppia
      // ------------------------------------------------------------------------------
      if (Origine.Fs() && Destinazione.DestinazioneCumulativa()){
         TRACESTRING("Relazioni tipo 2:");
         
         for(RelazioniCumulative->Cerca(A,TrattaCumulativo);
            RelazioniCumulative->Ok;
            RelazioniCumulative->CercaNext(TrattaCumulativo)){
            
            TRACEID(TrattaCumulativo.IdStazioneTransitoFS);
            
            for(Instradamenti->Cerca(Da,TrattaCumulativo.IdStazioneTransitoFS,TrattaInterno);
               Instradamenti->Ok;
               Instradamenti->CercaNext(TrattaInterno)){
               
               if(TrattaInterno.NumeroStazioniInstradamento() > 4) {
                  ERRSTRING("2: Ignorato instradamento interno con piu' di 4 stazioni intermedie per cumulativo");
                  continue; // Prossimo instradamento
               }
               
               MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
               Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
               Perco.Percorso = new MM_I_PERCORSO();
               Perco.Percorso->Set(TrattaInterno);
               Perco.Percorso->Add(TrattaCumulativo);
               Perco.Percorso->ImpostaCCRDestinazione(Da,A);
               Perco.Percorso->Trace("2: Percorso servizio interno + Cumulativo",LtrcPerc);
               (*Out) += &Perco;
               ValoreSoluzioni += 100;  // Bene
            } /* endfor */
//<<<    for RelazioniCumulative->Cerca A,TrattaCumulativo ;
         } /* endfor */
//<<< if  Origine.Fs   && Destinazione.DestinazioneCumulativa    
      }  /* endif */
      
      // ------------------------------------------------------------------------------
      // 3  - Una tratta del servizio interno da origine ad un transito mare
      //       Tirrenia + tratto mare Tirrenia
      // ------------------------------------------------------------------------------
      if (Origine.Fs() && Destinazione.TransitoMareTirrenia() &&
            Origine.Sarda() != Destinazione.Sarda()    // Da Continente a Sardegna o viceversa
      ){
         TRACESTRING("Relazioni tipo 3:");
         
         for(RelazioniTirrenia->Cerca(A,TrattaTirrenia);
            RelazioniTirrenia->Ok;
            RelazioniTirrenia->CercaNext(TrattaTirrenia)){
            
            // Controllo che la relazione tirrenia mi porti dalla parte giusta
            STAZIONI::R_STRU Intermedia = Stazioni[TrattaTirrenia.IdStazioneImbarco];
            if(Intermedia.Sarda() != Origine.Sarda())continue;
            
            TRACEID(TrattaTirrenia.IdStazioneImbarco);
            
            for(Instradamenti->Cerca(Da,TrattaTirrenia.IdStazioneImbarco,TrattaInterno);
               Instradamenti->Ok;
               Instradamenti->CercaNext(TrattaInterno)){
               
               MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
               Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
               Perco.Percorso = new MM_I_PERCORSO();
               Perco.Percorso->Set(TrattaInterno);
               Perco.Percorso->Add(TrattaTirrenia);
               Perco.Percorso->ImpostaCCRDestinazione(Da,A);
               Perco.Percorso->Trace("3: Percorso servizio Interno + Mare Tirrenia ",LtrcPerc);
               (*Out) += &Perco;
               ValoreSoluzioni += 10; // Le tratte mare sono per definizione sospette: contano meno per determinare quale e' il percorso  "diretto" e quale l' inverso
            } /* endfor */
//<<<    for RelazioniTirrenia->Cerca A,TrattaTirrenia ;
         } /* endfor */
//<<< if  Origine.Fs   && Destinazione.TransitoMareTirrenia   &&
      }  /* endif */
      
      // ------------------------------------------------------------------------------
      // 4  - Partenza da stazione transito mare Tirrenia, tratto mare Tirrenia,
      //      tratto interno FS
      // ------------------------------------------------------------------------------
      if (Origine.Fs() && Origine.TransitoMareTirrenia() && Destinazione.Fs()&&
            Origine.Sarda() != Destinazione.Sarda()    // Da Continente a Sardegna o viceversa
      ){
         TRACESTRING("Relazioni tipo 4:");
         for(InstradamentiCum->Cerca(A,TrattaInterno2);
            InstradamentiCum->Ok;
            InstradamentiCum->CercaNextDestinazione(TrattaInterno2)){
            
            // Controllo di non aver attraversato il mare con una relazione FS
            STAZIONI::R_STRU Intermedia = Stazioni[TrattaInterno2.IdStazionePartenza];
            if(Intermedia.Sarda() != Destinazione.Sarda())continue;
            
            TRACEID(TrattaInterno2.IdStazionePartenza);
            
            for(RelazioniTirrenia->Cerca(TrattaInterno2.IdStazionePartenza,TrattaTirrenia);
               RelazioniTirrenia->Ok;
               RelazioniTirrenia->CercaNext(TrattaTirrenia)){
               
               TRACEID(TrattaTirrenia.IdStazioneImbarco);
               
               // Vedo se sono realmente arrivato a destinazione
               if(TrattaTirrenia.IdStazioneImbarco!= Da)continue;
               
               MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
               Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
               Perco.Percorso = new MM_I_PERCORSO();
               Perco.Percorso->Set(TrattaTirrenia);
               Perco.Percorso->Add(TrattaInterno2);
               Perco.Percorso->ImpostaCCRDestinazione(Da,A);
               Perco.Percorso->Trace("4: partenza da Mare Tirrenia + servizio interno",LtrcPerc);
               (*Out) += &Perco;
               ValoreSoluzioni += 1;  // Le tratte mare sono per definizione sospette: contano meno per determinare quale e' il percorso  "diretto" e quale l' inverso
            } /* endfor */
//<<<    for InstradamentiCum->Cerca A,TrattaInterno2 ;
         } /* endfor */
//<<< if  Origine.Fs   && Origine.TransitoMareTirrenia   && Destinazione.Fs  &&
      }  /* endif */
      
      // ------------------------------------------------------------------------------
      // 5  - Partenza da stazione transito mare Tirrenia, tratto mare Tirrenia,
      //      tratto interno FS, tratto cumulativo semplice o doppio
      // ------------------------------------------------------------------------------
      if (Origine.Fs() && Origine.TransitoMareTirrenia() && Destinazione.DestinazioneCumulativa()&&
            Origine.Sarda() != Destinazione.Sarda()    // Da Continente a Sardegna o viceversa
      ){
         TRACESTRING("Relazioni tipo 5:");
         for(RelazioniCumulative->Cerca(A,TrattaCumulativo);
            RelazioniCumulative->Ok;
            RelazioniCumulative->CercaNext(TrattaCumulativo)){
            
            TRACEID(TrattaCumulativo.IdStazioneTransitoFS);
            
            for(InstradamentiCum->Cerca(TrattaCumulativo.IdStazioneTransitoFS,TrattaInterno2);
               InstradamentiCum->Ok;
               InstradamentiCum->CercaNextDestinazione(TrattaInterno2)){
               
               // Controllo di non aver attraversato il mare con una relazione FS
               STAZIONI::R_STRU Intermedia = Stazioni[TrattaInterno2.IdStazionePartenza];
               if(Intermedia.Sarda() != Destinazione.Sarda())continue;
               
               TRACEID(TrattaInterno2.IdStazionePartenza);
               
               for(RelazioniTirrenia->Cerca(TrattaInterno2.IdStazionePartenza,TrattaTirrenia);
                  RelazioniTirrenia->Ok;
                  RelazioniTirrenia->CercaNext(TrattaTirrenia)){
                  
                  TRACEID(TrattaTirrenia.IdStazioneImbarco);
                  
                  // Vedo se sono realmente arrivato a destinazione
                  if(TrattaTirrenia.IdStazioneImbarco!= Da)continue;
                  
                  if(TrattaInterno2.NumeroStazioniInstradamento() > 4) {
                     ERRSTRING("5: Ignorato instradamento interno con piu' di 4 stazioni intermedie per cumulativo");
                     continue; // Prossimo instradamento
                  }
                  
                  MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
                  Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
                  Perco.Percorso = new MM_I_PERCORSO();
                  Perco.Percorso->Set(TrattaTirrenia);
                  Perco.Percorso->Add(TrattaInterno2);
                  Perco.Percorso->Add(TrattaCumulativo);
                  Perco.Percorso->ImpostaCCRDestinazione(Da,A);
                  Perco.Percorso->Trace("5: partenza da Mare Tirrenia + interno + cumulativo",LtrcPerc);
                  (*Out) += &Perco;
                  ValoreSoluzioni += 1;  // Le tratte mare sono per definizione sospette: contano meno per determinare quale e' il percorso  "diretto" e quale l' inverso
//<<<          for RelazioniTirrenia->Cerca TrattaInterno2.IdStazionePartenza,TrattaTirrenia ;
               } /* endfor */
//<<<       for InstradamentiCum->Cerca TrattaCumulativo.IdStazioneTransitoFS,TrattaInterno2 ;
            } /* endfor */
//<<<    for RelazioniCumulative->Cerca A,TrattaCumulativo ;
         } /* endfor */
//<<< if  Origine.Fs   && Origine.TransitoMareTirrenia   && Destinazione.DestinazioneCumulativa  &&
      }  /* endif */
      
      // ------------------------------------------------------------------------------
      // 6  - Partenza da stazione FS, tratta interna, tratto mare Tirrenia,
      //      tratto interno FS
      // ------------------------------------------------------------------------------
      if (Origine.Fs() && Destinazione.Fs()&&
            Origine.Sarda() != Destinazione.Sarda()    // Da Continente a Sardegna o viceversa
      ){
         TRACESTRING("Relazioni tipo 6:");
         for(InstradamentiCum->Cerca(A,TrattaInterno2);
            InstradamentiCum->Ok;
            InstradamentiCum->CercaNextDestinazione(TrattaInterno2)){
            
            // Controllo di non aver attraversato il mare con una relazione FS
            STAZIONI::R_STRU Intermedia = Stazioni[TrattaInterno2.IdStazionePartenza];
            if(Intermedia.Sarda() != Destinazione.Sarda())continue;
            
            TRACEID(TrattaInterno2.IdStazionePartenza);
            
            for(RelazioniTirrenia->Cerca(TrattaInterno2.IdStazionePartenza,TrattaTirrenia);
               RelazioniTirrenia->Ok;
               RelazioniTirrenia->CercaNext(TrattaTirrenia)){
               
               TRACEID(TrattaTirrenia.IdStazioneImbarco);
               
               for(Instradamenti->Cerca(Da,TrattaTirrenia.IdStazioneImbarco,TrattaInterno);
                  Instradamenti->Ok;
                  Instradamenti->CercaNext(TrattaInterno)){
                  
                  if(TrattaInterno.NumeroStazioniInstradamento() > 0 &&
                        TrattaInterno2.NumeroStazioniInstradamento() > 0) {
                     ERRSTRING("6: Ignorata coppia di instradamenti non contigui !");
                     continue; // Prossimo instradamento
                  }
                  
                  MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
                  Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
                  Perco.Percorso = new MM_I_PERCORSO();
                  Perco.Percorso->Set(TrattaInterno);
                  Perco.Percorso->Add(TrattaTirrenia);
                  Perco.Percorso->Add(TrattaInterno2);
                  Perco.Percorso->ImpostaCCRDestinazione(Da,A);
                  Perco.Percorso->Trace("6: Interno + Tirrenia + servizio interno",LtrcPerc);
                  (*Out) += &Perco;
                  ValoreSoluzioni += 1;  // Le tratte mare sono per definizione sospette: contano meno per determinare quale e' il percorso  "diretto" e quale l' inverso
               } /* endfor */
//<<<       for RelazioniTirrenia->Cerca TrattaInterno2.IdStazionePartenza,TrattaTirrenia ;
            } /* endfor */
//<<<    for InstradamentiCum->Cerca A,TrattaInterno2 ;
         } /* endfor */
//<<< if  Origine.Fs   && Destinazione.Fs  &&
      }  /* endif */
      
      // ------------------------------------------------------------------------------
      // 7  - Partenza da stazione FS, tratta interna, tratto mare Tirrenia,
      //      tratto cumulativo semplice o doppio
      // ------------------------------------------------------------------------------
      // Caso Dubbio: potrebbe non capitare MAI
      // ------------------------------------------------------------------------------
      if (Origine.Fs() && Destinazione.DestinazioneCumulativa() &&
            Origine.Sarda() != Destinazione.Sarda()    // Da Continente a Sardegna o viceversa
      ){
         TRACESTRING("Relazioni tipo 7:");
         
         for(RelazioniCumulative->Cerca(A,TrattaCumulativo);
            RelazioniCumulative->Ok;
            RelazioniCumulative->CercaNext(TrattaCumulativo)){
            
            TRACEID(TrattaCumulativo.IdStazioneTransitoFS);
            
            for(RelazioniTirrenia->Cerca(TrattaCumulativo.IdStazioneTransitoFS,TrattaTirrenia);
               RelazioniTirrenia->Ok;
               RelazioniTirrenia->CercaNext(TrattaTirrenia)){
               
               // Controllo che la relazione tirrenia mi porti dalla parte giusta
               STAZIONI::R_STRU Intermedia = Stazioni[TrattaTirrenia.IdStazioneImbarco];
               if(Intermedia.Sarda() != Origine.Sarda())continue;
               
               TRACEID(TrattaTirrenia.IdStazioneImbarco);
               
               for(Instradamenti->Cerca(Da,TrattaTirrenia.IdStazioneImbarco,TrattaInterno);
                  Instradamenti->Ok;
                  Instradamenti->CercaNext(TrattaInterno)){
                  
                  if(TrattaInterno.NumeroStazioniInstradamento() > 4) {
                     ERRSTRING("7: Ignorato instradamento interno con piu' di 4 stazioni intermedie per cumulativo");
                     continue; // Prossimo instradamento
                  }
                  
                  MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
                  Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
                  Perco.Percorso = new MM_I_PERCORSO();
                  Perco.Percorso->Set(TrattaInterno);
                  Perco.Percorso->Add(TrattaTirrenia);
                  Perco.Percorso->Add(TrattaCumulativo);
                  Perco.Percorso->ImpostaCCRDestinazione(Da,A);
                  Perco.Percorso->Trace("7: Percorso servizio Interno + Mare Tirrenia ",LtrcPerc);
                  (*Out) += &Perco;
                  ValoreSoluzioni += 10;  // Le tratte mare sono per definizione sospette: contano meno per determinare quale e' il percorso  "diretto" e quale l' inverso
               } /* endfor */
//<<<       for RelazioniTirrenia->Cerca TrattaCumulativo.IdStazioneTransitoFS,TrattaTirrenia ;
            } /* endfor */
//<<<    for RelazioniCumulative->Cerca A,TrattaCumulativo ;
         } /* endfor */
//<<< if  Origine.Fs   && Destinazione.DestinazioneCumulativa   &&
      }  /* endif */
      
      // ------------------------------------------------------------------------------
      // 8  - Partenza da stazione FS, tratta interna, tratto mare Tirrenia,
      //      tratto interno FS, tratto cumulativo semplice o doppio
      // ------------------------------------------------------------------------------
      if (Origine.Fs() && Destinazione.DestinazioneCumulativa() &&
            Origine.Sarda() != Destinazione.Sarda()    // Da Continente a Sardegna o viceversa
      ){
         TRACESTRING("Relazioni tipo 8:");
         
         for(RelazioniCumulative->Cerca(A,TrattaCumulativo);
            RelazioniCumulative->Ok;
            RelazioniCumulative->CercaNext(TrattaCumulativo)){
            
            TRACEID(TrattaCumulativo.IdStazioneTransitoFS);
            
            for(InstradamentiCum->Cerca(TrattaCumulativo.IdStazioneTransitoFS,TrattaInterno2);
               InstradamentiCum->Ok;
               InstradamentiCum->CercaNextDestinazione(TrattaInterno2)){
               
               // Controllo di non aver attraversato il mare con una relazione FS
               STAZIONI::R_STRU Intermedia = Stazioni[TrattaInterno2.IdStazionePartenza];
               if(Intermedia.Sarda() != Destinazione.Sarda())continue;
               
               for(RelazioniTirrenia->Cerca(TrattaInterno2.IdStazionePartenza,TrattaTirrenia);
                  RelazioniTirrenia->Ok;
                  RelazioniTirrenia->CercaNext(TrattaTirrenia)){
                  
                  TRACEID(TrattaInterno2.IdStazionePartenza);
                  
                  // Controllo che la relazione tirrenia mi porti dalla parte giusta
                  STAZIONI::R_STRU Intermedia =  Stazioni[TrattaTirrenia.IdStazioneImbarco];
                  if(Intermedia.Sarda() != Origine.Sarda())continue;
                  
                  TRACEID(TrattaTirrenia.IdStazioneImbarco);
                  
                  for(Instradamenti->Cerca(Da,TrattaTirrenia.IdStazioneImbarco,TrattaInterno);
                     Instradamenti->Ok;
                     Instradamenti->CercaNext(TrattaInterno)){
                     
                     if(TrattaInterno.NumeroStazioniInstradamento() > 4 ||
                           TrattaInterno2.NumeroStazioniInstradamento() > 4) {
                        ERRSTRING("8: Ignorato instradamento interno con piu' di 4 stazioni intermedie per cumulativo");
                        continue; // Prossimo instradamento
                     }
                     
                     if(TrattaInterno.NumeroStazioniInstradamento() > 0 &&
                           TrattaInterno2.NumeroStazioniInstradamento() > 0) {
                        ERRSTRING("8: Ignorata coppia di instradamenti non contigui !. Per cumulativo");
                        continue; // Prossimo instradamento
                     }
                     
                     MM_PERCORSO_E_SOLUZIONI & Perco = * new MM_PERCORSO_E_SOLUZIONI;
                     Perco.Soluzioni = new MM_ELENCO_SOLUZIONE();
                     Perco.Percorso = new MM_I_PERCORSO();
                     Perco.Percorso->Set(TrattaInterno);
                     Perco.Percorso->Add(TrattaTirrenia);
                     Perco.Percorso->Set(TrattaInterno2); // Il Set invece dell' Add mette la stazione di partenza negli instradamenti
                     Perco.Percorso->Add(TrattaCumulativo);
                     Perco.Percorso->ImpostaCCRDestinazione(Da,A);
                     Perco.Percorso->Trace("8: Percorso servizio Interno + Mare Tirrenia ",LtrcPerc);
                     (*Out) += &Perco;
                     ValoreSoluzioni += 1;  // Le tratte mare sono per definizione sospette: contano meno per determinare quale e' il percorso  "diretto" e quale l' inverso
//<<<             for Instradamenti->Cerca Da,TrattaTirrenia.IdStazioneImbarco,TrattaInterno ;
                  } /* endfor */
//<<<          for RelazioniTirrenia->Cerca TrattaInterno2.IdStazionePartenza,TrattaTirrenia ;
               } /* endfor */
//<<<       for InstradamentiCum->Cerca TrattaCumulativo.IdStazioneTransitoFS,TrattaInterno2 ;
            } /* endfor */
//<<<    for RelazioniCumulative->Cerca A,TrattaCumulativo ;
         } /* endfor */
//<<< if  Origine.Fs   && Destinazione.DestinazioneCumulativa   &&
      }  /* endif */
      
      
//<<< if Da == 0 && Destinazione.DestinazioneCumulativa    
   }  /* endif */
   
   // ------------------------------------------------------------------------------
   // Scarto i percorsi troppo lunghi
   //----------------------------------------------------------------------------
   // Modifica  7/12/1996: Esclusi i percorsi del tipo 1 (che non essendo ottenuti
   // tramite combinazioni sono sempre sicuramente valide) tutte le altre
   // combinazioni sono soggette alla regola che non devono eccedere il 100%
   // della lunghezza della combinazione / percorso piu' breve (o 100 Km).
   // I valori esatti sono controllati dalla var di environment ESCLUSIONE_PRECARICATI
   // NB: Andando dal continente in Sardegna o viceversa non considero nella
   // logica le combinazioni "Escluso Mare" in quanto molto piu' corte delle altre.
   // Inoltre il limite percentuale e' elevato (al minimo) al 100%, anche se
   // la variabile di environment ha un valore inferiore.
   int MinPerc   = 1000000; // Per scartare i percorsi troppo lunghi
   BOOL TraversoMare = Origine.Sarda() != Destinazione.Sarda();    // Da Continente a Sardegna o viceversa
   FORALL((*Out),l1){
      DATI_TARIFFAZIONE & Tar = (*Out)[l1]->Percorso->DatiTariffazione;
      int KmPer = Tar.Lunghezza();
      int KmMar = Tar.KmMare + Tar.Andata.KmMare ;
      if(TraversoMare && KmMar == 0)continue;  // Ignoro gli escluso mare
      Bottom(MinPerc   , KmPer );  // Determino il minimo percorso
   }
   int CoeffEsclus[2] = { 100,100 };   GetEnvArray(CoeffEsclus,2,"ESCLUSIONE_PRECARICATI");
   int KmMassimi = MinPerc   +  (MinPerc * CoeffEsclus[0])/ 100;   // Add 100 %
   Top(KmMassimi, CoeffEsclus[1])                              ;   // Not less than 100 KM
   if(TraversoMare) Top(KmMassimi, 2* MinPerc )                ;   // Limite minimo per le tratte mare
   TRACESTRING( VRS(KmMassimi) + VRS(TraversoMare) );
   FORALL((*Out),l2){
      DATI_TARIFFAZIONE & Tar = (*Out)[l2]->Percorso->DatiTariffazione;
      int KmPer = Tar.Lunghezza();
      int KmMar = Tar.KmMare + Tar.Andata.KmMare ;
      if(TraversoMare && KmMar == 0)continue;  // Ignoro gli escluso mare
      int KmCum = Tar.KmConcessi1 + Tar.KmConcessi2;
      if(KmCum == 0 && KmMar == 0) continue;   // E' di tipo 1
      if(KmPer > KmMassimi){
         (*Out)[l2]->Percorso->Trace("**** Eliminato percorso troppo lungo",LtrcPerc);
         Out->Elimina(l2);
      };
   }
   
   // ------------------------------------------------------------------------------
   // GESTIONE RITORNO
   // ------------------------------------------------------------------------------
   // Invoco lo stesso algoritmo per gestire il ritorno
   // Modifica 25/11/1996: Lo chiamo sempre e poi decido cosa utilizzare
   if (AggiungiA == NULL){   // Non trovate soluzioni al primo giro
      ERRSTRING("Analisi soluzioni del percorso inverso");
      MM_ELENCO_PERCORSO_E_SOLUZIONI * Inverso = new MM_ELENCO_PERCORSO_E_SOLUZIONI("Percorsi Predeterminati",this);
      DWORD Vs = ValoreSoluzioni ;
      M300_RichiestaPerRelazione(A,Da,Inverso);
      BOOL MeglioInverso = ValoreSoluzioni > Vs;
      if (MeglioInverso) {
         delete Out;
         Out = Inverso;
      } else {
         delete Inverso;
      } /* endif */
   } /* endif */
   
   // ------------------------------------------------------------------------------
   // TERMINATE
   // ------------------------------------------------------------------------------
   if(AggiungiA == NULL){   // Altrimenti sto gestendo il percorso inverso
      
      // Inversione dei percorsi con Da / A scambiati
      ORD_FORALL((*Out),i){
         MM_I_PERCORSO & Percorso = *((*Out)[i]->Percorso);
         if(Percorso.FormaStandard[0] == A) Percorso.Inverti();
      }
      
      TRACESTRING("Sort Dei percorsi");
      if(MultiThread) DOE1; // I Sort vanno semaforizzati per problemi Borland
      qsort(Out->ArrayPVOID,Out->Dim(),sizeof(void*),MM_sort_functionPerc);
      if(MultiThread) DOE2;
      
      TRACESTRING("Fine");
      DosReleaseMutexSem(hmtx);
   };
   
   
   #else
   ERRSTRING("Spiacente: la gestione dei precaricati e' disabilitata Rel "+STRINGA(Da)+" "+STRINGA(A)+" "+STRINGA(AggiungiA));
   BEEP;
   #endif
   
   return Out;
//<<< MM_ELENCO_PERCORSO_E_SOLUZIONI * MM_RETE_FS::M300_RichiestaPerRelazione ID Da,ID A,
};


//----------------------------------------------------------------------------
// RichiestaPerRelazione
//----------------------------------------------------------------------------
MM_ELENCO_PERCORSO_E_SOLUZIONI * MM_RETE_FS::RichiestaPerRelazione(ID Da,ID A,MOTIVO Motivo, SDATA  Data,WORD Ora, OPZIONE_UTENTE OpzioneUtente){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::RichiestaPerRelazione()"
   
   
   #ifndef NO_SYNC_AP
   StartMot(); // Per sincronizzarsi con autoplay
   #endif
   
   ReasonCode = RIC_OK; // Se va tutto bene e' OK
   
   #ifdef PARTEPRECARICATO   // Devo usare i precaricati per bigliettare
   #ifdef PARTEINFORMATORE   // Posso utilizzare il motore
   BOOL EnginIsNotValid = !(STRINGA(getenv("MOTORE_SI_VALIDO")) == "SI");
   BOOL CombinaPrecaricati = STRINGA(getenv("COMBINA_PRECARICATI")) == "SI";
   if (CombinaPrecaricati && Motivo == PER_BIGLIETTARE) Motivo = PER_ENTRAMBI_I_MOTIVI;
   switch (Motivo) {
   case PER_INFORMATIVA:
      {
         MM_ELENCO_PERCORSO_E_SOLUZIONI & El2 = * SIPAX_RichiestaPerRelazione(Da,A,Motivo,Data,Ora,OpzioneUtente);
         if(&El2){
            FORALL(El2,i){
               if (EnginIsNotValid && El2[i]->Percorso->DatiTariffazione.Stato == DATI_TARIFFAZIONE::TARIFFA_VALIDA) {
                  El2[i]->Percorso->DatiTariffazione.Stato = DATI_TARIFFAZIONE::TARIFFA_VALIDA_SOLO_PER_INFORMATIVA;
               } /* endif */
            }
         }
         return &El2;
      }
   case PER_ENTRAMBI_I_MOTIVI:
      {
         MM_ELENCO_PERCORSO_E_SOLUZIONI & El1 = * M300_RichiestaPerRelazione(Da,A);
         MM_ELENCO_PERCORSO_E_SOLUZIONI & El2 = * SIPAX_RichiestaPerRelazione(Da,A,Motivo,Data,Ora,OpzioneUtente);
         if (&El1) {
            if(&El2){
               
               FORALL(El2,i){
                  if (EnginIsNotValid && El2[i]->Percorso->DatiTariffazione.Stato == DATI_TARIFFAZIONE::TARIFFA_VALIDA) {
                     El2[i]->Percorso->DatiTariffazione.Stato = DATI_TARIFFAZIONE::TARIFFA_VALIDA_SOLO_PER_INFORMATIVA;
                  } /* endif */
                  El1 += El2[i];
                  El2[i] = NULL;
               }
            }
            delete &El2;
            return &El1;
         } else {
            if(&El2){
               FORALL(El2,i){
                  if (EnginIsNotValid && El2[i]->Percorso->DatiTariffazione.Stato == DATI_TARIFFAZIONE::TARIFFA_VALIDA) {
                     El2[i]->Percorso->DatiTariffazione.Stato = DATI_TARIFFAZIONE::TARIFFA_VALIDA_SOLO_PER_INFORMATIVA;
                  } /* endif */
               }
            }
            return &El2;
//<<<    if  &El1   
         } /* endif */
//<<< e PER_ENTRAMBI_I_MOTIVI:
      }
//<<< switch  Motivo   
   case PER_BIGLIETTARE_MA_USO_MOTORE_E_POLIMETRICHE:
      return SIPAX_RichiestaPerRelazione(Da,A,Motivo,Data,Ora, OpzioneUtente);
   case PER_BIGLIETTARE:
   default:
      return M300_RichiestaPerRelazione(Da,A);
   } /* endswitch */
   
   #else   // >>> #ifdef PARTEINFORMATORE   // NON Posso utilizzare il motore
   return M300_RichiestaPerRelazione(Da,A);
   #endif
   
   #else   // >>> #ifdef PARTEPRECARICATO   // NON Devo usare i precaricati per bigliettare
   
   #ifdef PARTEINFORMATORE   // Posso utilizzare il motore
   return SIPAX_RichiestaPerRelazione(Da,A,Motivo,Data,Ora, OpzioneUtente);
   #else   // >>> #ifdef PARTEINFORMATORE   // NON Posso utilizzare il motore
   #error "Si deve definire o PARTEPRECARICATO o PARTEINFORMATORE"
   return NULL;
   #endif
   #endif
//<<< MM_ELENCO_PERCORSO_E_SOLUZIONI * MM_RETE_FS::RichiestaPerRelazione ID Da,ID A,MOTIVO Motivo, SDATA  Data,WORD Ora, OPZIONE_UTENTE OpzioneUtente  
};

//----------------------------------------------------------------------------
// SIPAX_RichiestaPerRelazione
//----------------------------------------------------------------------------
// Questo metodo crea un MM_ELENCO_PERCORSO_E_SOLUZIONI che dovra' essere
// poi cancellato con un delete
void MM_RETE_FS::InterrompiElaborazione(){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::InterrompiElaborazione()"
   ERRSTRING("Richiesta interruzione elaborazione");
   DatiOrarioA->ElaborazioneInterrotta = TRUE;
};
MM_ELENCO_PERCORSO_E_SOLUZIONI * MM_RETE_FS::SIPAX_RichiestaPerRelazione(
   ID Da,
   ID A,
   MOTIVO Motivo,
   SDATA  Data,
   WORD Ora,
   OPZIONE_UTENTE OpzioneUtente
){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::SIPAX_RichiestaPerRelazione()"
   
   MM_ELENCO_PERCORSO_E_SOLUZIONI * EPerc = NULL;
   ReasonCode = RIC_OK;
   
   #ifdef  PARTEINFORMATORE
   GRAFO & Grafo = GRAFO::Gr();
   WORD OraAttuale = Time() / 6000;
   if(Ora == 0xffff)Ora = OraAttuale;
   
   if(Ora >= 1440){
      ReasonCode = RIC_BAD_ORA;
      return NULL;
   }
   
   ULONG Time1= Time();
   
   TRACESTRINGL("Richiesta relazione da ID "+STRINGA(Da)+" ad ID "+STRINGA(A)+
      " Data "+STRINGA(Data)+" ora "+STRINGA(ORA(Ora))+
      " livello ricerca = "+STRINGA(MM_CR.TipoRicerca)+
      " orario in uso: "+STRINGA(InizioValiditaOrario)+
      " -> "+STRINGA(FineValiditaOrario),2);
   
   if(!Grafo[Da].Vendibile || !Grafo[A].Vendibile){
      ERRSTRING("Origine "+STRINGA(Da)+" o destinazione "+STRINGA(A)+" non vendibile: Problema ignorato");
      TRACEID(Da);
      TRACEID(A);
      ReasonCode = Grafo[Da].Vendibile ? RIC_BAD_A : RIC_BAD_DA ;
      return NULL;
   }
   
   if(Da == A){
      ReasonCode = RIC_DA_IS_A;
      return NULL;
   }
   
   DosRequestMutexSem(hmtx, -1);
   Free();
   BOOL OkOrario = !SetOrarioDelGiorno(Data);
   BOOL EliminaSoluzioni = !OkOrario;
   if (!OkOrario){
      ReasonCode = RIC_BAD_DATA; // Debbo comunque segnalare che ho problemi di orario
      ERRSTRING("Impossibile impostare l' orario per il giorno "+STRINGA(Data));
      // Se non ho fatto la richiesta per pura informativa cerco comunque di dare una risposta
      // basandomi su un qualche orario
      // Per prima cosa provo ad andare indietro di un anno e poi a cambiare giorno
      // in modo da mettermi al corrispondente giorno della settimana
      if(Motivo != PER_INFORMATIVA){
         SDATA Data2 = Data;
         Data2.Anno --;
         BYTE  GG1 = Data.GiornoDellaSettimana(); // 0 = Lunedi ...
         BYTE  GG2 = Data2.GiornoDellaSettimana(); // 0 = Lunedi ...
         int Delta = (7 + GG2 - GG1) % 7 ;
         while(Delta){--Delta;--Data2;}; // Provo ad andare indietro
         OkOrario = !SetOrarioDelGiorno(Data2);
         if(!OkOrario){
            ERRSTRING("Fallito tentativo di impostare l' orario per il giorno "+STRINGA(Data2));
            // Provo anche ad andare avanti nel caso fossero i primi giorni del precedente orario
            for(int i = 0; i < 7 ; i++)++Data2; // Provo ad andare avanti di una settimana
            OkOrario = !SetOrarioDelGiorno(Data2);
            if(!OkOrario){
               ERRSTRING("Fallito tentativo di impostare l' orario per il giorno "+STRINGA(Data2));
               // Adesso provo sulla prima data "possibile" sull' orario corrente
               if (DatiOrarioA) {  // Altrimenti non ho orari validi AT ALL
                  Data2 = DatiOrarioA->ValiditaOrario.Inizio;
                  for(int i = 0; i < 7 ; i++)++Data2; // Vado avanti di una settimana
                  BYTE  GG2 = Data2.GiornoDellaSettimana(); // 0 = Lunedi ...
                  Delta = (7 + GG1 - GG2) % 7 ;
                  while(Delta){--Delta;++Data2;}; // E vado ancora avanti
                  OkOrario = !SetOrarioDelGiorno(Data2);
                  if(!OkOrario){
                     ERRSTRING("Fallito tentativo di impostare l' orario per il giorno "+STRINGA(Data2));
                     // Adesso provo sulla prima data "possibile" sull' altro orario
                     DATI_ORARIO_FS * Ta = OrarioInCorso;
                     if(Ta == DatiOrarioA ) Ta = OrarioProssimo;
                     if(Ta){
                        Data2 = Ta->ValiditaOrario.Inizio;
                        for(int i = 0; i < 7 ; i++)++Data2; // Vado avanti di una settimana
                        BYTE  GG2 = Data2.GiornoDellaSettimana(); // 0 = Lunedi ...
                        Delta = (7 + GG1 - GG2) % 7 ;
                        while(Delta){--Delta;++Data2;}; // E vado ancora avanti
                        OkOrario = !SetOrarioDelGiorno(Data2);
                        if(!OkOrario){
                           ERRSTRING("Fallito tentativo di impostare l' orario per il giorno "+STRINGA(Data2));
                        }
                     }
                  }
//<<<          if  DatiOrarioA     // Altrimenti non ho orari validi AT ALL
               }
//<<<       if !OkOrario  
            }
//<<<    if !OkOrario  
         }
         if(OkOrario)Data = Data2;
//<<< if Motivo != PER_INFORMATIVA  
      }
//<<< if  !OkOrario  
   }
   if (!OkOrario){
      BEEP;
   } else {
      MM_CONTROLLO_RICERCA::SetProfondita(TipoRicerca); // Imposto profondita' di ricerca
      
      // Adesso controllo se e' possibile utilizzare dei risultati memorizzati invece di ricalcolarli ex novo.
      BOOL TrovatoSuCache = FALSE;
      RICHIESTA_MOTORE Richiesta;
      ZeroFill(Richiesta); // Necessario perche' lavorando su BIT field potrei lasciare degli spare bits in uno stato impredicibile
      if (CacheMotore.Attiva) {
         Richiesta.KeyAssoluta.From    = Da;
         Richiesta.KeyAssoluta.To      = A ;
         Richiesta.KeyAssoluta.Livello = MM_CR.TipoRicerca;
         Richiesta.KeyAssoluta.Giorno = Data.Giorno     ;
         Richiesta.KeyAssoluta.Mese   = Data.Mese       ;
         Richiesta.KeyAssoluta._Anno  = Data.Anno - 1980;
         Richiesta.DataEdOraRichiesta.Sec2   = 0               ;
         Richiesta.DataEdOraRichiesta.Minuti = Ora % 60        ;
         Richiesta.DataEdOraRichiesta.Ore    = Ora / 60        ;
         Richiesta.DataEdOraRichiesta.Giorno = Data.Giorno     ;
         Richiesta.DataEdOraRichiesta.Mese   = Data.Mese       ;
         Richiesta.DataEdOraRichiesta._Anno  = Data.Anno - 1980;
         #ifdef DBGCACHE2
         Bprintf3("Richiesta: Da %i A %i Livello %i Partenza : %s",
            Da,A,MM_CR.TipoRicerca,(CPSZ)Richiesta.DataEdOraRichiesta.TimeStamp());
         #endif
         
         EPerc = CacheMotore.GetSol(Richiesta,Orario); // Cerco la soluzione su Cache
         TrovatoSuCache = (EPerc != NULL);
      } /* endif */
      
      if(!TrovatoSuCache){
         STRINGA NomeP("Percorsi Calcolati e soluzioni");
         EPerc = new MM_ELENCO_PERCORSO_E_SOLUZIONI(NomeP,this);
         BOOL Result;
         Result = DatiOrarioA->NewRisolvi_ST(Da,A,Data,Ora);
         if(!Result){
            ERRSTRING("Errore: Relazione non accettabile");
            ReasonCode = RIC_BAD_REL;
         } else {
            PreparaOut(*EPerc);
            TRACESTRING("Sort Dei percorsi");
            EPerc->Sort(Ora);
            
            // Salvo su cache
            if (CacheMotore.Attiva && !CacheMotore.PutSol(Richiesta, EPerc, Orario)){
               ERRSTRING("Errore salvando la soluzione su Cache");
            }
            
            TRACESTRING("Fine");
         } /* endif */
      } /* endif */
//<<< if  !OkOrario  
   };
   
   DosReleaseMutexSem(hmtx);
   
   if(EliminaSoluzioni && EPerc){
      FORALL((*EPerc), Pe) (*EPerc)[Pe]->Soluzioni->Clear();
   }
   
   #ifdef DBGSOL
   if(EPerc)EPerc->Trace("Percorsi Trovati: ",LtrcPerc);
   #endif
   
   #else
   ERRSTRING("Spiacente: la gestione delle tariffe via orario e' disabilitata");
   BEEP;
   #endif
   TRACESTRINGL("Tempo di elaborazione: "+STRINGA(10*(Time() - Time1))+" millisecondi",2);
   return EPerc;
//<<< MM_ELENCO_PERCORSO_E_SOLUZIONI * MM_RETE_FS::SIPAX_RichiestaPerRelazione 
};

// ---------------------------------------------------------------------
// Funzioncina interna per trasferire le soluzioni in out
// ---------------------------------------------------------------------
void MM_RETE_FS::PreparaOut(MM_ELENCO_PERCORSO_E_SOLUZIONI & EPerc ){
   #undef TRCRTN
   #define TRCRTN "MM_RETE::PreparaOut"
   GRAFO & Grafo = GRAFO::Gr();
   TRACESTRING("Trovati "+STRINGA(Grafo.PercorsiGrafo.Dim())+" percorsi sul grafo con "+STRINGA(DatiOrarioA->Soluzioni.Dim())+" soluzioni");
   DatiOrarioA->ValorizzaSoluzioni();
   TRACESTRING("Valorizzate Soluzioni: vi sono "+STRINGA(Grafo.Instradamenti.Dim())+" instradamenti");
   ORD_FORALL(Grafo.Instradamenti,is){
      TRACELONG("Instradamento Nø ",is+1);
      MM_PERCORSO_E_SOLUZIONI * PercSol = new MM_PERCORSO_E_SOLUZIONI();
      MM_I_PERCORSO * Perc  = new MM_I_PERCORSO();
      Perc->DaPrecaricato=FALSE;
      PercSol->Percorso = Perc;
      MM_ELENCO_SOLUZIONE * Sols = new MM_ELENCO_SOLUZIONE;
      PercSol->Soluzioni= Sols;
      PERCORSO_INSTRADAMENTI & Instradamento = * Grafo.Instradamenti[is];
      Perc->Set(Instradamento);
      Perc->FormaStandard.Trace(*GRAFO::Grafo,TRCRTN " Forma Standard:");
      if (Perc->FormaStandard.Dim() > 8) {
         ERRSTRING("Attenzione: Piu' di 6 stazioni intermedie di instradamento, biglietto non ben stampabile");
         // BEEP;
      } /* endif */
      TRACEVSTRING2(Perc->FormaStandardStampabile);
      TRACEVLONG(Perc->DatiTariffazione.Lunghezza());
      ORD_FORALL(DatiOrarioA->Soluzioni,so){
         SOLUZIONE & Soluzione = * DatiOrarioA->Soluzioni[so];
         if(&Soluzione == NULL)continue; // Gia' elaborato
         PERCORSO_INSTRADAMENTI  * IstradSol = Grafo.PercorsiGrafo[Soluzione.Percorso]->Istr;
         if (IstradSol == & Instradamento && Soluzione.Valida) { // Ok e' di questo instradamento ed e' valida
            (*Sols) += &Soluzione;
            DatiOrarioA->Soluzioni[so] = NULL;
         } /* endif */
      };
      
      if( Sols->Dim() == 0){
         delete PercSol;
      } else if(PercSol->Percorso->DatiTariffazione.Lunghezza() > 0 ){
         EPerc += PercSol;
      } else if( !ScartaSoluzioniNonTariffabili ){
         EPerc += PercSol;
      } else {
         ERRSTRING("Eliminato percorso non tariffabile con "+STRINGA(Sols->Dim())+" soluzioni");
         BEEP;
         delete PercSol;
      }
//<<< ORD_FORALL Grafo.Instradamenti,is  
   };
   TRACESTRING("Cancello le soluzioni non utilizzate in output");
   DatiOrarioA->Soluzioni.Clear();
//<<< void MM_RETE_FS::PreparaOut MM_ELENCO_PERCORSO_E_SOLUZIONI & EPerc   
};

// ---------------------------------------------------------------------
// Questo metodo e' per il caricamento su TPF ma puo' essere usato anche altrove
// ---------------------------------------------------------------------
// Ritorna esattamente gli stessi dati ed errori di richiesta per relazione
// NB: Non tiene conto della data, ma fornisce la periodicita' in Out
MM_ELENCO_PERCORSO_E_SOLUZIONI * MM_RETE_FS::MezziDirettiPrenotabili(ID Da,ID A){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::RETE_FS::MezziDirettiPrenotabili"
   
   MM_ELENCO_PERCORSO_E_SOLUZIONI * EPerc = NULL;
   ReasonCode = RIC_OK;
   
   GRAFO & Grafo = GRAFO::Gr();
   ULONG Time1= Time();
   
   TRACESTRINGL("Richiesta relazione da ID "+STRINGA(Da)+" ad ID "+STRINGA(A)+
      " livello ricerca = "+STRINGA(MM_CR.TipoRicerca)+
      " orario in uso: "+STRINGA(InizioValiditaOrario)+
      " -> "+STRINGA(FineValiditaOrario),2);
   
   if(!Grafo[Da].Vendibile || !Grafo[A].Vendibile){
      ERRSTRING("Origine "+STRINGA(Da)+" o destinazione "+STRINGA(A)+" non vendibile: Problema ignorato");
      TRACEID(Da);
      TRACEID(A);
      ReasonCode = Grafo[Da].Vendibile ? RIC_BAD_A : RIC_BAD_DA ;
      return NULL;
   }
   
   if(Da == A){
      ReasonCode = RIC_DA_IS_A;
      return NULL;
   }
   
   DosRequestMutexSem(hmtx, -1);
   Free();
   STRINGA NomeP("Percorsi Calcolati e soluzioni");
   EPerc = new MM_ELENCO_PERCORSO_E_SOLUZIONI(NomeP,this);
   BOOL Result;
   Result = DatiOrarioA->MezziDirettiPrenotabili(Da,A,DataCorrente);
   if(!Result){
      ERRSTRING("Errore: Relazione non accettabile");
      ReasonCode = RIC_BAD_REL;
   } else {
      PreparaOut(*EPerc);
      TRACESTRING("Sort Dei percorsi");
      EPerc->Sort(0);
      
      TRACESTRING("Fine");
   } /* endif */
   
   DosReleaseMutexSem(hmtx);
   
   #ifdef DBGSOL
   if(EPerc)EPerc->Trace("Percorsi Trovati: ",LtrcPerc);
   #endif
   
   TRACESTRINGL("Tempo di elaborazione: "+STRINGA(10*(Time() - Time1))+" millisecondi",2);
   return EPerc;
   
//<<< MM_ELENCO_PERCORSO_E_SOLUZIONI * MM_RETE_FS::MezziDirettiPrenotabili ID Da,ID A  
};

//----------------------------------------------------------------------------
// MM_ELENCO_PERCORSO_E_SOLUZIONI::Sort
//----------------------------------------------------------------------------
void MM_ELENCO_PERCORSO_E_SOLUZIONI::Sort(WORD Ora){
   #undef TRCRTN
   #define TRCRTN "MM_ELENCO_PERCORSO_E_SOLUZIONI::Sort"
   
   // Questa strutturina per ordinare anche per ora di arrivo e non solo per ora partenza
   // Gestisco pero' (per semplicita') solo due soluzioni che partono alla stessa ora
   struct SUMMARY {
      WORD OrdineBase   ;
      WORD TempoViaggio ;
      BYTE Extra;
   };
   static HASH<SUMMARY> Hash(128,128);
   
   Hash.Clear();
   
   if (Ora == 0xffff)Ora = Time() / 6000;
   TRACESTRING2("Sort Delle soluzioni, Ora di partenza = ",ORA(Ora));
   // Eseguo il sort delle soluzioni nell' ambito del percorso
   ORD_FORALL(THIS,ii){
      //TRACEVPOINTER(this);
      //TRACEVPOINTER(THIS[ii]);
      //TRACEVPOINTER(THIS[ii]->Percorso);
      //TRACEVPOINTER(THIS[ii]->Percorso->Reserved);
      PERCORSO_INSTRADAMENTI & Pi = *( PERCORSO_INSTRADAMENTI *)THIS[ii]->Percorso->Reserved;
      if(&Pi)Pi.MinPartenza = 9999;
      MM_ELENCO_SOLUZIONE & Sols = * THIS[ii]->Soluzioni;
      if (&Sols != NULL) {
         FORALL(Sols,i){ // Determino L' ordine delle soluzioni e la minima ora di partenza per ogni percorso
            MM_SOLUZIONE & Sol = *Sols[i];
            Sol.Ordine = 10 * TempoTrascorso(Ora,Sol.OraPartenzaVera());
            SUMMARY &Wrk = *Hash.Alloca();
            Wrk.OrdineBase  = Sol.Ordine;
            Wrk.TempoViaggio= TempoTrascorso(Sol.OraPartenzaVera(),Sol.OraArrivoVera());
            if( Sol.TempoTotaleDiPercorrenza > 1440 )Wrk.TempoViaggio = Sol.TempoTotaleDiPercorrenza ; // Correzione per i viaggi lunghi e cambio data
            
            // Cerco nell' Hash
            SUMMARY & Wrk2 = *Hash.Cerca( &Wrk, 2);
            if (&Wrk2 == NULL) { // Non trovata
               Wrk.Extra = 5; // Medium value
            } else if(Wrk2.TempoViaggio < Wrk.TempoViaggio) {
               Wrk.Extra = Wrk2.Extra + 1;
            } else {
               Wrk.Extra = Wrk2.Extra - 1;
            } /* endif */
            Sol.Ordine += Wrk.Extra;
            Hash.Metti(2); // Aggiungo
            
            if(&Pi)Bottom(Pi.MinPartenza , TempoTrascorso(Ora,Sol.OraPartenzaVera()));
//<<<    FORALL Sols,i   // Determino L' ordine delle soluzioni e la minima ora di partenza per ogni percorso
         } /* endfor */
         if(MultiThread) DOE1; // I Sort vanno semaforizzati per problemi Borland
         qsort(&Sols[0],Sols.Dim(),sizeof(void*),MM_sort_functionSol); // Sort delle soluzioni del percorso
         if(MultiThread) DOE2;
//<<< if  &Sols != NULL   
      } /* endif */
//<<< ORD_FORALL THIS,ii  
   };
   
   // Eseguo il sort dei percorsi (si trascina appresso anche le soluzioni)
   TRACESTRING("Sort Dei percorsi");
   if(MultiThread) DOE1; // I Sort vanno semaforizzati per problemi Borland
   qsort(ArrayPVOID,Dim(),sizeof(void*),MM_sort_functionPerc);
   if(MultiThread) DOE2;
   
//<<< void MM_ELENCO_PERCORSO_E_SOLUZIONI::Sort WORD Ora  
};

//----------------------------------------------------------------------------
// MM_RETE_FS::AttivaAutoDiagnostica
//----------------------------------------------------------------------------
// Questo metodo attiva l' AutoDiagnostica
// L' autodiagnostica si disattiva da sŠ alla fine di ogni richiesta
void MM_RETE_FS::AttivaAutoDiagnostica(STRINGA & ElencoTreni){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::AttivaAutoDiagnostica"
   ERRSTRING(" " VRS(ElencoTreni));
   DATI_ORARIO_FS::AutoDiagnostica = TRUE;
   DATI_ORARIO_FS::Target.Imposta(ElencoTreni);
};
// Questo metodo attiva l' AutoDiagnostica
// L' autodiagnostica si disattiva da sŠ alla fine di ogni richiesta
//----------------------------------------------------------------------------
// MM_RETE_FS::AttivaAutoDiagnosticaPath
//----------------------------------------------------------------------------
void MM_RETE_FS::AttivaAutoDiagnosticaPath(ARRAY_ID & StazioniIntermedie){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::AttivaAutoDiagnosticaPath"
   DATI_ORARIO_FS::AutoDiagnosticaPath = TRUE;
   DATI_ORARIO_FS::StazioniTarget = StazioniIntermedie;
   ERRSTRING(" " VRS(StazioniIntermedie.ToStringa()));
};

//----------------------------------------------------------------------------
// Free
//----------------------------------------------------------------------------
// Questo rilascia i buffers interni se inattivo per evitare tempi inutili di attesa.
void MM_RETE_FS::Free(){
   #undef TRCRTN
   #define TRCRTN "MM_RETE_FS::Free"
   if(hmtx)DosRequestMutexSem(hmtx, -1);
   // Vedo se vi siano gia' delle soluzioni REALI
   int CountSol = 0;
   ELENCO_Oggetti CercaSubordinati(const STRINGA & Classe=NUSTR);
   FORALL(OggettiDellaComponente,i){
      if (OggettiDellaComponente[i].IsA("MM_ELENCO_PERCORSO_E_SOLUZIONI")) {
         MM_ELENCO_PERCORSO_E_SOLUZIONI & El = (MM_ELENCO_PERCORSO_E_SOLUZIONI&)OggettiDellaComponente[i];
         FORALL(El,j)
            if(El[j]->Soluzioni != NULL && El[j]->Soluzioni->Dim() > 0){
            CountSol ++;
            break;
         };
      } /* endif */
   };
   if(CountSol > 0) {
      ERRSTRING("Errore : non sono state cancellate tutte le aree della precedente richiesta");
      BEEP;
      OggettiDellaComponente.Trace();
   } /* endif */
   GRAFO::Grafo->Clear();
   if(hmtx)DosReleaseMutexSem(hmtx);
//<<< void MM_RETE_FS::Free   
};
//----------------------------------------------------------------------------
// MM_PERCORSO
//----------------------------------------------------------------------------
// NB: Se opero con i precaricati creo MM_I_PERCORSO, altrimenti MM_PERCORSO
MM_PERCORSO::MM_PERCORSO(const STRINGA& Nome, MM_RETE_FS * Creatore):
OGGETTO(Nome,ALTRI,Creatore),MM_I_PERCORSO(){
   #undef TRCRTN
   #define TRCRTN "@MM_PERCORSO"
   ClassName = "MM_PERCORSO";
};
MM_I_PERCORSO::MM_I_PERCORSO(){
   #undef TRCRTN
   #define TRCRTN "@MM_I_PERCORSO"
   DatiTariffazione.KmReali                        = 0;
   DatiTariffazione.KmAggiuntivi                   = 0;
   DatiTariffazione.KmMare                         = 0;
   CodCumCVB                                       = 0;
   DatiTariffazione.KmConcessi1                    = 0;
   DatiTariffazione.CodConcessione1                = 0;
   DatiTariffazione.CodLinea1                      = 0;
   DatiTariffazione.TransitoFS1                    = 0;
   DatiTariffazione.KmConcessi2                    = 0;
   DatiTariffazione.CodConcessione2                = 0;
   DatiTariffazione.CodLinea2                      = 0;
   DatiTariffazione.TransitoFS2                    = 0;
   DatiTariffazione.TransitoFS3                    = 0;
   PrefComm                                        = 0;
   DatiTariffazione.CodiceTariffaRegionale         = 0xFE; // = Deve essere ancora impostato
   DatiTariffazione.CodiceCCRDestinazione          = 0;
   DatiTariffazione.CodiceCCROrigine               = 0;
   DatiTariffazione.Stato                          = DATI_TARIFFAZIONE::TARIFFA_NON_CALCOLATA;
   DatiTariffazione.Diretta = FALSE;
   memset(&DatiTariffazione.Andata,0,sizeof(DatiTariffazione.Andata))  ;
   DaPrecaricato =TRUE;
//<<< MM_I_PERCORSO::MM_I_PERCORSO   
};
//----------------------------------------------------------------------------
// MM_ELENCO_PERCORSO
//----------------------------------------------------------------------------
MM_ELENCO_PERCORSO::MM_ELENCO_PERCORSO(const STRINGA& Nome, MM_RETE_FS * Creatore):
OGGETTO(Nome,ALTRI,Creatore){
   #undef TRCRTN
   #define TRCRTN "@MM_ELENCO_PERCORSO"
   ClassName = "MM_ELENCO_PERCORSO";
};
//----------------------------------------------------------------------------
// MM_ELENCO_PERCORSO_E_SOLUZIONI
//----------------------------------------------------------------------------
MM_ELENCO_PERCORSO_E_SOLUZIONI::MM_ELENCO_PERCORSO_E_SOLUZIONI(const STRINGA& Nome, MM_RETE_FS * Creatore):
OGGETTO(Nome,ALTRI,Creatore){
   #undef TRCRTN
   #define TRCRTN "@MM_ELENCO_PERCORSO_E_SOLUZIONI"
   ClassName = "MM_ELENCO_PERCORSO_E_SOLUZIONI";
};

//----------------------------------------------------------------------------
// MM_PERIODICITA : Accesso alle funzioni di periodicita'
//----------------------------------------------------------------------------
BOOL MM_PERIODICITA::Circola(SDATA & Giorno){
   PERIODICITA & Per = (PERIODICITA &)THIS;
   return Per.Circola (Giorno - PERIODICITA::InizioOrario);
};
void  MM_PERIODICITA::Trace(STRINGA Msg, int Livello) {
   PERIODICITA & Per = (PERIODICITA &)THIS;
   Per.Trace(Msg,Livello);
};
ELENCO_S MM_PERIODICITA::Decod(BOOL ConTestata,BOOL DoubleSpace,BOOL Points){
   PERIODICITA & Per = (PERIODICITA &)THIS;
   return Per.Decod(ConTestata,DoubleSpace,Points);
};
//----------------------------------------------------------------------------
// MM_PERIODICITA::Circolazione_STRINGA()
//----------------------------------------------------------------------------
const char * MM_PERIODICITA::Circolazione_STRINGA(){  // Rimane valida fino alla prossima chiamata
   #undef TRCRTN
   #define TRCRTN "MM_PERIODICITA::Circolazione_STRINGA"
   PERIODICITA & Per = (PERIODICITA &)THIS;
   static char * Settimana = "LMmGVSD ";
   static char Out[8];
   BYTE Set = Per.Sett();
   for (int i=0,j=1;i < 7;i++ ) {
      if(j & Set) {
         Out[i] = Settimana[i];
      } else {
         Out[i] = ' ';
      } /* endif */
      j += j; // moltiplico per 2
   } /* endfor */
   Out[7]= '\0';
   return Out;
}
//----------------------------------------------------------------------------
// MM_SOLUZIONE
//----------------------------------------------------------------------------
void _export MM_SOLUZIONE::GetNote(){ ((SOLUZIONE *)this)->GetNote(); }

//----------------------------------------------------------------------------
// Trace di MM_I_PERCORSO
//----------------------------------------------------------------------------
void _export MM_I_PERCORSO::Trace(const STRINGA& Msg, int Livello){
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::Trace"
   
   char Out[256];
   if(Livello > trchse)return;
   ERRSTRING("...............................................");
   ERRSTRING(Msg);
   FormaStandard.Trace(Stazioni,"Stazioni del percorso",1);
   ERRSTRING("Forma stampabile: "+FormaStandardStampabile);
   if(PrefComm       ){TRACEVLONGL(PrefComm       ,1);};
   DatiTariffazione.Trace("Dati di tariffazione",1);
};

//----------------------------------------------------------------------------
// Trace di MM_ELENCO_PERCORSO
//----------------------------------------------------------------------------
void _export MM_ELENCO_PERCORSO::Trace(const STRINGA& Msg, int Livello){
   #undef TRCRTN
   #define TRCRTN "MM_ELENCO_PERCORSO::Trace"
   
   char Out[256];
   if(Livello > trchse)return;
   
   if(Msg != NUSTR)ERRSTRING(Msg);
   
   ERRSTRING("...............................................");
   if(Componente == NULL){
      ERRSTRING("? Non ho il collegamento con MM_RETE_FS");
      BEEP;
      return;
   };
   
   ORD_FORALL(THIS,is){
      MM_I_PERCORSO & Perc = *THIS[is];
      sprintf(Out,"Instradamento Nø %i\t%s\tLunghezza %i Km\n",is,(CPSZ)Perc.FormaStandardStampabile,Perc.DatiTariffazione.Lunghezza());
      ERRSTRING(Out);
   };
//<<< void _export MM_ELENCO_PERCORSO::Trace const STRINGA& Msg, int Livello  
};

//----------------------------------------------------------------------------
// Trace di MM_ELENCO_PERCORSO_E_SOLUZIONI
//----------------------------------------------------------------------------
void _export MM_ELENCO_PERCORSO_E_SOLUZIONI::Trace(const STRINGA& Msg, int Livello){
   #undef TRCRTN
   #define TRCRTN "MM_ELENCO_PERCORSO_E_SOLUZIONI::Trace"
   
   char Out[256];
   if(Livello > trchse)return;
   
   if(Msg != NUSTR)ERRSTRING(Msg);
   
   if(Componente == NULL){
      ERRSTRING("? Non ho il collegamento con MM_RETE_FS");
      BEEP;
      return;
   };
   
   ORD_FORALL(THIS,is){
      MM_I_PERCORSO & Perc = * THIS[is]->Percorso;
      sprintf(Out,"Instradamento Nø %i\t%s\tLunghezza %i Km\n",is,(CPSZ)Perc.FormaStandardStampabile,Perc.DatiTariffazione.Lunghezza());
      ERRSTRING(Out);
      MM_ELENCO_SOLUZIONE & Soluzioni = * THIS[is]->Soluzioni;
      ORD_FORALL(Soluzioni,so){
         Soluzioni[so]->Trace("Soluzione Nø "+STRINGA(so),Livello);
      };
   };
//<<< void _export MM_ELENCO_PERCORSO_E_SOLUZIONI::Trace const STRINGA& Msg, int Livello  
};
void MM_I_PERCORSO::StoreTo(BUFR & To, ELENCO & Instradamenti){
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::StoreTo()"
   
   #ifdef DBGCACHE
   TRACELONG("Inizio: DimBuf = ",To.Dim());
   #endif
   To.Store2((BUFR&)FormaStandard  );
   To.Store2((BUFR&)IstradamentoCVB);
   To.Store(FormaStandardStampabile);
   To.Store(CodCumCVB              );
   To.Store(PrefComm               );
   To.Store(DaPrecaricato          );
   To.Store(VRB(DatiTariffazione)  );
   if(!DaPrecaricato){
      int Idx = Instradamenti.CercaPerPuntatore(Reserved);
      if(Idx <  0){
         // Metto in Instradamenti i percorsi (del Grafo) utilizzati
         Instradamenti += Reserved;
         Idx =  Instradamenti.Dim() -1;
      }
      To.Store((USHORT)Idx  ); // Non memorizzo direttamente Reserved;
   }
   #ifdef DBGCACHE
   Trace("Fine: DimBuf = "+STRINGA(To.Dim()));
   #endif
   
//<<< void MM_I_PERCORSO::StoreTo BUFR & To, ELENCO & Instradamenti  
};
void MM_I_PERCORSO::ReStoreFrom(BUFR & From){                    // Richiede che il grafo abbia gia' gli instradamenti
   #undef TRCRTN
   #define TRCRTN "MM_I_PERCORSO::ReStoreFrom()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: " VRS(sizeof(THIS))+" Pointer = ",From.Pointer);
   #endif
   From.ReStore2((BUFR&)FormaStandard  );
   From.ReStore2((BUFR&)IstradamentoCVB);
   From.ReStore(FormaStandardStampabile);
   From.ReStore(CodCumCVB              );
   From.ReStore(PrefComm               );
   From.ReStore(DaPrecaricato          );
   From.ReStore(VRB(DatiTariffazione)  );
   if(!DaPrecaricato){
      USHORT Idx;
      From.ReStore(Idx                    );
      Reserved = GRAFO::Gr().Instradamenti[Idx];
   } else {
      Reserved = NULL;
   }
   #ifdef DBGCACHE
   Trace("Fine: Pointer = "+STRINGA(From.Pointer));
   #endif
//<<< void MM_I_PERCORSO::ReStoreFrom BUFR & From                      // Richiede che il grafo abbia gia' gli instradamenti
};

//----------------------------------------------------------------------------
// MM_SOLUZIONE::StoreTo
//----------------------------------------------------------------------------
#define AfterUsate (offsetof(MM_SOLUZIONE,Tratte) + (NumeroTratte * sizeof(TRATTA_TRENO)))
#define AfterTutte (offsetof(MM_SOLUZIONE,Tratte) + (MAX_TRATTE   * sizeof(TRATTA_TRENO)))
void MM_SOLUZIONE::StoreTo(BUFR & To){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::StoreTo"
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::StoreTo()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: DimBuf = ",To.Dim());
   #endif
   
   // Store dei dati SALTANDO le tratte non utilizzate
   To.Store(this, AfterUsate );
   To.Store((BYTE*)this + AfterTutte ,  sizeof(MM_SOLUZIONE) - AfterTutte );
   #ifdef DBGCACHE
   Trace("Fine: DimBuf = "+STRINGA(To.Dim()));
   #endif
};
//----------------------------------------------------------------------------
// MM_SOLUZIONE::ReStoreFrom
//----------------------------------------------------------------------------
void MM_SOLUZIONE::ReStoreFrom(BUFR & From){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::ReStoreFrom"
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::ReStoreFrom()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: " VRS(sizeof(THIS))+" Pointer = ",From.Pointer);
   #endif
   
   // ReStore dei dati SALTANDO le tratte non utilizzate
   // Faccio in tre volte per ristorare la variabile NumeroTratte
   ZeroFill(Tratte);
   From.ReStore(this,offsetof(MM_SOLUZIONE,Tratte));
   From.ReStore(&Tratte, NumeroTratte * sizeof(TRATTA_TRENO));
   From.ReStore((BYTE*)this + AfterTutte ,  sizeof(MM_SOLUZIONE) - AfterTutte );
   InfoNote = NULL;
   for (int i = 0;i < NumeroTratte ;i++ )Tratte[i].NomeSpecifico=NULL;
   #ifdef DBGCACHE
   Trace("Fine: Pointer = "+STRINGA(From.Pointer));
   #endif
};
#undef AfterUsate
#undef AfterTutte
//----------------------------------------------------------------------------
// MM_SOLUZIONE::IdentificaStazioni
//----------------------------------------------------------------------------
BOOL MM_SOLUZIONE::IdentificaStazioni(ARRAY_ID & NodiDiTransito, BOOL AncheTransiti){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::IdentificaStazioni"
   
   NodiDiTransito.Clear();
   static ARRAY_NOD Transiti(100); // Transiti DI UNA TRATTA
   
   for (int t = 0; t <  NumeroTratte ;t++ ) {
      Transiti.Clear();
      
      SOLUZIONE::TRATTA_TRENO    & Tratta = Tratte[t];
      CLU_BUFR & Clu = *CLU_BUFR::GetCluster(Tratta.IdCluster , Tratta.Concorde, TRUE);
      if(&Clu == NULL){
         ERRSTRING("Errore nell' accesso ai dati dei treni");
         NodiDiTransito.Clear();
         return FALSE;
      }
      
      if(Tratta.IdMezzoVirtuale == 0){ // E' una tratta MULTISTAZIONE
         if(t != 0 ){
            NodiDiTransito += Clu.Nodi[Tratta.OfsIn].IdNodo;
         }
      } else {
         
         ID Da = Clu.Nodi[Tratta.OfsIn].IdNodo;  // Prima stazione della tratta
         // ID A  = Clu.Nodi[Tratta.OfsOut].IdNodo; // Ultima stazione della tratta
         NodiDiTransito += Da;
         Clu.FindTreno(Tratta.OfsTreno);
         BYTE Pin  = Clu.Nodo(Tratta.OfsIn).ProgressivoStazione;
         BYTE Pout = Clu.Nodo(Tratta.OfsOut).ProgressivoStazione;
         // Limiti della scansione
         int MinIdx, MaxIdx;
         if (Clu.Concorde) {
            MinIdx = 1;
            MaxIdx = Clu.Dat->NumeroNodi;
         } else {
            MinIdx = Clu.Dat->NumeroNodi - 1;
            MaxIdx = 0;
         } /* endif */
         INFOSTAZ * Stz = &Clu.Nodo(MinIdx);
         INFOSTAZ * Lim = &Clu.Nodo(MaxIdx);
         CLUSSTAZ * Nd  = &Clu.Nodi[MinIdx];
         // BYTE Last = 0;
         while ( Stz != Lim){
            INFOSTAZ & INodo = *Stz;
            if(  // Selezione delle stazioni di transito
               (INodo.ProgressivoStazione >  Pin ) && // Skip stazioni fuori range
               (INodo.ProgressivoStazione <  Pout) && // Skip stazioni fuori range
               (
                  INodo.Ferma() ||
                  (AncheTransiti && INodo.TransitaOCambiaTreno && IsNodo(Nd->IdNodo))
               ) // Stazione da considerare per il treno
            ){
               NOD Transito;
               Transito.Id          = Nd->IdNodo;
               Transito.Progressivo = INodo.ProgressivoStazione;
               Transito.Ferma       = INodo.Ferma();
               Transiti += Transito;
               if (Transito.Progressivo == 0) {
                  ERRSTRING("Errore: la stazione non dovrebbe essere valida per l' instradamento del treno");
                  BEEP;
               } /* endif */
               // Last = INodo.ProgressivoStazione;
            };
            if (Clu.Concorde) {
               Stz ++;
               Nd  ++;
            } else {
               Stz --;
               Nd  --;
            } /* endif */
//<<<    while   Stz != Lim  
         } ;
         // Sort per progressivo stazione
         if(MultiThread) DOE1; // I Sort vanno semaforizzati per problemi Borland
         qsort(&Transiti[0],Transiti.Dim(),sizeof(NOD),SortPiu);
         if(MultiThread) DOE2;
         SCAN(Transiti,Tr,NOD){
            NodiDiTransito += Tr.Id;
         } ENDSCAN ;
//<<< if Tratta.IdMezzoVirtuale == 0   // E' una tratta MULTISTAZIONE
      }
      
      if(t == NumeroTratte-1){ // Ultima tratta
         if(Tratta.IdMezzoVirtuale != 0) NodiDiTransito += Tratta.IdStazioneOut;
      } /* endfor */
//<<< for  int t = 0; t <  NumeroTratte ;t++    
   }
   
   NodiDiTransito.Trace(GRAFO::Gr(),"NodiDiTransito");
   
   return TRUE;
//<<< BOOL MM_SOLUZIONE::IdentificaStazioni ARRAY_ID & NodiDiTransito, BOOL AncheTransiti  
};
//----------------------------------------------------------------------------
// MM_SOLUZIONE::KmParziali
//----------------------------------------------------------------------------
DATI_TARIFFAZIONE_2 MM_SOLUZIONE::KmParziali(ID Da, ID A){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::KmParziali"
   
   DATI_TARIFFAZIONE_2 Out;
   Out.Clear();
   
   ARRAY_ID NodiDiTransito;
   
   if(!IdentificaStazioni(NodiDiTransito, FALSE)){
      ERRSTRING("Errore nell' identificazione nodi di transito");
      Out.Stato = DATI_TARIFFAZIONE::TRATTE_ILLEGALI;
      return Out;
   } /* endif */
   if( !NodiDiTransito.Contiene(Da) || !NodiDiTransito.Contiene(A) ){
      ERRSTRING("Errati parametri di chiamata: Da o A non sono fermate della soluzione");
      Out.Stato = DATI_TARIFFAZIONE::TRATTE_ILLEGALI;
      return Out;
   } /* endif */
   
   if(!IdentificaStazioni(NodiDiTransito, TRUE)){
      ERRSTRING("Errore nell' identificazione nodi di transito");
      Out.Stato = DATI_TARIFFAZIONE::TRATTE_ILLEGALI;
      return Out;
   } /* endif */
   
   int P1 = NodiDiTransito.Dim();
   while(P1 > 0 && NodiDiTransito[P1] != Da) P1 --;
   while (P1-- > 0 ) NodiDiTransito -= 0;
   int P2 = NodiDiTransito.Posizione(A);
   if (P2 < 0) {
      ERRSTRING("Errore : Stazione Da successiva rispetto stazione A");
      Out.Stato = DATI_TARIFFAZIONE::TRATTE_ILLEGALI;
      return Out;
   } else {
      while (P2 < NodiDiTransito.Dim()-1) NodiDiTransito -= P2+1;
   } /* endif */
   if(NodiDiTransito[0] != Da || NodiDiTransito.Last () != A){
      ERRSTRING("Errore  interno algoritmo");
      BEEP;
      Out.Stato = DATI_TARIFFAZIONE::TRATTE_ILLEGALI;
      return Out;
   }
   
   PERCORSO_GRAFO Pg2(NodiDiTransito);
   
   // Dal percorso genero dei nuovi dati di tariffazione
   PERCORSO_POLIMETRICHE PercP;
   PercP.Valorizza(Pg2);
   Out.Set(PercP.DatiTariffazione);
   
   Out.FormaStandard += PercP.Origine;
   ORD_FORALL(PercP.StazioniDiInstradamento,i){
      STAZ_FS & Staz = (*GRAFO::Grafo)[PercP.StazioniDiInstradamento[i]];
      if (Staz.StazioneFS){             // Se e' una stazione FS
         Out.FormaStandard += Staz.Id;
         Out.IstradamentoCVB += Staz.CCR;
         Out.FormaStandardStampabile += Staz.Nome7();
         Out.FormaStandardStampabile += '*';
      } /* endif */
   }
   Out.FormaStandard += PercP.Destinazione;
   
   // SE VUOTA SCRIVO DIRETTA
   if (!Out.FormaStandardStampabile.Dim()) { Out.FormaStandardStampabile="DIRETTA*"; }
   // PERDO L'* FINALE
   Out.FormaStandardStampabile = Out.FormaStandardStampabile(0,Out.FormaStandardStampabile.Dim()-2);
   
   Out.Trace("Dati calcolati per la tratta parziale:");
   return Out;
//<<< DATI_TARIFFAZIONE_2 MM_SOLUZIONE::KmParziali ID Da, ID A  
};
//----------------------------------------------------------------------------
// MM_SOLUZIONE::KmParziali
//----------------------------------------------------------------------------
// Ritorna i dati di tariffazione per tratte parziali (contigue) della soluzione
// In caso di errore imposta il campo Stato dei DATI_TARIFFAZIONE
// Le tratte vanno da 0 a NumeroTratte - 1, gli estremi sono inclusi.
DATI_TARIFFAZIONE_2 MM_SOLUZIONE::KmParziali(BYTE PrimaTratta, BYTE SecondaTratta,
   class MM_I_PERCORSO * Percorso){
   #undef TRCRTN
   #define TRCRTN "MM_SOLUZIONE::KmParziali"
   
   TRACEVLONG(PrimaTratta);
   TRACEVLONG(SecondaTratta);
   TRACEVLONG(NumeroTratte);
   
   DATI_TARIFFAZIONE_2 Out;
   Out.Clear();
   
   if (SecondaTratta < PrimaTratta || SecondaTratta >= NumeroTratte || Percorso == NULL) {
      TRACEVPOINTER(Percorso);
      ERRSTRING("Errati parametri di chiamata");
      Out.Stato = DATI_TARIFFAZIONE::TRATTE_ILLEGALI;
      return Out;
   } /* endif */
   
   // Determino le stazioni di transito a partire dai dati dei treni
   ARRAY_ID NodiDiTransito;
   BOOL Ok = ((SOLUZIONE*)this)->IdentificaNodiDiTransito(NodiDiTransito,PrimaTratta, SecondaTratta);
   if (!Ok){
      ERRSTRING("Errore nell' identificazione nodi di transito");
      Out.Stato = DATI_TARIFFAZIONE::TRATTE_ILLEGALI;
      return Out;
   }
   
   PERCORSO_GRAFO Pg2(NodiDiTransito);
   
   // Dal percorso genero dei nuovi dati di tariffazione
   PERCORSO_POLIMETRICHE PercP;
   PercP.Valorizza(Pg2);
   Out.Set(PercP.DatiTariffazione);
   
   Out.FormaStandard += PercP.Origine;
   ORD_FORALL(PercP.StazioniDiInstradamento,i){
      STAZ_FS & Staz = (*GRAFO::Grafo)[PercP.StazioniDiInstradamento[i]];
      if (Staz.StazioneFS){             // Se e' una stazione FS
         Out.FormaStandard += Staz.Id;
         Out.IstradamentoCVB += Staz.CCR;
         Out.FormaStandardStampabile += Staz.Nome7();
         Out.FormaStandardStampabile += '*';
      } /* endif */
   }
   Out.FormaStandard += PercP.Destinazione;
   
   // SE VUOTA SCRIVO DIRETTA
   if (!Out.FormaStandardStampabile.Dim()) { Out.FormaStandardStampabile="DIRETTA*"; }
   // PERDO L'* FINALE
   Out.FormaStandardStampabile = Out.FormaStandardStampabile(0,Out.FormaStandardStampabile.Dim()-2);
   
   Out.Trace("Dati calcolati per la tratta parziale:");
   return Out;
//<<< DATI_TARIFFAZIONE_2 MM_SOLUZIONE::KmParziali BYTE PrimaTratta, BYTE SecondaTratta,
};


void MM_PERCORSO_E_SOLUZIONI::StoreTo(BUFR & To, ELENCO & Instradamenti){
   #undef TRCRTN
   #define TRCRTN "MM_PERCORSO_E_SOLUZIONI::StoreTo()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: DimBuf = ",To.Dim());
   #endif
   
   Percorso->StoreTo(To,Instradamenti);
   To.Store((USHORT)Soluzioni->Dim() );
   ORD_FORALL((*Soluzioni),i){
      (*Soluzioni)[i]->StoreTo(To);
   }
   #ifdef DBGCACHE
   TRACESTRING("Fine: Nø Soluzioni = "+STRINGA(Soluzioni->Dim())+" DimBuf = "+STRINGA(To.Dim()));
   #endif
};
void MM_PERCORSO_E_SOLUZIONI::ReStoreFrom(BUFR & From){                    // Richiede che il grafo abbia gia' gli instradamenti
   #undef TRCRTN
   #define TRCRTN "MM_PERCORSO_E_SOLUZIONI::ReStoreFrom()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: " VRS(sizeof(THIS))+" Pointer = ",From.Pointer);
   #endif
   Percorso = new MM_I_PERCORSO;
   Soluzioni= new MM_ELENCO_SOLUZIONE;
   Percorso->ReStoreFrom(From);
   USHORT DimSol;
   From.ReStore(DimSol );
   for(int i = 0; i < DimSol; i++){
      (*Soluzioni) += new MM_SOLUZIONE;
      (*Soluzioni)[i]->ReStoreFrom(From);
   }
   #ifdef DBGCACHE
   TRACESTRING("Fine: Nø Soluzioni = "+STRINGA(Soluzioni->Dim())+" Pointer = "+STRINGA(From.Pointer));
   #endif
};
void MM_ELENCO_PERCORSO_E_SOLUZIONI::StoreTo(BUFR & To, ELENCO & Instradamenti){
   #undef TRCRTN
   #define TRCRTN "MM_ELENCO_PERCORSO_E_SOLUZIONI::StoreTo()"
   
   #ifdef DBGCACHE
   TRACELONG("Inizio: DimBuf = ",To.Dim());
   #endif
   // To.Store(NumSoluzioniAVideo);
   To.Store((USHORT)Dim());
   ORD_FORALL(THIS,i)THIS[i]->StoreTo(To,Instradamenti);
   #ifdef DBGCACHE
   Trace("Fine: DimBuf = "+STRINGA(To.Dim()));
   #endif
};
void MM_ELENCO_PERCORSO_E_SOLUZIONI::ReStoreFrom(BUFR & From){                    // Richiede che il grafo abbia gia' gli instradamenti
   #undef TRCRTN
   #define TRCRTN "MM_ELENCO_PERCORSO_E_SOLUZIONI::ReStoreFrom()"
   #ifdef DBGCACHE
   TRACELONG("Inizio: " VRS(sizeof(THIS))+" Pointer = ",From.Pointer);
   #endif
   // From.ReStore(NumSoluzioniAVideo);
   USHORT NumPs;
   From.ReStore(NumPs);
   for (int i = 0; i < NumPs; i++) {
      THIS += new MM_PERCORSO_E_SOLUZIONI();
      THIS.Last()->ReStoreFrom(From);
   } /* endfor */
   #ifdef DBGCACHE
   Trace("Fine: Pointer = "+STRINGA(From.Pointer));
   #endif
};
//----------------------------------------------------------------------------
// ~MM_ELENCO_SOLUZIONE
//----------------------------------------------------------------------------
MM_ELENCO_SOLUZIONE::~MM_ELENCO_SOLUZIONE(){
   #undef TRCRTN
   #define TRCRTN "~MM_ELENCO_SOLUZIONE"
   SCAN(THIS, PSol, P_MM_SO ){
      if(PSol)delete PSol;
   } ENDSCAN ;
};
//----------------------------------------------------------------------------
// INSTRPRE::Trace()
//----------------------------------------------------------------------------
void INSTRPRE::Trace(const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN "INSTRPRE::Trace"
   if(Livello > trchse)return;
   ERRSTRING(Messaggio);
   TRACEVLONGL(IdStazioneDestinazione,1);
   TRACEVLONGL(IdStazionePartenza    ,1);
   TRACEVLONGL(KmReali               ,1);
   TRACEVLONGL(KmAggiuntivi          ,1);
   TRACEVLONGL(KmMare                ,1);
   TRACEVLONGL(Ordine                ,1);
   ARRAY_ID Stz;
   for (int i = 0 ; i <  NumeroStazioniInstradamento(); i ++ ) {
      Stz += Stazioni[i];
   } /* endfor */
   ERRSTRING( "Stazioni: " + Stz.ToStringa() );
}
//----------------------------------------------------------------------------
// RELCUMUL::Trace()
//----------------------------------------------------------------------------
void RELCUMUL::Trace(const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN " "
   if(Livello > trchse)return;
   ERRSTRING(Messaggio);
   TRACEVLONGL(IdStazioneDestinazione   ,1);
   TRACEVLONGL(TipoRelazione            ,1);
   TRACEVLONGL(IdStazioneTransitoFS     ,1);
   TRACEVLONGL(CodiceCVB                ,1);
   TRACEVLONGL(KmConcessi1              ,1);
   TRACEVLONGL(CodConcessione1          ,1);
   TRACEVLONGL(CodLinea1                ,1);
   TRACEVLONGL(IdStazioneTransitoFS2    ,1);
   TRACEVLONGL(IdStazioneTransitoFS3    ,1);
   TRACEVLONGL(KmFs                     ,1);
   TRACEVLONGL(KmConcessi2              ,1);
   TRACEVLONGL(CodConcessione2          ,1);
   TRACEVLONGL(CodLinea2                ,1);
   TRACEVLONGL(CodiceTariffaRegionale   ,1);
};
void TRAVERSATA_MARE::Trace(const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN " "
   if(Livello > trchse)return;
   ERRSTRING(Messaggio);
   TRACEVLONGL(IdStazioneSbarco      ,1);
   TRACEVLONGL(IdStazioneImbarco     ,1);
   TRACEVLONGL(KmMare                ,1);
   TRACEVLONGL(CodSocietaMare        ,1);
   TRACEVLONGL(CodLineaMare          ,1);
};
