/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "PendingBreakpoint.h"
#include "Engine.h"
#include "Program.h"
#include "BPDocumentContext.h"
#include "BoundBreakpoint.h"
#include "ErrorBreakpoint.h"
#include "ComEnumWithCount.h"
#include "Events.h"
#include "Module.h"
#include "BpResolutionLocation.h"
#include "BPBinderCallback.h"
#include "BPBinders.h"
#include <memory>

using namespace std;


typedef CComEnumWithCount< 
    IEnumDebugBoundBreakpoints2, 
    &IID_IEnumDebugBoundBreakpoints2, 
    IDebugBoundBreakpoint2*, 
    _CopyInterface<IDebugBoundBreakpoint2>,
    CComMultiThreadModel
> EnumDebugBoundBreakpoints;

typedef CComEnumWithCount< 
    IEnumDebugErrorBreakpoints2, 
    &IID_IEnumDebugErrorBreakpoints2, 
    IDebugErrorBreakpoint2*, 
    _CopyInterface<IDebugErrorBreakpoint2>,
    CComMultiThreadModel
> EnumDebugErrorBreakpoints;

//typedef CComEnumWithCount< 
//    IEnumDebugCodeContexts2, 
//    &IID_IEnumDebugCodeContexts2, 
//    IDebugCodeContext2*, 
//    _CopyInterface<IDebugCodeContext2>, 
//    CComMultiThreadModel
//> EnumDebugCodeContexts;
//
//


namespace Mago
{
    // PendingBreakpoint

    PendingBreakpoint::PendingBreakpoint()
        :   mId( 0 ),
            mDeleted( false ),
            mSentEvent( false ),
            mLastBPId( 0 )
    {
        mState.flags = PBPSF_NONE;
        mState.state = PBPS_NONE;
    }

    PendingBreakpoint::~PendingBreakpoint()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugPendingBreakpoint2 

    HRESULT PendingBreakpoint::GetState( PENDING_BP_STATE_INFO* pState )
    {
        if ( pState == NULL )
            return E_INVALIDARG;

        *pState = mState;
        if ( mDeleted )
            pState->state = PBPS_DELETED;
        return S_OK;
    }

    HRESULT PendingBreakpoint::GetBreakpointRequest( IDebugBreakpointRequest2** ppBPRequest )
    {
        if ( ppBPRequest == NULL )
            return E_INVALIDARG;
        if ( mDeleted )
            return E_BP_DELETED;

        _ASSERT( mBPRequest != NULL );
        *ppBPRequest = mBPRequest;
        (*ppBPRequest)->AddRef();
        return S_OK; 
    } 

    HRESULT PendingBreakpoint::Virtualize( BOOL fVirtualize )
    {
        if ( mDeleted )
            return E_BP_DELETED;

        if ( fVirtualize )
            mState.flags |= PBPSF_VIRTUALIZED;
        else
            mState.flags &= ~PBPSF_VIRTUALIZED;

        return S_OK;
    }

    HRESULT PendingBreakpoint::Enable( BOOL fEnable )
    {
        if ( mDeleted )
            return E_BP_DELETED;

        if ( fEnable )
            mState.state = PBPS_ENABLED;
        else
            mState.state = PBPS_DISABLED;

        return S_OK;
    }

    HRESULT PendingBreakpoint::SetCondition( BP_CONDITION bpCondition )
    {
        if ( mDeleted )
            return E_BP_DELETED;

        return E_NOTIMPL;
    }

    HRESULT PendingBreakpoint::SetPassCount( BP_PASSCOUNT bpPassCount )
    {
        if ( mDeleted )
            return E_BP_DELETED;

        return E_NOTIMPL;
    }

