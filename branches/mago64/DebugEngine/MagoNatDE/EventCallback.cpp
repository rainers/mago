/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EventCallback.h"
#include "Events.h"
#include "Engine.h"
#include "Program.h"
#include "Thread.h"
#include "Module.h"
#include "PendingBreakpoint.h"
#include "BoundBreakpoint.h"
#include "ComEnumWithCount.h"
#include <MagoCVConst.h>


typedef CComEnumWithCount< 
    IEnumDebugBoundBreakpoints2, 
    &IID_IEnumDebugBoundBreakpoints2, 
    IDebugBoundBreakpoint2*, 
    _CopyInterface<IDebugBoundBreakpoint2>, 
    CComMultiThreadModel
> EnumDebugBoundBreakpoints;


namespace Mago
{
    const BPCookie EntryPointCookie = 1;


    EventCallback::EventCallback( Engine* engine )
        :   mRefCount( 0 ),
            mEngine( engine )
    {
    }

    void EventCallback::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void EventCallback::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }


    HRESULT EventCallback::SendEvent( EventBase* eventBase, Program* program, Thread* thread )
    {
        HRESULT hr = S_OK;
        CComPtr<IDebugEngine2>          ad7Engine;
        CComPtr<IDebugProgram2>         ad7Prog;
        IDebugEventCallback2*           ad7Callback = NULL;
        //CComPtr<IDebugEventCallback2>   ad7Callback;
        CComPtr<IDebugThread2>          ad7Thread;

        hr = mEngine->QueryInterface( __uuidof( IDebugEngine2 ), (void**) &ad7Engine );
        _ASSERT( hr == S_OK );

        hr = program->QueryInterface( __uuidof( IDebugProgram2 ), (void**) &ad7Prog );
        _ASSERT( hr == S_OK );

        if ( thread != NULL )
        {
            hr = thread->QueryInterface( __uuidof( IDebugThread2 ), (void**) &ad7Thread );
            _ASSERT( hr == S_OK );
        }

        ad7Callback = program->GetCallback();

        hr = eventBase->Send( ad7Callback, ad7Engine, ad7Prog, ad7Thread );

        return hr;
    }


    void EventCallback::OnProcessStart( IProcess* process )
    {
        OutputDebugStringA( "EventCallback::OnProcessStart\n" );
    }

    void EventCallback::OnProcessExit( IProcess* process, DWORD exitCode )
    {
        OutputDebugStringA( "EventCallback::OnProcessExit\n" );

        HRESULT     hr = S_OK;
        RefPtr<ProgramDestroyEvent> event;
        RefPtr<Program>             prog;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        mEngine->DeleteProgram( prog.Get() );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        event->Init( exitCode );

        SendEvent( event.Get(), prog.Get(), NULL );
    }

    void EventCallback::OnThreadStart( IProcess* process, ::Thread* coreThread )
    {
        OutputDebugStringA( "EventCallback::OnThreadStart\n" );

        HRESULT     hr = S_OK;
        RefPtr<ThreadCreateEvent>   event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        hr = prog->CreateThread( coreThread, thread );
        if ( FAILED( hr ) )
            return;

        hr = prog->AddThread( thread.Get() );
        if ( FAILED( hr ) )
            return;

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        SendEvent( event.Get(), prog.Get(), thread.Get() );
    }

    void EventCallback::OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
    {
        OutputDebugStringA( "EventCallback::OnThreadExit\n" );

        HRESULT     hr = S_OK;
        RefPtr<ThreadDestroyEvent>  event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        if ( !prog->FindThread( threadId, thread ) )
            return;

        prog->DeleteThread( thread.Get() );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        event->Init( exitCode );

        SendEvent( event.Get(), prog.Get(), thread.Get() );
    }

    void EventCallback::OnModuleLoad( IProcess* process, IModule* module )
    {
        mEngine->BeginBindBP();
        OnModuleLoadInternal( process, module );
        mEngine->EndBindBP();
    }

    void EventCallback::OnModuleUnload( IProcess* process, Address baseAddr )
    {
        mEngine->BeginBindBP();
        OnModuleUnloadInternal( process, baseAddr );
        mEngine->EndBindBP();
    }

    void EventCallback::OnModuleLoadInternal( IProcess* process, IModule* coreModule )
    {
        OutputDebugStringA( "EventCallback::OnModuleLoad\n" );

        HRESULT     hr = S_OK;
        RefPtr<ModuleLoadEvent>     event;
        RefPtr<Program>             prog;
        RefPtr<Module>              mod;
        CComPtr<IDebugModule2>      mod2;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        hr = prog->CreateModule( coreModule, mod );
        if ( FAILED( hr ) )
            return;

        hr = prog->AddModule( mod.Get() );
        if ( FAILED( hr ) )
            return;

        hr = mod->LoadSymbols( false );
        // later we'll check if symbols were loaded

        hr = mEngine->BindPendingBPsToModule( mod.Get(), prog.Get() );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        hr = mod->QueryInterface( __uuidof( IDebugModule2 ), (void**) &mod2 );
        if ( FAILED( hr ) )
            return;

        // TODO: message
        event->Init( mod2, NULL, true );

        SendEvent( event.Get(), prog.Get(), NULL );

        //-------------------------

        RefPtr<SymbolSearchEvent>       symEvent;
        CComPtr<IDebugModule3>          mod3;
        MODULE_INFO_FLAGS               flags = 0;
        RefPtr<MagoST::ISession>        session;
        CComBSTR                        name;

        mod->GetName( name );

        hr = mod->QueryInterface( __uuidof( IDebugModule3 ), (void**) &mod3 );
        if ( FAILED( hr ) )
            return;

        hr = MakeCComObject( symEvent );
        if ( FAILED( hr ) )
            return;

        if ( mod->GetSymbolSession( session ) )
            flags |= MIF_SYMBOLS_LOADED;

        symEvent->Init( mod3, name.m_str, flags );

        hr = SendEvent( symEvent.Get(), prog.Get(), NULL );
    }

    void EventCallback::OnModuleUnloadInternal( IProcess* process, Address baseAddr )
    {
        OutputDebugStringA( "EventCallback::OnModuleUnload\n" );

        HRESULT     hr = S_OK;
        RefPtr<ModuleLoadEvent>     event;
        RefPtr<Program>             prog;
        RefPtr<Module>              mod;
        CComPtr<IDebugModule2>      mod2;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        if ( !prog->FindModule( baseAddr, mod ) )
            return;

        prog->DeleteModule( mod.Get() );

        mEngine->UnbindPendingBPsFromModule( mod.Get(), prog.Get() );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        hr = mod->QueryInterface( __uuidof( IDebugModule2 ), (void**) &mod2 );
        if ( FAILED( hr ) )
            return;

        // TODO: message
        event->Init( mod2, NULL, false );

        SendEvent( event.Get(), prog.Get(), NULL );
    }

    void EventCallback::OnOutputString( IProcess* process, const wchar_t* outputString )
    {
        OutputDebugStringA( "EventCallback::OnOutputString\n" );

        HRESULT     hr = S_OK;
        RefPtr<OutputStringEvent>   event;
        RefPtr<Program>             prog;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        event->Init( outputString );

        hr = SendEvent( event.Get(), prog.Get(), NULL );
    }

    // find the entry point that the user defined in their program

    bool FindUserEntryPoint( Module* mainMod, Address& entryPoint )
    {
        HRESULT hr = S_OK;
        RefPtr<MagoST::ISession> session;

        if ( !mainMod->GetSymbolSession( session ) )
            return false;

        MagoST::EnumNamedSymbolsData enumData = { 0 };

        hr = session->FindFirstSymbol( MagoST::SymHeap_GlobalSymbols, "D main", 6, enumData );
        if ( hr != S_OK )
            return false;

        MagoST::SymHandle handle;

        hr = session->GetCurrentSymbol( enumData, handle );
        if ( FAILED( hr ) )
            return false;

        MagoST::SymInfoData infoData = { 0 };
        MagoST::ISymbolInfo* symInfo = NULL;

        hr = session->GetSymbolInfo( handle, infoData, symInfo );
        if ( FAILED( hr ) )
            return false;

        uint16_t section = 0;
        uint32_t offset = 0;

        if ( !symInfo->GetAddressSegment( section ) 
            || !symInfo->GetAddressOffset( offset ) )
            return false;

        uint64_t addr = session->GetVAFromSecOffset( section, offset );
        if ( addr == 0 )
            return false;

        entryPoint = (Address) addr;
        return true;
    }

    void EventCallback::OnLoadComplete( IProcess* process, DWORD threadId )
    {
        OutputDebugStringA( "EventCallback::OnLoadComplete\n" );

        HRESULT     hr = S_OK;
        RefPtr<LoadCompleteEvent>   event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        if ( !prog->FindThread( threadId, thread ) )
            return;

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        hr = SendEvent( event.Get(), prog.Get(), thread.Get() );

        IProcess*   coreProc = prog->GetCoreProcess();

        Address entryPoint = coreProc->GetEntryPoint();

        if ( entryPoint != 0 )
        {
            RefPtr<Module> mod;

            if ( prog->FindModuleContainingAddress( entryPoint, mod ) )
            {
                Address userEntryPoint = 0;
                if ( FindUserEntryPoint( mod, userEntryPoint ) )
                    entryPoint = userEntryPoint;
            }

            hr = prog->SetInternalBreakpoint( entryPoint, EntryPointCookie );
            // if we couldn't set the BP, then don't expect it later
            if ( FAILED( hr ) )
                entryPoint = 0;

            prog->SetEntryPoint( entryPoint );
        }
    }

    RunMode EventCallback::OnException( 
        IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec )
    {
        const DWORD DefaultState = EXCEPTION_STOP_SECOND_CHANCE;

        OutputDebugStringA( "EventCallback::OnException\n" );

        HRESULT     hr = S_OK;
        RefPtr<ExceptionEvent>      event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return RunMode_Break;

        if ( !prog->FindThread( threadId, thread ) )
            return RunMode_Break;

        prog->NotifyException( firstChance, exceptRec );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return RunMode_Break;

        event->Init( prog.Get(), firstChance, exceptRec, prog->CanPassExceptionToDebuggee() );

        DWORD state = DefaultState;
        ExceptionInfo info;
        bool found = false;

        if ( event->GetSearchKey() == ExceptionEvent::Name )
        {
            found = mEngine->FindExceptionInfo( event->GetGUID(), event->GetExceptionName(), info );
        }
        else // search by code
        {
            found = mEngine->FindExceptionInfo( event->GetGUID(), event->GetCode(), info );
        }

        // if not found, then check against the catch-all entry
        if ( !found )
            found = mEngine->FindExceptionInfo( event->GetGUID(), event->GetRootExceptionName(), info );

        if ( found )
        {
            if ( event->GetSearchKey() == ExceptionEvent::Code )
                event->SetExceptionName( info.bstrExceptionName );
            state = info.dwState;
        }

        if ( (  firstChance && ( state & EXCEPTION_STOP_FIRST_CHANCE ) ) ||
             ( !firstChance && ( state & EXCEPTION_STOP_SECOND_CHANCE ) ) )
        {
            hr = SendEvent( event.Get(), prog.Get(), thread.Get() );
            return RunMode_Break;
        }
        else
        {
            RefPtr<MessageTextEvent>    msgEvent;
            CComBSTR                    desc;

            hr = MakeCComObject( msgEvent );
            if ( FAILED( hr ) )
                return RunMode_Run;

            hr = event->GetExceptionDescription( &desc );
            if ( FAILED( hr ) )
                return RunMode_Run;

            desc.Append( L"\n" );

            msgEvent->Init( MT_REASON_EXCEPTION, desc );

            hr = SendEvent( msgEvent.Get(), prog.Get(), thread.Get() );
            return RunMode_Run;
        }
    }

    RunMode EventCallback::OnBreakpointInternal( 
        Program* prog, Thread* thread, Address address, bool embedded )
    {
        HRESULT     hr = S_OK;

        if ( embedded )
        {
            RefPtr<EmbeddedBreakpointEvent> event;

            hr = MakeCComObject( event );
            if ( FAILED( hr ) )
                return RunMode_Run;

            event->Init( prog );

            hr = SendEvent( event, prog, thread );
            if ( FAILED( hr ) )
                return RunMode_Run;

            return RunMode_Break;
        }
        else
        {
            std::vector< BPCookie > iter;
            int         stoppingBPs = 0;

            hr = prog->EnumBPCookies( address, iter );
            if ( FAILED( hr ) )
                return RunMode_Run;

            for ( std::vector< BPCookie >::iterator it = iter.begin(); it != iter.end(); it++ )
            {
                if ( *it != EntryPointCookie )
                {
                    stoppingBPs++;
                }
            }

            if ( stoppingBPs > 0 )
            {
                RefPtr<BreakpointEvent>     event;
                CComPtr<IEnumDebugBoundBreakpoints2>    enumBPs;

                hr = MakeCComObject( event );
                if ( FAILED( hr ) )
                    return RunMode_Run;

                InterfaceArray<IDebugBoundBreakpoint2>  array( stoppingBPs );

                if ( array.Get() == NULL )
                    return RunMode_Run;

                int i = 0;
                for ( std::vector< BPCookie >::iterator it = iter.begin(); it != iter.end(); it++ )
                {
                    if ( *it != EntryPointCookie )
                    {
                        IDebugBoundBreakpoint2* bp = (IDebugBoundBreakpoint2*) *it;

                        _ASSERT( i < stoppingBPs );
                        array[i] = bp;
                        array[i]->AddRef();
                        i++;
                    }
                }

                hr = MakeEnumWithCount<EnumDebugBoundBreakpoints>( array, &enumBPs );
                if ( FAILED( hr ) )
                    return RunMode_Run;

                event->Init( enumBPs );

                hr = SendEvent( event, prog, thread );
                if ( FAILED( hr ) )
                    return RunMode_Run;

                return RunMode_Break;
            }
            else if ( (prog->GetEntryPoint() != 0) && (address == prog->GetEntryPoint()) )
            {
                RefPtr<EntryPointEvent> entryPointEvent;

                hr = MakeCComObject( entryPointEvent );
                if ( FAILED( hr ) )
                    return RunMode_Run;

                hr = SendEvent( entryPointEvent, prog, thread );
                if ( FAILED( hr ) )
                    return RunMode_Run;

                return RunMode_Break;
            }
        }

        return RunMode_Run;
    }

    RunMode EventCallback::OnBreakpoint( 
        IProcess* process, uint32_t threadId, Address address, bool embedded )
    {
        OutputDebugStringA( "EventCallback::OnBreakpoint\n" );

        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;
        RunMode                     runMode = RunMode_Break;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return RunMode_Run;

        if ( !prog->FindThread( threadId, thread ) )
            return RunMode_Run;

        runMode = OnBreakpointInternal( prog, thread, address, embedded );

        // If we stopped because of a regular BP before reaching the entry point, 
        // then we shouldn't stop at the entry point

        // Test if we're at the entrypoint, in addition to whether we stopped, because 
        // we could have decided to keep going even though we're at the entry point

        Address entryPoint = prog->GetEntryPoint();

        if ( (entryPoint != 0) && ((runMode == RunMode_Break) || (address == entryPoint)) )
        {
            prog->RemoveInternalBreakpoint( entryPoint, EntryPointCookie );
            prog->SetEntryPoint( 0 );
        }

        return runMode;
    }

    void EventCallback::OnStepComplete( IProcess* process, uint32_t threadId )
    {
        OutputDebugStringA( "EventCallback::OnStepComplete\n" );

        HRESULT hr = S_OK;
        RefPtr<StepCompleteEvent>   event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        if ( !prog->FindThread( threadId, thread ) )
            return;

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        hr = SendEvent( event.Get(), prog.Get(), thread.Get() );
    }

    void EventCallback::OnAsyncBreakComplete( IProcess* process, uint32_t threadId )
    {
    }

    void EventCallback::OnError( IProcess* process, HRESULT hrErr, IEventCallback::EventCode event )
    {
    }

    ProbeRunMode EventCallback::OnCallProbe( 
        IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange )
    {
        OutputDebugStringA( "EventCallback::OnCallProbe\n" );

        RefPtr<Program>             prog;
        RefPtr<Module>              mod;
        RefPtr<MagoST::ISession>    session;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return ProbeRunMode_Run;

        if ( !prog->FindModuleContainingAddress( address, mod ) )
            return ProbeRunMode_Run;

        if ( !mod->GetSymbolSession( session ) )
            return ProbeRunMode_Run;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        sec = session->GetSecOffsetFromVA( address, offset );
        if ( sec == 0 )
            return ProbeRunMode_Run;

        MagoST::LineNumber  line = { 0 };

        if ( !session->FindLine( sec, offset, line ) )
        {
            if ( FindThunk( session, sec, offset, thunkRange ) )
                return ProbeRunMode_WalkThunk;

            return ProbeRunMode_Run;
        }

        return ProbeRunMode_Break;
    }

    bool EventCallback::FindThunk( 
        MagoST::ISession* session, uint16_t section, uint32_t offset, AddressRange& thunkRange )
    {
        HRESULT hr = S_OK;
        MagoST::SymHandle symHandle;

        hr = session->FindOuterSymbolByAddr( MagoST::SymHeap_GlobalSymbols, section, offset, symHandle );
        if ( hr != S_OK )
        {
            hr = session->FindOuterSymbolByAddr( 
                MagoST::SymHeap_StaticSymbols, section, offset, symHandle );
        }
        if ( hr == S_OK )
        {
            MagoST::SymInfoData infoData;
            MagoST::ISymbolInfo* symInfo = NULL;

            hr = session->GetSymbolInfo( symHandle, infoData, symInfo );
            if ( hr == S_OK )
            {
                if ( symInfo->GetSymTag() == MagoST::SymTagThunk )
                {
                    uint32_t length = 0;
                    symInfo->GetAddressOffset( offset );
                    symInfo->GetAddressSegment( section );
                    symInfo->GetLength( length );

                    uint64_t addr = session->GetVAFromSecOffset( section, offset );
                    thunkRange.Begin = (Address) addr;
                    thunkRange.End = (Address) addr + length - 1;
                    return true;
                }
            }
        }

        return false;
    }
}
