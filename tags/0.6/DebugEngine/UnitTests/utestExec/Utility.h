/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


BOOL CreateAnonymousPipe( 
    HANDLE* readHandle, 
    HANDLE* writeHandle, 
    LPSECURITY_ATTRIBUTES secAttrRead, 
    LPSECURITY_ATTRIBUTES secAttrWrite,
    DWORD bufSize, 
    const wchar_t* pipeName, 
    bool overlappedRead, 
    bool overlappedWrite );


struct FrameX86
{
    DWORD   Eip;
    DWORD   Ebp;
};

struct FrameX64
{
    DWORD64 Rip;
    DWORD64 Rbp;
};

void ReadCallstackX86( HANDLE hProcess, HANDLE hThread, std::list<FrameX86>& stack );
void ReadCallstackX64( HANDLE hProcess, HANDLE hThread, std::list<FrameX64>& stack );


#ifdef _WIN64

typedef WOW64_CONTEXT               CONTEXT_X86;
#define CONTEXT_X86_FULL            WOW64_CONTEXT_FULL
#define GetThreadContextX86         Wow64GetThreadContext
#define SetThreadContextX86         Wow64SetThreadContext
#define EXCEPTION_BREAKPOINT_X86    0x4000001F
#define EXCEPTION_SINGLE_STEP_X86   0x4000001E

#else

typedef CONTEXT                     CONTEXT_X86;
#define CONTEXT_X86_FULL            CONTEXT_FULL
#define GetThreadContextX86         GetThreadContext
#define SetThreadContextX86         SetThreadContext
#define EXCEPTION_BREAKPOINT_X86    EXCEPTION_BREAKPOINT
#define EXCEPTION_SINGLE_STEP_X86   EXCEPTION_SINGLE_STEP

#endif
