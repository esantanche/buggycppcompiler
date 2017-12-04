//----------------------------------------------------------------------------
// FILE DSEROFF.CPP
//----------------------------------------------------------------------------

#include <db_host.hpp>
#include <math.h>
#include <stdio.h>

extern int OkPrintf;

// LOGICA DEL PROGRAMMA: Si parte dalla costituzione dei mezzi virtuali, dato
//                       che in output dovr• produrre una riga per mezzo vir-
//                       tuale (+ progressivo). Per ciascun mezzo viaggiante
//                       in costituzione, ne cerco l'identificativo interno
//                       nel file F_SRVTRN; una volta trovato il record, la
//                       relativa struttura MM_INFO la compongo con le altre
//                       del mezzo virtuale in una MM_INFO in memoria. Da que-
//                       sto processo escludo i servizi non uniformi. Alla fine
//                       dei mezzi viaggianti del virtuale di turno, la strut-
//                       tura MM_INFO risultante verr… esaminata bit per bit
//                       e scriver• un record per servizio trovato.
//                       Successivamente, un secondo passaggio sar… riservato
//                       ai servizi non uniformi trovati durante il primo
//                       giro.

// ---------------------------------------------------------------------------
// Prototipi di funzione
// ---------------------------------------------------------------------------
int RicercaServiziMezzoViaggiante( ELENCO &,
                                   MEZZI_VIAGGIANTI &,
                                   MM_INFO &,
                                   ELENCO &,
                                   ELENCO &,
                                   ELENCO &,
                                   ELENCO &,
                                   LISTE_DECODIFICA *,
                                   FILE_DSEROFF *,
                                   ULONG *,
                                   FILE * );
int ElencaMezziViaggianti( FILE_MEZZI_VIAGGIANTI *,
                           ULONG,
                           ELENCO &,
                           ELENCO & );
int CaricaServizi2( ELENCO & );
int CaricaDettagliServizio( ELENCO & );
int CaricaFermate2( ELENCO & );
ULONG DecodificaScriviServiziUniformi( MEZZI_VIAGGIANTI *,
                                       MM_INFO &,
                                       FILE_DSEROFF *,
                                       FILE *,
                                       LISTE_DECODIFICA * );
int ValorizzaScriviStruttura( int iUniforme,
                              int iCodiceTipoServizio,
                              int iCodiceServizio,
                              MEZZI_VIAGGIANTI & rsMezzoViaggiante,
                              PERIODICITA_IN_CHIARO * poPeriodicitaInChiaro,
                              int iContPeriodi,
                              void * pvServizio,
                              size_t iSizeServizio,
                              ELENCO & roElencoStazioni,
                              LISTE_DECODIFICA *,
                              FILE *,
                              FILE_DSEROFF * poDSEROFF );
int DecodificaCodiciStazioni( DETTAGLIO_SERVIZI & rsDettaglioServizi,
                              ELENCO & roElencoStazioni,
                              DSEROFF & rsDSEROFF,
                              FILE * fp,
                              MEZZI_VIAGGIANTI & rsMezzoViaggiante );
// int OrdinaMezziViaggianti( const void * m1, const void * m2 );
// int CallbackOrdinaMezzi( const void * poMezzo1, const void * poMezzo2 );

// ---------------------------------------------------------------------------
// MACRO
// ---------------------------------------------------------------------------
#define EsaminaServizio( SERVIZIO, DESCRIZIONE, CODTIPSER, CODSER )                                                     \
{                                                                                                                       \
    iFound = FALSE;                                                                                                     \
    ORD_FORALL( roElencoDettagliServizio, iIndex )                                                                      \
    {                                                                                                                   \
        struct DETTAGLIO_SERVIZI * psDettaglioServizio = ( DETTAGLIO_SERVIZI * )roElencoDettagliServizio[ iIndex ];     \
                                                                                                                        \
        if( psDettaglioServizio -> NumeroMezzoVg == psServizio -> NumeroMezzoVg )                                       \
        {                                                                                                               \
            if( psDettaglioServizio -> Servizi.SERVIZIO )                                                               \
            {                                                                                                           \
                fprintf( fp, DESCRIZIONE );                                                                             \
                iFound = TRUE;                                                                                          \
                                                                                                                        \
                PERIODICITA_IN_CHIARO * poPeriodicitaInChiaro = NULL;                                                   \
                poPeriodicitaInChiaro = psDettaglioServizio -> Periodicita.EsplodiPeriodicita();                        \
                                                                                                                        \
                for( int iCont = 0; iCont < poPeriodicitaInChiaro -> Dim(); iCont ++ )                                  \
                {                                                                                                       \
                    ValorizzaScriviStruttura( FALSE,                                                                    \
                                              CODTIPSER,                                                                \
                                              CODSER,                                                                   \
                                              rsMezzoViaggiante,                                                        \
                                              poPeriodicitaInChiaro,                                                    \
                                              iCont,                                                                    \
                                              ( void *  )psDettaglioServizio,                                           \
                                              sizeof( DETTAGLIO_SERVIZI ),                                              \
                                              roElencoStazioni,                                                         \
                                              psListeDecodifica,                                                        \
                                              fp,                                                                       \
                                              poDSEROFF );                                                              \
                    ( * pulNumScritti ) ++;                                                                             \
                }                                                                                                       \
                                                                                                                        \
                ELENCO_S oElencoPeriodi = poPeriodicitaInChiaro -> PeriodicitaLeggibile();                              \
                ORD_FORALL( oElencoPeriodi, iContPeriodi )                                                              \
                {                                                                                                       \
                    fprintf( fp, "        %s\n", ( PSZ )( CPSZ )oElencoPeriodi[ iContPeriodi ] );                       \
                }                                                                                                       \
            }                                                                                                           \
        }                                                                                                               \
    }                                                                                                                   \
    if( ! iFound )                                                                                                      \
    {                                                                                                                   \
        sServiziUniformi.SERVIZIO = psServizio -> Servizi.SERVIZIO;                                                     \
    }                                                                                                                   \
}

