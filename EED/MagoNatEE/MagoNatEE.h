/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "../Real/Real.h"
#include "../Real/Complex.h"
#include "../EED/EED.h"
#include <oleauto.h>

#if 1 // building into static library now
#define EXPORT 
#elif defined( MAGONATEE_EXPORTS )
#define EXPORT __declspec( dllexport )
#else
#define EXPORT __declspec( dllimport )
#endif


namespace MagoEE
{
    namespace EED
    {
        EXPORT HRESULT Init();
        EXPORT void Uninit();

        EXPORT HRESULT MakeTypeEnv( int ptrSize, ITypeEnv*& typeEnv );
        EXPORT HRESULT MakeNameTable( NameTable*& nameTable );

        EXPORT HRESULT ParseText( 
            const wchar_t* text, 
            ITypeEnv* typeEnv, 
            NameTable* strTable, 
            IEEDParsedExpr*& expr );

        EXPORT HRESULT EnumValueChildren( 
            IValueBinder* binder, 
            const wchar_t* parentExprText,
            const EvalResult& parentVal,
            ITypeEnv* typeEnv,
            NameTable* strTable,
            const FormatOptions& fmtopts,
            IEEDEnumValues*& enumerator );

        EXPORT HRESULT FormatBasicValue( const DataObject& objVal, const FormatOptions& fmtopt, BSTR& outStr );
        EXPORT HRESULT FormatValue( IValueBinder* binder, const DataObject& objVal, const FormatOptions& fmtopt, BSTR& outStr );

        EXPORT HRESULT GetRawStringLength( IValueBinder* binder, const DataObject& objVal, uint32_t& length );
        EXPORT HRESULT FormatRawString(
            IValueBinder* binder, 
            const DataObject& objVal, 
            uint32_t bufCharLen,
            uint32_t& bufCharLenWritten,
            wchar_t* buf );

        // returns: S_FALSE on error not found

        EXPORT HRESULT GetErrorString( HRESULT hresult, BSTR& outStr );
    }
}
