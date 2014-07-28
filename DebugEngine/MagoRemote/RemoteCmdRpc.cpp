/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "RemoteCmdRpc.h"
#include "App.h"
#include "EventCallback.h"
#include "MagoRemoteCmd_h.h"
#include "MagoRemoteEvent_h.h"
#include "RpcUtil.h"
#include <..\Exec\DebuggerProxy.h>
#include <MagoDECommon.h>
#include <list>
#include <map>


struct SessionContext
{
    typedef std::map<UINT32, IProcess*> CmdProcessMap;

    MagoCore::DebuggerProxy ExecThread;
    HCTXEVENT               HEventContext;
    CmdProcessMap           ProcMap;
    Guard                   ProcGuard;
    GUID                    Uuid;
    long                    RefCount;

    ~SessionContext()
    {
        for ( CmdProcessMap::iterator it = ProcMap.begin();
            it != ProcMap.end();
            it++ )
        {
            it->second->Release();
        }
    }

    HRESULT AddProcess( IProcess* process )
    {
        _ASSERT( process != NULL );
        typedef CmdProcessMap M;

        GuardedArea guard( ProcGuard );

        M::_Pairib pair = ProcMap.insert( CmdProcessMap::value_type( process->GetId(), process ) );
        _ASSERT( pair.second );
        if ( pair.second )
            process->AddRef();

        return S_OK;
    }

    void RemoveProcess( UINT32 pid )
    {
        GuardedArea guard( ProcGuard );

        CmdProcessMap::iterator it = ProcMap.find( pid );
        if ( it == ProcMap.end() )
            return;

        it->second->Release();
        ProcMap.erase( it );
    }

    bool FindProcess( UINT32 pid, IProcess*& process )
    {
        GuardedArea guard( ProcGuard );

        CmdProcessMap::iterator it = ProcMap.find( pid );
        if ( it == ProcMap.end() )
            return false;

        process = it->second;
        process->AddRef();

        return true;
    }
};

struct CmdContext
{
    SessionContext* Session;
};

typedef std::list<SessionContext*> SessionList;


SessionList gSessionList;
Guard       gSessionGuard;


HRESULT InitRpcServerLocal( const wchar_t* sessionUuidStr )
{
    HRESULT         hr = S_OK;
    RPC_STATUS      rpcRet = RPC_S_OK;
    bool            registered = false;
    std::wstring    endpoint( AGENT_CMD_IF_LOCAL_ENDPOINT_PREFIX );

    endpoint.append( sessionUuidStr );

    rpcRet = RpcServerUseProtseqEp(
        AGENT_LOCAL_PROTOCOL_SEQUENCE,
        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
        (RPC_WSTR) endpoint.c_str(),
        NULL );
    if ( rpcRet != RPC_S_OK )
    {
        hr = HRESULT_FROM_WIN32( rpcRet );
        goto Error;
    }

    rpcRet = RpcServerRegisterIf2(
        MagoRemoteCmd_v1_0_s_ifspec,
        NULL,
        NULL,
        RPC_IF_ALLOW_LOCAL_ONLY,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
        (unsigned int) -1,
        NULL );
    if ( rpcRet != RPC_S_OK )
    {
        hr = HRESULT_FROM_WIN32( rpcRet );
        goto Error;
    }
    registered = true;

    rpcRet = RpcServerListen( 1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE );
    if ( rpcRet != RPC_S_OK )
    {
        hr = HRESULT_FROM_WIN32( rpcRet );
        goto Error;
    }

Error:
    if ( FAILED( hr ) )
    {
        if ( registered )
            RpcServerUnregisterIf( MagoRemoteCmd_v1_0_s_ifspec, NULL, FALSE );
    }
    return hr;
}

void UninitRpcServer()
{
    RpcServerUnregisterIf( NULL, NULL, FALSE );
    RpcMgmtStopServerListening( NULL );
    RpcMgmtWaitServerListen();
}

