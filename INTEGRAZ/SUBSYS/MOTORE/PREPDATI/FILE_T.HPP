//----------------------------------------------------------------------------
// FILE_T.HPP : Contiene le strutture dei files T
//----------------------------------------------------------------------------

#ifndef HO_FILE_T_HPP
#define HO_FILE_T_HPP

// Questi per poter sempre definire i tipi di accesso ai dati
#include "BASE.HPP"
#include "FILE_RW.HPP"

#ifndef  HO__ML_BASE
#include "ML_BASE.HPP"
#endif

struct LOCALITA {  // Localita' associata ad una fermata ed informazioni relative
   IDTRENO  IdentTreno                    ; // Identificativo Treno 
   char     TestRecord                    ; // Se non e' A il record NON e' valido
   char     CodiceSuQuadro           [ 3] ; // Codice (all' interno del quadro) della localita'
   CCR      Codice                        ; // Codice CCR della localita'
   char     Descrizione              [20] ; // Descrizione della localita'
   char     KmDaOrigine              [ 4] ; // Numero Km da origine Treno
   union {
      struct{
         char     TrattaSecondaria        ; // * Se Set
         char     FermataAlternativa      ; // * Se Set
         char     TrattaDeviata           ; // * Se Set
         char     Panda                   ; // * Se Set
         char     Dogana                  ; // * Se Set
         };
      char  Flags                    [16] ;
   };                                
   NOTE     NoteDiLocalita           [ 5] ; // Note di Localita'
   #ifdef OLD_T_FORMAT // Vecchio formato dei files "T"
   char     Filler                        ;
   #endif
   char CR_LF[2];
};

// Questo per CATEGOR.T
struct CATEGORIE {
     char     Categoria                [15] ; // Categoria      Treno
     char     Codice                    [2] ; // Codice categoria: da 1 a 28. Attenzione al riutilizzo dei codici
     char     TempoRidottoCoincidenza       ; // * = Regionale, metropolitano ecc.
     char     Fumatori                      ; // * = Si                           
     char     Navale                        ; // * = Traghetto, monocarena ecc
     char     SicuramentePrenotabile        ; // * = Pendolini, IC/EC ...
     char     SicuramentePrenotazioneObbli  ; // * = Pendolini
     char     SupplementoIC                 ; // * IC
     char     SupplementoEC                 ; // * EC / EN ...
     char     SupplementoETR                ; // * ETR, Eurostar
     char     TipoConcesso                  ; // * = Treno di ferrovia concessa
     char     TipoInternazionale            ; // * = Treno di tipologia non italiana
      char CR_LF[2];
};
struct TABORARI {    // Corse, Treni, Fermate

   char TipoRecord;// Tipo del record

   union {

      char Record_Size [79];

      struct  {  // Record di tipo 2 (Identificativo Corsa)
         char     Filler2_1                [ 3] ;
         char     NumFermate               [ 3] ; // Numero Fermate e/o Transiti = N¯ records di tipo 5
         CCR      StazioneProvenienza           ; // Codice CCR stazione Origine del treno
         CCR      StazioneDestinazione          ; // Codice CCR stazione Destinazione del treno
         IDTRENO  IdentTrenoProvenienza         ; // Identificativo Treno Provenienza
         IDTRENO  IdentTrenoDestinazione        ; // Identificativo Treno Destinazione
      } R2 ;

      struct  {  // Record di tipo 3 (Descrittivo Treno)
         IDTRENO  IdentTreno                    ; // Identificativo Treno 
         char     Denominazione            [20] ; // Denominazione  Treno
         char     Categoria                [15] ; // Categoria      Treno
         char     KeyTreno                 [ 5] ; // Identificatorre Treno (univoco per i prenotabili)
         union {
            FLAGS_TRENO Flags;
            char     Flags_3               [ 9] ;
         };
         NOTE     NoteDiTreno              [ 5] ; // Note di treno
         char * Identificatore(){
            #ifdef OLD_T_FORMAT // Vecchio formato dei files "T"
            return IdentTreno;
            #else // Nuovo formato
            return KeyTreno;
            #endif
         }
      } R3;

      struct  {  // Record di tipo 4 (Periodicita' Treno) e 6 (Periodicita' Fermata)
         char     Progressivo              [ 2] ; // Progressivo del record di periodicita'
         PERIODIC Periodicita              [ 5] ; // Progressivo del record di periodicita'
      } R4;

      struct  {  // Record di tipo 5 (Fermata o transito)
         char     Localita                 [ 3] ; // Codice identificativo localita'
         ORARIO   Arrivo                        ; // Ora di arrivo
         ORARIO   Partenza                      ; // Ora di partenza
         union {
            struct {
               char     Transito                ; // * Se Set
               char     FermataPeriodica        ; // * Se Set
               char     FermataFacoltativa      ; // * Se Set
               char     FermataViaggInPartenza  ; // * Se Set
               char     FermataViaggInArrivo    ; // * Se Set
               char     FermataDiServizio       ; // * Se Set
               } Flags;
            char     Flags_5               [10] ;
         };
         NOTE     NoteDiFermata            [ 5] ; // Note di fermata
      } R5 ;

   };      // Fine Union