//----------------------------------------------------------------------------
// Elabora_DSEROFF -----------------------------------------------------------
//----------------------------------------------------------------------------
int Elabora_DSEROFF( LISTE_DECODIFICA * psListeDecodifica, ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "Elabora_DSEROFF"

    int iRet = 0;
    FILE * fp = fopen( TRC_PATH "dseroff.trc", "w" );

    printf( "\nInizio elaborazione DSEROFF\n" );
    fprintf( fp, "\nInizio elaborazione DSEROFF\n" );

    struct MEZZO_VIRTUALE sMezzoVirtuale;
    struct MEZZI_VIAGGIANTI sMezzoViaggiante;


    if( OkPrintf ) printf( "\nApertura files... " );
    fprintf( fp, "\nApertura files... " );

    // File di input
    FILE_MEZZI_VIAGGIANTI * poMezzoViaggiante = new FILE_MEZZI_VIAGGIANTI( PATH_IN "MEZZIVGG.TMP" );

    // File di output
    FILE_DSEROFF * poDSEROFF = new FILE_DSEROFF( PATH_OUT "DSEROFF.DAT" );

    // Preparo il file di output per la scrittura
    poDSEROFF -> Clear( NUSTR, FALSE );

    // Controllo l'apertura di tutti i file
    if( ! poMezzoViaggiante -> FileHandle() )
    {
        BEEP;
        printf( "IN ERRORE! (MEZZIVGG.TMP)\n" );
        fprintf( fp, "IN ERRORE! (MEZZIVGG.TMP)\n" );
        iRet = 1;
    }
    else if( ! poDSEROFF -> FileHandle() )
    {
        BEEP;
        printf( "IN ERRORE! (DSEROFF.DAT)\n" );
        fprintf( fp, "IN ERRORE! (DSEROFF.DAT)\n" );
        iRet = 1;
    }
    else
    {
        printf( "Completata.\n" );
        fprintf( fp, "Completata.\n" );


        if( OkPrintf ) printf( "Calcolo lunghezza files... " );
        fprintf( fp, "Calcolo lunghezza files... " );

        // Contatori
        ULONG ulNumMezziViaggianti = poMezzoViaggiante -> NumRecordsTotali();
        ULONG ulCurrServizio = 0L;
        ULONG ulCurrMezzoViaggiante;    // Lo azzero ad ogni servizio letto.
        ULONG ulNumScritti = 0L;

        // Variabili di appoggio
        CHAR szAppo[ 31 ];
        int iCodiceTipoServizio = 0;
        int iCodiceServizio = 0;
        MM_INFO sSrv;
        ELENCO oElencoMezziViaggianti;
        ELENCO oElencoServiziMezzoViaggiante;
        ELENCO oElencoServiziNonUniformi;
        ELENCO oElencoServizi;
        ELENCO oElencoDettagliServizio;
        ELENCO oElencoStazioni( 0, 240000 );

        if( OkPrintf ) printf( "Completata.\n\n" );
        fprintf( fp, "Completata.\n\n" );
        if( OkPrintf ) printf( "Numero Mezzi Viaggianti....: %ld\n", ulNumMezziViaggianti );
        fprintf( fp, "Numero Mezzi Viaggianti....: %ld\n", ulNumMezziViaggianti );

        // Inizializzo il sistema della periodicit….
        if( OkPrintf ) printf( "\nInizializzazione della periodicit…... " );
        fprintf( fp, "\nInizializzazione della periodicit…... " );
        T_PERIODICITA :: Init( DATE_PATH, "DATE.T" );
        T_PERIODICITA :: ImpostaProblema( T_PERIODICITA :: Inizio_Dati_Caricati, T_PERIODICITA :: Fine_Dati_Caricati, T_PERIODICITA :: Inizio_Dati_Caricati );
        if( OkPrintf ) printf( "Completata.\n" );
        fprintf( fp, "Completata.\n" );

        iRet = CaricaServizi2( oElencoServizi );
        if( iRet )
        {
            printf( "CaricaServizi2() in errore!\n" );
            fprintf( fp, "CaricaServizi2() in errore!\n" );
        }
        else
        {
            iRet = CaricaDettagliServizio( oElencoDettagliServizio );
            if( iRet )
            {
                printf( "CaricaDettagliServizio() in errore!\n" );
                fprintf( fp, "CaricaDettagliServizio() in errore!\n" );
            }
            else
            {
                iRet = CaricaFermate2( oElencoStazioni );
                if( iRet )
                {
                    printf( "CaricaFermate2() in errore!\n" );
                    fprintf( fp, "CaricaFermate2() in errore!\n" );
                }
                else
                {
                    // Riempio l'elenco delle famiglie.
                    iRet = ElencaMezziViaggianti( poMezzoViaggiante,
                                                  ulNumMezziViaggianti,
                                                  roElencoRighe,
                                                  oElencoMezziViaggianti );
                    if( iRet )
                    {
                        printf( "ElencaMezziViaggianti() in errore!\n" );
                        fprintf( fp, "ElencaMezziViaggianti() in errore!\n" );
                    }
                    else
                    {
                        // Pulisco preventivamente la struttura in memoria.
                        sSrv.Clear();

                        // Ciclo sui mezzi viaggianti (MEZZIVGG.TMP in memoria).
                        STRINGA oOldMezzo = NUSTR;
                        struct MEZZI_VIAGGIANTI * psMezzoViaggiante = NULL;
                        struct MEZZI_VIAGGIANTI * psLastMezzoViaggiante = NULL;

                        psMezzoViaggiante = ( struct MEZZI_VIAGGIANTI * )oElencoMezziViaggianti[ ( unsigned int )0 ];
                        oOldMezzo = psMezzoViaggiante -> Famiglia;

                        ORD_FORALL( oElencoMezziViaggianti, iIndexMezziViaggianti )
                        {
                            psMezzoViaggiante = ( struct MEZZI_VIAGGIANTI * )oElencoMezziViaggianti[ iIndexMezziViaggianti ];

                            if( STRINGA( psMezzoViaggiante -> Famiglia ) != oOldMezzo )
                            {
                                // Ho cambiato progressivo all'interno della famiglia.
                                // Devo esaminare i bit dei servizi e scrivere i record
                                // in output.
                                ulNumScritti += DecodificaScriviServiziUniformi( psLastMezzoViaggiante,
                                                                                 sSrv,
                                                                                 poDSEROFF,
                                                                                 fp,
                                                                                 psListeDecodifica );

                                // Salvo la nuova coppia NUMVIR+PRGVIR.
                                oOldMezzo = psMezzoViaggiante -> Famiglia;

                                // Pulisco la struttura in memoria.
                                sSrv.Clear();
                            }

                            // Cerco nel file F_DETSER i record corrispondenti al mezzo viag-
                            // giante di turno.
                            iRet = RicercaServiziMezzoViaggiante( oElencoServizi,
                                                                  * psMezzoViaggiante,
                                                                  sSrv,
                                                                  oElencoServiziMezzoViaggiante,
                                                                  oElencoServiziNonUniformi,
                                                                  oElencoDettagliServizio,
                                                                  oElencoStazioni,
                                                                  psListeDecodifica,
                                                                  poDSEROFF,
                                                                  & ulNumScritti,
                                                                  fp );
                            if( iRet )
                            {
                                printf( "RicercaServizi() in errore!\n" );
                                fprintf( fp, "RicercaServizi() in errore!\n" );
                                break;
                            }

                            // Combino i servizi trovati per il mezzo viaggiante.
                            ORD_FORALL( oElencoServiziMezzoViaggiante, iIndexServiziMezzo )
                            {
                                FILE_SERVIZI :: R_STRU * psServizio = ( FILE_SERVIZI :: R_STRU * )oElencoServiziMezzoViaggiante[ iIndexServiziMezzo ];
                                sSrv.CombinaTratta( psServizio -> Servizi );
                            }
                            if( OkPrintf ) printf( "Mezzi viaggianti processati: %05d, Record scritti: %05d"
                                    "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
                                    iIndexMezziViaggianti + 1, ulNumScritti );

                            psLastMezzoViaggiante = psMezzoViaggiante;
                        }
                    }
                }
            }
        }

        if( OkPrintf ) printf( "\nScaricamento elenchi... " );
        FORALL( oElencoMezziViaggianti, iContMezziViaggianti )
        {
            struct MEZZI_VIAGGIANTI * psMezzoViaggiante = ( MEZZI_VIAGGIANTI * )oElencoMezziViaggianti[ iContMezziViaggianti ];
            delete psMezzoViaggiante;
        }
        FORALL( oElencoServiziMezzoViaggiante, iContServiziMezzoViaggiante )
        {
            FILE_SERVIZI :: R_STRU * psServizio = ( FILE_SERVIZI :: R_STRU * )oElencoServiziMezzoViaggiante[ iContServiziMezzoViaggiante ];
            delete psServizio;
        }
        FORALL( oElencoServiziNonUniformi, iContServiziNonUniformi )
        {
            FILE_SERVIZI :: R_STRU * psServizio = ( FILE_SERVIZI :: R_STRU * )oElencoServiziNonUniformi[ iContServiziNonUniformi ];
            delete psServizio;
        }
        FORALL( oElencoServizi, iContServizi )
        {
            FILE_SERVIZI :: R_STRU * psServizio = ( FILE_SERVIZI :: R_STRU * )oElencoServizi[ iContServizi ];
            delete psServizio;
        }
        FORALL( oElencoDettagliServizio, iContDettagliServizio )
        {
            struct DETTAGLIO_SERVIZI * psDettaglioServizio = ( DETTAGLIO_SERVIZI * )oElencoDettagliServizio[ iContDettagliServizio ];
            delete psDettaglioServizio;
        }
        FORALL( oElencoStazioni, iContStazioni )
        {
            struct FERMATE_VIRT * psFermata = ( FERMATE_VIRT * )oElencoStazioni[ iContStazioni ];

            delete psFermata;
        }
        if( OkPrintf ) printf( "completato.\n" );

        // Chiusura files
        if( OkPrintf ) printf( "Chiusura files... " );
        delete poDSEROFF;
        delete poMezzoViaggiante;
    }

    fclose( fp );

    return iRet;
}

