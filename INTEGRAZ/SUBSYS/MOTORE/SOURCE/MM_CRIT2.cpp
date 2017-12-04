//----------------------------------------------------------------------------
// DEF_IND.CPP : gestione criteri arresto e selezione del motore
//----------------------------------------------------------------------------
// Questo file contiene insiemi di costanti che controllano i
// criteri del motore
// Si veda MM_CRIT2.HPP e MM_CRITE.CPP
//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "base.hpp"
#include "oggetto.h"
#include "motglue.hpp"
#include "MM_CRIT2.HPP"

static MM_CONTROLLO_RICERCA CR_Minima={
   Minima , // TipoRicerca           : Tipo ricerca attualmente in uso
   1      , // LIM_180               : Indica di usare solo stazioni in cui almeno un cambio soddisfi il limite dei 180 minuti
   15     , // DIRETTO               : Numero di ipotesi da generare per i problemi DIRETTO_FACILE(considerando solo le legali)
   30     , // BASE                  : Numero base di ipotesi da generare    (considerando solo le legali)                           
   70     , // MAX_IPO               : Numero massimo di ipotesi da generare (considerando solo le legali ed a parte le estensioni)  
   35     , // EXT_IPO               : Fattore di estensione * 10 (=  Numero di ipotesi aggiuntive che gestisco bloccando gli I/O)
   1000   , // MAXPT                 : Massimo numero di path_cambi generabili (LEGALI ED ILLEGALI) 
   225    , // MaxAttesa             : Tempo di attesa massimo alla stazione (escluso Notte e traghetti)
   30     , // CAMBIO_SICURO         : Si considera NON a rischio un cambio con tempo di attesa di almeno CAMBIO_SICURO Minuti
   30     , // NOQUAL                : Penalita' (assoluta!) per l' uso di un treno a supplemento per meno di meta' del percorso o pochi Km 
   150    , // LIM_DIST              : Limite percentuale di distanza (Su PERCORSO_GRAFO, che e' preciso) oppure 50 Km 
   10,15  , // TempoCoincidenza[2]   : Tempi minimi di coincidenza per le 2 classi di stazioni 
   45     , // MinEqvCambio          : Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   5      , // MinEqvKm              : Km che equivalgono ad un minuto di ritardo (indicatore di convenienza per il passeggero)
   15     , // DeltaTimeSignificativo: Delta costo minimo per cui una soluzione si considera "incompatibile" con un' altra
   15     , // URBANO                : Penalizzazione per l' uso di una tratta urbana
   1      , // EliminaTroppoCostose  : Elimina le soluzioni "troppo costose" rispetto al costo tipico (in particolare: le notturne)
   250    , // LimiteSoloLettiCucc   : Limite Km sotto il cui non permette l' uso di treni con soli letti o cuccette
   50     , // LimitePrenobb         : Limite Km sotto il cui non permette l' uso di treni a prenotazione obbligatoria ...
   50     , // Lim2Prenobb           : Limite correlato (*2 : Sono 100 Km)
   125    , // Lim1PenSuppl          : Limite Km sotto cui penalizza le soluzioni che fanno pagare un supplemento ...
   50     , // Lim2PenSuppl          : Limite Km sotto cui penalizza doppiamente le soluzioni che fanno pagare un supplemento
   60     , // CostoCambioNotturno   : Penalizzazione per un cambio durante le ore notturne. Raddoppiata per i treni con letti e cuccette
   20     , // CAMBIO_SICURO2        : Questa e' additiva su pi— tratte a differenza di CAMBIO_SICURO 
   
   // Questi indicatori servono al calcolo del costo assoluto: sono pertanto minori rispetto ai loro analoghi per il calcolo del costo
   30     , // MinEqvCambio2         : Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   
//<<< static MM_CONTROLLO_RICERCA CR_Minima= 
};

