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

    HRESULT MakeTypeEnv( int ptrSize, ITypeEnv*& typeEnv );
    HRESULT MakeNameTable( NameTable*& nameTable );
    HRESULT ParseText( const wchar_t* text, ITypeEnv* typeEnv, NameTable* strTable, IEEDParsedExpr*& expr );
    HRESULT StripFormatSpecifier( std::wstring& text, FormatOptions& fmtopt );
    HRESULT AppendFormatSpecifier( std::wstring& text, const FormatOptions& fmtopt );

    HRESULT EnumValueChildren( 
        IValueBinder* binder, 
        const wchar_t* parentExprText,
        const DataObject& parentVal, 
        ITypeEnv* typeEnv,
        NameTable* strTable,
        const FormatOptions& fmtopts,
        IEEDEnumValues*& enumerator );

    void FillValueTraits( EvalResult& result, Expression* expr );

    HRESULT GetErrorString( HRESULT hresult, std::wstring& outStr );
}
