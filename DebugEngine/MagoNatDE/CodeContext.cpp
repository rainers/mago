/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "CodeContext.h"
#include "Module.h"


namespace Mago
{
    // CodeContext

    CodeContext::CodeContext()
        :   mAddr( 0 )
    {
        memset( &mFuncSH, 0, sizeof mFuncSH );
        memset( &mBlockSH, 0, sizeof mBlockSH );
    }

    CodeContext::~CodeContext()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugMemoryContext2 

    HRESULT CodeContext::GetName( BSTR* pbstrName ) 
    {
        // The name of a memory context is not normally used.
        UNREFERENCED_PARAMETER( pbstrName );
        return E_NOTIMPL;
    }

    HRESULT CodeContext::GetInfo(
        CONTEXT_INFO_FIELDS dwFields,
        CONTEXT_INFO*       pInfo ) 
    {
        if ( pInfo == NULL )
            return E_INVALIDARG;

        pInfo->dwFields = 0;

        if ( (dwFields & CIF_MODULEURL) != 0 )
        {
            if ( mModule != NULL )
            {
                MODULE_INFO modInfo = { 0 };

                mModule->GetInfo( MIF_URL, &modInfo );

                // it was allocated by Module::GetInfo for modInfo.m_bstrUrl, so take it
                // now the output structure owns it
                pInfo->bstrModuleUrl = modInfo.m_bstrUrl;

                if ( pInfo->bstrModuleUrl != NULL )
                    pInfo->dwFields |= CIF_MODULEURL;
            }
        }

        if ( (dwFields & CIF_FUNCTION) != 0 )
        {
            pInfo->bstrFunction = GetFunctionName();

            if ( pInfo->bstrFunction != NULL )
                pInfo->dwFields |= CIF_FUNCTION;
        }

        if ( (dwFields & CIF_FUNCTIONOFFSET) != 0 )
        {
            pInfo->posFunctionOffset = GetFunctionOffset();
            pInfo->dwFields |= CIF_FUNCTIONOFFSET;
        }

        if ( (dwFields & CIF_ADDRESS) != 0 )
        {
            // TODO: maybe we should have a central place for formatting

            const size_t    HexDigitCount = sizeof mAddr * 2;   // 2 chars a byte in hex
            wchar_t addrStr[HexDigitCount + 2 + 1] = L"";

            if ( sizeof mAddr > sizeof( int ) )
                swprintf_s( addrStr, L"0x%016I64x", mAddr );
            else
                swprintf_s( addrStr, L"0x%08x", mAddr );

            pInfo->bstrAddress = SysAllocString( addrStr );

            if ( pInfo->bstrAddress != NULL )
                pInfo->dwFields |= CIF_ADDRESS;
        }

        if ( (dwFields & CIF_ADDRESSOFFSET) != 0 )
        {
        }

        if ( (dwFields & CIF_ADDRESSABSOLUTE) != 0 )
        {
        }

        return S_OK;
    }

    HRESULT CodeContext::Add(
        UINT64                 dwCount,
        IDebugMemoryContext2** ppMemCxt ) 
    {
        if ( ppMemCxt == NULL )
            return E_INVALIDARG;

        // just truncate the offset
        Address newAddr = mAddr + (Address) dwCount;

        return MakeCodeContext( newAddr, mModule, mDocContext, ppMemCxt );
    }

    HRESULT CodeContext::Subtract(
        UINT64                 dwCount,
        IDebugMemoryContext2** ppMemCxt )
    {
        if ( ppMemCxt == NULL )
            return E_INVALIDARG;

        // just truncate the offset
        Address newAddr = mAddr - (Address) dwCount;

        return MakeCodeContext( newAddr, mModule, mDocContext, ppMemCxt );
    }

