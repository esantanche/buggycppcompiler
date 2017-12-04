//----------------------------------------------------------------------------
// FILE DCOSMEZV.CPP
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include <DB_HOST.HPP>
#include <stdio.h>

extern int OkPrintf;

static FILE * fp1;

//----------------------------------------------------------------------------
// PROTOTIPI
//----------------------------------------------------------------------------
int OrdinaMezziViaggianti( const void * m1, const void * m2 );

//----------------------------------------------------------------------------
// Elabora_DFERMEZVIR --------------------------------------------------------
//----------------------------------------------------------------------------
int Elabora_DCOSMEZVIR(LISTE_DECODIFICA * psListeDecodifica, ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "Elabora_DCOSMEZVIR"

    int iRet = 0;

    ELENCO oRigheOutput;

    printf( "\nInizio elaborazione DCOSMEZV\n" );

    fp1 = fopen( TRC_PATH "appo.trc", "w" );

    struct FERMATE_VIRT sFermateVirtuale;
    struct DCOSMEZVIR * psDCOSMEZVIR = NULL;

    F_FERMATE_VIRT * poFermateVirtuale = new F_FERMATE_VIRT(PATH_IN "M0_FERMV.TM1");
    F_MEZZO_VIRTUALE * poMezzoVirtuale = new F_MEZZO_VIRTUALE(PATH_IN "M0_TRENV.TM1");
    FILE_DCOSMEZVIR * poDCOSMEZVIR = new FILE_DCOSMEZVIR(PATH_OUT "DCOSMEZV.DAT");
    FILE_MEZZI_VIAGGIANTI * poMezziViaggianti = new FILE_MEZZI_VIAGGIANTI( PATH_IN "MEZZIVGG.TMP" );

    poDCOSMEZVIR->Clear(NUSTR, FALSE);
    poMezziViaggianti -> Clear( NUSTR, FALSE );

    if( ! poFermateVirtuale -> FileHandle() ||
        ! poMezzoVirtuale -> FileHandle() ||
        ! poMezziViaggianti -> FileHandle() ||
        ! poDCOSMEZVIR -> FileHandle() )
    {
        iRet = 1;
        BEEP;
        printf( "ERRORE apertura file M0_FERMV.TM1/M0_TRENV.TM1/DCOSMEZV.DAT\n" );
        TRACESTRING("ERRORE apertura file M0_FERMV.TM1/M0_TRENV.TM1/DCOSMEZV.DAT");
    }
    else
    {
        ULONG ulNumFermate = poFermateVirtuale->NumRecordsTotali();
        printf( "Numero fermate virtuali disponibili: %ld\n", ulNumFermate );
        ULONG ulCurrRec = 0L;

        int oldTrenoVirtuale = -1;
        int oldTrenoFisico = -1;
        int iProgressivoMezzoViaggiante = 1;
        int iProgressivoStazioneInizio = -1;
        int iProgressivoStazioneFine = -1;
        int iCount = 0;

        for( ulCurrRec = 0L; ulCurrRec < ulNumFermate; ulCurrRec ++ )
        {
            sFermateVirtuale = poFermateVirtuale->FixRec( ulCurrRec );

            if( ulCurrRec == 0L )   /* Solo la prima volta... */
            {
                oldTrenoVirtuale = sFermateVirtuale.MezzoVirtuale;
                oldTrenoFisico = sFermateVirtuale.TrenoFisico;
                iProgressivoStazioneInizio = sFermateVirtuale.Progressivo;
            }

            if( sFermateVirtuale.MezzoVirtuale == oldTrenoVirtuale )
            {
//              iProgressivoStazioneFine = sFermateVirtuale.Progressivo;

                if( sFermateVirtuale.TrenoFisico == oldTrenoFisico )
                {
                    iCount ++;
  /* **** */        iProgressivoStazioneFine = sFermateVirtuale.Progressivo;
                }
                else
                {
                    psDCOSMEZVIR = new DCOSMEZVIR;

//                  if( iCount > 0 )    // Se ho trovato pi— di un record per
//                  {                   // lo stesso TrenoFisico...
                        Scrivi_DCOSMEZV( poMezzoVirtuale,
                                         poDCOSMEZVIR,
                                         poMezziViaggianti,
                                         & sFermateVirtuale,
                                         psDCOSMEZVIR,
                                         psListeDecodifica,
                                         roElencoRighe,
                                         oldTrenoVirtuale,
                                         oldTrenoFisico,
                                         iProgressivoStazioneInizio,
                                         iProgressivoStazioneFine,
                                         iProgressivoMezzoViaggiante,
                                         iCount,
                                         oRigheOutput,
                                         ulCurrRec );
//                  }
                    iProgressivoMezzoViaggiante ++;
//                  iProgressivoStazioneInizio = sFermateVirtuale.Progressivo;
                    iProgressivoStazioneInizio = iProgressivoStazioneFine;
   /* ***** */      iProgressivoStazioneFine = sFermateVirtuale.Progressivo;
                    oldTrenoFisico = sFermateVirtuale.TrenoFisico;
                    iCount = 0;
                }
            }
            else
            {
                psDCOSMEZVIR = new DCOSMEZVIR;

//              iProgressivoStazioneFine = sFermateVirtuale.Progressivo;

                Scrivi_DCOSMEZV( poMezzoVirtuale,
                                 poDCOSMEZVIR,
                                 poMezziViaggianti,
                                 & sFermateVirtuale,
                                 psDCOSMEZVIR,
                                 psListeDecodifica,
                                 roElencoRighe,
                                 oldTrenoVirtuale,
                                 oldTrenoFisico,
                                 iProgressivoStazioneInizio,
                                 iProgressivoStazioneFine,
                                 iProgressivoMezzoViaggiante,
                                 iCount,
                                 oRigheOutput,
                                 ulCurrRec );
                iProgressivoMezzoViaggiante = 1;
                oldTrenoVirtuale = sFermateVirtuale.MezzoVirtuale;
                oldTrenoFisico = sFermateVirtuale.TrenoFisico;
                iProgressivoStazioneInizio = sFermateVirtuale.Progressivo;
                iProgressivoStazioneFine = sFermateVirtuale.Progressivo;
                iCount = 0;
            }
        }

        // Riordino il file dei mezzi viaggianti allo scopo di velocizzare
        // la successiva elaborazione per la tabella DSEROFF.
        printf( "\nInizio Sort del file MEZZIVGG.TMP... " );
        poMezziViaggianti -> ReSort( OrdinaMezziViaggianti );
        printf( "Terminata.\n" );

//      poMezziViaggianti -> Init();
        ULONG ulNumMezziVgg = poMezziViaggianti -> NumRecordsTotali();
        MEZZI_VIAGGIANTI sMezzoViaggiante;

        FILE * fp;

        fp = fopen( TRC_PATH "mezzivgg.trc", "w" );
        for( ULONG i = 0; i < ulNumMezziVgg ; i ++ )
        {
            sMezzoViaggiante = poMezziViaggianti -> FixRec( i );
            fprintf( fp, "Vgg: %05d Vir: %05d Prg: %02d Staz.Ini.: %03d Staz.Fin.: %03d\n",
                     sMezzoViaggiante.MezzoViaggiante,
                     sMezzoViaggiante.MezzoVirtuale,
                     sMezzoViaggiante.ProgressivoVirtuale,
                     sMezzoViaggiante.PrgStazioneInizio,
                     sMezzoViaggiante.PrgStazioneFine );
            if( OkPrintf ) printf( "%05d\b\b\b\b\b", i + 1 );
        }

        // Ora esamino l'elenco delle righe in output per assegnare
        // l'indicatore di percorso intero (INDPRCITA).
        printf( "\nInizio attribuzione 'Indicatore percorso intero'... " );
        int iIndexSave = 0;

        ORD_FORALL( oRigheOutput, iIndex )
        {
            struct DCOSMEZVIR * psRiga = ( DCOSMEZVIR * )oRigheOutput[ iIndex ];

            int iNumVir = 0;
            char szAppo[ 31 ];

            memset( szAppo, 0x00, 31 );
            memcpy( szAppo, psRiga -> NUMVIR, 5 );
            iNumVir = atoi( szAppo );

            int iMaxDelta = 0;
            int iNumTrovati = 0;
            iIndexSave = 0;

            ORD_FORALL( oRigheOutput, iIndex2 )
            {
                struct DCOSMEZVIR * psRiga2 = ( DCOSMEZVIR * )oRigheOutput[ iIndex2 ];
                int iNumVir2 = 0;

                memset( szAppo, 0x00, 31 );
                memcpy( szAppo, psRiga2 -> NUMVIR, 5 );
                iNumVir2 = atoi( szAppo );

                if( iNumVir2 == iNumVir )
                {
                    int iPrgInizio = 0, iPrgFine = 0;
                    char acNumMezzo[ 11 ];

                    memset( acNumMezzo, 0x00, 11 );
                    memcpy( acNumMezzo, psRiga2 -> NUMMEZVGG, 10 );

                    if( ! memcmp( acNumMezzo, psRiga -> NUMMEZVGG, 10 ) )
                    {
                        iNumTrovati ++;

                        memset( szAppo, 0x00, 31 );
                        memcpy( szAppo, psRiga2 -> PRGINI, 3 );
                        iPrgInizio = atoi( szAppo );
                        memset( szAppo, 0x00, 31 );
                        memcpy( szAppo, psRiga2 -> PRGFIN, 3 );
                        iPrgFine = atoi( szAppo );

                        if( ( iPrgFine - iPrgInizio ) > iMaxDelta )
                        {
                            iIndexSave = iIndex2;
                            iMaxDelta = ( iPrgFine - iPrgInizio );
                        }
                    }
                }
            }

            // Se ho trovato una sola volta il mezzo vgg nella famiglia oppure
            // se il mezzo vgg che ha maggiore percorrenza nella famiglia Š
            if( iNumTrovati == 1 )
            {
                psRiga -> INDPRCITA = '1';
            }
            else if( iNumTrovati > 1 )
            {
                if( iIndexSave == iIndex )
                {
                    psRiga -> INDPRCITA = '1';
                }
                else
                {
                    psRiga -> INDPRCITA = '0';
                }
            }
            else
            {
                printf( "ERRORE!\n" );
            }

            if( OkPrintf ) printf( "%05d\b\b\b\b\b", iIndex );
        }
        printf( "terminata.\n" );

        // Ora scrivo il file in output.
        printf( "Inizio scrittura su disco del file 'DCOSMEZV.DAT'... " );
        ORD_FORALL( oRigheOutput, iIndexOutput )
        {
            struct DCOSMEZVIR * psOutput = ( DCOSMEZVIR * )oRigheOutput[ iIndexOutput ];
            poDCOSMEZVIR -> AddRecordToEnd( BUFR( psOutput, sizeof( DCOSMEZVIR ) ) );
        }
        printf( "\nterminata." );

        fclose( fp );

        delete poMezziViaggianti;
        delete poDCOSMEZVIR;
        delete poMezzoVirtuale;
        delete poFermateVirtuale;
    }

    fclose( fp1 );

    printf( "\nFine elaborazione DCOSMEZV\n" );

    return iRet;
}

