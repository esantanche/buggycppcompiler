//----------------------------------------------------------------------------
// FILE DFERMEZV.CPP
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

//----------------------------------------------------------------------------
// Elabora_DFERMEZVIR --------------------------------------------------------
//----------------------------------------------------------------------------
int Elabora_DFERMEZVIR(LISTE_DECODIFICA * psListeDecodifica, ELENCO & roElencoRighe )
{
    #undef TRCRTN
    #define TRCRTN "Elabora_DFERMEZVIR"

    int iRet = 0;

    struct FERMATE_VIRT sFermateVirtuale;
    struct PERIODICITA_FERMATA_VIRT sPerioFermate;
    struct DFERMEZVIR sDFERMEZVIR;

    F_FERMATE_VIRT * poFermateVirtuale = new F_FERMATE_VIRT(PATH_IN "M0_FERMV.TM1");
    F_PERIODICITA_FERMATA_VIRT * poPerio = new F_PERIODICITA_FERMATA_VIRT(PATH_IN "M1_FERMV.TM1");
    FILE_DFERMEZVIR * poDFERMEZVIR = new FILE_DFERMEZVIR(PATH_OUT "DFERMEZV.DAT");

    poDFERMEZVIR->Clear(NUSTR, FALSE);

    if (! poFermateVirtuale->FileHandle() ||
        ! poPerio->FileHandle() ||
        ! poDFERMEZVIR->FileHandle())
    {
        iRet = 1;
        BEEP;
        TRACESTRING("ERRORE apertura file M0_FERMV.TM1/M1_FERMV.TM1/DFERMEZV.DAT");
    }
    else
    {
        printf("\nInizio elaborazione tabella DFERMEZVIR ...\n");
        TRACESTRING ("Inizio elaborazione tabella DFERMEZVIR ...\n");

        ULONG ulNum = poFermateVirtuale->NumRecordsTotali();
        ULONG ulNumPerio = poPerio->NumRecordsTotali();
        ULONG ulNumRecord = 0;
        ULONG ulNumRecord1 = 0;
//      iNumeroRigheScritte = 0;

        printf( "Numero fermate virtuali lette: %ld\n", ulNum );
        printf( "Numero periodicit… lette: %ld\n", ulNumPerio );
        printf( "\nInizio scrittura fermate virtuali...\n" );

        char acOldVir[ 8 ];
        memset( acOldVir, 0x00, 8 );
        memset( acOldVir, 0x20, 7 );

        for (; ulNumRecord < ulNum ; )
        {
            sFermateVirtuale = poFermateVirtuale->FixRec(ulNumRecord);
            ulNumRecord++;

            int iNumeroMezzoVirtuale = 0;

            memset(&sDFERMEZVIR, ' ', sizeof(sDFERMEZVIR) );

            if( ! DecodificaMezzoVirtuale( sFermateVirtuale.MezzoVirtuale,
                                           roElencoRighe,
                                           sDFERMEZVIR.NUMVIR,
                                           sDFERMEZVIR.PRGVIR ) )
            {
                if( memcmp( sDFERMEZVIR.NUMVIR, acOldVir, 7 ) )
                {
                    memcpy( acOldVir, sDFERMEZVIR.NUMVIR, 5 );
                    memcpy( acOldVir + 5, sDFERMEZVIR.PRGVIR, 2 );
                }

                char szAppo[ 6 ];

                sprintf( szAppo, "%03d", sFermateVirtuale.Progressivo );
                memcpy( sDFERMEZVIR.PRGSTA, szAppo, strlen( szAppo ) );

                sprintf( szAppo, "%05d", sFermateVirtuale.CCR );
                memcpy( sDFERMEZVIR.CODSTA, szAppo, strlen( szAppo ) );

                int iOre, iMinuti;
                char cOre[2+1];
                char cMinuti[2+1];
                STRINGA oOraMinuti;

                iOre = 0;
                iMinuti = 0;

                while (sFermateVirtuale.OraArrivo >= 60)
                {
                    iOre++;
                    sFermateVirtuale.OraArrivo -= 60;
                }

                iMinuti = sFermateVirtuale.OraArrivo;

                sprintf(cOre, "%0*d", sizeof(cOre)-1, iOre);
                sprintf(cMinuti, "%0*d", sizeof(cMinuti)-1, iMinuti);
                oOraMinuti = cOre;
                oOraMinuti += ":";
                oOraMinuti += cMinuti;
                oOraMinuti += ":00";

                memcpy(sDFERMEZVIR.ORAARR, (const char *) oOraMinuti, 8);

                iOre = 0;
                iMinuti = 0;

                while (sFermateVirtuale.OraPartenza >= 60)
                {
                    iOre++;
                    sFermateVirtuale.OraPartenza -= 60;
                }

                iMinuti = sFermateVirtuale.OraPartenza;

                sprintf(cOre, "%0*d", sizeof(cOre)-1, iOre);
                sprintf(cMinuti, "%0*d", sizeof(cMinuti)-1, iMinuti);
                oOraMinuti = cOre;
                oOraMinuti += ":";
                oOraMinuti += cMinuti;
                oOraMinuti += ":00";

                memcpy(sDFERMEZVIR.ORAPAR, (const char *) oOraMinuti, 8);

                sprintf( szAppo, "%04d", sFermateVirtuale.ProgKm );
                memcpy( sDFERMEZVIR.PROGKM, szAppo, 4 );

                if (sFermateVirtuale.GiornoSuccessivoArrivo ||
                    sFermateVirtuale.GiornoSuccessivoPartenza)
                {
                    sDFERMEZVIR.SCS = '1';
                }
                else
                {
                    sDFERMEZVIR.SCS = '0';
                }

                if (sFermateVirtuale.Transito)
                {
                    DecodTipoFermata( psListeDecodifica,
                                      (STRINGA) "TRANSITO",
                                       sDFERMEZVIR.CODTIPFER);
                }
                else if (sFermateVirtuale.FermataPartenza &&
                         !sFermateVirtuale.FermataArrivo)
                {
                    DecodTipoFermata( psListeDecodifica,
                                      (STRINGA) "PARTENZA",
                                       sDFERMEZVIR.CODTIPFER);
                }
                else if (sFermateVirtuale.FermataArrivo &&
                         !sFermateVirtuale.FermataPartenza)
                {
                    DecodTipoFermata( psListeDecodifica,
                                      (STRINGA) "ARRIVO",
                                       sDFERMEZVIR.CODTIPFER);
                }
                else
                {
                    DecodTipoFermata( psListeDecodifica,
                                      (STRINGA) "FERMATA",
                                       sDFERMEZVIR.CODTIPFER);
                }

                memcpy(sDFERMEZVIR.CODRETSTA, "83", 2);
                memcpy(sDFERMEZVIR.DATINIVAL, (const char *) psListeDecodifica->oDataInizio, 10);
                memcpy(sDFERMEZVIR.DATFINVAL, (const char *) psListeDecodifica->oDataFine, 10);
                memcpy(sDFERMEZVIR.CODDSZ, (const char *) psListeDecodifica->oCodiceDistribuzione, 8);
                memcpy(sDFERMEZVIR.CR_LF, "\r\n",2);

                sDFERMEZVIR.INDECZ = '0';

                for (; ulNumRecord1 < ulNumPerio ; )
                {
                    sPerioFermate = poPerio->FixRec(ulNumRecord1);

                    if (sFermateVirtuale.MezzoVirtuale == sPerioFermate.MezzoVirtuale) // stesso mezzo virtuale
                    {
                        if (sFermateVirtuale.Progressivo == sPerioFermate.Progressivo) // trovata periodicit…
                        {
                            sDFERMEZVIR.INDECZ = '1';
                            ulNumRecord1++;
                            break;
                        }
                        else if (sFermateVirtuale.Progressivo < sPerioFermate.Progressivo) // periodicit… di una fermata successiva
                        {
                            break;
                        }
                        else if (sFermateVirtuale.Progressivo > sPerioFermate.Progressivo) // buco tra periodicit…
                        {
                            ulNumRecord1++;
                        }
                    }
                    else if (sFermateVirtuale.MezzoVirtuale < sPerioFermate.MezzoVirtuale) // peridicit… mezzo virtuale successivo
                    {
                        break;
                    }
                    else if (sFermateVirtuale.MezzoVirtuale > sPerioFermate.MezzoVirtuale) // buco peridicit… mezzi virtuali
                    {
                        ulNumRecord1++;
                    }
                }

                iRet = ScriviDFERMEZVIR(poDFERMEZVIR, &sDFERMEZVIR);

                if (iRet)
                    break;

                if( ulNumRecord % 1000 == 0 )
                {
                    if( OkPrintf ) printf( "%06ld...\b\b\b\b\b\b\b\b\b", ulNumRecord );
                }
            }
        }

        delete poFermateVirtuale;
        delete poDFERMEZVIR;
        delete poPerio;

        printf("\n... Fine elaborazione tabella DFERMEZVIR\n");
        TRACESTRING("... Fine elaborazione tabella DFERMEZVIR");
    }

    return iRet;
}

//----------------------------------------------------------------------------
// ScriviDFERMEZVIR ----------------------------------------------------------
//----------------------------------------------------------------------------
int ScriviDFERMEZVIR(FILE_DFERMEZVIR * poDFERMEZVIR,
                     DFERMEZVIR * psDFERMEZVIR)
{
    #undef TRCRTN
    #define TRCRTN "ScriviDFERMEZVIR"

    int iRet = 0;

//  iNumeroRigheScritte++;

    poDFERMEZVIR->AddRecordToEnd(BUFR(psDFERMEZVIR,sizeof(*psDFERMEZVIR)));

    // Modificato per ridurre i tempi di scrittura a video
//  if(iNumeroRigheScritte % 100 == 0)
//  {
//     if( OkPrintf ) printf("Numero fermate scritte: %05d\r", iNumeroRigheScritte);
//  }

    return iRet;
}

