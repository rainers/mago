/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "NameTable.h"


namespace MagoEE
{
    class SimpleNameTable : public NameTable
    {
        typedef std::vector<ByteString*>    ByteStringVector;
        typedef std::vector<Utf16String*>   Utf16StringVector;
        typedef std::vector<Utf32String*>   Utf32StringVector;

        long                mRefCount;
        ByteStringVector    mByteStrs;
        Utf16StringVector   mUtf16Strs;
        Utf32StringVector   mUtf32Strs;

    public:
        SimpleNameTable();
        ~SimpleNameTable();

        virtual void AddRef();
        virtual void Release();

        virtual ByteString* AddString( const char* str, size_t length );
        virtual Utf16String* AddString( const wchar_t* str, size_t length );
        virtual Utf32String* AddString( const dchar_t* str, size_t length );

        virtual Utf16String* GetEmpty();
    };
}
