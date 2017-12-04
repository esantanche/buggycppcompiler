#define PATHFILEDEFINIZIONE "C:\\integraz\\subsys\\motore\\prepdati\\"
//#define PATHFILEDEFINIZIONE "D:\\motore1\\prepdati\\"
#define NOMEFILEDEFINIZIONE "date.txt"
#define NOMEFILE "prv1.txt"


#define LIVELLI_DI_TRACE_DEL_PROGRAMMA 1

#include <elenco.h>
#include "base.hpp"
#include "alfa0.hpp"
#include "file_t.hpp"
#include "scandir.h"
#include "carica_t.hpp"

#define NumTreni  11


char GGMMAA::limitiMese[12]={31,29,31,30,31,30,31,31,30,31,30,31};

 
// Bisestile: anno divisibile per 4, non divisibile per 400 oppure divisibile per 2000
BOOL GGMMAA::Bisestile(int Anno)const{
   return (!(Anno % 4)) && ((Anno % 400) || !(Anno % 2000) );
};


BOOL GGMMAA::DaGGMMAA(STRINGA & From){
   // Converte in formato GGMMAA
   GG[0] = From[0];
   GG[1] = From[1];
   int Giorno= atoi(GG);
   if(Giorno < 1 || Giorno > 31) return FALSE;
   MM[2] = From[2];
   MM[3] = From[3];
   int Mese=atoi(MM);
   if(Mese < 1 || Mese > 12) return FALSE;
   if(Giorno > limitiMese[Mese])return FALSE;
   AA[4] = From[4];
   AA[5] = From[5];
   int Anno= atoi(AA);
   if(Anno < 1995 || Anno >2100 ) return FALSE;
   if(!Bisestile(Anno) && Mese == 2 && Giorno == 29)return FALSE;
   return TRUE;
}

 

 
int main()
{
   
   STRINGA PathFileDefinizione = PATHFILEDEFINIZIONE;
   STRINGA NomeFile            = NOMEFILE;
   PERIODIC Periodicita[55];
   
   TRACEREGISTER2(NULL,"CARICA_T","CARICA_T.TRC");
   
   T_PERIODICITA::Init(PATHFILEDEFINIZIONE, NOMEFILEDEFINIZIONE);
   
   // Mostro i dati
   // ORD_FORALL(T_PERIODICITA::PeriodicitaPerCodice,p){
   //    T_PERIODICITA::PeriodicitaPerCodice[p].HEXDUMP("PeriodicitaPerCodice["+STRINGA(p)+"]:");
   // };
   // TRACESTRING("FINE DUMP DEI DATI");
   
   
   
   
   STRINGA FileDatiTreni = PathFileDefinizione + NomeFile;
   // Adesso scandisco il file e carico le variabili di base
   if(FileDatiTreni != NUSTR && TestFileExistance(FileDatiTreni)) {
      FILE_RO Fil(FileDatiTreni);
      STRINGA Linea;
      ELENCO_S Toks = Linea.Tokens("=");
      STRINGA ID        = Toks[0];
      STRINGA NomeTreno = Toks[1];
      int i=0;
      while(Fil.gets(Linea)) {
         TRACESTRING2("Identificativo del treno:", Toks[0]);
            TRACESTRING2("Nome del treno:", Toks[1]);
         TRACESTRING(Linea);
         Periodicita[i].Codice[4]=Linea[4];
         Periodicita[i].Codice[5]=Linea[5];
         Periodicita[i].InizioPeriodo.DaGGMMAA(Linea(6, 11));
         Periodicita[i].FinePeriodo.DaGGMMAA(Linea(12, 17));
         i+=1;
      }
   }
   
   for(int i=2; i<NumTreni; i++){
      //prendi la periodicita del particolare treno dal file dei dati di prova
      
      PERIODICITA Out=T_PERIODICITA::ComponiPeriodicita(NumTreni, Periodicita);

      STRINGA Msg="Dump contenuto di T_PERIODICITA::PeriodicitaPerCodice["+STRINGA(i)+"]";
      //T_PERIODICITA::PeriodicitaPerCodice[i].Trace(Msg);
      Out.Trace(Msg, LIVELLO_DI_TRACE_DEL_PROGRAMMA, 6);
   }
   
   TRACETERMINATE;
   return 0;
} 



#include "ft_aux.hpp"
#include "ft_lib.cpp"
