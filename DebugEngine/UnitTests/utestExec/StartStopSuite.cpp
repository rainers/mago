/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "stdafx.h"
#include "StartStopSuite.h"
#include "EventCallbackBase.h"
#include "MultiEventCallbackBase.h"

using namespace std;


struct CheckWindowInfo
{
    DWORD           ProcId;
    HWND            DebuggeeHwnd;
};

const DWORD DefaultTimeoutMillis = 500;
const DWORD DefaultIOTimeoutMillis = 5 * 1000;


StartStopSuite::StartStopSuite()
{
    TEST_ADD( StartStopSuite::TestInit );
    TEST_ADD( StartStopSuite::TestLaunchDestroyExecStopped );
    TEST_ADD( StartStopSuite::TestLaunchDestroyExecRunning );
    TEST_ADD( StartStopSuite::TestTerminateStopped );
    TEST_ADD( StartStopSuite::TestTerminateRunning );
    TEST_ADD( StartStopSuite::TestBeginToEnd );
    TEST_ADD( StartStopSuite::TestOptionsSameConsole );
    TEST_ADD( StartStopSuite::TestOptionsNewConsole );
    TEST_ADD( StartStopSuite::TestDetachRunning );
    TEST_ADD( StartStopSuite::TestDetachStopped );
    TEST_ADD( StartStopSuite::TestAttach );
    TEST_ADD( StartStopSuite::TestAsyncBreak );
    TEST_ADD( StartStopSuite::TestMultiProcess );
}

void StartStopSuite::setup()
{
    mExec = new Exec();
    mCallback = new EventCallbackBase();

    mCallback->AddRef();
}

void StartStopSuite::tear_down()
{
    if ( mExec != NULL )
    {
        delete mExec;
        mExec = NULL;
    }

    if ( mCallback != NULL )
    {
        mCallback->Release();
        mCallback = NULL;
    }
}

void StartStopSuite::TestInit()
{
    TEST_ASSERT( FAILED( mExec->Init( NULL ) ) );
    TEST_ASSERT( SUCCEEDED( mExec->Init( mCallback ) ) );
}

void StartStopSuite::TestLaunchDestroyExecStopped()
{
    uint32_t    pid = 0;

    {
        Exec    exec;

        TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

        LaunchInfo  info = { 0 };
        wchar_t     cmdLine[ MAX_PATH ] = L"";
        RefPtr<IProcess>   proc;

        swprintf_s( cmdLine, L"\"%s\"", SimplestDebuggee );

        info.CommandLine = cmdLine;
        info.Exe = SimplestDebuggee;

        TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc.Ref() ) ) );

        pid = proc->GetId();

        bool        sawLoadCompleted = false;

        for ( ; !mCallback->GetProcessExited(); )
        {
            HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

            TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
            TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

            if ( proc->IsStopped() )
            {
                if ( !sawLoadCompleted && mCallback->GetLoadCompleted() )
                {
                    sawLoadCompleted = true;
                    break;
                }
                else
                {
                    TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, true ) ) );
                }
            }
        }

        TEST_ASSERT( !mCallback->GetProcessExited() );
    }

    TEST_ASSERT( pid != 0 );

    AssertProcessFinished( pid );
}

void StartStopSuite::TestLaunchDestroyExecRunning()
{
    uint32_t    pid = 0;

    {
        Exec    exec;

        TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

        LaunchInfo  info = { 0 };
        wchar_t     cmdLine[ MAX_PATH ] = L"";
        RefPtr<IProcess>   proc;

        swprintf_s( cmdLine, L"\"%s\"", SleepingDebuggee );

        info.CommandLine = cmdLine;
        info.Exe = SimplestDebuggee;

        TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc.Ref() ) ) );
        pid = proc->GetId();

        bool        sawLoadCompleted = false;

        for ( ; !mCallback->GetProcessExited(); )
        {
            HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

            if ( (hr == E_TIMEOUT) && sawLoadCompleted )
                break;

            TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
            TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

            if ( proc->IsStopped() )
            {
                if ( !sawLoadCompleted && mCallback->GetLoadCompleted() )
                {
                    sawLoadCompleted = true;
                }

                TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, true ) ) );
            }
        }

        TEST_ASSERT( !mCallback->GetProcessExited() );
    }

    TEST_ASSERT( pid != 0 );

    AssertProcessFinished( pid );
}

