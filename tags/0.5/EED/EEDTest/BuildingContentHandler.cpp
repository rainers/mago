/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "BuildingContentHandler.h"
#include "Element.h"

using namespace std;


//----------------------------------------------------------------------------
//  ProxyAttributeElement
//----------------------------------------------------------------------------

class ProxyAttributeElement : public Element
{
    int         mChildrenAdded;
    wstring     mAttrName;
    Element*    mParent;

public:
    ProxyAttributeElement( const wchar_t* attrName, Element* parent );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


ProxyAttributeElement::ProxyAttributeElement( const wchar_t* attrName, Element* parent )
    :   mChildrenAdded( 0 ),
        mAttrName( attrName ),
        mParent( parent )
{
}

void ProxyAttributeElement::AddChild( Element* elem )
{
    if ( mChildrenAdded > 0 )
        throw L"Only one child supported for complex attribute.";

    mChildrenAdded++;
    mParent->SetAttribute( mAttrName.c_str(), elem );
}

void ProxyAttributeElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    throw L"Proxy attribute can't have attributes.";
}

void ProxyAttributeElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    throw L"Proxy attribute can't have attributes.";
}

void ProxyAttributeElement::PrintElement()
{
    throw L"ProxyAttributeElement::PrintElement not supported.";
}


//----------------------------------------------------------------------------
//  BuildingContentHandler
//----------------------------------------------------------------------------

BuildingContentHandler::BuildingContentHandler( ElementFactory* factory )
:   mRefCount( 0 ),
    mFactory( factory )
{
}

Element*    BuildingContentHandler::GetRoot()
{
    return mRoot.Get();
}

HRESULT BuildingContentHandler::QueryInterface( const IID& riid, void** ppvObject )
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

ULONG BuildingContentHandler::AddRef()
{
    return InterlockedIncrement( &mRefCount );
}

ULONG BuildingContentHandler::Release()
{
    LONG    newRef = InterlockedDecrement( &mRefCount );
    _ASSERT( newRef >= 0 );
    if ( newRef == 0 )
    {
        delete this;
    }
    return newRef;
}

HRESULT BuildingContentHandler::putDocumentLocator( 
    /* [in] */ ISAXLocator *pLocator)
{
    return S_OK;
}

HRESULT BuildingContentHandler::startDocument()
{
    return S_OK;
}

HRESULT BuildingContentHandler::endDocument()
{
    return S_OK;
}

HRESULT BuildingContentHandler::startPrefixMapping( 
    /* [in] */ const wchar_t *pwchPrefix,
    /* [in] */ int cchPrefix,
    /* [in] */ const wchar_t *pwchUri,
    /* [in] */ int cchUri)
{
    return E_NOTIMPL;
}

HRESULT BuildingContentHandler::endPrefixMapping( 
    /* [in] */ const wchar_t *pwchPrefix,
    /* [in] */ int cchPrefix)
{
    return E_NOTIMPL;
}

HRESULT BuildingContentHandler::startElement( 
    /* [in] */ const wchar_t *pwchNamespaceUri,
    /* [in] */ int cchNamespaceUri,
    /* [in] */ const wchar_t *pwchLocalName,
    /* [in] */ int cchLocalName,
    /* [in] */ const wchar_t *pwchQName,
    /* [in] */ int cchQName,
    /* [in] */ ISAXAttributes *pAttributes)
{
    try
    {
        auto_ptr<Element>   elem;
        int                 attrCount = 0;
        const wchar_t*      dotPtr = NULL;

        dotPtr = wmemchr( pwchLocalName, L'.', cchLocalName );
        if ( dotPtr != NULL )
        {
            wstring name;
            int lenToDot = (dotPtr - pwchLocalName) + 1;

            // dot can't be at the end
            if ( lenToDot >= cchLocalName )
                return E_FAIL;
            // this is a complex attribute, so it has to have a parent
            if ( mStack.empty() )
                return E_FAIL;

            // get the attribute name part
            name.append( dotPtr + 1, cchLocalName - lenToDot );
            mAttrProxy = new ProxyAttributeElement( name.c_str(), mStack.back() );
            mStack.push_back( mAttrProxy.Get() );
            return S_OK;
        }

        wstring localName;
        localName.append( pwchLocalName, cchLocalName );

        elem.reset( mFactory->NewElement( localName.c_str() ) );
        if ( elem.get() == NULL )
            return E_FAIL;

        pAttributes->getLength( &attrCount );
        for ( int i = 0; i < attrCount; i++ )
        {
            wstring name;
            wstring val;
            const wchar_t*  strPtr = NULL;
            int             strLen = 0;

            pAttributes->getLocalName( i, &strPtr, &strLen );
            name.append( strPtr, strLen );

            pAttributes->getValue( i, &strPtr, &strLen );
            val.append( strPtr, strLen );

            elem->SetAttribute( name.c_str(), val.c_str() );
        }

        // someone has to be responsible for the element: mRoot (content handler) or top of stack element
        if ( !mStack.empty() )
            mStack.back()->AddChild( elem.get() );
        else
            mRoot = elem.get();

        mStack.push_back( elem.get() );

        elem.release();
    }
    catch ( const wchar_t* msg )
    {
        printf( "Error: %ls\n", msg );
        return E_FAIL;
    }

    return S_OK;
}

HRESULT BuildingContentHandler::endElement( 
    /* [in] */ const wchar_t *pwchNamespaceUri,
    /* [in] */ int cchNamespaceUri,
    /* [in] */ const wchar_t *pwchLocalName,
    /* [in] */ int cchLocalName,
    /* [in] */ const wchar_t *pwchQName,
    /* [in] */ int cchQName)
{
    mStack.pop_back();
    return S_OK;
}

HRESULT BuildingContentHandler::characters( 
    /* [in] */ const wchar_t *pwchChars,
    /* [in] */ int cchChars)
{
    return S_OK;
}

HRESULT BuildingContentHandler::ignorableWhitespace( 
    /* [in] */ const wchar_t *pwchChars,
    /* [in] */ int cchChars)
{
    return E_NOTIMPL;
}

HRESULT BuildingContentHandler::processingInstruction( 
    /* [in] */ const wchar_t *pwchTarget,
    /* [in] */ int cchTarget,
    /* [in] */ const wchar_t *pwchData,
    /* [in] */ int cchData)
{
    return E_NOTIMPL;
}

HRESULT BuildingContentHandler::skippedEntity( 
    /* [in] */ const wchar_t *pwchName,
    /* [in] */ int cchName)
{
    return E_NOTIMPL;
}