/* -------------------------------------------------------------------------- */
/*  Scrivi_DCOSMEZV                                                           */
/* -------------------------------------------------------------------------- */
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
                     ULONG ulCurrRec )
{
    #undef TRCRTN
    #define TRCRTN "Scrivi_DCOSMEZV"

    int iRet = 0;
    static ULONG ulRecordScritti;
    struct MEZZO_VIRTUALE sMezzoVirtuale;
    int iMezzoDaMotore = 0;

    /* Ricerco il record del Mezzo Virtuale */
    if( ! poMezzoVirtuale -> Seek( iTrenoVirtuale ) )
    {
        BEEP;
        printf( "Non Š stato trovato il Mezzo Virtuale '%d'\n", iTrenoVirtuale );
        iRet = 1;
    }
    else
    {
        memset( psDCOSMEZVIR, ' ', sizeof( DCOSMEZVIR ) );

        sMezzoVirtuale = poMezzoVirtuale -> RecordCorrente();

        if( ! DecodificaMezzoVirtuale( iTrenoVirtuale,
                                       roElencoRighe,
                                       psDCOSMEZVIR -> NUMVIR,
                                       psDCOSMEZVIR -> PRGVIR ) )
        {
            /* Estraggo il numero del Mezzo Viaggiante dalla struttura */
            /* del mezzo virtuale, prendendo quello con indice */
            /* psFermateVirtuale -> TrenoFisico. */
            CHAR szAppo[ 16 ];

            // Aggiorno 'iTrenoVirtuale' col nuovo valore dalle famiglie.
            iMezzoDaMotore = iTrenoVirtuale;
            memset( szAppo, 0x00, sizeof( szAppo ) );
            memcpy( szAppo, psDCOSMEZVIR -> NUMVIR, 5 );
            iTrenoVirtuale = atoi( szAppo );

            memset( szAppo, 0x00, 16 );
            memcpy( szAppo, sMezzoVirtuale.Mv[ iTrenoFisico ].IdentTreno, 10 );
            memcpy( psDCOSMEZVIR -> NUMMEZVGG, szAppo, 10 );

            /* Valorizzo il Codice Rete ed il Codice Gestore Servizio. */
            memcpy( psDCOSMEZVIR -> CODRET, "83", 2 );
            memcpy( psDCOSMEZVIR -> CODGSRSER, "083", 3 );

            /* Valorizzo il Codice Tipo Mezzo Viaggiante. */
            if ( sMezzoVirtuale.Mv[ iTrenoFisico ].TipoMezzo == MM_INFO :: TRAGHETTO ||
                 sMezzoVirtuale.Mv[ iTrenoFisico ].TipoMezzo == MM_INFO :: MONOCARENA ||
                 sMezzoVirtuale.Mv[ iTrenoFisico ].TipoMezzo == MM_INFO :: ALISCAFO )
            {
                DecodTipoMezzoViaggiante( psListeDecodifica,
                                          (STRINGA) "TRAGHETTO",
                                          psDCOSMEZVIR -> CODTIPMEZVGG);
            }
            else if (sMezzoVirtuale.Mv[ iTrenoFisico ].TipoMezzo == MM_INFO :: AUTOBUS )
            {
                DecodTipoMezzoViaggiante( psListeDecodifica,
                                          (STRINGA) "AUTOBUS",
                                          psDCOSMEZVIR -> CODTIPMEZVGG);
            }
            else
            {
                DecodTipoMezzoViaggiante( psListeDecodifica,
                                          (STRINGA) "TRENO",
                                          psDCOSMEZVIR -> CODTIPMEZVGG);
            }

            /* Valorizzo il Progressivo del Mezzo Viaggiante */
            /* all'interno del Virtuale                      */
            sprintf( szAppo, "%02d", iProgressivoMezzoViaggiante );
            memcpy( psDCOSMEZVIR -> PRGMEZVGG, szAppo, 2 );

            /* Valorizzo i progressivi delle stazioni inizio e fine */
            sprintf( szAppo, "%03d", iProgressivoStazioneInizio );
            memcpy( psDCOSMEZVIR -> PRGINI, szAppo, 3 );
            sprintf( szAppo, "%03d", iProgressivoStazioneFine );
            memcpy( psDCOSMEZVIR -> PRGFIN, szAppo, 3 );

            fprintf( fp1, "iTrenoVirtuale = %05d - iProgressivoStazioneFine = %03d\n", iTrenoVirtuale, iProgressivoStazioneFine );




            /* Valorizzo gli ultimi campi della struttura. */
            memcpy( psDCOSMEZVIR -> DATINIVAL, ( const char * )psListeDecodifica -> oDataInizio, 10 );
            memcpy( psDCOSMEZVIR -> DATFINVAL, ( const char * )psListeDecodifica -> oDataFine, 10 );

            memcpy( psDCOSMEZVIR -> CODDSZ, ( const char * )psListeDecodifica -> oCodiceDistribuzione, 8 );

            memcpy( psDCOSMEZVIR -> CR_LF, "\r\n",2 );

            /* *FAN* þþ */
            // 13/09/1996
            // Scrivo il file intermedio dei mezzi viaggianti (serve per DSEROFF).
            MEZZI_VIAGGIANTI sMezziViaggianti;

            memset( & sMezziViaggianti, 0x20, sizeof( MEZZI_VIAGGIANTI ) );
            sMezziViaggianti.MezzoViaggiante = sMezzoVirtuale.Mv[ iTrenoFisico ].NumeroMezzo;
            memcpy( sMezziViaggianti.IdentTreno, sMezzoVirtuale.Mv[ iTrenoFisico ].IdentTreno, 10 );
            sMezziViaggianti.MezzoVirtuale = iTrenoVirtuale;
            memset( szAppo, 0x00, sizeof( szAppo ) );
            memcpy( szAppo, psDCOSMEZVIR -> PRGVIR, 2 );
            sMezziViaggianti.ProgressivoVirtuale = atoi( szAppo );
            sprintf( sMezziViaggianti.Famiglia, "%05d%02d", sMezziViaggianti.MezzoVirtuale, sMezziViaggianti.ProgressivoVirtuale );
            sMezziViaggianti.PeriodicitaMezzoVirtuale = sMezzoVirtuale.PeriodicitaMV;
            sMezziViaggianti.PrgStazioneInizio = iProgressivoStazioneInizio;
            sMezziViaggianti.PrgStazioneFine = iProgressivoStazioneFine;
            sMezziViaggianti.iMezzoDaMotore = iMezzoDaMotore;

            fprintf( fp1, "iTrenoVirtuale = %05d - sMezziViaggianti.PrgStazioneFine = %03d\n\n", iTrenoVirtuale, sMezziViaggianti.PrgStazioneFine );

            poMezziViaggianti -> AddRecordToEnd( BUFR( & sMezziViaggianti, sizeof( MEZZI_VIAGGIANTI ) ) );
            /* *FAN* þþ */

            // Aggiungo la nuova riga all'elenco per l'output.
            roRigheOutput += ( void * ) psDCOSMEZVIR;

            /* Scrivo il record. */
//          poDCOSMEZVIR -> AddRecordToEnd( BUFR( psDCOSMEZVIR, sizeof( DCOSMEZVIR ) ) );
            if( OkPrintf ) printf( "Record letti: %06d\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", ulCurrRec+1, ++ ulRecordScritti );
        }
    }

    return iRet;
}

// Questa Š la funzione di comparazione tra due mezzi viaggianti, utilizzata
// durante il processo di riordinamento del file MEZZIVGG in base alla key.
int OrdinaMezziViaggianti( const void * m1, const void * m2 )
{
    int iRet = 0;

    MEZZI_VIAGGIANTI * poMezzo1 = ( MEZZI_VIAGGIANTI * )m1;
    MEZZI_VIAGGIANTI * poMezzo2 = ( MEZZI_VIAGGIANTI * )m2;

    return strcmp( poMezzo1 -> Famiglia, poMezzo2 -> Famiglia );
}
