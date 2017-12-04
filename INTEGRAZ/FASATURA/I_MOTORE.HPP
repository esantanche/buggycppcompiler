//========================================================================
// Strutture di interfaccia con il motore
//========================================================================
// VERSIONE CON SERVIZIO CUMULATIVO
// 1G

#ifndef HO_I_MOTORE_HPP
#define HO_I_MOTORE_HPP

#ifndef HO_BASE_HPP
#include "base.hpp"
#endif

#ifndef HO_OGGETTO_H
#include <oggetto.h>
#endif

#ifndef HO_STAZIONI_HPP
#include <id_stazi.hpp>
#endif

#ifndef HO_MOTGLUE_HPP
#include "motglue.hpp"
#endif


// Questa variabile mi dice se si debba lavorare con i precaricati o con le polimetriche informatizzate
extern BOOL UsaIPrecaricati;

//----------------------------------------------------------------------------
// MM_PERCORSO e relativo elenco dinamico
//----------------------------------------------------------------------------
// Questa classe rappresenta un percorso gia' ridotto in forma standard;
// E' una struttura dati tipicamente utilizzata solo nei costruttori
// delle classi applicative
struct MM_I_PERCORSO {
   
   MM_I_PERCORSO();
   ARRAY_ID FormaStandard; // Forma standard del percorso
   // Il primo ID e' l' ID della stazione di origine
   // L' ultimo ID e' l' ID della stazione di destinazione
   // Gli altri sono i "nodi" di instradamento.
   // Potrebbero essere delle stazioni non nodali  ma solo se
   // strettamente necessarie per rappresentare l' instradamento.
   ARRAY_DW IstradamentoCVB; // Serie di codici di instradamento 
   // I codici riportati sono codici CCR, non id interni
   // I codici delle stazioni italiane non comprendono il codice rete
   
   STRINGA FormaStandardStampabile; // E' stampabile sul biglietto

   WORD CodCumCVB;        // Codice CVB della relazione cumulativa (Solo precaricati)
   BYTE PrefComm ;        // Per compatibilita' M300: Indica ordine di preferenza COMMERCIALE (Solo precaricati)

   DATI_TARIFFAZIONE  DatiTariffazione;

   void * Reserved;  // Dati interni utilizzati dal motore

