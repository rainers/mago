/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Engine.h"
#include "Program.h"
#include "ProgramNode.h"
#include "EventCallback.h"
#include "Events.h"
#include "PendingBreakpoint.h"
#include "ComEnumWithCount.h"
#include "BpResolutionLocation.h"

using namespace std;


typedef CComEnumWithCount< 
    IEnumDebugPrograms2, 
    &IID_IEnumDebugPrograms2, 
    IDebugProgram2*, 
    _CopyInterface<IDebugProgram2>,
    CComMultiThreadModel
> EnumDebugPrograms;


namespace Mago
{
    // Engine

    Engine::Engine()
        :   mPollThreadStarted( false ),
            mSentEngineCreate( false ),
            mLastBPId( 0 ),
            mLastModId( 0 )
    {
    }

    HRESULT Engine::FinalConstruct()
    {
        HRESULT                 hr = S_OK;
        RefPtr<IMachine>        machine;
        RefPtr<IEventCallback>  callback( new EventCallback( this ) );

        if ( callback.Get() == NULL )
            return E_OUTOFMEMORY;

        hr = MakeMachineX86( machine.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = mDebugger.Init( machine.Get(), callback.Get() );
        if ( FAILED( hr ) )
            return hr;

        return hr;
    }

    void Engine::FinalRelease()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugEngine2 methods 

    HRESULT Engine::EnumPrograms( IEnumDebugPrograms2** ppEnum )
    {
        return MakeEnumWithCount<
            EnumDebugPrograms,
            IEnumDebugPrograms2,
            ProgramMap,
            IDebugProgram2 >( mProgs, ppEnum );
    }

    HRESULT Engine::SetException( EXCEPTION_INFO* pException )
    {
        GuardedArea area( mExceptionGuard );
        return mExceptionInfos.SetException( pException );
    }

    HRESULT Engine::RemoveSetException( EXCEPTION_INFO* pException )
    {
        GuardedArea area( mExceptionGuard );
        return mExceptionInfos.RemoveSetException( pException );
    }

    HRESULT Engine::RemoveAllSetExceptions( REFGUID guidType )
    {
        GuardedArea area( mExceptionGuard );
        return mExceptionInfos.RemoveAllSetExceptions( guidType );
    }

    bool Engine::FindExceptionInfo( const GUID& guid, DWORD code, ExceptionInfo& excInfo )
    {
        GuardedArea area( mExceptionGuard );
        return mExceptionInfos.FindExceptionInfo( guid, code, excInfo );
    }

    bool Engine::FindExceptionInfo( const GUID& guid, LPCOLESTR name, ExceptionInfo& excInfo )
    {
        GuardedArea area( mExceptionGuard );
        return mExceptionInfos.FindExceptionInfo( guid, name, excInfo );
    }

    HRESULT Engine::GetEngineId( GUID* pguidEngine )
    {
        if ( pguidEngine == NULL )
            return E_INVALIDARG;

        *pguidEngine = ::GetEngineId();
        return S_OK;
    }

    HRESULT Engine::DestroyProgram( IDebugProgram2* pProgram )
    {
        HRESULT     hr = S_OK;
        DWORD       id = 0;

        RefPtr<Program>     prog;

        hr = ::GetProcessId( pProgram, id );
        if ( FAILED( hr ) )
            return hr;

        if ( !FindProgram( id, prog ) )
            return E_NOT_FOUND;

        DeleteProgram( prog.Get() );

        ShutdownIfNeeded();

        return hr;
    }

    HRESULT Engine::SetLocale( WORD wLangID )
    { return E_NOTIMPL; } 

    HRESULT Engine::SetRegistryRoot( LPCOLESTR pszRegistryRoot )
    {
        HRESULT hr = S_OK;

        hr = mExceptionInfos.SetRegistryRoot( pszRegistryRoot );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT Engine::SetMetric( LPCOLESTR pszMetric, VARIANT varValue )
    {
        _RPT1( _CRT_WARN, "SetMetric '%ls'\n", pszMetric );
        return E_NOTIMPL;
    }

    HRESULT Engine::CauseBreak()
    { return E_NOTIMPL; } 

    HRESULT Engine::Attach( 
        IDebugProgram2** rgpPrograms, 
        IDebugProgramNode2** rgpProgramNodes, 
        DWORD celtPrograms, 
        IDebugEventCallback2* pCallback, 
        ATTACH_REASON dwReason )
    {
        HRESULT     hr = S_OK;
        DWORD       id = 0;

        RefPtr<Program>     prog;

        if ( celtPrograms != 1 )
            return E_UNEXPECTED;

        hr = ::GetProcessId( rgpPrograms[0], id );
        if ( FAILED( hr ) )
            return hr;

        if ( !FindProgram( id, prog ) )
            return E_NOT_FOUND;

        prog->SetPortSettings( rgpPrograms[0] );
        prog->SetCallback( pCallback );
        prog->SetDebuggerProxy( &mDebugger );

        CComPtr<IDebugEngine2>      engine;
        CComPtr<IDebugProgram2>     prog2;

        hr = QueryInterface( IID_IDebugEngine2, (void**) &engine );
        _ASSERT( hr == S_OK );

        hr = prog->QueryInterface( __uuidof( IDebugProgram2 ), (void**) &prog2 );
        _ASSERT( hr == S_OK );

        if ( !mSentEngineCreate )
        {
            RefPtr<EngineCreateEvent>   event;

            hr = MakeCComObject( event );
            if ( FAILED( hr ) )
                return hr;

            event->Init( engine );

            hr = event->Send( pCallback, engine, prog2, NULL );
            if ( FAILED( hr ) )
                return hr;

            mSentEngineCreate = true;
        }

        RefPtr<ProgramCreateEvent>  progCreateEvent;

        hr = MakeCComObject( progCreateEvent );
        if ( FAILED( hr ) )
            return hr;

        hr = progCreateEvent->Send( pCallback, engine, prog2, NULL );
        if ( FAILED( hr ) )
            return hr;

        if ( dwReason == ATTACH_REASON_LAUNCH )
        {
            hr = mDebugger.ResumeProcess( prog->GetCoreProcess() );
            if ( FAILED( hr ) )
                return hr;
        }

        prog->SetAttached();

        return hr;
    }

    HRESULT Engine::ContinueFromSynchronousEvent( IDebugEvent2* pEvent )
    {
        OutputDebugStringA( "ContinueFromSynchronousEvent\n" );

        // TODO: verify that it's our program destroy event
        ShutdownIfNeeded();

        return S_OK;
    }

    HRESULT Engine::CreatePendingBreakpoint( 
       IDebugBreakpointRequest2* pBPRequest, 
       IDebugPendingBreakpoint2** ppPendingBP )
    {
        HRESULT             hr = S_OK;
        BP_LOCATION_TYPE    locType = BPLT_NONE;
        RefPtr<PendingBreakpoint>           pendBP;
        CComPtr<IDebugPendingBreakpoint2>   ad7PendBP;

        // only code breakpoints are supported now
        pBPRequest->GetLocationType( &locType );
        if ( (locType & BPT_CODE) != BPT_CODE )
            return E_FAIL;

        // only file line code breakpoints are supported now
        {
            BpRequestInfo   reqInfo;
            pBPRequest->GetRequestInfo( BPREQI_BPLOCATION, &reqInfo );

            if ( (reqInfo.bpLocation.bpLocationType != BPLT_CODE_FILE_LINE)
                && (reqInfo.bpLocation.bpLocationType != BPLT_CODE_ADDRESS)
                && (reqInfo.bpLocation.bpLocationType != BPLT_CODE_CONTEXT) )
                return E_FAIL;
        }

        hr = MakeCComObject( pendBP );
        if ( FAILED( hr ) )
            return hr;

        const DWORD Id = GetNextBPId();

        pendBP->Init( Id, this, pBPRequest, mCallback, &mDebugger );

        hr = pendBP->QueryInterface( __uuidof( IDebugPendingBreakpoint2 ), (void**) &ad7PendBP );
        if ( FAILED( hr ) )
            return hr;

        *ppPendingBP = ad7PendBP.Detach();

        return hr;
    }

    HRESULT Engine::AddPendingBP( PendingBreakpoint* pendingBP )
    {
        const DWORD Id = pendingBP->GetId();
        GuardedArea guard( mPendingBPGuard );

        mBPs.insert( BPMap::value_type( Id, pendingBP ) );

        return S_OK;
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugEngineLaunch2 methods 

    HRESULT Engine::LaunchSuspended( 
       LPCOLESTR             pszMachine,
       IDebugPort2*          pPort,
       LPCOLESTR             pszExe,
       LPCOLESTR             pszArgs,
       LPCOLESTR             pszDir,
       BSTR                  bstrEnv,
       LPCOLESTR             pszOptions,
       LAUNCH_FLAGS          dwLaunchFlags,
       DWORD                 hStdInput,
       DWORD                 hStdOutput,
       DWORD                 hStdError,
       IDebugEventCallback2* pCallback,
       IDebugProcess2**      ppDebugProcess
    )
    {
        HRESULT             hr = S_OK;
        LaunchInfo          info = { 0 };
        wstring             cmdLine;

        if ( mCallback == NULL )
            mCallback = pCallback;

        // TODO: implement
        if ( (dwLaunchFlags & LAUNCH_NODEBUG) != 0 )
        {
            return E_NOTIMPL;
        }

        // ignore LAUNCH_ENABLE_ENC, we don't support edit-and-continue

        if ( (dwLaunchFlags & LAUNCH_MERGE_ENV) != 0 )
        {
            // TODO: merge env if needed
        }

        cmdLine = BuildCommandLine( pszExe, pszArgs );

        info.CommandLine = cmdLine.c_str();
        info.Exe = pszExe;
        info.Dir = pszDir;
        info.EnvBstr = bstrEnv;
        info.StdInput = (HANDLE) hStdInput;
        info.StdOutput = (HANDLE) hStdOutput;
        info.StdError = (HANDLE) hStdError;
        info.Suspend = true;

        hr = EnsurePollThreadRunning();
        if ( FAILED( hr ) )
            return hr;

        hr = LaunchSuspendedInternal(
            pPort,
            info,
            pCallback,
            ppDebugProcess );

        ShutdownIfNeeded();

        return hr;
    }

    HRESULT Engine::LaunchSuspendedInternal( 
       IDebugPort2*          pPort,
       LaunchInfo&           launchParams,
       IDebugEventCallback2* pCallback,
       IDebugProcess2**      ppDebugProcess
        )
    {
        HRESULT             hr = S_OK;
        RefPtr<IProcess>    proc;
        RefPtr<Program>     prog;
        AD_PROCESS_ID       fullProcId = { 0 };

        GUID portId;
        CComBSTR portName;
        CComPtr<IDebugPortRequest2> portRequest;
        CComPtr<IDebugPortSupplier2> portSupplier;
        hr = pPort->GetPortId( &portId );
        hr = pPort->GetPortName( &portName );
        hr = pPort->GetPortRequest( &portRequest );
        hr = pPort->GetPortSupplier( &portSupplier );

        if ( portRequest != NULL )
        {
            CComBSTR reqPortName;
            portRequest->GetPortName( &reqPortName );
        }

        CComQIPtr<IDebugDefaultPort2> defPort = pPort;
        if ( defPort != NULL )
        {
            CComPtr<IDebugCoreServer3> server;

            hr = defPort->QueryIsLocal();
            hr = defPort->GetServer( &server );

            if ( server != NULL )
            {
                CComBSTR serverName;
                CComBSTR serverFriendlyName;
                CComBSTR machineName;
                MACHINE_INFO macInfo;
                CONNECTION_PROTOCOL protocol;

                hr = server->QueryIsLocal();
                hr = server->GetServerName( &serverName );
                hr = server->GetServerFriendlyName( &serverFriendlyName );
                hr = server->GetMachineName( &machineName );
                hr = server->GetMachineInfo( 0xffffffff, &macInfo );
                hr = server->GetConnectionProtocol( &protocol );
            }
        }

        CComQIPtr<IDebugWindowsComputerPort2 > portEx = pPort;
        if ( portEx != NULL )
        {
            COMPUTER_INFO compInfo;

            hr = portEx->GetComputerInfo( &compInfo );
            int a = 1;
        }

        GUID supplierId;
        CComBSTR supplierName;
        hr = portSupplier->CanAddPort();
        hr = portSupplier->GetPortSupplierId( &supplierId );
        hr = portSupplier->GetPortSupplierName( &supplierName );

        CComQIPtr<IDebugPortSupplier3> portSupplier3 = portSupplier;
        if ( portSupplier3 != NULL )
        {
            hr = portSupplier3->CanPersistPorts();
        }

        CComQIPtr<IDebugPortSupplierDescription2> psDesc = portSupplier;
        if ( psDesc != NULL )
        {
            CComBSTR desc;
            PORT_SUPPLIER_DESCRIPTION_FLAGS descFlags = 0;
            psDesc->GetDescription( &descFlags, &desc );
            descFlags = descFlags;
        }

        hr = mDebugger.Launch( &launchParams, proc.Ref() );
        if ( FAILED( hr ) )
            return hr;

        fullProcId.ProcessIdType = AD_PROCESS_ID_SYSTEM;
        fullProcId.ProcessId.dwProcessId = proc->GetId();

        hr = pPort->GetProcess( fullProcId, ppDebugProcess );
        if ( FAILED( hr ) )
            goto Error;

        hr = MakeCComObject( prog );
        if ( FAILED( hr ) )
            goto Error;

        prog->SetEngine( this );
        prog->SetProcess( *ppDebugProcess );
        prog->SetCoreProcess( proc.Get() );

        mProgs.insert( ProgramMap::value_type( proc->GetId(), prog ) );

Error:
        if ( FAILED( hr ) )
        {
            if ( proc.Get() != NULL )
            {
                mDebugger.TerminateNewProcess( proc.Get() );
            }
        }

        return hr;
    }

    HRESULT Engine::ResumeProcess( 
       IDebugProcess2* pProcess
    )
    {
        HRESULT hr = S_OK;

        hr = ResumeProcessInternal( pProcess );

        ShutdownIfNeeded();

        return hr;
    }

    HRESULT Engine::ResumeProcessInternal( IDebugProcess2* pProcess )
    {
        HRESULT         hr = S_OK;
        DWORD           id = 0;

        RefPtr<Program>             prog;
        RefPtr<ProgramNode>         progNode;
        CComPtr<IDebugPort2>        port;
        CComPtr<IDebugDefaultPort2> defaultPort;
        CComPtr<IDebugPortNotify2>  portNotify;

        hr = ::GetProcessId( pProcess, id );
        if ( FAILED( hr ) )
            return hr;

        if ( !FindProgram( id, prog ) )
            return E_NOT_FOUND;

        hr = pProcess->GetPort( &port );
        if ( FAILED( hr ) )
            goto Error;

        hr = port.QueryInterface( &defaultPort );
        if ( FAILED( hr ) )
            goto Error;

        hr = defaultPort->GetPortNotify( &portNotify );
        if ( FAILED( hr ) )
            goto Error;

        hr = MakeCComObject( progNode );
        if ( FAILED( hr ) )
            goto Error;

        progNode->SetProcessId( id );

        hr = portNotify->AddProgramNode( progNode.Get() );
        if ( FAILED( hr ) )
            goto Error;

        if ( !prog->GetAttached() )
        {
            hr = E_FAIL;
            goto Error;
        }

        // Attach should resume the process

Error:
        if ( FAILED( hr ) )
        {
            if ( prog.Get() != NULL )
            {
                mDebugger.TerminateNewProcess( prog->GetCoreProcess() );
                DeleteProgram( prog.Get() );
            }
        }

        return hr;
    }

    HRESULT Engine::CanTerminateProcess( 
       IDebugProcess2* pProcess
    )
    {
        // CanTerminateProcess during stopping the debugger, but there is a racing condition
        //  with EXIT_PROCESS_DEBUG_EVENT, which deletes the program from mProgs.
        // returning error here causes a MessageBox, to suppress it, we ignore
        //  the error codes and pretend the world's perfect
        HRESULT         hr = S_OK;
        DWORD           id = 0;

        RefPtr<Program>             prog;

        hr = ::GetProcessId( pProcess, id );
        if ( FAILED( hr ) )
            return S_OK; //hr;

        if ( !FindProgram( id, prog ) )
            return S_OK; // E_NOT_FOUND;

        return hr;
    }

    HRESULT Engine::TerminateProcess( 
       IDebugProcess2* pProcess
    )
    {
        // see comment in CanTeminateProcess
        HRESULT         hr = S_OK;
        DWORD           id = 0;

        RefPtr<Program>             prog;

        hr = ::GetProcessId( pProcess, id );
        if ( FAILED( hr ) )
            return S_OK; // hr;

        if ( !FindProgram( id, prog ) )
            return S_OK; // E_NOT_FOUND;

        hr = mDebugger.Terminate( prog->GetCoreProcess() );
        return S_OK; // hr
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // privtate methods 

    HRESULT Engine::EnsurePollThreadRunning()
    {
        // TODO: do we have to guard this?

        HRESULT hr = S_OK;

        if ( !mPollThreadStarted )
        {
            hr = mDebugger.Start();
            if ( FAILED( hr ) )
                return hr;

            mPollThreadStarted = true;
        }

        return hr;
    }

    void Engine::ShutdownIfNeeded()
    {
        if ( mProgs.size() > 0 )
            return;

        mDebugger.Shutdown();
        // TODO: this should probably be guarded, too

        for ( BPMap::iterator it = mBPs.begin();
            it != mBPs.end();
            it++ )
        {
            it->second->Dispose();
        }

        mBPs.clear();
    }

    bool Engine::FindProgram( DWORD id, RefPtr<Program>& prog )
    {
        ProgramMap::iterator    it = mProgs.find( id );

        if ( it == mProgs.end() )
            return false;

        prog = it->second;
        return true;
    }

    void Engine::DeleteProgram( Program* prog )
    {
        mProgs.erase( prog->GetCoreProcess()->GetId() );

        prog->Dispose();
    }

    void Engine::OnPendingBPDelete( PendingBreakpoint* pendingBP )
    {
        _ASSERT( pendingBP != NULL );

        DWORD       id = pendingBP->GetId();
        GuardedArea guard( mPendingBPGuard );

        mBPs.erase( id );
        pendingBP->Dispose();
    }

    DWORD Engine::GetNextBPId()
    {
        mLastBPId++;
        return mLastBPId;
    }

    DWORD Engine::GetNextModuleId()
    {
        mLastModId++;
        return mLastModId;
    }

    void Engine::ForeachProgram( ProgramCallback* callback )
    {
        for ( ProgramMap::iterator it = mProgs.begin();
            it != mProgs.end();
            it++ )
        {
            if ( !callback->AcceptProgram( it->second.Get() ) )
                break;
        }
    }

    HRESULT Engine::BindPendingBPsToModule( Module* mod, Program* prog )
    {
        _ASSERT( mod != NULL );

        HRESULT     hr = S_OK;
        GuardedArea guard( mPendingBPGuard );

        for ( BPMap::iterator it = mBPs.begin();
            it != mBPs.end();
            it++ )
        {
            // TODO: what about error code?
            it->second->BindToModule( mod, prog );
        }

        return hr;
    }

    HRESULT Engine::UnbindPendingBPsFromModule( Module* mod, Program* prog )
    {
        _ASSERT( mod != NULL );

        HRESULT     hr = S_OK;
        GuardedArea guard( mPendingBPGuard );

        for ( BPMap::iterator it = mBPs.begin();
            it != mBPs.end();
            it++ )
        {
            // TODO: what about error code?
            it->second->UnbindFromModule( mod, prog );
        }

        return hr;
    }
}
