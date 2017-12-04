#ifndef ASFSPRN9
#define ASFSPRN9

#define POSTI_1A        9
#define POSTI_2A       10
#define CUCCETTE_1A    13
#define CUCCETTE_2A    14
#define LETTI_1A       15
#define LETTI_2A       16
#define RISTORANTE     25
#define PRENOT_FAC      3
#define PRENOT_OBB      4
#define PRENOT_S1A      6
#define FUMATORI       26

/*
1. per i dati "posti_1a posti_2a cuccette_1a cuccette_2a letti_1a letti_2a"
     0 = tipo di posto non disponibile
     1 = tipo di posto disponibile ma non prenotabile
     2 = tipo di posto disponibile e prenotabile
     3 = tipo di posto disponibile a prenotazione obbligatoria
2. per il dato "ristorante"
     0 = tipo di posto non disponibile
     1 = tipo di posto disponibile e prenotabile
3. per il dato "prenotabilita"
     0 = nessuna prenotabilita
     1 = prenotazione facoltativa
     2 = prenotazione obbigatoria
     3 = prenotazione facoltativa solo in 1a
     4 = prenotazione obbligatoria solo in 1a
4. per il dato "fumatori"
     0 = aree non presenti
     1 = aree presenti
*/

typedef struct _posti
  {
    int posti_1a;
    int posti_2a;
    int cuccette_1a;
    int cuccette_2a;
    int letti_1a;
    int letti_2a;
    int ristorante;
    int prenotabilita;
    int fumatori;
  } POSTI;

POSTI TipiPostiDisponibili(int, int *);

#endif
