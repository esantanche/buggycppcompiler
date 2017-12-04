//----------------------------------------------------------------------------
// FILE ALL_ALL.C
//----------------------------------------------------------------------------

// 같같같같같같같같같같같� ISTANZIAZIONE DELLE VARIABILI STATIC 같같같같같같같같같같�
// Per evitare abend nello statup dell' applicazione e' meglio
// riunire qui tutte le variabili ALL di OOLIB

// EMS001
typedef unsigned long BOOL;
#include "oggetto.h"
ELENCO_Oggetti OGGETTO::All;
