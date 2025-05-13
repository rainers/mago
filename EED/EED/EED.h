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

#include <functional>

////////////////////////////////
// async completion:
// - enabled by passing non-empty complete function
// - if result delayed, return S_QUEUED
// - if result evaluated eagerly, return S_OK
// - if error occurs before queing complete, don't call complete
// - if error occurs during execution of complete, forward it to next complete, but
//   keep success status S_OK or S_QUEUED
// 
// to be used as return code for async requests, S_OK means evaluated eagerly
#define S_QUEUED ((HRESULT)2L)
#define E_OPERATIONCANCELED 0x8013153b // identical to COR_E_OPERATIONCANCELED

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
        virtual HRESULT Evaluate(const EvalOptions& options, IValueBinder* binder, EvalResult& result,
            std::function<HRESULT(HRESULT, EvalResult)> complete ) = 0;
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

        struct EvaluateNextResult
        {
            std::wstring name;
            std::wstring fullName;
            EvalResult result = { 0 };
        };

        // TODO: can we use something else, like CString, instead of using wstring here?
        virtual HRESULT EvaluateNext( 
            const EvalOptions& options, 
            EvalResult& result,
            std::wstring& name,
            std::wstring& fullName,
            std::function<HRESULT(HRESULT hr, EvaluateNextResult)> complete ) = 0;
    };

    HRESULT Init();
    void Uninit();

    extern bool gShowVTable;
    extern bool gExpandableStrings;
    extern bool gHideReferencePointers;
    extern bool gRemoveLeadingHexZeroes;
    extern bool gRecombineTuples;
    extern bool gShortenTypeNames;
    extern bool gShowDArrayLengthInType;
    extern bool gCallDebuggerFunctions;
    extern bool gCallDebuggerRanges;
    extern bool gCallPropertyMethods;
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

    HRESULT FillValueTraits( IValueBinder* binder, EvalResult& result, Expression* expr,
        std::function<HRESULT(HRESULT, EvalResult)> complete );

    RefPtr<Type> GetDebuggerProp( IValueBinder* binder, ITypeStruct* ts, const wchar_t* call, Address& fnaddr );
    RefPtr<Type> GetDebuggerPropType( Type* fntype );
    HRESULT EvalDebuggerProp( IValueBinder* binder, RefPtr<Type> fntype, Address fnaddr,
                              Address objAddr, DataObject& propValue,
                              std::function<HRESULT(HRESULT, DataObject)> complete );
    bool IsForwardRange( IValueBinder* binder, Type* type );

    HRESULT GetErrorString( HRESULT hresult, std::wstring& outStr );

    int GetTupleName( const char* sym, size_t len, std::wstring* tupleName = nullptr );
    int GetTupleName( const wchar_t* sym, std::wstring* tupleName = nullptr );

    int GetParamIndex( const char* sym, size_t len );
    int GetParamIndex( const wchar_t* sym );

    std::wstring to_wstring( const char* str, size_t slen );
    std::string to_string( const wchar_t* str, size_t slen );
}
