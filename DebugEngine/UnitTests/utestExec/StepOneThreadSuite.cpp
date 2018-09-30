/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "stdafx.h"
#include "StepOneThreadSuite.h"
#include "EventCallbackBase.h"
#include <dia2.h>
#include <atlbase.h>


enum Action
{
    Action_Go,
    Action_StepInstruction,
    Action_StepRange,
};

struct ExpectedEvent
{
    ExecEvent   Code;
    int         FunctionIndex;
    uintptr_t   AddressOffset;
    DWORD       ExceptionCode;
};

struct WantedAction
{
    Action      Action;
    bool        StepIn;
    bool        SourceMode;
    bool        CanStepInFunction;
    int         FunctionIndex;
    uintptr_t   BPAddressOffset;
};

struct Step
{
    ExpectedEvent   Event;
    WantedAction    Action;
};

enum
{
    Func_None,
    Func_Scenario1Func0,
    Func_Scenario1Func1,
    Func_Scenario1Func2,
    Func_Scenario1Func3,
};

struct Symbol
{
    const wchar_t*  Name;
    DWORD           RelativeAddress;
};


// dia2.h has the wrong IID?
// {2F609EE1-D1C8-4E24-8288-3326BADCD211}
EXTERN_C const GUID DECLSPEC_SELECTANY guidIDiaSession =
{ 0x2F609EE1, 0xD1C8, 0x4E24, { 0x82, 0x88, 0x33, 0x26, 0xBA, 0xDC, 0xD2, 0x11 } };


const DWORD DefaultTimeoutMillis = 500;


