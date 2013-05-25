/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "BreakpointResolution.h"


namespace Mago
{
    // BreakpointResolution

    BreakpointResolution::BreakpointResolution()
    {
    }

    BreakpointResolution::~BreakpointResolution()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugBreakpointResolution2 

    HRESULT BreakpointResolution::GetBreakpointType( BP_TYPE* pBPType ) 
    {
        if ( pBPType == NULL )
            return E_INVALIDARG;

        *pBPType = mResLoc.bpType;
        return S_OK;
    }

    HRESULT BreakpointResolution::GetResolutionInfo(         
        BPRESI_FIELDS       dwFields,
        BP_RESOLUTION_INFO* pBPResolutionInfo )
    {
        if ( pBPResolutionInfo == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        pBPResolutionInfo->dwFields = 0;

        if ( (dwFields & BPRESI_BPRESLOCATION) != 0 )
        {
            hr = mResLoc.CopyTo( pBPResolutionInfo->bpResLocation );
            if ( FAILED( hr ) )
                return hr;

            pBPResolutionInfo->dwFields |= BPRESI_BPRESLOCATION;
        }

        if ( (dwFields & BPRESI_PROGRAM) != 0 )
        {
            if ( mAD7Prog != NULL )
            {
                pBPResolutionInfo->pProgram = mAD7Prog;
                pBPResolutionInfo->pProgram->AddRef();
                pBPResolutionInfo->dwFields |= BPRESI_PROGRAM;
            }
        }

        if ( (dwFields & BPRESI_THREAD) != 0 )
        {
            if ( mAD7Thread != NULL )
            {
                pBPResolutionInfo->pThread = mAD7Thread;
                pBPResolutionInfo->pThread->AddRef();
                pBPResolutionInfo->dwFields |= BPRESI_THREAD;
            }
        }

        return S_OK;
    }


    //----------------------------------------------------------------------------

    HRESULT BreakpointResolution::Init( 
            BpResolutionLocation& bpresLoc,
            IDebugProgram2* pProgram,
            IDebugThread2* pThread )
    {
        HRESULT hr = S_OK;

        std::swap( mResLoc, bpresLoc );

        mAD7Prog = pProgram;
        mAD7Thread = pThread;

        return hr;
    }
}
