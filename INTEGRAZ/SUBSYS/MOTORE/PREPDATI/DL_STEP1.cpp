//----------------------------------------------------------------------------
// FILE DL_STEP1.CPP (ex DMEZVIR)
//----------------------------------------------------------------------------
/*
   Il file e' dericato da una rielaborazione e razionalizzazione del
   vecchio DMEZVIR

   Innanzitutto viene chiamata la routine CreaFamiglie per identificare
   le famiglie dei treni virtuali e scriverle sul file DMEZVIR.TMP .

   Poi si chiama ImpostaDMEZVIR che legge il file dei mezzi virtuali, lo
   ricodifica secondo lo standard HOST (routine ImpostaDMEZVIR ) e lo
   scrive sul file DMEZVIR.DAT .

*/
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2
#define PGM      "DL_STEP1"

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "DB_HOST.HPP"
#include "DISJ_SET.HPP"
#include "ML_BASE.HPP"

extern int OkPrintf;

//----------------------------------------------------------------------------
// PROTOTIPI
//----------------------------------------------------------------------------
int  CreaFamiglie();
int Elabora_DMEZVIR();
int ImpostaDMEZVIR( MEZZO_VIRTUALE & Mv , DMEZVIR & sDMEZVIR, F_FERMATE_VIRT & TabFv );
BOOL InitStazioni(const STRINGA& Path);

// ---------------------------------------------------------------------------
// VARIABILI GLOBALI
// ---------------------------------------------------------------------------
F_MEZZO_VIAGG * PMezziViaggianti;


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
   TryTime(0);
   Bprintf("Inizio Programma");
   
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
   
   F_MEZZO_VIAGG MezziViaggianti(PATH_OUT "MZVIAG.TMP");
   PMezziViaggianti = & MezziViaggianti ;
   
   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore

   InitStazioni(PATH_DATI);
   
   
   // ---------------------------------------------------------
   // Body
   // ---------------------------------------------------------
   
   TryTime(0);
   Bprintf("Creazione famiglie");
   Rc = CreaFamiglie();
   TryTime(0);
   Bprintf("Caricamento dati mezzi virtuali");
   if(Rc == 0)Rc = Elabora_DMEZVIR();
   
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TryTime(0);
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
   
//<<< int main int argc, char *argv     
}

//----------------------------------------------------------------------------
// Elabora_DMEZVIR
//----------------------------------------------------------------------------
int Elabora_DMEZVIR() {
   #undef TRCRTN
   #define TRCRTN "Elabora_DMEZVIR"
   
   FILE_MEZZO_VIRTUALE_APPO MezziVirtualiConFamiglia(PATH_OUT "DMEZVIR.TMP");
   FILE_DMEZVIR             FileDMEZVIR(PATH_OUT "DMEZVIR.DAT");
   FileDMEZVIR.Clear(NUSTR, FALSE);
   F_FERMATE_VIRT           TabFv(PATH_OUT "M0_FERMV.TM1");
   
   if( ! FileDMEZVIR.FileHandle() ) {
      BEEP;
      Bprintf("Errore: impossibile creare il file "PATH_OUT "DMEZVIR.DAT");
      return 1;
   }
   Bprintf("Numero mezzi virtuali da elaborare: %05d", MezziVirtualiConFamiglia.Dim());
   
   Bprintf2("-----------------------------------------------------" );
   Bprintf2("Scandisco il file e creo il file di output DMEZVIR");
   Bprintf2("-----------------------------------------------------" );
   ORD_FORALL(MezziVirtualiConFamiglia, i1){
      TryTime(i1);
      
      MEZZO_VIRTUALE_APPO & MV_Appo  = MezziVirtualiConFamiglia[i1];
      MEZZO_VIRTUALE & MezzoVirtuale = MV_Appo.sMezzoVirtuale;
      assert(MezzoVirtuale.NumeroFermateValide >= 2);
      
      DMEZVIR sDMEZVIR; // Record di Output di appoggio
      int Rc = ImpostaDMEZVIR( MezzoVirtuale, sDMEZVIR , TabFv);
      if(Rc) return Rc;
      
      // Questi dati sono le chiavi di accesso Host
      sprintf( sDMEZVIR.NUMVIR, "%05d", MV_Appo.iNumeroVirtuale );
      sprintf( sDMEZVIR.PRGVIR, "%02d", MV_Appo.iPrgVirtuale );
      
      FileDMEZVIR.AddRecordToEnd( VRB( sDMEZVIR ) );
      
   }
   
   return 0;
//<<< int Elabora_DMEZVIR    
}

