/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ProgValueEnv.h"
#include "EventCallbackBase.h"
#include <MagoDEE.h>
#include "DataValue.h"
#include "DataEnv.h"
#include "DiaDecls.h"
#include "SymUtil.h"
//#include <winternl.h>
#include "winternl2.h"
#include <MagoCVSTI.h>
#include <MagoCVConst.h>

using namespace std;
using namespace boost;
using MagoEE::Type;
using namespace MagoST;

// from DMD source
#define DMD_OEM 0x42    // Digital Mars OEM number (picked at random)
#define DMD_OEM_DARRAY      1
#define DMD_OEM_AARRAY      2
#define DMD_OEM_DELEGATE    3


class EventCallback : public EventCallbackBase
{
    bool        mBPHit;
    BPCookie    mBPCookie;
    uint32_t    mLastThreadId;

public:
    EventCallback()
        :   mBPHit( false ),
            mBPCookie( 0 ),
            mLastThreadId( 0 )
    {
    }

    uint32_t GetLastThreadId()
    {
        return mLastThreadId;
    }

    bool TakeBPHit( BPCookie& cookie )
    {
        if ( !mBPHit )
            return false;

        cookie = mBPCookie;
        mBPHit = false;
        return true;
    }

    RunMode OnBreakpoint( IProcess* process, uint32_t threadId, Address address, Enumerator<BPCookie>* iter )
    {
        mLastThreadId = threadId;
        mBPHit = true;
        mBPCookie = iter->GetCurrent();
        return RunMode_Break;
    }
};


ProgramValueEnv::ProgramValueEnv( const wchar_t* progPath, uint32_t stopRva, MagoEE::ITypeEnv* typeEnv )
:   mProgPath( progPath ),
    mStopRva( stopRva ),
    mTypeEnv( typeEnv ),
    mExec( NULL ),
    mProc( NULL ),
    mThreadId( 0 ),
    mSymSession( NULL ),
    mThisTI( 0 )
#if 0
    mClassSym( NULL )
#endif
{
    memset( &mFuncSH, 0, sizeof mFuncSH );
    memset( &mBlockSH, 0, sizeof mBlockSH );
}

ProgramValueEnv::~ProgramValueEnv()
{
#if 0
    if ( mClassSym != NULL )
        mClassSym->Release();
#endif
    if ( mSymSession != NULL )
        mSymSession->Release();
    if ( mProc != NULL )
        mProc->Release();
    if ( mExec != NULL )
        delete mExec;
}

HRESULT ProgramValueEnv::StartProgram()
{
    HRESULT hr = S_OK;
    RefPtr<EventCallback>   callback;
    RefPtr<IProcess>        proc;
    LaunchInfo              launchInfo = { 0 };
    const BPCookie          Cookie = 1;
    RefPtr<IModule>         procMod;

    auto_ptr<Exec>  exec( new Exec() );

    callback = new EventCallback();
    callback->SetExec( exec.get() );

    hr = exec->Init( callback );
    if ( FAILED( hr ) )
        return hr;

    launchInfo.Exe = mProgPath.c_str();
    launchInfo.CommandLine = mProgPath.c_str();

    hr = exec->Launch( &launchInfo, proc.Ref() );
    if ( FAILED( hr ) )
        return hr;

    bool    loaded = false;

    for ( ; ; )
    {
        BPCookie    cookie = 0;

        hr = exec->WaitForDebug( INFINITE );
        if ( FAILED( hr ) )
            return hr;

        hr = exec->DispatchEvent();
        if ( FAILED( hr ) )
            return hr;

        if ( proc->IsStopped() )
        {
            if ( !loaded && callback->GetLoadCompleted() )
            {
                loaded = true;

                procMod = callback->GetProcessModule();
                Address         va = procMod->GetImageBase() + mStopRva;

                hr = exec->SetBreakpoint( proc, va, Cookie );
                if ( FAILED( hr ) )
                    return hr;
            }

            if ( callback->TakeBPHit( cookie ) && (cookie == Cookie) )
            {
                mThreadId = callback->GetLastThreadId();
                break;
            }

            hr = exec->Continue( proc, false );
            if ( FAILED( hr ) )
                return hr;
        }
    }

    // the process is now where we want it, so load its symbols
    _ASSERT( procMod != NULL );
    hr = LoadSymbols( procMod->GetImageBase() );
    if ( FAILED( hr ) )
        return hr;

    // transfer ownership to our object, since we want the process to stay alive while we test
    mExec = exec.release();
    mProc = proc.Detach();

    return S_OK;
}

