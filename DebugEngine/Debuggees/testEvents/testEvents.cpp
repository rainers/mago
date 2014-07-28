// testEvents.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>


void PrintInt( int x )
{
    char    str[11 + 1] = "";
    char*   s = str;
    int     rem = 0;
    int     quot = x;

    do
    {
        rem = quot % 10;
        quot = quot / 10;
        *s = '0' + rem;
        s++;
    } while ( quot > 0 );

    size_t  len = 0;

    while ( s != str )
    {
        len++;
        s--;
    }

    if ( len > 0 )
    {
        char*   begin = str;
        char*   end = str + len - 1;

        for ( ; begin < end; begin++, end-- )
        {
            char    c = *begin;
            *begin = *end;
            *end = c;
        }
    }

    OutputDebugStringA( str );
}

void Scenario1()
{
    // as part of testing, we might have an unhandled exception, so let us crash and not show a dialog
    SetErrorMode( SEM_NOGPFAULTERRORBOX );

    _asm
    {
        mov edx, 0
        mov eax, 107835
        mov ebx, 0
        idiv ebx

        ; either of these ways will do the trick
        //int 3
        pushf
        or dword ptr [esp], 100h
        popf
        ; the CPU runs one more instruction before the SS exception is raised
        ; so, put some filler that won't change eax
        nop
    }
}

void Scenario2()
{
    __try
    {
        int zero = 0;
        int n = 107835 / zero;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        _asm
        {
            mov eax, 1877514773
            int 3
        }
    }
}

int RunException( int scenario )
{
    int ret = 0;

    switch ( scenario )
    {
    case 1: Scenario1(); break;
    case 2: Scenario2(); break;
    //case 3: Scenario3(); break;
    default:
        ret = 1;
        break;
    }

    return ret;
}

int RunDetach( int scenario, const wchar_t* eventName )
{
    HANDLE hEvent = OpenEvent( SYNCHRONIZE, FALSE, eventName );
    if ( hEvent == NULL )
        return -1;

    int x = 10;
    _asm { int 3 }
    x++;
    WaitForSingleObject( hEvent, 10 * 1000 );
    CloseHandle( hEvent );
    x++;
    _asm { nop }
    x++;
    return x;
}

int RunAttach( int scenario, unsigned int cookie1 )
{
    wchar_t eventName[64] = L"";
    swprintf_s( eventName, L"utestExec_attach-%d", GetCurrentProcessId() );
    HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, eventName );
    if ( hEvent == NULL )
        return -1;

    SetEvent( hEvent );

    // if this program runs by itself, loop forever
    // else, the unit test will swallow the breakpoint exception and this program can end

    while ( true )
    {
        __try
        {
            unsigned int cookie2 = 0;
            _asm
            {
                int 3
                mov cookie2, eax
            }
            return cookie1 ^ cookie2;
        }
        __except ( 1 )
        {
        }
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    if ( (argc > 1) && (_wcsicmp( argv[1], L"exception" ) == 0) )
    {
        if ( argc < 3 )
            return -1;
        return RunException( _wtoi( argv[2] ) );
    }
    else if ( argc > 1 && _wcsicmp( argv[1], L"detach" ) == 0 )
    {
        if ( argc < 4 )
            return -1;
        return RunDetach( _wtoi( argv[2] ), argv[3] );
    }
    else if ( (argc > 1) && (_wcsicmp( argv[1], L"attach" ) == 0) )
    {
        if ( argc < 4 )
            return -1;
        return RunAttach( _wtoi( argv[2] ), _wtoi( argv[3] ) );
    }

    const wchar_t   RepeatingWstr[] = L"The daily news in Japanese is:  \x6bce\x65e5\x306e\x30cb\x30e5\x30fc\x30b9.";
    const char      RepeatingAstr[] = "The daily news in Spanish is:   la noticia diaria.";

    HMODULE hMod = NULL;

    hMod = LoadLibrary( L"gdi32.dll" );

    OutputDebugStringA( "I write an ASCII message." );
    OutputDebugStringA( "Filler in ASCII." );
    OutputDebugStringW( L"And now a Unicode one. \x02a7. Adiós!" ); // a tesh in the middle

    hMod = LoadLibrary( L"ws2_32.dll" );

    FreeLibrary( hMod );
    hMod = LoadLibrary( L"ws2_32.dll" );

    OutputDebugStringA( "I write an ASCII message." );
    OutputDebugStringW( L"a\x4e00\x6c34" L"Axyz_0123\xD834\xDD1E!?" );
    OutputDebugStringW( L"And now a Unicode one. Bye!" );

    hMod = LoadLibrary( L"tapi32.dll" );

    // longer than a memory page
    wchar_t longWstr[ 5000 ] = L"";
    char    longAstr[ 5000 ] = "";
    size_t  repWstrLen = wcslen( RepeatingWstr );
    size_t  repAstrLen = strlen( RepeatingAstr );

    for ( int i = 0; i < (_countof( longWstr ) / _countof( RepeatingWstr )); i++ )
    {
        size_t  charsLeft = _countof( longWstr ) - (i * repWstrLen);

        wcscpy_s( &longWstr[ i * repWstrLen ], charsLeft, RepeatingWstr );
    }

    for ( int i = 0; i < (_countof( longAstr ) / _countof( RepeatingAstr )); i++ )
    {
        size_t  charsLeft = _countof( longAstr ) - (i * repAstrLen);

        strcpy_s( &longAstr[ i * repAstrLen ], charsLeft, RepeatingAstr );
    }

    OutputDebugStringA( longAstr );
    OutputDebugStringW( longWstr );

    return 0;
}

