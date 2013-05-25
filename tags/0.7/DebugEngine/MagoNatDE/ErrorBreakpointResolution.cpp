/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ErrorBreakpointResolution.h"


namespace Mago
{
    // ErrorBreakpointResolution

    ErrorBreakpointResolution::ErrorBreakpointResolution()
        :   mErrType( 0 )
    {
    }

    ErrorBreakpointResolution::~ErrorBreakpointResolution()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugErrorBreakpointResolution2 

    HRESULT ErrorBreakpointResolution::GetBreakpointType( BP_TYPE* pBPType ) 
    {
        if ( pBPType == NULL )
            return E_INVALIDARG;

        *pBPType = mResLoc.bpType;
        return S_OK;
    }

    HRESULT ErrorBreakpointResolution::GetResolutionInfo(         
        BPERESI_FIELDS       dwFields,
        BP_ERROR_RESOLUTION_INFO* pErrorResolutionInfo )
    {
        if ( pErrorResolutionInfo == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        pErrorResolutionInfo->dwFields = 0;

        if ( (dwFields & BPERESI_BPRESLOCATION) != 0 )
        {
            hr = mResLoc.CopyTo( pErrorResolutionInfo->bpResLocation );
            if ( FAILED( hr ) )
                return hr;

            pErrorResolutionInfo->dwFields |= BPERESI_BPRESLOCATION;
        }

        if ( (dwFields & BPERESI_PROGRAM) != 0 )
        {
            if ( mAD7Prog != NULL )
            {
                pErrorResolutionInfo->pProgram = mAD7Prog;
                pErrorResolutionInfo->pProgram->AddRef();
                pErrorResolutionInfo->dwFields |= BPERESI_PROGRAM;
            }
        }

        if ( (dwFields & BPERESI_THREAD) != 0 )
        {
            if ( mAD7Thread != NULL )
            {
                pErrorResolutionInfo->pThread = mAD7Thread;
                pErrorResolutionInfo->pThread->AddRef();
                pErrorResolutionInfo->dwFields |= BPERESI_THREAD;
            }
        }

        if ( (dwFields & BPERESI_MESSAGE) != 0 )
        {
            pErrorResolutionInfo->bstrMessage = mMsg.Copy();
            pErrorResolutionInfo->dwFields |= BPERESI_MESSAGE;
        }

        if ( (dwFields & BPERESI_TYPE) != 0 )
        {
            pErrorResolutionInfo->dwType = mErrType;
            pErrorResolutionInfo->dwFields |= BPERESI_TYPE;
        }

        return S_OK;
    }


    //----------------------------------------------------------------------------

    HRESULT ErrorBreakpointResolution::Init( 
            BpResolutionLocation& bpresLoc,
            IDebugProgram2* pProgram,
            IDebugThread2* pThread, 
            const wchar_t* msg,
            BP_ERROR_TYPE errType )
    {
        HRESULT hr = S_OK;

        mMsg = msg;

        std::swap( mResLoc, bpresLoc );

        mAD7Prog = pProgram;
        mAD7Thread = pThread;
        mErrType = errType;

        return hr;
    }
}
