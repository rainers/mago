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

            // we still have process running, 
            // so treat it as if we're shutting down our own app
            TerminateProcess( proc->GetHandle(), AbnormalTerminateCode );

            // we're still debugging, so stop, so the debuggee can really close
            DebugActiveProcessStop( proc->GetId() );
        }

        mProcMap->clear();
    }

    CleanupLastDebugEvent();

    if ( mCallback != NULL )
    {
        mCallback->Release();
        mCallback = NULL;
    }

    return S_OK;
}


HRESULT Exec::WaitForDebug( uint32_t millisTimeout )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
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

HRESULT Exec::ContinueDebug( bool handleException )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown || mIsDispatching )
        return E_WRONG_STATE;

    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
    DWORD   status = handleException ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED;

    Process*    proc = FindProcess( mLastEvent.dwProcessId );
    if ( (proc != NULL) && !proc->IsTerminating() )
    {
        IMachine*   machine = proc->GetMachine();
        _ASSERT( machine != NULL );

        hr = machine->OnContinue();

        if ( FAILED( hr ) )
            goto Error;
    }

    // always treat the SS exception as a step complete
    // always treat the BP exception as a user BP, instead of an exception

    if ( mLastEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT 
        && ((mLastEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
        || (mLastEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP)) )
        status = DBG_CONTINUE;

    bRet = ::ContinueDebugEvent( mLastEvent.dwProcessId, mLastEvent.dwThreadId, status );
    _ASSERT( bRet );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }
    
    CleanupLastDebugEvent();

    if ( proc != NULL )
        proc->SetStopped( false );

Error:
    return hr;
}

    // returns S_OK: continue; S_FALSE: don't continue
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

    HRESULT     hr = S_OK;
    Process*    proc = NULL;

    Log::LogDebugEvent( mLastEvent );

    proc = FindProcess( mLastEvent.dwProcessId );
    _ASSERT( proc != NULL );
    if ( proc == NULL )
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    proc->SetStopped( true );

    IMachine*   machine = proc->GetMachine();
    _ASSERT( (mLastEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) || (machine != NULL) );

    if ( machine != NULL )
    {
        machine->OnStopped( mLastEvent.dwThreadId );
    }

    mIsDispatching = true;

    switch ( mLastEvent.dwDebugEventCode )
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        {
            RefPtr<Module>  mod;
            RefPtr<Thread>  thread;

            hr = CreateModule( proc, mLastEvent, mod );
            if ( FAILED( hr ) )
                goto Error;

            hr = CreateThread( proc, mLastEvent, thread );
            if ( FAILED( hr ) )
                goto Error;

            proc->AddThread( thread.Get() );
            proc->SetEntryPoint( (Address) mLastEvent.u.CreateProcessInfo.lpStartAddress );

            machine->OnCreateThread( thread.Get() );

            if ( mCallback != NULL )
            {
                mCallback->OnProcessStart( proc );
                mCallback->OnModuleLoad( proc, mod.Get() );
                mCallback->OnThreadStart( proc, thread.Get() );
            }
        }
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        {
            RefPtr<Thread>  thread;

            hr = CreateThread( proc, mLastEvent, thread );
            if ( FAILED( hr ) )
                goto Error;

            proc->AddThread( thread.Get() );

            machine->OnCreateThread( thread.Get() );

            if ( mCallback != NULL )
            {
                mCallback->OnThreadStart( proc, thread.Get() );
            }
        }
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        {
            if ( mCallback != NULL )
                mCallback->OnThreadExit( proc, mLastEvent.dwThreadId, mLastEvent.u.ExitThread.dwExitCode );

            proc->DeleteThread( mLastEvent.dwThreadId );

            machine->OnExitThread( mLastEvent.dwThreadId );
        }
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        {
            proc->SetDeleted();

            if ( mCallback != NULL )
                mCallback->OnProcessExit( proc, mLastEvent.u.ExitProcess.dwExitCode );

            mProcMap->erase( mLastEvent.dwProcessId );
        }
        break;

    case LOAD_DLL_DEBUG_EVENT:
        {
            RefPtr<Module>  mod;

            hr = CreateModule( proc, mLastEvent, mod );
            if ( FAILED( hr ) )
                goto Error;

            if ( mCallback != NULL )
            {
                mCallback->OnModuleLoad( proc, mod.Get() );
            }
        }
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        {
            if ( mCallback != NULL )
                mCallback->OnModuleUnload( proc, (Address) mLastEvent.u.UnloadDll.lpBaseOfDll );
        }
        break;

    case EXCEPTION_DEBUG_EVENT:
        {
            // we shouldn't handle any stopping events after Terminate
            if ( proc->IsTerminating() )
                break;

            // it doesn't matter if we launched or attached
            if ( !proc->ReachedLoaderBp() )
            {
                proc->SetReachedLoaderBp();

                if ( mCallback != NULL )
                    mCallback->OnLoadComplete( proc, mLastEvent.dwThreadId );

                hr = S_FALSE;
            }
            else
            {
                MachineResult    result = MacRes_NotHandled;
                    
                hr = machine->OnException( mLastEvent.dwThreadId, &mLastEvent.u.Exception, result );
                if ( FAILED( hr ) )
                    goto Error;

                if ( result == MacRes_PendingCallbackBP )
                {
                    MachineAddress  addr = 0;
                    int             count = 0;
                    BPCookie*       cookies = NULL;

                    machine->GetPendingCallbackBP( addr, count, cookies );
                    IterEnum<BPCookie, BPCookie*> en( cookies, cookies + count, count );

                    if ( mCallback->OnBreakpoint( proc, mLastEvent.dwThreadId, addr, &en) == RunMode_Run)
                        result = MacRes_HandledContinue;
                    else
                        result = MacRes_HandledStopped;
                }
                else if ( result == MacRes_PendingCallbackStep )
                {
                    mCallback->OnStepComplete( proc, mLastEvent.dwThreadId );
                    result = MacRes_HandledStopped;
                }

                if ( result == MacRes_NotHandled )
                {
                    hr = S_FALSE;
                    if ( mCallback != NULL )
                    {
                        if ( mCallback->OnException( 
                            proc, 
                            mLastEvent.dwThreadId,
                            (mLastEvent.u.Exception.dwFirstChance > 0),
                            &mLastEvent.u.Exception.ExceptionRecord ) == RunMode_Run )
                            hr = S_OK;
                    }
                }
                else if ( result == MacRes_HandledStopped )
                {
                    hr = S_FALSE;
                }
            }
        }
        break;

    case OUTPUT_DEBUG_STRING_EVENT:
        {
            hr = HandleOutputString( proc );
        }
        break;

    case RIP_EVENT:
        break;
    }

