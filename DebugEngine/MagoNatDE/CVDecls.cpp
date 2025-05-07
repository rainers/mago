/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "CVDecls.h"
#include "ExprContext.h"
#include "RegisterSet.h"
#include <MagoCVConst.h>

using namespace MagoST;


namespace Mago
{
//----------------------------------------------------------------------------
//  CVDecl
//----------------------------------------------------------------------------

    CVDecl::CVDecl( 
        SymbolStore* symStore,
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

            std::string shortName;
            if( MagoEE::gShortenTypeNames && mSession &&
                mSession->FindUDTShortName( pstrName.GetName(), pstrName.GetLength(), shortName) == S_OK )
                hr = Utf8To16( shortName.c_str(), shortName.length(), mName.m_str );
            else
                hr = Utf8To16( pstrName.GetName(), pstrName.GetLength(), mName.m_str );
            if ( FAILED( hr ) )
                return NULL;
        }

        return mName;
    }

    bool CVDecl::hasGetAddressOverload()
    {
        return false;
    }

    bool CVDecl::GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder )
    {
        HRESULT hr = S_OK;

        hr = binder->GetAddress( this, addr );
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

    bool CVDecl::GetVTableShape( Declaration*& decl )
    {
        MagoST::TypeIndex vsIndex;

        if ( !mSymInfo->GetVShape( vsIndex ) )
            return false;

        MagoST::TypeHandle vsTypeHandle = { 0 };
        if ( !mSession->GetTypeFromTypeIndex( vsIndex, vsTypeHandle ) )
            return false;

        HRESULT hr = mSymStore->MakeDeclarationFromSymbol( vsTypeHandle, decl );

        return !FAILED( hr );
    }

    bool CVDecl::GetVtblOffset( int& offset )
    {
        return mSymInfo->GetVtblOffset( offset );
    }

    bool CVDecl::IsField()
    {
        MagoST::DataKind   kind = MagoST::DataIsUnknown;

        if ( !mSymInfo->GetDataKind( kind ) )
            return false;

        return kind == DataIsMember;
    }

    bool CVDecl::IsStaticField()
    {
        MagoST::DataKind   kind = MagoST::DataIsUnknown;

        if ( !mSymInfo->GetDataKind( kind ) )
            return false;

        return kind == DataIsStaticMember;
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

    bool CVDecl::IsRegister()
    {
        return false;
    }

    bool CVDecl::IsFunction()
    {
        return mSymInfo->GetSymTag() == MagoST::SymTagFunction;
    }

    bool CVDecl::IsStaticFunction()
    {
        return false;
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
            SymbolStore* symStore,
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
//  FunctionCVDecl
//----------------------------------------------------------------------------

    FunctionCVDecl::FunctionCVDecl(
            SymbolStore* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo )
    : GeneralCVDecl( symStore, infoData, symInfo )
    {
    }

    bool FunctionCVDecl::hasGetAddressOverload()
    {
        return true;
    }

    bool FunctionCVDecl::GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder )
    {
        uint32_t    offset = 0;
        uint16_t    sec = 0;

        TypeIndex classIndex;
        if ( mSymInfo->GetClass( classIndex ) )
        {
            TypeHandle handle;
            if( mSession->GetTypeFromTypeIndex( classIndex, handle ) )
            {
                MagoST::SymInfoData infoData;
                ISymbolInfo* classSym;

                if( mSession->GetTypeInfo( handle, infoData, classSym ) == S_OK )
                {
                    SymString name, className;
                    if( mSymInfo->GetName( name ) && classSym->GetName( className ) )
                    {
                        std::string stdName( name.GetName(), name.GetLength() );
                        std::string fqName( className.GetName(), className.GetLength() );

                        TypeIndex fntype;
                        RefPtr<MagoEE::Type> fn;
                        if( mSymInfo->GetType( fntype ) )
                            mSymStore->GetTypeFromTypeSymbol( fntype, fn.Ref() );

                        auto fnTest = [&](TypeIndex type) 
                        {
                            RefPtr<MagoEE::Type> pointed;
                            mSymStore->GetTypeFromTypeSymbol( type, pointed.Ref() );
                            // ignore calling convention, different for symbol and type!?
                            if( pointed && fn )
                                if ( auto pfn = pointed->AsTypeFunction() )
                                    if ( auto tfn = fn->AsTypeFunction() )
                                        pfn->SetCallConv( tfn->GetCallConv() );
                            return pointed && fn ? pointed->Equals(fn) : pointed == fn;
                        };
                        fqName.append( "." ).append( stdName );
                        if( mSession->FindGlobalSymbolAddress( fqName.c_str(), addr, fnTest ) == S_OK )
                            return true;
                    }
                }
            }
        }

        if ( !mSymInfo->GetAddressOffset( offset ) )
            return false;
        if ( !mSymInfo->GetAddressSegment( sec ) )
            return false;

        addr = mSession->GetVAFromSecOffset( sec, offset );
        if ( addr == 0 )
            return false;

        return true;
    }

    bool FunctionCVDecl::IsFunction()
    {
        return true;
    }

    bool FunctionCVDecl::IsStaticFunction()
    {
        TypeIndex classIndex;
        if ( mSymInfo->GetClass( classIndex ) )
        {
            uint16_t mod;
            if ( mSymInfo->GetMod( mod ) )
                return ( mod & MODstatic ) != 0;
        }
        return true;
    }

//----------------------------------------------------------------------------
//  ClosureVarCVDecl
//----------------------------------------------------------------------------

    ClosureVarCVDecl::ClosureVarCVDecl( SymbolStore* symStore, 
                                        const MagoST::SymInfoData& infoData,
                                        MagoST::ISymbolInfo* symInfo,
                                        const MagoST::SymHandle& closureSH,
                                        const std::vector<MagoST::TypeHandle>& chain )
        : GeneralCVDecl( symStore, infoData, symInfo ),
          mClosureSH( closureSH ),
          mChain( chain )
    {
    }

    bool ClosureVarCVDecl::hasGetAddressOverload()
    {
        return true;
    }

    bool ClosureVarCVDecl::GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder )
    {
        MagoST::SymInfoData     closureInfoData = { 0 };
        MagoST::ISymbolInfo*    closureInfo = NULL;
        HRESULT hr = mSession->GetSymbolInfo( mClosureSH, closureInfoData, closureInfo );
        if ( hr != S_OK )
            return false;

        RefPtr<Declaration> closureDecl;
        hr = mSymStore->MakeDeclarationFromDataSymbol( closureInfoData, closureInfo, closureDecl.Ref() );
        if( hr != S_OK )
            return false;

        hr = binder->GetAddress( closureDecl, addr );
        if( hr != S_OK )
            return false;

        RefPtr<MagoEE::Type> ptrType;
        if ( !closureDecl->GetType( ptrType.Ref() ) )
            return false;
        uint32_t ptrSize = ptrType->GetSize();

        MagoEE::Address closAddr = 0;
        uint32_t read;
        hr = mSymStore->ReadMemory( addr, ptrSize, read, (uint8_t*) &closAddr );
        if (hr != S_OK || read != ptrSize)
            return false;

        // walk chain
        for ( size_t c = 0; c < mChain.size(); c++ )
        {
            MagoST::SymInfoData     chainInfoData = { 0 };
            MagoST::ISymbolInfo*    chainInfo = NULL;
            hr = mSession->GetTypeInfo( mChain[c], chainInfoData, chainInfo );
            if (hr != S_OK)
                return false;

            int32_t offset;
            if ( !chainInfo->GetOffset(offset) )
                return false;

            hr = mSymStore->ReadMemory( closAddr + offset, ptrSize, read, (uint8_t*)&closAddr );
            if ( hr != S_OK || read != ptrSize )
                return false;
        }
        int32_t offset;
        if( !mSymInfo->GetOffset( offset ) )
            return false;

        addr = closAddr + offset;
        return true;
    }

    bool ClosureVarCVDecl::IsField()
    {
        return false;
    }
    bool ClosureVarCVDecl::IsVar()
    {
        return true;
    }

//----------------------------------------------------------------------------
//  TypeCVDecl
//----------------------------------------------------------------------------

    TypeCVDecl::TypeCVDecl( 
            SymbolStore* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo )
    :   CVDecl( symStore, infoData, symInfo )
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

        ty = ModuleContext::GetBasicTy( baseTypeId, size );
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

            if ( decl && decl->IsFunction() )
            {
                if (!decl->IsStaticFunction())
                {
                    RefPtr<MagoEE::Type> type;
                    RefPtr<MagoEE::Type> dgtype;
                    if( decl->GetType( type.Ref() ) )
                    {
                        hr = mTypeEnv->NewDelegate( type, dgtype.Ref() );
                        if ( FAILED(hr) )
                            return hr;
                    }
                    // TODO: add SetType to interface?
                    ((FunctionCVDecl*)decl)->SetType( dgtype );
                }
            }
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
        SymbolStore* symStore,
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
        while ( (res = NextMember( childTH )) < 0 )
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

        if ( mSymInfo->GetSymTag() != MagoST::SymTagUDT )
            return false;

        count = CountMembers();
        mListScopes.resize(1);

        if ( !mSymInfo->GetFieldList( flistIndex ) )
            return false;

        if ( !mSession->GetTypeFromTypeIndex( flistIndex, flistHandle ) )
            return false;

        if ( mSession->SetChildTypeScope( flistHandle, mListScopes[0] ) != S_OK )
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
            while ( NextMember( memberTH ) < 0 )
            {
            }
        }

        return true;
    }

    uint16_t TypeCVDeclMembers::CountMembers()
    {
        MagoST::TypeIndex   flistIndex = 0;
        MagoST::TypeHandle  flistHandle = { 0 };
        uint16_t            maxCount = 0;
        uint16_t            count = 0;

        if ( !mSymInfo->GetFieldCount( maxCount ) )
            return 0;

        if ( !mSymInfo->GetFieldList( flistIndex ) )
            return 0;

        if ( !mSession->GetTypeFromTypeIndex( flistIndex, flistHandle ) )
            return 0;

        mListScopes.resize(1);
        if ( mSession->SetChildTypeScope( flistHandle, mListScopes[0] ) != S_OK )
            return 0;

        // let NextMember() filter what fields to show
        for ( uint16_t i = 0; /*i < maxCount*/; i++ )
        {
            MagoST::TypeHandle      memberTH = { 0 };

            int res = NextMember( memberTH );
            if( res < 0 )
                continue;
            if ( res == 0 )
                break;

            count++;
        }

        return count;
    }

    int TypeCVDeclMembers::NextMember( MagoST::TypeHandle& memberTH )
    {
        MagoST::SymInfoData     infoData;
        MagoST::ISymbolInfo*    symInfo = NULL;
        MagoST::SymTag          tag = MagoST::SymTagNull;

        while ( !mSession->NextType( mListScopes.back(), memberTH ) )
        {
            if (mListScopes.size() == 1)
                return 0;
            mListScopes.pop_back();
        }

        if ( mSession->GetTypeInfo( memberTH, infoData, symInfo ) != S_OK )
            return 0;

        tag = symInfo->GetSymTag();

        if ( (tag != MagoST::SymTagData) && (tag != MagoST::SymTagBaseClass) )
            return -1;

        if ( tag == MagoST::SymTagBaseClass && gOptions.flatClassFields )
        {
            MagoST::TypeIndex   flistIndex = 0;
            MagoST::TypeHandle  flistHandle = { 0 };

            if (!symInfo->GetFieldList( flistIndex ) )
                return -1;

            if (!mSession->GetTypeFromTypeIndex( flistIndex, flistHandle ) )
                return -1;

            MagoST::TypeScope baseScope;
            if ( mSession->SetChildTypeScope( flistHandle, baseScope ) != S_OK )
                return -1;

            mListScopes.push_back( baseScope );
            return -1;
        }

        LocationType locType;
        if( !gOptions.showStaticsInAggr && symInfo->GetLocation( locType ) )
            if( locType == LocIsStatic )
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

    bool ClassRefDecl::hasGetAddressOverload()
    {
        return true;
    }

    bool ClassRefDecl::GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder )
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

    bool ClassRefDecl::GetVTableShape( Declaration*& decl )
    {
        return false;
    }

    bool ClassRefDecl::GetVtblOffset( int& offset )
    {
        return false;
    }

    bool ClassRefDecl::IsField()
    {
        return false;
    }

    bool ClassRefDecl::IsStaticField()
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

    bool ClassRefDecl::IsRegister()
    {
        return false;
    }

    bool ClassRefDecl::IsFunction()
    {
        return false;
    }

    bool ClassRefDecl::IsStaticFunction()
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

