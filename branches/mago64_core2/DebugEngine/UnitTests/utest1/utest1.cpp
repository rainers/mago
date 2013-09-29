/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// test1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\..\Exec\Types.h"
#include "..\..\Exec\Exec.h"
#include "..\..\Exec\EventCallback.h"
#include "..\..\Exec\IProcess.h"
#include "..\..\Exec\IModule.h"
#include "..\..\Exec\Enumerator.h"


class _EventCallback : public IEventCallback
{
    Exec*       mExec;
    IModule*    mMod;
    bool        mHitBp;

public:
    _EventCallback()
        :   mExec( NULL ),
            mMod( NULL ),
            mHitBp( false )
    {
    }

    ~_EventCallback()
    {
        if ( mMod != NULL )
            mMod->Release();
    }

    void SetExec( Exec* exec )
    {
        mExec = exec;
    }

    bool GetModule( IModule*& mod )
    {
        mod = mMod;
        mod->AddRef();
        return mod != NULL;
    }

    bool GetHitBp()
    {
        return mHitBp;
    }

    virtual void AddRef()
    {
    }

    virtual void Release()
    {
    }

    virtual void OnProcessStart( IProcess* process )
    {
    }

    virtual void OnProcessExit( IProcess* process, DWORD exitCode )
    {
    }

    virtual void OnThreadStart( IProcess* process, Thread* thread )
    {
    }

    virtual void OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
    {
    }

    virtual void OnModuleLoad( IProcess* process, IModule* module )
    {
        char*   macName = "";

        switch ( module->GetMachine() )
        {
        case IMAGE_FILE_MACHINE_I386: macName = "x86"; break;
        case IMAGE_FILE_MACHINE_IA64: macName = "ia64"; break;
        case IMAGE_FILE_MACHINE_AMD64: macName = "x64"; break;
        }

        if ( sizeof( Address ) == sizeof( uintptr_t ) )
            printf( "  %p %d %s '%ls'\n", module->GetImageBase(), module->GetSize(), macName, module->GetExePath() );
        else
            printf( "  %08I64x %d %s '%ls'\n", module->GetImageBase(), module->GetSize(), macName, module->GetExePath() );

        if ( mMod == NULL )
        {
            mMod = module;
            mMod->AddRef();
        }
    }

    virtual void OnModuleUnload( IProcess* process, Address baseAddr )
    {
        if ( sizeof( Address ) == sizeof( uintptr_t ) )
            printf( "  %p\n", baseAddr );
        else
            printf( "  %08I64x\n", baseAddr );
    }

    virtual void OnOutputString( IProcess* process, const wchar_t* outputString )
    {
        printf( "  '%ls'\n", outputString );
    }

    virtual void OnLoadComplete( IProcess* process, DWORD threadId )
    {
        UINT_PTR    baseAddr = (UINT_PTR) mMod->GetImageBase();

        // 0x003C137A, 0x003C1395
        // 1137A, 11395

#if 0
        mExec->SetBreakpoint( process, baseAddr + 0x0001137A, (void*) 33 );
        mExec->SetBreakpoint( process, baseAddr + 0x00011395, (void*) 17 );

        //mExec->SetBreakpoint( process, 0x003C137A, (void*) 257 );

        //mExec->RemoveBreakpoint( process, 0x003C137A, (void*) 33 );
        //mExec->RemoveBreakpoint( process, 0x003C137A, (void*) 257 );

        //mExec->RemoveBreakpoint( process, 0x003C1395, (void*) 33 );

        //mExec->RemoveBreakpoint( process, 0x003C1395, (void*) 17 );
#endif
    }

    virtual RunMode OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec )
    {
        if ( sizeof( Address ) == sizeof( uintptr_t ) )
            printf( "  %p %08x\n", exceptRec->ExceptionAddress, exceptRec->ExceptionCode );
        else
            printf( "  %08I64x %08x\n", exceptRec->ExceptionAddress, exceptRec->ExceptionCode );
        return RunMode_Break;
    }

    virtual RunMode OnBreakpoint( IProcess* process, uint32_t threadId, Address address, bool embedded )
    {
        if ( sizeof( Address ) == sizeof( uintptr_t ) )
            printf( "  breakpoint at %p\n", address );
        else
            printf( "  breakpoint at %08I64x\n", address );

        mHitBp = true;

        UINT_PTR    baseAddr = (UINT_PTR) mMod->GetImageBase();

        //mExec->RemoveBreakpoint( process, baseAddr + 0x0001137A, (void*) 257 );
        //mExec->SetBreakpoint( process, baseAddr + 0x0001137A, (void*) 257 );
        //mExec->RemoveBreakpoint( process, baseAddr + 0x00011395, (void*) 129 );
        //mExec->SetBreakpoint( process, baseAddr + 0x00011395, (void*) 129 );

        return RunMode_Break;
    }

    virtual void OnStepComplete( IProcess* process, uint32_t threadId )
    {
        printf( "  Step complete\n" );
    }

    virtual void OnAsyncBreakComplete( IProcess* process, uint32_t threadId )
    {
    }

    virtual void OnError( IProcess* process, HRESULT hrErr, EventCode event )
    {
        printf( "  *** ERROR: %08x while %d\n", hrErr, event );
    }

    virtual RunMode OnCallProbe( IProcess* process, uint32_t threadId, Address address )
    {
        return RunMode_Run;
    }
};


