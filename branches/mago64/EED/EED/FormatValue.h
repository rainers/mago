/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    HRESULT FormatBasicValue( const DataObject& objVal, int radix, std::wstring& outStr );
    HRESULT FormatValue( IValueBinder* binder, const DataObject& objVal, int radix, std::wstring& outStr );

    HRESULT GetRawStringLength( IValueBinder* binder, const DataObject& objVal, uint32_t& length );
    HRESULT FormatRawString(
        IValueBinder* binder, 
        const DataObject& objVal, 
        uint32_t bufCharLen,
        uint32_t& bufCharLenWritten,
        wchar_t* buf );
}
