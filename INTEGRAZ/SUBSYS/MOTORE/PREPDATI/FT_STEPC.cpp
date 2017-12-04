//----------------------------------------------------------------------------
// FT_STEPC.CPP: Controllo dei files di input e risoluzione inconsistenze
//----------------------------------------------------------------------------
// Controlli effettuati:
//  Progressivi fuori ordine o non consecutivi
//  fermate corrispondenti a stazioni chiuse.
//  fermate in cui si parte prima di arrivare.
//  fermate in cui arriva prima di partire dalla stazione precedente
//  fermate con inconsistenze nel chilometraggio
//  fermate con ora arrivo/partenza vuote SOSPETTE
//  Tratte percorse a piu' di 350 Km/h
// I TRANSITI NON SONO CONSIDERATI
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3
#include "FT_PATHS.HPP"  // Path da utilizzare

// EMS001 Win
typedef unsigned long BOOL;

#include "oggetto.h"
#include "elenco.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ML_IN.HPP"
// EMS002 #include "eventi.h"
#include "seq_proc.hpp"

#define AGGIORNA  // Definisce se aggiornare il file

#define PGM      "FT_STEPC"

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
      // EMS003 Win SetStatoProcesso(PGM, SP_PARTITO);
      if (trchse > 2) trchse = 2; // Per evitare trace abissali involontari

      // EMS003 Win SetPriorita(); // Imposta la priorita'

      GESTIONE_ECCEZIONI_ON

      DosError(2); // Disabilita Popup di errore

      TryTime(0);
      CLASSIFICA_TRENI::CaricaClassifiche(PATH_DATI);

      printf("Controllo dei files prodotti dallo step 0\n");
      printf("I TRANSITI NON SONO CONSIDERATI\n");

      ELENCO_S StazioniChiuseConFermate ;

      FILE * Out;
      Out = fopen(PATH_OUT PGM ".OUT","w");    // EMS004 VA "wt" --> "w"

      // Apro l' archivio stazioni
      STAZIONI Stazioni(PATH_DATI,"ID_STAZI.DB",640000); // Lo metto tutto in memoria

      // Apro treni virtuali e relative fermate
      F_MEZZO_VIRTUALE TabTv(PATH_OUT "M0_TRENV.TM0");
      F_FERMATE_VIRT TabFv(PATH_OUT "M0_FERMV.TM0");
      TabTv.PermettiScrittura();
      TabFv.PermettiScrittura();

      // Variabili di statistica
      int NumProgressiviFuoriOrdine       = 0;
      int NumFermateChiuse                = 0;
      int NumTransitiChiusi               = 0;
      int NumTF_ArrivoDopoPartenza        = 0;
      int NumTF_ArrivoPrimaPartenzaPrecedente   = 0;
      int NumTF_KmInconsistenti           = 0;
      int NumTF_Ore0Sospette              = 0;
      int NumTF_TratteVeloci              = 0;

      int LastProg=0;
      int LastOraPartenza=0;
      int LastKm         =0;

      // ---------------------------------------------------------
      // Controllo tabelle
      // ---------------------------------------------------------
      TRACESTRING("Controllo tabelle");
      fprintf(Out,"=============================================================\n");
      fprintf(Out,"Controllo Tabelle\n");
      fprintf(Out,"=============================================================\n");
      fprintf(Out,
         "Controlli Effettuati: \n"
         "  1) Progressivi fuori ordine o non consecutivi                       \n"
         "  2) fermate corrispondenti a stazioni chiuse.                        \n"
         "  3) fermate in cui si parte prima di arrivare.                       \n"
         "  4) fermate in cui arriva prima di partire dalla stazione precedente \n"
         "  5) fermate con inconsistenze nel chilometraggio                     \n"
         "  6) fermate con ora arrivo/partenza vuote SOSPETTE                   \n"
         "  7) Tratte percorse a piu' di 350 Km/h                               \n"
      );
      fprintf(Out,"=============================================================\n");
      fprintf(Out,"I TRANSITI NON SONO CONSIDERATI\n");
      fprintf(Out,"=============================================================\n");
      printf("Controllo Tabelle\n");
      int Nr = TabFv.NumRecordsTotali();
      int LastMv = 0;
      for (int f = 0; f < Nr ; f++ ) {
         TryTime(f);
         FERMATE_VIRT Fermata = TabFv.FixRec(f); // Lavoro per copia per permettere l' accesso ad altri records
         // Mi posiziono alla stazione corrispondente
         STAZIONI::R_STRU & Stazione = Stazioni[Fermata.Id];


         // Ignoro i transiti
         if(Fermata.Transito ){ // Accetto la segnalazione del transito per stazioni chiuse
            // if(!Stazione.Aperta){
            if(!Stazione.Vendibile()){
               // TRACESTRING2(Buf,"Transito per stazione chiusa");
               NumTransitiChiusi ++;
            }
            continue;
         }


         char Buf[512]; Buf[0]=0;
         MM_INFO Info_Mezzo;
         ZeroFill(Info_Mezzo);
         // Macro gestione errori
         #define ERR TabTv.Seek(Fermata.MezzoVirtuale);                          \
         sprintf(Buf,"Fermata Nø %i M.V. %i Treno Nø %s Stazione %i '%s' ",      \
            Fermata.Progressivo,Fermata.MezzoVirtuale,                           \
            St(TabTv.RecordCorrente().Mv[Fermata.TrenoFisico].IdentTreno),       \
            Stazione.IdStazione,Stazione.NomeStazione);                          \
         Info_Mezzo.TipoMezzo = TabTv.RecordCorrente().TipoMezzoDominante



         //----------------------------------------------------------------------------
         // Controlli effettuati:
         //  Progressivi fuori ordine o non consecutivi
         //  fermate corrispondenti a stazioni chiuse.
         //  fermate in cui si parte prima di arrivare.
         //  fermate in cui arriva prima di partire dalla stazione precedente
         //  fermate con inconsistenze nel chilometraggio
         //  fermate con ora arrivo/partenza vuote SOSPETTE
         //  Tratte percorse a piu' di 350 Km/h
         //----------------------------------------------------------------------------
         if(Fermata.MezzoVirtuale != LastMv){ // Cambiato mezzo virtuale
            // TRACEVLONG(Fermata.MezzoVirtuale);
            // Inizializzazioni
            LastProg=0;
            LastOraPartenza=0;
            LastKm = 0;
         }

         int KmPercorsi       = Fermata.ProgKm - LastKm;
         int TempoPercorrenza = (Fermata.OraArrivo + 1440 - LastOraPartenza) % 1440;
         int VelocitaTratta;
         if(TempoPercorrenza ){ // I '+1' e '-1' per gli errori di arrotondamento
            VelocitaTratta   = (60 * (KmPercorsi-1)) / (TempoPercorrenza+1);
         } else {
            VelocitaTratta = 0;
         }
         // Limite di tempo oltre cui accetto il passaggio per la mezzanotte
         // Fino a 99 Km dalla precedente fermata: 1 ora e mezza
         // Per ogni 100 km o frazione in piu'   : 1 altra ora
         int TimeLimit = 1440 - ((1+ KmPercorsi/100) * 60) - 30;

         //  Progressivi fuori ordine o non consecutivi
         if(Fermata.Progressivo <=  LastProg){
            ERR ;
            TRACESTRING2(Buf,"Progressivi fuori ordine");
            fprintf(Out,"%s Progressivi fuori ordine: Da %i a %i\n",Buf, LastProg,Fermata.Progressivo);
            NumProgressiviFuoriOrdine ++;
         }

         //  fermate corrispondenti a stazioni chiuse.
         // if(!Stazione.Aperta){
         if(!Stazione.Vendibile()){
            ERR ;
            if(Fermata.Transito){ // Accetto la segnalazione del transito per stazioni chiuse
               // TRACESTRING2(Buf,"Transito per stazione chiusa");
               NumTransitiChiusi ++;
               // Le fermate di servizio sono gestite come transiti
               if(Fermata.FermataDiServizio){ // Accetto la segnalazione della fermata per motivi di servizio sulle stazioni chiuse
                  fprintf(Out,"Fermata per motivi di servizio a stazione chiusa: %s\n",Buf);
               }
            } else {
               TRACESTRING2(Buf,"Fermata su stazione chiusa: trasformata in transito");
               fprintf(Out,"%s Fermata su stazione chiusa: trasformata in transito\n",Buf);
               STRINGA Ferm = Stazione.NomeStazione;
               if(!StazioniChiuseConFermate.Contiene(Ferm)) StazioniChiuseConFermate += Ferm;
               Fermata.Transito         = 1;
               Fermata.FermataPartenza  = 0;
               Fermata.FermataArrivo    = 0;
               #ifdef AGGIORNA
               TabFv.FILE_FIX::ModifyRecord(f,BUFR(&Fermata,sizeof(Fermata)));

               if(TabTv.Seek(Fermata.MezzoVirtuale)){
                  TabTv.RecordCorrente().NumeroFermateValide --;
                  TabTv.ModifyCurrentRecord();
               } else {
                  TRACEINT("Errore ricerca Treno virtuale Nø ",Fermata.MezzoVirtuale);
               }
               #endif
               NumFermateChiuse ++;
//<<<       if Fermata.Transito   // Accetto la segnalazione del transito per stazioni chiuse
            }
//<<<    if !Stazione.Vendibile
         }

         //  fermate con piu' di 100 Km dalla precedente fermata
         //if(Fermata.ProgKm > LastKm + 100){
         //   ERR ;
         //   TRACESTRING2(Buf,"WARNING: Km > Km precedente fermata + 100");
         //   fprintf(Out,"%s WARNING: Km (%i) > Km precedente fermata (%i) + 100\n",Buf,Fermata.ProgKm,LastKm);
         //}

         //  fermate in cui arriva prima di partire dalla stazione precedente
         if (
            Fermata.OraArrivo                    &&
            Fermata.OraArrivo <  LastOraPartenza &&
            LastOraPartenza <  TimeLimit
         ) {
            ERR ;
            if(!Info_Mezzo.Navale()){ // Ignoro queste segnalazioni per i traghetti
               TRACESTRING2(Buf,"Ora arrivo "+STRINGA(ORA(Fermata.OraArrivo))+" <  Ora partenza precedente fermata "+STRINGA(ORA(LastOraPartenza))+".");
               fprintf(Out,"%s Ora arrivo %s <  Ora partenza precedente fermata %s\n",Buf,ORA(Fermata.OraArrivo),ORA(LastOraPartenza));
               // Fermata.OraArrivo = LastOraPartenza;
               #ifdef AGGIORNA
               // TabFv.FILE_FIX::ModifyRecord(f,BUFR(&Fermata,sizeof(Fermata)));
               #endif
               NumTF_ArrivoPrimaPartenzaPrecedente ++;
            }
         } else if ( //  fermate in cui si parte prima di arrivare.
            Fermata.OraArrivo                    &&
            Fermata.OraArrivo == LastOraPartenza &&
            KmPercorsi  > 2
         ) {
            ERR ;
            if(!Info_Mezzo.Navale()){ // Ignoro queste segnalazioni per i traghetti
               TRACESTRING2(Buf,"Ora arrivo "+STRINGA(ORA(Fermata.OraArrivo))+" == Ora partenza precedente fermata "+STRINGA(ORA(LastOraPartenza))+" Percorsi piu' di 2 Km.");
               fprintf(Out,"%s Ora arrivo %s == Partenza ultima fermata, Distanza %i Km\n",Buf,ORA(Fermata.OraArrivo),KmPercorsi);
               // Fermata.OraArrivo = LastOraPartenza;
               #ifdef AGGIORNA
               // TabFv.FILE_FIX::ModifyRecord(f,BUFR(&Fermata,sizeof(Fermata)));
               #endif
               NumTF_ArrivoPrimaPartenzaPrecedente ++;
            }
//<<<    if
         } else if ( //  fermate in cui si parte prima di arrivare.
            Fermata.OraArrivo                            &&
            Fermata.OraPartenza                          &&
            Fermata.OraArrivo > Fermata.OraPartenza      &&
            Fermata.OraArrivo < TimeLimit
         ) {
            ERR ;
            if(!Info_Mezzo.Navale()){ // Ignoro queste segnalazioni per i traghetti
               TRACESTRING2(Buf,"Ora arrivo > Ora partenza");
               fprintf(Out,"%s Ora arrivo %s > Ora partenza %s\n",Buf,ORA(Fermata.OraArrivo),ORA(Fermata.OraPartenza));
               // Fermata.OraPartenza = Fermata.OraArrivo;
               #ifdef AGGIORNA
               // TabFv.FILE_FIX::ModifyRecord(f,BUFR(&Fermata,sizeof(Fermata)));
               #endif
               NumTF_ArrivoDopoPartenza ++;
            }
         } else if ( VelocitaTratta > 350 ){
            ERR ;
            if(!Info_Mezzo.Navale()){ // Ignoro queste segnalazioni per i traghetti
               TRACESTRING2(Buf,"Tratta con velocita' > 350 Km/h");
               fprintf(Out,"%s Tratta con velocita' (%i) > 350 Km/h\n",Buf,VelocitaTratta);
               NumTF_TratteVeloci ++;
            }
//<<<    if
         } /* endif */

         //  fermate con inconsistenze nel chilometraggio
         if(Fermata.ProgKm < LastKm){
            ERR ;
            TRACESTRING2(Buf,"Km < Km precedente fermata: forzo Km = Km precedenti");
            fprintf(Out,"%s Km (%i) < Km precedente fermata (%i) : forzo Km = Km precedenti\n",Buf,Fermata.ProgKm,LastKm);
            Fermata.ProgKm = LastKm;
            #ifdef AGGIORNA
            TabFv.FILE_FIX::ModifyRecord(f,BUFR(&Fermata,sizeof(Fermata)));
            #endif
            NumTF_KmInconsistenti  ++;
         }


         //  fermate con ora arrivo/partenza vuote SOSPETTE
         if (
            Fermata.OraArrivo == 0   &&
            Fermata.Progressivo > 1  &&
            LastOraPartenza <  TimeLimit
         ) {
            ERR ;
            TRACESTRING2(Buf,"Sospetto arrivo a mezzanotte esatta");
            fprintf(Out,"%s Sospetto arrivo a mezzanotte esatta\n",Buf);
            NumTF_Ore0Sospette  ++;
         } else if (     // Non segnalo due volte sullo stesso record
            Fermata.OraPartenza == 0                  &&
            (f != Nr-1 && TabFv.FixRec(f+1).Progressivo > 1) &&
            LastOraPartenza <  23 * 60
         ) {
            ERR ;
            TRACESTRING2(Buf,"Sospetta partenza a mezzanotte esatta");
            fprintf(Out,"%s Sospetta partenza a mezzanotte esatta\n",Buf);
            NumTF_Ore0Sospette  ++;
         }

         // Salvo i dati per la prossima fermata
         LastProg         = Fermata.Progressivo  ;
         LastOraPartenza  = Fermata.OraPartenza  ;
         LastKm           = Fermata.ProgKm       ;
         LastMv           = Fermata.MezzoVirtuale;

//<<< for  int f = 0; f < Nr ; f++
      }

      #define See(_a) printf("%s = %i\n", #_a , _a); fprintf(Out,"%s = %i\n", #_a , _a);
      See(NumProgressiviFuoriOrdine          );
      See(NumFermateChiuse                   );
      See(NumTransitiChiusi                  );
      See(NumTF_ArrivoDopoPartenza           );
      See(NumTF_ArrivoPrimaPartenzaPrecedente);
      See(NumTF_KmInconsistenti              );
      See(NumTF_Ore0Sospette                 );
      See(NumTF_TratteVeloci                 );

      TryTime(0);

      if(StazioniChiuseConFermate.Dim()){
         StazioniChiuseConFermate.Sort();
         fprintf(Out,"===========================================\n");
         fprintf(Out,"Elenco Stazioni NON VENDIBILI cui risultano fermate (non di servizio):\n");
         ORD_FORALL(StazioniChiuseConFermate,i){
            fprintf(Out,"%s\n",(CPSZ)StazioniChiuseConFermate[i]);
         }
         fprintf(Out,"===========================================\n");
      } /* endif */

      TRACESTRING("PROGRAMMA TERMINATO");

      // ---------------------------------------------------------
      // Fine
      // ---------------------------------------------------------
      GESTIONE_ECCEZIONI_OFF
      TRACETERMINATE;

      fclose(Out);
   }
   exit(0);
//<<< void main int argc,char *argv
}

