//========================================================================
// Componente di gestione Dati Orario della rete FS
// Dati base, Nodi Grafo e Percorso fisico
//========================================================================
// Attenzione: Cambiando i formati dei files dati del motore in modo da
// renderlo incompatibile, modificare la #define ORARIO_FORMATO_ATTUALE  
// Vedere la classe VALIDITA_ORARIO
//========================================================================
// 

#ifndef HO_MM_GRAFO_HPP
#define HO_MM_GRAFO_HPP


#ifndef HO_MOTGLUE_HPP
#include "MOTGLUE.HPP"
#endif

#ifndef HO_MM_BASIC_HPP
#include "MM_BASIC.HPP"
#endif

// Queste define per decidere quante soluzioni vedere per ogni percorso
#define VEDI_PRIME_SOLUZIONI 5
#define RIGHE_A_VIDEO        12
#define MAX_TRATTE_POLIMETRICA 40
#define MAXECC               10   // Massimo numero di stazioni su di un percorso eccezionale

typedef WORD ID;   

typedef WORD DIST ;      // Formato interno delle distanze = Km
#define BigDIST 0xffff   // Massima distanza

//----------------------------------------------------------------------------
// NOTA: Questa funzione e' veloce, ma pone il vincolo che gli ID dei nodi precedano gli ID delle
// altre stazioni. 
inline BOOL IsNodo(WORD Id){return Id > 0 && Id <= MAXNODI;};
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Metodi per gestire matrici triangolari o quadrate di bits
//----------------------------------------------------------------------------
// per ogni elemento accetto al massimo 16 BITS !
// Size e' aggiornato e' riporta il numero di bytes utilizzati.
// Si suppone Buffer inizializzato a 0, e sufficientemente grande
// Se si usa la forma con Riga,Colonna deve valere la regola
// che Colonna <= Riga  il primo elemento e' 0,0
BOOL SetBits(void * Buffer,int & Size,int IdxElem, int NumBits, WORD Valore); // FALSE su errore
inline BOOL SetTrgBits(void * Buffer,int & Size,int Riga,int Colonna,int NumBits, WORD Valore){
     return SetBits(Buffer,Size,((Riga  * (Riga +1)) >> 1) + Colonna,NumBits, Valore); };
inline BOOL SetQuadBits(void * Buffer,int & Size,int Riga,int Colonna, int NumColonne, int NumBits, WORD Valore){
     return SetBits(Buffer,Size,(Riga  * NumColonne) + Colonna,NumBits, Valore); };
WORD GetBits(void * Buffer,int IdxElem, int NumBits);
inline WORD GetTrgBits(void * Buffer,int Riga,int Colonna, int NumBits){
   return GetBits(Buffer,((Riga  * (Riga +1)) >> 1) + Colonna,NumBits);};
inline WORD GetQuadBits(void * Buffer,int Riga,int Colonna, int NumColonne, int NumBits){
   return GetBits(Buffer,(Riga  * NumColonne) + Colonna,NumBits);};

//----------------------------------------------------------------------------
// QUAD_TAR
//----------------------------------------------------------------------------
// Struttura per gestire fino a quattro tariffe regionali
// e per identificare i match ...
struct QUAD_TAR { // Per abbonamenti regionali                       // "ne TRC"
     BIT   TariffaRegione     :5 ;     // Codice tariffa regionale di appartenenza
     BIT   Prima_Estensione   :5 ;     // 2ø regione di appartenenza (Codice tariffa regionale )
     BIT   Seconda_Estensione :5 ;     // 3ø regione di appartenenza (Codice tariffa regionale ) 
     BIT   Terza_Estensione   :5 ;     // 4ø regione di appartenenza (Codice tariffa regionale ) 
                                       // NB: La terza estensione e' di solo TRANSITO: non si applica
                                       // al primo ed ultimo nodo
     BIT   NonValido          :1 ;     // Set ad 1 se non valido
     BIT   VarieRegioni       :1 ;     // Set ad 1 se non valido

  BOOL Empty() const ;

  void Set(BYTE Tar,BYTE Es1,BYTE Es2,BYTE Es3); // Impostazione a partire dai dati sciolti

  QUAD_TAR & operator &=(QUAD_TAR & Altro);

  BYTE Identifica()const ; // Identifica la tariffa regionale da applicare

  QUAD_TAR Terminal() {     // Filtra via la terza estensione per la prima ed ultima stazione
     QUAD_TAR Tar = THIS;
     Tar.Terza_Estensione = 0;
     return Tar;
  };

};

//----------------------------------------------------------------------------
// Questa classe serve per la gestione degli instradamenti condizionali
//----------------------------------------------------------------------------
// Equivale ad insiemi di max 48 elementi
// I dati stanno su MM_GRAF4.DB
class ISTR_COND {                                                    // "ne TRC"
   public:
   BOOL operator == (ISTR_COND Dato){return (Bytes_0_3 == Dato.Bytes_0_3  &&  Bytes_4_5 == Dato.Bytes_4_5);};
   void operator &= (ISTR_COND Dato){Bytes_0_3 &= Dato.Bytes_0_3 ; Bytes_4_5 &= Dato.Bytes_4_5;};
   BOOL Inibizione(ISTR_COND Dato){ return (Bytes_0_3 & Dato.Bytes_0_3) | (Bytes_4_5 & Dato.Bytes_4_5);};
   BOOL Inibizione(ISTR_COND Dato1, ISTR_COND Dato2){
      return 
      (Bytes_0_3 & Dato1.Bytes_0_3 & Dato2.Bytes_0_3) | 
      (Bytes_4_5 & Dato1.Bytes_4_5 & Dato2.Bytes_4_5) ; 
   };
   void Clear(){Bytes_0_3=0;Bytes_4_5=0;};

   // Nota:         & 0xf == %16   (per velocizzare)
   void Set(BYTE BitNum){
      if(BitNum < 32){
         Bytes_0_3 |= 1 << BitNum;
      } else if(BitNum < 40){
         Bytes_4_5 |= 1 << (BitNum & 0xf );
      } else {
         ERRINT("Invalid BitNum=",BitNum);
         BEEP;
      }
   };

   BOOL Empty(){ return (Bytes_0_3 | Bytes_4_5 ) == 0;};

   operator const char * () const; // Conversione a char *
   
   private:
   DWORD Bytes_0_3;
   WORD  Bytes_4_5;
};


//----------------------------------------------------------------------------
// STAZ_FS
//----------------------------------------------------------------------------
// Questa classe contiene i dati delle stazioni del grafo
//
// Vi e' un entrata per tutte le stazioni, con le caratteristiche
// della stazione stessa.
//
// Per le sole stazioni nodali sono riportate le stazioni collegate, 
// mediante puntamento ad un' array di primi vicini.
//
// Per le altre stazioni e' possibile accedere al ramo e da questo
// vedere le stazioni collegate.
//
//----------------------------------------------------------------------------
// I dati sono fisicamente archiviati su MM_GRAF1.DB in questo stesso formato
// I dati estesi su MM_GRAF1.EXT
//----------------------------------------------------------------------------

