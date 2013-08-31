/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "stdafx.h"
#include "StepOneThreadSuite.h"
#include "EventCallbackBase.h"


enum Action
{
    Action_Go,
    Action_StepInstruction,
};

struct ExpectedEvent
{
    ExecEvent   Code;
    uintptr_t   AddressOffset;
    DWORD       ExceptionCode;
};

struct WantedAction
{
    Action      Action;
    bool        StepIn;
    bool        SourceMode;
    bool        CanStepInFunction;
    uintptr_t   BPAddressOffset;
};

struct Step
{
    ExpectedEvent   Event;
    WantedAction    Action;
};


const DWORD DefaultTimeoutMillis = 500;


StepOneThreadSuite::StepOneThreadSuite()
{
    TEST_ADD( StepOneThreadSuite::StepInstructionInAssembly );
    TEST_ADD( StepOneThreadSuite::StepInstructionOver );
    TEST_ADD( StepOneThreadSuite::StepInstructionInSourceHaveSource );
    TEST_ADD( StepOneThreadSuite::StepInstructionInSourceNoSource );
    TEST_ADD( StepOneThreadSuite::StepInstructionOverInterruptedByBP );
}

void StepOneThreadSuite::setup()
{
    mExec = new Exec();
    mCallback = new EventCallbackBase();

    mCallback->AddRef();
}

void StepOneThreadSuite::tear_down()
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

