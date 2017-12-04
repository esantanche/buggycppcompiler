#ifndef ASFSDATA
#define ASFSDATA

typedef struct _coincidenza
  {
     char            *stazione;
     char            *arrivo;
     char            *attesa;
  } COINCIDENZA;                                           /* End structure*/

typedef struct _soluzione
  {
     char            *orario_p;
     char            *stazione_p;
     char            *orario_a;
     char            *stazione_a;
     char            *tempo;
     int              num_coincidenze;
     COINCIDENZA     *coincidenze;
     int              num_servizi;
     int             *servizi;
     int              num_tipitreni;
     int             *tipitreni;
     char            *prezzo1;
     char            *prezzo2;
  } SOLUZIONE;                                             /* End structure*/

typedef struct _acronimo
  {
     char            *corto;
     char            *lungo;
  } ACRONIMO;                                              /* End structure*/

typedef struct _treno
  {
     int              tipo;
     char            *numero;
     char            *nome;
     char            *stazione_p;
     char            *orario_p;
     char            *stazione_a;
     char            *orario_a;
     int              num_servizi;
     int             *servizi;
     char            *note;
  } TRENO;                                                 /* End structure*/

typedef struct _soluzioni
  {
     char            *stazione_p;
     char            *stazione_a;
     int              giorno;
     int              mese;
     int              anno;
     int              ora;
     int              minuto;
     int              num_soluzioni;
     int              num_soluzioni_possibili;
     int              flag_coincidenze;
     int              sort;
     SOLUZIONE       *soluzioni;
     int              num_acronimi;
     ACRONIMO        *acronimi;
  } SOLUZIONI;                                             /* End structure*/

typedef struct _dettagli
  {
     char            *stazione_p;
     char            *stazione_a;
     int              giorno;
     int              mese;
     int              anno;
     int              ora;
     int              minuto;
     char            *orario;
     int              ind_soluzione;
     int              ind_soluzione_orig;
     int              num_soluzioni;
     char            *prezzo1;
     char            *prezzo2;
     int              km;
     char            *instradamento;
     int              sort;
     int              num_treni;
     TRENO           *treni;
  } DETTAGLI;                                              /* End structure*/

typedef struct _errori
  {
     int              motore;
     int              richiesta;
     int              stazione_p;
     int              stazione_a;
     int              data;
     int              ora;
     int              soluzioni;
     int              num_stazioni_p;
     char           **stazioni_p;
     int              num_stazioni_a;
     char           **stazioni_a;
  } ERRORI;                                                /* End structure*/

typedef struct _indata
  {
     char            *stazione_p;
     char            *stazione_a;
     char            *giorno;
     char            *mese;
     char            *anno;
     char            *ora;
     char            *minuto;
     char            *dettaglio;
     char            *lingua;
     char            *indpage;
     char            *sort;
  } INDATA;

typedef struct _risposta
  {
     int              retcode;
     INDATA           indata;
     union {
        SOLUZIONI     soluzioni;
        DETTAGLI      dettagli;
        ERRORI        errori;
     } Dati;
  } RISPOSTA;                                              /* End structure*/


void      Parse_Risposta(char *, RISPOSTA *);
SOLUZIONI Parse_Soluzioni(char *);
DETTAGLI  Parse_Dettagli(char *);
ERRORI    Parse_Errori(char *);
void      Free_Risposta(RISPOSTA);
void      Free_Soluzioni(SOLUZIONI);
void      Free_Dettagli(DETTAGLI);
void      Free_Errori(ERRORI);

#endif
