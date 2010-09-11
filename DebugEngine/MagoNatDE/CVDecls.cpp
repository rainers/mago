/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "CVDecls.h"
#include "ExprContext.h"
#include <MagoCVConst.h>

using namespace MagoST;


namespace Mago
{
//----------------------------------------------------------------------------
//  CVDecl
//----------------------------------------------------------------------------

    CVDecl::CVDecl( 
        ExprContext* symStore,
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo )
        :   mRefCount( 0 ),
            mSymStore( symStore )
    {
        _ASSERT( symStore != NULL );
        _ASSERT( symInfo != NULL );

        HRESULT hr = S_OK;

        hr = symStore->GetSession( mSession.Ref() );
        _ASSERT( hr == S_OK );

        mSession->CopySymbolInfo( infoData, mSymInfoData, mSymInfo );
        mTypeEnv = symStore->GetTypeEnv();
    }

    CVDecl::~CVDecl()
    {
    }

    MagoST::ISymbolInfo* CVDecl::GetSymbol()
    {
        return mSymInfo;
    }

    void CVDecl::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void CVDecl::Release()
    {
        InterlockedDecrement( &mRefCount );
        _ASSERT( mRefCount >= 0 );
        if ( mRefCount == 0 )
            delete this;
    }

    const wchar_t* CVDecl::GetName()
    {
        if ( mName == NULL )
        {
            HRESULT     hr = S_OK;
            PasString*  pstrName = NULL;

            if ( !mSymInfo->GetName( pstrName ) )
                return NULL;

            hr = Utf8To16( pstrName->name, pstrName->len, mName.m_str );
            if ( FAILED( hr ) )
                return NULL;
        }

        return mName;
    }

    bool CVDecl::GetAddress( MagoEE::Address& addr )
    {
        HRESULT hr = S_OK;

        hr = mSymStore->GetAddress( this, addr );
        if ( FAILED( hr ) )
            return false;

        return true;
    }

    bool CVDecl::GetOffset( int& offset )
    {
        return mSymInfo->GetOffset( offset );
    }

    bool CVDecl::GetSize( uint32_t& size )
    {
        return false;
    }

    bool CVDecl::GetBackingTy( MagoEE::ENUMTY& ty )
    {
        return false;
    }

    bool CVDecl::IsField()
    {
        MagoST::DataKind   kind = MagoST::DataIsUnknown;

        if ( !mSymInfo->GetDataKind( kind ) )
            return false;

        return kind == DataIsMember;
    }

    bool CVDecl::IsVar()
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

        // note that these are not vars: DataIsConstant, DataIsMember
        }

        return false;
    }

    bool CVDecl::IsConstant()
    {
        MagoST::DataKind   kind = MagoST::DataIsUnknown;

        if ( !mSymInfo->GetDataKind( kind ) )
            return false;

        return kind == DataIsConstant;
    }

    bool CVDecl::IsType()
    {
        MagoST::SymTag   tag = MagoST::SymTagNull;

        tag = mSymInfo->GetSymTag();

        switch ( tag )
        {
        case MagoST::SymTagUDT:
        case MagoST::SymTagEnum:
        case MagoST::SymTagFunctionType:
        case MagoST::SymTagPointerType:
        case MagoST::SymTagArrayType:
        case MagoST::SymTagBaseType:
        case MagoST::SymTagTypedef:
        case MagoST::SymTagCustomType:
        case MagoST::SymTagManagedType:
        case MagoST::SymTagNestedType:
            return true;
        }

        return false;
    }

    bool CVDecl::IsBaseClass()
    {
        return mSymInfo->GetSymTag() == MagoST::SymTagBaseClass;
    }

    HRESULT CVDecl::FindObject( const wchar_t* name, Declaration*& decl )
    {
        return E_NOT_FOUND;
    }

    bool CVDecl::EnumMembers( MagoEE::IEnumDeclarationMembers*& members )
    {
        return false;
    }


//----------------------------------------------------------------------------
//  GeneralCVDecl
//----------------------------------------------------------------------------

    GeneralCVDecl::GeneralCVDecl( 
            ExprContext* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo )
    :   CVDecl( symStore, infoData, symInfo )
    {
    }

    bool GeneralCVDecl::GetType( MagoEE::Type*& type )
    {
        if ( mType == NULL )
            return false;

        type = mType;
        type->AddRef();
        return true;
    }

    void GeneralCVDecl::SetType( MagoEE::Type* type )
    {
        mType = type;
    }


