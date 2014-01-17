/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "RemoteEventRpc.h"
#include "IRemoteEventCallback.h"
#include "MagoRemoteEvent_i.h"


struct EventContext
{
    RefPtr<Mago::IRemoteEventCallback>  Callback;
};


namespace Mago
{
    IRemoteEventCallback*   mCallback;


    void SetRemoteEventCallback( IRemoteEventCallback* callback )
    {
        IRemoteEventCallback*   oldCallback = NULL;

        if ( callback != NULL )
            callback->AddRef();

        oldCallback = (IRemoteEventCallback*) 
            InterlockedExchangePointer( (void**) &mCallback, callback );

        if ( oldCallback != NULL )
            oldCallback->Release();
    }

    // The interface that's returned still has a reference.
    IRemoteEventCallback* TakeEventCallback()
    {
        return (IRemoteEventCallback*) InterlockedExchangePointer( (void**) &mCallback, NULL );
    }
}


// The RPC runtime will call this function, if the connection to the client is lost.
void __RPC_USER HCTXEVENT_rundown( HCTXEVENT hContext )
{
    MagoRemoteEvent_Close( &hContext );
}

HRESULT MagoRemoteEvent_Open( 
    /* [in] */ handle_t hBinding,
    /* [in] */ const GUID *sessionUuid,
    /* [out] */ HCTXEVENT *phContext)
{
    if ( hBinding == NULL || sessionUuid == NULL || phContext == NULL )
        return E_INVALIDARG;

    UniquePtr<EventContext> context( new EventContext() );
    if ( context.Get() == NULL )
        return E_OUTOFMEMORY;

    context->Callback.Attach( Mago::TakeEventCallback() );

    if ( context->Callback.Get() == NULL )
        return E_FAIL;

    if ( context->Callback->GetSessionGuid() != *sessionUuid )
        return E_FAIL;

    *phContext = context.Detach();

    return S_OK;
}

void MagoRemoteEvent_Close( 
    /* [out][in] */ HCTXEVENT *phContext)
{
    if ( phContext == NULL || *phContext == NULL )
        return;

    EventContext*   context = (EventContext*) *phContext;
    delete context;

    *phContext = NULL;
}

void MagoRemoteEvent_OnProcessStart( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnProcessStart( pid );
}

void MagoRemoteEvent_OnProcessExit( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ DWORD exitCode)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnProcessExit( pid, exitCode );
}

void MagoRemoteEvent_OnThreadStart( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_ThreadInfo *thread)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnThreadStart( pid, thread );
}

void MagoRemoteEvent_OnThreadExit( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ DWORD threadId,
    /* [in] */ DWORD exitCode)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnThreadExit( pid, threadId, exitCode );
}

void MagoRemoteEvent_OnModuleLoad( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_ModuleInfo *modInfo)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnModuleLoad( pid, modInfo );
}

void MagoRemoteEvent_OnModuleUnload( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address baseAddress)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnModuleUnload( pid, baseAddress );
}

void MagoRemoteEvent_OnOutputString( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [string][in] */ const wchar_t *outputString)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnOutputString( pid, outputString );
}

void MagoRemoteEvent_OnLoadComplete( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ DWORD threadId)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnLoadComplete( pid, threadId );
}

MagoRemote_RunMode MagoRemoteEvent_OnException( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ DWORD threadId,
    /* [in] */ boolean firstChance,
    /* [in] */ unsigned int recordCount,
    /* [in][size_is] */ MagoRemote_ExceptionRecord *exceptRecords)
{
    if ( hContext == NULL )
        return MagoRemote_RunMode_Run;

    EventContext*   context = (EventContext*) hContext;

    MagoRemote_RunMode mode = context->Callback->OnException( 
        pid, 
        threadId, 
        firstChance ? true : false, 
        recordCount, 
        exceptRecords );

    return mode;
}

MagoRemote_RunMode MagoRemoteEvent_OnBreakpoint( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int threadId,
    /* [in] */ MagoRemote_Address address,
    /* [in] */ boolean embedded)
{
    if ( hContext == NULL )
        return MagoRemote_RunMode_Run;

    EventContext*   context = (EventContext*) hContext;

    MagoRemote_RunMode mode = context->Callback->OnBreakpoint( 
        pid, 
        threadId, 
        address, 
        embedded ? true : false );

    return mode;
}

void MagoRemoteEvent_OnStepComplete( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int threadId)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnStepComplete( pid, threadId );
}

void MagoRemoteEvent_OnAsyncBreak( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int threadId)
{
    if ( hContext == NULL )
        return;

    EventContext*   context = (EventContext*) hContext;

    context->Callback->OnAsyncBreakComplete( pid, threadId );
}

MagoRemote_ProbeRunMode MagoRemoteEvent_OnCallProbe( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int threadId,
    /* [in] */ MagoRemote_Address address,
    /* [out] */ MagoRemote_AddressRange *thunkRange)
{
    if ( hContext == NULL )
        return MagoRemote_PRunMode_Run;

    EventContext*   context = (EventContext*) hContext;

    MagoRemote_ProbeRunMode mode = context->Callback->OnCallProbe( 
        pid, 
        threadId, 
        address, 
        thunkRange );

    return mode;
}
