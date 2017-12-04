//----------------------------------------------------------------------------                             c
// FP_DIRC.CPP: Imposta stazioni di diramazione condizionata
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "oggetto.h"
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "ALFA0.HPP"
#include "POLIM.HPP"
#include "seq_proc.HPP"
#include "FT_PATHS.HPP"  // Path da utilizzare
#include "scandir.h"

#define PGM      "FP_DIRC"

#define NUMECCEZIONI 6
ARRAY_ID CCR_Eccezionali[NUMECCEZIONI];
int      CodiceModifica[NUMECCEZIONI];
int      TipiModifica[NUMECCEZIONI];

// Tipi modifica:
// 0 : Toglie il codice indicato dalle stazioni con i codici CCR indicati
// 1 : Aggiunge il codice indicato dalle stazioni con i codici CCR indicati
// 2 : Toglie il codice indicato da tutte le stazioni 

void SetCodici();
void ModificaLinea(STRINGA & Linea,int CodiceModifica , int Tipo); // Routine di modifica linea

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
   
   int ReturnCode = 0;
   
   
   
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   SetStatoProcesso(PGM, SP_PARTITO);
   
   
   GESTIONE_ECCEZIONI_ON
   
   SetCodici(); // Imposta i codici da modificare
   
   
   
   STRINGA Linea;
   
   FILE_RO Poli(PATH_POLIM "POLIMETR.CED");
   FILE_RW PoliOut(PATH_POLIM "POLIMETR.TMP");
   PoliOut.SetSize(0);
   
   
   while(Poli.gets(Linea,4096)){
      if(Linea[0] == '3') {
         REC3 & Rec3 = *(REC3*)(CPSZ)Linea;
         int CCR = It(Rec3.CodCCR); // Identifica il CCR
         for (int i = 0 ;i < NUMECCEZIONI ;i++ ) {
            if(CCR && (
            CCR_Eccezionali[i].Contiene(CCR) ||   // E' un codice CCR da modificare? 
            TipiModifica[i] == 2                  // Oppure e' una modifica estesa a tutti i codici
            )){ 
               ModificaLinea(Linea,CodiceModifica[i],TipiModifica[i]);// Se si' chiama la routine di modifica linea
            }
         } /* endfor */
      }
      #ifndef AGGIUNGI_TIPO_POLIMETRICA
      if(Linea[0] == '1') { // Descrizione polimetrica
         // Identifico il tipo
         REC1 & Rec1 = *(REC1*)(CPSZ)Linea;
         POLIMETRICA_CIP Polimetrica;
         strcpy(Polimetrica.Descrizione,(CPSZ)STRINGA(Rec1.Descrizione));
         if (!strcmp(Polimetrica.Descrizione,"Tavola di allacciamento                           *")) {
            Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::ALLACCIAMENTO;
         } else if (!strcmp(Polimetrica.Descrizione,"Tavola di diramazione                             *")) {
            Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::DIRAMAZIONE;
         } else {
            Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::LOCALE;
            if (!strcmp(Polimetrica.Descrizione,"Da Civitavecchia Marittima a Golfo Aranci         *")) {
               Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::MARE_FS;
            }
            if (!strcmp(Polimetrica.Descrizione, "Direttissima Genova-Tortona                       *")) {
               Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::SOLO_LOCALE;
            }
            if (!strcmp(Polimetrica.Descrizione, "Eccellente-Rosarno                                *")) {
               Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::SOLO_LOCALE;
            }
            if (!strcmp(Polimetrica.Descrizione, "Maccarese/Fregene-Roma S. Pietro                  *")) {
               Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::SOLO_LOCALE;
            }
            if (!strcmp(Polimetrica.Descrizione, "Firenze Rifredi-Firenze Campo Marte               *")) {
               Polimetrica.TipoPolimetrica = POLIMETRICA_CIP::SOLO_LOCALE;
            }
         } /* endif */
         char Tmp[20];
         sprintf(Tmp,"%2.2i",Polimetrica.TipoPolimetrica);
         Linea = Linea(0,15)+STRINGA(Tmp)+Linea(16,9999);
      } /* endif */
      #endif
      PoliOut.printf("%s\r\n",(CPSZ)Linea); // Scrivi la linea in OUT
   } 
   
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return ReturnCode;
//<<< int main(int argc,char *argv[]){
}