    HRESULT PendingBreakpoint::EnumBoundBreakpoints( IEnumDebugBoundBreakpoints2** ppEnum )
    {
        if ( ppEnum == NULL )
            return E_INVALIDARG;
        if ( mDeleted )
            return E_BP_DELETED;

        HRESULT     hr = S_OK;
        size_t      boundBPCount = 0;
        GuardedArea guard( mBoundBPGuard );

        for ( BindingMap::iterator it = mBindings.begin();
            it != mBindings.end();
            it++ )
        {
            ModuleBinding&  bind = it->second;
            boundBPCount += bind.BoundBPs.size();
        }

        InterfaceArray<IDebugBoundBreakpoint2>  array( boundBPCount );
        int i = 0;

        if ( array.Get() == NULL )
            return E_OUTOFMEMORY;

        for ( BindingMap::iterator it = mBindings.begin();
            it != mBindings.end();
            it++ )
        {
            ModuleBinding&  bind = it->second;

            for ( ModuleBinding::BPList::iterator itBind = bind.BoundBPs.begin();
                itBind != bind.BoundBPs.end();
                itBind++, i++ )
            {
                hr = (*itBind)->QueryInterface( __uuidof( IDebugBoundBreakpoint2 ), (void**) &array[i] );
                _ASSERT( hr == S_OK );
            }
        }

        return MakeEnumWithCount<EnumDebugBoundBreakpoints>( array, ppEnum );
    }

    HRESULT PendingBreakpoint::EnumBoundBreakpoints( ModuleBinding* binding, IEnumDebugBoundBreakpoints2** ppEnum )
    {
        _ASSERT( binding != NULL );
        if ( ppEnum == NULL )
            return E_INVALIDARG;
        if ( mDeleted )
            return E_BP_DELETED;

        HRESULT     hr = S_OK;
        size_t      boundBPCount = binding->BoundBPs.size();
        GuardedArea guard( mBoundBPGuard );

        InterfaceArray<IDebugBoundBreakpoint2>  array( boundBPCount );
        int     i = 0;

        if ( array.Get() == NULL )
            return E_OUTOFMEMORY;

        ModuleBinding&  bind = *binding;

        for ( ModuleBinding::BPList::iterator itBind = bind.BoundBPs.begin();
            itBind != bind.BoundBPs.end();
            itBind++, i++ )
        {
            hr = (*itBind)->QueryInterface( __uuidof( IDebugBoundBreakpoint2 ), (void**) &array[i] );
            _ASSERT( hr == S_OK );
        }

        return MakeEnumWithCount<EnumDebugBoundBreakpoints>( array, ppEnum );
    }

    HRESULT PendingBreakpoint::EnumErrorBreakpoints( BP_ERROR_TYPE bpErrorType, 
                                                     IEnumDebugErrorBreakpoints2** ppEnum)
    {
        if ( ppEnum == NULL )
            return E_INVALIDARG;
        if ( mDeleted )
            return E_BP_DELETED;

        HRESULT     hr = S_OK;
        size_t      errorBPCount = 0;
        GuardedArea guard( mBoundBPGuard );

        for ( BindingMap::iterator it = mBindings.begin();
            it != mBindings.end();
            it++ )
        {
            ModuleBinding&  bind = it->second;
            if ( bind.ErrorBP.Get() != NULL )
                errorBPCount++;
        }

        InterfaceArray<IDebugErrorBreakpoint2>  array( errorBPCount );
        int i = 0;

        if ( array.Get() == NULL )
            return E_OUTOFMEMORY;

        for ( BindingMap::iterator it = mBindings.begin();
            it != mBindings.end();
            it++ )
        {
            ModuleBinding&  bind = it->second;

            if ( bind.ErrorBP.Get() != NULL )
            {
                hr = bind.ErrorBP->QueryInterface( __uuidof( IDebugErrorBreakpoint2 ), (void**) &array[i] );
                _ASSERT( hr == S_OK );
                i++;
            }
        }

        return MakeEnumWithCount<EnumDebugErrorBreakpoints>( array, ppEnum );
    }

    HRESULT PendingBreakpoint::EnumCodeContexts( IEnumDebugCodeContexts2** ppEnum )
    {
        if ( ppEnum == NULL )
            return E_INVALIDARG;
        if ( mDeleted )
            return E_BP_DELETED;

        _ASSERT( false );

        return E_NOTIMPL;
    }

    HRESULT PendingBreakpoint::Delete()
    {
        if ( !mDeleted )
        {
            mEngine->OnPendingBPDelete( this );
        }

        // TODO: should we return E_BP_DELETED if already deleted?
        return S_OK;
    }

    void PendingBreakpoint::Dispose()
    {
        if ( !mDeleted )
        {
            mDeleted = true;

            mEngine.Release();
            mBPRequest.Release();
            mDocContext.Release();

            GuardedArea guard( mBoundBPGuard );

            for ( BindingMap::iterator it = mBindings.begin();
                it != mBindings.end();
                it++ )
            {
                ModuleBinding&  bind = it->second;

                for ( ModuleBinding::BPList::iterator itBind = bind.BoundBPs.begin();
                    itBind != bind.BoundBPs.end();
                    itBind++ )
                {
                    (*itBind)->Dispose();
                }
            }

            mBindings.clear();
        }
    }

