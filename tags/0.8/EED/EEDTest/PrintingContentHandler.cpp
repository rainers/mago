/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "PrintingContentHandler.h"


PrintingContentHandler::PrintingContentHandler()
:   mRefCount( 0 ),
    mIndent( 0 )
{
}

HRESULT PrintingContentHandler::QueryInterface( const IID& riid, void** ppvObject )
{
    IUnknown*   unk = NULL;

    if ( ppvObject == NULL )
        return E_POINTER;

    if ( riid == IID_IUnknown )
        unk = static_cast<IUnknown*>( this );
    else if ( riid == IID_ISAXContentHandler )
        unk = static_cast<ISAXContentHandler*>( this );
    else
        return E_NOINTERFACE;

    unk->AddRef();
    *ppvObject = unk;

    return S_OK;
}

ULONG PrintingContentHandler::AddRef()
{
    return InterlockedIncrement( &mRefCount );
}

ULONG PrintingContentHandler::Release()
{
    LONG    newRef = InterlockedDecrement( &mRefCount );
    _ASSERT( newRef >= 0 );
    if ( newRef == 0 )
    {
        delete this;
    }
    return newRef;
}

HRESULT PrintingContentHandler::putDocumentLocator( 
    /* [in] */ ISAXLocator *pLocator)
{
    return S_OK;
}

HRESULT PrintingContentHandler::startDocument()
{
    return S_OK;
}

HRESULT PrintingContentHandler::endDocument()
{
    return S_OK;
}

HRESULT PrintingContentHandler::startPrefixMapping( 
    /* [in] */ const wchar_t *pwchPrefix,
    /* [in] */ int cchPrefix,
    /* [in] */ const wchar_t *pwchUri,
    /* [in] */ int cchUri)
{
    return E_NOTIMPL;
}

HRESULT PrintingContentHandler::endPrefixMapping( 
    /* [in] */ const wchar_t *pwchPrefix,
    /* [in] */ int cchPrefix)
{
    return E_NOTIMPL;
}

HRESULT PrintingContentHandler::startElement( 
    /* [in] */ const wchar_t *pwchNamespaceUri,
    /* [in] */ int cchNamespaceUri,
    /* [in] */ const wchar_t *pwchLocalName,
    /* [in] */ int cchLocalName,
    /* [in] */ const wchar_t *pwchQName,
    /* [in] */ int cchQName,
    /* [in] */ ISAXAttributes *pAttributes)
{
    Indent();
    printf( "start element \"%.*ls\"\n", cchLocalName, pwchLocalName );
    mIndent += 4;
    return S_OK;
}

HRESULT PrintingContentHandler::endElement( 
    /* [in] */ const wchar_t *pwchNamespaceUri,
    /* [in] */ int cchNamespaceUri,
    /* [in] */ const wchar_t *pwchLocalName,
    /* [in] */ int cchLocalName,
    /* [in] */ const wchar_t *pwchQName,
    /* [in] */ int cchQName)
{
    mIndent -= 4;
    Indent();
    printf( "end element \"%.*ls\"\n", cchLocalName, pwchLocalName );
    return S_OK;
}

HRESULT PrintingContentHandler::characters( 
    /* [in] */ const wchar_t *pwchChars,
    /* [in] */ int cchChars)
{
    Indent();
    printf( "chars \"%.*ls\"\n", cchChars, pwchChars );
    return S_OK;
}

HRESULT PrintingContentHandler::ignorableWhitespace( 
    /* [in] */ const wchar_t *pwchChars,
    /* [in] */ int cchChars)
{
    return E_NOTIMPL;
}

HRESULT PrintingContentHandler::processingInstruction( 
    /* [in] */ const wchar_t *pwchTarget,
    /* [in] */ int cchTarget,
    /* [in] */ const wchar_t *pwchData,
    /* [in] */ int cchData)
{
    return E_NOTIMPL;
}

HRESULT PrintingContentHandler::skippedEntity( 
    /* [in] */ const wchar_t *pwchName,
    /* [in] */ int cchName)
{
    return E_NOTIMPL;
}

void PrintingContentHandler::Indent()
{
    for ( int i = 0; i < mIndent; i++ )
        putc( ' ', stdout );
}