static HRESULT FindSymbols( const wchar_t* exePath, Symbol* symbols, int count )
{
    HRESULT                 hr;
    CComPtr<IDiaDataSource> source;
    CComPtr<IDiaSession>    session;
    CComPtr<IDiaSymbol>     global;

    // Obtain access to the provider
    GUID msdia140 = { 0xe6756135, 0x1e65, 0x4d17, { 0x85, 0x76, 0x61, 0x07, 0x61, 0x39, 0x8c, 0x3c } };
    GUID msdia120 = { 0x3BFCEA48, 0x620F, 0x4B6B, { 0x81, 0xF7, 0xB9, 0xAF, 0x75, 0x45, 0x4C, 0x7D } };
    GUID msdia110 = { 0x761D3BCD, 0x1304, 0x41D5, { 0x94, 0xE8, 0xEA, 0xC5, 0x4E, 0x4A, 0xC1, 0x72 } };
    GUID msdia100 = { 0xB86AE24D, 0xBF2F, 0x4AC9, { 0xB5, 0xA2, 0x34, 0xB1, 0x4E, 0x4C, 0xE1, 0x1D } }; // same as msdia80

    hr = CoCreateInstance( msdia140, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &source );
    if ( FAILED( hr ) )
        hr = CoCreateInstance( msdia120, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &source );
    if ( FAILED( hr ) )
        hr = CoCreateInstance( msdia110, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &source );
    if ( FAILED( hr ) )
        hr = CoCreateInstance( msdia100, NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (void **) &source );
    if ( FAILED( hr ) )
        return hr;
    //hr = mSource->loadDataForExe( filename, searchPath, callback );
    hr = source->loadDataForExe( exePath, NULL, NULL );
    if ( FAILED( hr ) )
        return hr;
    // Open a session for querying symbols
    hr = source->openSession( &session );
    if ( FAILED( hr ) )
        return hr;
    // Retrieve a reference to the global scope
    hr = session->get_globalScope( &global );
    if ( hr != S_OK )
        return hr;

    for ( int i = 0; i < count; i++ )
    {
        Symbol& symbol = symbols[i];
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = global->findChildren( SymTagNull, symbol.Name, nsCaseSensitive, &enumSymbols );
        if ( FAILED( hr ) )
            return hr;

        CComPtr<IDiaSymbol> pSymbol;
        DWORD fetched = 0;

        hr = enumSymbols->Next( 1, &pSymbol, &fetched );
        if ( FAILED( hr ) )
            return hr;
        if ( hr != S_OK )
            return E_FAIL;

        hr = pSymbol->get_relativeVirtualAddress( &symbol.RelativeAddress );
        if ( FAILED( hr ) )
            return hr;
    }

    return S_OK;
}

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
        { { ExecEvent_Breakpoint,   Func_Scenario1Func0, 0x0005, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0006, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0008, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0002, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0004, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0007, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x000C, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x000C, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x000E, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func3, 0x0000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0013, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0005, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x000D, 0 }, { Action_Go, true, false } },
        { { ExecEvent_ProcessExit,  Func_None,           0,      0 }, { Action_Go, true, false } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::StepInstructionInSourceHaveSource()
{
    Step    steps[] = 
    {
        { { ExecEvent_Breakpoint,   Func_Scenario1Func0, 0x0005, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0006, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0008, 0 }, { Action_StepInstruction, true, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0000, 0 }, { Action_StepInstruction, true, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0002, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0004, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0007, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x000C, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x000C, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x000E, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func3, 0x0000, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0013, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0005, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x000D, 0 }, { Action_Go, true, true } },
        { { ExecEvent_ProcessExit,  Func_None,           0,      0 }, { Action_Go, true, true } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::StepInstructionInSourceNoSource()
{
    Step    steps[] = 
    {
        { { ExecEvent_Breakpoint,   Func_Scenario1Func0, 0x0005, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0006, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0008, 0 }, { Action_StepInstruction, true, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0000, 0 }, { Action_StepRange, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0005, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x000D, 0 }, { Action_Go, true, true } },
        { { ExecEvent_ProcessExit,  Func_None,           0,      0 }, { Action_Go, true, true } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::StepInstructionOver()
{
    Step    steps[] = 
    {
        { { ExecEvent_Breakpoint,   Func_Scenario1Func0, 0x0005, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0006, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0008, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0000, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0000, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0002, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0004, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0007, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x000C, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x000E, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func2, 0x0013, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0005, 0 }, { Action_StepInstruction, false, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x000D, 0 }, { Action_Go, true, false } },
        { { ExecEvent_ProcessExit,  Func_None,           0,      0 }, { Action_Go, true, false } },
    };

    RunDebuggee( steps, _countof( steps ) );
}

void StepOneThreadSuite::StepInstructionOverInterruptedByBP()
{
    Step    steps[] = 
    {
        { { ExecEvent_Breakpoint,   Func_Scenario1Func0, 0x0005, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0006, 0 }, { Action_StepInstruction, true, false } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x0008, 0 }, { Action_StepInstruction, true, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0000, 0 }, { Action_StepInstruction, false, true, false, Func_Scenario1Func2, 0x000C } },
        { { ExecEvent_Breakpoint,   Func_Scenario1Func2, 0x000C, 0 }, { Action_Go, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func1, 0x0005, 0 }, { Action_StepInstruction, true, true } },
        { { ExecEvent_StepComplete, Func_Scenario1Func0, 0x000D, 0 }, { Action_Go, true, true } },
        { { ExecEvent_ProcessExit,  Func_None,           0,      0 }, { Action_Go, true, true } },
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

    Symbol funcs[] =
    {
        { NULL, 0 },
        { L"Scenario1Func0", 0 },
        { L"Scenario1Func1", 0 },
        { L"Scenario1Func2", 0 },
        { L"Scenario1Func3", 0 },
    };
    TEST_ASSERT_RETURN( SUCCEEDED( FindSymbols( Debuggee, funcs, _countof( funcs ) ) ) );

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
        HRESULT hr = exec.WaitForEvent( DefaultTimeoutMillis );

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
            if ( mCallback->GetLastThreadId() != 0 )
            {
                TEST_ASSERT_RETURN( process->FindThread( mCallback->GetLastThreadId(), thread.Ref() ) );
                TEST_ASSERT_RETURN( exec.GetThreadContext( 
                    process, thread->GetId(), CONTEXT_X86_FULL, 0, &context, sizeof context ) == S_OK );
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
                uintptr_t   funcRva = funcs[curStep->Event.FunctionIndex].RelativeAddress;
                uintptr_t   addr = (funcRva + curStep->Event.AddressOffset + baseAddr);

                if ( context.Eip != addr )
                {
                    sprintf_s( msg, "Expected instruction pointer at %08x, got %08x.", addr, context.Eip );
                    TEST_FAIL_MSG( msg );
                    break;
                }
            }

            mCallback->SetCanStepInFunctionReturnValue( curStep->Action.CanStepInFunction );

            if ( curStep->Action.BPAddressOffset != 0 )
            {
                uintptr_t   funcRva = funcs[curStep->Action.FunctionIndex].RelativeAddress;
                uintptr_t   addr = (funcRva + curStep->Action.BPAddressOffset + baseAddr);
                TEST_ASSERT_RETURN( SUCCEEDED( exec.SetBreakpoint( process.Get(), addr ) ) );
            }

            if ( curStep->Action.Action == Action_StepInstruction )
            {
                TEST_ASSERT_RETURN( SUCCEEDED( 
                    exec.StepInstruction( process.Get(), curStep->Action.StepIn, true ) ) );
                continued = true;
            }
            else if ( curStep->Action.Action == Action_StepRange )
            {
                AddressRange range = { context.Eip, context.Eip };
                TEST_ASSERT_RETURN( SUCCEEDED( 
                    exec.StepRange( process.Get(), curStep->Action.StepIn, range, true ) ) );
                continued = true;
            }
        }

        if ( !continued )
            TEST_ASSERT_RETURN( SUCCEEDED( exec.Continue( process, true ) ) );
    }

    TEST_ASSERT( mCallback->GetLoadCompleted() );
    TEST_ASSERT( mCallback->GetProcessExited() );
}
