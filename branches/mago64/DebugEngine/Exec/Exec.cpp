/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Exec.h"
#include "Process.h"
#include "Thread.h"
#include "Module.h"
#include "PathResolver.h"
#include "EventCallback.h"
#include "Machine.h"
#include "MakeMachine.h"
#include "Iter.h"
#include <Psapi.h>

using namespace std;


class ProcessMap : public std::map< uint32_t, RefPtr< Process > >
{
};

class ProbeCallback : public IProbeCallback
{
    IEventCallback* mCallback;
    Process*        mProcess;

public:
    ProbeCallback( IEventCallback* callback, Process* process )
        :   mCallback( callback ),
            mProcess( process )
    {
        _ASSERT( callback != NULL );
        _ASSERT( process != NULL );
    }

    virtual ProbeRunMode OnCallProbe( 
        IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange )
    {
        mProcess->Unlock();
        ProbeRunMode mode = mCallback->OnCallProbe( process, threadId, address, thunkRange );
        mProcess->Lock();
        return mode;
    }
};


// indexed by Win32 Debug API Event Code (for ex: EXCEPTION_DEBUG_EVENT)
const IEventCallback::EventCode     gEventMap[] = 
{
    IEventCallback::Event_None,
    IEventCallback::Event_Exception,
    IEventCallback::Event_ThreadStart,
    IEventCallback::Event_ProcessStart,
    IEventCallback::Event_ThreadExit,
    IEventCallback::Event_ProcessExit,
    IEventCallback::Event_ModuleLoad,
    IEventCallback::Event_ModuleUnload,
    IEventCallback::Event_OutputString,
    IEventCallback::Event_None
};

const uint32_t  NormalTerminateCode = 0;
const uint32_t  AbnormalTerminateCode = _UI32_MAX;


Exec::Exec()
:   mTid( 0 ),
    mCallback( NULL ),
    mPathBuf( NULL ),
    mPathBufLen( 0 ),
    mProcMap( NULL ),
    mResolver( NULL ),
    mIsDispatching( false ),
    mIsShutdown( false )
{
    memset( &mLastEvent, 0, sizeof mLastEvent );
}

Exec::~Exec()
{
    if ( mTid == 0 || mTid == GetCurrentThreadId() )
        Shutdown();

    delete [] mPathBuf;
    delete mProcMap;
    delete mResolver;

    if ( mCallback != NULL )
    {
        mCallback->Release();
        mCallback = NULL;
    }
}


HRESULT Exec::Init( IEventCallback* callback )
{
    // already initialized?
    if ( mTid != 0 )
        return E_ALREADY_INIT;
    if ( callback == NULL )
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    auto_ptr<ProcessMap>   map( new ProcessMap() );
    if ( map.get() == NULL )
        return E_OUTOFMEMORY;

    auto_ptr<PathResolver>  resolver( new PathResolver() );
    if ( resolver.get() == NULL )
        return E_OUTOFMEMORY;

    hr = resolver->Init();
    if ( FAILED( hr ) )
        return hr;

    mPathBuf = new wchar_t[ MAX_PATH ];
    if ( mPathBuf == NULL )
        return E_OUTOFMEMORY;
    mPathBufLen = MAX_PATH;

    callback->AddRef();

    mCallback = callback;
    mProcMap = map.release();
    mResolver = resolver.release();

    // start off our thread affinity
    mTid = GetCurrentThreadId();

    return S_OK;
}

HRESULT Exec::Shutdown()
{
    _ASSERT( mTid == 0 || mTid == GetCurrentThreadId() );
    if ( mTid != 0 && mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    if ( mIsDispatching )
        return E_WRONG_STATE;
    if ( mIsShutdown )
        return S_OK;

    mIsShutdown = true;

    if ( mProcMap != NULL )
    {
        for ( ProcessMap::iterator it = mProcMap->begin();
            it != mProcMap->end();
            it++ )
        {
            Process*    proc = it->second.Get();

            ProcessGuard guard( proc );

            if ( !proc->IsDeleted() )
            {
                // TODO: if detaching: lock process and detach its machine, then detach process

                // we still have process running, 
                // so treat it as if we're shutting down our own app
                TerminateProcess( proc->GetHandle(), AbnormalTerminateCode );

                // we're still debugging, so stop, so the debuggee can really close
                DebugActiveProcessStop( proc->GetId() );

                proc->SetDeleted();
                proc->SetMachine( NULL );
            }
        }

        mProcMap->clear();
    }

    CleanupLastDebugEvent();

    // it would be nice to release the callback here

    return S_OK;
}


HRESULT Exec::WaitForEvent( uint32_t millisTimeout )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown || mIsDispatching )
        return E_WRONG_STATE;
    // if we haven't continued since the last wait, we'll just return E_TIMEOUT

    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;

    bRet = ::WaitForDebugEvent( &mLastEvent, millisTimeout );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

