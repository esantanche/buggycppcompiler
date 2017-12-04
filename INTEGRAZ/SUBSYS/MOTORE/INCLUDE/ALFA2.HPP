//========================================================================
// ALFA2.HPP 
//========================================================================
//

#ifndef HO_ALFA2_HPP
#define HO_ALFA2_HPP

#ifndef HO_MM_BASIC_HPP
#include "MM_BASIC.HPP"
#endif

//----------------------------------------------------------------------------
// Struttura per determinare le caratteristiche di un cluster all' interno di un percorso
//----------------------------------------------------------------------------

// La seguente struttura identifica un nodo di cambio
// PCambio e PCambioNext identificano il nodo (contengono
// l' indice nei due cluster tra cui avviene il cambio).
// Qualita mi serve per il sort
struct NCAM {                                                      // "ne TRC"
   WORD PCambio;      // Punta al nodo di cambio tra i nodi del cluster "prima"
   WORD PCambioNext;  // Punta al nodo di cambio tra i nodi del cluster "dopo"
   BYTE Qualita;      // Se = 0x80 il nodo e' OK, altrimenti e' una seconda scelta
   WORD DeltaKm;      // Se > 0 il nodo allunga il percorso (non imposto mai a numeri negativi)
   WORD Peso;
   // Per gli algoritmi di coincidenza
   NCAM(WORD a,WORD b,BYTE Qual,ID Id,WORD Delta);
   ~NCAM();
};


struct ARRAY_NCAM : public ARRAY_DINAMICA<NCAM> {                     // "ne TRC"
   ARRAY_NCAM(ULONG i=32):ARRAY_DINAMICA<NCAM>(i){};
   void Sort();
};


struct CLUSTER_PERCORSO {                                          // "ne TRC"
   CLUSTER_PERCORSO();
   ~CLUSTER_PERCORSO();
   ID         IdCluster   ;  // Esterno
   CLU_BUFR * DatiCluster ;  // Dati del cluster
   BYTE  SensoNormaleDiPercorrenza; // TRUE = Concorde  FALSE = Discorde (con il senso del Cluster)
   short Km1; // Km dal primo nodo (originale) del cluster al cluster
   short Km2; // Km dal secondo nodo (originale) del cluster al cluster
   
   // Per velocizzare gli algoritmi limitando lo spazio di ricerca
   GRUPPO GruppoIn ;           // Gruppo di ingresso
   GRUPPO GruppoOut;           // Gruppo di Uscita
   GRUPPO GruppoComposito;     // == GruppoIn & GruppoOut

   int LimiteTreni; // Numero limite dei treni che possono essere fuori ordine
                    // Due treni la cui posizione nel cluster sia > LimiteTreni sono
                    // sicuramente in ordine corretto di partenza

   // metodi di accesso a soluzione corrente
   NCAM & NodoDiCambio(){return NodiDiCambio[PNodoCambio];};
   ID     IdNodoDiCambio(){return DatiCluster->Nodi[NodiDiCambio[PNodoCambio].PCambio].IdNodo;};

   ARRAY_NCAM NodiDiCambio;  // Non contiene gli ID ma delle strutture NCAM 
                             // Rappresenta i nodi in cui si puo' cambiare ( LASCIARE il cluster )
   STRINGA Caratteristiche(BYTE PCambioPotenziale); // Per DEBUG

// Mi indica se un nodo puo' essere un nodo di salita o discesa
   BOOL NodoSignificativo(ID IdNodo,CLUSTER_PERCORSO * Precedente,BYTE P_Origine);
   
// Questi dati sono utilizzati dagli algoritmi per trovare le soluzioni
// Sono in particolare i dati della soluzione corrente
// Si noti che Treno e' necessario per gestire percorsi con cluster duplicati
   BOOL MultiS                       ; // Vero se il cluster e' multistazione
   int  PTreno                       ; // Indice del MV    corrente (Nel cluster)
   CLUSTRENO * Treno                 ; // Treno corrente
   ID   IdMezv                       ; // E suo Id
   BYTE SingoloIngresso              ; // Vero se ho un solo nodo possibile di ingresso al cluster (puntato da PNodoIn)
   int  PNodoIn                      ; // Indice del nodo con cui accedo al cluster (nei nodi del cluster)
   int  PNodoOut                     ; // Indice del nodo con cui esco dal cluster (nei nodi del cluster)
   int  PNodoCambio                  ; // Indice del nodo di cambio con cui esco dal cluster (indice in NodiDicambio)
   int  PosPTreno                    ; // Posizione di PTreno in NodiCambio[PNodiCambio].TreniFiltrati;
   WORD Attesa                       ; // Tempo di attesa DALLA TRATTA PRECEDENTE
   WORD OraInizioAttesa,OraFineAttesa; // Per poter escludere se necessario le ore dall' 1 alle 6
   WORD AttesaValida()               ; // Esclude le ore dall' 1 alle 6 di notte
   WORD OraArrivoTreno               ; // Ora arrivo MV 

