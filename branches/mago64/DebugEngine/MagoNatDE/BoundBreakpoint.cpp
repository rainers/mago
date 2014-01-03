/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "BoundBreakpoint.h"
#include "PendingBreakpoint.h"
#include "Program.h"


namespace Mago
{
    // BoundBreakpoint

    BoundBreakpoint::BoundBreakpoint()
    :   mId( 0 ),
        mState( BPS_NONE ),
        mAddr( 0 )
    {
    }

    BoundBreakpoint::~BoundBreakpoint()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugBoundBreakpoint2 

    HRESULT BoundBreakpoint::GetState( BP_STATE* pState )
    {
        if ( pState == NULL )
            return E_INVALIDARG;

        *pState = mState;
        return S_OK;
    }

    HRESULT BoundBreakpoint::GetHitCount( DWORD* pdwHitCount )
    { return E_NOTIMPL; }
    HRESULT BoundBreakpoint::SetHitCount( DWORD dwHitCount )
    { return E_NOTIMPL; }
    HRESULT BoundBreakpoint::SetCondition( BP_CONDITION bpCondition )
    { return E_NOTIMPL; }
    HRESULT BoundBreakpoint::SetPassCount( BP_PASSCOUNT bpPassCount )
    { return E_NOTIMPL; }

    HRESULT BoundBreakpoint::GetPendingBreakpoint( 
        IDebugPendingBreakpoint2** ppPendingBreakpoint )
    {
        if ( ppPendingBreakpoint == NULL )
            return E_INVALIDARG;

        return mPendingBP->QueryInterface( __uuidof( IDebugPendingBreakpoint2 ), (void**) ppPendingBreakpoint );
    }

    HRESULT BoundBreakpoint::GetBreakpointResolution( 
        IDebugBreakpointResolution2** ppBPResolution )
    {
        if ( ppBPResolution == NULL )
            return E_INVALIDARG;

        *ppBPResolution = mBPRes;
        (*ppBPResolution)->AddRef();
        return S_OK;
    }

    HRESULT BoundBreakpoint::Delete()
    {
        if ( mState != BPS_DELETED )
        {
            mPendingBP->OnBoundBPDelete( this );
        }

        // TODO: should we return E_BP_DELETED if already deleted?
        return S_OK;
    }

    void BoundBreakpoint::Dispose()
    {
        GuardedArea guard( mStateGuard );

        if ( mState != BPS_DELETED )
        {
            Enable( FALSE );

            mState = BPS_DELETED;
        }
    }

    HRESULT BoundBreakpoint::Enable( BOOL fEnable )
    {
        GuardedArea guard( mStateGuard );

        if ( mState == BPS_DELETED )
            return E_BP_DELETED;

        HRESULT             hr = S_OK;

        if ( fEnable && (mState != BPS_ENABLED) )
        {
            hr = mProg->SetInternalBreakpoint( mAddr, (BPCookie) this );
            if ( FAILED( hr ) )
                return hr;

            mState = BPS_ENABLED;
        }
        else if ( !fEnable && (mState != BPS_DISABLED) )
        {
            hr = mProg->RemoveInternalBreakpoint( mAddr, (BPCookie) this );
            if ( FAILED( hr ) )
                return hr;

            mState = BPS_DISABLED;
        }

        return hr;
    }


    //----------------------------------------------------------------------------

    void BoundBreakpoint::Init( 
            DWORD id,
            Address addr,
            PendingBreakpoint* pendingBreakpoint, 
            IDebugBreakpointResolution2* resolution,
            Program* prog )
    {
        _ASSERT( id != 0 );
        _ASSERT( pendingBreakpoint != NULL );
        _ASSERT( resolution != NULL );
        _ASSERT( prog != NULL );

        mId = id;
        mAddr = addr;
        mPendingBP = pendingBreakpoint;
        mBPRes = resolution;
        mProg = prog;
    } 

    DWORD BoundBreakpoint::GetId()
    {
        return mId;
    }
}
