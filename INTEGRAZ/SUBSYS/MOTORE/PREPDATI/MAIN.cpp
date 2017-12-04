// Abilitazione della trace.
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

// Inclusioni standard del C.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Inclusioni per l'ambiente OS/2.
#include <os2.h>

// Inclusioni per OOLIB.
#include <std.h>
#include <trc2.h>

// Inclusioni per l'applicazione.
#include <db_host.hpp>

TRACEVARIABLES;

// Variabili globali.
int OkPrintf;

// Main del programma.
int main( int argc, char * argv[] )
{
   #undef TRCRTN
   #define TRCRTN "DB_HOST"
   
   TRACEREGISTER( NULL, "DB_HOST" );
   TRACEEXE( argv[ 0 ] );
   TRACEPARAMS( argc, argv );
   
   TRACESTRING( ">>> Inizio elaborazione" );
   
   SetPriorita(); // Imposta la priorita'
   GESTIONE_ECCEZIONI_ON
   DosError(2); // Disabilita Popup di errore
   
   // Abilito o disabilito le printf a video
   OkPrintf = TRUE;
   
   int iRet = 0;
   int iCreaFamiglie;
   
   if( argc > 1 ) {
      if( ! strcmp( argv[ 1 ], "-W" ) ) {
         iCreaFamiglie = TRUE;
      } else if( ! strcmp( argv[ 1 ], "-r" ) ) {
         iCreaFamiglie = FALSE;
      } else {
         iRet = 1;
         printf( "Parametro non riconosciuto.\n"
            "    Immettere: 'DB_HOST -W' (maiuscolo) per creare le famiglie (ed il file .CAC)\n"
            "               'DB_HOST -r' per leggere il file .CAC (cache)\n" );
      }
      
      if( ! iRet ) {
         ELENCO oElencoRighe;
         
         LISTE_DECODIFICA sListeDecodifica;
         LISTE_DECODIFICA * psListeDecodifica = & sListeDecodifica;
         
         printf("\nDB_HOST: Inizio elaborazione ...\n");
         
         iRet = InizializzaListeDecodifica(psListeDecodifica);
         
         if (iRet) {
            BEEP;
            printf("\nERRORE: file DECOD.DAT non trovato o inutilizzabile\n");
         }
         
         //          Scrivi_DUMP();
         
         iRet = Elabora_DMEZVIR(psListeDecodifica, oElencoRighe, iCreaFamiglie );
         
         //          if (!iRet)
         //          {
         //              iRet = Elabora_DMEZVGG(psListeDecodifica, oElencoRighe );
         //          }
         
         //          if (!iRet)
         //          {
         //              iRet = Elabora_DFERMEZVIR(psListeDecodifica, oElencoRighe );
         //          }
         
         //          if (!iRet)
         //          {
         //              iRet = Elabora_DCOSMEZVIR(psListeDecodifica, oElencoRighe );
         //          }
         
         //            if( ! iRet )
         //            {
         //                iRet = Elabora_DECCFERMEZ( psListeDecodifica, oElencoRighe );
         //            }
         
         //          if( ! iRet )
         //          {
         //              iRet = Elabora_DPERMEZVIR( psListeDecodifica, oElencoRighe );
         //          }
         
         if( ! iRet ) {
            iRet = Elabora_DSEROFF( psListeDecodifica, oElencoRighe );
         }
         
         if (iRet) {
            BEEP;
            printf("\nElaborazione interrotta");
         } else {
            printf("\n... DB_HOST: Fine elaborazione.\nChiusura applicazione in corso...\n");
         }
//<<< if  ! iRet    
      }
//<<< if  argc > 1    
   } else {
      iRet = 1;
      printf( "Errato numero di parametri.\n"
         "    Immettere: 'DB_HOST -w' per creare le famiglie (ed il file .CAC)\n"
         "               'DB_HOST -r' per leggere il file .CAC (cache)\n" );
   }
   
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return ( iRet ? 999 : 0 );
//<<< int main  int argc, char * argv    
}

//----------------------------------------------------------------------------
// MY_ELENCO
//----------------------------------------------------------------------------
MY_ELENCO :: MY_ELENCO() : ELENCO() {
   #undef TRCRTN
   #define TRCRTN "MY_ELENCO"
}

//----------------------------------------------------------------------------
// @MY_ELENCO
//----------------------------------------------------------------------------
MY_ELENCO::MY_ELENCO( MY_ELENCO & roElenco ) : ELENCO( ( ELENCO & ) roElenco ) {
   #undef TRCRTN
   #define TRCRTN "@MY_ELENCO"
}