   char CR_LF[2];
};

class  FILE_LOCALITA : public FILE_FIX {
   public:
   FILE_LOCALITA(const STRINGA& NomeFile,ULONG BufSize=64000):
   FILE_FIX(NomeFile,sizeof(LOCALITA),BufSize){};
   LOCALITA &  operator [](ULONG Indice){ Posiziona(Indice); return *(LOCALITA*) RecordC; };
   LOCALITA &  RecordCorrente(){ return *(LOCALITA*) RecordC; };
};
class  FILE_TABORARI : public FILE_FIX {
   public:
   FILE_TABORARI(const STRINGA& NomeFile,ULONG BufSize=64000):
   FILE_FIX(NomeFile,sizeof(TABORARI),BufSize){};
   TABORARI &  operator [](ULONG Indice){ Posiziona(Indice); return *(TABORARI*) RecordC; };
   TABORARI &  RecordCorrente(){ return *(TABORARI*) RecordC; };
};

struct TEMPI_INTERSCAMBIO_1 {
   char Filler[3]         ;
   char TipoRecord        ;// Tipo del record
   char NumStazioni[2]    ;
   char Nome[80]          ;// Di solito e' piu' corto
};

struct TEMPI_INTERSCAMBIO_2 {
   char Acronimo[3]       ;// Acronimo della stazione
   char TipoRecord        ; // Tipo del record
   char CodStazione[5]    ; // Codice CCR
   char Denominazione[20] ; // Denominazione  stazione
   TEMPI_INTER TempiInterscambio[10]; // Tempi di interscambio
};

struct INFOCOMM {  // Informazioni Commerciali

   char TipoRecord;// Tipo del record

   union {

      char Record_Size [79];

      struct  {  // Record di tipo 1 (Identifica il treno)
         IDTRENO IdentTreno                   ; // Identificativo Treno 
         char    Categoria               [20] ; // Categoria      Treno
         CCR     StazioneProvenienza          ; // Codice CCR stazione Origine del treno      (NB: Ignorare)
         CCR     StazioneDestinazione         ; // Codice CCR stazione Destinazione del treno (NB: Ignorare)
      } R1;

      struct {
          NOTE    Nota                        ; // Codice della nota che contiene le informazioni testuali
          CCR     StazioneInizioServizio      ; // Codice CCR stazione DEL TRENO in cui ha inizio il servizio
          CCR     StazioneFineServizio        ; // Codice CCR stazione DEL TRENO in cui termina il servizio (0 se interessa la sola stazione di partenza)
          CCR     UltimaStazioneSalitaServ    ; // Codice CCR ultima stazione di salita da cui si puo' usufruire del servizio ( o 0 se il concetto non si applica)
          CCR     StazInizioServizioSeMV      ; // Su Mezzi virtuali con treni multipli: Stazione effettiva inizio servizio
          IDTRENO TrenoInizioServizioSeMV     ; // E relativo treno (ignorare i *INT , *EST)
          CCR     StazFineServizioSeMV        ; // Su Mezzi virtuali con treni multipli: Stazione effettiva fine servizio
          IDTRENO TrenoFineServizioSeMV       ; // E relativo treno (ignorare i *INT , *EST)
          char    ServizioDelTreno            ; // Il record attribuisce un servizio al treno
          char    ServizioDeiTreniProvenODest ; // Il records attribuisce un servizio ai treni provenienza e/o destinazione
          char    ServizioAttivoAFine         ; // Il servizio e' ancora attivo alla StazioneFineServizio
      } R2;

      // NB: Da prove effettuate con il programma PROVANOT ho visto che la periodicitÖ
      // ä relativa alla partenza del treno e NON alla stazione di inizio del servizo
      // In pratica se un treno che circola il lunedç parte da Milano alle 23, ed alle 6 
      // di mattina gli attaccano una carrozza a Roma, che porta con se una nota, se
      // sulla nota e' indicata una periodicitÖ essa ä relativa a Milano e quindi
      // sarÖ del tipo "circola il lunedç"
      struct  {  // Record di tipo 3 (Periodicita' )
         char     Progressivo              [ 2] ; // Progressivo del record di periodicita'
         PERIODIC Periodicita              [ 5] ; // Progressivo del record di periodicita'
      } R3;

   };  // Fine Union

   char CR_LF[2];

};

class  FILE_INFOCOMM : public FILE_FIX {
   public:
   FILE_INFOCOMM(const STRINGA& NomeFile,ULONG BufSize=64000):
   FILE_FIX(NomeFile,sizeof(INFOCOMM),BufSize){};
   INFOCOMM &  operator [](ULONG Indice){ Posiziona(Indice); return *(INFOCOMM*) RecordC; };
   INFOCOMM &  RecordCorrente(){ return *(INFOCOMM*) RecordC; };
};