HRESULT ProgramValueEnv::FindObject( const wchar_t* name, MagoEE::Declaration*& decl )
{
    HRESULT hr = S_OK;
    int     nzChars = 0;
    scoped_array<char>  nameChars;
    SymHandle   childSH;
    DWORD   flags = 0;

#if _WIN32_WINNT >= _WIN32_WINNT_LONGHORN
    flags = WC_ERR_INVALID_CHARS;
#endif

    // TODO: who should do the full search?

    nzChars = WideCharToMultiByte( CP_UTF8, flags, name, -1, NULL, 0, NULL, NULL );
    if ( nzChars == 0 )
        return HRESULT_FROM_WIN32( GetLastError() );

    nameChars.reset( new char[ nzChars ] );
    if ( nameChars.get() == NULL )
        return E_OUTOFMEMORY;

    nzChars = WideCharToMultiByte( CP_UTF8, flags, name, -1, nameChars.get(), nzChars, NULL, NULL );
    if ( nzChars == 0 )
        return HRESULT_FROM_WIN32( GetLastError() );

    // take away one for the terminator
    hr = mSymSession->FindChildSymbol( mBlockSH, nameChars.get(), nzChars-1, childSH );
    if ( hr != S_OK )
    {
        EnumNamedSymbolsData    enumData = { 0 };

        for ( int i = 0; i < SymHeap_Count; i++ )
        {
            hr = mSymSession->FindFirstSymbol( (SymbolHeapId) i, nameChars.get(), nzChars-1, enumData );
            if ( hr == S_OK )
                break;
        }

        if ( hr != S_OK )
            return E_NOT_FOUND;

        // TODO: for now we only care about the first one
        hr = mSymSession->GetCurrentSymbol( enumData, childSH );
        if ( FAILED( hr ) )
            return hr;
    }

    hr = MakeDeclarationFromSymbol( mSymSession, childSH, mTypeEnv, decl );
    if ( FAILED( hr ) )
        return hr;

    return S_OK;
}

void ConvertVariantToDataVal( const Variant& var, MagoEE::DataValue& dataVal )
{
    switch ( var.Tag )
    {
    case VarTag_Char:   dataVal.Int64Value = var.Data.I8;   break;
    case VarTag_Short:  dataVal.Int64Value = var.Data.I16;  break;
    case VarTag_UShort: dataVal.UInt64Value = var.Data.U16;  break;
    case VarTag_Long:   dataVal.Int64Value = var.Data.I32;  break;
    case VarTag_ULong:  dataVal.UInt64Value = var.Data.U32;  break;
    case VarTag_Quadword:   dataVal.Int64Value = var.Data.I64;  break;
    case VarTag_UQuadword:  dataVal.UInt64Value = var.Data.U64;  break;

    case VarTag_Real32: dataVal.Float80Value.FromFloat( var.Data.F32 ); break;
    case VarTag_Real64: dataVal.Float80Value.FromDouble( var.Data.F64 ); break;
    case VarTag_Real80: memcpy( &dataVal.Float80Value, var.Data.RawBytes, sizeof dataVal.Float80Value ); break;
        break;

    case VarTag_Complex32:  
        dataVal.Complex80Value.RealPart.FromFloat( var.Data.C32.Real );
        dataVal.Complex80Value.ImaginaryPart.FromFloat( var.Data.C32.Imaginary );
        break;
    case VarTag_Complex64: 
        dataVal.Complex80Value.RealPart.FromDouble( var.Data.C64.Real );
        dataVal.Complex80Value.ImaginaryPart.FromDouble( var.Data.C64.Imaginary );
        break;
    case VarTag_Complex80: 
        _ASSERT( offsetof( Complex10, RealPart ) < offsetof( Complex10, ImaginaryPart ) );
        memcpy( &dataVal.Complex80Value, var.Data.RawBytes, sizeof dataVal.Complex80Value );
        break;

    case VarTag_Real48:
    case VarTag_Real128:
    case VarTag_Complex128:
    case VarTag_Varstring: 
        break;
    }
}

