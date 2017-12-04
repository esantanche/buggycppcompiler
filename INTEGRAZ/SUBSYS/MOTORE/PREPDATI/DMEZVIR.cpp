 //----------------------------------------------------------------------------
// FILE DMEZVIR.CPP
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include <DB_HOST.HPP>
#include <stdio.h>

extern int OkPrintf;

//----------------------------------------------------------------------------
// PROTOTIPI
//----------------------------------------------------------------------------
int CreaFamiglie( ELENCO & roElencoRighe );
int ScriviCache( ELENCO & roElencoRighe );
int LeggiCache( ELENCO & roElencoRighe );
int CaricaFermate( ELENCO & roElencoFermate );
int CaricaServizi( ELENCO & roElencoServizi );

// ---------------------------------------------------------------------------
// VARIABILI GLOBALI
// ---------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CreaFamiglie --------------------------------------------------------------
//----------------------------------------------------------------------------
int CreaFamiglie( ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "CreaFamiglie"

    int iRet = 0;
    ELENCO oElencoMezziFamiglia;
    MEZZO_VIRTUALE_APPO * psRecordMotore = NULL;
    int iCurrNumVirtuale = 1;
    int iCurrPrgVirtuale = 0;
    int iAggiuntoMezzoInFamiglia;
    int iAlmenoUnRecordTrattato = FALSE;
    int iRecordMarcati = 0;
    FILE * fp1 = fopen( TRC_PATH "famiglie.trc", "w" );

    do  // CICLO FINCHE' CI SONO RECORD NON MARCATI
    {   // (iAlmenoUnRecordTrattato)
        iAlmenoUnRecordTrattato = FALSE;

        do  // CICLO SE HO AGGIUNTO ALMENO DUE MEZZI ALLA FAMIGLIA
        {   // (iAggiuntoMezzoInFamiglia && oElencoMezziFamiglia.Dim() > 1)
            iAggiuntoMezzoInFamiglia = FALSE;

            // CICLO A CASCATA SUI RECORD
            for( int iContRighe = 0; iContRighe < roElencoRighe.Dim(); iContRighe ++ )
            {
                psRecordMotore = ( MEZZO_VIRTUALE_APPO * )roElencoRighe[ iContRighe ];

                if( ! psRecordMotore -> iMarked )
                {
                    iAlmenoUnRecordTrattato = TRUE;

                    // Se iCurrPrgVirtuale == 0 vuol dire che Š il PRIMISSIMO
                    // giro che sto facendo, oppure che ho terminato la
                    // famiglia precedente e ne devo creare una nuova.
                    if( iCurrPrgVirtuale == 0 ) // FAMIGLIA NON ANCORA POPOLATA
                    {
                        if( OkPrintf ) printf( "\nComincio la famiglia %05d dal record %05d...\n", iCurrNumVirtuale, iContRighe );
                        fprintf( fp1, "\nComincio la famiglia %05d dal record %05d...\n", iCurrNumVirtuale, iContRighe );

                        // I mezzi reali del primo record o del primo record
                        // non marcato (se in giro successivo) formano subito
                        // una famiglia.
                        psRecordMotore = ( MEZZO_VIRTUALE_APPO * )roElencoRighe[ iContRighe ];
                        for( int iIndexReali = 0; iIndexReali < psRecordMotore -> sMezzoVirtuale.NumMezziComponenti; iIndexReali ++ )
                        {
                            MEZZO_VIRTUALE :: MEZZO_VIAGGIANTE * psMezzoViaggiante;
                            psMezzoViaggiante = &( psRecordMotore -> sMezzoVirtuale.Mv[ iIndexReali ] );

                            oElencoMezziFamiglia += ( void * )psMezzoViaggiante;
                            iAggiuntoMezzoInFamiglia = TRUE;
                        }
                        // ** CREO **  LA NUOVA FAMIGLIA
                        psRecordMotore -> iNumeroVirtuale = iCurrNumVirtuale;
                        psRecordMotore -> iPrgVirtuale = ++ iCurrPrgVirtuale;
                        psRecordMotore -> iMarked = 1;
                        iRecordMarcati ++;
                    }
                    else
                    {
                        int * Posizioni = new int[ psRecordMotore -> sMezzoVirtuale.NumMezziComponenti ];
                        memset( Posizioni, 0x00, ( psRecordMotore -> sMezzoVirtuale.NumMezziComponenti ) * sizeof( int ) );

                        int iTrovato = FALSE;

                        // MEZZI REALI 1ø ciclo
                        for( int iIndexReali = 0; iIndexReali < psRecordMotore -> sMezzoVirtuale.NumMezziComponenti; iIndexReali ++ )
                        {
                            MEZZO_VIRTUALE :: MEZZO_VIAGGIANTE * psMezzoViaggiante;
                            psMezzoViaggiante = &( psRecordMotore -> sMezzoVirtuale.Mv[ iIndexReali ] );

                            // FAMIGLIA
                            ORD_FORALL( oElencoMezziFamiglia, iContElenco )
                            {
                                MEZZO_VIRTUALE :: MEZZO_VIAGGIANTE * psMezzoInElenco;
                                psMezzoInElenco = ( MEZZO_VIRTUALE :: MEZZO_VIAGGIANTE * )oElencoMezziFamiglia[ iContElenco ];

                                if( ( psMezzoInElenco -> NumeroMezzo == psMezzoViaggiante -> NumeroMezzo )
                                    &&
                                    ( psMezzoInElenco -> TipoMezzo == psMezzoViaggiante -> TipoMezzo ) )
                                {
                                    Posizioni[ iIndexReali ] = 1;
                                    iTrovato = TRUE;
                                    break;  // ESCO DAL CICLO SULL'ELENCO FAMIGLIA
                                }
                            }
                        }

                        // almeno un mezzo reale Š stato trovato quindi aggiungo quelli
                        // nuovi
                        if( iTrovato )
                        {
                            psRecordMotore -> iNumeroVirtuale = iCurrNumVirtuale;
                            psRecordMotore -> iPrgVirtuale = ++ iCurrPrgVirtuale;
                            psRecordMotore -> iMarked = 1;
                            iRecordMarcati ++;

                            // MEZZI REALI 2ø ciclo
                            for( int iIndexReali = 0; iIndexReali < psRecordMotore -> sMezzoVirtuale.NumMezziComponenti; iIndexReali ++ )
                            {
                                MEZZO_VIRTUALE :: MEZZO_VIAGGIANTE * psMezzoViaggiante;
                                psMezzoViaggiante = &( psRecordMotore -> sMezzoVirtuale.Mv[ iIndexReali ] );

                                if( Posizioni[ iIndexReali ] == 0 )
                                {
                                    oElencoMezziFamiglia += ( void * )psMezzoViaggiante;
                                    iAggiuntoMezzoInFamiglia = TRUE;
                                }
                            }
                        }

                        delete Posizioni;
                    }
                }
            }
        } while( oElencoMezziFamiglia.Dim() > 1 && iAggiuntoMezzoInFamiglia );

        if( OkPrintf ) printf( "Terminata la famiglia %05d (Prgg: 1-%d) - Record da elab.: %05d.\n", iCurrNumVirtuale, iCurrPrgVirtuale, roElencoRighe.Dim() - iRecordMarcati );
        fprintf( fp1, "Terminata la famiglia %05d (Prgg: 1-%d) - Record da elab.: %05d; Nø Mezzi Reali: %03d.\n", iCurrNumVirtuale, iCurrPrgVirtuale, roElencoRighe.Dim() - iRecordMarcati, oElencoMezziFamiglia.Dim() );
        if( OkPrintf ) printf( "Mezzi in famiglia: " );
        fprintf( fp1, "Mezzi in famiglia: " );
        ORD_FORALL( oElencoMezziFamiglia, iContMezzi )
        {
            MEZZO_VIRTUALE :: MEZZO_VIAGGIANTE * psMezzoInElenco;
            psMezzoInElenco = ( MEZZO_VIRTUALE :: MEZZO_VIAGGIANTE * )oElencoMezziFamiglia[ iContMezzi ];

            if( OkPrintf ) printf( "%05d ", psMezzoInElenco -> NumeroMezzo );
            fprintf( fp1, "%05d ", psMezzoInElenco -> NumeroMezzo );
        }
        if( OkPrintf ) printf( "\n" );
        fprintf( fp1, "\n" );

        iCurrNumVirtuale ++;
        iCurrPrgVirtuale = 0;
        oElencoMezziFamiglia.Clear();

    } while( iAlmenoUnRecordTrattato );

    fclose( fp1 );

    return iRet;
}