static MM_CONTROLLO_RICERCA CR_Media ={
   Media  , // TipoRicerca           : Tipo ricerca attualmente in uso
   1      , // LIM_180               : Indica di usare solo stazioni in cui almeno un cambio soddisfi il limite dei 180 minuti
   35     , // DIRETTO               : Numero di ipotesi da generare per i problemi DIRETTO_FACILE(considerando solo le legali)
   70     , // BASE                  : Numero base di ipotesi da generare    (considerando solo le legali)                           
   200    , // MAX_IPO               : Numero massimo di ipotesi da generare (considerando solo le legali ed a parte le estensioni)  
   40     , // EXT_IPO               : Fattore di estensione * 10 (=  Numero di ipotesi aggiuntive che gestisco bloccando gli I/O)
   2000   , // MAXPT                 : Massimo numero di path_cambi generabili (LEGALI ED ILLEGALI) 
   270    , // MaxAttesa             : Tempo di attesa massimo alla stazione (escluso Notte e traghetti)
   30     , // CAMBIO_SICURO         : Si considera NON a rischio un cambio con tempo di attesa di almeno CAMBIO_SICURO Minuti
   30     , // NOQUAL                : Penalita' (assoluta!) per l' uso di un treno a supplemento per meno di meta' del percorso o pochi Km 
   150    , // LIM_DIST              : Limite percentuale di distanza (Su PERCORSO_GRAFO, che e' preciso) oppure 50 Km
   10,15  , // TempoCoincidenza[2]   : Tempi minimi di coincidenza per le 2 classi di stazioni 
   45     , // MinEqvCambio          : Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   5      , // MinEqvKm              : Km che equivalgono ad un minuto di ritardo (indicatore di convenienza per il passeggero)
   12     , // DeltaTimeSignificativo: Delta costo minimo per cui una soluzione si considera "incompatibile" con un' altra 
   15     , // URBANO                : Penalizzazione per l' uso di una tratta urbana
   0      , // EliminaTroppoCostose  : Elimina le soluzioni "troppo costose" rispetto al costo tipico (in particolare: le notturne)
   250    , // LimiteSoloLettiCucc   : Limite Km sotto il cui non permette l' uso di treni con soli letti o cuccette
   50     , // LimitePrenobb         : Limite Km sotto il cui non permette l' uso di treni a prenotazione obbligatoria ...
   50     , // Lim2Prenobb           : Limite correlato (*2 : Sono 100 Km)
   125    , // Lim1PenSuppl          : Limite Km sotto cui penalizza le soluzioni che fanno pagare un supplemento ...
   50     , // Lim2PenSuppl          : Limite Km sotto cui penalizza doppiamente le soluzioni che fanno pagare un supplemento
   60     , // CostoCambioNotturno   : Penalizzazione per un cambio durante le ore notturne. Raddoppiata per i treni con letti e cuccette
   10     , // CAMBIO_SICURO2        : Questa e' additiva su pi— tratte a differenza di CAMBIO_SICURO 
   
   // Questi indicatori servono al calcolo del costo assoluto: sono pertanto minori rispetto ai loro analoghi per il calcolo del costo
   15     , // MinEqvCambio2         : Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   
//<<< static MM_CONTROLLO_RICERCA CR_Media = 
};


static MM_CONTROLLO_RICERCA CR_TanteSol ={
   TanteSol,// TipoRicerca           : Tipo ricerca attualmente in uso
   0      , // LIM_180               : Indica di usare solo stazioni in cui almeno un cambio soddisfi il limite dei 180 minuti
   50     , // DIRETTO               : Numero di ipotesi da generare per i problemi DIRETTO_FACILE(considerando solo le legali)
   250    , // BASE                  : Numero base di ipotesi da generare    (considerando solo le legali)                           
   1000   , // MAX_IPO               : Numero massimo di ipotesi da generare (considerando solo le legali ed a parte le estensioni)  
   25     , // EXT_IPO               : Fattore di estensione * 10 (=  Numero di ipotesi aggiuntive che gestisco bloccando gli I/O)
   6000   , // MAXPT                 : Massimo numero di path_cambi generabili (LEGALI ED ILLEGALI) 
   360    , // MaxAttesa             : Tempo di attesa massimo alla stazione (escluso Notte e traghetti)
   30     , // CAMBIO_SICURO         : Si considera NON a rischio un cambio con tempo di attesa di almeno CAMBIO_SICURO Minuti
   30     , // NOQUAL                : Penalita' (assoluta!) per l' uso di un treno a supplemento per meno di meta' del percorso o pochi Km 
   150    , // LIM_DIST              : Limite percentuale di distanza (Su PERCORSO_GRAFO, che e' preciso) oppure 50 Km
   10,15  , // TempoCoincidenza[2]   : Tempi minimi di coincidenza per le 2 classi di stazioni 
   45     , // MinEqvCambio          : Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   5      , // MinEqvKm              : Km che equivalgono ad un minuto di ritardo (indicatore di convenienza per il passeggero)
   10     , // DeltaTimeSignificativo: Delta costo minimo per cui una soluzione si considera "incompatibile" con un' altra 
   15     , // URBANO                : Penalizzazione per l' uso di una tratta urbana
   0      , // EliminaTroppoCostose  : Elimina le soluzioni "troppo costose" rispetto al costo tipico (in particolare: le notturne)
   250    , // LimiteSoloLettiCucc   : Limite Km sotto il cui non permette l' uso di treni con soli letti o cuccette
   50     , // LimitePrenobb         : Limite Km sotto il cui non permette l' uso di treni a prenotazione obbligatoria ...
   50     , // Lim2Prenobb           : Limite correlato (*2 : Sono 100 Km)
   125    , // Lim1PenSuppl          : Limite Km sotto cui penalizza le soluzioni che fanno pagare un supplemento ...
   50     , // Lim2PenSuppl          : Limite Km sotto cui penalizza doppiamente le soluzioni che fanno pagare un supplemento
   60     , // CostoCambioNotturno   : Penalizzazione per un cambio durante le ore notturne. Raddoppiata per i treni con letti e cuccette
   0      , // CAMBIO_SICURO2        : Questa e' additiva su pi— tratte a differenza di CAMBIO_SICURO 
   
   // Questi indicatori servono al calcolo del costo assoluto: sono pertanto minori rispetto ai loro analoghi per il calcolo del costo
   0     , // MinEqvCambio2         : Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   
//<<< static MM_CONTROLLO_RICERCA CR_TanteSol = 
};

