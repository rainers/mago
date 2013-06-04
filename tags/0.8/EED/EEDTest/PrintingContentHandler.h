/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class PrintingContentHandler : public ISAXContentHandler
{
    LONG    mRefCount;
    int     mIndent;

public:
    PrintingContentHandler();

    virtual HRESULT STDMETHODCALLTYPE QueryInterface( const IID& riid, void** ppvObject );
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    virtual HRESULT STDMETHODCALLTYPE putDocumentLocator( 
        /* [in] */ ISAXLocator *pLocator);
    
    virtual HRESULT STDMETHODCALLTYPE startDocument();
    
    virtual HRESULT STDMETHODCALLTYPE endDocument();
    
    virtual HRESULT STDMETHODCALLTYPE startPrefixMapping( 
        /* [in] */ const wchar_t *pwchPrefix,
        /* [in] */ int cchPrefix,
        /* [in] */ const wchar_t *pwchUri,
        /* [in] */ int cchUri);
    
    virtual HRESULT STDMETHODCALLTYPE endPrefixMapping( 
        /* [in] */ const wchar_t *pwchPrefix,
        /* [in] */ int cchPrefix);
    
    virtual HRESULT STDMETHODCALLTYPE startElement( 
        /* [in] */ const wchar_t *pwchNamespaceUri,
        /* [in] */ int cchNamespaceUri,
        /* [in] */ const wchar_t *pwchLocalName,
        /* [in] */ int cchLocalName,
        /* [in] */ const wchar_t *pwchQName,
        /* [in] */ int cchQName,
        /* [in] */ ISAXAttributes *pAttributes);
    
    virtual HRESULT STDMETHODCALLTYPE endElement( 
        /* [in] */ const wchar_t *pwchNamespaceUri,
        /* [in] */ int cchNamespaceUri,
        /* [in] */ const wchar_t *pwchLocalName,
        /* [in] */ int cchLocalName,
        /* [in] */ const wchar_t *pwchQName,
        /* [in] */ int cchQName);
    
    virtual HRESULT STDMETHODCALLTYPE characters( 
        /* [in] */ const wchar_t *pwchChars,
        /* [in] */ int cchChars);
    
    virtual HRESULT STDMETHODCALLTYPE ignorableWhitespace( 
        /* [in] */ const wchar_t *pwchChars,
        /* [in] */ int cchChars);
    
    virtual HRESULT STDMETHODCALLTYPE processingInstruction( 
        /* [in] */ const wchar_t *pwchTarget,
        /* [in] */ int cchTarget,
        /* [in] */ const wchar_t *pwchData,
        /* [in] */ int cchData);
    
    virtual HRESULT STDMETHODCALLTYPE skippedEntity( 
        /* [in] */ const wchar_t *pwchName,
        /* [in] */ int cchName);

private:
    void Indent();
};
