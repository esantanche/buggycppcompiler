//----------------------------------------------------------------------------
// FILE STRINGA.C
//----------------------------------------------------------------------------
// Definizioni di funzioni per la classe STRINGA
//----------------------------------------------------------------------------
//EMS001 devo togliere questo define (NO_STRINGA_INLINE) che impedisce la definizione
// del costruttore di base della classe stringa (vedi stringa.h in fondo)
// #define NO_STRINGA_INLINE

//----------------------------------------------------------------------------
// Gestione del trace
//----------------------------------------------------------------------------
#define LIVELLO_DI_TRACE_DEL_PROGRAMMA 6

// EMS002
typedef unsigned long BOOL;

#include "ELENCO_S.H"
#include <malloc.h>
#include <myalloc.h>

//----------------------------------------------------------------------------
// Ottimizzazione : massimo speed
//----------------------------------------------------------------------------
// Sembra funzionare, ma per il momento non la attivo
// #pragma option -O2

//----------------------------------------------------------------------------
// Classe Stringa
//----------------------------------------------------------------------------
// Questo non controlla che a non sia null ed accetta la lunghezza
// In compenso il char* puo' non essere null terminate
inline STRINGA::STRINGA(const char *a,USHORT Leng){ // costruttore veloce per le funzioni interne
{
      Len = Leng;
      if(Len >= STRINGA_AUTO_ALLOC_DIM){
         Dati = (char*)malloc(Len + 1);
         MEMIGNORE(Dati);
      } else {
         Dati = DatiDiretti;
      };
      if( Len ) memcpy(Dati, a,Len);
      Dati[Len]='\0';
   }
};
// Idem ed inoltre concatena due stringhe
inline STRINGA::STRINGA(const char *a,USHORT Lenga,const char* b,USHORT Lengb){ // costruttore veloce per le funzioni interne
      Len = Lenga+Lengb;
      if(Len >= STRINGA_AUTO_ALLOC_DIM){
         Dati = (char*)malloc(Len + 1);
         MEMIGNORE(Dati);
      } else {
         Dati = DatiDiretti;
      };
      if(Lenga)memcpy(Dati, a,Lenga);
      if(Lengb)memcpy(Dati+Lenga, b,Lengb);
      Dati[Len]='\0';
};

STRINGA& STRINGA::Pad(  USHORT Lun, char ch)
{
   CHAR* Work = new CHAR[ Lun+1 ];
   memset( Work, 0x00, Lun+1);
   ToFix( Work, Lun, ch);
   THIS = Work;
   delete Work;
   return THIS;
}

STRINGA& STRINGA::RPad( USHORT Lun, char ch)
{
   CHAR* Work = new CHAR[ Lun+1 ];
   memset( Work, 0x00, Lun+1);
   Rovescia();
   ToFix( Work, Lun, ch );
   THIS = Work;
   Rovescia();
   delete Work;
   return THIS;
}
STRINGA& STRINGA::Rovescia()
{
   if( this == NULL ) return NUSTR;
   STRINGA Work;
   if( Len > 0 ) {
      FORALL( THIS, i ) Work += THIS[i];
      THIS = Work;
   }
   return THIS;
}
//----------------------------------------------------------------------------
// ToFix : Scrive su di un' area a lunghezza fissa
// Viene usata principalmente per interfacciare il COBOL
//----------------------------------------------------------------------------

// Nuova Versione (by Nosella)
void STRINGA::ToFix(char * Dest, USHORT Leng, char Riemp)const
{
   if(Dest == NULL || Leng == 0) return;

   if(this == NULL || Len == 0)
   {
      memset(Dest,Riemp,Leng);
      return;
   }

   if(Len >= Leng)
   {
      if( Leng ) memcpy(Dest, Dati, Leng); // Copia su Dest
      return;
   }

   if( Len ) memcpy(Dest,Dati,Len);
   if( Leng > Len ) memset(Dest+Len,Riemp,Leng-Len);     // La riempio di blanks
   return;
}

