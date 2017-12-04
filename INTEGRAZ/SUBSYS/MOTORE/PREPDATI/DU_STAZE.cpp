//----------------------------------------------------------------------------
// DU_STAZI: DUMP ID_STAZI.DB
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1


#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "MM_VARIE.HPP"
#include "FT_PATHS.HPP"  // Path da utilizzare

#define NOME "ID_STAZI"
//----------------------------------------------------------------------------

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
#define NOME_IN   "ID_STAZI.DB"
#define NOME_OUT  "ID_STAZI.OUT"
                                                                                    
int  main(int argc,char *argv[]){                                                   
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
                                                                                    
   // TRACEREGISTER2(NULL,NOME,PATH_DUMMY NOME ".TRC");

   printf("Analizzo i records di %s scrivendo il risultato su %s\n",NOME_IN,NOME_OUT);

   //STAZIONI InFile(PATH_DATI );
   STAZIONI InFile("",NOME_IN);
   STRINGA HDR1,HDR2,FMT;                                                           
   HDR1 = "RecN ";                                                                  
   HDR2 = "     ";                                                                  
   FMT  = "%-5u";
                                                                                    
//----------------------------------------------------------------------------      
// Parte specifica: Macro definizione formato di stampa
//----------------------------------------------------------------------------
   // MK1(  , "0","u","","")
   MK1(NomeStazione               ,"36","s","NomeStazione","")
   MK1(IdStazione                 , "5","u","Id","Staz")
   MK1(Nome7                      , "8","s","Nome7","")
   MK1(vendibile                  , "2","u","V","e")
   MK1(StazioneFs                 , "2","u","F","s")
   MK1(estera                     , "2","u","E","s")
   MK1(Aperta                     , "2","u","A","p")
   MK1(Fittizia                   , "2","u","V","i")
   MK1(Informativa                , "2","u","I","f")
   MK1(InPrenotazione             , "2","u","P","r")
   MK1(StazioneCumulativo         , "2","u","S","c")
   MK1(DestinazCumulativo         , "2","u","D","c")
   MK1(CodiceCCR                  , "6","u","CCR","")
   MK1(CCRCumulativo1             , "6","u","CUMUL.","CCR")
   MK1(SocietaConcessa1           , "3","u","1","SC")
   MK1(CCRCumulativo2             , "6","u","CUMUL.","CCR")
   MK1(SocietaConcessa2           , "3","u","2","SC")
   MK1(IdStazione1                , "5","u","Id1","")
   MK1(IdStazione2                , "5","u","Id2","")
   MK1(Distanza1                  , "4","u","D1","")
   MK1(Distanza2                  , "4","u","D2","")
   MK1(ProgRamo                   , "4","u","Prg","Rm")
   MK1(CodiceInstradamentoCvb     , "5","u","Inst","Cvb")
   MK1(ImpiantoCommerciale        , "6","u","Impia","Comm")
   MK1(CodiceTPN                  , "5","u","TPN","")
   MK1(IstatRegione               , "3","u","Is","at")
   MK1(TariffaRegione             , "3","u","Ta","Rg")
   MK1(Prima_Estensione           , "3","u","E1","")
   MK1(Seconda_Estensione         , "3","u","E2","")
   MK1(Terza_Estensione           , "3","u","E3","")
   MK1(StazioneTraMarCum          , "2","u","Tr","Ma")
   MK1(TipoStazione               , "2","u","T","s")
   MK1(TipoImpiantoFisico         , "2","u","T","i")
   MK1(CarImpiantoFisico          , "2","u","C","i")
   MK1(NomeStaz12                 ,"13","s","Nome12","")
   MK1(NomeStaz20                 ,"21","s","Nome20 ( o Estero 3)","")
   MK1(CCREstero                  , "6","u","CCR","Ester")
   MK1(CCREstero2                 , "6","u","CCR","Este2")
   MK1(CCREstero3                 , "6","u","CCR","Este3")
   MK1(CodiceContabAusiliario     , "6","u","Aux","Contb")
   MK1(CCRVendita                 , "6","u","CCR","Vendi")
   MK1(NomeEstero                 ,"21","s","Nome Estero ","")
   MK1(NomeEstero2                ,"21","s","Nome Estero 2 ","")

//----------------------------------------------------------------------------


   FILE * Out;
   Out = fopen(NOME_OUT,"w"); //EMS tolgo il t di "wt"

   printf("InFile.Dim()=%d\n",  InFile.Dim());

   ORD_FORALL(InFile,i){
      STAZIONI::R_STRU & Record = InFile[i];
      if (i % NUMRIGHE_DUMP == 0) {
         if(i != 0)fprintf(Out,"\n");
         fprintf(Out,"%s\n%s\n",(CPSZ)HDR1,(CPSZ)HDR2);
         printf("%d\n",i);
         // TRACESTRING(HDR1);
         // TRACESTRING(HDR2);
      } /* endif */
      char Buf[500];
      sprintf(Buf,(CPSZ)FMT,i

//----------------------------------------------------------------------------
// Parte specifica: Macro definizione variabili da stampare
//----------------------------------------------------------------------------
   MK2(NomeStazione               ,"36","s","NomeStazione","")
   MK2(IdStazione                 , "5","u","Id","Staz")
   MK2(Nome7                      , "8","s","Nome7","")
   MK2(vendibile                  , "2","u","V","e")
   MK2(StazioneFs                 , "2","u","F","s")
   MK2(estera                     , "2","u","E","s")
   MK2(Aperta                     , "2","u","A","p")
   MK2(Fittizia                   , "2","u","V","i")
   MK2(Informativa                , "2","u","I","f")
   MK2(InPrenotazione             , "2","u","P","r")
   MK2(StazioneCumulativo         , "2","u","S","c")
   MK2(DestinazCumulativo         , "2","u","D","c")
   MK2(CodiceCCR                  , "6","u","CCR","")
   MK2(CCRCumulativo1             , "6","u","CUMUL.","CCR")
   MK2(SocietaConcessa1           , "3","u","1","SC")
   MK2(CCRCumulativo2             , "6","u","CUMUL.","CCR")
   MK2(SocietaConcessa2           , "3","u","2","SC")
   MK2(IdStazione1                , "5","u","Id1","")
   MK2(IdStazione2                , "5","u","Id2","")
   MK2(Distanza1                  , "4","u","D1","")
   MK2(Distanza2                  , "4","u","D2","")
   MK2(ProgRamo                   , "4","u","Prg","Rm")
   MK2(CodiceInstradamentoCvb     , "5","u","Inst","Cvb")
   MK2(ImpiantoCommerciale        , "6","u","Impia","Comm")
   MK2(CodiceTPN                  , "5","u","TPN","")
   MK2(IstatRegione               , "3","u","Is","at")
   MK2(TariffaRegione             , "3","u","Ta","Rg")
   MK2(Prima_Estensione           , "3","u","E1","")
   MK2(Seconda_Estensione         , "3","u","E2","")
   MK2(Terza_Estensione           , "3","u","E3","")
   MK2(StazioneTraMarCum          , "2","u","Tr","Ma")
   MK2(TipoStazione               , "2","u","T","s")
   MK2(TipoImpiantoFisico         , "2","u","T","i")
   MK2(CarImpiantoFisico          , "2","u","C","i")
   MK2(NomeStaz12                 ,"13","s","Nome12","")
   MK2(NomeStaz20                 ,"21","s","Nome20 ( o Estero 3)","")
   MK2(CCREstero                  , "6","u","CCR","Ester")
   MK2(CCREstero2                 , "6","u","CCR","Este2")
   MK2(CCREstero3                 , "6","u","CCR","Este3")
   MK2(CodiceContabAusiliario     , "6","u","Aux","Contb")
   MK2(CCRVendita                 , "6","u","CCR","Vendi")
   MK2(NomeEstero                 ,"21","s","Nome Estero ","")
   MK2(NomeEstero2                ,"21","s","Nome Estero 2 ","")

//----------------------------------------------------------------------------

      );
      fprintf(Out,"%s\n",Buf);
      // TRACESTRING(Buf);
      // TRACEHEX("Record:",(CPSZ)&Record,sizeof(MY_STRU));
   } /* endfor */

   fclose(Out);

   TRACETERMINATE;
   
   return 0;
//<<< void main(int argc,char *argv[]){
}