struct STAZ_FS  { // 29 BYTES * 5000 stazioni ~ 150 K                // "ne TRC"
   
   // Questa variabile e' utilizzata per inibire selettivamente
   // dei nodi all' algoritmo del percorso minimo.
   // Durante l' esecuzione dell' algoritmo viene
   // messa in AND con una opportuna variabile.
   enum ABILITAZIONI{
     CONTINENTE=1,   // Abilita tutte le stazioni del continente
     SICILIA   =2,   // Abilita tutte le stazioni della Sicilia
     SARDEGNA  =4,   // Abilita tutte le stazioni della Sardegna
   };

   QUAD_TAR Tariffe;            // Per le tariffe regionali

   BIT  Id                : 13; // Id della stazione
   BIT  StazioneFS        :  1; // 1 = E' stazione FS
   BIT  NumeroConcesse    :  2; // Numero societa' concesse (0 = non e' stazione concessa)
   BIT  NumeroPrimiVicini :  4; // Numero primi vicini (0 se non e' una stazione nodale)
   BIT  NumeroPolimetriche:  4; // Numero polimetriche in cui e' presente la stazione
   BIT  IdRamo            : 10; // Id del ramo (0 se Nodo reale)
   BIT  DiDiramazione     :  1; // 1 = Stazione di diramazione o di allacciamento
   BIT  Nodale            :  1; // 1 = e' un NODO reale
   BIT  Terminale         :  1; // 1 = e' una stazione terminale
   BIT  DiAllacciamento   :  1; // 1 = Stazione di allacciamento
   BIT  DiInstradamento   :  1; // 1 = STAZIONE DI INSTRADAMENTO
   BIT  DiTransito        :  1; // 1 = STAZIONE DI TRANSITO (verso FC): Dato rilevato da RETISTRU
   BIT  Vendibile         :  1; // 
   BIT  InstrGrafo        :  1; // Sono le stazioni nodali e le stazioni intermedie dei rami che NON seguono il percorso piu' breve
   BIT  Abilitazione      :  3; // vedi l' enum
   BIT  CCR               : 17; // Codice CCR stazione 
   BIT  Km1               :  9; // Km verso la prima stazione del ramo (se non nodale)
   BIT  ProgRamo          :  6; // Progressivo nell' ambito del ramo (se non nodale)
                                // Parte da 1; Non coincide con quello di ID_STAZI
                                // il quale ignora le stazioni eliminate dal grafo.
                                // Puo' essere usato come indice di accesso nelle stazioni del RAMO (sottraendo 1)
   BIT  IsCnd             :  7; // Istradamento condizionale (Id)
   BIT  PDati             : 19; // Puntatore (BYTE Offset) ai dati della stazione.
                                // I dati comprendono:
                                //   - NumeroPrimiVicini  PRIMO_V
                                //   - NumeroPolimetriche P_POLI
                                //   - NumeroConcesse     D_CONCESSE
                                //   - Eventuali dati di instradamento
                                //   - Eventuali dati di diramazione
   char IdentIstr[7]          ; // Identificatore per instradamenti (NOME7) : Non e' NULL terminated

   inline ISTR_COND &  IstrCond();
   
   // In questa classe vi sono le relazioni tra stazioni che
   // configurano i rami del GRAFO.
   // Vi sono riportate SOLO le relazioni a grana grossa tra nodo e nodo
   // Le relazioni a grana fine possono essere ricavate dalla tabella RAMO
   struct PRIMO_V {  // 5 Bytes * 400 Nodi * 3.2 primi vicini medi = 6.400 Bytes
      BIT Id           : 13;  // Id del PrimoVicino
      BIT Distanza     :  9;  // Distanza del primo vicino (MAX 512 Km)
      BIT Mare         :  1;  // Se vero indica che i KM sono Km mare (Sicilia-continente = 0)
      BIT Concesso     :  1;  // Se vero indica che i KM sono Km concessi                     
      BIT Instradamento:  1;  // Indica che il ramo ha una stazione di instradamento (interna)
      BIT IdRamo       : 10;  // Punta alla tabella rami
   };
   inline PRIMO_V * PrimiVicini()const;

   // Queste invece sono le relazioni con le polimetriche
   struct P_POLI {  // 3 bytes * 1.2 Polim * 5000 stazioni = 18 K
      BIT  IdPoli      : 10;  // Id della polimetrica
      BIT  TipoPoli    :  4;  // Tipo della polimetrica
      BIT  IdxStazione :  9;  // Indice (=in PO_STAZ) della stazione nella polimetrica
                              // Sono escluse le stazioni non valide
   };
   inline P_POLI * Polimetriche()const;

   // Questi i dati necessari per le societa' concesse
   struct D_CONCESSE { // 4 Byte * 1000 stazioni = 4K
      BIT  CCRCumulativo      :17; // Codice CCR stazione / societa' concessa
      BIT  SocietaConcessa    : 7; // Codice societa' concessa
   };
   inline D_CONCESSE * Concesse()const;
   
   // E questi i dati delle stazioni di instradamento
   struct D_INSTR { // 4 bytes * 300 stazioni = 1.2K
      WORD CodiceInstr;       // Codice di instradamento CVB
      WORD Rank;              // Importanza: 1 = Piu' Importante
   };
   inline D_INSTR & Instradamento()const;

   // E Questi i dati specifici delle stazioni di diramazione
   struct D_DIRAM {
      BIT IsCndInibit  : 7;  // Inibizioni di istradamento condizionale (Id)
      inline ISTR_COND &  IstrCondInibit();
   };
   inline D_DIRAM & Diramazione()const;

   // .............................................
   // Variabili di algoritmo 
   // .............................................
   DIST Distanza1              ; // Prima distanza Complessiva fino a destinazione
   WORD Nodo1                  ; // Id del primo vicino (Sia su grafo che su polimetrica)
   BYTE Progressivo            ; // Indica se toccato dall' algoritmo
   // Queste variabili sono riutilizzate in modo differente a seconda che stia risolvendo
   // sul grafo o sulle polimetriche (NB: non faccio mai le due cose insieme)
   union {
      // Questi dati per i percorsi - polimetriche
      struct {
         WORD Poli1                  ; // Id del primo vicino (polimetrica)
         BYTE Verso                  ; // Verso della polimetrica
         BYTE NumStep                ; // Numero Step in cui e' stato aggiornato = Numero tratte
      } Plm;
      // Questi dati per i percorsi su grafo: servono a dare preferenza ai percorsi
      // su rete interna rispetto a quelli su rete cumulativa o a parita' a scegliere
      // sempre lo stesso percorso (Criterio: Minimizzare la somma degli ID)
      // 
      struct {
         BYTE NumCum                 ; // Numero di RAMI cumulativi nel percorso
         BIT  SumId      : 24        ; // Somma degli ID (basterebbero meno bits)
      } Grf;
   };
   