    HRESULT CodeContext::Compare(
        CONTEXT_COMPARE        compare,
        IDebugMemoryContext2** rgpMemoryContextSet,
        DWORD                  dwMemoryContextSetLen,
        DWORD*                 pdwMemoryContext )
    {
        if ( rgpMemoryContextSet == NULL )
            return E_INVALIDARG;
        if ( dwMemoryContextSetLen == 0 )
            return E_INVALIDARG;
        if ( pdwMemoryContext == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        for ( int i = 0; i < (int) dwMemoryContextSetLen; i++ )
        {
            CComPtr<IMagoMemoryContext> magoCode;
            Address     otherAddr = 0;
            bool        result = false;

            hr = rgpMemoryContextSet[i]->QueryInterface( __uuidof( IMagoMemoryContext ), (void**) &magoCode );
            if ( FAILED( hr ) )
                return E_COMPARE_CANNOT_COMPARE;

            magoCode->GetAddress( otherAddr );

            switch ( compare )
            {
            case CONTEXT_EQUAL:
                result = (mAddr == otherAddr);
                break;

            case CONTEXT_LESS_THAN:
                result = (mAddr < otherAddr);
                break;

            case CONTEXT_GREATER_THAN:
                result = (mAddr > otherAddr);
                break;

            case CONTEXT_LESS_THAN_OR_EQUAL:
                result = (mAddr <= otherAddr);
                break;

            case CONTEXT_GREATER_THAN_OR_EQUAL:
                result = (mAddr >= otherAddr);
                break;

                // TODO:
            case CONTEXT_SAME_SCOPE:
                result = (mAddr == otherAddr);
                break;

            case CONTEXT_SAME_FUNCTION:
            case CONTEXT_SAME_MODULE:
            case CONTEXT_SAME_PROCESS:
                break;
            }

            if ( result )
            {
                *pdwMemoryContext = i;
                return S_OK;
            }
        }

        return S_FALSE;
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugCodeContext2 

    HRESULT CodeContext::GetDocumentContext( IDebugDocumentContext2** ppSrcCxt ) 
    {
        if ( ppSrcCxt == NULL )
            return E_INVALIDARG;

        *ppSrcCxt = mDocContext;
        if ( *ppSrcCxt != NULL )
            (*ppSrcCxt)->AddRef();

        return S_OK;
    }

    HRESULT CodeContext::GetLanguageInfo(         
        BSTR* pbstrLanguage,
        GUID* pguidLanguage )
    {
        if ( mDocContext == NULL )
            return S_FALSE;

        return mDocContext->GetLanguageInfo( pbstrLanguage, pguidLanguage );
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IMagoCodeContext

    HRESULT CodeContext::GetAddress( Address& addr )
    {
        addr = mAddr;
        return S_OK;
    }


    //----------------------------------------------------------------------------

    HRESULT CodeContext::MakeCodeContext( 
        Address newAddr, 
        Module* mod, 
        IDebugDocumentContext2* docContext, 
        IDebugMemoryContext2** ppMemCxt )
    {
        if ( ppMemCxt == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        RefPtr<CodeContext> newContext;

        hr = MakeCComObject( newContext );
        if ( FAILED( hr ) )
            return hr;

        hr = newContext->Init( newAddr, mod, docContext );
        if ( FAILED( hr ) )
            return hr;

        *ppMemCxt = newContext.Detach();

        return S_OK;
    }

    HRESULT CodeContext::Init( Address addr, Module* mod, IDebugDocumentContext2* docContext )
    {
        mAddr = addr;
        mModule = mod;
        mDocContext = docContext;
        return S_OK;
    }

    HRESULT CodeContext::FindFunction()
    {
        HRESULT hr = S_OK;
        RefPtr<MagoST::ISession>    session;
        MagoST::SymHandle           symHandle = { 0 };

        // already found
        if ( !IsSymHandleNull( mFuncSH ) )
            return S_OK;

        if ( mModule == NULL )
            return E_NOT_FOUND;

        if ( !mModule->GetSymbolSession( session ) )
            return E_NOT_FOUND;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        sec = session->GetSecOffsetFromVA( mAddr, offset );
        if ( sec == 0 )
            return E_NOT_FOUND;

        // TODO: verify that it's a function or public symbol (or something else?)
        hr = session->FindOuterSymbolByAddr( MagoST::SymHeap_GlobalSymbols, sec, offset, symHandle );
        if ( FAILED( hr ) )
        {
            hr = session->FindOuterSymbolByAddr( MagoST::SymHeap_StaticSymbols, sec, offset, symHandle );
            if ( FAILED( hr ) )
            {
                hr = session->FindOuterSymbolByAddr( MagoST::SymHeap_PublicSymbols, sec, offset, symHandle );
                if ( FAILED( hr ) )
                    return hr;
            }
        }

        mFuncSH = symHandle;

        hr = session->FindInnermostSymbol( mFuncSH, sec, offset, mBlockSH );
        // it might be a public symbol, which doesn't have anything: blocks, args, or locals

        return S_OK;
    }

    BSTR CodeContext::GetFunctionName()
    {
        HRESULT hr = S_OK;
        MagoST::SymInfoData         infoData = { 0 };
        MagoST::ISymbolInfo*        symInfo = NULL;
        RefPtr<MagoST::ISession>    session;
        SymString                   pstrName;
        CComBSTR                    bstrName;

        hr = FindFunction();
        if ( FAILED( hr ) )
            return NULL;

        if ( !mModule->GetSymbolSession( session ) )
            return NULL;

        hr = session->GetSymbolInfo( mFuncSH, infoData, symInfo );
        if ( FAILED( hr ) )
            return NULL;

        if ( !symInfo->GetName( pstrName ) )
            return NULL;

        hr = Utf8To16( pstrName.GetName(), pstrName.GetLength(), bstrName.m_str );
        if ( FAILED( hr ) )
            return NULL;

        return bstrName.Detach();
    }

    TEXT_POSITION CodeContext::GetFunctionOffset()
    {
        TEXT_POSITION   pos = { 0 };
#if 0
        HRESULT         hr = S_OK;
        RefPtr<MagoST::ISession>    session;
        MagoST::SymInfoData         infoData = { 0 };
        MagoST::ISymbolInfo*        symInfo = NULL;
        uint32_t                    startOffset = 0;

        hr = FindFunction();
        if ( FAILED( hr ) )
            return pos;

        if ( !mModule->GetSymbolSession( session ) )
            return pos;

        hr = session->GetSymbolInfo( mFuncSH, infoData, symInfo );
        if ( FAILED( hr ) )
            return pos;

        if ( !symInfo->GetDebugStart( startOffset ) )
            return pos;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        sec = session->GetSecOffsetFromVA( mAddr, offset );
        if ( sec == 0 )
            return pos;

        session->FindLine(
#endif
        return pos;
    }
}
