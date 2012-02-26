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
    class ITypeEnv;


    struct DArray
    {
        dlength_t   Length;
        Address     Addr;
    };


    struct DDelegate
    {
        Address     ContextAddr;
        Address     FuncAddr;
    };


    enum DataValueKind
    {
        DataValueKind_None,
        DataValueKind_Int64,
        DataValueKind_UInt64,
        DataValueKind_Float80,
        DataValueKind_Complex80,
        DataValueKind_Addr,
        DataValueKind_Array,
        DataValueKind_Delegate,
    };


    union DataValue
    {
        int64_t         Int64Value;
        uint64_t        UInt64Value;
        Real10          Float80Value;
        Complex10       Complex80Value;
        Address         Addr;
        DArray          Array;
        DDelegate       Delegate;
    };


    struct DataObject
    {
        // Includes a Declaration member that can be used for things like S.a and s1.a 
        //      where S is a struct type, and s1 is a var of S type. 
        //      That needs to carry properties like offsetof.
        RefPtr<Type>        _Type;
        Address             Addr;
        DataValue           Value;

        // Since we're dealing only with values, we always have to have a type.
        // Objects with non-scalar types leave Value unused.
        //  literal or calculated value:    _Type
        //  calculated value with address:  _Type, Addr
    };


    struct EvalOptions
    {
        bool    AllowAssignment;
    };


    class IScope
    {
    public:
        virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl ) = 0;
        // TODO: add GetThis and GetSuper from IValueBinder?
    };


    // TODO: derive from IScope?
    class IValueBinder
    {
    public:
        virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl ) = 0;

        virtual HRESULT GetThis( Declaration*& decl ) = 0;
        virtual HRESULT GetSuper( Declaration*& decl ) = 0;
        virtual HRESULT GetReturnType( Type*& type ) = 0;

        virtual HRESULT GetValue( Declaration* decl, DataValue& value ) = 0;
        virtual HRESULT GetValue( Address addr, Type* type, DataValue& value ) = 0;
        virtual HRESULT GetValue( Address aArrayAddr, Type* keyType, const DataValue& keyValue, Address& valueAddr ) = 0;

        virtual HRESULT SetValue( Declaration* decl, const DataValue& value ) = 0;
        virtual HRESULT SetValue( Address addr, Type* type, const DataValue& value ) = 0;

        virtual HRESULT ReadMemory( Address addr, uint32_t sizeToRead, uint32_t& sizeRead, uint8_t* buffer ) = 0;
    };
}