//----------------------------------------------------------------------------
// Costruttore di copia : Crea una nuova stringa uguale a quella indicata
//----------------------------------------------------------------------------

STRINGA::STRINGA(const STRINGA & b)
{
   // Se mi viene passata una stringa vuota o nulla creo una stringa vuota.
   if(& b == NULL  )
   {
      Len = 0;
      Dati = DatiDiretti;
      *Dati = '\0';
   }
   else
   {
      Len = b.Len;
      if(Len >= STRINGA_AUTO_ALLOC_DIM){
         Dati = (char*)malloc(Len + 1);
         MEMIGNORE(Dati);
      } else {
         Dati = DatiDiretti;
      };
      if( Len ) memcpy(Dati, b,Len);
      Dati[Len]='\0';
   }
}

//----------------------------------------------------------------------------
// Costruttore a partire da un array di caratteri (char *)
// (Conversione da char * a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const char * a)
{
   if(a == NULL )
   {
      Len = 0;
      Dati = DatiDiretti;
      *Dati = '\0';
   }
   else
   {
      Len = strlen(a);
      if(Len >= STRINGA_AUTO_ALLOC_DIM){
         Dati = (char*)malloc(Len + 1);
         MEMIGNORE(Dati);
      } else {
         Dati = DatiDiretti;
      };
      strcpy(Dati, a);
   }
}

//----------------------------------------------------------------------------
// Costruttore a partire da un carattere.
// (Conversione da char a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const char a)
{
   Dati = DatiDiretti;
   if( a == '\0' ) {
     Len = 0;
     *Dati = '\0';
   }
   else  {
     Len = 1;
     Dati[0] = a;
     Dati[1] = '\0';
   }

}

//----------------------------------------------------------------------------
// Costruttore a partire da un int.
// (Conversione da int a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const int a)
{
   itoa(a,DatiDiretti,10);
   Len = strlen(DatiDiretti);
   Dati = DatiDiretti;
}

//----------------------------------------------------------------------------
// Costruttore a partire da uno short.
// (Conversione da short a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const short a)
{
   int A = a;
   itoa(A,DatiDiretti,10);
   Len = strlen(DatiDiretti);
   Dati = DatiDiretti;
}

//----------------------------------------------------------------------------
// Costruttore a partire da un long.
// (Conversione da long a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const long a)
{
   ltoa(a,DatiDiretti,10);
   Len = strlen(DatiDiretti);
   Dati = DatiDiretti;
}

//----------------------------------------------------------------------------
// Costruttore a partire da un double.
// (Conversione da double a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const double a)
{
   char appo[129];

   memset( appo        , 0x00 , sizeof( appo )        );
   memset( DatiDiretti , 0x00 , sizeof( DatiDiretti ) );

   sprintf( appo , "%lf" , a);

   TRACESTRING( "appo = "+STRINGA( appo ) );

   for( SHORT i = strlen( appo ) -1; i >= 0 ; i-- ) {
        if( appo[i] == '0' )
            appo[i] = 0x00;
        else break;
   } /* endfor */

   if( strlen( appo ) < STRINGA_AUTO_ALLOC_DIM - 1 ) {
       Dati = DatiDiretti;
   } else {
       Dati = (char*)malloc( strlen( appo ) + 1 );
       memset( Dati , 0x00 , strlen( appo ) + 1 );
   }
   memcpy( Dati , appo , strlen( appo ) );
   Len = strlen(Dati);

}

//----------------------------------------------------------------------------
// Costruttore a partire da un unsigned int.
// (Conversione da unsigned int a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const unsigned int a)
{
   ultoa(a,DatiDiretti,10);
   Len = strlen(DatiDiretti);
   Dati = DatiDiretti;
}

