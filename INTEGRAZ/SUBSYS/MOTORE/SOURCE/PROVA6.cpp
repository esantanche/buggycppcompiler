//----------------------------------------------------------------------------
// PROVA6.cpp
//    -- Programma determinazione percorsi piu' brevi tra due stazioni
//    -- Soluzioni in ordine di ora di partenza
//    -- Logica astratta ...
//----------------------------------------------------------------------------
// Gestione del trace
// #define TRACE_0  // Parte con il trace a 0
// Inoltre la variabile di environment TRACECLEAR cancella il trace ad ogni richiesta
//----------------------------------------------------------------------------
#define PER_CONFRONTI  // Taglia via tutte le informazioni soggette a cambiamento
// Da una versione all' altra del programma
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

#define MODULO_OS2_DIPENDENTE
#define INCL_DOSDATETIME
#define INCL_WINSHELLDATA   

#include "stdio.h"
#include "conio.h"
#include "dos.h"
#include <oggetto.h>
#include <scandir.h>
#include "id_stazi.hpp"
#include "MOTGLUE.HPP"
#include "I_MOTORE.hpp"
#include "alfa0.hpp"
#include "myalloc.h"
#define PATH_OR1   "\\FILE_T\\OUT\\"       // Orario estivo
#define PATH_OR2   "\\FILE_T2\\OUT\\"      // Orario Invernale
#define PATH_DATI  "\\DATIMOT\\"
STRINGA Path_Dati,Path_Estate,Path_Inverno;

// Per disabilitare i popup di errore
extern "C" {
   APIRET APIENTRY  DosError(ULONG error);
};

extern BOOL RiconciliazioneAbilitata ;

#undef TRCRTN
#define TRCRTN "Main"

char * DecodRicerca[4] = { "Minima","Media","TanteSol","CasiDifficili"};

#define TESTTRACE(a) _Abil2(a)  // trace  condizionato
SDATA data;
WORD Ora;
BOOL cercaTutto;
BOOL chiedi(ID & Da ,ID & A, MM_RETE_FS & Rete);
FILE * Prn;

int MM_sort_function1( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "MM_sort_function1()"
   MM_SOLUZIONE * A = *(MM_SOLUZIONE **) a;
   MM_SOLUZIONE * B = *(MM_SOLUZIONE **) b;
   // Criterio: e' il motore che ha impostato un livello di
   // desiderabilita' della soluzione nella variabile Ordine.
   return int(A->Ordine) - int(B->Ordine);
};
char * Get(char * NomeFileProfilo , char * Key1, char * Key2 );

// Flags globali
BOOL Print            = FALSE;
BOOL fTratta          = FALSE;
BOOL fPeriodicita     = FALSE;
BOOL Batch            = FALSE;
BOOL fDettagliTariffe = FALSE;
BOOL fNote            = FALSE;
BOOL fServizi         = FALSE;
BOOL MostraProfiler   = FALSE;

STAZIONI_ALL_PER_NOME *StAll;

FILE * FIn;
FILE * FOut;
ULONG TotReq =0;

void PUTS(const char* Buf,BOOL Trc=FALSE){
   fprintf(FOut,"%-4i| %s\n",TotReq,Buf);
   if(Print)fprintf(Prn,"%s\n",Buf);
   if(Trc)TRACESTRING(Buf);
};

char * Motivo[] = {
   "Ok                                                       ",
   "Data non in orario (o formato errato)                    ",
   "Ora errata (>= 1440)                                     ",
   "Stazione errata (non esistente o non vendibile)          ",
   "Stazione errata (non esistente o non vendibile)          ",
   "Relazione non accettabile (o non trovata nei precaricati)",
   "Stazione Da == Stazione A                                ",
   "Impostazioni generali errate                             "};

char * GG_SS[] = {"Luned","Marted","Mercoled","Gioved","Venerd","Sabato","Domenica"};

