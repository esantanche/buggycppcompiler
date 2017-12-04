//----------------------------------------------------------------------------
// FILE DECCFERM.CPP
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include <DB_HOST.HPP>
#include <math.h>

extern int OkPrintf;

class FILE_FERMATE_VIRT : public F_FERMATE_VIRT
{
    public:
        FILE_FERMATE_VIRT(const char* NomeFile,ULONG Size=64000) : F_FERMATE_VIRT(NomeFile,Size){};

        BOOL Seek(WORD Mezzo_Virtuale, WORD Progressivo)
        {
            BUFR Wrk;

            Wrk.Store( Mezzo_Virtuale );
            Wrk.Store( Progressivo );

            return Posiziona( Wrk ) && KeyEsatta;
        };
};

//----------------------------------------------------------------------------
// PROTOTIPI
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Elabora_DECCFERMEZ --------------------------------------------------------
//----------------------------------------------------------------------------
int Elabora_DECCFERMEZ(LISTE_DECODIFICA * psListeDecodifica, ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "Elabora_DECCFERMEZ"

    int iRet = 0;

    FILE * fp = fopen( TRC_PATH "deccferm.trc", "w" );

    struct FERMATE_VIRT sFermateVirtuale;
    struct DECCFERMEZ sDECCFERMEZ;
    struct PERIODICITA_FERMATA_VIRT sPeriodicita;

    FILE_FERMATE_VIRT * poFermateVirtuale = new FILE_FERMATE_VIRT(PATH_IN "M0_FERMV.TM1");
    F_PERIODICITA_FERMATA_VIRT * poPeriodicita = new F_PERIODICITA_FERMATA_VIRT (PATH_IN "M1_FERMV.TM1");
    FILE_DECCFERMEZ * poDECCFERMEZ = new FILE_DECCFERMEZ(PATH_OUT "DECCFERM.DAT");

    poDECCFERMEZ -> Clear( NUSTR, FALSE );

    if( ! poFermateVirtuale -> FileHandle() ||
        ! poPeriodicita -> FileHandle()     ||
        ! poDECCFERMEZ -> FileHandle() )
    {
        BEEP;
        iRet = 1;
        printf( "Errore nell'apertura dei file!\n" );
    }
    else
    {
        printf( "Inizio elaborazione tabella DECCFERM...\n" );

        // ULONG ulNumFermate = poFermateVirtuale -> NumRecordsTotali();
        ULONG ulNumPeriodicita = poPeriodicita -> NumRecordsTotali();
        ULONG ulCurrRec = 0L;
        ULONG ulNumScritti = 0L;

        TRACESTRING( "Sto per inizializzare la periodicit…..." );

        // Inizializzo la periodicit….
        T_PERIODICITA :: Init( DATE_PATH, "DATE.T" );
        T_PERIODICITA :: ImpostaProblema( T_PERIODICITA :: Inizio_Dati_Caricati, T_PERIODICITA :: Fine_Dati_Caricati, T_PERIODICITA :: Inizio_Dati_Caricati );

        TRACESTRING( "... inizializzata!" );

        // Scorro la tabella delle periodicita delle fermate
        // I record che trovo in questa tabella rappresentano gi… di per
        // s‚ le eccezioni delle fermate.
        printf( "\nNumero periodicit… da leggere: %ld\n", ulNumPeriodicita );
        for( ulCurrRec = 0L; ulCurrRec < ulNumPeriodicita; ulCurrRec ++ )
        {
            // Leggo il record corrente nella periodicit…
            sPeriodicita = poPeriodicita -> FixRec( ulCurrRec );

            // Pulisco la struttura del file di output
            memset( & sDECCFERMEZ, ' ', sizeof( DECCFERMEZ ) );

            // Sostituisco i valori originali del mezzo virtuale con
            // quelli decodificati
            if( ! DecodificaMezzoVirtuale( sPeriodicita.MezzoVirtuale,
                                           roElencoRighe,
                                           sDECCFERMEZ.NUMVIR,
                                           sDECCFERMEZ.PRGVIR ) )
            {
                PERIODICITA_IN_CHIARO * pPeriodicitaInChiaro = NULL;

                if( ( pPeriodicitaInChiaro = sPeriodicita.Periodicita.EsplodiPeriodicita() ) != NULL )
                {
                    for( int iCont = 0; iCont < pPeriodicitaInChiaro -> Dim(); iCont ++ )
                    {
                        sDECCFERMEZ.EFFETT = ( * pPeriodicitaInChiaro )[ iCont ].Tipo ? 'E' : 'S';

                        /* *FAN* þþ */
                        // ----------------------------------------------------
                        // 11/09/1996
                        // ----------------------------------------------------
                        // Accedo al file delle fermate dei mezzi virtuali per
                        // trovare la fermata corrispondente a quella in canna
                        // proveniente dalle eccezioni: se la fermata trovata
                        // non ha nessun bit acceso per quanto riguarda le
                        // caratteristiche di 'Facoltativa', 'Partenza' e
                        // 'Arrivo', scrivo nel campo 'CodiceTipoEccezioneFer-
                        // mata' "PERIODICA" e basta; altrimenti, decodifico
                        // i bit e scrivo "FACOLTATIVA", "SOLA SALITA" oppure
                        // "SOLA DISCESA", intendendo che la fermata in questio-
                        // ne Š PERIODICA E FACOLTATIVA, ecc.
                        if( poFermateVirtuale -> Seek( sPeriodicita.MezzoVirtuale, sPeriodicita.Progressivo ) )
                        {
                            sFermateVirtuale = poFermateVirtuale -> RecordCorrente();

                            if( sFermateVirtuale.FermataFacoltativa )
                            {
                                sDECCFERMEZ.CODTIPECC = '2'; // Facoltativa
                            }
                            else if( sFermateVirtuale.FermataPartenza &&
                                     ! sFermateVirtuale.FermataArrivo )
                            {
                                sDECCFERMEZ.CODTIPECC = '3'; // Sola Salita
                            }
                            else if( !sFermateVirtuale.FermataPartenza &&
                                     sFermateVirtuale.FermataArrivo )
                            {
                                sDECCFERMEZ.CODTIPECC = '4'; // Sola Discesa
                            }
                            else
                            {
                                sDECCFERMEZ.CODTIPECC = '1'; // Periodica e basta
                            }
                        }
                        else
                        {
                            fprintf( fp, "Non Š stata trovata una fermata in M0_FERMV (Mv:%05d, PrgSta:%03d)\n", sPeriodicita.MezzoVirtuale, sPeriodicita.Progressivo );
                        }
                        // ----------------------------------------------------
                        /* *FAN* þþ */

                        char szAppo[ 31 ];

                        sprintf( szAppo, "%03d", sPeriodicita.Progressivo );
                        memcpy( sDECCFERMEZ.PRGSTA, szAppo, strlen( szAppo ) );

                        if( ( * pPeriodicitaInChiaro )[ iCont ].TipoPeriodo != 0 )
                        {
                            sprintf( szAppo, "%02d", ( * pPeriodicitaInChiaro )[ iCont ].TipoPeriodo );
                            memcpy( sDECCFERMEZ.CODGIOSP, szAppo, strlen( szAppo ) );
                        }
                        else
                        {
                            memcpy( sDECCFERMEZ.CODGIOSP, "  ", 2 );
                        }

                        sprintf( szAppo, "%02d/%02d/%04d", ( * pPeriodicitaInChiaro )[ iCont ].Dal.Giorno,
                                                           ( * pPeriodicitaInChiaro )[ iCont ].Dal.Mese,
                                                           ( * pPeriodicitaInChiaro )[ iCont ].Dal.Anno );
                        memcpy( sDECCFERMEZ.DATAINI, szAppo, strlen( szAppo ) );

                        sprintf( szAppo, "%02d/%02d/%04d", ( * pPeriodicitaInChiaro )[ iCont ].Al.Giorno,
                                                           ( * pPeriodicitaInChiaro )[ iCont ].Al.Mese,
                                                           ( * pPeriodicitaInChiaro )[ iCont ].Al.Anno );
                        memcpy( sDECCFERMEZ.DATAFIN, szAppo, strlen( szAppo ) );

                        BYTE bGiorni = ( * pPeriodicitaInChiaro )[ iCont ].GiorniDellaSettimana;
                        fprintf( fp, "bGiorni=%X\n", bGiorni );
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
                        memcpy( sDECCFERMEZ.GIORNISET, szAppo, strlen( szAppo ) );

                        // Elimino i campi non significativi per il TPF
                        if( ! memcmp( sDECCFERMEZ.DATAINI, sDECCFERMEZ.DATAFIN, 10 ) )
                        {
                            memset( sDECCFERMEZ.CODGIOSP, 0x20, 2 );
                            memset( sDECCFERMEZ.GIORNISET, 0x20, 7 );
                        }
                        if( ! memcmp( sDECCFERMEZ.GIORNISET, "1111111", 7 ) &&
                            ! memcmp( sDECCFERMEZ.CODGIOSP, "  ", 2 ) )
                        {
                            memset( sDECCFERMEZ.GIORNISET, 0x20, 7 );
                        }
                        if( ! memcmp( sDECCFERMEZ.GIORNISET, "1111111", 7 ) )
                        {
                            memset( sDECCFERMEZ.GIORNISET, 0x20, 7 );
                        }

                        memcpy( sDECCFERMEZ.DATINIVAL, ( const char * )psListeDecodifica -> oDataInizio, 10 );
                        memcpy( sDECCFERMEZ.CODDSZ, ( const char * )psListeDecodifica -> oCodiceDistribuzione, 8 );
                        memcpy( sDECCFERMEZ.CR_LF, "\r\n", 2 );

                        poDECCFERMEZ -> AddRecordToEnd( BUFR( & sDECCFERMEZ, sizeof( DECCFERMEZ ) ) );
                        ulNumScritti ++;

                        if( OkPrintf ) printf( "Record letti: %06d, scritti: %06d\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", ulCurrRec+1, ulNumScritti );
                    }
                }
                else
                {
                    printf( "\n*** Metodo EsplodiPeriodicita() in errore. ***\n" );
                    break;
                }
            }
        }

        delete poDECCFERMEZ;
        delete poPeriodicita;
        delete poFermateVirtuale;
    }

    fclose( fp );

    return iRet;
}
