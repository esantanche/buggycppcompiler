//========================================================================
// FILE_RW
//========================================================================

#ifndef HO_FILES_HPP
#define HO_FILES_HPP

#ifndef HO_BASE_HPP
#include "base.hpp"
#endif

// EMS001 typedef LHANDLE HFILE;
// EMS002 sostituisco globalmente HFILE con HFILEOS2
typedef unsigned long BOOLOS2; // EMS003
typedef LHANDLE HFILEOS2;

#ifndef HO_ELENCO_S_H
#include <elenco_s.h>
#endif

// Funzioni per il sort rapido dei file BS
typedef int (*FILE_SORT_FUNC)( const void *a, const void *b);

//----------------------------------------------------------------------------
// Struttura per rappresentare Data ed ora dei files
//----------------------------------------------------------------------------
// Occupa 4 Bytes
struct _export F_DTM {

   BIT Sec2    : 5; // Secondi  /2
   BIT Minuti  : 6;
   BIT Ore     : 5;
   BIT Giorno  : 5;
   BIT Mese    : 4;
   BIT _Anno   : 7; // Anno - 1980

   int Anno() {return _Anno + 1980;};
   int Secondi() {return Sec2 * 2;};

   STRINGA TimeStamp();
   STRINGA Ora();
   STRINGA Data();

   operator ULONG () const { return * (ULONG*)this;};
   BOOL operator >  (F_DTM b) { return ULONG(THIS) >  ULONG(b) ; };
   BOOL operator <  (F_DTM b) { return ULONG(THIS) <  ULONG(b) ; };
   BOOL operator >= (F_DTM b) { return ULONG(THIS) >= ULONG(b) ; };
   BOOL operator <= (F_DTM b) { return ULONG(THIS) <= ULONG(b) ; };
   BOOL operator == (F_DTM b) { return ULONG(THIS) == ULONG(b) ; };
   BOOL operator != (F_DTM b) { return ULONG(THIS) != ULONG(b) ; };
   F_DTM& operator ++ (); // Giorno successivo
   F_DTM& operator -- (); // Giorno precedente

   F_DTM& AddMinuti(USHORT Mn); // Aggiunge un dato numero di minuti alla data ed ora

   void Adesso(); // Imposta i campi con l' istante attuale
//<<< struct _export F_DTM {
};

//----------------------------------------------------------------------------
// Api per ottenere i dati di uno o piu' files leggendoli da directory
//----------------------------------------------------------------------------
// Attenzione: per vedere se un singolo file esiste e' decisamente piu' veloce utilizzare TestFileExistance
struct _export RO_INFO {
   char  Nome[40];  // Senza Path ! Se piu' lungo tronca
   F_DTM Data  ;
   ULONG Size  ;
   BIT   Hidden     ;
   BIT   System     ;
   BIT   ReadOnly   ;
   BIT   Archived   ;
   BIT   Directory  ;
};
// Il costruttore esegue la ricerca su directory
struct _export RO_INFO_LIST {
    RO_INFO & operator[] (UINT Indice){ return Dati[Indice]; };
    ULONG Dim(){return Size;};
    RO_INFO_LIST(const STRINGA& PatternDaCercare, BOOL AncheHidden = FALSE, BOOL AncheDirectories = FALSE);
    virtual ~RO_INFO_LIST();

    int Rc; // Mi informa che ho avuto un errore nella ricerca

    // Funzioni di sort
    void SortNome();
    void SortData();
    void SortSize();

    STRINGA Path;    // Si applica a tutti i files
    STRINGA Pattern; // Lo salvo

    protected:
    ULONG Size;
    RO_INFO * Dati;

};

//----------------------------------------------------------------------------
// FILE_RO : Files in sola lettura
//----------------------------------------------------------------------------
class _export FILE_RO {
   public:

   FILE_RO( const char * NomeFile); // Esegue direttamente la Open
   virtual ~FILE_RO(); // Esegue la Close

   STRINGA Nome;  // Comprende Path

   const HFILEOS2 FileHandle(){return Handle ;};
   ULONG FileSize();
   F_DTM FileTms();

   BOOL  Posiziona(ULONG Posizione);  // Posiziona il file (FALSE se errore)
   BOOL  Posiziona();                 // Posiziona alla fine del file

