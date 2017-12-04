//========================================================================
// MM_PERIO.CPP : Classi per il caricamento iniziale dei files "t"
//========================================================================
//

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 2

#include "scandir.h"
#include "BASE.hpp"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"

//----------------------------------------------------------------------------
// Debug opzionale
//----------------------------------------------------------------------------
//#define DBGBASE       // Mostra le periodicita' base
//#define DBGCOMPONI    // Seguo algoritmo di composizione periodicita'
//#define DBGESPLODI    // Seguo algoritmo di esplosione periodicita'
//#define DBGESPLODI2   // Seguo algoritmo di esplosione periodicita' (nei dettagli)
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Variabili statiche di classe
//----------------------------------------------------------------------------
// Caricati da DATI_ORARIO_FS all' inizio dell' algoritmo
SDATA                   PERIODICITA::InizioOrario              ; // Inizio dati caricati
SDATA                   PERIODICITA::InizioOrarioFS            ; // Inizio validita' orario
SDATA                   PERIODICITA::FineOrarioFS              ; // Fine   validita' orario
SDATA                   PERIODICITA::DataC                     ; // Data Corrente
BYTE                    PERIODICITA::GiornoInizioOrario        ; // Lunedi' = 0, Martedi = 1 ....
WORD                    PERIODICITA::offsetPeriod              ; // Offset del periodo (giorni da inizio validita' orario)
T_PERIODICITA         T_PERIODICITA::InOrario                  ;  // Giorni dell' orario
T_PERIODICITA         T_PERIODICITA::InLimitiOrario            ;  // Giorni dell' orario
T_PERIODICITA         T_PERIODICITA::FuoriOrario               ;  // Giorni fuori dell' orario
static PERIODICITA                   GGSett[7]                 ; // Questi per la funzione Sett()
static PERIODICITA                   MaskOrario                ; // Questo maschera i giorni fuori dall' orario
static PERIODICITA                   MaskOrarioShift[5]        ; // Shiftati
ARRAY_T_PERIODICITA   PERIODICITA_IN_CHIARO::PeriodicitaBase   ;
T_PERIODICITA         PERIODICITA_IN_CHIARO::InSoppressione    ;
ELENCO_S              PERIODO_PERIODICITA::Decod_Tipo_Periodo(
   "tutti i giorni", "i giorni lavorativi", "i giorni festivi", "i giorni festivi ed il sabato",
   "i prefestivi","i postfestivi","gli scolastici","i non scolastici");
int                   T_PERIODICITA::ProgressivoFornitura      ;
STRINGA               T_PERIODICITA::NomeOrario                ;
SDATA                 T_PERIODICITA::Inizio_Orario_FS          ;
SDATA                 T_PERIODICITA::Fine_Orario_FS            ;
SDATA                 T_PERIODICITA::Inizio_AnnoScolastico     ;
SDATA                 T_PERIODICITA::Fine_AnnoScolastico       ;
SDATA                 T_PERIODICITA::Inizio_AnnoScolastico2    ;
SDATA                 T_PERIODICITA::Fine_AnnoScolastico2      ;
S_DATE                T_PERIODICITA::GiorniFestivi             ;
S_DATE                T_PERIODICITA::FestivitaSoloScolastiche  ;
ARRAY_T_PERIODICITA   T_PERIODICITA::PeriodicitaPerCodice      ;
ELENCO_S              T_PERIODICITA::DecodPeriodicitaPerCodice ;
SDATA                 T_PERIODICITA::Inizio_Dati_Forniti       ;
SDATA                 T_PERIODICITA::Fine_Dati_Forniti         ;
SDATA                 T_PERIODICITA::Inizio_Dati_Caricati      ;
SDATA                 T_PERIODICITA::Fine_Dati_Caricati        ;
SDATA                 T_PERIODICITA::Inizio_Ora_Legale         ;
SDATA                 T_PERIODICITA::Fine_Ora_Legale           ;
WORD                  T_PERIODICITA::Ora_Inizio_Ora_Legale     ;
WORD                  T_PERIODICITA::Ora_Fine_Ora_Legale       ;
static SDATA          MaxData                                  ;

static char limitiMese[12]={31,29,31,30,31,30,31,31,30,31,30,31};

struct AdHoc{ DWORD dw1; DWORD dw2; DWORD dw3; DWORD dw4; DWORD dw5; DWORD dw6; DWORD dw7; DWORD dw8;};

//----------------------------------------------------------------------------
// PERIODICITA
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// PERIODICITA::Set
//----------------------------------------------------------------------------
void PERIODICITA::Set(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::Set"
   AdHoc & A = *(AdHoc*)Dati;
   A.dw1 = 0xffffffff; A.dw2 = 0xffffffff; A.dw3 = 0xffffffff; A.dw4 = 0xffffffff;
   A.dw5 = 0xffffffff; A.dw6 = 0xffffffff; A.dw7 = 0xffffffff; A.dw8 = 0xffffffff;
};

//----------------------------------------------------------------------------
// PERIODICITA::ReSet
//----------------------------------------------------------------------------
void PERIODICITA::ReSet(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::ReSet"
   AdHoc & A = *(AdHoc*)Dati;
   A.dw1 = 0; A.dw2 = 0; A.dw3 = 0; A.dw4 = 0; A.dw5 = 0; A.dw6 = 0; A.dw7 = 0; A.dw8 = 0;
};

//----------------------------------------------------------------------------
// PERIODICITA::&=
//----------------------------------------------------------------------------
void PERIODICITA::operator &=(PERIODICITA & From){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::&="
   AdHoc & A = *(AdHoc*)Dati;
   AdHoc & B = *(AdHoc*)From.Dati;
   A.dw1 &= B.dw1; A.dw2 &= B.dw2; A.dw3 &= B.dw3; A.dw4 &= B.dw4;
   A.dw5 &= B.dw5; A.dw6 &= B.dw6; A.dw7 &= B.dw7; A.dw8 &= B.dw8;
} ;

//----------------------------------------------------------------------------
// PERIODICITA::|=
//----------------------------------------------------------------------------
void PERIODICITA::operator |=(PERIODICITA & From){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::|="
   AdHoc & A = *(AdHoc*)Dati;
   AdHoc & B = *(AdHoc*)From.Dati;
   A.dw1 |= B.dw1; A.dw2 |= B.dw2; A.dw3 |= B.dw3; A.dw4 |= B.dw4;
   A.dw5 |= B.dw5; A.dw6 |= B.dw6; A.dw7 |= B.dw7; A.dw8 |= B.dw8;
} ;
//----------------------------------------------------------------------------
// PERIODICITA::~
//----------------------------------------------------------------------------
void PERIODICITA::operator ~(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::~"
   AdHoc & A = *(AdHoc*)Dati;
   A.dw1 = ~ A.dw1; A.dw2 = ~ A.dw2; A.dw3 = ~ A.dw3; A.dw4 = ~ A.dw4;
   A.dw5 = ~ A.dw5; A.dw6 = ~ A.dw6; A.dw7 = ~ A.dw7; A.dw8 = ~ A.dw8;
}

//----------------------------------------------------------------------------
// PERIODICITA::==
//----------------------------------------------------------------------------
BOOL PERIODICITA::operator ==(const PERIODICITA & From)const {
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::=="
   // Questo operatore ha la caratteristica che il confronto si deve limitare
   // al periodo di validita' dell' orario (al di fuori non e' significativo)
   AdHoc & A = *(AdHoc*)Dati;
   AdHoc & B = *(AdHoc*)From.Dati;
   AdHoc & Mask = *(AdHoc*)MaskOrario.Dati;
   DWORD Accum,Out;
   Accum = A.dw1; Accum ^= B.dw1 ; Accum &= Mask.dw1; Out  = Accum;
   Accum = A.dw2; Accum ^= B.dw2 ; Accum &= Mask.dw2; Out |= Accum;
   Accum = A.dw3; Accum ^= B.dw3 ; Accum &= Mask.dw3; Out |= Accum;
   Accum = A.dw4; Accum ^= B.dw4 ; Accum &= Mask.dw4; Out |= Accum;
   Accum = A.dw5; Accum ^= B.dw5 ; Accum &= Mask.dw5; Out |= Accum;
   Accum = A.dw6; Accum ^= B.dw6 ; Accum &= Mask.dw6; Out |= Accum;
   Accum = A.dw7; Accum ^= B.dw7 ; Accum &= Mask.dw7; Out |= Accum;
   Accum = A.dw8; Accum ^= B.dw8 ; Accum &= Mask.dw8; Out |= Accum;
   return Out == 0;
}

//----------------------------------------------------------------------------
// PERIODICITA::Empty
//----------------------------------------------------------------------------
BOOL PERIODICITA::Empty(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::Empty"
   AdHoc & A = *(AdHoc*)Dati;
   DWORD Out;
   Out  = A.dw1; Out |= A.dw2; Out |= A.dw3; Out |= A.dw4;
   Out |= A.dw5; Out |= A.dw6; Out |= A.dw7; Out |= A.dw8;
   return Out == 0;
}

//----------------------------------------------------------------------------
// PERIODICITA::Sett
//----------------------------------------------------------------------------
BYTE PERIODICITA::Sett(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::Sett"
   // Questo operatore ha la caratteristica che il confronto si deve limitare
   // al periodo di validita' dell' orario (al di fuori non e' significativo)
   BYTE Out=0;
   for (int i = 0; i < 7  ; i ++ ) {
      DWORD * A = (DWORD *)Dati;
      DWORD * B = (DWORD *)GGSett[i].Dati;
      BOOL Ok = FALSE;
      Ok |=  A[0] & B[0]; Ok |=  A[1] & B[1]; Ok |=  A[2] & B[2]; Ok |=  A[3] & B[3];
      Ok |=  A[4] & B[4]; Ok |=  A[5] & B[5]; Ok |=  A[6] & B[6]; Ok |=  A[7] & B[7];
      if(Ok) Out += 1 << i;
   } /* endfor */
   return Out;
};
void PERIODICITA::ImpostaProblema(SDATA & InizioDatiCaricati,SDATA & InizioOrarioCorrente, SDATA & FineOrarioCorrente,  SDATA & DataCorrente){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::ImpostaProblema()"
   
   DataC = DataCorrente;
   if( InizioOrarioFS == InizioOrarioCorrente){
      offsetPeriod = DataCorrente - InizioOrario;
      TRACESTRING("Orario gia' impostato");
      TRACEVSTRING2(InizioOrario);
      TRACEVSTRING2(InizioOrarioFS);
      TRACEVSTRING2(FineOrarioFS);
      TRACEVSTRING2(DataCorrente);
      TRACEVLONG(offsetPeriod);
      TRACESTRING(VRS(GiornoInizioOrario) + " (Dati caricati: Data= "+STRINGA(InizioOrario)+") ");
      return;
   }
   InizioOrarioFS     = InizioOrarioCorrente;
   InizioOrario       = InizioDatiCaricati  ;
   FineOrarioFS       = FineOrarioCorrente;
   offsetPeriod       = DataCorrente - InizioOrario;
   GiornoInizioOrario = InizioOrario.GiornoDellaSettimana();
   
   TRACEVSTRING2(InizioOrario);
   TRACEVSTRING2(InizioOrarioFS);
   TRACEVSTRING2(FineOrarioFS);
   TRACEVSTRING2(DataCorrente);
   TRACEVLONG(offsetPeriod);
   TRACESTRING(VRS(GiornoInizioOrario) + " (Dati caricati: Data= "+STRINGA(InizioOrario)+") ");
   
   MaskOrario.ReSet();
   MaskOrario.Set(InizioOrarioFS, FineOrarioFS);
   
   int MaxOff = FineOrarioFS - InizioOrario;
   int GiornoFineOrarioFs = (GiornoInizioOrario + MaxOff) % 7;
   for (int Of = MaxOff; Of >= 0 ; Of -= 7 ) {
      GGSett[GiornoFineOrarioFs].Set(Of);
   } /* endfor */
   for (int i = 1; i < 7 ; i ++ ){
      int Idx = (700 + GiornoFineOrarioFs - i )%7;
      GGSett[Idx] = GGSett[(Idx+1)%7];
      GGSett[Idx].GiornoPrecedente();
   } /* endfor */
   for (i = 0; i < 7 ; i ++ ){
      GGSett[i] &= MaskOrario;
   } /* endfor */
//<<< void PERIODICITA::ImpostaProblema SDATA & InizioDatiCaricati,SDATA & InizioOrarioCorrente, SDATA & FineOrarioCorrente,  SDATA & DataCorrente  
};

