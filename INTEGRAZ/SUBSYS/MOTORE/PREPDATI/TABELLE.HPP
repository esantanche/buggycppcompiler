/*===================================================================
Nome modulo ......... : TABELLE.HPP
===================================================================*/
// Queste sono le strutture di output verso HOST

#ifndef TABELLE_HPP
#define TABELLE_HPP

struct DMEZVGG
{
    char        NUMMEZVGG[5];   // M0_TRENV (Elab)
    char        KEYMEZVGG[10];
    char        CODRET[2];      // Fisso 83
    char        CODGSRSER[3];   // Fisso 083
    char        CODTIPMEZVGG;   // Decod
    char        DATINIVAL[10];  // Decod
    char        DATFINVAL[10];  // Decod
    char        DEN[40];        // M0_TRENV (Elab) (solo se ha nome)
    char        FUM;            // S_TRENO (MM_INFO)
    char        IDE;            // S_TRENO (MM_INFO)
    char        CODDSZ[8];      // Decod
    char        CR_LF[2];
};

class  FILE_DMEZVGG : public FILE_FIX
{
   public:

    FILE_DMEZVGG(const STRINGA& NomeFile,ULONG BufSize=64000):
        FILE_FIX(NomeFile,sizeof(DMEZVGG),BufSize){};
    DMEZVGG &  operator [](ULONG Indice){ Posiziona(Indice); return *(DMEZVGG*) RecordC; };
    DMEZVGG &  RecordCorrente(){ return *(DMEZVGG*) RecordC; };
};

struct DMEZVIR
{
    char NUMVIR[5];
    char PRGVIR[2];
    char DATINIVAL[10];
    char DATFINVAL[10];
    char CODTIPMEZVGG;
    char CODSTAINI[5];
    char CODSTAFIN[5];
    char CODCLS[2];
    char Q_M_1;
    char CLA;
    char Q_M_2;
    char PRAOCC[2];
    char INDTIPVGG;
    char INDGESIVX;
    char CNG;
    char CODDSZ[8];
    char CR_LF[2];
};

class  FILE_DMEZVIR : public FILE_FIX
{
   public:

    FILE_DMEZVIR(const STRINGA& NomeFile,ULONG BufSize=64000):
        FILE_FIX(NomeFile,sizeof(DMEZVIR),BufSize){};
    DMEZVIR &  operator [](ULONG Indice){ Posiziona(Indice); return *(DMEZVIR*) RecordC; };
    DMEZVIR &  RecordCorrente(){ return *(DMEZVIR*) RecordC; };
};

struct DFERMEZVIR
{
    char NUMVIR[5];
    char PRGVIR[2];
    char PRGSTA[ 3 ];
    char DATINIVAL[10];
    char CODSTA[5];
    char CODRETSTA[2];
    char CODTIPFER;
    char DATFINVAL[10];
    char ORAARR[8];
    char ORAPAR[8];
    char INDECZ;
    char SCS;
    char CODDSZ[8];
    char PROGKM[4];
    char CR_LF[2];
};

class  FILE_DFERMEZVIR : public FILE_FIX
{
   public:

    FILE_DFERMEZVIR(const STRINGA& NomeFile,ULONG BufSize=64000):
        FILE_FIX(NomeFile,sizeof(DFERMEZVIR),BufSize){};
    DFERMEZVIR &  operator [](ULONG Indice){ Posiziona(Indice); return *(DFERMEZVIR*) RecordC; };
    DFERMEZVIR &  RecordCorrente(){ return *(DFERMEZVIR*) RecordC; };
};

struct DCOSMEZVIR
{
    char NUMVIR    [ 5 ];
    char PRGVIR    [ 2 ];
    char PRGMEZVGG [ 2 ];
    char NUMMEZVGG [ 10 ];
    char CODRET    [ 2 ];
    char CODGSRSER [ 3 ];
    char CODTIPMEZVGG;
    char PRGINI    [ 3 ];
    char DATINIVAL [ 10 ];
    char DATFINVAL [ 10 ];
    char PRGFIN    [ 3 ];
    char INDPRCITA;         // Mezzo che nella costituzione di un virtuale, da tutto se stesso.
                            // STEP.1 : Se un viaggiante appare in un solo virtuale della famiglia, ne Š completamente contenuto;
                            // STEP.2 : Se no, controllare in quale virtuale della famiglia il viaggiante ha la maggiore
                            //          differenza tra PRGFIN e PRGINI: quello sar… completamente contenuto.
    char CODDSZ    [ 8 ];
    char CR_LF     [ 2 ];
};

