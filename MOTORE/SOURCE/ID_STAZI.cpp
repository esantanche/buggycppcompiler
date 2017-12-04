// ---------------------------------------------------------------------
// ID_STAZI.CPP: classe STAZIONI e collegate
// ---------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 1

// EMS001
typedef unsigned long BOOL;

#define MODULO_OS2_DIPENDENTE
#define  NO_TRC_FOPEN // Per motivi di performance: Evito la ridefinizione delle funzioni di semaforizzazione
#define INCL_DOSSEMAPHORES   /* Semaphore values */
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "MM_VARIE.HPP"
#include "MM_BASIC.HPP"

char * _export DecodificaRcOS2(int Rc);

// ---------------------------------------------------------------------
// Aree statiche
// ---------------------------------------------------------------------
FILE_STZ  *             STAZIONI::FStazioni              ; // File delle stazioni
FILE_CCR  *             STAZIONI::FCcr                   ; // File dei codici CCR
int                     STAZIONI::CountOpened            ; // Registra il numero delle istanze: quando va a 0 chiude il file
HMTX                    STAZIONI::Mutex                  ; // Mutex per sincronizzare accessi multithread;
class CCR_ID *          CCR_ID::CcrId                    ; // Conversione da codice CCR ad Id
static CACHE<STAZIONI::R_STRU> StzCache(512,sizeof(ID))            ; // StzCache delle stazioni accedute piu' recentemente
static CACHE<CCR_STRU>         StzCacheCcr(512,sizeof(CCR_STRU))   ; // StzCache dei codici CCR (per estere)
static ARRAY_DINAMICA<CCR_STRU> ArrayCcrItaliani(3000)             ; // Codici CCR delle sole stazioni italiane (array sortata)

//----------------------------------------------------------------------------
// @STAZIONI
//----------------------------------------------------------------------------
STAZIONI::STAZIONI(const STRINGA& Path,const STRINGA & NomeFile,int PageSize){
   #undef TRCRTN
   #define TRCRTN "@STAZIONI"
   GetMutex();
   if(PageSize < 0) PageSize = 2048 - (2048 % sizeof(R_STRU) ); // E' il default
   Corrente = NULL;
   if (FStazioni == NULL) {
      FStazioni = new FILE_STZ(Path+NomeFile,PageSize);
      if(PageSize == 0){ // Tiene tutto in memoria
         FCcr      = new FILE_CCR(Path+"ID_CCR.DB",0);
      } else {
         FCcr      = new FILE_CCR(Path+"ID_CCR.DB",1024);
      } /* endif */
   } /* endif */
   if (ArrayCcrItaliani.Dim() == 0 ) {
      int Lim = min( 5000, FStazioni->Dim());// Cerco solo nei primi 5000 Id (e' solo uno speedup: non vi sarebbero problemi se qualche codice rimanesse fuori).
      for (int i = 0; i < Lim ; i++) {
         FStazioni->Posiziona(i);
         STAZIONI::R_STRU & staz = FStazioni->RecordCorrente();
         if(staz.Vendibile()) { // Solo per le stazioni vendibili ( e solo i codici CCR italiani)
            CCR_STRU Rec;
            Rec.Id = staz.IdStazione;
            if (staz.CodiceCCR) {
               Rec.CodiceCCR = staz.CodiceCCR;
               ArrayCcrItaliani += Rec;
            }
            if (staz.CCRCumulativo1) {
               Rec.CodiceCCR = staz.CCRCumulativo1;
               ArrayCcrItaliani += Rec;
            };
            if (staz.CCRCumulativo2) {
               Rec.CodiceCCR = staz.CCRCumulativo2;
               ArrayCcrItaliani += Rec;
            };
         } /* endif */
      } /* endfor */
    qsort((void *)&ArrayCcrItaliani[0], ArrayCcrItaliani.Dim(),sizeof(CCR_STRU),CompareDoubleWord );
//<<< if  ArrayCcrItaliani.Dim   == 0
   } /* endif */
   CountOpened ++;
   FreeMutex();
//<<< STAZIONI::STAZIONI const STRINGA& Path,const STRINGA & NomeFile,int PageSize
};
//----------------------------------------------------------------------------
// ~STAZIONI
//----------------------------------------------------------------------------
STAZIONI::~STAZIONI(){
   #undef TRCRTN
   #define TRCRTN "~STAZIONI"
   GetMutex();
   if (Corrente) {
      StzCache.UnLock(*Corrente);
   } /* endif */
   CountOpened --;
   if (CountOpened < 0 ) { // Controllo
      CountOpened = 0;
      BEEP;
   };
   if (CountOpened == 0 && FStazioni != NULL) {
      delete FStazioni;
      delete FCcr;
      StzCache.Reset();
      StzCacheCcr.Reset();
      FStazioni = NULL;
      FCcr      = NULL;
   } /* endif */
   FreeMutex();
//<<< STAZIONI::~STAZIONI
};

