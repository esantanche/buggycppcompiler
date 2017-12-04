//----------------------------------------------------------------------------
// PROVA.cpp V5
//    -- Programma determinazione percorsi piu' brevi tra due stazioni
//    -- Logica astratta ...
//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
// Se e' impostata la variabile di ambiente PROVA_BANALE
// allora espande le singole tratte del percorso
//----------------------------------------------------------------------------
// Per abilitare le cumulative:
// set PROVA_CUM=SI
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

typedef unsigned long BOOL;

#include "stdio.h"
#include "conio.h"
#include "dos.h"
#include <oggetto.h>
#include <scandir.h>
//#include <glue.h>
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

STAZIONI_ALL_PER_NOME *StAll;

void main(int argc,char * argv[]) {
   
   // Abilito il TRACE
   TRACEREGISTER2(NULL,"","PROVA.TRC");

	FILE_RW Out("\\D\\MOTORE\\PROVE\\PROVA.OUT");
	Out.SetSize(0);
	AddFileToBprintf(&Out);

	FILE * FOut = fopen("\\d\\motore\\prove\\prova.ou2","wt");

	//TRACEPRTY("Priorita'");
	Bprintf("Inizio PROVA.CPP tracelevel= %i\n",trchse);
   TRACEPARAMS(argc,argv);
   TRACESTRING("Inizio 1G"); // Imposta stampa compressa

	MM_RETE_FS & Rete = * MM_RETE_FS::CreaRete(PATH_DATI,PATH_OR1,PATH_OR2);

   TRACESTRING("Creata la RETE FS");
   
   STAZIONI Stazioni(PATH_DATI);
   
   ARRAY_ID Percorso;
   
   int AncheCum= 0;
   for (int i=1;i < argc ;i++ ) {
      if(argv[i][0] == '/'){
         if(argv[i][1] == 'I' || argv[i][1] == 'i') AncheCum = 0;
         if(argv[i][1] == 'C' || argv[i][1] == 'c') AncheCum = 1;
         if(argv[i][1] == 'X' || argv[i][1] == 'x') AncheCum = 99; // Solo cumulativo !
      } else {
         Percorso += atoi(argv[i]);
      };
   } /* endfor */

   BOOL Loop = Percorso.Dim() < 2;
   char sPercorso[250];
   do {
      if (Percorso.Dim() < 2) {
         Percorso.Clear();
         fprintf(stderr,"Inserire nuovo percorso o enter per terminare\n");
         gets(sPercorso);
			Bprintf("Percorso: ");
         if(sPercorso[0] == 'œ'){
            if(!StAll)StAll = new STAZIONI_ALL_PER_NOME(PATH_DATI);
				STRINGA Riga = (sPercorso+1);
				while (Riga.Last() == '\r' ||Riga.Last() == '\n') {
               Riga = Riga(0,Riga.Dim() -2);
            } /* endwhile */
            ELENCO_S Vals = Riga.Tokens("œ");
            BOOL OK = TRUE;
            ORD_FORALL(Vals,i){
               Vals[i].Strip();
               StAll->Posiziona(Vals[i]);
               int Id = StAll->RecordCorrente().IdStazione;
               if(!(Vals[i] == STRINGA(StAll->RecordCorrente().NomeStazione)(0,Vals[i].Dim()-1))){
                  Bprintf("\nNon identificata stazione '%s'\n",(CPSZ)Vals[i]);
                  Percorso.Clear();
						Percorso += 0;
                  break;
               }
               Percorso += Id;
               Bprintf(" %i",Id );
            }
         } else {
            ELENCO_S Ids =   STRINGA(sPercorso).Tokens(" ,;-");
				ORD_FORALL(Ids,i){
               if (Ids[i][0] == '/') {
                  if(Ids[i][1] == 'I' || Ids[i][1] == 'i') AncheCum = 0;
                  if(Ids[i][1] == 'C' || Ids[i][1] == 'c') AncheCum = 1;
                  if(Ids[i][1] == 'X' || Ids[i][1] == 'x') AncheCum = 99 ; // Solo cumulativo !
                  if(Ids[i][1] == 'Y' || Ids[i][1] == 'x') AncheCum = -98; // Solo FS no Tirrenia
					} else if (Ids[i].ToInt() > 0) {
                  Bprintf(" %i",Ids[i].ToInt() );
                  Percorso += Ids[i].ToInt() ;
					} /* endif */
				}
         } /* endif */
      } /* endif */
      if(Percorso.Dim() == 0)break;
      if(Percorso.Dim() <  2)continue;
      STRINGA Cumul; 
      switch (AncheCum) {
		case 0:
         Cumul = "- Solo servizio interno -";
         break;
      case 1:
         Cumul = "- Anche servizio cumulativo -";
         break;
      case 99:
         Cumul = "- Solo servizio cumulativo -";
        break;
      } /* endswitch */
      Bprintf("%s\n", (CPSZ)Cumul);
      TRACESTRING("==========================================");
      Percorso.Trace(Stazioni,STRINGA("Percorso Richiesto ")+ Cumul );
      TRACESTRING("==========================================");
      // MM_PERCORSO & Perc = *Rete.ViaggioLibero(Percorso,99);
      MM_PERCORSO & Perc = *Rete.ViaggioLibero(Percorso,AncheCum);
      if(&Perc == NULL){
         Bprintf("Percorso strutturalmente errato\n");
      } else {
			PERCORSO_POLIMETRICHE & Pp = *( PERCORSO_POLIMETRICHE *)Perc.Reserved;
         Bprintf("Percorso Grafo = %s\n",(CPSZ)Pp.Percorso.ToStringa());
         // Chiamo il printout del percorso
			Perc.PrintPercorso(FOut,Percorso);
			if (getenv("PROVA_BANALE")) {
				Bprintf("---------------> Esplosione delle tratte elementari\n");
            int KmBanali = 0;
            for (int p = 1; p < Percorso.Dim(); p ++ ) {
               ARRAY_ID Percorso2 ;
               Percorso2 += Percorso[p-1];
               Percorso2 += Percorso[p]  ;
               MM_PERCORSO & Perc2 = *Rete.ViaggioLibero(Percorso2);
               if(&Perc2){
						Perc2.ShortPrintPercorso(FOut);
                  KmBanali += Perc2.DatiTariffazione.KmReali;
                  delete &Perc2;
               }
            } /* endfor */
				Bprintf("<------------------ Km Banali : %i\n",KmBanali);
         } /* endif */
         delete &Perc;
//<<< if &Perc == NULL  
      }
      Percorso.Clear();
//<<< do  
   } while (argc < 3); /* enddo */
   
   puts("Fine");
   TRACESTRING("Chiusura normale Applicazione");
	TRACETERMINATE;
	fclose(FOut);
   exit(0);
//<<< void main int argc,char * argv     
}

