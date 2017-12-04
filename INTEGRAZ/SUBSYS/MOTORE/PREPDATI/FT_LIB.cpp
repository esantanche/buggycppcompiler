//----------------------------------------------------------------------------
// FT_LIB.CPP : Funzioni generali per accesso ai files T
//----------------------------------------------------------------------------

#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1
#include "FT_PATHS.HPP"  // Path da utilizzare

#define MODULO_OS2_DIPENDENTE
#define INCL_DOSPROCESS

// begin EMS Win

#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_DOSMISC
// EMS Win #define INCL_DOSMODULEMGR
#define INCL_DOSPROFILE

//#include "windows.h"

/*
// definizioni prelevate da os2def.h che non è
// possibile includere tutto perché ci sono molte definizioni
// in conflitto con Windows (winbase.h)
typedef unsigned long  APIRET;
typedef unsigned long  LHANDLE;
typedef PVOID *PPVOID;
typedef LHANDLE PID;            /* pid  *
typedef LHANDLE TID;            /* tid  *
//typedef LHANDLE HMODULE;        /* hmod *
typedef HMODULE *PHMODULE;
typedef PID *PPID;
typedef TID *PTID;
typedef int ( APIENTRY _PFN)  ();
typedef _PFN *PFN;
typedef struct _QWORD          /* qword *
{
	ULONG   ulLo;
	ULONG   ulHi;
} QWORD;
typedef QWORD *PQWORD;
#define BOOL32 BOOL
#define PBOOL32 PBOOL
#define CPSZ const char *
// fine definizioni estratte da os2def.h
*/

typedef unsigned long BOOL;

#include "os2def.h"

extern "C" {
#include "bsedos.h"
}

// end EMS Win

#include "oggetto.h"
#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "alfa0.hpp"
#include "MM_PERIO.HPP"
#include "FT_AUX.HPP"
#include "FILE_T.HPP"
#include "ML_WRK.HPP"
// EMS Win #include "eventi.h"
#include <stdarg.h>


//----------------------------------------------------------------------------
// SetPriorita
//----------------------------------------------------------------------------
void _export SetPriorita(){ // Imposta la priorita'
	#undef TRCRTN
	#define TRCRTN "SetPriorita"
	// begin EMS Win
	//if(STRINGA(getenv("IDLE_PREPDATI"))=="SI"){
	//	DosSetPriority(PRTYS_THREAD,PRTYC_IDLETIME, 16,(LONG)Tid());
	//}
	// end EMS Win
}

//----------------------------------------------------------------------------
// ORARIO::MinMz
//----------------------------------------------------------------------------
WORD  _export ORARIO::MinMz()const {   // Conversione in minuti dalla mezzanotte
   #undef TRCRTN
   #define TRCRTN "ORARIO::MinMz"
   ULONG HH = StringToInt(Ora,2);
   ULONG MM = StringToInt(Minuti,2);
   return 60 * (HH % 24) + MM;
};


//----------------------------------------------------------------------------
// BIN_ELENCO_S::Bin_Contiene
//----------------------------------------------------------------------------
BOOL _export BIN_ELENCO_S::Bin_Contiene(const STRINGA & Target){
   #undef TRCRTN
   #define TRCRTN "BIN_ELENCO_S::Bin_Contiene"
   int Min = 0;
   int Max = Dim()-1;
   int i;
   do {
      i = (Min + Max ) /2;
      if(THIS[i] < Target){
         Min = i;
      } else if(THIS[i] == Target){
         return TRUE;
      } else {
         Max = i-1;
      };
      if(Min == Max - 1) { // Posizione di stallo
         if(THIS[Max] == Target) return TRUE;
         Max = Min;
      };
   } while (Min < Max);
   if(THIS[Max] == Target) return TRUE;
   return FALSE;
//<<< BOOL _export BIN_ELENCO_S::Bin_Contiene const STRINGA & Target  
};

