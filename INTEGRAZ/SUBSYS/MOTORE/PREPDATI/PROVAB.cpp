//----------------------------------------------------------------------------
// FILE DL_STEP1.CPP (ex DMEZVIR)
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2
#define PGM      "PROVAB"

#include <DB_HOST.HPP>
#include <disj_set.hpp>

//----------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------
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
   F_MEZZO_VIAGG    MezziViaggianti(PATH_OUT "MZVIAG.TMP");             // apre file mezzi viaggianti
   F_MEZZO_VIRTUALE FileMezziV(PATH_OUT "M0_TRENV.TM1");                // apre file mezzi virtuali
   int DimensioneProblema = MezziViaggianti.Dim();
   
   assert(DimensioneProblema < 0xffff);                                 //controlla se la dimensione del problema Š supportabile
   DISJOINT_SET   Set(DimensioneProblema);                              //alloca il set da partizionare
   
   ORD_FORALL(FileMezziV,i){
      MEZZO_VIRTUALE &  Mvir=FileMezziV.FixRec(i);
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE & Base = Mvir.Mv[0];
      
      Set.Find(Base.NumeroMezzo);
      for (int j=1;j<Mvir.NumMezziComponenti ;j++ ) {
         MEZZO_VIRTUALE::MEZZO_VIAGGIANTE & MViag = Mvir.Mv[j];
         
         Set.Link(Base.NumeroMezzo , MViag.NumeroMezzo);
      } /* endfor */
   }
   
   
   int MassimoNumeroFamiglie = Set.ReNumber();
   
   int * Array= new int[MassimoNumeroFamiglie+1];
   memset(Array,0,sizeof(int)*(MassimoNumeroFamiglie+1));
   
   FILE_MEZZO_VIRTUALE_APPO OutFil(PATH_OUT "DMEZVIR.TMP");
   OutFil.Clear("File di appoggio MV/Famiglie");
   
   ORD_FORALL(FileMezziV,i1){
      MEZZO_VIRTUALE &  Mvir=FileMezziV.FixRec(i1);
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE & Base = Mvir.Mv[0];
      DISJOINT_SET::ELEMENT & El = *Set.Find(Base.NumeroMezzo);
      MEZZO_VIRTUALE_APPO Wrk;
      Wrk.sMezzoVirtuale   = Mvir                ;
      Wrk.iNumeroVirtuale  = El.Index            ; // Index viene utilizzato per valorizzare la famiglia
      Wrk.iPrgVirtuale     = ++ Array[El.Index]  ; // Progressivo nell' ambito della famiglia = lo valorizzo con il numero di mezzi virtuali che ho gi… trovato per la famiglia
      OutFil.AddRecordToEnd( &Wrk , sizeof(Wrk) );
   }

   delete [] Array;
   
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
   
//<<< int main int argc, char *argv     
}