//----------------------------------------------------------------------------
// STAZIONI::Posiziona
//----------------------------------------------------------------------------
// Torna TRUE se trova la stazione
BOOL STAZIONI::Posiziona(WORD Id){
   #undef TRCRTN
   #define TRCRTN "STAZIONI::Posiziona"
   GetMutex();
   //TRACEVLONG(Id);
   if (Corrente) {
      if(Corrente->IdStazione != Id){ // Se non e' la stazione corrente
         StzCache.UnLock(*Corrente);
         Corrente = StzCache.GetSlot((R_STRU*)&Id, LR_LOCK); // Accede se necessario al file!
         //TRACESTRING("Primo ramo if in Posiziona");
         //TRACESTRING(Corrente->NomeStazione);
      }
   } else {
      Corrente = StzCache.GetSlot((R_STRU*)&Id, LR_LOCK); // Accede se necessario al file!
      //TRACESTRING("Secondo ramo if in Posiziona");
      //TRACESTRING(Corrente->NomeStazione);
   } /* endif */
   FreeMutex();
   return Corrente != NULL;
};

//----------------------------------------------------------------------------
// STAZIONI::PosizionaPerCCR
//----------------------------------------------------------------------------
ID  STAZIONI::PosizionaPerCCR(DWORD CCR){
   #undef TRCRTN
   #define TRCRTN "STAZIONI::PosizionaPerCCR"
   ID Id= DecodificaCCR(CCR);
   if(Id) Posiziona(Id);
   return Id;
};

//----------------------------------------------------------------------------
// STAZIONI::DecodificaCCR
//----------------------------------------------------------------------------
ID  STAZIONI::DecodificaCCR(DWORD CCR){
   #undef TRCRTN
   #define TRCRTN "STAZIONI::DecodificaCCR"
   CCR_STRU * Pnt = NULL;
   if(CCR >= 8300000 && CCR < 8400000) CCR -= 8300000 ; // Gestione stazioni italiane
   if (CCR < 100000 ) {
      Pnt = (CCR_STRU *)bsearch(&CCR, (void *)&ArrayCcrItaliani[0], ArrayCcrItaliani.Dim(),sizeof(CCR_STRU),CompareDoubleWord );
   } /* endif */
   if(Pnt)return Pnt->Id;
   GetMutex();
   Pnt = StzCacheCcr.GetSlot((CCR_STRU*)&CCR, LR_NO_LOCK);
   FreeMutex();
   if(Pnt)return Pnt->Id;
   return 0;
};

//----------------------------------------------------------------------------
// STAZIONI::GetMutex
//----------------------------------------------------------------------------
// Ottiene il controllo delle risorse . return FALSE su errore
BOOL STAZIONI::GetMutex(){
   #undef TRCRTN
   #define TRCRTN "STAZIONI::GetMutex"
   ULONG Rc ;
   if (Mutex == 0) {
      Rc= DosCreateMutexSem(NULL, &Mutex, 0L, 0L);
      if (Rc) {
         ERRSTRING("Errore creando semaforo di accesso a stazioni: "+STRINGA(DecodificaRcOS2(Rc)));
         ERRSTRING("L' applicazione deve essere fermata! Provoco un ABEND");
         BEEP;
         char Buf[20];
         sprintf(Buf,"%s",(char*)NULL); // Provoco volutamente un abend
      } /* endif */
   } /* endif */
   Rc = DosRequestMutexSem( Mutex , 60000); // Max wait 1 minuto
   if(Rc){
      ERRSTRING("Errore nell' accesso al Mutex delle stazioni:"+STRINGA(DecodificaRcOS2(Rc)));
      BEEP;
      return FALSE;
   }
   return TRUE;
//<<< BOOL STAZIONI::GetMutex
};
//----------------------------------------------------------------------------
// STAZIONI::FreeMutex
//----------------------------------------------------------------------------
BOOL STAZIONI::FreeMutex(){
   #undef TRCRTN
   #define TRCRTN "STAZIONI::FreeMutex"
   ULONG Rc = DosReleaseMutexSem( Mutex );
   if(Rc){
      ERRSTRING("Errore nel rilascio del Mutex delle stazioni:"+STRINGA(DecodificaRcOS2(Rc)));
      // Non mando il beep perche' l' errore e' stato quasi sicuramente gia' segnalato
      return FALSE;
   }
   return TRUE;
};

