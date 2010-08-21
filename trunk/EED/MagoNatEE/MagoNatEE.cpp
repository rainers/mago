/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: Defines the exported functions for the DLL
*/

#include "Common.h"
#include "MagoNatEE.h"


namespace MagoEE
{
    namespace EED
    {
        HRESULT Init()
        {
            return MagoEE::Init();
        }

        void Uninit()
        {
            MagoEE::Uninit();
        }

        HRESULT MakeTypeEnv( ITypeEnv*& typeEnv )
        {
            return MagoEE::MakeTypeEnv( typeEnv );
        }

        HRESULT MakeNameTable( NameTable*& nameTable )
        {
            return MagoEE::MakeNameTable( nameTable );
        }

        HRESULT ParseText( const wchar_t* text, ITypeEnv* typeEnv, NameTable* strTable, IEEDParsedExpr*& expr )
        {
            return MagoEE::ParseText( text, typeEnv, strTable, expr );
        }

        HRESULT EnumValueChildren( 
            IValueBinder* binder, 
            const wchar_t* parentExprText, 
            const DataObject& parentVal, 
            ITypeEnv* typeEnv,
            NameTable* strTable,
            IEEDEnumValues*& enumerator )
        {
            return MagoEE::EnumValueChildren( binder, parentExprText, parentVal, typeEnv, strTable, enumerator );
        }

        HRESULT FormatBasicValue( const DataObject& objVal, int radix, BSTR& outStr )
        {
            HRESULT         hr = S_OK;
            std::wstring    stdStr;

            hr = MagoEE::FormatBasicValue( objVal, radix, stdStr );
            if ( FAILED( hr ) )
                return hr;

            outStr = SysAllocStringLen( stdStr.c_str(), stdStr.size() );
            if ( outStr == NULL )
                return E_OUTOFMEMORY;

            return S_OK;
        }

        HRESULT FormatValue( IValueBinder* binder, const DataObject& objVal, int radix, BSTR& outStr )
        {
            HRESULT         hr = S_OK;
            std::wstring    stdStr;

            hr = MagoEE::FormatValue( binder, objVal, radix, stdStr );
            if ( FAILED( hr ) )
                return hr;

            outStr = SysAllocStringLen( stdStr.c_str(), stdStr.size() );
            if ( outStr == NULL )
                return E_OUTOFMEMORY;

            return S_OK;
        }

        HRESULT GetRawStringLength( IValueBinder* binder, const DataObject& objVal, uint32_t& length )
        {
            return MagoEE::GetRawStringLength( binder, objVal, length );
        }

        HRESULT FormatRawString(
            IValueBinder* binder, 
            const DataObject& objVal, 
            uint32_t bufCharLen,
            uint32_t& bufCharLenWritten,
            wchar_t* buf )
        {
            return MagoEE::FormatRawString( binder, objVal, bufCharLen, bufCharLenWritten, buf );
        }


        static const wchar_t    gCommonErrStr[] = L": Error: ";

        static const wchar_t*   gErrStrs[] = 
        {
            L"Expression couldn't be evaluated",
            L"Syntax error",
            L"Incompatible types for operator",
            L"Value expected",
            L"Expression has no type",
            L"Type resolve failed",
            L"Bad cast",
            L"Expression has no address",
            L"L-value expected",
            L"Can't cast to bool",
            L"Divide by zero",
            L"Bad indexing operation",
            L"Symbol not found",
        };

        // returns: S_FALSE on error not found

        HRESULT GetErrorString( HRESULT hresult, BSTR& outStr )
        {
            DWORD   fac = HRESULT_FACILITY( hresult );
            DWORD   code = HRESULT_CODE( hresult );

            if ( fac != MagoEE::HR_FACILITY )
                return S_FALSE;

            if ( code >= _countof( gErrStrs ) )
                code = 0;

            size_t  len = wcslen( gErrStrs[code] );
            size_t  commonLen = wcslen( gCommonErrStr );
            size_t  totalLen = 5 + commonLen + len;     // "D0000"

            outStr = SysAllocStringLen( NULL, totalLen );
            if ( outStr == NULL )
                return E_OUTOFMEMORY;

            swprintf_s( outStr, totalLen + 1, L"D%04d%ls%ls", code + 1, gCommonErrStr, gErrStrs[code] );

            return S_OK;
        }
    }
}
