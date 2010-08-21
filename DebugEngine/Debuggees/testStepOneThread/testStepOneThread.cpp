// testStepOneThread.cpp : Defines the entry point for the console application.
//
// we turn off incremental linking in all builds, so we don't get the extra jump to a function
// we also change the Debug Information Format to Program Database (/Zi), because incremental linking is off

#include "stdafx.h"
#include <windows.h>


void __declspec( naked ) Scenario1Func3()
{
    _asm
    {
        ret
    }
}

void __declspec( naked ) Scenario1Func2()
{
    _asm
    {
        rep stos byte ptr [edi]
        rep stos byte ptr [edi]
        sub edi, 0
        mov ecx, 2
        rep stos byte ptr [edi]
        call Scenario1Func3
        ret
    }
}

void __declspec( naked ) Scenario1Func1()
{
    _asm
    {
        call Scenario1Func2
        ret
    }
}

void Scenario1()
{
    char    array[10] = { 0 };

    _asm
    {
        lea edi, array
        mov ecx, 5
        int 3
        mov al, 65
        call Scenario1Func1
    }

    for ( int i = 0; i < 7; i++ )
    {
        if ( array[i] != (char) 65 )
            RaiseException( 0xAA0034fc, 0, 0, NULL );
    }

    for ( int i = 7; i < _countof( array ); i++ )
    {
        if ( array[i] != (char) 0 )
            RaiseException( 0xAA0034fd, 0, 0, NULL );
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    if ( argc < 2 )
        return 1;

    int scenario = _wtoi( argv[1] );

    switch ( scenario )
    {
    case 1: Scenario1(); break;
    }

    return 0;
}