void StartStopSuite::TestBeginToEnd()
{
    Exec    exec;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo  info = { 0 };
    wchar_t     cmdLine[ MAX_PATH ] = L"";
    RefPtr<IProcess>   proc;

    swprintf_s( cmdLine, L"\"%s\"", SimplestDebuggee );

    info.CommandLine = cmdLine;
    info.Exe = SimplestDebuggee;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc.Ref() ) ) );

    uint32_t    pid = proc->GetId();

    for ( int i = 0; !mCallback->GetProcessExited(); i++ )
    {
        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( proc->IsStopped() )
            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, true ) ) );
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );
    TEST_ASSERT( mCallback->GetProcessExitCode() == 0 );

    AssertProcessFinished( pid );
}

void StartStopSuite::TestTerminateStopped()
{
    Exec    exec;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo  info = { 0 };
    wchar_t     cmdLine[ MAX_PATH ] = L"";
    RefPtr<IProcess>    proc;

    swprintf_s( cmdLine, L"\"%s\"", SleepingDebuggee );

    info.CommandLine = cmdLine;
    info.Exe = SleepingDebuggee;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc.Ref() ) ) );

    bool        sawLoadCompleted = false;
    uint32_t    pid = proc->GetId();

    for ( ; !mCallback->GetProcessExited(); )
    {
        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( proc->IsStopped() )
        {
            if ( !sawLoadCompleted && mCallback->GetLoadCompleted() )
            {
                sawLoadCompleted = true;

                TEST_ASSERT_RETURN( SUCCEEDED( exec.Terminate( proc.Get() ) ) );
            }
            else
            {
                TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, true ) ) );
            }
        }
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );
    TEST_ASSERT( mCallback->GetProcessExitCode() == 0 );

    AssertProcessFinished( pid );
}

void StartStopSuite::TestTerminateRunning()
{
    Exec    exec;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo  info = { 0 };
    wchar_t     cmdLine[ MAX_PATH ] = L"";
    RefPtr<IProcess>    proc;

    swprintf_s( cmdLine, L"\"%s\"", SleepingDebuggee );

    info.CommandLine = cmdLine;
    info.Exe = SleepingDebuggee;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc.Ref() ) ) );

    uint32_t    pid = proc->GetId();

    for ( ; !mCallback->GetProcessExited(); )
    {
        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

        // debuggee is in the middle of the sleep
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( proc->IsStopped() )
            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, true ) ) );
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( !mCallback->GetProcessExited() );

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Terminate( proc.Get() ) ) );
    for ( int i = 0; i < 10 && !mCallback->GetProcessExited(); i++ )
    {
        TEST_ASSERT_RETURN( SUCCEEDED( exec.WaitForEvent( DefaultTimeoutMillis ) ) );
        // this should dispatch EXIT_PROCESS
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );
        if ( proc->IsStopped() )
            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, false ) ) );
    }
    TEST_ASSERT( mCallback->GetProcessExited() );
    TEST_ASSERT( mCallback->GetProcessExitCode() == 0 );

    AssertProcessFinished( pid );
}

void StartStopSuite::TestOptionsSameConsole()
{
    TryOptions( false );
}

void StartStopSuite::TestOptionsNewConsole()
{
    TryOptions( true );
}

void StartStopSuite::TestDetachRunning()
{
    TestDetachCore( true );
}

void StartStopSuite::TestDetachStopped()
{
    TestDetachCore( false );
}