//----------------------------------------------------------------------------
// ImpostaDMEZVIR
//----------------------------------------------------------------------------
// Imposta tutti i dati del mezzo virtuale
// In particolare:
// Controlla se ci sono prima e seconda classe e se sono uniformi
// Inoltre controlla se vi siano cambi di classifica
//----------------------------------------------------------------------------
int ImpostaDMEZVIR( MEZZO_VIRTUALE & MezzoVirtuale , DMEZVIR & sDMEZVIR, F_FERMATE_VIRT & TabFv){
   
   #undef TRCRTN
   #define TRCRTN "ImpostaDMEZVIR"
   
   BlankFill(sDMEZVIR);
   
   ByteCopy(sDMEZVIR.DATINIVAL, (CPSZ) T_PERIODICITA::Inizio_Orario_FS);
   ByteCopy(sDMEZVIR.DATFINVAL, (CPSZ) T_PERIODICITA::Fine_Orario_FS  );
   
   // Considero la prima classifica del mezzo virtuale come se fosse la classifica di tutto il mezzo
   // Utilizzo la classifica per identificare il tipo mezzo viaggiante
   const CLASSIFICA_TRENI & Classf = Classifica(MezzoVirtuale.Mv[0].TipoMezzo);
   if (Classf.Navale){
      sDMEZVIR.CODTIPMEZVGG = MCRC_TRAGHETTO;
   } else if (STRINGA("AUTOBUS") == STRINGA(Classf.Descrizione).UpCase()){
      sDMEZVIR.CODTIPMEZVGG = MCRC_AUTOBUS;
   } else {
      sDMEZVIR.CODTIPMEZVGG = MCRC_TRENO;
   }
   
   if(!TabFv.Seek(MezzoVirtuale.MezzoVirtuale)){
      Bprintf("Errore accedendo alle fermate del Mezzo virtuale %i Treno[0] = %s", MezzoVirtuale.MezzoVirtuale, St(MezzoVirtuale.Mv[0].IdentTreno) );
      return 99; // Per me questo e' un errore fatale, inutile continuare
   }
   
   // ????????????????????????????????????????????????????????????????????????????
   // NB: Non trovo corretto copiare solo il CCR senza il codice rete: da rivedere
   // ????????????????????????????????????????????????????????????????????????????
   // Identifico le stazioni di partenza ed arrivo del mezzo virtuale
   sprintf(sDMEZVIR.CODSTAINI, "%05d", TabFv.RecordCorrente().CCR );
   assert(TabFv.RecordCorrente().FermataPartenza); // La prima fermata deve essere in partenza
   int NumIntermedie = 0;
   while (&TabFv.RecordCorrente()) {
      FERMATE_VIRT & Fermata = TabFv.RecordCorrente();
      if(Fermata.MezzoVirtuale != MezzoVirtuale.MezzoVirtuale) break;
      if(Fermata.FermataArrivo) {
         NumIntermedie ++;
         sprintf(sDMEZVIR.CODSTAFIN, "%05d", Fermata.CCR );
      }
      TabFv.Next();
   } /* endwhile */
   NumIntermedie -= 2;
   

   // Controllo che la stazione di partenza ed arrivo del treno NON coincidano
   if( !memcmp(sDMEZVIR.CODSTAINI, sDMEZVIR.CODSTAFIN, sizeof(sDMEZVIR.CODSTAFIN) )){
      Stazioni.PosizionaPerCCR(It(sDMEZVIR.CODSTAINI));
      Bprintf2("Warning stessa stazione di partenza ed arrivo con %i Intermedie Mezzo virtuale %i Treno[0] = %s Stazione CCR %5.5s %s", 
          NumIntermedie, MezzoVirtuale.MezzoVirtuale, St(MezzoVirtuale.Mv[0].IdentTreno), sDMEZVIR.CODSTAINI,Stazioni.RecordCorrente().NomeStazione
      ); 
   }
   
   // ????????????????????????????????????????????????????????????????????????????
   // Adesso imposto la classifica PC, ma va visto il problema della classifica Host
   // ????????????????????????????????????????????????????????????????????????????
   sprintf( sDMEZVIR.CODCLS, "%02d", Classf.Classifica );
   sDMEZVIR.Q_M_1 = ' '; // Classifica Uniforme
   
   // .............................................................................
   // Ora debbo identificare la classe e correggere la classifica se non e' uniforme
   // .............................................................................
   
   F_MEZZO_VIAGG & MezziViaggianti = *PMezziViaggianti;
   int PrimaClasse=0, SecondaClasse=0 , ClasseDisuniforme=0;
   int Classif;
   
   // Per tutti i mezzi componenti del mezzo virtuale
   for (int i = 0; i  < MezzoVirtuale.NumMezziComponenti; i ++) {
      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE & Mvg = MezzoVirtuale.Mv[i];  // Mezzo viaggiante in esame
      
      // Accedo ai dati del mezzo viaggiante
      MEZZO_VIAGG &  MezzoViagg = MezziViaggianti[Mvg.NumeroMezzo-1];
      assert(&MezzoViagg != NULL);
      
      int Prima = 0, Seconda = 0        ; // Prima e seconda classe della singola tratta
      int PrimaUnif = 0, SecondaUnif = 0; // Prima ... "uniformi"
      
      // Si noti che l' attribuzione non e' del tutto esatta, visto che
      // i servizi dipendono dalla tratta. Tuttavia per l' uso che se ne
      // fa e' OK
      Prima |=  MezzoViagg.Servizi.PostiASederePrima ;
      Prima |=  MezzoViagg.Servizi.CuccettePrima     ;
      Prima |=  MezzoViagg.Servizi.VagoniLettoPrima  ;
      
      Seconda |=  MezzoViagg.Servizi.PostiASedereSeconda ;
      Seconda |=  MezzoViagg.Servizi.CuccetteSeconda     ;
      Seconda |=  MezzoViagg.Servizi.VagoniLettoSeconda  ;
      
      PrimaUnif |=  MezzoViagg.ServiziUniformi.PostiASederePrima ;
      PrimaUnif |=  MezzoViagg.ServiziUniformi.CuccettePrima     ;
      PrimaUnif |=  MezzoViagg.ServiziUniformi.VagoniLettoPrima  ;
      
      SecondaUnif |=  MezzoViagg.ServiziUniformi.PostiASedereSeconda ;
      SecondaUnif |=  MezzoViagg.ServiziUniformi.CuccetteSeconda     ;
      SecondaUnif |=  MezzoViagg.ServiziUniformi.VagoniLettoSeconda  ;
      
      ClasseDisuniforme |= i > 0 &&  Prima != PrimaClasse    ;
      ClasseDisuniforme |= i > 0 &&  Seconda != SecondaClasse;
      ClasseDisuniforme |= Prima   != PrimaUnif              ;
      ClasseDisuniforme |= Seconda != SecondaUnif            ;
      
      PrimaClasse   |= Prima;
      SecondaClasse |= Prima;
      
      // Esamino se ci sono cambi di Classifica
      if (i > 0 &&  MezzoViagg.TipoMezzo != Classif) {
         sDMEZVIR.Q_M_1 = '?';
         memcpy(sDMEZVIR.CODCLS, "  ", 2);
         break;
      }
      Classif =  MezzoViagg.TipoMezzo;
//<<< for  int i = 0; i  < MezzoVirtuale.NumMezziComponenti; i ++   
   }
   if (ClasseDisuniforme == 0) {
      if (PrimaClasse){
         if (SecondaClasse) {
            sDMEZVIR.CLA = CODCL_PRIMASEC ;
         } else {
            sDMEZVIR.CLA = CODCL_PRIMA    ;
         }
      } else {
         if (SecondaClasse) {
            sDMEZVIR.CLA = CODCL_SECONDA  ;
         } else { // Non imposto la classe
            sDMEZVIR.CLA = ' ';
            sDMEZVIR.Q_M_2 = '?';
         }
      }
   }
   
   // ??????????????????????????????????????
   // Non mi sembra sia OK: e' random
   // ??????????????????????????????????????
   // Imposto la priorit… dell' occupazione
   ByteCopy(sDMEZVIR.PRAOCC, sDMEZVIR.PRGVIR);
   
   // ??????????????????????????????????????
   // Non e' valorizzato INDTIPVGG
   // Non e' valorizzato INDGESIVX
   // Si vende sempre senza contingenti
   // ??????????????????????????????????????
   
   sDMEZVIR.CNG = '1'; // Vendita senza contingenti
   
   // Il campo con il codice di distribuzione non deve essere impostato
   ByteCopy(sDMEZVIR.CODDSZ, "NON_VALE");
   
   ByteCopy(sDMEZVIR.CR_LF, "\r\n");
   
   return 0;
//<<< int ImpostaDMEZVIR  MEZZO_VIRTUALE & MezzoVirtuale , DMEZVIR & sDMEZVIR, F_FERMATE_VIRT & TabFv  
}

