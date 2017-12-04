//----------------------------------------------------------------------------
// DU_DATCL: DUMP dei dati dei cluster in formato finale
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  1

#include "BASE.HPP"
#include "FILE_RW.HPP"
#include "ID_STAZI.HPP"
#include "alfa0.hpp"
#include "mm_perio.hpp"


#define NOME "DAT_CLUS"
//----------------------------------------------------------------------------
#include "FT_PATHS.HPP"  // Path da utilizzare
#define NOME_TRACE     PATH_DUMMY NOME ".TRC"
//----------------------------------------------------------------------------
#define MOSTRA_PERIODICITA
//----------------------------------------------------------------------------

class  PERIO_FILE : public FILE_FIX {
   public:
   PERIO_FILE(): FILE_FIX(PATH_OUT "PERIODIC.DB", sizeof(PERIODICITA),0) {};
   PERIODICITA &  operator [](ULONG Indice){ Posiziona(Indice); return *(PERIODICITA*) RecordC; };
};


STAZIONI * pStazioni;
PERIO_FILE Periodicita;

//----------------------------------------------------------------------------      
// Main                                                                             
//----------------------------------------------------------------------------      
                                                                                    
int  main(int argc,char *argv[]){                                                   
   #undef TRCRTN                                                                    
   #define TRCRTN "Main()"                                                          

   TRACEREGISTER2(NULL,NOME,NOME_TRACE);

   int MinCluster, MaxCluster;
   if(argc < 2){
      puts("Utilizzo: DU_DATCL MinCluster [MaxCluster]");
      return 999;
   };

   MinCluster = atoi(argv[1]);
   MaxCluster = MinCluster; 
   if(argc >=3)MaxCluster = atoi(argv[2]);
   printf("Analizzo i dati dei clusters scrivendo il risultato sul trace:  %s\n",NOME_TRACE);

   STAZIONI OStazioni(PATH_DATI,"ID_STAZI.DB",0);
   pStazioni = &OStazioni;

   CLASSIFICA_TRENI::CaricaClassifiche(PATH_DATI);

   T_PERIODICITA::Init(PATH_IN,"DATE.T");
   PERIODICITA::ImpostaProblema( T_PERIODICITA::Inizio_Dati_Caricati, T_PERIODICITA::Inizio_Orario_FS, T_PERIODICITA::Fine_Orario_FS, T_PERIODICITA::Inizio_Dati_Caricati);

   CLUSTA Clusters(PATH_OUT);
   FILE_RO ExtClusters(PATH_OUT "CLUSSTAZ.EXT");

   ORD_FORALL(Clusters,i){
      Clusters.Posiziona(i);
      CLUSTERS_STAZ & Cs = Clusters.RecordCorrente();
      if( Cs.IdCluster < MinCluster)continue;
      if( Cs.IdCluster > MaxCluster)break;
      Bprintf3("======================================================================");
      Bprintf("Cluster Nø %i Tipo %i %i->%i Offset %i DimDatiC %i DimDatiD %i", Cs.IdCluster, Cs.TipoCluster, Cs.Id1, Cs.Id2, Cs.OffSetBytes, Cs.DimDatiC, Cs.DimDatiD);
      Bprintf3("======================================================================");
      if(Cs.DimDatiC > 0){
         Bprintf3(">>>>>>>>>>>>>>  Dati concordi: ");
         CLU_BUFR Buf;
         ExtClusters.Leggi(Cs.DimDatiC,Cs.OffSetBytes,Buf);
         Buf.Set();
         Buf.Concorde = 1;
         Buf.Dump(0,0);
      }
      if(Cs.DimDatiD > 0){
         Bprintf3(">>>>>>>>>>>>>>  Dati Discordi: ");
         CLU_BUFR Buf;
         ExtClusters.Leggi(Cs.DimDatiD,Cs.DimDatiC+Cs.OffSetBytes,Buf);
         Buf.Set();
         Buf.Concorde = 0;
         Buf.Dump(0,0);
      }
   }

   TRACETERMINATE;
   return 0;
}