//----------------------------------------------------------------------------
// Costruttore a partire da un unsigned short.
// (Conversione da unsigned short a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const unsigned short a)
{
   ULONG A = a;
   ultoa(A,DatiDiretti,10);
   Len = strlen(DatiDiretti);
   Dati = DatiDiretti;
}

//----------------------------------------------------------------------------
// Costruttore a partire da un unsigned long.
// (Conversione da unsigned long a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(const unsigned long a)
{
   ultoa(a,DatiDiretti,10);
   Len = strlen(DatiDiretti);
   Dati = DatiDiretti;
}

// Costruttore a Partire da void *
//----------------------------------------------------------------------------
// Costruttore a partire da un void *
// (Conversione da void * a STRINGA)
//----------------------------------------------------------------------------
STRINGA::STRINGA(void * Ptr )
{
   sprintf(DatiDiretti, "%p", Ptr);
   Len = strlen(DatiDiretti);
   Dati = DatiDiretti;
}
//----------------------------------------------------------------------------
// Operatore const char *
// (Conversione da da STRINGA a const char *)
// (essendo costante posso accedere direttamente)
//----------------------------------------------------------------------------
// MONTAGNA: vietato toccare questo codice !!!!
STRINGA::operator const char * ()const
{
   static char * NoChar = "";
// Montagna: Fix per evitare ABEND
// Dalla versione 1.2 in poi se la stringa ha l•unghezza 0 deve tornare un puntatore valido.
   if(this == NULL )return NoChar;
   return Dati;
}

//----------------------------------------------------------------------------
// Clear : Elimina il contenuto della stringa (ma non distrugge la stringa)
//----------------------------------------------------------------------------
void STRINGA::Clear()
{
   if(this != NULL)
   {
      if(Len >= STRINGA_AUTO_ALLOC_DIM) free(Dati);
      Len = 0;
      Dati = DatiDiretti;
      *Dati = '\0';
   }
}

//----------------------------------------------------------------------------
// Operatore = Assegna a questa stringa, la stringa indicata
// (Copia di stringa)
//----------------------------------------------------------------------------
STRINGA & STRINGA::operator=(const STRINGA & b)
{
   if(& b == NULL ){
      if(Len >=  STRINGA_AUTO_ALLOC_DIM) free(Dati);
      Dati = DatiDiretti;
      Len = 0;
      *Dati = '\0';
   } else {
      if(b.Len >= STRINGA_AUTO_ALLOC_DIM){
         if(b.Len > Len){
            if(Len >=  STRINGA_AUTO_ALLOC_DIM) free(Dati);
            Dati = (char*)malloc(b.Len + 1);
            MEMIGNORE(Dati);
         };
      } else {
         if(Len >=  STRINGA_AUTO_ALLOC_DIM) free(Dati);
         Dati = DatiDiretti;
      }
      Len = b.Len;
      strcpy(Dati, b.Dati);
   }
   return * this;
}

//----------------------------------------------------------------------------
// Operatore = Assegna a questa stringa, un char *
// (Copia di stringa)
//----------------------------------------------------------------------------
STRINGA &  STRINGA::operator = (const char * b)
{
   USHORT BLen;
   if( b == NULL || (BLen = strlen(b)) == 0){
       if(Len >=  STRINGA_AUTO_ALLOC_DIM) free(Dati);
       Dati = DatiDiretti;
       Len = 0;
      *Dati = '\0';
   } else {
      if(BLen >= STRINGA_AUTO_ALLOC_DIM){
         if(BLen > Len){
            if(Len >=  STRINGA_AUTO_ALLOC_DIM) free(Dati);
            Dati = (char*)malloc(BLen + 1);
            MEMIGNORE(Dati);
         };
      } else {
         if(Len >=  STRINGA_AUTO_ALLOC_DIM) free(Dati);
         Dati = DatiDiretti;
      }
      Len = BLen;
      strcpy(Dati, b);
   }
   return * this;
}

