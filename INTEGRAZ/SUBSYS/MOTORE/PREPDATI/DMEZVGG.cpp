 //----------------------------------------------------------------------------
// FILE DMEZVGG.CPP
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include <DB_HOST.HPP>
#include <TABELLE.HPP>

extern int OkPrintf;

//----------------------------------------------------------------------------
// PROTOTIPI
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Elabora_DMEZVGG -----------------------------------------------------------
//----------------------------------------------------------------------------
int Elabora_DMEZVGG ( LISTE_DECODIFICA * psListeDecodifica, ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "Elabora_DMEZVGG"

    int iRet = 0;
    int iNumMezziScritti = 0;

    struct DMEZVGG sDMEZVGG;

    FILE_SERVIZI_TRENO * poServizi = NULL;
    FILE_DMEZVGG * poDMEZVGG = NULL;

    printf( "\nInizio elaborazione tabella DMEZVGG...\n" );

    // Leggo tutti i mezzi virtuali: estraggo quelli viaggianti e scarto i
    // duplicati. Tengo tutto in memoria in ®oElencoMezzi¯;
    struct MEZZO_VIRTUALE sMezzoVirtuale;
    F_MEZZO_VIRTUALE * poMezzoVirtuale = new F_MEZZO_VIRTUALE(PATH_IN "M0_TRENV.TM1");
    if( ! poMezzoVirtuale -> FileHandle() )
    {
        BEEP;
        printf( "\nErrore nell'apertura del file '%s'\n", PATH_IN "M0_TRENV.TM1" );
        iRet = 1;
    }

    if( ! iRet )
    {
        poDMEZVGG = new FILE_DMEZVGG(PATH_OUT "DMEZVGG.DAT");
        poDMEZVGG->Clear(NUSTR, FALSE);
        if( ! poDMEZVGG -> FileHandle() )
        {
            BEEP;
            printf( "\nErrore nell'apertura del file '%s'\n", PATH_OUT "DMEZVGG.DAT" );
            iRet = 1;
        }
    }

    if( ! iRet )
    {
        poServizi = new FILE_SERVIZI_TRENO(PATH_IN "S_TRENO.TMP");
        if( ! poServizi -> FileHandle() )
        {
            BEEP;
            printf( "\nErrore nell'apertura del file '%s'\n", PATH_IN "S_TRENO.TMP" );
            iRet = 1;
        }
    }

    ULONG ulNumRecServizi = poServizi -> NumRecordsTotali();

    if( ! iRet )
    {
        ULONG ulNumRec = poMezzoVirtuale -> NumRecordsTotali();
        ULONG ulCurrRec = 0L;
        ELENCO_S oElencoMezzi;
        STRINGA oMezzo;

        if( OkPrintf ) printf( "Numero totale Mezzi Virtuali: %ld\n", ulNumRec );

        // Carico in memoria i servizi
        ELENCO oElencoServizi;
        for( ULONG ulCurrRecServizi = 0L; ulCurrRecServizi < ulNumRecServizi; ulCurrRecServizi ++ )
        {
            struct SERVIZI_TRENO * psServiziTreno = NULL;
            psServiziTreno = new SERVIZI_TRENO;
            ( * psServiziTreno ) = poServizi -> operator[]( ulCurrRecServizi );

            oElencoServizi += ( void * )psServiziTreno;
        }

        for( ; ulCurrRec < ulNumRec; ulCurrRec ++ )
        {
            sMezzoVirtuale = poMezzoVirtuale -> FixRec( ulCurrRec );

            // Effettuo una conversione dummy per considerare solo i mezzi
            // virtuali buoni (non scartati).
            char dummy1[ 5 ];
            char dummy2[ 2 ];
            if( ! DecodificaMezzoVirtuale( sMezzoVirtuale.MezzoVirtuale,
                                           roElencoRighe,
                                           dummy1,
                                           dummy2 ) )
            {
                for( int iContReali = 0; iContReali < sMezzoVirtuale.NumMezziComponenti; iContReali ++ )
                {
                    int iTrovato = FALSE;
                    struct MEZZO_VIRTUALE :: MEZZO_VIAGGIANTE * poMezzoVgg = & sMezzoVirtuale.Mv[ iContReali ];

                    oMezzo = St( poMezzoVgg -> IdentTreno );

                    ORD_FORALL( oElencoMezzi, iContMezzi )
                    {
                        if( oElencoMezzi[ iContMezzi ].Strip() == oMezzo.Strip() )
                        {
                            iTrovato = TRUE;
                            break;
                        }
                    }
                    if( ! iTrovato )    // Scarto i duplicati...
                    {
                        oElencoMezzi += oMezzo;

                        memset( & sDMEZVGG, ' ', sizeof( DMEZVGG ) );

                        if( poMezzoVgg -> KeyTreno[ 0 ] )
                        {
                            char szAppo[ 31 ];

                            memset( szAppo, 0x00, 31 );
                            memcpy( szAppo, poMezzoVgg -> KeyTreno, 5 );
                            memcpy( sDMEZVGG.NUMMEZVGG, szAppo, 5 );
                            memset( szAppo, 0x00, 31 );
                            memcpy( szAppo, poMezzoVgg -> IdentTreno, 10 );
                            memcpy( sDMEZVGG.KEYMEZVGG, szAppo, 10 );
                        }
                        memcpy( sDMEZVGG.CODRET, "83", 2 );
                        memcpy( sDMEZVGG.CODGSRSER, "083", 3 );

                        if( poMezzoVgg -> HaNome )
                        {
                            char szAppo[ 31 ];

                            memset( szAppo, 0x00, 31 );
                            memcpy( szAppo, sMezzoVirtuale.NomeMezzoVirtuale, strlen( sMezzoVirtuale.NomeMezzoVirtuale ) );
                            memcpy( sDMEZVGG.DEN, strupr( szAppo ), strlen( szAppo ) );
                        }

                        // Ricavo FUM e IDE.
                        ULONG ulCurrRecServizi = 0L;
                        int iTrovatoServizio = FALSE;
                        struct SERVIZI_TRENO * psServiziTreno = NULL;

                        ORD_FORALL( oElencoServizi, iContServizi )
                        {
                            psServiziTreno = ( SERVIZI_TRENO * )oElencoServizi[ iContServizi ];

                            if( ! memcmp(  poMezzoVgg -> IdentTreno, psServiziTreno -> IdentTreno, 10 ) )
                            {
                                iTrovatoServizio = TRUE;
                                break;
                            }
                        }
                        if( iTrovatoServizio )
                        {
                            if ( psServiziTreno -> Servizi.TipoMezzo == psServiziTreno -> Servizi.TRAGHETTO ||
                                 psServiziTreno -> Servizi.TipoMezzo == psServiziTreno -> Servizi.MONOCARENA ||
                                 psServiziTreno -> Servizi.TipoMezzo == psServiziTreno -> Servizi.ALISCAFO )
                            {
                                DecodTipoMezzoViaggiante( psListeDecodifica,
                                                          (STRINGA) "TRAGHETTO",
                                                          sDMEZVGG.CODTIPMEZVGG);
                            }
                            else if (psServiziTreno -> Servizi.TipoMezzo == psServiziTreno -> Servizi.AUTOBUS)
                            {
                                DecodTipoMezzoViaggiante( psListeDecodifica,
                                                          (STRINGA) "AUTOBUS",
                                                          sDMEZVGG.CODTIPMEZVGG);
                            }
                            else
                            {
                                DecodTipoMezzoViaggiante( psListeDecodifica,
                                                          (STRINGA) "TRENO",
                                                          sDMEZVGG.CODTIPMEZVGG);
                            }

                            if( psServiziTreno -> Servizi.Fumatori )
                            {
                                sDMEZVGG.FUM = '1';
                            }
                            else
                            {
                                sDMEZVGG.FUM = '0';
                            }

                            if( psServiziTreno -> Servizi.Prenotabilita )
                            {
                                sDMEZVGG.IDE = '1';
                            }
                            else
                            {
                                sDMEZVGG.IDE = '0';
                            }
                        }
                        else
                        {
                            char szAppo[ 11 ];
                            memset( szAppo, 0x00, 11 );
                            memcpy( szAppo, poMezzoVgg -> IdentTreno, 10 );
                            TRACESTRING( "Attenzione: servizi non trovati: Mezzo Viaggiante '" + STRINGA( szAppo ) + "'" );
                        }

                        memcpy( sDMEZVGG.DATINIVAL, ( const char * )psListeDecodifica -> oDataInizio, 10 );
                        memcpy( sDMEZVGG.DATFINVAL, ( const char * )psListeDecodifica -> oDataFine, 10 );

                        memcpy( sDMEZVGG.CODDSZ, ( const char * )psListeDecodifica -> oCodiceDistribuzione, 8 );
                        memcpy( sDMEZVGG.CR_LF, "\r\n", 2 );

                        poDMEZVGG -> AddRecordToEnd( BUFR( & sDMEZVGG, sizeof( DMEZVGG ) ) );
                        iNumMezziScritti ++;
                    }
                }
                if( OkPrintf ) printf( "Record letti: %06d, scritti: %06d\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", ulCurrRec+1, iNumMezziScritti );
            }
        }

        // Scarico i serivzi dalla memoria.
        FORALL( oElencoServizi, iContDelServizi )
        {
            struct SERVIZI_TRENO * psServiziTreno = NULL;
            psServiziTreno = ( SERVIZI_TRENO * )oElencoServizi[ iContDelServizi ];
            delete psServiziTreno;
        }
    }

    if( poMezzoVirtuale ) delete poMezzoVirtuale;
    if( poServizi ) delete poServizi;
    if( poDMEZVGG ) delete poDMEZVGG;

    printf("\n... Fine elaborazione tabella DMEZVGG\n");
    TRACESTRING("... Fine elaborazione tabella DMEZVGG");

    return iRet;
}