int  main(int argc,char * argv[]) {
   
   
   FIn = stdin;
   FOut = stdout;
   // FIn  = fopen("SUITE1.IN","rt");
   // FOut = fopen("SUITE1.OUT","wt");
   
   // setbuf(stdout,NULL);  // Nel caso sia batch
   
   // Abilito il TRACE
   TRACEREGISTER2(NULL,"","PROVA5.TRC");
   
   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore
   
   TRACEPRTY("Priorita'");
   DATETIME Now;
   DosGetDateTime(&Now);
   
   printf("Inizio PROVA6.CPP tracelevel= %i alle %2i:%2i:%2i del %2i/%2i/%4i \n",
      trchse,Now.hours,Now.minutes,Now.seconds,Now.day,Now.month,Now.year);
   
   TRACEPARAMS(argc,argv);
   TRACESTRING("Inizio 1G"); // Imposta stampa compressa
   
   DWORD Dummy; Cronometra(TRUE,Dummy);Cronometra(FALSE,Dummy); // Per calibrazione
   
   data.Anno  = 1997 ;
   data.Giorno=   10 ;
   data.Mese  =    4 ;
   
   Ora = 0;
   
   ID Da=0;
   ID A =0;
   
   char Buf[1024];
   
   ULONG TotTime=0;
   ULONG TotPerc=0;
   ULONG TotSol =0;
   
   TIPO_RICERCA Ricerca;
   Ricerca=Minima;
   // Ricerca=Media ;
   // Ricerca=TanteSol;
   // Ricerca=CasiDifficili;

   // Adesso identifico i paths da utilizzare 
   Path_Dati    = PATH_DATI;
   Path_Estate  = PATH_OR1;
   Path_Inverno = PATH_OR2;

   if(TestFileExistance("SIPAX.PRF")) {
      Path_Dati    = Get("SIPAX.PRF","IM_PUNTO_VENDITA>PUNTO_VENDITA;","DirDati");
      Path_Estate  = Get("SIPAX.PRF","IM_PUNTO_VENDITA>PUNTO_VENDITA;","DirOrario");
      Path_Inverno = Get("SIPAX.PRF","IM_PUNTO_VENDITA>PUNTO_VENDITA;","DirCambioOrario");

      if( Path_Dati    == NUSTR){ Bprintf("Errore: Path_Dati    Non e' definito sul file di profilo"); return 99;};
      if( Path_Estate  == NUSTR){ Bprintf("Errore: Path_Estate  Non e' definito sul file di profilo"); return 99;};
      if( Path_Inverno == NUSTR){ Bprintf("Errore: Path_Inverno Non e' definito sul file di profilo"); return 99;};

      if(Path_Dati.Last()    != '\\') Path_Dati   += '\\';
      if(Path_Estate.Last()  != '\\') Path_Estate += '\\';
      if(Path_Inverno.Last() != '\\') Path_Inverno+= '\\';
      Bprintf("Path_Dati    = %s",(CPSZ)Path_Dati    );
      Bprintf("Path_Estate  = %s",(CPSZ)Path_Estate  );
      Bprintf("Path_Inverno = %s",(CPSZ)Path_Inverno );
   } /* endif */
   
   MM_RETE_FS & Rete = * MM_RETE_FS::CreaRete(Path_Dati , Path_Estate, Path_Inverno);
   Rete.TipoRicerca = Ricerca;                
   
   MM_ELENCO_PERCORSO_E_SOLUZIONI * percSol =(MM_ELENCO_PERCORSO_E_SOLUZIONI*) NULL;
   
   // Apro l' archivio stazioni
   TRACESTRING("Apro l' archivio stazioni");
   STAZIONI Stazioni(PATH_DATI);
   
   // Metto il trace a 0 in modo da evitare trace abissali.
   // Puo' essere modificato a programma.
   #ifdef TRACE_0  // Parte con il trace a 0
   trchse = 0;
   #endif
   
   chiedi(Da,A,Rete);
   
   ULONG Tm = Time();
   Rete.SetOrarioDelGiorno(data); // Per una migliore valutazione dei tempi
   Tm = (Time()-Tm)*10;
   sprintf(Buf,"Tempo di caricamento iniziale dati orario: %i msec",Tm);
   #ifndef PER_CONFRONTI
   PUTS(Buf);
   #endif
   
   int NumProblema = 1;
   while (TRUE) {
      TotReq ++;
      if(NumProblema == 2)Mem_Reset();
      if(NumProblema > 1 && getenv("TRACECLEAR"))TRACECLEAR;
      if(Da == 0 || A == 0)break;
      TRACEVLONG(Da);
      TRACEVLONG(A);
      fprintf(stderr,"Problema Nø %i Da %i %s",NumProblema++,Da,Stazioni.DecodificaIdStazione(Da));
      fprintf(stderr," a %i %s\n",A,Stazioni.DecodificaIdStazione(A));
      
      TRACERESET;
      TRACESTRING("Inizio");
      ARRAY_ID StazioniDiCambio;
      
      percSol =(MM_ELENCO_PERCORSO_E_SOLUZIONI*) NULL;
      
      // Tm = Time();
      Cronometra(TRUE,Tm);
      PROFILER::Cronometra(11,TRUE);
      percSol = Rete.RichiestaPerRelazione(Da,A,PER_BIGLIETTARE_MA_USO_MOTORE_E_POLIMETRICHE,data,Ora);
      PROFILER::Cronometra(11,FALSE);
      // percSol = Rete.MezziDirettiPrenotabili(Da,A);
      // percSol = Rete.RichiestaPerRelazione(Da,A,PER_INFORMATIVA,data,Ora);
      // Tm = (Time()-Tm)*10;
      Cronometra(FALSE,Tm);
      Tm /= 1000;
      MM_ELENCO_PERCORSO_E_SOLUZIONI & PercSol = * percSol;
      if (!(&PercSol)) {
         sprintf(Buf,"Richiesta non Valida, Motivo = %s",Motivo[Rete.ReasonCode]);
         PUTS(Buf);
         TRACESTRING("=========================================");
      } else {
         PUTS("=====================================================================");
         sprintf(Buf,"Profondit… ricerca = %s Orario in uso: Da %s a %s"
           ,DecodRicerca[(int)Rete.TipoRicerca]
           ,(CPSZ)STRINGA(MM_RETE_FS::Rete->InizioValiditaOrario)
           ,(CPSZ)STRINGA(MM_RETE_FS::Rete->FineValiditaOrario)
         );
         PUTS(Buf);
         sprintf(Buf,"Data ricerca %s %s"  , GG_SS[data.GiornoDellaSettimana()],(CPSZ)STRINGA(data));
         PUTS(Buf);
         int Ofs = sprintf(Buf,"Relazione: %i -> %i : %s -> ",Da,A,Stazioni.DecodificaIdStazione(Da));
         sprintf(Buf+Ofs,"%s",Stazioni.DecodificaIdStazione(A));
         PUTS(Buf);
         int Num = 0;
         ORD_FORALL(PercSol,it)Num +=PercSol[it]->Soluzioni->Dim();
         sprintf(Buf,"Trovati %i percorsi e %i Soluzioni",PercSol.Dim(),Num);
         PUTS(Buf);
         TotPerc += PercSol.Dim(); TotSol += Num;
         sprintf(Buf,"Tempo di esecuzione %-5i msec",Tm);
         #ifndef PER_CONFRONTI
         PUTS(Buf);
         #endif
         TotTime += Tm;
         PUTS("=====================================================================");
         MM_ELENCO_SOLUZIONE Sols;
         ORD_FORALL(PercSol,is){
            MM_I_PERCORSO & Perc = * PercSol[is]->Percorso;
            sprintf(Buf,"Instradamento Nø %i %s ---->  DA %s a %s %i Km Via %s ",is, (CPSZ)Perc.DatiTariffazione.StatoInChiaro(),
               GRAFO::Gr()[Perc.FormaStandard[0]].Nome7(), GRAFO::Gr()[Perc.FormaStandard.Last()].Nome7(),
               Perc.DatiTariffazione.Lunghezza(), (CPSZ)Perc.FormaStandardStampabile );
            PUTS(Buf);
            if(fDettagliTariffe){
               printf ("DETTAGLI TARIFFE: \n");
               fflush(stdout);
               Perc.PrintPercorso(stdout);
               fflush(Prn);
               if(Print)Perc.PrintPercorso(Prn);
               // Sul trace anche i dati di tariffazione
               MM_SOLUZIONE * Sol = NULL;
               if( PercSol[is]->Soluzioni && PercSol[is]->Soluzioni->Dim()){
                  Sol = (*PercSol[is]->Soluzioni)[0];
                  DATI_TARIFFAZIONE_2 Dt = Sol->KmParziali(0,Sol->NumeroTratte - 1 ,&Perc);
                  // Fa da solo il trace
               }
            }
            if(PercSol[is]->Soluzioni && PercSol[is]->Soluzioni->Dim()){
               MM_ELENCO_SOLUZIONE & Soluzioni = * PercSol[is]->Soluzioni;
               ORD_FORALL(Soluzioni,so){
                  SOLUZIONE * Sol = (SOLUZIONE*) Soluzioni[so];
                  Sol->Percorso = is; // Punta al percorso
                  Soluzioni[so] = NULL;
                  // Inutile ricalcolarlo: ORA e' gia' cosi'
                  // Sol->Ordine = TempoTrascorso(Ora,Sol->OraPartenza());
                  Sols += Sol;
               };
            };
//<<<    ORD_FORALL PercSol,is  
         };
         if(Sols.Dim())qsort(&Sols[0],Sols.Dim(),sizeof(void*),MM_sort_function1);
         
         sprintf(Buf," ====> Soluzioni: ");
         PUTS(Buf);
         
         ORD_FORALL(Sols,i2){
            SOLUZIONE * Sol = (SOLUZIONE*) Sols[i2];
            MM_I_PERCORSO & Perc = * PercSol[Sol->Percorso]->Percorso;
            char * LastIdent = Sol->IdentPartenza;
            //sprintf(Buf,"  Soluzione ====== Parte %2i:%2i Arriva %2i:%2i  %s %s ======  Circola: %s Da %s A %s  Km %i (%i %i) Via %s",
            // .......  GRAFO::Gr().PercorsiGrafo[Sol->Percorso]->Len(), Sol->Km,
            sprintf(Buf,"  Soluzione ====== Parte %2i:%2i Arriva %2i:%2i %s %s ======  Circola: %s Da %s A %s  Km %i Via %s",
               Sol->OraPartenzaVera()/60, Sol->OraPartenzaVera()%60,
               Sol->OraArrivoVera()/60, Sol->OraArrivoVera()%60,
               Sol->AcronimoStazionePartenza, Sol->AcronimoStazioneArrivo  ,
               Sol->Per.Circolazione_STRINGA(),
               GRAFO::Gr()[Perc.FormaStandard[0]].Nome7(), GRAFO::Gr()[Perc.FormaStandard.Last()].Nome7(),
               Perc.DatiTariffazione.Lunghezza(),
               (CPSZ)Perc.FormaStandardStampabile );
            PUTS(Buf);
            // Questo per controllo incrociato tra Km grafo e soluzione
            if(getenv("MOSTRA_KM") && GRAFO::Gr().PercorsiGrafo.Dim() > Sol->Percorso) printf("  #### KmGrafo %i KmSoluzione %i\n", GRAFO::Gr().PercorsiGrafo[Sol->Percorso]->Len(), Sol->Km);
            
            STRINGA Ora1(ORA(Sol->TempoTotaleDiPercorrenza));
            STRINGA Ora2(ORA(Sol->TempoTotaleDiAttesa));
            STRINGA Clusters;
            for (int z = 0;z < Sol->NumeroTratte ;z++ ) {
               SOLUZIONE::TRATTA_TRENO & Tratta = (SOLUZIONE::TRATTA_TRENO &)Sol->Tratte[z];
               if(!Tratta.Concorde)Clusters += "#";
               Clusters += STRINGA(Tratta.IdCluster) + " ";
            };
            #ifdef PER_CONFRONTI
            sprintf(Buf,"Tempo Totale = %s Attesa = %s" ,(CPSZ)Ora1,(CPSZ)Ora2);
            #else
            sprintf(Buf,"Tempo Totale = %s Attesa = %s Clusters = %s",(CPSZ)Ora1,(CPSZ)Ora2,(CPSZ)Clusters);
            #endif
            PUTS(Buf);
            Sols[i2]->GetNote();
            for (int i = 0;i < Sol->NumeroTratte ;i++ ) {
               MM_SOLUZIONE::TRATTA_TRENO & Tratta = (MM_SOLUZIONE::TRATTA_TRENO &)Sol->Tratte[i];
               if(!StazioniDiCambio.Contiene(Tratta.IdStazioneIn))StazioniDiCambio += Tratta.IdStazioneIn;
               if(!StazioniDiCambio.Contiene(Tratta.IdStazioneOut))StazioniDiCambio += Tratta.IdStazioneOut;
               STRINGA Ora1(ORA(Tratta.OraIn));
               STRINGA Ora2(ORA(Tratta.OraOut));
               STRINGA NomeTreno=*Tratta.NomeSpecifico;
               #ifndef PER_CONFRONTI
               sprintf(Buf,"MV %i  Tratta %2i Treno %5s %s %s In %s %s  Out %s %s %s",Tratta.IdMezzoVirtuale
                  //sprintf(Buf,"  Tratta %2i Treno %5s %s %s In %s %s  Out %s %s %s"
                 ,i+1
                 ,Tratta.IdTreno
                 ,(CPSZ)NomeTreno
                 ,(CPSZ)MM_INFO::DecodTipoMezzo(Tratta.InfoTreno.TipoMezzo)
                 ,LastIdent
                 ,(CPSZ)Ora1
                 ,Tratta.IdentOut
                 ,(CPSZ)Ora2
                 ,(Tratta.InfoTreno.Prenotabilita ?  "Prenotabile" : "Non Prenotabile")
               );
               #else
               sprintf(Buf,"Tratta %2i Treno %5s %s %s In %s %s  Out %s %s %s"
                 ,i+1
                 ,Tratta.IdTreno
                 ,(CPSZ)NomeTreno
                 ,(CPSZ)MM_INFO::DecodTipoMezzo(Tratta.InfoTreno.TipoMezzo)
                 ,LastIdent
                 ,(CPSZ)Ora1
                 ,Tratta.IdentOut
                 ,(CPSZ)Ora2
                 ,(Tratta.InfoTreno.Prenotabilita ?  "Prenotabile" : "Non Prenotabile")
               );
               #endif
               PUTS(Buf);
               if (fTratta) {  // Dettagli di tratta
                  if (fServizi) {
                     char * Out = Buf;
                     Out += sprintf(Out,"Treno %s %s ",Tratta.IdTreno,(CPSZ)NomeTreno);
                     if(!Tratta.InfoTreno.EmptyServizi()){
                        sprintf(Out,"%s",(CPSZ)Tratta.InfoTreno.Decodifica(FALSE));
                        PUTS(Buf);
                     }
                  } /* endif */
                  //if (fPeriodicita) {
                  //   PUTS("Periodicita' di tratta:");
                  //   ELENCO_S Tmp = Tratta.Per.Decod(1,1,0);
                  //   ORD_FORALL(Tmp,i)PUTS((CPSZ)Tmp[i]);
                  //} /* endif */
               } /* endif */
               LastIdent = Tratta.IdentOut;
//<<<       for  int i = 0;i < Sol->NumeroTratte ;i++    
            } /* endfor */
            if (fServizi) {
               char * Out = Buf;
               Out += sprintf(Out,"Servizi complessivi: ");
               if(!Sol->InfoSoluzione.EmptyServizi()){
                  sprintf(Out,"%s",(CPSZ)Sol->InfoSoluzione.Decodifica(FALSE));
                  PUTS(Buf);
               }
            } /* endif */
            if (fPeriodicita) {
               PUTS("Periodicita' di soluzione:");
               ELENCO_S Tmp = Sol->Per.Decod(1,1,0);
               ORD_FORALL(Tmp,i)PUTS((CPSZ)Tmp[i]);
            } /* endif */
            if(fNote && Sol->InfoNote){
               ELENCO_S & Note = Sol->InfoNote->Note;
               TRACESTRING(VRS(Note.Dim()));
               ORD_FORALL(Note,i){
                  STRINGA Treni;
                  for (int t = 0;t < Sol->NumeroTratte ;t++ ) if(Sol->InfoNote->ApplicabileATratta(t,i))Treni += STRINGA('/')+STRINGA(Sol->Tratte[t].IdTreno).Strip();
                  if(Treni.Dim())Treni += "/ ";
                  if(i < Sol->InfoNote->NumNoteDiVendita){
                     sprintf(Buf,"==> %s%s",(CPSZ)Treni,(CPSZ)Note[i]);
                  } else {
                     sprintf(Buf,"... %s%s",(CPSZ)Treni,(CPSZ)Note[i]);
                  }
                  PUTS(Buf);
               }
               TRACESTRING("Done");
            }
//<<<    ORD_FORALL Sols,i2  
         };
         
         if(Rete.ReasonCode){
            sprintf(Buf,"??? Reason code richiesta non Zero, Motivo = %s",Motivo[Rete.ReasonCode]);
            PUTS(Buf);
         };
         
//<<< if  ! &PercSol    
      }
      TRACERESTORE;
      
      #ifndef PER_CONFRONTI
      sprintf(Buf,"Tempo di Esecuzione %i msec\n",Tm);
      PUTS(Buf);
      #endif
      
      ORD_FORALL(StazioniDiCambio,s){
         ID Id = StazioniDiCambio[s];
         sprintf(Buf,"Stazione %7s: Codice CCR %5i ID %4i : %s",
            GRAFO::Gr()[Id].Nome7(),Stazioni[Id].CodiceCCR,Id, Stazioni.RecordCorrente().NomeStazione);
         PUTS(Buf);
      };
      PUTS("");
      fflush(stdout);
      PROFILER::Trace(TRUE,1);
      if(MostraProfiler)PROFILER::PutStdOut(TRUE);
      PROFILER::Acquire(TRUE)         ; // Somma i dati di parziale in Totale; Opzionalmente azzera Parziale
      
      delete &PercSol;
      
      if (!chiedi(Da,A,Rete)) break;
      (&PercSol)=NULL;
//<<< while  TRUE   
   } /* endwhile */
   
   fflush(stdout);
   PROFILER::Trace(FALSE,1);
   if(MostraProfiler)PROFILER::PutStdOut(FALSE);
   
   // TRACESTRING("P0");
   // PROFILER::Cronometra(39,TRUE);
   // DosSleep(10000);
   // PROFILER::Cronometra(39,FALSE);
   // TRACESTRING("P1");
   // PROFILER::Trace(TRUE,1);
   // if(MostraProfiler)PROFILER::PutStdOut(TRUE);
   
   if (Batch) {
      sprintf(Buf,"Tempo di esecuzione Complessivo %i msec per %i richieste\n",TotTime,TotReq);
      PUTS(Buf);
      fprintf(stderr,"%s\n",Buf);
      sprintf(Buf,"Numero complessivo di percorsi %i e di soluzioni %i per %i richieste\n",TotPerc,TotSol,TotReq);
      PUTS(Buf);
      fprintf(stderr,"%s\n",Buf);
      if(TotReq> 5){
         sprintf(Buf,"Tempo medio di esecuzione %i msec \n",TotTime/TotReq);
         PUTS(Buf);
         fprintf(stderr,"%s\n",Buf);
      }
   } /* endif */
   
   delete &Rete;
   
   // Fine
   GESTIONE_ECCEZIONI_OFF
   TRACESTRING("Chiusura normale Applicazione");
   TRACETERMINATE;
   PUTS("Fine PROVA6.CPP");
   return 0;
//<<< int  main int argc,char * argv     
}