Error:
    return hr;
}

HRESULT Exec::ContinueInternal( Process* proc, bool handleException )
{
    _ASSERT( proc != NULL );
    _ASSERT( proc->IsStopped() );

    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
    DWORD   status = DBG_CONTINUE;
    ShortDebugEvent lastEvent = proc->GetLastEvent();

    // always treat the SS exception as a step complete
    // always treat the BP exception as a user BP, instead of an exception

    if ( (lastEvent.EventCode == EXCEPTION_DEBUG_EVENT) 
        && (lastEvent.ExceptionCode != EXCEPTION_BREAKPOINT)
        && (lastEvent.ExceptionCode != EXCEPTION_SINGLE_STEP) )
        status = handleException ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED;

    if ( !proc->IsDeleted() && !proc->IsTerminating() )
    {
        IMachine*   machine = proc->GetMachine();
        _ASSERT( machine != NULL );

        hr = machine->OnContinue();
        if ( FAILED( hr ) )
            goto Error;
    }

    bRet = ::ContinueDebugEvent( proc->GetId(), lastEvent.ThreadId, status );
    _ASSERT( bRet );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    proc->SetStopped( false );
    proc->ClearLastEvent();

Error:
    return hr;
}

HRESULT Exec::ContinueNoLock( Process* proc, bool handleException )
{
    _ASSERT( proc != NULL );
    _ASSERT( proc->IsStopped() );

    HRESULT     hr = S_OK;

    if ( !proc->IsDeleted() && !proc->IsTerminating() )
    {
        IMachine*   machine = proc->GetMachine();
        _ASSERT( machine != NULL );

        hr = machine->SetContinue();
        if ( FAILED( hr ) )
            goto Error;
    }

    hr = ContinueInternal( proc, handleException );

Error:
    return hr;
}

HRESULT Exec::Continue( IProcess* process, bool handleException )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown || mIsDispatching )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    Process*    proc = (Process*) process;

    ProcessGuard guard( proc );

    if ( !process->IsStopped() )
        return E_WRONG_STATE;

    hr = ContinueNoLock( proc, handleException );

    return hr;
}

HRESULT Exec::DispatchEvent()
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown || mIsDispatching )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    RefPtr<Process> proc;

    Log::LogDebugEvent( mLastEvent );

    proc = FindProcess( mLastEvent.dwProcessId );
    _ASSERT( proc != NULL );
    if ( proc == NULL )
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    {
        ProcessGuard guard( proc );

        // hand off the event
        proc->SetLastEvent( mLastEvent );
        proc->SetStopped( true );

        hr = DispatchAndContinue( proc, mLastEvent );
    }

Error:
    // if there was an error, leave the debuggee in break mode
    CleanupLastDebugEvent();
    return hr;
}

HRESULT Exec::DispatchAndContinue( Process* proc, const DEBUG_EVENT& debugEvent )
{
    HRESULT hr = S_OK;

    hr = DispatchProcessEvent( proc, debugEvent );
    if ( FAILED( hr ) )
        goto Error;

    if ( hr == S_OK )
    {
        hr = ContinueNoLock( proc, false );
    }
    else
    {
        // leave the debuggee in break mode
        hr = S_OK;
    }

Error:
    return hr;
}

    // returns S_OK: continue; S_FALSE: don't continue
