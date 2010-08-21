// testOptions.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>


int _tmain(int argc, _TCHAR* argv[])
{
    if ( argc != 4 )
        return 1;

    fwprintf( stderr, L"%ls\n", argv[3] );
    fwprintf( stderr, L"%ls\n", argv[1] );
    fwprintf( stderr, L"%ls\n", argv[2] );

    for ( int i = 0; i < 3; i++ )
    {
        char    varName[256] = "";
        char    varVal[256] = "";

        scanf_s( "%s", varName, _countof( varName ) );

        GetEnvironmentVariableA( varName, varVal, _countof( varVal ) );

        //OutputDebugStringA( varName );
        //OutputDebugStringA( varVal );
        
        printf( "%s\n", varVal );
    }

    return 0;
}

