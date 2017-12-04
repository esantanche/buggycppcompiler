//----------------------------------------------------------------------------
// DU_DETTA: DUMP dei dati finali operativi di note e servizi
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

typedef unsigned long BOOL;

#include "ft_paths.hpp"

#include "std.h"
#include "oggetto.h"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
//#include "ML_OUT.HPP"
#include "MM_DETTA.HPP"
#include "elenco.h"
//#include "eventi.h"
#include "seq_proc.hpp"
#include "file_t.hpp"

/*
enum TIPODATO {
	NOME_MEZZO_VIRTUALE,    // 21 Caratteri indicanti il nome del treno
	NOTA_MEZZO_VIRTUALE,    // Nota del mezzo virtuale
	NOTA_FERMATA       ,
	NOTA_COMMERCIALE   ,
	NOTA_COMMERCIALE_BANALE ,
	SERVIZI_MV              // Servizi NON uniformi
};
*/

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int  main(int argc,char *argv[]){
	#undef TRCRTN
	#define TRCRTN "Main()"

	ELENCO_S Ok ;

	TRACEREGISTER2(NULL,"","DU_DETTA.TRC");
	FILE_RW Out2("DU_DETTA.OUT");
	Out2.SetSize(0);
	AddFileToBprintf(&Out2);

	Bprintf(
	" Opzioni:\n"
	" yyy     : Mostra i servizi e le note del mezzo virtuale yyy\n\n"
	"Scrivo il risultato su %sDU_DETTA.TXT\n",PATH_DUMMY );

	for (int a = 1;a < argc ; a ++ ) {
		STRINGA Tmp(argv[a]);
		if(Tmp[0] == '/' ){
			// Per ora ignoro i parametri
		} else {
         Tmp.Strip();
         Ok += Tmp;
		}
   } /* endfor */

   if(Ok.Dim() == 0){
      Bprintf("Nessun MV specificato!\n");
      return 4;
   }

   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   PERIODICITA::ImpostaProblema( T_PERIODICITA::Inizio_Dati_Caricati, T_PERIODICITA::Inizio_Orario_FS, T_PERIODICITA::Fine_Orario_FS, T_PERIODICITA::Inizio_Dati_Caricati);

   ELENCO_S TipoSrv(
      "0=Di Fermata"
		,"1=Tutto il MV"
      ,"2=Da stazione a stazione (e tutte le intermedie)"
      ,"3=Da stazione a Stazione (no intermedie)"
      ,"4=Su tutte le stazioni attrezzate"
      ,"5=NON ha il servizio per la data fermata"
   );

	FILE * Out;
   Out = fopen(PATH_DUMMY "DU_DETTA.TXT" ,"wt");

   F_DETTAGLI_MV   Dettagli(PATH_OUT "MM_DETMV.DB");
   FILE_RO         Dettagli2(PATH_OUT "MM_DETMV.EXT");

   ORD_FORALL(Ok,Tr){
      Bprintf("Elaborazione MV %s\n",(CPSZ)Ok[Tr]);
      ID Id = Ok[Tr].ToInt();
      fprintf(Out, "=====================================================================\n");
      fprintf(Out,"Elaborazione MV %i \n",Id);
      fprintf(Out, "=====================================================================\n");
      if(!Dettagli.Seek(Id)){
         fprintf(Out," Il MV %i non ha dati di note e/o servizi non uniformi \n",Id);
      } else {
			 BUFR Buf;
          Dettagli2.Leggi( Dettagli.RecordCorrente().DimDati, Dettagli.RecordCorrente().OffsetDati, Buf);
          Buf.Pointer = 0;
          while (Buf.Pointer < Buf.Dim()) {
             BYTE Tipo = Buf[Buf.Pointer] & 7; // Solo 3 Bits
             switch (Tipo) {
				 case NOME_MEZZO_VIRTUALE:
                {
                   NOME_MV & Rec = *(NOME_MV*)(Buf.Dati + Buf.Pointer);
                   fprintf(Out,"Nome mezzo virtuale = %s Su mezzi viaggianti %x\n",Rec.Nome,Rec.Mvg);
                   Buf.Pointer += Rec.NumBytes;
					 }
					 break;
				 /*
				 case NOTA_COMMERCIALE:
					 {
						 TRACESTRING("NOTA COMMERCIALE !!!!!!");
						 //NOTA_COM_MV & Rec = *(NOTA_COM_MV*)(Buf.Dati + Buf.Pointer);
						 //fprintf(Out,"Nota COM Id = %i da stazione Prog %i a stazione Prog %i Periodicita'Idx = %i\n", Rec.IdNota, Rec.PrgFermata1, Rec.PrgFermata2, Rec.IdxPeriodicita);
						 //Buf.Pointer += sizeof(NOTA_COM_MV);
					 }
					 break;
				 case NOTA_COMMERCIALE_BANALE:
					 {
						 TRACESTRING("NOTA COMMERCIALE BANALE !!!!!!");
						 //NOTA_COM_MV & Rec = *(NOTA_COM_MV*)(Buf.Dati + Buf.Pointer);
						 //fprintf(Out,"Nota COM BANALE Id = %i da stazione Prog %i a stazione Prog %i Periodicita'Idx = %i\n", Rec.IdNota, Rec.PrgFermata1, Rec.PrgFermata2, Rec.IdxPeriodicita);
						 //Buf.Pointer += sizeof(NOTA_COM_MV);
					 }
					 break;
				 */

				 case NOTA_MZV:
					 {
						 NOTA_MV & Rec = *(NOTA_MV*)(Buf.Dati + Buf.Pointer);
						 fprintf(Out,"Nota Id = %i Su mezzi viaggianti %x\n",Rec.IdNota,Rec.Mvg);
						 Buf.Pointer += sizeof(NOTA_MV);
					 }
					 break;
				 
				 case NOTA_FERMATA       :
                {
						 NOTA_FV & Rec = *(NOTA_FV*)(Buf.Dati + Buf.Pointer);
						 fprintf(Out,"Nota Id = %i Su fermata Id =  %x\n",Rec.IdNota,Rec.PrgFermata);
                   Buf.Pointer += sizeof(NOTA_FV);
                }
					 break;
				 /*
				 case SERVIZI_MV       :
					 {
						 SERVIZIO_MV Rec = *(SERVIZIO_MV *)(Buf.Dati + Buf.Pointer);
						 fprintf(Out,"Servizi: Tipo Servizi %s Servizi %s Da Prog1 %i a Prog2 %i Periodicita' Idx=%i\n",
							 (CPSZ)TipoSrv[Rec.TipoSRV], (CPSZ)Rec.Servizi.Decodifica(), Rec.PrgFermata1, Rec.PrgFermata1, Rec.IdxPeriodicita);
						 Buf.Pointer += sizeof(SERVIZIO_MV );
					 }
					 break;
				 */
				 default:
					 fprintf(Out,"??? Tipo nota non identificato = %i\n",Tipo);
					 Buf.Pointer = Buf.Dim(); // Fine del gioco
					 break;
				 } /* endswitch */
          } /* endwhile */
       }
   }

   fclose(Out);

	TRACETERMINATE;
   return 0;
//<<< void main(int argc,char *argv[]){
}