//----------------------------------------------------------------------------
// MY_ELENCO::Sort
//----------------------------------------------------------------------------
MY_ELENCO & MY_ELENCO::Sort( PFN_QSORT * Compare, size_t ElementSize ) {
   #undef TRCRTN
   #define TRCRTN "MY_ELENCO::Sort"
   if( Compare == NULL || ! NumPVOID )
      return THIS;
   
   MY_ELENCO Temp( THIS );
   
   qsort( Temp.ArrayPVOID, Temp.NumPVOID, ElementSize, Compare );
   
   THIS = Temp;
   return THIS;
}

//----------------------------------------------------------------------------
// InizializzaListeDecodifica
//----------------------------------------------------------------------------
int InizializzaListeDecodifica(LISTE_DECODIFICA * poListe) {
   #undef TRCRTN
   #define TRCRTN "InizializzaListeDecodifica"
   int iRet = 1;
   poListe->oElDecTipoMezzoViagg.Clear();
   poListe->oElDecClasse.Clear();
   poListe->oElDecClassifica.Clear();
   poListe->oElDecFermate.Clear();
   
   ULONG ulNumRecord = 0;
   STRINGA oRiga;
   int iNumeroListaInLettura;
   
   FILE_DECODIFICA * poFileDecodifica = new FILE_DECODIFICA( (STRINGA) "DECOD.DAT" );
   
   if (poFileDecodifica) {
      ULONG ulNumRec = poFileDecodifica->NumRecordsTotali();
      
      for (; ulNumRecord < ulNumRec ; ) {
         poFileDecodifica->sDecodifica = (*poFileDecodifica)[ulNumRecord];
         ulNumRecord++;
         
         oRiga = St(poFileDecodifica->sDecodifica.DescrizioneDecodifica);
         
         if (! (oRiga(0,1) == (STRINGA) "//") )
            {
            if (oRiga(0,1) == (STRINGA) "##") {
               if (oRiga.Pos((STRINGA)"## CODICE_TIPO_MEZZO_VIAGGIANTE") > -1) {
                  iNumeroListaInLettura = 1;
               } else if (oRiga.Pos((STRINGA)"## CLASSE") > -1) {
                  iNumeroListaInLettura = 2;
               } else if (oRiga.Pos((STRINGA)"## CLASSIFICA") > -1) {
                  iNumeroListaInLettura = 3;
               } else if (oRiga.Pos((STRINGA)"## CODICE_TIPO_FERMATA") > -1) {
                  iNumeroListaInLettura = 4;
               }
            } else if (oRiga(0,1) == (STRINGA) "%%") {
               if (oRiga.Pos((STRINGA)"%% DataInizio") > -1) {
                  poListe->oDataInizio = St(poFileDecodifica->sDecodifica.Valore_Sipax);
               } else if (oRiga.Pos((STRINGA)"%% DataFine") > -1) {
                  poListe->oDataFine = St(poFileDecodifica->sDecodifica.Valore_Sipax);
               } else if (oRiga.Pos((STRINGA)"%% CodiceDistribuzione") > -1) {
                  poListe->oCodiceDistribuzione = St(poFileDecodifica->sDecodifica.Valore_Sipax);
               }
            } else {
               oRiga += (STRINGA) poFileDecodifica->sDecodifica.SeparatoreInizio;
               oRiga += St(poFileDecodifica->sDecodifica.Valore_File_T);
               oRiga += (STRINGA) poFileDecodifica->sDecodifica.SeparatoreFine;
               oRiga += (STRINGA) poFileDecodifica->sDecodifica.SeparatoreInizio1;
               oRiga += St(poFileDecodifica->sDecodifica.Valore_Sipax);
               oRiga += (STRINGA) poFileDecodifica->sDecodifica.SeparatoreFine1;
               iRet = 0;
               
               if (iNumeroListaInLettura == 1) {
                  poListe->oElDecTipoMezzoViagg += oRiga;
               } else if (iNumeroListaInLettura == 2) {
                  poListe->oElDecClasse += oRiga;
               } else if (iNumeroListaInLettura == 3) {
                  poListe->oElDecClassifica += oRiga;
               } else if (iNumeroListaInLettura == 4) {
                  poListe->oElDecFermate += oRiga;
               }
               
//<<<       if  oRiga 0,1  ==  STRINGA  "##"   
            }
//<<<    if  !  oRiga 0,1  ==  STRINGA  "//"   
         }
//<<< for  ; ulNumRecord < ulNumRec ;    
      }
      
      delete poFileDecodifica;
//<<< if  poFileDecodifica   
   }
   
   return iRet;
//<<< int InizializzaListeDecodifica LISTE_DECODIFICA * poListe   
}