void StartStopSuite::TestDetachCore( bool detachWhileRunning )
{
    enum State
    {
        State_Init,
        State_SetBP,
        State_Stepped,
        State_Done
    };

    Exec    exec;
    State   state = State_Init;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo      info = { 0 };
    wchar_t         cmdLine[ MAX_PATH ] = L"";
    IProcess*       proc = NULL;
    const wchar_t*  Debuggee = EventsDebuggee;
    wchar_t         eventName[256] = L"";
    const wchar_t*  runningPart = detachWhileRunning ? L"Running" : L"Stopped";

    swprintf_s( eventName, L"utestExec_detach%s-%d", runningPart, GetCurrentProcessId() );

    HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, eventName );
    TEST_ASSERT_RETURN( hEvent != NULL );

    swprintf_s( cmdLine, L"\"%s\" detach 1 %s", Debuggee, eventName );

    info.CommandLine = cmdLine;
    info.Exe = Debuggee;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc ) ) );

    uint32_t    pid = proc->GetId();
    RefPtr<IProcess>    process( proc );

    proc->Release();
    mCallback->SetTrackLastEvent( true );

    for ( int i = 0; !mCallback->GetProcessExited(); i++ )
    {
        bool    handled = true;

        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( state == State_Done )
        {
            TEST_FAIL( "Got an event after detaching." );
        }

        if ( process->IsStopped() )
        {
            if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_LoadComplete) )
            {
                hr = exec.SetBreakpoint( process, 0x0041187B );
                TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
                state = State_SetBP;
            }
            else if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_Breakpoint) )
            {
                if ( state == State_SetBP )
                {
                    hr = exec.StepInstruction( process, true, false );
                    TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
                    state = State_Stepped;
                }
                else
                {
                    TEST_FAIL( "Got an unexpected breakpoint." );
                }
            }
            else if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_StepComplete) )
            {
                if ( state == State_Stepped )
                {
                    if ( detachWhileRunning )
                    {
                        hr = exec.Continue( process, false );
                        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
                    }
                    hr = exec.Detach( process );
                    TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
                    state = State_Done;
                }
                else
                {
                    TEST_FAIL( "Got an unexpected step." );
                }
            }

            if ( state != State_Done && state != State_Stepped )
                TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, handled ) ) );
        }
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );

    TEST_ASSERT( state == State_Done );

    // assert that it's still running
    BOOL waitRet = WaitForSingleObject( process->GetHandle(), 0 );
    TEST_ASSERT( waitRet == WAIT_TIMEOUT );

    // Let it run. Then wait a little for it to end.
    SetEvent( hEvent );
    AssertProcessFinished( process->GetId(), 13 );
}