BOOL chiedi(ID& Da ,ID& A, MM_RETE_FS & Rete) {
   char uno[256];
   char due[256];
   fprintf(FOut,"      Profondit… ricerca = %s Orario in uso '%s -> %s ' Data ricerca %s Ora %i:%i"
     ,DecodRicerca[(int)Rete.TipoRicerca]
     ,(CPSZ)Rete.InizioValiditaOrario
     ,(CPSZ)Rete.FineValiditaOrario
     ,(CPSZ)STRINGA(data)
     ,Ora/60,Ora % 60 );
   if(Print)fprintf(FOut," Print ON");
   if(fPeriodicita)fprintf(FOut," Periodicita ON");
   if(fTratta)fprintf(FOut," Dett. Tratta ON");
   if(fDettagliTariffe)fprintf(FOut," Dett. Tariffe ON");
   if(fNote)fprintf(FOut," Note ON");
   if(fServizi)fprintf(FOut," Servizi ON");
   fprintf(FOut,"\n");
   do {
      if(!Batch){
         puts("Percorso? Inserire Da, A o ( D/O/P/T/H/M/L/X/R/G/N/S/$):");
         puts("   Data                  : Cambia la data in cui effettuare la ricerca");
         puts("   Ora                   : Cambia l' ora in cui effettuare la ricerca");
         puts("   Periodicita toggle    : Abilita / Disabilita i dettagli di periodicita'");
         puts("   Tratta toggle         : Abilita / Disabilita i dettagli di tratta");
         puts("   Hard Medium Low eXtra : Imposta la profondita' di ricerca");
         puts("   pRint toggle          : Abilita / Disabilita la stampa");
         puts("   dettaGli tariffe      : Abilita / Disabilita l' output dei dettagli tariffe");
         puts("   Note                  : Abilita / Disabilita l' output delle note");
         puts("   Servizi (Vuole Tratta): Abilita / Disabilita l' output dei servizi");
         puts("   $  Controllo trace    ");
         puts("   œ  Stazione per nome  Es: œCasarsa œAulla");
         puts("   õ  TreniAutoCheck (Nomi treni) ");
         puts("   #  PathAutoCheck (NOMI stazioni intermedie, NO origine e destinazione)");
         // puts("   @  Riconciliazione (0/1)");
         puts("Per terminare : inserire '0' e dare invio");
         puts("");
      }
      // while( kbhit()) getch(); // Vuota i caratteri presenti nel buffer di input
      do {
         fgets(uno,255,FIn);
         if(Batch)fputs(uno,FOut);
         TRACEVSTRING2(uno);
      } while (uno[0] == ';'  ); // Commento
      if (uno[0]=='R'||uno[0]=='r'){
         if(Prn == NULL){
            Prn = fopen("LPT1","wt");
            fprintf(Prn,"0");
         }
         Print = !Print;
      }
      if (uno[0]== 'P'||uno[0]== 'p') fPeriodicita = ! fPeriodicita;
      if (uno[0]== 'T'||uno[0]== 't') fTratta      = ! fTratta     ;
      if (uno[0]== 'G'||uno[0]== 'g') fDettagliTariffe = ! fDettagliTariffe ;
      if (uno[0]== 'N'||uno[0]== 'n') fNote = ! fNote;
      if (uno[0]== 'S'||uno[0]== 's') fServizi = ! fServizi;
      if (uno[0]== 'L'||uno[0]== 'l') Rete.TipoRicerca= Minima ;
      if (uno[0]== 'M'||uno[0]== 'm') Rete.TipoRicerca= Media  ;
      if (uno[0]== 'H'||uno[0]== 'h') Rete.TipoRicerca= CasiDifficili;
      if (uno[0]== 'X'||uno[0]== 'x') Rete.TipoRicerca= TanteSol;
      if (uno[0]== '!') MostraProfiler = !MostraProfiler;
      if (uno[0]== '$'){
         trchse =  uno[1] - '0';
         printf("Modificato tracelevel= %i\n",trchse);
      };
      if (uno[0]== '@'){
         RiconciliazioneAbilitata =  uno[1] != '0';
      };
      if (uno[0]== 'õ'){
         STRINGA Treni(uno + 1);
         while(Treni.Dim() && Treni.Last() == '\r' || Treni.Last() == '\n')Treni = Treni(0, Treni.Dim() -2);
         MM_RETE_FS::Rete->AttivaAutoDiagnostica(Treni);
      }
      if ((uno[0]=='B'||uno[0]=='b') && !Batch) {
         Batch = TRUE; // Modalita' batch
         fputs(uno,FOut);
         fputs("===> Abilitato BATCH MODE\n",FOut);
      };
      if (uno[0]=='D'||uno[0]=='d') {
         fputs("Immettere data (GG MM AAAA)",FOut);
         int gg,mm,aaaa;
         do {
            fgets(due,255,FIn);
            if(Batch)fputs(due,FOut);
            TRACEVSTRING2(due);
         } while (due[0] == ';' ); // Commento
         sscanf(due,"%u %u %u",&gg,&mm,&aaaa);
         fprintf(FOut,"Data Immessa: %2.2i/%2.2i/%4.4i\n",gg,mm,aaaa);
         data.Giorno = gg;
         data.Mese   = mm;
         data.Anno   = aaaa;
      }
      if (uno[0]=='O'||uno[0]=='o') {
         fputs("Immettere Ora  (HH MM)",FOut);
         int hh,mm ;
         do {
            fgets(due,255,FIn);
            if(Batch)fputs(due,FOut);
            TRACEVSTRING2(due);
         } while (due[0] == ';' ); // Commento
         sscanf(due,"%u %u ",&hh,&mm);
         fprintf(FOut,"Ora  Immessa: %2.2i:%2.2i\n",hh,mm);
         Ora = 60 * hh + mm;
      }
      if (uno[0]=='œ') {
         if(!StAll)StAll = new STAZIONI_ALL_PER_NOME(Path_Dati);
         STRINGA Riga(uno+1); Riga.Strip();
         while (Riga.Last() == '\r' ||Riga.Last() == '\n') {
            Riga = Riga(0,Riga.Dim() -2);
         } /* endwhile */
         ELENCO_S Vals = Riga.Tokens("œ");
         if(Vals.Dim() >= 2){
            Vals[0].Strip();
            Vals[1].Strip();
            StAll->Posiziona(Vals[0]);
            int Id1 = StAll->RecordCorrente().IdStazione;
            if(!(Vals[0] == STRINGA(StAll->RecordCorrente().NomeStazione)(0,Vals[0].Dim()-1))){
               fprintf(FOut,"Non identificata stazione '%s'\n",(CPSZ)Vals[0]);
               Id1 = 0;
            }
            StAll->Posiziona(Vals[1]);
            int Id2 = StAll->RecordCorrente().IdStazione;
            if(!(Vals[1] == STRINGA(StAll->RecordCorrente().NomeStazione)(0,Vals[1].Dim()-1))){
               fprintf(FOut,"Non identificata stazione '%s'\n",(CPSZ)Vals[1]);
               Id2 = 0;
            }
            if(Id1 > 0 && Id2 > 0 && Id1 != Id2){
               sprintf(uno,"%i %i",Id1,Id2);
            }
            
         }
//<<< if  uno 0 =='œ'   
      }
      if (uno[0]=='#') {
         ARRAY_ID Instr;
         if(!StAll)StAll = new STAZIONI_ALL_PER_NOME(Path_Dati);
         STRINGA Riga(uno+1); Riga.Strip();
         while (Riga.Last() == '\r' ||Riga.Last() == '\n') {
            Riga = Riga(0,Riga.Dim() -2);
         } /* endwhile */
         ELENCO_S Vals = Riga.Tokens("#");
         BOOL OK = TRUE;
         ORD_FORALL(Vals,i){
            Vals[i].Strip();
            StAll->Posiziona(Vals[i]);
            int Id = StAll->RecordCorrente().IdStazione;
            if(!(Vals[i] == STRINGA(StAll->RecordCorrente().NomeStazione)(0,Vals[i].Dim()-1))){
               fprintf(FOut,"Non identificata stazione '%s'\n",(CPSZ)Vals[i]);
               Instr.Clear();
               OK = FALSE;
               break;
            }
            Instr += Id;
         }
         if(OK) MM_RETE_FS::Rete->AttivaAutoDiagnosticaPath(Instr);
//<<< if  uno 0 =='œ'   
      }
//<<< do  
   } while (  uno[0] < '0' || uno[0] > '9' );
   fputs("",FOut);
   TRACESTRING("Ok Immessi i dati");
   TRACEVSTRING2(uno);
   STRINGA Linea(uno);
   Linea.Strip();
   ELENCO_S Toks = Linea.Tokens(" ,;");
   Da =  Toks[0].ToInt();
   TRACEVLONG(Da);
   A =  Toks[1].ToInt();
   TRACEVLONG(A);
   if (!A||!Da)return FALSE;
   return TRUE;
//<<< BOOL chiedi ID& Da ,ID& A, MM_RETE_FS & Rete   
}