// Torna un intero con la circolazione per i 4 giorni a partire dal giorno corrente.
// Il parametro ParteGiornoPrecedente shifta di un giorno per compensare le fermate
// effettuate il giorno successivo alla partenza del treno
BYTE _fastcall PERIODICITA::Circola4(BYTE ParteGiornoPrecedente ){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::Circola4"
   
   // Trace("SoloPerOra");
   WORD Out, OffSet;
   BYTE Idx;
   // TRACEVSTRING2(Hex());
   if (ParteGiornoPrecedente) {
      if (offsetPeriod) {
         OffSet = offsetPeriod - 1; // Correzione per il giorno precedente
      } else {
         Out = Dati[0] & 0x7; // Considero i primi tre giorni dell' orario
         Out << 1;            // Correggo per il giorno di differenza. Si noti che il primo giorno non circola
         return Out;
      } /* endif */
   } else {
      OffSet = offsetPeriod;
   }
   Idx    = OffSet >> 3  ;
   //TRACEVLONG(Idx);
   if (Idx < MAX_MMP_BYTES - 1) {
      Out = * (WORD*) (Dati + Idx); // Carico due Bytes
   } else {
      Out =   Dati[Idx]; // Carico un solo Byte
   } /* endif */
   //TRACEVLONG(Out);
   //TRACEVLONG(OffSet & 0x7);
   // Shifto Out in modo da avere il giorno corrente come LSB
   Out >>= (OffSet & 0x7) ;
   //TRACEVLONG(Out);
   // Mask 4 giorni
   Out &= 0xf;
   //TRACEVLONG(Out);
   return Out;
//<<< BYTE _fastcall PERIODICITA::Circola4 BYTE ParteGiornoPrecedente   
};

// Queste funzioni NON e' necessario siano ad alta efficienza
void PERIODICITA::Set(SDATA InizioPeriodo , SDATA FinePeriodo){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::Set"
   //TRACESTRING3("InizioPeriodo,FinePeriodo=", InizioPeriodo , FinePeriodo);
   if(InizioPeriodo < InizioOrario)InizioPeriodo = InizioOrario;
   int Offs1 = InizioPeriodo - InizioOrario;
   int Offs2 = min(FinePeriodo   - InizioOrario , 8 * MAX_MMP_BYTES -1);
   if(Offs1 > Offs2) return;
   for (int i = Offs1; i <= Offs2 ; i++ ) {
      Set(i);
   } /* endfor */
}
void PERIODICITA::ReSet(SDATA InizioPeriodo , SDATA FinePeriodo){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::ReSet"
   //TRACESTRING3("InizioPeriodo,FinePeriodo=", InizioPeriodo , FinePeriodo);
   if(InizioPeriodo < InizioOrario)InizioPeriodo = InizioOrario;
   int Offs1 = InizioPeriodo - InizioOrario;
   int Offs2 = min(FinePeriodo   - InizioOrario , 8 * MAX_MMP_BYTES -1);
   if(Offs1 > Offs2) return;
   for (int i = Offs1; i <= Offs2 ; i++ ) {
      ReSet(i);
   } /* endfor */
}

// E questa mi ritorna una stringa con i dati di periodicita' in esadecimale
STRINGA PERIODICITA::Hex(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::Hex"
   char Tmp[MAX_MMP_BYTES*2 + 1];
   BYTE  * Byte = Dati;
   for (int i =0  ;i < MAX_MMP_BYTES ; i++) {
      sprintf(Tmp + 2*i,"%02x",*Byte);
      Byte ++;
   } /* endfor */
   Tmp[2 * MAX_MMP_BYTES] = 0;
   return STRINGA(Tmp);
};

void  PERIODICITA::Trace(const STRINGA & Msg, int Livello) {
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::Trace()"
   
   if(Livello > trchse) return;
   ERRSTRING(Msg);
   ELENCO_S Tmp = Decod();
   ORD_FORALL(Tmp,i)ERRSTRING(Tmp[i]);
}

ELENCO_S PERIODICITA::Decod(BOOL ConTestata,BOOL DoubleSpace,BOOL Points){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA::Decod()"
   
   typedef char DATIMESE[63];
   static char Gs[8]= "LMmGVSD";          // Mercoledi = m
   static DATIMESE Vuoto= "                                                               ";
   static DATIMESE Punti= "...............................................................";
   static DATIMESE MatMesi[12];           // matrice dei mesi
   static ELENCO_S Mesi;
   
   if(Mesi.Dim() == 0){
      Mesi += "GENNAIO  ";
      Mesi += "FEBRAIO  ";
      Mesi += "MARZO    ";
      Mesi += "APRILE   ";
      Mesi += "MAGGIO   ";
      Mesi += "GIUGNO   ";
      Mesi += "LUGLIO   ";
      Mesi += "AGOSTO   ";
      Mesi += "SETTEMBRE";
      Mesi += "OTTOBRE  ";
      Mesi += "NOVEMBRE ";
      Mesi += "DICEMBRE ";
   }
   
   int Unit = DoubleSpace  ? 2 : 1;
   int Size = 31 * Unit;
   DATIMESE LineaVuota;
   if (Points) {
      memmove(LineaVuota,Punti,sizeof(DATIMESE));
   } else {
      memmove(LineaVuota,Vuoto,sizeof(DATIMESE));
   } /* endif */
   for(int m = 0; m<12; m++)memcpy(MatMesi[m],LineaVuota,Size);
   
   
   for(SDATA Day=InizioOrario; Day <= FineOrarioFS; ++ Day){
      int Offset= Day - InizioOrario;
      if(Circola(Offset)){
         int GiornoDellaSettimana = (Offset + GiornoInizioOrario) %7;
         MatMesi[Day.Mese-1][Unit*(Day.Giorno-1)]= Gs[GiornoDellaSettimana];
      }
   }
   
   //Trace della matrice di periodicit… mese per mese nel formato voluto
   ELENCO_S Out;
   
   if (ConTestata) {
      if (DoubleSpace) {
         Out += "   Mese    | 1   3   5   7   9  11  13  15  17  19  21  23  25  27  29  31";
         Out += "           |   2   4   6   8  10  12  14  16  18  20  22  24  26  28  30  ";
      } else {
         Out += "   Mese    |          1         2         3 ";
         Out += "           | 1234567890123456789012345678901";
      } /* endif */
   } /* endif */
   
   for (int i = 0;  i < 12 ; i++){ // Dodici mesi
      int Mese = (InizioOrario.Mese + i - 1 )%12; // Il -1 per indicizzare le array
      
      BOOL Ok = memcmp(MatMesi[Mese], LineaVuota,Size);
      
      if (Ok) {  // Se ho un mese non vuoto
         int Limit = Size;
         Limit -= (31 - limitiMese[Mese])*Unit;
         if(Mese == 1){  // Febbraio
            int Anno = InizioOrario.Anno;
            if(InizioOrario.Mese + i - 1 > 12) Anno ++; // Avevo superato l' anno
            if((Anno % 4) || (!(Anno % 100) && (Anno % 400)))Limit -= Unit; // Non Bisestile
         }
         MatMesi[Mese][Limit]=0; // Fine riga
         STRINGA Tmp;
         Tmp += Mesi[Mese];
         Tmp += "  | ";
         Tmp += MatMesi[Mese];
         Out +=Tmp;
      }
   }
   return Out;
//<<< ELENCO_S PERIODICITA::Decod BOOL ConTestata,BOOL DoubleSpace,BOOL Points  
}

//----------------------------------------------------------------------------
// T_PERIODICITA
//----------------------------------------------------------------------------

void T_PERIODICITA::R_SetGiornoDellaSett(GIORNI_SETTIMANA Giorno, BOOL s, SDATA InizioPeriodo, SDATA FinePeriodo){
   
   int  Offs1  = (InizioOrario < InizioPeriodo) ? (InizioPeriodo - InizioOrario) : 0;
   int  Offs2  = min(FinePeriodo - InizioOrario, 8 * MAX_MMP_BYTES -1);
   
   // Identificazione del primo giorno nel periodo che corrisponde
   // al giorno della settimana dato
   Offs1 += (700 + Giorno - (GiornoInizioOrario + Offs1) ) % 7;
   
   
   for(int Offset=Offs1; Offset<=Offs2; Offset+=7){  //setto a 0 o 1 i bit corrispondenti
      if(s) Set(Offset); else ReSet(Offset);
   } /* endfor */
   
}

void T_PERIODICITA::R_SetFeste    (BOOL s, SDATA InizioPeriodo, SDATA FinePeriodo  ) {
   
   int  Offs1  = (InizioOrario < InizioPeriodo) ? (InizioPeriodo - InizioOrario) : 0;
   int  Offs2  = min(FinePeriodo - InizioOrario, 8 * MAX_MMP_BYTES -1);
   
   ORD_FORALL(GiorniFestivi,i){
      int Offset = GiorniFestivi[i] - InizioOrario;
      if(Offset >= Offs1 && Offset <= Offs2){
         if(s) Set(Offset); else ReSet(Offset);
      } //endif
   }
}

void T_PERIODICITA::R_SetPostFeriale    (BOOL s, SDATA InizioPeriodo, SDATA FinePeriodo  ) {
   
   int  Offs1  = (InizioOrario < InizioPeriodo) ? (InizioPeriodo - InizioOrario) : 0;
   int  Offs2  = min(FinePeriodo - InizioOrario, 8 * MAX_MMP_BYTES -1);
   
   SDATA Limit1  = (InizioOrario < InizioPeriodo) ? InizioPeriodo : InizioOrario;
   
   // Imposto i festivi postferiali
   ORD_FORALL(GiorniFestivi,i){
      SDATA Precedente = GiorniFestivi[i]; --Precedente ;
      if(Precedente.GiornoDellaSettimana() == DOMENICA)continue;
      ORD_FORALL(GiorniFestivi,j){ // Vedo se il precedente e' un giorno festivo
         if(Precedente == GiorniFestivi[j])break;
      }
      if(j < GiorniFestivi.Dim())continue;  // Si era anch' esso un festivo
      int Offset = GiorniFestivi[i] - InizioOrario;
      if(Offset >= Offs1 && Offset <= Offs2){
         if(s) Set(Offset); else ReSet(Offset);
      } //endif
   }
   
   // E' inefficiente ma ... who cares ?
   SDATA Current = Limit1 ;
   for(int Offset=Offs1; Offset<=Offs2; ++ Offset , ++Current){
      if(Current.GiornoDellaSettimana() != DOMENICA)continue; // Solo le domeniche
      SDATA Precedente = Current; --Precedente ;
      ORD_FORALL(GiorniFestivi,j){ // Vedo se il precedente e' un giorno festivo
         if(Precedente == GiorniFestivi[j])break;
      }
      if(j < GiorniFestivi.Dim())continue;  // Si era anch' esso un festivo
      int Offset = Current - InizioOrario;
      if(s) Set(Offset); else ReSet(Offset);
   } /* endfor */
//<<< void T_PERIODICITA::R_SetPostFeriale     BOOL s, SDATA InizioPeriodo, SDATA FinePeriodo     
}