   // .............................................
   // Metodi Varii
   // .............................................
   const char * Nome7()const;           // Rientrante fino a 10 chiamate
   
   WORD PosDaId(ID IdNodo)const;        // Cerca nell' array dei primi vicini  : Funziona solo per stazioni nodali 
   PRIMO_V & PvDaId(ID IdNodo)const;    // Cerca nell' array dei primi vicini  : Funziona solo per stazioni nodali
   DIST DistDaId(ID IdStazione) const;  // Ritorna i Km: Funziona in generale per due stazioni sullo stesso ramo
   BYTE ConcDaId(ID IdStazione) const;  // Indica se il ramo e' concesso. Funziona in generale per due stazioni sullo stesso ramo

   inline PRIMO_V & PrimoVicino( BYTE Idx ) const;
   inline STAZ_FS & NodoPrimoVicino( BYTE Idx ) const;

   // Test di tipologia
   BOOL IsNodo()const{return (BOOL)Nodale;};
   BOOL IsNodoTerminale()const{return (BOOL)Terminale;};
   BOOL IsStazione()const{return !Nodale && !Terminale;};
   BOOL IsInstrGrafo()const{return (BOOL)InstrGrafo;};

   // Metodi per rilevare la coincidenza tra piu' polimetriche
   // Le polimetriche debbono appartenere alle zone indicate.
   // Le polimetriche cumulative debbono essere comprese nell' elenco 
   void SetPolimetriche(ARRAY_ID & Polims,const ARRAY_ID & Cumulative,WORD Zone = 0xffff);
   // Idem ma imposta SOLO le polimetriche non di diramazione
   void SetPolimetricheNoDiramaz(ARRAY_ID & Polims, const ARRAY_ID & Cumulative, WORD Zone = 0xffff);
   // I metodi seguenti NON sono rientranti
   void AndPolimetriche(ARRAY_ID & Polims,WORD Zone = 0xffff);
   void OrPolimetriche(ARRAY_ID & Polims,WORD Zone = 0xffff);
   // Questo metodo ritorna l' indice nella polimetrica o -1 se non appartiene alla polimetrica
   int HaPolimetrica(ID IdPolimetrica);

};

//----------------------------------------------------------------------------
// RAMO
//---------------------------------------------------------------------------
// In questa classe vi sono i dati dei rami.
// I dati sono fisicamente archiviati su MM_GRAF2.DB in questo stesso formato,
// I dati estesi su MM_GRAF2.EXT
// I dati sono ordinati per coppia IdStaz1 / IdStaz2 in modo che e' 
// possibile accedere con ricerca dicotomica oltre che per indice
// L' Id del ramo e' l' indice del ramo nella tabella.
// L' elemento con indice 0 e' vuoto.
// Le stazioni del ramo NON comprendono i NODI ne' l' eventuale stazione terminale
//----------------------------------------------------------------------------
struct RAMO {  // 10 Bytes * 500 Rami  =  5.000 Bytes                // "ne TRC"
   BIT IdRamo       : 10;  // Id del ramo
   BIT IdStaz1      : 13;  // Prima stazione del Ramo
   BIT IdStaz2      : 13;  // Seconda stazione del Ramo
   BIT KmRamo       :  9;  // Lunghezza complessive del ramo
   BIT Mare         :  1;  // Se vero indica che i KM sono Km mare (Sicilia-continente = 0)
   BIT Concesso     :  1;  // Se vero indica che i KM sono Km concessi
   BIT CodConcessa  :  7;  // Eventuale codice ferrovia concessa
   BIT LineaConcessa:  2;  // e relativa linea
   BIT Instradamento:  1;  // Indica che il ramo ha una stazione di instradamento (interna: cioe' non compresi i nodi)
   BIT NumStazioni  :  6;  // Numero stazioni del ramo (max 64)
   WORD PDati           ;  // Puntatore (BYTE Offset) ai dati del ramo.
                                // I dati comprendono:
                                //   - NumStazioni STAZIONE_R
                    
   // 2 Bytes * 5000 Stazioni = 10 k Bytes
   struct STAZIONE_R {
      BIT Id        : 13;
      BIT Instrad   :  1;  // Se TRUE e' una stazione di instradamento
      BIT Diram     :  1;  // Se TRUE e' una stazione di diramazione
      BIT Vendibile :  1;  // Indica se la stazione e' vendibile.
                           
   };
   inline STAZIONE_R * StazioniDelRamo();

   // Questo metodo torna la prima stazione di instradamento del ramo 
   // compresa tra Da ed A (nell' ordine)
   // Da ed A debbono appartenere al ramo (o esserne stazioni terminali)
   // Torna 0 se non ho stazione di instradamento
   ID PrimaStazioneDiInstradamento(ID Da, ID A); 
};
// Routine di ordinamento rami
int Cmp_RAMO(const void * a, const void * b);

//----------------------------------------------------------------------------
// POLIMETRICA
//----------------------------------------------------------------------------
// In questa classe vi sono i dati delle polimetriche.
// Sono contenuti i dati di tutte le polimetriche (normali,
// di diramazione e di allacciamento).
// I dati sono fisicamente archiviati su MM_GRAF3.DB
// nel formato definito da POLIMETRICA_SU_FILE
// e su MM_GRAF3.EXT 9 dati estesi
// L' elemento con indice 0 e' vuoto ed e' non valido
// Gli ID delle polimetriche sono interni: i numeri di riferimento
// come pure le descrizioni sono presenti solo su file.
// Ai dati delle stazioni si accede per indice all' interno
// delle stazioni della polimetrica.
// E' comunque fornito un metodo che permette di passare dall' Id 
// di una stazione all' indice.
//----------------------------------------------------------------------------
struct POLIMETRICA {  // 15 Bytes * 1000 polimetriche = 15 K         // "ne TRC"

