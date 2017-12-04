//----------------------------------------------------------------------------
// SEQ_PROC.HPP
//----------------------------------------------------------------------------
// Gestione globale stato dei processi e sequenzializzazione
// La gestione e' a livello di macchina e non di rete
// Non gestisce processi che partono in copie multiple
//----------------------------------------------------------------------------

#ifndef HO_SEQ_PROC_HPP
#define HO_SEQ_PROC_HPP

enum STATOPROCESSO {
   SP_NON_PARTITO,       // Il processo non e' ancora stato attivato
   SP_PARTITO,           // Il processo e' stato attivato ma non e' ancora operativo
   SP_ATTIVO             // Il processo e' operativo
};


// La routine seguente deve essere chiamata due volte da ogni processo:
// La prima all' inizio del MAIN con Stato = SP_PARTITO
// La seconda dopo le operazioni di inizializzazione con Stato = SP_ATTIVO
// Alla fine del processo lo stato torna automaticamente a SP_NON_PARTITO
void _export SetStatoProcesso(const STRINGA & NomeProcesso, STATOPROCESSO Stato);

// Il metodo seguente aspetta finche' il processo non arrivi allo stato SP_ATTIVO, oppure
// fino a che il processo non abendi e torni nello stato SP_NON_PARTITO
// E' possibile specificare un timeout in millisecondi (-1= infinito)
STATOPROCESSO _export WaitTransizioneStato(const STRINGA & NomeProcesso,ULONG TimeOut=-1);


// Il metodo seguente ritorna lo stato del processo dato il nome
STATOPROCESSO _export QueryStatoProcesso(const STRINGA & NomeProcesso);

// Il metodo seguente ritorna il PID del processo dato il nome (o -1 se non lo trova o e' terminato)
int _export QueryPidProcesso(const STRINGA & NomeProcesso);

// Il metodo torna il nome del processo corrente
// Se full e' vero torna il nome esattamente come e' stato fornito
// se falso torna la sola componente di nome (serve per avere dei nomi specifici legati al processo)
STRINGA _export NomeProcessoCorrente(BOOL Full=FALSE);


#endif
