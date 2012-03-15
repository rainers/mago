/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


typedef DWORD PVOID_32;
typedef DWORD PPVOID_32;
typedef DWORD PWSTR_32;
typedef DWORD HANDLE_32;
typedef DWORD PPEB_32;

typedef DWORD64 PVOID_64;
typedef DWORD64 PPVOID_64;
typedef DWORD64 PWSTR_64;
typedef DWORD64 HANDLE_64;
typedef DWORD64 PPEB_64;


struct CLIENT_ID_32
{
   DWORD     UniqueProcess;
   DWORD     UniqueThread;
};

struct CLIENT_ID_64
{
   DWORD     UniqueProcess;
   DWORD     Reserved1;
   DWORD     UniqueThread;
   DWORD     Reserved2;
};


//----------------------------------------------------------------------------
// TEB
//----------------------------------------------------------------------------

struct TEB32
{
    NT_TIB32                     Tib;                               // 000
    PVOID_32                     EnvironmentPointer;                // 01c
    CLIENT_ID_32                 ClientId;                          // 020
    PVOID_32                     ActiveRpcHandle;                   // 028
    PVOID_32                     ThreadLocalStoragePointer;         // 02c
    PPEB_32                      Peb;                               // 030
    ULONG                        LastErrorValue;                    // 034
    BYTE                         Reserved1[0x8C];                   // 038
    ULONG                        Lcid;                              // 0c4
    BYTE                         Reserved2[0xD48];                  // 0c8
    PVOID                        TlsSlots[64];                      // e10
};


struct TEB64
{
    NT_TIB64                     Tib;                               // 0000
    PVOID_64                     EnvironmentPointer;                // 0038
    CLIENT_ID_64                 ClientId;                          // 0040
    PVOID_64                     ActiveRpcHandle;                   // 0050
    PVOID_64                     ThreadLocalStoragePointer;         // 0058
    PPEB_64                      Peb;                               // 0060
    ULONG                        LastErrorValue;                    // 0068
    BYTE                         Reserved1[0x9c];                   // 006c
    ULONG                        Lcid;                              // 0108
    BYTE                         Reserved2[0x1374];                 // 010c
    PVOID64                      TlsSlots[64];                      // 1480
};
