/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class SAXErrorHandler : public ISAXErrorHandler  
{
    LONG    mRefCount;

public:
    SAXErrorHandler();
    virtual ~SAXErrorHandler();

    // This must be correctly implemented, if your handler must be a COM Object (in this example it does not)
    HRESULT __stdcall QueryInterface( const GUID& riid,void** ppvObject );
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    virtual HRESULT STDMETHODCALLTYPE error( 
        /* [in] */ ISAXLocator *pLocator,
        /* [in] */ const wchar_t *pwchErrorMessage,
        /* [in] */ HRESULT hrErrorCode);

    virtual HRESULT STDMETHODCALLTYPE fatalError( 
        /* [in] */ ISAXLocator *pLocator,
        /* [in] */ const wchar_t *pwchErrorMessage,
        /* [in] */ HRESULT hrErrorCode);

    virtual HRESULT STDMETHODCALLTYPE ignorableWarning( 
        /* [in] */ ISAXLocator *pLocator,
        /* [in] */ const wchar_t *pwchErrorMessage,
        /* [in] */ HRESULT hrErrorCode);

private:
    void ReportError(
        /* [in] */ const wchar_t __RPC_FAR *pwchVal,
        /* [in] */ HRESULT errCode);
};
