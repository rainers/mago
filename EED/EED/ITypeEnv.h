/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    class Type;
    class Declaration;
    class ParameterList;
    class Parameter;
    enum ENUMTY;
    enum ALIASTY;


    class ITypeEnv
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual int GetPointerSize() = 0;
        virtual Type* GetType( ENUMTY ty ) = 0;
        virtual Type* GetVoidPointerType() = 0;
        virtual Type* GetAliasType( ALIASTY ty ) = 0;

        virtual HRESULT NewPointer( Type* pointed, Type*& pointer ) = 0;
        virtual HRESULT NewReference( Type* pointed, Type*& pointer ) = 0;
        virtual HRESULT NewDArray( Type* elem, Type*& type ) = 0;
        virtual HRESULT NewAArray( Type* elem, Type* key, Type*& type ) = 0;
        virtual HRESULT NewSArray( Type* elem, uint32_t length, Type*& type ) = 0;
        virtual HRESULT NewStruct( Declaration* decl, Type*& type ) = 0;
        virtual HRESULT NewEnum( Declaration* decl, Type*& type ) = 0;
        virtual HRESULT NewTypedef( const wchar_t* name, Type* aliasedType, Type*& type ) = 0;
        virtual HRESULT NewParam( StorageClass storage, Type* type, Parameter*& param ) = 0;
        virtual HRESULT NewParams( ParameterList*& paramList ) = 0;
        virtual HRESULT NewFunction( Type* returnType, ParameterList* params, uint8_t callConv, int varArgs, Type*& type ) = 0;
        virtual HRESULT NewDelegate( Type* funcType, Type*& type ) = 0;
    };
}
