/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "EE.h"
#include "ITypeEnv.h"
#include "NameTable.h"
#include "Declaration.h"
#include "Eval.h"
#include "Type.h"
#include "FormatValue.h"
#include "FromRawValue.h"
#include "Array.h"


namespace MagoEE
{
    class Expression;

    struct EvalResult
    {
        DataObject  ObjVal;
        bool        ReadOnly;
        bool        HasString;
        bool        HasChildren;
        bool        HasRawChildren;
        bool        IsStaticField; // for struct enumerations
        bool        IsBaseClass;
        bool        IsMostDerivedClass;
        bool        IsArrayContinuation; // for arr[1000..length]
    };

    class IEEDParsedExpr
    {
    public:
        virtual ~IEEDParsedExpr() { }

        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual HRESULT Bind( const EvalOptions& options, IValueBinder* binder ) = 0;
        virtual HRESULT Evaluate( const EvalOptions& options, IValueBinder* binder, EvalResult& result ) = 0;
    };

    class IEEDEnumValues
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual uint32_t GetCount() = 0;
        virtual uint32_t GetIndex() = 0;
        virtual void Reset() = 0;
        virtual HRESULT Skip( uint32_t count ) = 0;
        virtual HRESULT Clone( IEEDEnumValues*& copiedEnum ) = 0;

        // TODO: can we use something else, like CString, instead of using wstring here?
        virtual HRESULT EvaluateNext( 
            const EvalOptions& options, 
            EvalResult& result,
            std::wstring& name,
            std::wstring& fullName ) = 0;
    };

    HRESULT Init();
    void Uninit();

    extern bool gShowVTable;
    extern bool gExpandableStrings;
    extern bool gHideReferencePointers;
    extern bool gRemoveLeadingHexZeroes;
    extern bool gRecombineTuples;
    extern bool gShowDArrayLengthInType;
    extern bool gCallDebuggerFunctions;
    extern bool gCallDebuggerUseMagoGC;

    extern uint32_t gMaxArrayLength;

    HRESULT MakeTypeEnv( int ptrSize, ITypeEnv*& typeEnv );
    HRESULT MakeNameTable( NameTable*& nameTable );
    HRESULT ParseText( const wchar_t* text, ITypeEnv* typeEnv, NameTable* strTable, IEEDParsedExpr*& expr );
    HRESULT StripFormatSpecifier( std::wstring& text, FormatOptions& fmtopt );
    HRESULT AppendFormatSpecifier( std::wstring& text, const FormatOptions& fmtopt );

    HRESULT EnumValueChildren( 
        IValueBinder* binder, 
        const wchar_t* parentExprText,
        const EvalResult& parentVal,
        ITypeEnv* typeEnv,
        NameTable* strTable,
        const FormatOptions& fmtopts,
        IEEDEnumValues*& enumerator );

    void FillValueTraits( IValueBinder* binder, EvalResult& result, Expression* expr );

    RefPtr<Type> GetDebuggerCall( ITypeStruct* ts, const wchar_t* call, Address& fnaddr );

    HRESULT GetErrorString( HRESULT hresult, std::wstring& outStr );

    int GetTupleName( const char* sym, size_t len, std::wstring* tupleName = nullptr );
    int GetTupleName( const wchar_t* sym, std::wstring* tupleName = nullptr );

    std::wstring to_wstring( const char* str, size_t slen );
}
