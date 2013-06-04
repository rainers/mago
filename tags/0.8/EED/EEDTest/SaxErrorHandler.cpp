/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "SaxErrorHandler.h"


#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SAXErrorHandler::SAXErrorHandler()
:   mRefCount( 0 )
{

}

SAXErrorHandler::~SAXErrorHandler()
{

}

HRESULT STDMETHODCALLTYPE SAXErrorHandler::error( 
    /* [in] */ ISAXLocator __RPC_FAR *pLocator,
    /* [in] */ const wchar_t * pwchErrorMessage,
    /* [in] */ HRESULT errCode)
{
    int line = 0;
    int col = 0;

    pLocator->getLineNumber( &line );
    pLocator->getColumnNumber( &col );
    printf( "%d, %d: ", line, col );

    ReportError(pwchErrorMessage, errCode); 
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SAXErrorHandler::fatalError( 
    /* [in] */ ISAXLocator __RPC_FAR *pLocator,
    /* [in] */ const wchar_t * pwchErrorMessage,
    /* [in] */ HRESULT errCode)
{
    int line = 0;
    int col = 0;

    pLocator->getLineNumber( &line );
    pLocator->getColumnNumber( &col );
    printf( "%d, %d: ", line, col );

    ReportError(pwchErrorMessage, errCode); 
    return S_OK;
}

HRESULT STDMETHODCALLTYPE SAXErrorHandler::ignorableWarning( 
    /* [in] */ ISAXLocator __RPC_FAR *pLocator,
    /* [in] */ const wchar_t * pwchErrorMessage,
    /* [in] */ HRESULT errCode)
{
    return S_OK;
}

HRESULT SAXErrorHandler::QueryInterface( const GUID& riid, void** ppvObject )
{
    IUnknown*   unk = NULL;

    if ( ppvObject == NULL )
        return E_POINTER;

    if ( riid == IID_IUnknown )
        unk = static_cast<IUnknown*>( this );
    else if ( riid == IID_ISAXErrorHandler )
        unk = static_cast<ISAXErrorHandler*>( this );
    else
        return E_NOINTERFACE;

    unk->AddRef();
    *ppvObject = unk;

    return S_OK;
}

ULONG SAXErrorHandler::AddRef()
{
    return InterlockedIncrement( &mRefCount );
}

ULONG SAXErrorHandler::Release()
{
    LONG    newRef = InterlockedDecrement( &mRefCount );
    _ASSERT( newRef >= 0 );
    if ( newRef == 0 )
    {
        delete this;
    }
    return newRef;
}

void SAXErrorHandler::ReportError(
                                      /* [in] */ const wchar_t __RPC_FAR *pwchVal,
                                      /* [in] */ HRESULT errCode)
{
    int len = wcslen(pwchVal);
    static wchar_t val[1000];
    len = len>999 ? 999 : len;
    wcsncpy_s( val, pwchVal, len );
    val[len] = 0;
    wprintf(L"\nError Message:\n%s ",val);
}