boost::shared_ptr<DataObj> ProgramValueEnv::GetValue( MagoEE::Declaration* decl )
{
    _ASSERT( decl != NULL );

    HRESULT                     hr = S_OK;
    DiaDecl*                    diaDecl = (DiaDecl*) decl;
    ISymbolInfo*                sym = diaDecl->GetSymbol();
    LocationType                loc = LocIsNull;
    boost::shared_ptr<DataObj>  val;
    MagoEE::DataValueKind       valKind = MagoEE::DataValueKind_None;
    MagoEE::DataValue           dataVal = { 0 };
    RefPtr<Type>                type;

    if ( !decl->GetType( type.Ref() ) )
        throw L"Can't get type for this decl.";

    if ( !sym->GetLocation( loc ) )
        throw L"Can't get value for this decl.";

    switch ( loc )
    {
    case LocIsRegRel:
        {
            uint32_t    reg = 0;
            int32_t     offset = 0;
            MagoEE::Address addr = 0;

            if ( !sym->GetRegister( reg ) )
                throw L"Can't get value for this decl.";

            if ( !sym->GetOffset( offset ) )
                throw L"Can't get value for this decl.";

            hr = GetRegValue( reg, valKind, dataVal );
            if ( FAILED( hr ) )
                throw L"Can't get value for this decl.";

            if ( (valKind != MagoEE::DataValueKind_Int64) && (valKind != MagoEE::DataValueKind_UInt64) )
                throw L"Can't get value for this decl.";

            addr = dataVal.UInt64Value + offset;

            val = GetValue( addr, type, decl );
        }
        break;

    case LocIsBitField:
        _ASSERT( false );
        break;

    case LocIsConstant:
        {
            Variant var;

            if ( !sym->GetValue( var ) )
                throw L"Can't get value for this decl.";

            val.reset( new RValueObj() );
            ConvertVariantToDataVal( var, val->Value );
            val->SetType( type );
        }
        break;

    case LocIsEnregistered:
        {
            uint32_t    reg = 0;

            if ( !sym->GetRegister( reg ) )
                throw L"Can't get value for this decl.";

            hr = GetRegValue( reg, valKind, dataVal );
            if ( FAILED( hr ) )
                throw L"Can't get value for this decl.";

            val.reset( new LValueObj( decl, 0 ) );
            // TODO: check that the type is compatible with the DataValueKind
            val->Value = dataVal;
            val->SetType( type );
        }
        break;

    case LocIsStatic:
        {
            ULONGLONG       diaAddr = 0;
            uint16_t        sec = 0;
            uint32_t        offset = 0;

            if ( !sym->GetAddressOffset( offset ) )
                throw L"Can't get value for this decl.";
            if ( !sym->GetAddressSegment( sec ) )
                throw L"Can't get value for this decl.";
            diaAddr = mSymSession->GetVAFromSecOffset( sec, offset );
            if ( diaAddr == 0 )
                throw L"Can't get value for this decl.";

            val = GetValue( (MagoEE::Address) diaAddr, type, decl );
        }
        break;

    case LocIsThisRel:
        _ASSERT( false );
        break;

    case LocIsTLS:
        {
            uint32_t    offset = 0;
            MagoEE::DataValueKind   kind = MagoEE::DataValueKind_None;
            MagoEE::DataValue       dataVal = { 0 };
            DWORD       tebAddr = 0;
            DWORD       tlsPtrAddr = 0;
            DWORD       tlsArrayAddr = 0;
            DWORD       tlsBufAddr = 0;
            MagoEE::Address addr = 0;

            if ( !sym->GetAddressOffset( offset ) )
                throw L"Can't get value for this decl.";

            // TODO: CPU-dependent
            hr = GetRegValue( CV_REG_FS, kind, dataVal );
            if ( FAILED( hr ) )
                throw L"Can't get value for this decl.";

            hr = GetSegBase( (DWORD) dataVal.UInt64Value, tebAddr );
            if ( FAILED( hr ) )
                throw L"Can't get value for this decl.";

            tlsPtrAddr = tebAddr + offsetof( TEB32, ThreadLocalStoragePointer );

            SIZE_T  lenRead = 0;
            SIZE_T  lenUnreadable = 0;

            hr = mExec->ReadMemory( mProc, tlsPtrAddr, 4, lenRead, lenUnreadable, (uint8_t*) &tlsArrayAddr );
            if ( FAILED( hr ) )
                throw L"Can't get value for this decl.";
            if ( lenRead < 4 )
                throw L"Can't get value for this decl.";

            if ( (tlsArrayAddr == 0) || (tlsArrayAddr == tlsPtrAddr) )
            {
                // go to the reserved slots in the TEB
                tlsArrayAddr = tebAddr + offsetof( TEB32, TlsSlots );
            }

            // assuming TLS slot 0
            hr = mExec->ReadMemory( mProc, tlsArrayAddr, 4, lenRead, lenUnreadable, (uint8_t*) &tlsBufAddr );
            if ( FAILED( hr ) )
                throw L"Can't get value for this decl.";
            if ( lenRead < 4 )
                throw L"Can't get value for this decl.";

            addr = tlsBufAddr + offset;

            val = GetValue( addr, type, decl );
        }
        break;

    default:
        throw L"Can't get value for this decl.";
    }

    return val;
}

boost::shared_ptr<DataObj> ProgramValueEnv::GetValue( MagoEE::Address address, MagoEE::Type* type )
{
    return GetValue( address, type, NULL );
}

boost::shared_ptr<DataObj> ProgramValueEnv::GetValue( MagoEE::Address address, Type* type, MagoEE::Declaration* decl )
{
    HRESULT hr = S_OK;
    boost::shared_ptr<DataObj> val( new LValueObj( decl, address ) );

    val->SetType( type );

    uint8_t     targetBuf[ sizeof( MagoEE::DataValue ) ] = { 0 };
    size_t      targetSize = type->GetSize();
    SIZE_T      lenRead = 0;
    SIZE_T      lenUnreadable = 0;

    hr = mExec->ReadMemory( mProc, (Address) address, targetSize, lenRead, lenUnreadable, targetBuf );
    if ( FAILED( hr ) )
        throw L"Couldn't read memory.";
    if ( lenRead < targetSize )
        throw L"Couldn't read memory.";

    if ( type->IsPointer() )
    {
        val->Value.Addr = DataEnv::ReadInt( targetBuf, 0, targetSize, false );
    }
    else if ( type->IsIntegral() )
    {
        val->Value.UInt64Value = DataEnv::ReadInt( targetBuf, 0, targetSize, type->IsSigned() );
    }
    else if ( type->IsReal() || type->IsImaginary() )
    {
        val->Value.Float80Value = DataEnv::ReadFloat( targetBuf, 0, type );
    }
    else if ( type->IsComplex() )
    {
        val->Value.Complex80Value.RealPart = DataEnv::ReadFloat( targetBuf, 0, type );
        val->Value.Complex80Value.ImaginaryPart = DataEnv::ReadFloat( targetBuf, 0, type );
    }
    else if ( type->IsDArray() )
    {
        const size_t LengthSize = type->AsTypeDArray()->GetLengthType()->GetSize();
        const size_t AddressSize = type->AsTypeDArray()->GetPointerType()->GetSize();

        val->Value.Array.Length = (MagoEE::dlength_t) DataEnv::ReadInt( targetBuf, 0, LengthSize, false );
        memcpy( &val->Value.Array.Addr, targetBuf + LengthSize, AddressSize );
    }
    else if ( type->IsAArray() )
    {
        val->Value.Addr = DataEnv::ReadInt( targetBuf, 0, targetSize, false );
    }
    else if ( type->IsDelegate() )
    {
        const size_t AddressSize = mTypeEnv->GetVoidPointerType()->GetSize();

        val->Value.Delegate.ContextAddr = (MagoEE::dlength_t) DataEnv::ReadInt( targetBuf, 0, AddressSize, false );
        memcpy( &val->Value.Delegate.FuncAddr, targetBuf + AddressSize, AddressSize );
    }

    return val;
}