   void Trace(const STRINGA& Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
   void PrintPercorso(int Handle,ARRAY_ID & Richiesta = *(ARRAY_ID*)NULL);
   void ShortPrintPercorso(int Handle);
   void ShortPrintPercorso(FILE * Out){ShortPrintPercorso(fileno(Out));};
   void PrintPercorso(FILE * Out,ARRAY_ID & Richiesta = *(ARRAY_ID*)NULL){ PrintPercorso(fileno(Out),Richiesta);};

   BOOL DaPrecaricato; // Indica se il percorso e' precaricato o e' calcolato dal motore

   protected:
   // Questi metodi debbono essere utilizzati normalmente solo dal motore

   void Set(struct PERCORSO_INSTRADAMENTI & Inst);  // Imposta da instradamento calcolato dal motore
   void Set(struct INSTRPRE& Instradamento);        // Imposta da instradamento precaricato M300 like
   void Set(struct RELCUMUL& TrattaCumul);          // Imposta da instradamento precaricato M300 like
   void Set(struct TRAVERSATA_MARE & TrattaTragh);  // Imposta da instradamento precaricato M300 like 

   void Add(struct INSTRPRE& TrattaFS);             // Aggiunge una seconda tratta presa da instradamento FS
   void Add(struct RELCUMUL& TrattaCumul);          // Aggiunge una seconda tratta presa da instradamento cumulativo
   void Add(struct TRAVERSATA_MARE & TrattaTragh);  // Aggiunge una seconda tratta presa da Tirrenia

   void ImpostaCCRDestinazione(ID Origine, ID Destinazione);  // Imposta il CCR dell' origine e della destinazione
   

   void Inverti();                                   // Inverte un percorso; 

   void StoreTo(BUFR & To, ELENCO & Instradamenti);  // Mette in Instradamenti i percorsi (del Grafo) utilizzati
   void ReStoreFrom(BUFR & From);                    // Richiede che il grafo abbia gia' gli instradamenti

   // Per rendere accessibili i metodi precedenti
   friend class MM_RETE_FS;
   friend class MM_PERCORSO_E_SOLUZIONI;
   friend class MM_ELENCO_PERCORSO_E_SOLUZIONI;

   
//<<< struct MM_I_PERCORSO {
};

// Elenco dinamico di MM_I_PERCORSO
typedef MM_I_PERCORSO * P_MM_PE;
#define DIN_ARRAY MM_ELENCO_I_PERCORSO
#define CLAS P_MM_PE
#define DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
#define CopiaMembro(_CLAS) (Beep(__FILE__,__LINE__,1),(P_MM_PE)NULL) // Vieto la copia
#include "dinarray.h"


// Elenco dinamico di MM_SOLUZIONE
typedef MM_SOLUZIONE * P_MM_SO;
#define DIN_ARRAY MM_ELENCO_SOLUZIONE_I
#define CLAS P_MM_SO
#include "dinarray.h"

class _export MM_ELENCO_SOLUZIONE : public MM_ELENCO_SOLUZIONE_I {
   public : ~MM_ELENCO_SOLUZIONE();
};


//----------------------------------------------------------------------------
// MM_PERCORSO_E_SOLUZIONI e relativo elenco dinamico
//----------------------------------------------------------------------------
struct MM_PERCORSO_E_SOLUZIONI {
   MM_I_PERCORSO         * Percorso;
   MM_ELENCO_SOLUZIONE * Soluzioni;
   MM_PERCORSO_E_SOLUZIONI(){Percorso = NULL; Soluzioni = NULL;};
   ~MM_PERCORSO_E_SOLUZIONI(){
      if(Percorso  != NULL)delete Percorso;
      if(Soluzioni != NULL)delete Soluzioni;
   };
   void StoreTo(BUFR & To ,ELENCO & Instradamenti);  // Mette in Instradamenti i percorsi (del Grafo) utilizzati
   void ReStoreFrom(BUFR & From);                    // Richiede che il grafo abbia gia' gli instradamenti
};
// Elenco dinamico di MM_PERCORSO_E_SOLUZIONI
typedef MM_PERCORSO_E_SOLUZIONI* P_MM_PE_SO;
#define DIN_ARRAY MM_ELENCO_I_PERCORSO_E_SOLUZIONI
#define CLAS P_MM_PE_SO
#define DIN_ARRAY_DELETE_OGGETTI_PUNTATI_ALLA_DISTRUZIONE
#define CopiaMembro(_CLAS) (Beep(__FILE__,__LINE__,1),(P_MM_PE_SO)NULL) // Vieto la copia
#include "dinarray.h"


//----------------------------------------------------------------------------
// Classi di output
//----------------------------------------------------------------------------
// Le routine creano oggetti di queste classi.
// Se non cancellati prima del Free (o di cambio Origine, Destinazione) l' applicazione protesta
struct MM_PERCORSO : public OGGETTO,MM_I_PERCORSO {
   MM_PERCORSO(const STRINGA& Nome,class MM_RETE_FS * Creatore);
   void Trace(const STRINGA& Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA){ MM_I_PERCORSO::Trace(Msg,Livello);};
};
struct MM_ELENCO_PERCORSO : public OGGETTO,MM_ELENCO_I_PERCORSO {
   MM_ELENCO_PERCORSO(const STRINGA& Nome,class MM_RETE_FS * Creatore); 
   void Trace(const STRINGA& Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
};
struct MM_ELENCO_PERCORSO_E_SOLUZIONI : public OGGETTO,MM_ELENCO_I_PERCORSO_E_SOLUZIONI {
   MM_ELENCO_PERCORSO_E_SOLUZIONI(const STRINGA& Nome,class MM_RETE_FS * Creatore); 
   void Trace(const STRINGA& Msg, int Livello = LIVELLO_DI_TRACE_DEL_PROGRAMMA);
   void _export Sort(WORD Ora=0xffff); // Sort dell' elenco in base al criterio normale FS.
   // Ora= Minuti dalla mezzanotte  (default = Ora attuale)
   
   void StoreTo(BUFR & To, ELENCO & Instradamenti);  // Mette in Instradamenti i percorsi (del Grafo) utilizzati
   void ReStoreFrom(BUFR & From);                    // Richiede che il grafo abbia gia' gli instradamenti
};

//----------------------------------------------------------------------------
// Questo enum indica il motivo per cui e' chiamato il motore. 
//----------------------------------------------------------------------------
// Da valorizzare OBBLIGATORIAMENTE nella chiamata
enum MOTIVO {
   PER_BIGLIETTARE ,
   PER_INFORMATIVA ,
   PER_ENTRAMBI_I_MOTIVI ,
   PER_BIGLIETTARE_MA_USO_MOTORE_E_POLIMETRICHE,
};

//----------------------------------------------------------------------------
// Questo enum e' per dare al motore un parametro di controllo di modo
//----------------------------------------------------------------------------
enum OPZIONE_UTENTE {
  OPZ_NONE           = 0  ,  // Default

