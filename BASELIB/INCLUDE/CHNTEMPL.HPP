//----------------------------------------------------------------------------
// CHNTEMPL.HPP
//----------------------------------------------------------------------------
// Templates per la gestione di dati a lunghezza fissa
//----------------------------------------------------------------------------

#ifndef HO_CHNTEMPL_HPP
#define HO_CHNTEMPL_HPP

//----------------------------------------------------------------------------
// Per stampe su PIU' output
//----------------------------------------------------------------------------
// Bprintf3 : Scrive solo sul trace 
// Bprintf2 : Scrive sul trace e sui files di log addizionali
// Bprintf  : Scrive su STDOUT, sul trace e sui files di log addizionali
//----------------------------------------------------------------------------
int Bprintf(const char * Format, ...);      // StdOut,Trace, Files addizionali
int Bprintf2(const char * Format, ...);     // Trace, Files addizionali
int Bprintf3(const char * Format, ...);     // Solo Trace
void AddFileToBprintf(class FILE_RW * Out); // Aggiunge un file di LOG: Mettere NULL per resettare e chiudere i files 
#ifdef OKTRACE
#define BPRINTF3 if(_Abil)Bprintf3    // Testa il tracelevel
#else
#define BPRINTF3 if(FALSE)Bprintf3    // Eliminata poi dal compilatore
#endif

//----------------------------------------------------------------------------
// Prototipi funzioni
//----------------------------------------------------------------------------
// Questa macro converte in intero il dato contenuto in un campo a dimensione fissa NON NULL TERMINATED
#define It(_a)             StringToInt(_a,sizeof(_a))
// Questa macro converte in char * il dato contenuto in un campo a dimensione fissa NON NULL TERMINATED 
// NB: Ritorna un puntamento ad un' area temporanea, riutilizzata dopo 10 chiamate
// NB: Max 255 caratteri
#define St(_a)             StringFrFix(_a,sizeof(_a))
ULONG   StringToInt( const char *a, BYTE NumCaratteri);
char *  StringFrFix( const char *a, BYTE NumCaratteri); // Rientrante solo 10 volte
// Questa macro converte un intero in forma carattere NON NULL TERMINATED
void    AsgnFromInt(char * To, int From, int NumChar);

//----------------------------------------------------------------------------
// CHN: Stringa a dimensione fissa
//----------------------------------------------------------------------------
template <int NCHAR> 
class CHN {
   public :
   char Dati[NCHAR];
   
   // Questo operatore fa una copia temporanea NULL TERMINATED e ritorna un puntatore.
   operator const char *()   const { return St(Dati);};
   operator STRINGA () const { return St(Dati);};
   void Clear(){ memset(Dati,' ', sizeof(Dati));};
   BOOL Empty() const { 
      const char * c = Dati + NCHAR -1;
      while (c >= Dati){
         if(*c != ' ')return  FALSE;
         c--;
      } /* endwhile */
      return TRUE;
   }
   char & operator[](int Idx){ return Dati[Idx];};
   BOOL operator == (const  CHN<NCHAR> & b) const { return memcmp(Dati, b.Dati, NCHAR) == 0 ;};
   BOOL operator >  (const  CHN<NCHAR> & b) const { return memcmp(Dati, b.Dati, NCHAR) >  0 ;};
   BOOL operator >= (const  CHN<NCHAR> & b) const { return memcmp(Dati, b.Dati, NCHAR) >= 0 ;};
   BOOL operator <  (const  CHN<NCHAR> & b) const { return memcmp(Dati, b.Dati, NCHAR) <  0 ;};
   BOOL operator <= (const  CHN<NCHAR> & b) const { return memcmp(Dati, b.Dati, NCHAR) <= 0 ;};
   BOOL operator != (const  CHN<NCHAR> & b) const { return memcmp(Dati, b.Dati, NCHAR)      ;};
   void operator  = (const  CHN<NCHAR> & From) { memcpy(Dati,From.Dati,NCHAR);};
   void operator  = (const char * From) { memcpy(Dati,From,NCHAR);};
   void operator  = (const STRINGA & From) { From.ToFix(Dati,NCHAR);};
   int  Cfr(const  CHN<NCHAR> & b) const { return memcmp(Dati, b.Dati, NCHAR) ;}; 
};


//----------------------------------------------------------------------------
// NMN: Numero a dimensione fissa
//----------------------------------------------------------------------------
template <int NCHAR> 
class NMN {
   public :
   char Dati[NCHAR];
   
   operator int   ()   const { return It(Dati);};
   // Non e' prevista una conversione esplicita a STRINGA o char* per permettere 
   // operazioni dirette senza ambiguit…
   // Usare invece le seguenti funzioni
   char *  ToChar() const { return St(Dati);};
   STRINGA ToStringa() const { return St(Dati);};
   void Clear(){ memset(Dati,'0', sizeof(Dati));};
   BOOL Empty() const { 
      const char * c = Dati + NCHAR -1;
      while (c >= Dati){
         if(*c != ' ' && *c != '0')return  FALSE;
         c--;
      } /* endwhile */
      return TRUE;
   }
   BOOL operator == (const  NMN<NCHAR> & b) const { return It(Dati) == It(b.Dati) ;};
   BOOL operator >  (const  NMN<NCHAR> & b) const { return It(Dati) >  It(b.Dati) ;}; 
   BOOL operator >= (const  NMN<NCHAR> & b) const { return It(Dati) >= It(b.Dati) ;}; 
   BOOL operator <  (const  NMN<NCHAR> & b) const { return It(Dati) <  It(b.Dati) ;}; 
   BOOL operator <= (const  NMN<NCHAR> & b) const { return It(Dati) <= It(b.Dati) ;}; 
   BOOL operator != (const  NMN<NCHAR> & b) const { return It(Dati) != It(b.Dati) ;}; 
   BOOL operator == (int b) const { return It(Dati) == b ;};
   BOOL operator >  (int b) const { return It(Dati) >  b ;};
   BOOL operator >= (int b) const { return It(Dati) >= b ;};
   BOOL operator <  (int b) const { return It(Dati) <  b ;};
   BOOL operator <= (int b) const { return It(Dati) <= b ;};
   BOOL operator != (int b) const { return It(Dati) != b ;};
   void operator  = (int From) { AsgnFromInt(Dati , From, sizeof(Dati)) ;};
   int  Cfr(const  NMN<NCHAR> & b) const {return It(Dati) - It(b.Dati)  ;}; 
};
#endif
