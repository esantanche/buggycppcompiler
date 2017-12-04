//----------------------------------------------------------------------------                             c
// FP_QUA.CPP: Controllo corrispondenza Km con Km dei percorsi calcolati da CVB
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  3


//----------------------------------------------------------------------------
//#define NO_CUMULATIVO // Non usa i rami cumulativi per i percorsi minimi
//----------------------------------------------------------------------------

#include "stdio.h"
#include "conio.h"
#include "dos.h"
#include <oggetto.h>
#include <scandir.h>
#include <glue.h>
#include "ft_paths.hpp"
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "id_stazi.hpp"
#include "MOTGLUE.HPP"
#include "I_MOTORE.hpp"
#include "alfa0.hpp"
#include "seq_proc.hpp"
#include "mm_perio.hpp"
#include "ft_aux.hpp"
#include "ml_out.hpp"

#include "FT_PATHS.HPP"  // Path da utilizzare

#define PGM   "FP_QUA"

// #define OK_1KM   // Gestisce separatamente le differenze di 1 Km
// #define OK_NONV  // Mostra le stazioni non vendibili


//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
   
   TRACEREGISTER2(NULL,PGM, PATH_DUMMY PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);
   
   SetStatoProcesso(PGM, SP_PARTITO);
   
   DosError(2); // Disabilita Popup di errore
   
   SetPriorita(); // Imposta la priorita'

   PROFILER::Clear(FALSE);
   PROFILER::Clear(TRUE);

   FILE_RW Out(PATH_OUT   PGM ".LOG");
   Out.SetSize(0);
   AddFileToBprintf(&Out);
   
   // Apro l' archivio decodifica codici CCR
   CCR_ID Ccr_Id(PATH_DATI);
   
   if (CCR_ID::CcrId == NULL) {
      CCR_ID::CcrId = & Ccr_Id;
   } /* endif */
   
   MM_RETE_FS & Rete = * MM_RETE_FS::CreaRete(PATH_DATI,NUSTR,NUSTR);
   
   
   GRAFO & Grafo = *GRAFO::Grafo;
   int Rc = 0;
   
   ARRAY_ID Percorso;
   
   FILE_RO Input(PATH_POLIM "POLIPERC.UFF");
   
   STRINGA(Linea);
   
   int Id,Ccr;

   int NumOk      = 0; // Casi Ok
   int NumNonId   = 0; // Casi con stazione non identificabile
   int NumNonVend = 0; // Casi con origine o destinazione non vendibile
   int NumDiff1Km = 0; // Casi con 1 Km di differenza
   int NumDiffKm  = 0; // Casi con differente Chilometraggio
   int NumKo      = 0; // Casi in cui non riesco a calcolare la distanza
   
   int NumLinea = 0;

   printf("Analisi dei dati: portare pazienza \n");
   ID LastId1, LastId2,Id1,Id2;

   while(Input.gets(Linea,256)){

      Grafo.Clear(); // Per deallocare le risorse riservate

      if((++NumLinea) % 100 == 0){
         char Buf[256];
         #ifdef OK_1KM
         sprintf(Buf,"Linea Nø %i ( %i%% ) Ok = %i DiffKm = %i Diff1Km = %i NonVend = %i NonId = %i Ko = %i", NumLinea,Input.PosizioneCorrente()*100/Input.FileSize(), NumOk, NumDiffKm , NumDiff1Km , NumNonVend , NumNonId, NumKo ); 
         #else 
         sprintf(Buf,"Linea Nø %i ( %i%% ) Ok = %i DiffKm = %i NonVend = %i NonId = %i Ko = %i", NumLinea,Input.PosizioneCorrente()*100/Input.FileSize(), NumOk, NumDiffKm , NumNonVend , NumNonId, NumKo ); 
         #endif
         if(NumLinea > 10000 && NumLinea % 10000 == 100){
            printf("\n%s",Buf);
            Bprintf2("%s",Buf);
         } else {
            printf("\r%s",Buf);
         }
      };
      
      Percorso.Clear();
      Ccr = Linea(0,4).ToInt(); 
      Id1 = Id  = Ccr_Id.CercaCCR(Ccr);
      if(Id <= 0){
         Bprintf2("Codice CCR non identificato: %s linea Nø %i %s",Ccr,NumLinea,(CPSZ)Linea);
         NumNonId ++;
         continue;
      } else {
         if( ! Grafo[Id].Vendibile ) {
            #ifdef OK_NONV
            Bprintf2("Stazione Origine non vendibile %i %s",Id,Stazioni[Id].NomeStazione); 
            #endif
            NumNonVend ++;
            continue;
         }
         Percorso += Id;
      }
      int Pointer = 15; // Prima ho i chilometraggi
      while( Linea.Dim() > Pointer ){
         Ccr = Linea(Pointer,Pointer+4).ToInt();
         Pointer += 5;
         Id  = Ccr_Id.CercaCCR(Ccr);
         if(Id <= 0){
            Bprintf2("Codice CCR non identificato: %i linea Nø %i %s",Ccr,NumLinea,(CPSZ)Linea);
            NumNonId ++;
            break;
         } else {
            Percorso += Id;
         }
      };
      if(Id <= 0)continue;
      Ccr = Linea(5,9).ToInt();
      Id2 = Id  = Ccr_Id.CercaCCR(Ccr);
      if(Id <= 0){
         Bprintf2("Codice CCR non identificato: %i linea Nø %i %s",Ccr,NumLinea,(CPSZ)Linea);
         NumNonId ++;
         continue;
      } else {
         if( ! Grafo[Id].Vendibile ) {
            #ifdef OK_NONV
            Bprintf2("Stazione Destinazione non vendibile %i %s",Id,Stazioni[Id].NomeStazione); 
            #endif
            NumNonVend ++;
            continue;
         }
         Percorso += Id;
      }
      
      if(Percorso.Dim() == 0)continue;
      if(Id1 == LastId1 && Id2 == LastId2)continue; // Relazione duplicata
      LastId1 = Id1; LastId2 = Id2;
      MM_PERCORSO & Perc = *Rete.ViaggioLibero(Percorso,FALSE);
      if ( & Perc == NULL){
         Bprintf2("Non calcolabile soluzione per linea Nø %i %s",NumLinea,(CPSZ)Linea);
         Percorso.Trace(Stazioni,"Percorso Richiesto (Da CVB Km "+Linea(10,14)+")");
         NumKo  ++;
         continue;
      }

      int DeltaKm = abs( Perc.DatiTariffazione.KmReali - Linea(10,14).ToInt());
      if( DeltaKm == 0 && Perc.DatiTariffazione.KmConcessi1 == 0 ){
         // Bprintf2("OK Linea %s",(CPSZ)Linea);
         NumOk ++;
         delete &Perc;
         continue; // Tutto OK
      };


      #ifdef OK_1KM
      if( DeltaKm == 1 ){
         // Bprintf2("OK Linea %s",(CPSZ)Linea);
         Bprintf2("Differenza di 1 Km Percorso Nø %i %s (Da CVB Km %s Da Grafo %i)",NumLinea,(CPSZ)Percorso.ToStringa(Stazioni),(CPSZ)Linea(10,14),Perc.DatiTariffazione.KmReali);

         NumDiff1Km ++;
         delete &Perc;
         continue; // Tutto OK
      };
      #endif

      NumDiffKm ++;
      
      Bprintf2("==========================================");
      Bprintf2("Km errati su Percorso Nø %i %s (Da CVB Km %s)",NumLinea,(CPSZ)Linea,(CPSZ)Linea(10,14));
      Percorso.Trace(Stazioni,"Percorso Richiesto (Da CVB Km "+Linea(10,14)+")");
      // Chiamo il printout del percorso
      // Perc.PrintPercorso(stdout,Percorso);
      Perc.PrintPercorso(Out.FileHandle(),Percorso);

      //Bprintf2("---------------> Esplosione delle tratte elementari");
      //int KmBanali = 0;
      //for (int p = 1; p < Percorso.Dim(); p ++ ) {
      //   ARRAY_ID Percorso2 ;
      //   Percorso2 += Percorso[p-1];
      //   Percorso2 += Percorso[p]  ;
      //   MM_PERCORSO & Perc2 = *Rete.ViaggioLibero(Percorso2);
      //   if(&Perc2){
      //      // Perc2.ShortPrintPercorso(stdout);
      //      Perc2.ShortPrintPercorso(Out.FileHandle());
      //      KmBanali += Perc2.DatiTariffazione.KmReali;
      //      delete &Perc2;
      //   } else {
      //      Bprintf2("Non esplosa tratta %i -> %i causa problemi",Percorso[p-1],Percorso[p]);
      //   }
      //} /* endfor */
      //Bprintf2("<------------------ Km Banali : %i\n",KmBanali);

      delete &Perc;
   };

   Bprintf("\n\n ========================================== \n");
   Bprintf(" Casi Ok                                        %i",NumOk       ); 
   Bprintf(" Casi con stazione non identificabile           %i",NumNonId    ); 
   Bprintf(" Casi con origine o destinazione non vendibile  %i",NumNonVend  ); 
   #ifdef OK_1KM
   Bprintf(" Casi con 1 Km di differenza                    %i",NumDiff1Km  ); 
   #endif
   Bprintf(" Casi con differente Chilometraggio             %i",NumDiffKm   ); 
   Bprintf(" Casi in cui non riesco a calcolare la distanza %i",NumKo       ); 
   
   TRACETERMINATE;
   
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   return Rc;
//<<< int main int argc,char *argv    
}