//----------------------------------------------------------------------------
// LeggiCache ----------------------------------------------------------------
//----------------------------------------------------------------------------
int LeggiCache( ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "LeggiCache"

    int iRet = 0;

    printf( "\nLeggo l'elenco dei mezzi virtuali dal file di cache...\n" );
    FILE_MEZZO_VIRTUALE_APPO * poMEZZO_VIRTUALE_APPO = new FILE_MEZZO_VIRTUALE_APPO( PATH_OUT "DMEZVIR.CAC" );
    MEZZO_VIRTUALE_APPO * poStruct;
    ULONG ulNumRecordsCache = poMEZZO_VIRTUALE_APPO -> NumRecordsTotali();
    ULONG ulCurrRec = 0L;

    roElencoRighe.Clear();

    printf( "Numero mezzi virtuali contenuti nel file di cache: %ld\n", ulNumRecordsCache );

    printf( "Inizio lettura cache..." );
    for( ulCurrRec = 0L; ulCurrRec < ulNumRecordsCache; )
    {
        poStruct = new MEZZO_VIRTUALE_APPO;
        * poStruct = poMEZZO_VIRTUALE_APPO -> operator[]( ulCurrRec );
        roElencoRighe += ( void * ) poStruct;
        ulCurrRec ++;
    }
    delete poMEZZO_VIRTUALE_APPO;
    printf( "Fatto (letti %ld records).\n", ( long )roElencoRighe.Dim() );

    return iRet;
}

