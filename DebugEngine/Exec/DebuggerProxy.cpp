/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DebuggerProxy.h"
#include "CommandFunctor.h"
#include "EventCallback.h"
#include "Thread.h"
#include <ObjBase.h>
#include <process.h>


typedef unsigned ( __stdcall *CrtThreadProc )( void * );

// these values can be tweaked, as long as we're responsive and don't spin
const DWORD EventTimeoutMillis = 50;
const DWORD CommandTimeoutMillis = 0;


namespace MagoCore
{
    DebuggerProxy::DebuggerProxy()
        :   mhThread( NULL ),
            mWorkerTid( 0 ),
            mCallback( NULL ),
            mhReadyEvent( NULL ),
            mhCommandEvent( NULL ),
            mhResultEvent( NULL ),
            mCurCommand( NULL ),
            mShutdown( false )
    {
    }

    DebuggerProxy::~DebuggerProxy()
    {
        // TODO: use smart pointers

        Shutdown();

        if ( mhThread != NULL )
            CloseHandle( mhThread );

        if ( mhReadyEvent != NULL )
            CloseHandle( mhReadyEvent );

        if ( mhCommandEvent != NULL )
            CloseHandle( mhCommandEvent );

        if ( mhResultEvent != NULL )
            CloseHandle( mhResultEvent );
    }

    HRESULT DebuggerProxy::Init( IEventCallback* callback )
    {
        _ASSERT( callback != NULL );
        if ( (callback == NULL ) )
            return E_INVALIDARG;
        if ( (mCallback != NULL ) )
            return E_ALREADY_INIT;

        HandlePtr   hReadyEvent;
        HandlePtr   hCommandEvent;
        HandlePtr   hResultEvent;

        hReadyEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( hReadyEvent.IsEmpty() )
            return GetLastHr();

        hCommandEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( hCommandEvent.IsEmpty() )
            return GetLastHr();

        hResultEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( hResultEvent.IsEmpty() )
            return GetLastHr();

        mhReadyEvent = hReadyEvent.Detach();
        mhCommandEvent = hCommandEvent.Detach();
        mhResultEvent = hResultEvent.Detach();

        mCallback = callback;
        mCallback->AddRef();

        return S_OK;
    }

    HRESULT DebuggerProxy::Start()
    {
        _ASSERT( mCallback != NULL );
        if ( (mCallback == NULL ) )
            return E_UNEXPECTED;
        if ( mhThread != NULL )
            return E_UNEXPECTED;

        HandlePtr   hThread;

        hThread = (HANDLE) _beginthreadex(
            NULL,
            0,
            (CrtThreadProc) DebugPollProc,
            this,
            0,
            NULL );
        if ( hThread.IsEmpty() )
            return GetLastHr();

        HANDLE      waitObjs[2] = { mhReadyEvent, hThread.Get() };
        DWORD       waitRet = 0;

        // TODO: on error, thread will be shutdown from Shutdown method

        waitRet = WaitForMultipleObjects( _countof( waitObjs ), waitObjs, FALSE, INFINITE );
        if ( waitRet == WAIT_FAILED )
            return GetLastHr();

        if ( waitRet == WAIT_OBJECT_0 + 1 )
        {
            DWORD   exitCode = (DWORD) E_FAIL;

            // the thread ended because of an error, let's get the return exit code
            GetExitCodeThread( hThread.Get(), &exitCode );

            return (HRESULT) exitCode;
        }
        _ASSERT( waitRet == WAIT_OBJECT_0 );

        mhThread = hThread.Detach();

        return S_OK;
    }

    void DebuggerProxy::Shutdown()
    {
        // TODO: is this enough?

        mShutdown = true;

        if ( mhThread != NULL )
        {
            // there are no infinite waits on the poll thread, so it'll detect shutdown signal
            WaitForSingleObject( mhThread, INFINITE );
        }

        if ( mCallback != NULL )
        {
            mCallback->Release();
            mCallback = NULL;
        }
    }

