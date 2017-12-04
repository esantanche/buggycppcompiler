/*===================================================================
Nome modulo ......... : DB_HOST.HPP
===================================================================*/

//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
//+-+-+-  CONTROLLO INCLUSIONI MULTIPLE  -+-+-+-+-+-+-+-+-+-+-+-+-+-
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
#ifndef DBHOST_HPP
#define DBHOST_HPP

//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
//+-+-+-  INCLUSIONI  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
#include "ft_paths.hpp"
#include <oggetto.h>
#include <eventi.h>
#include <base.hpp>

#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ml_in.hpp"
#include "ml_wrk.hpp"
#include "TABELLE.HPP"
#include <trc2.h>

#define DATE_PATH  PATH_IN
#define TRC_PATH   PATH_DUMMY

static int iNumeroRigheScritte;

// ------------------------------------------------------------------
// MACRO_CLASSIFICA
// ------------------------------------------------------------------
// Questa e' una macro classifica, necessaria per l' HOST
enum MACRO_CLASSIFICA {
   MCRC_TRENO     = '1',   // Notare che uso un carattere !
   MCRC_TRAGHETTO = '2',
   MCRC_AUTOBUS   = '3'
};

// ------------------------------------------------------------------
// CODICI_CLASSE
// ------------------------------------------------------------------
// Questi sono i codici di classe da utilizzare su Host
enum CODICI_CLASSE {
   CODCL_PRIMA    = '1',   // Notare che uso un carattere !
   CODCL_SECONDA  = '2',   
   CODCL_PRIMASEC = '3'    
};

// ------------------------------------------------------------------
//  MEZZO_VIRTUALE_APPO
// ------------------------------------------------------------------
// Questa struttura e' relativa al file DMEZVIR.CAC che contiene i dati 
// delle famiglie dei mezzi virtuali. e' un temporaneo che serve da 
// trascodifica tra il vecchio formato (soli mezzi virtuali) ed il nuovo
// (famiglia + progressivo).
// ------------------------------------------------------------------
struct MEZZO_VIRTUALE_APPO
{
    // int iMarked;  // Non pió utilizzato: Montagna
    int iNumeroVirtuale;
    int iPrgVirtuale;
    MEZZO_VIRTUALE sMezzoVirtuale;
};

class  FILE_MEZZO_VIRTUALE_APPO : public FILE_FIX
{
   public:

    FILE_MEZZO_VIRTUALE_APPO(const STRINGA& NomeFile,ULONG BufSize=sizeof(MEZZO_VIRTUALE_APPO)):
        FILE_FIX(NomeFile,sizeof(MEZZO_VIRTUALE_APPO),BufSize){};
    MEZZO_VIRTUALE_APPO &  operator [](ULONG Indice){ Posiziona(Indice); return *(MEZZO_VIRTUALE_APPO*) RecordC; };
    MEZZO_VIRTUALE_APPO &  RecordCorrente(){ return *(MEZZO_VIRTUALE_APPO*) RecordC; };
};

// ------------------------------------------------------------------
// ???? Decod.dat  da vedere cosa diviene
// ------------------------------------------------------------------
// Utilizzato da pslistedecodifica
// ------------------------------------------------------------------
struct DECODIFICA
{
    char DescrizioneDecodifica    [50] ;
    char SeparatoreInizio;
    char Valore_File_T            [10] ;
    char SeparatoreFine;
    char SeparatoreInizio1;
    char Valore_Sipax             [10] ;
    char SeparatoreFine1;
    char CR_LF[2];
};

class  FILE_DECODIFICA : public FILE_FIX
{

   public:

    DECODIFICA sDecodifica;

    FILE_DECODIFICA(const STRINGA& NomeFile,ULONG BufSize=64000):
        FILE_FIX(NomeFile,sizeof(DECODIFICA),BufSize){};
    DECODIFICA &  operator [](ULONG Indice){ Posiziona(Indice); return *(DECODIFICA*) RecordC; };
    DECODIFICA &  RecordCorrente(){ return *(DECODIFICA*) RecordC; };
};

struct LISTE_DECODIFICA
{
    STRINGA oDataInizio;
    STRINGA oDataFine;
    STRINGA oCodiceDistribuzione;
    ELENCO_S oElDecTipoMezzoViagg;
    ELENCO_S oElDecClasse;
    ELENCO_S oElDecClassifica;
    ELENCO_S oElDecFermate;
};

