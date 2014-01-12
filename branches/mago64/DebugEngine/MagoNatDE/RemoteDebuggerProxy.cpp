/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "RemoteDebuggerProxy.h"
#include "ArchData.h"
#include "Config.h"
#include "EventCallback.h"
#include "MagoRemoteCmd_i.h"
#include "MagoRemoteEvent_i.h"
#include "RegisterSet.h"
#include "RemoteProcess.h"
#include "RpcUtil.h"
#include <MagoDECommon.h>


#define AGENT_X64_VALUE         L"Remote_x64"


namespace Mago
{
    HRESULT StartAgent( const wchar_t* sessionGuidStr )
    {
        int                 ret = 0;
        BOOL                bRet = FALSE;
        STARTUPINFO         startupInfo = { 0 };
        PROCESS_INFORMATION processInfo = { 0 };
        HandlePtr           hEventPtr;
        std::wstring        cmdLine;
        wchar_t             eventName[AgentStartupEventNameLength + 1] = AGENT_STARTUP_EVENT_PREFIX;
        wchar_t             agentPath[MAX_PATH] = L"";
        int                 agentPathLen = _countof( agentPath );
        HKEY                hKey = NULL;

        startupInfo.cb = sizeof startupInfo;

        ret = OpenRootRegKey( false, hKey );
        if ( ret != ERROR_SUCCESS )
            return HRESULT_FROM_WIN32( ret );

        ret = GetRegString( hKey, AGENT_X64_VALUE, agentPath, agentPathLen );
        RegCloseKey( hKey );
        if ( ret != ERROR_SUCCESS )
            return HRESULT_FROM_WIN32( ret );

        wcscat_s( eventName, sessionGuidStr );

        hEventPtr = CreateEvent( NULL, TRUE, FALSE, eventName );
        if ( hEventPtr.IsEmpty() )
            return GetLastHr();

        cmdLine.append( L"\"" );
        cmdLine.append( agentPath );
        cmdLine.append( L"\" -exclusive " );
        cmdLine.append( sessionGuidStr );

        bRet = CreateProcess(
            agentPath,
            &cmdLine.front(),   // not empty, so we can call it
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &startupInfo,
            &processInfo );
        if ( !bRet )
            return GetLastHr();

        HANDLE handles[] = { hEventPtr, processInfo.hProcess };
        DWORD waitRet = WaitForMultipleObjects( 
            _countof( handles ), 
            handles, 
            FALSE, 
            AgentStartupTimeoutMillis );

        CloseHandle( processInfo.hProcess );
        CloseHandle( processInfo.hThread );

        if ( waitRet == WAIT_FAILED )
            return GetLastHr();
        if ( waitRet == WAIT_TIMEOUT )
            return E_TIMEOUT;
        if ( waitRet == WAIT_OBJECT_0 + 1 )
            return E_FAIL;

        return S_OK;
    }