//----------------------------------------------------------------------------
//  TypeCVDecl
//----------------------------------------------------------------------------

    TypeCVDecl::TypeCVDecl( 
            ExprContext* symStore,
            MagoST::TypeHandle typeHandle,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo )
    :   CVDecl( symStore, infoData, symInfo ),
        mTypeHandle( typeHandle )
    {
    }

    bool TypeCVDecl::GetType( MagoEE::Type*& type )
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

        default:
            return false;
        }

        return true;
    }

    bool TypeCVDecl::GetSize( uint32_t& size )
    {
        return mSymInfo->GetLength( size );
    }

    bool TypeCVDecl::GetBackingTy( MagoEE::ENUMTY& ty )
    {
        HRESULT                 hr = S_OK;
        MagoST::SymTag          tag = MagoST::SymTagNull;
        DWORD                   baseTypeId = 0;
        uint32_t                size = 0;
        MagoST::TypeIndex       backingTypeIndex = 0;
        MagoST::TypeHandle      backingTypeHandle = { 0 };
        MagoST::SymInfoData     infoData = { 0 };
        MagoST::ISymbolInfo*    symInfo = NULL;

        tag = mSymInfo->GetSymTag();

        if ( tag != MagoST::SymTagEnum )
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

        ty = ExprContext::GetBasicTy( baseTypeId, size );
        if ( ty == MagoEE::Tnone )
            return false;

        return true;
    }

    HRESULT TypeCVDecl::FindObject( const wchar_t* name, Declaration*& decl )
    {
        HRESULT             hr = S_OK;
        MagoST::TypeHandle  childTH = { 0 };
        MagoST::TypeIndex   flistIndex = 0;
        MagoST::TypeHandle  flistHandle = { 0 };
        CAutoVectorPtr<char>    u8Name;
        size_t                  u8NameLen = 0;

        hr = Utf16To8( name, wcslen( name ), u8Name.m_p, u8NameLen );
        if ( FAILED( hr ) )
            return hr;

        if ( !mSymInfo->GetFieldList( flistIndex ) )
            return E_NOT_FOUND;

        for ( ; ; )
        {
            if ( !mSession->GetTypeFromTypeIndex( flistIndex, flistHandle ) )
                return E_NOT_FOUND;

            hr = mSession->FindChildType( flistHandle, u8Name, (BYTE) u8NameLen, childTH );
            if ( hr == S_OK )
                break;

            MagoST::TypeScope       scope = { 0 };
            MagoST::TypeHandle      baseTH = { 0 };
            MagoST::SymInfoData     baseInfoData = { 0 };
            MagoST::ISymbolInfo*    baseInfo = NULL;
            MagoST::TypeIndex       baseClassTI = 0;
            MagoST::TypeHandle      baseClassTH = { 0 };
            MagoST::SymInfoData     baseClassInfoData = { 0 };
            MagoST::ISymbolInfo*    baseClassInfo = NULL;

            hr = mSession->SetChildTypeScope( flistHandle, scope );
            if ( hr != S_OK )
                return E_NOT_FOUND;

            // base classes are first in the field list
            if ( !mSession->NextType( scope, baseTH ) )
                return E_NOT_FOUND;

            hr = mSession->GetTypeInfo( baseTH, baseInfoData, baseInfo );
            if ( hr != S_OK )
                return E_NOT_FOUND;

            if ( baseInfo->GetSymTag() != MagoST::SymTagBaseClass )
                return E_NOT_FOUND;

            if ( !baseInfo->GetType( baseClassTI ) )
                return E_NOT_FOUND;

            if ( !mSession->GetTypeFromTypeIndex( baseClassTI, baseClassTH ) )
                return E_NOT_FOUND;

            hr = mSession->GetTypeInfo( baseClassTH, baseClassInfoData, baseClassInfo );
            if ( hr != S_OK )
                return E_NOT_FOUND;

            if ( !baseClassInfo->GetFieldList( flistIndex ) )
                return E_NOT_FOUND;
        }

        if ( mSymInfo->GetSymTag() == MagoST::SymTagEnum )
        {
            MagoST::SymInfoData     childInfoData = { 0 };
            MagoST::ISymbolInfo*    childSymInfo = NULL;
            RefPtr<MagoEE::Type>    type;

            hr = mSession->GetTypeInfo( childTH, childInfoData, childSymInfo );
            if ( FAILED( hr ) )
                return hr;

            if ( !GetType( type.Ref() ) )
                return E_FAIL;

            hr = mSymStore->MakeDeclarationFromDataSymbol( childInfoData, childSymInfo, type, decl );
            if ( FAILED( hr ) )
                return hr;
        }
        else
        {
            hr = mSymStore->MakeDeclarationFromSymbol( childTH, decl );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }

    bool TypeCVDecl::EnumMembers( MagoEE::IEnumDeclarationMembers*& members )
    {
        RefPtr<TypeCVDeclMembers>   cvMembers;

        if ( mSymInfo->GetSymTag() != MagoST::SymTagUDT )
            return false;

        cvMembers = new TypeCVDeclMembers(
            mSymStore,
            mSymInfoData,
            mSymInfo );
        if ( cvMembers == NULL )
            return false;

        members = cvMembers.Detach();

        return true;
    }

    TypeCVDeclMembers::TypeCVDeclMembers( 
        ExprContext* symStore,
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo )
        :   mRefCount( 0 ),
            mSymStore( symStore ),
            mCount( 0 ),
            mIndex( 0 )
    {
        _ASSERT( symStore != NULL );
        _ASSERT( symInfo != NULL );

        HRESULT hr = S_OK;

        hr = symStore->GetSession( mSession.Ref() );
        _ASSERT( hr == S_OK );

        mSession->CopySymbolInfo( infoData, mSymInfoData, mSymInfo );

        Reset();
    }

    void TypeCVDeclMembers::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void TypeCVDeclMembers::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    uint32_t TypeCVDeclMembers::GetCount()
    {
        return mCount;
    }

    bool TypeCVDeclMembers::Next( MagoEE::Declaration*& decl )
    {
        HRESULT             hr = S_OK;
        MagoST::TypeHandle  childTH = { 0 };

        if ( mIndex >= GetCount() )
            return false;

        if ( !NextMember( mListScope, childTH ) )
            return false;

        mIndex++;

        hr = mSymStore->MakeDeclarationFromSymbol( childTH, decl );
        if ( FAILED( hr ) )
            return false;

        return true;
    }

    bool TypeCVDeclMembers::Reset()
    {
        MagoST::TypeIndex   flistIndex = 0;
        MagoST::TypeHandle  flistHandle = { 0 };
        uint16_t            count = 0;

        mIndex = 0;
        mCount = 0;
        memset( &mListScope, 0, sizeof mListScope );

        if ( mSymInfo->GetSymTag() != MagoST::SymTagUDT )
            return false;

        count = CountMembers();

        if ( !mSymInfo->GetFieldList( flistIndex ) )
            return false;

        if ( !mSession->GetTypeFromTypeIndex( flistIndex, flistHandle ) )
            return false;

        if ( mSession->SetChildTypeScope( flistHandle, mListScope ) != S_OK )
            return false;

        mCount = count;

        return true;
    }

    bool TypeCVDeclMembers::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mIndex) )
        {
            mIndex = GetCount();
            return false;
        }

        mIndex += count;

        for ( uint32_t i = 0; i < count; i++ )
        {
            MagoST::TypeHandle  memberTH;

            NextMember( mListScope, memberTH );
        }

        return true;
    }

    uint16_t TypeCVDeclMembers::CountMembers()
    {
        MagoST::TypeIndex   flistIndex = 0;
        MagoST::TypeHandle  flistHandle = { 0 };
        MagoST::TypeScope   flistScope = { 0 };
        uint16_t            maxCount = 0;
        uint16_t            count = 0;

        if ( !mSymInfo->GetFieldCount( maxCount ) )
            return 0;

        if ( !mSymInfo->GetFieldList( flistIndex ) )
            return 0;

        if ( !mSession->GetTypeFromTypeIndex( flistIndex, flistHandle ) )
            return 0;

        if ( mSession->SetChildTypeScope( flistHandle, flistScope ) != S_OK )
            return 0;

        for ( uint16_t i = 0; i < maxCount; i++ )
        {
            MagoST::TypeHandle      memberTH = { 0 };

            if ( !NextMember( flistScope, memberTH ) )
                continue;

            count++;
        }

        return count;
    }

    bool TypeCVDeclMembers::NextMember( MagoST::TypeScope& scope, MagoST::TypeHandle& memberTH )
    {
        MagoST::SymInfoData     infoData;
        MagoST::ISymbolInfo*    symInfo = NULL;
        MagoST::SymTag          tag = MagoST::SymTagNull;

        if ( !mSession->NextType( scope, memberTH ) )
            return false;

        if ( mSession->GetTypeInfo( memberTH, infoData, symInfo ) != S_OK )
            return false;

        tag = symInfo->GetSymTag();

        if ( (tag != MagoST::SymTagData) && (tag != MagoST::SymTagBaseClass) )
            return false;

        return true;
    }
}
