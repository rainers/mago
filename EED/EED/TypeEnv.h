/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "TypeCommon.h"
#include "ITypeEnv.h"


namespace MagoEE
{
    class Type;
    class TypeBasic;
    enum ENUMTY;


    class TypeEnv : public ITypeEnv
    {
        long            mRefCount;
        RefPtr<Type>    mBasic[TMAX];
        RefPtr<Type>    mVoidPtr;
        ENUMTY          mAlias[ALIASTMAX];
        int             mPtrSize;

    public:
        TypeEnv( int pointerSize );

        bool Init();

        virtual void AddRef();
        virtual void Release();

        virtual int GetPointerSize();
        virtual Type* GetType( ENUMTY ty );
        virtual Type* GetVoidPointerType();
        virtual Type* GetAliasType( ALIASTY ty );

        virtual HRESULT NewPointer( Type* pointed, Type*& pointer );
        virtual HRESULT NewReference( Type* pointed, Type*& pointer );
        virtual HRESULT NewDArray( Type* elem, Type*& type );
        virtual HRESULT NewAArray( Type* elem, Type* key, Type*& type );
        virtual HRESULT NewSArray( Type* elem, uint32_t length, Type*& type );
        virtual HRESULT NewStruct( Declaration* decl, Type*& type );
        virtual HRESULT NewEnum( Declaration* decl, Type*& type );
        virtual HRESULT NewTypedef( const wchar_t* name, Type* aliasedType, Type*& type );
        virtual HRESULT NewParam( StorageClass storage, Type* type, Parameter*& param );
        virtual HRESULT NewParams( ParameterList*& paramList );
        virtual HRESULT NewFunction( Type* returnType, ParameterList* params, int varArgs, Type*& type );
        virtual HRESULT NewDelegate( Type* funcType, Type*& type );
    };
}