    HRESULT PendingBreakpoint::CanBind( IEnumDebugErrorBreakpoints2** ppErrorEnum )
    {
        if ( ppErrorEnum == NULL )
            return E_INVALIDARG;
        if ( mDeleted )
            return E_BP_DELETED;

        return E_NOTIMPL;
    }

    HRESULT PendingBreakpoint::Bind()
    {
        _RPT0( _CRT_WARN, "PendingBreakpoint::Bind Enter\n" );

        mEngine->BeginBindBP();

        // Call BindToAllModules on the poll thread for speed.
        // Long term, we should try to remove the poll thread requirement from 
        // the BP set and remove operations for clarity (with speed built-in).

        HRESULT hr = BindToAllModules();

        mEngine->EndBindBP();

        _RPT0( _CRT_WARN, "PendingBreakpoint::Bind Leave\n" );

        return hr;
    }

    HRESULT MakeBinder( IDebugBreakpointRequest2* bpRequest, unique_ptr<BPBinder>& binder )
    {
        BP_LOCATION_TYPE    locType = 0;

        bpRequest->GetLocationType( &locType );

        if ( locType == BPLT_CODE_FILE_LINE )
        {
            binder.reset( new BPCodeFileLineBinder( bpRequest ) );
        }
        else if ( locType == BPLT_CODE_ADDRESS )
        {
            binder.reset( new BPCodeAddressBinder( bpRequest ) );
        }
        else if ( locType == BPLT_CODE_CONTEXT )
        {
            binder.reset( new BPCodeAddressBinder( bpRequest ) );
        }
        else
            return E_FAIL;

        if ( binder.get() == NULL )
            return E_OUTOFMEMORY;

        return S_OK;
    }

    // The job of Bind:
    // - Generate bound or error breakpoints
    // - Establish the document context
    // - Enable the bound breakpoints
    // - Send the bound or error events

    HRESULT PendingBreakpoint::BindToAllModules()
    {
        GuardedArea             guard( mBoundBPGuard );

        if ( mDeleted )
            return E_BP_DELETED;

        HRESULT                 hr = S_OK;
        BpRequestInfo           reqInfo;
		unique_ptr<BPBinder>    binder;

        hr = MakeBinder( mBPRequest, binder );
        if ( FAILED( hr ) )
            return hr;

        // generate bound and error breakpoints
        BPBinderCallback        callback( binder.get(), this, mDocContext.Get() );
        mEngine->ForeachProgram( &callback );

        if ( mDocContext.Get() == NULL )
        {
            // set up our document context, since we didn't have one
            callback.GetDocumentContext( mDocContext );
        }

        // enable all bound BPs if we're enabled
        if ( mState.state == PBPS_ENABLED )
        {
            for ( BindingMap::iterator it = mBindings.begin();
                it != mBindings.end();
                it++ )
            {
                ModuleBinding&  bind = it->second;

                for ( ModuleBinding::BPList::iterator itBind = bind.BoundBPs.begin();
                    itBind != bind.BoundBPs.end();
                    itBind++ )
                {
                    (*itBind)->Enable( TRUE );
                }
            }
        }

        if ( callback.GetBoundBPCount() > 0 )
        {
            // send a bound event

            CComPtr<IEnumDebugBoundBreakpoints2>    enumBPs;

            hr = EnumBoundBreakpoints( &enumBPs );
            if ( FAILED( hr ) )
                return hr;

            hr = SendBoundEvent( enumBPs );
            mSentEvent = true;
        }
        else if ( callback.GetErrorBPCount() > 0 )
        {
            // send an error event

            RefPtr<ErrorBreakpoint> errorBP;

            callback.GetLastErrorBP( errorBP );

            hr = SendErrorEvent( errorBP.Get() );
            mSentEvent = true;
        }
        else
        {
            // allow adding this pending BP, even if there are no loaded modules (including program)
            hr = S_OK;
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = mEngine->AddPendingBP( this );
        }

        return hr;
    }

