/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DiaDecls.h"
#include "ProgValueEnv.h"
#include <MagoCVConst.h>

using MagoEE::Type;
using namespace MagoST;


//----------------------------------------------------------------------------
//  DiaDecl
//----------------------------------------------------------------------------

DiaDecl::DiaDecl( 
                   MagoST::ISession* session, 
                   const MagoST::SymInfoData& infoData, 
                   MagoST::ISymbolInfo* symInfo, 
                   MagoEE::ITypeEnv* typeEnv )
:   mRefCount( 0 ),
    mSession( session ),
    //mSymInfoData( infoData ),
    //mSymInfo( symInfo ),
    mTypeEnv( typeEnv )
{
    session->CopySymbolInfo( infoData, mSymInfoData, mSymInfo );
}

MagoST::ISymbolInfo* DiaDecl::GetSymbol()
{
    return mSymInfo;
}

void DiaDecl::AddRef()
{
    InterlockedIncrement( &mRefCount );
}

void DiaDecl::Release()
{
    InterlockedDecrement( &mRefCount );
    _ASSERT( mRefCount >= 0 );
    if ( mRefCount == 0 )
        delete this;
}

const wchar_t* DiaDecl::GetName()
{
    if ( mName == NULL )
    {
        SymString   pstrName;
        int         nChars = 0;
        BSTR        bstrName = NULL;
        CComBSTR    ccomBstrName;

        if ( !mSymInfo->GetName( pstrName ) )
            return NULL;

        nChars = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, pstrName.GetName(), pstrName.GetLength(), NULL, 0 );
        if ( nChars == 0 )
            return NULL;

        bstrName = SysAllocStringLen( NULL, nChars );       // allocates 1 more char
        if ( bstrName == NULL )
            return NULL;

        ccomBstrName.Attach( bstrName );

        nChars = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, pstrName.GetName(), pstrName.GetLength(), ccomBstrName.m_str, nChars + 1 );
        if ( nChars == 0 )
            return NULL;
        ccomBstrName.m_str[nChars] = L'\0';

        mName.Attach( ccomBstrName.Detach() );
    }

    return mName;
}

bool DiaDecl::GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder )
{
    HRESULT     hr = S_OK;
    uint32_t    offset = 0;
    uint16_t    sec = 0;

    if ( !mSymInfo->GetAddressOffset( offset ) )
        return false;
    if ( !mSymInfo->GetAddressSegment( sec ) )
        return false;

    addr = mSession->GetVAFromSecOffset( sec, offset );
    if ( addr == 0 )
        return false;

    return true;
}

bool DiaDecl::GetOffset( int& offset )
{
    return mSymInfo->GetOffset( offset );
}

bool DiaDecl::GetSize( uint32_t& size )
{
    return false;
}

bool DiaDecl::GetBackingTy( MagoEE::ENUMTY& ty )
{
    return false;
}

bool DiaDecl::GetUdtKind( MagoEE::UdtKind& kind )
{
    MagoST::UdtKind cvUdtKind;

    if ( !mSymInfo->GetUdtKind( cvUdtKind ) )
        return false;

    switch ( cvUdtKind )
    {
    case MagoST::UdtStruct: kind = MagoEE::Udt_Struct;  break;
    case MagoST::UdtClass:  kind = MagoEE::Udt_Class;   break;
    case MagoST::UdtUnion:  kind = MagoEE::Udt_Union;   break;
    default:
        _ASSERT( false );
        kind = MagoEE::Udt_Struct;
        break;
    }

    return true;
}

bool DiaDecl::GetBaseClassOffset( Declaration* baseClass, int& offset )
{
    return false;
}

bool DiaDecl::GetVTableShape( Declaration*& decl )
{
    return false;
}

bool DiaDecl::GetVtblOffset( int& offset )
{
    return false;
}

bool DiaDecl::GetBitfieldRange( uint32_t& position, uint32_t& length )
{
    // TODO
    return false;
}

bool DiaDecl::IsField()
{
    MagoST::DataKind   kind = MagoST::DataIsUnknown;

    if ( !mSymInfo->GetDataKind( kind ) )
        return false;

    return kind == DataIsMember;
}

bool DiaDecl::IsBitField()
{
    // TODO
    return false;
}

bool DiaDecl::IsVar()
{
    MagoST::DataKind   kind = MagoST::DataIsUnknown;

    if ( !mSymInfo->GetDataKind( kind ) )
        return false;

    switch ( kind )
    {
    case DataIsFileStatic:
    case DataIsGlobal:
    case DataIsLocal:
    case DataIsObjectPtr:
    case DataIsParam:
    case DataIsStaticLocal:
    case DataIsStaticMember:
        return true;

    // note that these are not vars:
    // case DataIsConstant:
    // case DataIsMember:
    }

    return false;
}

