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
        :   mAddr( 0 ),
            mPtrSize( 0 )
    {
        memset( &mFuncSH, 0, sizeof mFuncSH );
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
            wchar_t addrStr[MaxAddrStringLength + 1] = L"";

            FormatAddress( addrStr, _countof( addrStr ), mAddr, mPtrSize, true );

            pInfo->bstrAddress = SysAllocString( addrStr );

            if ( pInfo->bstrAddress != NULL )
                pInfo->dwFields |= CIF_ADDRESS;
        }

        if ( (dwFields & CIF_ADDRESSOFFSET) != 0 )
        {
            uint32_t offset = 0;
            if( GetAddressOffset( offset ) && offset != 0 )
            {
                wchar_t offStr[MaxAddrStringLength + 1] = L"";
                swprintf_s( offStr, MaxAddrStringLength, L"+0x%x", offset );

                pInfo->bstrAddressOffset = SysAllocString( offStr );

                if ( pInfo->bstrAddressOffset != NULL )
                    pInfo->dwFields |= CIF_ADDRESSOFFSET;
            }
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

        // just truncate the offset in the new context
        Address64 newAddr = mAddr + (Address64) dwCount;

        return MakeCodeContext( newAddr, mModule, mDocContext, ppMemCxt, mPtrSize );
    }

    HRESULT CodeContext::Subtract(
        UINT64                 dwCount,
        IDebugMemoryContext2** ppMemCxt )
    {
        if ( ppMemCxt == NULL )
            return E_INVALIDARG;

        // just truncate the offset in the new context
        Address64 newAddr = mAddr - (Address64) dwCount;

        return MakeCodeContext( newAddr, mModule, mDocContext, ppMemCxt, mPtrSize );
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
            Address64   otherAddr = 0;
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

            case CONTEXT_SAME_FUNCTION:
            case CONTEXT_SAME_MODULE:
            case CONTEXT_SAME_PROCESS:
            case CONTEXT_SAME_SCOPE: 
            {
                MagoST::SymHandle funcSH, blockSH;
                RefPtr<Module> module;
                if( magoCode->GetScope( funcSH, blockSH, module ) == S_OK )
                {
                    FindFunction();
                    switch ( compare )
                    {
                    case CONTEXT_SAME_SCOPE: 
                        result = memcmp( &mBlockSH.back(), &blockSH, sizeof( blockSH ) ) == 0;
                        break;
                    case CONTEXT_SAME_FUNCTION:
                        result = memcmp( &mFuncSH, &funcSH, sizeof( mFuncSH ) ) == 0;
                        break;
                    case CONTEXT_SAME_MODULE:
                        result = (mModule == module);
                        break;
                    case CONTEXT_SAME_PROCESS:
                        // TODO
                        break;
                    }
                }
            }
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

    HRESULT CodeContext::GetAddress( Address64& addr )
    {
        addr = mAddr;
        return S_OK;
    }

    HRESULT CodeContext::GetScope( MagoST::SymHandle& funcSH, MagoST::SymHandle& blockSH, RefPtr<Module>& module )
    {
        FindFunction();
        funcSH = mFuncSH;
        blockSH = mBlockSH.back();
        module = mModule;
        return S_OK;
    }


    //----------------------------------------------------------------------------

    HRESULT CodeContext::MakeCodeContext( 
        Address64 newAddr, 
        Module* mod, 
        IDebugDocumentContext2* docContext, 
        IDebugMemoryContext2** ppMemCxt,
        int ptrSize )
    {
        if ( ppMemCxt == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        RefPtr<CodeContext> newContext;

        hr = MakeCComObject( newContext );
        if ( FAILED( hr ) )
            return hr;

        hr = newContext->Init( newAddr, mod, docContext, ptrSize );
        if ( FAILED( hr ) )
            return hr;

        *ppMemCxt = newContext.Detach();

        return S_OK;
    }

    HRESULT CodeContext::Init( 
        Address64 addr, Module* mod, IDebugDocumentContext2* docContext, int ptrSize )
    {
        if ( ptrSize < sizeof( UINT64 ) )
            addr &= 0xFFFFFFFF;

        mAddr = addr;
        mModule = mod;
        mDocContext = docContext;
        mPtrSize = ptrSize;
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
        uint32_t    symOff = 0;

        // TODO: verify that it's a function or public symbol (or something else?)
        hr = session->FindGlobalSymbolByAddr( mAddr, false, symHandle, sec, offset, symOff );
        if ( FAILED( hr ) )
            return hr;

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

    bool CodeContext::GetAddressOffset( uint32_t& offset )
    {
        HRESULT hr = S_OK;
        MagoST::SymInfoData         infoData = { 0 };
        MagoST::ISymbolInfo*        symInfo = NULL;
        RefPtr<MagoST::ISession>    session;
        SymString                   pstrName;
        CComBSTR                    bstrName;

        hr = FindFunction();
        if ( FAILED( hr ) )
            return false;

        if ( !mModule->GetSymbolSession( session ) )
            return false;

        hr = session->GetSymbolInfo( mFuncSH, infoData, symInfo );
        if ( FAILED( hr ) )
            return false;

        uint32_t off;
        uint16_t seg;
        if( !symInfo->GetAddressOffset( off ) )
            return false;
        if( !symInfo->GetAddressSegment( seg ) )
            return false;
        uint64_t addr = session->GetVAFromSecOffset( seg, off );
        if ( addr == 0 )
            return false;

        offset = (uint32_t) ( mAddr - addr );
        return true;
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