//----------------------------------------------------------------------------
// ScriviCache ---------------------------------------------------------------
//----------------------------------------------------------------------------
int ScriviCache( ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "ScriviCache"

    int iRet = 0;

    printf( "----------\nScrivo su disco l'elenco calcolato (cache)...\n" );
    FILE_MEZZO_VIRTUALE_APPO * poMEZZO_VIRTUALE_APPO = new FILE_MEZZO_VIRTUALE_APPO( PATH_OUT "DMEZVIR.CAC" );
    poMEZZO_VIRTUALE_APPO -> Clear( NUSTR, FALSE );
    if ( ! poMEZZO_VIRTUALE_APPO -> FileHandle() )
    {
        printf( "Errore nella creazione del file '%s'!\n", PATH_OUT "DMEZVIR.CAC" );
        iRet = 1;
        BEEP;
    }
    else
    {
        FILE * fp = fopen( TRC_PATH "mezzivir.trc", "w" );
        ORD_FORALL( roElencoRighe, iContRighe )
        {
            MEZZO_VIRTUALE_APPO * poStruct = ( MEZZO_VIRTUALE_APPO * )roElencoRighe[ iContRighe ];
            poMEZZO_VIRTUALE_APPO -> AddRecordToEnd( BUFR( poStruct, sizeof( MEZZO_VIRTUALE_APPO ) ) );
            fprintf( fp, "NumVir: %05d - PrgVir: %02d - Mezzi Reali: ", poStruct -> iNumeroVirtuale, poStruct -> iPrgVirtuale );
            for( int iIndexReali = 0; iIndexReali < poStruct -> sMezzoVirtuale.NumMezziComponenti; iIndexReali ++ )
            {
                char acKeyTreno[ 6 ];

                memset( acKeyTreno, 0x00, 6 );
                memcpy( acKeyTreno, poStruct -> sMezzoVirtuale.Mv[ iIndexReali ].KeyTreno, 5 );
                fprintf( fp, "%s ", acKeyTreno );
            }
            fprintf( fp, "\n" );
            if( OkPrintf ) printf( "%06d...\b\b\b\b\b\b\b\b\b", iContRighe );
        }
        fclose( fp );
    }
    delete poMEZZO_VIRTUALE_APPO;
    printf( "\n...Fatto.\n" );

    return iRet;
}

