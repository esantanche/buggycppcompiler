#ifndef HO_SPX_MAIN
#define HO_SPX_MAIN
//----------------------------------------------------------------------------
// FILE SPX_MAIN.HPP
//----------------------------------------------------------------------------
// Questa classe controlla il menu' di sistema della mappe principale
// di tutte le applicazioni SIPAX

class _export SPX_MAIN : public MAP_VIEW {
    public:

    SPX_MAIN(const STRINGA & Nome, INI_BUFFER & InitBuf, class OBJ_SET * OSet);
    // ~SPX_MAIN();

    virtual BOOL ChiudiMappa(BOOL Show=TRUE);
   virtual int DispatchAsync(        OGGETTO        * Sender,
                               const STRINGA        & Metodo,
                                     ELENCO         & Parametri,
                               const STRINGA        & Messaggio = NUSTR);
};

#endif
