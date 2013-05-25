/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ErrorBreakpoint.h"


namespace Mago
{
    // ErrorBreakpoint

    ErrorBreakpoint::ErrorBreakpoint()
    {
    }

    ErrorBreakpoint::~ErrorBreakpoint()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugErrorBreakpoint2 

    HRESULT ErrorBreakpoint::GetPendingBreakpoint( IDebugPendingBreakpoint2** ppPendingBreakpoint ) 
    {
        if ( ppPendingBreakpoint == NULL )
            return E_INVALIDARG;

        *ppPendingBreakpoint = mPendingBP;
        (*ppPendingBreakpoint)->AddRef();
        return S_OK;
    }

    HRESULT ErrorBreakpoint::GetBreakpointResolution( IDebugErrorBreakpointResolution2** ppErrorResolution ) 
    {
        if ( ppErrorResolution == NULL )
            return E_INVALIDARG;

        *ppErrorResolution = mBPRes;
        (*ppErrorResolution)->AddRef();
        return S_OK;
    }


    //----------------------------------------------------------------------------

    void ErrorBreakpoint::Init( 
            IDebugPendingBreakpoint2* ppPendingBreakpoint, 
            IDebugErrorBreakpointResolution2* ppErrorResolution )
    {
        _ASSERT( ppPendingBreakpoint != NULL );
        _ASSERT( ppErrorResolution != NULL );

        mPendingBP = ppPendingBreakpoint;
        mBPRes = ppErrorResolution;
    } 
}
