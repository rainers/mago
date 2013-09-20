/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "stdafx.h"
#include "EventSuite.h"
#include "EventCallbackBase.h"
#include "Utility.h"

using namespace std;
using namespace boost;


const DWORD     DefaultTimeoutMillis = 500;
const wchar_t*  OutputStrings[] = 
{
    L"I write an ASCII message.",
    L"Filler in ASCII.",
    L"And now a Unicode one. ?. Adiós!",
    L"I write an ASCII message.",
    L"a??Axyz_0123?\?!?",           // why does Windows convert that char outside the BMP to two ANSI chars?
    L"And now a Unicode one. Bye!",
    L"<placeholder>",               // for a very long string
    L"<placeholder>",               // for a very long string
};


EventSuite::EventSuite()
{
    TEST_ADD( EventSuite::TestModuleLoad );
    TEST_ADD( EventSuite::TestOutputString );
    TEST_ADD( EventSuite::TestExceptionHandledFirstChance );
    TEST_ADD( EventSuite::TestExceptionNotHandledFirstChanceNotCaught );
    TEST_ADD( EventSuite::TestExceptionNotHandledFirstChanceCaught );
    TEST_ADD( EventSuite::TestExceptionNotHandledAllChances );
}

void EventSuite::setup()
{
    mExec = new Exec();
    mCallback = new EventCallbackBase();

    mCallback->AddRef();
}

void EventSuite::tear_down()
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


void EventSuite::TestModuleLoad()
{
    RefPtr<IProcess>    proc;

    RunDebuggee( proc );

    AssertModuleLoads( proc.Get() );
}

void EventSuite::TestOutputString()
{
    RefPtr<IProcess>    proc;

    RunDebuggee( proc );

    AssertOutputStrings();
}

void EventSuite::RunDebuggee( RefPtr<IProcess>& process )
{
    Exec    exec;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo      info = { 0 };
    wchar_t         cmdLine[ MAX_PATH ] = L"";
    IProcess*       proc = NULL;
    const wchar_t*  Debuggee = EventsDebuggee;

    swprintf_s( cmdLine, L"\"%s\"", Debuggee );

    info.CommandLine = cmdLine;
    info.Exe = Debuggee;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc ) ) );

    uint32_t    pid = proc->GetId();
    mCallback->SetTrackEvents( true );

    process = proc;
    proc->Release();

    for ( int i = 0; !mCallback->GetProcessExited(); i++ )
    {
        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( exec.DispatchEvent() ) );

        if ( process->IsStopped() )
            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, true ) ) );
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );

    //AssertProcessFinished( pid );
}

void EventSuite::AssertModuleLoads( IProcess* process )
{
    const EventList&    list = mCallback->GetEvents();
    bool    dllLoad1 = false;
    bool    dllUnload = false;
    bool    dllLoad2 = false;
    bool    tapiLoaded = false;
    bool    procImageLoaded = false;
    int     loadOrder = 0;
    wchar_t drive[MAX_PATH] = L"";
    wchar_t dir[MAX_PATH] = L"";
    wchar_t fileName[MAX_PATH] = L"";
    wchar_t ext[MAX_PATH] = L"";

    TEST_ASSERT_RETURN( _wsplitpath_s( EventsDebuggee, drive, dir, fileName, ext ) == 0 );
    wcscat_s( fileName, ext );

    for ( EventList::const_iterator it = list.begin();
        it != list.end();
        it++ )
    {
        const EventNode*  node = it->get();

        if ( node->Code == IEventCallback::Event_ModuleUnload )
        {
            if ( wcsstr( ((ModuleUnloadEventNode*) node)->Module->GetExePath(), L"ws2_32.dll" ) != NULL )
            {
                if ( dllLoad1 && !dllUnload && !dllLoad2 )
                    dllUnload = true;
                else
                    TEST_FAIL( "ws2_32.dll unloaded out-of-order." );
            }
        }

        if ( node->Code != IEventCallback::Event_ModuleLoad )
            continue;

        loadOrder++;

        const ModuleLoadEventNode*  modNode = (ModuleLoadEventNode*) node;
        vector<wchar_t>     fullPathBuf( MAX_PATH );
        wchar_t*            pRet = NULL;

        do
        {
            pRet = _wfullpath( &fullPathBuf[0], modNode->Module->GetExePath(), fullPathBuf.size() );
            if ( pRet == NULL )
            {
                fullPathBuf.resize( fullPathBuf.size() * 2 );
            }
        } while ( pRet == NULL );

        TEST_ASSERT( _wcsicmp( &fullPathBuf[0], modNode->Module->GetExePath() ) == 0 );
        TEST_ASSERT( GetFileAttributes( modNode->Module->GetExePath() ) != INVALID_FILE_ATTRIBUTES );

        // TODO: test size?

        size_t          exePathLen = wcslen( modNode->Module->GetExePath() );
        const wchar_t*  lastWack = wcsrchr( modNode->Module->GetExePath(), L'\\' );

        TEST_ASSERT_MSG( lastWack != NULL, "There was no backslash." );

        if ( lastWack == NULL )
            continue;

        const wchar_t*  name = lastWack + 1;
        size_t          nameLen = wcslen( name );

        TEST_ASSERT( nameLen > 0 );

        if ( _wcsicmp( name, L"ws2_32.dll" ) == 0 )
        {
            if ( !dllLoad1 && !dllUnload && !dllLoad2 )
                dllLoad1 = true;
            else if ( dllLoad1 && dllUnload && !dllLoad2 )
                dllLoad2 = true;
            else
                TEST_FAIL( "ws2_32.dll loaded out-of-order." );
        }
        else if ( _wcsicmp( name, L"tapi32.dll" ) == 0 )
        {
            TEST_ASSERT( !tapiLoaded );
            tapiLoaded = true;
        }
        else if ( _wcsicmp( name, fileName ) == 0 )
        {
            TEST_ASSERT( !procImageLoaded );
            TEST_ASSERT( loadOrder == 1 );
            TEST_ASSERT( _wcsicmp( modNode->Module->GetExePath(), process->GetExePath() ) == 0 );
            procImageLoaded = true;
        }
    }

    TEST_ASSERT( procImageLoaded );
}

