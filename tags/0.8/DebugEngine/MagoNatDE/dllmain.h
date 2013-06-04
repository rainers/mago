/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: global module object used for ATL and COM features
*/

class CMagoNatDEModule : public CAtlDllModuleT< CMagoNatDEModule >
{
    typedef CAtlDllModuleT< CMagoNatDEModule >  base;

public :
    DECLARE_LIBID(LIBID_MagoNatDELib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_MAGONATDE, "{B84E2188-D769-444A-AF27-95165CE204B0}")

    HRESULT RegisterServer( BOOL bRegTypeLib = FALSE, const CLSID* pCLSID = NULL ) throw();
    HRESULT UnregisterServer( BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL ) throw();
};

extern class CMagoNatDEModule _AtlModule;
