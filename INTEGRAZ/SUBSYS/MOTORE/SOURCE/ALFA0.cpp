//----------------------------------------------------------------------------
// ALFA.CPP
//----------------------------------------------------------------------------
//
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA  2

#include "BASE.hpp"
#include "alfa0.hpp"
#include "MM_CRIT2.HPP"
#include "MM_PATH.hpp"
#include "MM_COMBI.hpp"
#include "mm_detta.hpp"

#define MAXC 10000 // Controllo Anti Loop

//----------------------------------------------------------------------------
// Debug opzionale
//----------------------------------------------------------------------------
//#define DBGN          // Seguo algoritmo percorso minimo: Dettagli
//#define DBG1          // Seguo algoritmo percorso minimo: Sommario
//#define DBG2          // Mostra dati dei nodi aggiunti
//#define DBG2A         // Mostra solo i nuovi dati e per i nodi aggiunti in un secondo tempo
//#define DBG3          // DIRAMAZIONE: Informazione di massima Step by step
//#define DBG4          // DIRAMAZIONE: dettaglio
//#define DBG5          // Dati di tutti i nodi e primi vicini dopo la fine di risolvi
//#define DBG6          // Risultati di Risolvi senza primi vicini (solo per nodi "done")
//#define DBG7          // Primi vicini dopo estensione
//#define DBG10         // Seguo IdentificaNodoDiDiramazione e CompletaPercorso
//#define DBG11         // Mostra tempi di imposta, risolvi ed estendi
//#define DBG12         // Segue Next() (anche con DBG3)
//#define DBG13         // Mostra il percorso minimo sul grafo da origine a destinazione
//#define DBG14         // Mostra TENTATIVI duplicati
//#define DBG15         // Debug DeterminaServizi()
//#define DBG15B        // Debug DeterminaServizi(): dettaglio periodicit…
//#define DBG16         // Debug Regionale()
//----------------------------------------------------------------------------
//#define LNAMES          // Fa mostrare i nomi completi delle stazioni sul trace
//----------------------------------------------------------------------------
#if defined(DBG2) && !defined(DBG2A)
#define DBG2A         // Mostra solo i nuovi dati e per i nodi aggiunti in un secondo tempo
#endif
//----------------------------------------------------------------------------

// Queste servono per rilevare i LOOPS : Usate da CompletaPercorso e GeneraDelta
static WORD Prg = 0xffff;
static WORD * Collision;

//----------------------------------------------------------------------------
// DATI_ORARIO_FS:: ImpostaProblema()
//----------------------------------------------------------------------------
// Prevede che StazionePartenza, StazioneDestinazione e Tolleranza siano
// impostati.

