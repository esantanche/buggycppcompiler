//========================================================================
// Disjoint SET
//========================================================================
/*
   Questa classetta realizza l' algoritmo di generazione dei Disjoint 
   set descritto da Tarjan (Data structures and network algorithms).

   In pratica gli elementi del set sono rappresentati da interi, da 0
   ad N con N prefissato e non troppo grande (NO set sparsi).

   Poichä ho usato una WORD si ha che N <= 0xffff

   La classe realizza i tre metodi fondamentali: 

   MakeSet(i) :  Assegna un elemento creando un SET vuoto

   Find(i)    :  Torna l' elemento canonico del SET di i (gli assegna un set se 
      non appartiene ad un set)

   Link(i,j)  :  Unisce due set disgiunti in un unico set .
      Funziona anche se i due set non sono disgiunti

   Inoltre:

   ReNumber   : Numera in progressione i disjoint set trovati (parte da 1)
                Ritorna il numero massimo di indice utilizzato (0 se il set e' vuoto)

*/
//========================================================================

struct DISJOINT_SET {

   struct ELEMENT {
       ELEMENT * Parent;  // Parent ( o elemento canonico ...) 
       WORD      Rank  ;  // Rank
       WORD      Index ;  // Indice (valorizzato da ReNumber() )
   };

   DISJOINT_SET(WORD NumeroElementi){
      Array = new ELEMENT[NumeroElementi+1];
      memset(Array,0, sizeof(ELEMENT) * (NumeroElementi+1));
      MaxEl = NumeroElementi;
   };

   ~DISJOINT_SET(){delete [] Array;};

   ELEMENT * MakeSet( WORD i){
      assert( i <= MaxEl);
      ELEMENT * El = Array + i;
      assert(El->Parent == NULL);
      El->Parent = El; 
      El->Rank   = 0;
      return El;
   };

   ELEMENT * Find(WORD i){
      assert( i <= MaxEl);
      ELEMENT * El = Array + i;
      if(El->Parent == NULL){ // MakeSet
         El->Parent = El; 
         El->Rank   = 0;
         return El;
      }
      if (El->Parent != El)El->Parent = Find2(El->Parent);
      return El->Parent;
   };

   ELEMENT * Find2(ELEMENT * El){
      if (El->Parent != El)El->Parent = Find2(El->Parent);
      return El->Parent;
   };

   ELEMENT * Link(int i, int j){
      ELEMENT * El1 = Find(i);
      ELEMENT * El2 = Find(j);
      if(El1 == El2)return El1;
      if(El1->Rank < El2->Rank){
         El1->Parent = El2;
         return El2;
      } else if(El1->Rank > El2->Rank){
         El2->Parent = El1;
         return El1;
      } else {
         El1->Parent = El2;
         El2->Rank ++;
         return El2;
      } /* endif */
   }

   int  ReNumber(){  // Chiamare una sola volta
      int Next = 1;
      ELEMENT * El = Array;
      for (int i = 0; i <= MaxEl ; i++ ) {
         if(El->Parent != NULL){ // Assegnato
            ELEMENT * El2 = Find(i);
            if(El2->Index == 0){ 
               El2->Index = Next;
               Next ++;
            }
         }
         El ++;
      } /* endfor */
      return Next - 1;
   };

   protected :

   ELEMENT * Array;
   WORD      MaxEl;

};
