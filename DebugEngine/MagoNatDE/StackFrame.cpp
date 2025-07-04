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
#include "MagoCVConst.h"
#include "../../DebugEngine/MagoNatDE/EnumFrameInfo.h"

#include <memory>


namespace Mago
{
    StackFrame::StackFrame()
        :   mPC( 0 ),
            mPtrSize( 0 )
    {
        memset( &mFuncSH, 0, sizeof mFuncSH );
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

        hr = codeContext->Init( mPC, mModule, docContext, mPtrSize );
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
        return GetInfoAsync( dwFieldSpec, nRadix, pFrameInfo, {} );
    }

    HRESULT StackFrame::GetInfoAsync(
       FRAMEINFO_FLAGS dwFieldSpec,
       UINT            nRadix,
       FRAMEINFO*      pFrameInfo,
       std::function<HRESULT(HRESULT hr, const FRAMEINFO& frameInfo)> complete )
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
            hr = GetLanguageName( &pFrameInfo->m_bstrLanguage );
            if ( SUCCEEDED( hr ) )
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

        if ( (dwFieldSpec & FIF_FUNCNAME) != 0 )
        {
            std::function<HRESULT(HRESULT hr, const std::wstring&)> funcComplete;
            if ( complete )
            {
                FrameInfo frameInfo;
                Mago::_CopyFrameInfo::copy(&frameInfo, pFrameInfo);
                funcComplete = [frameInfo , complete] (HRESULT hr, const std::wstring& funcName) mutable
                {
                    if (SUCCEEDED(hr))
                    {
                        frameInfo.m_dwValidFields |= FIF_FUNCNAME;
                        frameInfo.m_bstrFuncName = SysAllocString( funcName.c_str() );
                    }
                    hr = complete(hr, frameInfo);
                    return hr;
                };
            }

            hr = GetFunctionName( dwFieldSpec, nRadix, &pFrameInfo->m_bstrFuncName, funcComplete );
            if ( hr == S_OK )
                pFrameInfo->m_dwValidFields |= FIF_FUNCNAME;
        }
        else if( complete )
            return complete( hr, *pFrameInfo );

        //FIF_STALECODE m_fStaleCode
        //FIF_ANNOTATEDFRAME m_fAnnotatedFrame