// Questa e' una versione modificata, per fare un rapido dump
void CLU_BUFR::Dump(ID Da, ID A){   
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::Dump()"
   
   STAZIONI & OStazioni = *pStazioni;

   if(Dat == NULL){
      ERRSTRING("Cluster vuoto: ???");
      BEEP;
      return;
   }

   int j,k;
   
   TRACESTRING("==========================================================");
   TRACESTRING("====== ANALISI DEL CLUSTER "+STRINGA(Dat->IdCluster) +"  ========");
   TRACESTRING("==========================================================");
   TRACESTRING("Senso Di Percorrenza: "+ STRINGA(Concorde ? "Normale" : "Inverso") );
   TRACEVLONG(Dat->IdCluster            );
   TRACEVLONG(Dat->NumeroNodi           );
   TRACEVLONG(Dat->TotStazioni          );
   TRACEVLONG(Dat->NumeroTreni          );
   TRACEVLONG(Dat->DimDati              );
   TRACELONG("Nodi del cluster: Nø ",Dat->NumeroNodi);
   for (j=0;j < Dat->NumeroNodi ; j++ ) { // Trace dei nodi
      TRACESTRING(" Nodo["+STRINGA(j)+STRINGA("] ") + STRINGA(Nodi[j].MediaKm)+ STRINGA(" KmMedi ") +
         " ["+STRINGA(Nodi[j].Gruppi)+"] " +STRINGA(Nodi[j].IdNodo) +" "+STRINGA(OStazioni[Nodi[j].IdNodo].NomeStazione));
   } /* endfor */
   TRACESTRING("Altre stazioni:");
   for (j=Dat->NumeroNodi;j < Dat->TotStazioni ; j++ ) { // Trace delle stazioni
      TRACESTRING(" Nodo["+STRINGA(j)+STRINGA("] ") + STRINGA(Nodi[j].MediaKm)+ STRINGA(" KmMedi ") +
         " ["+STRINGA(Nodi[j].Gruppi)+"] " +STRINGA(Nodi[j].IdNodo) +" "+STRINGA(OStazioni[Nodi[j].IdNodo].NomeStazione));
   } /* endfor */
   
   if(Tipo == CLUSTER_MULTISTAZIONE) { // Cluster multistazione
      for (k = 0; k < Dat->NumeroNodi; k++) {
         for (j = 0;  j < Dat->NumeroNodi; j++) {
            COLLEG_URBANO & Cu = Collegamento(k,j);
            TRACESTRING("Collegamento urbano tra stazioni "+STRINGA(Nodi[k].IdNodo)+" e "+STRINGA(Nodi[j].IdNodo)+" Km="+ STRINGA(Cu.Km)+" minuti="+STRINGA(Cu.Minuti));
         } /* endfor */
      } /* endfor */
   } else {
      FindTreno(0);
      {
      for (k=0 ;k < Dat->NumeroTreni; NextTreno(),k++ ) {
         {
            TRACELONG("Offset Treno ["+STRINGA(ProgTreno())+"]", ULONG(((BYTE*)Treno) - Dati));
            // Se non ferma ad origine e destinazione SKIP
            char Tre[6]; memmove(Tre,IdTrenoCorrente(0),5);Tre[5] = 0;
            STRINGA Prg="/"+STRINGA(Treno->ProgressivoPeriodicita );
            if(Treno->ProgressivoPeriodicita == 0)Prg.Clear();
            TRACESTRING(" ...................... MV["+STRINGA(k)+STRINGA("] ")+
               STRINGA(Treno->IdMezv)+Prg+" Treno "+STRINGA(Tre)+
               " ["+STRINGA(Treno->Gruppo)+"]"+
               " Classifica: "+STRINGA(Treno->InfoUniforme.TipoMezzo)+
               " "+MM_INFO::DecodTipoMezzo(Treno->InfoUniforme.TipoMezzo)+
               " LimiteTC: "+STRINGA(Treno->KmLimiteTrattaCorta)
            );
            #ifdef MOSTRA_PERIODICITA
            Periodicita[Treno->IdxPeriodicita].Trace("Periodicita' del mezzo virtuale (Idx = "+STRINGA(Treno->IdxPeriodicita)+") :");
            #endif
            WORD NNodi = Dat->NumeroNodi;
            for (j=0;j < NNodi;  j++ ) {
               INFOSTAZ & NodoInEsame = Nodo(j);
               BOOL Ferma    = NodoInEsame.Ferma();
               if(!Ferma && ! NodoInEsame.TransitaOCambiaTreno )continue; // Skip dei nodi su cui non ferma ne transita
               // Dati relativi al passaggio del treno sul nodo.
               // Desunti da dati generali e poi corretti con dati specifici
               short int Sosta = (NodoInEsame.Partenza && NodoInEsame.Arrivo) ?
               (NodoInEsame.OraPartenza - NodoInEsame.OraArrivo) : 0; // Durata della sosta
               STRINGA Msg;
               Msg = "Nodo["+STRINGA(j)+STRINGA("]");
               Msg += "  ["+STRINGA(NodoInEsame.ProgressivoStazione)+"]\t";
               //WORD Ora = (NodoInEsame.OraArrivo ? NodoInEsame.OraArrivo : NodoInEsame.OraPartenza);
               // Msg += ORA(Ora)+STRINGA("\t");
               Msg += ORA(NodoInEsame.OraArrivo)+STRINGA("->");
               Msg += ORA(NodoInEsame.OraPartenza)+STRINGA("\t");
               Msg += " ["+STRINGA(Nodi[j].Gruppi)+"]";
               if(Sosta> 1){ 
                  Msg += " Sosta "+STRINGA(Sosta)+" ";
               } else if(NodoInEsame.Arrivo && NodoInEsame.Partenza){
                  Msg += " Ferma ";
               } else if(NodoInEsame.Partenza){
                  Msg += " Parte ";
               } else if(NodoInEsame.Arrivo){
                  Msg += " Arriva ";
               } else if(NodoInEsame.TransitaOCambiaTreno){
                  Msg += " Transita ";
               }
               TRACESTRING(Msg+STRINGA(Nodi[j].IdNodo) +" "+STRINGA(OStazioni[Nodi[j].IdNodo].NomeStazione)); 
   //<<<    for  j=0;j < NNodi;  j++    
            } /* endfor */
         }
//<<< for  k=0 ;k < Dat->NumeroTreni; NextTreno  ,k++    
      } /* endfor */
      }
//<<< if Tipo == CLUSTER_MULTISTAZIONE    // Cluster multistazione
   } /* endif */
}
