/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class ATL_NO_VTABLE ErrorBreakpoint : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugErrorBreakpoint2
    {
        CComPtr<IDebugPendingBreakpoint2>           mPendingBP;
        CComPtr<IDebugErrorBreakpointResolution2>   mBPRes;

    public:
        ErrorBreakpoint();
        ~ErrorBreakpoint();

    DECLARE_NOT_AGGREGATABLE(ErrorBreakpoint)

    BEGIN_COM_MAP(ErrorBreakpoint)
        COM_INTERFACE_ENTRY(IDebugErrorBreakpoint2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugErrorBreakpoint2 

        STDMETHOD( GetPendingBreakpoint )( IDebugPendingBreakpoint2** ppPendingBreakpoint );
        STDMETHOD( GetBreakpointResolution )( IDebugErrorBreakpointResolution2** ppErrorResolution );

    public:
        void    Init( 
            IDebugPendingBreakpoint2* ppPendingBreakpoint, 
            IDebugErrorBreakpointResolution2* ppErrorResolution );
    };
}