static MM_CONTROLLO_RICERCA CR_CasiDifficili ={
   CasiDifficili,// TipoRicerca      : Tipo ricerca attualmente in uso
   0      , // LIM_180               : Indica di usare solo stazioni in cui almeno un cambio soddisfi il limite dei 180 minuti
   50     , // DIRETTO               : Numero di ipotesi da generare per i problemi DIRETTO_FACILE(considerando solo le legali)
   250    , // BASE                  : Numero base di ipotesi da generare    (considerando solo le legali)
   1000   , // MAX_IPO               : Numero massimo di ipotesi da generare (considerando solo le legali ed a parte le estensioni)
   25     , // EXT_IPO               : Fattore di estensione * 10 (=  Numero di ipotesi aggiuntive che gestisco bloccando gli I/O)
   6000   , // MAXPT                 : Massimo numero di path_cambi generabili (LEGALI ED ILLEGALI)
   360    , // MaxAttesa             : Tempo di attesa massimo alla stazione (escluso Notte e traghetti)
   30     , // CAMBIO_SICURO         : Si considera NON a rischio un cambio con tempo di attesa di almeno CAMBIO_SICURO Minuti
   30     , // NOQUAL                : Penalita' (assoluta!) per l' uso di un treno a supplemento per meno di meta' del percorso o pochi Km 
   150    , // LIM_DIST              : Limite percentuale di distanza (Su PERCORSO_GRAFO, che e' preciso) oppure 50 Km
   10,15  , // TempoCoincidenza[2]   : Tempi minimi di coincidenza per le 2 classi di stazioni 
   45     , // MinEqvCambio          : Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   5      , // MinEqvKm              : Km che equivalgono ad un minuto di ritardo (indicatore di convenienza per il passeggero)
   8      , // DeltaTimeSignificativo: Delta costo minimo per cui una soluzione si considera "incompatibile" con un' altra 
   15     , // URBANO                : Penalizzazione per l' uso di una tratta urbana
   0      , // EliminaTroppoCostose  : Elimina le soluzioni "troppo costose" rispetto al costo tipico (in particolare: le notturne)
   250    , // LimiteSoloLettiCucc   : Limite Km sotto il cui non permette l' uso di treni con soli letti o cuccette
   50     , // LimitePrenobb         : Limite Km sotto il cui non permette l' uso di treni a prenotazione obbligatoria ...
   50     , // Lim2Prenobb           : Limite correlato (*2 : Sono 100 Km)
   125    , // Lim1PenSuppl          : Limite Km sotto cui penalizza le soluzioni che fanno pagare un supplemento ...
   50     , // Lim2PenSuppl          : Limite Km sotto cui penalizza doppiamente le soluzioni che fanno pagare un supplemento
   60     , // CostoCambioNotturno   : Penalizzazione per un cambio durante le ore notturne. Raddoppiata per i treni con letti e cuccette
   0      , // CAMBIO_SICURO2        : Questa e' additiva su pi— tratte a differenza di CAMBIO_SICURO 
   
   // Questi indicatori servono al calcolo del costo assoluto: sono pertanto minori rispetto ai loro analoghi per il calcolo del costo
   0     , // MinEqvCambio2         : Minuti di penalita' equivalenti ad un cambio (indicatore di scomodita' per il passeggero)
   
//<<< static MM_CONTROLLO_RICERCA CR_CasiDifficili = 
};

MM_CONTROLLO_RICERCA MM_CONTROLLO_RICERCA::CR_Corrente = CR_Media;

void MM_CONTROLLO_RICERCA::SetProfondita(TIPO_RICERCA Tipo){
   switch (Tipo) {
   case Minima:
      CR_Corrente = CR_Minima;
      break;
   case TanteSol :
      CR_Corrente = CR_TanteSol;
      break;
   case CasiDifficili:
      CR_Corrente = CR_CasiDifficili;
      break;
   default:
   case Media:
      CR_Corrente = CR_Media;
      break;
   } /* endswitch */
};