//----------------------------------------------------------------------------
// Operatore + : Concatena due stringhe senza modificare le stringhe originali
//----------------------------------------------------------------------------
STRINGA operator+(const STRINGA & a, const STRINGA & b)
{
   if(&a == NULL){
      return STRINGA(b);
   } else if(&b == NULL){
      return STRINGA(a);
   } else {
      return STRINGA(a.Dati,a.Len,b.Dati,b.Len);
   }
}

//----------------------------------------------------------------------------
// Operatore + : Concatena due stringhe senza modificare le stringhe originali
//----------------------------------------------------------------------------
STRINGA operator+(const STRINGA & a, const char * b)
{
   if(&a == NULL){
      return STRINGA(b);
   } else if(b == NULL){
      return STRINGA(a);
   } else {
      return STRINGA(a.Dati,a.Len,b,(USHORT)strlen(b));
   }
}


STRINGA operator+(const char * b,const STRINGA & a)
{
   if(&a == NULL){
      return STRINGA(b);
   } else if(b == NULL){
      return STRINGA(a);
   } else {
      return STRINGA(b,(USHORT)strlen(b),a.Dati,a.Len);
   }
}


//----------------------------------------------------------------------------
// Operatore += : Aggiunge un'altra stringa in fondo a questa stringa
//----------------------------------------------------------------------------
STRINGA & STRINGA::operator+= (const STRINGA & b)
{
   if(& b != NULL && b.Len > 0)
   {
      if(Len + b.Len < STRINGA_AUTO_ALLOC_DIM ){
         strcpy(Dati+Len,b.Dati);
      } else {
         char * Tmp = (char*)malloc(Len + b.Len + 1);
         if(Len > 0) strcpy(Tmp, Dati);
         strcpy(Tmp+Len, b.Dati);
         if(Len >= STRINGA_AUTO_ALLOC_DIM)free(Dati);
         Dati = Tmp;
         MEMIGNORE(Dati);
      };
      Len += b.Len;
   }
   return * this;
}


//----------------------------------------------------------------------------
// Operatore + : Concatena un carattere a una stringa senza modificare
// gli addendi
//----------------------------------------------------------------------------
STRINGA operator+ (const STRINGA & a, const char b)
{
   char Strg[2]; Strg[0]= b;Strg[1]='\0';
   USHORT aLung = (& a == NULL) ? 0 : a.Len;
   return STRINGA(a.Dati,aLung,Strg,(USHORT)1);
}

//----------------------------------------------------------------------------
// Operatore += : Aggiunge un carattere nuovo in fondo a questa stringa
//----------------------------------------------------------------------------
STRINGA & STRINGA::operator+= (const char b)
{
   if((Len + 1) >= STRINGA_AUTO_ALLOC_DIM ){
      char * Tmp = (char*)malloc(Len + 2);
      strcpy(Tmp, Dati);
      if(Len >= STRINGA_AUTO_ALLOC_DIM ) free(Dati);
      Dati = Tmp;
      MEMIGNORE(Dati);
   };
   Dati[Len++] = b;
   Dati[Len] = '\0';
   return * this;
}


//----------------------------------------------------------------------------
// Operatore < : Ritorna TRUE se a precede b.
//----------------------------------------------------------------------------
BOOL operator < (const STRINGA & a,const STRINGA & b)
{
   if(& b == NULL) return FALSE;
   if(& a == NULL ) return TRUE ;
   return (strcmp(a.Dati, b.Dati) < 0 );
}

//----------------------------------------------------------------------------
// Operatore > : Ritorna TRUE se a segue b.
//----------------------------------------------------------------------------
BOOL operator > (const STRINGA & a, const STRINGA & b)
{
   if(& a == NULL ) return FALSE;
   if(& b == NULL ) return TRUE ;
   return (strcmp(a.Dati, b.Dati) > 0);
}