HRESULT GetEventBinding( const GUID* sessionUuid, RPC_BINDING_HANDLE& hBinding )
{
    RPC_STATUS          rpcRet = RPC_S_OK;
    RPC_WSTR            strBinding = NULL;
    wchar_t             sessionUuidStr[GUID_LENGTH + 1] = L"";
    std::wstring        endpoint( AGENT_EVENT_IF_LOCAL_ENDPOINT_PREFIX );

    hBinding = NULL;

    StringFromGUID2( *sessionUuid, sessionUuidStr, _countof( sessionUuidStr ) );
    endpoint.append( sessionUuidStr );

    rpcRet = RpcStringBindingCompose(
        NULL,
        AGENT_LOCAL_PROTOCOL_SEQUENCE,
        NULL,
        (RPC_WSTR) endpoint.c_str(),
        NULL,
        &strBinding );
    if ( rpcRet != RPC_S_OK )
        return HRESULT_FROM_WIN32( rpcRet );

    rpcRet = RpcBindingFromStringBinding( strBinding, &hBinding );
    RpcStringFree( &strBinding );
    if ( rpcRet != RPC_S_OK )
        return HRESULT_FROM_WIN32( rpcRet );

    // MSDN recommends letting the RPC runtime resolve the binding, so skip RpcEpResolveBinding

    return S_OK;
}

HRESULT OpenEventInterface( handle_t hEventBinding, const GUID* sessionUuid, HCTXEVENT* phEventContext )
{
    HRESULT hr = S_OK;

    __try
    {
        hr = MagoRemoteEvent_Open( hEventBinding, sessionUuid, phEventContext );
    }
    __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
    {
        _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_Open: %08X\n", 
            RpcExceptionCode() );
        hr = HRESULT_FROM_WIN32( RpcExceptionCode() );
    }

    return hr;
}

void CloseEventInterface( HCTXEVENT* phEventContext )
{
    __try
    {
        MagoRemoteEvent_Close( phEventContext );
    }
    __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
    {
        _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_Close: %08X\n", 
            RpcExceptionCode() );

        RpcSsDestroyClientContext( phEventContext );
    }
}

// The RPC runtime will call this function, if the connection to the client is lost.
void __RPC_USER HCTXCMD_rundown( HCTXCMD hContext )
{
    MagoRemoteCmd_Close( &hContext );
}

HRESULT MakeSessionContext( const GUID* uuid, HCTXEVENT hEventCtx, SessionContext*& context )
{
    _ASSERT( uuid != NULL );
    _ASSERT( hEventCtx != NULL );

    HRESULT                         hr = S_OK;
    RefPtr<Mago::EventCallback>     callback;
    UniquePtr<SessionContext>       sessionContext;

    callback = new Mago::EventCallback( hEventCtx );
    if ( callback.Get() == NULL )
        return E_OUTOFMEMORY;

    sessionContext.Attach( new SessionContext() );
    if ( sessionContext.IsEmpty() )
        return E_OUTOFMEMORY;

    sessionContext->HEventContext = hEventCtx;
    sessionContext->RefCount = 0;
    sessionContext->Uuid = *uuid;

    hr = sessionContext->ExecThread.Init( callback.Get() );
    if ( FAILED( hr ) )
        return hr;

    hr = sessionContext->ExecThread.Start();
    if ( FAILED( hr ) )
        return hr;

    context = sessionContext.Detach();

    return S_OK;
}