//----------------------------------------------------------------------------
// STAZIONI::R_STRU::InitForCache
//----------------------------------------------------------------------------
BOOL STAZIONI::R_STRU::InitForCache(class STAZIONI::R_STRU * Key, void *){
   #undef TRCRTN
   #define TRCRTN "STAZIONI::R_STRU::InitForCache"
   ID Id = Key->IdStazione;
   // Accedo al file
   // EMS002 VA devo sostituire a FStazioni, STAZIONI::FStazioni
   if(STAZIONI::FStazioni->Posiziona(Key->IdStazione)){
      // EMS002 VA
      memmove(this,&STAZIONI::FStazioni->RecordCorrente(), sizeof(STAZIONI::R_STRU));
      assert(Id == IdStazione || IdStazione == 0);
      IdStazione = Id;
      return TRUE;
   } else {
      ZeroFill(THIS);
      IdStazione = Id;
      return FALSE;
   };
};

//----------------------------------------------------------------------------
// STAZIONI::DimBuffers
//----------------------------------------------------------------------------
ULONG STAZIONI::DimBuffers(){
   #undef TRCRTN
   #define TRCRTN "STAZIONI::DimBuffers"
   ULONG Size = 0;
   Size += FStazioni->DimBuffers();
   Size += FCcr->DimBuffers();
   Size += StzCache.AreaAllocata();
   Size += StzCacheCcr.AreaAllocata();
   return Size;
};
//----------------------------------------------------------------------------
// STAZIONI::Dim
//----------------------------------------------------------------------------
int STAZIONI::Dim() {
   #undef TRCRTN
   #define TRCRTN "STAZIONI::Dim"
   return FStazioni->NumRecordsTotali();
};
//----------------------------------------------------------------------------
// CCR_STRU::InitForCache
//----------------------------------------------------------------------------
BOOL CCR_STRU::InitForCache(class CCR_STRU * Key, void * ){
   #undef TRCRTN
   #define TRCRTN "CCR_STRU::InitForCache"
   // Accedo al file
   //TRACESTRING("Sto per chiamare la Seek");
   if(STAZIONI::FCcr->Seek(Key->CodiceCCR)){
      memmove(this,&STAZIONI::FCcr->RecordCorrente(), sizeof(CCR_STRU));
      return TRUE;
   };
   return FALSE;
};

//----------------------------------------------------------------------------
// STAZIONI_BYNAME::KeySet
//----------------------------------------------------------------------------
void STAZIONI_BYNAME::KeySet(){
   #undef TRCRTN
   #define TRCRTN "STAZIONI_BYNAME::KeySet"
   ID Id = *(ID*)RecordC;
   STAZIONI::R_STRU & Staz = MieStazioni[Id];
   assert(Staz.IdStazione == Id);
   // Copia il nome della stazione compreso lo zero finale
   KeyCorrente.Clear();
   KeyCorrente.Store(Staz.NomeStazione,KeyLength);
   KeyCorrente.Store(&NumRec,sizeof(ULONG)); // Aggiungo indice
   this->ExtraSet();  // Aggiungo l' Id della stazione
   KeyCorrente.Pointer=0;
};

