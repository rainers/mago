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

        HRESULT MakeTypeEnv( int ptrSize, ITypeEnv*& typeEnv )
        {
            return MagoEE::MakeTypeEnv( ptrSize, typeEnv );
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
            const EvalResult& parentVal,
            ITypeEnv* typeEnv,
            NameTable* strTable,
            const FormatOptions& fmtopts,
            IEEDEnumValues*& enumerator )
        {
            return MagoEE::EnumValueChildren( binder, parentExprText, parentVal, typeEnv, strTable, fmtopts, enumerator );
        }

        HRESULT FormatBasicValue( const DataObject& objVal, const FormatOptions& fmtopt, BSTR& outStr )
        {
            HRESULT         hr = S_OK;
            std::wstring    stdStr;

            hr = MagoEE::FormatBasicValue( objVal, fmtopt, stdStr );
            if ( FAILED( hr ) )
                return hr;

            outStr = SysAllocStringLen( stdStr.c_str(), stdStr.size() );
            if ( outStr == NULL )
                return E_OUTOFMEMORY;

            return S_OK;
        }

        HRESULT FormatValue( IValueBinder* binder, const DataObject& objVal, const FormatOptions& fmtopt,
                             BSTR& outStr, std::function<HRESULT(HRESULT, BSTR)> complete )
        {
            HRESULT         hr = S_OK;
            std::wstring    stdStr;

            auto completeEE = [complete, &outStr](HRESULT hr, std::wstring stdStr)
            {
                BSTR bStr = NULL;
                if( hr == S_OK )
                {
                    bStr = SysAllocStringLen(stdStr.c_str(), stdStr.size());
                    if( bStr == NULL )
                        hr = E_OUTOFMEMORY;
                }
                if (complete)
                    return complete(hr, bStr);
                outStr = bStr;
                return hr;
            };
            FormatData fmtdata{ fmtopt };
            hr = MagoEE::FormatValue( binder, objVal, fmtdata,
                                      complete ? completeEE : std::function<HRESULT(HRESULT, std::wstring)>{});
            if( FAILED( hr ) )
                return hr;
            return complete ? hr : completeEE(hr, fmtdata.outStr);
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

        // returns: S_FALSE on error not found

        HRESULT GetErrorString( HRESULT hresult, BSTR& outStr )
        {
            std::wstring errStr;
            HRESULT hr = MagoEE::GetErrorString( hresult, errStr );
            if ( hr != S_OK )
                return hr;

            outStr = SysAllocStringLen( errStr.c_str(), errStr.size() );
            if ( outStr == NULL )
                return E_OUTOFMEMORY;

            return S_OK;
        }
    }
}
