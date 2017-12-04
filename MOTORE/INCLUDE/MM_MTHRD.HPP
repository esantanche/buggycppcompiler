//========================================================================
// MM_MTHRD.HPP
//========================================================================
// Definizione della classe FIFO
// Gestisce dei buffer FIFO in cui l' inserimento e l' estrazione
// di un elemento dal buffer puo' avvenire su threads differenti
// L' allocazione/deallocazione degli elementi deve essere gestita
// all' esterno della classe.
// NB: L' inserimento e l' estrazione dei dati puo' avvenire da piu' thread
// contemporaneamente.
//========================================================================

#ifndef HO_MM_MTHRD_HPP
#define HO_MM_MTHRD_HPP


class MT_FIFO : public OGGETTO {

   public:

   MT_FIFO( USHORT FifoDim, const STRINGA & Nome , OGGETTO * Compo = NULL );
   ~MT_FIFO();                    
                                  
   BOOL   Put(void * Elemento);   // Mette un elemento nella lista; Se la lista e' piena aspetta
                                  // NB : se nel frattempo interviene un CLEAR l' elemento non
                                  // viene realmente inserito e torna FALSE;
                                  // Analogamente in caso di errore.
   void * Get();                  // Ottiene un elemento nella lista; Se la lista e' vuota aspetta
                                  // NB : se nel frattempo interviene un CLEAR ritorna NULL
                                  // Analogamente in caso di errore.

   void Clear(BOOL Destroy=FALSE);// Pulisce la lista; Termina eventuali Get e Put in sospeso
                                  // Rimane inibito fino al prossimo activate

   void Activate(){Inibito = FALSE;};

   USHORT QueueDim(){return FifoDim;};// Ritorna la dimensione della coda
   USHORT QueueUsed();                // Ritorna il numero di slots occupati
   BOOL   CodaPiena(){return Coda[IndiceScrivi]!= NULL;}; 
   BOOL   CodaVuota(){return Coda[IndiceLeggi] == NULL;};

   ULONG Bad;                     // Se != 0 la coda non puo' essere utilizzata
                                  // In tal caso contiene un rc di errore o 99999999 (Errore generico)

   protected:

   BOOL   Inibito;                // Settato da Clear; Resettato da Activate;

   USHORT FifoDim;                // Dimensione della coda FIFO
   USHORT IndiceLeggi;            // Indice del prossimo elemento da leggere
   USHORT IndiceScrivi;           // Indice del prossimo elemento da leggere

   ULONG  FermaSuGet;             // Numero richieste di Get in sospeso per coda vuota
   ULONG  FermaSuPut;             // Numero richieste di Put in sospeso per coda piena
   
   HMTX   Mutex;                  // Mutex per accesso al FIFO. E' sempre rilasciato ASAP
   HEV    Piena;                  // Semaforo di attesa per coda piena
   HEV    Vuota;                  // Semaforo di attesa per coda vuota

   void ** Coda;                  // Elementi della coda

   BOOL GetMutex();               // Ottiene il controllo della coda. return FALSE su errore
   BOOL FreeMutex();              // Rilascia il controllo della coda.return FALSE su errore

};

#endif