void StartStopSuite::TestAttach()
{
    enum State
    {
        State_Init,
        State_Done
    };

    Exec    exec;
    State   state = State_Init;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    wchar_t         cmdLine[ MAX_PATH ] = L"";
    IProcess*       proc = NULL;
    const wchar_t*  Debuggee = EventsDebuggee;
    BOOL            bRet = FALSE;
    UUID            uuid = { 0 };

    CoCreateGuid( &uuid );
    unsigned int    cookie1 = uuid.Data1;
    unsigned int    cookie2 = (uuid.Data2 | (uuid.Data3 << 16)) ^ GetCurrentProcessId();

    swprintf_s( cmdLine, L"\"%s\" attach 1 %d", Debuggee, cookie1 );

    STARTUPINFO startupInfo = { 0 };
    PROCESS_INFORMATION procInfo = { 0 };
    startupInfo.cb = sizeof startupInfo;
    bRet = CreateProcess(
        Debuggee,
        cmdLine,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &startupInfo,
        &procInfo );
    TEST_ASSERT_RETURN_MSG( bRet, "Couldn't start the debuggee." );

    CloseHandle( procInfo.hProcess );
    CloseHandle( procInfo.hThread );
    procInfo.hProcess = NULL;
    procInfo.hThread = NULL;

    // It seems that you can't attach to the process right after it starts up.
    // So, let it tell us when it's ready.
    wchar_t eventName[64] = L"";
    swprintf_s( eventName, L"utestExec_attach-%d", procInfo.dwProcessId );
    HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, eventName );
    WaitForSingleObject( hEvent, 5 * 1000 );

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Attach( procInfo.dwProcessId, proc ) ) );

    RefPtr<IProcess>    process( proc );

    proc->Release();
    mCallback->SetTrackLastEvent( true );

    for ( int i = 0; !mCallback->GetProcessExited(); i++ )
    {
        bool    handled = true;

        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( process->IsStopped() )
        {
            if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_Breakpoint) )
            {
                if ( state == State_Init )
                {
                    // it should be an embedded BP
                    CONTEXT context = { 0 };

                    hr = exec.GetThreadContext( 
                        process.Get(), 
                        mCallback->GetLastEvent()->ThreadId, 
                        CONTEXT_INTEGER,
                        0,
                        &context,
                        sizeof context );
                    TEST_ASSERT_RETURN( SUCCEEDED( hr ) );

                    context.Eax = cookie2;
                    hr = exec.SetThreadContext( 
                        process.Get(), 
                        mCallback->GetLastEvent()->ThreadId, 
                        &context,
                        sizeof context );
                    TEST_ASSERT_RETURN( SUCCEEDED( hr ) );

                    state = State_Done;
                }
                else
                {
                    TEST_FAIL( "Got an unexpected breakpoint." );
                }
            }

            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, handled ) ) );
        }
    }

    // When attaching, Windows fakes a loader BP in a dedicated thread.
    // But, another thread can throw an exception first. 
    // Furthermore, the process could end before reaching the loader BP.
    // None of this can happen when launching the process.
    // So, don't assert LoadCompleted.
    TEST_ASSERT( mCallback->GetProcessExited() );

    TEST_ASSERT( state == State_Done );

    uint32_t expectedExitCode = cookie1 ^ cookie2;
    AssertProcessFinished( process->GetId(), expectedExitCode );
}

void StartStopSuite::BuildEnv( wchar_t* env, int envSize, char* expectedEnv, int expectedEnvSize, char* requestedEnv, int requestedEnvSize )
{
    // there has to be room for a character and the terminator for 3 strings and the environment's terminator
    _ASSERT( envSize > 2*3 );

    int                 envNoTermSize = envSize - 1;    // leave room for env's terminator
    int                 count = 0;
    int                 pos = 0;
    GUID                guid = { 0 };
    wchar_t             guidStr[ 38 + 1 ] = L"";
    FILETIME            curFileTime = { 0 };

    // get values we'll use for the environment
    TEST_ASSERT( SUCCEEDED( CoCreateGuid( &guid ) ) );
    TEST_ASSERT( StringFromGUID2( guid, guidStr, _countof( guidStr ) ) > 0 );

    GetSystemTimeAsFileTime( &curFileTime );

    // ready the environmnent

    count = swprintf_s( &env[pos], envNoTermSize - pos, L"myGuid1=%s", guidStr );
    TEST_ASSERT_RETURN( count > 0 );
    pos += count + 1;   // including terminator

    count = swprintf_s( &env[pos], envNoTermSize - pos, L"ourTime2=%08x,%08x", curFileTime.dwHighDateTime, curFileTime.dwLowDateTime );
    TEST_ASSERT_RETURN( count > 0 );
    pos += count + 1;   // including terminator

    count = swprintf_s( &env[pos], envNoTermSize - pos, L"theString3=hello yo!" );
    TEST_ASSERT_RETURN( count > 0 );
    pos += count + 1;   // including terminator

    env[pos] = L'\0';   // put the environment terminator

    // debuggee will write these to the output stream in order: 2, 3, 1
    sprintf_s( expectedEnv, expectedEnvSize, "%08x,%08x\r\nhello yo!\r\n%ls\r\n", curFileTime.dwHighDateTime, curFileTime.dwLowDateTime, guidStr );

    strcpy_s( requestedEnv, requestedEnvSize, "ourTime2\ntheString3\nmyGuid1\n" );
}

