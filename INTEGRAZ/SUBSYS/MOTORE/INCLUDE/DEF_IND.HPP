//----------------------------------------------------------------------------
// MM_CONTROLLO_RICERCA
//----------------------------------------------------------------------------
// Questa classe contiene i dati per stabilire la profondita Minima Media o Massima 
// della ricerca effettuata dal motore, i filtri e quanto altro
// Ha una istanza statica che rappresenta le impostazioni attuali
// Ha in se i metodi per impostare:
// - Profondita' ricerca
// - Filtri
// - Numero massimo di cambi ammessi
// In linea di massima e' valorizzata dall' interfaccia (I_MOTORE) ed interrogata
// dalle routines del motore
//----------------------------------------------------------------------------

#define MM_CR  MM_CONTROLLO_RICERCA::CR_Corrente   // Acronimo

class MM_CONTROLLO_RICERCA {
  public:

  // Metodi di classe: impostano i valori di controllo correnti
   static  void SetProfondita(TIPO_RICERCA Tipo);

   static MM_CONTROLLO_RICERCA CR_Corrente;

   // Dati di istanza

   TIPO_RICERCA TipoRicerca;   // Tipo ricerca attualmente in uso
   int MINC ;   // MINIMO GARANTITO NON CONDIZIONATO DI CICLI
   int MINC2;   // MINIMO GARANTITO DI CICLI se non ho soluzioni con meno di 4 mezzi
   int MAX1 ;   // MASSIMO NUMERO DI CICLI ESEGUIBILI

   int MaxAttesa;                // Tempo di attesa massimo alla stazione (Escluso Notte)

   // Questi sono basati su indicatori
   int LIM_ATT                ;  // Limite attesa assoluta
   int PercentoKm             ;  // Limite percentuale di distanza (Su PERCORSO_ALFA, che e' impreciso) , oppure 100 Km
   int LIM_DIST               ;  // Limite percentuale di distanza (Su PERCORSO_GRAFO, che e' preciso) oppure 20 Km
   int GOOD_DIST              ;  // Distanza accettabile per soluzione
   int PER_ATT                ;  // Limite attesa relativa
   int MinSubito              ;  // Se la data ed ora di partenza sono quelle attuali :
                                 // Numero minimo di soluzioni BUONE che debbo trovare entro
                                 // 180 minuti
   
   int TOT_ZONE               ;  // Numero zone utilizzate
   int OraInizioZone[6]       ;  // Ora inizio delle zone che dividono la giornate
   int MinSolZone[6]          ;  // Minimo numero di soluzioni per considerare la zona soddisfatta
   int LimitePerSoddisfazioneParziale[6];
                                 // Numero di cicli a cui considero raggionevole arrestarmi in
                                 // funzione del numero di zone soddisfatte (comunque tutte presenti)
   int LimitePerZonePresentiMenoUna; // Idem quando manca una zona all' appello (cioe' non e' presen

   // I parametri seguenti sono utilizzati dall' algoritmo di individuazione
   // delle soluzioni TOPOLOGIA_ALFA::Combina()
   BOOL CERCA_TUTTO            ; // Se vero comprende anche nodi che allungano significativamente 
                                 // il percorso

   int  KmNonSignificativi     ; // Indica quanti Km extra sono tollerati per ogni cambio
                                 // prima che il percorso si consideri significativamente allungato
   int  PercNonSignif          ; // Idem ma in percentuale su percorso minimo

   int  MaxNodiCambioTraCluster; // Limita il numero dei nodi di cambio tra cluster in combina

   int  MinimoAttesa           ; // Questo dato indica il tempo sotto cui l' attesa e' comunque ragionevole

   // Questo parametro indica il tempo di coincidenza per le 4 classi di stazioni
   BYTE TempoCoincidenza[4];

   int MinEqvCambio;  // Minuti di penalita' (minimi) equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   int MinEqvKm    ;  // Km che equivalgono ad un minuto di ritardo (indicatore di convenienza per il passeggero)
   int DeltaTimeSignificativo; // Delta time minimo per cui una soluzione si considera piu' veloce di un' altra
                               // Al di sotto di questo valore i tempi di percorrenza sono trattati come eguali

};