   // Legge i dati e li mette in un BUFR (allocato con new)
   // in caso di errore : ritorna NULL
   BUFR * Leggi(ULONG NumBytes);                 // Legge sequenzialmente
   BUFR * Leggi(ULONG NumBytes,ULONG Posizione); // Posiziona e legge

   // Legge i dati e li mette in un BUFR preallocato (casomai fa il redim)
   // in caso di errore : ritorna FALSE
   BOOL Leggi(ULONG NumBytes,BUFR & Out);                 // Legge sequenzialmente
   BOOL Leggi(ULONG NumBytes,ULONG Posizione,BUFR & Out); // Posiziona e legge

   // Legge i dati e li mette in un' area esistente.
   // Ritorna opzionalmente il numero di bytes letti
   BOOL Leggi(void * Dati, ULONG NumBytes, ULONG & BytesLetti = *(ULONG*)NULL);

   // Legge i dati dalla linea posizione attuale ; Max 1024 caratteri
   // Attenzione: equivale ad una gets + sscanf, non proprio ad una scanf
   // Se i dati non sono terminati da un CRLF non funziona bene
   int  scanf(const char * Format, ...);

   char  * gets(char * Buffer, int MaxLinea=1024); // Legge i dati alla posizione attuale;
   BUFR  * gets(int MaxLinea = 1024);             // Legge una linea alla posizione attuale;
   BOOL  gets(STRINGA & Linea,int MaxLinea = 1024);// Legge una linea alla posizione attuale e la torna come stringa; Torna FALSE ad EOF od errore
   // Nota: il numero di caratteri letti e' MaxLinea.
   // Linee piu' lunghe di MaxLinea non sono gestite correttamente
   // Fisicamente vengono sempre letti MaxLinea Caratteri

   ULONG PosizioneCorrente();
   DWORD Rc(){return rc;};

   // Queste due funzioni solo per gestioni particolari: Di solito non servono
   WORD Open();  // Apre il file (gia' aperto dal costruttore)
   void Close(); // Chiude il file (chiuso anche dal distruttore)

   protected:


   FILE_RO(){}; // Non esegue direttamente la Open

   HFILEOS2 Handle;
   ULONG FSize;
   F_DTM FTms;
   DWORD OpenAction,OpenFlags; // Parametri di apertura OS2
   DWORD rc;                   // Ultimo errore OS/2

//<<< class _export FILE_RO {
};

//----------------------------------------------------------------------------
// FILE_RW : Files in lettura - scrittura
//----------------------------------------------------------------------------
class _export FILE_RW : public FILE_RO {

   public:
   FILE_RW( const char * NomeFile,BOOL Lazy=TRUE);
   // Esegue direttamente la Open (Exclusive !)

   // Scrive i dati prendendoli da un BUFR
   // in caso di errore : ritorna FALSE
   BOOL Scrivi(const BUFR & In);               // Scrive alla posizione corrente;
   BOOL Scrivi(const void * Dati, ULONG Dim);  // Idem

   // Allunga o Tronca il file
   // in caso di errore : ritorna FALSE
   BOOL SetSize(ULONG NewSize);

   // Metodo che Forza la data - ora del file
   // in caso di errore : ritorna FALSE
   // La data sara' nuovamente alterata se si richiama Scrivi.
   BOOL SetTms(F_DTM Tms);

   // Metodo che fa un rename del file.
   // Il file puo' essere aperto o chiuso, e viene lasciato come era.
   // Si deve specificare anche il path: se differente il file viene spostato.
   // in caso di errore : ritorna FALSE
   BOOL Rename(const char* NomeFile);


   int  printf(const char * Format, ...);    // Scrive alla posizione corrente;
   // Attenzione: a differenza della printf normale NON traduce i caratteri \n in \r\n
   int  puts(const char * Dati){             // Scrive una linea alla posizione corrente;
      return printf("%s\n",Dati);
   };

//<<< class _export FILE_RW : public FILE_RO {
};

//----------------------------------------------------------------------------
// FILE_FIX :  File in sola lettura con accesso per numero record
//----------------------------------------------------------------------------
// Puo' essere tirato tutto od in parte in mempria specificando un buffer
// (indicato come numero bytes da tirare dentro con un singolo I/O)
class  _export FILE_FIX : protected FILE_RO {

   public:
   FILE_FIX( const char* NomeFile,ULONG RecLen,ULONG PageSize);
   virtual ~FILE_FIX();