void StartStopSuite::MakeStdPipes( HandlePtr& inFileRead, HandlePtr& inFileWrite, 
                                  HandlePtr& outFileRead, HandlePtr& outFileWrite, 
                                  HandlePtr& errFileRead, HandlePtr& errFileWrite, bool& ok )
{
    // so we don't reuse other tests' pipe names
    static uint32_t seqNum = 0;
    wchar_t pipeName[ 256 ] = L"";
    SECURITY_ATTRIBUTES secAttr = { sizeof secAttr };

    seqNum++;
    ok = false;
    secAttr.bInheritHandle = TRUE;
    
    swprintf_s( pipeName, L"\\\\.\\pipe\\utestExec-in-%d-%u", GetCurrentProcessId(), seqNum );
    TEST_ASSERT_RETURN( 
        CreateAnonymousPipe( &inFileRead.Ref(), &inFileWrite.Ref(), &secAttr, NULL, 0, pipeName, false, true ) );
    
    swprintf_s( pipeName, L"\\\\.\\pipe\\utestExec-out-%d-%u", GetCurrentProcessId(), seqNum );
    TEST_ASSERT_RETURN( 
        CreateAnonymousPipe( &outFileRead.Ref(), &outFileWrite.Ref(), NULL, &secAttr, 0, pipeName, true, false ) );
    
    swprintf_s( pipeName, L"\\\\.\\pipe\\utestExec-err-%d-%u", GetCurrentProcessId(), seqNum );
    TEST_ASSERT_RETURN( 
        CreateAnonymousPipe( &errFileRead.Ref(), &errFileWrite.Ref(), NULL, &secAttr, 0, pipeName, true, false ) );

    ok = true;
}

