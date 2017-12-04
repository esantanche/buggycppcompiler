//----------------------------------------------------------------------------
// ML_BASE.HPP : Contiene delle definizione di base valide per il caricamento dati
//----------------------------------------------------------------------------
// Vi sono definiti: 
//  -  Tipi di dati varii
//  -  Classi ausiliarie per il caricamento dati 
//----------------------------------------------------------------------------

#ifndef  HO__ML_BASE
#define  HO__ML_BASE

//----------------------------------------------------------------------------
// Macro
//----------------------------------------------------------------------------
#define ByteCopy(_a,_b)    memmove(_a, _b, sizeof(_a))
#define ByteCmp(_a,_b)     memcmp(_a, _b, sizeof(_a))
#define ByteCopyR(_a,_b)   memmove(&_a, &_b, sizeof(_a))
#define BlankFill(_a)      memset(&_a,' ',sizeof(_a));
#define FillWithZeroes(_a) memset(&_a,'0',sizeof(_a));

//----------------------------------------------------------------------------
// Funzioni
//----------------------------------------------------------------------------
void SetPriorita(); // Imposta la priorita' (idle time)
// Per disabilitare i popup di errore
extern "C" {
   APIRET APIENTRY  DosError(ULONG error);
};
void TryTime(int i);
// Identifica le fermate che sono NOTORIAMENTE da ignorare
// In linea di massima sono i punti di fermata degli autobus.
// I codici dopo il 99000 sono codici fittizi o in attesa di attribuzione
// Il codice 99000 raccoglie le fermate estere
// I codici dopo 100000 sono codici internazionali
BOOL inline IgnoraFermata(ULONG CCR){return CCR >= 60000 ;};


//----------------------------------------------------------------------------
// Tipi usati dai files T (ADT)
//----------------------------------------------------------------------------
typedef char NOTE[4];
typedef char TEMPI_INTER[3];

struct  FLAGS_TRENO  {
   char     PrimaClasse             ; // * Se Set
   char     SecondaClasse           ; // * Se Set
   char     SupplementoRapido       ; // * Se Set
} ;

struct ORARIO  {
  char    Ora           [ 2] ;    // Ora
  char    Minuti        [ 2] ;    // Minuti
  WORD    MinMz() const ;   // Conversione in minuti dalla mezzanotte
};

#define NESSUNA_STAZIONE (*(struct CCR*)"0000000")
struct CCR {
   char CodiceRete[2];
   char CodiceCCR [5];

   int Cod()const { 
     // int Rete = It(CodiceRete); return (100000* Rete) + It(CodiceCCR);
     if(CodiceRete[0] == '8' && CodiceRete[1] == '3' ) return It(CodiceCCR);
     if(CodiceRete[0] == '0' && CodiceRete[1] == '0' ) return It(CodiceCCR);
     return 99000  ; // Illegale: Stazione straniera o non valida
   };
   int Cod2()const {  // Torna anche i codici illegali
     // int Rete = It(CodiceRete); return (100000* Rete) + It(CodiceCCR);
     if(CodiceRete[0] == '8' && CodiceRete[1] == '3' ) return It(CodiceCCR);
     return StringToInt( CodiceRete, sizeof(THIS) );
   };
   char * StrCod()const { 
     if(CodiceRete[0] == '8' && CodiceRete[1] == '3' ) return St(CodiceCCR);
     if(CodiceRete[0] == '0' && CodiceRete[1] == '0' ) return St(CodiceCCR);
     // return StringFrFix(CodiceRete, sizeof(THIS)); 
     return "0000000"; // Illegale: Stazione straniera
   };
   // Questo operatore fa una copia temporanea NULL TERMINATED e ritorna un puntatore.
   operator const char *()   const { return StringFrFix(CodiceRete, sizeof(THIS)); };
   operator STRINGA () const { return StringFrFix(CodiceRete, sizeof(THIS));};
   void Clear(){ memset(this,'0', sizeof(THIS));};
   BOOL Empty()const { return It(CodiceCCR) == 0;};
   BOOL operator == (const  CCR & b) const { return memcmp(this, &b, sizeof(CCR)) == 0 ;};
   BOOL operator != (const  CCR & b) const { return memcmp(this, &b, sizeof(CCR)) ;};
};
             
#define NESSUN_TRENO (*(struct IDTRENO*)"          ")
struct IDTRENO {
   char Id[10];
   // Questo operatore fa una copia temporanea NULL TERMINATED e ritorna un puntatore.
   operator const char *() const { return St(Id);};
   operator STRINGA () const { return St(Id);};
   char & operator[](int Idx){ return Id[Idx];};
   void Clear(){ memset(this,' ', sizeof(THIS));};
   BOOL operator == (const  IDTRENO & b) const { return memcmp(this, &b, sizeof(THIS)) == 0 ;};
   BOOL operator != (const  IDTRENO & b) const { return memcmp(this, &b, sizeof(THIS)) ;};
};

