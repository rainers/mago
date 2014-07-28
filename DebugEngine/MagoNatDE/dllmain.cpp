/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: Implementation of DllMain and overrides of global ATL module
*/

#include "Common.h"
#include "resource.h"
#include "MagoNatDE_i.h"
#include "dllmain.h"
#include "dbgmetric_alt.h"

#define LEAK_CHECK (defined _DEBUG)
#if LEAK_CHECK
#include <crtdbg.h>
#endif

const bool              UserSpecific = false;
static const wchar_t    RegistrationRoot[] = L"Software\\Microsoft\\VisualStudio\\9.0";

CMagoNatDEModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
#if LEAK_CHECK
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
    hInstance;
    return _AtlModule.DllMain(dwReason, lpReserved); 
}


HRESULT CMagoNatDEModule::RegisterServer( BOOL bRegTypeLib, const CLSID* pCLSID ) throw()
{
#if REGISTER_AD7METRICS
    SetMetric( metrictypeEngine, ::GetEngineId(), 
        metricCLSID, __uuidof( MagoNativeEngine ), 
        UserSpecific, (USHORT*) RegistrationRoot );
    SetMetric( metrictypeEngine, ::GetEngineId(), 
        metricName, (USHORT*) ::GetEngineName(), 
        UserSpecific, (USHORT*) RegistrationRoot );
    SetMetric( metrictypeEngine, ::GetEngineId(), 
        metricENC, (DWORD) 0, 
        UserSpecific, (USHORT*) RegistrationRoot );
#endif

    return base::RegisterServer( bRegTypeLib, pCLSID );
}

HRESULT CMagoNatDEModule::UnregisterServer( BOOL bUnRegTypeLib, const CLSID* pCLSID ) throw()
{
#if REGISTER_AD7METRICS
    RemoveMetric( metrictypeEngine, ::GetEngineId(), metricCLSID, (USHORT*) RegistrationRoot );
    RemoveMetric( metrictypeEngine, ::GetEngineId(), metricName, (USHORT*) RegistrationRoot );
    RemoveMetric( metrictypeEngine, ::GetEngineId(), metricENC, (USHORT*) RegistrationRoot );
#endif

    return base::UnregisterServer( bUnRegTypeLib, pCLSID );
}