//----------------------------------------------------------------------------
// ElencaMezziViaggianti -----------------------------------------------------
//----------------------------------------------------------------------------
// Questa funzione riempie 'roElencoMezziViaggianti' con i numeri virtuali a
// prescindere dai progressivi.
//----------------------------------------------------------------------------
int ElencaMezziViaggianti( FILE_MEZZI_VIAGGIANTI * poMezziViaggianti,
                           ULONG ulNumMezziViaggianti,
                           ELENCO & roElencoRighe,
                           ELENCO & roElencoMezziViaggianti )
{
    #undef TRCRTN
    #define TRCRTN "ElencaMezziViaggianti"

    int iRet = 0;
    int iFamiglia = 0;
    FILE * fp = fopen( TRC_PATH "mvgg.trc", "w" );
    struct MEZZI_VIAGGIANTI sMezzoViaggiante;

    if( OkPrintf ) printf( "Creaz. elenco mezzi vgg. ... " );

    // Ciclo sul file MEZZIVGG.TMP.
    for( int iIndex = 0; iIndex < ulNumMezziViaggianti; iIndex ++ )
    {
        sMezzoViaggiante = poMezziViaggianti -> FixRec( iIndex );

        struct MEZZI_VIAGGIANTI * psMezzo = new MEZZI_VIAGGIANTI;
        memcpy( psMezzo, & sMezzoViaggiante, sizeof( struct MEZZI_VIAGGIANTI ) );
        roElencoMezziViaggianti += ( void * )psMezzo;
    }

//  // Ordino l'elenco dei mezzi viaggianti.
//  roElencoMezziViaggianti.Sort( OrdinaMezziViaggianti, sizeof( struct MEZZI_VIAGGIANTI ) );

    // Scrivo la traccia.
    if( OkPrintf ) printf( "scrit. traccia... " );
    ORD_FORALL( roElencoMezziViaggianti , iIndex2 )
    {
        struct MEZZI_VIAGGIANTI * psMezzo = ( struct MEZZI_VIAGGIANTI * )roElencoMezziViaggianti [ iIndex2 ];
        fprintf( fp, "Famiglia: %s\n", psMezzo -> Famiglia );
    }

    if( OkPrintf ) printf( "OK.\n" );

    fclose( fp );

    return iRet;
}

// //----------------------------------------------------------------------------
// // CallbackOrdinaMezzi -------------------------------------------------------
// //----------------------------------------------------------------------------
// int CallbackOrdinaMezzi( const void * pvMezzo1, const void * pvMezzo2 )
// {
//     struct MEZZI_VIAGGIANTI * psMezzo1 = ( struct MEZZI_VIAGGIANTI * )pvMezzo1;
//     struct MEZZI_VIAGGIANTI * psMezzo2 = ( struct MEZZI_VIAGGIANTI * )pvMezzo2;
//
//     return strcmp( psMezzo1 -> Famiglia, psMezzo2 -> Famiglia );
// }

