#define DIMSTR 36
#define DIMACR 9

typedef struct 
{
  int NumTratta;
  char NomeTreno[DIMSTR];
  char IDTreno[6];
  char TipoTreno[DIMSTR];
  int MV;
  char Parte[6];
  char Arriva[6];
  char Da[DIMSTR];
  char A[DIMSTR];
  char FullDa[DIMSTR];
  char FullA[DIMSTR];
  int NumServiziTreno;
  char *Servizi;
  char Prenotazione[DIMSTR];
} INFOTRATTA;

typedef struct 
{
  int NumSoluzione;
  int NumTratte;
  int TUP;
  int TUA;
  INFOTRATTA *InfoTratte;
  char Periodicita[8];
  char Parte[6];
  char Arriva[6];
  char TTot[12];
  char TAtt[12];
  char Via[256]; 
  int Km;
  char Da[DIMSTR];
  char A[DIMSTR]; 
  int NumServizi;
  char *Servizi;
  char NotePartenza[256];
  char NoteArrivo[256];
} INFOSOLUZIONE;

typedef struct 
{
  char Via[256];
  int Km;
  char Da[DIMSTR];
  char A[DIMSTR];
} INSTRADAMENTO;

typedef struct 
{
   char Da[DIMSTR];
   char A[DIMSTR];
   int giorno;
   int mese;
   int anno;
   int hh_in;
   int mm_in;
   int hh_out;
   int mm_out;
   int NumInstradamenti;
   INSTRADAMENTO *Instradamenti;
   int NumSoluzioni;
   int NumSoluzioniInRange;
   INFOSOLUZIONE *InfoSoluzioni;
   int EsistonoCambi;
} PROBLEMA;


typedef struct 
{
  int NumCambi;
  char *Acronimi;
  char *NomiEstesi;
} STAZIONIDICAMBIO;


//Variabili per la lettura degli input da linea di comando
typedef struct {
   char staz_in[255];
   char staz_out[255];
   char giorno_in[3];
   char mese_in[3];
   char anno_in[5];
   char hh_in[3];
   char mm_in[3];
   char hh_out[3];
   char mm_out[3];
} STRUCT_INPUT;



