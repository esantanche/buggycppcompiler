//========================================================================
// Componente di gestione topologia della rete FS
//========================================================================
// 1G

#ifndef HO_TOPOLOG_HPP
#define HO_TOPOLOG_HPP

#define MAXNODI  400 // Massimo numero di nodi
#define MAXCAMBI 1000 // Massimo numero di nodi di cambio

#ifndef HO_STAZIONI_HPP
#include "ID_STAZI.hpp"
#endif

#ifndef HO_ELENCO_H
#include <elenco.h>
#endif

typedef WORD ID;   
#define ID_NON_VALIDO   0xffff
#define WORD_NON_VALIDA 0xffff

typedef WORD DIST ;      // Formato interno delle distanze = Km
#define BigDIST 0xffff   // Massima distanza

#define MAXPRIMIVICINI 6 // Massimo numero di nodi primi vicini SU FILE

#define MAIN_NODES 20 // Nodi piu' importanti ( Roma, napoli eccetera).
#define DIM_M    5    // Dimensione tabella MinNodi

#define ID_INTERNO ID             // Solo per leggibilita' dei prototipi.
#define ARRAY_ID_INTERNI ARRAY_ID // Solo per leggibilita' dei prototipi.


//----------------------------------------------------------------------------
// SCAN()
//----------------------------------------------------------------------------
// macro per scandire un' array, un elenco od altro
// OPERA AD ALTE PERFORMANCE, in quanto evita moltiplicazioni
// Vuole:
//    - L' array
//    - Il  nome di un reference (che viene creato) che accede all' elemento in scansione
//    - Il  nome del TIPO dei dati contenuti nell' array dinamica.
// Per ottenere l'indice corrente si utilizzi la macro SCAN_IDX
// (vuole array e nome del reference)
// Se la dimensione dell' array e' 2 si utilizzi SCAN_IDX_2 (molto piu' veloce)
// SCAN_NUM : Scandisce un' array VERA, vuole come parametro anche il numero elementi
// Queste macro ottengono lo stesso effetto ottenuto dall' optimizer
// del C++ (Stregth reduction) ma hanno il vantaggio di funzionare anche nel
// caso in cui l' accesso all' ARRAY si ottenga attraverso un metodo
// (nel qual caso l' optimizer SI DEVE arrendere)
//----------------------------------------------------------------------------
// Gli if sono necessari per evitare errori di bound check quando la dimensione e' 0
#define SCAN_LIM(_Array,_Start,_Stop,_Var,_Type)  \
int _Var##_AStart = _Start;                     \
int _Var##_AStop  = _Stop;                      \
if(_Var##_AStop > _Var##_AStart)                \
for(_Type & _Var= _Array[_Var##_AStart], *_Var##_STOP= (_Type*)&_Var + _Var##_AStop; &_Var < _Var##_STOP ; (_Type*)(&_Var)++)
#define SCAN_NUM(_Array,_Num,_Var,_Type)          \
int _Var##_AStop  = _Num;                       \
if(_Var##_AStop)                                \
for(_Type & _Var= _Array[0], *_Var##_STOP= (_Type*)&_Var + _Var##_AStop; &_Var < _Var##_STOP ; (_Type*)(&_Var)++)
#define SCAN(_Array,_Var,_Type)                   \
int _Var##_AStop  = _Array.Dim();               \
if(_Var##_AStop)                                \
for(_Type & _Var= _Array[0], *_Var##_STOP= (_Type*)&_Var + _Var##_AStop; &_Var < _Var##_STOP ; (_Type*)(&_Var)++)
#define SCAN_IDX(_Array,_Var) (&_Var - &_Array[0])
#define SCAN_IDX2(_Array,_Var) (((BYTE*)&_Var - (BYTE*)&_Array[0])>>1)
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// QUAD_TAR
//----------------------------------------------------------------------------
// Struttura per gestire fino a quattro tariffe regionali
// e per identificare i match ...
struct QUAD_TAR {
     BYTE  TariffaRegione        ;     // Per abbonamenti regionali : Corice tariffario regione
     BYTE  Prima_Estensione      ;     // Per abbonamenti regionali 2ø regione di appartenenza
     BYTE  Seconda_Estensione    ;     // Per abbonamenti regionali 3ø regione di appartenenza
     BYTE  Terza_Estensione      ;     // Per abbonamenti regionali 4ø regione di appartenenza
     BIT   NonValido          :1 ;     // Set ad 1 se non valido
     BIT   VarieRegioni       :1 ;     // Set ad 1 se non valido

  BOOL Empty(){ return ((*(DWORD*)&TariffaRegione)==0);};

  void Set(BYTE Tar,BYTE Es1,BYTE Es2,BYTE Es3); // Impostazione a partire dai dati sciolti

  QUAD_TAR & operator &=(QUAD_TAR & Altro);

  BYTE Identifica(); // Identifica la tariffa regionale da applicare

};


// Questa per contenere le tariffe regionali delle stazioni di un ramo: 
// Nota: sono possibili strutture piu' compatte e veloci: da rivedere
// se le performance e/o l' occupazione di memoria non risultassero adeguate.
struct TAR_RAMO  {
     WORD      Id1 ;  // primo nodo del ramo
     WORD      Id2 ;  // primo nodo del ramo
     BYTE      Prog;  // Progressivo nell' ambito del ramo (da 1 a crescere)
     WORD      IdStazione;   // Id Esterno della stazione
     QUAD_TAR  Tariffe;
};
struct ARRAY_TAR_RAMO : public BUFR {
   TAR_RAMO& operator[] (int Indice){
      #ifdef BOUND_CHECK
        if(Indice < 0 || Indice >= Dim() )BoundCheckError(Indice,Dim());
      #endif
      return * ( ((TAR_RAMO*)this->Dati) +Indice);
   };
   ULONG Dim(){return this->Length / sizeof(TAR_RAMO);};
   ARRAY_TAR_RAMO(ULONG i):BUFR(i* sizeof(TAR_RAMO)){SDim=-1;};
   void ReDim(ULONG Num){BUFR::ReDim(Num* sizeof(TAR_RAMO));SDim=-1;};
   void operator+= (TAR_RAMO& Tariffa){BUFR::Store(&Tariffa,sizeof(TAR_RAMO));}; 
   void Sort();                                // Sort per: Id1, Id2, Prog
   int Seek(WORD Key1, WORD Key2, BYTE Key3);  // Cerca nell' array sortata (esclusi altri elementi aggiunti in seguito)
   int SDim;                                   // -1 se non sortata, altrimenti dimensione dei dati sortati
};


//----------------------------------------------------------------------------
// NODO_FS
//----------------------------------------------------------------------------
struct PRIMO_V{          // 4 Bytes l' uno
   // E' un' estensione di NODO_ALFA, e rappresenta un primo vicino di un nodo
   ID_INTERNO   Id;              // Id del PrimoVicino
   DIST         Distanza;        // Distanza del primo vicino
};
struct NODO_FS {  // 42 bytes l' uno
   ID   IdNodo;             // Id esterno del Nodo (Codice DKM)
   WORD Rank;               // Il piu' importante ha Rank 1
   CHAR Ident[8];           // Identificatore per instradamenti
   CHAR Ident_Eqv[8];       // Identificatore generico per multicitta'
   WORD NumeroPrimiVicini;  // Numero effettivo primi vicini
   BYTE NodoDiCambio;       // Vero se e' un nodo reale od un nodo di cambio

   // Queste sono informazioni di stazione: per velocizzare le operazioni
   WORD Id1;                // Per i nodi di cambio: Primo nodo del ramo   (ID ESTERNO)
   WORD Id2;                // Per i nodi di cambio: Secondo nodo del ramo (ID ESTERNO)
   BYTE ProgRamo;           // Per i nodi di cambio: progressivo nell' ambito del ramo
   QUAD_TAR Tariffe;        // Per le tariffe regionali
   
   // Metodi
   WORD PosDaId(ID_INTERNO IdNodo);     // Cerca nell' array dei primi vicini
   DIST DistDaId(ID_INTERNO IdNodo){    // E ritorna la distanza
      return PrimiVicini[PosDaId(IdNodo)].Distanza;
   };
   
   // Variabili di algoritmo (diretto ed inverso)
   DIST       Distanza1;          // Prima distanza Complessiva fino a destinazione
   ID_INTERNO Nodo1 ;             // Id del primo vicino corrispondente
   WORD       Progressivo;
   
   // Primi vicini
   PRIMO_V *  PrimiVicini; // Nodi PrimiVicini (Codice DKM)
//<<< struct NODO_FS {  // 32 bytes l' uno 
};

struct ARRAYNODI : protected BUFR {
   // Questa deve essere inline
   NODO_FS & operator[] (ID Id){
      #ifdef BOUND_CHECK
      if(Id >= Dim())BoundCheckError(Id,Dim());
      #endif
      return * ( ((NODO_FS*)this->Dati) +Id);
   };
   ULONG Dim(){return this->Length / sizeof(NODO_FS);};
   // Ridefinizioni
   ARRAYNODI(ULONG i=0):BUFR(i* sizeof(NODO_FS)){};
   void SetDim(ULONG NumNodi);
   void Store(const NODO_FS& Nodo){BUFR::Store(&Nodo,sizeof(NODO_FS));};
   void operator+= (const NODO_FS& Nodo){Store(Nodo);};
};


//----------------------------------------------------------------------------
// PERCORSO_FISICO
//----------------------------------------------------------------------------
class PERCORSO_FISICO {
   public:
   DIST     Lunghezza;      // Lunghezza in Km del percorso
   LONG Len(){return (LONG)Lunghezza;}; // Iden ma adatta per calcoli
   ARRAY_ID Nodi;           // Array con i nodi del percorso : sono ID_INTERNI
   // Fanno eccezione i percorsi fisici forniti da INSTRAD che
   // alla fine provvede a convertire gli ID in formato esterno
   // e quelli forniti da PercorsoDa()

   void NodiExt();          // Converte Nodi in id Esterni
   
   PERCORSO_FISICO(class TOPOLOGIA_NODI& Topol):Topo(Topol){Lunghezza = 0;}; // Vuoto
   
   PERCORSO_FISICO(ID_INTERNO NodoIniziale,class TOPOLOGIA_NODI& Topol):Topo(Topol){Lunghezza = 0;Nodi += NodoIniziale;};
   
   // Nel metodo seguente NodiViaggio deve contenere SOLO ID interni
   PERCORSO_FISICO(ARRAY_ID_INTERNI& NodiViaggio,class TOPOLOGIA_NODI & Topol);
   
   PERCORSO_FISICO(PERCORSO_FISICO& From):Topo(From.Topo){Lunghezza = From.Lunghezza;Nodi = From.Nodi;};
   ~PERCORSO_FISICO(){};
   
   BOOL operator == (PERCORSO_FISICO & b);
   
   // Mi da' la parte di percorso compresa tra due nodi
   // I due nodi estremi sono compresi
   ARRAY_ID_INTERNI DammiParteCompresa(ID_INTERNO Fra, ID_INTERNO E);
   
   // Mi da' il nodo dominante del percorso compreso tra due nodi.
   // Ritorna l' indice e non l' id del nodo
   // Corrisponde al nodo con Rank maggiore con delle limitazioni
   // Se non trova un nodo dominante torna -1
   int NodoDominante(WORD Pos1, WORD Pos2,WORD LastRank);
   
   // Questa torna il numero di discordanze tra due percorsi in un dato tratto
   WORD Paragona(PERCORSO_FISICO & Altro,ID_INTERNO Fra, ID_INTERNO E);
   
   // Questa mi dice se in un dato tratto un percorso segue il minimum path
   // Se si' torna -1
   // Se no torna l' indice del nodo vincolante nel percorso
   int MinimumPath(WORD Pos1, WORD Pos2);
   
   // Questa torna il numero di nodi in un dato tratto compresi gli estremi
   WORD Conteggio(ID_INTERNO Fra, ID_INTERNO E);
   
   // Questa torna la lunghezza in un dato tratto compresi gli estremi
   DIST LunghezzaTratto(ID_INTERNO Fra, ID_INTERNO E);
   
   // Chiama risolvi e prolunga il percorso fino ad A
   void Prolunga(ID_INTERNO A);
   
   // Set di un percorso data un array di vincoli; Torna FALSE su errore
   BOOL Set(ARRAY_ID_INTERNI& NodiViaggio);
   // Trace dei nodi del percorso
   void Trace(const STRINGA& Messaggio, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
   void Clear(){Lunghezza=0;Nodi.Clear();};
   
   class TOPOLOGIA_NODI & Topo;
//<<< class PERCORSO_FISICO {
};

//----------------------------------------------------------------------------
// INSTRAD
//----------------------------------------------------------------------------
// Questa struttura e' utilizzata per l' interfaccia con i programmi applicativi
// I dati di input e di output con questa struttura sono ID Esterni
struct INSTRAD{
   // Instradamento Standard dato il viaggio
   // Gli ID sono ID ESTERNI se "Esterni" e' vera
   INSTRAD(ARRAY_ID& NodiViaggio,class TOPOLOGIA_NODI & Topo,BOOL Esterni=TRUE);
   ARRAY_ID NodiSoluzione;      // Contiene l' instradamento standard (id = Id Stazioni)
   // Rappresentato mediante gli ID esterni
   // Se vi sono problemi: ha Dim() == 0
   STRINGA Descrizione;         // Questo e' stampabile
   DIST Lunghezza;              // Lunghezza del percorso Standard
   
   DIST LunghezzaMV;            // Lunghezza del percorso minimo vincolato
   
   PERCORSO_FISICO PercorsoDiRiferimento; // Percorso di riferimento  Rappresentato mediante gli ID esterni 
   PERCORSO_FISICO PercorsoStandard;      // Percorso Standard        Rappresentato mediante gli ID esterni
   
   WORD NumScorciatoie; // Numero di scorciatoie: se 0 PercorsoStandard = PercorsoDiRiferimento

   BYTE CodiceTariffaRegionale; // 0    = Non si applica tariffa regionale
                                // 255  = Non sono in grado di determinarla
                                // Altre: si veda l' enum CODICI_TARIFFA_REGIONE in ID_STAZI.HPP
};


//----------------------------------------------------------------------------
// TOPOLOGIA_NODI
//----------------------------------------------------------------------------
// Questa classe e' relativa alla rappresentazione e risoluzione del grafo
// della rete FS
// Si deve innanzitutto impostare il problema con la funzione ImpostaProblema()
// Tale funzione deve estendere il grafo per tener conto delle stazioni
// che non sono nodi.
// Tali stazioni sono trasformate in nodi equivalenti.
// Il tutto e' gestito da due funzioni:
//     ImpostaProblema()    // Esegue impostazione
//     Free()               // Annulla impostazione
// Una volta impostato il problema si possono chiamare varie funzioni per
// trovare soluzioni sul grafo. Le stazioni debbono essere tutte nodi od
// essere state definite in ImpostaProblema
//     MinimoVincolato()    // Trova il percorso minimo vincolato tra una serie di nodi
//     MinimoAssoluto()     // Trova il percorso minimo tra una serie di nodi
// Questi metodi invece anche se vogliono che sia stato eseguito un ImpostaProblema
// che contiene la stazione "A" possono funzionare con qualsiasi stazione.
// Sono pensati per i batch veloci (simulazioni)
// Hanno come prerequisito la chiamata di Risolvi(A)
//     DistanzaDa()         // Metodo per ottenere la sola distanza
//     PercorsoDa()         // Metodo per ottenere il percorso
// Queste sono le funzioni tecniche che risolvono il problema del minimun path
// con un algoritmo di dinamic programming
//     Risolvi(A)
//     Risolvi(Da,A)
//----------------------------------------------------------------------------
class TOPOLOGIA_NODI {
   
   public:
   // Gli ID sono ID esterni; 
   // Gli id interni corrispondenti stanno in NodiProblema
   BOOL ImpostaProblema(ARRAY_ID&  NodiProb);
   
   TOPOLOGIA_NODI(const STRINGA& Path, const STRINGA& Nome= "NODI.DB");   // Carica la topologia dai files
   ~TOPOLOGIA_NODI();
   
   // I metodi seguenti tornano FALSE su errore. Gli ID sono esterni
   BOOL MinimoVincolato(); // Imposta il percorso minimo vincolato tra le stazioni date in ImpostaProblema
   BOOL MinimoAssoluto();  // Imposta il percorso minimo assoluto tra le stazioni date in ImpostaProblema
   BOOL MinimoVincolato(ARRAY_ID&  NodiPer); // Imposta il percorso minimo vincolato tra una serie di stazioni
   BOOL MinimoAssoluto(ARRAY_ID&  NodiPer);  // Imposta il percorso minimo assoluto tra una serie di stazioni
   
   
   BOOL Risolvi(ID A); // Prerequisito per le funzioni seguenti. Vuole un ID esterno
   
   // Il metodo fornisce la distanza da una qualunque stazione all' origine.
   // Ha come prerequisito che sia stata invocata Risolvi(A);
   // Eccezione: non torna la distanza corretta per A.
   // Opzionalmente puo' anche fornire una indicazione del verso di percorrenza
   // del ramo cui cui appartiene la stazione: il parametro verso puo' essere:
   //    0: La stazione e' un nodo (non terminale) 
   //    1: Stesso verso dell' ordine dei nodi in STAZIONI (dal numero minore a maggiore)
   //    2: Verso invertito rispetto all' ordine dei nodi in STAZIONI 
   //    3: Verso indifferente (stessa distanza nei due versi)
   // Il verso e' 1 se andando dalla stazione ad A percorro il ramo dal nodo
   // 1 al nodo 2 (che e' quindi il piu' vicino ad A)
   ULONG DistanzaDa(ID Esterno, int & Verso = *(int*)NULL);
   // Idem ma fornisce il percorso (in PercorsoMinimo). Il verso non e' gestito
   // Si noti che si usano gli Id ESTERNI
   BOOL  PercorsoDa(ID Da);  // FALSE su errore

   // Questa variabile contiene il percorso minimo ottenuto dai metodi
   // - MinimoVincolato()
   // - MinimoAssoluto()
   // - PercorsoDa()  // ATTENZIONE : imposta id esterni
   // Il percorso minimo e' normalmente impostato come array di id interni
   PERCORSO_FISICO PercorsoMinimo;
   
   // ........................................................
   // Da qui in poi i metodi sono tecnici ed interni al motore
   // ........................................................
   
   // L' ID di cui fare il trace e' considerato un ID interno se ha codice basso altrimenti
   // E' considerato un id esterno.
   void TraceId(const STRINGA& Msg, ID Id, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA); 
   
   
   // Queste per avere l' origine e la destinazione (come id interni)
   // Nota: si suppone che in ImpostaProblema i nodi siano stati dati nel corretto ordine
   ID_INTERNO IdOrigine(){return NodiProblema[0];};
   ID_INTERNO IdDestinazione(){return NodiProblema[NodiProblema.Dim()-1];};
   
   void Free();   // Invalida le soluzioni trovate
   
   NODO_FS & operator[] (int Indice){return AreaNodi[Indice];};
   
   // Tabella per l' algoritmo di rappresentazione percorso standard
   
   struct MINNODI {
      WORD Da;
      WORD A;
      WORD Min;
   };
   
   MINNODI MinNodi[DIM_M];
   
   // Questo mi dice se un dato nodo e' stato toccato dall' ultimo Risolvi (O Risolvi2)
   BOOL NodoConsiderato(ID_INTERNO Id){return AreaNodi[Id].Progressivo == ProgressivoCorrente;};
   
   
   BOOL AggiuntaNodoDiCambio(const ID NodoDiCambio); // Aggiunta permanente di un nodo di cambio
   BOOL AggiuntaNodoFittizio(const ID NodoFittizio);
   
   BOOL NodoDiCambio(ID Esterno); // Torna TRUE Se l' ID Esterno ha ID Interno definito ed e' un nodo di cambio
   BOOL Definito(ID Esterno);     // Torna TRUE Se l' ID Esterno ha ID Interno definito

   ARRAY_TAR_RAMO  TarRami;
   
   // protected:
   
   
   // Questi sono i nodi del problema. Le stazioni che non erano nodi sono state
   // trasformate in nodi equivalenti.
   ARRAY_ID_INTERNI  NodiProblema;
   
   // Questo e' impostato da Risolvi(A);
   ID_INTERNO A_Valido;
   // Queste sono le stazioni del ramo di A_Valido (se non e' un nodo)
   // per gestire le distanze ed i collegamenti da stazioni dello stesso ramo.
   ID A_Val1,A_Val2; // Sono id esterni
   
   BOOL VerificaPercorso(ARRAY_ID_INTERNI & Percorso); // Torna FALSE se non compatibile con problema impostato
   
   // Buffer di tutti i nodi
   ARRAYNODI AreaNodi; // Area dei nodi (riallocazione Dinamica)
   ID NumNodiReali;    // = Numero di Nodi Fs 
   ID NumNodi;         // = Numero di Nodi reali + nodi di cambio
   ID TotNodi;         // e numero totale (comprende i fittizi specifici del problema)
   
   BOOL      ImpostazioneOK;    // Se false non parte la soluzione del problema
   
   // Metodi tecnici per risoluzione problema percorso minimo
   BOOL Risolvi(ID_INTERNO Da, ID_INTERNO A);       // Percorso minimo tra due nodi
   BOOL Risolvi_Ramo(ID_INTERNO Da, ID_INTERNO A);  // Variante utilizzata per l' aggiunta nodi
   
   
   WORD      ProgressivoCorrente;
   
   STAZIONI * Stazioni;
   
   friend class PERCORSO_FISICO;
   friend class INSTRAD;
   friend class ARRAY_ID;
   friend class INTERVALLO;
//<<< class TOPOLOGIA_NODI {
};

//----------------------------------------------------------------------------
// INTERVALLO
//----------------------------------------------------------------------------
struct INTERVALLO {
   
   INTERVALLO(){};
   INTERVALLO(INTERVALLO& Itv){memmove(this,&Itv,sizeof(INTERVALLO));};
   
   // Torna TRUE se il set origina una scorciatoia valida sull' intervallo
   BOOL Set(WORD Ofs1,WORD Ofs2,PERCORSO_FISICO & PercorsoRif,DIST Limite);
   
   WORD P1; // Punta a Nodo1 nel percorso di riferimento
   WORD P2; // Punta a Nodo2 nel percorso di riferimento
   
   ID_INTERNO Nodo1;
   ID_INTERNO Nodo2;
   
   DIST DeltaDistanza;
   WORD DeltaNodi;
   int  DeltaRank;
   
} ;
//----------------------------------------------------------------------------
// ELENCO_INTERVALLI
//----------------------------------------------------------------------------
// Array dinamica di puntatori ad INTERVALLI
//----------------------------------------------------------------------------
typedef INTERVALLO * P_I;
#define DIN_ARRAY EL_P_I
#define CLAS P_I
#define DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
#define CopiaMembro(_CLAS) (Beep(__FILE__,__LINE__,1),(P_I)NULL) // Vieto la copia
#include "dinarray.h"

struct ELENCO_INTERVALLI : public EL_P_I {
   void Sort(); // Sort per DeltaDistanza
};

//----------------------------------------------------------------------------
// Conversione da Id esterni ad interni 
//----------------------------------------------------------------------------
// Tabella di conversione da Id esterni ad interni
extern ARRAY_ID TabIdInterni;
inline ID_INTERNO Interno(ID Esterno){ return TabIdInterni[Esterno]; };

#endif