//----------------------------------------------------------------------------
// RicercaServiziMezzoViaggiante ---------------------------------------------
//----------------------------------------------------------------------------
// Questa funzione accede al fine F_SRVTRN ed estrae i record corrispondenti
// al mezzo viaggiante passato; i servizi uniformi li inserisce nell'elenco
// 'roElencoServiziMezzoViaggiante', quelli non uniformi nell'elenco
// 'roElencoServiziNonUniformi'. Inoltre combina le MM_INFO dei servizi
// uniformi nella struttura 'rsSrv'.
//----------------------------------------------------------------------------
int RicercaServiziMezzoViaggiante( ELENCO & roElencoServizi,
                                   MEZZI_VIAGGIANTI & rsMezzoViaggiante,
                                   MM_INFO & rsSrv,
                                   ELENCO & roElencoServiziMezzoViaggiante,
                                   ELENCO & roElencoServiziNonUniformi,
                                   ELENCO & roElencoDettagliServizio,
                                   ELENCO & roElencoStazioni,
                                   LISTE_DECODIFICA * psListeDecodifica,
                                   FILE_DSEROFF * poDSEROFF,
                                   ULONG * pulNumScritti,
                                   FILE * fp )
{
    #undef TRCRTN
    #define TRCRTN "RicercaServiziMezzoViaggiante"

    int iRet = 0;
    int iTrovatoMezzoViaggiante = FALSE;
    struct FILE_SERVIZI :: R_STRU * psServizio = NULL;
    char szAppo[ 31 ];

    memset( szAppo, 0x00, 31 );
    memcpy( szAppo, rsMezzoViaggiante.IdentTreno, 10 );
    fprintf( fp, "\nMezzo Viaggiante: %s (%05d) - Famiglia: %05d, Prog.: %02d\n",
                 szAppo,
                 rsMezzoViaggiante.MezzoViaggiante,
                 rsMezzoViaggiante.MezzoVirtuale,
                 rsMezzoViaggiante.ProgressivoVirtuale );
    fprintf( fp, "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ\n" );

    // Svuoto l'elenco dei servizi per il mezzo viaggiante corrente.
    roElencoServiziMezzoViaggiante.Clear();

    // Ciclo sul file F_SRVTRN. Per ora utilizzo una ricerca sequenziale
    // brutale per non complicare ulteriormente la logica; in futuro... ;)
    ORD_FORALL( roElencoServizi, iIndex )
    {
        psServizio = ( FILE_SERVIZI :: R_STRU * )roElencoServizi[ iIndex ];

        if( psServizio -> NumeroMezzoVg == rsMezzoViaggiante.MezzoViaggiante )
        {
            iTrovatoMezzoViaggiante = TRUE;

            if( psServizio -> Uniforme )
            {
                FILE_SERVIZI :: R_STRU * poServizioUniforme = new FILE_SERVIZI :: R_STRU;
                memcpy( poServizioUniforme, psServizio, sizeof( FILE_SERVIZI :: R_STRU ) );
                roElencoServiziMezzoViaggiante += ( void * )poServizioUniforme;

                rsSrv.CombinaTratta( psServizio -> Servizi );

                fprintf( fp, "Servizio UNIFORME: %s\n", ( PSZ )( CPSZ )psServizio -> Servizi.Decodifica() );
            }
            else
            {
                FILE_SERVIZI :: R_STRU * poServizioNonUniforme = new FILE_SERVIZI :: R_STRU;
                memcpy( poServizioNonUniforme, psServizio, sizeof( FILE_SERVIZI :: R_STRU ) );
                roElencoServiziNonUniformi += ( void * )poServizioNonUniforme;

                fprintf( fp, "Servizio NON UNIFORME: %s", ( PSZ )( CPSZ )psServizio -> Servizi.Decodifica() );
                fprintf( fp, " - Inizio decodifica...\n" );

                // A questo punto mi ritrovo con una struttura MM_INFO che
                // ha uno o pi— servizi non uniformi. Devo discernere tra
                // i servizi uniformi (che accorper• a 'rsSrv') e non uniformi
                // (che dovr• andare a ricercare in 'oElencoDettagliServizio'
                // per il mezzo viaggiante corrente); per ognuno di questi ul-
                // timi, scriver• uno o pi— record in output, a seconda dei
                // periodi trovati.

                // Mi definisco una struttura MM_INFO di appoggio nella quale
                // inserir• i soli servizi che risulteranno essere comunque
                // uniformi. Sar… questa struttura ad essere combinata con
                // 'rsSrv'.
                MM_INFO sServiziUniformi;
                int iFound = FALSE;

                // Struttura di output
                struct DSEROFF sDSEROFF;

                memset( & sServiziUniformi, 0x00, sizeof( MM_INFO ) );

                // Comincio ad esaminare i gruppi di servizi alla ricerca di
                // quelli non uniformi.

                // Controllo i servizi prenotabili
                if( psServizio -> Servizi.PrenotDisUniforme )
                {
                    // Ricerco nei dettagli servizio se c'Š (per il mezzo viag-
                    // giante corrente) un record con una struttura MM_INFO che
                    // abbia settato il bit del servizio di turno.

                    EsaminaServizio( PostiASederePrima, "    PostiASederePrima", 2, 1 );
                    EsaminaServizio( PostiASedereSeconda, "    PostiASedereSeconda", 2, 2 );
                    EsaminaServizio( SleeperetteSeconda, "    SleeperetteSeconda", 2, 3 );
                    EsaminaServizio( AutoAlSeguito, "    AutoAlSeguito", 8, 0 );
                    EsaminaServizio( Invalidi, "    Invalidi", 6, 1 );
                }
                else
                {
                    sServiziUniformi.PostiASederePrima = psServizio -> Servizi.PostiASederePrima;
                    sServiziUniformi.PostiASedereSeconda = psServizio -> Servizi.PostiASedereSeconda;
                    sServiziUniformi.SleeperetteSeconda = psServizio -> Servizi.SleeperetteSeconda;
                    sServiziUniformi.AutoAlSeguito = psServizio -> Servizi.AutoAlSeguito;
                    sServiziUniformi.Invalidi = psServizio -> Servizi.Invalidi;
                }

                // Controllo i servizi di trasporto
                if( psServizio -> Servizi.ServTraspDisUniformi )
                {
                    // Ricerco nei dettagli servizio se c'Š (per il mezzo viag-
                    // giante corrente) un record con una struttura MM_INFO che
                    // abbia settato il bit del servizio di turno.

                    EsaminaServizio( PostiASederePrima, "    PostiASederePrima", ( ( psServizio -> Servizi.Prenotabilita ) ? 2 : 1 ), 1 );
                    EsaminaServizio( PostiASedereSeconda, "    PostiASedereSeconda", ( ( psServizio -> Servizi.Prenotabilita ) ? 2 : 1 ), 2 );
                    EsaminaServizio( SleeperetteSeconda, "    SleeperetteSeconda", ( ( psServizio -> Servizi.Prenotabilita ) ? 2 : 1 ), 3 );

// Modificato il 29/05/1997 su richiesta di Massimo Romano.
//                  EsaminaServizio( CuccetteSeconda, "    CuccetteSeconda", 4, 2 );
                    EsaminaServizio( CuccetteSeconda, "    CuccetteSeconda", 4, 0 );

// Tolto il 29/05/1997 su richiesta di Massimo Romano.
//                  EsaminaServizio( VagoniLettoPrima, "    VagoniLettoPrima", 3, 1 );
//                  EsaminaServizio( VagoniLettoSeconda, "    VagoniLettoSeconda", 3, 2 );

                    if( psServizio -> Servizi.VagoniLettoPrima || psServizio -> Servizi.VagoniLettoSeconda )
                    {
                        EsaminaServizio( VagoniLettoPrima, "    VagoniLettoPrima/Seconda", 3, 0 );
                    }
//                  EsaminaServizio( VagoniLettoSeconda, "    VagoniLettoSeconda", 3, 0 );
                    EsaminaServizio( Biciclette, "    Biciclette", 6, 2 );
                    EsaminaServizio( Animali, "    Animali", 6, 3 );

         /*****/    EsaminaServizio( AutoAlSeguito, "    AutoAlSeguito", 8, 0 );
         /*****/    EsaminaServizio( Invalidi, "    Invalidi", 6, 1 );
                }
                else
                {
                    sServiziUniformi.PostiASederePrima = psServizio -> Servizi.PostiASederePrima;
                    sServiziUniformi.PostiASedereSeconda = psServizio -> Servizi.PostiASedereSeconda;
                    sServiziUniformi.SleeperetteSeconda = psServizio -> Servizi.SleeperetteSeconda;
                    sServiziUniformi.CuccetteSeconda = psServizio -> Servizi.CuccetteSeconda;
                    sServiziUniformi.VagoniLettoPrima = psServizio -> Servizi.VagoniLettoPrima;
                    sServiziUniformi.VagoniLettoSeconda = psServizio -> Servizi.VagoniLettoSeconda;
                    sServiziUniformi.Biciclette = psServizio -> Servizi.Biciclette;
                    sServiziUniformi.Animali = psServizio -> Servizi.Animali;

         /*****/    sServiziUniformi.AutoAlSeguito = psServizio -> Servizi.AutoAlSeguito;
         /*****/    sServiziUniformi.Invalidi = psServizio -> Servizi.Invalidi;
                }

                // Controllo i servizi Generici
                if( psServizio -> Servizi.ServGenerDisUniformi )
                {
                    // Ricerco nei dettagli servizio se c'Š (per il mezzo viag-
                    // giante corrente) un record con una struttura MM_INFO che
                    // abbia settato il bit del servizio di turno.

                    if( psServizio -> Servizi.Ristoro || psServizio -> Servizi.BuffetBar )
                    {
                        EsaminaServizio( Ristoro, "    Ristoro/BuffetBar", 5, 5 );
                    }
//                  EsaminaServizio( BuffetBar, "    BufferBar", 5, 5 );
                    EsaminaServizio( SelfService, "    SelfService", 5, 4 );
                    EsaminaServizio( Ristorante, "    Ristorante", 5, 0 );
                }
                else
                {
                    sServiziUniformi.Ristoro = psServizio -> Servizi.Ristoro;
                    sServiziUniformi.BuffetBar = psServizio -> Servizi.BuffetBar;
                    sServiziUniformi.SelfService = psServizio -> Servizi.SelfService;
                    sServiziUniformi.Ristorante = psServizio -> Servizi.Ristorante;
                }

                if( ! psServizio -> Servizi.PrenotDisUniforme  &&
                    ! psServizio -> Servizi.ServTraspDisUniformi &&
                    ! psServizio -> Servizi.ServGenerDisUniformi )
                {
                    printf( "Un rec. di F_SRVTRN (MVgg=%05d) Š disuniforme, ma non ha nessun bit in ON.\n", psServizio -> NumeroMezzoVg );
                }

                rsSrv.CombinaTratta( sServiziUniformi );
            }
        }
    }

    return iRet;
}