  // Queste di seguito sono opzione assolute: DEVONO essere rispettate
  OPZ_AUTO_AL_SEGUITO     ,
  OPZ_INVALIDI            ,
  OPZ_BICICLETTE          ,
  OPZ_ANIMALI             ,
  OPZ_ASSOLUTE_TAPPO = 100,  // Fine opzioni assolute

  // Queste di seguito sono opzioni di preferenza: 
  // Avvantaggiano le soluzioni che le rispettano
  OPZ_PRENOTABILITA       ,
  OPZ_POSTI_A_SEDERE      ,
  OPZ_CUCCETTE            ,
  OPZ_VAGONI_LETTO        ,
  OPZ_VIAGGIO_ECONOMICO   ,
  OPZ_RISTORANTE          ,
  OPZ_TRENO_VERDE         ,

  OPZ_PREFEREN_TAPPO = 200  // Fine opzioni di preferenza
};

//----------------------------------------------------------------------------
// RETE_FS
//----------------------------------------------------------------------------
// Questa classe rappresenta il grafo della rete FS.
// E' usata per schermare i dettagli algoritmici contenuti tutti
// in Dati Orario
// Deve esistere una sola istance di questa classe. 
// Il nome e' assegnato automaticamente  = RETE_FS
class _export MM_RETE_FS : public OGGETTO {
   public:
   
   // Il costruttore e' cambiato per gestire il doppio orario
   // Inoltre e' divenuto privato
   // PathInCorso e PathProssimo indicano normalmente SUBDIRECTORIES di Path (ma non e' obbligatorio)
   // I dati di struttura della rete non dipendono dall' orario e stanno su Path
   // Il motore parte con l' orario corrente settato
   // Attenzione: i PATH debbono avere l' "\" finale
   static MM_RETE_FS * CreaRete(
      const STRINGA& Path              // Path su cui stanno i files del GRAFO 
     ,const STRINGA& PathInCorso       // Path su cui sta l'orario in corso 
     ,const STRINGA& PathProssimo      // Path su cui sta l'orario prossimo 
     ,const STRINGA& PathCache = NUSTR // Path su cui stanno i files di cache (Per entrambi gli orari)   (default = Path) 
     ,const STRINGA& PathFast1 = NUSTR // Path su cui stanno i files di fast start dell' orario corrente (default = PathInCorso)  
     ,const STRINGA& PathFast2 = NUSTR // Path su cui stanno i files di fast start dell' orario seguente (default = PathProssimo) 
   );
   
   ~MM_RETE_FS();
   
   // ---------------------------------------------------------------------
   // Queste funzioni alterano il comportamento del motore
   // ---------------------------------------------------------------------
   
   // Questa variabile controlla la profondita Minima Media o Massima della ricerca
   // effettuata dal motore. 
   // Se modificata vengono ANCHE resettati tutti i filtri impostati.
   TIPO_RICERCA TipoRicerca;

   void InterrompiElaborazione();
   
   // ---------------------------------------------------------------------
   // Interfaccia pubblica del motore
   // ---------------------------------------------------------------------
   
   // Questo metodo crea un ELENCO_PERCORSO_E_SOLUZIONI 
   // Nell' elenco sono presenti solo le soluzioni che circolano alla data specificata.
   // L' ora espressa come minuti dalla mezzanotte.
   // Il metodo reimposta se necessario l' orario e la data corrente
   // Se nessun orario copre la data richiesta viene utilizzato un differente orario e
   // vengono mandate in out i soli percorsi (solo se Motivo != PER_INFORMATIVA ).
   MM_ELENCO_PERCORSO_E_SOLUZIONI * RichiestaPerRelazione(
      ID             Da                        ,
      ID             A                         ,
      MOTIVO         Motivo                    ,
      SDATA          Data                      ,
      WORD           Ora           = 0xffff    ,
      OPZIONE_UTENTE OpzioneUtente = OPZ_NONE
   );

   // Questo e' un codice di errore impostato dalla precedente chiamata
   enum { 
      RIC_OK,                 // Ok
      RIC_BAD_DATA,           // Data non in orario (o formato errato)
      RIC_BAD_ORA,            // Ora errata (>= 1440)
      RIC_BAD_DA,             // Stazione errata (non esistente o non vendibile)
      RIC_BAD_A,              // Stazione errata (non esistente o non vendibile) 
      RIC_BAD_REL,            // Relazione non accettabile (o non trovata nei precaricati)
      RIC_DA_IS_A,            // Stazione Da == Stazione A
      RIC_BAD_SETTINGS        // Impostazioni generali errate (od altri errori interni del motore) 
   } ReasonCode;
   