HRESULT Exec::DispatchProcessEvent( Process* proc, const DEBUG_EVENT& debugEvent )
{
    _ASSERT( proc != NULL );

    HRESULT         hr = S_OK;
    // because of the lock, hold onto the process, 
    // even if EXIT_PROCESS event would have destroyed it
    RefPtr<Process> procRef = proc;
    IMachine*       machine = proc->GetMachine();

    // we shouldn't handle any stopping events after Terminate
    if ( proc->IsDeleted()
        || (proc->IsTerminating() && debugEvent.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT) )
    {
        // continue
        hr = S_OK;
        goto Error;
    }
    _ASSERT( machine != NULL );

    machine->OnStopped( debugEvent.dwThreadId );
    mIsDispatching = true;

    switch ( debugEvent.dwDebugEventCode )
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        {
            RefPtr<Module>  mod;
            RefPtr<Thread>  thread;

            hr = CreateModule( proc, debugEvent, mod );
            if ( FAILED( hr ) )
                goto Error;

            hr = CreateThread( proc, debugEvent, thread );
            if ( FAILED( hr ) )
                goto Error;

            proc->AddThread( thread.Get() );
            proc->SetEntryPoint( (Address) debugEvent.u.CreateProcessInfo.lpStartAddress );
            proc->SetStarted();

            machine->OnCreateThread( thread.Get() );

            if ( mCallback != NULL )
            {
                proc->Unlock();
                mCallback->OnProcessStart( proc );
                mCallback->OnModuleLoad( proc, mod.Get() );
                mCallback->OnThreadStart( proc, thread.Get() );
                proc->Lock();
            }
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        {
            hr = HandleCreateThread( proc, debugEvent );
        }
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        {
            if ( mCallback != NULL )
            {
                proc->Unlock();
                mCallback->OnThreadExit( proc, debugEvent.dwThreadId, debugEvent.u.ExitThread.dwExitCode );
                proc->Lock();
            }

            proc->DeleteThread( debugEvent.dwThreadId );

            hr = machine->OnExitThread( debugEvent.dwThreadId );
        }
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        {
            proc->SetDeleted();
            proc->SetMachine( NULL );

            if ( proc->IsStarted() && mCallback != NULL )
            {
                proc->Unlock();
                mCallback->OnProcessExit( proc, debugEvent.u.ExitProcess.dwExitCode );
                proc->Lock();
            }

            mProcMap->erase( debugEvent.dwProcessId );
        }
        break;

    case LOAD_DLL_DEBUG_EVENT:
        {
            RefPtr<Module>  mod;

            hr = CreateModule( proc, debugEvent, mod );
            if ( FAILED( hr ) )
                goto Error;

            if ( proc->GetOSModule() == NULL )
            {
                proc->SetOSModule( mod );
            }

            if ( mCallback != NULL )
            {
                proc->Unlock();
                mCallback->OnModuleLoad( proc, mod.Get() );
                proc->Lock();
            }
        }
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        {
            if ( mCallback != NULL )
            {
                proc->Unlock();
                mCallback->OnModuleUnload( proc, (Address) debugEvent.u.UnloadDll.lpBaseOfDll );
                proc->Lock();
            }
        }
        break;

    case EXCEPTION_DEBUG_EVENT:
        {
            hr = HandleException( proc, debugEvent );
        }
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        {
            hr = HandleOutputString( proc, debugEvent );
        }
        break;

    case RIP_EVENT:
        break;
    }

Error:
    if ( FAILED( hr ) )
    {
        _ASSERT( debugEvent.dwDebugEventCode < _countof( gEventMap ) );
        IEventCallback::EventCode   code = gEventMap[ debugEvent.dwDebugEventCode ];

        if ( mCallback != NULL )
        {
            proc->Unlock();
            mCallback->OnError( proc, hr, code );
            proc->Lock();
        }
    }

    mIsDispatching = false;

    return hr;
}

HRESULT Exec::HandleCreateThread( Process* proc, const DEBUG_EVENT& debugEvent )
{
    HRESULT         hr = S_OK;
    IMachine*       machine = proc->GetMachine();
    RefPtr<Thread>  thread;

    hr = CreateThread( proc, debugEvent, thread );
    if ( FAILED( hr ) )
        goto Error;

    proc->AddThread( thread.Get() );

    hr = machine->OnCreateThread( thread.Get() );
    if ( FAILED( hr ) )
        goto Error;

    if ( proc->GetSuspendCount() > 0 )
    {
        // if all threads are meant to be suspended, then include this one
        ThreadControlProc controlProc = machine->GetWinSuspendThreadProc();
        HANDLE hThread = debugEvent.u.CreateThread.hThread;

        for ( int i = 0; i < proc->GetSuspendCount(); i++ )
        {
            DWORD suspendCount = controlProc( hThread );
            if ( suspendCount == (DWORD) -1 )
            {
                hr = GetLastHr();
                goto Error;
            }
        }
    }

    if ( mCallback != NULL )
    {
        proc->Unlock();
        mCallback->OnThreadStart( proc, thread.Get() );
        proc->Lock();
    }

Error:
    return hr;
}