void ProgramValueEnv::SetValue( MagoEE::Address address, DataObj* obj )
{
    throw L"Not implemented: ProgramValueEnv::SetValue.";
}

RefPtr<MagoEE::Declaration> ProgramValueEnv::GetThis()
{
    HRESULT hr = S_OK;
    SymHandle   childSH;
    RefPtr<MagoEE::Declaration> decl;

    hr = mSymSession->FindChildSymbol( mBlockSH, "this", 4, childSH );
    if ( hr != S_OK )
        return NULL;

    hr = MakeDeclarationFromSymbol( mSymSession, childSH, mTypeEnv, decl.Ref() );
    if ( FAILED( hr ) )
        return NULL;

    return decl;
}

RefPtr<MagoEE::Declaration> ProgramValueEnv::GetSuper()
{
    _ASSERT( false );
    return NULL;
}

bool ProgramValueEnv::GetArrayLength( MagoEE::dlength_t& length )
{
    return false;
}

HRESULT ProgramValueEnv::LoadSymbols( DWORD64 loadAddr )
{
    HRESULT hr = S_OK;
    RefPtr<IDataSource>     dataSource;
    RefPtr<ISession>        session;
    SymHandle               funcSym = { 0 };
    SymHandle               blockSym = { 0 };
    SymInfoData             infoData = { 0 };
    ISymbolInfo*            symInfo = NULL;

    hr = MakeDataSource( dataSource.Ref() );
    if ( FAILED( hr ) )
        return hr;

    hr = dataSource->LoadDataForExe( mProgPath.c_str(), NULL, NULL );
    if ( FAILED( hr ) )
        return hr;

    hr = dataSource->InitDebugInfo();
    if ( FAILED( hr ) )
        return hr;

    hr = dataSource->OpenSession( session.Ref() );
    if ( FAILED( hr ) )
        return hr;

    session->SetLoadAddress( loadAddr );

    mSymSession = session.Detach();

    hr = FindSymbolByRVA( mStopRva, funcSym, blockSym );
    if ( FAILED( hr ) )
        return hr;

    mFuncSH = funcSym;
    mBlockSH = blockSym;

    hr = mSymSession->GetSymbolInfo( funcSym, infoData, symInfo );
    if ( hr == S_OK )
    {
        symInfo->GetThis( mThisTI );
        // TODO: if it has a this type, then there's a this var. Look for it
    }

    return S_OK;
}

HRESULT ProgramValueEnv::FindSymbolByRVA( DWORD rva, MagoST::SymHandle& handle, MagoST::SymHandle& innermostChild )
{
    HRESULT     hr = S_OK;
    uint16_t    sec = 0;
    uint32_t    offset = 0;

    sec = mSymSession->GetSecOffsetFromRVA( rva, offset );
    if ( sec == 0 )
        return E_FAIL;

    hr = FindOuterSymbolByRVA( rva, handle );
    if ( hr != S_OK )
        return E_NOT_FOUND;

    hr = mSymSession->FindInnermostSymbol( handle, sec, offset, innermostChild );
    if ( hr != S_OK )
        innermostChild = handle;

    return S_OK;
}

HRESULT ProgramValueEnv::FindOuterSymbolByRVA( DWORD rva, MagoST::SymHandle& handle )
{
    HRESULT hr = S_OK;

    hr = mSymSession->FindOuterSymbolByRVA( SymHeap_GlobalSymbols, rva, handle );
    if ( hr == S_OK )
        return S_OK;

    hr = mSymSession->FindOuterSymbolByRVA( SymHeap_StaticSymbols, rva, handle );
    if ( hr == S_OK )
        return S_OK;

    hr = mSymSession->FindOuterSymbolByRVA( SymHeap_PublicSymbols, rva, handle );
    if ( hr == S_OK )
        return S_OK;

    return E_NOT_FOUND;
}

    // TODO: these declarations can be cached. We can keep a small cache for each module