void StartStopSuite::TryOptions( bool newConsole )
{
    Exec    exec;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo          info = { 0 };
    wchar_t             cmdLine[ MAX_PATH ] = L"";
    RefPtr<IProcess>    proc;
    HandlePtr           inFileRead;
    HandlePtr           outFileRead;
    HandlePtr           errFileRead;
    HandlePtr           inFileWrite;
    HandlePtr           outFileWrite;
    HandlePtr           errFileWrite;
    wchar_t             dir[ MAX_PATH ] = L"";
    wchar_t             env[ 512 ] = L"";
    OVERLAPPED          readErrOverlapped = { 0 };
    OVERLAPPED          readOutOverlapped = { 0 };
    OVERLAPPED          writeOverlapped = { 0 };
    HandlePtr           readErrEvent( CreateEvent( NULL, TRUE, FALSE, NULL ) );
    HandlePtr           readOutEvent( CreateEvent( NULL, TRUE, FALSE, NULL ) );
    HandlePtr           writeEvent( CreateEvent( NULL, TRUE, FALSE, NULL ) );
    char                expectedEnv[ 256 ] = "";
    char                requestedEnv[ 256 ] = "";
    bool                ok = false;

    // use an unlikely folder
    TEST_ASSERT_RETURN( GetSystemDirectory( dir, _countof( dir ) ) > 0 );

    // debuggee will write these to the error stream in order: 1, 2, 3
    // the C standard streams in debuggee are in ANSI mode, so use char[] here
    const char          ExpectedArgs[] = "is_easy1\r\n_2gamma\r\n3+3\r\n";
    swprintf_s( cmdLine, L"\"%s\" _2gamma \"3+3\" is_easy1", OptionsDebuggee );

    char expectedArgsAndCurDir[ MAX_PATH ] = "";
    sprintf_s( expectedArgsAndCurDir, "%s%ls\r\n", ExpectedArgs, dir );

    BuildEnv( env, _countof( env ), expectedEnv, _countof( expectedEnv ), requestedEnv, _countof( requestedEnv ) );

    MakeStdPipes( inFileRead, inFileWrite, outFileRead, outFileWrite, errFileRead, errFileWrite, ok );
    TEST_ASSERT_RETURN_MSG( ok, "Couldn't make pipes" );

    info.CommandLine = cmdLine;
    info.Exe = OptionsDebuggee;
    info.NewConsole = newConsole;
    info.Dir = dir;
    info.EnvBstr = env;
    info.StdInput = inFileRead.Get();
    info.StdOutput = outFileWrite.Get();
    info.StdError = errFileWrite.Get();

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc.Ref() ) ) );

    char        response[ 256 ] = "";
    char        outResponse[ 256 ] = "";
    DWORD       bytesRead = 0;
    DWORD       bytesWritten = 0;
    DWORD       waitRet = 0;
    BOOL        bRet = FALSE;
    bool        sawLoadComplete = false;

    readErrOverlapped.hEvent = readErrEvent.Get();
    readOutOverlapped.hEvent = readOutEvent.Get();
    writeOverlapped.hEvent = writeEvent.Get();

    TEST_ASSERT( !ReadFile( errFileRead.Get(), response, (DWORD) strlen( expectedArgsAndCurDir ), &bytesRead, &readErrOverlapped ) );
    TEST_ASSERT( GetLastError() == ERROR_IO_PENDING );

    TEST_ASSERT( !WriteFile( inFileWrite.Get(), requestedEnv, (DWORD) strlen( requestedEnv ), &bytesWritten, &writeOverlapped));
    TEST_ASSERT( GetLastError() == ERROR_IO_PENDING );

    TEST_ASSERT( !ReadFile( outFileRead.Get(), outResponse, (DWORD) strlen( expectedEnv ), &bytesRead, &readOutOverlapped ) );
    TEST_ASSERT( GetLastError() == ERROR_IO_PENDING );

    // TODO: detect being stuck in a loop:
    //  EXCEPTION_DEBUG_EVENT (1) : PID=29444, TID=31488, exc=c0000005 at 73C005B0
    //  EXCEPTION_DEBUG_EVENT (1) : PID=29444, TID=31488, exc=c0000005 at 73C005B0
    //  EXCEPTION_DEBUG_EVENT (1) : PID=29444, TID=31488, exc=c0000005 at 73C005B0
    //  ...

    for ( ; !mCallback->GetProcessExited(); )
    {
        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

        // one reason for timeout is if debuggee writes more to std files than we expect
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( proc->IsStopped() )
        {
            if ( !sawLoadComplete && mCallback->GetLoadCompleted() )
            {
                sawLoadComplete = true;
                // now that the app is loaded, its window should be visible if it has one
                AssertConsoleWindow( newConsole, proc->GetId() );
            }

            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, false ) ) );
        }
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );
    TEST_ASSERT( mCallback->GetProcessExitCode() == -333 );

    waitRet = WaitForSingleObject( readErrEvent.Get(), DefaultIOTimeoutMillis );
    TEST_ASSERT( waitRet == WAIT_OBJECT_0 );
    TEST_ASSERT( GetOverlappedResult( errFileRead.Get(), &readErrOverlapped, &bytesRead, FALSE ) );
    response[bytesRead] = '\0';
    TEST_ASSERT( strcmp( response, expectedArgsAndCurDir ) == 0 );

    waitRet = WaitForSingleObject( writeEvent.Get(), DefaultIOTimeoutMillis );
    TEST_ASSERT( waitRet == WAIT_OBJECT_0 );
    TEST_ASSERT( GetOverlappedResult( inFileWrite.Get(), &writeOverlapped, &bytesWritten, FALSE ) );

    waitRet = WaitForSingleObject( readOutEvent.Get(), DefaultIOTimeoutMillis );
    TEST_ASSERT( waitRet == WAIT_OBJECT_0 );
    TEST_ASSERT( GetOverlappedResult( outFileRead.Get(), &readOutOverlapped, &bytesRead, FALSE ) );
    outResponse[bytesRead] = '\0';
    TEST_ASSERT( strcmp( outResponse, expectedEnv ) == 0 );

    // was anything else written that shouldn't have?

    TEST_ASSERT( !ReadFile( errFileRead.Get(), response, 1, &bytesRead, &readErrOverlapped ) );
    TEST_ASSERT( GetLastError() == ERROR_IO_PENDING );
    TEST_ASSERT( bytesRead == 0 );

    TEST_ASSERT( !ReadFile( outFileRead.Get(), outResponse, 1, &bytesRead, &readOutOverlapped ) );
    TEST_ASSERT( GetLastError() == ERROR_IO_PENDING );
    TEST_ASSERT( bytesRead == 0 );
}

