/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "BpResolutionLocation.h"


namespace Mago
{
    class ATL_NO_VTABLE ErrorBreakpointResolution : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugErrorBreakpointResolution2
    {
        BP_ERROR_TYPE           mErrType;
        BpResolutionLocation    mResLoc;
        CComBSTR                mMsg;
        CComPtr<IDebugProgram2> mAD7Prog;
        CComPtr<IDebugThread2>  mAD7Thread;

    public:
        ErrorBreakpointResolution();
        ~ErrorBreakpointResolution();

    DECLARE_NOT_AGGREGATABLE(ErrorBreakpointResolution)

    BEGIN_COM_MAP(ErrorBreakpointResolution)
        COM_INTERFACE_ENTRY(IDebugErrorBreakpointResolution2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugErrorBreakpointResolution2 

        STDMETHOD( GetBreakpointType )( BP_TYPE* pBPType );
        STDMETHOD( GetResolutionInfo )( 
            BPERESI_FIELDS       dwFields,
            BP_ERROR_RESOLUTION_INFO* pErrorResolutionInfo );

    public:
        HRESULT Init( 
                BpResolutionLocation& bpresLoc, 
                IDebugProgram2* pProgram,
                IDebugThread2* pThread, 
                const wchar_t* msg,
                BP_ERROR_TYPE errType );
    };
}