//----------------------------------------------------------------------------
// Classi di uso generale
//----------------------------------------------------------------------------
// Questa classe va bene per ricerche rapide in elenchi sortati di stringhe (che non cambiano al volo)
class BIN_ELENCO_S : public ELENCO_S {
   public:
  BOOL Bin_Contiene(const STRINGA & Target);
};

// Questa classe e' analoga ad MM_INFO, ma gestisce meglio servizi non uniformi
struct FULL_INFO : public MM_INFO {
   BIT      TipoSRV               : 3; // 0 = Per tutto il mezzo viaggiante o virtuale, a seconda dei casi.
                                       // 1 = Si effettua alla fermata indicata
                                       // 2 = Soppresso alla fermata indicata
                                       // 3 = Da stazione (non oltre MaxDa) a stazione e tutte le intermedie
                                       // 4 = Da stazione a Stazione ma non le intermedie
   BIT      DiMezzoVirtuale       : 1; // 0 = Servizio di treno; 1 = Di mezzo virtuale
   BIT      InProsecuzione        : 1; // Indica che il servizio (di treno) e' in realta' parte di un servizio di mezzo virtuale
   T_PERIODICITA PeriodicitaServizi  ; // Periodicita' dei servizi 
      // NB: Da prove effettuate con il programma PROVANOT ho visto che la periodicitÖ
      // ä relativa alla partenza del treno e NON alla stazione di inizio del servizo
      // In pratica se un treno che circola il lunedç parte da Milano alle 23, ed alle 6 
      // di mattina gli attaccano una carrozza a Roma, che porta con se una nota, se
      // sulla nota e' indicata una periodicitÖ essa ä relativa a Milano e quindi
      // sarÖ del tipo "circola il lunedç"
   CCR      Da                       ; // Per TipoSRV = 1 / 2 e' la fermata di effettuazione / soppressione
   CCR      A                        ; // Solo per TipoSRV = 3,4
   CCR      MaxDa                    ; // Solo per TipoSRV = 3  : Ultima stazione in salita da cui si puo' usufruire del servizio

   // Questa funzione mi dice se il servizio e' uniforme.
   BOOL Uniforme(){ return TipoSRV == 0 && PeriodicitaServizi == T_PERIODICITA::InOrario ;};

   void Clear(){
      MM_INFO::Clear();
      TipoSRV == 0; 
      DiMezzoVirtuale = 0; 
      InProsecuzione  = 0; 
      PeriodicitaServizi= T_PERIODICITA::InOrario ; 
      BlankFill(Da); 
      BlankFill(MaxDa); 
      BlankFill(A); 
   };

   STRINGA ToStringa();
};

// Analogamente per le note
struct FULL_NOTA                  {
   WORD     CodiceNota               ; // Codice della nota
   BIT      TipoNOT               : 3; // 0 = Per tutto il mezzo viaggiante o virtuale, a seconda dei casi.
                                       // 1 = Si effettua alla fermata indicata
                                       // 2 = Soppresso alla fermata indicata
                                       // 3 = Da stazione (non oltre MaxDa) a stazione e tutte le intermedie
                                       // 4 = Da stazione a Stazione ma non le intermedie
   BIT      DiMezzoVirtuale       : 1; // 0 = Servizio di treno; 1 = Di mezzo virtuale
   BIT      InProsecuzione        : 1; // Indica che la nota e' in realta' parte di un mezzo virtuale
   T_PERIODICITA PeriodicitaNota     ; // Periodicita' della nota
      // NB: Da prove effettuate con il programma PROVANOT ho visto che la periodicitÖ
      // ä relativa alla partenza del treno e NON alla stazione di inizio del servizo
      // In pratica se un treno che circola il lunedç parte da Milano alle 23, ed alle 6 
      // di mattina gli attaccano una carrozza a Roma, che porta con se una nota, se
      // sulla nota e' indicata una periodicitÖ essa ä relativa a Milano e quindi
      // sarÖ del tipo "circola il lunedç"
   CCR      Da                       ; // Per TipoNOT = 1 / 2 e' la fermata di effettuazione / soppressione
   CCR      A                        ; // Solo per TipoNOT = 3,4
   CCR      MaxDa                    ; // Solo per TipoNOT = 3  : Ultima stazione in salita da cui si puo' usufruire del servizio

   // Questa funzione mi dice se la nota e' uniforme.
   BOOL Uniforme(){ return TipoNOT == 0 && PeriodicitaNota == T_PERIODICITA::InOrario ;};

   void Clear(){
      CodiceNota      = 0; 
      TipoNOT         = 0; 
      DiMezzoVirtuale = 0; 
      InProsecuzione  = 0; 
      PeriodicitaNota = T_PERIODICITA::InOrario ; 
      BlankFill(Da); 
      BlankFill(MaxDa); 
      BlankFill(A); 
   };
   STRINGA ToStringa();
};

#endif