void EventSuite::AssertOutputStrings()
{
    const EventList&    list = mCallback->GetEvents();
    int     strIndex = 0;
    bool    failedMatch = false;

    // ready very long strings that are longer than a memory page
    const wchar_t   RepeatingWstr[] = L"The daily news in Japanese is:  ???????.";
    const wchar_t   RepeatingAstr[] = L"The daily news in Spanish is:   la noticia diaria.";
    wchar_t longWstr[ 5000 ] = L"";
    wchar_t longAstr[ 5000 ] = L"";
    size_t  repWstrLen = wcslen( RepeatingWstr );
    size_t  repAstrLen = wcslen( RepeatingAstr );

    for ( int i = 0; i < (_countof( longWstr ) / _countof( RepeatingWstr )); i++ )
    {
        size_t  charsLeft = _countof( longWstr ) - (i * repWstrLen);

        wcscpy_s( &longWstr[ i * repWstrLen ], charsLeft, RepeatingWstr );
    }

    for ( int i = 0; i < (_countof( longAstr ) / _countof( RepeatingAstr )); i++ )
    {
        size_t  charsLeft = _countof( longAstr ) - (i * repAstrLen);

        wcscpy_s( &longAstr[ i * repAstrLen ], charsLeft, RepeatingAstr );
    }

    for ( EventList::const_iterator it = list.begin();
        it != list.end();
        it++ )
    {
        const EventNode*  node = it->get();

        if ( node->Code != IEventCallback::Event_OutputString )
            continue;

        const OutputStringEventNode*  outNode = (OutputStringEventNode*) node;

        if ( strIndex < _countof( OutputStrings ) )
        {
            const wchar_t*  refStr = OutputStrings[strIndex];

            if ( strIndex == (_countof( OutputStrings ) - 2) )
                refStr = longAstr;
            else if ( strIndex == (_countof( OutputStrings ) - 1) )
                refStr = longWstr;

            bool    matches = wcscmp( refStr, outNode->String.c_str() ) == 0;

            if ( !matches && !failedMatch )
            {
                TEST_ASSERT_MSG( matches, "String doesn't match what's expected." );
                // do this so we don't flood the log with the same error
                failedMatch = true;
            }

            strIndex++;
        }
        else
        {
            TEST_FAIL( "There were more OutputString messages than expected." );
            break;
        }
    }

    TEST_ASSERT_MSG( strIndex == _countof( OutputStrings ), "There weren't enough OutputString messages." );
}

void EventSuite::TestExceptionHandledFirstChance()
{
    TryHandlingException( true, true );
}

void EventSuite::TestExceptionNotHandledFirstChanceNotCaught()
{
    TryHandlingException( false, false );
}

