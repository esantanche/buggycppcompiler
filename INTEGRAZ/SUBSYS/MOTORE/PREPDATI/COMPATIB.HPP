//----------------------------------------------------------------------------
// COMPATIB.HPP Classi di compatibilit… per caricamento dati
// Da eliminare ASAP: il supporto sar… interrotto
//----------------------------------------------------------------------------

// -----------------------------------------------------------------------
// Strutture intermedie con i servizi per treno
// -----------------------------------------------------------------------
// La classifica dei treni e' valorizzata in accordo con la tabella
// presente su MM_INFO
// NB: L' ordine dei treni DEVE essere lo stesso dei  record di TABORARI

// A prescindere dalla periodicita' e dalle stazioni
struct SERVIZI_TRENO {
    char      IdentTreno               [10] ; // Identificativo Treno
    MM_INFO   Servizi                       ; // Servizi
    BYTE      Uniforme                      ; // 1 se non ha periodicita' ne' stazioni disuniformi
    DWORD     NumRecOrig                    ; // Puntamento su file TABORARI
};

class  FILE_SERVIZI_TRENO : public FILE_FIX {
   public:
   FILE_SERVIZI_TRENO(const STRINGA& NomeFile,ULONG BufSize=64000):
   FILE_FIX(NomeFile,sizeof(SERVIZI_TRENO),BufSize){};
   SERVIZI_TRENO &  operator [](ULONG Indice){ Posiziona(Indice); return *(SERVIZI_TRENO*) RecordC; };
   SERVIZI_TRENO &  RecordCorrente(){ return *(SERVIZI_TRENO*) RecordC; };
};

// Considerando anche la periodicita'
struct PERIODICITA_SERVIZI_TRENO {
    char          IdentTreno               [10] ; // Identificativo Treno
    WORD          Progressivo                   ; // Progressivo record
    T_PERIODICITA Periodicita                   ; // Periodicita' dei servizi
    MM_INFO       Servizi                       ; // Servizi
};
// NB: Si adotta la convenzione che in un record sono messi tutti i servizi
// che hanno la stessa periodicita' (Limitatamente all' orario corrente)
// Quindi lo stesso servizio non puo' apparire su piu' records, ne la stessa
// periodicita' puo' essere presente su piu' records (per lo stesso treno)


class  FILE_PERIODICITA_SERVIZI_TRENO : public FILE_FIX {
   public:
   FILE_PERIODICITA_SERVIZI_TRENO(const STRINGA& NomeFile,ULONG BufSize=64000):
   FILE_FIX(NomeFile,sizeof(PERIODICITA_SERVIZI_TRENO),BufSize){};
   PERIODICITA_SERVIZI_TRENO &  operator [](ULONG Indice){ Posiziona(Indice); return *(PERIODICITA_SERVIZI_TRENO*) RecordC; };
   PERIODICITA_SERVIZI_TRENO &  RecordCorrente(){ return *(PERIODICITA_SERVIZI_TRENO*) RecordC; };
};

// A prescindere dalla periodicita' e dalle stazioni
class  FILE_SERVIZI : public FILE_BS {
   public:
   struct R_STRU {
      DWORD NumeroMezzoVg   ; // KEY: e' il mezzo viaggiante, non il mezzo virtuale
      MM_INFO   Servizi                       ; // Servizi (NB: Le note sono ancora da aggiornare)
      BYTE      Uniforme                      ; // 1 se non ha periodicita' ne' stazioni disuniformi
   };

   // Costruttore
   FILE_SERVIZI(const char* NomeFile,ULONG Size) : FILE_BS(NomeFile,sizeof(R_STRU),4,Size,0,FALSE){Init();};
   
   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(DWORD Mezzo_Vg){ BUFR Wrk; Wrk.Store(Mezzo_Vg); return Posiziona(Wrk) && KeyEsatta;};
   
   // Definizione operatori
   R_STRU&  RecordCorrente(){ return *(R_STRU*) RecordC; };
   R_STRU&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(R_STRU*) RecordC; };
   
   // Metodi virtuali da ridefinire
   virtual int  Compare(const void * Key1,const void* Key2,ULONG DimConfronto ){ return CompareDoubleWord(Key1,Key2);};
};

struct DETTAGLIO_SERVIZI  {  
   // Va dato solo per i servizi non uniformi. 
   // E' indifferente se vengono caricati con delle strutture MM_INFO che contengono ciascuna un 
   // solo tipo servizio oppure mettendo a fattor comune piu' servizi che abbiano la stessa periodicita'
   // e stazioni di inizio e fine servizio
   DWORD NumeroMezzoVg               ; // KEY: e' il mezzo viaggiante, non il mezzo virtuale
   BYTE     TipoSRV                  ; // 0 = Di fermata 1 = tutto il MV 2 = Da stazione a stazione 
                                       // (e tutte le intermedie) 3 = Da stazione a Stazione (no 
                                       // intermedie) 4 = Su tutte le stazioni attrezzate
                                       // 5 = NON ha il servizio per la data fermata. ?????
   T_PERIODICITA Periodicita         ; // Periodicita' 
   char     Ccr1[5]                  ; // CCR  della prima fermata utile in salita per usufruire del servizio ( o 0 )
   char     Ccr2[5]                  ; // CCR  dell' ultima fermata utile in discesa per usufruire del servizio ( o 0 )
   MM_INFO  Servizi                  ; // Servizi (NB: un servizio puo' comparire in piu' records
                                       // di questo tipo e viceversa).
   BYTE     SrvAttivoAFine           ; // Prende di peso il corrispondente flag su infotrasporti
};


class  FILE_DETTAGLIO_SERVIZI : public FILE_BS {        
   public:
   // Costruttore
   FILE_DETTAGLIO_SERVIZI(const char* NomeFile,ULONG Size) : FILE_BS(NomeFile,sizeof(DETTAGLIO_SERVIZI),4,Size,0,FALSE){Init();};
   
   // Seek: Torna FALSE sia su errore che se non trova il record esatto ( = Prima fermata del mezzo virtuale)
   BOOL Seek(DWORD Mezzo_Vg){ BUFR Wrk; Wrk.Store(Mezzo_Vg); return Posiziona(Wrk) && KeyEsatta;};
   
   // Definizione operatori
   DETTAGLIO_SERVIZI&  RecordCorrente(){ return *(DETTAGLIO_SERVIZI*) RecordC; };
   DETTAGLIO_SERVIZI&  FixRec(DWORD Indice){ FILE_FIX::Posiziona(Indice); return *(DETTAGLIO_SERVIZI*) RecordC; };
   
};