   // NB: l' ordine dell' enum e' anche un ordine di preferenza (a parita' di Km)
   // I codici debbono essere coerenti con quelli utilizzati su POLIM.HPP
   enum  TIPO_POLIMETRICA { 
      CUMULATIVA   = 1,     // Cumulativa semplice (percorso piu' breve)
      MARE_CUM     = 3,     // Cumulativa mare (Tirrenia)
      URBANA_CUM   = 4,     // Tratta urbana di congiunzione tra stazione di diramazione FS e stazione cumulativa
      LOCALE       = 5,     // Polimetrica locale, contenuta all' interno di una zona
      SOLO_LOCALE  = 6,     // Polimetrica SOLO locale, non puo' essere usata per tariffare altro che tratte contenute
      DIRAMAZIONE  = 7,     // Polimetrica di diramazione
      ALLACCIAMENTO= 8,     // Polimetrica di allacciamento
      MARE_FS      = 9
   };
   // I dati dei chilometraggi sono registrati :
   // Come vettore di distanze dalla prima stazione e matrice triangolare di scostamenti per LOCALE, MARE_FS
   // Come vettore di distanze dalla prima stazione e matrice triangolare di distanze per DIRAMAZIONE, ed ALLACCIAMENTO
   // Come vettori di distanze dalle solo stazioni di transito per CUMULATIVE e MARE_CUM
   // NB: Per le relazioni cumualtive non vendibili vengono tornati 9999 Km come distanza
   // NNB: Per ora (provvisoriamente) registrati come polimetriche matriciali (== diramazione)


   BIT  IdPoli         : 10;  // Id della polimetrica
   BIT  KmPoli         : 11;  // Max Km della polimetrica (= parametro di preferenza tra polimetriche)
   BIT  TipoPoli       :  4;  // Tipo della polimetrica (Vedi enum precedente)
   BIT  NumStazioni    :  9;  // Numero Stazioni della polimetrica
                              // Sono escluse le stazioni non valide
   BIT  NumStazDiram   :  6;  // Numero Stazioni di diramazione della polimetrica
   BIT  NumBits        :  4;  // Numero di bits per elemento della polimetrica
                              // Sono escluse le distanze dalla prima stazione della polimetrica
                              // (o dalla stazione di diramazione per le cumulative)
   BIT  PDati          : 20;  // Puntatore (Byte offset) dei dati della polimetrica.
                              // Formato di immagazzinamento:
                              // Polimetriche LOCALI o MARE_FS :
                              //   - NumStazioni strutture di tipo PO_STAZ.
                              //   - NumStazDiram strutture di tipo PO_DIRA.
                              //   - Matrice triangolare di elementi di NumBits BITS.
                              //     Comprende (NumStazioni - 2)(NumStazioni -1)/2 elementi
                              //     Vi sono riportate le distanze (delta da formula predeterminata)
                              // Polimetriche CUMULATIVE :
                              //   - NumStazioni strutture di tipo PO_STAZ.
                              //   - NumStazDiram strutture di tipo PO_DIRA.
                              //   - Matrice NumStazDiram * NumStazioni di elementi di NumBits BITS.
                              //     Vi sono riportate le distanze dalle stazioni di diramazione
                              // Polimetriche di  DIRAMAZIONE o ALLACCIAMENTO:
                              //   - NumStazioni strutture di tipo PO_STAZ.
                              //   - NumStazDiram strutture di tipo PO_DIRA.
                              //   - Matrice triangolare di elementi di NumBits BITS.
                              //     Comprende (NumStazioni - 2)(NumStazioni -1)/2 elementi
                              //     Vi sono riportate le distanze (non sotto forma di delta)
                              // NumBits e' sempre > 0 ed e' sempre impostato 
   signed int  Base     : 9;  // Valore da sottrarre ad ogni elemento della polimetrica
   BIT  SocietaConcessa : 7;  // Codice societa' concessa (0 per FS)
   WORD Zone               ;  // Zone della polimetrica (solo le polimetriche di allacciamento ne hanno piu' di una)
                              // Si utilizza un bit per zona . Zona 15 = Tutte le cumulative

   // Questa e' una funzione di preferenza tra polimetriche. Potrebbe divenire piu' sofisticata
   BOOL PreferibileA(POLIMETRICA & a){ return TipoPoli > a.TipoPoli;};

   BOOL Contiene(ID Staz); // Mi dice se una stazione appartiene alla polimetrica;

   BOOL Concessa(){ return TipoPoli == CUMULATIVA || TipoPoli == MARE_CUM  || TipoPoli == URBANA_CUM  ;};

   struct PO_STAZ {    // 5 bytes * 5000 = 25K
   // Vettore di distanze ASSOLUTE dalla prima stazione della polimetrica
   // Sono comprese solo le stazioni VALIDE
      BIT Id        : 13;  // Id Stazione
      BIT Km        : 11;  // Distanza dalla prima stazione della polimetrica
      BIT Livello   :  3;  // Livello del ramo (0 = ramo principale)
      BIT Radice    :  9;  // Indice della radice nella polimetrica (0 = ramo principale)
      BIT Diram     :  1;  // Se TRUE e' una stazione di diramazione
   };                
   inline PO_STAZ * StazioniDellaPolimetrica();

   struct PO_DIRA {    // 4 bytes * 500 = 2K
   // Sono comprese solo le stazioni di diramazione (o di allacciamento) VALIDE
      BIT Id           : 13;  // Id Stazione
      BIT Idx          :  9;  // Indice stazione (nella polimetrica)
   };                
   inline PO_DIRA * StazioniDiramazione();

   // Torna BigDIST se non appartengono entrambi alla polimetrica
   DIST DistanzaTra(ID From,ID To){ BYTE Verso; return DistanzaTra(From,To,Verso); };
   DIST DistanzaTra(ID From,ID To,BYTE & Verso); 
   DIST DistanzaTra(int Idx1,int Idx2,struct POLIMETRICA::PO_STAZ * Sp); 

};
struct POLIMETRICA_SU_FILE : public POLIMETRICA {  // 70 Bytes
   char     Nome[11]        ;
   char     Descrizione[51] ;
};
class F_POLIM : public FILE_FIX {
   public:
   // Costruttore
   F_POLIM(const char* NomeFile,ULONG Size=4*sizeof(POLIMETRICA_SU_FILE)) : 
      FILE_FIX(NomeFile,sizeof(POLIMETRICA_SU_FILE),Size){};

   // Definizione operatori
   POLIMETRICA_SU_FILE&  RecordCorrente(){ return *(POLIMETRICA_SU_FILE*) RecordC; };
   POLIMETRICA_SU_FILE&  operator [](WORD Indice){ Posiziona(Indice); return *(POLIMETRICA_SU_FILE*) RecordC; };
};


//----------------------------------------------------------------------------
// PERCORSO_GRAFO
//----------------------------------------------------------------------------
// Questa classe e' il modo basilare di rappresentare i percorsi sul grafo
class _export PERCORSO_GRAFO {                                               // "ne TRC"
   public:

   ARRAY_ID Nodi;                 // Array con i nodi del percorso
                                  // Contiene anche stazioni non nodali:
                                  // Origine
                                  // Destinazione
                                  // Punti di inversione

   ARRAY_ID NodiCorretti;         // Idem dopo le correzioni per eccezioni normative
                                  // Vuoto se il percorso e' inibito o se NumeroCorrezioni == 0

   WORD     NumeroCorrezioni;     // Numero correzioni apportate 

   LONG Len();                    // Lunghezza in Km del percorso: Fast per algoritmi