    HRESULT DebuggerProxy::InvokeCommand( CommandFunctor& cmd )
    {
        if ( mWorkerTid == GetCurrentThreadId() )
        {
            // since we're on the poll thread, we can run the command directly
            cmd.Run();
        }
        else
        {
            GuardedArea area( mCommandGuard );

            mCurCommand = &cmd;

            ResetEvent( mhResultEvent );
            SetEvent( mhCommandEvent );

            DWORD   waitRet = 0;
            HANDLE  handles[2] = { mhResultEvent, mhThread };

            waitRet = WaitForMultipleObjects( _countof( handles ), handles, FALSE, INFINITE );
            mCurCommand = NULL;
            if ( waitRet == WAIT_FAILED )
                return GetLastHr();

            if ( waitRet == WAIT_OBJECT_0 + 1 )
                return CO_E_REMOTE_COMMUNICATION_FAILURE;
        }

        return S_OK;
    }

//----------------------------------------------------------------------------
// Commands
//----------------------------------------------------------------------------

    HRESULT DebuggerProxy::Launch( LaunchInfo* launchInfo, IProcess*& process )
    {
        HRESULT         hr = S_OK;
        LaunchParams    params( mExec );

        params.Settings = launchInfo;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        if ( SUCCEEDED( params.OutHResult ) )
            process = params.OutProcess.Detach();

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::Attach( uint32_t id, IProcess*& process )
    {
        HRESULT         hr = S_OK;
        AttachParams    params( mExec );

        params.ProcessId = id;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        if ( SUCCEEDED( params.OutHResult ) )
            process = params.OutProcess.Detach();

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::Terminate( IProcess* process )
    {
        HRESULT         hr = S_OK;
        TerminateParams params( mExec );

        params.Process = process;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::Detach( IProcess* process )
    {
        HRESULT         hr = S_OK;
        DetachParams    params( mExec );

        params.Process = process;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::ResumeLaunchedProcess( IProcess* process )
    {
        HRESULT             hr = S_OK;
        ResumeLaunchedProcessParams params( mExec );

        params.Process = process;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::ReadMemory( 
        IProcess* process, 
        Address address,
        uint32_t length, 
        uint32_t& lengthRead, 
        uint32_t& lengthUnreadable, 
        uint8_t* buffer )
    {
        // call it directly for performance (it gets called a lot for stack walking)
        // we're allowed to, since this function is now free threaded
        return mExec.ReadMemory( process, address, length, lengthRead, lengthUnreadable, buffer );
    }

    HRESULT DebuggerProxy::WriteMemory( 
        IProcess* process, 
        Address address,
        uint32_t length, 
        uint32_t& lengthWritten, 
        uint8_t* buffer )
    {
        HRESULT             hr = S_OK;
        WriteMemoryParams   params( mExec );

        params.Process = process;
        params.Address = address;
        params.Length = length;
        params.Buffer = buffer;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        if ( SUCCEEDED( params.OutHResult ) )
            lengthWritten = params.OutLengthWritten;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::SetBreakpoint( IProcess* process, Address address )
    {
        return mExec.SetBreakpoint( process, address );
    }

    HRESULT DebuggerProxy::RemoveBreakpoint( IProcess* process, Address address )
    {
        return mExec.RemoveBreakpoint( process, address );
    }

    HRESULT DebuggerProxy::StepOut( IProcess* process, Address targetAddr, bool handleException )
    {
        HRESULT                 hr = S_OK;
        StepOutParams           params( mExec );

        params.Process = process;
        params.TargetAddress = targetAddr;
        params.HandleException = handleException;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::StepInstruction( IProcess* process, bool stepIn, bool handleException )
    {
        HRESULT                 hr = S_OK;
        StepInstructionParams   params( mExec );

        params.Process = process;
        params.StepIn = stepIn;
        params.HandleException = handleException;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::StepRange( IProcess* process, bool stepIn, AddressRange range, bool handleException )
    {
        HRESULT         hr = S_OK;
        StepRangeParams params( mExec );

        params.Process = process;
        params.StepIn = stepIn;
        params.Range = range;
        params.HandleException = handleException;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::Continue( IProcess* process, bool handleException )
    {
        HRESULT         hr = S_OK;
        ContinueParams  params( mExec );

        params.Process = process;
        params.HandleException = handleException;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::Execute( IProcess* process, bool handleException )
    {
        HRESULT         hr = S_OK;
        ExecuteParams   params( mExec );

        params.Process = process;
        params.HandleException = handleException;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::AsyncBreak( IProcess* process )
    {
        HRESULT             hr = S_OK;
        AsyncBreakParams    params( mExec );

        params.Process = process;

        hr = InvokeCommand( params );
        if ( FAILED( hr ) )
            return hr;

        return params.OutHResult;
    }

    HRESULT DebuggerProxy::GetThreadContext( 
        IProcess* process, 
        uint32_t threadId, 
        uint32_t features, 
        uint64_t extFeatures, 
        void* context, 
        uint32_t size )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        hr = mExec.GetThreadContext( process, threadId, features, extFeatures, context, size );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT DebuggerProxy::SetThreadContext( 
        IProcess* process, 
        uint32_t threadId, 
        const void* context, 
        uint32_t size )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        HRESULT         hr = S_OK;

        hr = mExec.SetThreadContext( process, threadId, context, size );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT DebuggerProxy::GetPData( 
        IProcess* process, 
        Address address, 
        Address imageBase, 
        uint32_t size, 
        uint32_t& sizeRead, 
        uint8_t* pdata )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        HRESULT         hr = S_OK;

        hr = mExec.GetPData( process, address, imageBase, size, sizeRead, pdata );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }


//----------------------------------------------------------------------------
//  Poll thread
//----------------------------------------------------------------------------

    DWORD    DebuggerProxy::DebugPollProc( void* param )
    {
        _ASSERT( param != NULL );

        DebuggerProxy*    pThis = (DebuggerProxy*) param;

        CoInitializeEx( NULL, COINIT_MULTITHREADED );
        HRESULT hr = pThis->PollLoop();
        CoUninitialize();

        return hr;
    }

    HRESULT DebuggerProxy::PollLoop()
    {
        HRESULT hr = S_OK;

        hr = mExec.Init( mCallback, this );
        if ( FAILED( hr ) )
            return hr;

        SetReadyThread();

        while ( !mShutdown )
        {
            hr = mExec.WaitForEvent( EventTimeoutMillis );
            if ( FAILED( hr ) )
            {
                if ( hr == E_HANDLE )
                {
                    // no debuggee has started yet
                    Sleep( EventTimeoutMillis );
                }
                else if ( hr != E_TIMEOUT )
                    break;
            }
            else
            {
                hr = mExec.DispatchEvent();
                if ( FAILED( hr ) )
                    break;
            }

            hr = CheckMessage();
            if ( FAILED( hr ) )
                break;
        }

        mExec.Shutdown();

        OutputDebugStringA( "Poll loop shutting down.\n" );
        return hr;
    }

    void    DebuggerProxy::SetReadyThread()
    {
        _ASSERT( mhReadyEvent != NULL );

        mWorkerTid = GetCurrentThreadId();
        SetEvent( mhReadyEvent );
    }

    HRESULT DebuggerProxy::CheckMessage()
    {
        HRESULT hr = S_OK;
        DWORD   waitRet = 0;

        waitRet = WaitForSingleObject( mhCommandEvent, CommandTimeoutMillis );
        if ( waitRet == WAIT_FAILED )
            return GetLastHr();

        if ( waitRet == WAIT_TIMEOUT )
            return S_OK;

        hr = ProcessCommand( mCurCommand );
        if ( FAILED( hr ) )
            return hr;

        ResetEvent( mhCommandEvent );
        SetEvent( mhResultEvent );

        return S_OK;
    }

    HRESULT DebuggerProxy::ProcessCommand( CommandFunctor* cmd )
    {
        _ASSERT( cmd != NULL );
        if ( cmd == NULL )
            return E_POINTER;

        cmd->Run();

        return S_OK;
    }

    void DebuggerProxy::SetSymbolSearchPath( const std::wstring& searchPath )
    {
        mSymbolSearchPath = searchPath;
    }
    const std::wstring& DebuggerProxy::GetSymbolSearchPath() const
    {
        return mSymbolSearchPath;
    }
}
