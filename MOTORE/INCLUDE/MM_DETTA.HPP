//========================================================================
// Dati di dettaglio: Note e servizi
//========================================================================
// 

#ifndef HO_MM_DETTA_HPP
#define HO_MM_DETTA_HPP


#ifndef HO_MOTGLUE_HPP
#include "MOTGLUE.HPP"
#endif


//----------------------------------------------------------------------------
// Dati delle note e nomi dei MV: acceduti solo per treni specifici
//----------------------------------------------------------------------------
// NB: Metto un solo record DETTAGLI per ogni MV
// Anche il nome del treno e' rappresentato come un dato ausiliario
//----------------------------------------------------------------------------
/*
        MM_DETMV.DB               MM_DETMV.EXT
        FILE_BS                   FILE_RO
     ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿       ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
     ³   DETTAGLI_MV  ³       ³ Pi— records per MV       ³
     ³                ³       ÃÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄ´
     ³                ÃÄÄÄÄÄÄ>´ Nota ³Nome   ³Nota  ³... ³
     ³                ³       ³      ³       ³Comm. ³    ³
     ³                ³       ³      ³       ³      ³    ³
     ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ       ÀÄÂÄÄÂÄÁÄÄÄÄÄÄÄÁÄÂÄÂÄÄÁÄÄÄÄÙ
                                ³  ³    ÚÄÄÄÄÄÄÙ ³
                                v  ÀÄÄÄÄ³ÄÄÄ>Ä¿  v
                  ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄ¿    ³  ÚÄÄÁÄÄÁÄÄÄÄÄÄÄÄ¿
                  ³ Periodicit…    Ã<ÄÄÄÙ  ³ Testi        ³
                  ³ Note e fermate ³       ³ Note         ³
                  ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ       ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
                       FILE_RO                 FILE_BS  +  FILE_RO
                       PERIODIC.SV1            MM_NOTE.DB  e MM_NOTE.EXT


   Le classifiche, gli Id ed i servizi dei treni viceversa vengono messi
   direttamente nei dati dei cluster.

   In particolare vi sono due aree con strutture a formattazione non
   fissa: la prima contiene gli identificativi dei treni (solo se pi— di
   uno) la seconda classifiche e servizi.

   Si noti infine per gli indici di periodicit… che il valore 0
   indica "tutti i giorni"

*/
//----------------------------------------------------------------------------

struct DETTAGLI_MV {  // 6 * 5000 ~ 30Kbytes

   WORD IdMezzoVirtuale            ; // Key
   BIT  DimDati                 :10; // Max 1k  bytes
   BIT  OffsetDati              :22; // Max 4 Mbytes

};

// Questo file contiene nomi dei treni e note
class F_DETTAGLI_MV : public FILE_BS { // MM_DETMV.DB

   public:

