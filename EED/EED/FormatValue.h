/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    static const uint32_t kMaxFormatValueLength = 100; // soft limit to eventually abort recursions

    HRESULT FormatBasicValue( const DataObject& objVal, const FormatOptions& fmtopt, std::wstring& outStr );
    HRESULT FormatValue( IValueBinder* binder, const DataObject& objVal, const FormatOptions& fmtopt, std::wstring& outStr, uint32_t maxLength );

    HRESULT GetRawStringLength( IValueBinder* binder, const DataObject& objVal, uint32_t& length );
    HRESULT FormatRawString(
        IValueBinder* binder, 
        const DataObject& objVal, 
        uint32_t bufCharLen,
        uint32_t& bufCharLenWritten,
        wchar_t* buf );

    HRESULT FormatTextViewerString( IValueBinder* binder, const DataObject& objVal, std::wstring& text );

	HRESULT FormatRawStructValue( IValueBinder* binder, const void* srcBuf, Type* type, const FormatOptions& fmtopt, std::wstring& outStr, uint32_t maxLength );
}
