//----------------------------------------------------------------------------
// STRESS.cpp 
//    Lancia il motore in continuazione per provare a stressarlo
//    Scrive su STRESS.LOG le operazioni effettuate
//    Vuole i seguenti parametri: Giorno, mese, anno, ora,minuti, Livello (0 = normale ...) 
// NON ATTIVA IL TRACE
//----------------------------------------------------------------------------
#define DBG_RICONCILIAZIONE
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
#define MODULO_OS2_DIPENDENTE
#define INCL_DOSDATETIME

#include "id_stazi.hpp"
#include "I_MOTORE.hpp"
#include "alfa0.hpp"
#include "mm_grafo.hpp"
#define PATH_IN    "\\FILE_T\\"
#define PATH_OR1   "\\FILE_T\\OUT\\"       // Orario estivo
#define PATH_OR2   "\\FILE_T2\\OUT\\"      // Orario Invernale
#define PATH_DATI  "\\DATIMOT\\"
#define PATH_DUMMY "\\TMP\\"
extern BOOL RiconciliazioneNecessaria;
extern BOOL RiconciliazioneAbilitata ;

// Per disabilitare i popup di errore
extern "C" {
   APIRET APIENTRY  DosError(ULONG error);
   // Per modificare la priorita' 
   APIRET APIENTRY  DosSetPriority(ULONG scope, ULONG prtyclass, LONG delta, ULONG PorTid);
};

SDATA data;
WORD Ora;
TIPO_RICERCA Ricerca;

#undef TRCRTN
#define TRCRTN "Main"

