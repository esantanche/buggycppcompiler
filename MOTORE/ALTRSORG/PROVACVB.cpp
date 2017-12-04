//----------------------------------------------------------------------------
// AD HOC PER PROVE CVB
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "stdio.h"
#include "conio.h"
#include "dos.h"
#include <oggetto.h>
#include <scandir.h>
#include <glue.h>
#include "id_stazi.hpp"
#include "MOTGLUE.HPP"
#include "I_MOTORE.hpp"
#include "alfa0.hpp"
#define PATH_IN    "\\FILE_T\\"
#define PATH_OR1   "\\FILE_T\\OUT\\"       // Orario estivo
#define PATH_OR2   "\\FILE_T2\\OUT\\"      // Orario Invernale
#define PATH_DATI  "\\DATIMOT\\"
#define PATH_DUMMY "\\TMP\\"

#undef TRCRTN
#define TRCRTN "Main"

char * DecodRicerca[4] = { "Minima","Media","TanteSol","CasiDifficili"};

#define TESTTRACE(a) _Abil2(a)  // trace  condizionato
SDATA data;
WORD Ora;
BOOL cercaTutto;
BOOL chiedi(ID & Da ,ID & A, MM_ELENCO_PERCORSO_E_SOLUZIONI & PercSol,MM_RETE_FS & Rete);
FILE * Prn;

STAZIONI_ALL_PER_NOME *StAll;

FILE * FIn;
FILE * FOut;

void PUTS(const char* Buf,BOOL Trc=TRUE){
   fprintf(FOut,"%s\n",Buf);
};


void main(int argc,char * argv[]) {
   
   
   FIn = stdin;
   FOut = stdout;
   // FIn  = fopen("SUITE1.IN","rt");
   // FOut = fopen("SUITE1.OUT","wt");
   
   // setbuf(stdout,NULL);  // Nel caso sia batch
   
   
   GESTIONE_ECCEZIONI_ON
   
   printf("Inizio PROVACVB.CPP tracelevel= %i\n",trchse);
   
   data.Anno  = 1995 ;
   data.Giorno=    8 ;
   data.Mese  =   11 ;
   
   Ora = 0;
   
   ID Da=0;
   ID A =0;
   
   
   ULONG TotTime=0;
   ULONG TotReq =0;
   
   TIPO_RICERCA Ricerca;
   Ricerca=Minima;
   // Ricerca=Media ;
   // Ricerca=TanteSol;
   // Ricerca=CasiDifficili;
   
   int NonPos = 0;
   
   MM_RETE_FS & Rete = * MM_RETE_FS::CreaRete(PATH_DATI,PATH_OR1,PATH_OR2);
   
   Rete.TipoRicerca = Ricerca;
   Rete.ScartaSoluzioniNonTariffabili = TRUE;
   
   MM_ELENCO_PERCORSO_E_SOLUZIONI * percSol =(MM_ELENCO_PERCORSO_E_SOLUZIONI*) NULL;
   
   // Apro l' archivio stazioni 
   TRACESTRING("Apro l' archivio stazioni");
   STAZIONI Stazioni(PATH_DATI);
   
   chiedi(Da,A,*percSol,Rete);
   
   
   char Buf[500];
   int NumProblema = 1;
   while (TRUE) {
      if(Da == 0 || A == 0)break;
      fprintf(stderr,"Problema Nø %i Da %i %s",NumProblema++,Da,Stazioni.DecodificaIdStazione(Da));
      fprintf(stderr," a %i %s\n",A,Stazioni.DecodificaIdStazione(A));
      ARRAY_ID StazioniDiCambio;
      
      percSol =(MM_ELENCO_PERCORSO_E_SOLUZIONI*) NULL;
      
      percSol = Rete.RichiestaPerRelazione(Da,A,PER_BIGLIETTARE_MA_USO_MOTORE_E_POLIMETRICHE,data,Ora);
      MM_ELENCO_PERCORSO_E_SOLUZIONI & PercSol = * percSol;
      if (!(&PercSol)) {
         sprintf(Buf,"Richiesta non Valida\n");
         PUTS(Buf);
      } else
      {
         ORD_FORALL(PercSol,is){
            MM_I_PERCORSO & Perc = * PercSol[is]->Percorso;
            printf("%s Km: FS %i Concessi %i Concessi2 %i Mare %i Tirrenia %i\n",
               (CPSZ)(Perc.FormaStandard.ToStringa())
              ,Perc.DatiTariffazione.KmReali
              ,Perc.DatiTariffazione.KmConcessi1
              ,Perc.DatiTariffazione.KmConcessi2
              ,Perc.DatiTariffazione.KmMare
              ,Perc.DatiTariffazione.Andata.KmMare
            );
         };
      }
      
      if (!chiedi(Da,A,PercSol,Rete)) break;
      delete &PercSol;
      (&PercSol)=NULL;
//<<< while  TRUE   
   } /* endwhile */
   
   fflush(stdout);
   
   delete &Rete;
   
   // Fine
   GESTIONE_ECCEZIONI_OFF
//<<< void main int argc,char * argv     
}

BOOL chiedi(ID& Da ,ID& A, MM_ELENCO_PERCORSO_E_SOLUZIONI & PercSol,MM_RETE_FS & Rete) {
   char uno[256];
   char due[256];
   do {
      fgets(uno,255,FIn);
      if (uno[0]=='D'||uno[0]=='d') {
         // fputs("Immettere data (GG MM AAAA)",FOut);
         int gg,mm,aaaa;
         fgets(due,255,FIn);
         sscanf(due,"%u %u %u",&gg,&mm,&aaaa);
         fprintf(stderr,"Data Immessa: %2.2i/%2.2i/%4.4i\n",gg,mm,aaaa);
         data.Giorno = gg;
         data.Mese   = mm;
         data.Anno   = aaaa;
      }
   } while (  uno[0] < '0' || uno[0] > '9' );
   STRINGA Linea(uno);
   Linea.Strip();
   ELENCO_S Toks = Linea.Tokens(" ,;");
   Da =  Toks[0].ToInt();
   TRACEVLONG(Da);
   A =  Toks[1].ToInt();
   TRACEVLONG(A);
   if (!A||!Da)return FALSE;
   return TRUE;
//<<< BOOL chiedi ID& Da ,ID& A, MM_ELENCO_PERCORSO_E_SOLUZIONI & PercSol,MM_RETE_FS & Rete   
}

