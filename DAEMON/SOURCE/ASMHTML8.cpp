
#undef TRCRTN
#define TRCRTN "FILE asmhtml8"

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "ipcfssrv.h"
#include "trc2.h"

// begin EMS prove
//#define ScaricaBuffer ipcServerNextWrite(pOut, strlen(pOut)); *pOut=0;
//#define ScaricaBuffer \
//   TRACESTRING(STRINGA(pOut));   \
//   Ftrace = fopen("TRACEDAE.TRC","a");              \
//   fprintf(Ftrace,"OOO %s\n",pOut);                \
//   fclose(Ftrace); *pOut=0;
#define ScaricaBuffer  ipcServerNextWrite(pOut, strlen(pOut));     \
   TRACESTRING(STRINGA(pOut));   \
   Ftrace = fopen("D:\\EMS\\DAEMON\\EXE\\TRACEDAE.TRC","a");              \
   fprintf(Ftrace,"%s\n",pOut);                \
   fclose(Ftrace); *pOut=0;


// end EMS

void Domani(int, int, int, int *, int *, int *);
void TraUnMinuto(int, int, int, int, int, int *, int *, int *, int *, int *);
void Time2hm(char *, int *,int *);

// begin EMS
#define FieldWriteString(a,b) strcat((a),(b));strcat((a),"\1")
#define FieldWriteInt(a,b) sprintf(bufint,"%d",(b));strcat((a),bufint);strcat((a),"\1")
//#define FieldWriteString(a,b) strcat((a),(b));strcat((a),"-")
//#define FieldWriteInt(a,b) sprintf(bufint,"%d",(b));strcat((a),bufint);strcat((a),"-")
// end EMS

///////////////////////////////////////////////////////////////////////////////
//        AsMHtml
///////////////////////////////////////////////////////////////////////////////

