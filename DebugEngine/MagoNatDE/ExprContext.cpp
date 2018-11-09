/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ExprContext.h"
#include "Expr.h"
#include "Module.h"
#include "Thread.h"
#include "CVDecls.h"
#include "IDebuggerProxy.h"
#include "RegisterSet.h"
#include "winternl2.h"
#include "ArchDataX86.h"
#include "DRuntime.h"
#include "Program.h"
#include "ICoreProcess.h"
#include <MagoCVConst.h>

#include <algorithm>

using namespace MagoST;

// use references instead of pointers to classes
#define USE_REFERENCE_TYPE  1


// from DMD source
#define DMD_OEM 0x42    // Digital Mars OEM number (picked at random)
#define DMD_OEM_DARRAY      1
#define DMD_OEM_AARRAY      2
#define DMD_OEM_DELEGATE    3


namespace Mago
{
    // ExprContext

    ExprContext::ExprContext()
        :   mPC( 0 )
    {
        memset( &mFuncSH, 0, sizeof mFuncSH );
    }

    ExprContext::~ExprContext()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugExpressionContext2 

    HRESULT ExprContext::GetName( BSTR* pbstrName ) 
    {
        return E_NOTIMPL;
    }

    HRESULT ExprContext::ParseText( 
            LPCOLESTR pszCode,
            PARSEFLAGS dwFlags,
            UINT nRadix,
            IDebugExpression2** ppExpr,
            BSTR* pbstrError,
            UINT* pichError )
    {
        Log::LogMessage( "ExprContext::ParseText\n" );

        HRESULT hr = S_OK;
        RefPtr<Expr>    expr;
        RefPtr<MagoEE::IEEDParsedExpr>  parsedExpr;

        hr = MakeCComObject( expr );
        if ( FAILED( hr ) )
            return hr;

        hr = MagoEE::ParseText( pszCode, GetTypeEnv(), GetStringTable(), parsedExpr.Ref() );
        if ( FAILED( hr ) )
        {
            MagoEE::EED::GetErrorString( hr, *pbstrError );
            return hr;
        }

        MagoEE::EvalOptions options = MagoEE::EvalOptions::defaults;
        options.Radix = (uint8_t) nRadix;

        hr = parsedExpr->Bind( options, this );
        if ( FAILED( hr ) )
        {
            MagoEE::EED::GetErrorString( hr, *pbstrError );
            return hr;
        }

        hr = expr->Init( parsedExpr, pszCode, this );
        if ( FAILED( hr ) )
            return hr;

        *ppExpr = expr.Detach();
        return S_OK;
    }