//----------------------------------------------------------------------------
// STAZIONI_MAIU_PER_NOME::KeySet
//----------------------------------------------------------------------------
void STAZIONI_MAIU_PER_NOME::KeySet(){
   #undef TRCRTN
   #define TRCRTN "STAZIONI_MAIU_PER_NOME::KeySet"
   ID Id = *(ID*)RecordC;
   STAZIONI::R_STRU & Staz = MieStazioni[Id];
   assert(Staz.IdStazione == Id);
   // Copia il nome della stazione compreso lo zero finale
   KeyCorrente.Clear();
   char * pNome = (char *)(&KeyCorrente.Last() + 1);  // EMS003 VA cast (char *)
   KeyCorrente.Store(Staz.NomeStazione,KeyLength);
   strupr(pNome);
   KeyCorrente.Store(&NumRec,sizeof(ULONG)); // Aggiungo indice
   this->ExtraSet();  // Aggiungo l' Id della stazione
   KeyCorrente.Pointer=0;
};
//----------------------------------------------------------------------------
// STAZIONI_BYNAME::ExtraSet
//----------------------------------------------------------------------------
void STAZIONI_BYNAME::ExtraSet(){
   #undef TRCRTN
   #define TRCRTN "STAZIONI_BYNAME::ExtraSet"
   STAZIONI::R_STRU & Staz = MieStazioni.RecordCorrente();
   KeyCorrente.Store(&Staz.IdStazione,sizeof(ID));
   BYTE Tipo                     = 0;
   if(Staz.Fs())           Tipo += 1;
   if(Staz.IsCumulativa()) Tipo += 2;
   if(Staz.Estera())       Tipo += 4;
   KeyCorrente.Store(&Tipo,sizeof(Tipo));
};
//----------------------------------------------------------------------------
// @CCR_ID
//----------------------------------------------------------------------------
CCR_ID::CCR_ID (const char * Path,const char * NomeFile): STAZIONI(Path,NomeFile) {
   #undef TRCRTN
   #define TRCRTN "@CCR_ID"
   if(CcrId) { BEEP;return; }; // Chiamato piu' volte il costruttore
   CcrId = this;
};

//----------------------------------------------------------------------------
// ~CCR_ID
//----------------------------------------------------------------------------
CCR_ID::~CCR_ID (){
   #undef TRCRTN
   #define TRCRTN "~CCR_ID"
   CcrId = NULL;
};
//----------------------------------------------------------------------------
// CCR_ID::operator[]
//----------------------------------------------------------------------------
// Rende un ID interno di stazione dato il codice CCR
// Torna 0 se al CCR NON corrisponde un ID
ID CCR_ID::operator[] (int CodiceCcr){
   #undef TRCRTN
   #define TRCRTN "CCR_ID::operator[]"
   return DecodificaCCR(CodiceCcr);
};

//----------------------------------------------------------------------------
// CCR_ID::Id
//----------------------------------------------------------------------------
// Eguale all' operatore: Ok anche per stazioni estere
int CCR_ID::Id(int CodiceCcr){
   #undef TRCRTN
   #define TRCRTN "CCR_ID::Id"
   return DecodificaCCR(CodiceCcr);
};

//----------------------------------------------------------------------------
// CCR_ID::CercaCCR
//----------------------------------------------------------------------------
// rende un ID interno di stazione dato il codice CCR
// Se la stazione non e' definita sul grafo (Estera)
// ritorna -1
int CCR_ID::CercaCCR(int CodiceCcr){
   #undef TRCRTN
   #define TRCRTN "CCR_ID::CercaCCR"
   GetMutex();
   ID Id= PosizionaPerCCR(CodiceCcr);
   if(Id != 0 && !RecordCorrente().Nazionale())Id=0;
   FreeMutex();
   return Id;
};
//----------------------------------------------------------------------------
// ARRAY_ID::Trace()
//----------------------------------------------------------------------------
void ARRAY_ID::Trace(STAZIONI & Staz,const STRINGA& Messaggio, int Livello){
   #undef TRCRTN
   #define TRCRTN "  "
   if(Livello > trchse)return;
   ERRSTRING(Messaggio);
   ORD_FORALL(THIS,i){
      STRINGA Msg;
      ID Id = THIS[i];
      Msg = "Id["+ STRINGA(i)+ "] ";
      Msg += Staz.Id2Ident(Id);
      Msg += " =";
      Msg += STRINGA(Id);
      Msg += STRINGA("\t");
      Msg += Staz.DecodificaIdStazione(Id);
      Msg += " ";
      ERRSTRING(Msg);
   };
   ERRSTRING("");
};
//----------------------------------------------------------------------------
// ARRAY_ID::ToStringa
//----------------------------------------------------------------------------
STRINGA ARRAY_ID::ToStringa(STAZIONI & Staz,WORD MinIdx,WORD MaxIdx){
   #undef TRCRTN
   #define TRCRTN "ARRAY_ID::ToStringa"
   if(Dim() <= MinIdx || MaxIdx < MinIdx)return STRINGA("[]");

   char Tmp[1024];
   char * Out = Tmp;
   ORD_FORALL(THIS,i){
      if(i >= MinIdx && i <= MaxIdx){
         strcpy(Out,", ");
         strncpy(Out+2,(CPSZ)Staz.DecodificaIdStazione(THIS[i]),11);
         Out[13]=0;
         Out = Out +strlen(Out);
      };
   };
   Tmp[0]='[';
   *Out = ']';
   *(Out+1) = '\0';
   return STRINGA(Tmp);
};