//----------------------------------------------------------------------------
// CaricaServizi2 -------------------------------------------------------------
//----------------------------------------------------------------------------
int CaricaServizi2( ELENCO & roElencoServizi )
{
    #undef TRCRTN
    #define TRCRTN "CaricaServizi2"

    int iRet = 0;

    if( OkPrintf ) printf( "Caricamento servizi... " );

    FILE_SERVIZI :: R_STRU * psServizio;
    FILE_SERVIZI * poServizi = new FILE_SERVIZI( PATH_IN "F_SRVTRN.TMP", sizeof( struct FILE_SERVIZI :: R_STRU ) );
    ULONG ulNumServizi = poServizi -> NumRecordsTotali();

    for( int iIndex = 0; iIndex < ulNumServizi; iIndex ++ )
    {
        psServizio = new FILE_SERVIZI :: R_STRU;
        * psServizio = poServizi -> FixRec( iIndex );
        roElencoServizi += ( void * )psServizio;
    }

    delete poServizi;

    if( OkPrintf ) printf( "completato (%d record).\n", -- iIndex );

    return iRet;
}

//----------------------------------------------------------------------------
// CaricaDettagliServizio ----------------------------------------------------
//----------------------------------------------------------------------------
int CaricaDettagliServizio( ELENCO & roElencoDettagliServizio )
{
    #undef TRCRTN
    #define TRCRTN "CaricaDettagliServizio"

    int iRet = 0;

    if( OkPrintf ) printf( "Caricamento dettagli servizio... " );

    struct DETTAGLIO_SERVIZI * psDettaglioServizio;
    FILE_DETTAGLIO_SERVIZI * poDettaglioServizio = new FILE_DETTAGLIO_SERVIZI( PATH_IN "F_DETSER.TMP", sizeof( struct DETTAGLIO_SERVIZI ) );
    ULONG ulNumDettagliServizio = poDettaglioServizio -> NumRecordsTotali();

    for( int iIndex = 0; iIndex < ulNumDettagliServizio; iIndex ++ )
    {
        psDettaglioServizio = new DETTAGLIO_SERVIZI;
        * psDettaglioServizio = poDettaglioServizio -> FixRec( iIndex );
        roElencoDettagliServizio += ( void * )psDettaglioServizio;
    }

    delete poDettaglioServizio;

    if( OkPrintf ) printf( "completato (%d record).\n", -- iIndex );

    return iRet;
}

//----------------------------------------------------------------------------
// CaricaFermate2 -------------------------------------------------------------
//----------------------------------------------------------------------------
int CaricaFermate2( ELENCO & roElencoStazioni )
{
    #undef TRCRTN
    #define TRCRTN "CaricaFermate2"

    int iRet = 0;

    if( OkPrintf ) printf( "Caricamento fermate... " );

    struct FERMATE_VIRT * psFermata;
    F_FERMATE_VIRT * poFermata = new F_FERMATE_VIRT( PATH_IN "M0_FERMV.TM1" );
    ULONG ulNumFermate = poFermata -> NumRecordsTotali();

    for( int iIndex = 0; iIndex < ulNumFermate ; iIndex ++ )
    {
        psFermata = new FERMATE_VIRT ;
        * psFermata = poFermata -> FixRec( iIndex );
        roElencoStazioni += ( void * )psFermata;
    }

    delete poFermata;

    if( OkPrintf ) printf( "completato (%d record).\n", -- iIndex );

    return iRet;
}

