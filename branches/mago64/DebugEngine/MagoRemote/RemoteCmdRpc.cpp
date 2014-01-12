/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "MagoRemoteCmd_h.h"
#include "MagoRemoteEvent_h.h"
#include "RpcUtil.h"
#include <MagoDECommon.h>


struct CmdContext
{
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
    }
}

// The RPC runtime will call this function, if the connection to the client is lost.
void __RPC_USER HCTXCMD_rundown( HCTXCMD hContext )
{
    MagoRemoteCmd_Close( &hContext );
}

HRESULT MagoRemoteCmd_Open( 
    /* [in] */ handle_t hBinding,
    /* [in] */ const GUID *sessionUuid,
    /* [out] */ HCTXCMD *phContext)
{
    UNREFERENCED_PARAMETER( hBinding );

    HRESULT                         hr = S_OK;
    RPC_BINDING_HANDLE              hEventBinding = NULL;
    HCTXEVENT                       hEventCtx = NULL;
    UniquePtr<CmdContext>           context;

    *phContext = NULL;

    hr = GetEventBinding( sessionUuid, hEventBinding );
    if ( FAILED( hr ) )
        return hr;

    hr = OpenEventInterface( hEventBinding, sessionUuid, &hEventCtx );
    // Now that we've connected and gotten a context handle, 
    // we don't need the binding handle anymore.
    RpcBindingFree( &hEventBinding );
    if ( FAILED( hr ) )
        return hr;

    // TODO: store the context handle, and if it fails, close the event interface
    hEventCtx = NULL;

    context.Attach( new CmdContext() );
    if ( context.IsEmpty() )
        return E_OUTOFMEMORY;

    *phContext = context.Detach();

    return S_OK;
}

void MagoRemoteCmd_Close( 
    /* [out][in] */ HCTXCMD *phContext)
{
    if ( phContext == NULL || *phContext == NULL )
        return;

    CmdContext* context = (CmdContext*) *phContext;

    delete context;

    *phContext = NULL;
}

HRESULT MagoRemoteCmd_Launch( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ MagoRemote_LaunchInfo *launchInfo,
    /* [out] */ MagoRemote_ProcInfo *procInfo)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Attach( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [out] */ MagoRemote_ProcInfo *procInfo)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Terminate( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Detach( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_ResumeProcess( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
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
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_WriteMemory( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address,
    /* [in] */ unsigned int length,
    /* [out] */ unsigned int *lengthWritten,
    /* [in][size_is] */ byte *buffer)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_SetBreakpoint( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_RemoveBreakpoint( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_StepOut( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address targetAddr,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_StepInstruction( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean stepIn,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
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

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Continue( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Execute( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_AsyncBreak( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
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
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_SetThreadContext( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int tid,
    /* [in] */ unsigned int size,
    /* [in][size_is] */ byte *regBuffer)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}
