//----------------------------------------------------------------------------
// FT_EXTR2: Estrazione pezzi selezionati di Taborari e localita
//----------------------------------------------------------------------------
// Estrae solo i records relativi ad un insieme selezionato di TRENI
// Questa e' la versione rapida che utilizza i risultati immagazzinati in PTRENO
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
#include "FT_PATHS.HPP"  // Path da utilizzare
#include "mm_varie.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "ML_IN.HPP"
#include "seq_proc.hpp"
#include "file_t.hpp"
#include "ctype.h"

#define PGM      "FT_EXTR"

const char Spaces[] = "                                                                                ";

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"
   
   TRACEREGISTER2(NULL,PGM, PATH_OUT PGM ".TRC");
   trchse = 2; // Forzo il tracelevel a 2
   GESTIONE_ECCEZIONI_ON
   
   PTRENO::Restore();
   
   printf(
      "Utilizzo: FT_EXTR2 [ListaTreni|@file]\n"
      " @file   : File contenente la lista dei treni (Commenti iniziano per ';')\n"
      "Per ulteriori opzioni si usi ft_extr\n\n");
   
   BIN_ELENCO_S Ok ;
   BOOL DaFile = FALSE;
   STRINGA NomeFile;

   for (int a = 1;a < argc ; a ++ ) {
      STRINGA Tmp(argv[a]);
      if(Tmp[0] == '/' ){
      } else if(Tmp[0] == '$' ){
      } else if(Tmp[0] == '%' ){
      } else if(Tmp[0] == '#' ){
      } else if(Tmp[0] == '@' ){
         DaFile = TRUE;
         NomeFile = Tmp(1, Tmp.Dim());
      } else {
         Ok += Tmp.Pad(10);
      }
   } /* endfor */
   
   
   if (DaFile) {
      FILE_RO ListaTreni(NomeFile);
      STRINGA Linea;
      while (ListaTreni.gets(Linea)) {
         if(Linea[0] == ';')continue; // Commento
         if(Linea[0] == '/')continue; // Commento
         Ok += Linea.Pad(10);
      } /* endwhile */
   }
   
   FILE_RW OutL(PATH_OUT "LOCALITA.XTR"); OutL.SetSize(0);
   FILE_RW OutT(PATH_OUT "TABORARI.XTR"); OutT.SetSize(0);
   FILE_RW OutI(PATH_OUT "INFOCOMM.XTR"); OutI.SetSize(0);
   
   // Apro l' archivio LOCALITA'
   FILE_LOCALITA Loc(PATH_IN "LOCALITA.T");
   
   // Apro l' archivio TABORARI
   FILE_TABORARI TabOr(PATH_IN "TABORARI.T");
   
   // Apro l' archivio INFOCOMM
   FILE_INFOCOMM InfCo(PATH_IN "INFOCOMM.T");
   
   Ok.Sort();
   ORD_FORALL(Ok,k){
      IDTRENO IdTreno = *(IDTRENO*)(CPSZ)Ok[k];
      PTRENO & Treno = *PTRENO::Get(IdTreno);
      
      for(int j = Treno.PTaborariR2; j < TabOr.Dim(); j++){
         TABORARI & To = TabOr[j];
         int Tr = To.TipoRecord - '0';
         if(Tr == 2 && j != Treno.PTaborariR2) break; // Sono arrivato al prossimo treno
         OutT.Scrivi(&To,sizeof(TABORARI));
      } /* endfor */
      
      for(int i = Treno.PLocalita ; i < Loc.Dim(); i++ ){
         LOCALITA & Lo = Loc[i];
         if(Lo.IdentTreno != IdTreno)break;
         OutL.Scrivi(&Lo,sizeof(LOCALITA));
      }
      
      for(int x = Treno.PInfocomm ; x < InfCo.Dim(); x++ ){
         INFOCOMM & Inf = InfCo[x];
         int Tr = Inf.TipoRecord - '0';
         if(Tr == 1 && Inf.R1.IdentTreno != Treno.IdTreno)break;
         OutI.Scrivi(&Inf,sizeof(INFOCOMM));
      };
//<<< ORD_FORALL Ok,k  
   };
   
   
   // ---------------------------------------------------------
   // Fine
   // ---------------------------------------------------------
   GESTIONE_ECCEZIONI_OFF
   TRACETERMINATE;
   
   return 0;
//<<< int main int argc,char *argv    
}

