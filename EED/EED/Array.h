/*
   Copyright (c) 2012 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


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

union aaAUnion
{
    aaA32 _32;
    aaA64 _64;
};