BOOL CALLBACK CheckWindowProc( HWND hWnd, LPARAM param )
{
    CheckWindowInfo*    winInfo = (CheckWindowInfo*) param;
    DWORD               procId = 0;
    DWORD               threadId = 0;

    threadId = GetWindowThreadProcessId( hWnd, &procId );

    if ( procId == winInfo->ProcId )
    {
        winInfo->DebuggeeHwnd = hWnd;
        SetLastError( NO_ERROR );
        return FALSE;
    }

    return TRUE;
}

void StartStopSuite::AssertConsoleWindow( bool newConsole, DWORD procId )
{
    CheckWindowInfo winInfo = { 0 };

    winInfo.ProcId = procId;

    TEST_ASSERT( EnumWindows( CheckWindowProc, (LPARAM) &winInfo ) || (GetLastError() == NO_ERROR) );
    TEST_ASSERT( (winInfo.DebuggeeHwnd != NULL) == newConsole );
}

void StartStopSuite::AssertProcessFinished( uint32_t pid )
{
    HANDLE  hProc = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid );

    if ( hProc != NULL )
    {
        DWORD   waitRet = WaitForSingleObject( hProc, 10 * 1000 );
        if ( waitRet != WAIT_OBJECT_0 )
            TerminateProcess( hProc, (UINT) -1 );
        CloseHandle( hProc );

        TEST_ASSERT( waitRet != (DWORD) -1 );
        TEST_ASSERT_MSG( waitRet == WAIT_OBJECT_0, "Debuggee should not be running after Exec is destroyed." )
    }
}

void StartStopSuite::AssertProcessFinished( uint32_t pid, uint32_t expectedExitCode )
{
    HANDLE  hProc = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid );

    if ( hProc != NULL )
    {
        DWORD   waitRet = WaitForSingleObject( hProc, 10 * 1000 );
        DWORD   exitCode = 0;

        TEST_ASSERT( GetExitCodeProcess( hProc, &exitCode ) );

        if ( waitRet != WAIT_OBJECT_0 )
            TerminateProcess( hProc, (UINT) -1 );

        CloseHandle( hProc );

        TEST_ASSERT( exitCode == expectedExitCode );
        TEST_ASSERT( waitRet != (DWORD) -1 );
        TEST_ASSERT_MSG( waitRet == WAIT_OBJECT_0, "Debuggee should not be running after Exec is destroyed." );
    }
}

void StartStopSuite::TestAsyncBreak()
{
    enum State
    {
        State_Init,
        State_Sync1,
        State_GotBreak,
        State_Done
    };

    Exec    exec;
    State   state = State_Init;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo          info = { 0 };
    wchar_t             cmdLine[MAX_PATH] = L"";
    RefPtr<IProcess>    proc;
    const wchar_t*      Debuggee = EventsDebuggee;

    swprintf_s( cmdLine, L"\"%s\" break 1", Debuggee );

    info.CommandLine = cmdLine;
    info.Exe = Debuggee;

    mCallback->SetTrackLastEvent( true );

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc.Ref() ) ) );

    uint32_t    pid = proc->GetId();

    for ( int i = 0; !mCallback->GetProcessExited(); i++ )
    {
        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );
        bool handled = false;

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( state == State_Init )
        {
            if ( mCallback->GetLastEvent()->Code == ExecEvent_OutputString )
            {
                state = State_Sync1;
                TEST_ASSERT_RETURN( SUCCEEDED( exec.AsyncBreak( proc ) ) );
            }
        }
        else if ( state == State_Sync1 )
        {
            if ( mCallback->GetLastEvent()->Code == ExecEvent_Breakpoint )
                state = State_GotBreak;
        }
        else if ( state == State_GotBreak )
        {
            if ( mCallback->GetLastEvent()->Code == ExecEvent_Exception
                && ((ExceptionEventNode*) mCallback->GetLastEvent().get())->Exception.ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO )
            {
                CONTEXT context = { 0 };

                hr = exec.GetThreadContext(
                    proc.Get(),
                    mCallback->GetLastEvent()->ThreadId,
                    CONTEXT_INTEGER,
                    0,
                    &context,
                    sizeof context );
                TEST_ASSERT_RETURN( SUCCEEDED( hr ) );

                context.Ebx = 1;

                hr = exec.SetThreadContext(
                    proc.Get(),
                    mCallback->GetLastEvent()->ThreadId,
                    &context,
                    sizeof context );
                TEST_ASSERT_RETURN( SUCCEEDED( hr ) );

                // The div instruction will run again, but now dividing by non-zero.
                handled = true;
                state = State_Done;
            }
        }

        if ( proc->IsStopped() )
            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, handled ) ) );
    }

    TEST_ASSERT( state == State_Done );
    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );
    TEST_ASSERT( mCallback->GetProcessExitCode() == 1911 );

    AssertProcessFinished( pid );
}