bool Exec::FoundLoaderBp( Process* proc, const DEBUG_EVENT& debugEvent )
{
    if ( !proc->ReachedLoaderBp() )
    {
        if ( debugEvent.u.Exception.ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT )
        {
            Address exceptAddr = (Address) debugEvent.u.Exception.ExceptionRecord.ExceptionAddress;
            Module* osMod = proc->GetOSModule();

            if ( osMod != NULL && osMod->Contains( exceptAddr ) )
            {
                return true;
            }
        }
    }

    return false;
}

HRESULT Exec::HandleException( Process* proc, const DEBUG_EVENT& debugEvent )
{
    HRESULT hr = S_OK;
    IMachine* machine = proc->GetMachine();

    // it doesn't matter if we launched or attached
    if ( FoundLoaderBp( proc, debugEvent ) )
    {
        proc->SetReachedLoaderBp();

        if ( mCallback != NULL )
        {
            proc->Unlock();
            mCallback->OnLoadComplete( proc, debugEvent.dwThreadId );
            proc->Lock();
        }

        hr = S_FALSE;
    }
    else
    {
        MachineResult   result = MacRes_NotHandled;
        ProbeCallback   probeCallback( mCallback, proc );
                    
        machine->SetCallback( &probeCallback );
        hr = machine->OnException( debugEvent.dwThreadId, &debugEvent.u.Exception, result );
        machine->SetCallback( NULL );
        if ( FAILED( hr ) )
            goto Error;

        if ( result == MacRes_PendingCallbackBP 
            || result == MacRes_PendingCallbackEmbeddedBP )
        {
            Address         addr = 0;
            bool            embedded = (result == MacRes_PendingCallbackEmbeddedBP);

            machine->GetPendingCallbackBP( addr );

            proc->Unlock();
            RunMode mode = mCallback->OnBreakpoint( proc, debugEvent.dwThreadId, addr, embedded );
            proc->Lock();
            if ( mode == RunMode_Run )
                result = MacRes_HandledContinue;
            else if ( mode == RunMode_Wait )
                result = MacRes_HandledStopped;
            else // Break
            {
                result = MacRes_HandledStopped;
                hr = machine->CancelStep();
                if ( FAILED( hr ) )
                    goto Error;
            }
        }
        else if ( result == MacRes_PendingCallbackStep 
            || result == MacRes_PendingCallbackEmbeddedStep )
        {
            proc->Unlock();
            mCallback->OnStepComplete( proc, debugEvent.dwThreadId );
            proc->Lock();
            result = MacRes_HandledStopped;
        }

        if ( result == MacRes_NotHandled )
        {
            hr = S_FALSE;
            if ( mCallback != NULL )
            {
                proc->Unlock();
                if ( mCallback->OnException( 
                    proc, 
                    debugEvent.dwThreadId,
                    (debugEvent.u.Exception.dwFirstChance > 0),
                    &debugEvent.u.Exception.ExceptionRecord ) == RunMode_Run )
                    hr = S_OK;
                proc->Lock();
            }
        }
        else if ( result == MacRes_HandledStopped )
        {
            hr = S_FALSE;
        }
        // else, MacRes_HandledContinue, hr == S_OK
    }

Error:
    return hr;
}

