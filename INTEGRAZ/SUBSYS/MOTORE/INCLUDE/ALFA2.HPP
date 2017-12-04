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
                 �����������������������������������������������������������ͻ
               �����������������������������������������������������������ͻ �
             �����������������������������������������������������������ͻ � �
             �                        CLUSTER_PERCORSO                   � � �
             �����������������������������������������������������������͹ � �
             � IdCluster                                                 � � �
             � SensoNormaleDiPercorrenza                                 � � �
             �  ������������������NodiDiCambio.Dim()������������������Ŀ � � �
             �  �                                                      � � � �
             �  ������������������������������������������������������ͻ � � �
             �  �                 ARRAY_NCAM NodiDiCambio              � � � �
             �  ������������������������������������������������������͹ � � �
             �  � Qualita                  � Qualita                   � � � �
             �  � Peso                     � Peso                      � �
             �  � PCambio                  � PCambio �����{Cluster Corrente}��Ŀ
             �  � PCambioNext              � PCambioNext �{Cluster Successivo}��
             �  �������������������������ͻ�������������������������ͻ � �    ��
             �  �� BUFR TreniFiltrati     ��� BUFR TreniFiltrati     � � � � ���
             �  �������������������������͹�������������������������͹ � � � ���
       �����������Ptreno                  ���Ptreno                  � � � � ���
       �     �  �������������������������͹�������������������������͹ � � � ���
       �����������Ptreno                  ���Ptreno                  � � � � ���
       ��    �  �������������������������ͼ�������������������������ͼ � � � ���
       ��    �  �                          �                           � � � ���
       �����������TrenoCorrente            � TrenoCorrente             � � �ͼ��
       ���   �  ������������������������������������������������������ͼ �ͼ  ��
       ���   � DatiCluster �Ŀ                                           �    ��
       ���   ���������������ͳ������������������������������������������ͼ    ��
       ���                   V                                                ��
       ���   ����������������������������������������������ͻ                 ��
       ��V   �                  CLU_BUFR                    �                 ��
       ��V   ����������������������������������������������͹                 ��
       �������� Treno (Punta Al Treno Corrente)             �                 ��
       �������� FirstTreno                                  �                 ��
       �������� Nodi                                        �                 ��
       ����� �  Dat �Ŀ                                     �                 ��
       ����� ��������ͳ������������������������������������ͼ                 ��
       �����          V                                                       ��
       ����� ��������������������������������������������������������������ͻ ��
       ����� �                        CLUSTERS_STAZ_EXT                     � ��
       ����� ��������������������������������������������������������������͹ ��
       ����� � NumeroNodi                                                   � ��
       ����� � NumeroTreni                                                  � ��
       ����� �                                             �������������������ٳ
       ����� �                                             �         �����������
       ����� �                    ��������NumeroNodi������ĳ��������ĳ���Ŀ �
       ����� �                    �                        V         V    � �
       ����� �                    �                        V         V    � �
       ��������������������������>���������������������������������������ͻ �
       ����  �                    �   CLUSSTAZ  NodiDelcluster            � �
       ����  �                    ���������������������������������������͹ �
       ����  �                    �  IdNodo �  IdNodo �  IdNodo �  IdNodo � �
       ����  �                    ���������������������������������������ͼ �
       ����  �                         ^                     ^              �
       ����  �                         ^                     ^              �
       ����  �                         �                     ������������������Ŀ
       ����  �                         ���������������������������������������Ŀ�
       ����  �                    ��������������NumeroNodi���������������Ŀ �  ��
       ����  �                    �                                       � �  ��
       ���������> �������������������������������������������������������ͻ �  ��
       ���   �    �  �     CLUSTRENO TreniDelCluster                      � �  ��
       ��������>> �  ����������������������������������������������������͹ �  ��
        ��   �    N  � IdTreno    �           INFONODI Nodi               � �  ��
        ��   �    u  � SensoPerc. ���������������������������������������͹ �  ��
        ��   �    m  �            �Ferma,Ora�Ferma,Ora�Ferma,Ora�Ferma,Ora� �  ��
        �������>> e  ����������������������������������������������������͹ �  ��
         �   �    r  � IdTreno    �           INFONODI Nodi               � �  ��
         �   �    o  � SensoPerc. ���������������������������������������͹ �  ��
         �   �    T  �            �Ferma,Ora�Ferma,Ora�Ferma,Ora�Ferma,Ora� �  ��
         ������>  r  ����������������������������������������������������͹ �  ��
             �    e  � IdTreno    �           INFONODI Nodi               � �  ��
             �    n  � SensoPerc. ���������������������������������������͹ �  ��
             �    i  �            �Ferma,Ora�Ferma,Ora�Ferma,Ora�Ferma,Ora� �  ��
             �    �� ����������������������������������������������������ͼ �  ��
             ��������������������������������������������������������������ͼ  ��
            �����������������������ͻ                                          ��
            � Dati Locali di Combina�                                          ��
            �����������������������͹                                          ��
            � P_Origine      �����������{Primo Cluster}������������������������ٳ
            � P_Destinazione �����������{Ultimo Cluster}�������������������������
            �                       �
            �����������������������ͼ


*/    
#endif