//----------------------------------------------------------------------------
//  TupleDecl
//----------------------------------------------------------------------------

    TupleDecl::TupleDecl( const wchar_t* name, MagoEE::TypeTuple* tt, MagoEE::ITypeEnv* typeEnv )
        :   mRefCount( 0 ),
            mName( name ),
            mType( tt ),
            mTypeEnv( typeEnv )
    {
    }

    void TupleDecl::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void TupleDecl::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    MagoEE::Declaration* TupleDecl::firstField()
    {
        return mType->GetElementDecl( 0 );
    }

    const wchar_t* TupleDecl::GetName()
    {
        return mName.data();
    }

    bool TupleDecl::GetType( MagoEE::Type*& type )
    {
        type = mType;
        type->AddRef();

        return true;
    }

    bool TupleDecl::GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder )
    {
        return firstField()->GetAddress( addr, binder );
    }

    bool TupleDecl::GetOffset( int& offset )
    {
        offset = 0; // fields have offsets relativ to the outer struct
        return true;
    }

    bool TupleDecl::GetSize( uint32_t& size )
    {
        return false;
    }

    bool TupleDecl::GetBackingTy( MagoEE::ENUMTY& ty )
    {
        return false;
    }

    bool TupleDecl::GetUdtKind( MagoEE::UdtKind& kind )
    {
        return false;
    }

    bool TupleDecl::GetBaseClassOffset( Declaration* baseClass, int& offset )
    {
        return false;
    }

    bool TupleDecl::GetVTableShape( Declaration*& decl )
    {
        return false;
    }

    bool TupleDecl::GetVtblOffset( int& offset )
    {
        return false;
    }

    bool TupleDecl::IsField()
    {
        return firstField()->IsField();
    }

    bool TupleDecl::IsStaticField()
    {
        return firstField()->IsStaticField();
    }

    bool TupleDecl::IsVar() 
    {
        return firstField()->IsVar();
    }

    bool TupleDecl::IsConstant()
    {
        return firstField()->IsConstant();
    }

    bool TupleDecl::IsType()
    {
        return firstField()->IsType();
    }

    bool TupleDecl::IsBaseClass()
    {
        return false;
    }

    bool TupleDecl::IsRegister()
    {
        return false;
    }

    bool TupleDecl::IsFunction()
    {
        return false;
    }

    bool TupleDecl::IsStaticFunction()
    {
        return false;
    }


    HRESULT TupleDecl::FindObject( const wchar_t* name, Declaration*& decl )
    {
        return E_NOT_FOUND;
    }

    bool TupleDecl::EnumMembers( MagoEE::IEnumDeclarationMembers*& members )
    {
        return false;
    }

    HRESULT TupleDecl::FindObjectByValue( uint64_t intVal, Declaration*& decl )
    {
        return E_NOT_FOUND;
    }