bool DiaDecl::IsConstant()
{
    MagoST::DataKind   kind = MagoST::DataIsUnknown;

    if ( !mSymInfo->GetDataKind( kind ) )
        return false;

    return kind == DataIsConstant;
}

bool DiaDecl::IsType()
{
    MagoST::SymTag   tag = MagoST::SymTagNull;

    tag = mSymInfo->GetSymTag();

    switch ( tag )
    {
    case SymTagUDT:
    case SymTagEnum:
    case SymTagFunctionType:
    case SymTagPointerType:
    case SymTagArrayType:
    case SymTagBaseType:
    case SymTagTypedef:
    case SymTagCustomType:
    case SymTagManagedType:
    case MagoST::SymTagNestedType:
        return true;
    }

    return false;
}

bool DiaDecl::IsBaseClass()
{
    return mSymInfo->GetSymTag() == MagoST::SymTagBaseClass;
}

bool DiaDecl::IsStaticField()
{
    return false;
}

bool DiaDecl::IsRegister()
{
    return false;
}

bool DiaDecl::IsFunction()
{
    return false;
}

bool DiaDecl::IsStaticFunction()
{
    return false;
}

HRESULT DiaDecl::FindObject( const wchar_t* name, Declaration*& decl )
{
    return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
}

bool DiaDecl::EnumMembers( MagoEE::IEnumDeclarationMembers*& members )
{
    return false;
}

HRESULT DiaDecl::FindObjectByValue( uint64_t intVal, Declaration*& decl )
{
    return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
}


//----------------------------------------------------------------------------
//  GeneralDiaDecl
//----------------------------------------------------------------------------

GeneralDiaDecl::GeneralDiaDecl( 
        MagoST::ISession* session, 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv )
:   DiaDecl( session, infoData, symInfo, typeEnv )
{
}

bool GeneralDiaDecl::GetType( Type*& type )
{
    if ( mType == NULL )
        return false;

    type = mType;
    type->AddRef();
    return true;
}

void GeneralDiaDecl::SetType( Type* type )
{
    mType = type;
}


//----------------------------------------------------------------------------
//  TypeDiaDecl
//----------------------------------------------------------------------------

TypeDiaDecl::TypeDiaDecl( 
        MagoST::ISession* session, 
        MagoST::TypeHandle typeHandle,
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv )
:   DiaDecl( session, infoData, symInfo, typeEnv ),
    mTypeHandle( typeHandle )
{
}

bool TypeDiaDecl::GetType( Type*& type )
{
    HRESULT hr = S_OK;
    MagoST::SymTag  tag = MagoST::SymTagNull;

    tag = mSymInfo->GetSymTag();

    switch ( tag )
    {
    case SymTagUDT:
        {
            MagoST::UdtKind    kind = MagoST::UdtStruct;

            if ( !mSymInfo->GetUdtKind( kind ) )
                return false;

            switch ( kind )
            {
            case UdtStruct:
            case UdtUnion:
            case UdtClass:
                hr = mTypeEnv->NewStruct( this, type );
                break;

            //case UdtClass:
                // TODO:
                _ASSERT( false );
            default:
                return false;
            }

            if ( FAILED( hr ) )
                return false;
        }
        break;

    case SymTagEnum:
        hr = mTypeEnv->NewEnum( this, type );
        if ( FAILED( hr ) )
            return false;
        break;

    case SymTagFunctionType:
    case SymTagPointerType:
    case SymTagArrayType:
    case SymTagBaseType:
    case SymTagTypedef:
    case SymTagCustomType:
    case SymTagManagedType:
        return false;

    default:
        return false;
    }

    return true;
}

bool TypeDiaDecl::GetSize( uint32_t& size )
{
    return mSymInfo->GetLength( size );
}

bool TypeDiaDecl::GetBackingTy( MagoEE::ENUMTY& ty )
{
    HRESULT         hr = S_OK;
    SymTag          tag = SymTagNull;
    DWORD           baseTypeId = 0;
    uint32_t        size = 0;
    TypeIndex       backingTypeIndex = 0;
    TypeHandle      backingTypeHandle = { 0 };
    SymInfoData     infoData = { 0 };
    ISymbolInfo*    symInfo = NULL;

    // TODO: should work for more than just enums

    tag = mSymInfo->GetSymTag();

    if ( tag != SymTagEnum )
        return false;

    if ( !mSymInfo->GetType( backingTypeIndex ) )
        return false;

    if ( !mSession->GetTypeFromTypeIndex( backingTypeIndex, backingTypeHandle ) )
        return false;

    hr = mSession->GetTypeInfo( backingTypeHandle, infoData, symInfo );
    if ( FAILED( hr ) )
        return false;

    if ( !symInfo->GetBasicType( baseTypeId ) )
        return false;

    if ( !symInfo->GetLength( size ) )
        return false;

    ty = ProgramValueEnv::GetBasicTy( baseTypeId, size );
    if ( ty == MagoEE::Tnone )
        return false;

    return true;
}

