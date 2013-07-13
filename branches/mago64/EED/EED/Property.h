/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    class ITypeEnv;
    class Type;
    class Declaration;
    union DataValue;


    class StdProperty
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type ) = 0;
        virtual bool UsesParentValue() = 0;
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result ) = 0;
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result ) = 0;
    };
}