HRESULT ProgramValueEnv::MakeDeclarationFromSymbol( MagoST::ISession* session, MagoST::SymHandle handle, MagoEE::ITypeEnv* typeEnv, MagoEE::Declaration*& decl )
{
    HRESULT hr = S_OK;
    SymTag  tag = MagoST::SymTagNull;
    SymInfoData     infoData = { 0 };
    ISymbolInfo*    symInfo = NULL;

    hr = session->GetSymbolInfo( handle, infoData, symInfo );
    if ( FAILED( hr ) )
        return hr;

    tag = symInfo->GetSymTag();

    switch ( tag )
    {
    case SymTagData:
    case SymTagFunction:
        hr = MakeDeclarationFromDataSymbol( session, infoData, symInfo, typeEnv, decl );
        break;

    case SymTagTypedef:
        hr = MakeDeclarationFromTypedefSymbol( session, infoData, symInfo, typeEnv, decl );
        break;

        //hr = MakeDeclarationFromFunctionSymbol( session, infoData, symInfo, typeEnv, decl );
        break;

    default:
        return E_FAIL;
    }

    return hr;
}

HRESULT ProgramValueEnv::MakeDeclarationFromFunctionSymbol( 
    MagoST::ISession* session, 
    const MagoST::SymInfoData& infoData, 
    MagoST::ISymbolInfo* symInfo, 
    MagoEE::ITypeEnv* typeEnv, 
    MagoEE::Declaration*& decl )
{
    HRESULT         hr = S_OK;
    TypeIndex       typeIndex = 0;
    SymString       pstrName1;
    TypeHandle      handle = { 0 };
    SymInfoData     typeInfoData = { 0 };
    ISymbolInfo*    typeInfo = NULL;
    SymString       pstrName2;
    RefPtr<Type>    refType;

    return E_NOTIMPL;
}

HRESULT ProgramValueEnv::MakeDeclarationFromTypedefSymbol( 
    MagoST::ISession* session, 
    const MagoST::SymInfoData& infoData, 
    MagoST::ISymbolInfo* symInfo, 
    MagoEE::ITypeEnv* typeEnv, 
    MagoEE::Declaration*& decl )
{
    HRESULT         hr = S_OK;
    TypeIndex       typeIndex = 0;
    SymString       pstrName1;
    TypeHandle      handle = { 0 };
    SymInfoData     typeInfoData = { 0 };
    ISymbolInfo*    typeInfo = NULL;
    SymString       pstrName2;
    RefPtr<Type>    refType;

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

    refType = GetTypeFromTypeSymbol( session, typeIndex, typeEnv );

	if ( (pstrName2.GetName() != NULL)
        && (pstrName1.GetLength() == pstrName2.GetLength()) 
        && (strncmp( pstrName1.GetName(), pstrName2.GetName(), pstrName1.GetLength() ) == 0) )
    {
        // the typedef has the same name as the type, 
        // so let's use the referenced type directly, as if there's no typedef

        decl = refType->GetDeclaration();
        decl->AddRef();
    }
    else
    {
        RefPtr<GeneralDiaDecl>  diaDecl;
        RefPtr<Type>            type;

        diaDecl = new GeneralDiaDecl( session, infoData, symInfo, typeEnv );

        hr = typeEnv->NewTypedef( diaDecl->GetName(), refType, type.Ref() );
        if ( FAILED( hr ) )
            return hr;

        diaDecl->SetType( type );

        decl = diaDecl.Detach();
    }

    return S_OK;
}

HRESULT ProgramValueEnv::MakeDeclarationFromSymbol( MagoST::ISession* session, MagoST::TypeHandle handle, MagoEE::ITypeEnv* typeEnv, MagoEE::Declaration*& decl )
{
    HRESULT hr = S_OK;
    SymTag  tag = MagoST::SymTagNull;
    SymInfoData     infoData = { 0 };
    ISymbolInfo*    symInfo = NULL;

    hr = session->GetTypeInfo( handle, infoData, symInfo );
    if ( FAILED( hr ) )
        return hr;

    tag = symInfo->GetSymTag();

    switch ( tag )
    {
    case SymTagData:
        hr = MakeDeclarationFromDataSymbol( session, infoData, symInfo, typeEnv, decl );
        break;

    default:
        return E_FAIL;
    }

    return hr;
}

MagoEE::ENUMTY ProgramValueEnv::GetBasicTy( DWORD diaBaseTypeId, DWORD size )
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

HRESULT ProgramValueEnv::MakeDeclarationFromDataSymbol( 
    MagoST::ISession* session, 
    const MagoST::SymInfoData& infoData,
    MagoST::ISymbolInfo* symInfo, 
    MagoEE::ITypeEnv* typeEnv, 
    MagoEE::Declaration*& decl )
{
    HRESULT         hr = S_OK;
    TypeIndex       typeIndex = 0;
    RefPtr<Type>    type;
    RefPtr<GeneralDiaDecl> diaDecl;

    if ( !symInfo->GetType( typeIndex ) )
        return E_FAIL;

    type = GetTypeFromTypeSymbol( session, typeIndex, typeEnv );
    if ( type == NULL )
        return E_FAIL;

    hr = MakeDeclarationFromDataSymbol( session, infoData, symInfo, typeEnv, type, (MagoEE::Declaration*&) diaDecl.Ref() );
    if ( FAILED( hr ) )
        return hr;

    decl = diaDecl.Detach();
    return S_OK;
}