HRESULT TypeDiaDecl::FindObject( const wchar_t* name, Declaration*& decl )
{
    HRESULT             hr = S_OK;
    int                 nzChars = 0;
    UniquePtr<char[]>   nameChars;
    MagoST::TypeHandle  childTH = { 0 };
    MagoST::TypeIndex   flistIndex = 0;
    MagoST::TypeHandle  flistHandle = { 0 };
    DWORD               flags = 0;

#if _WIN32_WINNT >= _WIN32_WINNT_LONGHORN
    flags = WC_ERR_INVALID_CHARS;
#endif

    // TODO: what about the superclasses?

    nzChars = WideCharToMultiByte( CP_UTF8, flags, name, -1, NULL, 0, NULL, NULL );
    if ( nzChars == 0 )
        return HRESULT_FROM_WIN32( GetLastError() );

    nameChars.Attach( new char[ nzChars ] );
    if ( nameChars.Get() == NULL )
        return E_OUTOFMEMORY;

    nzChars = WideCharToMultiByte( CP_UTF8, flags, name, -1, nameChars.Get(), nzChars, NULL, NULL );
    if ( nzChars == 0 )
        return HRESULT_FROM_WIN32( GetLastError() );


    if ( !mSymInfo->GetFieldList( flistIndex ) )
        return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );


    for ( ; ; )
    {
        if ( !mSession->GetTypeFromTypeIndex( flistIndex, flistHandle ) )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        hr = mSession->FindChildType( flistHandle, nameChars.Get(), nzChars-1, childTH );
        if ( hr == S_OK )
            break;

        TypeScope       scope = { 0 };
        TypeHandle      baseTH = { 0 };
        SymInfoData     baseInfoData = { 0 };
        ISymbolInfo*    baseInfo = NULL;
        TypeIndex       baseClassTI = 0;
        TypeHandle      baseClassTH = { 0 };
        SymInfoData     baseClassInfoData = { 0 };
        ISymbolInfo*    baseClassInfo = NULL;

        hr = mSession->SetChildTypeScope( flistHandle, scope );
        if ( hr != S_OK )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        if ( !mSession->NextType( scope, baseTH ) )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        hr = mSession->GetTypeInfo( baseTH, baseInfoData, baseInfo );
        if ( hr != S_OK )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        if ( baseInfo->GetSymTag() != MagoST::SymTagBaseClass )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        if ( !baseInfo->GetType( baseClassTI ) )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        if ( !mSession->GetTypeFromTypeIndex( baseClassTI, baseClassTH ) )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        hr = mSession->GetTypeInfo( baseClassTH, baseClassInfoData, baseClassInfo );
        if ( hr != S_OK )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        if ( !baseClassInfo->GetFieldList( flistIndex ) )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
    }


    if ( GetTag() == SymTagEnum )
    {
        SymInfoData     childInfoData = { 0 };
        ISymbolInfo*    childSymInfo = NULL;
        RefPtr<Type>    type;

        hr = mSession->GetTypeInfo( childTH, childInfoData, childSymInfo );
        if ( FAILED( hr ) )
            return hr;

        if ( !GetType( type.Ref() ) )
            return E_FAIL;

        hr = ProgramValueEnv::MakeDeclarationFromDataSymbol( mSession, childInfoData, childSymInfo, mTypeEnv, type, decl );
        if ( FAILED( hr ) )
            return hr;
    }
    else
    {
        hr = ProgramValueEnv::MakeDeclarationFromSymbol( mSession, childTH, mTypeEnv, decl );
        if ( FAILED( hr ) )
            return hr;
    }

    return S_OK;
}

bool TypeDiaDecl::EnumMembers( MagoEE::IEnumDeclarationMembers*& members )
{
    return false;
}

SymTag TypeDiaDecl::GetTag()
{
    HRESULT         hr = S_OK;
    SymInfoData     infoData = { 0 };
    ISymbolInfo*    symInfo = NULL;

    hr = mSession->GetTypeInfo( mTypeHandle, infoData, symInfo );
    if ( FAILED( hr ) )
        return SymTagNull;

    return symInfo->GetSymTag();
}