void T_PERIODICITA::R_SetFesteScol(BOOL s, SDATA InizioPeriodo, SDATA FinePeriodo  ) {
   
   int  Offs1  = (InizioOrario < InizioPeriodo) ? (InizioPeriodo - InizioOrario) : 0;
   int  Offs2  = min(FinePeriodo - InizioOrario, 8 * MAX_MMP_BYTES -1);
   
   ORD_FORALL(FestivitaSoloScolastiche,i){
      int Offset = FestivitaSoloScolastiche[i] - InizioOrario;
      if(Offset >= Offs1 && Offset <= Offs2){
         if(s) Set(Offset); else ReSet(Offset);
      } //endif
   }
}

void T_PERIODICITA::R_SetPreFeste(BOOL s, SDATA InizioPeriodo , SDATA FinePeriodo   ) {
   
   int  Offs1  = (InizioOrario < InizioPeriodo) ? (InizioPeriodo - InizioOrario) : 0;
   int  Offs2  = min(FinePeriodo - InizioOrario, 8 * MAX_MMP_BYTES -1);
   
   ORD_FORALL(GiorniFestivi,i){
      int Offset = GiorniFestivi[i] - InizioOrario - 1;
      if(Offset >= Offs1 && Offset <= Offs2){
         if(s) Set(Offset); else ReSet(Offset);
      } //endif
   }
}

void T_PERIODICITA::R_SetPostFeste(BOOL s, SDATA InizioPeriodo , SDATA FinePeriodo   ) {
   
   int  Offs1  = (InizioOrario < InizioPeriodo) ? (InizioPeriodo - InizioOrario) : 0;
   int  Offs2  = min(FinePeriodo - InizioOrario, 8 * MAX_MMP_BYTES -1);
   
   ORD_FORALL(GiorniFestivi,i){
      int Offset = GiorniFestivi[i] - InizioOrario + 1;
      if(Offset >= Offs1 && Offset <= Offs2){
         if(s) Set(Offset); else ReSet(Offset);
      } //endif
   }
}


//----------------------------------------------------------------------------
// PERIODIC::Analisi
//----------------------------------------------------------------------------
STRINGA PERIODIC::Analisi(){ // Dump in chiaro di cosa rappresenta
   #undef TRCRTN
   #define TRCRTN "PERIODIC::Analisi"
   if (Codice[0] != '0' || Codice[1] != '0' ) {
      // La conversione a STRINGA aggiunge gli "/" al formato scritto sul file
      // Dopodiche' si puo' convertire ad SDATA
      SDATA Inizio, Fine;
      Inizio.DaGGMMAAAA(STRINGA(InizioPeriodo));
      Fine.DaGGMMAAAA(STRINGA(FinePeriodo));
      int Co = 10 *(Codice[0] - '0') + (Codice[1] - '0');
      return STRINGA(" Codice ")+ STRINGA(Co) +" "+ T_PERIODICITA::DecodPeriodicitaPerCodice[Co]+" Da "+STRINGA(Inizio)+" a "+STRINGA(Fine);
   } else {
      return NUSTR;
   } /* endif */
};

//----------------------------------------------------------------------------
// T_PERIODICITA::ComponiPeriodicita
//----------------------------------------------------------------------------
void T_PERIODICITA::ComponiPeriodicita(int NumPeriodic, PERIODIC * Periodicita, BOOL Reset) {
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::ComponiPeriodicita"
   
   T_PERIODICITA Wrk;
   SDATA Inizio;
   SDATA Fine;
   
   #ifdef DBGCOMPONI
   TRACEVLONG(Reset);
   #endif
   
   BOOL Init = TRUE;
   // Questo ciclo esterno e' per mettere le sospensioni DOPO le effettuazioni
   for (int sTipo = 1; sTipo >= 0; sTipo --) {
      for (int i=0; i<=NumPeriodic; i++) {
         if (Periodicita[i].Codice[0] == '0' && Periodicita[i].Codice[1] == '0')continue;
         int Codice = 10 *(Periodicita[i].Codice[0] - '0') + (Periodicita[i].Codice[1] - '0');
         
         if(PeriodicitaPerCodice[Codice].Tipo != sTipo)continue;
         
         Wrk=PeriodicitaPerCodice[Codice];
         
         // La conversione a STRINGA aggiunge gli "/" al formato scritto sul file
         // Dopodiche' si puo' convertire ad SDATA
         Inizio.DaGGMMAAAA(STRINGA(Periodicita[i].InizioPeriodo));
         Fine.DaGGMMAAAA(STRINGA(Periodicita[i].FinePeriodo));
         #ifdef DBGCOMPONI
         TRACESTRING(" Codice "+ STRINGA(Codice)+" "+ DecodPeriodicitaPerCodice[Codice]+" Da "+STRINGA(Inizio)+" a "+STRINGA(Fine));
         #endif
         
         
         -- Inizio; // Intervallo aperto: perche' servono per pulire i complementi
         ++ Fine  ; // Intervallo aperto: perche' servono per pulire i complementi
         
         if(PeriodicitaPerCodice[Codice].Tipo == 0){        //caso della sospensione
            if (Inizio >= InizioOrario) Wrk.Set(InizioOrario, Inizio );
            if (Fine   <= MaxData    )      Wrk.Set(Fine,MaxData    );
         } else {  // Effettuazione
            if (Inizio >= InizioOrario) Wrk.ReSet(InizioOrario, Inizio );
            if (Fine   <= MaxData    )      Wrk.ReSet(Fine,MaxData    );
         } /* endif */
         
         if (Reset && Init) { // Il primo imposta
            THIS = Wrk;
            Init = FALSE;
         } else {      // I successivi modificano
            if(PeriodicitaPerCodice[Codice].Tipo == 0){        //caso della sospensione
               THIS &= Wrk;
            } else     {                //caso della effettuazione
               THIS |= Wrk;
            }
         }    // endif
         #ifdef DBGCOMPONI
         Trace("Risultato: ");
         #endif
//<<< for  int i=0; i<=NumPeriodic; i++   
      }
//<<< for  int sTipo = 1; sTipo >= 0; sTipo --   
   } /* endfor */
   // Correzione: aggiungo eventualmente 1 giorno prima e dopo i limiti dell' orario
   if(THIS == InLimitiOrario) THIS = InLimitiOrario;
//<<< void T_PERIODICITA::ComponiPeriodicita int NumPeriodic, PERIODIC * Periodicita, BOOL Reset   
}

//----------------------------------------------------------------------------
//  T_PERIODICITA::Cardinalita
//----------------------------------------------------------------------------
WORD T_PERIODICITA::Cardinalita(){ // Ritorna il numero di elementi impostati ad 1 (Nei limiti orario);
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::Cardinalita()"
   
   int Out=0;
   AdHoc & A = *(AdHoc*)Dati;
   AdHoc & B = *(AdHoc*)InOrario.Dati;
   Out += NumeroBits(A.dw1 & B.dw1);
   Out += NumeroBits(A.dw2 & B.dw2);
   Out += NumeroBits(A.dw3 & B.dw3);
   Out += NumeroBits(A.dw4 & B.dw4);
   Out += NumeroBits(A.dw5 & B.dw5);
   Out += NumeroBits(A.dw6 & B.dw6);
   Out += NumeroBits(A.dw7 & B.dw7);
   Out += NumeroBits(A.dw8 & B.dw8);
   
   //int MaxOff = FineOrarioFS - InizioOrario;
   //for (int Of = MaxOff; Of >= 0 ; Of -- ) {
   //   if(Circola(Of))Out ++;
   //} /* endfor */
   return Out;
//<<< WORD T_PERIODICITA::Cardinalita    // Ritorna il numero di elementi impostati ad 1  Nei limiti orario ;
};

