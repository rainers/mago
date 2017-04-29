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
#include "ICoreProcess.h"
#include "DRuntime.h"
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


    void EventCallback::OnProcessStart( DWORD uniquePid )
    {
        Log::LogMessage( "EventCallback::OnProcessStart\n" );
    }

    void EventCallback::OnProcessExit( DWORD uniquePid, DWORD exitCode )
    {
        Log::LogMessage( "EventCallback::OnProcessExit\n" );

        HRESULT     hr = S_OK;
        RefPtr<ProgramDestroyEvent> event;
        RefPtr<Program>             prog;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
            return;

        mEngine->DeleteProgram( prog.Get() );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        event->Init( exitCode );

        SendEvent( event.Get(), prog.Get(), NULL );
    }

    void EventCallback::OnThreadStart( DWORD uniquePid, ICoreThread* coreThread )
    {
        Log::LogMessage( "EventCallback::OnThreadStart\n" );

        HRESULT     hr = S_OK;
        RefPtr<ThreadCreateEvent>   event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
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

    void EventCallback::OnThreadExit( DWORD uniquePid, DWORD threadId, DWORD exitCode )
    {
        Log::LogMessage( "EventCallback::OnThreadExit\n" );

        HRESULT     hr = S_OK;
        RefPtr<ThreadDestroyEvent>  event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
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

    void EventCallback::OnModuleLoad( DWORD uniquePid, ICoreModule* module )
    {
        mEngine->BeginBindBP();
        OnModuleLoadInternal( uniquePid, module );
        mEngine->EndBindBP();
    }

    void EventCallback::OnModuleUnload( DWORD uniquePid, Address64 baseAddr )
    {
        mEngine->BeginBindBP();
        OnModuleUnloadInternal( uniquePid, baseAddr );
        mEngine->EndBindBP();
    }

    void EventCallback::OnModuleLoadInternal( DWORD uniquePid, ICoreModule* coreModule )
    {
        Log::LogMessage( "EventCallback::OnModuleLoad\n" );

        HRESULT     hr = S_OK;
        RefPtr<ModuleLoadEvent>     event;
        RefPtr<Program>             prog;
        RefPtr<Module>              mod;
        CComPtr<IDebugModule2>      mod2;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
            return;

        hr = prog->CreateModule( coreModule, mod );
        if ( FAILED( hr ) )
            return;

        hr = prog->AddModule( mod.Get() );
        if ( FAILED( hr ) )
            return;

        hr = mod->LoadSymbols( false );
        // later we'll check if symbols were loaded

        prog->UpdateAAVersion( mod.Get() );

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
        CComBSTR                        msg;

        hr = mod->QueryInterface( __uuidof( IDebugModule3 ), (void**) &mod3 );
        if ( FAILED( hr ) )
            return;

        hr = MakeCComObject( symEvent );
        if ( FAILED( hr ) )
            return;

        if ( mod->GetSymbolSession( session ) )
            flags |= MIF_SYMBOLS_LOADED;

        mod->GetPath( msg );
        if( flags & MIF_SYMBOLS_LOADED )
            msg.Append( L" loaded, symbols found.");
        else
            msg.Append( L" loaded, no symbols.");

        symEvent->Init( mod3, msg.m_str, flags );

        hr = SendEvent( symEvent.Get(), prog.Get(), NULL );
    }

    void EventCallback::OnModuleUnloadInternal( DWORD uniquePid, Address64 baseAddr )
    {
        Log::LogMessage( "EventCallback::OnModuleUnload\n" );

        HRESULT     hr = S_OK;
        RefPtr<ModuleLoadEvent>     event;
        RefPtr<Program>             prog;
        RefPtr<Module>              mod;
        CComPtr<IDebugModule2>      mod2;
        CComBSTR                    msg;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
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

        mod->GetPath( msg );
        msg.Append( L" unloaded.");

        event->Init( mod2, msg.m_str, false );

        SendEvent( event.Get(), prog.Get(), NULL );
    }

    void EventCallback::OnOutputString( DWORD uniquePid, const wchar_t* outputString )
    {
        Log::LogMessage( "EventCallback::OnOutputString\n" );

        HRESULT     hr = S_OK;
        RefPtr<OutputStringEvent>   event;
        RefPtr<Program>             prog;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
            return;

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        event->Init( outputString );

        hr = SendEvent( event.Get(), prog.Get(), NULL );
    }

    // find the entry point that the user defined in their program

    bool FindGlobalSymbolAddress(Module* mainMod, const char* symbol, Address64& symaddr)
    {
        HRESULT hr = S_OK;
        RefPtr<MagoST::ISession> session;

        if ( !mainMod->GetSymbolSession( session ) )
            return false;

        MagoST::EnumNamedSymbolsData enumData = { 0 };

        hr = session->FindFirstSymbol( MagoST::SymHeap_GlobalSymbols, symbol, strlen(symbol), enumData );
        if ( hr != S_OK )
            hr = session->FindFirstSymbol( MagoST::SymHeap_StaticSymbols, symbol, strlen(symbol), enumData );
        if ( hr != S_OK )
            hr = session->FindFirstSymbol( MagoST::SymHeap_PublicSymbols, symbol, strlen(symbol), enumData );
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

        symaddr = (Address64) addr;
        return true;
    }

    bool FindUserEntryPoint( Module* mainMod, Address64& entryPoint )
    {
        return FindGlobalSymbolAddress( mainMod, "D main", entryPoint );
    }

    void EventCallback::OnLoadComplete( DWORD uniquePid, DWORD threadId )
    {
        Log::LogMessage( "EventCallback::OnLoadComplete\n" );

        HRESULT     hr = S_OK;
        RefPtr<LoadCompleteEvent>   event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
            return;

        if ( !prog->FindThread( threadId, thread ) )
            return;

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        hr = SendEvent( event.Get(), prog.Get(), thread.Get() );

        Address64 entryPoint = prog->FindEntryPoint();

        if ( entryPoint != 0 )
        {
            RefPtr<Module> mod;

            if ( prog->FindModuleContainingAddress( entryPoint, mod ) )
            {
                Address64 userEntryPoint = 0;
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
        DWORD uniquePid, DWORD threadId, bool firstChance, const EXCEPTION_RECORD64* exceptRec )
    {
        const DWORD DefaultState = EXCEPTION_STOP_SECOND_CHANCE;

        Log::LogMessage( "EventCallback::OnException\n" );

        HRESULT     hr = S_OK;
        RefPtr<ExceptionEvent>      event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
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
            {
                wchar_t name[256] = L"";
                _swprintf_p( name, _countof( name ), L"0x%08x: %s", event->GetCode(), (BSTR) info.bstrExceptionName );
                event->SetExceptionName( name );
            }
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
        Program* prog, Thread* thread, Address64 address, bool embedded )
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
        DWORD uniquePid, uint32_t threadId, Address64 address, bool embedded )
    {
        Log::LogMessage( "EventCallback::OnBreakpoint\n" );

        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;
        RunMode                     runMode = RunMode_Break;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
            return RunMode_Run;

        if ( !prog->FindThread( threadId, thread ) )
            return RunMode_Run;

        runMode = OnBreakpointInternal( prog, thread, address, embedded );

        // If we stopped because of a regular BP before reaching the entry point, 
        // then we shouldn't stop at the entry point

        // Test if we're at the entrypoint, in addition to whether we stopped, because 
        // we could have decided to keep going even though we're at the entry point

        Address64 entryPoint = prog->GetEntryPoint();

        if ( (entryPoint != 0) && ((runMode == RunMode_Break) || (address == entryPoint)) )
        {
            prog->RemoveInternalBreakpoint( entryPoint, EntryPointCookie );
            prog->SetEntryPoint( 0 );
        }

        return runMode;
    }

    void EventCallback::OnStepComplete( DWORD uniquePid, uint32_t threadId )
    {
        Log::LogMessage( "EventCallback::OnStepComplete\n" );

        HRESULT hr = S_OK;
        RefPtr<StepCompleteEvent>   event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
            return;

        if ( !prog->FindThread( threadId, thread ) )
            return;

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        hr = SendEvent( event.Get(), prog.Get(), thread.Get() );
    }

    void EventCallback::OnAsyncBreakComplete( DWORD uniquePid, uint32_t threadId )
    {
    }

    void EventCallback::OnError( DWORD uniquePid, HRESULT hrErr, IEventCallback::EventCode event )
    {
    }

    ProbeRunMode EventCallback::OnCallProbe( 
        DWORD uniquePid, uint32_t threadId, Address64 address, AddressRange64& thunkRange )
    {
        Log::LogMessage( "EventCallback::OnCallProbe\n" );

        RefPtr<Program>             prog;
        RefPtr<Module>              mod;
        RefPtr<MagoST::ISession>    session;

        if ( !mEngine->FindProgram( uniquePid, prog ) )
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
        MagoST::ISession* session, uint16_t section, uint32_t offset, AddressRange64& thunkRange )
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
                    if ( addr == 0 )
                        return false;
                    thunkRange.Begin = (Address64) addr;
                    thunkRange.End = (Address64) addr + length - 1;
                    return true;
                }
            }
        }

        return false;
    }
}
