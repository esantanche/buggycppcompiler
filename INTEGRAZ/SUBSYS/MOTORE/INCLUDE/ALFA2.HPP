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
                 浜様様様様様様様様様様様様様様様様様様様様様様様様様様様様様�
               浜様様様様様様様様様様様様様様様様様様様様様様様様様様様様様� �
             浜様様様様様様様様様様様様様様様様様様様様様様様様様様様様様� � �
             �                        CLUSTER_PERCORSO                   � � �
             麺様様様様様様様様様様様様様様様様様様様様様様様様様様様様様� � �
             � IdCluster                                                 � � �
             � SensoNormaleDiPercorrenza                                 � � �
             �  敖陳陳陳陳陳陳陳陳NodiDiCambio.Dim()陳陳陳陳陳陳陳陳陳朕 � � �
             �  �                                                      � � � �
             �  浜様様様様様様様様様様様様様様様様様様様様様様様様様様融 � � �
             �  �                 ARRAY_NCAM NodiDiCambio              � � � �
             �  麺様様様様様様様様様様様様曜様様様様様様様様様様様様様郵 � � �
             �  � Qualita                  � Qualita                   � � � �
             �  � Peso                     � Peso                      � �
             �  � PCambio                  � PCambio 陳陳�{Cluster Corrente}陳朕
             �  � PCambioNext              � PCambioNext �{Cluster Successivo}審
             �  塞様様様様様様様様様様様様産浜様様様様様様様様様様様融 � �    崖
             �  査 BUFR TreniFiltrati     査� BUFR TreniFiltrati     � � � � 些�
             �  彩様様様様様様様様様様様様杭麺様様様様様様様様様様様郵 � � � 些�
       敖陳陳陳陳�Ptreno                  査�Ptreno                  � � � � 些�
       �     �  彩様様様様様様様様様様様様杭麺様様様様様様様様様様様郵 � � � 些�
       楽陳陳陳陳�Ptreno                  査�Ptreno                  � � � � 些�
       崖    �  哉様様様様様様様様様様様様失藩様様様様様様様様様様様夕 � � � 些�
       崖    �  �                          �                           � � � 些�
       崖敖陳陳陳�TrenoCorrente            � TrenoCorrente             � � 才竺�
       崖�   �  藩様様様様様様様様様様様様擁様様様様様様様様様様様様様夕 才�  崖
       崖�   � DatiCluster 陳�                                           �    崖
       崖�   藩様様様様様様様獲様様様様様様様様様様様様様様様様様様様様様�    崖
       崖�                   V                                                崖
       崖�   浜様様様様様様様様様様様様様様様様様様様様様様融                 崖
       崖V   �                  CLU_BUFR                    �                 崖
       崖V   麺様様様様様様様様様様様様様様様様様様様様様様郵                 崖
       崖鳴陳陳 Treno (Punta Al Treno Corrente)             �                 崖
       崖楽陳陳 FirstTreno                                  �                 崖
       崖崖敖陳 Nodi                                        �                 崖
       崖崖� �  Dat 陳�                                     �                 崖
       崖崖� 藩様様様由様様様様様様様様様様様様様様様様様様夕                 崖
       崖崖�          V                                                       崖
       崖崖� 浜様様様様様様様様様様様様様様様様様様様様様様様様様様様様様様融 崖
       崖崖� �                        CLUSTERS_STAZ_EXT                     � 崖
       崖崖� 麺様様様様様様様様様様様様様様様様様様様様様様様様様様様様様様郵 崖
       崖崖� � NumeroNodi                                                   � 崖
       崖崖� � NumeroTreni                                                  � 崖
       崖崖� �                                             敖陳陳陳陳陳陳陳陳潰�
       崖崖� �                                             �         敖陳陳陳陳�
       崖崖� �                    敖陳陳陳NumeroNodi陳陳陳蝶陳陳陳陳蝶陳陳� �
       崖崖� �                    �                        V         V    � �
       崖崖� �                    �                        V         V    � �
       崖崖青陳陳陳陳陳陳陳陳陳陳>浜様様様様様様様様様様様様様様様様様様様� �
       崖崖  �                    �   CLUSSTAZ  NodiDelcluster            � �
       崖崖  �                    麺様様様様僕様様様様僕様様様様僕様様様様� �
       崖崖  �                    �  IdNodo �  IdNodo �  IdNodo �  IdNodo � �
       崖崖  �                    藩様様様様瞥様様様様瞥様様様様瞥様様様様� �
       崖崖  �                         ^                     ^              �
       崖崖  �                         ^                     ^              �
       崖崖  �                         �                     青陳陳陳陳陳陳陳陳朕
       崖崖  �                         青陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳陳審
       崖崖  �                    敖陳陳陳陳陳陳NumeroNodi陳陳陳陳陳陳陳陳� �  崖
       崖崖  �                    �                                       � �  崖
       崖垣陳陳�> 敖追様様様様様様様様様様様様様様様様様様様様様様様様様様� �  崖
       崖�   �    �  �     CLUSTRENO TreniDelCluster                      � �  崖
       静田陳陳>> �  麺様様様様様曜様様様様様様様様様様様様様様様様様様様郵 �  崖
        崖   �    N  � IdTreno    �           INFONODI Nodi               � �  崖
        崖   �    u  � SensoPerc. 麺様様様様僕様様様様僕様様様様僕様様様様� �  崖
        崖   �    m  �            �Ferma,Ora�Ferma,Ora�Ferma,Ora�Ferma,Ora� �  崖
        静陳陳�>> e  麺様様様様様洋様様様様擁様様様様擁様様様様擁様様様様郵 �  崖
         �   �    r  � IdTreno    �           INFONODI Nodi               � �  崖
         �   �    o  � SensoPerc. 麺様様様様僕様様様様僕様様様様僕様様様様� �  崖
         �   �    T  �            �Ferma,Ora�Ferma,Ora�Ferma,Ora�Ferma,Ora� �  崖
         青陳陳>  r  麺様様様様様洋様様様様擁様様様様擁様様様様擁様様様様郵 �  崖
             �    e  � IdTreno    �           INFONODI Nodi               � �  崖
             �    n  � SensoPerc. 麺様様様様僕様様様様僕様様様様僕様様様様� �  崖
             �    i  �            �Ferma,Ora�Ferma,Ora�Ferma,Ora�Ferma,Ora� �  崖
             �    青 藩様様様様様擁様様様様擁様様様様擁様様様様擁様様様様夕 �  崖
             藩様様様様様様様様様様様様様様様様様様様様様様様様様様様様様様夕  崖
            浜様様様様様様様様様様様�                                          崖
            � Dati Locali di Combina�                                          崖
            麺様様様様様様様様様様様�                                          崖
            � P_Origine      陳陳陳陳陳�{Primo Cluster}陳陳陳陳陳陳陳陳陳陳陳陳抒
            � P_Destinazione 陳陳陳陳陳�{Ultimo Cluster}陳陳陳陳陳陳陳陳陳陳陳陳�
            �                       �
            藩様様様様様様様様様様様�


*/    
#endif
