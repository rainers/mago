/*
   Copyright (c) 2012 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

// V0: up to dmd 2.067 - buckets with linked lists
// V1: dmd 2.068       - open address hash map

struct DArray32
{
    uint32_t    length; // size_t
    uint32_t    ptr;    // T*
};

struct aaA32
{
    uint32_t    next;   // aaA*
    uint32_t    hash;   // size_t
    // key
    // value
};

struct BB32
{
    DArray32    b;      // aaA*[]
    uint32_t    nodes;  // size_t
    uint32_t    firstUsedBucket;  // size_t, added in dmd 2.067
    uint32_t    keyti;  // TypeInfo
    // aaA*[4]  binit
};

struct Bucket32
{
    uint32_t    hash;   // size_t
    uint32_t    entry;  // void*
};

struct BB32_V1
{
    DArray32    buckets;   // Bucket[]
    uint32_t    used;      // uint
    uint32_t    deleted;   // uint
    uint32_t    entryTI;   // TypeInfo_Struct 
    uint32_t    firstUsed; // uint
    uint32_t    keysz;     // uint 
    uint32_t    valsz;     // uint 
    uint32_t    valoff;    // uint 
    uint8_t     flags;     // Flags keyHasPostblit(1), hasPointers(2)
};

struct DArray64
{
    uint64_t    length; // size_t
    uint64_t    ptr;    // T*
};

struct aaA64
{
    uint64_t    next;   // aaA*
    uint64_t    hash;   // size_t
    // key
    // value
};

struct BB64
{
    DArray64    b;      // aaA*[]
    uint64_t    nodes;  // size_t
    uint64_t    firstUsedBucket;  // size_t, added in dmd 2.067
    uint64_t    keyti;  // TypeInfo
    // aaA*[4]  binit
};

struct Bucket64
{
    uint64_t    hash;   // size_t
    uint64_t    entry;  // void*
};

struct BB64_V1
{
    DArray64    buckets;   // Bucket[]
    uint32_t    used;      // uint
    uint32_t    deleted;   // uint
    uint64_t    entryTI;   // TypeInfo_Struct 
    uint32_t    firstUsed; // uint
    uint32_t    keysz;     // uint 
    uint32_t    valsz;     // uint 
    uint32_t    valoff;    // uint 
    uint8_t     flags;     // Flags keyHasPostblit(1), hasPointers(2)
};

union aaAUnion
{
    aaA32 _32;
    aaA64 _64;
};
