/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "BPDocumentContext.h"
#include "PendingBreakpoint.h"


namespace Mago
{
    HRESULT BPDocumentContext::EnumCodeContexts( 
       IEnumDebugCodeContexts2** ppEnumCodeCxts
    )
    {
        // TODO: let's see if this function is ever called
        _ASSERT( false );

        if ( mBPParent.Get() != NULL )
            return mBPParent->EnumCodeContexts( ppEnumCodeCxts );

        return E_FAIL;
    }

    HRESULT BPDocumentContext::Init(
        PendingBreakpoint* pendingBP,
        const wchar_t* filename,
        TEXT_POSITION& statementBegin,
        TEXT_POSITION& statementEnd,
        const wchar_t* langName,
        const GUID& langGuid )
    {
        mBPParent = pendingBP;

        return DocumentContext::Init( filename, statementBegin, statementEnd, langName, langGuid );
    }

    HRESULT BPDocumentContext::Clone( DocumentContext** ppDocContext )
    {
        HRESULT hr = S_OK;
        RefPtr<BPDocumentContext>   docContext;

        hr = MakeCComObject( docContext );
        if ( FAILED( hr ) )
            return hr;

        hr = docContext->Init( mBPParent, mFilename, mStatementBegin, mStatementEnd, mLangName, mLangGuid );
        if ( FAILED( hr ) )
            return hr;

        *ppDocContext = docContext.Detach();
        return S_OK;
    }

    void BPDocumentContext::Dispose()
    {
        mBPParent.Release();
    }
}
