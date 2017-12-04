//----------------------------------------------------------------------------
// DU_SRVTR: DUMP ID_SRVTR.TMP
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "motglue.HPP"

struct MY_STRU {
//----------------------------------------------------------------------------
// Parte specifica: Definizione del records su file
//----------------------------------------------------------------------------
// Qui va la definizione della struttura
   DWORD NumeroMezzoVg   ; // KEY: e' il mezzo viaggiante, non il mezzo virtuale
   MM_INFO   Servizi                       ; // Servizi (NB: Le note sono ancora da aggiornare)
   BYTE      Uniforme                      ; // 1 se non ha periodicita' ne' stazioni disuniformi

};
#define NOME "F_SRVTRN"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_OUT       PATH_OUT NOME ".TXT"
#define NOME_IN        PATH_OUT NOME ".TMP"
#define NOME_TRACE     NOME ".TRC"
#define BUF_DIM        128000
//----------------------------------------------------------------------------

class  MY_FILE : public FILE_FIX {
   public:
   MY_FILE():
   FILE_FIX(NOME_IN,sizeof(MY_STRU),BUF_DIM){};
   MY_STRU &  operator [](ULONG Indice){ Posiziona(Indice); return *(MY_STRU*) RecordC; };
};
STRINGA Add(char * a,char*b){
   char Buf[100];
   sprintf(Buf,a,b);
   return STRINGA(Buf);
};
#define MK1(_a,_b,_c,_d,_e) FMT += "%-" _b _c ;HDR1 += Add("%-" _b "s",_d) ; HDR2 += Add("%-" _b "s",_e);
#define MK2(_a,_b,_c,_d,_e) ,Record._a
//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      
                                                                                    
