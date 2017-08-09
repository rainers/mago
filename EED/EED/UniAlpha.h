/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    bool IsHexDigit( wchar_t c );
    bool IsOctalDigit( wchar_t c );
    bool IsIdentChar( wchar_t c );
    bool IsUniAlpha( wchar_t ch );
    bool IsIdentifier( const wchar_t* str );

    int Utf8To32( const char* utf8Str, int utf8Len, dchar_t* utf32Str, int utf32Len );
    int Utf8To16( const char* utf8Str, int utf8Len, wchar_t* utf16Str, int utf16Len );
    int Utf16To8( const wchar_t* utf16Str, int utf16Len, char* utf8Str, int utf8Len );
    int Utf16To32( const wchar_t* utf16Str, int utf16Len, dchar_t* utf32Str, int utf32Len );
    int Utf32To16( bool ignoreErrors, const dchar_t* utf32Str, int utf32Len, wchar_t* utf16Str, int utf16Len, bool& truncated );

    void AppendChar32( std::wstring& str, dchar_t c );
    void AppendChar32( std::vector<char>& str, dchar_t c );

    size_t dcslen( const dchar_t* s );
    dchar_t* dmemchr( dchar_t* s, dchar_t c, size_t n );
}