   int  FaCorrezione();           // Effettua una correzione: 
                                  // 0 = Non ho (Piu') Correzioni da fare
                                  // 1 = Ho effettuato correzione
                                  // 2 = Il percorso e' inibito alla tariffazione

   DATI_TARIFFAZIONE & LunghezzaTariffabile(BYTE Modalita= 0);
   // Modalita' (per ora) : 
   //   0 : Non gestisce eccezioni tariffarie ( modalita' usata da Viaggio libero ) ma gestisce le inibizioni
   //   1 : Gestisce eccezioni tariffarie concordate con Cipriano / CVB ( modalita' usata da motore )
   // E' usata anche per individuare i percorsi generati da viaggio libero: per essi
   // e' soppresso il tentativo di individuare un precedente percorso equivalente.

   BOOL operator == (PERCORSO_GRAFO & b);
   
   // Chiama risolvi e prolunga il percorso fino ad A
   // Nota: Le stazioni non nodali non necessarie sono eliminate direttamente dall' algoritmo
   BOOL Prolunga(ID A);
   
   // Set di un percorso dato un array di vincoli; Torna FALSE su errore
   // Nota: Le stazioni non nodali non necessarie sono eliminate direttamente dall' algoritmo
   BOOL Set(ARRAY_ID& NodiViaggio,BOOL SegnalaErrore = TRUE);

   // Idem ma i vincoli sono espansi in due fasi: Nella prima si utilizzano le polimetriche per 
   // individuare al meglio dei vincoli addizionali intermedi, nella seconda si chiama
   // il precedente metodo usando anche i vincoli addizionali
   BOOL SetUtilizzandoPolimetriche(ARRAY_ID& NodiViaggio,BOOL SegnalaErrore = TRUE);

   // Questo metodo elimina eventuali stazioni non nodali anomale
   void Normalizza();

   void Clear(){ Nodi.Clear(); Lg = -1;};

   // Costruttori
   PERCORSO_GRAFO(){ Lg = -1; Istr = NULL; NumeroCorrezioni=0; };
   PERCORSO_GRAFO(ARRAY_ID & NodiViaggio){ Lg = -1; Istr = NULL; NumeroCorrezioni=0; Set(NodiViaggio); };
   
   // Trace dei nodi del percorso
   void Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   // Questo e' un trace del percorso piu' breve tra due nodi
   static void TraceShortest(ID Da,ID A, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   class PERCORSO_INSTRADAMENTI * Istr;

   // Questo metodo determina le stazioni vincolanti del percorso
   // sono tornati gli indici in Nodi
   BOOL DeterminaStazioniVincolanti(ARRAY_ID & IdxVincoliOrigine,ARRAY_ID & IdxVincoliDestinazione);

   BOOL IsPuntoDiInversione( int I); // Mi dice se il nodo Iesimo e' un punto di inversione del percorso

   // Questo metodo determina QUALI ferrovie concesse sono state utilizzate per il percorso grafo. 
   // Puo' tornare un ID per indicare che , causa un collegamento diretto tra due ferrovie concesse,
   // non e' possibile tariffare il percorso (l' ID e' quello della stazione di passaggio
   // tra le due ferrovie concesse )
   ID DeterminaFerrovieConcesseUtilizzate(ARRAY_ID & FerrovieConcesse);


   protected :
   short Lg; // Buffer lunghezza fast del percorso: Inizialmente = -1 per indicare che va ricalcolata

   private:
   int VincolanteOrigine(WORD IdxOrig);
   int VincolanteDestinazione(WORD IdxDest);

};

//----------------------------------------------------------------------------
// PERCORSO_INSTRADAMENTI
//----------------------------------------------------------------------------
class PERCORSO_INSTRADAMENTI {                                       // "ne TRC"
   public:

   enum STATO {NON_VALORIZZATO,NON_TARIFFABILE, TARIFFABILE
      ,NO_PERCORSO_POLIMETRICHE  // Errore perche' non sono riuscito a collegare le stazioni via polimetriche
      ,TROPPE_POLIMETRICHE       // Errore: ci vogliono trope polimetriche per collegare le stazioni, superrato limite interno
      ,NO_ISTRADAMENTO           // Errore: non riuscito ad individuare l' instradamento
      ,SOLO_CUMULATIVO           // Ha servizio cumulativo MA NON tratte FS
      ,TRE_CUMULATIVO            // Ha tre o piu' tratte cumulative
   } Stato;

   struct VINCOLO {
      WORD Idx;                  // Indice in Percorso del vincolo (o della stazione precedente se Offs > 0)
      WORD Offs;                 // 0 se il vincolo e' gia' presente in Percorso 
                                 // Altrimenti offset nel proprio ramo del vincolo 
      ID   Id;                   // Id del vincolo
      WORD Idx1;                 // Indice minimo  equivalente al vincolo in percorso
      WORD Idx2;                 // Indice massimo equivalente al vincolo in percorso
      ID   Inibita;              // Se valorizzato vuol dire che non si deve utilizzare tale stazione (inibita per diramazione condizionata)
      void Trace(const STRINGA& Msg){TRACESTRING((Msg + VRS(Idx) + VRS(Offs) + VRS(Id) +VRS(Idx1) + VRS(Idx2) ));};
   } ;

   ARRAY_ID Percorso;                       // Percorso Grafo originale
   ARRAY_ID StazioniDiInstradamento;        // Stazioni di instradamento "Leggibili"
   ARRAY_DINAMICA<VINCOLO> VincoliAssoluti; // Vincoli determinati matematicamente

   ID Origine;
   ID Destinazione;

   WORD MinPartenza;       // Se ha soluzioni collegate e' il minimo tempo prima della partenza della soluzione
   WORD NumPrimeSoluzioni; // Per determinare quante soluzioni mostrare a video per ogni percorso

   DATI_TARIFFAZIONE DatiTariffazione;

   BOOL operator ==( const PERCORSO_INSTRADAMENTI & Altro)const;


   BOOL Valorizza(ARRAY_ID & StazioniDiInstradamento);
   virtual BOOL Valorizza(PERCORSO_GRAFO & Percorso)=0;
   static PERCORSO_INSTRADAMENTI * CreaPercorso(); // Sostituisce il costruttore

   BOOL DeterminaPuntiDiInstradamento(PERCORSO_GRAFO & Percorso);

   virtual void StoreTo(    BUFR & To);
   virtual void ReStoreFrom(BUFR & From);

   void Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   // Questo metodo controlla che una data stazione "appartenga" (o meglio sia compatibile)
   // con la parte di percorso compresa tra due vincoli
   // Ritorna: 0 = Compatibile , 1 = Incompatibilita' forse sanabile , 2 = Incompatibilita' non sanabile
   ULONG ControllaCompatibilitaSuTratta(ID IdToTest, VINCOLO & V1, VINCOLO & V2);

