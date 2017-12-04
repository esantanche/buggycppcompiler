#include "dummy.h"
//----------------------------------------------------------------------------
// FILE OGGETTO.H
//----------------------------------------------------------------------------

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°° REPORT AGGIORNAMENTI °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Data ultima modifica documentazione:         14/10/92;

// Data ultima modifica codice:                 00/00/92;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°° DESCRIZIONE DELLA CLASSE °°°°°°°°°°°°°°°°°°°°°°°°°°°°
//
// Questo file dichiara la classe OGGETTO.
//
// La classe OGGETTO Š la superclasse della gerarchia della OOLIB dalla quale
// ereditano le classi che gestiscono: la visualizzazione (VIEW e PM), gli oggetti
// del modello, i link e l'Objset.

#ifndef HO_OGGETTO_H
#define HO_OGGETTO_H   // Indica che questo file e' stato incluso

#ifndef HO_STD_H       // Includo le librerie standard del progetto se gi…
#include <std.h>       // non sono state incluse
#endif

#ifndef HO_STRINGA_H   // Includo la classe STRINGA che serve per la gestione delle
#include <stringa.h>   // stringhe di caratteri se gi… non Š stata inclusa
#endif

#ifndef HO_ELENCO_H
#include <elenco.h>  
#endif               

#ifndef HO_ELENCO_O_H  // Includo la classe ELENCO_O che serve per la gestione degli
#include <elenco_o.h>  // elenchi di oggetti della classe OGGETTO se gi… non Š stata
#endif                 // inclusa

//#ifndef HO_ERROR_H     // Includo la classe ERROR che serve per la gestione e
//#include <error.h>     // visualizzazione dei messaggi di errore se gi… non Š stata
//#endif                 // inclusa

//#ifndef HO_EVENTI_H    // Includo il file EVENTI.H nel quale sono contenuti  i
//#include <eventi.h>    // messaggi che vengono scambiati fra la parte PM e la parte
//#endif                 // VIEW, le funzioni e le procedure per effetuare lo scambio;

// °°°°°°°°°°°°°°°°°°°°°°°°°°°° DICHIARAZIONE DELLA CLASSE °°°°°°°°°°°°°°°°°°°°°°°°°°°

class ELENCO_EL_S;
class ELENCO_S;

class _export OGGETTO {

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°       AREA  PUBLIC       °°°°°°°°°°°°°°°°°°°°°°°°°°°°

   public :

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°       ATTRIBUTI       °°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   STRINGA Nome;          // Nome dell'oggetto

   TIPOGGE TipoOggetto;   // Tipo dell'oggetto
                          // Il tipo TIPOGGE e' definito in ELENCO_O.H

   BOOL DistruggiSubord;

   static ELENCO_Oggetti All;           // Elenco che contiene tutti gli oggetti
                                        // appartenenti alla gerarchia di OGGETTO
                                        // instanziati dalla applicazione;


// °°°°°°°°°°°°°° RELAZIONI FRA OGGETTI APPARTENENTI ALLA GERARCHIA  °°°°°°°°°°°°°°°°
//
// Per poter descrivere e implementare relazioni fra oggetti di classi
// della gerarchia di OGGETTO ( es: relazioni fra oggetti della mappa e mappa alla
// quale appartengono, ...) posso utilizzare i seguenti oggetti appartenenti alla
// classe OGGETTO:
// - Owners e Members: per descrivere relazioni generiche attraverso le quali posso
//      implementare qualsiasi tipo di relazione: uno-a-uno, uno-a-molti, molti-a-molti;
// - Componente e OggettiDellaComponente : servono per rappresentare relazioni gerarchiche
//      di composizione (Uno-a-molti);
//
// Una ulteriore possibilit… di descrizione di relazioni gerarchiche Š data facendo
// uso di oggetti esterni di relazione (LINK) che permettono di rappresentare delle
// relazioni con attributi anche di tipo molti-a-molti.
// Di solito il puntamento a tali oggetti esterni e' effettuato, a seconda dei casi,
// attraverso gli oggetti Owners o Members
//
//
// NOTA:
//    la relazione di composizione che fa uso degli oggetti Componente ed
//    OggettiDellaComponente puo' rappresentare una gerarchia a piu' livelli ma
//    solamente di tipo strettamente gerarchica, non Š possibile implementare
//    gerarchie a grafo o con pi— padri;


   OGGETTO * Componente;                // Oggetto componente, all'interno di una
                                        // relazione di composizione, cui appartiene
                                        // l'oggetto;