//----------------------------------------------------------------------------
// DecodificaScriviServiziUniformi -------------------------------------------
//----------------------------------------------------------------------------
ULONG DecodificaScriviServiziUniformi( MEZZI_VIAGGIANTI * psMezzoViaggiante,
                                       MM_INFO & rsSrv,
                                       FILE_DSEROFF * poDSEROFF,
                                       FILE * fp,
                                       LISTE_DECODIFICA * psListeDecodifica )
{
    #undef TRCRTN
    #define TRCRTN "DecodificaScriviServiziUniformi"

    char szAppo[ 31 ];
    ULONG ulNumScritti = 0;

    // Struttura di output
    struct DSEROFF sDSEROFF;

    // Esamino i bit della struttura MM_INFO risultante e scrivo un record
    // per ogni servizio e per ogni periodo del mezzo virtuale di appartenenza.
    PERIODICITA_IN_CHIARO * poPeriodicitaInChiaro = NULL;

    if( ( poPeriodicitaInChiaro = psMezzoViaggiante -> PeriodicitaMezzoVirtuale.EsplodiPeriodicita() ) != 0 )
    {
        // Devo scrivere un record per ogni periodo trovato
        // nella periodicit… del mezzo virtuale corrente.
        if( ! poPeriodicitaInChiaro -> Dim() )
        {
            printf( "\nATTENZIONE: La Periodicit… del mezzo virtuale %05d non contiene periodi !\n", psMezzoViaggiante -> MezzoVirtuale );
            fprintf( fp, "\nATTENZIONE: La Periodicit… del mezzo virtuale %05d non contiene periodi !\n", psMezzoViaggiante -> MezzoVirtuale );
        }
        else
        {
            for( int iCont = 0; iCont < poPeriodicitaInChiaro -> Dim(); iCont ++ )
            {
                // Pulisco la struttura di output.
                memset( & sDSEROFF, 0x20, sizeof( DSEROFF ) );

                // Valorizzo NUMVIR e PRGVIR
                sprintf( szAppo, "%05d", psMezzoViaggiante -> MezzoVirtuale );
                memcpy( sDSEROFF.NUMVIR, szAppo, strlen( szAppo ) );
                sprintf( szAppo, "%02d", psMezzoViaggiante -> ProgressivoVirtuale );
                memcpy( sDSEROFF.PRGVIR, szAppo, strlen( szAppo ) );

                // Dato che il servizio Š uniforme rispetto al mezzo
                // virtuale, uso come progressivi di inizio e fine
                // (stazioni) quelle del mezzo viaggiante in canna.
                sprintf( szAppo, "%03d", psMezzoViaggiante -> PrgStazioneInizio );
                memcpy( sDSEROFF.PRGINI, szAppo, strlen( szAppo ) );
                sprintf( szAppo, "%03d", psMezzoViaggiante -> PrgStazioneFine );
                memcpy( sDSEROFF.PRGFIN, szAppo, strlen( szAppo ) );

                // Valorizzo DataInizio e DataFine del periodo.
                sprintf( szAppo, "%02d/%02d/%04d", ( * poPeriodicitaInChiaro )[ iCont ].Dal.Giorno,
                                                   ( * poPeriodicitaInChiaro )[ iCont ].Dal.Mese,
                                                   ( * poPeriodicitaInChiaro )[ iCont ].Dal.Anno );
                memcpy( sDSEROFF.DATAINI, szAppo, strlen( szAppo ) );
                sprintf( szAppo, "%02d/%02d/%04d", ( * poPeriodicitaInChiaro )[ iCont ].Al.Giorno,
                                                   ( * poPeriodicitaInChiaro )[ iCont ].Al.Mese,
                                                   ( * poPeriodicitaInChiaro )[ iCont ].Al.Anno );
                memcpy( sDSEROFF.DATAFIN, szAppo, strlen( szAppo ) );

                // Valorizzo CodiceGiornoSpeciale nella struttura
                // di output.
                if( ( * poPeriodicitaInChiaro )[ iCont ].TipoPeriodo != 0 )
                {
                    sprintf( szAppo, "%02d", ( * poPeriodicitaInChiaro )[ iCont ].TipoPeriodo );
                    memcpy( sDSEROFF.CODGIOSP, szAppo, strlen( szAppo ) );
                }
                else
                {
                    memcpy( sDSEROFF.CODGIOSP, "  ", 2 );
                }

                // Valorizzo Effettuazione nella struttura di output.
                sDSEROFF.EFFETT = ( * poPeriodicitaInChiaro )[ iCont ].Tipo ? 'E' : 'S';

                // Valorizzo GiorniSettimanali nella struttura
                // di output.
                BYTE bGiorni = ( * poPeriodicitaInChiaro )[ iCont ].GiorniDellaSettimana;
                memset( szAppo, 0x00, sizeof( szAppo ) );
                for( int iIndex = 0; iIndex < 7; iIndex ++ )
                {
                    char chGiorno = '0';
                    if( bGiorni & ( 1 << iIndex ) )
                    {
                        chGiorno = '1';
                    }
                    szAppo[ iIndex ] = chGiorno;
                }
                memcpy( sDSEROFF.GIORNISET, szAppo, strlen( szAppo ) );

                // Elimino i campi non significativi per il TPF
                if( ! memcmp( sDSEROFF.DATAINI, sDSEROFF.DATAFIN, 10 ) )
                {
                    memset( sDSEROFF.CODGIOSP, 0x20, 2 );
                    memset( sDSEROFF.GIORNISET, 0x20, 7 );
                }
                if( ! memcmp( sDSEROFF.GIORNISET, "1111111", 7 ) &&
                    ! memcmp( sDSEROFF.CODGIOSP, "  ", 2 ) )
                {
                    memset( sDSEROFF.GIORNISET, 0x20, 7 );
                }
                if( ! memcmp( sDSEROFF.GIORNISET, "1111111", 7 ) )
                {
                    memset( sDSEROFF.GIORNISET, 0x20, 7 );
                }

                // I campi Classe, Obbligatorio e APagamento non
                // vengono ad oggi valorizzati. Valorizzo gli
                // ultimi campi della struttura di output.
                memcpy( sDSEROFF.DATINIVAL, (const char *)psListeDecodifica -> oDataInizio, 10 );
                memcpy( sDSEROFF.CODDSZ, ( const char * )psListeDecodifica -> oCodiceDistribuzione, 8 );
                memcpy( sDSEROFF.CR_LF, "\r\n", 2 );

                // Ora scrivo un record per servizio trovato nella MM_INFO.
                // Riuso la maggior parte della struttura di output cambiando
                // solo i campi CODSER e CODTIPSER.
                int iCodiceTipoServizio = 0;
                int iCodiceServizio = 0;

                if( rsSrv.Prenotabilita )
                {
                    iCodiceTipoServizio = 2;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );

                    if( rsSrv.PostiASederePrima )
                    {
                        iCodiceServizio = 1;
                        sprintf( szAppo, "%02d", iCodiceServizio );
                        memcpy( sDSEROFF.CODSER, szAppo, 2 );
                        poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                        ulNumScritti ++;
                    }
                    if( rsSrv.PostiASedereSeconda )
                    {
                        iCodiceServizio = 2;
                        sprintf( szAppo, "%02d", iCodiceServizio );
                        memcpy( sDSEROFF.CODSER, szAppo, 2 );
                        poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                        ulNumScritti ++;
                    }
                    if( rsSrv.SleeperetteSeconda )
                    {
                        iCodiceServizio = 3;
                        sprintf( szAppo, "%02d", iCodiceServizio );
                        memcpy( sDSEROFF.CODSER, szAppo, 2 );
                        poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                        ulNumScritti ++;
                    }
                }
                else
                {
                    iCodiceTipoServizio = 1;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );

                    if( rsSrv.PostiASederePrima )
                    {
                        iCodiceServizio = 1;
                        sprintf( szAppo, "%02d", iCodiceServizio );
                        memcpy( sDSEROFF.CODSER, szAppo, 2 );
                        poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                        ulNumScritti ++;
                    }
                    if( rsSrv.PostiASedereSeconda )
                    {
                        iCodiceServizio = 2;
                        sprintf( szAppo, "%02d", iCodiceServizio );
                        memcpy( sDSEROFF.CODSER, szAppo, 2 );
                        poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                        ulNumScritti ++;
                    }
                    if( rsSrv.SleeperetteSeconda )
                    {
                        iCodiceServizio = 3;
                        sprintf( szAppo, "%02d", iCodiceServizio );
                        memcpy( sDSEROFF.CODSER, szAppo, 2 );
                        poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                        ulNumScritti ++;
                    }
                }

                if( rsSrv.VagoniLettoPrima || rsSrv.VagoniLettoSeconda )
                {
                    iCodiceTipoServizio = 3;
                    iCodiceServizio = 0;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }

                if( rsSrv.CuccetteSeconda )
                {
                    iCodiceTipoServizio = 4;

// Modificato il 30/05/1997 su richiesta di Massimo Romano.
//                  iCodiceServizio = 2;
                    iCodiceServizio = 0;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }

                if( rsSrv.Ristoro || rsSrv.BuffetBar )
                {
                    iCodiceTipoServizio = 5;
                    iCodiceServizio = 5;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }

                if( rsSrv.SelfService )
                {
                    iCodiceTipoServizio = 5;
                    iCodiceServizio = 4;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }

                if( rsSrv.Ristorante )
                {
                    iCodiceTipoServizio = 5;
                    iCodiceServizio = 0;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }

                if( rsSrv.Invalidi )
                {
                    iCodiceTipoServizio = 6;
                    iCodiceServizio = 1;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }

                if( rsSrv.Biciclette )
                {
                    iCodiceTipoServizio = 6;
                    iCodiceServizio = 2;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }

                if( rsSrv.Animali )
                {
                    iCodiceTipoServizio = 6;
                    iCodiceServizio = 3;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }

                if( rsSrv.AutoAlSeguito )
                {
                    iCodiceTipoServizio = 8;
                    iCodiceServizio = 0;

                    sprintf( szAppo, "%02d", iCodiceTipoServizio );
                    memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
                    sprintf( szAppo, "%02d", iCodiceServizio );
                    memcpy( sDSEROFF.CODSER, szAppo, 2 );
                    poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
                    ulNumScritti ++;
                }
            }
        }
    }

    return ulNumScritti;
}