//----------------------------------------------------------------------------
// Operatore <= : Ritorna TRUE se a precede b o le due stringhe sono uguali.
//----------------------------------------------------------------------------
BOOL operator<=(const STRINGA& a,const STRINGA& b)
{
   if(& a == NULL ) return TRUE ;
   if(& b == NULL ) return FALSE;
   return (strcmp(a.Dati, b.Dati) <= 0);
}

//----------------------------------------------------------------------------
// Operatore <= : Ritorna TRUE se a segue b o le due stringhe sono uguali.
//----------------------------------------------------------------------------
BOOL operator >= (const STRINGA & a, const STRINGA & b)
{
   if(& b == NULL ) return TRUE ;
   if(& a == NULL ) return FALSE;
   return (strcmp(a.Dati, b.Dati) >= 0);
}

//----------------------------------------------------------------------------
// Operatore == : Ritorna TRUE se le stringhe a e b sono uguali.
//----------------------------------------------------------------------------
BOOL operator == (const STRINGA & a, const STRINGA & b)
{
   if(& b == NULL )
      return (& a == NULL || a.Len == 0);
   if(& a == NULL )
      return ( b.Len == 0);
   if(a.Len != b.Len)
      return FALSE;
   return (!strcmp(a.Dati,b.Dati));
}

//----------------------------------------------------------------------------
// Operatore == : Ritorna TRUE se le stringhe a e b sono uguali.
// (a Š un char *)
//----------------------------------------------------------------------------
BOOL operator == (const char * a, const STRINGA & b)
{
   if(& b == NULL )
      return (a == NULL || * a == 0);
   if(a == NULL || * a == 0)
      return ( b.Len == 0);
   return (!strcmp(a, b.Dati));
}

//----------------------------------------------------------------------------
// Operatore == : Ritorna TRUE se le stringhe a e b sono uguali.
// (b Š un char *)
//----------------------------------------------------------------------------
BOOL operator == (const STRINGA & a, const char * b)
{
   if(& a == NULL )
      return (b == NULL || * b == 0);
   if(b == NULL || * b == 0)
      return ( a.Len == 0);
   return (!strcmp(a.Dati, b));
}

//----------------------------------------------------------------------------
// Operatore != : Ritorna TRUE se le stringhe a e b sono diverse.
//----------------------------------------------------------------------------
BOOL operator!=(const STRINGA & a, const STRINGA & b)
{
   if(& b == NULL )
      return (& a != NULL && a.Len != 0);
   if(& a == NULL )
      return (b.Len != 0);
   if(a.Len != b.Len)
      return TRUE;
   return BOOL(strcmp(a.Dati, b.Dati));
}

//----------------------------------------------------------------------------
// STRINGACmp : confronta due stringhe in operazioni di sort o simili.
// Ritorna un intero >, =, o < 0 a seconda che A sia >, =, o < B
//----------------------------------------------------------------------------
int cdecl STRINGACmp(STRINGA & A , STRINGA & B)
{
   if(& B == NULL || B.Len == 0 )
   {
      if(& A == NULL || A.Len == 0 )
      return 0;                    // Due stringhe vuote sono eguali
      return -1;                   // B e' vuota A non lo Š quindi A > B
   }
   if(& A == NULL || A.Len == 0 )
      return -1;                   // A e' vuota B non lo Š quindi A < B
   return strcmp(A.Dati, B.Dati);
}

//----------------------------------------------------------------------------
// PSTRINGACmp : confronta due tabelle di puntatori a stringhe.
// Ritorna un intero >, =, o < 0 a seconda che A sia >, =, o < B
//----------------------------------------------------------------------------
int PSTRINGACmp(const void * A , const void * B)
{

   STRINGA Uno = (CPSZ)**((STRINGA **) A);
   STRINGA Due = (CPSZ)**((STRINGA **) B);
   return STRINGACmp(Uno,Due);
}

//----------------------------------------------------------------------------
// Operatore [] : Ritorna l'i-esimo elemento della stringa
//----------------------------------------------------------------------------