    HRESULT ExprContext::Evaluate( 
        MagoEE::Declaration* decl, 
        MagoEE::DataObject& resultObj )
    {
        HRESULT hr = S_OK;

        hr = GetValue( decl, resultObj.Value );
        if ( FAILED( hr ) )
            return hr;

        decl->GetType( resultObj._Type.Ref() );

        decl->GetAddress( resultObj.Addr );

        return S_OK;
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // MagoEE::IValueBinder

    HRESULT ExprContext::FindObject( 
        const wchar_t* name, 
        MagoEE::Declaration*& decl,
		uint32_t findFlags )
    {
        HRESULT hr = S_OK;
        CAutoVectorPtr<char>        u8Name;
        size_t                      u8NameLen = 0;
        MagoST::SymHandle           symHandle = { 0 };
        RefPtr<MagoST::ISession>    session;
        RefPtr<MagoEE::Declaration> origDecl;
        MagoEE::UdtKind             udtKind = MagoEE::Udt_Struct;

        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        hr = Utf16To8( name, wcslen( name ), u8Name.m_p, u8NameLen );
        if ( FAILED( hr ) )
            return hr;

        hr = E_NOT_FOUND;
        if( findFlags & FindObjectLocal )
            hr = FindLocalSymbol( u8Name, u8NameLen, symHandle );
        
        if ( hr != S_OK && ( findFlags & FindObjectClosure ) != 0 )
        {
            hr = FindClosureSymbol( u8Name, u8NameLen, decl );
            if ( hr == S_OK )
                return hr;
        }
        
        if ( hr != S_OK && ( findFlags & FindObjectGlobal ) != 0 )
            hr = FindGlobalSymbol( u8Name, u8NameLen, symHandle );

        if ( hr != S_OK && ( findFlags & FindObjectRegister ) != 0 )
        {
            decl = RegisterCVDecl::CreateRegisterSymbol( this, u8Name );
            if( decl )
            {
                decl->AddRef();
                return S_OK;
            }
            return E_NOT_FOUND;
        }

        hr = MakeDeclarationFromSymbol( symHandle, origDecl.Ref() );
        if ( FAILED( hr ) )
            return hr;

#if USE_REFERENCE_TYPE
        if ( ( findFlags & FindObjectNoClassDeref ) == 0 && origDecl->IsType()
            && origDecl->GetUdtKind( udtKind ) && ( udtKind == MagoEE::Udt_Class ) )
        {
            decl = new ClassRefDecl( origDecl, mTypeEnv );
            if ( decl == NULL )
                return E_OUTOFMEMORY;

            decl->AddRef();
        }
        else
        {
            decl = origDecl.Detach();
        }
#else
        UNREFERENCED_PARAMETER( udtKind );

        decl = origDecl.Detach();
#endif

        return S_OK;
    }

    HRESULT ExprContext::FindObjectType( 
        MagoEE::Declaration* decl,
        const wchar_t* name, 
        MagoEE::Type*& type )
    {
        MagoEE::Declaration* keydecl = NULL;
        HRESULT hr = decl->FindObject( name, keydecl );
        if( FAILED( hr ) )
            return hr;

        MagoEE::Type* keytype = NULL;
        hr = E_INVALIDARG;
        if( keydecl != NULL && keydecl->GetType( keytype ) && keytype != NULL )
        {
            type = keytype->Unaliased();
            if( type != keytype )
            {
                type->AddRef();
                keytype->Release();
            }
            hr = S_OK;
        }
        if( keydecl )
            keydecl->Release();
        return hr;
    }

    HRESULT ExprContext::GetThis( MagoEE::Declaration*& decl )
    {
        return FindObject( L"this", decl, FindObjectLocal | FindObjectClosure | FindObjectNoClassDeref );
    }

    HRESULT ExprContext::GetSuper( MagoEE::Declaration*& decl )
    {
        return E_NOTIMPL;
    }

    HRESULT ExprContext::GetReturnType( MagoEE::Type*& type )
    {
        return E_NOTIMPL;
    }

    HRESULT ExprContext::GetAddress( MagoEE::Declaration* decl, MagoEE::Address& addr )
    {
        if ( decl == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        CVDecl*                     cvDecl = (CVDecl*) decl;
        MagoST::ISymbolInfo*        sym = cvDecl->GetSymbol();
        MagoST::LocationType        loc = MagoST::LocIsNull;
        MagoEE::DataValueKind       valKind = MagoEE::DataValueKind_None;
        MagoEE::DataValue           dataVal = { 0 };
        RefPtr<MagoST::ISession>    session;

        if ( !mModule->GetSymbolSession( session ) )
            return E_NOT_FOUND;

        if ( !sym->GetLocation( loc ) )
            return E_FAIL;

        switch ( loc )
        {
        case LocIsRegRel:
            {
                uint32_t    reg = 0;
                int32_t     offset = 0;

                if ( !sym->GetRegister( reg ) )
                    return E_FAIL;

                if ( !sym->GetOffset( offset ) )
                    return E_FAIL;

                hr = GetRegValue( reg, valKind, dataVal );
                if ( FAILED( hr ) )
                    return E_FAIL;

                if ( (valKind != MagoEE::DataValueKind_Int64) && (valKind != MagoEE::DataValueKind_UInt64) )
                    return E_FAIL;

                addr = dataVal.UInt64Value + offset;
            }
            break;

        case LocIsEnregistered:
            {
                uint32_t    reg = 0;

                if ( !sym->GetRegister( reg ) )
                    return E_FAIL;

                hr = GetRegValue( reg, valKind, dataVal );
                if ( FAILED( hr ) )
                    return E_FAIL;

                if ( (valKind != MagoEE::DataValueKind_Int64) && (valKind != MagoEE::DataValueKind_UInt64) )
                    return E_FAIL;

                addr = dataVal.UInt64Value;
            }
            break;

        case LocIsStatic:
            {
                uint16_t        sec = 0;
                uint32_t        offset = 0;

                if ( !sym->GetAddressOffset( offset ) )
                    return E_FAIL;
                if ( !sym->GetAddressSegment( sec ) )
                    return E_FAIL;

                addr = session->GetVAFromSecOffset( sec, offset );
                if ( addr == 0 )
                    return E_FAIL;
            }
            break;

        case LocIsTLS:
            {
                uint32_t        offset = 0;
                Address64       tebAddr = 0;
                Address64       tlsArrayPtrAddr = 0;
                Address64       tlsArrayAddr = 0;
                Address64       tlsBufAddr = 0;
                uint32_t        ptrSize = mTypeEnv->GetPointerSize();

                if ( !sym->GetAddressOffset( offset ) )
                    return E_FAIL;

                tebAddr = GetTebBase();

                if ( ptrSize == sizeof( UINT64 ) )
                    tlsArrayPtrAddr = tebAddr + offsetof( TEB64, ThreadLocalStoragePointer );
                else
                    tlsArrayPtrAddr = tebAddr + offsetof( TEB32, ThreadLocalStoragePointer );

                uint32_t    lenRead = 0;

#if !defined( _M_IX86 )
#error Mago doesn't implement a debugger engine for the current architecture.
#endif
                // read the pointer straight into a long address, even if the pointer is short
                hr = ReadMemory( 
                    tlsArrayPtrAddr, 
                    ptrSize, 
                    lenRead, 
                    (uint8_t*) &tlsArrayAddr );
                if ( FAILED( hr ) )
                    return hr;
                if ( lenRead < ptrSize )
                    return E_FAIL;

                if ( (tlsArrayAddr == 0) || (tlsArrayAddr == tlsArrayPtrAddr) )
                {
                    // go to the reserved slots in the TEB
                    if ( ptrSize == sizeof( UINT64 ) )
                        tlsArrayAddr = tebAddr + offsetof( TEB64, TlsSlots );
                    else
                        tlsArrayAddr = tebAddr + offsetof( TEB32, TlsSlots );
                }

                // assuming TLS slot 0
                hr = ReadMemory( 
                    tlsArrayAddr, 
                    ptrSize, 
                    lenRead, 
                    (uint8_t*) &tlsBufAddr );
                if ( FAILED( hr ) )
                    return hr;
                if ( lenRead < ptrSize )
                    return E_FAIL;

                addr = tlsBufAddr + offset;
            }
            break;

        default:
            if( !cvDecl->GetAddress( addr ) )
                return E_FAIL;
            break;
        }

        return S_OK;
    }

    HRESULT ExprContext::GetValue( 
        MagoEE::Declaration* decl, 
        MagoEE::DataValue& value )
    {
        if ( decl == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        CVDecl*                     cvDecl = (CVDecl*) decl;
        MagoST::ISymbolInfo*        sym = cvDecl->GetSymbol();
        MagoST::LocationType        loc = MagoST::LocIsNull;
        MagoEE::DataValueKind       valKind = MagoEE::DataValueKind_None;
        RefPtr<MagoEE::Type>        type;

        if ( !decl->GetType( type.Ref() ) )
            return E_FAIL;

        if ( !sym->GetLocation( loc ) )
            return E_FAIL;

        switch ( loc )
        {
        case LocIsRegRel:
        case LocIsStatic:
        case LocIsTLS:
        case LocIsThisRel: // if accessed by a closure
            {
                MagoEE::Address addr = 0;

                hr = GetAddress( decl, addr );
                if ( FAILED( hr ) )
                    return hr;

                return GetValue( addr, type, value );
            }
            break;

        case LocIsConstant:
            {
                MagoST::Variant var = { 0 };

                if ( !sym->GetValue( var ) )
                    return E_FAIL;

                ConvertVariantToDataVal( var, value );
            }
            break;

        case LocIsEnregistered:
            {
                uint32_t    reg = 0;

                if ( !sym->GetRegister( reg ) )
                    return E_FAIL;

                return GetRegValue( reg, valKind, value );
            }
            break;

        // GetValue is not the right way to get a LocIsThisRel, and LocIsBitField isn't supported
        default:
            return E_FAIL;
        }

        return S_OK;
    }

    HRESULT ExprContext::GetValue( 
        MagoEE::Address addr, 
        MagoEE::Type* type, 
        MagoEE::DataValue& value )
    {
        HRESULT         hr = S_OK;
        uint8_t         targetBuf[ sizeof( MagoEE::DataValue ) ] = { 0 };
        size_t          targetSize = type->GetSize();
        uint32_t        lenRead = 0;

        if ( type->IsSArray() )
        {
            value.Addr = addr;
            return S_OK;
        }
        // no value to get for complex/aggregate types
        if ( !type->IsScalar() 
            && !type->IsDArray() 
            && !type->IsAArray() 
            && !type->IsDelegate() )
            return S_OK;

        _ASSERT( targetSize <= sizeof targetBuf );
        if ( targetSize > sizeof targetBuf )
            return E_UNEXPECTED;

        hr = ReadMemory( 
            (Address64) addr, 
            targetSize, 
            lenRead, 
            targetBuf );
        if ( FAILED( hr ) )
            return hr;
        if ( lenRead < targetSize )
            return HRESULT_FROM_WIN32( ERROR_PARTIAL_COPY );

        return FromRawValue( targetBuf, type, value );
    }

    HRESULT ExprContext::GetValue(
        MagoEE::Address aArrayAddr, 
        const MagoEE::DataObject& key, 
        MagoEE::Address& valueAddr )
    {
        return GetDRuntime()->GetValue( aArrayAddr, key, valueAddr );
    }

    int ExprContext::GetAAVersion()
    {
        return GetDRuntime()->GetAAVersion();
    }

    HRESULT ExprContext::GetClassName( MagoEE::Address addr, std::wstring& className )
    {
        BSTR bstrClassname;
        HRESULT hr = GetDRuntime()->GetClassName( addr, &bstrClassname );
        if ( SUCCEEDED( hr ) )
        {
            className = bstrClassname;
            SysFreeString( bstrClassname );
        }
        return hr;
    }

    HRESULT ExprContext::SetValue( 
        MagoEE::Declaration* decl, 
        const MagoEE::DataValue& value )
    {
        if ( decl == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        CVDecl*                     cvDecl = (CVDecl*) decl;
        MagoST::ISymbolInfo*        sym = cvDecl->GetSymbol();
        MagoST::LocationType        loc = MagoST::LocIsNull;
        RefPtr<MagoEE::Type>        type;

        if ( !decl->GetType( type.Ref() ) )
            return E_FAIL;

        if ( !sym->GetLocation( loc ) )
            return E_FAIL;

        switch ( loc )
        {
        case LocIsRegRel:
        case LocIsStatic:
        case LocIsTLS:
            {
                MagoEE::Address addr = 0;

                hr = GetAddress( decl, addr );
                if ( FAILED( hr ) )
                    return hr;

                return SetValue( addr, type, value );
            }
            break;

        case LocIsEnregistered:
            {
                uint32_t    reg = 0;

                if ( !sym->GetRegister( reg ) )
                    return E_FAIL;

                // TODO: writing to registers
                return E_NOTIMPL;
            }
            break;

        // SetValue is not the right way to get a LocIsThisRel, and LocIsBitField isn't supported
        // can't write to constants either
        default:
            return E_FAIL;
        }

        return S_OK;
    }

    HRESULT ExprContext::SetValue( 
        MagoEE::Address addr, 
        MagoEE::Type* type, 
        const MagoEE::DataValue& value )
    {
        HRESULT         hr = S_OK;
        uint8_t         sourceBuf[ sizeof( MagoEE::DataValue ) ] = { 0 };
        size_t          sourceSize = type->GetSize();
        uint32_t        lenWritten = 0;

        // no value to set for complex/aggregate types
        if ( !type->IsScalar() 
            && !type->IsDArray() 
            && !type->IsAArray() 
            && !type->IsDelegate() )
            return S_OK;

        if ( sourceSize > sizeof sourceBuf )
            return E_FAIL;

        if ( type->IsPointer() )
        {
            hr = WriteInt( sourceBuf, sizeof sourceBuf, type, value.Addr );
        }
        else if ( type->IsIntegral() )
        {
            hr = WriteInt( sourceBuf, sizeof sourceBuf, type, value.UInt64Value );
        }
        else if ( type->IsReal() || type->IsImaginary() )
        {
            hr = WriteFloat( sourceBuf, sizeof sourceBuf, type, value.Float80Value );
        }
        else if ( type->IsComplex() )
        {
            const size_t    PartSize = type->GetSize() / 2;

            hr = WriteFloat( sourceBuf, sizeof sourceBuf, type, value.Complex80Value.RealPart );
            if ( FAILED( hr ) )
                return hr;

            hr = WriteFloat( 
                sourceBuf + PartSize, 
                sizeof sourceBuf - PartSize, 
                type, 
                value.Complex80Value.ImaginaryPart );
        }
        else if ( type->IsDArray() )
        {
            MagoEE::ITypeDArray* arrayType = type->AsTypeDArray();
            const size_t LengthSize = arrayType->GetLengthType()->GetSize();
            //const size_t AddressSize = arrayType->GetPointerType()->GetSize();

            hr = WriteInt( sourceBuf, sizeof sourceBuf, arrayType->GetLengthType(), value.Array.Length );
            if ( FAILED( hr ) )
                return hr;

            hr = WriteInt( 
                sourceBuf + LengthSize, 
                sizeof sourceBuf - LengthSize, 
                arrayType->GetPointerType(), 
                value.Array.Addr );
        }
        else if ( type->IsAArray() )
        {
            hr = WriteInt( sourceBuf, sizeof sourceBuf, type, value.Addr );
        }
        else if ( type->IsDelegate() )
        {
            MagoEE::Type* voidPtrType = mTypeEnv->GetVoidPointerType();
            const size_t AddressSize = voidPtrType->GetSize();

            hr = WriteInt( sourceBuf, sizeof sourceBuf, voidPtrType, value.Delegate.ContextAddr );
            if ( FAILED( hr ) )
                return hr;

            hr = WriteInt( 
                sourceBuf + AddressSize, 
                sizeof sourceBuf - AddressSize, 
                voidPtrType, 
                value.Delegate.FuncAddr );
        }
        else
            return E_FAIL;

        if ( FAILED( hr ) )
            return hr;

        hr = WriteMemory( 
            (Address64) addr, 
            sourceSize, 
            lenWritten, 
            sourceBuf );
        if ( FAILED( hr ) )
            return hr;
        if ( lenWritten < sourceSize )
            return HRESULT_FROM_WIN32( ERROR_PARTIAL_COPY );

        return S_OK;
    }

    HRESULT ExprContext::ReadMemory( 
        MagoEE::Address addr, 
        uint32_t sizeToRead, 
        uint32_t& sizeRead, 
        uint8_t* buffer )
    {
        HRESULT         hr = S_OK;
        uint32_t        len = sizeToRead;
        uint32_t        lenRead = 0;
        uint32_t        lenUnreadable = 0;
        IDebuggerProxy* debuggerProxy = mThread->GetDebuggerProxy();

        hr = debuggerProxy->ReadMemory(
            mThread->GetCoreProcess(),
            (Address64) addr,
            len,
            lenRead,
            lenUnreadable,
            buffer );
        if ( FAILED( hr ) )
            return hr;

        sizeRead = lenRead;

        return S_OK;
    }

    HRESULT ExprContext::WriteMemory( 
        MagoEE::Address addr, 
        uint32_t sizeToWrite, 
        uint32_t& sizeWritten, 
        uint8_t* buffer )
    {
        HRESULT         hr = S_OK;
        uint32_t        len = sizeToWrite;
        uint32_t        lenWritten = 0;
        IDebuggerProxy* debuggerProxy = mThread->GetDebuggerProxy();

        hr = debuggerProxy->WriteMemory(
            mThread->GetCoreProcess(),
            (Address64) addr,
            len,
            lenWritten,
            buffer );
        if ( FAILED( hr ) )
            return hr;

        sizeWritten = lenWritten;

        return S_OK;
    }

    std::wstring SymStringToWString( const SymString& sym )
    {
        int len = MultiByteToWideChar( CP_UTF8, 0, sym.GetName(), sym.GetLength(), NULL, 0 );
        std::wstring wname;
        wname.resize(len);
        MultiByteToWideChar( CP_UTF8, 0, sym.GetName(), sym.GetLength(), (wchar_t*)wname.data(), len);
        return wname;
    }

    HRESULT ExprContext::SymbolFromAddr( MagoEE::Address addr, std::wstring& symName, MagoEE::Type** pType )
    {
        RefPtr<MagoST::ISession>    session;
        MagoST::SymHandle           symHandle = { 0 };

        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        uint32_t    symOff = 0;
        HRESULT hr = session->FindGlobalSymbolByAddr( addr, symHandle, sec, offset, symOff );
        if ( FAILED( hr ) )
            return hr;

        if ( symOff != 0 )
            return E_NOT_FOUND;

        MagoST::SymInfoData infoData = { 0 };
        MagoST::ISymbolInfo* symInfo;
        hr = session->GetSymbolInfo( symHandle, infoData, symInfo );
        if ( FAILED( hr ) )
            return hr;

        if ( pType )
        {
            MagoST::TypeIndex typeIndex;
            symInfo->GetType( typeIndex );
            hr = GetTypeFromTypeSymbol( typeIndex, *pType );
            if ( FAILED( hr ) )
                return hr;
        }

        SymString pstrName;
        if ( !symInfo->GetName( pstrName ) )
            return E_FAIL;

        symName = SymStringToWString( pstrName );
        return S_OK;
    }

    HRESULT ExprContext::CallFunction( MagoEE::Address addr, MagoEE::ITypeFunction* func, MagoEE::Address arg, MagoEE::DataObject& value )
    {
        return E_MAGOEE_CALL_NOT_IMPLEMENTED;
    }

    ////////////////////////////////////////////////////////////////////////////// 

    HRESULT ExprContext::Init( 
        Module* module, 
        Thread* thread,
        MagoST::SymHandle funcSH, 
        std::vector<MagoST::SymHandle>& blockSH,
        Address64 pc,
        IRegisterSet* regSet )
    {
        _ASSERT( regSet != NULL );
        _ASSERT( module != NULL );
        _ASSERT( thread != NULL );

        mModule = module;
        mThread = thread;
        mFuncSH = funcSH;
        mBlockSH = blockSH;
        mPC = pc;
        mRegSet = regSet;

        // we have to be able to evaluate expressions even if there aren't symbols

        HRESULT hr = S_OK;
        ArchData*   archData = mThread->GetCoreProcess()->GetArchData();

        hr = MagoEE::EED::MakeTypeEnv( archData->GetPointerSize(), mTypeEnv.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = MagoEE::EED::MakeNameTable( mStrTable.Ref() );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    Thread* ExprContext::GetThread()
    {
        return mThread.Get();
    }


    ////////////////////////////////////////////////////////////////////////////// 

    HRESULT ExprContext::FindLocalSymbol( const char* name, size_t nameLen, MagoST::SymHandle& localSH )
    {
        RefPtr<MagoST::ISession>    session;

        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        HRESULT hr = E_FAIL;
        for ( auto it = mBlockSH.rbegin(); hr != S_OK && it != mBlockSH.rend(); it++)
            hr = session->FindChildSymbol( *it, name, nameLen, localSH );
        
        if ( hr != S_OK )
            return E_NOT_FOUND;
        return S_OK;
    }

    HRESULT ExprContext::FindClosureSymbol( const char* name, size_t nameLen, MagoEE::Declaration*& decl )
    {
        if ( mBlockSH.empty() )
            return E_NOT_FOUND;

        RefPtr<MagoST::ISession>    session;
        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        // if both exist, __capture is always the same as __closptr.__chain
        MagoST::SymHandle closureSH;
        HRESULT hr = session->FindChildSymbol( *mBlockSH.begin(), "__closptr", 9, closureSH );
        if ( hr != S_OK )
            hr = session->FindChildSymbol( *mBlockSH.begin(), "__capture", 9, closureSH );
        if (hr != S_OK)
            return E_NOT_FOUND;

        MagoST::SymInfoData     closureInfoData = { 0 };
        MagoST::ISymbolInfo*    closureInfo = NULL;
        hr = session->GetSymbolInfo( closureSH, closureInfoData, closureInfo );
        if (hr != S_OK)
            return E_NOT_FOUND;

        std::vector<MagoST::TypeHandle> chain;
        while (true)
        {
            MagoST::TypeIndex       pointerTI = { 0 };
            MagoST::TypeHandle      pointerTH = { 0 };
            MagoST::SymInfoData     pointerInfoData = { 0 };
            MagoST::ISymbolInfo*    pointerInfo = NULL;

            // get pointer type
            if ( !closureInfo->GetType( pointerTI ) ||
                 !session->GetTypeFromTypeIndex( pointerTI, pointerTH ) ||
                 session->GetTypeInfo( pointerTH, pointerInfoData, pointerInfo ) != S_OK )
                break;
            closureInfo = pointerInfo;

            // dereference pointer type
            if ( !closureInfo->GetType( pointerTI ) ||
                 !session->GetTypeFromTypeIndex( pointerTI, pointerTH ) )
                break;

            MagoST::TypeHandle fieldTH;
            hr = session->FindChildType( pointerTH, name, nameLen, fieldTH );
            if ( hr == S_OK )
            {
                MagoST::TypeIndex       fieldTI = { 0 };
                MagoST::SymInfoData     fieldInfoData = { 0 };
                MagoST::ISymbolInfo*    fieldInfo = NULL;
                if ( session->GetTypeInfo( fieldTH, fieldInfoData, fieldInfo) != S_OK ||
                     !fieldInfo->GetType( fieldTI ) )
                    break;

                MagoST::TypeIndex       typeIndex = 0;
                RefPtr<MagoEE::Type>    type;

                if ( !fieldInfo->GetType( typeIndex ) )
                    break;

                hr = GetTypeFromTypeSymbol( typeIndex, type.Ref() );
                if ( FAILED( hr ) )
                    break;

                auto closDecl = new ClosureVarCVDecl( this, fieldInfoData, fieldInfo, closureSH, chain );
                closDecl->SetType( type );
                closDecl->AddRef();

                decl = closDecl;
                return hr;
            }

            MagoST::TypeHandle  chainTH = { 0 };
            if ( session->FindChildType( pointerTH, "__chain", 7, chainTH ) != S_OK ||
                 session->GetTypeInfo( chainTH, closureInfoData, closureInfo ) != S_OK )
                break;

            chain.push_back( chainTH );
        }
        return E_NOT_FOUND;
    }

	HRESULT ExprContext::FindGlobalSymbol( const char* name, size_t nameLen, MagoST::SymHandle& globalSH )
    {
        RefPtr<MagoST::ISession>    session;

        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        HRESULT hr = FindGlobalSymbol( session, name, nameLen, globalSH );
        if ( FAILED( hr ) )
        {
            // look for static and global symbols by adding the scope to the name
            // the scope is determined from the current function name, so it does
            // not work with extern(C) functions
            SymInfoData     infoData = { 0 };
            ISymbolInfo*    symInfo = NULL;

            hr = session->GetSymbolInfo( mFuncSH, infoData, symInfo );
            if ( SUCCEEDED( hr ) && symInfo )
            {
                SymString       pstrName;
                if ( symInfo->GetName( pstrName ) )
                {
                    std::string sym( name, nameLen );
                    std::string scopeName( pstrName.GetName(), pstrName.GetLength() );
                    while ( scopeName.length() > 0 )
                    {
                        std::string symName = scopeName + "." + sym;
                        hr = FindGlobalSymbol( session, symName.c_str(), symName.length(), globalSH );
                        if ( SUCCEEDED( hr ) )
                            break;
                        std::string::reverse_iterator it = std::find( scopeName.rbegin(), scopeName.rend(), '.' );
                        if( it == scopeName.rend() )
                            break;
                        scopeName.erase( it.base() - 1, scopeName.end() );
                    }
                }
            }
        }
        return hr;
    }

    HRESULT ExprContext::FindGlobalSymbol( 
        MagoST::ISession* session, 
        const char* name, 
        size_t nameLen, 
        MagoST::SymHandle& globalSH )
    {
        HRESULT hr = S_OK;
        MagoST::EnumNamedSymbolsData    enumData = { 0 };

        for ( int i = 0; i < MagoST::SymHeap_Count; i++ )
        {
            hr = session->FindFirstSymbol( (MagoST::SymbolHeapId) i, name, nameLen, enumData );
            if ( hr == S_OK )
                break;
        }

        if ( hr != S_OK )
            return E_NOT_FOUND;

        // TODO: for now we only care about the first one
        hr = session->GetCurrentSymbol( enumData, globalSH );

        session->FindSymbolDone( enumData );

        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT ExprContext::GetSession( MagoST::ISession*& session )
    {
        RefPtr<MagoST::ISession>    spSession;

        if ( !mModule->GetSymbolSession( spSession ) )
            return E_FAIL;

        session = spSession.Detach();
        return S_OK;
    }

    MagoEE::ITypeEnv* ExprContext::GetTypeEnv()
    {
        return mTypeEnv;
    }

    MagoEE::NameTable* ExprContext::GetStringTable()
    {
        return mStrTable;
    }

    MagoST::SymHandle ExprContext::GetFunctionSH()
    {
        return mFuncSH;
    }

    Mago::IRegisterSet* ExprContext::GetRegisterSet()
    {
        return mRegSet;
    }

    const std::vector<MagoST::SymHandle>& ExprContext::GetBlockSH()
    {
        return mBlockSH;
    }

    Address64 ExprContext::GetPC()
    {
        return mPC;
    }

    // TODO: these declarations can be cached. We can keep a small cache for each module
    HRESULT ExprContext::MakeDeclarationFromSymbol( 
        MagoST::SymHandle handle, 
        MagoEE::Declaration*& decl )
    {
        HRESULT                     hr = S_OK;
        MagoST::SymTag              tag = MagoST::SymTagNull;
        MagoST::SymInfoData         infoData = { 0 };
        MagoST::ISymbolInfo*        symInfo = NULL;
        RefPtr<MagoST::ISession>    session;
        MagoST::TypeIndex           typeIndex;

        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        hr = session->GetSymbolInfo( handle, infoData, symInfo );
        if ( FAILED( hr ) )
            return hr;

        tag = symInfo->GetSymTag();

        switch ( tag )
        {
        case SymTagData:
            hr = MakeDeclarationFromDataSymbol( infoData, symInfo, decl );
            break;

        case SymTagFunction:
            hr = MakeDeclarationFromFunctionSymbol( infoData, symInfo, decl );
            break;

        case SymTagTypedef:
            hr = MakeDeclarationFromTypedefSymbol( infoData, symInfo, decl );
            break;

        default:
            // try type declarations
            if( symInfo->GetType( typeIndex ) )
            {
                RefPtr<MagoEE::Type> type;
                hr = GetTypeFromTypeSymbol( typeIndex, type.Ref() );
                if( !FAILED( hr ) )
                    decl = type->GetDeclaration();
                if( !decl )
                    hr = E_FAIL;
                else
                    decl->AddRef();
            }
            else
                hr = E_FAIL;
            break;
        }

        return hr;
    }

    HRESULT ExprContext::MakeDeclarationFromSymbol( 
        MagoST::TypeHandle handle, 
        MagoEE::Declaration*& decl )
    {
        HRESULT hr = S_OK;
        MagoST::SymTag              tag = MagoST::SymTagNull;
        MagoST::SymInfoData         infoData = { 0 };
        MagoST::ISymbolInfo*        symInfo = NULL;
        RefPtr<MagoST::ISession>    session;

        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        hr = session->GetTypeInfo( handle, infoData, symInfo );
        if ( FAILED( hr ) )
            return hr;

        tag = symInfo->GetSymTag();

        switch ( tag )
        {
        case SymTagData:
            hr = MakeDeclarationFromDataSymbol( infoData, symInfo, decl );
            break;

        case SymTagFunction:
            hr = MakeDeclarationFromFunctionSymbol( infoData, symInfo, decl );
            break;

        case SymTagBaseClass:
            hr = MakeDeclarationFromBaseClassSymbol( infoData, symInfo, decl );
            break;

        case SymTagTypedef:
            hr = MakeDeclarationFromTypedefSymbol( infoData, symInfo, decl );
            break;

        case SymTagVTableShape:
            hr = MakeDeclarationFromVTableShapeSymbol( infoData, symInfo, decl );
            break;

        default:
            return E_FAIL;
        }

        return hr;
    }

    HRESULT ExprContext::MakeDeclarationFromDataSymbol( 
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::Declaration*& decl )
    {
        HRESULT                 hr = S_OK;
        MagoST::TypeIndex       typeIndex = 0;
        RefPtr<MagoEE::Type>    type;

        if ( !symInfo->GetType( typeIndex ) )
            return E_FAIL;

        hr = GetTypeFromTypeSymbol( typeIndex, type.Ref() );
        if ( FAILED( hr ) )
            return hr;

        return MakeDeclarationFromDataSymbol( infoData, symInfo, type, decl );
    }

    HRESULT ExprContext::MakeDeclarationFromDataSymbol( 
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::Type* type,
        MagoEE::Declaration*& decl )
    {
        MagoST::LocationType    loc;
        RefPtr<GeneralCVDecl>   cvDecl;

        if ( !symInfo->GetLocation( loc ) )
            return E_FAIL;

        switch ( loc )
        {
        case LocIsRegRel:
        case LocIsBitField:
        case LocIsConstant:
        case LocIsEnregistered:
        case LocIsStatic:
        case LocIsThisRel:
        case LocIsTLS:
            cvDecl = new GeneralCVDecl( this, infoData, symInfo );
            break;

        default:
            return E_FAIL;
        }

        if ( cvDecl == NULL )
            return E_OUTOFMEMORY;

        cvDecl->SetType( type );

        decl = cvDecl.Detach();
        return S_OK;
    }

    HRESULT ExprContext::MakeDeclarationFromFunctionSymbol(
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo,
        MagoEE::Declaration*& decl )
    {
        HRESULT                 hr = S_OK;
        MagoST::TypeIndex       typeIndex = 0;
        RefPtr<MagoEE::Type>    type;
        RefPtr<MagoST::ISession> session;

        if ( !symInfo->GetType( typeIndex ) )
            return E_FAIL;

        hr = GetTypeFromTypeSymbol( typeIndex, type.Ref() );
        if ( FAILED(hr) )
            return hr;

        RefPtr<FunctionCVDecl>   cvDecl;
        cvDecl = new FunctionCVDecl( this, infoData, symInfo );
        cvDecl->SetType( type );

        decl = cvDecl.Detach();
        return S_OK;
    }

    HRESULT ExprContext::MakeDeclarationFromTypedefSymbol(
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::Declaration*& decl )
    {
        HRESULT                 hr = S_OK;
        MagoST::TypeIndex       typeIndex = 0;
        SymString               pstrName1;
        MagoST::TypeHandle      handle = { 0 };
        MagoST::SymInfoData     typeInfoData = { 0 };
        MagoST::ISymbolInfo*    typeInfo = NULL;
        SymString               pstrName2;
        RefPtr<MagoEE::Type>    refType;
        RefPtr<MagoST::ISession>    session;

        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        if ( !symInfo->GetType( typeIndex ) )
            return E_FAIL;

        if ( !symInfo->GetName( pstrName1 ) )
            return E_FAIL;

        if ( !session->GetTypeFromTypeIndex( typeIndex, handle ) )
            return E_FAIL;

        hr = session->GetTypeInfo( handle, typeInfoData, typeInfo );
        if ( FAILED( hr ) )
            return hr;

        // it doesn't have to have a name: for example a pointer type
        typeInfo->GetName( pstrName2 );

        hr = GetTypeFromTypeSymbol( typeIndex, refType.Ref() );
        if ( FAILED( hr ) )
            return hr;

        if ( (pstrName2.GetName() != NULL)
            && (pstrName1.GetLength() == pstrName2.GetLength()) 
            && (strncmp( pstrName1.GetName(), pstrName2.GetName(), pstrName1.GetLength() ) == 0) 
            && refType->GetDeclaration() )
        {
            // the typedef has the same name as the type, 
            // so let's use the referenced type directly, as if there's no typedef

            decl = refType->GetDeclaration().Detach();
        }
        else
        {
            RefPtr<GeneralCVDecl>   cvDecl;
            RefPtr<MagoEE::Type>    type;

            cvDecl = new GeneralCVDecl( this, infoData, symInfo );
            if ( cvDecl == NULL )
                return E_OUTOFMEMORY;

            hr = mTypeEnv->NewTypedef( cvDecl->GetName(), refType, type.Ref() );
            if ( FAILED( hr ) )
                return hr;

            cvDecl->SetType( type );

            decl = cvDecl.Detach();
        }

        return S_OK;
    }

    HRESULT ExprContext::MakeDeclarationFromBaseClassSymbol( 
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::Declaration*& decl )
    {
        HRESULT                 hr = S_OK;
        MagoST::TypeIndex       typeIndex = 0;
        RefPtr<MagoEE::Type>    type;
        RefPtr<GeneralCVDecl>   cvDecl;

        if ( !symInfo->GetType( typeIndex ) )
            return E_FAIL;

        hr = GetTypeFromTypeSymbol( typeIndex, type.Ref() );
        if ( FAILED( hr ) )
            return hr;

        cvDecl = new GeneralCVDecl( this, infoData, symInfo );
        if ( cvDecl == NULL )
            return E_OUTOFMEMORY;

        cvDecl->SetType( type );

        decl = cvDecl.Detach();

        return S_OK;
    }

    HRESULT ExprContext::MakeDeclarationFromVTableShapeSymbol( 
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::Declaration*& decl )
    {
        HRESULT                 hr = S_OK;
        RefPtr<MagoEE::Type>    type;
        RefPtr<MagoEE::Type>    ptype;
        RefPtr<GeneralCVDecl>   cvDecl;
        uint32_t count;

        // build a static array of void pointers
        if ( !symInfo->GetCount( count ) )
            return E_FAIL;

        MagoEE::Type* vptype = mTypeEnv->GetVoidPointerType();
        hr = mTypeEnv->NewSArray( vptype, count, type.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = mTypeEnv->NewPointer( type, ptype.Ref() );
        if ( FAILED( hr ) )
            return hr;

        cvDecl = new VTableCVDecl( this, count, ptype );
        if ( cvDecl == NULL )
            return E_OUTOFMEMORY;

        decl = cvDecl.Detach();

        return S_OK;
    }

    MagoEE::ENUMTY ExprContext::GetBasicTy( DWORD diaBaseTypeId, DWORD size )
    {
        MagoEE::ENUMTY  ty = MagoEE::Tnone;

        // TODO: what about ifloat, idouble, ireal?
        switch ( diaBaseTypeId )
        {
        case btVoid:
            ty = MagoEE::Tvoid;
            break;

        case btChar:
            switch ( size )
            {
            case 1: ty = MagoEE::Tchar; break;
            case 4: ty = MagoEE::Tdchar; break;
            }
            break;

        case btWChar:
        case btChar16:
            ty = MagoEE::Twchar;
            break;

        case btChar32:
            ty = MagoEE::Twchar;
            break;

        case btInt:
        case btLong:
            switch ( size )
            {
            case 1: ty = MagoEE::Tint8; break;
            case 2: ty = MagoEE::Tint16; break;
            case 4: ty = MagoEE::Tint32; break;
            case 8: ty = MagoEE::Tint64; break;
            }
            break;

        case btUInt:
        case btULong:
            switch ( size )
            {
            case 1: ty = MagoEE::Tuns8; break;
            case 2: ty = MagoEE::Tuns16; break;
            case 4: ty = MagoEE::Tuns32; break;
            case 8: ty = MagoEE::Tuns64; break;
            }
            break;

        case btFloat:
            switch ( size )
            {
            case 4: ty = MagoEE::Tfloat32;  break;
            case 8: ty = MagoEE::Tfloat64;  break;
            case 10: ty = MagoEE::Tfloat80;  break;
            }
            break;

        case btBool:
            ty = MagoEE::Tbool;
            break;

        case btComplex:
            switch ( size )
            {
            case 8: ty = MagoEE::Tcomplex32;  break;
            case 16: ty = MagoEE::Tcomplex64;  break;
            case 20: ty = MagoEE::Tcomplex80;  break;
            }
            break;
        }

        return ty;
    }

    HRESULT ExprContext::GetTypeFromTypeSymbol( 
        MagoST::TypeIndex typeIndex,
        MagoEE::Type*& type )
    {
        HRESULT                 hr = S_OK;
        MagoST::SymTag          tag = MagoST::SymTagNull;
        MagoST::TypeHandle      typeTH = { 0 };
        MagoST::SymInfoData     infoData = { 0 };
        MagoST::ISymbolInfo*    symInfo = NULL;
        RefPtr<MagoST::ISession>    session;

        if ( GetSession( session.Ref() ) != S_OK )
            return E_FAIL;

        if ( !session->GetTypeFromTypeIndex( typeIndex, typeTH ) )
            return E_FAIL;

        hr = session->GetTypeInfo( typeTH, infoData, symInfo );
        if ( FAILED( hr ) )
            return hr;

        tag = symInfo->GetSymTag();

        switch ( tag )
        {
        case SymTagUDT:
        case SymTagEnum:
            return GetUdtTypeFromTypeSymbol( typeTH, infoData, symInfo, type );

        case SymTagFunctionType:
            return GetFunctionTypeFromTypeSymbol( typeTH, infoData, symInfo, type );

        case SymTagBaseType:
            return GetBasicTypeFromTypeSymbol( infoData, symInfo, type );

        case SymTagPointerType:
            {
                RefPtr<MagoEE::Type>    pointed;
                RefPtr<MagoEE::Type>    pointer;
                MagoST::TypeIndex       pointedTypeIndex = 0;

                if ( !symInfo->GetType( pointedTypeIndex ) )
                    return E_NOT_FOUND;

                hr = GetTypeFromTypeSymbol( pointedTypeIndex, pointed.Ref() );
                if ( FAILED( hr ) )
                    return E_NOT_FOUND;

#if USE_REFERENCE_TYPE
                if ( (pointed->AsTypeStruct() != NULL) 
                    && (pointed->AsTypeStruct()->GetUdtKind() == MagoEE::Udt_Class) )
                {
                    hr = mTypeEnv->NewReference( pointed, pointer.Ref() );
                }
                else
                {
                    hr = mTypeEnv->NewPointer( pointed, pointer.Ref() );
                }
#else
                hr = mTypeEnv->NewPointer( pointed, pointer.Ref() );
#endif
                if ( hr != S_OK )
                    return E_NOT_FOUND;

                type = pointer.Detach();
                return S_OK;
            }
            break;

        case SymTagArrayType:
            {
                RefPtr<MagoEE::Type>    elemType;
                RefPtr<MagoEE::Type>    arrayType;
                uint32_t                len = 0;
                uint32_t                elemSize = 0;
                uint32_t                count = 0;
                MagoST::TypeIndex       elemTypeIndex = 0;

                if ( !symInfo->GetType( elemTypeIndex ) )
                    return E_NOT_FOUND;

                if ( !symInfo->GetLength( len ) )
                    return E_NOT_FOUND;

                hr = GetTypeFromTypeSymbol( elemTypeIndex, elemType.Ref() );
                if ( FAILED( hr ) )
                    return hr;

                elemSize = elemType->GetSize();
                if ( elemSize != 0 )
                    count = len / elemSize;

                hr = mTypeEnv->NewSArray( elemType, count, arrayType.Ref() );
                if ( FAILED( hr ) )
                    return hr;

                type = arrayType.Detach();
                return S_OK;
            }
            break;

        case SymTagTypedef:
            _ASSERT( false );
            break;

        case SymTagCustomType:
            return GetCustomTypeFromTypeSymbol( infoData, symInfo, type );

        //case SymTagManagedType:
        default:
            return E_FAIL;
        }

        return E_FAIL;
    }

    HRESULT ExprContext::GetFunctionTypeFromTypeSymbol( 
        MagoST::TypeHandle typeHandle,
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo,
        MagoEE::Type*& type )
    {
        HRESULT hr = S_OK;
        RefPtr<MagoEE::ParameterList>   params;
        MagoST::TypeIndex       retTI = 0;
        RefPtr<MagoEE::Type>    retType;
        MagoST::TypeIndex       paramListTI = 0;
        MagoST::TypeHandle      paramListTH = { 0 };
        MagoST::SymInfoData     paramListInfoData = { 0 };
        MagoST::ISymbolInfo*    paramListInfo = NULL;
        std::vector<MagoST::TypeIndex> paramTIs;
        RefPtr<MagoST::ISession>    session;

        if ( GetSession( session.Ref() ) != S_OK )
            return E_NOT_FOUND;

        if ( !symInfo->GetType( retTI ) )
            return E_FAIL;

        hr = GetTypeFromTypeSymbol( retTI, retType.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = mTypeEnv->NewParams( params.Ref() );
        if ( FAILED( hr ) )
            return hr;

        if ( !symInfo->GetParamList( paramListTI ) )
            return E_FAIL;

        if ( !session->GetTypeFromTypeIndex( paramListTI, paramListTH ) )
            return E_FAIL;

        hr = session->GetTypeInfo( paramListTH, paramListInfoData, paramListInfo );
        if ( FAILED( hr ) )
            return hr;

        if ( !paramListInfo->GetTypes( paramTIs ) )
            return E_FAIL;

        for ( uint32_t i = 0; i < paramTIs.size(); i++ )
        {
            RefPtr<MagoEE::Type>        paramType;
            RefPtr<MagoEE::Parameter>   param;

            hr = GetTypeFromTypeSymbol( paramTIs[i], paramType.Ref() );
            if ( FAILED( hr ) )
                return hr;

            hr = mTypeEnv->NewParam( 0, paramType, param.Ref() );
            if ( FAILED( hr ) )
                return hr;

            params->List.push_back( param );
        }

        uint8_t callConv;
        if ( !symInfo->GetCallConv( callConv ) )
            return E_FAIL;

        // TODO: var args
        hr = mTypeEnv->NewFunction( retType, params, callConv, 0, type );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT ExprContext::GetUdtTypeFromTypeSymbol( 
        MagoST::TypeHandle typeHandle,
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo,
        MagoEE::Type*& type )
    {
        RefPtr<MagoEE::Declaration> decl;

        decl = new TypeCVDecl( this, typeHandle, infoData, symInfo );
        if ( decl == NULL )
            return E_OUTOFMEMORY;

        SymString name;
        if( !symInfo->GetName( name ) )
            return E_INVALIDARG;

        bool isArray = false;
        bool isString = false;
        bool usesWchar = false;
        bool usesDchar = false;
        const char* namePtr = name.GetName();
        size_t nameLen = name.GetLength();
        if( nameLen == 6 && strncmp( namePtr, "string", 6 ) == 0 )
            isArray = isString = true;
        else if( nameLen == 7 && strncmp( namePtr, "wstring", 7 ) == 0 )
            isArray = isString = usesWchar = true;
        else if( nameLen == 7 && strncmp( namePtr, "dstring", 7 ) == 0 )
            isArray = isString = usesDchar = true;
        else if( nameLen == 6 && strncmp( namePtr, "dArray", 6 ) == 0 )
            isArray = true;
        if( !isArray && nameLen > 2 )
        {
            isArray = strcmp( namePtr + nameLen - 2, "[]" ) == 0;
            usesDchar = isArray && ( ( nameLen == 7 && strncmp( namePtr, "dchar[]", 7 ) == 0 ) || 
                                     ( nameLen == 14 && strncmp( namePtr, "const(dchar)[]", 14 ) == 0 ) );
        }
        if( isArray )
        {
            MagoEE::Declaration* ptrdecl = NULL;
            HRESULT hr = decl->FindObject( L"ptr", ptrdecl );
            if( FAILED( hr ) )
                return hr;
            MagoEE::Type* ptrtype = NULL;
            hr = E_INVALIDARG;
            if( ptrdecl != NULL && ptrdecl->GetType( ptrtype ) && ptrtype != NULL )
            {
                MagoEE::ITypeNext* nexttype = ptrtype->AsTypeNext();
                if( nexttype != NULL )
                {
                    RefPtr<MagoEE::Type> ntype;
                    if( usesDchar )
                        ntype = mTypeEnv->GetType( MagoEE::Tdchar );
                    else if( usesWchar )
                        ntype = mTypeEnv->GetType( MagoEE::Twchar );
                    else if( isString )
                        ntype = mTypeEnv->GetType( MagoEE::Tchar );
                    else
                        ntype = nexttype->GetNext();

                    if( isString )
                        ntype = ntype->MakeInvariant();
                    hr = mTypeEnv->NewDArray( ntype, type );
                }
            }

            if( ptrtype )
                ptrtype->Release();
            if( ptrdecl )
                ptrdecl->Release();
            return hr;
        }
        
        if( nameLen == 9 && strncmp( namePtr, "dDelegate", 9 ) == 0 )
        {
            MagoEE::Declaration* ptrdecl = NULL;
            HRESULT hr = decl->FindObject( L"funcptr", ptrdecl );
            if( FAILED( hr ) )
                return hr;
            MagoEE::Type* ptrtype = NULL;
            hr = E_INVALIDARG;
            if( ptrdecl != NULL && ptrdecl->GetType( ptrtype ) && ptrtype != NULL )
            {
                hr = mTypeEnv->NewDelegate( ptrtype, type );
            }
            if( ptrtype )
                ptrtype->Release();
            if( ptrdecl )
                ptrdecl->Release();
            return hr;
        }

        bool isAArray = nameLen == 11 && strncmp(namePtr, "dAssocArray", 11) == 0;
        if ( !isAArray )
            isAArray = nameLen > 2 && namePtr[nameLen - 1] == ']' && namePtr[nameLen - 2] != '[';
        if( isAArray )
        {
            MagoEE::Type* keytype = NULL;
            MagoEE::Type* valtype = NULL;
            HRESULT hr = FindObjectType( decl, L"__key_t", keytype );
            if( !FAILED( hr ) )
                hr = FindObjectType( decl, L"__val_t", valtype );
            if( !FAILED( hr ) )
            {
                hr = mTypeEnv->NewAArray( valtype, keytype, type );
            }
            if( keytype )
                keytype->Release();
            if( valtype )
                valtype->Release();
            if( !FAILED( hr ) )
                return hr;
        }

        if ( !decl->GetType( type ) )
            return E_NOT_FOUND;

        return S_OK;
    }

    HRESULT ExprContext::GetBasicTypeFromTypeSymbol( 
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo,
            MagoEE::Type*& type )
    {
        DWORD           baseTypeId = 0;
        uint32_t        size = 0;
        MagoEE::ENUMTY  ty = MagoEE::Tnone;

        if ( !symInfo->GetBasicType( baseTypeId ) )
            return E_NOT_FOUND;

        if ( !symInfo->GetLength( size ) )
            return E_NOT_FOUND;

        ty = GetBasicTy( baseTypeId, size );
        if ( ty == MagoEE::Tnone )
            return E_NOT_FOUND;

        type = mTypeEnv->GetType( ty );
        if ( type == NULL )
            return E_NOT_FOUND;

        RefPtr<MagoEE::Type> mtype;
        uint16_t mod;
        if ( symInfo->GetMod( mod ) && ( mod & MODtypesMask ) != 0 )
            type = mtype = type->MakeMod( (MOD) ( mod & MODtypesMask ) );

        type->AddRef();
        return S_OK;
    }

    HRESULT ExprContext::GetCustomTypeFromTypeSymbol( 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo,
        MagoEE::Type*& type )
    {
        HRESULT     hr = S_OK;
        uint32_t    oemId = 0;
        uint32_t    oemSymId = 0;

        if ( !symInfo->GetOemId( oemId ) )
            return E_FAIL;
        if ( oemId != DMD_OEM )
            return E_FAIL;

        if ( !symInfo->GetOemSymbolId( oemSymId ) )
            return E_FAIL;

        switch ( oemSymId )
        {
        case DMD_OEM_DARRAY:
        case DMD_OEM_AARRAY:
        case DMD_OEM_DELEGATE:
            {
                RefPtr<MagoEE::Type>           types[2];
                std::vector<MagoST::TypeIndex> typeIndexes;

                if ( !symInfo->GetTypes( typeIndexes ) )
                    return E_FAIL;

                if ( typeIndexes.size() != 2 )
                    return E_FAIL;

                for ( uint32_t i = 0; i < typeIndexes.size(); i++ )
                {
                    hr = GetTypeFromTypeSymbol( typeIndexes[i], types[i].Ref() );
                    if ( FAILED( hr ) )
                        return hr;
                }

                switch ( oemSymId )
                {
                case DMD_OEM_DARRAY:
                    hr = mTypeEnv->NewDArray( types[1], type );
                    break;

                case DMD_OEM_AARRAY:
                    hr = mTypeEnv->NewAArray( types[1], types[0], type );
                    break;

                case DMD_OEM_DELEGATE:
                    hr = mTypeEnv->NewDelegate( types[1], type );
                    break;
                }

                return hr;
            }
            break;

        default:
            return E_FAIL;
        }
    }


    HRESULT ExprContext::GetRegValue( DWORD reg, MagoEE::DataValueKind& kind, MagoEE::DataValue& value )
    {
        HRESULT         hr = S_OK;
        RegisterValue   regVal = { 0 };

        if ( reg == CV_REG_EDXEAX )
        {
            hr = mRegSet->GetValue( RegX86_EDX, regVal );
            if ( FAILED( hr ) )
                return hr;

            value.UInt64Value = regVal.Value.I32;
            value.UInt64Value <<= 32;

            hr = mRegSet->GetValue( RegX86_EAX, regVal );
            if ( FAILED( hr ) )
                return hr;

            value.UInt64Value |= regVal.Value.I32;
            kind = MagoEE::DataValueKind_UInt64;
        }
        else
        {
            ArchData*           archData = NULL;

            archData = mThread->GetCoreProcess()->GetArchData();

            int archRegId = archData->GetArchRegId( reg );
            if ( archRegId < 0 )
                return E_NOT_FOUND;

            hr = mRegSet->GetValue( archRegId, regVal );
            if ( FAILED( hr ) )
                return hr;

            switch ( regVal.Type )
            {
            case RegType_Int8:  value.UInt64Value = regVal.Value.I8;    kind = MagoEE::DataValueKind_UInt64;    break;
            case RegType_Int16: value.UInt64Value = regVal.Value.I16;   kind = MagoEE::DataValueKind_UInt64;    break;
            case RegType_Int32: value.UInt64Value = regVal.Value.I32;   kind = MagoEE::DataValueKind_UInt64;    break;
            case RegType_Int64: value.UInt64Value = regVal.Value.I64;   kind = MagoEE::DataValueKind_UInt64;    break;

            case RegType_Float32:
                value.Float80Value.FromFloat( regVal.Value.F32 );
                kind = MagoEE::DataValueKind_Float80;
                break;

            case RegType_Float64:
                value.Float80Value.FromDouble( regVal.Value.F64 );
                kind = MagoEE::DataValueKind_Float80;
                break;

            case RegType_Float80:   
                memcpy( &value.Float80Value, regVal.Value.F80Bytes, sizeof value.Float80Value );
                kind = MagoEE::DataValueKind_Float80;
                break;

            default:
                return E_FAIL;
            }
        }

        return S_OK;
    }

    Address64 ExprContext::GetTebBase()
    {
        return mThread->GetCoreThread()->GetTebBase();
    }

    DRuntime* ExprContext::GetDRuntime()
    {
        return mThread->GetProgram()->GetDRuntime();
    }
}
