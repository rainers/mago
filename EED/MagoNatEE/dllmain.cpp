/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: Defines the entry point for the DLL
*/

#include "Common.h"


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  reason,
                       LPVOID lpReserved )
{
    UNREFERENCED_PARAMETER( lpReserved );

    switch ( reason )
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hModule );
        MagoEE::Init();
        break;

    case DLL_PROCESS_DETACH:
        MagoEE::Uninit();
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}