ULONG LastTime;
ULONG LastTime2;
ULONG InitTime=0;
//----------------------------------------------------------------------------
// TryTime
//----------------------------------------------------------------------------
void _export TryTime(int i){
	#undef TRCRTN
	#define TRCRTN "TryTime"
	// EMS Win
	return;
	if(InitTime == 0){
		LastTime = LastTime2 = Time();
		InitTime = LastTime;
	};
	if(i == 0 || Time() - LastTime  > 1000){
		TRACEINT("Record Nø ",i);
		LastTime = Time();
	};
	if(Time() - LastTime2  > 3000){
		printf("Passati %i secondi Record Nø%i\n",(Time() - InitTime)/100,i);
		LastTime2 = Time();
	};
	if(i == 0)printf("Passati %i secondi\n",(Time() - InitTime)/100);
	// EMS Win
	return;
};

//----------------------------------------------------------------------------
// F_CLUSTER_MEZZOV::Compare
//----------------------------------------------------------------------------
int  _export F_CLUSTER_MEZZOV::Compare(const void * Key1,const void* Key2,ULONG DimConfronto){
   #undef TRCRTN
   #define TRCRTN "F_CLUSTER_MEZZOV::Compare"
   CLUSTER_MEZZOV & A = *(CLUSTER_MEZZOV *) Key1;
   CLUSTER_MEZZOV & B = *(CLUSTER_MEZZOV *) Key2;
   if(DimConfronto < 3 || (A.IdCluster      != B.IdCluster    ) ) return (int) A.IdCluster      - (int) B.IdCluster      ;
   if(DimConfronto < 4 || (A.VersoConcorde  != B.VersoConcorde) ) return (int) A.VersoConcorde  - (int) B.VersoConcorde  ;
   if(DimConfronto < 6 || (A.OraPartenza    != B.OraPartenza  ) ) return (int) A.OraPartenza    - (int) B.OraPartenza    ;
   return (int) A.MezzoVirtuale  - (int) B.MezzoVirtuale  ;
};