BOOL DATI_ORARIO_FS::ImpostaProblema( ID Origine, ID Destinazione, SDATA Data){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::ImpostaProblema()"
   
   GRAFO & Grafo = GRAFO::Gr();
   ElaborazioneInterrotta = FALSE;
   T_TRENO::PerDirettiPrenotabili = FALSE ; // E' il default
   
   ERRSTRING(" Inizio "+STRINGA(Origine)+" --> "+STRINGA(Destinazione)+" Data: "+STRINGA(Data)+" "+
      STRINGA(Grafo[Origine].Nome7())+ " ==> "+STRINGA(Grafo[Destinazione].Nome7())  );
   TRACESTRING(" Stazioni Target :"+ DATI_ORARIO_FS::StazioniTarget.ToStringa());
   
   SOLUZIONE::Allocator.Clear();     // Cancello tutte le soluzioni dal precedente problema
   
   if(ImpostazioneOK)this->Free();
   if(Origine == Destinazione) return FALSE;
   
   Activate(); // Carica da file se necessario . Di solito quando arriva qui e' gia' stato caricato dalla funzione (di interfaccia) SetOrarioDelGiorno
   
   NuoveSoluzioni = FALSE; // per il criterio di arresto
   
   // Imposto i parametri di periodicita' in accordo con la data
   PERIODICITA::ImpostaProblema(ValiditaOrario.Inizio,ValiditaOrario.InizioUfficiale,ValiditaOrario.FineUfficiale,Data);
   
   ARRAY_ID Or_Dest; Or_Dest += Origine; Or_Dest += Destinazione;
   
   // Calcola il percorso minimo tra origine e destinazionew: mi serve per un riferimento di distanza
   {
      ARRAY_ID OrDes;
      OrDes += Origine;
      OrDes += Destinazione;
      PERCORSO_GRAFO PercorsoMinimo;
      if ( Grafo[Origine].StazioneFS && Grafo[Destinazione].StazioneFS) {
         Grafo.AbilitaCumulativo = FALSE;
      } else {
         Grafo.AbilitaCumulativo = TRUE;
      } /* endif */
      if (PercorsoMinimo.Set(OrDes)) {
         DistanzaMinima = Grafo[Origine].Distanza1;
         DistanzaLimite = 3000; // Non accetto soluzioni con piu' di 3000 Km
         #ifdef DBG13
         TRACEVLONG(DistanzaMinima);
         PercorsoMinimo.Trace("Percorso Minimo su Grafo");
         #endif
      } else {
         ERRSTRING("Errori nell' impostazione del percorso sul grafo");
         return FALSE;
      } /* endif */
//<<< // Calcola il percorso minimo tra origine e destinazionew: mi serve per un riferimento di distanza
   }
   Grafo.AbilitaCumulativo = TRUE; // Per il lavoro successivo
   
   // Vedo se debbo usare i traghetti (verso la sardegna)
   int Abilitaz = Grafo[Origine].Abilitazione | Grafo[Destinazione].Abilitazione ;
   GestioneParticolareTraghetti = ( (Abilitaz & STAZ_FS::SARDEGNA) &&  (Abilitaz != STAZ_FS::SARDEGNA) );
   
   // ............................
   // Eventuale aggiunta di origine e destinazione come nodi fittizi
   // ............................
   if (!DefinisciNodoDiCambio(Origine)) { // L' aggiunta del nodo di cambio e' andata male
      ERRSTRING("Errori nell' impostazione dell' Origine");
      Free();
      return FALSE;
   } /* endif */
   if (!DefinisciNodoDiCambio(Destinazione)) { // L' aggiunta del nodo di cambio e' andata male
      ERRSTRING("Errori nell' impostazione della Destinazione");
      Free();
      return FALSE;
   } /* endif */
   
   IdStazioneOrigine      = Interno(Origine);
   IdStazioneDestinazione = Interno(Destinazione);
   TRACEVLONG(Interno(Origine)     );
   TRACEVLONG(Interno(Destinazione));
   PATH_CAMBI::ImpostaProblema(Origine, Destinazione, this); // Imposta anche per le classi collegate
   
   ImpostazioneOK = TRUE;
   
   IPOTESI::UsaDatiInCache = FALSE;
   
   #ifdef DBG11
   TRACEVLONG(DistanzaMinima);
   TRACESTRING("Fine");
   #endif
   return TRUE;
//<<< BOOL DATI_ORARIO_FS::ImpostaProblema  ID Origine, ID Destinazione, SDATA Data  
};
//----------------------------------------------------------------------------
// DATI_ORARIO_FS:: Free()
//----------------------------------------------------------------------------
void DATI_ORARIO_FS::Free(){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::Free()"
   
   TRACESTRING("Inizio Pulizia");
   Soluzioni.Clear();
   SOLUZIONE::Allocator.Clear();
   GRAFO::Grafo->Clear();
   
   // ............................
   // Gestione nodi di cambio fittizi
   // ............................
   while(LimiteNodi > BaseNodi) {
      ID Ext = Esterno(LimiteNodi);
      if (NODO_CAMBIO::TotNodiDiCambio > BaseNodi + 1) {
         // Questo test serve per gestire le situazioni in cui un nodo e' aggiunto ai vecchi dati ma non ai nuovi
         // a causa di errori
         NODO_CAMBIO & Nc = NodiDiCambio[NODO_CAMBIO::TotNodiDiCambio-1];
         SCAN_NUM(Nc.Collegamenti(),Nc.NumeroCollegamenti,Pv2,COLLEGAMENTO){
            NODO_CAMBIO & Altro = NodiDiCambio[Pv2.Id];
            BOOL Done = FALSE;
            SCAN_NUM(Altro.Collegamenti(),Altro.NumeroCollegamenti,Pv3,COLLEGAMENTO){
               if(Pv3.Id == NODO_CAMBIO::TotNodiDiCambio-1 ) {
                  // Vi sposto l' ultimo dei primi vicini
                  // Il -- aggiusta l' indirizzo e contemporaneamente anche il numero dei primi vicini
                  memmove(&Pv3,Altro.Collegamenti(--Altro.NumeroCollegamenti),sizeof(COLLEGAMENTO));
                  Done = TRUE;
                  break;
               } /* endif */
            } ENDSCAN ;
            assert2(Done,"Controllo di aver effettivamente cancellato i dati dei nodi fittizi dai collegamenti");
         } ENDSCAN ;
         NODO_CAMBIO::TotNodiDiCambio --;
      } /* endif */
      
      TabIdInterni[Ext] = 0;
      TabIdEsterni[LimiteNodi] = 0;
      LimiteNodi --;
//<<< while LimiteNodi > BaseNodi   
   } /* endif */
   // ............................
   
   ImpostazioneOK = FALSE;
   
   TRACESTRING("Fine");
//<<< void DATI_ORARIO_FS::Free   
};
//----------------------------------------------------------------------------
// DATI_ORARIO_FS::DefinisciNodoDiCambio()
//----------------------------------------------------------------------------
// Questa funzione aggiunge un nodo di cambio alla DATI_ORARIO_FS.
// Viene utilizzata:
//  - In fase di caricamento iniziale dei Dati Orario
//  - Successivamente quando l' origine o la destinazione
//    non siano nodi di cambio (vengono aggiunti come nodi di cambio fittizi)
//----------------------------------------------------------------------------
BOOL DATI_ORARIO_FS::DefinisciNodoDiCambio(ID Esterno){
   #undef TRCRTN
   #define TRCRTN "DATI_ORARIO_FS::DefinisciNodoDiCambio()"
   
   if(TabIdInterni[Esterno]!= 0)return TRUE; // Gia' definito
   
   #ifdef DBG2A
   if(NODO_CAMBIO::NodiDiCambio) TRACELONG("Aggiunta nodo di cambio temporaneo: Esterno = ", Esterno);
   #elif defined(DBG2)
   if(NODO_CAMBIO::NodiDiCambio){
      TRACELONG("Aggiunta nodo di cambio temporaneo: Esterno = ", Esterno);
   } else TRACELONG("Aggiunta nodo di cambio Permanente: Esterno = ", Esterno);
   #endif
   
   
   if(Esterno >= Grafo.TotStazioniGrafo){
      ERRINT("/* ==== ERRORE ==== */ Impossibile identificare stazione con id :",Esterno);
      BEEP;
      return FALSE;
   };
   STAZ_FS & Staz = Grafo[Esterno];
   if(!Staz.Vendibile){
      ERRSTRING("/* ==== ERRORE ==== */ Non e' attiva la stazione "+STRINGA(Esterno)+" :"+STRINGA(Stazioni.DecodificaIdStazione(Esterno)));
      // BEEP;     // Per il momento non do errore
      return FALSE;
   } /* endif */
   if(!FilStaz->Seek(Esterno)){
      ERRSTRING("/* ==== ERRORE ==== */ Non e' definita per l' orario la stazione "+STRINGA(Esterno)+" :"+STRINGA(Stazioni.DecodificaIdStazione(Esterno)));
      BEEP;
      return FALSE;
   }
   if (NODO_CAMBIO::NodiDiCambio == NULL && FilStaz->RecordCorrente().TipoStazione == 0) {
      ERRSTRING("/* ==== ERRORE ==== */ Non e' un nodo di cambio la stazione "+STRINGA(Esterno)+" :"+STRINGA(Stazioni.DecodificaIdStazione(Esterno)));
      BEEP;
      return FALSE;
   } /* endif */
   
   TabIdInterni[Esterno] = ++ LimiteNodi;
   TabIdEsterni[LimiteNodi] = Esterno;
   
   if(NODO_CAMBIO::NodiDiCambio){ // Altrimenti e' il caricamento iniziale
      #ifdef DBG2A
      DWORD Elapsed; Cronometra(TRUE,Elapsed);
      #endif
      Collider.Redim(Header.NumClusters+100);
      Collider.Reset();
      NODO_CAMBIO * Last  = & NODO_CAMBIO::NodiDiCambio[NODO_CAMBIO::TotNodiDiCambio-1];
      NODO_CAMBIO & Nuovo = *(Last +1);
      int IdxNuovo = Last->IdxCollegamenti + 2 + Last->NumeroCollegamenti ;
      int IdxNuovo2= Last->IdxClusters     + Last->NumeroClusters ;
      Nuovo.Id                               = Esterno                      ;
      Nuovo.IdxClusters                      = IdxNuovo2                    ;
      Nuovo.NumeroClusters                   = 0                            ;
      Nuovo.IdxCollegamenti                  = IdxNuovo                     ;
      Nuovo.NumeroCollegamenti               = 0                            ;
      BS_STAZIONE_F & FileStaz = * FilStaz;
      if(!FileStaz.Seek(Esterno)){
         ERRINT("/* ==== ERRORE ==== */ Impossibile accedere ai dati (MM_STAZN) della stazione con id :",Esterno);
         BEEP;
         return FALSE;
      };
      Nuovo.FascePartenza                    = FileStaz.RecordCorrente().FascePartenza       ;
      Nuovo.FasceArrivo                      = FileStaz.RecordCorrente().FasceArrivo         ;
      Nuovo.ClasseCoincidenza                = FileStaz.RecordCorrente().ClasseCoincidenza   ;
      Nuovo.Citta                            = FileStaz.RecordCorrente().Citta               ;
      // Carico i clusters
      F_STAZN_CLUS & FClusters = * StazCluster;
      for (FClusters.Seek(Esterno);FClusters.RecordCorrente().Stazione == Esterno ;FClusters.Next() ) {
         STAZN_CLUS & Scl = FClusters.RecordCorrente();
         STAZN_CLUS & Scl2 = NODO_CAMBIO::StzClusters[IdxNuovo2++];
         Collider.Set(Scl.IdCluster);
         Scl2 = Scl;
         Scl2.Stazione = Interno(Scl2.Stazione);
         Nuovo.NumeroClusters ++;
         #if defined(DBG2)
         TRACESTRING("Nodo "+STRINGA(Esterno)+" Aggiunta  relazione con cluster: "+STRINGA(Scl.IdCluster));
         #elif defined(DBG2A)
         if(NODO_CAMBIO::NodiDiCambio != NULL) TRACESTRING("Nodo "+STRINGA(Esterno)+" Aggiunta  relazione con cluster: "+STRINGA(Scl.IdCluster));
         #endif
      }
      // Carico i collegamenti: Poiche' i dati che ho sono unidirezionali debbo fare una scansione generale
      // per trovare le relazioni tra clusters e stazioni (e debbo tener conto dei gruppi)
      for (int i =NODO_CAMBIO::TotNodiDiCambio ; i > 0; i-- ) {
         // Questo e' il primo cluster del nodo di cambio in esame
         STAZN_CLUS * Clust  = NODO_CAMBIO::StzClusters + Last->IdxClusters;
         COLLEGAMENTO * Coll = NULL, *Coll2 = NULL;
         for (int j  = Last->NumeroClusters ; j > 0; j-- ) {
            if(Collider.Test(Clust->IdCluster)){ // OK Ha lo stesso cluster
               // Cerco l' analogo collegamento
               STAZN_CLUS * Clust2  = NODO_CAMBIO::StzClusters + Nuovo.IdxClusters;
               for (int k  = Nuovo.NumeroClusters ; k > 0; k-- ) {
                  if(Clust2->IdCluster == Clust->IdCluster)break; // Ne trovo sempre uno
                  Clust2 ++;
               }
               assert(Clust2->IdCluster == Clust->IdCluster);
               // Verifico che i gruppi coincidano
               if(Clust->Gruppi & Clust2->Gruppi){
                  // Acquisisco un nuovo collegamento per entrambi i nodi
                  if(Coll == NULL){
                     Coll = Last->Collegamenti(Last->NumeroCollegamenti ++);
                     Coll2= Nuovo.Collegamenti(Nuovo.NumeroCollegamenti ++);
                     // Li carico con dati di default
                     Coll->Id               = Interno(Nuovo.Id);
                     Coll->Km               = GRAFO::Gr().DistanzaTra(Nuovo.Id,Last->Id); // Calcolo la minima distanza sul grafo tra i nodi
                     Coll->PesoTratta       = 1;
                     Coll->Penalita         = 0; // Un valore vale l' altro: Non ho dati
                     Coll->OrariPartenza    = 0;
                     Coll->OrariArrivo      = 0;
                     Coll->TcollMin         = 0;
                     Coll->TcollMax         = 0;
                     Coll->Partenza32       = 0xffffffff;
                     Coll->Arrivo32         = 0xffffffff;
                     Coll2->Id              = Interno(Last->Id);
                     Coll2->Km              = Coll->Km;
                     Coll2->PesoTratta      = 1;
                     Coll2->Penalita        = 0; // Un valore vale l' altro: Non ho dati
                     Coll2->OrariPartenza   = 0;
                     Coll2->OrariArrivo     = 0;
                     Coll2->TcollMin        = 0;
                     Coll2->TcollMax        = 0;
                     Coll2->Partenza32      = 0xffffffff;
                     Coll2->Arrivo32        = 0xffffffff;
//<<<             if Coll == NULL  
                  };
                  // Determino il verso sul cluster
                  BOOL Concorde = Clust2->Progressivo >= Clust->Progressivo;
                  // Aggiorno orari di partenza ed arrivo per quanto possibile
                  if (Concorde) {
                     Coll->OrariPartenza    |= Clust->OrariPartenzaC;
                     Coll->OrariArrivo      |= Clust2->OrariArrivoC ;
                     Coll2->OrariPartenza   |= Clust2->OrariPartenzaD;
                     Coll2->OrariArrivo     |= Clust->OrariArrivoD   ;
                  } else {
                     Coll->OrariPartenza    |= Clust->OrariPartenzaD;
                     Coll->OrariArrivo      |= Clust2->OrariArrivoD ;
                     Coll2->OrariPartenza   |= Clust2->OrariPartenzaC;
                     Coll2->OrariArrivo     |= Clust->OrariArrivoC   ;
                  } /* endif */
//<<<          if Clust->Gruppi & Clust2->Gruppi  
               }
//<<<       if Collider.Test Clust->IdCluster    // OK Ha lo stesso cluster
            }
            Clust ++; // Test prossimo cluster
//<<<    for  int j  = Last->NumeroClusters ; j > 0; j--    
         }
         Last --; // Stazione di cambio precedente
//<<< for  int i =NODO_CAMBIO::TotNodiDiCambio ; i > 0; i--    
      } /* endfor */
      // Aggiustamenti finali
      NODO_CAMBIO::TotNodiDiCambio ++;
      #ifdef DBG2A
      TRACESTRING("Microsecondi per aggiunta nuovo nodo : "+STRINGA(Cronometra(FALSE,Elapsed)));
      TRACEVLONG(Nuovo.NumeroClusters   );
      TRACEVLONG(Nuovo.NumeroCollegamenti);
      #endif
//<<< if NODO_CAMBIO::NodiDiCambio   // Altrimenti e' il caricamento iniziale
   }
   
   #ifdef DBG2
   TraceIdEsterno("Aggiunta nodo di cambio per stazione: ",Esterno);
   #endif
   
   return TRUE;
//<<< BOOL DATI_ORARIO_FS::DefinisciNodoDiCambio ID Esterno  
}
//----------------------------------------------------------------------------
// DATI_ORARIO_FS::TraceIdInterno()
//----------------------------------------------------------------------------
// L' id deve essere un id interno
void DATI_ORARIO_FS::TraceIdInterno(const STRINGA& Msg, ID Id,int Livello){
   #undef TRCRTN
   #define TRCRTN " "
   if(Livello > trchse)return;
   STRINGA Out;
   Out = " IdNodo = " + STRINGA(Id)  +STRINGA(" ö ")+STRINGA(Esterno(Id));
   #ifdef LNAMES
   Out += STRINGA(" ") + Stazioni.DecodificaIdStazione(Esterno(Id));
   #else
   Out += STRINGA(" ") + Grafo[Esterno(Id)].Nome7();
   #endif
   Out += " ";
   ERRSTRING(Msg + Out);
};
//----------------------------------------------------------------------------
// DATI_ORARIO_FS::TraceIdEsterno
//----------------------------------------------------------------------------
void DATI_ORARIO_FS::TraceIdEsterno(const STRINGA& Msg, ID Id,int Livello){
   #undef TRCRTN
   #define TRCRTN " "
   if(Livello > trchse)return;
   STRINGA Out;
   Out = " IdNodo = " + STRINGA(Interno(Id))  +STRINGA(" ö ")+STRINGA(Id);
   #ifdef LNAMES
   Out += STRINGA(" ") + Stazioni.DecodificaIdStazione(Id);
   #else
   Out += STRINGA(" ") + Grafo[Id].Nome7();
   #endif
   Out += " ";
   ERRSTRING(Msg + Out);
};
void DATI_ORARIO_FS::TraceCluster(const STRINGA& Msg, ID Id,int Livello){
   #undef TRCRTN
   #define TRCRTN " "
   if(Livello > trchse)return;
   STRINGA Out;
   Out = " IdCluster= " + STRINGA(Id) ;
   CLUSTER & Clu = CLUSTER::Clusters[Id];
   Out += " (";
   Out += STRINGA(Clu.Id1)+"->"+STRINGA(Clu.Id2);
   Out +=" Tipo " +STRINGA(Clu.Tipo)+STRINGA(")");
   #ifdef LNAMES
   Out += STRINGA(" ") + Stazioni.DecodificaIdStazione(Clu.Id1);
   Out += STRINGA(" ") + Stazioni.DecodificaIdStazione(Clu.Id2);
   #else
   Out += STRINGA(" ") + Grafo[Clu.Id1].Nome7();
   Out += STRINGA(" ") + Grafo[Clu.Id2].Nome7();
   #endif
   Out += STRINGA(" ")+ STRINGA(max( Clu.DimDatiC , Clu.DimDatiD ))+ " Bytes";
   Out += " ";
   ERRSTRING(Msg + Out);
}
//----------------------------------------------------------------------------
// ORA()
//----------------------------------------------------------------------------
// E' rientrante 10 volte
const char * ORA(short int Ora){
   #undef TRCRTN
   #define TRCRTN "ORA"
   int Giorni = 0;
   if(Ora < 0)  Ora += 1440; // Per il passaggio attraverso la mezzanotte
   while (Ora > 1440) {
      Giorni ++;
      Ora -= 1440;
   } /* endwhile */
   static char OutBuf[200];
   static int  Idx ;
   char * Out = OutBuf + (20 * Idx);
   Idx = (Idx + 1)% 10;
   int hh = Ora / 60;
   int mm = Ora % 60;
   if(Giorni == 0){
      sprintf(Out,"%2.2i:%2.2i",hh,mm);
   } else {
      sprintf(Out,"%i gg %2.2i:%2.2i",Giorni,hh,mm);
   };
   return Out;
//<<< const char * ORA short int Ora  
};
//----------------------------------------------------------------------------
// SDATA
//----------------------------------------------------------------------------
BYTE SDATA::GiornoDellaSettimana(){ // 0 = Lunedi ...
   #undef TRCRTN
   #define TRCRTN "SDATA"
   static SDATA VecchioLunedi;
   if( VecchioLunedi.Anno == 0)VecchioLunedi.DaGGMMAAAA("07/01/1980");
   return (THIS  -  VecchioLunedi) % 7;
}
SDATA::operator STRINGA()const{
   return STRINGA(Giorno) + "/" +STRINGA(Mese)+"/"+STRINGA(Anno);
};
static char limitiMese[12]={31,29,31,30,31,30,31,31,30,31,30,31};
BOOL SDATA::Bisestile()const{
   // Bisestile: anno divisibile per 4, non divisibile per 100 oppure divisibile per 400
   return (!(Anno % 4)) && ((Anno % 100) || !(Anno % 400) );
};
BOOL SDATA::DaGGMMAAAA(const STRINGA & From){
   // Converte da formato "GG/MM/AAAA
   ELENCO_S GGmmAAAA = From.Tokens("/");
   if (GGmmAAAA.Dim() != 3)return FALSE;
   Giorno = GGmmAAAA[0].ToInt();
   if(Giorno < 1 || Giorno > 31) return FALSE;
   Mese = GGmmAAAA[1].ToInt();
   if(Mese < 1 || Mese > 12) return FALSE;
   if(Giorno > limitiMese[Mese-1])return FALSE;
   Anno = GGmmAAAA[2].ToInt();
   if(Anno < 1990 || Anno > 2100 ) return FALSE;
   if(!Bisestile() && Mese == 2 && Giorno == 29)return FALSE;
   return TRUE;
}
BOOL SDATA::operator >  (const SDATA & b)const{ return Anno > b.Anno || ((Anno == b.Anno ) && ( Mese > b.Mese || ((Mese == b.Mese) && Giorno > b.Giorno)));};
BOOL SDATA::operator <  (const SDATA & b)const{ return Anno < b.Anno || ((Anno == b.Anno ) && ( Mese < b.Mese || ((Mese == b.Mese) && Giorno < b.Giorno)));};
BOOL SDATA::operator == (const SDATA & b)const{ return Anno == b.Anno && Mese == b.Mese && Giorno == b.Giorno;};
BOOL SDATA::operator >= (const SDATA & b)const{ return THIS > b || THIS == b ;};
BOOL SDATA::operator <= (const SDATA & b)const{ return THIS < b || THIS == b ;};
BOOL SDATA::operator != (const SDATA & b)const{ return ! (THIS == b);};
SDATA & SDATA::operator ++(){
   Giorno ++;
   if (Giorno > limitiMese[Mese-1] || (Giorno == 29 && Mese == 2 && !Bisestile()) ) {
      Giorno = 1;
      Mese ++;
   } /* endif */
   
   if(Mese > 12){
      Mese = 1;
      Anno ++;
   };
   return THIS;
};
SDATA & SDATA::operator --(){
   Giorno --;
   if (Giorno == 0 ) {
      Mese --;
      if(Mese == 0){
         Anno --;
         Mese = 12;
         Giorno = 31;
      } else {
         Giorno = limitiMese[Mese - 1];
         if (Giorno == 29 && Mese == 2 && !Bisestile())Giorno --;
      } /* endif */
   } /* endif */
   return THIS;
};

