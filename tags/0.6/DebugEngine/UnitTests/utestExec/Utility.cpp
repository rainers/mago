/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "stdafx.h"
#include "Utility.h"


BOOL CreateAnonymousPipe( 
    HANDLE* readHandle, 
    HANDLE* writeHandle, 
    LPSECURITY_ATTRIBUTES secAttrRead, 
    LPSECURITY_ATTRIBUTES secAttrWrite,
    DWORD bufSize, 
    const wchar_t* pipeName, 
    bool overlappedRead, 
    bool overlappedWrite )
{
    BOOL            bRet = FALSE;
    FileHandlePtr   hRead;
    FileHandlePtr   hWrite;
    OVERLAPPED      overlapped = { 0 };
    HandlePtr       event( CreateEvent( NULL, TRUE, FALSE, NULL ) );
    DWORD           bytesTransferred = 0;
    DWORD           readOverlappedFlag = overlappedRead ? FILE_FLAG_OVERLAPPED : 0;
    DWORD           writeOverlappedFlag = overlappedWrite ? FILE_FLAG_OVERLAPPED : 0;

    overlapped.hEvent = event.Get();

    hWrite = CreateNamedPipe( 
        pipeName, 
        PIPE_ACCESS_OUTBOUND | writeOverlappedFlag, 
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_REJECT_REMOTE_CLIENTS, 
        1, 
        bufSize, 
        bufSize, 
        0, 
        secAttrWrite );
    if ( hWrite.IsEmpty() )
        return FALSE;

    hRead = CreateFile( 
        pipeName,
        GENERIC_READ,
        0,
        secAttrRead,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | readOverlappedFlag,
        NULL );
    if ( hRead.IsEmpty() )
        return FALSE;

    bRet = ConnectNamedPipe( hWrite.Get(), &overlapped );
    if ( bRet || (GetLastError() != ERROR_PIPE_CONNECTED) )
        return FALSE;

    *readHandle = hRead.Detach();
    *writeHandle = hWrite.Detach();

    return TRUE;
}


void ReadCallstackX86( HANDLE hProcess, HANDLE hThread, std::list<FrameX86>& stack )
{
    const int       MaxStackDepth = 10000;
    FrameX86        frame = { 0 };
#if _WIN64
    WOW64_CONTEXT   context = { 0 };
#else
    CONTEXT         context = { 0 };
#endif

#if _WIN64
    context.ContextFlags = WOW64_CONTEXT_CONTROL;
#else
    context.ContextFlags = CONTEXT_CONTROL;
#endif

#if _WIN64
    Wow64GetThreadContext( hThread, &context );
#else
    GetThreadContext( hThread, &context );
#endif

    DWORD   nextAddr = context.Ebp;
    DWORD   retAddr = context.Eip;

    for ( int i = 0; i < MaxStackDepth; i++ )
    {
        frame.Eip = retAddr;
        frame.Ebp = nextAddr;

        stack.push_back( frame );

        if ( nextAddr == 0 )
            break;

        BOOL    bRet = FALSE;
        SIZE_T  bytesRead = 0;

        bRet = ReadProcessMemory( hProcess, (void*) (nextAddr+sizeof nextAddr), &retAddr, sizeof retAddr, &bytesRead );
        if ( !bRet )
            break;

        bRet = ReadProcessMemory( hProcess, (void*) nextAddr, &nextAddr, sizeof nextAddr, &bytesRead );
        if ( !bRet )
            break;
    }
}


void ReadCallstackX64( HANDLE hProcess, HANDLE hThread, std::list<FrameX64>& stack )
{
    // disable printing x64 callstack, because current way is not right
#if 0
#ifdef _WIN64
    const int       MaxStackDepth = 10000;
    FrameX64        frame = { 0 };
    CONTEXT         context = { 0 };

    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    GetThreadContext( hThread, &context );

    printf( "-- %04x\n", context.SegCs );

    DWORD64 nextAddr = context.Rbp;
    DWORD64 retAddr = context.Rip;

    for ( int i = 0; i < MaxStackDepth; i++ )
    {
        frame.Rip = retAddr;
        frame.Rbp = nextAddr;

        stack.push_back( frame );

        if ( nextAddr == 0 )
            break;

        BOOL    bRet = FALSE;
        SIZE_T  bytesRead = 0;

        bRet = ReadProcessMemory( hProcess, (void*) (nextAddr+sizeof nextAddr), &retAddr, sizeof retAddr, &bytesRead );
        if ( !bRet )
            break;

        bRet = ReadProcessMemory( hProcess, (void*) nextAddr, &nextAddr, sizeof nextAddr, &bytesRead );
        if ( !bRet )
            break;
    }
#endif
#endif
}