HRESULT ProgramValueEnv::MakeDeclarationFromDataSymbol( 
    MagoST::ISession* session, 
    const MagoST::SymInfoData& infoData,
    MagoST::ISymbolInfo* symInfo, 
    MagoEE::ITypeEnv* typeEnv, 
    MagoEE::Type* type,
    MagoEE::Declaration*& decl )
{
    LocationType    loc;
    RefPtr<GeneralDiaDecl> diaDecl;

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
        diaDecl = new GeneralDiaDecl( session, infoData, symInfo, typeEnv );
        break;

    default:
        return E_FAIL;
    }

    diaDecl->SetType( type );

    decl = diaDecl.Detach();
    return S_OK;
}

#if 0
HRESULT ProgramValueEnv::MakeDeclarationFromDataSymbol1( 
    MagoST::ISession* session, 
    const MagoST::SymInfoData& infoData,
    MagoST::ISymbolInfo* symInfo, 
    MagoEE::ITypeEnv* typeEnv, 
    MagoEE::Type* type,
    MagoEE::Declaration*& decl )
{
    HRESULT hr = S_OK;
    LocationType    loc;
    TypeIndex       typeIndex = 0;
    RefPtr<GeneralDiaDecl> diaDecl;

    if ( !symInfo->GetLocation( loc ) )
        return E_FAIL;

    if ( !symInfo->GetType( typeIndex ) )
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
        diaDecl = new GeneralDiaDecl( session, infoData, symInfo, typeEnv );
        break;

    default:
        return E_FAIL;
    }

    RefPtr<Type>    type;

    type = GetTypeFromTypeSymbol( session, typeIndex, typeEnv );
    if ( type == NULL )
        return E_FAIL;

    diaDecl->SetType( type );

    decl = diaDecl.Detach();
    return S_OK;
}
#endif

RefPtr<Type> ProgramValueEnv::GetTypeFromTypeSymbol( 
    MagoST::ISession* session, 
    TypeIndex typeIndex,
    MagoEE::ITypeEnv* typeEnv )
{
    HRESULT hr = S_OK;
    SymTag  tag = SymTagNull;
    TypeHandle      type = { 0 };
    SymInfoData     infoData = { 0 };
    ISymbolInfo*    symInfo = NULL;

    if ( !session->GetTypeFromTypeIndex( typeIndex, type ) )
        return NULL;

    hr = session->GetTypeInfo( type, infoData, symInfo );
    if ( FAILED( hr ) )
        return NULL;

    tag = symInfo->GetSymTag();

    switch ( tag )
    {
    case SymTagUDT:
        return GetUdtTypeFromTypeSymbol( session, type, infoData, symInfo, typeEnv );

    case SymTagEnum:
        return GetUdtTypeFromTypeSymbol( session, type, infoData, symInfo, typeEnv );

    case SymTagFunctionType:
        return GetFunctionTypeFromTypeSymbol( session, type, infoData, symInfo, typeEnv );

    case SymTagBaseType:
        return GetBasicTypeFromTypeSymbol( infoData, symInfo, typeEnv );

    case SymTagPointerType:
        {
            RefPtr<Type>        pointed;
            RefPtr<Type>        pointer;
            MagoST::TypeIndex   pointedTypeIndex = 0;

            if ( !symInfo->GetType( pointedTypeIndex ) )
                throw L"Can't get type.";

            pointed = GetTypeFromTypeSymbol( session, pointedTypeIndex, typeEnv );
            if ( pointed == NULL )
                throw L"Can't get type.";

            hr = typeEnv->NewPointer( pointed, pointer.Ref() );
            if ( hr != S_OK )
                throw L"Can't get type.";

            return pointer;
        }
        break;

    case SymTagArrayType:
        {
            RefPtr<Type>        elemType;
            RefPtr<Type>        arrayType;
            uint32_t            count = 0;
            MagoST::TypeIndex   elemTypeIndex = 0;

            if ( !symInfo->GetType( elemTypeIndex ) )
                throw L"Can't get type.";

            if ( !symInfo->GetCount( count ) )
                throw L"Can't get type.";

            elemType = GetTypeFromTypeSymbol( session, elemTypeIndex, typeEnv );
            if ( elemType == NULL )
                throw L"Can't get type.";

            hr = typeEnv->NewSArray( elemType, count, arrayType.Ref() );
            if ( hr != S_OK )
                throw L"Can't get type.";

            return arrayType;
        }
        break;

    case SymTagTypedef:
        _ASSERT( false );
        break;

    case SymTagCustomType:
        return GetCustomTypeFromTypeSymbol( session, infoData, symInfo, typeEnv );

    //case SymTagManagedType:
    default:
        return NULL;
    }

    return NULL;
}