   ULONG NumRecordsTotali() const {return NumRecords;};  // Numero totale di records del file
   ULONG NumRecordCorrente()const {return NumRec; };     // Numero record corrente (da 0 a NumRecordsTotali - 1)
   ULONG Dim() const {return NumRecords;};               // Per permettere di utilizzare le macro FORALL

   // Posizionamento per numero record. Ritorna FALSE se fuori range od in caso di errore
   // Se torna FALSE non e' posizionato ad alcun record (RecordC = NULL);
   virtual BOOL Posiziona(ULONG NumeroRecord);
   BOOL ToEnd();

   // Posizionamento al record successivo
   BOOL Next(){return Posiziona(NumRec + 1);};

   // Questo deve rimanere visibile
   FILE_RO::FileSize;
   FILE_RO::FileTms;
   FILE_RO::Nome;
   FILE_RO::FileHandle;
   FILE_RO::Rc;

   // Questa funzione permette di cambiare dinamicamente la dimensione del buffer.
   // Utile quando si passa da fasi di scansione sequenziale a fasi di accesso
   // random e viceversa (gli accessi fisici sono sempre fatti per Pagina)
   void ModificaPageSize(ULONG NewPageSize);

   // Questo metodo ritorna la dimensione dei buffers riservati in memoria
   ULONG DimBuffers(){return FileBuffer.Dim();};

   // ----------------------------------------------------
   // FUNZIONI PER AGGIORNAMENTO ARCHIVI
   // ----------------------------------------------------
   // PERMETTI SCRITTURA
   // QUESTA FUNZIONE RIAPRE IL FILE IN MODO RW/ESCLUSIVO
   // ATTENZIONE:
   // E' pensata per aggiornamenti degli archivi, potrebbe
   // causare problemi se usata nella normale operativita'.
   // (in caso di cadute di macchina).
   // Il livello di sicurezza e' quello di un file piatto
   // E' prerequisito per AddRecord,ModifyRecord,ModifyCurrentRecord;
   BOOL PermettiScrittura();       // Prerequisito per le altre
   // Clear puo' essere usata in alternativa a PermettiScrittura;
   // Chiama PermettiScrittura, azzera la dimensione del file, e pone un Header all' inizio
   // EMS002 cambio BOOL in BOOLOS2 per problemi di differente definizione
   // del BOOL di OS2 e di quello di Windows
   virtual BOOL Clear(const STRINGA & Descrizione= NUSTR,BOOLOS2 Header=TRUE);
   // Funzioni di modifica od aggiunta di un record.
   BOOL ModifyRecord(ULONG NumeroRecord,const BUFR & NewRec); // Modifica il record NumeroRecord
   // Viene aggiunto alla fine del file  ; Per inserzioni di massa riposizionare alla fine
   // del file dopo ogni insert: la scrittura sara' bufferizzata. Usare AddRecordToEnd
   BOOL AddRecord(const BUFR & NewRec);
   BOOL AddRecord(const void * Dati, ULONG Dim);  // Idem
   BOOL AddRecordToEnd(const BUFR & NewRec); // Come la precedente, ma posiziona prima alla fine
   BOOL AddRecordToEnd(const void * Dati, ULONG Dim);  // Idem

   // Nota: ModifyRecord lascia il file posizionato sul nuovo record.
   // AddRecord non modifica la posizione del file
   // AddRecord corrompe i files a chiave: non eseguire seek prima di avere risortato il file
   // o aver eseguito un Init().
   BOOL ReSort(FILE_SORT_FUNC SortFunction,BOOL Header=TRUE); // Sort fisico del file
   // Questa funzione puo' essere usata anche per i FILE_BS, e' veloce ma :
   //   - Carica tutto il file in memoria
   //   - Ha bisogno di una funzione di sort separata (puo' essere fonte di errori per i FILE_BS)
   //
   // La funzione seguente rende permanente la modifica diretta del record corrente
   // Deve essere fatta subito dopo il la modifica, e vale finche' non si riposiziona il file (e viene caricato un altro buffer)
   // ATTENZIONE: anche le modifiche agli altri records dello stesso buffer sono permanenti.
   // Se si modifica la chiave per un record BS poi si deve effettuare un RESORT
   BOOL ModifyCurrentRecord();
   void Flush(); // Forza lo scarico del buffer

   STRINGA DescrizioneSuFile(); // Se presente un Header torna la descrizione come letta da file (altrimenti EMPTYSTR)
   BOOL    HaHeader(){return FileHeader != NULL;}; // Vero se il file ha un header