HRESULT OpenSession(
    const GUID* sessionUuid,
    UniquePtr<CmdContext>& context )
{
    HRESULT                         hr = S_OK;
    RPC_BINDING_HANDLE              hEventBinding = NULL;
    HCTXEVENT                       hEventCtx = NULL;
    UniquePtr<SessionContext>       sessionContext;
    UniquePtr<CmdContext>           cmdContext;

    cmdContext.Attach( new CmdContext() );
    if ( cmdContext.IsEmpty() )
        return E_OUTOFMEMORY;

    hr = GetEventBinding( sessionUuid, hEventBinding );
    if ( FAILED( hr ) )
        return hr;

    hr = OpenEventInterface( hEventBinding, sessionUuid, &hEventCtx );
    // Now that we've connected and gotten a context handle, 
    // we don't need the binding handle anymore.
    RpcBindingFree( &hEventBinding );
    if ( FAILED( hr ) )
        return hr;

    hr = MakeSessionContext( sessionUuid, hEventCtx, sessionContext.Ref() );
    if ( FAILED( hr ) )
    {
        CloseEventInterface( &hEventCtx );
        return hr;
    }

    {
        GuardedArea guard( gSessionGuard );

        sessionContext->RefCount = 1;
        gSessionList.push_back( sessionContext.Get() );
    }

    cmdContext->Session = sessionContext.Detach();
    context.Attach( cmdContext.Detach() );

    hEventCtx = NULL;

    return S_OK;
}

HRESULT ConnectSession( 
    const GUID* sessionUuid,
    UniquePtr<CmdContext>& context )
{
    GuardedArea guard( gSessionGuard );

    SessionList::iterator it;

    for ( it = gSessionList.begin();
        it != gSessionList.end();
        it++ )
    {
        if ( (*it)->Uuid == *sessionUuid )
            break;
    }

    if ( it == gSessionList.end() )
        return E_NOT_FOUND;

    UniquePtr<CmdContext>   cmdContext( new CmdContext() );
    if ( cmdContext.IsEmpty() )
        return E_OUTOFMEMORY;

    (*it)->RefCount++;
    cmdContext->Session = *it;

    context.Attach( cmdContext.Detach() );

    return S_OK;
}

HRESULT MagoRemoteCmd_Open( 
    /* [in] */ handle_t hBinding,
    /* [in] */ const GUID *sessionUuid,
    /* [in] */ boolean newSession,
    /* [out] */ HCTXCMD *phContext)
{
    if ( hBinding == NULL || sessionUuid == NULL || phContext == NULL )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    UniquePtr<CmdContext> context;

    *phContext = NULL;

    if ( newSession )
    {
        if ( !Mago::NewSession() )
            return E_WRONG_STATE;

        Mago::NotifyAddSession();

        hr = OpenSession( sessionUuid, context );
        if ( FAILED( hr ) )
        {
            Mago::NotifyRemoveSession();
            return hr;
        }
    }
    else
    {
        hr = ConnectSession( sessionUuid, context );
        if ( FAILED( hr ) )
            return hr;
    }

    *phContext = context.Detach();

    return S_OK;
}

void DisconnectSession( CmdContext*& cmdContext )
{
    _ASSERT( cmdContext != NULL );
    _ASSERT( cmdContext->Session != NULL );

    GuardedArea guard( gSessionGuard );

    SessionContext* session = cmdContext->Session;

    session->RefCount--;

    if ( session->RefCount == 0 )
    {
        session->ExecThread.Shutdown();
        CloseEventInterface( &session->HEventContext );

        delete session;

        gSessionList.remove( session );

        Mago::NotifyRemoveSession();
    }

    delete cmdContext;
    cmdContext = NULL;
}

void MagoRemoteCmd_Close( 
    /* [out][in] */ HCTXCMD *phContext)
{
    if ( phContext == NULL || *phContext == NULL )
        return;

    CmdContext* context = (CmdContext*) *phContext;

    DisconnectSession( context );

    *phContext = NULL;
}

