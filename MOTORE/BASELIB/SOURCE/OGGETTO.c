//----------------------------------------------------------------------------
// FILE OGGETTO.C
//----------------------------------------------------------------------------

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°° REPORT AGGIORNAMENTI °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Data ultima modifica documentazione:         16/10/92;

// Data ultima modifica codice:                 00/00/93;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°° DESCRIZIONE DELLA CLASSE °°°°°°°°°°°°°°°°°°°°°°°°°°°°
//
// Questo file implementa la classe OGGETTO.
//
// La classe OGGETTO Š la superclasse della gerarchia della OOLIB dalla quale
// ereditano le classi che gestiscono: la visualizzazione (VIEW e PM), gli oggetti
// del modello, i link e l'Objset.

//per favore non cambiare il livello di trace
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1       // Indica con quale livello di trace
#define MODULO_OS2_DIPENDENTE                   // deve essere compilato questo file;
#define INCL_DOSMISC
#define INCL_WINATOM
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES   // Semaphore values

#include "oggetto.h"
#include "elenco_o.h"
#include "scandir.h"

static HMTX  Rel_hmtx;               // Mutex semaphore handle

void _export Doe1()
{
   #undef TRCRTN
   #define TRCRTN "DOE1"
   // ---------------------------------------------------------------
   // Ottengo il semaforo di accesso all' area gestione della memoria
   // ---------------------------------------------------------------
   APIRET   rc;               // Return code
   if(Rel_hmtx == NULL){
         // Creo il semaforo di accesso all' area gestione della memoria
         rc = DosCreateMutexSem(NULL,&Rel_hmtx,0,0);
         if (rc != 0){
            TRACELONG("errore nella creazione del semaforo, rc = ",rc);
            BEEP;
            return;
         };
   };
   // Wait massimo 10 secondi: dopodiche' lo considero un DeadLock
   rc = DosRequestMutexSem(Rel_hmtx, 10000);
   if (rc != 0){
      TRACELONG("Errore nell' accesso al semaforo, rc = ",rc);
      if(rc == 640){
         ERRSTRING("E' un TimeOut: vuol dire che vi era stato un deadlock");
      };
      BEEP;
   }
}

