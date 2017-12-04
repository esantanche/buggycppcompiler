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


//----------------------------------------------------------------------------
// Parametri standard utilizzati dai criteri di ricerca.
//----------------------------------------------------------------------------
#define MAX_ATTESA_TRAGHETTI 720 // Mas 12 ore di attesa
// Range di variabilita' accettabile per LIM_ATT
#define MAX_ATTESA_MINIMA    180 // Sono sempre disposto ad aspettare fino a 3 ore
#define MAX_ATTESA_MASSIMA   360 // E mai piu' di 6 ore
// Intervallo notturno da non considerare per i tempi di attesa
#define INIZIO_NOTTE          60 // L' una di notte
#define FINE_NOTTE           360 // Le Sei di mattina
// Numero fasce in cui e' divisa la giornata. 
// Attenzione: Se supera 16 NON funzionano piu' le funzioni di caricamento,
// e comunque ogni modifica cambia il size dei COLLEGAMENTI, fare
// attenzione se si modifica il parametro
#define NUM_FASCE             10
//----------------------------------------------------------------------------


class MM_CONTROLLO_RICERCA {
  public:

  // Metodi di classe: impostano i valori di controllo correnti
   static  void SetProfondita(TIPO_RICERCA Tipo);

   static MM_CONTROLLO_RICERCA CR_Corrente;

   // --------------------------------------------
   // Nuova logica
   // --------------------------------------------

   TIPO_RICERCA  TipoRicerca   ; // Tipo ricerca attualmente in uso

   // Gestione delle ipotesi
   BOOL LIM_180                ; // Indica di usare solo stazioni in cui almeno un cambio soddisfi il limite dei 180 minuti
   int  DIRETTO                ; // Numero di ipotesi da generare per i problemi DIRETTO_FACILE(considerando solo le legali)
   int  BASE                   ; // Numero base di ipotesi da generare     (considerando solo le legali)
   int  MAX_IPO                ; // Numero massimo di ipotesi da generare  (considerando solo le legali ed a parte le estensioni) 
   int  EXT_IPO                ; // Fattore di estensione * 10 (=  Numero di ipotesi aggiuntive che gestisco bloccando gli I/O)
   int  MAXPT                  ; // MASSIMO NUMERO DI PATH_CAMBI GENERABILI (LEGALI ED ILLEGALI)
   int  MaxAttesa              ; // Tempo di attesa massimo alla stazione (Escluso Notte)
   int  CAMBIO_SICURO          ; // Si considera NON a rischio un cambio con tempo di attesa di almeno CAMBIO_SICURO Minuti
   int  NOQUAL                 ; // Penalita' (assoluta!) per l' uso di un treno a supplemento per meno di meta' del percorso o pochi Km
   int  LIM_DIST               ; // Limite percentuale di distanza (Su PERCORSO_GRAFO, che e' preciso) oppure 50 Km
                                 // NB: e' calcolata rispetto alla piu' corta SOLUZIONE
   BYTE TempoCoincidenza[2]    ; // Questo parametro indica il tempo di coincidenza per le 2 classi di stazioni
   int  MinEqvCambio           ; // Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   int  MinEqvKm               ; // Km che equivalgono ad un minuto di ritardo (indicatore di convenienza per il passeggero)
   int  DeltaTimeSignificativo ; // Delta costo minimo per cui una soluzione si considera "incompatibile" con un' altra 
                                 // Al di sotto di questo valore i tempi di percorrenza sono trattati come eguali
   int  URBANO                 ; // Penalizzazione per l' uso di una tratta urbana
   BOOL EliminaTroppoCostose   ; // Elimina le soluzioni "troppo costose" rispetto al costo tipico (in particolare: le notturne)
   int  LimiteSoloLettiCucc    ; // Limite Km sotto il cui non permette l' uso di treni con soli letti o cuccette
   int  LimitePrenobb          ; // Limite Km sotto il cui non permette l' uso di treni a prenotazione obbligatoria 
                                 // a meno che il percorso complessivo del treno sia < 2 * Lim2Prenobb
                                 // Il fattore 2 deriva dal fatto che in realt… testo con "KmLimiteTrattaCorta" che Š met… della lunghezza del percorso complessivo del treno
                                 // Si deve avere LimiteSoloLettiCucc > LimitePrenobb
   int  Lim2Prenobb            ; // Vedi sopra
   int  Lim1PenSuppl           ; // Limite Km sotto cui penalizza le soluzioni che fanno pagare un supplemento
                                 // Il limite non si applica se cumunque percorro almeno met… del percorso complessivo del treno
   int  Lim2PenSuppl           ; // Limite Km sotto cui penalizza doppiamente le soluzioni che fanno pagare un supplemento
   int  CostoCambioNotturno    ; // Penalizzazione per un cambio durante le ore notturne (std = 1h). 
                                 // Raddoppiata per i treni con letti e cuccette
   int  CAMBIO_SICURO2         ; // Questa e' additiva su pi— tratte a differenza di CAMBIO_SICURO

   // Questi indicatori servono al calcolo del costo assoluto: sono pertanto minori rispetto ai loro analoghi per il calcolo del costo
   int  MinEqvCambio2          ; // Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)

};
