/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <functional>

namespace MagoEE
{
    HRESULT FormatBasicValue( const DataObject& objVal, const FormatOptions& fmtopt, std::wstring& outStr );
    HRESULT FormatValue( IValueBinder* binder, const DataObject& objVal, FormatData& fmtdata,
                         std::function<HRESULT(HRESULT, std::wstring)> complete );

    HRESULT GetRawStringLength( IValueBinder* binder, const DataObject& objVal, uint32_t& length );
    HRESULT FormatRawString(
        IValueBinder* binder, 
        const DataObject& objVal, 
        uint32_t bufCharLen,
        uint32_t& bufCharLenWritten,
        wchar_t* buf );

    HRESULT FormatTextViewerString( IValueBinder* binder, const DataObject& objVal, std::wstring& text );

	HRESULT FormatRawStructValue( IValueBinder* binder, const void* srcBuf, Type* type, FormatData& fmtdata );
}