    HRESULT PendingBreakpoint::BindToModule( Module* mod, Program* prog )
    {
        GuardedArea             guard( mBoundBPGuard );

        if ( mDeleted )
            return E_BP_DELETED;
        if ( (mState.flags & PBPSF_VIRTUALIZED) == 0 )
            return E_FAIL;

        HRESULT                 hr = S_OK;
        BpRequestInfo           reqInfo;
		unique_ptr<BPBinder>    binder;

        hr = MakeBinder( mBPRequest, binder );
        if ( FAILED( hr ) )
            return hr;

        // generate bound and error breakpoints
        BPBinderCallback        callback( binder.get(), this, mDocContext.Get() );
        callback.BindToModule( mod, prog );

        if ( mDocContext.Get() == NULL )
        {
            // set up our document context, since we didn't have one
            callback.GetDocumentContext( mDocContext );
        }

        ModuleBinding*  binding = GetBinding( mod->GetId() );
        if ( binding == NULL )
            return E_FAIL;

        // enable all bound BPs if we're enabled
        if ( mState.state == PBPS_ENABLED )
        {
            ModuleBinding&  bind = *binding;

            for ( ModuleBinding::BPList::iterator itBind = bind.BoundBPs.begin();
                itBind != bind.BoundBPs.end();
                itBind++ )
            {
                (*itBind)->Enable( TRUE );
            }
        }

        if ( callback.GetBoundBPCount() > 0 )
        {
            // send a bound event

            CComPtr<IEnumDebugBoundBreakpoints2>    enumBPs;

            hr = EnumBoundBreakpoints( binding, &enumBPs );
            if ( FAILED( hr ) )
                return hr;

            hr = SendBoundEvent( enumBPs );
            mSentEvent = true;
        }
        else if ( callback.GetErrorBPCount() > 0 )
        {
            if ( mSentEvent )
            {
                // At the beginning, Bind was called, which bound to all mods at the
                // time. If it sent out a bound BP event, then there can be no error.
                // If it sent out an error BP event, then there's no need to repeat it.
                // If you do send out this unneeded event here, then it slows down mod
                // loading a lot. For ex., with 160 mods, mod loading takes ~10x longer.

                // So, don't send an error event!
                hr = S_OK;
            }
            else
            {
                RefPtr<ErrorBreakpoint> errorBP;

                callback.GetLastErrorBP( errorBP );

                hr = SendErrorEvent( errorBP.Get() );
                mSentEvent = true;
            }
        }
        else
            hr = E_FAIL;

        return hr;
    }

    HRESULT PendingBreakpoint::UnbindFromModule( Module* mod, Program* prog )
    {
        GuardedArea     guard( mBoundBPGuard );

        if ( mDeleted )
            return E_BP_DELETED;

        HRESULT         hr = S_OK;
        ModuleBinding*  binding = NULL;
        DWORD           boundBPCount = 0;
        RefPtr<ErrorBreakpoint> lastErrorBP;

        binding = GetBinding( mod->GetId() );
        if ( binding == NULL )
            return E_FAIL;

        for ( ModuleBinding::BPList::iterator it = binding->BoundBPs.begin();
            it != binding->BoundBPs.end();
            it++ )
        {
            BoundBreakpoint*    bp = it->Get();

            bp->Dispose();
            SendUnboundEvent( bp, prog );
        }

        mBindings.erase( mod->GetId() );

        for ( BindingMap::iterator it = mBindings.begin();
            it != mBindings.end();
            it++ )
        {
            ModuleBinding&  bind = it->second;
            boundBPCount += bind.BoundBPs.size();
            if ( bind.ErrorBP.Get() != NULL )
                lastErrorBP = bind.ErrorBP;
        }

        // if there're no more bound BPs, then send an error BP event
        // there should always be at least one error BP, because there should 
        // always be at least one module - the EXE

        if ( (boundBPCount == 0) && (lastErrorBP.Get() != NULL) )
        {
            SendErrorEvent( lastErrorBP.Get() );
        }

        return hr;
    }


    //----------------------------------------------------------------------------

    void PendingBreakpoint::Init( 
        DWORD id,
        Engine* engine,
        IDebugBreakpointRequest2* pBPRequest,
        IDebugEventCallback2* pCallback )
    {
        _ASSERT( id != 0 );
        _ASSERT( engine != NULL );
        _ASSERT( pBPRequest != NULL );
        _ASSERT( pCallback != NULL );

        mId = id;
        mEngine = engine;
        mBPRequest = pBPRequest;
        mCallback = pCallback;
    }