void InitDebug()
{
    int f = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
    f |= _CRTDBG_LEAK_CHECK_DF;     // should always use in debug build
    f |= _CRTDBG_CHECK_ALWAYS_DF;   // check on free AND alloc
    _CrtSetDbgFlag( f );

    //_CrtSetAllocHook( LocalMemAllocHook );
    //SetLocalMemWorkingSetLimit( 550 );
}

int _tmain( int argc, _TCHAR* argv[] )
{
    BOOL                bRet = FALSE;
    STARTUPINFO         startupInfo = { sizeof startupInfo };
    PROCESS_INFORMATION procInfo = { 0 };
    DEBUG_EVENT         event = { 0 };
    _EventCallback   callback;
    Exec        exec;
    HRESULT     hr = S_OK;
    LaunchInfo  info = { 0 };

    InitDebug();

    //char*   s1 = new ( _NORMAL_BLOCK, __FILE__, __LINE__ ) char[100];
    //strcpy( s1, "hello, yo!" );
    //char*   s2 = (char*) malloc( 300 );
    //strcpy( s2, "say what?" );

    callback.SetExec( &exec );

    hr = exec.Init( &callback );
    if ( FAILED( hr ) )
        goto Error;
    
#if 0
    bRet = CreateProcess( L"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\test1\\Debug\\test1.exe",
    //bRet = CreateProcess( L"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\test1\\x64\\Debug\\test1.exe",
        NULL,
        NULL,
        NULL,
        FALSE,
        DEBUG_ONLY_THIS_PROCESS,
        NULL,
        NULL,
        &startupInfo,
        &procInfo );
    if ( !bRet )
        goto Error;
#else

//#define TEST_APP64

#ifndef TEST_APP64
    info.CommandLine = L"\"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\Debug\\test1.exe\"";
    info.Exe = L"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\Debug\\test1.exe";
#else
    info.CommandLine =L"\"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\x64\\Debug\\test1.exe\"";
    info.Exe = L"\"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\x64\\Debug\\test1.exe\"";
#endif

    IProcess*   proc = NULL;

    //hr = exec.Attach( 5336, proc );
    hr = exec.Launch( &info, proc );
    if ( FAILED( hr ) )
        goto Error;
#endif

#if 0
    bRet = WaitForDebugEvent( &event, INFINITE );
    if ( !bRet )
        goto Error;
#else
    int stepCount = 0;

    for ( int i = 0; /* doesn't end */ ; i++ )
    {
        hr = exec.WaitForEvent( 1000 );
        if ( FAILED( hr ) )
            goto Error;

        hr = exec.DispatchEvent();
        if ( FAILED( hr ) )
            goto Error;

#if 1
        if ( proc->IsStopped() )
        {
            if ( callback.GetHitBp() )
            {
                stepCount++;

                //11728
                IModule*    mod = NULL;
                UINT_PTR    baseAddr = 0;

                callback.GetModule( mod );
                baseAddr = (UINT_PTR) mod->GetImageBase();
                mod->Release();

                //hr = exec.StepOut( proc, (void*) (baseAddr + 0x00011728) );
                //hr = exec.StepInstruction( proc, true );

                if ( stepCount > 1 )
                    hr = exec.StepInstruction( proc, true, true );
                else
                {
                    //113A5
                    AddressRange    range = { baseAddr + 0x0001137A, baseAddr + 0x000113A5 };
                    hr = exec.StepRange( proc, false, range, true );
                }

                if ( FAILED( hr ) )
                    goto Error;
            }
            else
            {
                hr = exec.Continue( proc, true );
                if ( FAILED( hr ) )
                    goto Error;
            }
        }
#endif

#if 1
        if ( i == 0 )
        {
            IModule*    mod = NULL;
            UINT_PTR    baseAddr = 0;

            callback.GetModule( mod );
            baseAddr = (UINT_PTR) mod->GetImageBase();

            // 0x003C137A, 0x003C1395
            // 1137A, 11395

            //exec.SetBreakpoint( proc, baseAddr + 0x0001138C, 255 );
            exec.SetBreakpoint( proc, baseAddr + 0x0001137A );
            //exec.SetBreakpoint( proc, baseAddr + 0x00011395, 129 );

            mod->Release();
        }
#endif
    }
#endif

Error:
    //exec.Detach( proc );

    // when the debugger goes away, so does the debuggee automatically

    //if ( procInfo.hThread != NULL )
    //{
    //    CloseHandle( procInfo.hThread );
    //}

    //if ( procInfo.hProcess != NULL )
    //{
    //    TerminateProcess( procInfo.hProcess, MAXINT );
    //    CloseHandle( procInfo.hProcess );
    //}

    if ( proc != NULL )
        proc->Release();

    return 0;
}