int  main(int argc,char * argv[]) {
   
   puts("STRESS vuole i seguenti parametri: Giorno, mese, anno, ora,minuti, Livello (0 = normale ...) NumSkip NumDo TRACELEVEL");
   if(argc != 10){
     DosBeep(2000,1000);
     return 99;
   }
   data.Giorno = atoi(argv[1]);
   data.Mese   = atoi(argv[2]);
   data.Anno   = atoi(argv[3]);
   Ora         = 60 * atoi(argv[4]) + atoi(argv[5]);
   Ricerca     = (TIPO_RICERCA)atoi(argv[6]);
   ULONG NumSkip = atoi(argv[7]);
   ULONG NumDo   = atoi(argv[8]);

   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore

   // Abilito il TRACE
   TRACEREGISTER2(NULL,"","STRESS.TRC");
   TRACEPARAMS(argc,argv);

   DosSetPriority(2,1, 16,(LONG)Tid());
   
   TRACEPRTY("Priorita'");
   DATETIME Now;
   DosGetDateTime(&Now);
   
   printf("Inizio STRESS alle %2.2i:%2.2i:%2.2i del %2.2i/%2.2i/%4.4i \n\n", Now.hours,Now.minutes,Now.seconds,Now.day,Now.month,Now.year);
   printf("Ricerca per data  %2.2i/%2.2i/%4.4i ora partenza %2.2i:%2.2i Livello= %i\n\n", data.Giorno,data.Mese,data.Anno,Ora/60,Ora%60,Ricerca);
   

   // Imposto il livello di trace 
   trchse = atoi(argv[9]);;
   int TRCHSE = trchse;
   
   ULONG TotTime=0;
   ULONG Tot100Time=0;
   ULONG TotPerc=0;
   ULONG TotSol =0;
   ULONG TotReq =0;
   
   MM_RETE_FS & Rete = * MM_RETE_FS::CreaRete(PATH_DATI,PATH_OR1,PATH_OR2);
   Rete.TipoRicerca = Ricerca;
   Rete.SetOrarioDelGiorno(data); // Per una migliore valutazione dei tempi
   // Accedo al grafo
   GRAFO & Grafo = GRAFO::Gr();

   int MaxStazione = Grafo.TotStazioniGrafo;

   DWORD RandA = 0xABCDEF01;
   DWORD RandB = 0xA2C7E105;
   DWORD RandC = 0xFFEE0055;
   MM_ELENCO_PERCORSO_E_SOLUZIONI * percSol =(MM_ELENCO_PERCORSO_E_SOLUZIONI*) NULL;
   
   
   int NumProblema = 0;
   FILE * Log = fopen("STRESS.LOG","wt");
   fprintf(Log, "Da:    A:    NPe NSo Errori note e discordanze   (Pn Km Senza Riconc. -> Km Con Riconc.)\n");

   int Fifo[100]; ZeroFill(Fifo);
   int Avg  = 0;
   int Avg2 = 0;
   ID Da,A,DaOld,AOld;
   BOOL UsataCache = TRUE;
   while (TRUE) {
      RandC = RandC * 7 + 0xA2A7E107;
      if (RandC % 4 == 0 ) { // Uso la cache (circa una volta ogni 4)
         Da = DaOld;
         A  = AOld ;
         UsataCache = TRUE;
      } else {
         RandA = RandA * 5 + 0xABCDEF01;
         RandB = RandB * 7 + 0xA2C7E105;
         Da = RandA % MaxStazione;
         A  = RandB % MaxStazione;
         if(UsataCache){
            DaOld = Da; 
            AOld = A;
            UsataCache = FALSE;
         }
      } /* endif */
      if(Da == A)continue;
      if(Da*A == 0)continue;
      if(!Grafo[Da].Vendibile)continue;
      if(!Grafo[A].Vendibile )continue;
      TotReq ++;
      if(TotReq < NumSkip)continue;
      if(TotReq >= NumSkip+NumDo)break;

      LastBeep.Clear();
      if(NumProblema == 1)Mem_Reset();

      fprintf(Log,"%5.5i->%5.5i",Da,A);
      fflush(Log);
      trchse = 1;
      ERRSTRING(STRINGA(Da)+STRINGA("=>")+STRINGA(A));
      trchse = TRCHSE;
      int Tm = TimePart();
      RiconciliazioneNecessaria = FALSE;
      RiconciliazioneAbilitata  = TRUE ;
      percSol =(MM_ELENCO_PERCORSO_E_SOLUZIONI*) NULL;
      percSol = Rete.RichiestaPerRelazione(Da,A,PER_INFORMATIVA,data,Ora);
      Tm = TimePart()-Tm;
      MM_ELENCO_PERCORSO_E_SOLUZIONI & PercSol = * percSol;
      int NumP = 0,NumS = 0;

      if (!(&PercSol)) {
         char * Motivo[] = {
            "Ok                                                       ",
            "Data non in orario (o formato errato)                    ",
            "Ora errata (>= 1440)                                     ",
            "Stazione errata (non esistente o non vendibile)          ",
            "Stazione errata (non esistente o non vendibile)          ",
            "Relazione non accettabile (o non trovata nei precaricati)",
            "Stazione Da == Stazione A                                ",
            "Impostazioni generali errate                             "};
         fprintf(Log," Err %s\n",Motivo[Rete.ReasonCode]);
         fflush(Log);
      } else {
         NumP = PercSol.Dim();
         if(NumP){ // Ho trovato qualcosa
            Tot100Time += Tm;
            Tot100Time -= Fifo[NumProblema % 100];
            Fifo[NumProblema % 100] = Tm;
            NumProblema ++;
            TotTime += Tm;
            TotPerc += PercSol.Dim(); TotSol += NumS;
            Avg  = Tot100Time / min(100,NumProblema);
            Avg2 = TotTime    / NumProblema;
         }
         ORD_FORALL(PercSol,it)NumS +=PercSol[it]->Soluzioni->Dim();
         fprintf(Log," %3.3i %3.3i",PercSol.Dim(),NumS);
         #ifdef DBG_RICONCILIAZIONE
         char Buffer[300];
         char * To = Buffer;
         BOOL Davvero = FALSE;
         if (RiconciliazioneNecessaria) {
            RiconciliazioneAbilitata  = FALSE ;
            ORD_FORALL(PercSol,is){
               MM_I_PERCORSO & Perc = * PercSol[is]->Percorso;
               int Km1 =  Perc.DatiTariffazione.Lunghezza();
               PERCORSO_POLIMETRICHE & Pp = *( PERCORSO_POLIMETRICHE *)Perc.Reserved;
               Pp.Stato = PERCORSO_POLIMETRICHE::NON_VALORIZZATO;
               PERCORSO_GRAFO Pg(Pp.Percorso);
               int Lg = Pg.Len();
               Pp.Valorizza(Pg);
               int Km2 =  Pp.DatiTariffazione.Lunghezza();
               if(Km1 != Km2){
                  To += sprintf(To," P%i %i->%i (Grf %i)",is,Km2,Km1,Lg);
                  Davvero = TRUE;
               }
            }
         } /* endif */
         if(Davvero){
            fprintf(Log," %s->",Stazioni[Da].NomeStazione);
            fprintf(Log,"%s %s",Stazioni[A].NomeStazione,Buffer);
         }
         #endif
         if(LastBeep.Dim())fprintf(Log," BEEP %s",(CPSZ)LastBeep); 
         fprintf(Log,"\n");
         fflush(Log);
      }
      
      printf("\r%i %5.5i -> %5.5i %3.3i Per %3.3i Sol %5.5i msec Avg(100)time= %2.2i.%2.2i Avg= %2.2i.%2.2i NumOk = %6i " 
        ,TotReq, Da,A,NumP,NumS, Tm * 10, Avg/100, Avg % 100, Avg2/100, Avg2 % 100, NumProblema);
      delete percSol;
      percSol=NULL;
   } /* endwhile */
   printf ("\n");
   
   delete &Rete;
   GESTIONE_ECCEZIONI_OFF
   TRACESTRING("Chiusura normale Applicazione");
   TRACETERMINATE;
   return 0;
}
