/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "StackFrame.h"
#include "Thread.h"
#include "Module.h"
#include "SingleDocumentContext.h"
#include "CodeContext.h"
#include "ExprContext.h"
#include "FrameProperty.h"
#include "RegisterSet.h"


namespace Mago
{
    StackFrame::StackFrame()
        :   mPC( 0 )
    {
        memset( &mFuncSH, 0, sizeof mFuncSH );
        memset( &mBlockSH, 0, sizeof mBlockSH );
    }

    StackFrame::~StackFrame()
    {
    }

    HRESULT StackFrame::GetCodeContext( 
       IDebugCodeContext2** ppCodeCxt )
    {
        if ( ppCodeCxt == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        RefPtr<CodeContext> codeContext;

        hr = MakeCComObject( codeContext );
        if ( FAILED( hr ) )
            return hr;

        CComPtr<IDebugDocumentContext2> docContext;
        hr = GetDocumentContext( &docContext );
        // there doesn't have to be a document context

        hr = codeContext->Init( mPC, mModule, docContext );
        if ( FAILED( hr ) )
            return hr;

        return codeContext->QueryInterface( __uuidof( IDebugCodeContext2 ), (void**) ppCodeCxt );
    }

    HRESULT StackFrame::GetDocumentContext( 
       IDebugDocumentContext2** ppCxt )
    {
        if ( ppCxt == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        RefPtr<SingleDocumentContext>   docContext;
        LineInfo    line;

        hr = MakeCComObject( docContext );
        if ( FAILED( hr ) )
            return hr;

        hr = GetLineInfo( line );
        if ( FAILED( hr ) )
            return hr;

        hr = docContext->Init( line.Filename, line.LineBegin, line.LineEnd, line.LangName, line.LangGuid );
        if ( FAILED( hr ) )
            return hr;

        return docContext->QueryInterface( __uuidof( IDebugDocumentContext2 ), (void**) ppCxt );
    }

    HRESULT StackFrame::GetName( 
       BSTR* pbstrName )
    {
        return E_NOTIMPL;
    }

    HRESULT StackFrame::GetInfo( 
       FRAMEINFO_FLAGS dwFieldSpec,
       UINT            nRadix,
       FRAMEINFO*      pFrameInfo )
    {
        if ( pFrameInfo == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        pFrameInfo->m_dwValidFields = 0;

        if ( (dwFieldSpec & FIF_FRAME) != 0 )
        {
            hr = QueryInterface( __uuidof( IDebugStackFrame2 ), (void**) &pFrameInfo->m_pFrame );
            _ASSERT( hr == S_OK );
            pFrameInfo->m_dwValidFields |= FIF_FRAME;
        }

        if ( (dwFieldSpec & FIF_DEBUG_MODULEP) != 0 )
        {
            if ( mModule.Get() != NULL )
            {
                hr = mModule->QueryInterface( __uuidof( IDebugModule2 ), (void**) &pFrameInfo->m_pModule );
                _ASSERT( hr == S_OK );
                pFrameInfo->m_dwValidFields |= FIF_DEBUG_MODULEP;
            }
        }

        if ( (dwFieldSpec & FIF_STACKRANGE) != 0 )
        {
#if 0
            if ( mDiaStackFrame != NULL )
            {
                // TODO: review all this
                UINT64  base = 0;
                DWORD   size = 0;

                mDiaStackFrame->get_base( &base );
                mDiaStackFrame->get_size( &size );

                pFrameInfo->m_addrMax = base;
                pFrameInfo->m_addrMin = base - size;
                pFrameInfo->m_dwValidFields |= FIF_STACKRANGE;
            }
#endif
        }

        if ( (dwFieldSpec & FIF_DEBUGINFO) != 0 )
        {
            if ( mModule.Get() != NULL )
            {
                RefPtr<MagoST::ISession>    session;

                pFrameInfo->m_fHasDebugInfo = mModule->GetSymbolSession( session );
            }
            else
                pFrameInfo->m_fHasDebugInfo = FALSE;

            pFrameInfo->m_dwValidFields |= FIF_DEBUGINFO;
        }

        if ( (dwFieldSpec & FIF_FUNCNAME) != 0 )
        {
            hr = GetFunctionName( dwFieldSpec, nRadix, &pFrameInfo->m_bstrFuncName );
            if ( SUCCEEDED( hr ) )
                pFrameInfo->m_dwValidFields |= FIF_FUNCNAME;
        }

        if ( (dwFieldSpec & FIF_RETURNTYPE) != 0 )
        {
            //pFrameInfo->m_bstrReturnType;
            //pFrameInfo->m_dwValidFields |= FIF_RETURNTYPE;
        }

        if ( (dwFieldSpec & FIF_ARGS) != 0 )
        {
            //pFrameInfo->m_bstrArgs;
            //pFrameInfo->m_dwValidFields |= FIF_ARGS;
        }

        if ( (dwFieldSpec & FIF_LANGUAGE) != 0 )
        {
            pFrameInfo->m_bstrLanguage = SysAllocString( L"D" );
            pFrameInfo->m_dwValidFields |= FIF_LANGUAGE;
        }

        if ( (dwFieldSpec & FIF_MODULE) != 0 )
        {
            if ( mModule != NULL )
            {
                CComBSTR modName;
                mModule->GetName( modName );
                pFrameInfo->m_bstrModule = modName.Detach();
                if ( pFrameInfo->m_bstrModule != NULL )
                    pFrameInfo->m_dwValidFields |= FIF_MODULE;
            }
        }

        //FIF_STALECODE m_fStaleCode
        //FIF_ANNOTATEDFRAME m_fAnnotatedFrame

        return S_OK;
    }

    HRESULT StackFrame::GetPhysicalStackRange( 
       UINT64* paddrMin,
       UINT64* paddrMax )
    {
        return E_NOTIMPL;
    }

    HRESULT StackFrame::GetExpressionContext( 
       IDebugExpressionContext2** ppExprCxt )
    {
        OutputDebugStringA( "StackFrame::GetExpressionContext\n" );

        HRESULT hr = S_OK;

        if ( mModule == NULL )
            return E_NOT_FOUND;

        hr = MakeExprContext();
        if ( FAILED( hr ) )
            return hr;

        *ppExprCxt = mExprContext;
        (*ppExprCxt)->AddRef();
        return S_OK;
    }

    HRESULT StackFrame::MakeExprContext()
    {
        HRESULT hr = S_OK;

        // ignore any error, because we don't have to have symbols for expression eval
        hr = FindFunction();

        if ( mExprContext == NULL )
        {
            GuardedArea guard( mExprContextGuard );

            if ( mExprContext == NULL )
            {
                RefPtr<ExprContext>         exprContext;

                hr = MakeCComObject( exprContext );
                if ( FAILED( hr ) )
                    return hr;

                hr = exprContext->Init( mModule, mThread, mFuncSH, mBlockSH, mPC, mRegSet );
                if ( FAILED( hr ) )
                    return hr;

                mExprContext = exprContext;
            }
        }

        return S_OK;
    }

    HRESULT StackFrame::GetLanguageInfo( 
       BSTR* pbstrLanguage,
       GUID* pguidLanguage )
    {
        return E_NOTIMPL;
    }

    HRESULT StackFrame::GetDebugProperty( 
       IDebugProperty2** ppDebugProp )
    {
        OutputDebugStringA( "StackFrame::GetDebugProperty\n" );

        if ( ppDebugProp == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        RefPtr<FrameProperty>   frameProp;

        hr = MakeExprContext();
        if ( FAILED( hr ) )
            return hr;

        hr = MakeCComObject( frameProp );
        if ( FAILED( hr ) )
            return hr;

        hr = frameProp->Init( mRegSet, mExprContext );
        if ( FAILED( hr ) )
            return hr;

        *ppDebugProp = frameProp.Detach();
        return S_OK;
    }

    HRESULT StackFrame::EnumProperties( 
       DEBUGPROP_INFO_FLAGS      dwFieldSpec,
       UINT                      nRadix,
       REFIID                    refiid,
       DWORD                     dwTimeout,
       ULONG*                    pcelt,
       IEnumDebugPropertyInfo2** ppEnum )
    {
        OutputDebugStringA( "StackFrame::EnumProperties\n" );

        HRESULT hr = S_OK;
        CComPtr<IDebugProperty2>    rootProp;

        hr = GetDebugProperty( &rootProp );
        if ( FAILED( hr ) )
            return hr;

        hr = rootProp->EnumChildren(
            dwFieldSpec,
            nRadix,
            refiid,
            DBG_ATTRIB_ALL,
            NULL,
            dwTimeout,
            ppEnum );
        if ( FAILED( hr ) )
            return hr;

        if ( pcelt != NULL )
        {
            hr = (*ppEnum)->GetCount( pcelt );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }

    HRESULT StackFrame::GetThread( 
       IDebugThread2** ppThread )
    {
        if ( ppThread == NULL )
            return E_INVALIDARG;

        return mThread->QueryInterface( __uuidof( IDebugThread2 ), (void**) ppThread );
    }


    //----------------------------------------------------------------------------

    void StackFrame::Init(
        Address pc,
        IRegisterSet* regSet,
        Thread* thread,
        Module* module )
    {
        _ASSERT( regSet != NULL );
        _ASSERT( thread != NULL );
        // there might be no module

        mPC = pc;
        mRegSet = regSet;
        mThread = thread;
        mModule = module;
    }

    HRESULT StackFrame::GetLineInfo( LineInfo& info )
    {
        HRESULT hr = S_OK;
        RefPtr<MagoST::ISession>    session;

        if ( mModule.Get() == NULL )
            return E_NOT_FOUND;

        if ( !mModule->GetSymbolSession( session ) )
            return E_NOT_FOUND;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        sec = session->GetSecOffsetFromVA( mPC, offset );
        if ( sec == 0 )
            return E_FAIL;

        MagoST::LineNumber line = { 0 };
        if ( !session->FindLine( sec, offset, line ) )
            return E_FAIL;

        info.LineBegin.dwLine = line.Number;
        //info.LineEnd.dwLine = line.NumberEnd;
        info.LineEnd.dwLine = line.Number;
        info.LineBegin.dwColumn = 0;
        info.LineEnd.dwColumn = 0;

        info.LineBegin.dwLine--;
        info.LineEnd.dwLine--;

        // if there's no column info, then these are 0
        if ( info.LineBegin.dwColumn > 0 )
            info.LineBegin.dwColumn--;
        if ( info.LineEnd.dwColumn > 0 )
            info.LineEnd.dwColumn--;

        info.Address = (Address) session->GetVAFromSecOffset( line.Section, line.Offset );

        MagoST::FileInfo    fileInfo = { 0 };
        hr = session->GetFileInfo( line.CompilandIndex, line.FileIndex, fileInfo );
        if ( FAILED( hr ) )
            return hr;

        hr = Utf8To16( fileInfo.Name, fileInfo.NameLength, info.Filename.m_str );
        if ( FAILED( hr ) )
            return hr;

        // TODO: from the compiland we're supposed to get the language
        info.LangName = L"D";
        info.LangGuid = GetDLanguageId();

        return hr;
    }

    HRESULT StackFrame::GetFunctionName( 
        FRAMEINFO_FLAGS flags, 
        UINT radix, 
        BSTR* funcName )
    {
        _ASSERT( funcName != NULL );
        HRESULT hr = S_OK;
        CString fullName;

        if ( (flags & FIF_FUNCNAME_MODULE) != 0 )
        {
            if ( mModule != NULL )
            {
                CComBSTR modNameBstr;
                mModule->GetName( modNameBstr );
                fullName.Append( modNameBstr );
                fullName.AppendChar( L'!' );
            }
        }

        hr = AppendFunctionNameWithSymbols( flags, radix, fullName );
        if ( FAILED( hr ) )
        {
            hr = AppendFunctionNameWithAddress( flags, radix, fullName );
            if ( FAILED( hr ) )
                return hr;
        }

        *funcName = fullName.AllocSysString();

        return hr;
    }

    HRESULT StackFrame::AppendFunctionNameWithAddress( 
        FRAMEINFO_FLAGS flags, 
        UINT radix, 
        CString& fullName )
    {
        C_ASSERT( sizeof mPC == 4 );
        fullName.AppendFormat( L"%08x", mPC );

        return S_OK;
    }

    HRESULT StackFrame::AppendFunctionNameWithSymbols( 
        FRAMEINFO_FLAGS flags, 
        UINT radix, 
        CString& fullName )
    {
        HRESULT hr = S_OK;
        RefPtr<MagoST::ISession>    session;
        MagoST::SymInfoData         infoData = { 0 };
        MagoST::ISymbolInfo*        symInfo = NULL;

        if ( mModule.Get() == NULL )
            return E_NOT_FOUND;

        if ( !mModule->GetSymbolSession( session ) )
            return E_NOT_FOUND;

        hr = FindFunction();
        if ( FAILED( hr ) )
            return hr;

        hr = session->GetSymbolInfo( mFuncSH, infoData, symInfo );
        if ( FAILED( hr ) )
            return hr;

        hr = AppendFunctionNameWithSymbols( flags, radix, session, symInfo, fullName );

        // for reference: you get the language by going to a lexical ancestor
        // that's a compiland, then you get a compiland detail from it

        return hr;
    }

    HRESULT StackFrame::AppendFunctionNameWithSymbols( 
        FRAMEINFO_FLAGS flags, 
        UINT radix, 
        MagoST::ISession* session,
        MagoST::ISymbolInfo* symInfo, 
        CString& fullName )
    {
        _ASSERT( session != NULL );
        _ASSERT( symInfo != NULL );
        HRESULT hr = S_OK;
        CComBSTR funcNameBstr;

        PasString*  pstrName = NULL;
        if ( !symInfo->GetName( pstrName ) )
            return E_NOT_FOUND;

        hr = Utf8To16( pstrName->GetName(), pstrName->GetLength(), funcNameBstr.m_str );
        if ( FAILED( hr ) )
            return hr;

        fullName.Append( funcNameBstr );
        fullName.AppendChar( L'(' );
        fullName.AppendChar( L')' );

        bool hasLineInfo = false;
        Address baseAddr = 0;

        if ( (flags & FIF_FUNCNAME_LINES) != 0 )
        {
            LineInfo line;
            if ( SUCCEEDED( GetLineInfo( line ) ) )
            {
                hasLineInfo = true;
                baseAddr = line.Address;
                // lines are 1-based to user, but 0-based from symbol store
                DWORD lineNumber = line.LineBegin.dwLine + 1;
                const wchar_t* lineStr = GetString( IDS_LINE );

                fullName.AppendFormat( L" %s %u", lineStr, lineNumber );
            }
        }

        if ( !hasLineInfo )
        {
            uint16_t sec = 0;
            uint32_t offset = 0;

            symInfo->GetAddressSegment( sec );
            symInfo->GetAddressOffset( offset );
            baseAddr = (Address) session->GetVAFromSecOffset( sec, offset );
        }

        if ( ((flags & FIF_FUNCNAME_OFFSET) != 0) && (mPC != baseAddr) )
        {
            const wchar_t* bytesStr = GetString( IDS_BYTES );
            fullName.AppendFormat( L" + 0x%x %s", mPC - baseAddr, bytesStr );
        }

        return S_OK;
    }

    HRESULT StackFrame::FindFunction()
    {
        HRESULT hr = S_OK;
        RefPtr<MagoST::ISession>    session;
        MagoST::SymHandle           symHandle = { 0 };

        // already found
        if ( memcmp( &mFuncSH, &symHandle, sizeof mFuncSH ) != 0 )
            return S_OK;

        if ( mModule.Get() == NULL )
            return E_NOT_FOUND;

        if ( !mModule->GetSymbolSession( session ) )
            return E_NOT_FOUND;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        sec = session->GetSecOffsetFromVA( mPC, offset );
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
}