HRESULT Exec::HandleOutputString( Process* proc, const DEBUG_EVENT& debugEvent )
{
    HRESULT         hr = S_OK;
    const uint16_t  TotalLen = debugEvent.u.DebugString.nDebugStringLength;
    SIZE_T          bytesRead = 0;
    BOOL            bRet = FALSE;
    boost::scoped_array< wchar_t >  wstr( new wchar_t[ TotalLen ] );
    
    if ( wstr.get() == NULL )
        return E_OUTOFMEMORY;

    if ( debugEvent.u.DebugString.fUnicode )
    {
        bRet = ::ReadProcessMemory( 
            proc->GetHandle(), 
            debugEvent.u.DebugString.lpDebugStringData, 
            wstr.get(), 
            TotalLen * sizeof( wchar_t ), 
            &bytesRead );
        wstr[ TotalLen - 1 ] = L'\0';

        if ( !bRet )
            return GetLastHr();
    }
    else
    {
        boost::scoped_array< char >     astr( new char[ TotalLen ] );
        int     countRet = 0;

        if ( astr == NULL )
            return E_OUTOFMEMORY;

        bRet = ::ReadProcessMemory( 
            proc->GetHandle(), 
            debugEvent.u.DebugString.lpDebugStringData, 
            astr.get(), 
            TotalLen * sizeof( char ), 
            &bytesRead );
        astr[ TotalLen - 1 ] = '\0';

        if ( !bRet )
            return GetLastHr();

        countRet = MultiByteToWideChar(
            CP_ACP,
            MB_ERR_INVALID_CHARS | MB_USEGLYPHCHARS,
            astr.get(),
            -1,
            wstr.get(),
            TotalLen );

        if ( countRet == 0 )
            return GetLastHr();
    }

    if ( mCallback != NULL )
    {
        proc->Unlock();
        mCallback->OnOutputString( proc, wstr.get() );
        proc->Lock();
    }

    return hr;
}


Process*    Exec::FindProcess( uint32_t id )
{
    ProcessMap::iterator    it = mProcMap->find( id );

    if ( it == mProcMap->end() )
        return NULL;

    return it->second.Get();
}

void Exec::CleanupLastDebugEvent()
{
    if ( mLastEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT )
    {
        CloseHandle( mLastEvent.u.CreateProcessInfo.hFile );
    }
    else if ( mLastEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT )
    {
        CloseHandle( mLastEvent.u.LoadDll.hFile );
    }

    memset( &mLastEvent, 0, sizeof mLastEvent );
}

