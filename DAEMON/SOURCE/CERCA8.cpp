//   ********************************************************************************************    //
//   *****  MODULO DI RICERCA DELLE SOLUZIONI RICHIESTE : CERCA8.CPP                        *****    //
//   ********************************************************************************************    //
//   Contiene le seguenti funzioni                                                                   //
//   int Cerca( MM_RETE_FS *, ID, ID , int , int , int , int , int )                                 //
//   int Timetoi(char )                                                                              //
//   void SortSoluzioni(PROBLEMA , int )                                                             //
//                                                                                                   //
//                                                                                                   //
//                                                                                                   //
//   ********************************************************************************************    //
// Se definita abilita la trace di OOLIB
// #define OUTDEVSCR

int Cerca( MM_RETE_FS * pRete,ID IdStIn, ID IdStOut, int gg, int mm, int aaaa, int hh, int mn )
{
   // riceve in input ID della stazione di partenza, di arrivo, la data e l'ora e restituisce il numero di soluzioni
   // riempie la struttura PROBLEMA (vedi file frmtstru.h)

   #undef TRCRTN
   #define TRCRTN "Cerca"

   SDATA data;
   char Buf[500];
   ULONG TotPerc=0;
   ULONG TotSol =0;
   int fEsCambi=0;
   int CntCambi;

   //dichiaro una var di tipo Rete e la faccio puntare all' indirizzo
   // in pRete
   MM_RETE_FS & Rete = * pRete;

   // .....................................................................................
   // Modifica montagna
   // .....................................................................................
   // Per motivi di performance l' archivio stazioni NON deve essere aperto ogni volta
   // Inoltre e' opportuno che venga caricato tutto in memoria:
   // Si prega di gestire nel main come sopra indicato,  ed eventualmente di utilizzare un extern
   // in eventuali altri moduli compilati separatamente

   // extern STAZIONI * PStazioni; // Se sono in un' altro sorgente rispetto al MAIN
   STAZIONI & Stazioni = *PStazioni;

   MM_ELENCO_PERCORSO_E_SOLUZIONI * percSol=(MM_ELENCO_PERCORSO_E_SOLUZIONI*) NULL;

      data.Anno  = aaaa ;
      data.Giorno=   gg ;
      data.Mese  =   mm ;
      Ora = 60*hh+mn;
      ID Da=IdStIn;
      ID A =IdStOut;

      TRACEVLONG(Da);
      TRACEVLONG(A);
      Dprintf("ID Da=%d, A=%d, Anno=%d, Giorno=%d, Mese=%d",Da,A,data.Anno,data.Giorno,data.Mese);

      TRACERESET;
      TRACESTRING("Inizio ricerca sol");
      Dprintf("Inizio ricerca sol");
      ARRAY_ID StazioniDiCambio;

      percSol =(MM_ELENCO_PERCORSO_E_SOLUZIONI*) NULL;

      percSol = Rete.RichiestaPerRelazione(Da,A,PER_BIGLIETTARE_MA_USO_MOTORE_E_POLIMETRICHE,data,Ora);
      Dprintf("Esito Rete.RichiestaPerRelazione percSol=%x", percSol);
      // lancia ricerca ..................................
      MM_ELENCO_PERCORSO_E_SOLUZIONI & PercSol = * percSol;
      if (!(&PercSol))
      {
         #ifdef OUTDEVSCR
           sprintf(Buf,"Richiesta non Valida\n");
           PUTS(Buf);
         #endif

         TRACESTRING("=========================================");
         return 0;

      } else {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // Qui OCCORRE STRUTTURARE MEGLIO IL CODICE
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        // La query ha prodotto risultati validi. Instanzio le strutture
        // necessarie a contenere i dati di out.

        /********* Settaggio dati str. Problema*******************/

        sprintf(Problema.Da,"%s",Stazioni.DecodificaIdStazione(Da));
        sprintf(Problema.A,"%s",Stazioni.DecodificaIdStazione(A));
        Problema.giorno=data.Giorno;
        Problema.mese=data.Mese;
        Problema.anno=data.Anno;
        Problema.hh_in=hh;
        Problema.mm_in=mn;

        #ifdef OUTDEVSCR
           sprintf(Buf,"[Informazioni Generali Soluzione]");
           PUTS(Buf);
           sprintf(Buf,"Data = %s" ,(CPSZ)STRINGA(data) );
           PUTS(Buf);
           sprintf(Buf,"Ora = %i:%i" ,hh,mn );
           PUTS(Buf);
           sprintf(Buf,"Stazione P. = %s",Stazioni.DecodificaIdStazione(Da));
           PUTS(Buf);
           sprintf(Buf,"Stazione A. = %s",Stazioni.DecodificaIdStazione(A));
           PUTS(Buf);
        #endif

         int Num = 0;
//         ORD_FORALL(PercSol,it)Num +=PercSol[it]->Soluzioni->Dim();
         ORD_FORALL(PercSol,it){
         Num +=PercSol[it]->Soluzioni->Dim();
         }

        #ifdef OUTDEVSCR
           sprintf(Buf,"Num. Instradamenti = %i ",PercSol.Dim());
           PUTS(Buf);
           sprintf(Buf,"Num. Soluzioni = %i ",Num);
           PUTS(Buf);
        #endif

         // E' il numero totale di soluzioni che il motore estrae;
         // serve ad Aldo per formattare in MakeHtml.CPP
         Problema.NumSoluzioniInRange = Num;

         TotPerc += PercSol.Dim(); TotSol += Num;


         Problema.NumInstradamenti=PercSol.Dim();
         Problema.NumSoluzioni = min(Num,MAXSOL);

         // Allocazione spazio mem. per contenere le strutture dati
         TRCbegin " Problema.NumInstradamenti=%d, Problema.NumSoluzioni = %d\n",
                    Problema.NumInstradamenti, Problema.NumSoluzioni  TRCend
         TRCbegin " sizeof(INSTRADAMENTO)=%d, sizeof(INFOSOLUZIONE)=%d ",
                    sizeof(INSTRADAMENTO), sizeof(INFOSOLUZIONE)  TRCend

         Problema.Instradamenti=(INSTRADAMENTO *)malloc(sizeof(INSTRADAMENTO)*Problema.NumInstradamenti);
         Problema.InfoSoluzioni=(INFOSOLUZIONE *)malloc(sizeof(INFOSOLUZIONE)*Problema.NumSoluzioni);

         if (Problema.Instradamenti==NULL || Problema.InfoSoluzioni==NULL)
         {

          Dprintf("Errore di Allocazione Memoria 1");
          ExitProcess(1);//exit(1); EMS
         }
         // Fine Settaggio dati str. Problema - escluse strutture dinamiche


         MM_ELENCO_SOLUZIONE Sols;

         #ifdef OUTDEVSCR
            sprintf(Buf,"[Instradamenti]");
            PUTS(Buf);
         #endif

         ORD_FORALL(PercSol,is)
         {

            MM_I_PERCORSO & Perc = * PercSol[is]->Percorso;

            sprintf(((Problema.Instradamenti)+is)->Da,"%7s",GRAFO::Gr()[Perc.FormaStandard[0]].Nome7() );
            sprintf(((Problema.Instradamenti)+is)->A,"%7s",GRAFO::Gr()[Perc.FormaStandard.Last()].Nome7() );


            sprintf( ((Problema.Instradamenti)+is)->Via,"%s",(CPSZ)Perc.FormaStandardStampabile );
            //TRACESTRING(STRINGA(((Problema.Instradamenti)+is)->Via)+"-"+STRINGA((CPSZ)Perc.FormaStandardStampabile) );

            //((Problema.Instradamenti)+is)->Km=Perc.DatiTariffazione.Lunghezza();
            ((Problema.Instradamenti)+is)->Km=Perc.DatiTariffazione.KmReali+Perc.DatiTariffazione.KmConcessi1+Perc.DatiTariffazione.KmConcessi2;


          #ifdef OUTDEVSCR
             sprintf(Buf,"Instradamento = %i",is );
             PUTS(Buf);
             sprintf(Buf,"Da = %s",GRAFO::Gr()[Perc.FormaStandard[0]].Nome7() );
             PUTS(Buf);
             sprintf(Buf,"A  = %s",GRAFO::Gr()[Perc.FormaStandard.Last()].Nome7() );
             PUTS(Buf);
             sprintf(Buf,"Km = %i",((Problema.Instradamenti)+is)->Km);
             PUTS(Buf);
             sprintf(Buf,"Via= %s",(CPSZ)Perc.FormaStandardStampabile);
             PUTS(Buf);
          #endif
            // fine stampa instradamenti


            if(fDettagliTariffe)
            {
               //PUTS("DETTAGLI TARIFFE:");
               // EMS fflush(stdout);
               // EMS Perc.PrintPercorso(stdout);
               fflush(Prn);
               if(Print)Perc.PrintPercorso(Prn);

               // Sul trace anche i dati di tariffazione
               MM_SOLUZIONE * Sol = (*PercSol[is]->Soluzioni)[0];
               DATI_TARIFFAZIONE_2 Dt = Sol->KmParziali(0,Sol->NumeroTratte - 1 ,&Perc);
               // Fa da solo il trace
            }

            MM_ELENCO_SOLUZIONE & Soluzioni = * PercSol[is]->Soluzioni;
            ORD_FORALL(Soluzioni,so)
            {
               SOLUZIONE * Sol = (SOLUZIONE*) Soluzioni[so];
               Sol->Percorso = is; // Punta al percorso
               Soluzioni[so] = NULL;
               Sol->Ordine = TempoTrascorso(Ora,Sol->OraPartenza());
               Sols += Sol;

            };

//<<<    ORD_FORALL PercSol,is
         };

         if(Sols.Dim())qsort(&Sols[0],Sols.Dim(),sizeof(void*),MM_sort_function1);

         #ifdef OUTDEVSCR
           sprintf(Buf,"[Soluzioni]");
           PUTS(Buf);
         #endif

         int iCntSol=0;

        ORD_FORALL(Sols,i2)
        {
         if (i2<Problema.NumSoluzioni)
         {

           iCntSol++;

         #ifdef OUTDEVSCR
           sprintf(Buf,"\nSoluzione num = %i",iCntSol);
           PUTS(Buf);
         #endif

           SOLUZIONE * Sol = (SOLUZIONE*) Sols[i2];
           MM_I_PERCORSO & Perc = * PercSol[Sol->Percorso]->Percorso;
           char * LastIdent = Sol->IdentPartenza;

           // verifica che la soluzione parta entro il limite max imposto

         #ifdef OUTDEVSCR
            sprintf(Buf,"Partenza ore = %2i:%2i",Sol->OraPartenza()/60, Sol->OraPartenza()%60);
            PUTS(Buf);
            sprintf(Buf,"Arrivo ore = %2i:%2i",Sol->OraArrivo()/60, Sol->OraArrivo()%60);
            PUTS(Buf);
            sprintf(Buf,"Periodicita = %s",Sol->Per.Circolazione_STRINGA());
            PUTS(Buf);
            sprintf(Buf,"Stazione P. = %s",GRAFO::Gr()[Perc.FormaStandard[0]].Nome7());
            PUTS(Buf);
            sprintf(Buf,"Stazione A. = %s",GRAFO::Gr()[Perc.FormaStandard.Last()].Nome7());
            PUTS(Buf);
            sprintf(Buf,"Km = %i",Perc.DatiTariffazione.Lunghezza());
            PUTS(Buf);
            sprintf(Buf,"Via = %s",(CPSZ)Perc.FormaStandardStampabile );
            PUTS(Buf);
         #endif

            STRINGA Ora1(ORA(Sol->TempoTotaleDiPercorrenza));
            STRINGA Ora2(ORA(Sol->TempoTotaleDiAttesa));
            STRINGA Clusters;

            //Riempimento struttura INFOSOLUZIONE della struttura PROBLEMA
            ((Problema.InfoSoluzioni)+i2)->NumSoluzione=i2;

            sprintf(((Problema.InfoSoluzioni)+i2)->Da,"%7s",GRAFO::Gr()[Perc.FormaStandard[0]].Nome7() );
            sprintf(((Problema.InfoSoluzioni)+i2)->A,"%7s",GRAFO::Gr()[Perc.FormaStandard.Last()].Nome7() );
            sprintf( ((Problema.InfoSoluzioni)+i2)->Via,"%s",(CPSZ)Perc.FormaStandardStampabile);

            TRACESTRING("VIA IN CICLO SOLUZIONI :"+ STRINGA(((Problema.InfoSoluzioni)+i2)->Via));

//            ((Problema.InfoSoluzioni)+i2)->Km=Perc.DatiTariffazione.Lunghezza();
            ((Problema.InfoSoluzioni)+i2)->Km=Perc.DatiTariffazione.KmReali+Perc.DatiTariffazione.KmConcessi1+Perc.DatiTariffazione.KmConcessi2;
            sprintf( ((Problema.InfoSoluzioni)+i2)->Periodicita,"%s",Sol->Per.Circolazione_STRINGA());

            sprintf(((Problema.InfoSoluzioni)+i2)->TTot,"%s",(CPSZ)Ora1);
            sprintf(((Problema.InfoSoluzioni)+i2)->TAtt,"%s",(CPSZ)Ora2);


            sprintf(((Problema.InfoSoluzioni)+i2)->Parte,"%02i:%02i",Sol->OraPartenza()/60, Sol->OraPartenza()%60);
            sprintf(((Problema.InfoSoluzioni)+i2)->Arriva,"%02i:%02i",Sol->OraArrivo()/60, Sol->OraArrivo()%60);


            for (int z = 0;z < Sol->NumeroTratte ;z++ )
            {
               SOLUZIONE::TRATTA_TRENO & Tratta = (SOLUZIONE::TRATTA_TRENO &)Sol->Tratte[z];
               if(!Tratta.Concorde)Clusters += "#";
               Clusters += STRINGA(Tratta.IdCluster) + " ";
            };

         #ifdef OUTDEVSCR
            sprintf(Buf,"Tempo Totale = %s",(CPSZ)Ora1 );
            PUTS(Buf);
            sprintf(Buf,"Attesa = %s",(CPSZ)Ora2 );
            PUTS(Buf);
         #endif

            Sols[i2]->GetNote();

            //Allocazione Memoria per struttura INFOTRATTA di INFOSOLUZIONE

            ((Problema.InfoSoluzioni)+i2)->InfoTratte=(INFOTRATTA *)malloc(sizeof(INFOTRATTA)*Sol->NumeroTratte);

            if (((Problema.InfoSoluzioni)+i2)->InfoTratte==NULL )
            {
             Dprintf("\n Errore di Allocazione Memoria 2\n");
             exit(1);
            }
            //Fine Allocazione Memoria per struttura INFOTRATTA di INFOSOLUZIONE

            ((Problema.InfoSoluzioni)+i2)->TUP=0;
            ((Problema.InfoSoluzioni)+i2)->TUA=0;

            for (int i = 0;i < Sol->NumeroTratte ;i++ )
            {
               int ctc;
               MM_SOLUZIONE::TRATTA_TRENO & Tratta = (MM_SOLUZIONE::TRATTA_TRENO &)Sol->Tratte[i];

               if(!StazioniDiCambio.Contiene(Tratta.IdStazioneIn))StazioniDiCambio += Tratta.IdStazioneIn;
               if(!StazioniDiCambio.Contiene(Tratta.IdStazioneOut))StazioniDiCambio += Tratta.IdStazioneOut;


               STRINGA Ora1(ORA(Tratta.OraIn));
               STRINGA Ora2(ORA(Tratta.OraOut));
               STRINGA NomeTreno=*Tratta.NomeSpecifico;


         #ifdef OUTDEVSCR
               sprintf(Buf,"\nMV = %i",Tratta.IdMezzoVirtuale);
               PUTS(Buf);
               sprintf(Buf,"Tratta = %2i",i+1);
               PUTS(Buf);
               sprintf(Buf,"ID Treno = %5s" ,Tratta.IdTreno);
               PUTS(Buf);
               sprintf(Buf,"Nome Treno =%s",(CPSZ)NomeTreno);
               PUTS(Buf);
               sprintf(Buf,"Tipo Treno = %s" ,(CPSZ)MM_INFO::DecodTipoMezzo(Tratta.InfoTreno.TipoMezzo));
               PUTS(Buf);
               sprintf(Buf,"Staz. in. = %s " ,LastIdent);
               PUTS(Buf);
               sprintf(Buf,"Ora in. = %s " ,(CPSZ)Ora1);
               PUTS(Buf);
               sprintf(Buf,"Staz. fine = %s " ,Tratta.IdentOut);
               PUTS(Buf);
               sprintf(Buf,"Ora fine = %s " ,(CPSZ)Ora2);
               PUTS(Buf);
               sprintf(Buf,"Prenotaz. = %s " ,(Tratta.InfoTreno.Prenotabilita ?  "Prenotabile" : "Non Prenotabile"));
               PUTS(Buf);
         #endif

               /*****************************************************************************/
               //Riempimento struttura INFOTRATTA della struttura INFOSOLUZIONE corrente
               /*****************************************************************************/
               ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->NumTratta=i;

               sprintf( ((((Problema.InfoSoluzioni)+i2) ->InfoTratte)+i)->NomeTreno,"%s",(CPSZ)NomeTreno);
               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->IDTreno,"%5s",Tratta.IdTreno);
               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->TipoTreno,"%s",(CPSZ)MM_INFO::DecodTipoMezzo(Tratta.InfoTreno.TipoMezzo));
               ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->MV=Tratta.IdMezzoVirtuale;

               /* gestione partenza da tratta urbana  francesco  8/05/97
               if (Tratta.IdMezzoVirtuale==0)
                {
                 if (i==0)  ((Problema.InfoSoluzioni)+i2)->TUP=1;

         #ifdef OUTDEVSCR
                   sprintf(Buf,"TUP = %i " ,((Problema.InfoSoluzioni)+i2)->TUP);
                   PUTS(Buf);
         #endif

                 if (i==((Sol->NumeroTratte)-1)) ((Problema.InfoSoluzioni)+i2)->TUA=1;

                }


                fine gestione tratte urbane */

               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->Parte,"%s",(CPSZ)Ora1);
               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->Arriva,"%s",(CPSZ)Ora2);
               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->Da,"%s",LastIdent);
               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->A,"%s",Tratta.IdentOut);

               /**************************************************************************/
               // gestione nomi lunghi di stazione per la tratta
               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->FullDa,"%s",Stazioni.DecodificaIdStazione(Tratta.IdStazioneIn));
               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->FullA,"%s",Stazioni.DecodificaIdStazione(Tratta.IdStazioneOut));

               /**************************************************************************/


               sprintf( ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+i)->Prenotazione,"%s",(Tratta.InfoTreno.Prenotabilita ?  "Prenotabile" : "Non Prenotabile"));
               /*****************************************************************************/
               // fine riempimento dati base struttura di tratta
               /*****************************************************************************/
               ctc=i;

               if (fTratta) {  // Dettagli di tratta

         #ifdef OUTDEVSCR
                  sprintf(Buf,"[Dettaglio Treno]");
                  PUTS(Buf);
                  sprintf(Buf,"Treno = %s ",(CPSZ)NomeTreno);
                  PUTS(Buf);
                  sprintf(Buf,"ID Treno = %s ",Tratta.IdTreno);
                  PUTS(Buf);
         #endif

                  int j;
                  if (fServizi)
                  {
                     DWORD dServizi = *(DWORD*)&Tratta.InfoTreno;
                     BOOL HoOut = FALSE;

         #ifdef OUTDEVSCR
                     sprintf(Buf,"[Servizi Treno]");
                     PUTS(Buf);
         #endif
                     j=0;
                     int step=LENNOMESRV;
                     char * ListaServiziTreno;
                     ListaServiziTreno=(char *)malloc(sizeof(char));

                     if(ListaServiziTreno==NULL)
                     {
                        printf("\n Errore di Allocazione Memoria 3\n");
                        exit(1);
                     }
                     for (int i = 0;i < 32  ;i ++ )
                     {
                        if(((1 << i) & dServizi) && NomiServizi[i][0] != '#')
                        {
                           HoOut = TRUE;
                           j++;

         #ifdef OUTDEVSCR
                           sprintf(Buf,"Servizio = %s ",(CPSZ)NomiServizi[i]);
                           PUTS(Buf);
         #endif

                           ListaServiziTreno=(char *)realloc(ListaServiziTreno,(sizeof(char)*step)*j);
                           if(ListaServiziTreno==NULL)
                           {
                            printf("\n Errore di Allocazione Memoria 3 - %i\n",j);
                            exit(1);
                           }
                           strcpy((ListaServiziTreno+step*(j-1)),(CPSZ)NomiServizi[i]);
                        };
                     } /* endfor */

                     ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+ctc)->NumServiziTreno=j;
                     if (HoOut)
                     {

         #ifdef OUTDEVSCR
                       sprintf(Buf,"Totale Servizi Treno = %i",j);
                       PUTS(Buf);
         #endif
                       ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+ctc)->Servizi=ListaServiziTreno;
                     } else {
                      ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+ctc)->NumServiziTreno=0;
                      ((((Problema.InfoSoluzioni)+i2)->InfoTratte)+ctc)->Servizi=NULL;
                     }
                  } /* endif */
               } /* endif */

               LastIdent = Tratta.IdentOut;
            } /* endfor */


            /***************************/
            // servizi per la soluzione
            /***************************/
            if (fServizi)
            {
               DWORD dServizi = *(DWORD*)&Sol->InfoSoluzione;

               BOOL HoOut = FALSE;

               int step=LENNOMESRV;
               char * ListaServizi;
               ListaServizi=(char *)malloc(sizeof(char));

               if(ListaServizi==NULL)
               {
                 printf("\n Errore di Allocazione Memoria 4\n");
                 exit(1);
               }

         #ifdef OUTDEVSCR
               sprintf(Buf,"[Servizi di Tratta]");
               PUTS(Buf);
         #endif

               int k=0;
               for (int i = 0;i < 32  ;i ++ ) {
                  if(((1 << i) & dServizi) && NomiServizi[i][0] != '#'){
                     HoOut = TRUE;
                     k++;

         #ifdef OUTDEVSCR
                     sprintf(Buf,"Servizio = %s ",(CPSZ)NomiServizi[i]);
                     PUTS(Buf);
         #endif

                     ListaServizi=(char *)realloc(ListaServizi,(sizeof(char)*step)*k);
                     if(ListaServizi==NULL)
                     {
                      printf("\n Errore di Allocazione Memoria 4 - %i\n",k);
                      exit(1);
                     }
                     strcpy((ListaServizi+step*(k-1)),NomiServizi[i]);

                  };
               } /* endfor */

               ((Problema.InfoSoluzioni)+i2)->NumServizi=k;

               if(HoOut)
               {
         #ifdef OUTDEVSCR
                  sprintf(Buf,"Totale Servizi Soluzione = %i ",k);
                  PUTS(Buf);
         #endif
                  ((Problema.InfoSoluzioni)+i2)->Servizi=ListaServizi;
               }
               else
               {
                ((Problema.InfoSoluzioni)+i2)->Servizi=NULL;
               }

            } /* endif */
            if(fPeriodicita) {

         #ifdef OUTDEVSCR
               //PUTS("Periodicita' di soluzione:");
               //ELENCO_S Tmp = Sol->Per.Decod(1,1,0);
               //ORD_FORALL(Tmp,i)PUTS((CPSZ)Tmp[i]);
         #endif

            } /* endif */

            if(fNote && Sol->InfoNote){

               ELENCO_S & Note = Sol->InfoNote->Note;
               Sol->InfoNote->NumNoteDiVendita;

               ORD_FORALL(Note,i){
                  if(i < Sol->InfoNote->NumNoteDiVendita){

                     // Commentato da N. Ticconi per limiti di Buf (500)
                     // sprintf(Buf,"%s",(CPSZ)Note[i](0,497));
                     // PUTS(Note[i]);
                  } else {
                     // sprintf(Buf,"%s",(CPSZ)Note[i]);
                     // PUTS(Buf);
                  }
         #ifdef OUTDEVSCR
                  PUTS("[Note]");
                  sprintf(Buf,"%s",(CPSZ)Note[i](0,497));
                  PUTS(Buf);
         #endif
               }


            /********************************************************************************/
            /* Gestione Lunghezza e Durata Complessiva Soluzione nel caso di Tratte urbane  */
            /* in partenza o arrivo.                                                        */
            /* TTot tiene conto delle sole tr. extraurbane!!                                */
            /********************************************************************************/

            int TimeTot,TimePTP,TimeATP,TimeDT,TimeDTT;

            int apptup, apptua, appsca;
            apptup=(Problema.InfoSoluzioni+i2)->TUP;
            apptua=(Problema.InfoSoluzioni+i2)->TUA;

            /*******************************************/
            /* Gestione Tratte Urbane ed Extraurbane   */
            /* NumTratte conta le sole tr. extraurbane */
            /*******************************************/
            ((Problema.InfoSoluzioni)+i2)->NumTratte=(Sol->NumeroTratte)-(apptup+apptua);

            appsca=((((Problema.InfoSoluzioni)+i2)->NumTratte)-1)+apptup;


            TimePTP =Timetoi( ((Problema.InfoSoluzioni+i2)->InfoTratte + apptup )->Parte );
            TimeATP =Timetoi( ((Problema.InfoSoluzioni+i2)->InfoTratte + appsca )->Arriva );

            if (TimeATP>=TimePTP) {TimeDT=TimeATP-TimePTP;
            } else {TimeDT=(1440-TimePTP)+TimeATP;
            } /* endif */

            if (strlen(((Problema.InfoSoluzioni)+i2)->TTot)<6)
            {   // la soluzione ha durata < di 24 h
                sprintf(((Problema.InfoSoluzioni)+i2)->TTot,"%02i:%02i",TimeDT/60,TimeDT%60  );
            } else
            {
                sprintf((((Problema.InfoSoluzioni)+i2)->TTot)+5,"%02i:%02i",TimeDT/60,TimeDT%60  );
            } /* endif */




            /********************************/
            /* Gestione esistenza cambi     */
            /********************************/


            if ( ( (((Problema.InfoSoluzioni)+i2)->NumTratte) >1 ) && (i2<MAXSOL) )
            {
             fEsCambi=1;
            }


            }
        } // chiude l'if sul limite delle soluzioni da considerare