void ModificaLinea(STRINGA & Linea,int CodiceModifica, int Tipo ){
   REC3 & Rec3 = *(REC3*)(CPSZ)Linea;
   BOOL Found = FALSE;
   for (int x=0;x < 14  ; x++ ) {
      int Is = It(Rec3.NodiC[x].Insieme);
      if(Rec3.NodiC[x].Flag == '*')Is = -Is;
      if (Is == CodiceModifica) {

         if(Tipo == 0 || Tipo == 2){
            Rec3.NodiC[x].Insieme[0] = '0';
            Rec3.NodiC[x].Insieme[1] = '0';
            Rec3.NodiC[x].Flag       = ' ';
         }

         Found = TRUE;
         break;
      }
   } /* endfor */

   if(Tipo == 0 || Tipo == 2) Found = TRUE;

   if(!Found){
      for (int x=0;x < 14  ; x++ ) {
         int Is = It(Rec3.NodiC[x].Insieme);
         if (Is == 0) {
            Rec3.NodiC[x].Insieme[0] = '0' + (abs(CodiceModifica) / 10 );
            Rec3.NodiC[x].Insieme[1] = '0' + (abs(CodiceModifica) % 10 );
            Rec3.NodiC[x].Flag       = (CodiceModifica >= 0) ? ' ' : '*';
            break;
         }
      } /* endfor */
      if(x > 14)printf("non vi era spazio per la modifica, linea %s\n",(CPSZ)Linea);
   }

};