RefPtr<Type> ProgramValueEnv::GetFunctionTypeFromTypeSymbol( 
    MagoST::ISession* session, 
    MagoST::TypeHandle typeHandle,
    const MagoST::SymInfoData& infoData,
    MagoST::ISymbolInfo* symInfo,
    MagoEE::ITypeEnv* typeEnv )
{
    HRESULT hr = S_OK;
    RefPtr<Type>    type;
    RefPtr<MagoEE::ParameterList>   params;
    TypeIndex       retTI = 0;
    RefPtr<Type>    retType;
    uint32_t        paramCount = 0;
    TypeIndex       paramListTI = 0;
    TypeHandle      paramListTH = { 0 };
    SymInfoData     paramListInfoData = { 0 };
    ISymbolInfo*    paramListInfo = NULL;
    std::vector<TypeIndex> paramTIs;

    if ( !symInfo->GetType( retTI ) )
        throw L"Couldn't get return type.";

    retType = GetTypeFromTypeSymbol( session, retTI, typeEnv );

    hr = typeEnv->NewParams( params.Ref() );
    if ( FAILED( hr ) )
        throw L"Couldn't make new param list.";

    if ( !symInfo->GetParamList( paramListTI ) )
        throw L"Couldn't get param list.";

    if ( !session->GetTypeFromTypeIndex( paramListTI, paramListTH ) )
        throw L"Couldn't get param list.";

    hr = session->GetTypeInfo( paramListTH, paramListInfoData, paramListInfo );
    if ( FAILED( hr ) )
        throw L"Couldn't get param list info.";

    if ( !paramListInfo->GetCount( paramCount ) )
        throw L"Couldn't get param count.";

    if ( !paramListInfo->GetTypes( paramTIs ) )
        throw L"Couldn't get param types.";

    for ( uint32_t i = 0; i < paramCount; i++ )
    {
        RefPtr<Type>    paramType;
        RefPtr<MagoEE::Parameter>   param;

        paramType = GetTypeFromTypeSymbol( session, paramTIs[i], typeEnv );

        param = new MagoEE::Parameter( 0, paramType );

        params->List.push_back( param );
    }

    // TODO: calling convention/var args
    hr = typeEnv->NewFunction( retType, params, 0, type.Ref() );
    if ( FAILED( hr ) )
        throw L"Couldn't make new function type.";

    return type;
}

RefPtr<Type> ProgramValueEnv::GetUdtTypeFromTypeSymbol( 
    MagoST::ISession* session, 
    MagoST::TypeHandle typeHandle,
    const MagoST::SymInfoData& infoData,
    MagoST::ISymbolInfo* symInfo,
    MagoEE::ITypeEnv* typeEnv )
{
    HRESULT hr = S_OK;
    RefPtr<MagoEE::Declaration> decl;
    RefPtr<Type>    type;

    decl = new TypeDiaDecl( session, typeHandle, infoData, symInfo, typeEnv );

    if ( !decl->GetType( type.Ref() ) )
        throw L"Couldn't get type.";

    return type;
}

RefPtr<Type> ProgramValueEnv::GetBasicTypeFromTypeSymbol( 
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo,
        MagoEE::ITypeEnv* typeEnv )
{
    HRESULT     hr = S_OK;
    DWORD       baseTypeId = 0;
    uint32_t    size = 0;
    MagoEE::ENUMTY  ty = MagoEE::Tnone;

    if ( !symInfo->GetBasicType( baseTypeId ) )
        return NULL;

    if ( !symInfo->GetLength( size ) )
        return NULL;

    ty = GetBasicTy( baseTypeId, size );
    if ( ty == MagoEE::Tnone )
        throw L"Basic type not found.";

    return typeEnv->GetType( ty );
}

RefPtr<Type> ProgramValueEnv::GetCustomTypeFromTypeSymbol( 
    MagoST::ISession* session, 
    const MagoST::SymInfoData& infoData, 
    MagoST::ISymbolInfo* symInfo,
    MagoEE::ITypeEnv* typeEnv )
{
    HRESULT     hr = S_OK;
    uint32_t    count = 0;
    uint32_t    oemId = 0;
    uint32_t    oemSymId = 0;

    if ( !symInfo->GetOemId( oemId ) )
        throw L"Can't get type.";
    if ( oemId != DMD_OEM )
        throw L"Can't get type.";

    if ( !symInfo->GetOemSymbolId( oemSymId ) )
        throw L"Can't get type.";

    if ( !symInfo->GetCount( count ) )
        throw L"Can't get type.";

    switch ( oemSymId )
    {
    case DMD_OEM_DARRAY:
    case DMD_OEM_AARRAY:
    case DMD_OEM_DELEGATE:
        {
            RefPtr<Type>        types[2];
            RefPtr<Type>        type;
            std::vector<TypeIndex> typeIndexes;

            if ( count != 2 )
                throw L"Can't get type.";

            if ( !symInfo->GetTypes( typeIndexes ) )
                throw L"Can't get type.";

            for ( DWORD i = 0; i < count; i++ )
                types[i] = GetTypeFromTypeSymbol( session, typeIndexes[i], typeEnv );

            switch ( oemSymId )
            {
            case DMD_OEM_DARRAY:
                hr = typeEnv->NewDArray( types[1], type.Ref() );
                break;

            case DMD_OEM_AARRAY:
                hr = typeEnv->NewAArray( types[1], types[0], type.Ref() );
                break;

            case DMD_OEM_DELEGATE:
                hr = typeEnv->NewDelegate( types[1], type.Ref() );
                break;
            }

            if ( hr != S_OK )
                throw L"Can't get type.";

            return type;
        }
        break;

    default:
        throw L"Can't get type.";
    }
}

