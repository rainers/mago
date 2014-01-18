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
#include <map>


struct CmdContext
{
    typedef std::map<UINT32, IProcess*> CmdProcessMap;

    MagoCore::DebuggerProxy ExecThread;
    HCTXEVENT               HEventContext;
    CmdProcessMap           ProcMap;
    Guard                   ProcGuard;

    ~CmdContext()
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

HRESULT MakeCmdContext( HCTXEVENT hEventCtx, CmdContext*& context )
{
    HRESULT                         hr = S_OK;
    RefPtr<Mago::EventCallback>     callback;
    UniquePtr<CmdContext>           cmdContext;

    callback = new Mago::EventCallback( hEventCtx );
    if ( callback.Get() == NULL )
        return E_OUTOFMEMORY;

    cmdContext.Attach( new CmdContext() );
    if ( cmdContext.IsEmpty() )
        return E_OUTOFMEMORY;

    cmdContext->HEventContext = hEventCtx;

    hr = cmdContext->ExecThread.Init( callback.Get() );
    if ( FAILED( hr ) )
        return hr;

    hr = cmdContext->ExecThread.Start();
    if ( FAILED( hr ) )
        return hr;

    context = cmdContext.Detach();

    return S_OK;
}

HRESULT OpenSession(
    const GUID* sessionUuid,
    HCTXCMD* phContext )
{
    HRESULT                         hr = S_OK;
    RPC_BINDING_HANDLE              hEventBinding = NULL;
    HCTXEVENT                       hEventCtx = NULL;
    UniquePtr<CmdContext>           context;

    hr = GetEventBinding( sessionUuid, hEventBinding );
    if ( FAILED( hr ) )
        return hr;

    hr = OpenEventInterface( hEventBinding, sessionUuid, &hEventCtx );
    // Now that we've connected and gotten a context handle, 
    // we don't need the binding handle anymore.
    RpcBindingFree( &hEventBinding );
    if ( FAILED( hr ) )
        return hr;

    hr = MakeCmdContext( hEventCtx, context.Ref() );
    if ( FAILED( hr ) )
    {
        CloseEventInterface( &hEventCtx );
        return hr;
    }

    hEventCtx = NULL;

    *phContext = context.Detach();

    return S_OK;
}

HRESULT MagoRemoteCmd_Open( 
    /* [in] */ handle_t hBinding,
    /* [in] */ const GUID *sessionUuid,
    /* [out] */ HCTXCMD *phContext)
{
    if ( hBinding == NULL || sessionUuid == NULL || phContext == NULL )
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    *phContext = NULL;

    if ( !Mago::NewSession() )
        return E_WRONG_STATE;

    Mago::NotifyAddSession();

    hr = OpenSession( sessionUuid, phContext );
    if ( FAILED( hr ) )
    {
        Mago::NotifyRemoveSession();
        return hr;
    }

    return S_OK;
}

void MagoRemoteCmd_Close( 
    /* [out][in] */ HCTXCMD *phContext)
{
    if ( phContext == NULL || *phContext == NULL )
        return;

    CmdContext* context = (CmdContext*) *phContext;

    context->ExecThread.Shutdown();
    CloseEventInterface( &context->HEventContext );

    delete context;

    *phContext = NULL;

    Mago::NotifyRemoveSession();
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

    hr = context->ExecThread.Launch( &execLaunchInfo, process.Ref() );
    if ( FAILED( hr ) )
        goto Error;

    hr = ToWireProcess( process.Get(), procInfo );
    if ( FAILED( hr ) )
        goto Error;

    hr = context->AddProcess( process.Get() );
    if ( FAILED( hr ) )
        goto Error;

Error:
    if ( FAILED( hr ) )
    {
        if ( procInfo->ExePath != NULL )
            MIDL_user_free( procInfo->ExePath );
        if ( process.Get() != NULL )
            context->ExecThread.Terminate( process.Get() );
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

    hr = context->ExecThread.Attach( pid, process.Ref() );
    if ( FAILED( hr ) )
        goto Error;

    hr = ToWireProcess( process.Get(), procInfo );
    if ( FAILED( hr ) )
        goto Error;

    hr = context->AddProcess( process.Get() );
    if ( FAILED( hr ) )
        goto Error;

Error:
    if ( FAILED( hr ) )
    {
        if ( procInfo->ExePath != NULL )
            MIDL_user_free( procInfo->ExePath );
        if ( process.Get() != NULL )
            context->ExecThread.Detach( process.Get() );
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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.Terminate( process.Get() );

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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.Detach( process.Get() );

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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.ResumeLaunchedProcess( process.Get() );

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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.ReadMemory( 
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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.WriteMemory( 
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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.SetBreakpoint( process.Get(), (Address) address );

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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.RemoveBreakpoint( process.Get(), (Address) address );

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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.StepOut( 
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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.StepInstruction( 
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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.StepRange( 
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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.Continue( process.Get(), handleException ? true : false );

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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.Execute( process.Get(), handleException ? true : false );

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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.AsyncBreak( process.Get() );

    return hr;
}

HRESULT MagoRemoteCmd_GetThreadContext( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int tid,
    /* [in] */ unsigned int mainFeatureMask,
    /* [in] */ unsigned __int64 extFeatureMask,
    /* [in] */ unsigned int size,
    /* [out] */ unsigned int *sizeRead,
    /* [out][length_is][size_is] */ byte *regBuffer)
{
    // TODO:
    UNREFERENCED_PARAMETER( extFeatureMask );

    if ( hContext == NULL || sizeRead == NULL || regBuffer == NULL )
        return E_INVALIDARG;

    HRESULT             hr = S_OK;
    CmdContext*         context = (CmdContext*) hContext;
    RefPtr<IProcess>    process;

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    // TODO: pass the masks to ExecThread, which can set them specific to the machine
    CONTEXT* contextX86 = (CONTEXT*) regBuffer;
    contextX86->ContextFlags = mainFeatureMask;

    hr = context->ExecThread.GetThreadContext( process.Get(), tid, regBuffer, size );
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

    if ( !context->FindProcess( pid, process.Ref() ) )
        return E_NOT_FOUND;

    hr = context->ExecThread.SetThreadContext( process.Get(), tid, regBuffer, size );

    return hr;
}