void SetCodici(){
   
   // Soppressione dei codici 38,39,40
   TipiModifica[0]= 2     ; // Toglie il codice
   CodiceModifica[0]=   38    ;
   TipiModifica[1]= 2     ; // Toglie il codice
   CodiceModifica[1]=  -38    ;
   TipiModifica[2]= 2     ; // Toglie il codice
   CodiceModifica[2]=   39    ;
   TipiModifica[3]= 2     ; // Toglie il codice
   CodiceModifica[3]=  -39    ;
   TipiModifica[4]= 2     ; // Toglie il codice
   CodiceModifica[4]=   40    ;
   TipiModifica[5]= 2     ; // Toglie il codice
   CodiceModifica[5]=  -40    ;

#ifdef OLD_CODICI
   // ---------------------------------
   // Eccellente - Rosarno
   // ---------------------------------
   // Attenzione: togliere gli zeri in testa ai codici CCR o li interpreta come ottali
   // ---------------------------------
   TipiModifica[0]= 1     ; // In aggiunta
   TipiModifica[1]= 1     ; // In aggiunta
   CodiceModifica[0]= -38     ;
   CCR_Eccezionali[0]+= 11788 ; // Eccellente
   CCR_Eccezionali[0]+= 11765 ; // Rosarno
   CodiceModifica[1]= 38      ;
   CCR_Eccezionali[1]+=  9823 ;
   CCR_Eccezionali[1]+= 11833 ;
   CCR_Eccezionali[1]+= 11788 ;
   CCR_Eccezionali[1]+= 11749 ;
   CCR_Eccezionali[1]+= 12301 ;
   CCR_Eccezionali[1]+= 11461 ;
   CCR_Eccezionali[1]+= 11739 ;
   CCR_Eccezionali[1]+= 11781 ;
   CCR_Eccezionali[1]+= 11782 ;
   CCR_Eccezionali[1]+= 11765 ;
   CCR_Eccezionali[1]+= 11811 ;
   CCR_Eccezionali[1]+= 11465 ;
   CCR_Eccezionali[1]+= 11774 ;
   CCR_Eccezionali[1]+=  9823 ;
   CCR_Eccezionali[1]+= 11700 ;
   CCR_Eccezionali[1]+= 11701 ;
   CCR_Eccezionali[1]+= 11702 ;
   CCR_Eccezionali[1]+= 11703 ;
   CCR_Eccezionali[1]+= 11705 ;
   CCR_Eccezionali[1]+= 11706 ;
   CCR_Eccezionali[1]+= 11707 ;
   CCR_Eccezionali[1]+= 11708 ;
   CCR_Eccezionali[1]+= 11709 ;
   CCR_Eccezionali[1]+= 11710 ;
   CCR_Eccezionali[1]+= 11711 ;
   CCR_Eccezionali[1]+= 11712 ;
   CCR_Eccezionali[1]+= 11713 ;
   CCR_Eccezionali[1]+= 11714 ;
   CCR_Eccezionali[1]+= 11715 ;
   CCR_Eccezionali[1]+= 11716 ;
   CCR_Eccezionali[1]+= 11717 ;
   CCR_Eccezionali[1]+= 11718 ;
   CCR_Eccezionali[1]+= 11719 ;
   CCR_Eccezionali[1]+= 11720 ;
   CCR_Eccezionali[1]+= 11721 ;
   CCR_Eccezionali[1]+= 11722 ;
   CCR_Eccezionali[1]+= 11723 ;
   CCR_Eccezionali[1]+= 11724 ;
   CCR_Eccezionali[1]+= 11725 ;
   CCR_Eccezionali[1]+= 11726 ;
   CCR_Eccezionali[1]+= 11727 ;
   CCR_Eccezionali[1]+= 11728 ;
   CCR_Eccezionali[1]+= 11729 ;
   CCR_Eccezionali[1]+= 11730 ;
   CCR_Eccezionali[1]+= 11731 ;
   CCR_Eccezionali[1]+= 11732 ;
   CCR_Eccezionali[1]+= 11733 ;
   CCR_Eccezionali[1]+= 11734 ;
   CCR_Eccezionali[1]+= 11735 ;
   CCR_Eccezionali[1]+= 11736 ;
   CCR_Eccezionali[1]+= 11737 ;
   CCR_Eccezionali[1]+= 11738 ;
   CCR_Eccezionali[1]+= 11739 ;
   CCR_Eccezionali[1]+= 11740 ;
   CCR_Eccezionali[1]+= 11785 ;
   CCR_Eccezionali[1]+= 11741 ;
   CCR_Eccezionali[1]+= 11742 ;
   CCR_Eccezionali[1]+= 11743 ;
   CCR_Eccezionali[1]+= 11744 ;
   CCR_Eccezionali[1]+= 11745 ;
   CCR_Eccezionali[1]+= 11746 ;
   CCR_Eccezionali[1]+= 11747 ;
   CCR_Eccezionali[1]+= 11748 ;
   CCR_Eccezionali[1]+= 11749 ;
   CCR_Eccezionali[1]+= 11750 ;
   CCR_Eccezionali[1]+= 11751 ;
   CCR_Eccezionali[1]+= 11788 ;
   CCR_Eccezionali[1]+= 11752 ;
   CCR_Eccezionali[1]+= 11753 ;
   CCR_Eccezionali[1]+= 11754 ;
   CCR_Eccezionali[1]+= 11757 ;
   CCR_Eccezionali[1]+= 11758 ;
   CCR_Eccezionali[1]+= 11759 ;
   CCR_Eccezionali[1]+= 11760 ;
   CCR_Eccezionali[1]+= 11786 ;
   CCR_Eccezionali[1]+= 11761 ;
   CCR_Eccezionali[1]+= 11762 ;
   CCR_Eccezionali[1]+= 11763 ;
   CCR_Eccezionali[1]+= 11764 ;
   CCR_Eccezionali[1]+= 11765 ;
   CCR_Eccezionali[1]+= 11784 ;
   CCR_Eccezionali[1]+= 11766 ;
   CCR_Eccezionali[1]+= 11768 ;
   CCR_Eccezionali[1]+= 11769 ;
   CCR_Eccezionali[1]+= 11770 ;
   CCR_Eccezionali[1]+= 11771 ;
   CCR_Eccezionali[1]+= 11772 ;
   CCR_Eccezionali[1]+= 11773 ;
   CCR_Eccezionali[1]+= 11774 ;
   CCR_Eccezionali[1]+= 11776 ;
   CCR_Eccezionali[1]+= 11777 ;
   CCR_Eccezionali[1]+= 11778 ;
   CCR_Eccezionali[1]+= 11779 ;
   CCR_Eccezionali[1]+= 11780 ;
   CCR_Eccezionali[1]+= 11781 ;
   CCR_Eccezionali[1]+= 11782 ;


   // ---------------------------------
   // Tortona - Arquata Scrivia
   // ---------------------------------
   // Attenzione: togliere gli zeri in testa ai codici CCR o li interpreta come ottali
   // ---------------------------------
   TipiModifica[2]= 1     ; // In aggiunta
   TipiModifica[3]= 1     ; // In aggiunta
   CodiceModifica[2]= -39     ;
   CCR_Eccezionali[2]+=  1810 ; // Tortona
   CCR_Eccezionali[2]+=  4207 ; // Arquata Scrivia
   CodiceModifica[3]= 39      ;
   CCR_Eccezionali[3]+=   470 ;
   CCR_Eccezionali[3]+=  1020 ;
   CCR_Eccezionali[3]+=  4207 ;
   CCR_Eccezionali[3]+=   462 ;
   CCR_Eccezionali[3]+=    70 ;
   CCR_Eccezionali[3]+=    16 ;
   CCR_Eccezionali[3]+=  1805 ;
   CCR_Eccezionali[3]+=  1944 ;
   CCR_Eccezionali[3]+=   314 ;
   CCR_Eccezionali[3]+=   179 ;
   CCR_Eccezionali[3]+=   828 ;
   CCR_Eccezionali[3]+=   233 ;
   CCR_Eccezionali[3]+=   232 ;
   CCR_Eccezionali[3]+=  1010 ;
   CCR_Eccezionali[3]+=  1003 ;
   CCR_Eccezionali[3]+=  1030 ;
   CCR_Eccezionali[3]+=  4700 ;
   CCR_Eccezionali[3]+=  4220 ;
   CCR_Eccezionali[3]+=  1811 ;
   CCR_Eccezionali[3]+=  1700 ;
   CCR_Eccezionali[3]+=  1640 ;
   CCR_Eccezionali[3]+=  1701 ;
   CCR_Eccezionali[3]+=  1632 ;
   CCR_Eccezionali[3]+=  1820 ;
   CCR_Eccezionali[3]+=    34 ;
   CCR_Eccezionali[3]+=   248 ;
   CCR_Eccezionali[3]+=  4203 ;
   CCR_Eccezionali[3]+=   984 ;
   CCR_Eccezionali[3]+=  1860 ;
   CCR_Eccezionali[3]+=  5000 ;
   CCR_Eccezionali[3]+=  1009 ;
   CCR_Eccezionali[3]+=  1037 ;
   CCR_Eccezionali[3]+=   109 ;
   CCR_Eccezionali[3]+=    53 ;
   CCR_Eccezionali[3]+=   240 ;
   CCR_Eccezionali[3]+=  1026 ;
   CCR_Eccezionali[3]+=   219 ;
   CCR_Eccezionali[3]+=    39 ;
   CCR_Eccezionali[3]+=  1810 ;
   CCR_Eccezionali[3]+=   455 ;
   CCR_Eccezionali[3]+=    41 ;
   CCR_Eccezionali[3]+=   245 ;
   CCR_Eccezionali[3]+=    21 ;
   CCR_Eccezionali[3]+=  1807 ;
   CCR_Eccezionali[3]+=  1700 ;
   CCR_Eccezionali[3]+=  1701 ;
   CCR_Eccezionali[3]+=  1820 ;
   CCR_Eccezionali[3]+=  1632 ;
   CCR_Eccezionali[3]+=  1801 ;
   CCR_Eccezionali[3]+=  1802 ;
   CCR_Eccezionali[3]+=  1803 ;
   CCR_Eccezionali[3]+=  1860 ;
   CCR_Eccezionali[3]+=  1811 ;
   CCR_Eccezionali[3]+=  1804 ;
   CCR_Eccezionali[3]+=  1805 ;
   CCR_Eccezionali[3]+=  1806 ;
   CCR_Eccezionali[3]+=  1807 ;
   CCR_Eccezionali[3]+=  1809 ;
   CCR_Eccezionali[3]+=  1810 ;
   CCR_Eccezionali[3]+=  4331 ;
   CCR_Eccezionali[3]+=  4330 ;
   CCR_Eccezionali[3]+=  4203 ;
   CCR_Eccezionali[3]+=   470 ;
   CCR_Eccezionali[3]+=  1940 ;
   CCR_Eccezionali[3]+=  1941 ;
   CCR_Eccezionali[3]+=  1810 ;
   CCR_Eccezionali[3]+=  4331 ;
   CCR_Eccezionali[3]+=  4330 ;
   CCR_Eccezionali[3]+=  4203 ;
   CCR_Eccezionali[3]+=  1809 ;
   CCR_Eccezionali[3]+=  1807 ;
   CCR_Eccezionali[3]+=  1942 ;
   CCR_Eccezionali[3]+=  1943 ;
   CCR_Eccezionali[3]+=  1944 ;
   CCR_Eccezionali[3]+=  1945 ;
   CCR_Eccezionali[3]+=  1947 ;
   CCR_Eccezionali[3]+=  1948 ;
   CCR_Eccezionali[3]+=  1949 ;
   CCR_Eccezionali[3]+=  1950 ;
   CCR_Eccezionali[3]+=  1951 ;
   CCR_Eccezionali[3]+=  5000 ;
   CCR_Eccezionali[3]+=   219 ;
   CCR_Eccezionali[3]+=   452 ;
   CCR_Eccezionali[3]+=   453 ;
   CCR_Eccezionali[3]+=   455 ;
   CCR_Eccezionali[3]+=   456 ;
   CCR_Eccezionali[3]+=   457 ;
   CCR_Eccezionali[3]+=   458 ;
   CCR_Eccezionali[3]+=   463 ;
   CCR_Eccezionali[3]+=   459 ;
   CCR_Eccezionali[3]+=   460 ;
   CCR_Eccezionali[3]+=   461 ;
   CCR_Eccezionali[3]+=   462 ;
   CCR_Eccezionali[3]+=   465 ;
   CCR_Eccezionali[3]+=   466 ;
   CCR_Eccezionali[3]+=   468 ;
   CCR_Eccezionali[3]+=   469 ;
   CCR_Eccezionali[3]+=   470 ;
   CCR_Eccezionali[3]+=  4200 ;
   CCR_Eccezionali[3]+=  4202 ;
   CCR_Eccezionali[3]+=  4203 ;
   CCR_Eccezionali[3]+=  4206 ;
   CCR_Eccezionali[3]+=  4207 ;
   CCR_Eccezionali[3]+=  4208 ;
   CCR_Eccezionali[3]+=  4209 ;
   CCR_Eccezionali[3]+=  4210 ;
   CCR_Eccezionali[3]+=  4211 ;
   CCR_Eccezionali[3]+=  4212 ;
   CCR_Eccezionali[3]+=  4214 ;
   CCR_Eccezionali[3]+=  4215 ;
   CCR_Eccezionali[3]+=  4216 ;
   CCR_Eccezionali[3]+=  4217 ;
   CCR_Eccezionali[3]+=  4218 ;
   CCR_Eccezionali[3]+=  4219 ;
   CCR_Eccezionali[3]+=  4220 ;
   CCR_Eccezionali[3]+=  4700 ;

   // ---------------------------------
   // Maccarese  - Roma S. Pietro
   // ---------------------------------
   // Attenzione: togliere gli zeri in testa ai codici CCR o li interpreta come ottali
   // ---------------------------------
   TipiModifica[4]= 1     ; // In aggiunta
   TipiModifica[5]= 1     ; // In aggiunta
   CodiceModifica[4]= -40     ;
   CCR_Eccezionali[4]+=  8020 ; // Maccarese
   CCR_Eccezionali[4]+=  8323 ; // Roma S. Pietro
   CodiceModifica[5]= 40      ;
   CCR_Eccezionali[5]+=  8020 ;
   CCR_Eccezionali[5]+=  8323 ;
   CCR_Eccezionali[5]+=  8207 ;
   CCR_Eccezionali[5]+=  5043 ;
   CCR_Eccezionali[5]+=  8311 ;
   CCR_Eccezionali[5]+=  6925 ;
   CCR_Eccezionali[5]+=  8010 ;
   CCR_Eccezionali[5]+=  8012 ;
   CCR_Eccezionali[5]+=  6420 ;
   CCR_Eccezionali[5]+=  6421 ;
   CCR_Eccezionali[5]+= 12852 ;
   CCR_Eccezionali[5]+=  6724 ;
   CCR_Eccezionali[5]+=  6404 ;
   CCR_Eccezionali[5]+=  8020 ;
   CCR_Eccezionali[5]+=  6820 ;
   CCR_Eccezionali[5]+=  6038 ;
   CCR_Eccezionali[5]+=  8209 ;
   CCR_Eccezionali[5]+=  6500 ;
   CCR_Eccezionali[5]+=  6805 ;
   CCR_Eccezionali[5]+=  6904 ;
   CCR_Eccezionali[5]+=  6506 ;
   CCR_Eccezionali[5]+=  6416 ;
   CCR_Eccezionali[5]+=  8323 ;
   CCR_Eccezionali[5]+=  8409 ;
   CCR_Eccezionali[5]+=  8405 ;
   CCR_Eccezionali[5]+=  6809 ;
   CCR_Eccezionali[5]+=  6922 ;
   CCR_Eccezionali[5]+=  8405 ;
   CCR_Eccezionali[5]+=  8323 ;
   CCR_Eccezionali[5]+=  8325 ;
   CCR_Eccezionali[5]+=  8329 ;
   CCR_Eccezionali[5]+=  8322 ;
   CCR_Eccezionali[5]+=  8328 ;
   CCR_Eccezionali[5]+=  8321 ;
   CCR_Eccezionali[5]+=  8330 ;
   CCR_Eccezionali[5]+=  8320 ;
   CCR_Eccezionali[5]+=  8319 ;
   CCR_Eccezionali[5]+=  8318 ;
   CCR_Eccezionali[5]+=  8317 ;
   CCR_Eccezionali[5]+=  8316 ;
   CCR_Eccezionali[5]+=  8315 ;
   CCR_Eccezionali[5]+=  8314 ;
   CCR_Eccezionali[5]+=  8313 ;
   CCR_Eccezionali[5]+=  8312 ;
   CCR_Eccezionali[5]+=  8311 ;
   CCR_Eccezionali[5]+=  8310 ;
   CCR_Eccezionali[5]+=  8309 ;
   CCR_Eccezionali[5]+=  8308 ;
   CCR_Eccezionali[5]+=  8327 ;
   CCR_Eccezionali[5]+=  8307 ;
   CCR_Eccezionali[5]+=  8306 ;
   CCR_Eccezionali[5]+=  8304 ;
   CCR_Eccezionali[5]+=  8303 ;
   CCR_Eccezionali[5]+=  8301 ;
   CCR_Eccezionali[5]+=  8326 ;
   CCR_Eccezionali[5]+=  8300 ;
   CCR_Eccezionali[5]+=  8207 ;

   // ---------------------------------
   // Domodossola - Arona 
   // ---------------------------------
   // Attenzione: togliere gli zeri in testa ai codici CCR o li interpreta come ottali
   // ---------------------------------
   TipiModifica[6]= 0     ; // Toglie il codice
   CodiceModifica[6]=   4     ;
   CCR_Eccezionali[6]+=  1007 ;
   CCR_Eccezionali[6]+=  1008 ;
   CCR_Eccezionali[6]+=  1009 ;
   CCR_Eccezionali[6]+=  1010 ;
   CCR_Eccezionali[6]+=  1011 ;
   CCR_Eccezionali[6]+=  1012 ;
   CCR_Eccezionali[6]+=  1013 ;
   CCR_Eccezionali[6]+=  1015 ;
   CCR_Eccezionali[6]+=  1016 ;
   CCR_Eccezionali[6]+=  1017 ;
   CCR_Eccezionali[6]+=  1018 ;
   CCR_Eccezionali[6]+=  1019 ;
   CCR_Eccezionali[6]+=  1020 ;

#endif

//<<< void SetCodici(){
};