//----------------------------------------------------------------------------
// Elabora_DMEZVIR -----------------------------------------------------------
//----------------------------------------------------------------------------
int Elabora_DMEZVIR(LISTE_DECODIFICA * psListeDecodifica, ELENCO & roElencoRighe, int iCreaFamiglie )
{
    #undef TRCRTN
    #define TRCRTN "Elabora_DMEZVIR"

    int iRet = 0;

    struct SERVIZI_TRENO sServiziTreno;

    MEZZO_VIRTUALE_APPO * psAppoVirtuale;
    MEZZO_VIRTUALE_APPO * psAppoVirtuale1;
    MEZZO_VIRTUALE_APPO * psAppoVirtuale2;
    F_MEZZO_VIRTUALE * poMezzoVirtuale = NULL;
    FILE_DMEZVIR * poDMEZVIR = NULL;
    FILE * fp = NULL;
    FILE * fp2 = NULL;
    ELENCO oElencoFermate( 0, 240000 );
    ELENCO oElencoServizi;

    struct DMEZVIR sDMEZVIR;

    if( iCreaFamiglie )
    {
        poMezzoVirtuale = new F_MEZZO_VIRTUALE(PATH_IN "M0_TRENV.TM1");
        poDMEZVIR = new FILE_DMEZVIR(PATH_OUT "DMEZVIR.DAT");
        poDMEZVIR->Clear(NUSTR, FALSE);
    }

    if( iCreaFamiglie && ( ! poMezzoVirtuale -> FileHandle() ||
        poMezzoVirtuale -> NumRecordsTotali() == 0 ) )
    {
        iRet = 1;
        BEEP;
        printf("\nErrore: file "PATH_IN "M0_TRENV.TM1 non trovato o inutilizzabile");
    }
    else if( iCreaFamiglie && ! poDMEZVIR -> FileHandle() )
    {
        iRet = 1;
        BEEP;
        printf("\nErrore: impossibile creare il file "PATH_OUT "DMEZVIR.DAT");
    }
    else
    {
        if( iCreaFamiglie )
        {
            printf("\nInizio elaborazione tabella DMEZVIR ...\n");
            TRACESTRING ("Inizio elaborazione tabella DMEZVIR ...");

            CaricaFermate( oElencoFermate );
            CaricaServizi( oElencoServizi );

            ULONG ulNum = poMezzoVirtuale->NumRecordsTotali();
            ULONG ulNumRecord = 0;
            int iTrovato = 0;

            if( OkPrintf ) printf("\nNumero mezzi virtuali letti: %05d", ulNum);

            fp = fopen( TRC_PATH "m0_trenv.trc", "w" );
            fp2 = fopen( TRC_PATH "nofermat.trc", "w" );

            int iRecordScartati = 0;
            fprintf( fp2, "ELENCO MEZZI VIRTUALI SCARTATI PER FERMATE VALIDE < 2\n"
                          "-----------------------------------------------------\n" );

            for( ; ulNumRecord < ulNum; ulNumRecord ++ )
            {
                psAppoVirtuale = new MEZZO_VIRTUALE_APPO;
                psAppoVirtuale->sMezzoVirtuale = poMezzoVirtuale->FixRec(ulNumRecord);
                if( psAppoVirtuale->sMezzoVirtuale.NumeroFermateValide > 1 )
                {
                    psAppoVirtuale->iMarked = 0;
                    psAppoVirtuale->iNumeroVirtuale = 0;
                    psAppoVirtuale->iPrgVirtuale = 0;

                    fprintf( fp, "NumRec: %05d - Mezzi: ", ulNumRecord );
                    for( int iIndex = 0; iIndex < psAppoVirtuale -> sMezzoVirtuale.NumMezziComponenti; iIndex ++ )
                    {
                        char acKeyTreno[6];

                        memset(acKeyTreno, 0x00, 6);
                        memcpy(acKeyTreno, psAppoVirtuale -> sMezzoVirtuale.Mv[ iIndex ].KeyTreno, 5);

                        fprintf( fp, "%05d(%s) ", psAppoVirtuale -> sMezzoVirtuale.Mv[ iIndex ].NumeroMezzo, acKeyTreno );
                    }
                    fprintf( fp, "\n" );

                    roElencoRighe += (void *) psAppoVirtuale;
                }
                else
                {
                    fprintf( fp2, "NumRec=%04d, MezzoVirtuale=%05d\n", ulNumRecord, psAppoVirtuale->sMezzoVirtuale.MezzoVirtuale );
                    iRecordScartati ++;
                    delete psAppoVirtuale;
                }
            }

            printf( "\nTotale record scartati per insufficienza di fermate: %03d\n", iRecordScartati );
            fclose( fp2 );
            fclose( fp );

            // int iNumElenco = roElencoRighe.Dim() -1;
            printf("\nInizio attribuzione Numero Virtuale e Progressivo Virtuale\n");
        }

        // --------------------------------------------------------------------
        // INIZIO SEZIONE -> Scommentare per creare il file .CAC
        // --------------------------------------------------------------------
        if( iCreaFamiglie )
        {
            iRet = CreaFamiglie( roElencoRighe );
        }
        // --------------------------------------------------------------------
        // FINE SEZIONE
        // --------------------------------------------------------------------

        // --------------------------------------------------------------------
        // INIZIO SEZIONE -> Scommentare se il file .CAC Š valido.
        // --------------------------------------------------------------------
        if( ! iCreaFamiglie )
        {
            iRet = LeggiCache( roElencoRighe );
        }
        // --------------------------------------------------------------------
        // FINE SEZIONE
        // --------------------------------------------------------------------

        if( iCreaFamiglie )
        {
            printf("\nAttribuzione Numero Virtuale e Progressivo Virtuale terminata\n");

            printf("\nInizio scrittura ...\n");
            fp = fopen( TRC_PATH "dmezvir.trc", "w" );

            for( int iContDel = 0; iContDel < roElencoRighe.Dim(); iContDel ++ )
            {
                char szAppo[ 11 ];

                psAppoVirtuale = (MEZZO_VIRTUALE_APPO *) roElencoRighe[iContDel];

                if( psAppoVirtuale->sMezzoVirtuale.NumeroFermateValide > 1 )
                {
                    memset( & sDMEZVIR, ' ', sizeof( sDMEZVIR ) );

                    // leggo le fermate
                    iRet = LeggiFermate(fp,
                                        oElencoFermate,
                                        psAppoVirtuale,
                                        sDMEZVIR.CODSTAINI,
                                        sDMEZVIR.CODSTAFIN);

                    if( ! iRet )
                    {

                        sprintf( szAppo, "%05d", psAppoVirtuale->iNumeroVirtuale );
                        memcpy( sDMEZVIR.NUMVIR, szAppo, strlen( szAppo ) );

                        sprintf( szAppo, "%02d", psAppoVirtuale->iPrgVirtuale );
                        memcpy( sDMEZVIR.PRGVIR, szAppo, strlen( szAppo ) );

                        memcpy(sDMEZVIR.DATINIVAL, (const char *) psListeDecodifica->oDataInizio, 10);
                        memcpy(sDMEZVIR.DATFINVAL, (const char *) psListeDecodifica->oDataFine, 10);

                        if (psAppoVirtuale->sMezzoVirtuale.TipoMezzo == sServiziTreno.Servizi.TRAGHETTO ||
                            psAppoVirtuale->sMezzoVirtuale.TipoMezzo == sServiziTreno.Servizi.MONOCARENA||
                            psAppoVirtuale->sMezzoVirtuale.TipoMezzo == sServiziTreno.Servizi.ALISCAFO)
                        {
                            DecodTipoMezzoViaggiante( psListeDecodifica,
                                                      (STRINGA) "TRAGHETTO",
                                                      sDMEZVIR.CODTIPMEZVGG);
                        }
                        else if (psAppoVirtuale->sMezzoVirtuale.TipoMezzo == sServiziTreno.Servizi.AUTOBUS)
                        {
                            DecodTipoMezzoViaggiante( psListeDecodifica,
                                                      (STRINGA) "AUTOBUS",
                                                      sDMEZVIR.CODTIPMEZVGG);
                        }
                        else
                        {
                            DecodTipoMezzoViaggiante( psListeDecodifica,
                                                      (STRINGA) "TRENO",
                                                      sDMEZVIR.CODTIPMEZVGG);
                        }

                        sprintf( szAppo, "%02d", psAppoVirtuale->sMezzoVirtuale.TipoMezzo );
                        memcpy( sDMEZVIR.CODCLS, szAppo, 2 );

                        STRINGA oIdTreno;
                        int iPrimaClasse = 0;
                        int iSecondaClasse = 0;
                        int iClasseDisuniforme = 0;

                        for (int iIndex = 0; iIndex < psAppoVirtuale->sMezzoVirtuale.NumMezziComponenti; iIndex++)
                        {
                            // inizio esame classe
                            oIdTreno = St((psAppoVirtuale->sMezzoVirtuale.Mv[iIndex]).IdentTreno);
                            oIdTreno.Strip();

                            iRet = LeggiMezzoViaggiante (oElencoServizi,
                                                         oIdTreno,
                                                         &sServiziTreno);

                            if (! iRet)
                            {
                                if (iIndex == 0)
                                {
                                    if (sServiziTreno.Servizi.PostiASederePrima ||
                                        sServiziTreno.Servizi.SleeperettePrima ||
                                        sServiziTreno.Servizi.CuccettePrima ||
                                        sServiziTreno.Servizi.VagoniLettoPrima)
                                    {
                                        iPrimaClasse = 1;
                                    }

                                    if (sServiziTreno.Servizi.PostiASedereSeconda ||
                                        sServiziTreno.Servizi.SleeperetteSeconda ||
                                        sServiziTreno.Servizi.CuccetteSeconda ||
                                        sServiziTreno.Servizi.VagoniLettoSeconda)
                                    {
                                        iSecondaClasse = 1;
                                    }
                                }
                                else
                                {
                                    if ( (sServiziTreno.Servizi.PostiASederePrima ||
                                          sServiziTreno.Servizi.SleeperettePrima ||
                                          sServiziTreno.Servizi.CuccettePrima ||
                                          sServiziTreno.Servizi.VagoniLettoPrima) &&
                                          iPrimaClasse == 0)
                                    {
                                        iClasseDisuniforme = 1;
                                    }

                                    if ( (sServiziTreno.Servizi.PostiASedereSeconda ||
                                          sServiziTreno.Servizi.SleeperetteSeconda ||
                                          sServiziTreno.Servizi.CuccetteSeconda ||
                                          sServiziTreno.Servizi.VagoniLettoSeconda) &&
                                          iSecondaClasse == 0)
                                    {
                                        iClasseDisuniforme = 1;
                                    }
                                }
                            }

                            // esamino se ci sono cambi di classifica
                            if ((psAppoVirtuale->sMezzoVirtuale.Mv[iIndex]).TipoMezzo != 0)
                            {
                                if( (psAppoVirtuale->sMezzoVirtuale.Mv[iIndex]).TipoMezzo !=
                                     psAppoVirtuale->sMezzoVirtuale.TipoMezzo )
                                {
                                    sDMEZVIR.Q_M_1 = '?';
                                    memcpy(sDMEZVIR.CODCLS, "  ", 2);
                                    break;
                                }
                            }
                        }

                        if (iClasseDisuniforme == 0)
                        {
                            if (iPrimaClasse &&
                                ! iSecondaClasse)
                            {
                                DecodClasse( psListeDecodifica,
                                             (STRINGA) "PRIMA_CLASSE",
                                             sDMEZVIR.CLA);
                            }
                            else if (! iPrimaClasse &&
                                iSecondaClasse)
                            {
                                DecodClasse( psListeDecodifica,
                                             (STRINGA) "SECONDA_CLASSE",
                                             sDMEZVIR.CLA);
                            }
                            else
                            {
                                DecodClasse( psListeDecodifica,
                                             (STRINGA) "PRIMA_SECONDA_CLASSE",
                                             sDMEZVIR.CLA);
                            }
                        }
                        else
                        {
                            sDMEZVIR.CLA = ' ';
                            sDMEZVIR.Q_M_2 = '?';
                        }


                        // scrivo il record DMEZVIR
                        memcpy(sDMEZVIR.PRAOCC, sDMEZVIR.PRGVIR, 2);
                        sDMEZVIR.CNG = '1';
                        memcpy(sDMEZVIR.CODDSZ, (const char *) psListeDecodifica->oCodiceDistribuzione, 8);
                        memcpy(sDMEZVIR.CR_LF, "\r\n",2);

                        iRet = ScriviDMEZVIR(poDMEZVIR, &sDMEZVIR);

                        if( OkPrintf ) printf( "%06d...\b\b\b\b\b\b\b\b\b", iContDel );
                    }
                    else
                    {
                        // Elimino da roElencoRighe i mezzi virtuali scartati
                        // in quanto non sono state trovate le corrispondenti
                        // fermate di partenza e/o arrivo.
                        roElencoRighe -= ( void * )psAppoVirtuale;
                        iContDel --;
                    }
                }
                else
                {
                    fprintf( fp, "Record scartato (fermate non sufficienti): NumVir=%05d, PrgVir=%02d\n", psAppoVirtuale -> iNumeroVirtuale, psAppoVirtuale -> iPrgVirtuale );
                }
            }
            fclose( fp );
        }

        // --------------------------------------------------------------------
        // INIZIO SEZIONE -> Scommentare per creare il file .CAC
        // --------------------------------------------------------------------
        if( iCreaFamiglie )
        {
            iRet = ScriviCache( roElencoRighe );

            // Disallocazione degli elenchi.
            if( OkPrintf ) printf( "Scaricamento elenchi... " );
            ORD_FORALL( oElencoFermate, iContDelFermate )
            {
                struct FERMATE_VIRT * psFermataVirtuale = ( FERMATE_VIRT * )oElencoFermate[ iContDelFermate ];
                delete psFermataVirtuale;

            }
            ORD_FORALL( oElencoServizi, iContDelServizi )
            {
                struct SERVIZI_TRENO * psServiziTreno = ( SERVIZI_TRENO * )oElencoServizi[ iContDelServizi ];
                delete psServiziTreno;
            }
            if( OkPrintf ) printf( "terminata.\n" );

            delete poDMEZVIR;
            delete poMezzoVirtuale;

            printf("Fine elaborazione tabella DMEZVIR.\n");
            TRACESTRING("... Fine elaborazione tabella DMEZVIR");
        }
        // --------------------------------------------------------------------
        // FINE SEZIONE
        // --------------------------------------------------------------------
    }

    return iRet;
}