void main(int argc,char *argv[]){                                                   
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          

   // TRACEREGISTER2(NULL,NOME,NOME_TRACE);

   printf("Analizzo i records di %s scrivendo il risultato su %s\n",NOME_IN,NOME_OUT);
                                                                                    
   MY_FILE & InFile = * new MY_FILE();                                              
   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN  ";                                                                  
   HDR2 = "      ";                                                                  
   FMT  = "%-6u";
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
// Qui va la definizione dei campi di stampa (la dim deve comprendere lo spazio verso il prossimo campo)
   // Es: MK1(IdStazioneDestinazione    ,"05","u","Tipo","Rel. ")
   // Es: MK1(NomeStazione              ,"36","s","Nomestazione","")
   MK1( NumeroMezzoVg                     ,"6","u","Num","MezVg")
   MK1(Servizi.HaNoteGeneriche            ,"3","u","N","G")
   MK1(Servizi.HaNoteDiVendita            ,"3","u","N","V")
   MK1(Servizi.PrenotDisUniforme          ,"3","u","P","D")
   MK1(Servizi.Prenotabilita              ,"5","u","Pren","")
   MK1(Servizi.PrenObbligItalia           ,"4","u","Pre","OI")
   MK1(Servizi.PrenObbligEstero           ,"4","u","Pre","OE")
   MK1(Servizi.PrenotabileSolo1Cl         ,"4","u","Pre","1")
   MK1(Servizi.ServTraspDisUniformi       ,"5","u","Tras","DisU")
   MK1(Servizi.ServizioBase               ,"5","u","Ser","Base")
   MK1(Servizi.PostiASederePrima          ,"5","u","Post","Sed1")
   MK1(Servizi.PostiASedereSeconda        ,"5","u","Post","Sed2")
   MK1(Servizi.SleeperettePrima           ,"6","u","Sleep","1")
   MK1(Servizi.SleeperetteSeconda         ,"6","u","Sleep","2")
   MK1(Servizi.CuccettePrima              ,"5","u","Cucc","1")
   MK1(Servizi.CuccetteSeconda            ,"5","u","Cucc","2")
   MK1(Servizi.VagoniLettoPrima           ,"4","u","Vag","L1")
   MK1(Servizi.VagoniLettoSeconda         ,"5","u","Vag","Let2")
   MK1(Servizi.AutoAlSeguito              ,"5","u","Auto","")
   MK1(Servizi.Invalidi                   ,"4","u","Inv","")
   MK1(Servizi.Biciclette                 ,"4","u","Bic","")
   MK1(Servizi.Animali                    ,"4","u","Ani","")
   MK1(Servizi.ServGenerDisUniformi       ,"6","u","Ser","GeDis")
   MK1(Servizi.Ristoro                    ,"5","u","Rist","oro")
   MK1(Servizi.BuffetBar                  ,"5","u","Buff","Bar")
   MK1(Servizi.SelfService                ,"5","u","Self","Ser")
   MK1(Servizi.Ristorante                 ,"5","u","Rist","nte")
   MK1(Servizi.Fumatori                   ,"4","u","Fum","")
   MK1(Servizi.ClassificaDisUniforme      ,"5","u","Clas","DisU")
   MK1(Servizi.TipoMezzo                  ,"5","u","Tipo","Mezzo")
   MK1( Uniforme                          ,"5" ,"u","Unif.","")

//----------------------------------------------------------------------------

   // puts((CPSZ)HDR1);
   // puts((CPSZ)HDR2);
   // puts((CPSZ)FMT);

   FILE * Out;
   Out = fopen(NOME_OUT,"wt");

   printf("Debbo analizzare %i records\n",InFile.NumRecordsTotali());

   for (ULONG i= 0;i < InFile.NumRecordsTotali() ;i++ ) {
      MY_STRU & Record = InFile[i];
      if (i % NUMRIGHE_DUMP == 0) {
         if(i != 0)fprintf(Out,"\n");
         fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
         // TRACESTRING(HDR1);
         // TRACESTRING(HDR2);
      } /* endif */
      char Buf[500];
      sprintf(Buf,(CPSZ)FMT,i
//----------------------------------------------------------------------------
// Parte specifica: Macro definizione variabili da stampare
//----------------------------------------------------------------------------
   // MK2(  ,"00","u","","")
   MK2(NumeroMezzoVg                      ,"6","p","Num","Mezzo Vg")
   MK2(Servizi.HaNoteGeneriche            ,"3","u","N","G")
   MK2(Servizi.HaNoteDiVendita            ,"3","u","N","V")
   MK2(Servizi.PrenotDisUniforme          ,"3","u","P","D")
   MK2(Servizi.Prenotabilita              ,"5","u","Pren","")
   MK2(Servizi.PrenObbligItalia           ,"4","u","Pre","OI")
   MK2(Servizi.PrenObbligEstero           ,"4","u","Pre","OE")
   MK2(Servizi.PrenotabileSolo1Cl         ,"4","u","Pre","1")
   MK2(Servizi.ServTraspDisUniformi       ,"5","u","Tras","DisU")
   MK2(Servizi.ServizioBase               ,"5","u","Ser","Base")
   MK2(Servizi.PostiASederePrima          ,"5","u","Post","Sed1")
   MK2(Servizi.PostiASedereSeconda        ,"5","u","Post","Sed2")
   MK2(Servizi.SleeperettePrima           ,"6","u","Sleep","1")
   MK2(Servizi.SleeperetteSeconda         ,"6","u","Sleep","2")
   MK2(Servizi.CuccettePrima              ,"5","u","Cucc","1")
   MK2(Servizi.CuccetteSeconda            ,"5","u","Cucc","2")
   MK2(Servizi.VagoniLettoPrima           ,"4","u","Vag","L1")
   MK2(Servizi.VagoniLettoSeconda         ,"5","u","Vag","Let2")
   MK2(Servizi.AutoAlSeguito              ,"5","u","Auto","")
   MK2(Servizi.Invalidi                   ,"4","u","Inv","")
   MK2(Servizi.Biciclette                 ,"4","u","Bic","")
   MK2(Servizi.Animali                    ,"4","u","Ani","")
   MK2(Servizi.ServGenerDisUniformi       ,"6","u","Ser","GeDis")
   MK2(Servizi.Ristoro                    ,"5","u","Rist","oro")
   MK2(Servizi.BuffetBar                  ,"5","u","Buff","Bar")
   MK2(Servizi.SelfService                ,"5","u","Self","Ser")
   MK2(Servizi.Ristorante                 ,"5","u","Rist","nte")
   MK2(Servizi.Fumatori                   ,"4","u","Fum","")
   MK2(Servizi.ClassificaDisUniforme      ,"5","u","Clas","DisU")
   MK2(Servizi.TipoMezzo                  ,"5","u","Tipo","Mezzo")
   MK2( Uniforme                          ,"5" ,"u","Unif.","")
//----------------------------------------------------------------------------

      );
      fprintf(Out,"%s\n",Buf);
      // TRACESTRING(Buf);
      // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
   } /* endfor */

   delete &InFile;
   fclose(Out);

   // TRACETERMINATE;
   
   exit(0);
//<<< void main(int argc,char *argv[]){
}