HRESULT Exec::Launch( LaunchInfo* launchInfo, IProcess*& process )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( launchInfo != NULL );
    if ( launchInfo == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT             hr = S_OK;
    wchar_t*            cmdLine = NULL;
    BOOL                bRet = FALSE;
    STARTUPINFO         startupInfo = { sizeof startupInfo };
    PROCESS_INFORMATION procInfo = { 0 };
    BOOL                inheritHandles = FALSE;
    DWORD               flags = DEBUG_ONLY_THIS_PROCESS | CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE;
    HandlePtr           hProcPtr;
    HandlePtr           hThreadPtr;
    ImageInfo           imageInfo = { 0 };
    RefPtr<IMachine>    machine;
    RefPtr<Process>     proc;

    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_SHOW;

    if ( (launchInfo->StdInput != NULL) || (launchInfo->StdOutput != NULL) || (launchInfo->StdError != NULL) )
    {
        startupInfo.hStdInput = launchInfo->StdInput;
        startupInfo.hStdOutput = launchInfo->StdOutput;
        startupInfo.hStdError = launchInfo->StdError;
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        inheritHandles = TRUE;
    }

    if ( launchInfo->NewConsole )
        flags |= CREATE_NEW_CONSOLE;
    if ( launchInfo->Suspend )
        flags |= CREATE_SUSPENDED;

    wchar_t*    pathRet = _wfullpath( mPathBuf, launchInfo->Exe, mPathBufLen );
    if ( pathRet == NULL )
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    hr = GetImageInfo( mPathBuf, imageInfo );
    if ( FAILED( hr ) )
        goto Error;

    hr = MakeMachine( imageInfo.MachineType, machine.Ref() );
    if ( FAILED( hr ) )
        goto Error;

    cmdLine = _wcsdup( launchInfo->CommandLine );
    if ( cmdLine == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    bRet = CreateProcess(
        NULL,
        cmdLine,
        NULL,
        NULL,
        inheritHandles,
        flags,
        (void*) launchInfo->EnvBstr,
        launchInfo->Dir,
        &startupInfo,
        &procInfo );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    hThreadPtr.Attach( procInfo.hThread );
    hProcPtr.Attach( procInfo.hProcess );

    proc = new Process( Create_Launch, procInfo.hProcess, procInfo.dwProcessId, mPathBuf );
    if ( proc.Get() == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hProcPtr.Detach();

    mProcMap->insert( ProcessMap::value_type( procInfo.dwProcessId, proc ) );

    machine->SetProcess( proc->GetHandle(), proc->GetId(), proc.Get() );
    proc->SetMachine( machine.Get() );

    proc->SetMachineType( imageInfo.MachineType );

    if ( launchInfo->Suspend )
        proc->SetLaunchedSuspendedThread( hThreadPtr.Detach() );

    process = proc.Detach();

Error:
    if ( FAILED( hr ) )
    {
        if ( !hProcPtr.IsEmpty() )
        {
            TerminateProcess( hProcPtr.Get(), AbnormalTerminateCode );
            DebugActiveProcessStop( procInfo.dwProcessId );
        }
    }

    if ( cmdLine != NULL )
        free( cmdLine );

    return hr;
}

HRESULT Exec::Attach( uint32_t id, IProcess*& process )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT             hr = S_OK;
    BOOL                bRet = FALSE;
    HandlePtr           hProcPtr;
    wstring             filename;
    RefPtr<Process>     proc;
    RefPtr<IMachine>    machine;
    ImageInfo           imageInfo;

    hProcPtr = OpenProcess( PROCESS_ALL_ACCESS, FALSE, id );
    if ( hProcPtr.Get() == NULL )
    {
        hr = GetLastHr();
        goto Error;
    }

    hr = mResolver->GetProcessModulePath( hProcPtr.Get(), filename );
    if ( FAILED( hr ) )
        // getting ERROR_PARTIAL_COPY here usually means we won't be able to attach
        goto Error;

    hr = GetProcessImageInfo( hProcPtr.Get(), imageInfo );
    if ( FAILED( hr ) )
        goto Error;

    hr = MakeMachine( imageInfo.MachineType, machine.Ref() );
    if ( FAILED( hr ) )
        goto Error;

    bRet = DebugActiveProcess( id );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    proc = new Process( Create_Attach, hProcPtr.Get(), id, filename.c_str() );
    if ( proc.Get() == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hProcPtr.Detach();

    mProcMap->insert( ProcessMap::value_type( id, proc ) );

    machine->SetProcess( proc->GetHandle(), proc->GetId(), proc.Get() );
    proc->SetMachine( machine.Get() );

    proc->SetMachineType( imageInfo.MachineType );

    process = proc.Detach();

Error:
    if ( FAILED( hr ) )
    {
        DebugActiveProcessStop( id );
    }

    return hr;
}

HRESULT Exec::ResumeLaunchedProcess( IProcess* process )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    Process* proc = (Process*) process;

    // lock the process, in case someone uses it when they shouldn't
    ProcessGuard guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;

    ResumeSuspendedProcess( process );

    return S_OK;
}

void Exec::ResumeSuspendedProcess( IProcess* process )
{
    Process* proc = (Process*) process;
    if ( proc->GetLaunchedSuspendedThread() != NULL )
    {
        ResumeThread( proc->GetLaunchedSuspendedThread() );
        proc->SetLaunchedSuspendedThread( NULL );
    }
}

HRESULT Exec::Terminate( IProcess* process )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    BOOL        bRet = FALSE;
    Process*    proc = (Process*) process;

    ProcessGuard guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;

    proc->SetTerminating();

    bRet = TerminateProcess( process->GetHandle(), NormalTerminateCode );
    _ASSERT( bRet );
    if ( !bRet )
    {
        hr = GetLastHr();
    }

    if ( proc->IsStarted() )
    {
        if ( process->IsStopped() )
        {
            ContinueInternal( proc, true );
        }
    }
    else
    {
        bRet = DebugActiveProcessStop( proc->GetId() );

        proc->SetDeleted();
        proc->SetMachine( NULL );

        mProcMap->erase( process->GetId() );
    }

    // the process will end even if all threads are suspended
    // we only have to detach or keep pumping its events

    return hr;
}

HRESULT Exec::Detach( IProcess* process )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;

    IMachine* machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    machine->Detach();

    // do the least needed to shut down
    proc->SetTerminating();

    if ( proc->IsStopped() )
    {
        // Throw out exceptions that are used for debugging, so that the 
        // debuggee isn't stuck with them, and likely crash.
        // Let the debuggee handle all other exceptions and events.

        ShortDebugEvent lastEvent = proc->GetLastEvent();

        if ( (lastEvent.EventCode == EXCEPTION_DEBUG_EVENT) 
            && (lastEvent.ExceptionCode == EXCEPTION_BREAKPOINT
            || lastEvent.ExceptionCode == EXCEPTION_SINGLE_STEP) )
        {
            ContinueInternal( proc, true );
        }
    }

    DebugActiveProcessStop( process->GetId() );
    ResumeSuspendedProcess( process );

    proc->SetDeleted();
    proc->SetMachine( NULL );

    if ( proc->IsStarted() && mCallback != NULL )
    {
        proc->Unlock();
        mCallback->OnProcessExit( proc, 0 );
        proc->Lock();
    }

    mProcMap->erase( process->GetId() );

    return hr;
}


    // not tied to the exec debugger thread
