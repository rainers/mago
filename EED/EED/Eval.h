/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <string>
#include <vector>
#include <functional>

namespace MagoEE
{
    class Type;
    class Declaration;
    class ITypeEnv;
    class ITypeFunction;
    class ITypeStruct;
    struct String;


    struct DArray
    {
        dlength_t   Length;
        Address     Addr;
        String*     LiteralString;
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
        bool    AllowFuncExec;
        bool    AllowPropertyExec;
        uint8_t Radix;
        int     Timeout;

        static EvalOptions defaults;
    };

    struct FormatOptions
    {
        uint32_t radix;
        bool raw;   // with modifier '!'
        bool prop;  // with modifier '@'
        bool stack; // displayed in the call stack

        FormatOptions( uint32_t r = 0 ) : radix( r ), raw( false ), prop( false ), stack( false ) {}
    };

    static const uint32_t kMaxFormatValueLength = 100; // soft limit to eventually abort recursions

    struct FormatData
    {
        FormatOptions opt;
        std::wstring outStr;
        uint32_t maxLength;
        uint32_t maxRecursion;

        FormatData( const FormatOptions& o, uint32_t ml = 100, uint32_t r = 6 )
            : opt( o ), maxLength( ml ), maxRecursion( r ) {}

        FormatData newScope( uint32_t reserveLen = 0 )
        {
            auto outlen = (uint32_t) outStr.length() + reserveLen;
            auto maxLen = std::max<uint32_t>( maxLength, outlen ) - outlen;
            return FormatData( opt, maxLen, maxRecursion - ( maxRecursion > 0 ? 1 : 0 ) );
        }
        bool isTooLong() const { return outStr.length() >= maxLength || maxRecursion == 0; }
    };

    enum SupportedFormatSpecifiers
    {
        FormatSpecRaw = '!',
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
        enum
        {
            FindObjectLocal = 1 << 0,
            FindObjectClosure = 1 << 1,
            FindObjectGlobal = 1 << 2,
            FindObjectRegister = 1 << 3,
            FindObjectNoClassDeref = 1 << 8,
            FindObjectTryFQN = 1 << 9,
            FindObjectAny = FindObjectLocal | FindObjectClosure | FindObjectGlobal | FindObjectRegister,
        };
        virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl, uint32_t findFlags ) = 0;
        virtual HRESULT FindDebugFunc( const wchar_t* name, ITypeStruct* ts, Type*& type, Address& fnaddr ) = 0;

        virtual HRESULT GetThis( Declaration*& decl ) = 0;
        virtual HRESULT GetSuper( Declaration*& decl ) = 0;
        virtual HRESULT GetReturnType( Type*& type ) = 0;
        virtual HRESULT NewTuple( const wchar_t* name, const std::vector<RefPtr<Declaration>>& decls, Declaration*& decl ) = 0;

        virtual HRESULT GetAddress( Declaration* decl, Address& addr ) = 0;
        virtual HRESULT GetValue( Declaration* decl, DataValue& value ) = 0;
        virtual HRESULT GetValue( Address addr, Type* type, DataValue& value ) = 0;
        virtual HRESULT GetValue( Address aArrayAddr, const DataObject& key, Address& valueAddr ) = 0;
        virtual int GetAAVersion() = 0;
        virtual HRESULT GetClassName( Address addr, std::wstring& className, bool derefOnce ) = 0;

        virtual HRESULT SetValue( Declaration* decl, const DataValue& value ) = 0;
        virtual HRESULT SetValue( Address addr, Type* type, const DataValue& value ) = 0;

        virtual HRESULT ReadMemory( Address addr, uint32_t sizeToRead, uint32_t& sizeRead, uint8_t* buffer ) = 0;
        virtual HRESULT SymbolFromAddr( Address addr, std::wstring& symName, MagoEE::Type** pType, DWORD* pOffset = nullptr ) = 0;
        virtual HRESULT CallFunction( Address addr, ITypeFunction* func, Address arg, DataObject& value,
            bool saveGC, std::function<HRESULT(HRESULT, DataObject)> complete ) = 0;
    };
}
