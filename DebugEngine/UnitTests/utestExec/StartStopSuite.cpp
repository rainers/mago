/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "stdafx.h"
#include "StartStopSuite.h"
#include "EventCallbackBase.h"

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
    // TODO: TestAttach, TestDetachStopped, TestDetachRunning
    TEST_ADD( StartStopSuite::TestOptionsSameConsole );
    TEST_ADD( StartStopSuite::TestOptionsNewConsole );
}

void StartStopSuite::setup()
{
    mExec = new Exec();
    mCallback = new EventCallbackBase();

    HRESULT hr = MakeMachineX86( mMachine );
    if ( FAILED( hr ) )
        throw "MakeMachineX86 failed.";

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

    if ( mMachine != NULL )
    {
        mMachine->Release();
        mMachine = NULL;
    }
}

void StartStopSuite::TestInit()
{
    TEST_ASSERT( FAILED( mExec->Init( NULL, NULL ) ) );
    TEST_ASSERT( FAILED( mExec->Init( mMachine, NULL ) ) );
    TEST_ASSERT( FAILED( mExec->Init( NULL, mCallback ) ) );

    TEST_ASSERT( SUCCEEDED( mExec->Init( mMachine, mCallback ) ) );
}

void StartStopSuite::TestLaunchDestroyExecStopped()
{
    uint32_t    pid = 0;

    {
        Exec    exec;

        TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mMachine, mCallback ) ) );

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
            HRESULT hr = exec.WaitForDebug( DefaultTimeoutMillis );

            TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
            TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

            if ( !sawLoadCompleted && mCallback->GetLoadCompleted() )
            {
                sawLoadCompleted = true;
                break;
            }
            else
            {
                TEST_ASSERT_RETURN( SUCCEEDED( exec.ContinueDebug( true ) ) );
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

        TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mMachine, mCallback ) ) );

        LaunchInfo  info = { 0 };
        wchar_t     cmdLine[ MAX_PATH ] = L"";
        IProcess*   proc = NULL;

        swprintf_s( cmdLine, L"\"%s\"", SleepingDebuggee );

        info.CommandLine = cmdLine;
        info.Exe = SimplestDebuggee;

        TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc ) ) );
        pid = proc->GetId();
        proc->Release();

        bool        sawLoadCompleted = false;

        for ( ; !mCallback->GetProcessExited(); )
        {
            HRESULT hr = exec.WaitForDebug( DefaultTimeoutMillis );

            if ( (hr == E_TIMEOUT) && sawLoadCompleted )
                break;

            TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
            TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

            if ( !sawLoadCompleted && mCallback->GetLoadCompleted() )
            {
                sawLoadCompleted = true;
            }

            TEST_ASSERT_RETURN( SUCCEEDED( exec.ContinueDebug( true ) ) );
        }

        TEST_ASSERT( !mCallback->GetProcessExited() );
    }

    TEST_ASSERT( pid != 0 );

    AssertProcessFinished( pid );
}

void StartStopSuite::TestBeginToEnd()
{
    Exec    exec;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mMachine, mCallback ) ) );

    LaunchInfo  info = { 0 };
    wchar_t     cmdLine[ MAX_PATH ] = L"";
    IProcess*   proc = NULL;

    swprintf_s( cmdLine, L"\"%s\"", SimplestDebuggee );

    info.CommandLine = cmdLine;
    info.Exe = SimplestDebuggee;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc ) ) );

    uint32_t    pid = proc->GetId();

    proc->Release();

    for ( int i = 0; !mCallback->GetProcessExited(); i++ )
    {
        HRESULT hr = exec.WaitForDebug( DefaultTimeoutMillis );

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        TEST_ASSERT_RETURN( SUCCEEDED( exec.ContinueDebug( true ) ) );
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );
    TEST_ASSERT( mCallback->GetProcessExitCode() == 0 );

    AssertProcessFinished( pid );
}

void StartStopSuite::TestTerminateStopped()
{
    Exec    exec;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mMachine, mCallback ) ) );

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
        HRESULT hr = exec.WaitForDebug( DefaultTimeoutMillis );

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( !sawLoadCompleted && mCallback->GetLoadCompleted() )
        {
            sawLoadCompleted = true;

            TEST_ASSERT_RETURN( SUCCEEDED( exec.Terminate( proc.Get() ) ) );
        }
        else
        {
            TEST_ASSERT_RETURN( SUCCEEDED( exec.ContinueDebug( true ) ) );
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

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mMachine, mCallback ) ) );

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
        HRESULT hr = exec.WaitForDebug( DefaultTimeoutMillis );

        // debuggee is in the middle of the sleep
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        TEST_ASSERT_RETURN( SUCCEEDED( exec.ContinueDebug( true ) ) );
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( !mCallback->GetProcessExited() );

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Terminate( proc.Get() ) ) );
    TEST_ASSERT_RETURN( SUCCEEDED( exec.WaitForDebug( DefaultTimeoutMillis ) ) );
    TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );
    TEST_ASSERT_RETURN( SUCCEEDED( exec.ContinueDebug( true ) ) );
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

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mMachine, mCallback ) ) );

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

    TEST_ASSERT( !ReadFile( errFileRead.Get(), response, strlen( expectedArgsAndCurDir ), &bytesRead, &readErrOverlapped ) );
    TEST_ASSERT( GetLastError() == ERROR_IO_PENDING );

    TEST_ASSERT( !WriteFile( inFileWrite.Get(), requestedEnv, strlen( requestedEnv ), &bytesWritten, &writeOverlapped));
    TEST_ASSERT( GetLastError() == ERROR_IO_PENDING );

    TEST_ASSERT( !ReadFile( outFileRead.Get(), outResponse, strlen( expectedEnv ), &bytesRead, &readOutOverlapped ) );
    TEST_ASSERT( GetLastError() == ERROR_IO_PENDING );

    for ( ; !mCallback->GetProcessExited(); )
    {
        HRESULT hr = exec.WaitForDebug( DefaultTimeoutMillis );

        // one reason for timeout is if debuggee writes more to std files than we expect
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( !sawLoadComplete && mCallback->GetLoadCompleted() )
        {
            sawLoadComplete = true;
            // now that the app is loaded, its window should be visible if it has one
            AssertConsoleWindow( newConsole, proc->GetId() );
        }

        TEST_ASSERT_RETURN( SUCCEEDED( exec.ContinueDebug( true ) ) );
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
        CloseHandle( hProc );

        TEST_ASSERT( waitRet != (DWORD) -1 );
        TEST_ASSERT_MSG( waitRet == WAIT_OBJECT_0, "Debuggee should not be running after Exec is destroyed." )
    }
}