   // Questo metodo mette in uso l'orario corrispondente ad un dato giorno
   // per la successiva richiesta. Torna TRUE su errore
   BOOL SetOrarioDelGiorno(SDATA & Day);
   
   // Questo crea un PERCORSO a partire da un insieme di stazioni toccate in un viaggio libero.
   // La variabile AbilitaCumulativo indica se il percorso possa passare anche su rami del cumulativo
   MM_PERCORSO * ViaggioLibero(ARRAY_ID & NodiDelViaggio,BOOL AbilitaCumulativo = FALSE);

   // ---------------------------------------------------------------------
   // Questo metodo e' per il caricamento su TPF ma puo' essere usato anche altrove
   // ---------------------------------------------------------------------
   // Ritorna esattamente gli stessi dati ed errori di richiesta per relazione
   // NB: Non tiene conto della data, ma fornisce la periodicita' in Out
   // NB: E' necessario prima aver chiamato almeno una volta SetOrarioDelGiorno
   MM_ELENCO_PERCORSO_E_SOLUZIONI * MezziDirettiPrenotabili(ID Da,ID A);

   // ---------------------------------------------------------------------
   // Questi metodi danno tutti i treni in partenza (od arrivo) da una stazione
   // ---------------------------------------------------------------------
   // Si puï scegliere se ottenere tutti i treni o solo quelli che originano dalla
   // stazione, ed eventualmente specificare un tempo minimo di fermata (In minuti)
   // NB: Il percorso e' NULL. Le soluzioni sono ordinate per ora di partenza (arrivo)
   MM_ELENCO_PERCORSO_E_SOLUZIONI * MezziInPartenza(ID Da,BOOL SoloOriginanti,BOOL SoloPrenotabili,WORD MinFermata); 
   MM_ELENCO_PERCORSO_E_SOLUZIONI * MezziInArrivo(ID Da,BOOL SoloDestinati,BOOL SoloPrenotabili,WORD MinFermata); 

   // ---------------------------------------------------------------------
   // Questa variabile mi dice se debbo scartare o meno le soluzioni
   // NON TARIFFABILI (che comunque possono essere utili per informativa)
   // ---------------------------------------------------------------------
   // Di default NON le scarto , tranne che per internet
   BOOL ScartaSoluzioniNonTariffabili;

   // ---------------------------------------------------------------------
   // Questo metodo imposta la lingua in cui fornisco le note.
   // ---------------------------------------------------------------------
   // Si puo' modificare direttamente prima della chiamata a MM_SOLUZIONE::GetNote();
   LINGUA Linguaggio ;

   static MM_RETE_FS  * Rete      ;  // Solo una rete per volta
   
   // Limiti dell' orario corrente
   SDATA      InizioValiditaOrario;
   SDATA      FineValiditaOrario  ;

   // Data corrente della richiesta
   SDATA      DataCorrente;

   // ---------------------------------------------------------------------
   // END
   // ---------------------------------------------------------------------
   // Ciï che segue e' solo per test o da usarsi all' interno del motore

   // Questo metodo attiva l' AutoDiagnostica
   // L' autodiagnostica si disattiva da sä alla fine di ogni richiesta
   void AttivaAutoDiagnostica(STRINGA & ElencoTreni);
   void AttivaAutoDiagnosticaPath(ARRAY_ID & StazioniIntermedie);

   // Questo rilascia i buffers interni se inattivo per evitare tempi inutili di attesa.
   // Puo' anche essere chiamato dall' applicazione
   void Free();
   
   // I dettagli interni sono nascosti al di fuori della DLL del motore
   #ifdef IN_MOTORE_CPP
   #include "I2_MOTO.hpp"
   #endif
   
   private:
   
   // Il costruttore ora e' privato
   MM_RETE_FS(
      const STRINGA& Path         // Path su cui stanno i files del GRAFO 
     ,const STRINGA& PathInCorso  // Path su cui sta l'orario in corso 
     ,const STRINGA& PathProssimo // Path su cui sta l'orario prossimo 
     ,const STRINGA& PathCache    // Path su cui stanno i files di cache (Per entrambi gli orari)
     ,const STRINGA& PathFast1    // Path su cui stanno i files di fast start dell' orario corrente 
     ,const STRINGA& PathFast2    // Path su cui stanno i files di fast start dell' orario seguente 
   );
   
   
};

#endif
