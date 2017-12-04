//----------------------------------------------------------------------------
// Test ed aggiornamento stazioni
//----------------------------------------------------------------------------
// Questo programma e' stato utilizzato in occasione del passaggio in esercizio
// della versione 1.6 dell' M300 Like
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "id_stazi.hpp"
#include "\wrk3\idstrazi.hpp"
#include "FT_PATHS.HPP"  // Path da utilizzare


#define PGM      "TK"

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      

int main(int argc,char *argv[]){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          

   int rc = 0;
      
   TRACEREGISTER2(NULL,PGM, PGM ".TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);

   STAZIONI     Staz("\\datimot\\");
   STAZIONI_ALL_PER_NOME StaNome("\\datimot\\");
   OLD_STAZIONI OldStaz("\\wrk3\\");
   OldStaz.PermettiScrittura();

   CCR_ID::CcrId = new CCR_ID(PATH_DATI);
   CCR_ID & CcrId = *(CCR_ID::CcrId);



   ORD_FORALL(OldStaz,i){

      OLD_STAZIONI::R_STRU & OldS = OldStaz[i];
      if(OldS.IdStazione == 0 ) continue;

      // Modifiche ad Hoc
      if(OldS.IdStazione == 5094 ){
         OldS.CCRCumulativo1  = 15016;
         OldS.CCRCumulativo2  = 15010;
         OldStaz.ModifyCurrentRecord();
      } /* endif */

      int NewId = 0;
      int CCR   = 0;
      if(OldS.CodiceCCR){
         NewId=  CcrId[OldS.CodiceCCR];
         CCR = OldS.CodiceCCR;
      } else if(OldS.CCRCumulativo1){
         NewId=  CcrId[OldS.CCRCumulativo1];
         CCR = OldS.CCRCumulativo1;
      } else {
         //printf("Non controllo stazione senza codice CCR %s\n",OldS.NomeStazione);
      } /* endif */

      if(CCR && NewId == 0){
         // Cerco di identificarla per nome
         StaNome.Posiziona(OldS.NomeStazione);
         if(StaNome.KeyEsatta && !strcmp(StaNome.RecordCorrente().NomeStazione,OldS.NomeStazione)){
            NewId = StaNome.RecordCorrente().IdStazione;
         } else {
            // printf("Non ha corrispondente codice CCR %i stazione %s ",CCR,OldS.NomeStazione);
            // printf("e non puo' essere identificata per nome\n");
         };
      };
      if(CCR && NewId){
         STAZIONI::R_STRU & NewS = Staz[NewId];
         #define Check(_a) if(NewS._a != OldS._a)printf("Differenti " #_a " (Old %i New %i) per stazione %s\n",OldS._a,NewS._a,OldS.NomeStazione);
         #define Check2(_a) if(strcmp(OldS._a ,NewS._a))printf("Differenti " #_a " (Old %s New %s) \n",OldS._a,NewS._a);

         Check(TariffaRegione);
         Check(Prima_Estensione);
         Check(Seconda_Estensione);
         Check(Terza_Estensione);
         Check(CodiceCCR);
         //Check(CCRCumulativo1);
         //Check(CCRCumulativo2);
         //Check2(NomeStazione);

         if (OldS.CodiceCCR == NewS.CodiceCCR && NewS.TariffaRegione != 0) {
            if (NewS.TariffaRegione != OldS.TariffaRegione) {
               OldS.TariffaRegione = NewS.TariffaRegione;
               OldStaz.ModifyCurrentRecord();
            } /* endif */
            if (NewS.Prima_Estensione != OldS.Prima_Estensione) {
               OldS.Prima_Estensione = NewS.Prima_Estensione;
               OldStaz.ModifyCurrentRecord();
            } /* endif */
         } /* endif */
      }
   }
   OldStaz.Flush();

   ARRAY_ID Anomali;
   Anomali += 4567 ;
   Anomali += 4671 ;
   Anomali += 4768 ;
   Anomali += 4925 ;
   Anomali += 4929 ;
   Anomali += 5028 ;
   Anomali += 5069 ;

   STAZIONI_ALL_PER_NOME OldStaNome("\\wrk3\\");
   STAZIONI_ALL_PER_NOME Out2("\\wrk3\\","ID_STALL.MOD");
   Out2.Clear();

   ORD_FORALL(OldStaNome,j){
      if (!Anomali.Contiene(OldStaNome[j].IdStazione)) {
         Out2.AddRecord(BUFR(&OldStaNome[j],sizeof(STAZIONI_ALL_PER_NOME::R_STRU)));
      } else {
         printf("Cancellata stazione %s\n",OldStaNome[j].NomeStazione);
      } /* endif */
   }


   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   TRACETERMINATE;
      
   
   return rc;
}