   ELENCO_Oggetti  OggettiDellaComponente;  // Oggetti posseduti dall'oggetto se
                                            // questi ha funzione di oggetto componente
                                            // nella relazione gerarchica di
                                            // composizione;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°       METODI      °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

//-----------------------------------------------------------------------------
// Costruttore di istanze della classe OGGETTO, non richiede Object
// Set;
//
// Parametri:
//
//      Nom     : nome dell'oggetto da creare;
//      Tipo    : tipo dell'oggetto da creare;
//      Compo   : puntatore all'eventuale oggetto che funge da
//                componente nella relazione di composizione cui
//                l'oggetto this eventualmente appartiene;
//-----------------------------------------------------------------------------
   OGGETTO( const STRINGA & Nom,
                  TIPOGGE   Tipo,
                  OGGETTO * Compo    = NULL,
                  BOOL      IsCompon = FALSE );

   virtual ~OGGETTO();         // Distruttore virtuale di default

//-----------------------------------------------------------------------------
// Valore dell' oggetto (di default restituisce una stringa
// Null, solo alcuni oggetti hanno un valore)
// Analogamente per il multivalore
//-----------------------------------------------------------------------------
   virtual const STRINGA  & Valore() const;
   virtual const ELENCO_S & ValoreElenco() const; // Per gli oggetti multivalore
   virtual BOOL  IsMultivalore() const;


//-----------------------------------------------------------------------------
// Trace dei dati dell' oggetto
//-----------------------------------------------------------------------------
   void Trace(USHORT TrcLev = LIVELLO_DI_TRACE_DEL_PROGRAMMA );

//-----------------------------------------------------------------------------
// Restituisce il nome del Lock applicato all'oggetto;
// Puo' essere NULL;
//-----------------------------------------------------------------------------
   const STRINGA Lock() const;

//-----------------------------------------------------------------------------
// Stabilisce una relazione generica fra l'oggetto this che ha
// funzione di OWNER e l'oggetto Member;
// L' Owner puo subclassare ???
//-----------------------------------------------------------------------------
   virtual void SetRelazione(OGGETTO * Member);

//-----------------------------------------------------------------------------
// ATTENZIONE: ELIMINA TUTTE LE INSTANCE DELLA RELAZIONE !!!
// Elimina la relazione generica fra l'oggetto this che ha funzione di
// OWNER e l'oggetto Member;
//-----------------------------------------------------------------------------
   virtual void UnSetRelazione(OGGETTO * Member);

//-----------------------------------------------------------------------------
// Ricerche per oggetti di una data classe (controllata con IsA)
//-----------------------------------------------------------------------------
   ELENCO_Oggetti CercaSubordinati(const STRINGA & Classe=NUSTR);

//-----------------------------------------------------------------------------
// Questi metodi con le relazioni generiche solo per il subclass di OOLIB
//-----------------------------------------------------------------------------
   const ELENCO_Oggetti&  CercaOwners(BOOL AncheAssociati=FALSE);
   const ELENCO_Oggetti&  CercaMembers(BOOL AncheAssociati=FALSE);

//-----------------------------------------------------------------------------
// Restituisce TRUE se l'oggetto Member appartiene alla relazione
// generica in cui l'oggetto this ha funzione di OWNER;
//-----------------------------------------------------------------------------
   BOOL Owns(OGGETTO * Member,BOOL AncheAssociati=FALSE);

//-----------------------------------------------------------------------------
// Restituisce TRUE se l'oggetto Owner appartiene alla relazione
// generica in cui l'oggetto this ha funzione di MEMBER;
//-----------------------------------------------------------------------------
   BOOL Owned(OGGETTO * Owner,BOOL AncheAssociati=FALSE);

//-----------------------------------------------------------------------------
// Relazione di appartenenza (L' Owner non puo' subclassare perche'
// si avrebbero problemi per i campi dichiarati direttamente nella
// inizalization LIST: nel caso in cui sia necessario modificare la
// relazione provvedere direttamente nel costruttore degli oggetti)  ?????
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Stabilisce una relazione di composizione fra l'oggetto this che
// ha funzione di oggetto componente e l'oggetto Member che viene ad
// esso subordinato;
//-----------------------------------------------------------------------------
   virtual void SetSubordinato(OGGETTO * Member);

//-----------------------------------------------------------------------------
// Elimina la relazione di composizione fra l'oggetto this che ha
// funzione di oggetto componente e l'oggetto Member che Š ad esso
// subordinato;
//-----------------------------------------------------------------------------
   virtual void UnSetSubordinato(OGGETTO * Member);

//-----------------------------------------------------------------------------
// Indica se l'oggetto this ha funzione di oggetto componente in una
// relazione gerarchica di composizione;
//-----------------------------------------------------------------------------
   BOOL IsComponente()const {return IsCompo;};

//-----------------------------------------------------------------------------
// Indica se l'oggetto Member Š subordinato all'oggetto this in una
// relazione gerarchica di composizione;
//-----------------------------------------------------------------------------
   virtual BOOL IsSubordinato(OGGETTO * Member);

//-----------------------------------------------------------------------------
// Restituisce il puntatore allo oggetto di nome ONome subordinato
// all'oggetto this all'interno di una relazione di composizione;
// Restituisce un puntatore a NULL nel caso l'oggetto con nome ONome
// non sia subordinato all'oggetto this;
// Il parametro Last indica da quale oggetto cominciare a cercare allo
// interno della lista degli oggetti subordinati;
//-----------------------------------------------------------------------------
   virtual OGGETTO * GetSubordinato(const STRINGA & ONome,OGGETTO * Last=NULL);

//-----------------------------------------------------------------------------
// La relazione di ereditarieta' NON e' gestita esplicitamente al momento
// perche' potrebbe creare problemi in presenza di ereditarieta' multipla
// Vi sono pero' i metodi SubClassato IsA e NomeClasseBase ????
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Restituisce il nome della classe BASE cui appartiene l'oggetto;
//-----------------------------------------------------------------------------
   const STRINGA& NomeClasseBase() const;

//-----------------------------------------------------------------------------
// Restituisce il nome della classe cui appartiene l'oggetto;
// Se l'oggetto appartiene ad una classe che sub_classa una classe
// di BASE i due nomi saranno diversi;
//-----------------------------------------------------------------------------
   const STRINGA& NomeClasse() const;

//-----------------------------------------------------------------------------
// Restituisce TRUE se l'oggetto appartiene ad una classe che
// sub_classa una classe BASE;
//-----------------------------------------------------------------------------
   BOOL SubClassato() const;

//-----------------------------------------------------------------------------
// Restituisce TRUE se l'oggetto this appartiene alla classe 'Classe'
// o se appartiene ad una classe della gerarchia dalla quale discende;
//-----------------------------------------------------------------------------
   virtual BOOL IsA(const STRINGA & Classe)const;

