/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "BpResolutionLocation.h"


namespace Mago
{
    class BreakpointResolution : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugBreakpointResolution2
    {
        BpResolutionLocation    mResLoc;
        CComPtr<IDebugProgram2> mAD7Prog;
        CComPtr<IDebugThread2>  mAD7Thread;

    public:
        BreakpointResolution();
        ~BreakpointResolution();

    DECLARE_NOT_AGGREGATABLE(BreakpointResolution)

    BEGIN_COM_MAP(BreakpointResolution)
        COM_INTERFACE_ENTRY(IDebugBreakpointResolution2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugBreakpointResolution2 

        STDMETHOD( GetBreakpointType )( BP_TYPE* pBPType );
        STDMETHOD( GetResolutionInfo )( 
            BPRESI_FIELDS       dwFields,
            BP_RESOLUTION_INFO* pBPResolutionInfo );

    public:
        // TODO: move with r-value refs
        HRESULT Init( 
                BpResolutionLocation& bpresLoc, 
                IDebugProgram2* pProgram,
                IDebugThread2* pThread );
    };
}