   // Questa funzione permette di modificare DIRETTAMENTE l' header del file.
   // E' anche possibile (e utile nella preparazione dei files) dichiarare direttamente un
   // file di tipo FIX o di tipo BS. I defaults lasciano i valori correnti
   // Torna FALSE su errore.
   // Si noti che e' possibile aprire un BS come se fosse un FIX, ma non viceversa
   BOOL    PutHeader(
      int     Tipo        =-1,    // 1= FIX 2=BS
      int     RecLen      =-1,    // Record Length
      int     KeyLen      =-1,    // Key Length
      const  char * Descr =NULL); // Descrizione del file (MAX 39 caratteri + NULL)

   void InvalidateBuffer();   // EMS004 resa pubblica

   protected:

   BOOL totalmenteInMemoria;

   // dati
   BUFR   FileBuffer;         // Contiene una pagina letta dal file
   ULONG  NumRec;
   ULONG  FirstIdInBuffer;
   ULONG  LastIdInBuffer;
   void   * RecordC;
   BOOL   Riscritto; // Per bufferizzare ModificaRecord
   BOOL LimitaTrace;  // Viene impostata per limitare il trace nella scansione iniziale
   ULONG HeaderSize;  // Dimensione dell' Header iniziale (se presente)
   struct FIX_HEADER * FileHeader; // Opzionale: puo' essere assente ed in tal caso e' NULL

   // Informazioni interne
   const LONG RecordLength;
   LONG NumRecords;       // Puo' variare con l' Add
   const LONG PageSize;   // Puo' comunque essere modificata con la funzione ModificaPageSize;

   // EMS004 Win void InvalidateBuffer(); resa pubblica

   virtual void EndSort(); // Per i file_fix invalida i buffer, per i BS ricarica le chiavi

   // Queste sono le definizioni standard: valide solo se il formato fisico coincide con il formato logico
   // altrimenti e' a cura di chi subclassa definire le corrette operazioni
   // Definisce l' operatore [i] ( = accesso al record Iesimo) e [] (= accesso record corrente)
   // Inoltre definisce la funzione RecordCorrente (equivale a [] )
   // In queste definizioni l' accesso ai files viene tradotto in una struttura
   // equivalente alla struttura fisica sui files
   // Copiare su tutte le sottoclassi
   // R_STRU&  RecordCorrente(){ return *(R_STRU*) RecordC; };
   // R_STRU&  operator [](ULONG Indice){ Posiziona(Indice); return *(R_STRU*) RecordC; };

//<<< class  _export FILE_FIX : protected FILE_RO {
};

//----------------------------------------------------------------------------
// FILE_BS :  File in sola lettura con ricerca dicotomica
//----------------------------------------------------------------------------
// La chiave e' formata dai primi N bytes del record oppure fornita
// dalla funzione virtuale KeySet() (deve comunque essere a lunghezza fissa=KeyLength);
// Vi possono essere due tipi di accesso "diretto":
//   - Per indice del record
//   - Per chiave
// Ogni operazione di I/O tira dentro PageSize Bytes (bufferizzati localmente)
// La classe base e' FILE_FIX
//----------------------------------------------------------------------------
// Attenzione: Nelle sottoclassi si possono ridefinire le funzioni di
// caricamento della chiave dal file e di paragone tra due chiavi.
// Per limitazioni C++ in tal caso si deve impostare Inizializza
// a FALSE ed invocare la funzione Init() nel costruttore della
// sottoclasse
//----------------------------------------------------------------------------
class  _export FILE_BS : public FILE_FIX {
   public:
   FILE_BS( const char* NomeFile,ULONG RecLen,ULONG KeyLen,ULONG PageSize,
      ULONG ExtraLen=0,BOOL Inizializza=TRUE);

   BUFR KeyCorrente;       // Relativa Chiave (+ puntatore Id ed ExtraLen)

   // Posizionamento per numero record. Ritorna FALSE se fuori range od in caso di errore
   // Se torna FALSE non e' posizionato ad alcun record (RecordC = NULL)
   virtual BOOL Posiziona(ULONG NumeroRecord){
      if(!FILE_FIX::Posiziona(NumeroRecord))return FALSE;
      this->KeySet();return TRUE;};

