/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


inline HRESULT GetLastHr()
{
    return HRESULT_FROM_WIN32( GetLastError() );
}


struct OMFDirEntry;
union CodeViewSymbol;

// a Pascal String that is length prefixed and unterminated
// common storage format for strings in CodeView

struct PasString
{
    size_t GetLength() const 
    { 
        const unsigned char* p = data;
        return ReadLength( p );
    }

    const char* GetName() const 
    { 
        const unsigned char* p = data;
        ReadLength( p );
        return (const char*) p;
    }

    static size_t ReadLength( const unsigned char*& p )
    {
        // this uses a non-standard extension to CodeView that allows for long names
        // normally names are at most 255 chars, and length prefix is 1 byte

        size_t len = *p++;
        if ( len == 0xff && *p == 0 )
        {
            len = p[1] | (p[2] << 8);
            p += 3;
        }
        return len;
    }

    unsigned char data[1];
};

inline void assign(SymString& sym, const PasString* pas)
{
    const unsigned char* p = pas->data;
    size_t len = pas->ReadLength( p );
    sym.set( len, (const char*) p, false );
}

namespace MagoST
{
    struct TypeHandleIn
    {
        BYTE*           Type;
        WORD            Index;
        WORD            Tag;
    };

    struct SymHandleIn
    {
        CodeViewSymbol*     Sym;
        OMFDirEntry*        HeapDir;
    };

    struct EnumNamedSymbolsDataIn
    {
        void*               CurPair;
        void*               LimitPair;
        BYTE*               HeapBase;
        OMFDirEntry*        HeapDir;
        uint32_t            Hash;

        uint32_t            NameLen;
        const char*         NameChars;
        CodeViewSymbol*     Sym;
        OMFDirEntry*        SymHeapDir;
    };
}