class  FILE_DCOSMEZVIR : public FILE_FIX
{
   public:

    FILE_DCOSMEZVIR(const STRINGA& NomeFile,ULONG BufSize=64000):
        FILE_FIX(NomeFile,sizeof(DCOSMEZVIR),BufSize){};
    DCOSMEZVIR &  operator [](ULONG Indice){ Posiziona(Indice); return *(DCOSMEZVIR*) RecordC; };
    DCOSMEZVIR &  RecordCorrente(){ return *(DCOSMEZVIR*) RecordC; };
};

struct DECCFERMEZ
{
    char NUMVIR [ 5 ];
    char PRGVIR [ 2 ];
    char CODTIPECC;
    char PRGSTA [ 3 ];
    char DATAINI [ 10 ];
    char CODGIOSP [ 2 ];
    char EFFETT;
    char DATAFIN [ 10 ];
    char GIORNISET [ 7 ];
    char CODDSZ    [ 8 ];
    char DATINIVAL [ 10 ];
    char CR_LF     [ 2 ];
};

class  FILE_DECCFERMEZ : public FILE_FIX
{
   public:

    FILE_DECCFERMEZ(const STRINGA& NomeFile,ULONG BufSize=64000):
        FILE_FIX(NomeFile,sizeof(DECCFERMEZ),BufSize){};
    DECCFERMEZ &  operator [](ULONG Indice){ Posiziona(Indice); return *(DECCFERMEZ*) RecordC; };
    DECCFERMEZ &  RecordCorrente(){ return *(DECCFERMEZ*) RecordC; };
};

struct DPERMEZVIR
{
    char NUMVIR    [ 5 ];
    char PRGVIR    [ 2 ];
    char PRGINI    [ 3 ];   // Va impostato al PRG della prima fermata del virtuale.
    char PRGFIN    [ 3 ];   // Va impostato al PRG dell'ultima fermata del virtuale.
    char DATAINI   [ 10 ];
    char CODGIOSP  [ 2 ];
    char EFFETT;
    char GIORNISET [ 7 ];
    char DATAFIN   [ 10 ];
    char CODDSZ    [ 8 ];
    char DATINIVAL [ 10 ];
    char CR_LF     [ 2 ];
};

class  FILE_DPERMEZVIR : public FILE_FIX
{
   public:

    FILE_DPERMEZVIR(const STRINGA& NomeFile,ULONG BufSize=64000):
        FILE_FIX(NomeFile,sizeof(DPERMEZVIR),BufSize){};
    DPERMEZVIR &  operator [](ULONG Indice){ Posiziona(Indice); return *(DPERMEZVIR*) RecordC; };
    DPERMEZVIR &  RecordCorrente(){ return *(DPERMEZVIR*) RecordC; };
};

struct DSEROFF
{
    char NUMVIR    [ 5 ];       // Per questa tabella bisogna accorpare le
    char PRGVIR    [ 2 ];       // MM_INFO di tutti i viaggianti di ciascun
    char CODTIPSER [ 2 ];       // virtuale tramite il metodo CombinaTratta()
    char CODSER    [ 2 ];       // di MM_INFO; sulla struttura risultante,
    char PRGINI    [ 3 ];       // si fa il giro dei servizi scrivendo un
    char PRGFIN    [ 3 ];       // record per ogni bit acceso.
    char DATAINI   [ 10 ];
    char CODGIOSP  [ 2 ];
    char EFFETT;
    char CLASSE;
    char GIORNISET [ 7 ];
    char DATAFIN   [ 10 ];
    char OBBLIGATORIO;
    char PAGAMENTO;
    char CODDSZ    [ 8 ];
    char DATINIVAL [ 10 ];
    char CR_LF     [ 2 ];
};

class  FILE_DSEROFF : public FILE_FIX
{
   public:

    FILE_DSEROFF(const STRINGA& NomeFile,ULONG BufSize=64000):
        FILE_FIX(NomeFile,sizeof(DSEROFF),BufSize){};
    DSEROFF &  operator [](ULONG Indice){ Posiziona(Indice); return *(DSEROFF*) RecordC; };
    DSEROFF &  RecordCorrente(){ return *(DSEROFF*) RecordC; };
};

#endif
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
//+-+-+-  FINE MODULO  -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