//----------------------------------------------------------------------------
// LeggiMezzoViaggiante ------------------------------------------------------
//----------------------------------------------------------------------------
int LeggiMezzoViaggiante( ELENCO & roElencoServizi,
                          STRINGA & roTreno,
                          SERVIZI_TRENO * psServiziTreno)
{
    #undef TRCRTN
    #define TRCRTN "LeggiMezzoViaggiante"

    int iRet = 1;
    STRINGA oIdTrenoServizi = NUSTR;

    ORD_FORALL( roElencoServizi, ulNumRecord )
    {
        psServiziTreno = ( SERVIZI_TRENO * )roElencoServizi[ ulNumRecord ];

        oIdTrenoServizi = St( psServiziTreno -> IdentTreno );
        oIdTrenoServizi.Strip();

        if( oIdTrenoServizi == roTreno )
        {
            iRet = 0;
            break;
        }
    }

    return iRet;
}

//----------------------------------------------------------------------------
// LeggiFermate --------------------------------------------------------------
//----------------------------------------------------------------------------
int LeggiFermate( FILE * fp,
                  ELENCO & roElencoFermate,
                  MEZZO_VIRTUALE_APPO * psMezzoVirtuale,
                  char * CODSTAINI,
                  char * CODSTAFIN )
{
    #undef TRCRTN
    #define TRCRTN "LeggiFermate"

    int iRet = 0;
    struct FERMATE_VIRT * psFermateVirtuale = NULL;
    BOOL fTrovataPartenza = FALSE, fTrovatoArrivo = FALSE;
    char szAppo[ 6 ];
    long lLastCCR = 0L;

    ORD_FORALL( roElencoFermate, iIndex )
    {
        psFermateVirtuale = ( FERMATE_VIRT * )roElencoFermate[ iIndex ];

        if( psFermateVirtuale -> MezzoVirtuale == psMezzoVirtuale -> sMezzoVirtuale.MezzoVirtuale )
        {
            if( ! fTrovataPartenza )
            {
                fTrovataPartenza = TRUE;
                sprintf( szAppo, "%05ld", psFermateVirtuale -> CCR );
                memcpy( CODSTAINI, szAppo, strlen( szAppo ) );
            }
            lLastCCR = psFermateVirtuale -> CCR;
        }
        else
        {
            if( fTrovataPartenza )
            {
                fTrovatoArrivo = TRUE;
                sprintf( szAppo, "%05ld", lLastCCR );
                memcpy( CODSTAFIN, szAppo, strlen( szAppo ) );
            }
        }
        if( fTrovataPartenza && fTrovatoArrivo )
        {
            break;
        }
    }

//  ORD_FORALL( roElencoFermate, iIndex )
//  {
//      psFermateVirtuale = ( struct FERMATE_VIRT * )roElencoFermate[ iIndex ];
//
//      if( psFermateVirtuale -> MezzoVirtuale == psMezzoVirtuale -> sMezzoVirtuale.MezzoVirtuale )
//      {
//          char szAppo[ 6 ];
//
//          if ( psFermateVirtuale -> FermataPartenza && ! psFermateVirtuale -> FermataArrivo )
//          {
//              sprintf( szAppo, "%05ld", psFermateVirtuale -> CCR );
//              memcpy( CODSTAINI, szAppo, strlen( szAppo ) );
//              fTrovataPartenza = TRUE;
//          }
//
//          if ( psFermateVirtuale -> FermataArrivo && ! psFermateVirtuale -> FermataPartenza )
//          {
//              sprintf( szAppo, "%05ld", psFermateVirtuale -> CCR );
//              memcpy( CODSTAFIN, szAppo, strlen( szAppo ) );
//              fTrovatoArrivo = TRUE;
//          }
//
//          if( fTrovataPartenza && fTrovatoArrivo )
//          {
//              break;
//          }
//      }
//  }

    if( ! fTrovataPartenza || ! fTrovatoArrivo )
    {
        printf( "ATTENZIONE: non Š stata trovata la stazione " );
        fprintf( fp, "ATTENZIONE: non Š stata trovata la stazione " );
        if( ! fTrovataPartenza )
        {
            printf( "PARTENZA" );
            fprintf( fp, "PARTENZA" );
            if( ! fTrovatoArrivo )
            {
                printf( "/ARRIVO" );
                fprintf( fp, "/ARRIVO" );
            }
            else
            {
                printf( " " );
                fprintf( fp, " " );
            }
        }
        if( ! fTrovatoArrivo )
        {
            printf( "ARRIVO" );
            fprintf( fp, "ARRIVO" );
            if( ! fTrovataPartenza )
            {
                printf( "/PARTENZA" );
                fprintf( fp, "/PARTENZA" );
            }
            else
            {
                printf( " " );
                fprintf( fp, " " );
            }
        }
        printf( "per il mezzo %05d Prg:%02d !\n", psMezzoVirtuale -> iNumeroVirtuale, psMezzoVirtuale -> iPrgVirtuale );
        fprintf( fp, "per il mezzo %05d Prg:%02d !\n", psMezzoVirtuale -> iNumeroVirtuale, psMezzoVirtuale -> iPrgVirtuale );
        iRet = 1;
    }

    return iRet;
}