void _export Doe2()
{
   #undef TRCRTN
   #define TRCRTN "DOE2"
   // ---------------------------------------------------------------
   // Rilascio il semaforo di accesso all' area gestione della memoria
   // ---------------------------------------------------------------
   APIRET   rc;               // Return code
   rc = DosReleaseMutexSem(Rel_hmtx);
   if (rc != 0){
      TRACELONG("errore nel rilascio del semaforo, rc = ",rc);
      BEEP;
   }
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°    OGGETTO()    °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Costruttore di default, crea un oggetto con nome Nom di tipo Tipo in eventuale
// relazione di composizione con l'oggetto Compo che funge da oggetto componente;
// La initialization list inizializza i valori degli attributi Nome e TipoOggetto;
// L'inizializzazione degli attributi privati dell'oggetto viene effettuata dalla
// procedure Init();

OGGETTO::OGGETTO(const STRINGA & Nom,
                       TIPOGGE Tipo,
                       OGGETTO * Compo,
                       BOOL IsCompon ):
         Nome(Nom),TipoOggetto(Tipo)
{
   if( !Nome.Dim() ) {  ERRSTRING("ERRORE Oggetto senza Nome."); BEEP; }
   Init(Compo);
   IsCompo = IsCompon;
}

// Costruttore che richiede un Object Set;
// crea un oggetto con nome Nom di tipo Tipo in eventuale relazione di composizione
// con l'oggetto Compo che funge da oggetto componente; se IsCompon Š TRUE l'oggetto
// assume funzione di oggetto componente in una relazione di composizione;
// mette in relazione l'oggetto creato con l'Object Set e con l'oggetto che funge da
// componente in una relazione di composizione cui l'oggetto appartiene;


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°   ~OGGETTO()    °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°


// Distruttore virtuale;

OGGETTO::~OGGETTO()
{
#undef TRCRTN
#define TRCRTN "~OGGETTO"

// Effettuo una trace dell'oggetto in distruzione;
// TRACESTRING("Oggetto:"+Nome+", distrutto at "+STRINGA((void*)this)+" @@");

   InFaseDiDistruzione = TRUE;
   int i;

// Elimino tutte le relazioni di tipo generico tra l'oggetto ed altri oggetti;

// Cancello le relazioni generiche cui partecipo come OWNER: si fa affidamento
// sul fatto che nell'elenco di oggetti Members l'ordine di indice corrisponda
// con l'ordine temporale di creazione;

      while ( (i= Members.Dim()-1) >= 0 )
         this->UnSetRelazione(&Members[i]);

// Cancello le relazioni generiche e con nome(applicative)
// cui partecipo come MEMBER
      while ( (i= Owners.Dim()-1) >= 0 ){
         Owners[i].UnSetRelazione(this);
      }

// Cancello le relazioni di composizione delle quali l'oggetto this ha funzione di
// oggetto componente; indispensabile per la corretta distruzione dgli oggetti PM;
   FORALL(OggettiDellaComponente,k) {
      this->UnSetSubordinato(& OggettiDellaComponente[k]);
   }

// Elimino la relazione con l'oggetto componente a cui eventualmente appartengo;
   if(Componente != NULL) {
      Componente->UnSetSubordinato(this);
   }

// Tolgo l'oggetto dall'elenco di tutti le istanze della classe OGGETTO;
   DOE1;
   OGGETTO::All -= * this;
   DOE2;
// Effettuo una trace dell'oggetto in distruzione;
   TRACESTRINGL("Oggetto:"+Nome+", distrutto at "+STRINGA((void*)this)+" @@",2);
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°    Valore()     °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

//-----------------------------------------------------------------------------
// Restituisce il valore associato all' oggetto;
// per default e' uguale NULL, solo alcuni oggetti hanno associato un valore;
//-----------------------------------------------------------------------------
const STRINGA & OGGETTO::Valore()const
{
   return *(STRINGA *)NULL;
}

//-----------------------------------------------------------------------------
// Per gli oggetti multivalore
//-----------------------------------------------------------------------------
const ELENCO_S & OGGETTO::ValoreElenco() const
{
   return *(ELENCO_S *)NULL;
}

BOOL OGGETTO::IsMultivalore() const { return FALSE; };


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°    Lock()     °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Restituisce il nome del Lock applicato all'oggetto;
// Il nome lock deve sempre essere riferito al top della catena di subordinazione;

const STRINGA  OGGETTO::Lock()const
{
      return NUSTR;
}



// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° SetRelazione() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Stabilisce una relazione generica aggiungendo all'elenco degli oggetti Members
// l'oggetto Member; l'oggetto this assume funzione di OWNER nella relazione;

void OGGETTO::SetRelazione(OGGETTO * Member)
{
#undef TRCRTN
#define TRCRTN "OGGETTO::SetRelazione"

   if(Member == NULL )
   {
      TRACEPOINTER("Errore : il Member = " , Member );
      return;
   }
   DOE1;
   Members += * Member;

   // Aggiunge all'elenco di oggetti Owners dell'oggetto Member l'oggetto
   // this che ha funzione di OWNER;

   Member->Owners += * this;
   DOE2;
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° UnSetRelazione() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Elimina la relazione generica togliendo all'elenco degli oggetti Members
// l'oggetto Member; l'oggetto this aveva funzione di OWNER nella relazione;

void OGGETTO::UnSetRelazione(OGGETTO * Member)
{
#undef TRCRTN
#define TRCRTN "OGGETTO::UnSetRelazione"

   if (Member == NULL )
   {
      TRACEPOINTER("Errore : il Member = " , Member );
      return;
   }

// LSS Amedeo vuole che questo metodo tolga dagli elenchi () soltanto
// quegli oggetti che sono stati inseriti dal metodo SetRelazione(OGGETTO*)
// e lasci gli oggetti inseriti dal metodo SetRelazione(...,NomeRelazione)
// Quindi vanno tolti quegli oggetti con Num.Rel. pari a 0;

   DOE1;
   if (Members.TipoAreaAssociata() == ELENCO_Oggetti::Relaz_ID) {
      FORALL( Members, i) {
         if( (&(Members[i]) == Member ) && Members.NumAssociato(i) == 0)
              Members -= i;
      }
      FORALL( Member->Owners, j) {
         if( (&(Member->Owners[j]) == this) && Member->Owners.NumAssociato(j) == 0)
              Member->Owners -= j;
      }

   } else {

      FORALL( Members, i) {
         if( &(Members[i]) == Member )
              Members -= i;
      }
      FORALL( Member->Owners, j) {
         if( &(Member->Owners[j]) == this)
              Member->Owners -= j;
      }
   } /* endif */
   DOE2;
}

// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° SetSubordinato() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
// Stabilisce una relazione di composizione fra l'oggetto this che ha funzione di
// oggetto componente e l'oggetto Member che viene ad esso subordinato;

void OGGETTO::SetSubordinato(OGGETTO * Member)
{
#undef TRCRTN
#define TRCRTN "OGGETTO::SetSubordinato"

   if(Member == NULL ) {
      TRACEPOINTER("Errore : il Member = " , Member );
      return;
   }

// L'oggetto Member non Š ancora subordinato in una relazione di composizione;
   if(Member->Componente == NULL) {
      DOE1;
      OggettiDellaComponente += * Member;
      Member->Componente = this;
      DOE2;
   } else if(Member->Componente != this) {

   // L'oggetto Member Š gi… subordinato in un'altra relazione di composizione
   // con un oggetto diverso dall'oggetto this;

      TRACESTRING2("Non posso rendere subordinato l' oggetto",Member->Nome);
      TRACESTRING2("Ha gia' come componente l' oggetto",Member->Componente->Nome);
      BEEP;
   }
   // L'oggetto Member Š gi… subordinato all'oggetto this, tutto OK;
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°° UnSetSubordinato() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Elimina la relazione di composizione fra l'oggetto this che ha funzione di
// oggetto componente e l'oggetto Member che Š ad esso subordinato;
// L'eliminazione della relazione non implica la distruzione dei members;

void OGGETTO::UnSetSubordinato(OGGETTO * Member)
{
#undef TRCRTN
#define TRCRTN "OGGETTO::UnSetSubordinato"

   if(Member == NULL ) {
      TRACEPOINTER("Errore : il Member = " , Member );
      return;
   }
   if(Member->Componente != NULL && Member->Componente != this) {

   // L'oggetto Member Š subordinato ad un'oggetto diverso da this;

      TRACESTRING2("Non posso NON rendere subordinato l' oggetto",Member->Nome);
      TRACESTRING2("Ha come componente l' oggetto",Member->Componente->Nome);
      BEEP;
   } else {
      DOE1;
      OggettiDellaComponente -= * Member;
      Member->Componente = NULL;
      DOE2;
      if( DistruggiSubord
          && !Member->InFaseDiDistruzione ) {
           Member->InFaseDiDistruzione = TRUE;
           delete Member;
      }
   }
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°  IsSubordinato()  °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Indica se l'oggetto Member Š subordinato all'oggetto this in una relazione
// gerarchica di composizione;

BOOL OGGETTO::IsSubordinato(OGGETTO * Member)
{
#undef TRCRTN
#define TRCRTN "OGGETTO::IsSubordinato"

   return OggettiDellaComponente.CercaPerPuntatore(Member) >= 0;
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° GetSubordinato() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Restituisce il puntatore all'oggetto di nome ONome subordinato all'oggetto
// this all'interno di una relazione di composizione; Restituisce un puntatore
// a NULL nel caso l'oggetto con nome ONome non sia subordinato all'oggetto
// this; Il parametro Last indica da quale oggetto cominciare a cercare
// all'interno della lista degli oggetti subordinati;

OGGETTO * OGGETTO::GetSubordinato(const STRINGA & ONome,OGGETTO * Last)
{
#undef TRCRTN
#define TRCRTN "OGGETTO::GetSubordinato"

   return OggettiDellaComponente.CercaPerNome(ONome,Last);
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° NomeClasseBase() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Restituisce il nome della classe BASE cui appartiene l'oggetto;

const STRINGA & OGGETTO::NomeClasseBase()const
{
   return BaseClass;
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° NomeClasse() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Restituisce il nome della classe cui appartiene l'oggetto; Se l'oggetto
// appartiene ad una classe che sub_classa una classe di BASE i due nomi
// della classe e della classe base saranno diversi;

const STRINGA & OGGETTO::NomeClasse()const
{
   return ClassName;
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° SubClassato() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Restituisce TRUE se l'oggetto appartiene ad una classe che sub_classa una classe
// BASE;

BOOL OGGETTO::SubClassato()const
{
   return (ClassName != BaseClass);
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°    IsA()    °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Restituisce TRUE se l'oggetto this appartiene alla classe 'Classe' o se appartiene
// ad una classe della gerarchia che discende da essa;

BOOL OGGETTO::IsA(const STRINGA & Classe)const
{
   if(Classe == OGGETTO::NomeClasse())
      return TRUE;
   if(Classe == OGGETTO::NomeClasseBase())
      return TRUE;
   return (Classe == "OGGETTO");
}


// °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°   Init()   °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°

// Questo metodo viene richiamato da tutti i costruttori per inizializzare a valori
// di default l'istanza di classe OGGETTO. Viene aggiornato l'elenco All ed eventual-
// mente gestisco la relazione di composizione se l'oggetto e' subordinato

void OGGETTO::Init(OGGETTO * Compo)
{
#undef TRCRTN
#define TRCRTN "@OGGETTO"
// char tmp[100];
// sprintf(tmp,"Oggetto:%s Creato at %p @@",(CPSZ)this->Nome,this);
   TRACESTRINGL( "Oggetto:" + Nome + ", Creato at " + STRINGA((void*)this) + " @@",2);
   // Aggiungo l'oggetto all'elenco All di tutti gli oggetti

   DOE1;
   OGGETTO::All += * this;
   DOE2;

   Componente = NULL;
   IsCompo = TRUE;
   InFaseDiDistruzione = FALSE;
   DistruggiSubord = FALSE;

   // Subordino l'oggetto this se Š subordinato in una relazione di composizione
   // all'oggetto Compo;

   if(Compo != NULL) {
      IsCompo = FALSE;
      Compo->SetSubordinato(this);
   }

   // Quando MustFail viene settato a vero si intende terminare l'esecuzione del
   // costruttore e si distrugge l'oggetto.

   MustFail = FALSE;
   ClassName = BaseClass = "UNKNOWN";
}



// °°°°°°°°°°°°°°°°°°°°°°°°°  Metodi per la gestione dei LINK  °°°°°°°°°°°°°°°°°°°°°°

// Se non sono ridefiniti provocano un abend !


BOOL OGGETTO::VerificaFormaleValore(const STRINGA * Valore,
     OGGETTO * Origine, class LINK * Link, const STRINGA * NmLock)
{
   TRACESTRING("Ridefinire il metodo 'OGGETTO::VerificaFormaleValore'");
   Abend(1, this);
   return TRUE;
}


BOOL OGGETTO::Notifica_FaseDiVerifica(OGGETTO * Origine,
     class LINK * Link, const STRINGA * NmLock)
{
   TRACESTRING("Ridefinire il metodo 'OGGETTO::Notifica_FaseDiVerifica'");
   Abend(1, this);
   return TRUE;
}


BOOL OGGETTO::Agg_View_conn_da_Link(class OGGETTO_VIEW * Origine,
     class LINK * Link, const STRINGA * NmLock)
{
   TRACESTRING("Ridefinire il metodo 'OGGETTO::Agg_View_conn_da_Link'");
   Abend(1, this);
   return TRUE;
}


BOOL OGGETTO::Notif_Da_View_conn_da_Link(class OGGETTO_VIEW * Origine,
     class LINK * Link, const STRINGA * NmLock)
{
   TRACESTRING("Ridefinire il metodo 'OGGETTO::Notif_Da_View_conn_da_Link'");
   Abend(1, this);
   return TRUE;
}

BOOL OGGETTO::Agg_Model_conn_da_Link(OGGETTO * Origine,
     class LINK * Link, const STRINGA * NmLock, BOOL HoSyncPoint)
{
   TRACESTRING("Ridefinire il metodo 'OGGETTO::Agg_Model_conn_da_Link (STRINGA)'"+ Nome);
   Abend(1, this);
   return TRUE;
}

BOOL OGGETTO::Agg_Model_conn_da_Link( OGGETTO * Origine, class LINK *Link,
                             const ELENCO_S* NmLock, BOOL HoSyncPoint)
{
   return FALSE;
}
BOOL OGGETTO::Agg_Model_conn_da_Link( OGGETTO * Origine, class LINK *Link,
                             const ELENCO_EL_S* NmLock, BOOL HoSyncPoint)
{
   return FALSE;
}

BOOL OGGETTO::Notif_Da_Model_conn_da_Link(OGGETTO * Origine,
     class LINK * Link, const STRINGA * NmLock, BOOL HoSyncPoint)
{
   TRACESTRING("Ridefinire il metodo 'OGGETTO::Notif_Da_Model_conn_da_Link'");
   Abend(1, this);
   return TRUE;
}


// °°°°°°°°°°°°°°°°°°°°°°°°  CercaSubordinati() °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°
// Ricerche per oggetti di una data classe (controllata con IsA)
ELENCO_Oggetti OGGETTO::CercaSubordinati(const STRINGA & Classe){
   if(Classe == NUSTR)return OggettiDellaComponente;
   DOE1;
   ELENCO_Oggetti Out;
   ORD_FORALL(OggettiDellaComponente,i){
     if(OggettiDellaComponente[i].IsA(Classe))
        Out+=OggettiDellaComponente[i];
   }
   DOE2;
 return Out;
}


SHORT OGGETTO::GetOwnerPos( OGGETTO* Owner , USHORT Atom )
{
   BOOL Found = FALSE;
   if( Owner  == NULL ) return -1;
   if( Owners.TipoAreaAssociata() != ELENCO_Oggetti::Relaz_ID)return -1;

   DOE1;
   ORD_FORALL( Owners , Ind ) {
      if( &Owners[Ind] == Owner && Owners.NumAssociato( Ind ) == Atom ) {
         Found = TRUE;
         break;
      }
   }
   DOE2;
   return (Found) ? Ind : -1 ;
}

//----------------------------------------------------------------------------
// Questi metodi debbono avere livello di trace 1
//----------------------------------------------------------------------------
#undef LIVELLO_DI_TRACE_DEL_PROGRAMMA
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1       // Indica con quale livello di trace

//----------------------------------------------------------------------------
// Questi metodi con le relazioni generiche solo per il subclass di OOLIB
//----------------------------------------------------------------------------
const ELENCO_Oggetti&  OGGETTO::CercaOwners(BOOL AncheAssociati) {
   if((Owners.TipoAreaAssociata() != ELENCO_Oggetti::Relaz_ID) || AncheAssociati) return Owners;
   return Owners.CercaNonAssoc();
};
const ELENCO_Oggetti&  OGGETTO::CercaMembers(BOOL AncheAssociati) {
   if((Members.TipoAreaAssociata() != ELENCO_Oggetti::Relaz_ID) || AncheAssociati) return Members;
   return Members.CercaNonAssoc();
};
BOOL OGGETTO::Owns(OGGETTO * Member,BOOL AncheAssociati){
   return CercaMembers(BOOL(AncheAssociati)).Contiene(Member);
};

BOOL OGGETTO::Owned(OGGETTO * Owner,BOOL AncheAssociati){
   return CercaOwners(AncheAssociati).Contiene(Owner);
};

//++++++++++++++++++++++++++++++++++++++++++++++++++
// Time() Ritorna il tempo in centesimi di secondo dalla mezzanotte
//++++++++++++++++++++++++++++++++++++++++++++++++++
ULONG Time(){

   static ULONG    LastHMSH,Last;

   struct My_DATETIME    // Ridefinizione di DATETIME
      { ULONG   hmsh;    // Normalmente visto come ore minuti secondi centesimi.
        UCHAR   day; UCHAR   month; USHORT  year; SHORT   timezone; UCHAR   weekday;
      };

   union DT { My_DATETIME My ; DATETIME Std;};

   DT Now;
   DosGetDateTime(&Now.Std);
// Forse questi confronti possono essere migliorati ....
   if(Now.My.hmsh == LastHMSH) return Last;
   LastHMSH = Now.My.hmsh ;
   Last = (ULONG)Now.Std.hundredths +
          (ULONG)Now.Std.seconds * 100L +
          (ULONG)Now.Std.minutes * 6000L+
          (ULONG)Now.Std.hours   * 360000L;
   return Last;
};

//++++++++++++++++++++++++++++++++++++++++++++++++++
// TimePart() Ritorna il tempo in centesimi di secondo dalla partenza del programma (gestisce cambi giorno / mese / anno) 
//++++++++++++++++++++++++++++++++++++++++++++++++++
ULONG _export TimePart() {

   struct My_DATETIME          // Ridefinizione di DATETIME
   { ULONG   hmsh;             // Normalmente visto come ore minuti secondi centesimi.
     ULONG   daymonthyear;     // Mormalmente visto come giorno mese anno
     SHORT   timezone; UCHAR   weekday;
   };
   static ULONG LastData;
   static int   LastDeltaGG;
   union DT { 
      My_DATETIME My ; 
      DATETIME Std;

      int Giuliano()const{
         static int  valInzMese[2][13]=
         // g f  m  a  m   g   l   a   s   o   n   d    g
         {  0,31,59,90,120,151,181,212,243,273,304,334,366,
            0,31,60,91,121,152,182,213,244,274,305,335,367
         };
         // Bisestile: anno divisibile per 4, non divisibile per 100 oppure divisibile per 400
         int bis = (!(Std.year % 4)) && ((Std.year % 100) || !(Std.year % 400) );
         if( bis ) bis = 1;
         return valInzMese[bis][Std.month-1] + Std.day;
      };

      int DeltaGG(DT & b){
         if(My.daymonthyear ==  LastData) return LastDeltaGG;
         LastData = My.daymonthyear;
         int DeltaAnni = Std.year - b.Std.year;
         int DeltaGiorni = DeltaAnni * 365; // Differenza tra i 31/12 se non vi fossero anni bisestili
         int A1 = Std.year-1;   // Per calcolare la differenza al 31/12 dell' anno precedente
         int NumBisestili  =  A1 / 4 - A1 / 100 + A1 / 400;
         DeltaGiorni += NumBisestili;  
         A1 = b.Std.year-1; // Per calcolare la differenza al 31/12 dell' anno precedente
         NumBisestili  =  A1 / 4 - A1 / 100 + A1 / 400;
         DeltaGiorni -= NumBisestili;      //  Differenza al 31/12
         DeltaGiorni += (Giuliano() - b.Giuliano());
         LastDeltaGG = DeltaGiorni;
         return DeltaGiorni;
      }
   };

   #if sizeof( My_DATETIME ) != sizeof( DATETIME ) 
   #error "Controllare prego"
   #endif
   
   static ULONG      LastHMSH,Last;
   static DT         TempoAllaPartenza;
   static BOOL       Partito;
   
   
   
   if(!Partito){
      Partito = TRUE;
      Last = 0;
      DosGetDateTime(&TempoAllaPartenza.Std);
   };
   
   DT Now;
   DosGetDateTime(&Now.Std);
   if(Now.My.hmsh == LastHMSH) return Last;
   LastHMSH = Now.My.hmsh ;
   int DeltaGiorni = Now.DeltaGG(TempoAllaPartenza);
   Last =    (  (int)Now.Std.hundredths - (int)TempoAllaPartenza.Std.hundredths  )  +
   100     * (  (int)Now.Std.seconds    - (int)TempoAllaPartenza.Std.seconds     )  +
   6000    * (  (int)Now.Std.minutes    - (int)TempoAllaPartenza.Std.minutes     )  +
   360000  * (  (int)Now.Std.hours      - (int)TempoAllaPartenza.Std.hours       )  +
   8640000 * DeltaGiorni;
   
   // Ignoro cambio mese od anno
   return Last;
//<<< ULONG _export TimePart   
};