//----------------------------------------------------------------------------
// ValorizzaScriviStruttura --------------------------------------------------
//----------------------------------------------------------------------------
int ValorizzaScriviStruttura( int iUniforme,
                              int iCodiceTipoServizio,
                              int iCodiceServizio,
                              MEZZI_VIAGGIANTI & rsMezzoViaggiante,
                              PERIODICITA_IN_CHIARO * poPeriodicitaInChiaro,
                              int iContPeriodi,
                              void * pvServizio,
                              size_t iSizeServizio,
                              ELENCO & roElencoStazioni,
                              LISTE_DECODIFICA * psListeDecodifica,
                              FILE * fp,
                              FILE_DSEROFF * poDSEROFF )
{
    #undef TRCRTN
    #define TRCRTN "ValorizzaScriviStruttura"

    int iRet = 0;
    struct DSEROFF sDSEROFF;
    char szAppo[ 31 ];

    // Pulisco la struttura di output.
    memset( & sDSEROFF, 0x20, sizeof( DSEROFF ) );

    // Valorizzo NUMVIR e PRGVIR
    sprintf( szAppo, "%05d", rsMezzoViaggiante.MezzoVirtuale );
    memcpy( sDSEROFF.NUMVIR, szAppo, strlen( szAppo ) );
    sprintf( szAppo, "%02d", rsMezzoViaggiante.ProgressivoVirtuale );
    memcpy( sDSEROFF.PRGVIR, szAppo, strlen( szAppo ) );

    if( iUniforme )
    {
        // Dato che il servizio Š uniforme rispetto al mezzo
        // virtuale, uso come progressivi di inizio e fine
        // (stazioni) quelle del mezzo viaggiante in canna.
        sprintf( szAppo, "%03d", rsMezzoViaggiante.PrgStazioneInizio );
        memcpy( sDSEROFF.PRGINI, szAppo, strlen( szAppo ) );
        sprintf( szAppo, "%03d", rsMezzoViaggiante.PrgStazioneFine );
        memcpy( sDSEROFF.PRGFIN, szAppo, strlen( szAppo ) );
    }
    else
    {
        // Dato che il servizio NON Š uniforme rispetto al mezzo
        // virtuale, devo ricercare i progressivi di inizio e fine
        // del servizio a partire dai CCR del dettaglio.
        DETTAGLIO_SERVIZI sDettaglioServizi;
        memcpy( & sDettaglioServizi, pvServizio, iSizeServizio );
        iRet = DecodificaCodiciStazioni( sDettaglioServizi,
                                         roElencoStazioni,
                                         sDSEROFF,
                                         iContPeriodi == 0 ? fp : NULL,
                                         rsMezzoViaggiante );

        // Valorizzo i campi CodiceTipoServizio e CodiceServizio.
        sprintf( szAppo, "%02d", iCodiceTipoServizio );
        memcpy( sDSEROFF.CODTIPSER, szAppo, 2 );
        sprintf( szAppo, "%02d", iCodiceServizio );
        memcpy( sDSEROFF.CODSER, szAppo, 2 );
    }

    if( ! iRet )
    {
        // Valorizzo DataInizio e DataFine del periodo.
        sprintf( szAppo, "%02d/%02d/%04d", ( * poPeriodicitaInChiaro )[ iContPeriodi ].Dal.Giorno,
                                           ( * poPeriodicitaInChiaro )[ iContPeriodi ].Dal.Mese,
                                           ( * poPeriodicitaInChiaro )[ iContPeriodi ].Dal.Anno );
        memcpy( sDSEROFF.DATAINI, szAppo, strlen( szAppo ) );
        sprintf( szAppo, "%02d/%02d/%04d", ( * poPeriodicitaInChiaro )[ iContPeriodi ].Al.Giorno,
                                           ( * poPeriodicitaInChiaro )[ iContPeriodi ].Al.Mese,
                                           ( * poPeriodicitaInChiaro )[ iContPeriodi ].Al.Anno );
        memcpy( sDSEROFF.DATAFIN, szAppo, strlen( szAppo ) );

        // Valorizzo CodiceGiornoSpeciale nella struttura
        // di output.
        if( ( * poPeriodicitaInChiaro )[ iContPeriodi ].TipoPeriodo != 0 )
        {
            sprintf( szAppo, "%02d", ( * poPeriodicitaInChiaro )[ iContPeriodi ].TipoPeriodo );
            memcpy( sDSEROFF.CODGIOSP, szAppo, strlen( szAppo ) );
        }
        else
        {
            memcpy( sDSEROFF.CODGIOSP, "  ", 2 );
        }

        // Valorizzo Effettuazione nella struttura di output.
        sDSEROFF.EFFETT = ( * poPeriodicitaInChiaro )[ iContPeriodi ].Tipo ? 'E' : 'S';

        // Valorizzo GiorniSettimanali nella struttura
        // di output.
        BYTE bGiorni = ( * poPeriodicitaInChiaro )[ iContPeriodi ].GiorniDellaSettimana;
        memset( szAppo, 0x00, sizeof( szAppo ) );
        for( int iIndex = 0; iIndex < 7; iIndex ++ )
        {
            char chGiorno = '0';
            if( bGiorni & ( 1 << iIndex ) )
            {
                chGiorno = '1';
            }
            szAppo[ iIndex ] = chGiorno;
        }
        memcpy( sDSEROFF.GIORNISET, szAppo, strlen( szAppo ) );

        // Elimino i campi non significativi per il TPF
        if( ! memcmp( sDSEROFF.DATAINI, sDSEROFF.DATAFIN, 10 ) )
        {
            memset( sDSEROFF.CODGIOSP, 0x20, 2 );
            memset( sDSEROFF.GIORNISET, 0x20, 7 );
        }
        if( ! memcmp( sDSEROFF.GIORNISET, "1111111", 7 ) &&
            ! memcmp( sDSEROFF.CODGIOSP, "  ", 2 ) )
        {
            memset( sDSEROFF.GIORNISET, 0x20, 7 );
        }
        if( ! memcmp( sDSEROFF.GIORNISET, "1111111", 7 ) )
        {
            memset( sDSEROFF.GIORNISET, 0x20, 7 );
        }

        // I campi Classe, Obbligatorio e APagamento non
        // vengono ad oggi valorizzati. Valorizzo gli
        // ultimi campi della struttura di output.
        memcpy( sDSEROFF.DATINIVAL, (const char *)psListeDecodifica -> oDataInizio, 10 );
        memcpy( sDSEROFF.CODDSZ, ( const char * )psListeDecodifica -> oCodiceDistribuzione, 8 );
        memcpy( sDSEROFF.CR_LF, "\r\n", 2 );

        poDSEROFF -> AddRecordToEnd( BUFR( & sDSEROFF, sizeof( DSEROFF ) ) );
    }

    return iRet;
}