/* *FAN* ˛˛ */
// 13/09/1996
// ----------------------------------------------------------------------------
// Aggiunta questa struttura per creare un file di appoggio con chiave il
// mezzo viaggiante e come attributi il mezzo virtuale di appartenenza, la
// periodicitÖ del mezzo virtuale stesso ed i progressivi stazione di inizio
// e fine del mezzo viaggiante all'interno del virtuale.
// Questo serve per la generazione della tabella DSEROFF, che ha bisogno della
// costituzione del mezzi virtuali (cioä, a quale/i mezzo/i virtuale/i appar-
// tiene un dato mezzo viaggiante.
// ------------------------------------------------------------------
struct MEZZI_VIAGGIANTI
{
    DWORD MezzoViaggiante;      // Key (4 bytes)
    char IdentTreno[ 10 ];
    char Famiglia[ 8 ];         // NUMVIR+PRGVIR ("0000101")
    WORD MezzoVirtuale;         // Famiglia (calcolata)
    WORD ProgressivoVirtuale;   // Progressivo (calcolato)
    WORD iMezzoDaMotore;        // Numero virtuale cosç come in M0_TRENV.TMP
    T_PERIODICITA PeriodicitaMezzoVirtuale;
    WORD PrgStazioneInizio;
    WORD PrgStazioneFine;
};

class FILE_MEZZI_VIAGGIANTI : public FILE_BS
{
    public:
        FILE_MEZZI_VIAGGIANTI( const char * NomeFile, ULONG Size = 4096 ) :
            FILE_BS( NomeFile, sizeof( MEZZI_VIAGGIANTI ), 4, Size, 0, FALSE )
        {
            Init();
        };

        BOOL Seek( WORD MezzoViaggiante )
        {
            return Posiziona( BUFR( & MezzoViaggiante, sizeof( MezzoViaggiante ) ) ) && KeyEsatta;
        };

        MEZZI_VIAGGIANTI &  RecordCorrente()
        {
            return * ( MEZZI_VIAGGIANTI * )RecordC;
        };

        MEZZI_VIAGGIANTI &  FixRec( DWORD Indice )
        {
            FILE_FIX :: Posiziona( Indice );
            return * ( MEZZI_VIAGGIANTI * )RecordC;
        };

    private:

    protected:

};
/* *FAN* ˛˛ */

// ------------------------------------------------------------------
// Derivo una classe da ELENCO di OOLIB per aggiungergli il sort.
// ------------------------------------------------------------------
// Verificare se e' ancora utilizzata
// ------------------------------------------------------------------
class MY_ELENCO : public ELENCO
{
    public:
        MY_ELENCO();
        MY_ELENCO( MY_ELENCO & );
        MY_ELENCO & Sort( PFN_QSORT * Compare, size_t ElementSize );
};

//----------------------------------------------------------------------------
// PROTOTIPI
//----------------------------------------------------------------------------

void Scrivi_DUMP();
int Elabora_DMEZVGG(ELENCO & );
int Elabora_DFERMEZVIR(ELENCO & );
int Elabora_DCOSMEZVIR(ELENCO & );
int Elabora_DECCFERMEZ(ELENCO & );
int Elabora_DPERMEZVIR(ELENCO & );
int Elabora_DSEROFF( ELENCO & );
int Elabora_MEZZIVGG();

int ScriviDMEZVIR(FILE_DMEZVIR * poDMEZVIR,
                  DMEZVIR * psDMEZVIR);

int ScriviDFERMEZVIR(FILE_DFERMEZVIR * poDFERMEZVIR,
                     DFERMEZVIR * psDFERMEZVIR);

int Scrivi_DCOSMEZV( F_MEZZO_VIRTUALE * poMezzoVirtuale,
                     FILE_DCOSMEZVIR * poDCOSMEZVIR,
                     FILE_MEZZI_VIAGGIANTI * poMezziViaggianti,
                     struct FERMATE_VIRT * psFermateVirtuale,
                     struct DCOSMEZVIR * psDCOSMEZVIR,
                     LISTE_DECODIFICA * psListeDecodifica,
                     ELENCO & roElencoRighe,
                     int iTrenoVirtuale,
                     int iTrenoFisico,
                     int iProgressivoStazioneInizio,
                     int iProgressivoStazioneFine,
                     int iProgressivoMezzoViaggiante,
                     int iCount,
                     ELENCO & roRigheOutput,
                     ULONG ulCurrRec );

void DecodTipoMezzoViaggiante( LISTE_DECODIFICA * psListeDecodifica,
                               STRINGA & roNome,
                               char & roValore);

void DecodClasse( LISTE_DECODIFICA * psListeDecodifica,
                  STRINGA & roNome,
                  char & roValore);

void DecodClassifica( LISTE_DECODIFICA * psListeDecodifica,
                      int iValore,
                      char * pcCodice);

void DecodTipoFermata( LISTE_DECODIFICA * psListeDecodifica,
                       STRINGA & roNome,
                       char & roValore);

int DecodificaMezzoVirtuale( WORD MezzoDaMotore,
                             ELENCO & ,
                             char * poNumeroMezzoVirtuale,
                             char * poProgressivoMezzoVirtuale );


#endif

//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
//+-+-+-  FINE MODULO  -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