    HRESULT StartServer( const wchar_t* sessionGuidStr )
    {
        HRESULT         hr = S_OK;
        RPC_STATUS      rpcRet = RPC_S_OK;
        bool            registered = false;
        std::wstring    endpoint( AGENT_EVENT_IF_LOCAL_ENDPOINT_PREFIX );

        endpoint.append( sessionGuidStr );

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
            MagoRemoteEvent_v1_0_s_ifspec,
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
                RpcServerUnregisterIf( MagoRemoteEvent_v1_0_s_ifspec, NULL, FALSE );
        }
        return hr;
    }

    HRESULT StopServer()
    {
        RpcServerUnregisterIf( MagoRemoteEvent_v1_0_s_ifspec, NULL, FALSE );
        RpcMgmtStopServerListening( NULL );
        RpcMgmtWaitServerListen();
        return S_OK;
    }

    HRESULT OpenCmdInterface( RPC_BINDING_HANDLE hBinding, const GUID& sessionGuid, HCTXCMD& hContext )
    {
        HRESULT hr = S_OK;

        __try
        {
            hr = MagoRemoteCmd_Open( hBinding, &sessionGuid, &hContext );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            hr = HRESULT_FROM_WIN32( RpcExceptionCode() );
        }

        return hr;
    }

    HRESULT StartClient( const wchar_t* sessionGuidStr, const GUID& sessionGuid, HCTXCMD& hContext )
    {
        HRESULT             hr = S_OK;
        RPC_STATUS          rpcRet = RPC_S_OK;
        RPC_WSTR            strBinding = NULL;
        RPC_BINDING_HANDLE  hBinding = NULL;
        std::wstring        endpoint( AGENT_CMD_IF_LOCAL_ENDPOINT_PREFIX );

        endpoint.append( sessionGuidStr );

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

        hr = OpenCmdInterface( hBinding, sessionGuid, hContext );

        // Now that we've connected and gotten a context handle, 
        // we don't need the binding handle anymore.
        RpcBindingFree( &hBinding );

        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT StopClient( HCTXCMD& hContext )
    {
        __try
        {
            MagoRemoteCmd_Close( &hContext );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            // nothing to do
        }

        return S_OK;
    }


    //------------------------------------------------------------------------
    // RemoteDebuggerProxy
    //------------------------------------------------------------------------

    RemoteDebuggerProxy::RemoteDebuggerProxy()
        :   mSessionGuid( GUID_NULL ),
            mhContext( NULL )
    {
    }

    RemoteDebuggerProxy::~RemoteDebuggerProxy()
    {
        Shutdown();
    }

    HRESULT RemoteDebuggerProxy::Init( EventCallback* callback )
    {
        _ASSERT( callback != NULL );
        if ( (callback == NULL) )
            return E_INVALIDARG;

        mCallback = callback;

        return S_OK;
    }

    HRESULT RemoteDebuggerProxy::Start()
    {
        if ( mSessionGuid != GUID_NULL )
            return S_OK;

        HRESULT     hr = S_OK;
        GUID        sessionGuid = { 0 };
        wchar_t     sessionGuidStr[GUID_LENGTH + 1] = L"";
        int         ret = 0;

        hr = CoCreateGuid( &sessionGuid );
        if ( FAILED( hr ) )
            return hr;

        ret = StringFromGUID2( sessionGuid, sessionGuidStr, _countof( sessionGuidStr ) );
        _ASSERT( ret > 0 );
        if ( ret == 0 )
            return E_FAIL;

        mSessionGuid = sessionGuid;

        hr = StartAgent( sessionGuidStr );
        if ( FAILED( hr ) )
            return hr;

        hr = StartServer( sessionGuidStr );
        if ( FAILED( hr ) )
            return hr;

        hr = StartClient( sessionGuidStr, sessionGuid, mhContext );
        if ( FAILED( hr ) )
        {
            StopServer();
            return hr;
        }

        return S_OK;
    }

    void RemoteDebuggerProxy::Shutdown()
    {
        StopServer();
        StopClient( mhContext );
    }


    //----------------------------------------------------------------------------
    // IDebuggerProxy
    //----------------------------------------------------------------------------

    HRESULT RemoteDebuggerProxy::Launch( LaunchInfo* launchInfo, ICoreProcess*& process )
    {
        _ASSERT( launchInfo != NULL );
        if ( launchInfo == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Attach( uint32_t pid, ICoreProcess*& process )
    {
        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Terminate( ICoreProcess* process )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Detach( ICoreProcess* process )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::ResumeLaunchedProcess( ICoreProcess* process )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::ReadMemory( 
        ICoreProcess* process, 
        Address address,
        uint32_t length, 
        uint32_t& lengthRead, 
        uint32_t& lengthUnreadable, 
        uint8_t* buffer )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::WriteMemory( 
        ICoreProcess* process, 
        Address address,
        uint32_t length, 
        uint32_t& lengthWritten, 
        uint8_t* buffer )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::SetBreakpoint( ICoreProcess* process, Address address )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::RemoveBreakpoint( ICoreProcess* process, Address address )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::StepOut( ICoreProcess* process, Address targetAddr, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::StepInstruction( ICoreProcess* process, bool stepIn, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::StepRange( 
        ICoreProcess* process, bool stepIn, AddressRange range, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Continue( ICoreProcess* process, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Execute( ICoreProcess* process, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::AsyncBreak( ICoreProcess* process )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::GetThreadContext( 
        ICoreProcess* process, ICoreThread* thread, IRegisterSet*& regSet )
    {
        _ASSERT( process != NULL );
        _ASSERT( thread != NULL );
        if ( process == NULL || thread == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote
            || thread->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        // TODO: get flags and context size from archData

        HRESULT hr = S_OK;
        CONTEXT context = { 0 };
        DWORD contextFlags = CONTEXT_FULL 
            | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

        // TODO: send the request to the remote agent

        ArchData* archData = process->GetArchData();

        hr = archData->BuildRegisterSet( &context, sizeof context, regSet );
        if ( FAILED( hr ) )
            return hr;

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::SetThreadContext( 
        ICoreProcess* process, ICoreThread* thread, IRegisterSet* regSet )
    {
        _ASSERT( process != NULL );
        _ASSERT( thread != NULL );
        _ASSERT( regSet != NULL );
        if ( process == NULL || thread == NULL || regSet == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote
            || thread->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT         hr = S_OK;
        const void*     contextBuf = NULL;
        uint32_t        contextSize = 0;

        if ( !regSet->GetThreadContext( contextBuf, contextSize ) )
            return E_FAIL;

        // TODO: send it to the remote agent

        return E_NOTIMPL;
    }
}