struct NOTEPART {  // Note ( Commerciali e non)

   char TipoRecord;// Tipo del record
   NOTE   Nota                          ; // Codice della nota 

   union {

      char Record_Size [75];

      struct  {  // Record di tipo 1 (Identifica la nota)
         char   Filler                   [ 3] ;
         union {
            char     Flags_1 [70] ; // Flags della nota: 70 caratteri
            struct{  // Dettaglio dei flags
               char  FermataPerViaggiatoriInPartenza            ; // col.  9 * Se Set
               char  FermataFacoltativa                         ; // col. 10 * Se Set
               char  FermataFacoltativaPerViaggiatoriInPartenza ; // col. 11 * Se Set
               char  FermataFacoltativaPerViaggiatoriInArrivo   ; // col. 12 * Se Set
               char  FermataPerViaggiatoriInArrivo              ; // col. 13 * Se Set
               char  DoganaDiLocalita                           ; // col. 14 * Se Set
               char  ServizioDoganaInTreno                      ; // col. 15 * Se Set
               char  Panda                                      ; // col. 16 * Se Set
               char  FermataConServizioHandicap                 ; // col. 17 * Se Set
               char  FermataPeriodica                           ; // col. 18 * Se Set
               char  TrasportoAutoveicoli                       ; // col. 19 * Se Set
               char  TrasportoBiciclette                        ; // col. 20 * Se Set
               char  ServizioBuffet                             ; // col. 21 * Se Set
               char  ServizioRistoro                            ; // col. 22 * Se Set
               char  TrasportoCaniDiGrossaTaglia                ; // col. 23 * Se Set
               char  ServizioCuccetteNotte                      ; // col. 24 * Se Set
               char  ServizioCuccette                           ; // col. 25 * Se Set
               char  TrenoConServizioHandicap                   ; // col. 26 * Se Set
               char  Lettino                                    ; // col. 27 * Se Set
               char  Motrice                                    ; // col. 28 * Se Set
               char  Pendolino                                  ; // col. 29 * Se Set
               char  ServizioAutoSostitutivo                    ; // col. 30 * Se Set
               char  ServizioRistorante                         ; // col. 31 * Se Set
               char  PrenotazioneObbligatoria                   ; // col. 32 * Se Set
               char  ServizioSelfService                        ; // col. 33 * Se Set
               char  Sleeperette                                ; // col. 34 * Se Set
               char  NonFumatori                                ; // col. 35 * Se Set
               char  Traghetto                                  ; // col. 36 * Se Set
               char  VetturaDiretta                             ; // col. 37 * Se Set
            } Flags;
         };         
      } R1;

      struct {
         char   IdentificativoLingua            ; // Ignorare per ora
         char   Progressivo                [ 2] ; // Progressivo
         char   Testo                      [70] ; // Testo della nota (puo' stare su piu' righe)
      } R2;

      struct  {  // Record di tipo 3 (Periodicita' )
         char   Filler                          ;
         char     Progressivo              [ 2] ; // Progressivo del record di periodicita'
         PERIODIC Periodicita              [ 5] ; // Progressivo del record di periodicita'
      } R3;

   };  // Fine Union

   char CR_LF[2];
};

class  FILE_NOTEPART : public FILE_FIX {
   public:

   FILE_NOTEPART(const STRINGA& NomeFile,ULONG BufSize=64000):
   FILE_FIX(NomeFile,sizeof(NOTEPART),BufSize){};

   NOTEPART &  operator [](ULONG Indice){ Posiziona(Indice); return *(NOTEPART*) RecordC; };
   NOTEPART &  RecordCorrente(){ return *(NOTEPART*) RecordC; };

   // Questa funzione si posiziona sul file dato il codice della nota.
   // NB: Se la nota non esiste ritorna un NULL * (si deve testare) ed invalida
   // il buffer del file (anche RecordC e' NULL e puo' essere testato)
   // Sorgente su FT_LIB.CPP (sta su MOTO2.LIB)
   NOTEPART &  GetNota(USHORT IdNota);
};




struct CARROZZA {  // File carrozze

   IDTRENO  IdentTreno                    ; // Identificativo Treno 
   char     Segmento                      ; // Segmento di appartenenza
   char     Numero                   [ 4] ; // Numero della carrozza
   char     TipoRecord                    ; 
   char     ProgRecord               [ 2] ; // Progressivo del record
   PERIODIC Periodicita              [ 5] ; 
   char     Dummy                    [152-87]; 

   char CR_LF[2];

};

class  FILE_CARROZZE : public FILE_FIX {
   public:
   FILE_CARROZZE(const STRINGA& NomeFile,ULONG BufSize=64000):
   FILE_FIX(NomeFile,sizeof(CARROZZA),BufSize){};
   CARROZZA &  operator [](ULONG Indice){ Posiziona(Indice); return *(CARROZZA*) RecordC; };
   CARROZZA &  RecordCorrente(){ return *(CARROZZA*) RecordC; };
};



#endif