void AsMHtml()
{

   #undef TRCRTN
   #define TRCRTN "AsMHtml"

   char bufint[20];

   int TUP, TUA;
   int NumTrattNoUrb;
   int found;

   //FILE * fhtml;

   char href[1024];
   char rs[32];
   int minAttesa;

   TRACESTRING("DENTRO A AsMHtml !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

   ipcServerFirstWrite(pOut, 0);               /* Shared memory initialization*/

   char bufdet[1024];

   FieldWriteString(pOut,"0");
   FieldWriteString(pOut,Problema.Da);
   FieldWriteString(pOut,Problema.A);
   FieldWriteInt(pOut,Problema.giorno);
   FieldWriteInt(pOut,Problema.mese);
   FieldWriteInt(pOut,Problema.anno);
   FieldWriteInt(pOut,Problema.hh_in);
   FieldWriteInt(pOut,Problema.mm_in);

   int Limit = MAXSOL;
   if (Problema.NumSoluzioni < Limit) Limit = Problema.NumSoluzioni;
   FieldWriteInt(pOut,Limit);
   FieldWriteInt(pOut,Problema.NumSoluzioniInRange);

   if (Problema.EsistonoCambi > 0) {
      FieldWriteString(pOut,"1");
   }
   else {
      FieldWriteString(pOut,"0");
   }

   //printf("pOut: %s\n",pOut);
   FieldWriteInt(pOut,SortMethod);
   TRCbegin "DOPO ScaricaBuffer in AsMHtml pOut=|%s|\n", pOut TRCend
   ScaricaBuffer;



   for(int i = 0; i < Limit; i++)
   {

      TUP=(Problema.InfoSoluzioni+i)->TUP;
      TUA=(Problema.InfoSoluzioni+i)->TUA;
      NumTrattNoUrb=(Problema.InfoSoluzioni+i)->NumTratte;

      FieldWriteString(pOut,((Problema.InfoSoluzioni+i)->InfoTratte+TUP)->Parte);
      if (TUP==1) {
         FieldWriteString(pOut,((Problema.InfoSoluzioni+i)->InfoTratte+TUP)->Da);
      }
      else {
         FieldWriteString(pOut,"");
      }

      FieldWriteString(pOut,((Problema.InfoSoluzioni+i)->InfoTratte+(NumTrattNoUrb+TUP-1))->Arriva);
      if (TUA==1) {
         FieldWriteString(pOut,((Problema.InfoSoluzioni+i)->InfoTratte+(NumTrattNoUrb+TUP-1))->A);
      }
      else {
         FieldWriteString(pOut,"");
      }


      FieldWriteString(pOut,(Problema.InfoSoluzioni+i)->TTot);

      FieldWriteInt(pOut,NumTrattNoUrb-1);

      char *StazArNoBlank;
      for (int contra=TUP;contra<( NumTrattNoUrb+TUP-1);contra++) {
         char bufwait[6];
         StazArNoBlank=((((Problema.InfoSoluzioni+i)->InfoTratte)+contra)->A);
         FieldWriteString(pOut,StazArNoBlank);
         FieldWriteString(pOut,((((Problema.InfoSoluzioni+i)->InfoTratte)+contra)->Arriva));
         minAttesa=CalcolaAttesa( ((((Problema.InfoSoluzioni)+i)->InfoTratte)+contra)->Arriva, ((((Problema.InfoSoluzioni)+i)->InfoTratte)+(contra+1))->Parte);
         sprintf(bufwait, "%02i:%02i",(minAttesa/60),(minAttesa%60));
         FieldWriteString(pOut,bufwait);
      }                                                           /* endfor*/

      FieldWriteInt(pOut,(Problema.InfoSoluzioni+i)->NumServizi);

      char *ServNoBlank;

      for(int nusrv=0;nusrv<(Problema.InfoSoluzioni+i)->NumServizi;nusrv++){
         ServNoBlank=strtok( (((Problema.InfoSoluzioni+i)->Servizi)+(LENNOMESRV*nusrv))," ");
         found = 0;
         for (int nuts=0;nuts<NSERVIZI;nuts++) {
            if (strcmpi(arrayNomiServizi[nuts],ServNoBlank)==0) {
               FieldWriteInt(pOut,nuts);
               found = 1;
            }
         }                                                        /* endfor*/
         if (found == 0) {
            FieldWriteInt(pOut,28);
         }
      }                                                           /* endfor*/

      FieldWriteInt(pOut,NumTrattNoUrb);

      char *TreNoBlank;

      for (int contTreno=TUP;contTreno<NumTrattNoUrb+TUP;contTreno++ ) {
         TreNoBlank=((((Problema.InfoSoluzioni+i)->InfoTratte)+contTreno)->TipoTreno);
         found = 0;
         for (int nuttr=0;nuttr<NTTRENI;nuttr++) {
            if (strcmpi(arrayTipiTreni[nuttr],TreNoBlank)==0) {
               FieldWriteInt(pOut,nuttr);
               found = 1;
            }
         }                                                        /* endfor*/
         if (found == 0) {
            FieldWriteInt(pOut,17);
         }
      }                                                           /* endfor*/

      FieldWriteString(pOut,"");
      FieldWriteString(pOut,"");

      //printf("pOut: %s\n",pOut);
      ScaricaBuffer;

   }



   FieldWriteInt(pOut,StCambi.NumCambi);

   for (int concam=0;concam<StCambi.NumCambi ;concam++) {
      FieldWriteString(pOut,((StCambi.Acronimi)+(LENACRST*concam)));
      FieldWriteString(pOut,((StCambi.NomiEstesi)+(LENNMSTA*concam)));
   }                                                           /* endfor*/



   //scarico buffer
   //printf("pOut: %s\n",pOut);
   ScaricaBuffer;


//fclose(fhtml);
return ;
}


int CalcolaAttesa(char oAP[6], char oPS[6])
{
   int hora1, mmin1, hora2, mmin2, inmin1, inmin2, attesa;
   char *p;
   char oraArrPrec[6];
   char oraPartSucc[6];
   sprintf(oraArrPrec,"%s",oAP);
   sprintf(oraPartSucc,"%s",oPS);

   p=strtok(oraArrPrec,":");
   hora1=atoi(p);
   p=strtok(NULL,":");
   mmin1=atoi(p);

   inmin1=hora1*60 + mmin1;

   p=strtok(oraPartSucc,":");
   hora2=atoi(p);
   p=strtok(NULL,":");
   mmin2=atoi(p);

   inmin2=hora2*60 + mmin2;

   if (inmin2<inmin1)
       attesa=24*60+(inmin2-inmin1);
   else
       attesa=inmin2-inmin1;

   return attesa;

}

void Domani(int g1, int m1, int a1, int *g2, int *m2, int *a2)
{
  int lm[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (0 != a1 % 4) lm[1] = 28;
  else lm[1] = 29;

  if (g1 < lm[m1-1])
  {
     *g2 = g1 + 1;
     *m2 = m1;
     *a2 = a1;
  }
  else
  {
     *g2 = 1;
     if (m1 < 12)
     {
        *m2 = m1 + 1;
        *a2 = a1;
     }
     else
     {
        *m2 = 1;
        *a2 = a1 + 1;
     } /* endif */
  } /* endif */
}

void TraUnMinuto(int dg1, int dm1, int da1, int th1, int tm1,
                 int *dg2, int *dm2, int *da2, int *th2, int *tm2)
{
   if (tm1 < 59)
   {
      *tm2 = tm1 + 1;
      *th2 = th1;
      *dg2 = dg1;
      *dm2 = dm1;
      *da2 = da1;
   }
   else
   {
      *tm2 = 0;
      if (th1 < 23)
      {
         *th2 = th1 + 1;
         *dg2 = dg1;
         *dm2 = dm1;
         *da2 = da1;
      }
      else
         *th2 = 0;
         Domani(dg1,dm1,da1, dg2,dm2,da2);
      {
      } /* endif */
   } /* endif */
}

void Time2hm(char *time, int *h,int *m)
{
   char t[6];
   char *p;

   strcpy(t,time);
   p = strtok(t,":");
   *h = atoi(p);
   p = strtok(NULL,":");
   *m = atoi(p);
}