   protected :
   PERCORSO_INSTRADAMENTI();
};

//----------------------------------------------------------------------------
// Struttura utilizzata per la determinazione dei punti di instradamento "leggibili"
//----------------------------------------------------------------------------
struct TRATTA_DI_INSTRADAMENTO {
   ID    Id1;          // Inizio Tratta
   ID    Id2;          // Fine Tratta
   BYTE  Sezionabile;  // Indica che puo' essere suddiviso
   // La suddivisione e' inibita:
   // - Se la tratta contiene solo polimetriche cumulative
   // - Se la tratta deriva da una divisione INTERNA di una polimetrica
   BYTE  Vincolante1;  // TRUE se la stazione 1 e' vincolante
   WORD  Km;           // Lunghezza in Km della tratta (Su polimetrica, prima dell' accorpamento)
   BYTE  Pol1;         // Punta a TRATTA (Polimetrica) in cui si trova Id1 (Su polimetrica, prima dell' accorpamento) 
   BYTE  Pol2;         // Punta a TRATTA (Polimetrica) in cui si trova Id2 (Su polimetrica, prima dell' accorpamento) 
   
   void Trace(const STRINGA& Msg){ TRACESTRING2(Msg, VRS(Id1)+VRS(Id2)+VRS(Sezionabile)+VRS(Km)+VRS(Pol1)+VRS(Pol2)+VRS(Vincolante1));};
};

//----------------------------------------------------------------------------
// PERCORSO_POLIMETRICHE
//----------------------------------------------------------------------------
class PERCORSO_POLIMETRICHE : public PERCORSO_INSTRADAMENTI {        // "ne TRC"
   public:

   struct TRATTA {
      ID   IdPolimetrica        ;
      ID   IdStazioneIngresso   ;
      ID   IdStazioneUscita     ;
      DIST Km                   ;
      BYTE Verso                ;
   };

   USHORT NumeroTratte;
   TRATTA Tratte[MAX_TRATTE_POLIMETRICA];
   virtual BOOL Valorizza(PERCORSO_GRAFO & Percorso);
   virtual void StoreTo(BUFR & To);
   virtual void ReStoreFrom(BUFR & From);

   void Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);

   // Questo metodo per la riconciliazione tra Grafo e Polimetriche
   // Ritorna un codice di errore (se > 0) ed un Modo con il seguente significato:
   // Modo = 0: Non inserito vincolo aggiuntivo, continuare lo stesso se ReasonCode == 0
   // Modo = 1: Inserito vincolo aggiuntivo
   // Modo = 2: Sostituire primo vincolo
   // Modo = 3: Sostituire secondo vincolo
   int IdentificaVincoloAggiuntivo(VINCOLO & Aggiuntivo,  VINCOLO & V0, VINCOLO & V1, VINCOLO & V2, int & Modo, ARRAY_ID & FerrovieConcesseCompatibili );

   // Questo metodo accorpa le polimetriche e mette l' instradamento in forma leggibile
   // Torna FALSE su errore
   BOOL AccorpaPolimetriche(PERCORSO_GRAFO & PercorsoG , ARRAY_DINAMICA<TRATTA_DI_INSTRADAMENTO> & TratteInstradamento, ARRAY_ID & FerrovieConcesseUtilizzate); 

   PERCORSO_POLIMETRICHE();
};

//----------------------------------------------------------------------------
// ELENCO_PERCORSO_GRAFO
//----------------------------------------------------------------------------
// Elenchi dinamici di Puntatori a PERCORSO_GRAFO
//----------------------------------------------------------------------------
typedef PERCORSO_GRAFO * P_PERCORSO_GR;
#define DIN_ARRAY ELENCO_PERCORSO_GRAFO
#define CLAS P_PERCORSO_GR
#define DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
#define CopiaMembro(_CLAS) (Beep(__FILE__,__LINE__,1),(P_PERCORSO_GR)NULL) // Vieto la copia
#include "dinarray.h"

//----------------------------------------------------------------------------
// ELENCO_PERCORSO_INSTRADAMENTI
//----------------------------------------------------------------------------
// Elenchi dinamici di Puntatori a PERCORSO_INSTRADAMENTI
//----------------------------------------------------------------------------
typedef PERCORSO_INSTRADAMENTI * P_PERCORSO_IS;
#define DIN_ARRAY ELENCO_PERCORSO_INSTRADAMENTI
#define CLAS P_PERCORSO_IS
#define DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
#define CopiaMembro(_CLAS) (Beep(__FILE__,__LINE__,1),(P_PERCORSO_IS)NULL) // Vieto la copia
#include "dinarray.h"


//----------------------------------------------------------------------------
// Intradamenti eccezionali polimetriche  
//----------------------------------------------------------------------------
#define NM_VINC 6
struct ISTRADAMENTO_ECCEZIONALE_CUM {
   ID     StzVincolante[NM_VINC]; // Origine, destinazione e max 4 vincoli intermedi
                                  // L' origine in chiave
   ID     Polimetrica           ; // Id della polimetrica
   BYTE   Verso                 ; // Verso con cui seguo la polimetrica
   WORD   Km                    ; // Km della polimetrica.
};
class  F_ISTRECC : public FILE_BS {
   public:
   // Costruttore
   F_ISTRECC(const char* NomeFile,ULONG Size=0) : FILE_BS(NomeFile,sizeof(ISTRADAMENTO_ECCEZIONALE_CUM),2,Size,0,FALSE){Init();};
   
   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(ID Stazione){ BUFR Wrk; Wrk.Store(Stazione); return Posiziona(Wrk) && KeyEsatta;};
   
   // Definizione operatori
   ISTRADAMENTO_ECCEZIONALE_CUM&  RecordCorrente(){ return *(ISTRADAMENTO_ECCEZIONALE_CUM*) RecordC; };
   ISTRADAMENTO_ECCEZIONALE_CUM&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(ISTRADAMENTO_ECCEZIONALE_CUM*) RecordC; };
   
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto ){ return CompareWord(Key1,Key2);};
};
struct ISTRADAMENTO_ECCEZIONALE_REG {
   ID     NodiStd[10]     ; // Percorso in forma standard (max 10 stazioni)
                            // La chiave e' la prima stazione
   BYTE   NumeroNodi      ; // Numero nodi validi di NodiStd
   BYTE   CodiceRegione   ; // Codice regione da utilizzare
};
class  F_ISTRECC_REG : public FILE_BS {
   public:
   // Costruttore
   F_ISTRECC_REG(const char* NomeFile,ULONG Size=0) : FILE_BS(NomeFile,sizeof(ISTRADAMENTO_ECCEZIONALE_REG),2,Size,0,FALSE){Init();};
   
   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(ID Stazione){ BUFR Wrk; Wrk.Store(Stazione); return Posiziona(Wrk) && KeyEsatta;};
   