   // Posizionamento per chiave (anche parziale)
   // Si posiziona al PRIMO record con Key <= della chiave (parziale) data.
   // Ritorna FALSE in caso di errore. (Normalmente mai)
   // Se torna FALSE non e' posizionato ad alcun record (RecordC = NULL) e KeyEsatta = FALSE;
   virtual BOOL Posiziona(const BUFR& Key);
   BOOL KeyEsatta;     // Impostata dopo le ricerche per chiave
   // E' gestita anche sulle ricerche per chiave parziale
   // (TRUE Se i bytes forniti sono esatti)

   // Questo metodo mi dice subito se una data Key e' fuori range
   // (cioe' minore della chiave minima o maggiore della massima)
   // Cio' permette di ridurre il numero degli accessi fisici
   // Se il metodo torna TRUE la chiave NON si trova certamente su FILE.
   // (Piu' precisamente : -1 = Minore di tutte le chiavi, +1 = Maggiore ...)
   // Altrimenti si deve accedere per vedere se esiste
   // Il metodo non cambia la posizione od altri dati del file
   int  FuoriRange(const BUFR& Key)const;

   // Questo metodo ritorna la dimensione dei buffers riservati in memoria
   ULONG DimBuffers(){return FileBuffer.Dim() + DatiIndice.Dim();};


   // ----------------------------------------------------
   // FUNZIONI PER AGGIORNAMENTO ARCHIVI
   // ----------------------------------------------------
   // Valgono considerazione analoghe a quelle fatte per il file FIX
   // Clear puo' essere usata in alternativa a PermettiScrittura;
   // EMS002 cambio BOOL in BOOLOS2 per problemi di differente definizione
   // del BOOL di OS2 e di quello di Windows
   virtual BOOL Clear(const STRINGA & Descrizione=NUSTR,BOOLOS2 Header=TRUE);
   // Clear azzera la dimensione del file, e pone un Header all' inizio
   BOOL ModifyRecord(const BUFR& Chiave,const BUFR & NewRec);
   // Modifica il record con chiave (attuale) = Chiave.
   // Se e' cambiata la chiave (cio' se su NewRec e' presente una
   // chiave differente dall' attuale) poi si dovra' chiamare ReSort
   // Fino a che non si chiama ReSort l' accesso al file potra' essere
   // alterato (causa la presenza di chiavi non in ordine).
   // Si puo' utilizzare anche il metodo di FILE_FIX per accedere per numero record
   // (e' consigliabile).

   // Funzioni di SORT:
   // - Nei casi comuni (chiave in testa al file) usare ResortFAST
   // - In alternativa: usare ReSort con parametri (bisogna scrivere una funzione di sort ad Hoc)
   // - Su files molto grandi (> usare ReSort senza parametri ed aspettare con pazienza.
   BOOL ReSort(); // Esegue un sort del file e lo riscrive (Torna FALSE su errore)
   // RESORT: QUESTA FUNZIONE E' LENTA.
   // Per velocizzarla un poco: meglio avere aperto il file con pagesize grande
   // In caso di cadute il file viene completamente danneggiato.
   // Si noti che un alternativa piu' veloce e' di utilizzare la funzione
   // di sort dei FILE_FIX (ma attenzione che e' facile fare errori).
   BOOL ReSort(FILE_SORT_FUNC SortFunction,BOOL Header=TRUE){return FILE_FIX::ReSort(SortFunction,Header);};
   // Questa e' veloce, ma funziona SOLO se la chiave e' in testa al record
   BOOL ReSortFAST();

   BOOL ControllaSequenza();  // Questa funzione controlla la sequenza del file
   // Ritorna TRUE se la sequenza e' corretta
   // Il risultato viene anche scritto sul trace a tracelevel 1

   protected:

   // Informazioni per gli algoritmi
   const LONG KeyLength;
   const LONG ExtraLength;

   virtual void KeySet();     // Imposta KeyCorrente a partire da RecordCorrente
   // Default: esegue un memcopy di KeyLen Bytes ( piu' ID)
   virtual void ExtraSet(){}; // Concatena extra bytes a DatiIndice
   // Non sono considerati parte della chiave
   // Default
   // Confronto tra due chiavi (default: memcmp)
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0);

   BUFR  DatiIndice;    // Dati di indicizzazione primari
   // Per ogni pagina: Key Iniziale, IdIniziale, ExtraBytes, KeyFinale, IdFinale, ExtraBytes
   ULONG NumSlotIndice;
   ULONG SizeSlotIndice;

   void Init(); // Chiamare obbligatoriamente nei costruttori delle sottoclassi
   // Carica la tabella di indicizzazione
   // Default( valido se non si ridefiniscono le funzioni virtuali):
   // chiamato nel costruttore di FILE_BS se Inizializza e' true

   virtual void EndSort(); // Per i file_fix invalida i buffer, per i BS ricarica le chiavi

   private:
   static int FILE_BS::sort_function( const void *a, const void *b);
   static int FILE_BS::sort_function2( const void *a, const void *b);