        return hr;
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
        Log::LogMessage( "StackFrame::GetExpressionContext\n" );

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
        Log::LogMessage( "StackFrame::GetDebugProperty\n" );

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
        Log::LogMessage( "StackFrame::EnumProperties\n" );

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
        Address64 pc,
        IRegisterSet* regSet,
        Thread* thread,
        Module* module,
        int ptrSize,
        ExprContext* exprContext )
    {
        _ASSERT( regSet != NULL );
        _ASSERT( thread != NULL );
        // there might be no module
        _ASSERT( ptrSize == 4 || ptrSize == 8 );

        mPC = pc;
        mRegSet = regSet;
        mThread = thread;
        mModule = module;
        mPtrSize = ptrSize;
        mExprContext = exprContext;
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

        info.Address = (Address64) session->GetVAFromSecOffset( line.Section, line.Offset );
        if( !info.Address )
            return E_FAIL;

        MagoST::FileInfo    fileInfo = { 0 };
        hr = session->GetFileInfo( line.CompilandIndex, line.FileIndex, fileInfo );
        if ( FAILED( hr ) )
            return hr;

        hr = Utf8To16( fileInfo.Name.ptr, fileInfo.Name.length, info.Filename.m_str );
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
        BSTR* funcName,
        std::function<HRESULT(HRESULT hr, const std::wstring&)> complete )
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

        hr = AppendFunctionNameWithSymbols( flags, radix, fullName, complete );
        if ( FAILED( hr ) )
        {
            hr = AppendFunctionNameWithAddress( flags, radix, fullName );
            if ( FAILED( hr ) )
                return hr;
        }

        if( hr == S_OK )
            *funcName = fullName.AllocSysString();

        return hr;
    }

    HRESULT StackFrame::AppendFunctionNameWithAddress( 
        FRAMEINFO_FLAGS flags, 
        UINT radix, 
        CString& fullName )
    {
        wchar_t addrStr[MaxAddrStringLength + 1] = L"";

        FormatAddress( addrStr, _countof( addrStr ), mPC, mPtrSize, false );

        fullName.Append( addrStr );

        return S_OK;
    }

    HRESULT StackFrame::AppendFunctionNameWithSymbols( 
        FRAMEINFO_FLAGS flags, 
        UINT radix, 
        CString& fullName,
        std::function<HRESULT(HRESULT hr, const std::wstring&)> complete )
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

        hr = AppendFunctionNameWithSymbols( flags, radix, session, symInfo, fullName, complete );

        // for reference: you get the language by going to a lexical ancestor
        // that's a compiland, then you get a compiland detail from it

        return hr;
    }

    HRESULT StackFrame::AppendFunctionNameWithSymbols( 
        FRAMEINFO_FLAGS flags, 
        UINT radix, 
        MagoST::ISession* session,
        MagoST::ISymbolInfo* symInfo, 
        CString& fullName,
        std::function<HRESULT(HRESULT hr, const std::wstring&)> complete )
    {
        _ASSERT( session != NULL );
        _ASSERT( symInfo != NULL );
        HRESULT hr = S_OK;
        CComBSTR funcNameBstr;

        SymString   pstrName;
        if ( !symInfo->GetName( pstrName ) )
            return E_NOT_FOUND;

        std::string shortName;
        if( MagoEE::gShortenTypeNames &&
            session->FindFuncShortName( pstrName.GetName(), pstrName.GetLength(), shortName ) == S_OK )
            hr = Utf8To16( shortName.data(), shortName.size(), funcNameBstr.m_str );
        else
            hr = Utf8To16( pstrName.GetName(), pstrName.GetLength(), funcNameBstr.m_str );
        if ( FAILED( hr ) )
            return hr;

        fullName.Append( funcNameBstr );
        fullName.AppendChar( L'(' );

        CString tailName;
        tailName.AppendChar( L')' );

        bool hasLineInfo = false;
        Address64 baseAddr = mPC;

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

                tailName.AppendFormat( L" %s %u", lineStr, lineNumber );
            }
        }

        if ( !hasLineInfo )
        {
            uint16_t sec = 0;
            uint32_t offset = 0;

            symInfo->GetAddressSegment( sec );
            symInfo->GetAddressOffset( offset );
            baseAddr = (Address64) session->GetVAFromSecOffset( sec, offset );
            if ( baseAddr == 0 )
                return E_FAIL;
        }

        if ( ((flags & FIF_FUNCNAME_OFFSET) != 0) && (mPC != baseAddr) )
        {
            const wchar_t* bytesStr = GetString( IDS_BYTES );
            tailName.AppendFormat( L" + 0x%x %s", (uint32_t) (mPC - baseAddr), bytesStr );
        }

        if ((flags & FIF_FUNCNAME_ARGS_ALL) != 0)
        {
            hr = AppendArgs(flags, radix, session, symInfo, fullName, tailName, complete);
        }

        return hr;
    }

    HRESULT StackFrame::AppendArgs(
        FRAMEINFO_FLAGS flags, 
        UINT radix, 
        MagoST::ISession* session,
        MagoST::ISymbolInfo* symInfo, 
        CString& outputStr,
        CString& tailStr,
        std::function<HRESULT(HRESULT hr, const std::wstring&)> complete )
    {
        _ASSERT( session != NULL );
        _ASSERT( symInfo != NULL );
        HRESULT             hr = S_OK;
        MagoST::SymbolScope funcScope = { 0 };
        MagoST::SymHandle   childSH = { 0 };

        hr = MakeExprContext();
        if ( FAILED( hr ) )
            return hr;

        hr = session->SetChildSymbolScope( mFuncSH, funcScope );
        if ( FAILED( hr ) )
            return hr;

        struct Param
        {
            std::wstring type;
            std::wstring name;
            std::wstring value;
        };
        struct Closure
        {
            std::vector<Param> params;
            std::wstring headStr;
            std::wstring tailStr;
            HRESULT hrCombined = S_OK;
            size_t toComplete = 1; // keep it above 0 until all requests queued
            std::function<HRESULT(HRESULT hr, const std::wstring&)> complete;

            HRESULT done(HRESULT hr, size_t cnt)
            {
                hrCombine(hr);
                toComplete -= cnt;
                if (toComplete == 0 && complete)
                    return complete(hrCombined, buildFuncName());
                return hr;
            }
            HRESULT hrCombine(HRESULT hr)
            {
                if (hr != S_QUEUED && hr != S_OK && hr != E_OPERATIONCANCELED)
                    hr = hr;
                if (hrCombined != S_QUEUED && hrCombined != E_OPERATIONCANCELED)
                    if (hr == S_QUEUED || hr == E_OPERATIONCANCELED)
                        hrCombined = hr; // do not propagate errors on arguments
                return hrCombined;
            }

            std::wstring buildFuncName()
            {
                std::wstring str;
                for ( auto& p : params )
                {
                    if ( !str.empty() )
                        str.append( L", " );
                    str.append( p.type );
                    if (!p.type.empty() && !p.name.empty())
                        str.append(L" ");
                    str.append(p.name);
                    if ((!p.type.empty() || !p.name.empty()) && !p.value.empty())
                        str.append(L" = ");
                    str.append(p.value);
                }
                return headStr + str + tailStr;
            }
        };
        auto closure = std::make_shared<Closure>();
        closure->headStr = outputStr;
        closure->tailStr = tailStr;
        closure->complete = complete;

        bool showTypes = (flags & FIF_FUNCNAME_ARGS_TYPES) != 0;
        bool showNames = (flags & FIF_FUNCNAME_ARGS_NAMES) != 0;
        bool showValues = (flags & FIF_FUNCNAME_ARGS_VALUES) != 0;
        while ( session->NextSymbol( funcScope, childSH, ~0u ) )
        {
            MagoST::SymInfoData     childData = { 0 };
            MagoST::ISymbolInfo*    childSym = NULL;
            MagoST::SymTag          tag = MagoST::SymTagNull;
            MagoST::DataKind        kind = MagoST::DataIsUnknown;
            RefPtr<MagoEE::Type>    type;
            RefPtr<MagoEE::Declaration> decl;

            session->GetSymbolInfo( childSH, childData, childSym );
            if ( childSym == NULL )
                continue;

            tag = childSym->GetSymTag();
            if ( tag == MagoST::SymTagEndOfArgs )
                break;

            bool tryThis = tag == MagoST::SymTagData && closure->params.empty() && showValues;
            bool isParam = childSym->GetDataKind(kind) && kind == MagoST::DataIsParam;
            if ( !tryThis && !isParam )
                continue;

            mExprContext->GetModuleContext()->MakeDeclarationFromSymbol( childSH, decl.Ref() );
            if ( decl == NULL )
                continue;

            if ( gOptions.hideInternalNames )
            {
                auto name = decl->GetName();
                if ( name && name[0] == '_' && name[1] == '_' && MagoEE::GetParamIndex( name ) < 0 )
                    // do not hide ... parameter symbols
                    continue;
            }

            std::wstring typeStr;
            std::wstring nameStr;

            if ( showTypes )
            {
                if ( decl->GetType( type.Ref() ) )
                    type->ToString( typeStr );
            }

            if ( showNames || tryThis )
            {
                nameStr = decl->GetName();
                if ( tryThis && !isParam )
                {
                    if ( nameStr == L"this" )
                        typeStr.clear(); // type already in the name
                    else
                        continue;
                }
                if (!showNames)
                    nameStr.clear();
            }

            closure->params.push_back({ typeStr, nameStr });

            if ( showValues )
            {
                MagoEE::DataObject resultObj = { 0 };

                hr = mExprContext->Evaluate( decl, resultObj );
                if ( hr == S_OK )
                {
                    closure->toComplete++;
                    MagoEE::FormatOptions fmtopts( radix );
                    fmtopts.stack = true;
                    MagoEE::FormatData fmtdata( fmtopts, 32, 2 );
                    auto completeValue = !complete ? std::function<HRESULT( HRESULT, std::wstring )>{} :
                        [closure, idx = closure->params.size() - 1]( HRESULT hr, std::wstring valStr )
                        {
                            if (hr != S_OK && hr != E_OPERATIONCANCELED)
                                hr = MagoEE::GetErrorString( hr, valStr );

                            closure->params[idx].value = valStr;
                            return closure->done(hr, 1);
                        };
                    hr = MagoEE::FormatValue( mExprContext, resultObj, fmtdata, completeValue );
                    if ( hr != S_QUEUED && !completeValue )
                    {
                        if ( hr == S_OK )
                            closure->params.back().value = fmtdata.outStr;
                        closure->done( hr, 1 );
                    }
                    if ( hr == E_OPERATIONCANCELED )
                        break;
                }
                else
                    MagoEE::GetErrorString( hr, closure->params.back().value );
            }
        }
        session->EndSymbolScope( funcScope );

        closure->done( S_OK, 1 );
        if( !complete )
            outputStr = closure->buildFuncName().c_str();
        return closure->toComplete > 0 ? S_QUEUED : closure->hrCombined;
    }

    HRESULT StackFrame::GetLanguageName( BSTR* langName )
    {
        _ASSERT( langName != NULL );
        HRESULT hr = S_OK;

        hr = FindFunction();
        if ( FAILED( hr ) )
            return hr;

        // if a function was found for our address, then it has a language
        // we'll assume it's D for now
        *langName = SysAllocString( L"D" );
        if ( *langName == NULL )
            return E_OUTOFMEMORY;

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

        if ( mExprContext && mExprContext->GetPC() + mModule->GetAddress() == mPC )
        {
            mFuncSH = mExprContext->GetFunctionSH();
            if ( memcmp( &mFuncSH, &symHandle, sizeof mFuncSH ) != 0 )
                return S_OK;
        }

        if ( !mModule->GetSymbolSession( session ) )
            return E_NOT_FOUND;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        uint32_t    symOff = 0;

        // TODO: verify that it's a function or public symbol (or something else?)
        hr = session->FindGlobalSymbolByAddr( mPC, false, symHandle, sec, offset, symOff );
        if ( FAILED( hr ) )
            return hr;

        mFuncSH = symHandle;

        hr = session->FindInnermostSymbol( mFuncSH, sec, offset, mBlockSH );
        // it might be a public symbol, which doesn't have anything: blocks, args, or locals

        return S_OK;
    }
}