HRESULT ProgramValueEnv::GetRegValue( DWORD reg, MagoEE::DataValueKind& kind, MagoEE::DataValue& value )
{
    // TODO: this should come from Exec or somewhere else

    CONTEXT         context = { 0 };
    RefPtr<Thread>  thread;
    HANDLE          hThread = NULL;

    if ( !mProc->FindThread( mThreadId, thread.Ref() ) )
        return E_FAIL;

    hThread = thread->GetHandle();

    context.ContextFlags = CONTEXT_ALL;

    if ( !GetThreadContext( hThread, &context ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    switch ( reg )
    {
    case CV_REG_AL:     value.UInt64Value = (context.Eax) & 0xFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_CL:     value.UInt64Value = (context.Ecx) & 0xFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_DL:     value.UInt64Value = (context.Edx) & 0xFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_BL:     value.UInt64Value = (context.Ebx) & 0xFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_AH:     value.UInt64Value = (context.Eax >> 8) & 0xFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_CH:     value.UInt64Value = (context.Ecx >> 8) & 0xFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_DH:     value.UInt64Value = (context.Edx >> 8) & 0xFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_BH:     value.UInt64Value = (context.Ebx >> 8) & 0xFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_AX:     value.UInt64Value = context.Eax & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_CX:     value.UInt64Value = context.Ecx & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_DX:     value.UInt64Value = context.Edx & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_BX:     value.UInt64Value = context.Ebx & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_SP:     value.UInt64Value = context.Esp & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_BP:     value.UInt64Value = context.Ebp & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_SI:     value.UInt64Value = context.Esi & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_DI:     value.UInt64Value = context.Edi & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_EAX:     value.UInt64Value = context.Eax;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_ECX:     value.UInt64Value = context.Ecx;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_EDX:     value.UInt64Value = context.Edx;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_EBX:     value.UInt64Value = context.Ebx;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_ESP:     value.UInt64Value = context.Esp;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_EBP:     value.UInt64Value = context.Ebp;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_ESI:     value.UInt64Value = context.Esi;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_EDI:     value.UInt64Value = context.Edi;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_ES:     value.UInt64Value = context.SegEs;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_CS:     value.UInt64Value = context.SegCs;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_SS:     value.UInt64Value = context.SegSs;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_DS:     value.UInt64Value = context.SegDs;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_FS:     value.UInt64Value = context.SegFs;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_GS:     value.UInt64Value = context.SegGs;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_IP:     value.UInt64Value = context.Eip & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_FLAGS:     value.UInt64Value = context.EFlags & 0xFFFF;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_EIP:     value.UInt64Value = context.Eip;   kind = MagoEE::DataValueKind_UInt64;    break;
    case CV_REG_EFLAGS:     value.UInt64Value = context.EFlags;   kind = MagoEE::DataValueKind_UInt64;    break;

    case CV_REG_ST0:
    case CV_REG_ST1:
    case CV_REG_ST2:
    case CV_REG_ST3:
    case CV_REG_ST4:
    case CV_REG_ST5:
    case CV_REG_ST6:
    case CV_REG_ST7:
        {
            int index = reg - CV_REG_ST0;
            memcpy( &value.Float80Value, &context.FloatSave.RegisterArea[ index * 10 ], 10 );
            kind = MagoEE::DataValueKind_Float80;
        }
        break;

    case CV_REG_CTRL:   value.UInt64Value = context.FloatSave.ControlWord;  kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_STAT:   value.UInt64Value = context.FloatSave.StatusWord;   kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_TAG:    value.UInt64Value = context.FloatSave.TagWord;      kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_FPIP:   value.UInt64Value = 0;             kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_FPCS:   value.UInt64Value = context.FloatSave.Cr0NpxState;  kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_FPDO:   value.UInt64Value = context.FloatSave.DataOffset;   kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_FPDS:   value.UInt64Value = context.FloatSave.DataSelector; kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_ISEM:   value.UInt64Value = 0;             kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_FPEIP:  value.UInt64Value = 0;             kind = MagoEE::DataValueKind_UInt64;  break;
    case CV_REG_FPEDO:  value.UInt64Value = context.FloatSave.ErrorOffset;  kind = MagoEE::DataValueKind_UInt64;  break;

    case CV_REG_EDXEAX:
        value.UInt64Value = context.Edx;
        value.UInt64Value <<= 32;
        value.UInt64Value |= context.Eax;
        kind = MagoEE::DataValueKind_UInt64;
        break;
    }

    return S_OK;
}

HRESULT ProgramValueEnv::GetSegBase( DWORD segReg, DWORD& base )
{
    LDT_ENTRY   ldt = { 0 };
    DWORD       bytesRead = 0;
    BOOL        bRet = FALSE;
    RefPtr<Thread>  thread;
    HANDLE          hThread = NULL;

    if ( !mProc->FindThread( mThreadId, thread.Ref() ) )
        return E_FAIL;

    hThread = thread->GetHandle();

    bRet = GetThreadSelectorEntry( hThread, segReg, &ldt );
    if ( !bRet )
        return HRESULT_FROM_WIN32( GetLastError() );

    base = ldt.BaseLow | (ldt.HighWord.Bytes.BaseMid << 16) | (ldt.HighWord.Bytes.BaseHi << 24);
    return S_OK;
}