//<<< class  _export FILE_BS : public FILE_FIX {
};

//----------------------------------------------------------------------------
// FILE_KEY_TEXT :  File in sola lettura con ricerca dicotomica chiave = STRINGA
//----------------------------------------------------------------------------
// La chiave e' formata dai primi KEYLEN bytes del record che sono in formato STRINGA
// (oppure mediante la KeySet)
// Sul file puo' essere o meno gia' presente il NULL TERMINATOR della chiave
// La ricerca e' case insensitive (altrimenti ridefinire Compare())

class  _export FILE_KEY_TEXT : public FILE_BS {
   public:
   FILE_KEY_TEXT( const char* NomeFile,ULONG RecLen,ULONG KeyLen,ULONG PageSize,
      ULONG ExtraLen=0,BOOLOS2 Inizializza=TRUE);    // EMS002 ho cambiato BOOL in BOOLOS2

   const char * ChiaveCorrente();        // NULL Terminated

   // Posizionamento per chiave (anche parziale)
   // Si posiziona al PRIMO record con Key <= della chiave (parziale) data.
   // Ritorna FALSE in caso di errore. (Normalmente mai)
   virtual BOOL Posiziona(const STRINGA& ChiaveParziale) ;

   // Posizionamento per numero record. Ritorna FALSE se fuori range od in caso di errore
   virtual BOOL Posiziona(ULONG NumeroRecord){ return FILE_BS::Posiziona(NumeroRecord);};

   // Posizionamento per chiave (anche parziale)
   // Si posiziona al PRIMO record con Key <= della chiave (parziale) data.
   // Ritorna FALSE in caso di errore. (Normalmente mai)
   virtual BOOL Posiziona(const BUFR& Key){ return FILE_BS::Posiziona(Key);};

   protected:

   virtual void KeySet();
   // Confronto tra due chiavi
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto=0);

//<<< class  _export FILE_KEY_TEXT : public FILE_BS {
};

//----------------------------------------------------------------------------
// Funzioni di riferimento per compare con chiavi binarie di 2 e 4 bytes
//----------------------------------------------------------------------------
inline int CompareDoubleWord(const void * a,const void * b)
{
   return ((*(int*)a) - (*(int*)b) );
};
inline int CompareWord(const void * a,const void * b)
{
   return ((*(short*)a) - (*(short*)b) );
};
inline int Compare2DoubleWord(const void * a,const void * b)
{
   #define KEY1 ((int *)a)
   #define KEY2 ((int *)a)
   if ( KEY1[0] != KEY2[0] ) return KEY1[0] - KEY2[0];
   return KEY1[1] - KEY2[1];
   #undef KEY1
   #undef KEY2
};
inline int Compare2DoubleWord_D(const void * a,const void * b, ULONG DimConfronto)
{
   #define KEY1 ((int *)a)
   #define KEY2 ((int *)a)
   if ( KEY1[0] != KEY2[0] || DimConfronto < 5) return KEY1[0] - KEY2[0];
   return KEY1[1] - KEY2[1];
   #undef KEY1
   #undef KEY2
};
inline int Compare2Word(const void * a,const void * b)
{
   #define KEY1 ((short *)a)
   #define KEY2 ((short *)b)
   if ( KEY1[0] != KEY2[0]) return KEY1[0] - KEY2[0];
   return KEY1[1] - KEY2[1];
   #undef KEY1
   #undef KEY2
};
inline int Compare2Word_D(const void * a,const void * b, ULONG DimConfronto)
{
   #define KEY1 ((short *)a)
   #define KEY2 ((short *)b)
   if ( KEY1[0] != KEY2[0] || DimConfronto < 3) return KEY1[0] - KEY2[0];
   return KEY1[1] - KEY2[1];
   #undef KEY1
   #undef KEY2
};
#endif