UINT64 GetProcessorFeatures()
{
    UINT64 procFeatures = 0;

#if defined( _M_IX86 ) || defined( _M_X64 )
    if ( IsProcessorFeaturePresent( PF_MMX_INSTRUCTIONS_AVAILABLE ) )
        procFeatures |= Mago::PF_X86_MMX;

    if ( IsProcessorFeaturePresent( PF_3DNOW_INSTRUCTIONS_AVAILABLE ) )
        procFeatures |= Mago::PF_X86_3DNow;

    if ( IsProcessorFeaturePresent( PF_XMMI_INSTRUCTIONS_AVAILABLE ) )
        procFeatures |= Mago::PF_X86_SSE;

    if ( IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE ) )
        procFeatures |= Mago::PF_X86_SSE2;

    if ( IsProcessorFeaturePresent( PF_SSE3_INSTRUCTIONS_AVAILABLE ) )
        procFeatures |= Mago::PF_X86_SSE3;

    if ( IsProcessorFeaturePresent( PF_XSAVE_ENABLED ) )
        procFeatures |= Mago::PF_X86_AVX;
#else
#error Mago doesn't implement a remote agent for the current architecture.
#endif

    return procFeatures;
}

HRESULT ToWireProcess( IProcess* process, MagoRemote_ProcInfo* procInfo )
{
    wchar_t*    path = NULL;

    path = MidlAllocString( process->GetExePath() );
    if ( path == NULL )
        return E_OUTOFMEMORY;

    procInfo->ExePath = path;
    procInfo->MachineType = process->GetMachineType();
    procInfo->Pid = process->GetId();
    procInfo->MachineFeatures = GetProcessorFeatures();

    return S_OK;
}