struct KEY_MV {
   WORD  MezzoVirtuale               ; // KEY: Mezzo virtuale
   BYTE  ProgressivoPeriodicita      ; // KEY
   WORD  ProgressivoFermata          ; // KEY: Progressivo fermata (in ambito treno virtuale, Parte da 1)
};
//----------------------------------------------------------------------------
// F_MEZZO_VIRTUALE_2::Compare
//----------------------------------------------------------------------------
int  F_MEZZO_VIRTUALE_2::Compare(const void * Key1,const void* Key2,ULONG DimConfronto){
   #undef TRCRTN
   #define TRCRTN "F_MEZZO_VIRTUALE_2::Compare"
   KEY_MV & A = *(KEY_MV *) Key1;
   KEY_MV & B = *(KEY_MV *) Key2;
   if(DimConfronto < 3 || (A.MezzoVirtuale  != B.MezzoVirtuale) ) return (int) A.MezzoVirtuale  - (int) B.MezzoVirtuale  ;
   return (int) A.ProgressivoPeriodicita - (int) B.ProgressivoPeriodicita;
};
//----------------------------------------------------------------------------
// F_MEZZO_VIRTUALE_2::KeySet
//----------------------------------------------------------------------------
void F_MEZZO_VIRTUALE_2::KeySet(){     // Imposta KeyCorrente a partire da RecordCorrente
   #undef TRCRTN
   #define TRCRTN "F_MEZZO_VIRTUALE_2::KeySet"
   KeyCorrente.Clear();
   if (NumRecords == 0)return;
   MEZZO_VIRTUALE_2 & Rec = *(MEZZO_VIRTUALE_2 *)RecordC;
   KeyCorrente.Store(&Rec.MezzoVirtuale,sizeof(Rec.MezzoVirtuale));
   KeyCorrente.Store(&Rec.ProgressivoPeriodicita,sizeof(Rec.ProgressivoPeriodicita));
   KeyCorrente.Store(&NumRec,sizeof(ULONG)); // Aggiungo indice
   this->ExtraSet();
   KeyCorrente.Pointer=0;
};
//----------------------------------------------------------------------------
// F_FERMATE_VIRT_2::Compare
//----------------------------------------------------------------------------
int  F_FERMATE_VIRT_2::Compare(const void * Key1,const void* Key2,ULONG DimConfronto){
   #undef TRCRTN
   #define TRCRTN "F_FERMATE_VIRT_2::Compare"
   KEY_MV & A = *(KEY_MV *) Key1;
   KEY_MV & B = *(KEY_MV *) Key2;
   if(DimConfronto < 3 || (A.MezzoVirtuale  != B.MezzoVirtuale) ) return (int) A.MezzoVirtuale  - (int) B.MezzoVirtuale  ;
   if(DimConfronto < 4 || (A.ProgressivoPeriodicita != B.ProgressivoPeriodicita ) ) return (int) A.ProgressivoPeriodicita  - (int) B.ProgressivoPeriodicita  ;
   return (int) A.ProgressivoFermata - (int) B.ProgressivoFermata ;
};
//----------------------------------------------------------------------------
// F_FERMATE_VIRT_2::KeySet
//----------------------------------------------------------------------------
void F_FERMATE_VIRT_2::KeySet(){     // Imposta KeyCorrente a partire da RecordCorrente
   #undef TRCRTN
   #define TRCRTN "F_FERMATE_VIRT_2::KeySet"
   KeyCorrente.Clear();
   if (NumRecords == 0)return;
   FERMATE_VIRT_2 & Rec = *(FERMATE_VIRT_2 *)RecordC;
   KeyCorrente.Store(&Rec.MezzoVirtuale,sizeof(Rec.MezzoVirtuale));
   KeyCorrente.Store(&Rec.ProgressivoPeriodicita,sizeof(Rec.ProgressivoPeriodicita));
   KeyCorrente.Store(&Rec.Progressivo,sizeof(Rec.Progressivo));
   KeyCorrente.Store(&NumRec,sizeof(ULONG)); // Aggiungo indice
   this->ExtraSet();
   KeyCorrente.Pointer=0;
};

