/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    enum StringKind
    {
        StringKind_Byte,
        StringKind_Utf16,
        StringKind_Utf32
    };

    struct String
    {
        StringKind  Kind;
        uint32_t    Length;
    };

    struct ByteString : public String
    {
        char*       Str;
    };

    struct Utf16String : public String
    {
        wchar_t*    Str;
    };

    struct Utf32String : public String
    {
        dchar_t*    Str;
    };

    class NameTable
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual ByteString* AddString( const char* str, size_t length ) = 0;
        virtual Utf16String* AddString( const wchar_t* str, size_t length ) = 0;
        virtual Utf32String* AddString( const dchar_t* str, size_t length ) = 0;

        virtual Utf16String* GetEmpty() = 0;
    };
}