//----------------------------------------------------------------------------
// ScriviDMEZVIR -------------------------------------------------------------
//----------------------------------------------------------------------------
int ScriviDMEZVIR(FILE_DMEZVIR * poDMEZVIR,
                  DMEZVIR * psDMEZVIR)
{
    #undef TRCRTN
    #define TRCRTN "ScriviDMEZVIR"

    int iRet = 0;

//  iNumeroRigheScritte++;

    poDMEZVIR->AddRecordToEnd(BUFR(psDMEZVIR,sizeof(*psDMEZVIR)));

    // Modificato per ridurre i tempi di scrittura a video
//  if(iNumeroRigheScritte % 100 == 0)
//  {
//     if( OkPrintf ) printf("Numero mezzi virtuali scritti: %05d\r", iNumeroRigheScritte);
//  }

    return iRet;
}

int CaricaFermate( ELENCO & roElencoFermate )
{
    #undef TRCRTN
    #define TRCRTN "CaricaFermate"

    int iRet = 0;

    printf( "Caricamento fermate... " );
    F_FERMATE_VIRT * poFermateVirtuale = new F_FERMATE_VIRT( PATH_IN "M0_FERMV.TM1" );
    ULONG ulNumFermate = poFermateVirtuale -> NumRecordsTotali();
    struct FERMATE_VIRT * psFermataVirtuale = NULL;

    for( int iIndex = 0; iIndex < ulNumFermate; iIndex ++ )
    {
        struct FERMATE_VIRT sFermata;

        sFermata = poFermateVirtuale -> FixRec( iIndex );

        psFermataVirtuale = new FERMATE_VIRT;
        memcpy( psFermataVirtuale, & sFermata, sizeof( FERMATE_VIRT ) );
        roElencoFermate += ( void * )psFermataVirtuale;
    }
    delete poFermateVirtuale;
    printf( "terminato (%ld record).\n", -- iIndex );

    return iRet;
}

int CaricaServizi( ELENCO & roElencoServizi )
{
    #undef TRCRTN
    #define TRCRTN "CaricaServizi"

    int iRet = 0;

    printf( "Caricamento servizi... " );
    FILE_SERVIZI_TRENO * poServizi = new FILE_SERVIZI_TRENO(PATH_IN "S_TRENO.TMP");
    ULONG ulNumServizi = poServizi -> NumRecordsTotali();
    struct SERVIZI_TRENO * psServiziTreno = NULL;

    for( int iIndex = 0; iIndex < ulNumServizi; iIndex ++ )
    {
        struct SERVIZI_TRENO sServizio;

        sServizio = ( * poServizi )[ iIndex ];

        psServiziTreno = new SERVIZI_TRENO ;
        memcpy( psServiziTreno , & sServizio, sizeof( SERVIZI_TRENO ) );
        roElencoServizi += ( void * )psServiziTreno;
    }
    delete poServizi ;
    printf( "terminato (%ld record).\n", -- iIndex );

    return iRet;
}