   // Definizione operatori
   ISTRADAMENTO_ECCEZIONALE_REG&  RecordCorrente(){ return *(ISTRADAMENTO_ECCEZIONALE_REG*) RecordC; };
   ISTRADAMENTO_ECCEZIONALE_REG&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(ISTRADAMENTO_ECCEZIONALE_REG*) RecordC; };
   
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto ){ return CompareWord(Key1,Key2);};
};

//----------------------------------------------------------------------------
// Ecccezioni di normativa
//----------------------------------------------------------------------------
// Tratti  doppi da non considerare
// Tratti Iniziali o finali da non considerare
// ATTENZIONE: IL FILE DEVE ESSERE IN ORDINE DI TIPO
struct ECCEZ_NORMATIVA {
   BIT    Tipo         :3 ; // 1 = Tratta doppia tariffabile tra Id1 ed Id2, che segue il percorso indicato
                            //     Id1 deve essere un nodo (per definizione: il resto del percorso deve differire)
                            //     Id2 e' la stazione di inversione del percorso
                            //     Se ValeEsatto e' vero la tratta doppia si tariffa
                            //     Altrimenti la tratta doppia viene tagliata via
                            // 2 = Non considerare tratti che iniziano o finiscono con il percorso indicato
                            //     In Id1 metto la stazione iniziale del percorso
                            //     In Id2 metto il NUMERO di stazioni da eliminare
                            // 3 = La stazione Id1 e' inibita, tranne che per le relazioni qui riportate
                            //     In Id1 metto la stazione iniziale del percorso (non necessariamente nodo)
                            //     In Id2 metto la stazione finale (non necessariamente nodo)
                            // 4 = Sostituzione di percorso. Il record seguente (sempre con
                            //     tipo 4 ed Id1 ed Id2 a 0 ) contiene il percorso da sostituire
                            // 5 = Inibizione di relazione. La coppia O/D Id1 -> Id2 e' inibita
                            //     Id1 < Id2 (per non dover caricare il doppio di relazioni)
                            //     Se Id2 e' 0 la stazione Id1 e' inibita per tutte le stazioni
                            // 6 = Inibizione di gruppo. La stazione Id1 e' inibita verso tutte le
                            //     stazioni del suo stesso gruppo
                            //     Id2 rappresenta il gruppo di appartenenza della stazione
                            //     NB: Vi e' anche il concetto di sottogruppo: Le stazioni con
                            //     gruppo > 1000 sono considerate come appartenenti ad un
                            //     sottogruppo del gruppo principale corrispondente (= modulo 1000).
                            //     Si ha inibizione crociata tra sottogruppo e gruppo principale, 
                            //     ma non tra due stazioni di differenti sottogruppi
                            //     NB: Le stazioni possono appartenere a differenti gruppi / sottogruppi
   // Per le sole sostituzioni di percorso
   BIT    ValeInizio   :1 ; // L' eccezione vale se all' inizio del percorso 
   BIT    ValeFine     :1 ; // L' eccezione vale se alla fine del percorso   
   BIT    ValeInMezzo  :1 ; // L' eccezione vale se in mezzo al percorso     
   BIT    ValeEsatto   :1 ; // L' eccezione vale se esattamente il percorso  
   BIT    ValeInverso  :1 ; // L' eccezione vale anche per il percorso inverso
   // ....................................
   ID     Id1             ; 
   ID     Id2             ; 
   BYTE   DimPercorso     ; // Numero di stazioni del percorso
   ID     Percorso[MAXECC]; // Max MAXECC stazioni
   void Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
};
class  F_ECCZ_NORM : public FILE_FIX {
   public:
   // Costruttore: Di norma il file sta tutto in memoria
   F_ECCZ_NORM(const char* NomeFile,ULONG Size=0);
   
   // Definizione operatori
   ECCEZ_NORMATIVA&  RecordCorrente(){ return *(ECCEZ_NORMATIVA*) RecordC; };
   ECCEZ_NORMATIVA&  operator [](DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(ECCEZ_NORMATIVA*) RecordC; };

   // Questo metodo ritorna la dimensione dei buffers riservati in memoria
   ULONG DimBuffers(){return FILE_FIX::DimBuffers() + (StazioniConInibizione.Dim() / sizeof(ID)) + (GrandiCorrezioni.NumBits() / 8);};

   // ------------------------------------------------
   // Variabili di appoggio: Calcolate dal costruttore 
   // ------------------------------------------------
   void __fastcall DeterminaIdx(int Tipo, int & Minimo, int & Massimo); // Determina il minimo e massimo di scansione per tipo

   // Questi sono insiemi di stazioni con eccezioni tariffarie
   ARRAY_ID                      StazioniConInibizione; // Elenco delle stazioni la cui vendita e' generalmente inibita 
   ARRAY_ID                      CoppieConInibizione;   // Elenco delle stazioni la cui vendita e' inibita per coppie O/D 
   ARRAY_ID                      StazioniDiGruppoInibiz;// Elenco delle stazioni che appartengonbo ad un gruppo di inibizione
   SET                           GrandiCorrezioni;      // Stazioni per cui si deve rivedere il percorso 

   private:
   int IdxTipi[20];    // Indice ai records di un certo tipo  (OK perche' ordinato per tipo record)

};

//----------------------------------------------------------------------------
// Correzioni  alla distanza sui rami per tratte marine e casi particolari
//----------------------------------------------------------------------------
// Per la funzione DistanzaTra
struct CORREZIONI_RAMI {
   ID Id1;
   ID Id2;
   ID KmDaUsare;
};

//----------------------------------------------------------------------------
// GRAFO
//----------------------------------------------------------------------------
// Questa classe contiene gli algoritmi di risoluzione del grafo
// della rete FS.
// I percorsi generati sono normalizzati.
//----------------------------------------------------------------------------
class GRAFO {                                                        // "ne TRC"
   
   public:

   GRAFO(const STRINGA& Path);   // Carica dai files
   ~GRAFO();

   void Clear();
   
   // ..............................................................
   // Risoluzione secca problema percorso minimo tra due nodi
   // ..............................................................
   // Non modifica PercorsoMinimoCorrente !
   BOOL Risolvi(ID Da, ID A, int MaxDist = -1); // Se imposto maxdist la soluzione e' valida per tutte le stazioni nodali con distanza <= MaxDist

   // Metodo analogo ma la soluzione e' trovata sulle polimetriche
   // Vuole un SET con l' insieme delle ferrovie concesse utilizzabili
   BOOL RisolviPolimetriche(ID Da, ID A, ARRAY_ID & FerrovieConcesseCompatibili);