//----------------------------------------------------------------------------
// DecodificaCodiciStazioni --------------------------------------------------
//----------------------------------------------------------------------------
int DecodificaCodiciStazioni( DETTAGLIO_SERVIZI & rsDettaglioServizi,
                              ELENCO & roElencoStazioni,
                              DSEROFF & rsDSEROFF,
                              FILE * fp,
                              MEZZI_VIAGGIANTI & rsMezzoViaggiante )
{
    #undef TRCRTN
    #define TRCRTN "DecodificaCodiciStazioni"

    int iRet = 0;
    char szAppo[ 31 ];
    char szCCR1[ 6 ];
    char szCCR2[ 6 ];
    long lCCRInizio, lCCRFine;

    memset( szAppo, 0x00, 31 );
    memcpy( szAppo, rsDettaglioServizi.Ccr1, 5 );
    strcpy( szCCR1, szAppo );
    lCCRInizio = atol( szAppo );

    memset( szAppo, 0x00, 31 );
    memcpy( szAppo, rsDettaglioServizi.Ccr2, 5 );
    strcpy( szCCR2, szAppo );
    lCCRFine = atol( szAppo );

    int iFoundInizio = 0;
    int iFoundFine = 0;

    struct FERMATE_VIRT * psFermateVirtuale = NULL;

    ORD_FORALL( roElencoStazioni, iContStazioni )
    {
        psFermateVirtuale = ( FERMATE_VIRT * )roElencoStazioni[ iContStazioni ];

        if( psFermateVirtuale -> MezzoVirtuale == rsMezzoViaggiante.iMezzoDaMotore )
        {
            if( psFermateVirtuale -> CCR == lCCRInizio )
            {
                sprintf( szAppo, "%03d", psFermateVirtuale -> Progressivo );
                if( fp ) fprintf( fp, " - St.Inizio:%03d(CCR:%s) ", psFermateVirtuale -> Progressivo, szCCR1 );
                memcpy( rsDSEROFF.PRGINI, szAppo, strlen( szAppo ) );
                iFoundInizio = 1;
            }
            else if( psFermateVirtuale -> CCR == lCCRFine )
            {
                sprintf( szAppo, "%03d", psFermateVirtuale -> Progressivo );
                if( fp ) fprintf( fp, " St.Fine:%03d(CCR:%s) ", psFermateVirtuale -> Progressivo, szCCR2 );
                memcpy( rsDSEROFF.PRGFIN, szAppo, strlen( szAppo ) );
                iFoundFine = 1;
            }
            if( iFoundInizio && iFoundFine )
            {
                break;
            }
        }
    }

    if( ! iFoundInizio || ! iFoundFine )
    {
        printf( "\nFermate di inizio e/o fine non trovate per MezzoVirtuale=%05d.\n", rsMezzoViaggiante.iMezzoDaMotore );
        iRet = 1;

        if( ! iFoundInizio )
        {
            if( fp ) fprintf( fp, " - St.Inizio:???(CCR:%s) ", szCCR1 );
        }
        if( ! iFoundFine )
        {
            if( fp ) fprintf( fp, " St.Fine:???(CCR:%s) ", szCCR2 );
        }
        if( fp ) fprintf( fp, "*** RECORD NON SCRITTO ***\n" );
    }
    else
    {
        if( fp ) fprintf( fp, "\n" );
    }

    return iRet;
}

// Questa Š la funzione di comparazione tra due mezzi viaggianti, utilizzata
// durante il processo di riordinamento dell'elenco dei mezzi viaggianti in
// base alla key formata dalla famiglia (numvir+prgvir calcolati).
//int OrdinaMezziViaggianti( const void * m1, const void * m2 )
//{
//    #undef TRCRTN
//    #define TRCRTN "OrdinaMezziViaggianti"
//
//    struct MEZZI_VIAGGIANTI * psMezzo1 = ( ( struct MEZZI_VIAGGIANTI * )*( LONG * )m1 );
//    struct MEZZI_VIAGGIANTI * psMezzo2 = ( ( struct MEZZI_VIAGGIANTI * )*( LONG * )m2 );
//
//    return strcmp( psMezzo1 -> Famiglia, psMezzo2 -> Famiglia );
//}