int SDATA::Giuliano()const{
   static int  valInzMese[2][13]=
   // g f  m  a  m   g   l   a   s   o   n   d    g
   {  0,31,59,90,120,151,181,212,243,273,304,334,366,
      0,31,60,91,121,152,182,213,244,274,305,335,367
   };
   int bis= Bisestile() ? 1 : 0;
   return valInzMese[bis][Mese-1] + Giorno;
};

int  SDATA::operator - (const SDATA & b)const{ // Differenza tra due date
   
   int DeltaAnni = Anno - b.Anno;
   int DeltaGiorni = DeltaAnni * 365; // Differenza tra i 31/12 se non vi fossero anni bisestili
   int A1 = Anno-1;   // Per calcolare la differenza al 31/12 dell' anno precedente
   int NumBisestili  =  A1 / 4 - A1 / 100 + A1 / 400;
   DeltaGiorni += NumBisestili;
   A1 = b.Anno-1; // Per calcolare la differenza al 31/12 dell' anno precedente
   NumBisestili  =  A1 / 4 - A1 / 100 + A1 / 400;
   DeltaGiorni -= NumBisestili;      //  Differenza al 31/12
   DeltaGiorni += (Giuliano() - b.Giuliano());
   return DeltaGiorni;
};

//----------------------------------------------------------------------------
// CLU_BUFR::RidottoTempoCoinc
//----------------------------------------------------------------------------
// Questa funzione mi determina se il treno corrente e' regionale o comunque
// di un tipo che ha un ridotto tempo di coincidenza.
// Poiche' la classifica varia tratta per tratta si deve specificare a
// quale stazione ci si riferisca.
// Per le stazioni di cambio si considera il tutto regionale sia
// che sia regionale il treno in arrivo che il treno in partenza.
//----------------------------------------------------------------------------
BOOL __fastcall CLU_BUFR::RidottoTempoCoinc(BYTE P_Nodo){
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::RidottoTempoCoinc"
   
   if(Treno->InfoUniforme.ClassificaDisUniforme){
      
      BYTE * Pointer = (BYTE *)Dat + Dat->OffsetSrvTreni+ Treno->OffsetServizi ;
      #ifdef DBG16
      ERRSTRING(VRS(Treno->IdMezv) + VRS(P_Da) + VRS(P_A) + VRS(Treno->OffsetServizi));
      #endif
      
      // Prima di tutto leggo l' Header
      SRV_HDR & Header = *(SRV_HDR *)Pointer;
      
      // Le eventuali classifiche
      if (Header.NumClassifiche) {
         BYTE * Classifiche = Pointer + sizeof(SRV_HDR);
         INFOSTAZ & IstNodo = Treno->Nodi[P_Nodo];
         MM_INFO Wrk;
         Wrk.TipoMezzo = Classifiche[IstNodo.P_MezzoViaggiante];
         BOOL Reg = Wrk.RidottoTempoCoinc();
         #ifdef DBG16
         TRACESTRING(VRS(Reg)+ " Impostata Classifica a " +STRINGA(Wrk.TipoMezzo) + " "+ MM_INFO::DecodTipoMezzo(Wrk.TipoMezzo));
         #endif
         // Se la stazione e' di cambio allora devo includere il mezzo viaggiante precedente
         if(
            IstNodo.TransitaOCambiaTreno &&
            IstNodo.Arrivo               &&
            IstNodo.Partenza             &&
            IstNodo.P_MezzoViaggiante > 0
         ){
            Wrk.TipoMezzo = Classifiche[IstNodo.P_MezzoViaggiante-1];
            Reg |= Wrk.RidottoTempoCoinc();
            #ifdef DBG16
            TRACESTRING(VRS(Reg)+ " Staz Cambio: Impostata Classifica a " +STRINGA(Wrk.TipoMezzo) + " "+ MM_INFO::DecodTipoMezzo(Wrk.TipoMezzo));
            #endif
         };
         return Reg;
//<<< if  Header.NumClassifiche   
      }
//<<< if Treno->InfoUniforme.ClassificaDisUniforme  
   }
   return Treno->InfoUniforme.RidottoTempoCoinc();
//<<< BOOL __fastcall CLU_BUFR::RidottoTempoCoinc BYTE P_Nodo  
};

