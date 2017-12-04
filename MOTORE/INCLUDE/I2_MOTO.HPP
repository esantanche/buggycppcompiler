//========================================================================
// Dati privati della interfaccia con il motore
//========================================================================

// ---------------------------------------------------------------------
// Dati globali del motore
// ---------------------------------------------------------------------
// Non sono visibili al di fuori della DLL del motore
   GET_INSTRPRE * Instradamenti;       // Per compatibilita' M300: Dati differenti stazione per stazione
   GET_RELCUMUL * RelazioniCumulative; // Per compatibilita' M300
   GET_INSTRPRE * InstradamentiCum;    // Relazioni interne utilizzate per il cumulativo Tirrenia
   GET_MARECUM  * RelazioniTirrenia;   // TransitiMareTirrenia

   // Questi sono creati nel costruttore.
   // Se e' impostata la variabile d' ambiente NO_MOTORE=YES non sono creati
   // (Serve solo per fare le prove sui precaricati)

   class DATI_ORARIO_FS * DatiOrarioA   ; // Dati Orario orario attualmente in uso
   class DATI_ORARIO_FS * OrarioInCorso ; // Dati Orario orario Corrente
   class DATI_ORARIO_FS * OrarioProssimo; // Dati Orario orario Seguente 


   STRINGA Path;              // Path dei dati non dipendenti da Orario (Grafo)
   STRINGA OrarioCorrente;    // Path dei dati dell' orario corrente
   STRINGA OrarioSeguente;    // Path dei dati dell' orario seguente
   STRINGA PathFast1     ;    // Path dei dati di fast start dell' orario corrente
   STRINGA PathFast2     ;    // Path dei dati di fast start dell' orario seguente

   CACHE_MOTORE CacheMotore;

   // Funzione interna
   void PreparaOut(MM_ELENCO_PERCORSO_E_SOLUZIONI & EPerc );

   // ---------------------------------------------------------------------
   // Queste funzioni lavorano con i precaricati
   // ---------------------------------------------------------------------
   // Si noti che non modificano LastOrigine e LastDestinazione
   // Torna un MM_ELENCO_PERCORSO_E_SOLUZIONI senza soluzioni.
   MM_ELENCO_PERCORSO_E_SOLUZIONI * M300_RichiestaPerRelazione(ID Da,ID A, MM_ELENCO_PERCORSO_E_SOLUZIONI * AggiungiA=NULL);
          // Nota: Se Da == 0 e A == Stazione cumulativo fornisce tutti i
          // percorsi dai transiti alle stazioni del cumulativo

   // E queste no
   MM_ELENCO_PERCORSO_E_SOLUZIONI * SIPAX_RichiestaPerRelazione(ID Da,ID A,MOTIVO Motivo, SDATA  Data,WORD Ora, OPZIONE_UTENTE OpzioneUtente);