HRESULT MagoRemoteCmd_Launch( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ MagoRemote_LaunchInfo *launchInfo,
    /* [out] */ MagoRemote_ProcInfo *procInfo)
{
    if ( hContext == NULL || launchInfo == NULL || procInfo == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    LaunchInfo          execLaunchInfo = { 0 };
    RefPtr<IProcess>    process;

    execLaunchInfo.Exe = launchInfo->Exe;
    execLaunchInfo.CommandLine = launchInfo->CommandLine;
    execLaunchInfo.Dir = launchInfo->Dir;
    execLaunchInfo.EnvBstr = launchInfo->EnvBstr;

    if ( (launchInfo->Flags & MagoRemote_PFlags_NewConsole) != 0 )
        execLaunchInfo.NewConsole = true;
    if ( (launchInfo->Flags & MagoRemote_PFlags_Suspend) != 0 )
        execLaunchInfo.Suspend = true;

    hr = context->Session->ExecThread.Launch( &execLaunchInfo, process.Ref() );
    if ( FAILED( hr ) )
        goto Error;

    hr = ToWireProcess( process.Get(), procInfo );
    if ( FAILED( hr ) )
        goto Error;

    hr = context->Session->AddProcess( process.Get() );
    if ( FAILED( hr ) )
        goto Error;

Error:
    if ( FAILED( hr ) )
    {
        if ( procInfo->ExePath != NULL )
            MIDL_user_free( procInfo->ExePath );
        if ( process.Get() != NULL )
            context->Session->ExecThread.Terminate( process.Get() );
    }
    return hr;
}

HRESULT MagoRemoteCmd_Attach( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [out] */ MagoRemote_ProcInfo *procInfo)
{
    if ( hContext == NULL || procInfo == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    hr = context->Session->ExecThread.Attach( pid, process.Ref() );
    if ( FAILED( hr ) )
        goto Error;

    hr = ToWireProcess( process.Get(), procInfo );
    if ( FAILED( hr ) )
        goto Error;

    hr = context->Session->AddProcess( process.Get() );
    if ( FAILED( hr ) )
        goto Error;

Error:
    if ( FAILED( hr ) )
    {
        if ( procInfo->ExePath != NULL )
            MIDL_user_free( procInfo->ExePath );
        if ( process.Get() != NULL )
            context->Session->ExecThread.Detach( process.Get() );
    }
    return hr;
}

HRESULT MagoRemoteCmd_Terminate( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.Terminate( process.Get() );

    return hr;
}

HRESULT MagoRemoteCmd_Detach( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.Detach( process.Get() );

    return hr;
}

HRESULT MagoRemoteCmd_ResumeProcess( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.ResumeLaunchedProcess( process.Get() );

    return hr;
}

HRESULT MagoRemoteCmd_ReadMemory( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address,
    /* [in] */ unsigned int length,
    /* [out] */ unsigned int *lengthRead,
    /* [out] */ unsigned int *lengthUnreadable,
    /* [out][size_is] */ byte *buffer)
{
    if ( hContext == NULL || lengthRead == NULL || lengthUnreadable == NULL || buffer == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.ReadMemory( 
        process.Get(),
        (Address) address,
        length,
        *lengthRead,
        *lengthUnreadable,
        buffer );

    return hr;
}

HRESULT MagoRemoteCmd_WriteMemory( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address,
    /* [in] */ unsigned int length,
    /* [out] */ unsigned int *lengthWritten,
    /* [in][size_is] */ byte *buffer)
{
    if ( hContext == NULL || lengthWritten == NULL || buffer == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.WriteMemory( 
        process.Get(),
        (Address) address,
        length,
        *lengthWritten,
        buffer );

    return hr;
}

HRESULT MagoRemoteCmd_SetBreakpoint( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.SetBreakpoint( process.Get(), (Address) address );

    return hr;
}

HRESULT MagoRemoteCmd_RemoveBreakpoint( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.RemoveBreakpoint( process.Get(), (Address) address );

    return hr;
}

HRESULT MagoRemoteCmd_StepOut( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address targetAddr,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.StepOut( 
        process.Get(), 
        (Address) targetAddr, 
        handleException ? true : false );

    return hr;
}

HRESULT MagoRemoteCmd_StepInstruction( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean stepIn,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.StepInstruction( 
        process.Get(), 
        stepIn ? true : false, 
        handleException ? true : false );

    return hr;
}

HRESULT MagoRemoteCmd_StepRange( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean stepIn,
    /* [in] */ MagoRemote_AddressRange range,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;
    AddressRange        execRange = { (Address) range.Begin, (Address) range.End };

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.StepRange( 
        process.Get(), 
        stepIn ? true : false, 
        execRange, 
        handleException ? true : false );

    return hr;
}

HRESULT MagoRemoteCmd_Continue( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.Continue( process.Get(), handleException ? true : false );

    return hr;
}

HRESULT MagoRemoteCmd_Execute( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.Execute( process.Get(), handleException ? true : false );

    return hr;
}

HRESULT MagoRemoteCmd_AsyncBreak( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.AsyncBreak( process.Get() );

    return hr;
}

HRESULT MagoRemoteCmd_GetThreadContext( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int tid,
    /* [in] */ unsigned int featureMask,
    /* [in] */ unsigned __int64 extFeatureMask,
    /* [in] */ unsigned int size,
    /* [out] */ unsigned int *sizeRead,
    /* [out][length_is][size_is] */ byte *regBuffer)
{
    if ( hContext == NULL || sizeRead == NULL || regBuffer == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.GetThreadContext( 
        process.Get(), tid, featureMask, extFeatureMask, regBuffer, size );
    if ( SUCCEEDED( hr ) )
        *sizeRead = size;

    return hr;
}

HRESULT MagoRemoteCmd_SetThreadContext( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int tid,
    /* [in] */ unsigned int size,
    /* [in][size_is] */ byte *regBuffer)
{
    if ( hContext == NULL || regBuffer == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.SetThreadContext( process.Get(), tid, regBuffer, size );

    return hr;
}

HRESULT MagoRemoteCmd_GetPData( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address,
    /* [in] */ MagoRemote_Address imageBase,
    /* [in] */ unsigned int size,
    /* [out] */ unsigned int *sizeRead,
    /* [out][length_is][size_is] */ byte *pdataBuffer)
{
    if ( hContext == NULL || pdataBuffer == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->Session->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->Session->ExecThread.GetPData( 
        process.Get(), (Address) address, (Address) imageBase, size, *sizeRead, pdataBuffer );

    return hr;
}