//<<<   ORD_FORALL Sols,i2
         };

         Problema.EsistonoCambi=fEsCambi;
         fEsCambi=0;

      }
      TRACERESTORE;

      // fINE G. Gestione Tratte Urbane


      // Gestione Stazioni di Cambio
      // Allocazione spazio mem. per contenere le strutture dati

        StCambi.Acronimi=(char *)malloc(sizeof(char));
        StCambi.NomiEstesi=(char *)malloc(sizeof(char));
        if (StCambi.Acronimi==NULL || StCambi.NomiEstesi==NULL)
         {
          printf("\n Errore di Allocazione Memoria per Stazioni di Cambio\n");
          exit(1);
         }
        // Fine Allocazione spazio mem. per contenere le strutture dati


      CntCambi=0;

      // Commentato da N. Ticconi e sostituito con TRACESTRING
      // PUTS("[Stazioni di Cambio]");
      TRACESTRING("[Stazioni di Cambio]");

      ORD_FORALL(StazioniDiCambio,s)
      {
         ID Id = StazioniDiCambio[s];

      #ifdef OUTDEVSCR
         sprintf(Buf,
                 "%7s: Codice CCR %5i ID %4i : %s",
                 GRAFO::Gr()[Id].Nome7(),
                 Stazioni[Id].CodiceCCR,
                 Id,
                 Stazioni.RecordCorrente().NomeStazione);
         PUTS(Buf);
      #endif

         StCambi.Acronimi = (char *)realloc(StCambi.Acronimi, sizeof(char)*(LENACRST*(CntCambi+1)) );
         StCambi.NomiEstesi =(char *)realloc(StCambi.NomiEstesi, sizeof(char)*(LENNMSTA*(CntCambi+1)) );

         if (StCambi.Acronimi==NULL)
         {
          printf("\n Errore di Riallocazione Memoria per Acronimi Stazioni di Cambio\n");
          exit(1);
         }
         if (StCambi.NomiEstesi==NULL)
         {
          printf("\n Errore di Riallocazione Memoria per Nomi Estesi Stazioni di Cambio\n");
          exit(1);
         }
         sprintf( (StCambi.Acronimi)+(CntCambi*LENACRST), "%7s",GRAFO::Gr()[Id].Nome7());
         sprintf( (StCambi.NomiEstesi)+(CntCambi*LENNMSTA), "%s",Stazioni.RecordCorrente().NomeStazione);
         CntCambi++;
      };

      StCambi.NumCambi=CntCambi;

      // PUTS("");
      TRACESTRING("");

      // EMS fflush(stdout);
      PROFILER::Trace(TRUE,1);
      if(MostraProfiler)PROFILER::PutStdOut(TRUE);
      PROFILER::Acquire(TRUE)         ; // Somma i dati di parziale in Totale; Opzionalmente azzera Parziale

      delete &PercSol;

      //EMS ?K?K?K?K
      //(&PercSol)=NULL;
      percSol = NULL;

   // EMS fflush(stdout);
   PROFILER::Trace(FALSE,1);
   if(MostraProfiler)PROFILER::PutStdOut(FALSE);

   // PUTS(";Fine Soluzioni");
   TRACESTRING(";Fine Soluzioni");

   void SortSoluzioni(PROBLEMA, int);                               /* mod RS 26/02/97 */
    if (SortMethod >0) SortSoluzioni(Problema, SortMethod);/* mod RS 26/02/97 */

   return Problema.NumSoluzioni;
}
//** fine funzione *****************************************************