void EventSuite::TryHandlingException( bool firstTimeHandled, bool expectedChanceSecondTime )
{
    enum State
    {
        State_Init,
        State_FirstHandled,
        State_SecondHandled,
        State_Done
    };

    Exec    exec;
    State   state = State_Init;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo      info = { 0 };
    wchar_t         cmdLine[ MAX_PATH ] = L"";
    IProcess*       proc = NULL;
    const wchar_t*  Debuggee = EventsDebuggee;

    swprintf_s( cmdLine, L"\"%s\" exception 1", Debuggee );

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

        if ( process->IsStopped() )
        {
            CONTEXT_X86     context = { 0 };
            RefPtr<Thread>  thread;

            TEST_ASSERT_RETURN( process->FindThread( mCallback->GetLastThreadId(), thread.Ref() ) );
            context.ContextFlags = CONTEXT_X86_FULL;
            TEST_ASSERT_RETURN( 
                exec.GetThreadContext( process, thread->GetId(), &context, sizeof context ) == S_OK );

            if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_Exception) )
            {
                ExceptionEventNode* node = (ExceptionEventNode*) mCallback->GetLastEvent().get();

                if ( node->Exception.ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO )
                {
                    if ( state == State_Init )
                    {
                        TEST_ASSERT( node->FirstChance );
                        state = State_FirstHandled;
                        handled = firstTimeHandled;
                    }
                    else if ( state == State_FirstHandled )
                    {
                        TEST_ASSERT( node->FirstChance == expectedChanceSecondTime );
                        state = State_SecondHandled;

                        context.Ebx = 455;
                        TEST_ASSERT_RETURN( context.Eax != 237 );
                        TEST_ASSERT_RETURN( 
                            exec.SetThreadContext( 
                                process, thread->GetId(), &context, sizeof context ) == S_OK );
                    }
                    else
                    {
                        TEST_FAIL( "Too many Integer divides by zero." );
                        exec.Terminate( process.Get() );
                    }
                }
                else
                {
                    TEST_ASSERT( node->FirstChance );
                    TEST_FAIL( "Unexpected exception." );
                    exec.Terminate( process.Get() );
                }
            }
            else if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_StepComplete)
                && (state == State_SecondHandled) )
            {
                state = State_Done;
                TEST_ASSERT( context.Eax == 237 );
            }

            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, handled ) ) );
        }
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );

    TEST_ASSERT( state == State_Done );
}

void EventSuite::TestExceptionNotHandledFirstChanceCaught()
{
    enum State
    {
        State_Init,
        State_FirstNotHandled,
        State_Done
    };

    Exec    exec;
    State   state = State_Init;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo      info = { 0 };
    wchar_t         cmdLine[ MAX_PATH ] = L"";
    IProcess*       proc = NULL;
    const wchar_t*  Debuggee = EventsDebuggee;

    swprintf_s( cmdLine, L"\"%s\" exception 2", Debuggee );

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

        if ( process->IsStopped() )
        {
            if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_Exception) )
            {
                ExceptionEventNode* node = (ExceptionEventNode*) mCallback->GetLastEvent().get();

                TEST_ASSERT( node->FirstChance );

                if ( node->Exception.ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO )
                {
                    if ( state == State_Init )
                    {
                        state = State_FirstNotHandled;
                        handled = false;
                    }
                    else
                    {
                        TEST_FAIL( "Too many Integer divides by zero." );
                        exec.Terminate( process.Get() );
                    }
                }
                else
                {
                    TEST_FAIL( "Unexpected exception." );
                    exec.Terminate( process.Get() );
                }
            }
            else if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_Breakpoint)
                && (state == State_FirstNotHandled) )
            {
                state = State_Done;

                CONTEXT_X86     context = { 0 };
                RefPtr<Thread>  thread;

                TEST_ASSERT_RETURN( process->FindThread( mCallback->GetLastThreadId(), thread.Ref() ) );
                context.ContextFlags = CONTEXT_X86_FULL;
                TEST_ASSERT_RETURN( GetThreadContextX86( thread->GetHandle(), &context ) );
                TEST_ASSERT( context.Eax == 1877514773 );
            }

            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, handled ) ) );
        }
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );

    TEST_ASSERT( state == State_Done );
}

void EventSuite::TestExceptionNotHandledAllChances()
{
    enum State
    {
        State_Init,
        State_FirstNotHandled,
        State_SecondNotHandled,
        State_Done
    };

    Exec    exec;
    State   state = State_Init;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo      info = { 0 };
    wchar_t         cmdLine[ MAX_PATH ] = L"";
    IProcess*       proc = NULL;
    const wchar_t*  Debuggee = EventsDebuggee;

    swprintf_s( cmdLine, L"\"%s\" exception 1", Debuggee );

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

        if ( state == State_SecondNotHandled )
        {
            TEST_ASSERT( mCallback->GetProcessExited() );
            state = State_Done;
        }

        if ( process->IsStopped() )
        {
            if ( (mCallback->GetLastEvent().get() != NULL) 
                && (mCallback->GetLastEvent()->Code == ExecEvent_Exception) )
            {
                ExceptionEventNode* node = (ExceptionEventNode*) mCallback->GetLastEvent().get();

                if ( node->Exception.ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO )
                {
                    if ( state == State_Init )
                    {
                        TEST_ASSERT( node->FirstChance );
                        state = State_FirstNotHandled;
                        handled = false;
                    }
                    else if ( state == State_FirstNotHandled )
                    {
                        TEST_ASSERT( !node->FirstChance );
                        state = State_SecondNotHandled;
                        handled = false;
                    }
                    else
                    {
                        TEST_FAIL( "Too many Integer divides by zero." );
                        exec.Terminate( process.Get() );
                    }
                }
                else
                {
                    TEST_FAIL( "Unexpected exception." );
                    exec.Terminate( process.Get() );
                }
            }

            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, handled ) ) );
        }
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );

    TEST_ASSERT( state == State_Done );
}
