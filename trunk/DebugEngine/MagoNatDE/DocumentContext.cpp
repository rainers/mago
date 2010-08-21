/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DocumentContext.h"


namespace Mago
{
    // DocumentContext

    DocumentContext::DocumentContext()
        :   mLangGuid( GUID_NULL )
    {
        memset( &mStatementBegin, 0, sizeof mStatementBegin );
        memset( &mStatementEnd, 0, sizeof mStatementEnd );
    }

    DocumentContext::~DocumentContext()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugDocumentContext2 

    HRESULT DocumentContext::GetDocument( 
       IDebugDocument2** ppDocument
    )
    {
        // This method is for those debug engines that supply documents 
        // directly to the IDE. Otherwise, this method should return E_NOTIMPL.
        UNREFERENCED_PARAMETER( ppDocument );
        return E_NOTIMPL;
    }

    HRESULT DocumentContext::GetName( 
       GETNAME_TYPE gnType,
       BSTR*        pbstrFileName
    )
    {
        if ( pbstrFileName == NULL )
            return E_INVALIDARG;

        // TODO: base it on gnType
        *pbstrFileName = mFilename.Copy();
        return *pbstrFileName != NULL ? S_OK : E_OUTOFMEMORY;
    }

    // EnumCodeContexts is implemented in a derived class

    HRESULT DocumentContext::GetLanguageInfo( 
       BSTR* pbstrLanguage,
       GUID* pguidLanguage
    )
    {
        if ( (pbstrLanguage == NULL) && (pguidLanguage == NULL) )
            return E_INVALIDARG;

        if ( pbstrLanguage != NULL )
        {
            *pbstrLanguage = mLangName.Copy();
            if ( *pbstrLanguage == NULL )
                return E_OUTOFMEMORY;
        }

        if ( pguidLanguage != NULL )
            *pguidLanguage = mLangGuid;

        return S_OK;
    }

    HRESULT DocumentContext::GetStatementRange( 
       TEXT_POSITION* pBegPosition,
       TEXT_POSITION* pEndPosition
    )
    {
        if ( pBegPosition == NULL )
            return E_INVALIDARG;

        *pBegPosition = mStatementBegin;

        if ( pEndPosition != NULL )
            *pEndPosition = mStatementEnd;

        return S_OK;
    }

    HRESULT DocumentContext::GetSourceRange( 
       TEXT_POSITION* pBegPosition,
       TEXT_POSITION* pEndPosition
    )
    {
        return E_NOTIMPL;
    }

    HRESULT DocumentContext::Compare( 
       DOCCONTEXT_COMPARE       compare,
       IDebugDocumentContext2** rgpDocContextSet,
       DWORD                    dwDocContextSetLen,
       DWORD*                   pdwDocContext
    )
    {
        return E_NOTIMPL;
    }

    HRESULT DocumentContext::Seek( 
       int                      nCount,
       IDebugDocumentContext2** ppDocContext
    )
    {
        return E_NOTIMPL;
    }


    //----------------------------------------------------------------------------

    HRESULT DocumentContext::Init(
        const wchar_t* filename,
        TEXT_POSITION& statementBegin,
        TEXT_POSITION& statementEnd,
        const wchar_t* langName,
        const GUID& langGuid )
    {
        if ( filename == NULL )
            return E_INVALIDARG;

        mFilename = filename;
        mStatementBegin = statementBegin;
        mStatementEnd = statementEnd;
        mLangName = langName;
        mLangGuid = langGuid;

        if ( mFilename == NULL )
            return E_OUTOFMEMORY;

        return S_OK;
    }
}
