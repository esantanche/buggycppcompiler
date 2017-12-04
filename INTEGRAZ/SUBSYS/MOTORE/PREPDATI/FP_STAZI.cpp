//----------------------------------------------------------------------------
// FP_STAZI:
// Crea il file ID_STINT.UPP con nomi ed ID delle stazioni UpperCase
// Crea il file ID_STANO.DB  con gli id delle stazioni FS ordinati per nome
// Crea il file ID_STALL.DB  con gli id delle stazioni italiane ordinati per nome
// Crea il file ID_STEST.DB  con gli id delle stazioni ordinati per nome
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

// begin EMS001 Win

#define INCL_DOSSEMAPHORES
#define INCL_DOSMISC
#define INCL_DOSMODULEMGR
#define INCL_DOSPROFILE

#include "os2def.h"
typedef unsigned long BOOL;

extern "C" {
#include "bsedos.h"
}

// end EMS Win

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "MM_VARIE.HPP"

#include "FT_PATHS.HPP"  // Path da utilizzare

//----------------------------------------------------------------------------
// SortNome
//----------------------------------------------------------------------------
int SortNome( const void *a, const void *b){
   #undef TRCRTN
   #define TRCRTN "SortNome"
   STAZIONI::R_STRU & A = *(STAZIONI::R_STRU *) a;
   STAZIONI::R_STRU & B = *(STAZIONI::R_STRU *) b;
   return stricmp(A.NomeStazione, B.NomeStazione);
};

//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------

int main(int argc,char *argv[]){
   #undef TRCRTN
   #define TRCRTN "Main()"

   // Abilito il TRACE
   TRACEREGISTER2(NULL,"","FP_STAZI.TRC");
   TRACEPARAMS(argc,argv);
   TRACEEXE(argv[0]);

   // Creazione dei files ausiliari
   { // Copio il file stazioni su di un temporaneo, che poi sorto per nome
      TRACESTRING("Copia di ID_STAZI Su di un temporaneo");
      FILE_RW Temp(PATH_TEMP "TMPSTAZI.TMP");
      FILE_RO Staz(PATH_DATI "ID_STAZI.DB" );
      BUFR * Buf = Staz.Leggi(Staz.FileSize());
      Temp.SetSize(0);
      Temp.Scrivi(*Buf);
   }

   {
      TRACESTRING("Sort del temporaneo");
      FILE_STZ Temp(PATH_TEMP "TMPSTAZI.TMP", 2048);
      Temp.PermettiScrittura();
      Temp.ReSort(SortNome); // Sort fisico del file

      // Files di output
      TRACESTRING("Scrittura dei files di indirizzamento");
      FILE_FIX SoloFs(PATH_DATI "ID_STANO.DB", 2 , 64000);
      FILE_FIX Italiane(PATH_DATI "ID_STALL.DB", 2 , 64000);
      FILE_FIX Tutte(PATH_DATI "ID_STEST.DB", 2 , 64000);
      SoloFs.Clear("Stazioni FS per Nome");
      Italiane.Clear("Stazioni Italiane per Nome");
      Tutte.Clear("Anagrafica completa per Nome");
      TRACESTRING("Prima di PutHeader per SoloFs");
      SoloFs.PutHeader(2,sizeof(ID),NstazDIM);     // Forzo il file a FILE_BS (per la prossima apertura)
      TRACESTRING("Dopo di PutHeader per SoloFs");
      Italiane.PutHeader(2,sizeof(ID),NstazDIM);   // Forzo il file a FILE_BS (per la prossima apertura)
      Tutte.PutHeader(2,sizeof(ID),NstazDIM);      // Forzo il file a FILE_BS (per la prossima apertura)
      ORD_FORALL(Temp,Idx){
         //TRACEINT("Idx=",Idx);
         Temp.Posiziona(Idx);
         //TRACESTRING("Dopo Temp.Posiziona(Idx)");
         STAZIONI::R_STRU & Rec = Temp.RecordCorrente();
         if(Rec.IdStazione == 0)continue;
         if(!Rec.Vendibile()) continue;
         Tutte.AddRecordToEnd(VRB(Rec.IdStazione));
         if(Rec.Fs())SoloFs.AddRecordToEnd(VRB(Rec.IdStazione));
         if(Rec.Nazionale())Italiane.AddRecordToEnd(VRB(Rec.IdStazione));
      }
   }

   // Creo un file ad indice con i codici CCR
   {
      TRACESTRING("Creazione file ad indice dei codici CCR");
      FILE_CCR FileCcr(PATH_TEMP "ID_CCR.TMP" , 64000 );
      TRACESTRING("Dopo creazione FileCcr");
      FileCcr.Clear("Indice: da codici CCR ad Id Stazioni");
      TRACESTRING("dopo FileCcr.Clear");
      STAZIONI Staz(PATH_DATI);
      TRACESTRING("dopo STAZIONI Staz(PATH_DATI);");
      ORD_FORALL(Staz,i){
         STAZIONI::R_STRU & Stz = Staz[i];
         CCR_STRU Ccr ;
         if (Stz.IdStazione) {
            Ccr.Id = Stz.IdStazione;
            if (Stz.CodiceCCR) {
               Ccr.CodiceCCR = Stz.CodiceCCR ;
               FileCcr.AddRecordToEnd(VRB(Ccr));
            } /* endif */
            if (Stz.CCRCumulativo1) {
               Ccr.CodiceCCR = Stz.CCRCumulativo1 ;
               FileCcr.AddRecordToEnd(VRB(Ccr));
            } /* endif */
            if (Stz.CCRCumulativo2) {
               Ccr.CodiceCCR = Stz.CCRCumulativo2 ;
               FileCcr.AddRecordToEnd(VRB(Ccr));
            } /* endif */
            if (Stz.CCREstero) {
               Ccr.CodiceCCR = Stz.CCREstero ;
               FileCcr.AddRecordToEnd(VRB(Ccr));
            } /* endif */
            if (Stz.CCREstero2) {
               Ccr.CodiceCCR = Stz.CCREstero2 ;
               FileCcr.AddRecordToEnd(VRB(Ccr));
            } /* endif */
            if (Stz.CCREstero3) {
               Ccr.CodiceCCR = Stz.CCREstero3 ;
               FileCcr.AddRecordToEnd(VRB(Ccr));
            } /* endif */
         } /* endif */
      };
      TRACESTRING("prima di FileCcr.ReSortFAST();");
      FileCcr.ReSortFAST();
   }

   // Copio il file CCR sulla corretta destinazione
   {
      TRACESTRING("Copia di ID_CCR ");
      FILE_RO Temp(PATH_TEMP "ID_CCR.TMP");
      FILE_RW Ccr(PATH_DATI "ID_CCR.DB" );
      BUFR * Buf = Temp.Leggi(Temp.FileSize());
      Ccr.SetSize(0);
      Ccr.Scrivi(*Buf);
   }

   TRACESTRING("Fine");
   TRACETERMINATE;

   return 0;
//<<< int  main int argc,char *argv
}

