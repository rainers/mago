/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


typedef uintptr_t   Address;
typedef uintptr_t   MachineAddress;
typedef uint64_t    BPCookie;

struct AddressRange
{
    Address     Begin;
    Address     End;
};

enum RunMode
{
    RunMode_Run,
    RunMode_Break,
    RunMode_Wait,
};