   void VisualizzaErrore (short ErrID, STRINGA & ErrFile = EMPTYSTR,
        ELENCO_S & EleErrVar = *(ELENCO_S *)NULL);

   void ImpostaErrore (short ErrID, STRINGA & ErrFile = EMPTYSTR,
        ELENCO_S & EleErrVar = *(ELENCO_S *)NULL);

//-----------------------------------------------------------------------------
// Annulla l' eventuale errore impostato
//-----------------------------------------------------------------------------
   void AnnullaErrore ();

//-----------------------------------------------------------------------------
// Per evitare chiamate ricorsive al distruttore
//-----------------------------------------------------------------------------
   BOOL InFaseDiDistruzione;

//-----------------------------------------------------------------------------
// Verifica se un oggetto esiste (ancora)
//-----------------------------------------------------------------------------
   BOOL EsisteAncora() {return OGGETTO::All.CercaPerPuntatore(this) >= 0;};

   virtual BOOL VerificaFormaleValore(const STRINGA * Valore,
                                            OGGETTO * Origine = NULL,
                                      class LINK    * Link    = NULL,
                                      const STRINGA * NmLock  = NULL );

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°      AREA  PROTECTED     °°°°°°°°°°°°°°°°°°°°°°°°°°°

   protected  :

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°       ATTRIBUTI       °°°°°°°°°°°°°°°°°°°°°°°°°°°°°


   STRINGA BaseClass;   // Contiene il nome della classe BASE cui appartiene
                        // l'oggetto, la classe Š una di quelle definite nella
                        // parte VIEW della gerarchia di OGGETTO;

   STRINGA ClassName;   // Contiene il nome EFFETTIVO della classe cui appartiene
                        // l' oggetto, differisce dal nome contenuto in BaseClass
                        // se l'oggetto appartiene ad una classe che sub_classa una
                        // classe base;

   ELENCO_Oggetti Members;      // Elenco degli oggetti che appartengono ad una
                                // relazione generica in cui l'oggetto this ha
                                // funzione di oggetto OWNER;
                                // Questo elenco Š gestito dal metodo SetRelazione

   ELENCO_Oggetti Owners;       // Elenco degli oggetti che appartengono ad una
                                // relazione generica in cui l'oggetto this ha
                                // funzione di oggetto MEMBER;
                                // Questo elenco Š gestito dal metodo SetRelazione

   BOOL MustFail;       // Se settata a TRUE, in fase di creazione dell'oggetto
                        // fa fallire la creazione dell'oggetto stesso che verr…
                        // immediatamente distrutto;