char STRINGA::operator[](unsigned int Pos)const
{
   if(this == NULL)
      return '\0';
   if(Pos >= Len)
      return '\0';
   return this->Dati[Pos];
}

char & STRINGA::operator[](unsigned int Pos)
{
   static char vuoto;
   vuoto = 0x00;
   if(this == NULL)
      return vuoto;
   if(Pos >= Len)
      return vuoto;
   return this->Dati[Pos];
}

//----------------------------------------------------------------------------
// Operatore () : Ritorna la sottostringa compresa tra i limiti indicati
//----------------------------------------------------------------------------

STRINGA STRINGA::operator()(int Primo, int Ultimo)const
{
   if(this != NULL && Len >0 && Ultimo >= 0 && Primo < Len && Ultimo >= Primo)
   {
      if(Ultimo >= Len)Ultimo = Len-1;
      if(Primo < 0)Primo = 0;
      return STRINGA(Dati + Primo,(USHORT) (Ultimo - Primo + 1));
   } else {
      return STRINGA();
   }
}


//----------------------------------------------------------------------------
// Tokens : Separazione di una stringa in piu' stringhe (alla strtok)
// Se fornito Seps ritorna i separatori che  delimitavano le sottostringhe
//----------------------------------------------------------------------------
ELENCO_S STRINGA::Tokens(const STRINGA & Delimitatori, class ELENCO_S * Seps)const
{
   ELENCO_S Out;
   int Pippo[1]; // Questo per forzare init Stack altrimenti alloca non funziona con il borland (vedere manuale)
   if(this != NULL && Len >  0)
   {
      if(Seps) Seps->Clear();
      char * Copia = (char*)alloca(Len + 1);
      char * Delim = (char*)alloca(Delimitatori.Dim() + 1);
      if( Copia == NULL || Delim == NULL ) {
         ERRSTRING( "Stack Full" );
         return Out;
      }
      strcpy(Copia, (CPSZ)(* this));
      strcpy(Delim, (CPSZ)Delimitatori);
      char * Tok = Copia; // Tok Punta alla token in esame

      for(char * c = Copia; * c; c++)
      {
         for(char * d = Delim; * d; d++)
         {
            if(* c == * d)
            {
               * c = '\0';
               Out += STRINGA(Tok);
               if(Seps)
                  * Seps += STRINGA(* d);
               Tok = c + 1;
            }
         }
      }
// L' ultima token non e' stata scaricata, ma se e' vuota NON la scarico
      if(* Tok)
      {
         Out += STRINGA(Tok);
         if(Seps)
            * Seps += ""; // Separatore vuoto
      }
      // Queste istruzioni solo se non si usa la alloca()
      // free( Copia);
      // free( Delim);
   }
   return Out;
}

//----------------------------------------------------------------------------
// Ricerca di una sottostringa (-1 se non trovata, o primo carattere)
//----------------------------------------------------------------------------
int STRINGA::Pos(const STRINGA & SubStr) const
{
   if(this == NULL || Len == 0)
      return -1;
   const char * St = (CPSZ)(* this);
   const char * Sb = (CPSZ)SubStr;
   // EMS003 12.5.97 inserito il const
   const char * Fnd = strstr(St,Sb);
   if(Fnd == NULL)
      return -1;
   return ((const char *)Fnd - St);
}

//----------------------------------------------------------------------------
// Strip : Elimina spazi e Tabs iniziali e finali, converte sequenze di spazi
// e Tabs in un singolo spazio
//----------------------------------------------------------------------------
STRINGA & STRINGA::Strip()
{
   if(this == NULL || Len == 0 )
      return *this;

   char * Tmp = (char*)malloc(Len + 1);
   char * To = Tmp;
   BOOL Inibisco = TRUE;
   for(char * From = Dati; * From; From++)
   {
      if(* From == ' ' || * From == '\t')
      {
         if(!Inibisco)
            * To++ = ' ';
         Inibisco = TRUE;
      }
      else
      {
         * To++ = * From;
         Inibisco = FALSE;
      }
   }
   if(Inibisco && To > Tmp) To--; // Spazio Finale
   * To = '\0';
   * this = Tmp;
   // Questa istruzione solo se non si usa la alloca()
   free(Tmp);
   return * this;
}

