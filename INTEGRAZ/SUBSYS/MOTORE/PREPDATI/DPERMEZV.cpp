//----------------------------------------------------------------------------
// FILE DPERMEZV.CPP
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
// Elabora_DPERMEZVIR --------------------------------------------------------
//----------------------------------------------------------------------------
int Elabora_DPERMEZVIR(LISTE_DECODIFICA * psListeDecodifica, ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "Elabora_DPERMEZVIR"

    int iRet = 0;

    struct MEZZO_VIRTUALE sMezzoVirtuale;
    struct DPERMEZVIR sDPERMEZVIR;

    F_MEZZO_VIRTUALE * poMezzoVirtuale = new F_MEZZO_VIRTUALE(PATH_IN "M0_TRENV.TM1");
    FILE_DPERMEZVIR * poDPERMEZVIR = new FILE_DPERMEZVIR(PATH_OUT "DPERMEZV.DAT");

    poDPERMEZVIR -> Clear( NUSTR, FALSE );

    if( ! poMezzoVirtuale -> FileHandle() ||
        ! poDPERMEZVIR -> FileHandle() )
    {
        BEEP;
        iRet = 1;
        printf( "Errore nell'apertura dei file!\n" );
    }
    else
    {
        printf( "Inizio elaborazione tabella DPERMEZV...\n" );

        ULONG ulNumMezzi = poMezzoVirtuale -> NumRecordsTotali();
        ULONG ulCurrRec = 0L;
        ULONG ulNumScritti = 0L;

        FILE * fp = fopen( TRC_PATH "dpermezv.trc", "w" );

        T_PERIODICITA :: Init( DATE_PATH, "DATE.T" );
        T_PERIODICITA :: ImpostaProblema( T_PERIODICITA :: Inizio_Dati_Caricati, T_PERIODICITA :: Fine_Dati_Caricati, T_PERIODICITA :: Inizio_Dati_Caricati );

        // Scorro la tabella delle periodicita delle fermate
        // I record che trovo in questa tabella rappresentano gi… di per
        // s‚ le eccezioni delle fermate.
        if( OkPrintf ) printf( "\nNumero mezzi da leggere: %ld\n", ulNumMezzi );
        for( ulCurrRec = 0L; ulCurrRec < ulNumMezzi; ulCurrRec ++ )
        {
//          if( ulCurrRec > 100 )
//          {
//              break;
//          }

            // Leggo il record corrente nella periodicit…
            sMezzoVirtuale = poMezzoVirtuale -> FixRec( ulCurrRec );

//          if( ! sMezzoVirtuale.PeriodicitaDisuniformi )
//          {
                // Pulisco la struttura del file di output
                memset( & sDPERMEZVIR, ' ', sizeof( DPERMEZVIR ) );

                // Sostituisco i valori originali del mezzo virtuale con
                // quelli decodificati
                if( ! DecodificaMezzoVirtuale( sMezzoVirtuale.MezzoVirtuale,
                                               roElencoRighe,
                                               sDPERMEZVIR.NUMVIR,
                                               sDPERMEZVIR.PRGVIR ) )
                {
                    PERIODICITA_IN_CHIARO * pPeriodicitaInChiaro = NULL;

                    if( ( pPeriodicitaInChiaro = sMezzoVirtuale.PeriodicitaMV.EsplodiPeriodicita() ) )
                    {
                        ELENCO_S oElencoPeriodi = pPeriodicitaInChiaro -> PeriodicitaLeggibile();
                        ORD_FORALL( oElencoPeriodi, iContPeriodi )
                        {
                            char acNumVir[ 6 ];
                            memset( acNumVir, 0x00, 6 );
                            memcpy( acNumVir, sDPERMEZVIR.NUMVIR, 5 );
                            char acPrgVir[ 3 ];
                            memset( acPrgVir, 0x00, 3 );
                            memcpy( acPrgVir, sDPERMEZVIR.PRGVIR, 2 );
                            fprintf( fp, "MezzoDaMotore=%05d MezzoVirtuale=%s PrgVir=%s Periodicit…:%s\n", sMezzoVirtuale.MezzoVirtuale, acNumVir, acPrgVir, ( PSZ )( CPSZ )oElencoPeriodi[ iContPeriodi
                            ] );
                        }
                        fprintf( fp, "\n" );

                        for( int iCont = 0; iCont < pPeriodicitaInChiaro -> Dim(); iCont ++ )
                        {
                            sDPERMEZVIR.EFFETT = ( * pPeriodicitaInChiaro )[ iCont ].Tipo ? 'E' : 'S';

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
    //                      if( poFermateVirtuale -> Seek( sPeriodicita.MezzoVirtuale, sPeriodicita.Progressivo ) )
    //                      {
    //                          sFermateVirtuale = poFermateVirtuale -> RecordCorrente();
    //
    //                          if( sFermateVirtuale.FermataFacoltativa )
    //                          {
    //                              sDPERMEZVIR.CODTIPECC = '2'; // Facoltativa
    //                          }
    //                          else if( sFermateVirtuale.FermataPartenza &&
    //                                   ! sFermateVirtuale.FermataArrivo )
    //                          {
    //                              sDPERMEZVIR.CODTIPECC = '3'; // Sola Salita
    //                          }
    //                          else if( !sFermateVirtuale.FermataPartenza &&
    //                                   sFermateVirtuale.FermataArrivo )
    //                          {
    //                              sDPERMEZVIR.CODTIPECC = '4'; // Sola Discesa
    //                          }
    //                          else
    //                          {
    //                              sDPERMEZVIR.CODTIPECC = '1'; // Periodica e basta
    //                          }
    //                      }
    //                      else
    //                      {
    //                          if( OkPrintf ) printf( "\nNon Š stata trovata la fermata %d:%d\n", sPeriodicita.MezzoVirtuale, sPeriodicita.Progressivo );
    //                      }
                            // ----------------------------------------------------
                            /* *FAN* þþ */

                            char szAppo[ 31 ];

                            memcpy( sDPERMEZVIR.PRGINI, "000", 3 ); // Dummy
                            memcpy( sDPERMEZVIR.PRGFIN, "000", 3 ); // Dummy

                            if( ( * pPeriodicitaInChiaro )[ iCont ].TipoPeriodo != 0 )
                            {
                                sprintf( szAppo, "%02d", ( * pPeriodicitaInChiaro )[ iCont ].TipoPeriodo );
                                memcpy( sDPERMEZVIR.CODGIOSP, szAppo, strlen( szAppo ) );
                            }
                            else
                            {
                                memcpy( sDPERMEZVIR.CODGIOSP, "  ", 2 );
                            }


                            sprintf( szAppo, "%02d/%02d/%04d", ( * pPeriodicitaInChiaro )[ iCont ].Dal.Giorno,
                                                               ( * pPeriodicitaInChiaro )[ iCont ].Dal.Mese,
                                                               ( * pPeriodicitaInChiaro )[ iCont ].Dal.Anno );
                            memcpy( sDPERMEZVIR.DATAINI, szAppo, strlen( szAppo ) );

                            sprintf( szAppo, "%02d/%02d/%04d", ( * pPeriodicitaInChiaro )[ iCont ].Al.Giorno,
                                                               ( * pPeriodicitaInChiaro )[ iCont ].Al.Mese,
                                                               ( * pPeriodicitaInChiaro )[ iCont ].Al.Anno );
                            memcpy( sDPERMEZVIR.DATAFIN, szAppo, strlen( szAppo ) );

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
                            memcpy( sDPERMEZVIR.GIORNISET, szAppo, strlen( szAppo ) );

                            // Elimino i campi non significativi per il TPF
                            if( ! memcmp( sDPERMEZVIR.DATAINI, sDPERMEZVIR.DATAFIN, 10 ) )
                            {
                                memset( sDPERMEZVIR.CODGIOSP, 0x20, 2 );
                                memset( sDPERMEZVIR.GIORNISET, 0x20, 7 );
                            }
                            if( ! memcmp( sDPERMEZVIR.GIORNISET, "1111111", 7 ) &&
                                ! memcmp( sDPERMEZVIR.CODGIOSP, "  ", 2 ) )
                            {
                                memset( sDPERMEZVIR.GIORNISET, 0x20, 7 );
                            }
                            if( ! memcmp( sDPERMEZVIR.GIORNISET, "1111111", 7 ) )
                            {
                                memset( sDPERMEZVIR.GIORNISET, 0x20, 7 );
                            }

                            strcpy( szAppo, "02/06/1996" );
                            memcpy( sDPERMEZVIR.DATINIVAL, szAppo, 10 );
                            memcpy( sDPERMEZVIR.CODDSZ, ( const char * )psListeDecodifica -> oCodiceDistribuzione, 8 );
                            memcpy( sDPERMEZVIR.CR_LF, "\r\n", 2 );

                            poDPERMEZVIR -> AddRecordToEnd( BUFR( & sDPERMEZVIR, sizeof( DPERMEZVIR ) ) );
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
//          }
//          else
//          {
//              if( OkPrintf ) printf( "Mezzo %d ha periodicit… disuniformi.\n", sMezzoVirtuale.MezzoVirtuale );
//          }
        }

        fclose( fp );

        delete poDPERMEZVIR;
        delete poMezzoVirtuale;
    }

    return iRet;
}
