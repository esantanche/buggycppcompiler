/********************************************************************************/
/* PER ALDO: Variabili da usare per gestire i dettagli                          */
/* precedente e successivo:  TotaleSoluzioni (1-10) e SoluzioneCorrente (1-10)  */
/********************************************************************************/
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

//Da inserire la #define con pOutDett al posto di pOut;
//In dmnquery.cpp inserire il prototipo di questa funzione

// begin EMS prove
//#define ScaricaBufferDett ipcServerNextWrite(pOut, strlen(pOut)); *pOut=0;
//#define ScaricaBufferDett TRACESTRING(pOut); *pOut=0;
#define ScaricaBufferDett  ipcServerNextWrite(pOut, strlen(pOut));     \
   TRACESTRING(STRINGA(pOut));   \
   Ftrace = fopen("D:\\EMS\\DAEMON\\EXE\\TRACEDAE.TRC","a");              \
   fprintf(Ftrace,"[DT]%s\n",pOut);                \
   fclose(Ftrace); *pOut=0;
// end EMS

//#define ScaricaBufferDett fprintf(fhtml, pOut); *pOut=0;

void AsDHtml(int isoluz)
{

   #undef TRCRTN
   #define TRCRTN "AsDHtml"

   TRACESTRING("Inizio AsDHtml");

   char bufint[20];
   // begin EMS
   #define FieldWriteString(a,b) strcat((a),(b));strcat((a),"\1")
   #define FieldWriteInt(a,b) sprintf(bufint,"%d",(b));strcat((a),bufint);strcat((a),"\1")
   //#define FieldWriteString(a,b) strcat((a),(b));strcat((a),"-")
   //#define FieldWriteInt(a,b) sprintf(bufint,"%d",(b));strcat((a),bufint);strcat((a),"-")
   // end EMS

   char bufret[1024];
   int i;
   i=isoluz;
   int TotaleSoluzioni;
   int SoluzioneCorrente;
   SoluzioneCorrente=i+1;
   TotaleSoluzioni=Problema.NumSoluzioni;


   int TUP, TUA;
   int NumTrattNoUrb;

   TRACESTRING("IpcServerFirstWrite");

   ipcServerFirstWrite(pOut, 0);    // Shared memory initialization

   TUP=(Problema.InfoSoluzioni+i)->TUP;
   TRACESTRING("Problema.InfoSoluzioni ->TUP");

   TUA=(Problema.InfoSoluzioni+i)->TUA;
   TRACESTRING("Problema.InfoSoluzioni ->TUA");


   NumTrattNoUrb=(Problema.InfoSoluzioni+i)->NumTratte;
   TRACESTRING("(Problema.InfoSoluzioni+i)->NumTratte");

   //FILE *fhtml;
   //char nomefile[13];
   //sprintf(nomefile,"%s%i.htm",PATH_HTML,isoluz);

   //fhtml=fopen(nomefile,"wt");

   char *TreNoBlank;
   char *ServNoBlank;
   int arrayIndServiziTratta[NSERVIZI];
   int arrayIndTreniTratta[NTTRENI];

   TRACESTRING("Loop NSERVIZI");

   for (int contind=0;contind<NSERVIZI;contind++) {
      arrayIndServiziTratta[contind]=0;
   } /* endfor */

   TRACESTRING("Loop NTRENI");

   for (contind=0;contind<NTTRENI;contind++) {
      arrayIndTreniTratta[contind]=0;
   } /* endfor */


   FieldWriteString(pOut,"1");
   FieldWriteString(pOut,Problema.Da);
   FieldWriteString(pOut,Problema.A);
   FieldWriteInt(pOut,Problema.giorno);
   FieldWriteInt(pOut,Problema.mese);
   FieldWriteInt(pOut,Problema.anno);
   FieldWriteInt(pOut,Problema.hh_in);
   FieldWriteInt(pOut,Problema.mm_in);
   //   FieldWriteString(pOut,(Problema.InfoSoluzioni+i)->Parte);
   FieldWriteString(pOut,((Problema.InfoSoluzioni+i)->InfoTratte+TUP)->Parte);

   FieldWriteInt(pOut,i+1);

   FieldWriteInt(pOut,1+(Problema.InfoSoluzioni+i)->NumSoluzione);

   FieldWriteInt(pOut,TotaleSoluzioni);

   FieldWriteString(pOut,"");
   FieldWriteString(pOut,"");

   FieldWriteInt(pOut,((Problema.InfoSoluzioni)+i)->Km);

   FieldWriteString(pOut,((Problema.InfoSoluzioni)+i)->Via);

   FieldWriteInt(pOut,SortMethod);

   FieldWriteInt(pOut,NumTrattNoUrb);

   //printf("pOut: %s\n",pOut);

   TRACESTRING("SCARICABUFFERDET");

   ScaricaBufferDett;

   TRACESTRING("DOPO SCARICABUFFERDET");

   for (int itreno=TUP;itreno<(NumTrattNoUrb+TUP);itreno++) {

      TreNoBlank=((((Problema.InfoSoluzioni+i)->InfoTratte)+itreno)->TipoTreno);

      int found = -1;

      TRACESTRING("LOOP 1");

      for (int nuttr=0;nuttr<NTTRENI;nuttr++) {
         if (strcmpi(arrayTipiTreni[nuttr],TreNoBlank)==0) {
            found = nuttr;
            break;
         }
      } /* endfor*/

      if (found == -1) {
         TRACESTRING("Warning unlisted train type");
         TRACESTRING(TreNoBlank);
         nuttr = 17;
      }
      else
         nuttr = found;
      arrayIndTreniTratta[nuttr]++;

      if ( ((((Problema.InfoSoluzioni+i)->InfoTratte)+itreno)->MV)==0)
      {
         FieldWriteInt(pOut,nuttr);
         FieldWriteString(pOut,"");
         FieldWriteString(pOut,"Tratta urbana non FS");
      } else {
         FieldWriteInt(pOut,nuttr);
         FieldWriteString(pOut,(((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->IDTreno));
         FieldWriteString(pOut,(((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->NomeTreno));
      }

      FieldWriteString(pOut,(((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->FullDa));

      FieldWriteString(pOut,(((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->Parte));

      FieldWriteString(pOut,(((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->FullA));

      FieldWriteString(pOut,(((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->Arriva));

      FieldWriteInt(pOut,((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->NumServiziTreno);


      TRACESTRING("LOOP 2");

      for(int nusrv=0;nusrv<(((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->NumServiziTreno);nusrv++){
         ServNoBlank= ((((Problema.InfoSoluzioni+i)->InfoTratte+itreno)->Servizi)+(LENNOMESRV*nusrv));
         for (int nuts=0;nuts<NSERVIZI;nuts++) {
            if (strcmpi(arrayNomiServizi[nuts],ServNoBlank)==0)
            {
               FieldWriteInt(pOut,nuts);
            } /* endif*/
         }  /* endfor*/
      }  /* endfor*/

      FieldWriteString(pOut,"");

      // EMS printf("pOut: %s\n",pOut);

      TRACESTRING("ScaricaBufferDett");

      ScaricaBufferDett;

   } /* endfor*/

   // ScaricaBufferDett


   //fclose(fhtml);

   TRACESTRING("Fine AsDHtml")

   return;
}
