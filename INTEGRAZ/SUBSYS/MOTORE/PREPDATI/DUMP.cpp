//----------------------------------------------------------------------------
// FILE DUMP.CPP
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include <DB_HOST.HPP>
#include <math.h>

extern int OkPrintf;

void Scrivi_DUMP()
{
    #undef TRCRTN
    #define TRCRTN "Scrivi_DUMP"

    FILE * fp = NULL;
    int iIndex = 0;

    T_PERIODICITA :: Init( DATE_PATH, "DATE.T" );
    T_PERIODICITA :: ImpostaProblema( T_PERIODICITA :: Inizio_Dati_Caricati, T_PERIODICITA :: Fine_Dati_Caricati, T_PERIODICITA :: Inizio_Dati_Caricati );

    // F_DETSER
    printf( "Creazione del file di dump '" TRC_PATH "f_detser.dmp'... " );
    struct DETTAGLIO_SERVIZI sDettaglioServizio;
    FILE_DETTAGLIO_SERVIZI * poDettaglioServizio = new FILE_DETTAGLIO_SERVIZI( PATH_IN "F_DETSER.TMP", sizeof( struct DETTAGLIO_SERVIZI ) );
    fp = fopen( TRC_PATH "f_detser.dmp", "w" );
    if( ! fp )
    {
        printf( "\nErrore nella creazione del file.\n" );
    }
    else
    {
        ULONG ulNumDettagliServizio = poDettaglioServizio -> NumRecordsTotali();
        for( iIndex = 0; iIndex < ulNumDettagliServizio; iIndex ++ )
        {
            sDettaglioServizio = poDettaglioServizio -> FixRec( iIndex );

            char acCcr1[ 6 ];
            char acCcr2[ 6 ];

            memset( acCcr1, 0x00, 6 );
            memset( acCcr2, 0x00, 6 );
            memcpy( acCcr1, sDettaglioServizio.Ccr1, 5 );
            memcpy( acCcr2, sDettaglioServizio.Ccr2, 5 );
            fprintf( fp, "RecNo=%05d NumeroMezzoVg=%05d TipoSRV=%02d Ccr1=%s Ccr2=%s\n",
                         iIndex,
                         sDettaglioServizio.NumeroMezzoVg,
                         sDettaglioServizio.TipoSRV,
                         acCcr1,
                         acCcr2 );

            fprintf( fp, "%s\n", ( PSZ )( CPSZ )sDettaglioServizio.Servizi.Decodifica() );
            fprintf( fp, "HaNoteGeneriche      =%d\n", sDettaglioServizio.Servizi.HaNoteGeneriche       );
            fprintf( fp, "HaNoteDiVendita      =%d\n", sDettaglioServizio.Servizi.HaNoteDiVendita       );
            fprintf( fp, "PrenotDisUniforme    =%d\n", sDettaglioServizio.Servizi.PrenotDisUniforme     );
            fprintf( fp, "Prenotabilita        =%d\n", sDettaglioServizio.Servizi.Prenotabilita         );
            fprintf( fp, "PrenObbligItalia     =%d\n", sDettaglioServizio.Servizi.PrenObbligItalia      );
            fprintf( fp, "PrenObbligEstero     =%d\n", sDettaglioServizio.Servizi.PrenObbligEstero      );
            fprintf( fp, "PrenotabileSolo1Cl   =%d\n", sDettaglioServizio.Servizi.PrenotabileSolo1Cl    );
            fprintf( fp, "ServTraspDisUniformi =%d\n", sDettaglioServizio.Servizi.ServTraspDisUniformi  );
            fprintf( fp, "ServizioBase         =%d\n", sDettaglioServizio.Servizi.ServizioBase          );
            fprintf( fp, "PostiASederePrima    =%d\n", sDettaglioServizio.Servizi.PostiASederePrima     );
            fprintf( fp, "PostiASedereSeconda  =%d\n", sDettaglioServizio.Servizi.PostiASedereSeconda   );
            fprintf( fp, "SleeperettePrima     =%d\n", sDettaglioServizio.Servizi.SleeperettePrima      );
            fprintf( fp, "SleeperetteSeconda   =%d\n", sDettaglioServizio.Servizi.SleeperetteSeconda    );
            fprintf( fp, "CuccettePrima        =%d\n", sDettaglioServizio.Servizi.CuccettePrima         );
            fprintf( fp, "CuccetteSeconda      =%d\n", sDettaglioServizio.Servizi.CuccetteSeconda       );
            fprintf( fp, "VagoniLettoPrima     =%d\n", sDettaglioServizio.Servizi.VagoniLettoPrima      );
            fprintf( fp, "VagoniLettoSeconda   =%d\n", sDettaglioServizio.Servizi.VagoniLettoSeconda    );
            fprintf( fp, "AutoAlSeguito        =%d\n", sDettaglioServizio.Servizi.AutoAlSeguito         );
            fprintf( fp, "Invalidi             =%d\n", sDettaglioServizio.Servizi.Invalidi              );
            fprintf( fp, "Biciclette           =%d\n", sDettaglioServizio.Servizi.Biciclette            );
            fprintf( fp, "Animali              =%d\n", sDettaglioServizio.Servizi.Animali               );
            fprintf( fp, "ServGenerDisUniformi =%d\n", sDettaglioServizio.Servizi.ServGenerDisUniformi  );
            fprintf( fp, "Ristoro              =%d\n", sDettaglioServizio.Servizi.Ristoro               );
            fprintf( fp, "BuffetBar            =%d\n", sDettaglioServizio.Servizi.BuffetBar             );
            fprintf( fp, "SelfService          =%d\n", sDettaglioServizio.Servizi.SelfService           );
            fprintf( fp, "Ristorante           =%d\n", sDettaglioServizio.Servizi.Ristorante            );
            fprintf( fp, "Fumatori             =%d\n", sDettaglioServizio.Servizi.Fumatori              );
            fprintf( fp, "ClassificaDisUniforme=%d\n", sDettaglioServizio.Servizi.ClassificaDisUniforme );

            PERIODICITA_IN_CHIARO * poPeriodicitaInChiaro = NULL;
            poPeriodicitaInChiaro = sDettaglioServizio.Periodicita.EsplodiPeriodicita();

            ELENCO_S oElencoPeriodi = poPeriodicitaInChiaro -> PeriodicitaLeggibile();
            ORD_FORALL( oElencoPeriodi, iContPeriodi )
            {
                fprintf( fp, "    %s\n", ( PSZ )( CPSZ )oElencoPeriodi[ iContPeriodi ] );
            }
            fprintf( fp, "-----------------------------------------------------------------------------\n\n" );
        }
        fprintf( fp, "\nTOTALE RECORD: %06ld\n", ulNumDettagliServizio );
        fclose( fp );
        delete poDettaglioServizio;
        printf( "terminata.\n" );
    }

    // F_SRVTRN
    printf( "Creazione del file di dump '" TRC_PATH "f_srvtrn.dmp'... " );
    struct FILE_SERVIZI :: R_STRU sServizio;
    FILE_SERVIZI * poServizio = new FILE_SERVIZI( PATH_IN "F_SRVTRN.TMP", sizeof( struct FILE_SERVIZI :: R_STRU ) );
    fp = fopen( TRC_PATH "f_srvtrn.dmp", "w" );
    if( ! fp )
    {
        printf( "\nErrore nella creazione del file.\n" );
    }
    else
    {
        ULONG ulNumServizi = poServizio -> NumRecordsTotali();

        for( iIndex = 0; iIndex < ulNumServizi; iIndex ++ )
        {
            sServizio = poServizio -> FixRec( iIndex );

            fprintf( fp, "RecNo=%05d NumeroMezzoVg=%05d Uniforme=%d\n",
                         iIndex,
                         sServizio.NumeroMezzoVg,
                         sServizio.Uniforme );
            fprintf( fp, "%s\n", ( PSZ )( CPSZ )sServizio.Servizi.Decodifica() );
            fprintf( fp, "-----------------------------------------------------------------------------\n\n" );
        }
        fprintf( fp, "\nTOTALE RECORD: %06ld\n", ulNumServizi );
        fclose( fp );
        delete poServizio;
        printf( "terminata.\n" );

        // M0_TRENV
        printf( "Creazione del file di dump '" TRC_PATH "m0_trenv.dmp'... " );
        struct MEZZO_VIRTUALE sMezzoVirtuale;
        F_MEZZO_VIRTUALE * poMezzoVirtuale = new F_MEZZO_VIRTUALE(PATH_IN "M0_TRENV.TM1");
        fp = fopen( TRC_PATH "m0_trenv.dmp", "w" );
        if( ! fp )
        {
            printf( "Errore nella creazione del file.\n" );
        }
        ULONG ulNumMezzi = poMezzoVirtuale -> NumRecordsTotali();

        for( iIndex = 0; iIndex < ulNumMezzi ; iIndex ++ )
        {
            sMezzoVirtuale = poMezzoVirtuale -> FixRec( iIndex );

            PERIODICITA_IN_CHIARO * poPeriodicitaInChiaro = NULL;
            poPeriodicitaInChiaro = sMezzoVirtuale.PeriodicitaMV.EsplodiPeriodicita();

            fprintf( fp, "RecNo=%05d MezzoVirtuale=%05d NomeMezzoVirtuale=%s TipoMezzo=%02d GiornoSuccessivo=%d PeriodicitaDisuniformi=%d\n"
                         "DaCarrozzeDirette=%d FittizioLinearizzazione=%d NumeroFermateTransiti=%03d NumeroFermateValide=%03d\n",
                         iIndex,
                         sMezzoVirtuale.MezzoVirtuale,
                         sMezzoVirtuale.NomeMezzoVirtuale,
                         sMezzoVirtuale.TipoMezzo,
                         sMezzoVirtuale.GiornoSuccessivo,
                         sMezzoVirtuale.PeriodicitaDisuniformi,
                         sMezzoVirtuale.DaCarrozzeDirette,
                         sMezzoVirtuale.FittizioLinearizzazione,
                         sMezzoVirtuale.NumeroFermateTransiti,
                         sMezzoVirtuale.NumeroFermateValide );


            ELENCO_S oElencoPeriodi = poPeriodicitaInChiaro -> PeriodicitaLeggibile();
            ORD_FORALL( oElencoPeriodi, iContPeriodi )
            {
                fprintf( fp, "    GiorniDellaSettimana=%X; TipoPeriodo=%02d; %s\n", ( * poPeriodicitaInChiaro )[ iContPeriodi ].GiorniDellaSettimana,
                                                                                    ( * poPeriodicitaInChiaro )[ iContPeriodi ].TipoPeriodo,
                                                                                    ( PSZ )( CPSZ )oElencoPeriodi[ iContPeriodi ] );
            }

            for( int iContMezzi = 0; iContMezzi < sMezzoVirtuale.NumMezziComponenti; iContMezzi ++ )
            {
                char acIdentTreno[ 11 ];
                char acKeyTreno[ 6 ];

                memset( acIdentTreno, 0x00, 11 );
                memset( acKeyTreno, 0x00, 6 );

                memcpy( acIdentTreno, sMezzoVirtuale.Mv[ iContMezzi ].IdentTreno, 10 );
                memcpy( acKeyTreno, sMezzoVirtuale.Mv[ iContMezzi ].KeyTreno, 5 );

                fprintf( fp, "        Mezzo Viaggiante: NumeroMezzo=%06d TipoMezzo=%02d HaNome=%d StazioneDiCambio=%04d IdentTreno=%s KeyTreno=%s\n",
                         sMezzoVirtuale.Mv[ iContMezzi ].NumeroMezzo,
                         sMezzoVirtuale.Mv[ iContMezzi ].TipoMezzo,
                         sMezzoVirtuale.Mv[ iContMezzi ].HaNome,
                         sMezzoVirtuale.Mv[ iContMezzi ].StazioneDiCambio,
                         acIdentTreno,
                         acKeyTreno );
            }
            fprintf( fp, "-----------------------------------------------------------------------------\n\n" );
        }
        fprintf( fp, "\nTOTALE RECORD: %06ld\n", ulNumMezzi );
        fclose( fp );
        delete poMezzoVirtuale;
        printf( "terminata.\n" );
    }
#if 0
    // DFERMEZV
    printf( "Creazione del file di dump '" TRC_PATH "m0_fermv.dmp'... " );
    struct FERMATE_VIRT sFermata;
    F_FERMATE_VIRT * poFermateVirtuale = new F_FERMATE_VIRT(PATH_IN "M0_FERMV.TM1");
    fp = fopen( TRC_PATH "m0_fermv.dmp", "w" );
    if( ! fp )
    {
        printf( "\nErrore nella creazione del file.\n" );
    }
    else
    {
        ULONG ulNumFermate = poFermateVirtuale -> NumRecordsTotali();

        for( iIndex = 0; iIndex < ulNumFermate; iIndex ++ )
        {
            sFermata = poFermateVirtuale -> FixRec( iIndex );

            fprintf( fp, "MezzoVirtuale=%05d Progressivo=%04d Progressivo2=%04d Id=%04d CCR=%06d TrenoFisico=%d OraArrivo=%03d OraPartenza=%03d GiornoSuccessivoArrivo=%d GiornoSuccessivoPartenza=%d "
                         "ProgKm=%04d Transito=%d FermataFacoltativa=%d FermataPartenza=%d FermataArrivo=%d FermataDiServizio=%d HaNote=%d HaNoteLocalita=%d\n",
                         sFermata.MezzoVirtuale,
                         sFermata.Progressivo,
                         sFermata.Progressivo2,
                         sFermata.Id,
                         sFermata.CCR,
                         sFermata.TrenoFisico,
                         sFermata.OraArrivo,
                         sFermata.OraPartenza,
                         sFermata.GiornoSuccessivoArrivo,
                         sFermata.GiornoSuccessivoPartenza,
                         sFermata.ProgKm,
                         sFermata.Transito,
                         sFermata.FermataFacoltativa,
                         sFermata.FermataPartenza,
                         sFermata.FermataArrivo,
                         sFermata.FermataDiServizio,
                         sFermata.HaNote,
                         sFermata.HaNoteLocalita );
        }
        fprintf( fp, "\nTOTALE RECORD: %06ld\n", ulNumFermate );
        fclose( fp );
        delete poFermateVirtuale;
        printf( "terminata.\n" );
    }
#endif
}