Error:
    if ( FAILED( hr ) )
    {
        _ASSERT( mLastEvent.dwDebugEventCode < _countof( gEventMap ) );
        IEventCallback::EventCode   code = gEventMap[ mLastEvent.dwDebugEventCode ];

        if ( mCallback != NULL )
            mCallback->OnError( proc, hr, code );
    }

    mIsDispatching = false;

    return hr;
}

HRESULT Exec::HandleOutputString( Process* proc )
{
    HRESULT         hr = S_OK;
    const uint16_t  TotalLen = mLastEvent.u.DebugString.nDebugStringLength;
    SIZE_T          bytesRead = 0;
    BOOL            bRet = FALSE;
    boost::scoped_array< wchar_t >  wstr( new wchar_t[ TotalLen ] );
    
    if ( wstr.get() == NULL )
        return E_OUTOFMEMORY;

    if ( mLastEvent.u.DebugString.fUnicode )
    {
        bRet = ::ReadProcessMemory( 
            proc->GetHandle(), 
            mLastEvent.u.DebugString.lpDebugStringData, 
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
            mLastEvent.u.DebugString.lpDebugStringData, 
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
        mCallback->OnOutputString( proc, wstr.get() );
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
    machine->SetCallback( mCallback );
    proc->SetMachine( machine.Get() );

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

            if ( launchInfo->Suspend && !hThreadPtr.IsEmpty() )
                ResumeThread( hThreadPtr.Get() );
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

    HRESULT         hr = S_OK;
    BOOL            bRet = FALSE;
    HandlePtr       hProcPtr;
    wstring         filename;

    hProcPtr = OpenProcess( PROCESS_ALL_ACCESS, FALSE, id );
    if ( hProcPtr.Get() == NULL )
    {
        hr = GetLastHr();
        goto Error;
    }

    hr = mResolver->GetModulePath( hProcPtr.Get(), NULL, filename );
    if ( FAILED( hr ) )
        // getting ERROR_PARTIAL_COPY here usually means we won't be able to attach
        goto Error;

    bRet = DebugActiveProcess( id );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    {
        RefPtr<Process> proc = new Process( Create_Attach, hProcPtr.Get(), id, filename.c_str() );

        if ( proc.Get() == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        hProcPtr.Detach();

        mProcMap->insert( ProcessMap::value_type( id, proc ) );

        process = proc.Detach();
    }

Error:
    if ( FAILED( hr ) )
    {
        DebugActiveProcessStop( id );
    }

    return hr;
}

HRESULT Exec::ResumeProcess( IProcess* process )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

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

HRESULT Exec::TerminateNewProcess( IProcess* process )
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

    proc->SetTerminating();

    TerminateProcess( process->GetHandle(), NormalTerminateCode );
    DebugActiveProcessStop( process->GetId() );
    // TODO: since we're terminating the process, we might not need this
    ResumeSuspendedProcess( process );

    mProcMap->erase( process->GetId() );

    return S_OK;
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

    proc->SetTerminating();

    bRet = TerminateProcess( process->GetHandle(), NormalTerminateCode );
    _ASSERT( bRet );
    if ( !bRet )
    {
        hr = GetLastHr();
    }

    if ( process->IsStopped() )
    {
        ContinueDebug( true );
    }

    // TODO: since we're terminating the process, we might not need this
    ResumeSuspendedProcess( process );

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

    HRESULT hr = S_OK;

#if 0
    // TODO: any cleanup that needs to be done

    DebugActiveProcessStop( process->GetId() );

    ResumeSuspendedProcess( process );

    process->SetDeleted();

    if ( mCallback != NULL )
        mCallback->OnProcessExit( process, 0 );

    mProcMap->erase( process->GetId() );
#else
    hr = E_NOTIMPL;
#endif

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
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->ReadMemory( (MachineAddress) address, length, lengthRead, lengthUnreadable, buffer );

    return hr;
}


HRESULT Exec::WriteMemory( 
    IProcess* process, 
    Address address, 
    SIZE_T length, 
    SIZE_T& lengthWritten, 
    uint8_t* buffer )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->WriteMemory( (MachineAddress) address, length, lengthWritten, buffer );

    return hr;
}


    // not tied to the wait/continue state
HRESULT Exec::SetBreakpoint( IProcess* process, Address address, BPCookie cookie )
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
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->SetBreakpoint( (MachineAddress) address, cookie );

    return hr;
}

    // not tied to the wait/continue state