   void CaricaTreni(WORD OraIniziale); // Per il primo Cluster
   void CaricaTreni()                ; // Per gli altri Clusters
   BUFR TreniFiltrati                ; // Treni filtrati che fermano al nodo di cambio
   // Contiene solo i treni che fermano al nodo di transito
   // Impostato dalla funzione caricaTreni
   // Ogni elemento e' un indice nei treni del cluster
   int  TrenoCorrente                ; // Indice del treno corrente  in TreniFiltrati

   // Posiziona al primo treno (in ordine di arrivo) che parte dopo una data ora
   BOOL Posiziona(WORD OraIniziale,WORD Giorni,BOOL Regionale); // Torna False su errore
   // Posiziona al primo treno (in ordine di partenza) che parte dopo una data ora
   BOOL PosizionaPrimo(WORD OraIniziale);                            // Per la sola stazione di partenza
   // Posiziona al treno successivo (in ordine di arrivo)
   BOOL PosizionaNext();                                             // Per la sola stazione di partenza
   BYTE Caricato; // Vero se ho caricato i treni
   void Clear();

   INFOSTAZ & NodoIn() { return Treno->Nodi[PNodoIn];};
   INFOSTAZ & NodoOut(){ return Treno->Nodi[PNodoOut];};

   // Questa funzione cerca un nodo tra i dati del cluster, e ne ritorna l' indice (o 999 se non trovato)
   WORD PosNodoInCluster(ID Id); // Id e' l' ID Esterno del nodo

//<<< struct CLUSTER_PERCORSO {
};                                   

