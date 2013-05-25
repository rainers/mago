// testThread.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


DWORD CALLBACK ThreadProc( void* param )
{
    Sleep( 1500 );
    _asm
    {
        //mov ebx, 0
        //idiv ebx
        //int 3
        mov eax, 0
    }
    Sleep( 6000 );
    return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
    HANDLE  hThread[2] = { 0 };

    Sleep( 1000 );

    hThread[0] = CreateThread(
        NULL,
        0,
        ThreadProc,
        NULL,
        0,
        NULL );

    hThread[1] = CreateThread(
        NULL,
        0,
        ThreadProc,
        NULL,
        0,
        NULL );

    Sleep( 4500 );

    WaitForMultipleObjects( _countof( hThread ), hThread, TRUE, INFINITE );

    return 0;
}