//----------------------------------------------------------------------------
//  T_PERIODICITA::Init
//----------------------------------------------------------------------------
BOOL T_PERIODICITA::Init(const STRINGA &  PathFileDefinizione,const STRINGA & NomeFileDefinizione)
{
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::Init()"
   
   // Formato del file definizione: (e' previsto sia caricato a mano o scaricando tabelle DB2)
   // NOME_ORARIO              =Orario invernale 95-96
   // PROGRESSIVO_FORNITURA    =1
   // INIZIO_ORARIO_FS         =24/09/1995
   // FINE_ORARIO_FS           =01/06/1996
   // INIZIO_DATI_FORNITI      =24/09/1995
   // FINE_DATI_FORNITI        =01/06/1996
   // INIZIO_ORA_LEGALE        =28/05/1995
   // FINE_ORA_LEGALE          =24/09/1995
   // ORA_INIZIO_ORA_LEGALE    =180
   // ORA_FINE_ORA_LEGALE      =180
   // INIZIO_ORARIO_FS         =GG/MM/AAAA
   // FINE_ORARIO_FS           =GG/MM/AAAA
   // INIZIO_ANNOSCOLASTICO    =GG/MM/AAAA
   // FINE_ANNOSCOLASTICO      =GG/MM/AAAA
   // INIZIO_ANNOSCOLASTICO2   =GG/MM/AAAA
   // FINE_ANNOSCOLASTICO2     =GG/MM/AAAA
   // GIORNOFESTIVO            =GG/MM/AAAA
   // GIORNOFESTIVO            =GG/MM/AAAA
   //  ....
   // FESTIVITASOLOSCOLASTICA  =GG/MM/AAAA
   // FESTIVITASOLOSCOLASTICA  =GG/MM/AAAA
   //  ....
   
   
   // Inizializzazione variabili
   PeriodicitaPerCodice.ReDim(80);                 // Questo cambia l' allocazione
   
   ZeroFill(Inizio_Dati_Caricati    );
   ZeroFill(Fine_Dati_Caricati      );
   ZeroFill(Inizio_Orario_FS        );
   ZeroFill(Fine_Orario_FS          );
   ZeroFill(Inizio_Dati_Forniti     );
   ZeroFill(Fine_Dati_Forniti       );
   ZeroFill(Inizio_Ora_Legale       );
   ZeroFill(Fine_Ora_Legale         );
   ZeroFill(Inizio_AnnoScolastico   );
   ZeroFill(Fine_AnnoScolastico     );
   ZeroFill(Inizio_AnnoScolastico2  );
   ZeroFill(Fine_AnnoScolastico2    );
   Ora_Inizio_Ora_Legale =0;
   Ora_Fine_Ora_Legale   =0;
   GiorniFestivi.Clear();
   FestivitaSoloScolastiche.Clear();
   ProgressivoFornitura  =  0;
   
   STRINGA FileDefinizioneOrario = PathFileDefinizione + NomeFileDefinizione;
   // Adesso scandisco il file e carico le variabili di base
   if(FileDefinizioneOrario != NUSTR && TestFileExistance(FileDefinizioneOrario)) {
      FILE_RO Fil(FileDefinizioneOrario);
      STRINGA Linea;
      while(Fil.gets(Linea)) {
         Linea.Strip();
         Linea.UpCase();
         TRACEVSTRING2(Linea);
         if(Linea.Dim() < 2) continue; // Linea vuota o 1 solo spazio
         if(Linea[0]< 'A' || Linea[0]> 'Z' )continue; // Interpreto come commenti le linee che iniziano con un carattere strano
         ELENCO_S Toks = Linea.Tokens("=");
         if(Toks.Dim() != 2){
            ERRSTRING("Errore: mi aspetto rigidamente una sintassi del tipo KEYWORD = VALORE");
            BEEP;
            continue;
         }
         STRINGA KeyWord = Toks[0];
         STRINGA Value   = Toks[1];
         KeyWord.Strip();
         Value.Strip();
         BOOL Ok = FALSE;
         
         SDATA Tmp;ZeroFill(Tmp);
         if (KeyWord=="PROGRESSIVO_FORNITURA") {
            ProgressivoFornitura = Value.ToInt();
            Ok = ProgressivoFornitura > 0;
         } else if (KeyWord=="NOME_ORARIO") {
            NomeOrario = Value;
            Ok = 1;
         } else if (KeyWord=="INIZIO_DATI_FORNITI") {
            Ok = Inizio_Dati_Forniti.DaGGMMAAAA(Value);
         } else if (KeyWord=="FINE_DATI_FORNITI") {
            Ok = Fine_Dati_Forniti.DaGGMMAAAA(Value);
         } else if (KeyWord=="INIZIO_ORA_LEGALE") {
            Ok = Inizio_Ora_Legale.DaGGMMAAAA(Value);
         } else if (KeyWord=="FINE_ORA_LEGALE") {
            Ok = Fine_Ora_Legale.DaGGMMAAAA(Value);
         } else if (KeyWord=="ORA_INIZIO_ORA_LEGALE") {
            Ora_Inizio_Ora_Legale= Value.ToInt();
            Ok = Ora_Inizio_Ora_Legale < 1440 ;
         } else if (KeyWord=="ORA_FINE_ORA_LEGALE") {
            Ora_Fine_Ora_Legale= Value.ToInt();
            Ok = Ora_Fine_Ora_Legale < 1440 ;
         } else if (KeyWord=="INIZIO_ORARIO_FS") {
            Ok = Inizio_Orario_FS.DaGGMMAAAA(Value);
//<<<    if  KeyWord=="PROGRESSIVO_FORNITURA"   
         } else if (KeyWord=="FINE_ORARIO_FS")   {
            Ok = Fine_Orario_FS.DaGGMMAAAA(Value);
         } else if (KeyWord=="INIZIO_ANNOSCOLASTICO")  {
            Ok = Inizio_AnnoScolastico.DaGGMMAAAA(Value);
         } else if (KeyWord=="FINE_ANNOSCOLASTICO") {
            Ok = Fine_AnnoScolastico.DaGGMMAAAA(Value);
         } else if (KeyWord=="INIZIO_ANNOSCOLASTICO2")  {
            Ok = Inizio_AnnoScolastico2.DaGGMMAAAA(Value);
         } else if (KeyWord=="FINE_ANNOSCOLASTICO2") {
            Ok = Fine_AnnoScolastico2.DaGGMMAAAA(Value);
         } else if (KeyWord=="GIORNOFESTIVO") {
            Ok = Tmp.DaGGMMAAAA(Value);
            if(Ok && Tmp.GiornoDellaSettimana() == DOMENICA){
               Ok = FALSE;
               ERRSTRING("Fornita erroneamente la DOMENICA "+STRINGA(Tmp)+" come festivita' : Ignorata");
            };
            if(Ok) GiorniFestivi += Tmp;
         } else if (KeyWord=="FESTIVITASOLOSCOLASTICA") {
            Ok = Tmp.DaGGMMAAAA(Value);
            if(Ok && Tmp.GiornoDellaSettimana() == DOMENICA){
               Ok = FALSE;
               ERRSTRING("Fornita erroneamente la DOMENICA "+STRINGA(Tmp)+" come festivita' scolastica: Ignorata");
            };
            if(Ok)FestivitaSoloScolastiche += Tmp;
//<<<    if  KeyWord=="PROGRESSIVO_FORNITURA"   
         } else {
            ERRSTRING("KeyWord non riconosciuta: "+KeyWord);
            Ok = FALSE;
         }
         if (!Ok) {
            ERRSTRING("Non interpretata linea '"+Linea+"'"+" SDATA Impostata: "+STRINGA(Tmp));
            BEEP;
         } /* endif */
//<<< while Fil.gets Linea    
      }
      
      if(Inizio_Dati_Forniti > Inizio_Orario_FS ) Inizio_Orario_FS = Inizio_Dati_Forniti;
      Inizio_Dati_Caricati =  Inizio_Orario_FS;
      -- Inizio_Dati_Caricati ; // Per permettere shifts
      
      if(Fine_Dati_Forniti   < Fine_Orario_FS   ) Fine_Orario_FS   = Fine_Dati_Forniti;
      Fine_Dati_Caricati   =  Fine_Orario_FS;
      ++ Fine_Dati_Caricati  ; // Per permettere shifts
      
      
      // Scolastica
      FORALL(FestivitaSoloScolastiche,f1){
         FORALL(GiorniFestivi,f2){
            if(GiorniFestivi[f2] == FestivitaSoloScolastiche[f1]){
               ERRSTRING("Fornita erroneamente il giorno festivo "+STRINGA(GiorniFestivi[f2])+" come festivita' scolastica: Ignorato");
               FestivitaSoloScolastiche -= f1;
               break;
            }
         }
      };
      
      
      PERIODICITA::ImpostaProblema(Inizio_Dati_Caricati,Inizio_Orario_FS,Fine_Orario_FS,Inizio_Orario_FS);
      InOrario.ReSet();InOrario.Tipo = 1;InOrario.Set(Inizio_Orario_FS,Fine_Orario_FS);
      FuoriOrario = InOrario; ~FuoriOrario;
      InLimitiOrario.ReSet();InLimitiOrario.Tipo = 1;InLimitiOrario.Set(Inizio_Dati_Caricati,Fine_Dati_Caricati);
      
      TRACEVLONG(GiornoInizioOrario         );
      TRACEVSTRING2(Inizio_Dati_Caricati    );
      TRACEVSTRING2(Fine_Dati_Caricati      );
      TRACEVSTRING2(Inizio_Orario_FS        );
      TRACEVSTRING2(Fine_Orario_FS          );
      TRACEVSTRING2(Inizio_Dati_Forniti     );
      TRACEVSTRING2(Fine_Dati_Forniti       );
      TRACEVSTRING2(Inizio_Ora_Legale       );
      TRACEVSTRING2(Fine_Ora_Legale         );
      TRACEVLONG(Ora_Inizio_Ora_Legale      );
      TRACEVLONG(Ora_Fine_Ora_Legale        );
      TRACEVSTRING2(Inizio_AnnoScolastico   );
      TRACEVSTRING2(Fine_AnnoScolastico     );
      TRACEVSTRING2(Inizio_AnnoScolastico2  );
      TRACEVSTRING2(Fine_AnnoScolastico2    );
      ORD_FORALL(GiorniFestivi,i1){
         TRACEVSTRING2(GiorniFestivi[i1]);
      };
      ORD_FORALL(FestivitaSoloScolastiche,i2){
         TRACEVSTRING2(FestivitaSoloScolastiche[i2]);
      };
      
      // Caricamento dei tipi di periodicita'
      T_PERIODICITA  Wrk;
      
      if(Inizio_AnnoScolastico < Inizio_Dati_Caricati )Inizio_AnnoScolastico = Inizio_Dati_Caricati;
      if(Fine_AnnoScolastico > Fine_Dati_Caricati )Fine_AnnoScolastico = Fine_Dati_Caricati;
      if(Inizio_AnnoScolastico2 < Inizio_Dati_Caricati )Inizio_AnnoScolastico2 = Inizio_Dati_Caricati;
      if(Fine_AnnoScolastico2 > Fine_Dati_Caricati )Fine_AnnoScolastico2 = Fine_Dati_Caricati;
      
      // Trovo l' ultima data che entra in PERIODICITA
      MaxData = Inizio_Dati_Caricati;
      while ((MaxData - Inizio_Dati_Caricati) < 8 * MAX_MMP_BYTES -1) ++MaxData;
      
      MaskOrario.Trace("MaskOrario");
      
      
      // 0 ed 1: Vuote
      Wrk.ReSet();                  // Non circola mai
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "NON DEFINITA";
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "NON DEFINITA";
      
      
      // 2 : Si effettua sempre
      Wrk.Set();
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua";
      
      // 3: Si effettua nei giorni lavorativi
      Wrk.Set() ;
      Wrk.R_SetGiornoDellaSett(DOMENICA, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetFeste(0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei giorni lavorativi";
      
      // 4 : Come il precedente ma escludo i sabati
      Wrk = PeriodicitaPerCodice[3];  // Feriali
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei giorni lavorativi escluso il sabato";
      
      // 5:  Si effettua nei festivi
      Wrk.ReSet() ;
      Wrk.R_SetFeste(1,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(DOMENICA, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei giorni festivi";
      
      
      //6: Si effettua nei giorni scolastici
      Wrk.ReSet() ;
      Wrk.Set(Inizio_AnnoScolastico, Fine_AnnoScolastico);
      Wrk.Set(Inizio_AnnoScolastico2, Fine_AnnoScolastico2);
      Wrk &= PeriodicitaPerCodice[3];   // Feriali
      Wrk.R_SetFesteScol(0, Inizio_AnnoScolastico, Fine_AnnoScolastico);
      Wrk.R_SetFesteScol(0, Inizio_AnnoScolastico2, Fine_AnnoScolastico2);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei giorni scolastici";
      
      //7: Si effettua dalla Domenica al Venerd
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua dalla domenica al venerd";
      
      //8: Si effettua il Sabato e nei giorni festivi
      Wrk = PeriodicitaPerCodice[5]; // Festivi
      Wrk.R_SetGiornoDellaSett(SABATO, 1,Inizio_Dati_Caricati, MaxData);
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Sabato e nei giorni festivi";
      
      //9: Si effettua nei giorni scolastici escluso il sabato
      Wrk = PeriodicitaPerCodice[6]; // Scolastici
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei giorni scolastici escluso il sabato";
      
      //10: Si effettua nei giorni feriali non scolastici
      Wrk.Set();
      Wrk.ReSet(Inizio_AnnoScolastico, Fine_AnnoScolastico);
      Wrk.ReSet(Inizio_AnnoScolastico2, Fine_AnnoScolastico2);
      Wrk.R_SetFesteScol(1, Inizio_AnnoScolastico, Fine_AnnoScolastico);
      Wrk.R_SetFesteScol(1, Inizio_AnnoScolastico2, Fine_AnnoScolastico2);
      Wrk &= PeriodicitaPerCodice[3];   // Feriali
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei giorni feriali non scolastici";
      
      //11: Si effettua nei giorni non scolastici e festivi
      Wrk = PeriodicitaPerCodice[10]; // Non scolastici e feriali
      Wrk |= PeriodicitaPerCodice[5]; // Festivi
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei giorni non scolastici e festivi";
      
      
      //12: Si effettua il Luned feriale
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(LUNEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[3];   // Feriali
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Luned feriale";
      
      //13: Si effettua il Marted feriale
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(MARTEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[3];   // Feriali
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Marted feriale";
      
      //14: Si effettua il Mercoled feriale
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(MERCOLEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[3];   // Feriali
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Mercoled feriale";
      
      //15: Si effettua il Gioved feriale
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(GIOVEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[3];   // Feriali
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Gioved feriale";
      
      //16: Si effettua il Venerd feriale
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(VENERDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[3];   // Feriali
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Venerd feriale";
      
      //17: Si effettua il Sabato feriale
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(SABATO, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[3];   // Feriali
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Sabato feriale";
      
      //18: Si effettua la Domenica
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(DOMENICA, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua la Domenica";
      
      //19: Sospeso il Luned
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(LUNEDI, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso il Luned";
      
      //20: Sospeso il Marted
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(MARTEDI, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso il Marted";
      
      //21: Sospeso il Mercoled
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(MERCOLEDI, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso il Mercoled";
      
      
      //22: Sospeso il Gioved
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(GIOVEDI, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso il Gioved";
      
      //23: Sospeso il Venerd
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(VENERDI, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso il Venerd";
      
      //24: Sospeso il Sabato
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso il Sabato";
      
      //25: Si effettua il Marted scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(MARTEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Marted scolastico";
      
      //26: Si effettua nei giorni scolastici escluso il marted
      Wrk = PeriodicitaPerCodice[6]; // Scolastici
      Wrk.R_SetGiornoDellaSett(MARTEDI, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei giorni scolastici escluso il marted";
      
      //27: Si effettua il sabato scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(SABATO, 1, Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il sabato scolastico";
      
      //28: Sospeso
      Wrk.ReSet() ;
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso";
      
      //29: Si effettua nei pre-festivi
      Wrk.ReSet() ;
      Wrk.R_SetPreFeste(1,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(SABATO, 1, Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei pre-festivi";
      
      //30: Sospeso nei pre-festivi
      Wrk.Set() ;
      Wrk.R_SetPreFeste(0,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(SABATO, 0, Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso nei pre-festivi";
      
      //31: Si effettua nei post-festivi
      Wrk.ReSet() ;
      Wrk.R_SetPostFeste(1,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(LUNEDI,1, Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei post-festivi";
      
      //32: Sospeso nei post-festivi
      Wrk.Set() ;
      Wrk.R_SetPostFeste(0,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(LUNEDI, 0, Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso nei post-festivi";
      
      //33: Si effettua nei festivi infrasettimanali
      Wrk = PeriodicitaPerCodice[5]; // Festivi
      Wrk.R_SetGiornoDellaSett(DOMENICA, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua nei festivi infrasettimanali";
      
      //34: Sospeso la Domenica
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(DOMENICA, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso la Domenica";
      
      //35: Si effettua dal Luned al Venerd
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(DOMENICA, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua dal Luned al Venerd";
      
      //36: Si effettua dal Luned al Sabato
      Wrk.Set  ();
      Wrk.R_SetGiornoDellaSett(DOMENICA, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua dal Luned al Sabato";
      
      //37: Sospeso nei festivi
      Wrk = PeriodicitaPerCodice[3];   // Feriali
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso nei festivi";
      
      //38: Sospeso Sabato e festivi
      Wrk = PeriodicitaPerCodice[3];   // Feriali
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso Sabato e festivi";
      
      //39: Si effettua Sabato e Domenica
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(SABATO, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(DOMENICA, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua Sabato e Domenica";
      
      //40: Sospeso Sabato e Domenica
      Wrk.Set() ;
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(DOMENICA, 0,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Sospeso Sabato e Domenica";
      
      //41: Si effettua il Luned
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(LUNEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Luned";
      
      //42: Si effettua il Marted
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(MARTEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice += "Si effettua il Marted";
      
      //43: Si effettua il Mercoled
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(MERCOLEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il Mercoled";
      
      //44: Si effettua il Gioved
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(GIOVEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il Gioved";
      
      //45: Si effettua il Venerd
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(VENERDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il Venerd";
      
      //46: Si effettua il Sabato
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(SABATO, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il Sabato";
      
      //47: Si effettua il luned scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(LUNEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il luned scolastico";
      
      //48: Si effettua il mercoled scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(MERCOLEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il mercoled scolastico";
      
      //49: Si effettua il gioved scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(GIOVEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il gioved scolastico";
      
      //50: Si effettua il venerd scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(VENERDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il venerd scolastico";
      
      //51: Si effettua nei giorni scolastici escluso il luned
      Wrk = PeriodicitaPerCodice[6]; // Scolastici
      Wrk.R_SetGiornoDellaSett(LUNEDI, 0,Inizio_Dati_Caricati, MaxData);
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua nei giorni scolastici escluso il luned";
      
      //52: Si effettua nei giorni scolastici escluso il mercoled
      Wrk = PeriodicitaPerCodice[6]; // Scolastici
      Wrk.R_SetGiornoDellaSett(MERCOLEDI, 0,Inizio_Dati_Caricati, MaxData);
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua nei giorni scolastici escluso il mercoled";
      
      //53: Si effettua nei giorni scolastici escluso il gioved
      Wrk = PeriodicitaPerCodice[6]; // Scolastici
      Wrk.R_SetGiornoDellaSett(GIOVEDI, 0,Inizio_Dati_Caricati, MaxData);
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua nei giorni scolastici escluso il gioved";
      
      //54: Si effettua nei giorni scolastici escluso il venerd
      Wrk = PeriodicitaPerCodice[6]; // Scolastici
      Wrk.R_SetGiornoDellaSett(VENERDI, 0,Inizio_Dati_Caricati, MaxData);
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua nei giorni scolastici escluso il venerd";
      
      //55: Si effettua nei giorni feriali non scolastici escluso il sabato
      Wrk = PeriodicitaPerCodice[10];  // Non scolastici e feriali
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua nei giorni feriali non scolastici escluso il sabato";
      
      //56: Sospeso il Gioved non scolastico
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(GIOVEDI, 0,Inizio_Dati_Caricati, MaxData);
      Wrk |= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Sospeso il Gioved non scolastico";
      
      //57: Sospeso il Sabato non scolastico
      Wrk.Set() ;
      Wrk.R_SetGiornoDellaSett(SABATO, 0,Inizio_Dati_Caricati, MaxData);
      Wrk |= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Sospeso il Sabato non scolastico";
      
      //58: Si effettua il Luned non scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(LUNEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[11]; // Non Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il Luned non scolastico";
      
      //59: Si effettua il Gioved non scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(GIOVEDI, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[11]; // Non Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il Gioved non scolastico";
      
      //60: Si effettua il Sabato non scolastico
      Wrk.ReSet() ;
      Wrk.R_SetGiornoDellaSett(SABATO, 1,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[11]; // Non Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il Sabato non scolastico";
      
      //61: Sospeso il Gioved scolastico
      Wrk.Set();
      Wrk.R_SetGiornoDellaSett(GIOVEDI, 0, Inizio_Dati_Caricati, MaxData);
      Wrk |= PeriodicitaPerCodice[11]; // Non Scolastici
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Sospeso il Gioved scolastico";
      
      //62: Sospeso nei festivi infrasettimanali
      Wrk = PeriodicitaPerCodice[3];   // Feriali
      Wrk.R_SetGiornoDellaSett(DOMENICA, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Sospeso nei festivi infrasettimanali";
      
      //63: Si effettua nei feriali nel periodo scolastico
      Wrk = PeriodicitaPerCodice[3];   // Feriali
      Wrk &= PeriodicitaPerCodice[6]; // Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua nei feriali nel periodo scolastico";
      
      //64: Si effettua il martedi' non scolastico
      Wrk.ReSet();
      Wrk.R_SetGiornoDellaSett(MARTEDI, 1 ,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[11];  // Non Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il martedi' non scolastico";
      
      //65: Si effettua il mercoledi' non scolastico
      Wrk.ReSet();
      Wrk.R_SetGiornoDellaSett(MERCOLEDI, 1 ,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[11];  // Non Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il mercoledi' non scolastico";
      
      //66: Si effettua il venerdi' non scolastico
      Wrk.ReSet();
      Wrk.R_SetGiornoDellaSett(VENERDI, 1 ,Inizio_Dati_Caricati, MaxData);
      Wrk &= PeriodicitaPerCodice[11];  // Non Scolastici
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Si effettua il venerdi' non scolastico";
      
      //67: Riservato
      Wrk.ReSet();
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="RISERVATO";
      
      //68: Riservato
      Wrk.ReSet();
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="RISERVATO";
      
      //69: Sospeso dal lunedi al venerdi'
      Wrk.ReSet();
      Wrk.R_SetGiornoDellaSett(SABATO, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(DOMENICA, 1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+="Sospeso dal lunedi al venerdi'";
      
      //70: Sospeso dalla domenica al giovedi
      Wrk.ReSet();
      Wrk.R_SetGiornoDellaSett(VENERDI, 1 ,Inizio_Dati_Caricati, MaxData);
      Wrk.R_SetGiornoDellaSett(SABATO, 1 ,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=0;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+= "Sospeso dalla domenica al giovedi";
      
      //71: Si effettua nei giorni scolastici post-festivi
      Wrk = PeriodicitaPerCodice[6];   // Scolastici
      Wrk &= PeriodicitaPerCodice[31]; // Post Festivi
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+= "Si effettua nei giorni scolastici post-festivi";
      
      //72: Si effettua nei giorni seguenti i feriali
      Wrk.ReSet();
      Wrk.R_SetPostFeriale(1,Inizio_Dati_Caricati, MaxData);
      Wrk.Tipo=1;
      PeriodicitaPerCodice += Wrk;
      DecodPeriodicitaPerCodice+= "Si effettua nei giorni seguenti i feriali";
      
      #ifdef DBGBASE
      //fine caricamento dell' array delle periodicit… "PeriodicitaPerCodice"
      TRACESTRINGL("FINE CARICAMENTO DATI : Segue dettaglio periodicita' di base",1);
      
      // Trace dei dati caricati
      ORD_FORALL(PeriodicitaPerCodice,i){
         PeriodicitaPerCodice[i].Trace("["+STRINGA(i)+"] :"+ DecodPeriodicitaPerCodice[i],1);
      };
      #endif
      
      // Imposto anche periodicit…, per avere i corretti limiti dell' orario
      PERIODICITA::ImpostaProblema( T_PERIODICITA::Inizio_Dati_Caricati, T_PERIODICITA::Inizio_Orario_FS, T_PERIODICITA::Fine_Orario_FS, T_PERIODICITA::Inizio_Dati_Caricati);
      
      // Adesso Imposto i limiti shiftati dell' orario
      MaskOrarioShift[0] = MaskOrario;
      MaskOrarioShift[0].GiornoSeguente(); // Per confrontare con uno shift di -1
      MaskOrarioShift[0] &= MaskOrario;
      MaskOrarioShift[1] = MaskOrario;     // Per confrontare in assenza di shift
      MaskOrarioShift[2] = MaskOrario;
      MaskOrarioShift[2].GiornoPrecedente(); // Per confrontare con uno shift di 1
      MaskOrarioShift[2] &= MaskOrario;
      MaskOrarioShift[3] = MaskOrarioShift[2] ;
      MaskOrarioShift[3].GiornoPrecedente(); // Per confrontare con uno shift di 2
      MaskOrarioShift[3] &= MaskOrario;
      MaskOrarioShift[4] = MaskOrarioShift[3] ;
      MaskOrarioShift[4].GiornoPrecedente(); // Per confrontare con uno shift di 3
      MaskOrarioShift[4] &= MaskOrario;
      // for (int sh = 0;sh < 5  ; sh ++) MaskOrarioShift[sh].Trace("MaskOrarioShift["+STRINGA(sh)+"]",1);
      
      return TRUE;
//<<< if FileDefinizioneOrario != NUSTR && TestFileExistance FileDefinizioneOrario    
   } else {
      ERRSTRING("Errore: non esiste file "+FileDefinizioneOrario);
      BEEP;
      return FALSE;
   }
   
   
//<<< BOOL T_PERIODICITA::Init const STRINGA &  PathFileDefinizione,const STRINGA & NomeFileDefinizione 
};

//----------------------------------------------------------------------------
// T_PERIODICITA::ConfrontaShiftata
//----------------------------------------------------------------------------
BOOL __fastcall T_PERIODICITA::ConfrontaShiftata( const T_PERIODICITA & PeriodShiftata, signed char Shift) const {
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::ConfrontaShiftata"
   if(Shift < -1 || Shift > 3){
      ERRSTRING("Errore: shift illegale = " + STRINGA(Shift));
      BEEP;
      return 0;
   }
   // Questo operatore ha la caratteristica che il confronto si deve limitare
   // al periodo di validita' dell' orario (al di fuori non e' significativo)
   // ed escludendo shift giorni all' inizio od alla fine dell' orario
   AdHoc & A = *(AdHoc*)Dati;
   AdHoc & B = *(AdHoc*)PeriodShiftata.Dati;
   AdHoc & Mask = *(AdHoc*)MaskOrarioShift[Shift+1].Dati;
   DWORD Accum,Out;
   Accum = A.dw1; Accum ^= B.dw1 ; Accum &= Mask.dw1; Out  = Accum;
   Accum = A.dw2; Accum ^= B.dw2 ; Accum &= Mask.dw2; Out |= Accum;
   Accum = A.dw3; Accum ^= B.dw3 ; Accum &= Mask.dw3; Out |= Accum;
   Accum = A.dw4; Accum ^= B.dw4 ; Accum &= Mask.dw4; Out |= Accum;
   Accum = A.dw5; Accum ^= B.dw5 ; Accum &= Mask.dw5; Out |= Accum;
   Accum = A.dw6; Accum ^= B.dw6 ; Accum &= Mask.dw6; Out |= Accum;
   Accum = A.dw7; Accum ^= B.dw7 ; Accum &= Mask.dw7; Out |= Accum;
   Accum = A.dw8; Accum ^= B.dw8 ; Accum &= Mask.dw8; Out |= Accum;
   return Out == 0;
//<<< BOOL __fastcall T_PERIODICITA::ConfrontaShiftata  const T_PERIODICITA & PeriodShiftata, signed char Shift  const  
}
//----------------------------------------------------------------------------
// T_PERIODICITA::ConfrontaShiftando
//----------------------------------------------------------------------------
// Ha il side effect di alterare PeriodDaShiftare
BOOL __fastcall T_PERIODICITA::ConfrontaShiftando( T_PERIODICITA & PeriodDaShiftare, signed char Shift) const {
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::ConfrontaShiftando"
   if(Shift >= -1 && Shift <= 3){
      for (int Sh = Shift ;Sh > 0 ;Sh -- ) PeriodDaShiftare.GiornoPrecedente();
      for (    Sh = Shift ;Sh < 0 ;Sh ++ ) PeriodDaShiftare.GiornoSeguente();
      PeriodDaShiftare &= InLimitiOrario ;
   }
   
   return ConfrontaShiftata( PeriodDaShiftare, Shift);
};

//----------------------------------------------------------------------------
// T_PERIODICITA::ShiftMe
//----------------------------------------------------------------------------
void __fastcall T_PERIODICITA::ShiftMe(signed char Shift){
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::ShiftMe"
   if(Shift >= -1 && Shift <= 3){
      for (int Sh = Shift ;Sh > 0 ;Sh -- ) GiornoPrecedente();
      for (    Sh = Shift ;Sh < 0 ;Sh ++ ) GiornoSeguente();
      THIS &= InLimitiOrario ;
   } else {
      ERRSTRING("Errore: shift illegale = " + STRINGA(Shift));
      BEEP;
   }
};
// -------------------------------------------------------------
// T_PERIODICITA::PrimoPeriodoDiEffettuazione
// -------------------------------------------------------------
void T_PERIODICITA::PrimoPeriodoDiEffettuazione(SDATA & Dal, SDATA& Al){
   // Si blocca per 15 giorni consecutivi di non effettuazione
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::PrimoPeriodoDiEffettuazione"
   // ........................................
   // Identifico un periodo di effettuazione,
   // separati da almeno 15 giorni di sospensione
   // ........................................
   SDATA DataInEsame = Inizio_Dati_Caricati;
   WORD Offset = 0;
   BOOL InPeriodo = FALSE;
   int  GiorniSospensione = 0;
   while (DataInEsame <= Fine_Dati_Caricati) {
      if (Circola(Offset)) {
         GiorniSospensione = 0;
         if (InPeriodo) {
            Al  = DataInEsame;
         } else {
            InPeriodo = TRUE;
            Dal = DataInEsame;
            Al  = DataInEsame;
         } /* endif */
      } else {
         GiorniSospensione ++;
         if (InPeriodo) {
            if (GiorniSospensione >= 15) { // Fine Periodo
               break; // Fine
            } /* endif */
         } /* endif */
      } /* endif */
      ++ DataInEsame ;
      Offset ++;
   } /* endwhile */
//<<< void T_PERIODICITA::PrimoPeriodoDiEffettuazione SDATA & Dal, SDATA& Al  
};

// -------------------------------------------------------------
// T_PERIODICITA::PrimoPeriodoCostante
// -------------------------------------------------------------
void T_PERIODICITA::PrimoPeriodoCostante(SDATA & Dal, SDATA& Al, BYTE Euristic){
   // Si blocca quando cambia la periodicita' settimanale
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::PrimoPeriodoCostante"
   
   // Il periodo costante e' parte del periodo di effettuazione, che identifico subito
   PrimoPeriodoDiEffettuazione(Dal,Al);
   
   int  GiorniKO = 0;
   int  GiorniOK = 0;
   int  ConsecOK = 0;
   if((Al - Dal) >=  18){  // Se sono almeno due settimane e mezzo
      SDATA DataInEsame = Dal;
      SDATA MaxOk;
      WORD Offset = Dal - Inizio_Dati_Caricati ;
      WORD Offset2  = Offset + 7;
      for (int d = 7; d > 0  ; d--) ++ DataInEsame;
      while (DataInEsame <= Al) {
         if (Circola(Offset) == Circola(Offset2)) {   // Match
            GiorniOK ++;
            ConsecOK ++;
            if (Euristic != 2) {
               MaxOk  = DataInEsame;
            } else {
               if(ConsecOK > 2) MaxOk  = DataInEsame;
            } /* endif */
         } else {       // MisMatch
            GiorniKO ++;
            if (Euristic == 3) {
               break; // Nessun errore ammesso
            } else if (Euristic == 2) {  // Due giorni di tolleranza
               if(GiorniKO > 2)break;
               ConsecOK = 0; // reset per non far aggiungere dei falsi match
            } else {
               if(GiorniKO > 1)break; // Tollero un solo giorno di disallineamento
            } /* endif */
         } /* endif */
         ++ DataInEsame ;
         Offset ++;
         Offset2 ++;
//<<< while  DataInEsame <= Al   
      } /* endwhile */
      Al = MaxOk;
//<<< if  Al - Dal  >=  18    // Se sono almeno due settimane e mezzo
   }
   #ifdef DBGESPLODI
   TRACEVLONG(GiorniOK);
   #endif
   if(GiorniOK < 8){ // Se non ho piu' di una settimana di Match
      Dal = Al = Inizio_Dati_Caricati ; // Inibisce l' utilizzo dei dati
   };
//<<< void T_PERIODICITA::PrimoPeriodoCostante SDATA & Dal, SDATA& Al, BYTE Euristic  
};

// -------------------------------------------------------------
// T_PERIODICITA::EsplodiPeriodicita'
// -------------------------------------------------------------
PERIODICITA_IN_CHIARO * T_PERIODICITA::EsplodiPeriodicita(BYTE Euristic)const{
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::EsplodiPeriodicita()"
   
   if (PeriodicitaPerCodice.Dim() < 40) {
      ERRSTRING("Errore: Chiamare prima Init()");
      BEEP;
      return NULL;
   } /* endif */
   if( InizioOrario != Inizio_Dati_Caricati || FineOrarioFS != Fine_Orario_FS ){
      ERRSTRING("Errore: Si deve prima impostare il problema");
      TRACEVSTRING2(InizioOrario );
      TRACEVSTRING2(Inizio_Dati_Caricati);
      TRACEVSTRING2(FineOrarioFS);
      TRACEVSTRING2(Fine_Orario_FS);
      BEEP;
      return NULL;
   }
   
   // Genero l' insieme di vettori base
   if (PERIODICITA_IN_CHIARO::PeriodicitaBase.Dim() == 0) {
      PERIODICITA_IN_CHIARO::PeriodicitaBase += PeriodicitaPerCodice[2 ]; // 2 : Si effettua sempre
      PERIODICITA_IN_CHIARO::PeriodicitaBase += PeriodicitaPerCodice[3 ]; // 3: Si effettua nei giorni lavorativi
      PERIODICITA_IN_CHIARO::PeriodicitaBase += PeriodicitaPerCodice[5 ]; // 5:  Si effettua nei festivi
      PERIODICITA_IN_CHIARO::PeriodicitaBase += PeriodicitaPerCodice[8 ]; // 8: Si effettua il Sabato e nei giorni festivi
      PERIODICITA_IN_CHIARO::PeriodicitaBase += PeriodicitaPerCodice[29]; //29: Si effettua nei pre-festivi
      PERIODICITA_IN_CHIARO::PeriodicitaBase += PeriodicitaPerCodice[31]; //31: Si effettua nei post-festivi
      PERIODICITA_IN_CHIARO::PeriodicitaBase += PeriodicitaPerCodice[6 ]; // 6: Si effettua nei giorni scolastici
      PERIODICITA_IN_CHIARO::PeriodicitaBase += PeriodicitaPerCodice[11]; //11: Si effettua nei giorni non scolastici
      ORD_FORALL(PERIODICITA_IN_CHIARO::PeriodicitaBase,i) PERIODICITA_IN_CHIARO::PeriodicitaBase[i] &= InLimitiOrario;
      //ORD_FORALL(PERIODICITA_IN_CHIARO::PeriodicitaBase,i1) PERIODICITA_IN_CHIARO::PeriodicitaBase[i1].Trace("Periodicita base["+STRINGA(i1)+"]");
   } /* endif */
   
   PERIODICITA_IN_CHIARO * Per = new PERIODICITA_IN_CHIARO; // Questa e' la periodicita' da ritornare
   
   // Gestico i casi banali e frequentissimi in cui circoli tutti i giorni, o tutti i lavorativi, o tutti i festivi, o sabato e festivi
   T_PERIODICITA Copia = THIS;
   Copia &= InOrario;
   int Banale =  PERIODICITA_IN_CHIARO::PeriodicitaBase.Cerca( Copia ) ;
   if( Banale >= 0 ){
      PERIODO_PERIODICITA Perio ; // Periodo di effettuazione
      Perio.Dal           = Inizio_Orario_FS         ;
      Perio.Al            = Fine_Orario_FS           ;
      Perio.FormaCompatta = Copia;
      Perio.GiorniDellaSettimana  =0x7f ;
      Perio.Tipo = 1; // Effettuazione
      Perio.TipoPeriodo = PERIODO_PERIODICITA::TIPO_PERIODO(Banale) ; // Tipo Periodo
      #ifdef DBGESPLODI
      TRACESTRING("Caso Banale: circola "+ PERIODO_PERIODICITA::Decod_Tipo_Periodo[Banale] );
      #endif
      *Per += Perio;
   } else {
      PERIODICITA_IN_CHIARO::InSoppressione.ReSet();
      
      while ( Copia.Cardinalita() ){
         // Identifico N periodi di effettuazione, separati da almeno 15 giorni di sospensione
         PERIODO_PERIODICITA Perio ; // Periodo di effettuazione
         if (Euristic == 0 || Euristic >= 4 ) {
            Copia.PrimoPeriodoDiEffettuazione(Perio.Dal,Perio.Al);
         } else  {
            Copia.PrimoPeriodoCostante(Perio.Dal,Perio.Al,Euristic);
            if((Perio.Al - Perio.Dal) < 12) Copia.PrimoPeriodoDiEffettuazione(Perio.Dal,Perio.Al);
         } /* endif */
         // Trasferisco il periodo da Copia a Perio
         Perio.FormaCompatta.ReSet();
         Perio.FormaCompatta.Set( Perio.Dal, Perio.Al );
         Perio.FormaCompatta &= Copia;
         Copia.ReSet( Perio.Dal, Perio.Al );
         // Esplodo il periodo in componenti elementari
         Perio.Tipo = 1; // Effettuazione
         #ifdef DBGESPLODI
         Perio.FormaCompatta.Trace("Periodo base di suddivisione:");
         #endif
         Per->Popola(Perio,Euristic);    // Lo esplodo in forma "normale"
      } /* endwhile */
      
      // I giorni di soppressione potrebbero essere in parte un artefatto dovuto alla
      // scomposizione: Pertanto controllo con i dati originari
      T_PERIODICITA Wrk = THIS ; ~Wrk ; PERIODICITA_IN_CHIARO::InSoppressione &= Wrk ;
      
      #ifdef DBGESPLODI
      PERIODICITA_IN_CHIARO::InSoppressione.Trace("Giorni di soppressione del servizio:");
      #endif
      
      if (PERIODICITA_IN_CHIARO::InSoppressione.Cardinalita()) {
         // Identifico N periodi di soppressione, separati da almeno 15 giorni di sospensione
         PERIODO_PERIODICITA Perio ; // Periodo di effettuazione
         T_PERIODICITA Copia = PERIODICITA_IN_CHIARO::InSoppressione;
         Copia.PrimoPeriodoDiEffettuazione(Perio.Dal,Perio.Al);
         // Trasferisco il periodo da Copia a Perio
         Perio.FormaCompatta.ReSet();
         Perio.FormaCompatta.Set( Perio.Dal, Perio.Al );
         Perio.FormaCompatta &= Copia;
         Copia.ReSet( Perio.Dal, Perio.Al );
         // Esplodo il periodo in componenti elementari
         Perio.Tipo = 0; // Sospensione
         Per->Popola(Perio,0);    // Lo esplodo in forma "normale"
      } /* endif */
//<<< if  Banale >= 0   
   } /* endif */
   
   // Se non del tutto soddisfacente ci riprovo con delle euristiche differenti
   if(Euristic == 0){
      if(Per->Dim() > 3 ){
         PERIODICITA_IN_CHIARO * Per1 ; BOOL Prefer;
         for (int i = 1;i <= 6 ; i++ ) {
            if(i == 6 && ! Per->HoTipiParticolari) continue;
            Per1 = EsplodiPeriodicita(i);
            Prefer = Per1->Dim() < Per->Dim() ;
            Prefer |= Per->HoTipiParticolari && !Per1->HoTipiParticolari  && Per1->Dim() == Per->Dim() ;
            if(Prefer){ delete Per; Per = Per1; } else { delete Per1; }
         } /* endfor */
      }
   }
   
   return Per;
//<<< PERIODICITA_IN_CHIARO * T_PERIODICITA::EsplodiPeriodicita BYTE Euristic const 
};
// -------------------------------------------------------------
// PERIODO_PERIODICITA::Compute
// -------------------------------------------------------------
void PERIODO_PERIODICITA::Compute(){
   #undef TRCRTN
   #define TRCRTN "PERIODO_PERIODICITA::Compute"
   // Ricalcolo Dal ed Al;
   SDATA DataInEsame = T_PERIODICITA::Inizio_Dati_Caricati;
   BOOL Done = FALSE;
   WORD Offset = 0;
   while (DataInEsame <= T_PERIODICITA::Fine_Dati_Caricati) {
      if (FormaCompatta.Circola(Offset)) {
         if (!Done) Dal = DataInEsame;
         Al = DataInEsame;
         Done = TRUE;
      } /* endif */
      ++ DataInEsame ;
      Offset ++;
   } /* endwhile */
};

//----------------------------------------------------------------------------
// T_PERIODICITA::InChiaro
//----------------------------------------------------------------------------
STRINGA T_PERIODICITA::InChiaro(){
   #undef TRCRTN
   #define TRCRTN "T_PERIODICITA::InChiaro"
   STRINGA Period;
   PERIODICITA_IN_CHIARO * PerIc = EsplodiPeriodicita();
   if (PerIc) {
      ELENCO_S Tmp =  PerIc->PeriodicitaLeggibile();
      ORD_FORALL(Tmp,t)Period += Tmp[t]+"; ";
      delete PerIc;
   } else {
      Period = "Fallita decodifica Periodicita'";
   } /* endif */
   return Period;
};

//----------------------------------------------------------------------------
// PERIODICITA_IN_CHIARO::PeriodicitaLeggibile
//----------------------------------------------------------------------------
ELENCO_S PERIODICITA_IN_CHIARO::PeriodicitaLeggibile(){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA_IN_CHIARO::PeriodicitaLeggibile"
   
   ELENCO_S Out;
   
   typedef  char C15[16];
   static C15 GG[] = { "Lunedi ",  "Martedi ",  "Mercoledi ",  "Giovedi ",  "Venerdi ",  "Sabato ",  "Domenica " };
   
   ORD_FORALL(THIS,i){
      PERIODO_PERIODICITA & Per = THIS[i];
      STRINGA s;
      s += Per.Tipo ? "Si Effettua " : "E' sospeso ";
      if(Per.TipoPeriodo > PERIODO_PERIODICITA::TUTTI_I_GIORNI ) s+= PERIODO_PERIODICITA::Decod_Tipo_Periodo[Per.TipoPeriodo]+ " ";
      if(Per.TipoPeriodo == PERIODO_PERIODICITA::GIORNI_LAVORATIVI && Per.GiorniDellaSettimana == 0x1f){
         s += "tranne il sabato "; // La domenica e' implicita essendo lavorativo
      } else if (Per.GiorniDellaSettimana != 0x7f) {
         for (int Tranne = 0; Tranne < 7 ; Tranne ++ ) if(Per.GiorniDellaSettimana == 0x7f & ~(1 << Tranne) )break;
         if (Tranne < 6) {
            s += "tranne il " ;
            s += GG[Tranne];
         } else if (Tranne == 6) {
            s += "tranne la ";
            s += GG[Tranne];
         } else {
            s += "di ";
            for (int j = 0; j < 7 ;j ++ ) if(Per.GiorniDellaSettimana & (1 << j))s += GG[j];
         }
      }
      if (Per.Dal != Per.Al) {
         if(Per.Dal != T_PERIODICITA::Inizio_Orario_FS || Per.Al != T_PERIODICITA::Fine_Orario_FS ){
            s += "Dal "+ STRINGA(Per.Dal)+ " ";
            s += "Al  "+ STRINGA(Per.Al)+ " ";
         }
      } else {
         s += "il "+ STRINGA(Per.Dal)+ " ";
      } /* endif */
      if(s.Dim() <= 15 && Per.TipoPeriodo == PERIODO_PERIODICITA::TUTTI_I_GIORNI ) s+= "tutti i giorni ";
      Out += s;
//<<< ORD_FORALL THIS,i  
   }
   return Out;
//<<< ELENCO_S PERIODICITA_IN_CHIARO::PeriodicitaLeggibile   
};

// -------------------------------------------------------------
// PERIODICITA_IN_CHIARO::Trace
// -------------------------------------------------------------
void PERIODICITA_IN_CHIARO::Trace(const STRINGA&  Msg, int Livello ){
   
   #undef TRCRTN
   #define TRCRTN "PERIODICITA_IN_CHIARO::Trace"
   if(Livello > trchse)return;
   
   ERRSTRING(Msg);
   
   ELENCO_S DaTracciare = PeriodicitaLeggibile();
   ORD_FORALL(DaTracciare,i) ERRSTRING(DaTracciare[i]);
};

// -------------------------------------------------------------
// PERIODICITA_IN_CHIARO::Popola
// -------------------------------------------------------------
void PERIODICITA_IN_CHIARO::Popola( PERIODO_PERIODICITA & Origine , BYTE Euristic){
   #undef TRCRTN
   #define TRCRTN "PERIODICITA_IN_CHIARO::Popola"
   
   
   int Cardinalita = Origine.FormaCompatta.Cardinalita();
   if(Cardinalita == 0)return; // Nulla piu' da fare
   #ifdef DBGESPLODI
   TRACESTRING(VRS(Cardinalita));
   #endif
   
   // Pulisco la situazione : Giorni della settimana e Dal / Al;
   Origine.Compute();
   Origine.GiorniDellaSettimana = Origine.FormaCompatta.Sett();        // Informazione riassuntiva per giorni della settimana
   
   PERIODO_PERIODICITA BestMatch;
   BestMatch.FormaCompatta.ReSet();
   BestMatch.Tipo  = Origine.Tipo; // Effettuazione o Sospensione
   int CardinalitaBestMatch = 0;
   int PenalitaBM = 0;
   BYTE Circola7 =  Origine.GiorniDellaSettimana;
   T_PERIODICITA Filtro; Filtro.ReSet();Filtro.Set(Origine.Dal, Origine.Al );
   for ( PERIODO_PERIODICITA::TIPO_PERIODO Tipo = PERIODO_PERIODICITA::TUTTI_I_GIORNI;
      Tipo < PERIODO_PERIODICITA::TAPPO;
      int(Tipo)++){
      if(Euristic == 6 && Tipo >= PERIODO_PERIODICITA::PRE_FESTIVI) break;
      // Adesso esplodo per giorni della settimana
      for (BYTE Giorni = 0x7f; Giorni > 0 ; Giorni -- ) {               // Per tutte le combinazioni dei giorni
         if(Giorni != 0x7f ){
            if( Tipo >= PERIODO_PERIODICITA::GIORNI_FESTIVI) continue;  // Non mischio ad es. festivi ed indicazione giorno settimana
            if((Giorni & Circola7) != Giorni) continue;  // Ho qualche giorno di troppo: Ignoro la combinazione
         }
         
         T_PERIODICITA Wrk = PeriodicitaBase[Tipo];
         Wrk &= Filtro;
         // Elimino i giorni della settimana non necessari
         for (GIORNI_SETTIMANA gg = LUNEDI; gg <= DOMENICA; int(gg)++ ) {
            if ((Giorni & (1 << gg))== 0 ) {
               Wrk.R_SetGiornoDellaSett(gg,0,Origine.Dal, Origine.Al );
            } /* endif */
         } /* endfor */
         T_PERIODICITA Wrk1 = Wrk;        // Questo e' l' insieme dei bit corrispondenti alla periodicita' elementare
         Wrk &= Origine.FormaCompatta;    // E questo il match con i dati da esplodere
         int Card = Wrk.Cardinalita();
         #ifdef DBGESPLODI2
         Wrk1.Trace(VRS(Tipo) +" "+ PERIODO_PERIODICITA::Decod_Tipo_Periodo[Tipo]+"; "+VRS(Giorni)+" Wrk1:");
         Wrk.Trace("Wrk");
         TRACEVLONG(Card);
         #endif
         Wrk =  Origine.FormaCompatta;    // Per trovare i falsi giorni di circolazione
         ~Wrk;                            // Questi sono i giorni in cui non circola
         Wrk &= Wrk1;                     // E questi sono i giorni spuri introdotti
         int Card2 = Wrk.Cardinalita();   // Giorni in cui in si dovranno introdurre sospensioni
         #ifdef DBGESPLODI2
         Wrk.Trace("Wrk");
         TRACEVLONG(Tipo);
         TRACEVLONG(Giorni);
         TRACEVLONG(Card2);
         #endif
         
         if(Card2 > 0 && Origine.Tipo == 0)continue; // Quando sono in soppressione non accetto ulteriori soppressioni
         if(Euristic == 4){
            Card -= 2* Card2 ;
         } else {
            Card -= Card2 ;
         };
         int Penalita = ( Giorni == 0x7f ) ? 0 : 5;
         if(Tipo > PERIODO_PERIODICITA::GIORNI_FESTIVI ) Penalita += 5;
         if(Tipo > PERIODO_PERIODICITA::TUTTI_I_GIORNI ) Penalita += 3;
         if(Card > CardinalitaBestMatch ||
            (Card == CardinalitaBestMatch && Penalita < PenalitaBM)){
            BestMatch.TipoPeriodo          = Tipo ;
            BestMatch.FormaCompatta        = Wrk1 ;
            BestMatch.GiorniDellaSettimana = Giorni ;
            PenalitaBM = Penalita;
            CardinalitaBestMatch = Card;
            #ifdef DBGESPLODI2
            TRACESTRING("Elegibile a BestMatch");
            #endif
            if(Card == Cardinalita)break; // Inutile cercare ancora (velocizza i casi banali)
         }
//<<< for  BYTE Giorni = 0x7f; Giorni > 0 ; Giorni --                   // Per tutte le combinazioni dei giorni
      } /* endfor */
//<<< for   PERIODO_PERIODICITA::TIPO_PERIODO Tipo = PERIODO_PERIODICITA::TUTTI_I_GIORNI;
   }
   
   #ifdef DBGESPLODI
   TRACESTRING( VRS(CardinalitaBestMatch) + VRS(BestMatch.TipoPeriodo ) + VRS( BestMatch.GiorniDellaSettimana ));
   BestMatch.FormaCompatta.Trace("Best Match: ");
   #endif
   
   // Se la cardinalita' e' 0 allora debbo esplodere a giorni singoli ( o a serie di giorni contigui)
   if (CardinalitaBestMatch == 0) {
      SDATA DataInEsame = Origine.Dal;
      WORD Offset = DataInEsame - T_PERIODICITA::Inizio_Dati_Caricati;
      PERIODO_PERIODICITA Wrk;
      Wrk.Tipo                 = Origine.Tipo; // Effettuazione o Sospensione
      Wrk.TipoPeriodo          = PERIODO_PERIODICITA::TUTTI_I_GIORNI;
      Wrk.GiorniDellaSettimana = 0x7F ;
      
      BOOL Trigger = FALSE;
      while (DataInEsame <= Origine.Al  ) {
         if (Origine.FormaCompatta.Circola(Offset)) {
            if(!Trigger) Wrk.Dal     = DataInEsame;
            Wrk.Al                   = DataInEsame;
            Trigger = TRUE;
         } else {
            if(Trigger ) THIS += Wrk;
            Trigger = FALSE;
         };
         ++ DataInEsame ;
         Offset ++;
      }
      Wrk.FormaCompatta.ReSet();
      Wrk.FormaCompatta.Set(Wrk.Dal,Wrk.Al);
      if(Trigger ) THIS += Wrk;
      
      return; // Finito
      
//<<< if  CardinalitaBestMatch == 0   
   } else {
      // Aggiungo il Best Match
      BestMatch.Compute();
      THIS += BestMatch;
      
      // A questo punto mi rimane: un insieme di giorni che dovrei effettuare, ed un insieme di
      // giorni spuri per cui debbo indicare che non si effettuano.
      T_PERIODICITA Wrk,Wrk1;
      Wrk = BestMatch.FormaCompatta;
      ~Wrk;
      Wrk &= Origine.FormaCompatta; // Questi sono i Bits che rimangono da settare
      Wrk1 = Origine.FormaCompatta;
      ~Wrk1; // Questi sono i giorni di NON effettuazione
      Wrk1 &= BestMatch.FormaCompatta; // E questi quelli settati impropriamente
      
      InSoppressione |= Wrk1;
      
      // ........................................
      // CHIAMATE RICORSIVE
      // ........................................
      while ( Wrk.Cardinalita() ){
         if(Euristic != 5){
            // Identifico N periodi di effettuazione, separati da almeno 15 giorni di sospensione
            Wrk.PrimoPeriodoDiEffettuazione(Origine.Dal,Origine.Al);
         }
         // Trasferisco il periodo da Wrk ad Origine
         Origine.FormaCompatta.ReSet();
         Origine.FormaCompatta.Set( Origine.Dal, Origine.Al );
         Origine.FormaCompatta &= Wrk;
         Wrk.ReSet( Origine.Dal, Origine.Al );
         // Esplodo il periodo in componenti elementari
         Popola(Origine,Euristic); // Lo esplodo in forma "normale"
      } /* endwhile */
      
//<<< if  CardinalitaBestMatch == 0   
   } /* endif */
   
//<<< void PERIODICITA_IN_CHIARO::Popola  PERIODO_PERIODICITA & Origine , BYTE Euristic  
};

// -------------------------------------------------------------
// ARRAY_T_PERIODICITA::
// -------------------------------------------------------------
T_PERIODICITA & ARRAY_T_PERIODICITA::operator[] (UINT Indice){
   #undef TRCRTN
   #define TRCRTN "ARRAY_T_PERIODICITA::operator[]"
   #ifdef BOUND_CHECK
   if(Indice >= Dim())BoundCheckError(Indice,Dim());
   #endif
   return * ( ((T_PERIODICITA*)this->Dati) +Indice);
};
ULONG ARRAY_T_PERIODICITA::Dim(){return (this->Length /sizeof(T_PERIODICITA));};
ARRAY_T_PERIODICITA::ARRAY_T_PERIODICITA(ULONG i):BUFR(i* sizeof(T_PERIODICITA)){};
void ARRAY_T_PERIODICITA::SetDimAndInitialize(ULONG Len,BYTE Dato){if(Len*sizeof(T_PERIODICITA) > Alloc)ReDim(Len);Length = Len;memset(Dati,Dato,Len*sizeof(T_PERIODICITA));};
void ARRAY_T_PERIODICITA::ReDim(ULONG NumElem){BUFR::ReDim(NumElem* sizeof(T_PERIODICITA));};
void ARRAY_T_PERIODICITA::operator+= (const T_PERIODICITA & Date){BUFR::Store(& Date, sizeof(T_PERIODICITA));};
T_PERIODICITA& ARRAY_T_PERIODICITA::Last(){
   #undef TRCRTN
   #define TRCRTN "ARRAY_T_PERIODICITA::Last"
   #ifdef BOUND_CHECK
   if(!Length)BoundCheckError(0,0);
   #endif
   return *(T_PERIODICITA*)(Dati + Length -sizeof(T_PERIODICITA));
};
// Torna l' indice dell' oggetto o -1 se non trovato
int ARRAY_T_PERIODICITA::Cerca(const T_PERIODICITA & Target){
   #undef TRCRTN
   #define TRCRTN "ARRAY_T_PERIODICITA::Cerca"
   ORD_FORALL(THIS,i)if(THIS[i]== Target)return i;
   return -1;
};
// Queste funzioni leggono o scrivono l' array da file. Tornano FALSE su errore
BOOL ARRAY_T_PERIODICITA::Get(const STRINGA& NomeFile){
   #undef TRCRTN
   #define TRCRTN "ARRAY_T_PERIODICITA::Get"
   Clear();
   FILE_RO File(NomeFile);
   if(File.FileHandle() == NULL){
      T_PERIODICITA Dummy; ZeroFill(Dummy); THIS += Dummy; // Il primo record corrisponde a nussun giorno valido
      return FALSE;
   }
   ULONG Dim =  File.FileSize() / sizeof(T_PERIODICITA);
   if(Dim * sizeof(T_PERIODICITA) != File.FileSize() ){
      ERRSTRING("Attenzione: la dimensione non corrisponde");
      BEEP;
      return FALSE;
   }
   if (Dim == 0) {
      T_PERIODICITA Dummy; ZeroFill(Dummy); THIS += Dummy; // Il primo record corrisponde a nussun giorno valido
   } else {
      File.Leggi(File.FileSize(),THIS);
   } /* endif */
   return TRUE;
//<<< BOOL ARRAY_T_PERIODICITA::Get const STRINGA& NomeFile  
};
BOOL ARRAY_T_PERIODICITA::Put(const STRINGA& NomeFile){
   #undef TRCRTN
   #define TRCRTN "ARRAY_T_PERIODICITA::Put"
   FILE_RW File(NomeFile);
   if(File.FileHandle() == NULL){
      BEEP;
      return FALSE;
   }
   File.SetSize(0);
   File.Scrivi(THIS);
   return TRUE;
};

int ARRAY_T_PERIODICITA::Indice(const T_PERIODICITA & Target){
   #undef TRCRTN
   #define TRCRTN "ARRAY_T_PERIODICITA::Indice"
   int Idx = Cerca(Target);
   if(Idx < 0){THIS +=  T_PERIODICITA(Target); Idx = Dim() - 1;};
   return Idx;
};

//----------------------------------------------------------------------------
// S_DATE::operator -=
//----------------------------------------------------------------------------
void S_DATE::operator-= (UINT Pos){
   #undef TRCRTN
   #define TRCRTN "S_DATE::operator-="
   if( Pos >= Dim() ){
      BoundCheckError(Pos,Dim());
      return;
   };
   if (Dim() > (Pos + 1)) { // Test se e' l' ultimo elemento
      SDATA * x = & THIS[Pos];
      memmove(x,(x+1),sizeof(SDATA)*(Dim()-(Pos+1)));
   } /* endif */
   Length -= sizeof(SDATA);
   if(Pointer > Pos)Pointer -= sizeof(SDATA);
};
//----------------------------------------------------------------------------
// GGMMAA::operator STRINGA()
//----------------------------------------------------------------------------
_export GGMMAA::operator STRINGA() const{     // Conversione in stringa (Formato GG/MM/AAAA)
   #undef TRCRTN
   #define TRCRTN "GGMMAA::operator STRINGA()"
   char Out[11];
   Out[ 0] = GG[0];
   Out[ 1] = GG[1];
   Out[ 2] = '/';
   Out[ 3] = MM[0];
   Out[ 4] = MM[1];
   Out[ 5] = '/';
   if (AA[0] < '9') {   // Da 2000 a 2089
      Out[ 6] = '2';
      Out[ 7] = '0';
   } else {             // Da 1990 a 1999
      Out[ 6] = '1';
      Out[ 7] = '9';
   } /* endif */
   Out[ 8] = AA[0];
   Out[ 9] = AA[1];
   Out[10] = '\0';
   return STRINGA(Out);
//<<< _export GGMMAA::operator STRINGA   const      // Conversione in stringa  Formato GG/MM/AAAA 
};