//----------------------------------------------------------------------------
//  RegisterCVDecl
//----------------------------------------------------------------------------

    struct RegDesc
    {
        const char* name;
        uint32_t cvreg;
        MagoEE::ENUMTY type;
    };

    RegDesc regDesc[] =
    {
        { "AL",     CV_AMD64_AL,    MagoEE::Tint8 },
        { "CL",     CV_AMD64_CL,    MagoEE::Tint8 },
        { "DL",     CV_AMD64_DL,    MagoEE::Tint8 },
        { "BL",     CV_AMD64_BL,    MagoEE::Tint8 },
        { "AH",     CV_AMD64_AH,    MagoEE::Tint8 },
        { "CH",     CV_AMD64_CH,    MagoEE::Tint8 },
        { "DH",     CV_AMD64_DH,    MagoEE::Tint8 },
        { "BH",     CV_AMD64_BH,    MagoEE::Tint8 },
        { "AX",     CV_AMD64_AX,    MagoEE::Tint16 },
        { "CX",     CV_AMD64_CX,    MagoEE::Tint16 },
        { "DX",     CV_AMD64_DX,    MagoEE::Tint16 },
        { "BX",     CV_AMD64_BX,    MagoEE::Tint16 },
        { "SP",     CV_AMD64_SP,    MagoEE::Tint16 },
        { "BP",     CV_AMD64_BP,    MagoEE::Tint16 },
        { "SI",     CV_AMD64_SI,    MagoEE::Tint16 },
        { "DI",     CV_AMD64_DI,    MagoEE::Tint16 },
        { "EAX",    CV_AMD64_EAX,   MagoEE::Tint32 },
        { "ECX",    CV_AMD64_ECX,   MagoEE::Tint32 },
        { "EDX",    CV_AMD64_EDX,   MagoEE::Tint32 },
        { "EBX",    CV_AMD64_EBX,   MagoEE::Tint32 },
        { "ESP",    CV_AMD64_ESP,   MagoEE::Tint32 },
        { "EBP",    CV_AMD64_EBP,   MagoEE::Tint32 },
        { "ESI",    CV_AMD64_ESI,   MagoEE::Tint32 },
        { "EDI",    CV_AMD64_EDI,   MagoEE::Tint32 },
        { "ES",     CV_AMD64_ES,    MagoEE::Tint16 },
        { "CS",     CV_AMD64_CS,    MagoEE::Tint16 },
        { "SS",     CV_AMD64_SS,    MagoEE::Tint16 },
        { "DS",     CV_AMD64_DS,    MagoEE::Tint16 },
        { "FS",     CV_AMD64_FS,    MagoEE::Tint16 },
        { "GS",     CV_AMD64_GS,    MagoEE::Tint16 },
        { "FLAGS",  CV_AMD64_FLAGS, MagoEE::Tint16 },
        { "EIP",    CV_REG_EIP,     MagoEE::Tint32 },
        { "EFLAGS", CV_AMD64_EFLAGS, MagoEE::Tint32 },

        // Control registers
        { "CR0", CV_AMD64_CR0, MagoEE::Tint32 },
        { "CR1", CV_AMD64_CR1, MagoEE::Tint32 },
        { "CR2", CV_AMD64_CR2, MagoEE::Tint32 },
        { "CR3", CV_AMD64_CR3, MagoEE::Tint32 },
        { "CR4", CV_AMD64_CR4, MagoEE::Tint32 },
        { "CR8", CV_AMD64_CR8, MagoEE::Tint32 },

        // Debug registers
        { "DR0",  CV_AMD64_DR0,  MagoEE::Tint32 },
        { "DR1",  CV_AMD64_DR1,  MagoEE::Tint32 },
        { "DR2",  CV_AMD64_DR2,  MagoEE::Tint32 },
        { "DR3",  CV_AMD64_DR3,  MagoEE::Tint32 },
        { "DR4",  CV_AMD64_DR4,  MagoEE::Tint32 },
        { "DR5",  CV_AMD64_DR5,  MagoEE::Tint32 },
        { "DR6",  CV_AMD64_DR6,  MagoEE::Tint32 },
        { "DR7",  CV_AMD64_DR7,  MagoEE::Tint32 },
        { "DR8",  CV_AMD64_DR8,  MagoEE::Tint32 },
        { "DR9",  CV_AMD64_DR9,  MagoEE::Tint32 },
        { "DR10", CV_AMD64_DR10, MagoEE::Tint32 },
        { "DR11", CV_AMD64_DR11, MagoEE::Tint32 },
        { "DR12", CV_AMD64_DR12, MagoEE::Tint32 },
        { "DR13", CV_AMD64_DR13, MagoEE::Tint32 },
        { "DR14", CV_AMD64_DR14, MagoEE::Tint32 },
        { "DR15", CV_AMD64_DR15, MagoEE::Tint32 },

        { "GDTR", CV_AMD64_GDTR,   MagoEE::Tint32 },
        { "GDTL", CV_AMD64_GDTL,   MagoEE::Tint32 },
        { "IDTR", CV_AMD64_IDTR,   MagoEE::Tint32 },
        { "IDTL", CV_AMD64_IDTL,   MagoEE::Tint32 },
        { "LDTR", CV_AMD64_LDTR,   MagoEE::Tint32 },
        { "TR",   CV_AMD64_TR,     MagoEE::Tint32 },

        { "ST0",   CV_AMD64_ST0,   MagoEE::Tfloat80 },
        { "ST1",   CV_AMD64_ST1,   MagoEE::Tfloat80 },
        { "ST2",   CV_AMD64_ST2,   MagoEE::Tfloat80 },
        { "ST3",   CV_AMD64_ST3,   MagoEE::Tfloat80 },
        { "ST4",   CV_AMD64_ST4,   MagoEE::Tfloat80 },
        { "ST5",   CV_AMD64_ST5,   MagoEE::Tfloat80 },
        { "ST6",   CV_AMD64_ST6,   MagoEE::Tfloat80 },
        { "ST7",   CV_AMD64_ST7,   MagoEE::Tfloat80 },
        { "CTRL",  CV_AMD64_CTRL,  MagoEE::Tint16 },
        { "STAT",  CV_AMD64_STAT,  MagoEE::Tint16 },
        { "TAG",   CV_AMD64_TAG,   MagoEE::Tint8 },
        { "FPIP",  CV_AMD64_FPIP,  MagoEE::Tint32 },
        { "FPCS",  CV_AMD64_FPCS,  MagoEE::Tint32 },
        { "FPDO",  CV_AMD64_FPDO,  MagoEE::Tint32 },
        { "FPDS",  CV_AMD64_FPDS,  MagoEE::Tint32 },
        { "ISEM",  CV_AMD64_ISEM,  MagoEE::Tint32 },
        { "FPEIP", CV_AMD64_FPEIP, MagoEE::Tint32 },
        { "FPEDO", CV_AMD64_FPEDO, MagoEE::Tint32 },

        { "MM0", CV_AMD64_MM0, MagoEE::Tint64 },
        { "MM1", CV_AMD64_MM1, MagoEE::Tint64 },
        { "MM2", CV_AMD64_MM2, MagoEE::Tint64 },
        { "MM3", CV_AMD64_MM3, MagoEE::Tint64 },
        { "MM4", CV_AMD64_MM4, MagoEE::Tint64 },
        { "MM5", CV_AMD64_MM5, MagoEE::Tint64 },
        { "MM6", CV_AMD64_MM6, MagoEE::Tint64 },
        { "MM7", CV_AMD64_MM7, MagoEE::Tint64 },

        { "XMM0", CV_AMD64_XMM0, MagoEE::Tint32 },   // KATMAI registers (128 bit)
        { "XMM1", CV_AMD64_XMM1, MagoEE::Tint32 },
        { "XMM2", CV_AMD64_XMM2, MagoEE::Tint32 },
        { "XMM3", CV_AMD64_XMM3, MagoEE::Tint32 },
        { "XMM4", CV_AMD64_XMM4, MagoEE::Tint32 },
        { "XMM5", CV_AMD64_XMM5, MagoEE::Tint32 },
        { "XMM6", CV_AMD64_XMM6, MagoEE::Tint32 },
        { "XMM7", CV_AMD64_XMM7, MagoEE::Tint32 },

        { "XMM00", CV_AMD64_XMM0_0, MagoEE::Tfloat32 },   // KATMAI sub-registers
        { "XMM01", CV_AMD64_XMM0_1, MagoEE::Tfloat32 },
        { "XMM02", CV_AMD64_XMM0_2, MagoEE::Tfloat32 },
        { "XMM03", CV_AMD64_XMM0_3, MagoEE::Tfloat32 },
        { "XMM10", CV_AMD64_XMM1_0, MagoEE::Tfloat32 },
        { "XMM11", CV_AMD64_XMM1_1, MagoEE::Tfloat32 },
        { "XMM12", CV_AMD64_XMM1_2, MagoEE::Tfloat32 },
        { "XMM13", CV_AMD64_XMM1_3, MagoEE::Tfloat32 },
        { "XMM20", CV_AMD64_XMM2_0, MagoEE::Tfloat32 },
        { "XMM21", CV_AMD64_XMM2_1, MagoEE::Tfloat32 },
        { "XMM22", CV_AMD64_XMM2_2, MagoEE::Tfloat32 },
        { "XMM23", CV_AMD64_XMM2_3, MagoEE::Tfloat32 },
        { "XMM30", CV_AMD64_XMM3_0, MagoEE::Tfloat32 },
        { "XMM31", CV_AMD64_XMM3_1, MagoEE::Tfloat32 },
        { "XMM32", CV_AMD64_XMM3_2, MagoEE::Tfloat32 },
        { "XMM33", CV_AMD64_XMM3_3, MagoEE::Tfloat32 },
        { "XMM40", CV_AMD64_XMM4_0, MagoEE::Tfloat32 },
        { "XMM41", CV_AMD64_XMM4_1, MagoEE::Tfloat32 },
        { "XMM42", CV_AMD64_XMM4_2, MagoEE::Tfloat32 },
        { "XMM43", CV_AMD64_XMM4_3, MagoEE::Tfloat32 },
        { "XMM50", CV_AMD64_XMM5_0, MagoEE::Tfloat32 },
        { "XMM51", CV_AMD64_XMM5_1, MagoEE::Tfloat32 },
        { "XMM52", CV_AMD64_XMM5_2, MagoEE::Tfloat32 },
        { "XMM53", CV_AMD64_XMM5_3, MagoEE::Tfloat32 },
        { "XMM60", CV_AMD64_XMM6_0, MagoEE::Tfloat32 },
        { "XMM61", CV_AMD64_XMM6_1, MagoEE::Tfloat32 },
        { "XMM62", CV_AMD64_XMM6_2, MagoEE::Tfloat32 },
        { "XMM63", CV_AMD64_XMM6_3, MagoEE::Tfloat32 },
        { "XMM70", CV_AMD64_XMM7_0, MagoEE::Tfloat32 },
        { "XMM71", CV_AMD64_XMM7_1, MagoEE::Tfloat32 },
        { "XMM72", CV_AMD64_XMM7_2, MagoEE::Tfloat32 },
        { "XMM73", CV_AMD64_XMM7_3, MagoEE::Tfloat32 },

        { "XMM0DL", CV_AMD64_XMM0L, MagoEE::Tfloat64 },
        { "XMM1DL", CV_AMD64_XMM1L, MagoEE::Tfloat64 },
        { "XMM2DL", CV_AMD64_XMM2L, MagoEE::Tfloat64 },
        { "XMM3DL", CV_AMD64_XMM3L, MagoEE::Tfloat64 },
        { "XMM4DL", CV_AMD64_XMM4L, MagoEE::Tfloat64 },
        { "XMM5DL", CV_AMD64_XMM5L, MagoEE::Tfloat64 },
        { "XMM6DL", CV_AMD64_XMM6L, MagoEE::Tfloat64 },
        { "XMM7DL", CV_AMD64_XMM7L, MagoEE::Tfloat64 },

        { "XMM0DH", CV_AMD64_XMM0H, MagoEE::Tfloat64 },
        { "XMM1DH", CV_AMD64_XMM1H, MagoEE::Tfloat64 },
        { "XMM2DH", CV_AMD64_XMM2H, MagoEE::Tfloat64 },
        { "XMM3DH", CV_AMD64_XMM3H, MagoEE::Tfloat64 },
        { "XMM4DH", CV_AMD64_XMM4H, MagoEE::Tfloat64 },
        { "XMM5DH", CV_AMD64_XMM5H, MagoEE::Tfloat64 },
        { "XMM6DH", CV_AMD64_XMM6H, MagoEE::Tfloat64 },
        { "XMM7DH", CV_AMD64_XMM7H, MagoEE::Tfloat64 },

        { "MXCSR", CV_AMD64_MXCSR, MagoEE::Tint16 },   // XMM status register

        { "EMM0L", CV_AMD64_EMM0L, MagoEE::Tint64 },   // XMM sub-registers (WNI integer)
        { "EMM1L", CV_AMD64_EMM1L, MagoEE::Tint64 },
        { "EMM2L", CV_AMD64_EMM2L, MagoEE::Tint64 },
        { "EMM3L", CV_AMD64_EMM3L, MagoEE::Tint64 },
        { "EMM4L", CV_AMD64_EMM4L, MagoEE::Tint64 },
        { "EMM5L", CV_AMD64_EMM5L, MagoEE::Tint64 },
        { "EMM6L", CV_AMD64_EMM6L, MagoEE::Tint64 },
        { "EMM7L", CV_AMD64_EMM7L, MagoEE::Tint64 },

        { "EMM0H", CV_AMD64_EMM0H, MagoEE::Tint64 },
        { "EMM1H", CV_AMD64_EMM1H, MagoEE::Tint64 },
        { "EMM2H", CV_AMD64_EMM2H, MagoEE::Tint64 },
        { "EMM3H", CV_AMD64_EMM3H, MagoEE::Tint64 },
        { "EMM4H", CV_AMD64_EMM4H, MagoEE::Tint64 },
        { "EMM5H", CV_AMD64_EMM5H, MagoEE::Tint64 },
        { "EMM6H", CV_AMD64_EMM6H, MagoEE::Tint64 },
        { "EMM7H", CV_AMD64_EMM7H, MagoEE::Tint64 },

        // do not change the order of these regs, first one must be even too
        { "MM00", CV_AMD64_MM00, MagoEE::Tint32 },
        { "MM01", CV_AMD64_MM01, MagoEE::Tint32 },
        { "MM10", CV_AMD64_MM10, MagoEE::Tint32 },
        { "MM11", CV_AMD64_MM11, MagoEE::Tint32 },
        { "MM20", CV_AMD64_MM20, MagoEE::Tint32 },
        { "MM21", CV_AMD64_MM21, MagoEE::Tint32 },
        { "MM30", CV_AMD64_MM30, MagoEE::Tint32 },
        { "MM31", CV_AMD64_MM31, MagoEE::Tint32 },
        { "MM40", CV_AMD64_MM40, MagoEE::Tint32 },
        { "MM41", CV_AMD64_MM41, MagoEE::Tint32 },
        { "MM50", CV_AMD64_MM50, MagoEE::Tint32 },
        { "MM51", CV_AMD64_MM51, MagoEE::Tint32 },
        { "MM60", CV_AMD64_MM60, MagoEE::Tint32 },
        { "MM61", CV_AMD64_MM61, MagoEE::Tint32 },
        { "MM70", CV_AMD64_MM70, MagoEE::Tint32 },
        { "MM71", CV_AMD64_MM71, MagoEE::Tint32 },

        // Extended KATMAI registers
        { "XMM8",  CV_AMD64_XMM8,  MagoEE::Tint32 },   // KATMAI registers (128 bit)
        { "XMM9",  CV_AMD64_XMM9,  MagoEE::Tint32 },
        { "XMM10", CV_AMD64_XMM10, MagoEE::Tint32 },
        { "XMM11", CV_AMD64_XMM11, MagoEE::Tint32 },
        { "XMM12", CV_AMD64_XMM12, MagoEE::Tint32 },
        { "XMM13", CV_AMD64_XMM13, MagoEE::Tint32 },
        { "XMM14", CV_AMD64_XMM14, MagoEE::Tint32 },
        { "XMM15", CV_AMD64_XMM15, MagoEE::Tint32 },

        { "XMM80",  CV_AMD64_XMM8_0,  MagoEE::Tfloat32 },   // KATMAI sub-registers
        { "XMM81",  CV_AMD64_XMM8_1,  MagoEE::Tfloat32 },
        { "XMM82",  CV_AMD64_XMM8_2,  MagoEE::Tfloat32 },
        { "XMM83",  CV_AMD64_XMM8_3,  MagoEE::Tfloat32 },
        { "XMM90",  CV_AMD64_XMM9_0,  MagoEE::Tfloat32 },
        { "XMM91",  CV_AMD64_XMM9_1,  MagoEE::Tfloat32 },
        { "XMM92",  CV_AMD64_XMM9_2,  MagoEE::Tfloat32 },
        { "XMM93",  CV_AMD64_XMM9_3,  MagoEE::Tfloat32 },
        { "XMM100", CV_AMD64_XMM10_0, MagoEE::Tfloat32 },
        { "XMM101", CV_AMD64_XMM10_1, MagoEE::Tfloat32 },
        { "XMM102", CV_AMD64_XMM10_2, MagoEE::Tfloat32 },
        { "XMM103", CV_AMD64_XMM10_3, MagoEE::Tfloat32 },
        { "XMM110", CV_AMD64_XMM11_0, MagoEE::Tfloat32 },
        { "XMM111", CV_AMD64_XMM11_1, MagoEE::Tfloat32 },
        { "XMM112", CV_AMD64_XMM11_2, MagoEE::Tfloat32 },
        { "XMM113", CV_AMD64_XMM11_3, MagoEE::Tfloat32 },
        { "XMM120", CV_AMD64_XMM12_0, MagoEE::Tfloat32 },
        { "XMM121", CV_AMD64_XMM12_1, MagoEE::Tfloat32 },
        { "XMM122", CV_AMD64_XMM12_2, MagoEE::Tfloat32 },
        { "XMM123", CV_AMD64_XMM12_3, MagoEE::Tfloat32 },
        { "XMM130", CV_AMD64_XMM13_0, MagoEE::Tfloat32 },
        { "XMM131", CV_AMD64_XMM13_1, MagoEE::Tfloat32 },
        { "XMM132", CV_AMD64_XMM13_2, MagoEE::Tfloat32 },
        { "XMM133", CV_AMD64_XMM13_3, MagoEE::Tfloat32 },
        { "XMM140", CV_AMD64_XMM14_0, MagoEE::Tfloat32 },
        { "XMM141", CV_AMD64_XMM14_1, MagoEE::Tfloat32 },
        { "XMM142", CV_AMD64_XMM14_2, MagoEE::Tfloat32 },
        { "XMM143", CV_AMD64_XMM14_3, MagoEE::Tfloat32 },
        { "XMM150", CV_AMD64_XMM15_0, MagoEE::Tfloat32 },
        { "XMM151", CV_AMD64_XMM15_1, MagoEE::Tfloat32 },
        { "XMM152", CV_AMD64_XMM15_2, MagoEE::Tfloat32 },
        { "XMM153", CV_AMD64_XMM15_3, MagoEE::Tfloat32 },

        { "XMM8DL",  CV_AMD64_XMM8L,  MagoEE::Tfloat64 },
        { "XMM9DL",  CV_AMD64_XMM9L,  MagoEE::Tfloat64 },
        { "XMM10DL", CV_AMD64_XMM10L, MagoEE::Tfloat64 },
        { "XMM11DL", CV_AMD64_XMM11L, MagoEE::Tfloat64 },
        { "XMM12DL", CV_AMD64_XMM12L, MagoEE::Tfloat64 },
        { "XMM13DL", CV_AMD64_XMM13L, MagoEE::Tfloat64 },
        { "XMM14DL", CV_AMD64_XMM14L, MagoEE::Tfloat64 },
        { "XMM15DL", CV_AMD64_XMM15L, MagoEE::Tfloat64 },

        { "XMM8DH",  CV_AMD64_XMM8H,  MagoEE::Tfloat64 },
        { "XMM9DH",  CV_AMD64_XMM9H,  MagoEE::Tfloat64 },
        { "XMM10DH", CV_AMD64_XMM10H, MagoEE::Tfloat64 },
        { "XMM11DH", CV_AMD64_XMM11H, MagoEE::Tfloat64 },
        { "XMM12DH", CV_AMD64_XMM12H, MagoEE::Tfloat64 },
        { "XMM13DH", CV_AMD64_XMM13H, MagoEE::Tfloat64 },
        { "XMM14DH", CV_AMD64_XMM14H, MagoEE::Tfloat64 },
        { "XMM15DH", CV_AMD64_XMM15H, MagoEE::Tfloat64 },

        { "EMM8L",  CV_AMD64_EMM8L,  MagoEE::Tint64 },   // XMM sub-registers (WNI integer)
        { "EMM9L",  CV_AMD64_EMM9L,  MagoEE::Tint64 },
        { "EMM10L", CV_AMD64_EMM10L, MagoEE::Tint64 },
        { "EMM11L", CV_AMD64_EMM11L, MagoEE::Tint64 },
        { "EMM12L", CV_AMD64_EMM12L, MagoEE::Tint64 },
        { "EMM13L", CV_AMD64_EMM13L, MagoEE::Tint64 },
        { "EMM14L", CV_AMD64_EMM14L, MagoEE::Tint64 },
        { "EMM15L", CV_AMD64_EMM15L, MagoEE::Tint64 },

        { "EMM8H",  CV_AMD64_EMM8H,  MagoEE::Tint64 },
        { "EMM9H",  CV_AMD64_EMM9H,  MagoEE::Tint64 },
        { "EMM10H", CV_AMD64_EMM10H, MagoEE::Tint64 },
        { "EMM11H", CV_AMD64_EMM11H, MagoEE::Tint64 },
        { "EMM12H", CV_AMD64_EMM12H, MagoEE::Tint64 },
        { "EMM13H", CV_AMD64_EMM13H, MagoEE::Tint64 },
        { "EMM14H", CV_AMD64_EMM14H, MagoEE::Tint64 },
        { "EMM15H", CV_AMD64_EMM15H, MagoEE::Tint64 },

        // Low byte forms of some standard registers
        { "SIL", CV_AMD64_SIL, MagoEE::Tint8 },
        { "DIL", CV_AMD64_DIL, MagoEE::Tint8 },
        { "BPL", CV_AMD64_BPL, MagoEE::Tint8 },
        { "SPL", CV_AMD64_SPL, MagoEE::Tint8 },

        // 64-bit regular registers
        { "RAX", CV_AMD64_RAX, MagoEE::Tint64 },
        { "RBX", CV_AMD64_RBX, MagoEE::Tint64 },
        { "RCX", CV_AMD64_RCX, MagoEE::Tint64 },
        { "RDX", CV_AMD64_RDX, MagoEE::Tint64 },
        { "RSI", CV_AMD64_RSI, MagoEE::Tint64 },
        { "RDI", CV_AMD64_RDI, MagoEE::Tint64 },
        { "RBP", CV_AMD64_RBP, MagoEE::Tint64 },
        { "RSP", CV_AMD64_RSP, MagoEE::Tint64 },
        { "RIP", CV_AMD64_RIP, MagoEE::Tint64 }, // same cv ID as CV_REG_EIP

        // 64-bit integer registers with 8-, 16-, and 32-bit forms (B, W, and D)
        { "R8",  CV_AMD64_R8,  MagoEE::Tint64 },
        { "R9",  CV_AMD64_R9,  MagoEE::Tint64 },
        { "R10", CV_AMD64_R10, MagoEE::Tint64 },
        { "R11", CV_AMD64_R11, MagoEE::Tint64 },
        { "R12", CV_AMD64_R12, MagoEE::Tint64 },
        { "R13", CV_AMD64_R13, MagoEE::Tint64 },
        { "R14", CV_AMD64_R14, MagoEE::Tint64 },
        { "R15", CV_AMD64_R15, MagoEE::Tint64 },

        { "R8B",  CV_AMD64_R8B,  MagoEE::Tint8 },
        { "R9B",  CV_AMD64_R9B,  MagoEE::Tint8 },
        { "R10B", CV_AMD64_R10B, MagoEE::Tint8 },
        { "R11B", CV_AMD64_R11B, MagoEE::Tint8 },
        { "R12B", CV_AMD64_R12B, MagoEE::Tint8 },
        { "R13B", CV_AMD64_R13B, MagoEE::Tint8 },
        { "R14B", CV_AMD64_R14B, MagoEE::Tint8 },
        { "R15B", CV_AMD64_R15B, MagoEE::Tint8 },

        { "R8W",  CV_AMD64_R8W,  MagoEE::Tint16 },
        { "R9W",  CV_AMD64_R9W,  MagoEE::Tint16 },
        { "R10W", CV_AMD64_R10W, MagoEE::Tint16 },
        { "R11W", CV_AMD64_R11W, MagoEE::Tint16 },
        { "R12W", CV_AMD64_R12W, MagoEE::Tint16 },
        { "R13W", CV_AMD64_R13W, MagoEE::Tint16 },
        { "R14W", CV_AMD64_R14W, MagoEE::Tint16 },
        { "R15W", CV_AMD64_R15W, MagoEE::Tint16 },

        { "R8D",  CV_AMD64_R8D,  MagoEE::Tint32 },
        { "R9D",  CV_AMD64_R9D,  MagoEE::Tint32 },
        { "R10D", CV_AMD64_R10D, MagoEE::Tint32 },
        { "R11D", CV_AMD64_R11D, MagoEE::Tint32 },
        { "R12D", CV_AMD64_R12D, MagoEE::Tint32 },
        { "R13D", CV_AMD64_R13D, MagoEE::Tint32 },
        { "R14D", CV_AMD64_R14D, MagoEE::Tint32 },
        { "R15D", CV_AMD64_R15D, MagoEE::Tint32 },

        // AVX registers 256 bits
        { "YMM0",  CV_AMD64_YMM0,  MagoEE::Tint32 },
        { "YMM1",  CV_AMD64_YMM1,  MagoEE::Tint32 },
        { "YMM2",  CV_AMD64_YMM2,  MagoEE::Tint32 },
        { "YMM3",  CV_AMD64_YMM3,  MagoEE::Tint32 },
        { "YMM4",  CV_AMD64_YMM4,  MagoEE::Tint32 },
        { "YMM5",  CV_AMD64_YMM5,  MagoEE::Tint32 },
        { "YMM6",  CV_AMD64_YMM6,  MagoEE::Tint32 },
        { "YMM7",  CV_AMD64_YMM7,  MagoEE::Tint32 },
        { "YMM8",  CV_AMD64_YMM8,  MagoEE::Tint32 }, 
        { "YMM9",  CV_AMD64_YMM9,  MagoEE::Tint32 },
        { "YMM10", CV_AMD64_YMM10, MagoEE::Tint32 },
        { "YMM11", CV_AMD64_YMM11, MagoEE::Tint32 },
        { "YMM12", CV_AMD64_YMM12, MagoEE::Tint32 },
        { "YMM13", CV_AMD64_YMM13, MagoEE::Tint32 },
        { "YMM14", CV_AMD64_YMM14, MagoEE::Tint32 },
        { "YMM15", CV_AMD64_YMM15, MagoEE::Tint32 },

        // AVX registers upper 128 bits
        { "YMM0H",  CV_AMD64_YMM0H,  MagoEE::Tint32 },
        { "YMM1H",  CV_AMD64_YMM1H,  MagoEE::Tint32 },
        { "YMM2H",  CV_AMD64_YMM2H,  MagoEE::Tint32 },
        { "YMM3H",  CV_AMD64_YMM3H,  MagoEE::Tint32 },
        { "YMM4H",  CV_AMD64_YMM4H,  MagoEE::Tint32 },
        { "YMM5H",  CV_AMD64_YMM5H,  MagoEE::Tint32 },
        { "YMM6H",  CV_AMD64_YMM6H,  MagoEE::Tint32 },
        { "YMM7H",  CV_AMD64_YMM7H,  MagoEE::Tint32 },
        { "YMM8H",  CV_AMD64_YMM8H,  MagoEE::Tint32 }, 
        { "YMM9H",  CV_AMD64_YMM9H,  MagoEE::Tint32 },
        { "YMM10H", CV_AMD64_YMM10H, MagoEE::Tint32 },
        { "YMM11H", CV_AMD64_YMM11H, MagoEE::Tint32 },
        { "YMM12H", CV_AMD64_YMM12H, MagoEE::Tint32 },
        { "YMM13H", CV_AMD64_YMM13H, MagoEE::Tint32 },
        { "YMM14H", CV_AMD64_YMM14H, MagoEE::Tint32 },
        { "YMM15H", CV_AMD64_YMM15H, MagoEE::Tint32 },

        //Lower/upper 8 bytes of XMM registers.  Unlike CV_AMD64_XMM<regnum><H/L>, these
        //values reprsesent the bit patterns of the registers as 64-bit integers, not
        //the representation of these registers as a double.
        { "XMM0IL",  CV_AMD64_XMM0IL,  MagoEE::Tint64 },
        { "XMM1IL",  CV_AMD64_XMM1IL,  MagoEE::Tint64 },
        { "XMM2IL",  CV_AMD64_XMM2IL,  MagoEE::Tint64 },
        { "XMM3IL",  CV_AMD64_XMM3IL,  MagoEE::Tint64 },
        { "XMM4IL",  CV_AMD64_XMM4IL,  MagoEE::Tint64 },
        { "XMM5IL",  CV_AMD64_XMM5IL,  MagoEE::Tint64 },
        { "XMM6IL",  CV_AMD64_XMM6IL,  MagoEE::Tint64 },
        { "XMM7IL",  CV_AMD64_XMM7IL,  MagoEE::Tint64 },
        { "XMM8IL",  CV_AMD64_XMM8IL,  MagoEE::Tint64 },
        { "XMM9IL",  CV_AMD64_XMM9IL,  MagoEE::Tint64 },
        { "XMM10IL", CV_AMD64_XMM10IL, MagoEE::Tint64 },
        { "XMM11IL", CV_AMD64_XMM11IL, MagoEE::Tint64 },
        { "XMM12IL", CV_AMD64_XMM12IL, MagoEE::Tint64 },
        { "XMM13IL", CV_AMD64_XMM13IL, MagoEE::Tint64 },
        { "XMM14IL", CV_AMD64_XMM14IL, MagoEE::Tint64 },
        { "XMM15IL", CV_AMD64_XMM15IL, MagoEE::Tint64 },

        { "XMM0IH",  CV_AMD64_XMM0IH,  MagoEE::Tint64 },
        { "XMM1IH",  CV_AMD64_XMM1IH,  MagoEE::Tint64 },
        { "XMM2IH",  CV_AMD64_XMM2IH,  MagoEE::Tint64 },
        { "XMM3IH",  CV_AMD64_XMM3IH,  MagoEE::Tint64 },
        { "XMM4IH",  CV_AMD64_XMM4IH,  MagoEE::Tint64 },
        { "XMM5IH",  CV_AMD64_XMM5IH,  MagoEE::Tint64 },
        { "XMM6IH",  CV_AMD64_XMM6IH,  MagoEE::Tint64 },
        { "XMM7IH",  CV_AMD64_XMM7IH,  MagoEE::Tint64 },
        { "XMM8IH",  CV_AMD64_XMM8IH,  MagoEE::Tint64 },
        { "XMM9IH",  CV_AMD64_XMM9IH,  MagoEE::Tint64 },
        { "XMM10IH", CV_AMD64_XMM10IH, MagoEE::Tint64 },
        { "XMM11IH", CV_AMD64_XMM11IH, MagoEE::Tint64 },
        { "XMM12IH", CV_AMD64_XMM12IH, MagoEE::Tint64 },
        { "XMM13IH", CV_AMD64_XMM13IH, MagoEE::Tint64 },
        { "XMM14IH", CV_AMD64_XMM14IH, MagoEE::Tint64 },
        { "XMM15IH", CV_AMD64_XMM15IH, MagoEE::Tint64 },

        { "YMM0I0",  CV_AMD64_YMM0I0,  MagoEE::Tint64 },        // AVX integer registers
        { "YMM0I1",  CV_AMD64_YMM0I1,  MagoEE::Tint64 },
        { "YMM0I2",  CV_AMD64_YMM0I2,  MagoEE::Tint64 },
        { "YMM0I3",  CV_AMD64_YMM0I3,  MagoEE::Tint64 },
        { "YMM1I0",  CV_AMD64_YMM1I0,  MagoEE::Tint64 },
        { "YMM1I1",  CV_AMD64_YMM1I1,  MagoEE::Tint64 },
        { "YMM1I2",  CV_AMD64_YMM1I2,  MagoEE::Tint64 },
        { "YMM1I3",  CV_AMD64_YMM1I3,  MagoEE::Tint64 },
        { "YMM2I0",  CV_AMD64_YMM2I0,  MagoEE::Tint64 },
        { "YMM2I1",  CV_AMD64_YMM2I1,  MagoEE::Tint64 },
        { "YMM2I2",  CV_AMD64_YMM2I2,  MagoEE::Tint64 },
        { "YMM2I3",  CV_AMD64_YMM2I3,  MagoEE::Tint64 },
        { "YMM3I0",  CV_AMD64_YMM3I0,  MagoEE::Tint64 },
        { "YMM3I1",  CV_AMD64_YMM3I1,  MagoEE::Tint64 },
        { "YMM3I2",  CV_AMD64_YMM3I2,  MagoEE::Tint64 },
        { "YMM3I3",  CV_AMD64_YMM3I3,  MagoEE::Tint64 },
        { "YMM4I0",  CV_AMD64_YMM4I0,  MagoEE::Tint64 },
        { "YMM4I1",  CV_AMD64_YMM4I1,  MagoEE::Tint64 },
        { "YMM4I2",  CV_AMD64_YMM4I2,  MagoEE::Tint64 },
        { "YMM4I3",  CV_AMD64_YMM4I3,  MagoEE::Tint64 },
        { "YMM5I0",  CV_AMD64_YMM5I0,  MagoEE::Tint64 },
        { "YMM5I1",  CV_AMD64_YMM5I1,  MagoEE::Tint64 },
        { "YMM5I2",  CV_AMD64_YMM5I2,  MagoEE::Tint64 },
        { "YMM5I3",  CV_AMD64_YMM5I3,  MagoEE::Tint64 },
        { "YMM6I0",  CV_AMD64_YMM6I0,  MagoEE::Tint64 },
        { "YMM6I1",  CV_AMD64_YMM6I1,  MagoEE::Tint64 },
        { "YMM6I2",  CV_AMD64_YMM6I2,  MagoEE::Tint64 },
        { "YMM6I3",  CV_AMD64_YMM6I3,  MagoEE::Tint64 },
        { "YMM7I0",  CV_AMD64_YMM7I0,  MagoEE::Tint64 },
        { "YMM7I1",  CV_AMD64_YMM7I1,  MagoEE::Tint64 },
        { "YMM7I2",  CV_AMD64_YMM7I2,  MagoEE::Tint64 },
        { "YMM7I3",  CV_AMD64_YMM7I3,  MagoEE::Tint64 },
        { "YMM8I0",  CV_AMD64_YMM8I0,  MagoEE::Tint64 },
        { "YMM8I1",  CV_AMD64_YMM8I1,  MagoEE::Tint64 },
        { "YMM8I2",  CV_AMD64_YMM8I2,  MagoEE::Tint64 },
        { "YMM8I3",  CV_AMD64_YMM8I3,  MagoEE::Tint64 },
        { "YMM9I0",  CV_AMD64_YMM9I0,  MagoEE::Tint64 },
        { "YMM9I1",  CV_AMD64_YMM9I1,  MagoEE::Tint64 },
        { "YMM9I2",  CV_AMD64_YMM9I2,  MagoEE::Tint64 },
        { "YMM9I3",  CV_AMD64_YMM9I3,  MagoEE::Tint64 },
        { "YMM10I0", CV_AMD64_YMM10I0, MagoEE::Tint64 },
        { "YMM10I1", CV_AMD64_YMM10I1, MagoEE::Tint64 },
        { "YMM10I2", CV_AMD64_YMM10I2, MagoEE::Tint64 },
        { "YMM10I3", CV_AMD64_YMM10I3, MagoEE::Tint64 },
        { "YMM11I0", CV_AMD64_YMM11I0, MagoEE::Tint64 },
        { "YMM11I1", CV_AMD64_YMM11I1, MagoEE::Tint64 },
        { "YMM11I2", CV_AMD64_YMM11I2, MagoEE::Tint64 },
        { "YMM11I3", CV_AMD64_YMM11I3, MagoEE::Tint64 },
        { "YMM12I0", CV_AMD64_YMM12I0, MagoEE::Tint64 },
        { "YMM12I1", CV_AMD64_YMM12I1, MagoEE::Tint64 },
        { "YMM12I2", CV_AMD64_YMM12I2, MagoEE::Tint64 },
        { "YMM12I3", CV_AMD64_YMM12I3, MagoEE::Tint64 },
        { "YMM13I0", CV_AMD64_YMM13I0, MagoEE::Tint64 },
        { "YMM13I1", CV_AMD64_YMM13I1, MagoEE::Tint64 },
        { "YMM13I2", CV_AMD64_YMM13I2, MagoEE::Tint64 },
        { "YMM13I3", CV_AMD64_YMM13I3, MagoEE::Tint64 },
        { "YMM14I0", CV_AMD64_YMM14I0, MagoEE::Tint64 },
        { "YMM14I1", CV_AMD64_YMM14I1, MagoEE::Tint64 },
        { "YMM14I2", CV_AMD64_YMM14I2, MagoEE::Tint64 },
        { "YMM14I3", CV_AMD64_YMM14I3, MagoEE::Tint64 },
        { "YMM15I0", CV_AMD64_YMM15I0, MagoEE::Tint64 },
        { "YMM15I1", CV_AMD64_YMM15I1, MagoEE::Tint64 },
        { "YMM15I2", CV_AMD64_YMM15I2, MagoEE::Tint64 },
        { "YMM15I3", CV_AMD64_YMM15I3, MagoEE::Tint64 },

        { "YMM0F0",  CV_AMD64_YMM0F0,  MagoEE::Tfloat32 },        // AVX floating-point single precise registers
        { "YMM0F1",  CV_AMD64_YMM0F1,  MagoEE::Tfloat32 },
        { "YMM0F2",  CV_AMD64_YMM0F2,  MagoEE::Tfloat32 },
        { "YMM0F3",  CV_AMD64_YMM0F3,  MagoEE::Tfloat32 },
        { "YMM0F4",  CV_AMD64_YMM0F4,  MagoEE::Tfloat32 },
        { "YMM0F5",  CV_AMD64_YMM0F5,  MagoEE::Tfloat32 },
        { "YMM0F6",  CV_AMD64_YMM0F6,  MagoEE::Tfloat32 },
        { "YMM0F7",  CV_AMD64_YMM0F7,  MagoEE::Tfloat32 },
        { "YMM1F0",  CV_AMD64_YMM1F0,  MagoEE::Tfloat32 },
        { "YMM1F1",  CV_AMD64_YMM1F1,  MagoEE::Tfloat32 },
        { "YMM1F2",  CV_AMD64_YMM1F2,  MagoEE::Tfloat32 },
        { "YMM1F3",  CV_AMD64_YMM1F3,  MagoEE::Tfloat32 },
        { "YMM1F4",  CV_AMD64_YMM1F4,  MagoEE::Tfloat32 },
        { "YMM1F5",  CV_AMD64_YMM1F5,  MagoEE::Tfloat32 },
        { "YMM1F6",  CV_AMD64_YMM1F6,  MagoEE::Tfloat32 },
        { "YMM1F7",  CV_AMD64_YMM1F7,  MagoEE::Tfloat32 },
        { "YMM2F0",  CV_AMD64_YMM2F0,  MagoEE::Tfloat32 },
        { "YMM2F1",  CV_AMD64_YMM2F1,  MagoEE::Tfloat32 },
        { "YMM2F2",  CV_AMD64_YMM2F2,  MagoEE::Tfloat32 },
        { "YMM2F3",  CV_AMD64_YMM2F3,  MagoEE::Tfloat32 },
        { "YMM2F4",  CV_AMD64_YMM2F4,  MagoEE::Tfloat32 },
        { "YMM2F5",  CV_AMD64_YMM2F5,  MagoEE::Tfloat32 },
        { "YMM2F6",  CV_AMD64_YMM2F6,  MagoEE::Tfloat32 },
        { "YMM2F7",  CV_AMD64_YMM2F7,  MagoEE::Tfloat32 },
        { "YMM3F0",  CV_AMD64_YMM3F0,  MagoEE::Tfloat32 },
        { "YMM3F1",  CV_AMD64_YMM3F1,  MagoEE::Tfloat32 },
        { "YMM3F2",  CV_AMD64_YMM3F2,  MagoEE::Tfloat32 },
        { "YMM3F3",  CV_AMD64_YMM3F3,  MagoEE::Tfloat32 },
        { "YMM3F4",  CV_AMD64_YMM3F4,  MagoEE::Tfloat32 },
        { "YMM3F5",  CV_AMD64_YMM3F5,  MagoEE::Tfloat32 },
        { "YMM3F6",  CV_AMD64_YMM3F6,  MagoEE::Tfloat32 },
        { "YMM3F7",  CV_AMD64_YMM3F7,  MagoEE::Tfloat32 },
        { "YMM4F0",  CV_AMD64_YMM4F0,  MagoEE::Tfloat32 },
        { "YMM4F1",  CV_AMD64_YMM4F1,  MagoEE::Tfloat32 },
        { "YMM4F2",  CV_AMD64_YMM4F2,  MagoEE::Tfloat32 },
        { "YMM4F3",  CV_AMD64_YMM4F3,  MagoEE::Tfloat32 },
        { "YMM4F4",  CV_AMD64_YMM4F4,  MagoEE::Tfloat32 },
        { "YMM4F5",  CV_AMD64_YMM4F5,  MagoEE::Tfloat32 },
        { "YMM4F6",  CV_AMD64_YMM4F6,  MagoEE::Tfloat32 },
        { "YMM4F7",  CV_AMD64_YMM4F7,  MagoEE::Tfloat32 },
        { "YMM5F0",  CV_AMD64_YMM5F0,  MagoEE::Tfloat32 },
        { "YMM5F1",  CV_AMD64_YMM5F1,  MagoEE::Tfloat32 },
        { "YMM5F2",  CV_AMD64_YMM5F2,  MagoEE::Tfloat32 },
        { "YMM5F3",  CV_AMD64_YMM5F3,  MagoEE::Tfloat32 },
        { "YMM5F4",  CV_AMD64_YMM5F4,  MagoEE::Tfloat32 },
        { "YMM5F5",  CV_AMD64_YMM5F5,  MagoEE::Tfloat32 },
        { "YMM5F6",  CV_AMD64_YMM5F6,  MagoEE::Tfloat32 },
        { "YMM5F7",  CV_AMD64_YMM5F7,  MagoEE::Tfloat32 },
        { "YMM6F0",  CV_AMD64_YMM6F0,  MagoEE::Tfloat32 },
        { "YMM6F1",  CV_AMD64_YMM6F1,  MagoEE::Tfloat32 },
        { "YMM6F2",  CV_AMD64_YMM6F2,  MagoEE::Tfloat32 },
        { "YMM6F3",  CV_AMD64_YMM6F3,  MagoEE::Tfloat32 },
        { "YMM6F4",  CV_AMD64_YMM6F4,  MagoEE::Tfloat32 },
        { "YMM6F5",  CV_AMD64_YMM6F5,  MagoEE::Tfloat32 },
        { "YMM6F6",  CV_AMD64_YMM6F6,  MagoEE::Tfloat32 },
        { "YMM6F7",  CV_AMD64_YMM6F7,  MagoEE::Tfloat32 },
        { "YMM7F0",  CV_AMD64_YMM7F0,  MagoEE::Tfloat32 },
        { "YMM7F1",  CV_AMD64_YMM7F1,  MagoEE::Tfloat32 },
        { "YMM7F2",  CV_AMD64_YMM7F2,  MagoEE::Tfloat32 },
        { "YMM7F3",  CV_AMD64_YMM7F3,  MagoEE::Tfloat32 },
        { "YMM7F4",  CV_AMD64_YMM7F4,  MagoEE::Tfloat32 },
        { "YMM7F5",  CV_AMD64_YMM7F5,  MagoEE::Tfloat32 },
        { "YMM7F6",  CV_AMD64_YMM7F6,  MagoEE::Tfloat32 },
        { "YMM7F7",  CV_AMD64_YMM7F7,  MagoEE::Tfloat32 },
        { "YMM8F0",  CV_AMD64_YMM8F0,  MagoEE::Tfloat32 },
        { "YMM8F1",  CV_AMD64_YMM8F1,  MagoEE::Tfloat32 },
        { "YMM8F2",  CV_AMD64_YMM8F2,  MagoEE::Tfloat32 },
        { "YMM8F3",  CV_AMD64_YMM8F3,  MagoEE::Tfloat32 },
        { "YMM8F4",  CV_AMD64_YMM8F4,  MagoEE::Tfloat32 },
        { "YMM8F5",  CV_AMD64_YMM8F5,  MagoEE::Tfloat32 },
        { "YMM8F6",  CV_AMD64_YMM8F6,  MagoEE::Tfloat32 },
        { "YMM8F7",  CV_AMD64_YMM8F7,  MagoEE::Tfloat32 },
        { "YMM9F0",  CV_AMD64_YMM9F0,  MagoEE::Tfloat32 },
        { "YMM9F1",  CV_AMD64_YMM9F1,  MagoEE::Tfloat32 },
        { "YMM9F2",  CV_AMD64_YMM9F2,  MagoEE::Tfloat32 },
        { "YMM9F3",  CV_AMD64_YMM9F3,  MagoEE::Tfloat32 },
        { "YMM9F4",  CV_AMD64_YMM9F4,  MagoEE::Tfloat32 },
        { "YMM9F5",  CV_AMD64_YMM9F5,  MagoEE::Tfloat32 },
        { "YMM9F6",  CV_AMD64_YMM9F6,  MagoEE::Tfloat32 },
        { "YMM9F7",  CV_AMD64_YMM9F7,  MagoEE::Tfloat32 },
        { "YMM10F0", CV_AMD64_YMM10F0, MagoEE::Tfloat32 },
        { "YMM10F1", CV_AMD64_YMM10F1, MagoEE::Tfloat32 },
        { "YMM10F2", CV_AMD64_YMM10F2, MagoEE::Tfloat32 },
        { "YMM10F3", CV_AMD64_YMM10F3, MagoEE::Tfloat32 },
        { "YMM10F4", CV_AMD64_YMM10F4, MagoEE::Tfloat32 },
        { "YMM10F5", CV_AMD64_YMM10F5, MagoEE::Tfloat32 },
        { "YMM10F6", CV_AMD64_YMM10F6, MagoEE::Tfloat32 },
        { "YMM10F7", CV_AMD64_YMM10F7, MagoEE::Tfloat32 },
        { "YMM11F0", CV_AMD64_YMM11F0, MagoEE::Tfloat32 },
        { "YMM11F1", CV_AMD64_YMM11F1, MagoEE::Tfloat32 },
        { "YMM11F2", CV_AMD64_YMM11F2, MagoEE::Tfloat32 },
        { "YMM11F3", CV_AMD64_YMM11F3, MagoEE::Tfloat32 },
        { "YMM11F4", CV_AMD64_YMM11F4, MagoEE::Tfloat32 },
        { "YMM11F5", CV_AMD64_YMM11F5, MagoEE::Tfloat32 },
        { "YMM11F6", CV_AMD64_YMM11F6, MagoEE::Tfloat32 },
        { "YMM11F7", CV_AMD64_YMM11F7, MagoEE::Tfloat32 },
        { "YMM12F0", CV_AMD64_YMM12F0, MagoEE::Tfloat32 },
        { "YMM12F1", CV_AMD64_YMM12F1, MagoEE::Tfloat32 },
        { "YMM12F2", CV_AMD64_YMM12F2, MagoEE::Tfloat32 },
        { "YMM12F3", CV_AMD64_YMM12F3, MagoEE::Tfloat32 },
        { "YMM12F4", CV_AMD64_YMM12F4, MagoEE::Tfloat32 },
        { "YMM12F5", CV_AMD64_YMM12F5, MagoEE::Tfloat32 },
        { "YMM12F6", CV_AMD64_YMM12F6, MagoEE::Tfloat32 },
        { "YMM12F7", CV_AMD64_YMM12F7, MagoEE::Tfloat32 },
        { "YMM13F0", CV_AMD64_YMM13F0, MagoEE::Tfloat32 },
        { "YMM13F1", CV_AMD64_YMM13F1, MagoEE::Tfloat32 },
        { "YMM13F2", CV_AMD64_YMM13F2, MagoEE::Tfloat32 },
        { "YMM13F3", CV_AMD64_YMM13F3, MagoEE::Tfloat32 },
        { "YMM13F4", CV_AMD64_YMM13F4, MagoEE::Tfloat32 },
        { "YMM13F5", CV_AMD64_YMM13F5, MagoEE::Tfloat32 },
        { "YMM13F6", CV_AMD64_YMM13F6, MagoEE::Tfloat32 },
        { "YMM13F7", CV_AMD64_YMM13F7, MagoEE::Tfloat32 },
        { "YMM14F0", CV_AMD64_YMM14F0, MagoEE::Tfloat32 },
        { "YMM14F1", CV_AMD64_YMM14F1, MagoEE::Tfloat32 },
        { "YMM14F2", CV_AMD64_YMM14F2, MagoEE::Tfloat32 },
        { "YMM14F3", CV_AMD64_YMM14F3, MagoEE::Tfloat32 },
        { "YMM14F4", CV_AMD64_YMM14F4, MagoEE::Tfloat32 },
        { "YMM14F5", CV_AMD64_YMM14F5, MagoEE::Tfloat32 },
        { "YMM14F6", CV_AMD64_YMM14F6, MagoEE::Tfloat32 },
        { "YMM14F7", CV_AMD64_YMM14F7, MagoEE::Tfloat32 },
        { "YMM15F0", CV_AMD64_YMM15F0, MagoEE::Tfloat32 },
        { "YMM15F1", CV_AMD64_YMM15F1, MagoEE::Tfloat32 },
        { "YMM15F2", CV_AMD64_YMM15F2, MagoEE::Tfloat32 },
        { "YMM15F3", CV_AMD64_YMM15F3, MagoEE::Tfloat32 },
        { "YMM15F4", CV_AMD64_YMM15F4, MagoEE::Tfloat32 },
        { "YMM15F5", CV_AMD64_YMM15F5, MagoEE::Tfloat32 },
        { "YMM15F6", CV_AMD64_YMM15F6, MagoEE::Tfloat32 },
        { "YMM15F7", CV_AMD64_YMM15F7, MagoEE::Tfloat32 },
    
        { "YMM0D0",  CV_AMD64_YMM0D0,  MagoEE::Tfloat64 },        // AVX floating-point double precise registers
        { "YMM0D1",  CV_AMD64_YMM0D1,  MagoEE::Tfloat64 },
        { "YMM0D2",  CV_AMD64_YMM0D2,  MagoEE::Tfloat64 },
        { "YMM0D3",  CV_AMD64_YMM0D3,  MagoEE::Tfloat64 },
        { "YMM1D0",  CV_AMD64_YMM1D0,  MagoEE::Tfloat64 },
        { "YMM1D1",  CV_AMD64_YMM1D1,  MagoEE::Tfloat64 },
        { "YMM1D2",  CV_AMD64_YMM1D2,  MagoEE::Tfloat64 },
        { "YMM1D3",  CV_AMD64_YMM1D3,  MagoEE::Tfloat64 },
        { "YMM2D0",  CV_AMD64_YMM2D0,  MagoEE::Tfloat64 },
        { "YMM2D1",  CV_AMD64_YMM2D1,  MagoEE::Tfloat64 },
        { "YMM2D2",  CV_AMD64_YMM2D2,  MagoEE::Tfloat64 },
        { "YMM2D3",  CV_AMD64_YMM2D3,  MagoEE::Tfloat64 },
        { "YMM3D0",  CV_AMD64_YMM3D0,  MagoEE::Tfloat64 },
        { "YMM3D1",  CV_AMD64_YMM3D1,  MagoEE::Tfloat64 },
        { "YMM3D2",  CV_AMD64_YMM3D2,  MagoEE::Tfloat64 },
        { "YMM3D3",  CV_AMD64_YMM3D3,  MagoEE::Tfloat64 },
        { "YMM4D0",  CV_AMD64_YMM4D0,  MagoEE::Tfloat64 },
        { "YMM4D1",  CV_AMD64_YMM4D1,  MagoEE::Tfloat64 },
        { "YMM4D2",  CV_AMD64_YMM4D2,  MagoEE::Tfloat64 },
        { "YMM4D3",  CV_AMD64_YMM4D3,  MagoEE::Tfloat64 },
        { "YMM5D0",  CV_AMD64_YMM5D0,  MagoEE::Tfloat64 },
        { "YMM5D1",  CV_AMD64_YMM5D1,  MagoEE::Tfloat64 },
        { "YMM5D2",  CV_AMD64_YMM5D2,  MagoEE::Tfloat64 },
        { "YMM5D3",  CV_AMD64_YMM5D3,  MagoEE::Tfloat64 },
        { "YMM6D0",  CV_AMD64_YMM6D0,  MagoEE::Tfloat64 },
        { "YMM6D1",  CV_AMD64_YMM6D1,  MagoEE::Tfloat64 },
        { "YMM6D2",  CV_AMD64_YMM6D2,  MagoEE::Tfloat64 },
        { "YMM6D3",  CV_AMD64_YMM6D3,  MagoEE::Tfloat64 },
        { "YMM7D0",  CV_AMD64_YMM7D0,  MagoEE::Tfloat64 },
        { "YMM7D1",  CV_AMD64_YMM7D1,  MagoEE::Tfloat64 },
        { "YMM7D2",  CV_AMD64_YMM7D2,  MagoEE::Tfloat64 },
        { "YMM7D3",  CV_AMD64_YMM7D3,  MagoEE::Tfloat64 },
        { "YMM8D0",  CV_AMD64_YMM8D0,  MagoEE::Tfloat64 },
        { "YMM8D1",  CV_AMD64_YMM8D1,  MagoEE::Tfloat64 },
        { "YMM8D2",  CV_AMD64_YMM8D2,  MagoEE::Tfloat64 },
        { "YMM8D3",  CV_AMD64_YMM8D3,  MagoEE::Tfloat64 },
        { "YMM9D0",  CV_AMD64_YMM9D0,  MagoEE::Tfloat64 },
        { "YMM9D1",  CV_AMD64_YMM9D1,  MagoEE::Tfloat64 },
        { "YMM9D2",  CV_AMD64_YMM9D2,  MagoEE::Tfloat64 },
        { "YMM9D3",  CV_AMD64_YMM9D3,  MagoEE::Tfloat64 },
        { "YMM10D0", CV_AMD64_YMM10D0, MagoEE::Tfloat64 },
        { "YMM10D1", CV_AMD64_YMM10D1, MagoEE::Tfloat64 },
        { "YMM10D2", CV_AMD64_YMM10D2, MagoEE::Tfloat64 },
        { "YMM10D3", CV_AMD64_YMM10D3, MagoEE::Tfloat64 },
        { "YMM11D0", CV_AMD64_YMM11D0, MagoEE::Tfloat64 },
        { "YMM11D1", CV_AMD64_YMM11D1, MagoEE::Tfloat64 },
        { "YMM11D2", CV_AMD64_YMM11D2, MagoEE::Tfloat64 },
        { "YMM11D3", CV_AMD64_YMM11D3, MagoEE::Tfloat64 },
        { "YMM12D0", CV_AMD64_YMM12D0, MagoEE::Tfloat64 },
        { "YMM12D1", CV_AMD64_YMM12D1, MagoEE::Tfloat64 },
        { "YMM12D2", CV_AMD64_YMM12D2, MagoEE::Tfloat64 },
        { "YMM12D3", CV_AMD64_YMM12D3, MagoEE::Tfloat64 },
        { "YMM13D0", CV_AMD64_YMM13D0, MagoEE::Tfloat64 },
        { "YMM13D1", CV_AMD64_YMM13D1, MagoEE::Tfloat64 },
        { "YMM13D2", CV_AMD64_YMM13D2, MagoEE::Tfloat64 },
        { "YMM13D3", CV_AMD64_YMM13D3, MagoEE::Tfloat64 },
        { "YMM14D0", CV_AMD64_YMM14D0, MagoEE::Tfloat64 },
        { "YMM14D1", CV_AMD64_YMM14D1, MagoEE::Tfloat64 },
        { "YMM14D2", CV_AMD64_YMM14D2, MagoEE::Tfloat64 },
        { "YMM14D3", CV_AMD64_YMM14D3, MagoEE::Tfloat64 },
        { "YMM15D0", CV_AMD64_YMM15D0, MagoEE::Tfloat64 },
        { "YMM15D1", CV_AMD64_YMM15D1, MagoEE::Tfloat64 },
        { "YMM15D2", CV_AMD64_YMM15D2, MagoEE::Tfloat64 },
        { "YMM15D3", CV_AMD64_YMM15D3, MagoEE::Tfloat64 },
    };

    class ImplSymbolInfo : public ISymbolInfo
    {
    public:
        virtual bool GetName( SymString& name ) { return false; }
        virtual bool GetRegister( uint32_t& reg ) { return false; }
        virtual bool GetLength( uint32_t& length ) { return false; }
        virtual bool GetDataKind( DataKind& dataKind ) { return false; }
        virtual bool GetLocation( LocationType& locType ) { return false; }

        virtual bool GetType( TypeIndex& index ) { return false; }
        virtual bool GetAddressOffset( uint32_t& offset ) { return false; }
        virtual bool GetAddressSegment( uint16_t& segment ) { return false; }
        virtual bool GetOffset( int32_t& offset ) { return false; }
        virtual bool GetRegisterCount( uint8_t& count ) { return false; }
        virtual bool GetRegisters( uint8_t*& regs ) { return false; }
        virtual bool GetUdtKind( UdtKind& udtKind ) { return false; }
        virtual bool GetValue( Variant& value ) { return false; }

#if 1
        virtual bool GetDebugStart( uint32_t& start ) { return false; }
        virtual bool GetDebugEnd( uint32_t& end ) { return false; }
        //virtual bool GetProcFlags( CV_PROCFLAGS& flags ) { return false; }
        virtual bool GetProcFlags( uint8_t& flags ) { return false; }
        virtual bool GetThunkOrdinal( uint8_t& ordinal ) { return false; }
        virtual bool GetBasicType( DWORD& basicType ) { return false; }
        virtual bool GetIndexType( TypeIndex& index ) { return false; }
        virtual bool GetCount( uint32_t& count ) { return false; }
        virtual bool GetFieldCount( uint16_t& count ) { return false; }
        virtual bool GetFieldList( TypeIndex& index ) { return false; }
        virtual bool GetProperties( uint16_t& props ) { return false; }
        virtual bool GetDerivedList( TypeIndex& index ) { return false; }
        virtual bool GetVShape( TypeIndex& index ) { return false; }
        virtual bool GetCallConv( uint8_t& callConv ) { return false; }
        virtual bool GetParamCount( uint16_t& count ) { return false; }
        virtual bool GetParamList( TypeIndex& index ) { return false; }
        virtual bool GetClass( TypeIndex& index ) { return false; }
        virtual bool GetThis( TypeIndex& index ) { return false; }
        virtual bool GetThisAdjust( int32_t& adjust ) { return false; }
        virtual bool GetOemId( uint32_t& oemId ) { return false; }
        virtual bool GetOemSymbolId( uint32_t& oemSymId ) { return false; }
        virtual bool GetTypes( std::vector<TypeIndex>& indexes ) { return false; }
        virtual bool GetAttribute( uint16_t& attr ) { return false; }
        virtual bool GetVBaseOffset( uint32_t& offset ) { return false; }
        virtual bool GetVTableDescriptor( uint32_t index, uint8_t& desc ) { return false; }
        virtual bool GetVtblOffset( int& offset ) { return false; }
        virtual bool GetMod( uint16_t& mod ) { return false; }
#endif
    };

    class RegisterSymbolInfo : public ImplSymbolInfo
    {
        uint32_t mIndex;

    public:
        RegisterSymbolInfo( uint32_t regIdx ) : mIndex( regIdx ) {}

        virtual SymTag GetSymTag() { return SymTagData; }
        virtual bool GetName( SymString& name ) 
        {
            const char* regname = regDesc[mIndex].name;
            auto len = strlen( regname );
            name.set( len, regname, false );
            return true;
        }
        virtual bool GetRegister( uint32_t& reg )
        {
            reg = regDesc[ mIndex ].cvreg;
            return true;
        }
        virtual bool GetLength( uint32_t& length )
        {
            length = MagoEE::TypeBasic::GetTypeSize( regDesc[mIndex].type );
            return true;
        }
        virtual bool GetDataKind( DataKind& dataKind ) { dataKind = DataIsConstant; return true; }
        virtual bool GetLocation( LocationType& locType ) { locType = LocIsEnregistered; return true; }
    };

    static std::map<std::string, uint32_t> mapNameToRegIndex;

    static bool initRegs = []()
    {
        for( size_t i = 0; i < sizeof( regDesc ) / sizeof( regDesc[0] ); i++ )
            mapNameToRegIndex[ regDesc[i].name ] = i;

        return true;
    }();

    MagoST::SymInfoData registerInfoData;

    RegisterCVDecl::RegisterCVDecl( SymbolStore* symStore, uint32_t regIndex )
        : GeneralCVDecl( symStore, registerInfoData, (ISymbolInfo*) &registerInfoData )
    {
        // CVDecl asumes registerInfoData is an instance of mSymInfo
        mSymInfo = new RegisterSymbolInfo( regIndex ); 
        SetType( symStore->GetTypeEnv()->GetType( regDesc[regIndex].type ) );
    }

    RegisterCVDecl::~RegisterCVDecl()
    {
        delete (RegisterSymbolInfo*) mSymInfo;
    }

    bool RegisterCVDecl::IsRegister()
    {
        return true;
    }

    RegisterCVDecl* RegisterCVDecl::CreateRegisterSymbol( SymbolStore* symStore, const char* name )
    {
        char uname[12];
        auto len = strlen( name );
        if( len > 10 )
            return nullptr;
        for( size_t p = 0; p <= len; p++ )
            uname[p] = name[p] + ( name[p] >= 'a' && name[p] <= 'z' ? 'A' - 'a' : 0 );

        auto it = mapNameToRegIndex.find( uname );
        if( it == mapNameToRegIndex.end() )
            return nullptr;

        auto typeEnv = symStore->GetTypeEnv();
        bool isx64 = typeEnv && typeEnv->GetPointerSize() == 8;
        if( !isx64 && regDesc[it->second].cvreg > CV_AMD64_MM71 )
            return nullptr;

        return new RegisterCVDecl( symStore, it->second );
    }

    class VTableSymbolInfo : public ImplSymbolInfo
    {
        uint32_t mCount;

    public:
        VTableSymbolInfo( uint32_t cnt) : mCount( cnt ) {}

        virtual SymTag GetSymTag() { return SymTagData; }
        virtual bool GetOffset( int& offset ) { offset = 0; return true; }
        virtual bool GetDataKind( DataKind& dataKind ) { dataKind = DataIsConstant; return true; }
        virtual bool GetLocation( LocationType& locType ) { locType = LocIsThisRel; return true; }
    };

    VTableCVDecl::VTableCVDecl( SymbolStore* symStore, uint32_t count, MagoEE::Type* type )
        : GeneralCVDecl( symStore, registerInfoData, (ISymbolInfo*) &registerInfoData )
    {
        // CVDecl asumes registerInfoData is an instance of mSymInfo
        mSymInfo = new VTableSymbolInfo( count ); 
        SetType( type );
    }

    VTableCVDecl::~VTableCVDecl()
    {
        delete (VTableSymbolInfo*) mSymInfo;
    }

    bool VTableCVDecl::IsField()
    {
        return true;
    }

}