char * Get(char * NomeFileProfilo , char * Key1, char * Key2 ){

// Acquisizione dell'hancor block del W.F. e lettura file profilo
   LHANDLE hPrf =   PrfOpenProfile(0,NomeFileProfilo);

// Se non ho potuto aprirlo return.
   if(!hPrf){
      printf("Errore aprendo il file di profilo %s\n",NomeFileProfilo);
      return NULL;
   };

   BOOL Success = FALSE;

   ULONG DataLen;        // Dimensione del Buffer
   char * Buffer;

// Richiesta dati di profilo
   Success = PrfQueryProfileSize( hPrf, Key1, Key2, &DataLen);
   if(!Success || DataLen == 0 || DataLen > 1000){
      printf("Errore accedendo al file di profilo %s\n",NomeFileProfilo);
      return NULL;
   };
   Buffer = (char *) malloc(DataLen+1);

   Success = PrfQueryProfileData( hPrf, Key1, Key2,Buffer,&DataLen);
   if(!Success || DataLen == 0 || DataLen > 1000){
      printf("Errore accedendo al file di profilo %s\n",NomeFileProfilo);
      return NULL;
   };

   PrfCloseProfile(hPrf);

   // printf("P1: %c\n",*Buffer);
   if(*Buffer != 'S') return NULL ; // Tag errato
   Buffer ++;
   short int Size = *(short int *)Buffer;
   Buffer += 2;
   if (memcmp(Buffer,"ALFANUMERICO",12) == 0) {
      Buffer += Size;
      // printf("P2: %c\n",*Buffer);
      if(*Buffer != 'S') return NULL ; // Tag errato
      Buffer ++;
      Size = *(short int *)Buffer;
      Buffer += 2;
      Buffer[Size] = 0;
   } else if (memcmp(Buffer,"NUMERICO",8) == 0) {
      Buffer += Size;
      //printf("P3: %c\n",*Buffer);
      if(*Buffer != 'S') return NULL ; // Tag errato
      Buffer ++;
      Size = *(short int *)Buffer;
      Buffer += 2;
      Buffer[Size] = 0;
   } else {
      //printf("P4:\n");
      Buffer = NULL;
   } /* endif */
   return Buffer;
}
