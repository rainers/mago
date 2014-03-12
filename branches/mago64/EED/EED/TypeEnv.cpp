/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "TypeEnv.h"
#include "Type.h"


// TODO: check for failure to allocate


namespace MagoEE
{
    TypeEnv::TypeEnv( int pointerSize )
        :   mRefCount( 0 ),
            mPtrSize( pointerSize )
    {
    }

    bool TypeEnv::Init()
    {
        static ENUMTY basicTys[] = 
        {
            Tvoid, Tint8, Tuns8, Tint16, Tuns16, Tint32, Tuns32, Tint64, Tuns64,
            Tfloat32, Tfloat64, Tfloat80,
            Timaginary32, Timaginary64, Timaginary80,
            Tcomplex32, Tcomplex64, Tcomplex80,
            Tbool,                  // Tbit is obsolete
            Tchar, Twchar, Tdchar 
        };

        for ( int i = 0; i < _countof( basicTys ); i++ )
        {
            ENUMTY          ty = basicTys[i];
            RefPtr<Type>    type = new TypeBasic( ty );
            
            mBasic[ty] = type;
        }

        memset( mAlias, 0, sizeof mAlias );

        if ( mPtrSize == 8 )
        {
            mAlias[Tsize_t] = Tuns64;
            mAlias[Tptrdiff_t] = Tint64;
        }
        else if ( mPtrSize == 4 )
        {
            mAlias[Tsize_t] = Tuns32;
            mAlias[Tptrdiff_t] = Tint32;
        }
        else
        {
            _ASSERT_EXPR( false, L"Unknown pointer size." );
            return false;
        }

        mVoidPtr = new TypePointer( GetType( Tvoid ), mPtrSize );

        return true;
    }

    void TypeEnv::AddRef()
    {
        mRefCount++;
    }

    void TypeEnv::Release()
    {
        mRefCount--;
        _ASSERT( mRefCount >= 0 );
        if ( mRefCount == 0 )
        {
            delete this;
        }
    }

    Type* TypeEnv::GetType( ENUMTY ty )
    {
        if ( (ty < 0) || (ty >= TMAX) )
            return NULL;

        return mBasic[ty].Get();
    }

    Type* TypeEnv::GetVoidPointerType()
    {
        return mVoidPtr.Get();
    }

    Type* TypeEnv::GetAliasType( ALIASTY ty )
    {
        if ( (ty < 0) || (ty >= ALIASTMAX) )
            return NULL;

        return GetType( mAlias[ty] );
    }

    HRESULT TypeEnv::NewPointer( Type* pointed, Type*& pointer )
    {
        pointer = new TypePointer( pointed, mPtrSize );
        pointer->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewReference( Type* pointed, Type*& pointer )
    {
        pointer = new TypeReference( pointed, mPtrSize );
        pointer->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewDArray( Type* elem, Type*& type )
    {
        // TODO: don't allocate these every time
        RefPtr<Type>    lenType = GetType( Tuns32 );
        RefPtr<Type>    ptrType = new TypePointer( elem, mPtrSize );

        type = new TypeDArray( elem, lenType, ptrType );
        type->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewAArray( Type* elem, Type* key, Type*& type )
    {
        Type*   ptrType = GetVoidPointerType();

        if ( ptrType == NULL )
            return E_OUTOFMEMORY;

        type = new TypeAArray( elem, key, ptrType->GetSize() );
        type->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewSArray( Type* elem, uint32_t length, Type*& type )
    {
        type = new TypeSArray( elem, length );
        type->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewStruct( Declaration* decl, Type*& type )
    {
        type = new TypeStruct( decl );
        type->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewEnum( Declaration* decl, Type*& type )
    {
        type = new TypeEnum( decl );
        type->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewTypedef( const wchar_t* name, Type* aliasedType, Type*& type )
    {
        type = new TypeTypedef( name, aliasedType );
        type->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewParam( StorageClass storage, Type* type, Parameter*& param )
    {
        param = new Parameter( storage, type );
        param->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewParams( ParameterList*& paramList )
    {
        paramList = new ParameterList();
        paramList->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewFunction( Type* returnType, ParameterList* params, int varArgs, Type*& type )
    {
        type = new TypeFunction( params, returnType, varArgs );
        type->AddRef();
        return S_OK;
    }

    HRESULT TypeEnv::NewDelegate( Type* funcType, Type*& type )
    {
        RefPtr<Type>    ptrToFunc = new TypePointer( funcType, mPtrSize );
        type = new TypeDelegate( ptrToFunc );
        type->AddRef();
        return S_OK;
    }
}