HRESULT Exec::ReadMemory( 
    IProcess* process, 
    Address address, 
    SIZE_T length, 
    SIZE_T& lengthRead, 
    SIZE_T& lengthUnreadable, 
    uint8_t* buffer )
{
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    hr = machine->ReadMemory( (Address) address, length, lengthRead, lengthUnreadable, buffer );

    return hr;
}


HRESULT Exec::WriteMemory( 
    IProcess* process, 
    Address address, 
    SIZE_T length, 
    SIZE_T& lengthWritten, 
    uint8_t* buffer )
{
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;
    if ( !proc->IsStopped() )
        return E_WRONG_STATE;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    hr = machine->WriteMemory( (Address) address, length, lengthWritten, buffer );

    return hr;
}


    // not tied to the wait/continue state
HRESULT Exec::SetBreakpoint( IProcess* process, Address address )
{
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );
    bool        suspend = !proc->IsStopped() && !machine->IsBreakpointActive( address );

    if ( suspend )
    {
        hr = SuspendProcess( proc, machine->GetWinSuspendThreadProc() );
        if ( FAILED( hr ) )
            goto Error;
    }

    hr = machine->SetBreakpoint( address );

    if ( suspend )
    {
        HRESULT hrResume = ResumeProcess( proc, machine->GetWinSuspendThreadProc() );
        if ( SUCCEEDED( hr ) )
            hr = hrResume;
    }

Error:
    return hr;
}

    // not tied to the wait/continue state
HRESULT Exec::RemoveBreakpoint( IProcess* process, Address address )
{
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );
    bool        suspend = !proc->IsStopped() && !machine->IsBreakpointActive( address );

    if ( suspend )
    {
        hr = SuspendProcess( proc, machine->GetWinSuspendThreadProc() );
        if ( FAILED( hr ) )
            goto Error;
    }

    hr = machine->RemoveBreakpoint( address );

    if ( suspend )
    {
        HRESULT hrResume = ResumeProcess( proc, machine->GetWinSuspendThreadProc() );
        if ( SUCCEEDED( hr ) )
            hr = hrResume;
    }

Error:
    return hr;
}


HRESULT Exec::StepOut( IProcess* process, Address address, bool handleException )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown || mIsDispatching )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;
    if ( !proc->IsStopped() )
        return E_WRONG_STATE;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    hr = machine->SetStepOut( address );
    if ( FAILED( hr ) )
        goto Error;

    hr = ContinueInternal( proc, handleException );
    if ( FAILED( hr ) )
        goto Error;

Error:
    return hr;
}

HRESULT Exec::StepInstruction( IProcess* process, bool stepIn, bool handleException )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown || mIsDispatching )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;
    if ( !proc->IsStopped() )
        return E_WRONG_STATE;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    hr = machine->SetStepInstruction( stepIn );
    if ( FAILED( hr ) )
        goto Error;

    hr = ContinueInternal( proc, handleException );
    if ( FAILED( hr ) )
        goto Error;

Error:
    return hr;
}

HRESULT Exec::StepRange( 
    IProcess* process, bool stepIn, AddressRange range, bool handleException )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown || mIsDispatching )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;
    if ( !proc->IsStopped() )
        return E_WRONG_STATE;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    hr = machine->SetStepRange( stepIn, range );
    if ( FAILED( hr ) )
        goto Error;

    hr = ContinueInternal( proc, handleException );
    if ( FAILED( hr ) )
        goto Error;

Error:
    return hr;
}

HRESULT Exec::CancelStep( IProcess* process )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown || mIsDispatching )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;
    if ( !proc->IsStopped() )
        return E_WRONG_STATE;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    hr = machine->CancelStep();

    return hr;
}

HRESULT Exec::AsyncBreak( IProcess* process )
{
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    BOOL            bRet = FALSE;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;
    if ( proc->IsStopped() )
        return E_WRONG_STATE;

    bRet = DebugBreakProcess( process->GetHandle() );
    if ( !bRet )
        return GetLastHr();

    return S_OK;
}