   // Costruttore
   F_DETTAGLI_MV(const char* NomeFile,ULONG Size=2048) : FILE_BS(NomeFile,sizeof(DETTAGLI_MV),2,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto
   BOOL Seek(WORD Mezzo_Virtuale){ return Posiziona(BUFR(&Mezzo_Virtuale,sizeof(Mezzo_Virtuale))) && KeyEsatta;};

   // Definizione operatori
   DETTAGLI_MV&  RecordCorrente(){ return *(DETTAGLI_MV*) RecordC; };
   DETTAGLI_MV&  FixRec(DWORD Indice){FILE_FIX::Posiziona(Indice) ;return *(DETTAGLI_MV*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0){ return CompareWord(Key1,Key2);};

};

enum TIPODATO {
   NOME_MEZZO_VIRTUALE,    // 21 Caratteri indicanti il nome del treno
   NOTA_MZV           ,    // Nota di uno o piu' mezzi viaggianti
   NOTA_FERMATA       ,    // Nota di fermata
   NOTA_COMPLESSA          // Nota complessa
};

// -------------------------------------------------------------
// I seguenti dati su MM_DETMV.EXT
// -------------------------------------------------------------
// NB: Posso avere piu' di un record di tipo Nome per lo stesso mezzo virtuale
struct NOME_MV  {   // Tipo = NOME_MEZZO_VIRTUALE
   BIT      Tipo                  : 3;
   BIT      NumBytes              : 5; // Numero Bytes totale della struttura (Max 30 bytes utili)
   BIT      Mvg                  :MVG; // Indica quali mezzi viaggianti posseggano effettivamente il nome
   char     Nome[1]                  ; // Fino a 21 Bytes (20 + NULL Terminator)
};

// Nota di fermata : Si applica alla fermata e per tutta le periodicit…
struct NOTA_FV  {   // Tipo = NOTA_FERMATA
   BIT      Tipo                  : 3;
   BIT      Evidenza              : 1; // Nota da mettere in evidenza
   BIT      DueStaz               : 1; // Valida solo se presente sia alla stazione di partenza che di arrivo
   BIT      IdNota                :12; // Numero della nota
   BIT      PrgFermata            : 8; // Progressivo della fermata  nel MEZZO VIRTUALE !
};                          //     25 = 4 bytes

// Nota di mezzo viaggiante
struct NOTA_MV      {   // Tipo = NOTA_MZV
   BIT      Tipo                  : 3;
   BIT      Mvg                  :MVG; // Indica quali mezzi viaggianti posseggano effettivamente la nota
   BIT      Evidenza              : 1; // Nota da mettere in evidenza
   BIT      DueStaz               : 1; // Valida solo se presente sia alla stazione di partenza che di arrivo
   BIT      IdNota                :12; // Numero della nota
   BIT      IdxPeriodicita        :12; // Indice periodicita' (sostituisce quello della nota) (0 = usa quella della nota)
};                          //     35 = 5 bytes

// Nota complessa: massima variabilit…
struct NOTA_COMPLEX {   // Tipo = NOTA_COMPLESSA
   BIT      Tipo                  : 3;
   BIT      Evidenza              : 1; // Nota da mettere in evidenza
   BIT      DueStaz               : 1; // Valida solo se presente sia alla stazione di partenza che di arrivo
   BIT      IdNota                :12; // Numero della nota
   BIT      TipoNota              : 3; // 0 = Per tutto il mezzo virtuale
                                       // 1 = Si effettua alla fermata indicata
                                       // 2 = Soppresso alla fermata indicata
                                       // 3 = Da [stazione0,stazione1] a stazione e tutte le intermedie
                                       // 4 = Da stazione a Stazione ma non le intermedie
   BIT      IdxPeriodicita        :12; // Indice periodicita' (sostituisce quello della nota)
   BYTE     ProgDa                   ; // Prog della prima  fermata utile in salita per usufruire del servizio (inclusa)
   BYTE     ProgMaxDa                ; // Prog dell' ultima fermata utile in salita per usufruire del servizio (inclusa)
   BYTE     ProgA                    ; // Prog della ultima fermata utile in discesa per usufruire del servizio
                                       // Tutti i progressivi sono nel MEZZO VIRTUALE !
};                          //     55 = 7 bytes
// NB : Tutti i tipi di nota possono derivare da INFOCOMM
// NB1: Il progressivo che indica la stazione e' quello presente nei dati del treno virtuale
//      Uso tale dato perche' normalmente piu' utile dell' ID
// NB2: La periodicita' e' riferita alla data di partenza DEL MEZZO VIRTUALE (non del MEZZO VIAGGIANTE)
// NB3: PrgFermata1 e' utilizzata solo per TipoNota = 3
//      PrgFermata2 e' utilizzata solo per TipoNota = 3,4

// Il file delle periodicita' e' semplicemente un' array di PERIODICITA, indirizzata da un
// numero progressivo.
// Non definisco pertanto alcuna struttura

// I testi delle note sono mantenuti in records variabili indicizzati dal file IDX_TESTI_NOTE
// I testi sono delle semplici stringhe NULL_TERMINATED.
// I files sono MM_NOTE.DB  e MM_NOTE.EXT
// NB: La periodicita' e' riferita alla data di partenza DEL MEZZO VIRTUALE (non del MEZZO VIAGGIANTE)
class  IDX_TESTI_NOTE : public FILE_BS  {
   public:

   // EMS001 VA R_STRU --> ITN_R_STRU in tutta la classe
   struct ITN_R_STRU {
      WORD IdNota            ; // KEY Id della nota
      BIT  Linguaggio     : 4; // KEY : Any = 0, Francese = 1, Inglese = 2, Italiano = 3, Spagnolo = 4, Tedesco = 5
      BIT  DimDati        :11; // Max 2k  bytes
      BIT  OffsetDati     :21; // Max 2 Mbytes
      BIT  IdxPeriodicita :12; // Indice periodicita' della nota
      // FLAGS_NOTA Flags       ;
   };

   // Costruttore
   IDX_TESTI_NOTE(const char* NomeFile,ULONG Size=2048) : FILE_BS(NomeFile,sizeof(ITN_R_STRU),3,Size,0,FALSE){Init();};

   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(ID Nota, BYTE Lingua){ BUFR Wrk; Wrk.Store(Nota); Wrk.Store(Lingua); return Posiziona(Wrk) && KeyEsatta; };

   // Definizione operatori
   ITN_R_STRU&  RecordCorrente(){ return *(ITN_R_STRU*) RecordC; };
   ITN_R_STRU&  FixRec(ULONG Indice){ FILE_FIX::Posiziona(Indice); return *(ITN_R_STRU*) RecordC; };

   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=2){
      ITN_R_STRU & K1 = *(ITN_R_STRU*)Key1;
      ITN_R_STRU & K2 = *(ITN_R_STRU*)Key2;
      int C1 = (int)K1.IdNota - (int)K2.IdNota;  // Confronto id delle note
      if(C1 != 0 || DimConfronto < 3)return C1;
      return (int)K1.Linguaggio - (int)K2.Linguaggio ;
   };

};
//----------------------------------------------------------------------------
// Dati dei servizi e Identificatori treni
//----------------------------------------------------------------------------
/*
   Gli identificatori dei treni sono immagazzinati direttamente entro i
   dati dei treni (struttura CLUSTRENO ) se tutti i treni del mezzo
   virtuale hanno lo stesso nome (a 5 caratteri) , altrimenti
   in VIAGG vi e' un puntamento ad un' array di 5 caratteri per ogni
   treno del mezzo virtuale.
   Questo approccio permette di avere una funzione che d… direttamente
   l' ID, il che Š pratico.

   Per i dati dei servizi la faccenda Š piu' complessa:
   In CLUSTRENO vi e' la variabile InfoUniforme che contiene i servizi
   uniformi del treno (cioe' quelli validi a prescindere dalla tratta o
   dalla data).

   Per identificare i servizi (e la classifica) date la tratta e la data
   si debbono aggiungere a tali servizi i servizi non uniformi (se ve ne
   sono):  essi sono contenuti nella locazione puntata da OffsetServizi,
   se InfoUniforme.Disuniforme e' vero (altrimenti non ve ne sono).

   Di seguito e' descritto il formato di immagazzinamento.
*/
//----------------------------------------------------------------------------
// NB: Le periodicita' sono riferite alla data di partenza DEL MEZZO VIRTUALE (non del MEZZO VIAGGIANTE)
//----------------------------------------------------------------------------
struct SRV_HDR {
   BIT  NumClassifiche    :3; // Se Set seguono tanti bytes quanti sono i mezzi componenti del virtuale, ognuno con una classifica
   BIT  GruppiFermata     :7; // Numero dei GRUPPI di servizi di fermata (uniformi per data)
   BIT  GruppiTreno       :7; // Numero dei GRUPPI di servizi di treno
   BIT  GruppiComplessi   :7; // Numero dei GRUPPI complessi
};

// Ogni gruppo e' costituito da una serie di servizi con le stesse caratteristiche
struct SERV     {
   BIT  Servizio          :5; // Numero del servizio
   BIT  Continua          :1; // Se set il gruppo continua con un altro servizio
};

// Gruppi di servizi di fermata
struct SRV_FERM {
   BYTE ProgFermata         ; // Progressivo della fermata nel MEZZO VIRTUALE !
   SERV Srv[1]              ; // Servizi (1 o pi—).
};

// Gruppi di servizi di treno
struct SRV_TRENO {
   BIT  Mvg             :MVG; // Indica quali mezzi viaggianti posseggano effettivamente i servizi
   BIT  IdxPeriodicita   :12; // Indice periodicita'
   SERV Srv[1]              ; // Servizi (1 o pi—).
};

struct SRV_CMPLX {
   BIT  Tipo              :3; // 0 = Per tutto il mezzo virtuale
                              // 1 = Si effettua alla fermata indicata
                              // 2 = Soppresso alla fermata indicata
                              // 3 = Da [stazione0,stazione1] a stazione e tutte le intermedie
                              // 4 = Da stazione a Stazione ma non le intermedie
   BIT  IdxPeriodicita   :12; // Indice periodicita'
   BYTE ProgDa              ; // Prog della prima  fermata utile in salita per usufruire del servizio (inclusa)
   BYTE ProgMaxDa           ; // Prog dell' ultima fermata utile in salita per usufruire del servizio (inclusa)
   BYTE ProgA               ; // Prog della ultima fermata utile in discesa per usufruire del servizio
                              // Tutti i progressivi sono nel MEZZO VIRTUALE !
   SERV Srv[1]              ; // Servizi (1 o pi—).
};
#endif
