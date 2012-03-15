/*
   Copyright (c) 2012 Aldo J. Nunez

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
}