HRESULT Exec::GetThreadContext( IProcess* process, uint32_t threadId, void* context, uint32_t size )
{
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;
    if ( !proc->IsStopped() )
        return E_WRONG_STATE;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    hr = machine->GetThreadContext( threadId, context, size );

    return hr;
}

HRESULT Exec::SetThreadContext( IProcess* process, uint32_t threadId, const void* context, uint32_t size)
{
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT         hr = S_OK;
    Process*        proc = (Process*) process;

    ProcessGuard    guard( proc );

    if ( proc->IsDeleted() || proc->IsTerminating() )
        return E_PROCESS_ENDED;
    if ( !proc->IsStopped() )
        return E_WRONG_STATE;

    IMachine*   machine = proc->GetMachine();
    _ASSERT( machine != NULL );

    hr = machine->SetThreadContext( threadId, context, size );

    return hr;
}


HRESULT Exec::CreateModule( Process* proc, const DEBUG_EVENT& event, RefPtr<Module>& mod )
{
    _ASSERT( proc != NULL );

    HRESULT                     hr = S_OK;
    wstring                     path;
    RefPtr<Module>              newMod;
    const LOAD_DLL_DEBUG_INFO*  loadInfo = &event.u.LoadDll;
    LOAD_DLL_DEBUG_INFO         fakeLoadInfo = { 0 };
    ImageInfo                   imageInfo = { 0 };

    if ( event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT )
    {
        loadInfo = &fakeLoadInfo;
        fakeLoadInfo.lpBaseOfDll = event.u.CreateProcessInfo.lpBaseOfImage;
        fakeLoadInfo.dwDebugInfoFileOffset = event.u.CreateProcessInfo.dwDebugInfoFileOffset;
        fakeLoadInfo.nDebugInfoSize = event.u.CreateProcessInfo.nDebugInfoSize;
        fakeLoadInfo.hFile = event.u.CreateProcessInfo.hFile;
    }
    else
    {
        _ASSERT( event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT );
    }

    hr = mResolver->GetFilePath( 
        proc->GetHandle(), 
        loadInfo->hFile, 
        loadInfo->lpBaseOfDll, 
        path );
    if ( FAILED( hr ) )
        goto Error;

    hr = GetLoadedImageInfo( proc->GetHandle(), loadInfo->lpBaseOfDll, imageInfo );
    if ( FAILED( hr ) )
        goto Error;

    newMod = new Module( 
        (Address) loadInfo->lpBaseOfDll,
        imageInfo.Size,
        imageInfo.MachineType,
        path.c_str(),
        loadInfo->dwDebugInfoFileOffset,
        loadInfo->nDebugInfoSize );
    if ( newMod == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    mod = newMod;
    mod->SetPreferredImageBase( imageInfo.PrefImageBase );

Error:
    return hr;
}

HRESULT Exec::CreateThread( Process* proc, const DEBUG_EVENT& event, RefPtr<Thread>& thread )
{
    _ASSERT( proc != NULL );
    UNREFERENCED_PARAMETER( proc );

    HRESULT                     hr = S_OK;
    RefPtr<Thread>              newThread;
    const CREATE_THREAD_DEBUG_INFO*  createInfo = &event.u.CreateThread;
    CREATE_THREAD_DEBUG_INFO         fakeCreateInfo = { 0 };

    if ( event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT )
    {
        createInfo = &fakeCreateInfo;
        fakeCreateInfo.hThread = event.u.CreateProcessInfo.hThread;
        fakeCreateInfo.lpThreadLocalBase = event.u.CreateProcessInfo.lpThreadLocalBase;
        fakeCreateInfo.lpStartAddress = event.u.CreateProcessInfo.lpStartAddress;
    }
    else
    {
        _ASSERT( event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT );
    }

    HandlePtr   hThreadPtr;
    HANDLE      hCurProc = GetCurrentProcess();
    BOOL        bRet = DuplicateHandle( 
        hCurProc, 
        createInfo->hThread, 
        hCurProc, 
        &hThreadPtr.Ref(), 
        0, 
        FALSE, 
        DUPLICATE_SAME_ACCESS );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    newThread = new Thread( 
        hThreadPtr.Get(),
        event.dwThreadId,
        (Address) createInfo->lpStartAddress,
        (Address) createInfo->lpThreadLocalBase );
    if ( newThread == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    hThreadPtr.Detach();
    thread = newThread;

Error:
    return hr;
}