    DWORD PendingBreakpoint::GetId()
    {
        return mId;
    }

    // this should be the least frequent operation: individual bound BP deletes
    void PendingBreakpoint::OnBoundBPDelete( BoundBreakpoint* boundBP )
    {
        _ASSERT( boundBP != NULL );

        const DWORD Id = boundBP->GetId();
        GuardedArea guard( mBoundBPGuard );

        for ( BindingMap::iterator it = mBindings.begin();
            it != mBindings.end();
            it++ )
        {
            ModuleBinding&  bind = it->second;

            for ( ModuleBinding::BPList::iterator itBind = bind.BoundBPs.begin();
                itBind != bind.BoundBPs.end();
                itBind++ )
            {
                if ( (*itBind)->GetId() == Id )
                {
                    bind.BoundBPs.erase( itBind );
                    goto Found;
                }
            }
        }

    Found:
        boundBP->Dispose();
    }

    ModuleBinding*  PendingBreakpoint::GetBinding( DWORD modId )
    {
        BindingMap::iterator it = mBindings.find( modId );

        if ( it == mBindings.end() )
            return NULL;

        return &it->second;
    }

    ModuleBinding*  PendingBreakpoint::AddOrFindBinding( DWORD modId )
    {
        auto pib = mBindings.insert( BindingMap::value_type( modId, ModuleBinding() ) );

        // it doesn't matter if it already exists
        return &pib.first->second;
    }

    DWORD PendingBreakpoint::GetNextBPId()
    {
        mLastBPId++;
        return mLastBPId;
    }

    HRESULT PendingBreakpoint::SendBoundEvent( IEnumDebugBoundBreakpoints2* enumBPs )
    {
        HRESULT hr = S_OK;
        CComPtr<IDebugPendingBreakpoint2>       pendBP;
        CComPtr<IDebugEngine2>                  engine;
        RefPtr<BreakpointBoundEvent>            event;

        hr = QueryInterface( __uuidof( IDebugPendingBreakpoint2 ), (void**) &pendBP );
        _ASSERT( hr == S_OK );

        hr = mEngine->QueryInterface( __uuidof( IDebugEngine2 ), (void**) &engine );
        _ASSERT( hr == S_OK );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return hr;

        event->Init( enumBPs, pendBP );

        return event->Send( mCallback, engine, NULL, NULL );
    }

    HRESULT PendingBreakpoint::SendErrorEvent( ErrorBreakpoint* errorBP )
    {
        HRESULT hr = S_OK;
        CComPtr<IDebugEngine2>                  engine;
        CComPtr<IDebugErrorBreakpoint2>         ad7ErrorBP;
        RefPtr<BreakpointErrorEvent>            event;

        hr = errorBP->QueryInterface( __uuidof( IDebugErrorBreakpoint2 ), (void**) &ad7ErrorBP );
        _ASSERT( hr == S_OK );

        hr = mEngine->QueryInterface( __uuidof( IDebugEngine2 ), (void**) &engine );
        _ASSERT( hr == S_OK );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return hr;

        event->Init( ad7ErrorBP );

        return event->Send( mCallback, engine, NULL, NULL );
    }

    HRESULT PendingBreakpoint::SendUnboundEvent( BoundBreakpoint* boundBP, Program* prog )
    {
        HRESULT hr = S_OK;
        RefPtr<BreakpointUnboundEvent>  event;
        CComPtr<IDebugBoundBreakpoint2> ad7BP;
        CComPtr<IDebugProgram2>         ad7Prog;
        CComPtr<IDebugEngine2>          engine;

        if ( prog != NULL )
        {
            hr = prog->QueryInterface( __uuidof( IDebugProgram2 ), (void**) &ad7Prog );
            _ASSERT( hr == S_OK );
        }

        hr = mEngine->QueryInterface( __uuidof( IDebugEngine2 ), (void**) &engine );
        _ASSERT( hr == S_OK );

        hr = boundBP->QueryInterface( __uuidof( IDebugBoundBreakpoint2 ), (void**) &ad7BP );
        _ASSERT( hr == S_OK );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return hr;

        event->Init( ad7BP, BPUR_CODE_UNLOADED );

        return event->Send( mCallback, engine, ad7Prog, NULL );
    }
}