//----------------------------------------------------------------------------
// CLU_BUFR::DeterminaServizi
//----------------------------------------------------------------------------
// Questa funzione imposta servizi e classifica, dati gli indici
// delle stazioni limite della tratta.
// Opera sul MezzoVirtuale corrente
// P_Da : indice ( nel cluster ) della stazione di salita
// P_A  : indice ( nel cluster ) della stazione di discesa
// GG   : Data da considerare, espressa come offset da inizio orario
//        Se < 0 NON considera la periodicit… dei servizi
// Out  : Variabile di output, ove metto i servizi
// AnchePeriodicita : Se FALSE non considera la periodicit… ma solo la tratta
//----------------------------------------------------------------------------
void __fastcall CLU_BUFR::DeterminaServizi(BYTE P_Da, BYTE P_A, int GG, MM_INFO & Out){
   #undef TRCRTN
   #define TRCRTN "CLU_BUFR::DeterminaServizi"
   
   Out = Treno->InfoUniforme;
   #ifdef DBG15
   ERRSTRING("Info Uniforme: "+ Out.Decodifica()+ " MV " + STRINGA(Treno->IdMezv) +" Da ID " +STRINGA(Nodi[P_Da].IdNodo) +" Ad ID " +STRINGA(Nodi[P_A].IdNodo) );
   #endif
   if(!Out.Disuniforme)return;
   
   MM_INFO ServiziInibiti;
   MM_INFO ServiziAggiuntivi;
   DWORD & DwInibit = *(DWORD*) & ServiziInibiti;
   DWORD & DwAggiun = *(DWORD*) & ServiziAggiuntivi;   // Attivi su almeno una stazione della tratta
   DWORD   DwAggiun1= 0;                               // Attivi ad inizio tratta
   DWORD   DwAggiun2= 0;                               // Attivi a fine tratta
   ServiziInibiti.Clear();
   ServiziAggiuntivi.Clear();
   
   INFOSTAZ & IstDa = Treno->Nodi[P_Da];
   INFOSTAZ & IstA  = Treno->Nodi[P_A ];
   
   // Identifico i mezzi viaggianti coinvolti
   BYTE  MezziViaggianti=0;
   if(IstDa.P_MezzoViaggiante >   IstA.P_MezzoViaggiante){
      assert(IstDa.P_MezzoViaggiante <=  IstA.P_MezzoViaggiante);
      ERRSTRING(VRS(ProgTreno()) + VRS(Treno) + VRS(Treno->IdMezv) + VRS(P_Da) + VRS(P_A) );
      ERRSTRING( VRS(Nodo(P_Da).ProgressivoStazione) + VRS(Nodo(P_A).ProgressivoStazione));
      ERRSTRING( VRS(Nodi[P_Da].IdNodo) + VRS(Nodi[P_A].IdNodo));
   }
   for (BYTE x = IstDa.P_MezzoViaggiante; x <=  IstA.P_MezzoViaggiante; x++) {
      MezziViaggianti |= 1 << x;
   } /* endfor */
   
   // NO! La stazione di cambio punta gi… al mezzo viaggiante seguente
   // // Se la stazione finale della tratta e' stazione di cambio allora
   // // devo anche includere il mezzo viaggiante seguente
   // if(IstA.TransitaOCambiaTreno && IstA.Arrivo && IstA.Partenza ){
   //    MezziViaggianti |= 1 << x;
   // }
   
   // Se la stazione iniziale della tratta e' stazione di cambio allora
   // devo anche includere il mezzo viaggiante precedente
   if(
      IstDa.TransitaOCambiaTreno &&
      IstDa.Arrivo               &&
      IstDa.Partenza             &&
      IstDa.P_MezzoViaggiante > 0
   ){
      MezziViaggianti |= 1 << (IstDa.P_MezzoViaggiante-1);
   }
   
   //if(Treno->OffsetServizi > 5000){
   // if(Treno->IdMezv == 12339){
   //    BEEP;
   //    ERRSTRING("Info Uniforme: "+ Out.Decodifica()+ " MV " + STRINGA(Treno->IdMezv) +" Da ID " +STRINGA(Nodi[P_Da].IdNodo) +" Ad ID " +STRINGA(Nodi[P_A].IdNodo) );
   //    ERRSTRING(VRS(Treno->IdMezv) + VRS(MezziViaggianti)+ VRS(P_Da) + VRS(P_A) + VRS(Treno->OffsetServizi));
   //    ERRSTRING(VRS(IstDa.P_MezzoViaggiante) + VRS(IstA.P_MezzoViaggiante) + VRS(Dat->OffsetSrvTreni) + VRS(Dim()));
   // };
   
   BYTE * Pointer = (BYTE *)Dat + Dat->OffsetSrvTreni+ Treno->OffsetServizi ;
   #ifdef DBG15
   ERRSTRING(VRS(Treno->IdMezv) + VRS(MezziViaggianti)+ VRS(P_Da) + VRS(P_A) + VRS(Treno->OffsetServizi));
   BYTE * PointerIniziale = Pointer;
   #endif
   
   // Prima di tutto leggo l' Header
   SRV_HDR & Header = *(SRV_HDR *)Pointer;
   Pointer += sizeof(SRV_HDR);
   
   // Le eventuali classifiche
   if (Header.NumClassifiche) {
      BYTE * Classifiche = Pointer;
      Pointer += Header.NumClassifiche;
      Out.TipoMezzo = Classifiche[IstDa.P_MezzoViaggiante];
      int P_MV_A = IstA.P_MezzoViaggiante;
      // Se la stazione finale della tratta e' stazione di cambio allora
      // mi devo limitare al precedente
      if(
         IstA.TransitaOCambiaTreno &&
         IstA.Arrivo               &&
         IstA.Partenza             &&
         IstA.P_MezzoViaggiante > IstDa.P_MezzoViaggiante
      ){
         P_MV_A = IstA.P_MezzoViaggiante - 1; // Mezzo precedente
      }
      Out.ClassificaDisUniforme = Out.TipoMezzo != Classifiche[P_MV_A];
      #ifdef DBG15
      TRACESTRING("Impostata Classifica a " +STRINGA(Out.TipoMezzo) + " "+ MM_INFO::DecodTipoMezzo(Out.TipoMezzo));
      #endif
      
   } /* endif */
   
   // Leggo i servizi di fermata
   for (int i = Header.GruppiFermata; i > 0 ; i-- ) {
      SRV_FERM & ServiziFermata = *(SRV_FERM *) Pointer;
      #ifdef DBG15
      Bprintf3("Servizi su fermata Prg =  %i",ServiziFermata.ProgFermata,IdNodoDaProg(ServiziFermata.ProgFermata));
      STRINGA Msg;
      #endif
      BOOL Ok1= ServiziFermata.ProgFermata == IstDa.ProgressivoStazione;
      BOOL Ok2= ServiziFermata.ProgFermata == IstA.ProgressivoStazione ;
      SERV * Serv = ServiziFermata.Srv;
      if (Ok1 || Ok2) {
         while (Serv->Continua){
            DWORD Sft = 1 << Serv->Servizio ;
            if(Ok1) DwAggiun1 |= Sft;
            if(Ok2) DwAggiun2 |= Sft;
            #ifdef DBG15
            Msg += MM_INFO::DecServizio(Serv->Servizio,FALSE);
            Msg += ",";
            #endif
            Serv ++;
         };
         DWORD Sft = 1 << Serv->Servizio ;
         if(Ok1) DwAggiun1 |= Sft;
         if(Ok2) DwAggiun2 |= Sft;
         #ifdef DBG15
         Msg += MM_INFO::DecServizio(Serv->Servizio,FALSE);
         TRACESTRING("Servizi Aggiunti: "+Msg);
         #endif
      } else {
         while (Serv->Continua)Serv ++;
      } /* endif */
      Pointer = (BYTE *)(Serv+1);
//<<< for  int i = Header.GruppiFermata; i > 0 ; i--    
   } /* endfor */
   
   // Leggo i servizi di treno
   for (i = Header.GruppiTreno; i > 0 ; i-- ) {
      SRV_TRENO & ServiziTreno = *(SRV_TRENO *) Pointer;
      #ifdef DBG15
      Bprintf3("Servizi di treno su mezzi viaggianti %x Periodicit… Idx = %i", ServiziTreno.Mvg,ServiziTreno.IdxPeriodicita);
      STRINGA Msg;
      #endif
      BOOL Ok = ServiziTreno.Mvg & MezziViaggianti;
      BOOL Ok1 = ServiziTreno.Mvg & (1 << IstDa.P_MezzoViaggiante );
      if( IstDa.TransitaOCambiaTreno && IstDa.Arrivo && IstDa.Partenza && IstDa.P_MezzoViaggiante > 0 ){
         Ok1 |= ServiziTreno.Mvg & (1 << (IstDa.P_MezzoViaggiante-1) );
      }
      BOOL Ok2 = ServiziTreno.Mvg & (1 << IstA.P_MezzoViaggiante );
      if(IstA.TransitaOCambiaTreno && IstA.Arrivo && IstA.Partenza ){
         Ok2 |= ServiziTreno.Mvg & (1 << (IstA.P_MezzoViaggiante+1) );
      }
      if (Ok) {
         if ( GG >= 0 && ServiziTreno.IdxPeriodicita != 0) {  // Ha realmente periodicita'
            // Identifico la periodicita' in forma espansa
            // Si ricorda che la periodicit… e' sempre espressa in termini di data di
            // partenza del mezzo virtuale
            PERIODICITA & Perio = Orario.AllPeriodicita[ServiziTreno.IdxPeriodicita];
            // Valuto il giorno da considerare:
            Ok = Perio.Circola(GG);
            #ifdef DBG15B
            Perio.Trace(VRS(GG) + VRS(Ok));
            #endif
         } /* endif */
      } /* endif */
      SERV * Serv = ServiziTreno.Srv;
      if (Ok) {
         while (Serv->Continua){
            DWORD Sft = 1 << Serv->Servizio ;
            DwAggiun |= Sft;
            if(Ok1) DwAggiun1 |= Sft;
            if(Ok2) DwAggiun2 |= Sft;
            #ifdef DBG15
            Msg += MM_INFO::DecServizio(Serv->Servizio,FALSE);
            Msg += ",";
            #endif
            Serv ++;
         };
         DWORD Sft = 1 << Serv->Servizio ;
         DwAggiun |= Sft;
         if(Ok1) DwAggiun1 |= Sft;
         if(Ok2) DwAggiun2 |= Sft;
         #ifdef DBG15
         Msg += MM_INFO::DecServizio(Serv->Servizio,FALSE);
         TRACESTRING("Servizi Aggiunti: "+Msg);
         #endif
      } else {
         while (Serv->Continua)Serv ++;
//<<< if  Ok   
      } /* endif */
      Pointer = (BYTE *)(Serv+1);
//<<< for  i = Header.GruppiTreno; i > 0 ; i--    
   } /* endfor */
   
   // Leggo i servizi complessi
   for (i = Header.GruppiComplessi ; i > 0 ; i-- ) {
      SRV_CMPLX & ServiziComplex = *(SRV_CMPLX *) Pointer;
      #ifdef DBG15
      Bprintf3("Servizi COMPLX Tipo %i da stazione Prog %i Id %i a stazione Prog %i Id %i MaxIn Prog %i Id %i Periodicita'Idx = %i",
         ServiziComplex.Tipo,
         ServiziComplex.ProgDa , IdNodoDaProg(ServiziComplex.ProgDa) ,
         ServiziComplex.ProgA , IdNodoDaProg(ServiziComplex.ProgA) ,
         ServiziComplex.ProgMaxDa , IdNodoDaProg(ServiziComplex.ProgMaxDa) ,
         ServiziComplex.IdxPeriodicita);
      STRINGA Msg;
      #endif
      BOOL Ok = TRUE;
      BOOL Ok1= TRUE;
      BOOL Ok2= TRUE;
      BOOL Inibit = FALSE;
      if (GG >= 0 && ServiziComplex.IdxPeriodicita != 0) {  // Ha realmente periodicita'
         // Identifico la periodicita' in forma espansa
         // Si ricorda che la periodicit… e' sempre espressa in termini di data di
         // partenza della soluzione (e' cioe' gia' shiftata)
         // PERIODICITA & Perio = Per(ServiziComplex.IdxPeriodicita);
         PERIODICITA & Perio = Orario.AllPeriodicita[ServiziComplex.IdxPeriodicita];
         // Valuto il giorno da considerare:
         Ok = Perio.Circola(GG);
         #ifdef DBG15B
         Perio.Trace(VRS(GG) + VRS(Ok));
         #endif
      } /* endif */
      if (Ok) {
         // Debbo vedere se e' applicabile
         switch (ServiziComplex.Tipo) {
         case 0: // MV
            break;
         case 1: // Di fermata
            Ok1 = ServiziComplex.ProgDa == IstDa.ProgressivoStazione;
            Ok2 = ServiziComplex.ProgDa == IstA.ProgressivoStazione;
            Ok = TRUE;
            break;
         case 2: // Soppressa a fermata, attivo alle altre
            // Ok rimane TRUE per indicare che la periodicit… e' OK
            Inibit = ServiziComplex.ProgDa == IstDa.ProgressivoStazione;
            Inibit |= ServiziComplex.ProgDa == IstA.ProgressivoStazione;
            break;
         case 3: // Da-A ed intermedie
            Ok  = IstDa.ProgressivoStazione <  ServiziComplex.ProgA  && IstA.ProgressivoStazione >= ServiziComplex.ProgDa;
            Ok &= ServiziComplex.ProgMaxDa == 0 || IstDa.ProgressivoStazione <=  ServiziComplex.ProgMaxDa;
            Ok1  = ServiziComplex.ProgDa <= IstDa.ProgressivoStazione && IstDa.ProgressivoStazione < ServiziComplex.ProgA ;
            Ok2  = ServiziComplex.ProgDa < IstA.ProgressivoStazione && IstA.ProgressivoStazione <= ServiziComplex.ProgA ;
            break;
         case 4: // Da-A esatta
            Ok  = IstDa.ProgressivoStazione == ServiziComplex.ProgDa && IstA.ProgressivoStazione == ServiziComplex.ProgA ;
            break;
//<<<    switch  ServiziComplex.Tipo   
         } /* endswitch */
//<<< if  Ok   
      } /* endif */
      SERV * Serv = ServiziComplex.Srv;
      if (Ok) {
         if(Inibit){ // Inibizione servizio
            while (Serv->Continua){
               DWORD Sft = 1 << Serv->Servizio ;
               DwInibit |= Sft;
               #ifdef DBG15
               Msg += MM_INFO::DecServizio(Serv->Servizio,FALSE);
               Msg += ",";
               #endif
               Serv ++;
            };
            DWORD Sft = 1 << Serv->Servizio ;
            DwInibit |= Sft;
            #ifdef DBG15
            Msg += MM_INFO::DecServizio(Serv->Servizio,FALSE);
            TRACESTRING("Servizi Inibiti : "+Msg);
            #endif
         } else {
            while (Serv->Continua){
               DWORD Sft = 1 << Serv->Servizio ;
               DwAggiun |= 1 << Serv->Servizio ;
               if(Ok1)DwAggiun1 |= Sft;
               if(Ok2)DwAggiun2 |= Sft;
               #ifdef DBG15
               Msg += MM_INFO::DecServizio(Serv->Servizio,FALSE);
               Msg += ",";
               #endif
               Serv ++;
            };
            DWORD Sft = 1 << Serv->Servizio ;
            DwAggiun |= 1 << Serv->Servizio ;
            if(Ok1)DwAggiun1 |= Sft;
            if(Ok2)DwAggiun2 |= Sft;
            #ifdef DBG15
            Msg += MM_INFO::DecServizio(Serv->Servizio,FALSE);
            TRACESTRING("Servizi Aggiunti: "+Msg);
            #endif
//<<<    if Inibit   // Inibizione servizio
         }
//<<< if  Ok   
      } else {
         while (Serv->Continua)Serv ++;
      } /* endif */
      Pointer = (BYTE *)(Serv+1);
//<<< for  i = Header.GruppiComplessi ; i > 0 ; i--    
   } /* endfor */
   
   assert(Pointer <= (Dati + Dim()));
   
   #ifdef DBG15
   TRACEINT("Bytes complessivi di servizi e classifiche per la tratta : ", (Pointer - PointerIniziale));
   #endif
   
   // A questo punto debbo distinguere tra i servizi attivi solo se sono
   // attivi alle stazioni di partenza ed arrivo, e quelli che sono attivi
   // se lo sono in almeno una stazione.
   extern DWORD MaskDueStaz;
   DwAggiun1 &= DwAggiun2;    // Servizi combinati che ho sia in partenza che in arrivo
   DwAggiun1 &= MaskDueStaz;  // Mascherati
   DwAggiun  &= ~MaskDueStaz; // Reset sui servizi complessivi
   DwAggiun  |= DwAggiun1;    // E combinazione finale
   
   
   Out |= ServiziAggiuntivi;
   Out.NotIn(ServiziInibiti,FALSE);
   
//<<< void __fastcall CLU_BUFR::DeterminaServizi BYTE P_Da, BYTE P_A, int GG, MM_INFO & Out  
};