void StepOneThreadSuite::StepInstructionInAssembly()
{
    Step    steps[] = 
    {
        { { ExecEvent_Exception, 0x1069, EXCEPTION_BREAKPOINT_X86 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x106B, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1030, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1012, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1014, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1017, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x101C, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x101C, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x101E, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1023, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1035, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1070, 0 }, { Action_Go, true, false } },
        { { ExecEvent_ProcessExit, 0, 0 }, { Action_Go, true, false } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::StepInstructionInSourceHaveSource()
{
    Step    steps[] = 
    {
        { { ExecEvent_Exception, 0x1069, EXCEPTION_BREAKPOINT_X86 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x106B, 0 }, { Action_StepInstruction, true, true, true } },
        { { ExecEvent_StepComplete, 0x1030, 0 }, { Action_StepInstruction, true, true, true } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1012, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1014, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1017, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x101C, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x101C, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x101E, 0 }, { Action_StepInstruction, true, true } },
        //{ { ExecEvent_StepComplete, 0x1000, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1023, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1035, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1070, 0 }, { Action_Go, true, true } },
        { { ExecEvent_ProcessExit, 0, 0 }, { Action_Go, true, true } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::StepInstructionInSourceNoSource()
{
    Step    steps[] = 
    {
        { { ExecEvent_Exception, 0x1069, EXCEPTION_BREAKPOINT_X86 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x106B, 0 }, { Action_StepInstruction, true, true, true } },
        { { ExecEvent_StepComplete, 0x1030, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1035, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1070, 0 }, { Action_Go, true, true } },
        { { ExecEvent_ProcessExit, 0, 0 }, { Action_Go, true, true } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::StepInstructionOver()
{
    Step    steps[] = 
    {
        { { ExecEvent_Exception, 0x1069, EXCEPTION_BREAKPOINT_X86 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x106B, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1030, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, 0x1010, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, 0x1012, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, 0x1014, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, 0x1017, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, 0x101C, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, 0x101E, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, 0x1023, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, 0x1035, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, 0x1070, 0 }, { Action_Go, true, false } },
        { { ExecEvent_ProcessExit, 0, 0 }, { Action_Go, true, false } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::StepInstructionOverInterruptedByBP()
{
    Step    steps[] = 
    {
        { { ExecEvent_Exception, 0x1069, EXCEPTION_BREAKPOINT_X86 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x106B, 0 }, { Action_StepInstruction, true, true, true } },
        { { ExecEvent_StepComplete, 0x1030, 0 }, { Action_StepInstruction, false, true, false, 0x101C } },
        { { ExecEvent_Breakpoint, 0x101C, 0 }, { Action_Go, true, true } },
        { { ExecEvent_StepComplete, 0x1035, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, 0x1070, 0 }, { Action_Go, true, true } },
        { { ExecEvent_ProcessExit, 0, 0 }, { Action_Go, true, true } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::RunDebuggee( Step* steps, int stepsCount )
{
    Exec    exec;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Init( mCallback ) ) );

    LaunchInfo  info = { 0 };
    wchar_t     cmdLine[ MAX_PATH ] = L"";
    IProcess*   proc = NULL;
    const wchar_t*  Debuggee = StepOneThreadDebuggee;

    swprintf_s( cmdLine, L"\"%s\" 1", Debuggee );

    info.CommandLine = cmdLine;
    info.Exe = Debuggee;

    TEST_ASSERT_RETURN( SUCCEEDED( exec.Launch( &info, proc ) ) );

    uint32_t            pid = proc->GetId();
    int                 nextStep = 0;
    char                msg[1024] = "";
    RefPtr<IProcess>    process;

    process = proc;
    proc->Release();
    mCallback->SetTrackLastEvent( true );

    for ( int i = 0; !mCallback->GetProcessExited(); i++ )
    {
        bool    continued = false;
        HRESULT hr = exec.WaitForDebug( DefaultTimeoutMillis );

        // this should happen after process exit
        if ( hr == E_TIMEOUT )
            break;

        TEST_ASSERT_RETURN( SUCCEEDED( hr ) );
        TEST_ASSERT_RETURN( SUCCEEDED( hr = exec.DispatchEvent() ) );

        if ( !process->IsStopped() )
            continue;

        if ( (mCallback->GetLastEvent()->Code != ExecEvent_ModuleLoad)
            && (mCallback->GetLastEvent()->Code != ExecEvent_ModuleUnload)
            && (mCallback->GetLastEvent()->Code != ExecEvent_ThreadStart)
            && (mCallback->GetLastEvent()->Code != ExecEvent_ThreadExit)
            && (mCallback->GetLastEvent()->Code != ExecEvent_LoadComplete) )
        {
            Step*   curStep = &steps[nextStep];
            nextStep++;

            RefPtr<Thread>  thread;
            CONTEXT_X86     context = { 0 };
            context.ContextFlags = CONTEXT_X86_FULL;
            if ( mCallback->GetLastThreadId() != 0 )
            {
                TEST_ASSERT_RETURN( process->FindThread( mCallback->GetLastThreadId(), thread.Ref() ) );
                TEST_ASSERT_RETURN( GetThreadContextX86( thread->GetHandle(), &context ) );
            }

            uintptr_t   baseAddr = 0;
            RefPtr<IModule> procMod = mCallback->GetProcessModule();
            if ( procMod.Get() != NULL )
                baseAddr = (uintptr_t) procMod->GetImageBase();

            // TODO: test threadId

            if ( mCallback->GetLastEvent()->Code != curStep->Event.Code )
            {
                sprintf_s( msg, "Expected event '%s', got '%s'.", 
                    GetEventName( curStep->Event.Code ),
                    GetEventName( mCallback->GetLastEvent()->Code ) );
                TEST_FAIL_MSG( msg );
                break;
            }
            else if ( (curStep->Event.Code == ExecEvent_Exception)
                && (curStep->Event.ExceptionCode != ((ExceptionEventNode*) mCallback->GetLastEvent().get())->Exception.ExceptionCode) )
            {
                sprintf_s( msg, "Expected exception %08x, got %08x.", 
                    curStep->Event.ExceptionCode, 
                    ((ExceptionEventNode*) mCallback->GetLastEvent().get())->Exception.ExceptionCode );
                TEST_FAIL_MSG( msg );
                break;
            }
            else if ( (curStep->Event.Code == ExecEvent_Exception)
                || (curStep->Event.Code == ExecEvent_StepComplete) 
                || (curStep->Event.Code == ExecEvent_Breakpoint) )
            {
                uintptr_t   addr = (curStep->Event.AddressOffset + baseAddr);

                if ( context.Eip != addr )
                {
                    sprintf_s( msg, "Expected instruction pointer at %p, got %08x.", addr, context.Eip );
                    TEST_FAIL_MSG( msg );
                    break;
                }
            }

            mCallback->SetCanStepInFunctionReturnValue( curStep->Action.CanStepInFunction );

            if ( curStep->Action.BPAddressOffset != 0 )
            {
                uintptr_t   addr = (curStep->Action.BPAddressOffset + baseAddr);
                TEST_ASSERT_RETURN( SUCCEEDED( exec.SetBreakpoint( process.Get(), addr, 1 ) ) );
            }

            if ( curStep->Action.Action == Action_StepInstruction )
            {
                TEST_ASSERT_RETURN( SUCCEEDED( 
                    exec.StepInstruction( process.Get(), curStep->Action.StepIn, curStep->Action.SourceMode ) ) );
                TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, true ) ) );
                continued = true;
            }
        }

        if ( !continued )
            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, true ) ) );
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );
}