//----------------------------------------------------------------------------
// CreaFamiglie
//----------------------------------------------------------------------------
/*
   Questa routine crea le famiglie dei mezzi virtuali.

   Il principio base Š che due mezzi virtuali che abbiano almeno un
   mezzo viaggiante in comune debbono appartenere alla stessa
   famiglia.

   Questa propriet… deve essere applicata ricorsivamente.

   L' algoritmo opera nel modo seguente:

   1) Si identifica un domino base di tanti elementi quanti sono i
      potenziali mezzi viaggianti.

   2) Si applica un algoritmo efficiente di partizione del dominio base
      in disjoint sets , in base alla seguenti regole:

      - Inizialmente non esiste alcun set

      - Ad ogni mezzo viaggiante che fa parte di un mezzo virtuale
         associo un elemento di un set (in pratica: tramite il numero di
         mezzo viaggiante)

      - Se due viaggianti fanno parte dello stesso virtuale allora i due
         elementi debbono ricadere nello stesso set.

      Per i dettagli dell' algoritmo si veda disj_set.hpp

   3) Si rinumera l' insieme dei set cos identificati
      A questo punto si ha che:

      - Ogni virtuale ha tutti i mezzi componenti cui corrispondono
         elementi appartenenti allo stesso set.

      - Il numero associato al set pu• essere utilizzato come indice
         della famiglia.

   4) Riscandisco i mezzi virtuali ed assegno un progressivo di virtuale
      gestendo un arrai di contatori (uno per famiglia).

   I mezzi virtuali che non hanno almeno due fermate vengono filtrati via

*/
//----------------------------------------------------------------------------
int  CreaFamiglie(){
   #undef TRCRTN
   #define TRCRTN "CreaFamiglie"
   F_MEZZO_VIAGG    MezziViaggianti(PATH_OUT "MZVIAG.TMP");
   F_MEZZO_VIRTUALE FileMezziV(PATH_OUT "M0_TRENV.TM1");    // apre file mezzi virtuali
   int DimensioneProblema = MezziViaggianti.Dim();
   
   Bprintf2("-----------------------------------------------------" );
   Bprintf2("Creazione delle famiglie, Dimensione del problema = %i",DimensioneProblema);
   Bprintf2("-----------------------------------------------------" );
   
   assert(DimensioneProblema < 0xffff);           //controlla se la dimensione del problema Š supportabile
   DISJOINT_SET   Set(DimensioneProblema);        //alloca il set da partizionare
   
   ORD_FORALL(FileMezziV,i){
      MEZZO_VIRTUALE &  Mvir=FileMezziV.FixRec(i);
      if(Mvir.NumeroFermateValide <2) continue; // Ignoro i mezzi virtuali con meno di due fermate

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
      if(Mvir.NumeroFermateValide <2) continue; // Ignoro i mezzi virtuali con meno di due fermate

      MEZZO_VIRTUALE::MEZZO_VIAGGIANTE & Base = Mvir.Mv[0];
      DISJOINT_SET::ELEMENT & El = *Set.Find(Base.NumeroMezzo);
      MEZZO_VIRTUALE_APPO Wrk;
      Wrk.sMezzoVirtuale   = Mvir                ;
      Wrk.iNumeroVirtuale  = El.Index            ; // Index viene utilizzato per valorizzare la famiglia
      Wrk.iPrgVirtuale     = ++ Array[El.Index]  ; // Progressivo nell' ambito della famiglia = lo valorizzo con il numero di mezzi virtuali che ho gi… trovato per la famiglia
      OutFil.AddRecordToEnd( &Wrk , sizeof(Wrk) );
   }
   
   delete [] Array;
   return 0;
//<<< int  CreaFamiglie   
}