//----------------------------------------------------------------------------
// DecodTipoMezzoViaggiante
//----------------------------------------------------------------------------
void DecodTipoMezzoViaggiante( LISTE_DECODIFICA * psListeDecodifica, STRINGA & roNome, char & roValore) {
   #undef TRCRTN
   #define TRCRTN "DecodTipoMezzoViaggiante"
   roValore = ' ';
   STRINGA oLinea;
   
   FORALL (psListeDecodifica->oElDecTipoMezzoViagg, iCont)
      {
      oLinea = psListeDecodifica->oElDecTipoMezzoViagg[iCont](0,49);
      oLinea.Strip();
      
      if (oLinea == roNome) {
         roValore = (psListeDecodifica->oElDecTipoMezzoViagg[iCont])[63];
         break;
      }
   }
   
   return;
}

//----------------------------------------------------------------------------
// DecodClasse
//----------------------------------------------------------------------------
void DecodClasse( LISTE_DECODIFICA * psListeDecodifica, STRINGA & roNome, char & roValore) {
   #undef TRCRTN
   #define TRCRTN "DecodClasse"
   roValore = ' ';
   STRINGA oLinea;
   
   FORALL (psListeDecodifica->oElDecClasse, iCont)
      {
      oLinea = psListeDecodifica->oElDecClasse[iCont](0,49);
      oLinea.Strip();
      
      if (oLinea == roNome) {
         roValore = (psListeDecodifica->oElDecClasse[iCont])[63];
         break;
      }
   }
   
   return;
}

//----------------------------------------------------------------------------
// DecodClassifica
//----------------------------------------------------------------------------
void DecodClassifica( LISTE_DECODIFICA * psListeDecodifica, int iValore, char * pcCodice) {
   #undef TRCRTN
   #define TRCRTN "DecodClassifica"
   STRINGA oLinea;
   
   FORALL (psListeDecodifica->oElDecClassifica, iCont)
      {
      oLinea = psListeDecodifica->oElDecClassifica[iCont](51,60);
      
      if (oLinea.ToInt() == iValore) {
         memcpy(pcCodice, (const char *) psListeDecodifica->oElDecClassifica[iCont](63,64), 2);
         break;
      }
   }
   
   return;
}

//----------------------------------------------------------------------------
// DecodTipoFermata
//----------------------------------------------------------------------------
void DecodTipoFermata( LISTE_DECODIFICA * psListeDecodifica, STRINGA & roNome, char & roValore) {
   #undef TRCRTN
   #define TRCRTN "DecodTipoFermata"
   roValore = ' ';
   STRINGA oLinea;
   
   FORALL (psListeDecodifica->oElDecFermate, iCont)
      {
      oLinea = psListeDecodifica->oElDecFermate[iCont](0,49);
      oLinea.Strip();
      
      if (oLinea == roNome) {
         roValore = (psListeDecodifica->oElDecFermate[iCont])[63];
         break;
      }
   }
   
   return;
}

// ----------------------------------------------------------------------------
// ----------------------- DecodificaMezzoVirtuale ----------------------------
// ----------------------------------------------------------------------------
int DecodificaMezzoVirtuale( WORD MezzoDaMotore,
   ELENCO & roElencoRighe,
   char * poNumeroMezzoVirtuale,
   char * poProgressivoMezzoVirtuale )
{
   #undef TRCRTN
   #define TRCRTN "DecodificaMezzoVirtuale"
   int iRet = 0;
   int iTrovato = 0;
   
   ORD_FORALL( roElencoRighe, iCont )
   {
      MEZZO_VIRTUALE_APPO * poMezzo = NULL;
      
      poMezzo = ( MEZZO_VIRTUALE_APPO * )roElencoRighe[ iCont ];
      
      //      printf( "%d,%d,%d,%d\n", poMezzo -> iMarked, poMezzo -> iNumeroVirtuale, poMezzo -> iPrgVirtuale, poMezzo -> sMezzoVirtuale.MezzoVirtuale );
      
      if( poMezzo -> sMezzoVirtuale.MezzoVirtuale == MezzoDaMotore ) {
         char szAppo[ 6 ];
         
         sprintf( szAppo, "%05d", poMezzo -> iNumeroVirtuale );
         memcpy( poNumeroMezzoVirtuale, szAppo, 5 );
         sprintf( szAppo, "%02d", poMezzo -> iPrgVirtuale );
         memcpy( poProgressivoMezzoVirtuale, szAppo, 2 );
         iTrovato = 1;
         break;
      }
   }
   if( ! iTrovato ) {
      //      printf( "* ATTENZIONE * Non sono riuscito a convertire un Numero Mezzo Virtuale !!!\n" );
      //      printf( "* ---------- * MezzoDaMotore = %d\n", MezzoDaMotore );
      //      printf( "* ---------- * roElencoRighe.Dim() = %ld\n", ( long )roElencoRighe.Dim() );
      //      printf( "* ---------- * iCont = %d\n", iCont );
      iRet = 1;
   }
   
   return iRet;
//<<<    char * poProgressivoMezzoVirtuale  
}

