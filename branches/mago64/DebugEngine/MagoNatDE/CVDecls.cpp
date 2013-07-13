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
            SymString   pstrName;

            if ( !mSymInfo->GetName( pstrName ) )
                return NULL;

            hr = Utf8To16( pstrName.GetName(), pstrName.GetLength(), mName.m_str );
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

    bool CVDecl::GetUdtKind( MagoEE::UdtKind& kind )
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

    bool CVDecl::GetBaseClassOffset( Declaration* baseClass, int& offset )
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

    HRESULT CVDecl::FindObjectByValue( uint64_t intVal, Declaration*& decl )
    {
        return E_NOT_FOUND;
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

    bool TypeCVDecl::GetBaseClassOffset( Declaration* baseClass, int& offset )
    {
        MagoEE::UdtKind kind;

        if ( (baseClass == NULL) || !baseClass->IsType()
            || !baseClass->GetUdtKind( kind ) )
            return false;

        const wchar_t*  name16 = GetName();
        const wchar_t*  baseName16 = baseClass->GetName();

        if ( (name16 == NULL) || (baseName16 == NULL) )
            return false;

        if ( wcscmp( baseName16, name16 ) == 0 )
        {
            offset = 0;
            return true;
        }

        HRESULT         hr = S_OK;
        size_t          baseNameLen16 = wcslen( baseName16 );
        size_t          baseNameLen8 = 0;
        CAutoVectorPtr<char>    baseName8;

        hr = Utf16To8( baseName16, baseNameLen16, baseName8.m_p, baseNameLen8 );
        if ( FAILED( hr ) )
            return false;

        uint16_t            fieldCount = 0;
        MagoST::TypeIndex   fieldListTI = 0;
        MagoST::TypeHandle  fieldListTH = { 0 };

        if ( !mSymInfo->GetFieldCount( fieldCount ) )
            return false;

        if ( !mSymInfo->GetFieldList( fieldListTI ) )
            return false;

        if ( !mSession->GetTypeFromTypeIndex( fieldListTI, fieldListTH ) )
            return false;

        return FindBaseClass( baseName8, baseNameLen8, fieldListTH, fieldCount, offset );
    }

    bool TypeCVDecl::FindBaseClass( 
        const char* baseName, 
        size_t baseNameLen,
        MagoST::TypeHandle fieldListTH, 
        size_t fieldCount,
        int& offset )
    {
        FindBaseClassParams nameParams = { 0 };

        nameParams.Name = baseName;
        nameParams.NameLen = baseNameLen;

        ForeachBaseClass( fieldListTH, fieldCount, &TypeCVDecl::FindClassInList, &nameParams );

        if ( !nameParams.OutFound )
        {
            ForeachBaseClass( fieldListTH, fieldCount, &TypeCVDecl::RecurseClasses, &nameParams );
        }

        if ( nameParams.OutFound )
        {
            offset = nameParams.OutOffset;
            return true;
        }

        return false;
    }

    void TypeCVDecl::ForeachBaseClass(
        MagoST::TypeHandle fieldListTH, 
        size_t fieldCount,
        BaseClassFunc functor,
        void* context
        )
    {
        MagoST::TypeScope   flistScope = { 0 };

        if ( mSession->SetChildTypeScope( fieldListTH, flistScope ) != S_OK )
            return;

        // TODO: DMD is writing the wrong field list count
        //       when it's fixed, we should check fieldCount again
        for ( uint16_t i = 0; /*i < fieldCount*/; i++ )
        {
            MagoST::TypeHandle      memberTH = { 0 };
            MagoST::SymInfoData     memberInfoData = { 0 };
            MagoST::ISymbolInfo*    memberInfo = NULL;
            MagoST::SymTag          tag = MagoST::SymTagNull;
            MagoST::TypeIndex       classTI = 0;
            MagoST::TypeHandle      classTH = { 0 };
            MagoST::SymInfoData     classInfoData = { 0 };
            MagoST::ISymbolInfo*    classInfo = NULL;

            if ( !mSession->NextType( flistScope, memberTH ) )
                // no more
                break;

            if ( mSession->GetTypeInfo( memberTH, memberInfoData, memberInfo ) != S_OK )
                continue;

            tag = memberInfo->GetSymTag();
            if ( tag != MagoST::SymTagBaseClass )
                continue;

            if ( !memberInfo->GetType( classTI ) )
                continue;

            if ( !mSession->GetTypeFromTypeIndex( classTI, classTH ) )
                continue;

            if ( mSession->GetTypeInfo( classTH, classInfoData, classInfo ) != S_OK )
                continue;

            if ( classInfo->GetSymTag() != SymTagUDT )
                continue;

            if ( !(this->*functor)( memberInfo, classInfo, context ) )
                break;
        }
    }

    bool TypeCVDecl::FindClassInList(
        MagoST::ISymbolInfo* memberInfo,
        MagoST::ISymbolInfo* classInfo,
        void* context )
    {
        _ASSERT( memberInfo != NULL );
        _ASSERT( classInfo != NULL );
        _ASSERT( context != NULL );

        FindBaseClassParams*    params = (FindBaseClassParams*) context;
        SymString               className;

        if ( !classInfo->GetName( className ) )
            return true;

        if ( (className.GetLength() != params->NameLen) 
            || (strncmp( className.GetName(), params->Name, params->NameLen ) != 0) )
            return true;

        if ( !memberInfo->GetOffset( params->OutOffset ) )
            // found the we want one, but we can't get the offset, so quit
            return false;

        params->OutFound = true;

        return false;       // we found it, quit the search
    }

    bool TypeCVDecl::RecurseClasses(
        MagoST::ISymbolInfo* memberInfo,
        MagoST::ISymbolInfo* classInfo,
        void* context )
    {
        _ASSERT( memberInfo != NULL );
        _ASSERT( classInfo != NULL );
        _ASSERT( context != NULL );

        FindBaseClassParams*  params = (FindBaseClassParams*) context;
        uint16_t            fieldCount = 0;
        MagoST::TypeIndex   fieldListTI = 0;
        MagoST::TypeHandle  fieldListTH = { 0 };
        int                 offset = 0;
        int                 ourOffset = 0;

        if ( !memberInfo->GetOffset( ourOffset ) )
            return true;

        if ( !classInfo->GetFieldCount( fieldCount ) )
            return true;

        if ( !classInfo->GetFieldList( fieldListTI ) )
            return true;

        if ( !mSession->GetTypeFromTypeIndex( fieldListTI, fieldListTH ) )
            return true;

        if ( !FindBaseClass( 
            params->Name, 
            params->NameLen, 
            fieldListTH, 
            fieldCount, 
            offset ) )
            return true;

        params->OutFound = true;
        params->OutOffset = ourOffset + offset;

        return false;
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

            hr = mSession->FindChildType( flistHandle, u8Name, u8NameLen, childTH );
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

    HRESULT TypeCVDecl::FindObjectByValue( uint64_t intVal, Declaration*& decl )
    {
        HRESULT             hr = S_OK;
        MagoST::TypeHandle  childTH = { 0 };
        MagoST::TypeIndex   flistIndex = 0;
        MagoST::TypeHandle  flistHandle = { 0 };
        MagoST::TypeScope   scope = { 0 };
        bool                found = false;

        if ( !mSymInfo->GetFieldList( flistIndex ) )
            return E_NOT_FOUND;

        if ( !mSession->GetTypeFromTypeIndex( flistIndex, flistHandle ) )
            return E_NOT_FOUND;

        hr = mSession->SetChildTypeScope( flistHandle, scope );
        if ( FAILED( hr ) )
            return hr;

        for ( ; !found && mSession->NextType( scope, childTH ); )
        {
            ISymbolInfo*    childInfo = NULL;
            SymInfoData     childData = { 0 };
            Variant         value = { 0 };
            uint64_t        u64 = 0;

            hr = mSession->GetTypeInfo( childTH, childData, childInfo );
            if ( hr != S_OK )
                continue;

            if ( !childInfo->GetValue( value ) )
                continue;

            switch ( value.Tag )
            {
            case MagoST::VarTag_Char:       u64 = value.Data.I8;  break;
            case MagoST::VarTag_Short:      u64 = value.Data.I16; break;
            case MagoST::VarTag_UShort:     u64 = value.Data.U16; break;
            case MagoST::VarTag_Long:       u64 = value.Data.I32; break;
            case MagoST::VarTag_ULong:      u64 = value.Data.U32; break;
            case MagoST::VarTag_Quadword:   u64 = value.Data.I64; break;
            case MagoST::VarTag_UQuadword:  u64 = value.Data.U64; break;
            default:
                continue;
            }

            if ( u64 == intVal )
                found = true;
        }

        if ( !found )
            return E_NOT_FOUND;

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


//----------------------------------------------------------------------------
//  TypeCVDeclMembers
//----------------------------------------------------------------------------

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

        int res;
        // skip any invalid ones that we skipped during CountMembers
        while ( (res = NextMember( mListScope, childTH )) < 0 )
        {
        }
        if ( res <= 0 )
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

            // skip any invalid ones that we skipped during CountMembers
            while ( NextMember( mListScope, memberTH ) < 0 )
            {
            }
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

        // TODO: DMD is writing the wrong field list count
        //       when it's fixed, we should check maxCount again
        for ( uint16_t i = 0; /*i < maxCount*/; i++ )
        {
            MagoST::TypeHandle      memberTH = { 0 };

            int res = NextMember( flistScope, memberTH );
            if( res < 0 )
                continue;
            if ( res == 0 )
                break;

            count++;
        }

        return count;
    }

    int TypeCVDeclMembers::NextMember( MagoST::TypeScope& scope, MagoST::TypeHandle& memberTH )
    {
        MagoST::SymInfoData     infoData;
        MagoST::ISymbolInfo*    symInfo = NULL;
        MagoST::SymTag          tag = MagoST::SymTagNull;

        if ( !mSession->NextType( scope, memberTH ) )
            return 0;

        if ( mSession->GetTypeInfo( memberTH, infoData, symInfo ) != S_OK )
            return 0;

        tag = symInfo->GetSymTag();

        if ( (tag != MagoST::SymTagData) && (tag != MagoST::SymTagBaseClass) )
            return -1;

        return 1;
    }


//----------------------------------------------------------------------------
//  ClassRefDecl
//----------------------------------------------------------------------------

    ClassRefDecl::ClassRefDecl( Declaration* decl, MagoEE::ITypeEnv* typeEnv )
        :   mRefCount( 0 ),
            mOrigDecl( decl ),
            mTypeEnv( typeEnv )
    {
    }

    void ClassRefDecl::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void ClassRefDecl::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    const wchar_t* ClassRefDecl::GetName()
    {
        return NULL;
    }

    bool ClassRefDecl::GetType( MagoEE::Type*& type )
    {
        RefPtr<MagoEE::Type>    classType;

        if ( !mOrigDecl->GetType( classType.Ref() ) )
            return false;

        if ( FAILED( mTypeEnv->NewReference( classType, type ) ) )
            return false;

        return true;
    }

    bool ClassRefDecl::GetAddress( MagoEE::Address& addr )
    {
        return false;
    }

    bool ClassRefDecl::GetOffset( int& offset )
    {
        return false;
    }

    bool ClassRefDecl::GetSize( uint32_t& size )
    {
        return false;
    }

    bool ClassRefDecl::GetBackingTy( MagoEE::ENUMTY& ty )
    {
        return false;
    }

    bool ClassRefDecl::GetUdtKind( MagoEE::UdtKind& kind )
    {
        return false;
    }

    bool ClassRefDecl::GetBaseClassOffset( Declaration* baseClass, int& offset )
    {
        return false;
    }

    bool ClassRefDecl::IsField()
    {
        return false;
    }

    bool ClassRefDecl::IsVar()
    {
        return false;
    }

    bool ClassRefDecl::IsConstant()
    {
        return false;
    }

    bool ClassRefDecl::IsType()
    {
        return true;
    }

    bool ClassRefDecl::IsBaseClass()
    {
        return false;
    }

    HRESULT ClassRefDecl::FindObject( const wchar_t* name, Declaration*& decl )
    {
        return E_NOT_FOUND;
    }

    bool ClassRefDecl::EnumMembers( MagoEE::IEnumDeclarationMembers*& members )
    {
        return false;
    }

    HRESULT ClassRefDecl::FindObjectByValue( uint64_t intVal, Declaration*& decl )
    {
        return E_NOT_FOUND;
    }
}