//----------------------------------------------------------------------------
// @GEST_MULTI_STA
//----------------------------------------------------------------------------
GEST_MULTI_STA::GEST_MULTI_STA(const STRINGA& Path,FILE_RW * Fout, F_STAZIONE_MV & Fstaz){
   #undef TRCRTN
   #define TRCRTN "@GEST_MULTI_STA"
   // Carica i dati delle multistazioni FALSE su errore

	ID idStazioni_i4;  // EMS Win
	ID idStazioni_i5;  // EMS Win
	WORD Km = 0;   // EMS Win spostato fuori ciclo
	int i4;
	int i5;
	int iDimensione_multi_stazioni;

	NumCitta=0;

	TRACEVSTRING2(Path);
	FILE_RO Interscambio(Path+"TIME_LOC.UFF");
	STRINGA Linea;
	MULTI_STA * Multi= new MULTI_STA;
	ID Colleg[20][20];

	TEMPI_INTERSCAMBIO_1 Rec1;

	StazioniCoincidenzaEstesa.Clear();

	int Rg=0;
   ARRAY_ID StazioniValide;
	ELENCO_S Acronimi2;
   
   while (Interscambio.gets(Linea)) {
      char Tipo = Linea[3];
      if (Tipo == '1') {
         memmove(&Rec1,(CPSZ)Linea,Linea.Dim()+1); // COPIO la linea
         // Gestione citta'
         if(NumCitta < 63 ){
            if(Citta[NumCitta].Dim() > 2){ // La precedente citta' ha piu' di due stazioni
               if (++NumCitta >= 63) {
                  Bprintf("Raggiunto limite di 63 citta' multistazione ! ");
                  BEEP;
               } /* endif */
            } else {
               Citta[NumCitta].Clear();
            }
         }
         // Scarico dei precedenti dati
         if (StazioniValide.Dim() >= 2) {
            int Riga = 0, Colonna = 0;
            //TRACEVLONG(StazioniValide.Dim());
            Multi->Colleg  = new COLLEG_URBANO[StazioniValide.Dim() * StazioniValide.Dim()];
            // TRACEVPOINTER(Multi->Colleg);
            ORD_FORALL(Multi->Stazioni,i2){
               if( Multi->Stazioni[i2] == 0 ) continue;
               Colonna = 0;
               ORD_FORALL(Multi->Stazioni,i3){
                  if( Multi->Stazioni[i3] == 0 ) continue;
                  COLLEG_URBANO & Out = Multi->Colleg[Riga * StazioniValide.Dim() + Colonna];
                  if (i2 == i3) {
                     Out.Minuti = 0;
                  } else {
                     Out.Minuti = Colleg[i2][i3];
							//TRACESTRING("Riga, Colonna = "+ STRINGA(Riga) + "," + STRINGA(Colonna)+ " i2,i3 = " + STRINGA(i2) + "," + STRINGA(i3)+ " Out minuti = "+STRINGA(Out.Minuti));
						} /* endif */
                  Out.Km     = 0;
                  Colonna ++;
               }
               Riga ++;
            }
            Multi->Stazioni = StazioniValide;
            StazioniMS     += StazioniValide;
            Acronimi       += Acronimi2;
            if(GRAFO::Grafo){
					// Adesso debbo risolvere il grafo per trovare i KM
					// EMS Win
					iDimensione_multi_stazioni = (Multi->Stazioni).Dim(); // EMS Win
					//ORD_FORALL(Multi->Stazioni,i4){
					for (i4=0;i4<iDimensione_multi_stazioni;i4++) {
						//ORD_FORALL(Multi->Stazioni,i5){
						for (i5=i4;i5<iDimensione_multi_stazioni;i5++) {
							//if(i4 > i5) continue;
							// I KM sono valorizzati solo tra stazioni NON esclusivamente del servizio cumulativo
							// EMS Win
							idStazioni_i4 = Multi->Stazioni[i4];
							idStazioni_i5 = Multi->Stazioni[i5];
							if(i4 != i5 && GRAFO::Gr()[idStazioni_i4].StazioneFS &&
												GRAFO::Gr()[idStazioni_i5].StazioneFS){
								Km = GRAFO::Gr().DistanzaTra(idStazioni_i4, idStazioni_i5);
							}
							Multi->Colleg[i4 * (Multi->Stazioni.Dim()) + i5].Km = Km;
							Multi->Colleg[i5 * (Multi->Stazioni.Dim()) + i4].Km = Km;
						}
					}
				}
				ClustersMultiStazione += Multi;
				//ORD_FORALL(Multi->Stazioni,t1){
				//   ORD_FORALL(Multi->Stazioni,t2){
				//      COLLEG_URBANO & Cu = Multi->Colleg[t1 * Multi->Stazioni.Dim() + t2];
				//      TRACESTRING("Collegamento urbano tra stazioni "+STRINGA(Multi->Stazioni[t1])+" e "+STRINGA(Multi->Stazioni[t2])+" Km="+ STRINGA(Cu.Km)+" minuti="+STRINGA(Cu.Minuti));
				//   }
				//}
				Multi = NULL;
//<<<    if  StazioniValide.Dim   >= 2
			} /* endif */
			TRACESTRING2("Gestione multistazione per citta'",Rec1.Nome);
			if(Multi)delete Multi;
			// TRACESTRING("Alloc Multi");
			Multi = new MULTI_STA;
			//TRACEVPOINTER(Multi);
			Rg = 0;
			StazioniValide.Clear();
			Acronimi2.Clear();
//<<< if  Tipo == '1'
		} else if(Tipo == '2') {
			TEMPI_INTERSCAMBIO_2 & Rec2 = *(TEMPI_INTERSCAMBIO_2 *)(CPSZ)Linea;
         //TRACESTRING2("Stazione di ",St(Rec2.Denominazione));
         BOOL HaUnTempoInterscambio = FALSE ;
         for (int i = 0;i < It(Rec1.NumStazioni) ; i ++ ) {
            Colleg[Rg][i] = It(Rec2.TempiInterscambio[i]);
            if( It(Rec2.TempiInterscambio[i]))HaUnTempoInterscambio = TRUE;
            //TRACESTRING("Letto dato tra stazioni Idx = "+STRINGA(Rg)+","+STRINGA(i)+" ="+ STRINGA(Colleg[Rg][i])+"("+St(Rec2.TempiInterscambio[i])+")");
         } /* endfor */
         int CCR = It(Rec2.CodStazione);
         //TRACEVLONG(CCR);
         //TRACEVPOINTER(CCR_ID::CcrId);
         if (CCR == 0){
            if(Fout)Fout->printf("Non fornito il codice CCR: record = %s\n\r",(CPSZ)Linea);
            Multi->Stazioni +=  0;
         } else if( CCR_ID::CcrId->Id(CCR) <= 0) {
            if(Fout)Fout->printf("Codice CCR non valido: record = %s\n\r",(CPSZ)Linea);
            Multi->Stazioni +=  0;
         } else if( StazioniValide.Contiene(CCR_ID::CcrId->Id(CCR))){
            if(Fout)Fout->printf("Fornito codice CCR duplicato (o due codici equivalenti) per stazione %s\n\r",::Stazioni.DecodificaIdStazione(CCR_ID::CcrId->Id(CCR)));
            Multi->Stazioni +=  0;
         } else if( &Fstaz != NULL && Fstaz[CCR_ID::CcrId->Id(CCR)].NumClusters == 0){
            if(Fout)Fout->printf("Stazione non collegata con la rete FS %s\n\r",::Stazioni.DecodificaIdStazione(CCR_ID::CcrId->Id(CCR)));
            if(!Fout)printf("Stazione non collegata con la rete ferroviaria %s\n",::Stazioni.DecodificaIdStazione(CCR_ID::CcrId->Id(CCR)));
            Multi->Stazioni +=  0;
         } else {
            // Gestione citta'
            if(NumCitta < 63 ){
               Citta[NumCitta] +=  CCR_ID::CcrId->Id(CCR);
            }
            if( !HaUnTempoInterscambio){
               if(Fout)Fout->printf("Tempi di interscambio tutti nulli per stazione %s\n\r",::Stazioni.DecodificaIdStazione(CCR_ID::CcrId->Id(CCR)));
               Multi->Stazioni +=  0;
            } else {
               Multi->Stazioni +=  CCR_ID::CcrId->Id(CCR);
               Acronimi2 +=  St(Rec2.Acronimo);
               if(Acronimi2.Last() == STRINGA("   "))Acronimi2.Last() = "***";
               StazioniValide  +=  CCR_ID::CcrId->Id(CCR);
               if(Colleg[Rg][Rg]  > 0) StazioniCoincidenzaEstesa +=  CCR_ID::CcrId->Id(CCR);
            } /* endif */
//<<<    if  CCR == 0  
         } /* endif */
         Rg ++;
//<<< if  Tipo == '1'   
      } else {
         if(Fout)Fout->printf("??? Linea illegale su tabella tempi di interscambio: \"%s\"\n\r",(CPSZ)Linea);
         BEEP;
      } /* endif */
//<<< while  Interscambio.gets Linea    
   } /* endwhile */

   // Aggiungo villa san giovanni marittima che non e' riportata
   if(!StazioniCoincidenzaEstesa.Contiene(156)){
      ERRSTRING("Attenzione: manca villa san giovanni dall' elenco delle stazioni con");
      ERRSTRING("tempo di coincidenza esteso: La aggiungo a programma");
      StazioniCoincidenzaEstesa += 156;
   }
   
   
   TRACESTRING("Elenco delle MULTISTAZIONI");
   ORD_FORALL(StazioniMS,u)Bprintf3("Acronimo %s Stazione %i %s", (CPSZ)Acronimi[u],StazioniMS[u],Stazioni.DecodificaIdStazione(StazioniMS[u]));
   StazioniCoincidenzaEstesa.Trace(Stazioni,"Elenco delle stazioni con tempo di coincidenza esteso",2);
   TRACESTRING("Elenco delle Citta'");
   for (int c = 0;c < NumCitta ;c++ ) {
      ORD_FORALL(Citta[c],u)Bprintf3("Citta Nø %i Stazione %i %s",c,Citta[c][u],Stazioni.DecodificaIdStazione(Citta[c][u]));
   } /* endfor */
//<<< GEST_MULTI_STA::GEST_MULTI_STA const STRINGA& Path,FILE_RW * Fout, F_STAZIONE_MV & Fstaz  
}

