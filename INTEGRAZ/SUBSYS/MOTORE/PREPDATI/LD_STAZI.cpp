//----------------------------------------------------------------------------
// LD_STAZI : coricamento id_stazi su DB2
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

struct STAZIONI_PER_DB2 {
  char   CodiceRete              [ 2  ] ;
  char   CodiceStazione          [ 5  ] ;
  char   DescrizioneStazione35   [ 35 ] ;
  char   DescrizioneStazione12   [ 12 ] ;
  char   DescrizioneStazione7    [ 7  ] ;
  char   CodiceAusiliario        [ 5  ] ;
  char   CodiceTipoStazione      [ 2  ] ;
  char   CodiceCategoriaStazione [ 1  ] ;
  char   CodiceClasse            [ 3  ] ;
  char   FlagPrincipale          [ 1  ] ;
  char   CodiceStazioneMotore    [ 5  ] ;
  char   CodiceStazioneTpn       [ 5  ] ;
  char   CRLF                    [ 2  ] ;
};

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"

#include "FT_PATHS.HPP"
//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      
                                                                                    

// int main(int argc,char *argv[]){                                                   
int main(){
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          
                                                                                    
   // TRACEREGISTER2(NULL,NOME,NOME_TRACE);

   STAZIONI InFile(PATH_DATI);
   FILE_RW  OutFile(PATH_OUT "ID_STAZI.DB2");
   OutFile.SetSize(0);

   ORD_FORALL(InFile,i){
      STAZIONI::R_STRU & Staz = InFile[i];
      if(Staz.IdStazione == 0)continue;

      STAZIONI_PER_DB2 Out;
      ZeroFill(Out);

      STRINGA Tipo = "01";
      if(Staz.TipoImpiantoFisico == IMF_SCALO_MAR) Tipo = "02"; // Marittima

      #define Put1(_a,_b,_c)  STRINGA(Staz._a).ToFix(Out._b,_c);
      #define Put2(_a,_b,_c)  STRINGA(_a).ToFix(Out._b,_c);

      Put2( "83"         , CodiceRete               , 2  );
      Put1( CodiceCCR    , CodiceStazione           , 5  );
      Put1( NomeStazione , DescrizioneStazione35    , 35 );
      Put1( NomeStaz12   , DescrizioneStazione12    , 12 );
      Put1( Nome7        , DescrizioneStazione7     , 7  );
      Put2( ""           , CodiceAusiliario         , 5  );
      Put2( Tipo         , CodiceTipoStazione       , 2  );
      Put2( ""           , CodiceCategoriaStazione  , 1  );
      Put2( ""           , CodiceClasse             , 3  );
      Put2( ""           , FlagPrincipale           , 1  );
      Put1( IdStazione   , CodiceStazioneMotore     , 5  );
      Put1( CodiceTPN    , CodiceStazioneTpn        , 5  );
      Put2( "\r\n"       , CRLF                     , 2  );
      if(Staz.CodiceCCR == 0) Put1( CCRCumulativo1 , CodiceStazione , 5  );

      OutFile.Scrivi(&Out, sizeof(Out));
   };

   return 0;
}

