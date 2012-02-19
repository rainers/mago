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
