/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Property.h"


namespace MagoEE
{
    //------------------------------------------------------------------------
    //  Base
    //------------------------------------------------------------------------

    class PropertyBase : public StdProperty
    {
    public:
        //virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type )

        virtual bool UsesParentValue();

        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );

        virtual bool GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result );
    };


    //------------------------------------------------------------------------
    //  Size for all
    //------------------------------------------------------------------------

    class PropertySize : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    //------------------------------------------------------------------------
    //  Integrals
    //------------------------------------------------------------------------

    class PropertyIntegralMax : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyIntegralMin : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    //------------------------------------------------------------------------
    //  Floats
    //------------------------------------------------------------------------

    class PropertyFloatMax : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatMin : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatEpsilon : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatNan : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatInfinity : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatDigits : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );

        static bool GetDigits( Type* parentType, int& digits );
    };


    class PropertyFloatMantissaDigits : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatMax10Exp : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatMaxExp : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatMin10Exp : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatMinExp : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyFloatReal : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool UsesParentValue();
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result );
    };


    class PropertyFloatImaginary : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool UsesParentValue();
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result );
    };


    class PropertySArrayLength : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyDArrayLength : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool UsesParentValue();
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result );
    };


    class PropertySArrayPtr : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };


    class PropertyDArrayPtr : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool UsesParentValue();
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result );
    };


    class PropertyDelegatePtr : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool UsesParentValue();
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result );
    };


    class PropertyDelegateFuncPtr : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool UsesParentValue();
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, const DataValue& parentVal , DataValue& result );
    };


    class PropertyFieldOffset : public PropertyBase
    {
    public:
        virtual bool GetType( ITypeEnv* typeEnv, Type* parentType, Declaration* parentDecl, Type*& type );
        virtual bool GetValue( Type* parentType, Declaration* parentDecl, DataValue& result );
    };
}