//**********************************************************************
// funzione di servizio
//**********************************************************************
// void WebFormatter()
// {
//    int nsrv;
//    int nsrvt;
//    int ns;
//    int nt;
//    int x;
//    int nca;
//    int step;
//    step=LENNOMESRV;
//
//    if(trchse == 0)return ;
//
//    Bprintf("WebFormatter:");
//    Bprintf("Problema.NumInstradamenti= %i",Problema.NumInstradamenti);
//    for(x=0 ;x<Problema.NumInstradamenti;x++)
//    {
//     Bprintf("Da= %s",((Problema.Instradamenti)+x)->Da);
//     Bprintf("A= %s",((Problema.Instradamenti)+x)->A);
//     Bprintf("Via= %s",((Problema.Instradamenti)+x)->Via);
//     Bprintf("Km= %i",((Problema.Instradamenti)+x)->Km);
//    }
//    Bprintf("");
//
//    for(ns=0;ns<Problema.NumSoluzioni;ns++)
//    {
//         Bprintf("Num. Sol.= %i",((Problema.InfoSoluzioni)+ns)->NumSoluzione);
//         Bprintf("Parte: %s",((Problema.InfoSoluzioni)+ns)->Parte);
//         Bprintf("Arriva: %s",((Problema.InfoSoluzioni)+ns)->Arriva);
//         Bprintf("Da= %s",((Problema.InfoSoluzioni)+ns)->Da);
//         Bprintf("A= %s",((Problema.InfoSoluzioni)+ns)->A);
//         Bprintf("Via= %s",((Problema.InfoSoluzioni)+ns)->Via);
//         Bprintf("Km= %i",((Problema.InfoSoluzioni)+ns)->Km);
//         Bprintf("Period.= %s",((Problema.InfoSoluzioni)+ns)->Periodicita);
//         Bprintf("Tempo Totale= %s",((Problema.InfoSoluzioni)+ns)->TTot);
//         Bprintf("Tempo Attesa= %s",((Problema.InfoSoluzioni)+ns)->TAtt);
//
//
//      Bprintf("Num. Servizi Soluzione: %i",((Problema.InfoSoluzioni)+ns)->NumServizi);
//      for(nsrv=0;nsrv<((Problema.InfoSoluzioni)+ns)->NumServizi;nsrv++)
//      {
//        Bprintf("Servizio della Soluzione: %s",(((Problema.InfoSoluzioni)+ns)->Servizi)+(step*nsrv) );
//      }
//
//         Bprintf("*********************Informazioni di dettaglio sulle tratte*******************************");
//         Bprintf("Numero Tratte extraurbane= %i",((Problema.InfoSoluzioni)+ns)->NumTratte);
//         Bprintf("Tratta di Partenza Urbana= %i",((Problema.InfoSoluzioni)+ns)->TUP);
//         Bprintf("Tratta di Arrivo Urbana  = %i",((Problema.InfoSoluzioni)+ns)->TUA);
//
//    for(nt=0 ;nt<((Problema.InfoSoluzioni)+ns)->NumTratte;nt++)
//     {
//       Bprintf("Nome Treno: %s",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->NomeTreno);
//       Bprintf("Tipo Treno: %s",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->TipoTreno);
//       Bprintf("IDTreno= %s",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->IDTreno);
//       Bprintf("Tratta: %i",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->NumTratta);
//       Bprintf("Da= %s",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->Da);
//       Bprintf("A = %s",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->A);
//       Bprintf("Prenotazione: %s",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->Prenotazione);
//       Bprintf("Parte : %s",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->Parte);
//       Bprintf("Arriva: %s",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->Arriva);
//       Bprintf("MV    : %i",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->MV);
//
//       Bprintf("Num. Servizi Treno= %i",(((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->NumServiziTreno);
//
//       for(nsrvt=0;nsrvt < (((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->NumServiziTreno;nsrvt++)
//       {
//        Bprintf("Servizio del treno: %s", ((((Problema.InfoSoluzioni)+ns)->InfoTratte+nt)->Servizi)+(step*nsrvt) );
//       }
//     }
//    }
//    Bprintf("Stazioni di cambio");
//    for(nca=0;nca < StCambi.NumCambi ;nca++)
//    {
//     Bprintf("Acronimo: %s",(StCambi.Acronimi)+(nca*LENACRST) );
//     Bprintf("Stazione: %s",(StCambi.NomiEstesi)+(nca*LENNMSTA) );
//    }
//
//    Bprintf("WebFormatter:FINE");
//
// }



