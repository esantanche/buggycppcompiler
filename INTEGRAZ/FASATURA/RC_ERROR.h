/*
****************************************************************
**  FILE RC_ERROR.H
**
**    Return Code per contabilit… locale
**
****************************************************************
**  Vers. 1.0.5.0    24.03.1995
****************************************************************
*/

#ifndef RC_ERROR_H
#define RC_ERROR_H

enum  ERRORI_CONTAB {
   OK=0   // Operazione riuscita
  ,CI_BASE = 0x0000    // Information
  ,CW_BASE = 0x0010    // Warning
  ,CW_NOTFOUND                // Ricorrenze non trovate, corrisponde a SQLCODE = 100
  ,CW_TAG_NONDISP      // Non tutti i tagliandi richiesti sono disponibili
  ,CW_PTOVEND_UNDEF    // IWS non configurata come PuntoVendita
  ,CW_TURNO_UNDEF      // Nessun turno aperto per il Punto Vendita
  ,CW_DATA_INIZ               // Data iniziale non valida
  ,CW_ORA_INIZ_BIG            // Ora iniziale troppo grande
  ,CW_DATA_FIN                // Data finale non valida
  ,CW_ENTRY_ANNCAS     // Il valore deve essere compreso tra (LimiteInferiore) e LimiteSuperiore
  ,CW_DATA_INV         // Data non valida
  ,CW_DATA_ATTUALE     // Data di emissione non pu• essere maggiore della data attuale
  ,CW_ENTRY_STOCK      // I dati immessi non sono validi
  ,CW_ENTRY_AVENDERE   // La numerazione del tagliando "A vendere" non pu• essere essere minore di (num)
  ,CW_ENTRY_ANUMERO    // I dati modificati non sono validi
  ,CW_ANNTIT_DUP       // Richiesta di annullamento su titoli gi… annullati
  ,CW_UTINVALID        // Utente non abilitato per un servizio
  ,CW_PWINVALID        // Password non valida
  ,CE_BASE = 0x0030    // Error
  ,CE_SUBALLOC_QUERY_MEM      // Errore nella prima Allocazione FIsica Memoria per Query da DB
  ,CE_SUBREALLOC_QUERY_MEM    // Errore in successive Allocazioni di Memoria per Query da DB
  ,CE_MAX_QUERY_MEM           // Tabella di dimensioni eccessive: aumentare il limite massimo di memoria allocabile
  ,CE_ERRORE_DB               // Generico Errore sull'accesso al DB
  ,CE_RICH_NUM_TIT            // Il numero dei titoli richiesti eccede il massimo consentito
                              // oppure Š nullo
  ,CE_RICH_NUM_RIM            // Il numero dei rimborsi richiesti eccede il massimo consentito
                              // oppure Š nullo
  ,CE_SQL_LENGTH              // La lunghezza del comando SQL eccede il max consentito il massimo consentito
  ,CE_MEM_INSUFF              // Errore memeoria non sufficiente
  ,CE_RICH_TAG_ERR            // La richiesta tagliandi Š errata (es. tipo tagliando invalido)
  ,CE_LOCK_NOOK               // Impossibile fare il "lock" di righe del database
  ,CE_UNLOCK_NOOK             // Impossibile fare l' "unlock" di righe del database
  ,CE_SEQUENZA_INV            // La sequenza di invocazione del metodo Š invalida
  ,CE_CODIMPCOM_INV           // Codice Impianto Commerciale Errato
  ,CE_CODPTOVEN_INV           // Codice Punto Vendita Invalido
  ,CE_RICH_ECCESS             // Il numero dei tagliandi richiesti eccede quanto precedentemente bloccato
  ,CE_NUM_TIT_INV             // Numero Titolo Invalido
  ,CE_NUM_TAG_INV             // Numero Tagliando Invalido
  ,CE_FORN_INCOMPL            // Il numero dei tagliandi forniti per la stampa Š inferiore a quelli richiesti inizialmente
  ,CE_CHIU_TRANSIT_KO         // Errore nella fase di chiusura
  ,CE_IMPCOMM_UNDEF           // Impianto Commerciale non Š stato configurato
  ,CE_PROFILO_NOTFOUND        // Non trovato il file di Profilo
  ,CE_CONNECT_DBIMPIA         // Errore connessione Database Impianto
  ,CE_CONNECT_SRV_SIPAX       // Errore connessione Database Dati
  ,CE_OPEN_IMPLAV             // Errore Open Impianto Commerciale Lavoro
  ,CE_ELPD_NOTAVAIL           // Elenco Partite Diverse non disponibile - Inserimento inibito
  ,CE_DELETE_REG              // Errore durante la cancellazione della regolarizzazione
  ,CE_INSERT_REG              // Errore durante l'inserimento della regolarizzazione
  ,CE_QUADRATURA_DB           // Quadratura turno non effettuata per errori nell'accesso alla base dati
  ,CE_QUADRATURA              // Quadratura turno non effettuata per disallineamento dati
  ,CE_INSERT_PD               // Impossibile inserire la Partita Diversa
  ,CE_TURNO_DB                // Errore durante la lettura dei dati di turno
  ,CE_QUADRATURA_TOTDB        // Errore durante il ricalcolo - I totali non sono stati aggiornati
  ,CE_UPDATE_VEACC            // Errore nell'aggiornamento Versamento Acconto di Turno
  ,CE_CONFERMA_VEACC          // Errore nella conferma Versamento Acconto di Turno
  ,CE_DELETE_VEACC            // Errore nella cancellazione Versamento Acconto di Turno
  ,CE_LETTURA_VEACC           // Errore nella lettura Versamento Acconto di Turno
  ,CE_UPDATE_RICPV            // Errore nell'aggiornamento Versamento Personale Viaggiante
  ,CE_DELETE_RICPV            // Errore nella cancellazione Versamento Personale Viaggiante
  ,CE_LETTURA_RICPV           // Errore nella lettura tabella Versamento Personale Viaggiante
  ,CE_ANNULLA_TAG             // Una o pi— serie di tagliandi non sono stati annullati
  ,CE_INSERT_DISALL_TAG       // Errore durante la scrittura del Disallineamento
  ,CE_MODFON                  // Errore durante l'aggiornamento del FondoCassa
  ,CE_LETTURA_RNC             // Errore lettura tabella dei Resti non Corrisposti
  ,CE_OPEN_TURNOSPL           // Errore nell'apertura dello Sportello
  ,CE_APRI_TURNOFORZ          // Errore durante Apertura Sportello Forzata
  ,CE_APRITURNO_DB            // Errore durante Apertura Turno Sportello su accesso al Database
  ,CE_INIT_STOCK              // Errore di inizializzazione Stock del Punto Vendita
  ,CE_INIT_TURNO              // Errore di inizializzazione Turno di Sportello
  ,CE_LETTURA_TAGANN          // Errore durante la lettura dei Tagliandi Annullati
  ,CE_SOSPENDI_TURNO          // Errore nella Sospensione del Turno
  ,CE_CHIUDI_TURNO            // Errore nella Chiusura del Turno
  ,CE_INSERT_STOCK            // Errore durante l'Inserimento della Serie dei Tagliandi
  ,CE_UPDATE_STOCK           // Errore durante l'Inserimento del Tagliando dello Stock
  ,CE_ANNULLA_STOCK          // Errore durante l'Annullamento della Serie dei Tagliandi
  ,CE_DELETE_STOCK           // Errore durante la Cancellazione della Serie dei Tagliandi
  ,CE_LETTURA_DEPPV          // Elenco Depositi PV non disponibile
  ,CE_INSERT_RNC             // Errore inserimento dei Resti non Corrisposti
  ,CE_UPDATE_RNC             // Errore aggiornamento del Resto non Corrisposto
  ,CE_DELETE_RNC             // Errore cancellazione del Resto non Corrisposto
  ,CE_RIATTIVA_TURNO         // Errore nella Riattivazione del Turno
  ,CE_INSERT_RICPV           // Errore nell'inserimento Versamento Personale Viaggiante
  ,CE_LETTURA_AGENTI         // Elenco Agenti non disponibile
  ,CE_INSERT_VEACC           // Errore nell'inserimento Versamento Acconto di Turno
  ,CE_ANNTIT_INVAL           // Il titolo o i titoli da annullare sono invalidi
  ,CE_MODTUR_NOAUTH          // L'autorizzazione per la modifica casellario non Š valida
  ,CE_PREPARA_STAMPA         // Errore durante la creazione del file di stampa
  ,CE_RISERVA_STAMPANTE      // Impossibile accedere alla stampante
  ,CE_STAMPA_NOOK            // La stampa non Š stata completata correttamente
  ,CE_PUNTV_DB               // Errore durante la lettura o l'aggiornamento dei dati di punto vendita
  ,CE_NOOPER_CASSA           // Operazione impossibile con la situazione di cassa corrente
  ,CE_TURNO_WRITE            // Errore durante la scrittura dei dati di turno
  ,CE_LETTURA_STOCK          // Impossibile leggere il casellario
  ,CE_ANNULLA_TIT            // Errore durante l'annullamento di un titolo
  ,CE_OPEN_PUNTV             // Errore apertura Punto Vendita
  ,CE_CHIUSURA_COLL          // Errore nel collegamento durante la chiusura turno
  ,CE_CODMAC_CONGR           // Errore di congruenza sul codice macchina
  ,CE_CODOPE_CONGR           // Errore di congruenza sul codice operatore
  ,CE_VERS_NONC              // Versamenti in acconto di turno non confermati
  ,CE_UPDATE_PD              // Errore nella modifica Partita Diversa
  ,CE_DELETE_PD              // Errore nella cancellazione Partita Diversa
  ,CE_CHIUDI_PD              // Errore nella chiusura Partita Diversa
  ,CE_RIAPRI_PD              // Errore nella riapertura Partita Diversa
  ,CE_LETTURA_TIPTAG         // Errore nella lettura Tabella Tipi Tagliandi
  ,CE_COLLEG_KO              // Errore nel collegamento via SCP
  ,CE_IDETURNO_INV           // Identificativo Turno non valido
  ,CE_TPN_FAIL               // Errore nella memorizzazione transazione TPN
  ,CE_SPECIE_NOTFOUND        // Nessuna Specie Trovata
  ,CE_F_VC_INVALIDA          // L'interfaccia fornita da Vendita a Contabilit…
                             // Š invalida
  ,CE_F_VC_KO                // Impossibile utilizzare l'interfaccia di Vendita
                             // per errori interni al sistema di Contabilit…
  ,CE_RICH_NUM_RIMB          // Numero di rimborsi richiesto invalido
  ,CE_OP_NONCONS             // Operazione non consentita durante l'interfacciamento
                             // con vendite
  ,CE_ERR_PRETRASM           // Errore durante la creazione della trasmissione
  ,CE_AREA_INV               // Errore di area di trasmissione invalida
  ,CE_TIST_INV               // Errore di Timestamp non corretto
  ,CE_VERS_INV               // Errore di versione non corretta
  ,CE_PROFILE_NOTFOUND       // Profilo non trovato
  ,CE_PD_CONTABILE           // Errore chiusura Partita Diversa
};

#endif