/* 1G                  
                 ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
               ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป บ
             ษอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป บ บ
             บ                        CLUSTER_PERCORSO                   บ บ บ
             ฬอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออน บ บ
             บ IdCluster                                                 บ บ บ
             บ SensoNormaleDiPercorrenza                                 บ บ บ
             บ  ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤNodiDiCambio.Dim()ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ บ บ บ
             บ  ณ                                                      ณ บ บ บ
             บ  ษออออออออออออออออออออออออออออออออออออออออออออออออออออออป บ บ บ
             บ  บ                 ARRAY_NCAM NodiDiCambio              บ บ บ บ
             บ  ฬออออออออออออออออออออออออออหอออออออออออออออออออออออออออน บ บ บ
             บ  บ Qualita                  บ Qualita                   บ บ บ บ
             บ  บ Peso                     บ Peso                      บ บ
             บ  บ PCambio                  บ PCambio ฤฤฤฤฤ{Cluster Corrente}ฤฤฤฟ
             บ  บ PCambioNext              บ PCambioNext ฤ{Cluster Successivo}ฟณ
             บ  บษออออออออออออออออออออออออปบษออออออออออออออออออออออออป บ บ    ณณ
             บ  บบ BUFR TreniFiltrati     บบบ BUFR TreniFiltrati     บ บ บ บ บณณ
             บ  บฬออออออออออออออออออออออออนบฬออออออออออออออออออออออออน บ บ บ บณณ
       ฺฤฤฤฤฤฤฤฤฤฤPtreno                  บบบPtreno                  บ บ บ บ บณณ
       ณ     บ  บฬออออออออออออออออออออออออนบฬออออออออออออออออออออออออน บ บ บ บณณ
       ณฺฤฤฤฤฤฤฤฤฤPtreno                  บบบPtreno                  บ บ บ บ บณณ
       ณณ    บ  บศออออออออออออออออออออออออผบศออออออออออออออออออออออออผ บ บ บ บณณ
       ณณ    บ  บ                          บ                           บ บ บ บณณ
       ณณฺฤฤฤฤฤฤฤฤTrenoCorrente            บ TrenoCorrente             บ บ บอผณณ
       ณณณ   บ  ศออออออออออออออออออออออออออสอออออออออออออออออออออออออออผ บอผ  ณณ
       ณณณ   บ DatiCluster ฤฤฟ                                           บ    ณณ
       ณณณ   ศอออออออออออออออณอออออออออออออออออออออออออออออออออออออออออออผ    ณณ
       ณณณ                   V                                                ณณ
       ณณณ   ษออออออออออออออออออออออออออออออออออออออออออออออป                 ณณ
       ณณV   บ                  CLU_BUFR                    บ                 ณณ
       ณณV   ฬออออออออออออออออออออออออออออออออออออออออออออออน                 ณณ
       ณณฬฤฤฤฤฤ Treno (Punta Al Treno Corrente)             บ                 ณณ
       ณณณฺฤฤฤฤ FirstTreno                                  บ                 ณณ
       ณณณณฺฤฤฤ Nodi                                        บ                 ณณ
       ณณณณณ บ  Dat ฤฤฟ                                     บ                 ณณ
       ณณณณณ ศออออออออณอออออออออออออออออออออออออออออออออออออผ                 ณณ
       ณณณณณ          V                                                       ณณ
       ณณณณณ ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป ณณ
       ณณณณณ บ                        CLUSTERS_STAZ_EXT                     บ ณณ
       ณณณณณ ฬออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออน ณณ
       ณณณณณ บ NumeroNodi                                                   บ ณณ
       ณณณณณ บ NumeroTreni                                                  บ ณณ
       ณณณณณ บ                                             ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤูณ
       ณณณณณ บ                                             ณ         ฺฤฤฤฤฤฤฤฤฤู
       ณณณณณ บ                    ฺฤฤฤฤฤฤฤNumeroNodiฤฤฤฤฤฤฤณฤฤฤฤฤฤฤฤฤณฤฤฤฤฟ บ
       ณณณณณ บ                    ณ                        V         V    ณ บ
       ณณณณณ บ                    ณ                        V         V    ณ บ
       ณณณณภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ>ษอออออออออออออออออออออออออออออออออออออออป บ
       ณณณณ  บ                    บ   CLUSSTAZ  NodiDelcluster            บ บ
       ณณณณ  บ                    ฬอออออออออหอออออออออหอออออออออหอออออออออน บ
       ณณณณ  บ                    บ  IdNodo บ  IdNodo บ  IdNodo บ  IdNodo บ บ
       ณณณณ  บ                    ศอออออออออสอออออออออสอออออออออสอออออออออผ บ
       ณณณณ  บ                         ^                     ^              บ
       ณณณณ  บ                         ^                     ^              บ
       ณณณณ  บ                         ณ                     ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
       ณณณณ  บ                         ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟณ
       ณณณณ  บ                    ฺฤฤฤฤฤฤฤฤฤฤฤฤฤNumeroNodiฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ บ  ณณ
       ณณณณ  บ                    ณ                                       ณ บ  ณณ
       ณณณภฤฤฤฤฤ> ฺฤฤษออออออออออออออออออออออออออออออออออออออออออออออออออออป บ  ณณ
       ณณณ   บ    ณ  บ     CLUSTRENO TreniDelCluster                      บ บ  ณณ
       ภลลฤฤฤฤฤ>> ณ  ฬออออออออออออหอออออออออออออออออออออออออออออออออออออออน บ  ณณ
        ณณ   บ    N  บ IdTreno    บ           INFONODI Nodi               บ บ  ณณ
        ณณ   บ    u  บ SensoPerc. ฬอออออออออหอออออออออหอออออออออหอออออออออน บ  ณณ
        ณณ   บ    m  บ            บFerma,OraบFerma,OraบFerma,OraบFerma,Oraบ บ  ณณ
        ภลฤฤฤฤฤ>> e  ฬออออออออออออฮอออออออออสอออออออออสอออออออออสอออออออออน บ  ณณ
         ณ   บ    r  บ IdTreno    บ           INFONODI Nodi               บ บ  ณณ
         ณ   บ    o  บ SensoPerc. ฬอออออออออหอออออออออหอออออออออหอออออออออน บ  ณณ
         ณ   บ    T  บ            บFerma,OraบFerma,OraบFerma,OraบFerma,Oraบ บ  ณณ
         ภฤฤฤฤฤ>  r  ฬออออออออออออฮอออออออออสอออออออออสอออออออออสอออออออออน บ  ณณ
             บ    e  บ IdTreno    บ           INFONODI Nodi               บ บ  ณณ
             บ    n  บ SensoPerc. ฬอออออออออหอออออออออหอออออออออหอออออออออน บ  ณณ
             บ    i  บ            บFerma,OraบFerma,OraบFerma,OraบFerma,Oraบ บ  ณณ
             บ    ภฤ ศออออออออออออสอออออออออสอออออออออสอออออออออสอออออออออผ บ  ณณ
             ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ  ณณ
            ษอออออออออออออออออออออออป                                          ณณ
            บ Dati Locali di Combinaบ                                          ณณ
            ฬอออออออออออออออออออออออน                                          ณณ
            บ P_Origine      ฤฤฤฤฤฤฤฤฤฤฤ{Primo Cluster}ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤูณ
            บ P_Destinazione ฤฤฤฤฤฤฤฤฤฤฤ{Ultimo Cluster}ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู
            บ                       บ
            ศอออออออออออออออออออออออผ


*/    
#endif