   STRINGA  NomeLock;   // Contiene il nome del Lock cui appartiene l'oggetto;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°       METODI      °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

//-----------------------------------------------------------------------------
// Usata da ImpostaErrore e VisualizzaErrore.
// Cerca esclusivamente nel file dei messaggi ErrID
//-----------------------------------------------------------------------------
   STRINGA LeggiErrore (short ErrID, STRINGA & ErrFile = EMPTYSTR,
        ELENCO_S & EleErrVar = *(ELENCO_S *)NULL);

//-----------------------------------------------------------------------------
// Funzioni di supporto usate dalle funzioni "RELAZIONI APPLICATIVE"
//-----------------------------------------------------------------------------
   SHORT GetMemberPos( OGGETTO * Member , USHORT Atom );
   SHORT GetOwnerPos( OGGETTO * Member, USHORT Atom );
   ULONG CercaNomeRelazioneInAtomTable( const STRINGA & NomeRelazione );

//-----------------------------------------------------------------------------
// Questi metodi sono per la corretta gestione dei Links
// Se non ridefiniti generano un abend !
// LLL
// Quando arriva questo messaggio debbo controllare che il valore sia
// accettabile (Puo' anche essere NULL!);
// Ritorna TRUE se in ERRORE
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Verifica la correttezza formale del valore contenuto nel parametro
// Valore; viene implemementato per classi quali ENTRY_VW (Entry
// Field) per le quali Š necessario verificare che il valore immesso
// (assegnato al parametro Valore) rispetti alcune regole precise.
//
// Parametri:
//
//      Valore  : contiene il valore sul quale effettuare la verifica
//                di correttezza formale;
//      Origine : oggetto che ha assegnato il valore all'oggetto su
//                cui si sta effettuando la verifica formale; diverso
//                da NULL se l'oggetto this appartiene ad un LINK;
//      Link    : LINK  a cu appartiene l'oggetto;
//      NmLock  : nome del LOCK effettuato sull'oggetto;
//-----------------------------------------------------------------------------
   virtual BOOL Notifica_FaseDiVerifica(      OGGETTO * Origine,
                                        class LINK    * Link   = NULL,
                                        const STRINGA * NmLock = NULL );

//-----------------------------------------------------------------------------
// Vengo informato che un campo della VIEW cui sono connesso e' stato aggiornato
// Ritorna TRUE se in ERRORE
// (Nota:  di solito la verifica formale e' gia' passata)
//-----------------------------------------------------------------------------
   virtual BOOL Agg_View_conn_da_Link( class OGGETTO_VIEW * Origine,
                                       class LINK         * Link,
                                       const STRINGA      * NmLock = NULL);

   virtual BOOL Notif_Da_View_conn_da_Link( class OGGETTO_VIEW * Origine,
                                            class LINK         * Link,
                                            const STRINGA      * NmLock = NULL);

//-----------------------------------------------------------------------------
// Vengo informato che un campo del MODEL cui sono connesso e' stato aggiornato
// LA MODIFICA NON PUO' ESSERE RIFIUTATA
// Ritorna TRUE se in ERRORE
//-----------------------------------------------------------------------------
   virtual BOOL Agg_Model_conn_da_Link( OGGETTO * Origine,
                                        class LINK *Link,
                                        const STRINGA* NmLock = NULL,
                                        BOOL HoSyncPoint=FALSE);

   virtual BOOL Agg_Model_conn_da_Link( OGGETTO * Origine,
                                        class LINK *Link,
                                        const ELENCO_S* NmLock = NULL,
                                        BOOL HoSyncPoint=FALSE);

   virtual BOOL Agg_Model_conn_da_Link( OGGETTO * Origine,
                                        class LINK *Link,
                                        const ELENCO_EL_S* NmLock = NULL,
                                        BOOL HoSyncPoint=FALSE);

   virtual BOOL Notif_Da_Model_conn_da_Link( OGGETTO * Origine,
                                             class LINK *Link,
                                             const STRINGA* NmLock=NULL,
                                             BOOL HoSyncPoint=FALSE);

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°      AREA  PRIVATE    °°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   private:

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°       ATTRIBUTI       °°°°°°°°°°°°°°°°°°°°°°°°°°°°°

   BOOL IsCompo;        // Indica se l' oggetto e' un oggetto componente in una
                        // relazione gerarchica di composizione;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°       METODI      °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

//-----------------------------------------------------------------------------
// Routine di inizializzazione del valore dello attributo 'IsCompo';
//-----------------------------------------------------------------------------
   void Init(OGGETTO * Compo);

   friend class LLL;
   friend class HLL;
   friend class LINK;

// Questo per evitare che qualcuno erroneamente assegni un oggetto ad un altro, il che
// normalmente causa grossi problemi. Il metodo ovviamente NON e' implementato
   void operator = (OGGETTO & From);

   public:
   #include <spare.h>
};

// ---------------------------------------------------------------
// Queste macro sostituiscono la DosEnterCritSec e DosExitCritSec
// ---------------------------------------------------------------
#define DOE1 // Inibita per migrazione a Windows
#define DOE2 // Inibita per migrazione a Windows 
//#define DOE1 Doe1()
//#define DOE2 Doe2()
//void Doe1();
//void Doe2();

#endif
