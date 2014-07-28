/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// MagoRemote.cpp : Defines the entry point for the application.
//

// TODO: in addition to machine type, return processor features and OS version with launched or 
//       attached processes

#include "Common.h"
#include "MagoRemote.h"
#include "App.h"
#include <ShellAPI.h>
#include <string>


struct CmdLineOptions
{
    std::wstring    Error;
    const wchar_t*  SessionUuidStr;

    CmdLineOptions()
        :   SessionUuidStr( NULL )
    {
    }
};


void ParseCommandLine( int argc, wchar_t** argv, CmdLineOptions& options )
{
    if ( argc > 0 && wcscmp( argv[0], L"-exclusive" ) == 0 )
    {
        if ( argc == 2 )
        {
            options.SessionUuidStr = argv[1];
        }
        // else, set nothing, not even error message
    }
    else
    {
        options.Error = 
            L"The Mago Remote Debugging Agent doesn't support running separately from Visual Studio.";
    }
}

int APIENTRY wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow )
{
    UNREFERENCED_PARAMETER( hInstance );
	UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( nCmdShow );

    int ret = 0;
    int argc = 0;
    wchar_t** argv = NULL;
    CmdLineOptions options;

    if ( lpCmdLine != NULL && lpCmdLine[0] != L'\0' )
    {
        argv = CommandLineToArgvW( lpCmdLine, &argc );
        if ( argv == NULL )
            return -1;
    }

    ParseCommandLine( argc, argv, options );

    if ( options.SessionUuidStr != NULL )
    {
        ret = Mago::RunSingleSessionLoop( options.SessionUuidStr );
    }
    else if ( options.Error.size() > 0 )
    {
        MessageBox( NULL, options.Error.c_str(), L"MagoRemote", MB_OK | MB_ICONERROR );
    }

    // passing in NULL does nothing
    LocalFree( argv );

    return ret;
}