   // Questo metodo chiama il precedente e determina l' insieme di stazioni
   // di diramazione associato alla soluzione di un problema sulle polimetriche
   // In Out Mette la stazione finale ma non l' iniziale. Torna FALSE su errore
   BOOL IdentificaStazioniDiDiramazione(ID Da, ID A, ARRAY_ID & FerrovieConcesseCompatibili, ARRAY_ID & Out);

   // Macro utile quando serve solo la distanza: torna BigDist  su errore
   // Calcola in Particolare: Km Multistazioni e tutti i ProgKm sulle fermate
   // NB: Su alcune tratte la distanza e' corretta, tramite apposito file KmECCZ.MIO .
   DIST __fastcall DistanzaTra(ID Da, ID A);

   // Versione esportata : Torna -1 su errore
   static int _export CalcolaDistanzaTra(ID Da, ID A);

   // ..............................................................
   // Metodi di risoluzione complessiva del GRAFO
   // ..............................................................
   // Lascia impostata nei NODI le minime distanze verso la stazione A
   BOOL RisolviGrafo(ID A);                   // Questa per tutto il grafo
   // Questa estende ad una stazione non nodale. Se la stazione e' nodale ritorna subito
   // Vuole che sia stata prima chiamata Risolvi (Da,A) o RisolviGrafo.
   // Se l' estensione non e' possibile la stazione avra' Progressivo <> ProgressivoCorrente
   void __fastcall EstendiRisolvi(ID Da);

   // Per abbonamenti regionali
   //   0    = Applicare tariffa nazionale
   //   0xFF = NON SO (disabilitare vendita abbonamenti)
   BYTE CodiceTariffaRegionale(PERCORSO_GRAFO & Percorso, BOOL Cumulativo);
   
   // ........................................................
   // Miscellanea
   // ........................................................
   
   void TraceId(const STRINGA& Msg, ID Id, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA); 
   
   // Accessors
   STAZ_FS     & operator[](ID Indice){return StazioniGrafo[Indice];};
   

   // Di solito esiste una unica INSTANCE di GRAFO
   static GRAFO * _export Grafo;
   static GRAFO & _export Gr();

   // Questa funzione mi dice se x e' sul minimum path tra Da ad A
   // Al momento (Nov 1996) non e' piu' usata per motivi di performance, ma la lascio 
   // perche' potrebbe tornare utile
   BOOL SulMinimumPath(ID x,ID Da, ID A);

   // Questa funzione mi dice se due stazioni hanno un ramo in comune.
   // Ritorna: TRUE se lo hanno
   // Ha anche due parametri in cui ritorna il ramo in comune
   // ed i Km tra le due stazioni (negativi se le le due stazioni sono in
   // verso opposto al ramo).
   BOOL IdentificaRamo(ID Staz1,ID Staz2, ID & Ramo,int & Km);

   // Dati utilizzati da Risolvi o RisolviPolimetriche
   static DIST Limite; // E' il limite usato nell' algoritmo: i nodi con distanza > limite non sono stati realmente risolti
   BYTE   ProgressivoCorrente; // Per Risolvi
   ID     A_Valido;

   // Abilitazione dei rami cumulativi (99 = abilita SOLO cumulativo, per algoritmi interni)
   ULONG AbilitaCumulativo;

   // ..............................................................
   // Area con i dati del grafo
   // ..............................................................
   // STAZ_FS
   STAZ_FS      *    StazioniGrafo     ;
   int               TotStazioniGrafo  ;
   BUFR              ExtStazioniGrafo  ;
   // RAMO           
   RAMO         *    Rami              ;
   int               TotRami           ;
   BUFR              ExtRami           ;
   // POLIMETRICA    
   POLIMETRICA  *    Polim             ;
   int               TotPolim          ;
   BUFR              ExtPolim          ;
   F_POLIM      *    FilePolim         ;
   // ISTR_COND      
   ISTR_COND    *    IstrCond          ;
   int               TotIstrCond       ;

   CORREZIONI_RAMI * CorrezioniRami    ;
   int               TotCorrezioniRami ;

   // PERCORSO_INSTRADAMENTI
   ELENCO_PERCORSO_INSTRADAMENTI Instradamenti;

   // PERCORSO_GRAFO
   ELENCO_PERCORSO_GRAFO         PercorsiGrafo;

   // Istradamenti eccezionali del cumulativo
   F_ISTRECC      *              IstrEccez    ;

   // Istradamenti eccezionali per cui debbo specificare il codice tariffa regionale
   F_ISTRECC_REG  *              IstrEccez_REG;

   // Eccezioni di normativa
   F_ECCZ_NORM    *              EccezioniNormativa;

};



//----------------------------------------------------------------------------
// METODI INLINE
//----------------------------------------------------------------------------
inline STAZ_FS::PRIMO_V * STAZ_FS::PrimiVicini()const{
   #ifdef ENABLE_INNER // Abilita il trace sugli inner LOOPS
   PROFILER::Conta(3);
   #endif
   return (PRIMO_V*)(GRAFO::Grafo->ExtStazioniGrafo.Dati + PDati);
};
inline STAZ_FS::P_POLI * STAZ_FS::Polimetriche()const{return (P_POLI*)(PrimiVicini()+NumeroPrimiVicini);};
inline STAZ_FS::D_CONCESSE * STAZ_FS::Concesse()const{return (D_CONCESSE*)(Polimetriche()+NumeroPolimetriche);};
inline STAZ_FS::D_INSTR & STAZ_FS::Instradamento()const{return *(D_INSTR*)(Concesse()+NumeroConcesse);};
inline STAZ_FS::D_DIRAM & STAZ_FS::Diramazione()const{
   if(DiInstradamento) return *(D_DIRAM*)(&(Instradamento())+1); else return (D_DIRAM&)Instradamento();
};
inline STAZ_FS::PRIMO_V & STAZ_FS::PrimoVicino( BYTE Idx ) const { return PrimiVicini()[Idx]; };
inline STAZ_FS & STAZ_FS::NodoPrimoVicino( BYTE Idx ) const { return (*GRAFO::Grafo)[PrimoVicino(Idx).Id]; };
inline ISTR_COND & STAZ_FS::IstrCond(){return GRAFO::Grafo->IstrCond[IsCnd];};

inline ISTR_COND & STAZ_FS::D_DIRAM::IstrCondInibit(){return GRAFO::Grafo->IstrCond[IsCndInibit];};
inline RAMO::STAZIONE_R * RAMO::StazioniDelRamo(){return (STAZIONE_R*)(GRAFO::Grafo->ExtRami.Dati + PDati);};
inline POLIMETRICA::PO_STAZ * POLIMETRICA::StazioniDellaPolimetrica() {return (PO_STAZ*)(GRAFO::Grafo->ExtPolim.Dati + PDati);};
inline POLIMETRICA::PO_DIRA * POLIMETRICA::StazioniDiramazione(){return (PO_DIRA*)(StazioniDellaPolimetrica()+NumStazioni);};

#endif