//----------------------------------------------------------------------------
// MULTI_STA::Trace
//----------------------------------------------------------------------------
void MULTI_STA::Trace(STRINGA & Msg,int Livello){
   #undef TRCRTN
   #define TRCRTN "MULTI_STA::Trace"
   if(Livello > trchse)return;
   Stazioni.Trace(GRAFO::Gr(),Msg,Livello);
   char Buf[512];
   int i,j;
   char * Next;
   TRACESTRING("Matrice dei collegamenti in minuti");
   Next = Buf;
   Next += sprintf(Next," Stazioni : ");
   for (i=0;i < Stazioni.Dim() ; i++ )Next += sprintf(Next,"%5i",Stazioni[i]);
   TRACESTRING(Buf);
   for (i=0;i < Stazioni.Dim() ; i++ ){
      Next = Buf;
      Next += sprintf(Next,"  %5i    ",Stazioni[i]);
      for (j=0;j < Stazioni.Dim() ; j++ )Next += sprintf(Next,"%5i",Colleg[i*Stazioni.Dim()+j].Minuti);
      TRACESTRING(Buf);
   }
   TRACESTRING("Matrice dei collegamenti in Km ");
   Next = Buf;
   Next += sprintf(Next," Stazioni : ");
   for (i=0;i < Stazioni.Dim() ; i++ )Next += sprintf(Next,"%5i",Stazioni[i]);
   TRACESTRING(Buf);
   for (i=0;i < Stazioni.Dim() ; i++ ){
      Next = Buf;
      Next += sprintf(Next,"  %5i    ",Stazioni[i]);
		for (j=0;j < Stazioni.Dim() ; j++ )Next += sprintf(Next,"%5i",Colleg[i*Stazioni.Dim()+j].Km);
      TRACESTRING(Buf);
   }
//<<< void MULTI_STA::Trace STRINGA & Msg,int Livello  
};
//----------------------------------------------------------------------------
// GEST_MULTI_STA::Trace
//----------------------------------------------------------------------------
void GEST_MULTI_STA::Trace(const STRINGA & Msg,int Livello){
   #undef TRCRTN
   #define TRCRTN "GEST_MULTI_STA::Trace"
   if(Livello > trchse)return;
   TRACESTRING(Msg);
   StazioniMS.Trace(GRAFO::Gr(),"Elenco di tutte le multistazioni");
   ORD_FORALL(ClustersMultiStazione,k)ClustersMultiStazione[k]->Trace("Cluster MS Nø"+STRINGA(k),Livello);
};
//----------------------------------------------------------------------------
// Questa funzione si posiziona sul file dato il codice della nota.
// NB: Se la nota non esiste ritorna un NULL * (si deve testare) ed invalida
// il buffer del file (anche RecordC e' NULL e puo' essere testato)
//----------------------------------------------------------------------------
// FILE_NOTEPART::GetNota
//----------------------------------------------------------------------------
NOTEPART &  FILE_NOTEPART::GetNota(USHORT IdNota){
   #undef TRCRTN
   #define TRCRTN "FILE_NOTEPART::GetNota"
   
   static ARRAY_ID IdxNote; // Array in memoria di indicizzazione del file
   
   if (IdxNote.Dim() == 0) {
      ORD_FORALL(THIS,i){
         NOTEPART & Rec = THIS[i];
         if (Rec.TipoRecord == '1') {
            int IdNota = It(Rec.Nota);
            while (IdxNote.Dim() <= IdNota)IdxNote += 0xffff; // Aggiungo eventuali buchi nei codici di nota
            IdxNote[IdNota] = NumRecordCorrente();
         } /* endif */
      }
   } /* endif */
   
   if (IdNota >= IdxNote.Dim() || IdxNote[IdNota] == 0xffff) {
      InvalidateBuffer();
   } else {
      Posiziona(IdxNote[IdNota]);
   } /* endif */
   return RecordCorrente();
//<<< NOTEPART &  FILE_NOTEPART::GetNota USHORT IdNota  
};
//----------------------------------------------------------------------------
// FULL_INFO::ToStringa
//----------------------------------------------------------------------------
STRINGA FULL_INFO::ToStringa(){
   #undef TRCRTN
   #define TRCRTN "FULL_INFO::ToStringa"
   STRINGA Out = Decodifica(TRUE);
   if(Uniforme()) Out += " Uniforme";
   switch (TipoSRV) {
   case 0:
      Out += " Per tutto il mezzo ";
      if (DiMezzoVirtuale) {
         Out += "virtuale";
      } else {
         Out += "viaggiante";
      } /* endif */
      break;
   case 1:
      Out += " Si effettua alla fermata CCR "+ STRINGA(Da);
      break;
   case 2:
      Out += " Soppresso alla fermata CCR "+ STRINGA(Da);
      break;
   case 3:
      Out += " Dalla stazione con CCR "+ STRINGA(Da) + " a CCR "+STRINGA(A)+ " ed intermedie";
      if(MaxDa.Cod2())Out += " MaxDa = "+STRINGA(MaxDa);
      break;
   case 4:
      Out += " Dalla stazione con CCR "+ STRINGA(Da) + " a CCR "+STRINGA(A)+ " no intermedie";
      break;
//<<< switch  TipoSRV   
   } /* endswitch */
   if(PeriodicitaServizi != T_PERIODICITA::InOrario)Out += " " + PeriodicitaServizi.InChiaro();
   return Out;
   
//<<< STRINGA FULL_INFO::ToStringa   
};
//----------------------------------------------------------------------------
// FULL_NOTA::ToStringa
//----------------------------------------------------------------------------
STRINGA FULL_NOTA::ToStringa(){
   #undef TRCRTN
   #define TRCRTN "FULL_NOTA::ToStringa"
   STRINGA Out = "Nota Nø"+ STRINGA(CodiceNota);
   if(Uniforme()) Out += " Uniforme";
   switch (TipoNOT) {
   case 0:
      Out += " Per tutto il mezzo ";
      if (DiMezzoVirtuale) {
         Out += "virtuale";
      } else {
         Out += "viaggiante";
      } /* endif */
      break;
   case 1:
      Out += " Si effettua alla fermata CCR "+ STRINGA(Da);
      break;
   case 2:
      Out += " Soppresso alla fermata CCR "+ STRINGA(Da);
      break;
   case 3:
      Out += " Dalla stazione con CCR "+ STRINGA(Da) + " a CCR "+STRINGA(A)+ " ed intermedie";
      if(MaxDa.Cod2())Out += " MaxDa = "+STRINGA(MaxDa);
      break;
   case 4:
      Out += " Dalla stazione con CCR "+ STRINGA(Da) + " a CCR "+STRINGA(A)+ " no intermedie";
      break;
//<<< switch  TipoNOT   
   } /* endswitch */
   if(PeriodicitaNota != T_PERIODICITA::InOrario)Out += " " + PeriodicitaNota.InChiaro();
   return Out;
   
//<<< STRINGA FULL_NOTA::ToStringa   
};
//----------------------------------------------------------------------------
// INFO_AUX::ToStringa
//----------------------------------------------------------------------------
STRINGA INFO_AUX::ToStringa(){
   #undef TRCRTN
   #define TRCRTN "INFO_AUX::ToStringa"
   STRINGA Out = "MV Nø"+ STRINGA(MezzoVirtuale);
   if(Uniforme()) Out += " Uniforme";
   switch (Tipo) {
   case AUX_NOME       :
      Out += " Nome treno ID = "+STRINGA(Info);
      break;
   case AUX_CLASSIFICA :
      Out += " Classifica Id "+ STRINGA(Id) + " "+ MM_INFO::DecodTipoMezzo(Id);
      break;
   case AUX_NOTA       :
      Out += " Nota Nø " + STRINGA(Id);
      if(Info)Out += " Commerciale";
      break;
   case AUX_SERVIZIO   :
      Out +=  " Servizio "+ STRINGA(MM_INFO::DecServizio(Id,TRUE));
      break;
   }
   if (DiFermata) {
      Out += " Di FERMATA";
   } else if(DiMvg) {
      Out += " Di MEZZO VIAGGIANTE, MVG = "+ STRINGA(Mvg);
   } else {
      switch (TipoAUX) {
      case 0:
         Out += " Per tutto il mezzo virtuale";
         break;
      case 1:
         Out += " Si effettua alla fermata CCR "+ STRINGA(Da);
         break;
      case 2:
         Out += " Soppresso alla fermata CCR "+ STRINGA(Da);
         break;
      case 3:
         Out += " Dalla stazione con CCR "+ STRINGA(Da) + " a CCR "+STRINGA(A)+ " ed intermedie";
         if(MaxDa)Out += " MaxDa = "+STRINGA(MaxDa);
         break;
      case 4:
         Out += " Dalla stazione con CCR "+ STRINGA(Da) + " a CCR "+STRINGA(A)+ " no intermedie";
         break;
//<<< switch  TipoAUX   
      } /* endswitch */
//<<< if  DiFermata   
   } /* endif */
   if(Periodicita != T_PERIODICITA::InOrario)Out += " " + Periodicita.InChiaro();
   return Out;
   
//<<< STRINGA INFO_AUX::ToStringa   
};
//----------------------------------------------------------------------------
// INFO_AUX::Trace
//----------------------------------------------------------------------------
void INFO_AUX::Trace(const char * Msg){
   #undef TRCRTN
   #define TRCRTN "INFO_AUX::Trace"
   if(LIVELLO_DI_TRACE_DEL_PROGRAMMA > trchse)return;
   STRINGA DecodId(" ");
   if(Tipo == AUX_SERVIZIO  ) DecodId += MM_INFO::DecServizio(Id,FALSE);
   if(Tipo == AUX_CLASSIFICA) DecodId += MM_INFO::Categoria(Id);
   TRACESTRING("Mezzo virtuale: "+STRINGA(MezzoVirtuale) + " " + Msg);
   TRACESTRING( VRS(Tipo) + VRS(Id  )+ DecodId + VRS(Info) + VRS(TipoAUX) + VRS(Mvg ) + VRS(Da  ) + VRS(A   ) +
      VRS(MaxDa) + VRS(DiFermata ) + VRS(DiMvg     ) + VRS(Shift     ) );
   TRACEVSTRING2(Periodicita.InChiaro());
   // Periodicita.Trace("Periodicit…:");
};
