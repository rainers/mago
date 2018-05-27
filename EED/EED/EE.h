/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


enum TRUST
{
    TRUSTdefault = 0,
    TRUSTsystem = 1,    // @system (same as TRUSTdefault)
    TRUSTtrusted = 2,   // @trusted
    TRUSTsafe = 3,      // @safe
};

typedef uint64_t    StorageClass;

static const StorageClass  STCproperty = 0x100000000LL;
static const StorageClass  STCsafe     = 0x200000000LL;
static const StorageClass  STCtrusted  = 0x400000000LL;
static const StorageClass  STCsystem   = 0x800000000LL;

enum STC
{
    STCundefined    = 0,
    STCstatic       = 1,
    STCextern       = 2,
    STCconst        = 4,
    STCfinal        = 8,
    STCabstract     = 0x10,
    STCparameter    = 0x20,
    STCfield        = 0x40,
    STCoverride     = 0x80,
    STCauto         = 0x100,
    STCsynchronized = 0x200,
    STCdeprecated   = 0x400,
    STCin           = 0x800,        // in parameter
    STCout          = 0x1000,       // out parameter
    STClazy         = 0x2000,       // lazy parameter
    STCforeach      = 0x4000,       // variable for foreach loop
    STCcomdat       = 0x8000,       // should go into COMDAT record
    STCvariadic     = 0x10000,      // variadic function argument
    STCctorinit     = 0x20000,      // can only be set inside constructor
    STCtemplateparameter = 0x40000, // template parameter
    STCscope        = 0x80000,      // template parameter
    STCinvariant    = 0x100000,
    STCimmutable    = 0x100000,
    STCref          = 0x200000,
    STCinit         = 0x400000,     // has explicit initializer
    STCmanifest     = 0x800000,     // manifest constant
    STCnodtor       = 0x1000000,    // don't run destructor
    STCnothrow      = 0x2000000,    // never throws exceptions
    STCpure         = 0x4000000,    // pure function
    STCtls          = 0x8000000,    // thread local
    STCalias        = 0x10000000,   // alias parameter
    STCshared       = 0x20000000,   // accessible from multiple threads
    STCgshared      = 0x40000000,   // accessible from multiple threads
                    // but not typed as "shared"
    STC_TYPECTOR    = (STCconst | STCimmutable | STCshared),
};

/* pick this order of numbers so switch statements work better
 */
enum MOD
{
    MODnone,
    MODconst     = 1,   // type is const
    MODinvariant = 4,   // type is immutable
    MODimmutable = 4,   // type is immutable
    MODshared    = 2,   // type is shared

    MODtypesMask = 7,

    MODstatic    = 0x10, // method is static
    MODvirtual   = 0x20, // method is virtual
};


namespace MagoEE
{
    typedef uint64_t    Address;
    typedef uint64_t    dlength_t;
    typedef int64_t     doffset_t;
    typedef int64_t     dptrdiff_t;
    typedef uint32_t    dchar_t;
}


// the Object hierarchy depends on this
#include <list>

#include "Error.h"