//----------------------------------------------------------------------------
// ToLong : Ritorna il contenuto della stringa convertito in long o 0L se
// la conversione fallisce
//----------------------------------------------------------------------------
double STRINGA::ToDouble(BOOL * Rounded) const
{
   if (Dim() == 0) return 0L;
   // EMS004 VA il secondo parametro di strtod passa da (char *)NULL a  (char **)NULL
   return  strtod((CPSZ)(* this), (char **)NULL);
}

//----------------------------------------------------------------------------
// ToInt : Ritorna il contenuto della stringa convertito in int o 0 se
// la conversione fallisce
//----------------------------------------------------------------------------
float STRINGA::ToFloat(BOOL * Rounded) const
{
   if (Dim() == 0) return 0;
   return atof((CPSZ)(* this));
}

//----------------------------------------------------------------------------
// ToLong : Ritorna il contenuto della stringa convertito in long o 0L se
// la conversione fallisce
//----------------------------------------------------------------------------
LONG STRINGA::ToLong(BOOL * Rounded) const
{
   if (Dim() == 0) return 0L;
   return  atol((CPSZ)(* this));
}

//----------------------------------------------------------------------------
// ToInt : Ritorna il contenuto della stringa convertito in int o 0 se
// la conversione fallisce
//----------------------------------------------------------------------------
int STRINGA::ToInt(BOOL * Rounded) const
{
   if (Dim() == 0) return 0;
   return atoi((CPSZ)(* this));
}

//----------------------------------------------------------------------------
// ToPtr : Ritorna il contenuto della stringa Hex convertito in long
//----------------------------------------------------------------------------
void* STRINGA::ToPtr() const
{
   if(Dim() == 0)
      return 0L;

   return (void*)strtoul(Dati, NULL, 16);   // conversione stringa valore pointer
}

//----------------------------------------------------------------------------
// UpCase : Converte in maiuscole tutte le lettere della stringa
//----------------------------------------------------------------------------
STRINGA & STRINGA::UpCase()
{
   if(Dim() != 0)
      strupr(Dati);
   return  * this;
}

//----------------------------------------------------------------------------
// UpCase : Converte in minuscole tutte le lettere della stringa
//----------------------------------------------------------------------------
STRINGA & STRINGA::LoCase()
{
   if(Dim() != 0)
      strlwr(Dati);
   return  * this;
}

//----------------------------------------------------------------------------
// PrtStringa : Formatta le stringhe per la stampa
//----------------------------------------------------------------------------
STRINGA STRINGA::Prt()const
{
   char  p[500]; // Massimo 500 caratteri
   char * t =  p;

   if(this == NULL || Len == 0 || Dati == NULL)
   {
      strcpy(t, "\"\"");
   }
   else
   {
      * t++= '\"';
      for(unsigned char * f = (UCHAR *) Dati; * f; f++)
      {
         switch( *f)
         {
         case '\\' :
         case '\"' :
            * t++ ='\\';
            * t++ =*f;
            break;
         case '\t' :
            * t++ = '\\';
            * t++ = '\t';
            break;
         case '\n' :
            * t++ = '\\';
            * t++ = '\n';
            break;
         default :
            if(* f < 32)
            {
               int nc = sprintf(t, "\\0x%2.2x", (USHORT) * f);
               t += nc;
            }
            else
            {
               * t++ = * f;
            }
            break;
         }
      }
      * t++ = '\"';
      * t = '\0';
   }
   STRINGA Temp=(STRINGA)p;
   return Temp;
}

// [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[
// Costruttore e distruttore (analoghi dei metodi inline) stanno su stringa2.c
// [[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[

