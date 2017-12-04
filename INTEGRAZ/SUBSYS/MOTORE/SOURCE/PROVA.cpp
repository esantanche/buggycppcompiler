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

void main(int argc,char * argv[]) {
   
   // Abilito il TRACE
   TRACEREGISTER2(NULL,"","PROVA.TRC");
   
   TRACEPRTY("Priorita'");
   printf("Inizio PROVA.CPP tracelevel= %i\n",trchse);
   TRACEPARAMS(argc,argv);
   TRACESTRING("Inizio 1G"); // Imposta stampa compressa
   
   MM_RETE_FS & Rete = * MM_RETE_FS::CreaRete(PATH_DATI,NUSTR,NUSTR);

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
         ELENCO_S Ids =   STRINGA(sPercorso).Tokens(" ,;-");
         printf("Percorso: ");
         ORD_FORALL(Ids,i){
            if (Ids[i][0] == '/') {
               if(Ids[i][1] == 'I' || Ids[i][1] == 'i') AncheCum = 0;
               if(Ids[i][1] == 'C' || Ids[i][1] == 'c') AncheCum = 1;
               if(Ids[i][1] == 'X' || Ids[i][1] == 'x') AncheCum = 99; // Solo cumulativo !
            } else if (Ids[i].ToInt() > 0) {
               printf(" %i",Ids[i].ToInt() );
               Percorso += Ids[i].ToInt() ;
            } /* endif */
         }
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
      printf("%s\n", (CPSZ)Cumul);
      TRACESTRING("==========================================");
      Percorso.Trace(Stazioni,STRINGA("Percorso Richiesto ")+ Cumul );
      TRACESTRING("==========================================");
      // MM_PERCORSO & Perc = *Rete.ViaggioLibero(Percorso,99);
      MM_PERCORSO & Perc = *Rete.ViaggioLibero(Percorso,AncheCum);
      if(&Perc == NULL){
         printf("Percorso strutturalmente errato\n");
      } else {
         PERCORSO_POLIMETRICHE & Pp = *( PERCORSO_POLIMETRICHE *)Perc.Reserved;
         printf("Percorso Grafo = %s\n",(CPSZ)Pp.Percorso.ToStringa());
         // Chiamo il printout del percorso
         Perc.PrintPercorso(stdout,Percorso);
         if (getenv("PROVA_BANALE")) {
            printf("---------------> Esplosione delle tratte elementari\n");
            int KmBanali = 0;
            for (int p = 1; p < Percorso.Dim(); p ++ ) {
               ARRAY_ID Percorso2 ;
               Percorso2 += Percorso[p-1];
               Percorso2 += Percorso[p]  ;
               MM_PERCORSO & Perc2 = *Rete.ViaggioLibero(Percorso2);
               if(&Perc2){
                  Perc2.ShortPrintPercorso(stdout);
                  KmBanali += Perc2.DatiTariffazione.KmReali;
                  delete &Perc2;
               }
            } /* endfor */
            printf("<------------------ Km Banali : %i\n",KmBanali);
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
   exit(0);
//<<< void main int argc,char * argv     
}

