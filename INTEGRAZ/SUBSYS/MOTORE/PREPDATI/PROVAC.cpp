//----------------------------------------------------------------------------
// FILE DL_STEP1.CPP (ex DMEZVIR)
//----------------------------------------------------------------------------

/*      L'obiettivo di questa procedura Š quello di originare un file di uotput
        contenente le famiglie di treni virtuali con almeno due componenti.
        Successivamente si vuole scrivere su file di loggin l'elenco dei treni
        virtuali appartenenti ad una certa famiglia e dei suoi mezzi viaggianti

I passi da seguire sono i seguenti:
        1) Si legge dal file "DMEZVIR.TMP" generato dalla procedura Creafamiglie

        2) Scandendo tale file si pone ogni famiglia in una apposita struttura creata
           costruendo contemporaneamente la stringa contenente i mezzi viaggianti
           dei treni virtuali appartenenti alla famiglia in questione.

        3) Il risultato di questa operazione viene trascritto sul file "DMEZVIR2.TMP"

        4) Successivamente si da' in uscita una stampa che mostra per ogni famiglia
           i mezzi viaggianti di tale famiglia convertiti in (char*).
*/

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2
#define PGM      "PROVAB"

#include <DB_HOST.HPP>
#include <disj_set.hpp>
#include <string.h>

//----------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------


struct FAMIGLIA {                         //struttura di appoggio per la scrittura nel file di uscita
   MEZZO_VIRTUALE_APPO Wrk1;
   STRINGA MezziViaggianti;
}Famiglia,Fam;

int main(int argc, char *argv[]) {
   #undef TRCRTN
   #define TRCRTN "main"
   
   
   // ---------------------------------------------------------
   // Init
   // ---------------------------------------------------------
   int Rc = 0;
   
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   
   if(trchse > 2)trchse = 2; // Per evitare trace abissali involontari
   
   SetPriorita(); // Imposta la priorita'
   
   // Aggiungo il file di logging alle destinazioni di stampa
   FILE_RW Out(PATH_OUT PGM ".OUT");
   Out.SetSize(0);
   AddFileToBprintf(&Out);
   
   // Carico i limiti dell' orario fornito
   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   
   // Carico la decodifica delle classifiche
   CLASSIFICA_TRENI::CaricaClassifiche(PATH_DATI);
   
   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore
   
   TryTime(0);
   
   // ---------------------------------------------------------
   // Body
   // ---------------------------------------------------------
   
   
   
   F_MEZZO_VIRTUALE OutFil(PATH_OUT "DMEZVIR.TMP");                      //file di lettura
   FILE_MEZZO_VIRTUALE_APPO OutFil1(PATH_OUT "DMEZVIR2.TMP");            //file di scrittura
   OutFil1.Clear();
   
   
   ORD_FORALL(OutFil,i){
      Famiglia.Wrk1.sMezzoVirtuale = OutFil.FixRec(i);                               //scandisco riga per riga il file di lettura
      assert(Famiglia.Wrk1.iPrgVirtuale>1);                                          //testare se Famiglia ha pi— di un componente
      STRINGA S1;                                                              //crearsi una stringa di appoggio in cui sommare le stringhe dei mezzi viaggianti
      for (int j = 1 ; j < Famiglia.Wrk1.sMezzoVirtuale.NumMezziComponenti ; j++ ) {                                          //con un ciclo for scandirsi i mezzi viaggianti del mezzo virtuale e tradurli in stringa concatenandoli
         MEZZO_VIRTUALE::MEZZO_VIAGGIANTE & MViag = Famiglia.Wrk1.sMezzoVirtuale.Mv[j];
         char * Appo = new [sizeof(char *)];
         itoa(MViag,Appo,10);
         S1 += (STRINGA)Appo;                                                    //concatena le stringhe aggiungendo i blank
         S1 += " ";
         Famiglia.MezziViaggianti = S1;                                           //copiare in Famiglia.MezziViaggianti tale stringa di appoggio
         OutFil1.AddRecordToEnd( Famiglia.Wrk1.sMezzoVirtuale, sizeof(Famiglia.Wrk1.sMezzoVirtuale));                     //scrittura su file
      }   //endfor
   }
   
   ORD_FORALL(OutFil1,i1){
      Famiglia.Wrk1.sMezzoVirtuale = OutFil1.FixRec(i1);                                                                  //Apertura del file appena scritto e lettura riga per riga di questo file
      CPSZ f1 = (CPSZ) Famiglia.Wrk1.sMezzoVirtuale;
      CPSZ f2 = (CPSZ) S1;
      Bprintf2("treno virtuale %s, avente i treni viaggianti %s", f1, f2 );    //per ogni riga castare la stringa dei mezzi viaggianti mediante (CPSZ)
   }
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
   
//<<< int main int argc, char *argv     
}