int Timetoi(char ora[6])
{
   int hh, mm;

   char appora[6];
   char *p;

   sprintf(appora,"%s",ora);

   p=strtok(appora,":");
   hh=atoi(p);
   p=strtok(NULL,":");
   mm=atoi(p);

   return hh*60 + mm;
}

void SortSoluzioni(PROBLEMA p, int SortType)
{
  int *tm_nc;                   /*ordinamento per tempo di percorrenza o num. cambi*/
  char *pg, *ph, *t, *ptt;
  int i, j, gap;
  INFOSOLUZIONE tis;
  int tt;

  if (p.NumSoluzioni < 2) return;

  tm_nc = (int *)malloc(p.NumSoluzioni*sizeof(int));
  for (i=0; i<p.NumSoluzioni; i++) {
     tm_nc[i] = 0;
     if (SortType == 1) {                    /* richiesta soluzioni ordinate per tempo di percorrenza */
        ptt = (char *)malloc(1+strlen((p.InfoSoluzioni+i)->TTot));
        strcpy(ptt, (p.InfoSoluzioni+i)->TTot);
        pg = strstr(ptt, " gg ");
        if (NULL != pg) {
          tm_nc[i] = atoi(ptt) * 1440;
          ph = pg + 4;
        }
        else {
          ph = ptt;
        }
        t = strtok(ph,":");
        tm_nc[i] += 60 * atoi(t);
        t = strtok(NULL,":");
        tm_nc[i] += atoi(t);
        free(ptt);
     } else {                                    /* (SortType == 2)richiesta soluzioni ordinate per numero di cambi */
        if (p.EsistonoCambi) {
           tm_nc[i] = (p.InfoSoluzioni+i)->NumTratte;    /*vd. gestione cambi in Cerca() */
        } /* endif */
     } /* endif */
  } /* endifor */

  for (gap = p.NumSoluzioni; gap > 0; gap /= 2){
     for (i = gap; i < p.NumSoluzioni; i++){
        for (j = i-gap; j >= 0 && tm_nc[j] > tm_nc[j+gap]; j -= gap) {
           tt = tm_nc[j];
           tm_nc[j] = tm_nc[j+gap];
           tm_nc[j+gap] = tt;
           tis = (p.InfoSoluzioni)[j];
           (p.InfoSoluzioni)[j] = (p.InfoSoluzioni)[j + gap];
           (p.InfoSoluzioni)[j + gap] = tis;
        }/*endfor*/
     }/*endfor*/
  }/*endfor*/
  free(tm_nc);
  return;
}
