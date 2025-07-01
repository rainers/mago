/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

struct SymString
{
    size_t length : 24;
    size_t alloced : 1;
    const char* ptr;

    size_t GetLength() const { return length; }
    const char* GetName() const { return ptr; }

    SymString() : length( 0 ), ptr( NULL ), alloced( false ) {}
    ~SymString() { if ( alloced ) delete [] ptr; }

    void set( size_t len, const char* p, bool alloc )
    {
        if ( alloced ) delete [] ptr;
        length = len;
        ptr = p;
        alloced = alloc;
    }
};

namespace MagoST
{
    enum SymbolHeapId
    {
        SymHeap_GlobalSymbols,
        SymHeap_StaticSymbols,
        SymHeap_PublicSymbols,
        SymHeap_Max,
        SymHeap_Count = SymHeap_Max
    };

    struct TypeHandle
    {
        intptr_t unused1;
        int      unused2;
    };

    struct SymHandle
    {
        intptr_t unused1;
        intptr_t unused2;
    };

    struct SymInfoData
    {
        intptr_t unused1;
        intptr_t unused2;
        intptr_t unused3;
        intptr_t unused4;
    };

    struct EnumNamedSymbolsData
    {
        intptr_t    unused1;
        intptr_t    unused2;
        intptr_t    unused3;
        intptr_t    unused4;
        int         unused5;

        int         unused6;
        intptr_t    unused7;
        intptr_t    unused8;
        intptr_t    unused9;
    };

    struct SymbolScope
    {
        intptr_t    unused1;
        intptr_t    unused2;
        intptr_t    unused3;
        intptr_t    unused4;
        intptr_t    unused5;
    };

    struct TypeScope
    {
        intptr_t    unused1;
        intptr_t    unused2;
        intptr_t    unused3;
        intptr_t    unused4;
        char        unused5[4];
    };


    typedef DWORD TypeIndex;

    struct Complex32
    {
        float   Real;
        float   Imaginary;
    };

    struct Complex64
    {
        double  Real;
        double  Imaginary;
    };

    struct VarString
    {
        uint16_t    Length;
        const char* Chars;
    };

    enum VariantTag
    {
        VarTag_Char = 0x8000,
        VarTag_UChar = 0x8011,
        VarTag_Short = 0x8001,
        VarTag_UShort = 0x8002,
        VarTag_Long = 0x8003,
        VarTag_ULong = 0x8004,
        VarTag_Real32 = 0x8005,
        VarTag_Real64 = 0x8006,
        VarTag_Real80 = 0x8007,
        VarTag_Real128 = 0x8008,
        VarTag_Quadword = 0x8009,
        VarTag_UQuadword = 0x800a,
        VarTag_Real48 = 0x800b,
        VarTag_Complex32 = 0x800c,
        VarTag_Complex64 = 0x800d,
        VarTag_Complex80 = 0x800e,
        VarTag_Complex128 = 0x800f,
        VarTag_Varstring = 0x8010
    };

    struct Variant
    {
        union 
        {
            int8_t      I8;
            uint8_t     U8;
            int16_t     I16;
            uint16_t    U16;
            int32_t     I32;
            uint32_t    U32;
            int64_t     I64;
            uint64_t    U64;
            float       F32;
            double      F64;
            Complex32   C32;
            Complex64   C64;
            VarString   VarStr;
            uint8_t     RawBytes[32];   // enough to hold everything else, the biggest of which is Complex-128
        } Data;

        VariantTag      Tag;
    };


    struct CompilandInfo
    {
        WORD            SegmentCount;
        WORD            FileCount;
        SymString       Name;
    };

    struct SegmentInfo
    {
        WORD            SegmentIndex;
        WORD            LineCount;
        DWORD           StartOffset;
        DWORD           EndOffset;
    };

    struct FileInfo
    {
        WORD            SegmentCount;
        SymString       Name;
    };

    struct LineInfo
    {
        DWORD           Offset;
        WORD            LineNumber;
        WORD            Reserved1;
    };

    struct FileSegmentInfo
    {
        WORD            SegmentIndex;
        WORD            SegmentInstance;
        DWORD           Start;
        DWORD           End;
        WORD            LineCount;
        DWORD*          Offsets;
        WORD*           LineNumbers;
    };

    struct LineNumber
    {
        uint16_t    CompilandIndex;
        uint16_t    FileIndex;
        uint16_t    SegmentInstanceIndex;
        uint16_t    LineIndex;
        uint16_t    Number;
        uint16_t    NumberEnd;
        uint16_t    Section;
        uint32_t    Offset;
        uint32_t    Length;
    };
}
