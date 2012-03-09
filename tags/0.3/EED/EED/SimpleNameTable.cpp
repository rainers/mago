/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "SimpleNameTable.h"


namespace MagoEE
{
    SimpleNameTable::SimpleNameTable()
        :   mRefCount( 0 )
    {
    }

    void SimpleNameTable::AddRef()
    {
        mRefCount++;
    }

    void SimpleNameTable::Release()
    {
        mRefCount--;
        _ASSERT( mRefCount >= 0 );
        if ( mRefCount == 0 )
        {
            delete this;
        }
    }

    SimpleNameTable::~SimpleNameTable()
    {
        for ( ByteStringVector::iterator it = mByteStrs.begin();
            it != mByteStrs.end();
            it++ )
        {
            ByteString* s = *it;
            delete [] (char*) s;
        }

        for ( Utf16StringVector::iterator it = mUtf16Strs.begin();
            it != mUtf16Strs.end();
            it++ )
        {
            Utf16String* s = *it;
            delete [] (char*) s;
        }

        for ( Utf32StringVector::iterator it = mUtf32Strs.begin();
            it != mUtf32Strs.end();
            it++ )
        {
            Utf32String* s = *it;
            delete [] (char*) s;
        }
    }

    ByteString* SimpleNameTable::AddString( const char* str, size_t length )
    {
        char*       buf = new char[ sizeof( ByteString ) + (length + 1) * sizeof( char ) ];
        ByteString* byteStr = (ByteString*) buf;

        byteStr->Kind = StringKind_Byte;
        byteStr->Length = (uint32_t) length;
        byteStr->Str = buf + sizeof( ByteString );

        memcpy_s( byteStr->Str, length, str, length );
        byteStr->Str[length] = '\0';

        mByteStrs.push_back( byteStr );
        return byteStr;
    }

    Utf16String* SimpleNameTable::AddString( const wchar_t* str, size_t length )
    {
        char*           buf = new char[ sizeof( Utf16String ) + (length + 1) * sizeof( wchar_t ) ];
        Utf16String*    utf16Str = (Utf16String*) buf;

        utf16Str->Kind = StringKind_Utf16;
        utf16Str->Length = (uint32_t) length;
        utf16Str->Str = (wchar_t*) (buf + sizeof( Utf16String ));

        wmemcpy_s( utf16Str->Str, length, str, length );
        utf16Str->Str[length] = L'\0';

        mUtf16Strs.push_back( utf16Str );
        return utf16Str;
    }

    Utf32String* SimpleNameTable::AddString( const dchar_t* str, size_t length )
    {
        char*           buf = new char[ sizeof( Utf32String ) + (length + 1) * sizeof( dchar_t ) ];
        Utf32String*    utf32Str = (Utf32String*) buf;

        utf32Str->Kind = StringKind_Utf32;
        utf32Str->Length = (uint32_t) length;
        utf32Str->Str = (dchar_t*) (buf + sizeof( Utf32String ));

        memcpy_s( utf32Str->Str, length * 4, str, length * 4 );
        utf32Str->Str[length] = 0;

        mUtf32Strs.push_back( utf32Str );
        return utf32Str;
    }

    Utf16String* SimpleNameTable::GetEmpty()
    {
        static Utf16String  s;

        s.Kind = StringKind_Utf16;
        s.Length = 0;
        s.Str = L"";

        return &s;
    }
}