void StartStopSuite::TestMultiProcess()
{
    enum
    {
        Debuggees   = 2,
    };

    enum State
    {
        State_Init,
        State_Sync1,
        State_Done
    };

    struct DebuggeeState
    {
        RefPtr<IProcess>    Proc;
        int                 Synched;
    };

    Exec    exec;
    State   state = State_Init;

    RefPtr<MultiEventCallbackBase> callback = new MultiEventCallbackBase();

    callback->SetTrackLastEvent( true );

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( callback ) ) );

    LaunchInfo          info = { 0 };
    wchar_t             cmdLine[MAX_PATH] = L"";
    DebuggeeState       debuggeeStates[Debuggees];
    const wchar_t*      Debuggee = EventsDebuggee;
    unsigned int        synched = 0;
    unsigned int        totalExited = 0;

    for ( unsigned int i = 0; i < Debuggees; i++ )
    {
        swprintf_s( cmdLine, L"\"%s\" multi %u %u", Debuggee, i, GetCurrentProcessId() );

        info.CommandLine = cmdLine;
        info.Exe = Debuggee;

        TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, debuggeeStates[i].Proc.Ref() ) ) );
        debuggeeStates[i].Synched = 0;
    }

    for ( ; totalExited < Debuggees; )
    {
        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );
        bool handled = false;

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        DWORD pid = callback->GetLastEvent()->ProcessId;
        IProcess* proc = callback->GetProcess( pid );

        if ( callback->GetLastEvent()->Code == ExecEvent_ProcessExit )
            totalExited++;

        if ( state == State_Init || state == State_Sync1 )
        {
            if ( callback->GetLastEvent()->Code == ExecEvent_OutputString )
            {
                unsigned int i;
                for ( i = 0; i < Debuggees; i++ )
                {
                    if ( pid == debuggeeStates[i].Proc->GetId() )
                    {
                        TEST_ASSERT_RETURN( debuggeeStates[i].Synched == (state == State_Init ? 0 : 1) );
                        debuggeeStates[i].Synched++;
                        break;
                    }
                }
                TEST_ASSERT_RETURN( i < Debuggees );
                synched++;
                if ( synched == Debuggees )
                {
                    state = (State) (state + 1);
                    synched = 0;
                }
            }
        }

        if ( proc->IsStopped() )
            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( proc, handled ) ) );
    }

    TEST_ASSERT( state == State_Done );

    for ( unsigned int i = 0; i < Debuggees; i++ )
    {
        DWORD pid = debuggeeStates[i].Proc->GetId();

        TEST_ASSERT( callback->GetLoadCompleted( pid ) );
        TEST_ASSERT( callback->GetProcessExited( pid ) );
        TEST_ASSERT( callback->GetProcessExitCode( pid ) == 0 );

        AssertProcessFinished( pid );
    }
}
