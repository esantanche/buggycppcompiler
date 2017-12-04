//========================================================================
// Funzioni InLine di alfa
//========================================================================

#ifndef HO_ALFA_INL_HPP
#define HO_ALFA_INL_HPP

#undef abs
inline short int abs(short int a){return (a >= 0) ? a : -a;};

//-------------------------------------------------------------------------
// SuCluster() : Mi dice se sono su nodo o su Cluster
//-------------------------------------------------------------------------
inline BOOL ALFA_DIST::SuCluster()const{
   return (BOOL) (MezziCambi & 0x01);
};

//-------------------------------------------------------------------------
// Operator =
//-------------------------------------------------------------------------

inline void ALFA_DIST:: operator =(const ALFA_DIST & From){
   memmove (this,&From,sizeof(ALFA_DIST));
};

//-------------------------------------------------------------------------
// Operator +=
//-------------------------------------------------------------------------

inline void ALFA_DIST:: operator +=(const ALFA_DIST & Addendo){
// Regole di addizione:
// Addizione valida solo se entrambi le distanze sono su nodo o su cluster
//    La regola non viene rinforzata per ora
// Mezzicambi viene addizionato
// Distanza viene addizionata
// Riporto viene ignorato sui nodi, sottratto e ridotto a distanza reale sui cluster
   Distanza.In.Km     += Addendo.Distanza.In.Km    ;
   if(MezziCambi & 0x01){ // Sono su cluster
      Distanza.In.Km     += abs(Riporto.In.Km     - Addendo.Riporto.In.Km    );
      Riporto.In.Km    = 0; 
   }
   MezziCambi         += Addendo.MezziCambi;
   Pregio *= Addendo.Pregio;
};


inline void ALFA_DIST::AddShortDist(const SHORT_DIST & Addendo,NODO_ALFA & Nodo){
// Regole di addizione:
// Se SuCluster:
//    a Distanza viene aggiunto il delta tra Riporto ed Addendo
//    Riporto viene azzerato
// Altrimenti
//    Addendo viene copiato in riporto
// Mezzicambi viene incrementato
// Il tutto equivale a considerare una SHORT_DIST equivalente ad una ALFA_DIST
// con: Mezzicambi = 1 Distanza = 0 e Riporto = Addendo
   if(MezziCambi & 0x01){ // Sono su cluster
      Distanza.In.Km     += abs(Riporto.In.Km     - Addendo.In.Km    );
      Riporto.In.Km    = 0; 
      // if(Nodo.Cluster.NonPenalizzante)MezziCambi-= 2;
      if(Nodo.Cluster.Tipo == CLUSTER_MULTISTAZIONE)MezziCambi-= 2;
   } else {
      Riporto = Addendo; 
   }
   MezziCambi         ++;        // Incremento
   Pregio *= Nodo.Pregio;
};

//-------------------------------------------------------------------------
// funzioni di comparazione
//-------------------------------------------------------------------------
// Solo se il numero di cambi e' eguale vedo anche altri confronti.
inline int ALFA_DIST::Compara(const ALFA_DIST & b)const{
   int Delta = MezziCambi - b.MezziCambi ;
   if(Delta == 0){
      Delta = Distanza.In.Km - b.Distanza.In.Km;
   };
   if(Delta == 0){
      if(Pregio < b.Pregio ){  // Si noti che e' al contrario della distanza !
         Delta = 1;
      } else if(Pregio > b.Pregio ){
         Delta = -1;
      }
   }
   return Delta;
};
inline int Compara(const ALFA_DIST & a, const ALFA_DIST & b){
   return a.Compara(b);
};
inline int Compara2(const ALFA_DIST & a, const ALFA_DIST & b){
   int Delta;
   Delta = a.MezziCambi - b.MezziCambi ; // Predomina il numero cambi
   if(Delta == 0){         // Se hanno lo stesso Ok
      Delta = a.Distanza.In.Km - b.Distanza.In.Km; // Poi confronto i Km
      if(Delta == 0){
         if(a.Pregio < b.Pregio ){
            Delta = 1;
         } else if(a.Pregio > b.Pregio ){
            Delta = -1;
         }
      }
   };
   return Delta;
};
inline BOOL ALFA_DIST::FuoriTolleranza(){
// Vedo se ricade nell' intervallo di tolleranza
   return (
      (MezziCambi > Orario.LimiteTollerato.MezziCambi)           ||      // Troppi cambi
      (Distanza.In.Km > Orario.LimiteTollerato.Distanza.In.Km)           //   Troppi Km
      );
};

#endif