HRESULT Exec::RemoveBreakpoint( IProcess* process, Address address, BPCookie cookie )
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
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->RemoveBreakpoint( (MachineAddress) address, cookie );

    return hr;
}


HRESULT Exec::StepOut( IProcess* process, Address address )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->SetStepOut( address );
    if ( FAILED( hr ) )
        goto Error;

Error:
    return hr;
}

HRESULT Exec::StepInstruction( IProcess* process, bool stepIn, bool sourceMode )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->SetStepInstruction( stepIn, sourceMode );
    if ( FAILED( hr ) )
        goto Error;

Error:
    return hr;
}

HRESULT Exec::StepRange( IProcess* process, bool stepIn, bool sourceMode, AddressRange* ranges, int rangeCount )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->SetStepRange( stepIn, sourceMode, ranges, rangeCount );
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
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->CancelStep();

    return hr;
}

HRESULT Exec::AsyncBreak( IProcess* process )
{
    _ASSERT( mTid == GetCurrentThreadId() );
    if ( mTid != GetCurrentThreadId() )
        return E_WRONG_THREAD;
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    BOOL    bRet = FALSE;

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
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    IMachine*   machine = ((Process*) process)->GetMachine();

    _ASSERT( machine != NULL );
    hr = machine->GetThreadContext( threadId, context, size );

    return hr;
}

HRESULT Exec::SetThreadContext( IProcess* process, uint32_t threadId, const void* context, uint32_t size)
{
    _ASSERT( process != NULL );
    if ( process == NULL )
        return E_INVALIDARG;
    _ASSERT( mLastEvent.dwDebugEventCode != NO_DEBUG_EVENT );
    if ( mLastEvent.dwDebugEventCode == NO_DEBUG_EVENT )
        return E_UNEXPECTED;
    if ( mIsShutdown )
        return E_WRONG_STATE;

    HRESULT     hr = S_OK;
    IMachine*   machine = ((Process*) process)->GetMachine();

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

    uint16_t    machine = 0;
    uint32_t    size = 0;
    Address     prefImageBase = 0;
    
    hr = GetImageInfoFromPEHeader( proc->GetHandle(), loadInfo->lpBaseOfDll, machine, size, prefImageBase );
    if ( FAILED( hr ) )
        goto Error;

    newMod = new Module( 
        (Address) loadInfo->lpBaseOfDll,
        size,
        machine,
        path.c_str(),
        loadInfo->dwDebugInfoFileOffset,
        loadInfo->nDebugInfoSize );
    if ( newMod == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    mod = newMod;
    mod->SetPreferredImageBase( prefImageBase );

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
