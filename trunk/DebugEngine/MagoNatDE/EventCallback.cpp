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


typedef CComEnumWithCount< 
    IEnumDebugBoundBreakpoints2, 
    &IID_IEnumDebugBoundBreakpoints2, 
    IDebugBoundBreakpoint2*, 
    _CopyInterface<IDebugBoundBreakpoint2>, 
    CComMultiThreadModel
> EnumDebugBoundBreakpoints;


namespace Mago
{
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

    void EventCallback::OnModuleLoad( IProcess* process, IModule* coreModule )
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

    void EventCallback::OnModuleUnload( IProcess* process, Address baseAddr )
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
    }

    bool EventCallback::OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec )
    {
        const DWORD DefaultState = EXCEPTION_STOP_SECOND_CHANCE;

        OutputDebugStringA( "EventCallback::OnException\n" );

        HRESULT     hr = S_OK;
        RefPtr<ExceptionEvent>      event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return false;

        if ( !prog->FindThread( threadId, thread ) )
            return false;

        prog->NotifyException( firstChance, exceptRec );

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return false;

        event->Init( prog.Get(), firstChance, exceptRec, prog->CanPassExceptionToDebuggee() );

        DWORD state = DefaultState;
        if ( event->GetGUID() == GetEngineId() )
        {
            ExceptionInfo info;
            if ( mEngine->FindExceptionInfo( event->GetGUID(), event->GetExcpetionName(), info ) )
                state = info.dwState;
            // TODO: if not found, then check against the "D Exceptions" entry
        }
        else
        {
            ExceptionInfo info;
            if ( mEngine->FindExceptionInfo( event->GetGUID(), event->GetCode(), info ) )
            {
                event->SetExceptionName( info.bstrExceptionName );
                state = info.dwState;
            }
        }

        if ( (  firstChance && ( state & EXCEPTION_STOP_FIRST_CHANCE ) ) ||
             ( !firstChance && ( state & EXCEPTION_STOP_SECOND_CHANCE ) ) )
        {
            hr = SendEvent( event.Get(), prog.Get(), thread.Get() );
            return false;
        }
        else
        {
            RefPtr<MessageTextEvent>    msgEvent;
            CComBSTR                    desc;

            hr = MakeCComObject( msgEvent );
            if ( FAILED( hr ) )
                return true;

            hr = event->GetExceptionDescription( &desc );
            if ( FAILED( hr ) )
                return true;

            desc.Append( L"\n" );

            msgEvent->Init( MT_REASON_EXCEPTION, desc );

            hr = SendEvent( msgEvent.Get(), prog.Get(), thread.Get() );
            return true; // wants to continue
        }
    }

    void EventCallback::OnBreakpoint( IProcess* process, uint32_t threadId, Address address, Enumerator<BPCookie>* iter )
    {
        OutputDebugStringA( "EventCallback::OnBreakpoint\n" );

        HRESULT     hr = S_OK;
        RefPtr<BreakpointEvent>     event;
        RefPtr<Program>             prog;
        RefPtr<Thread>              thread;
        CComPtr<IEnumDebugBoundBreakpoints2>    enumBPs;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return;

        if ( !prog->FindThread( threadId, thread ) )
            return;

        hr = MakeCComObject( event );
        if ( FAILED( hr ) )
            return;

        InterfaceArray<IDebugBoundBreakpoint2>  array( iter->GetCount() );

        if ( array.Get() == NULL )
            return;

        for ( int i = 0; iter->MoveNext(); i++ )
        {
            IDebugBoundBreakpoint2* bp = (IDebugBoundBreakpoint2*) iter->GetCurrent();

            array[i] = bp;
            array[i]->AddRef();
        }

        hr = MakeEnumWithCount<EnumDebugBoundBreakpoints>( array, &enumBPs );
        if ( FAILED( hr ) )
            return;

        event->Init( enumBPs );

        hr = SendEvent( event.Get(), prog.Get(), thread.Get() );
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

    void EventCallback::OnError( IProcess* process, HRESULT hrErr, EventCode event )
    {
    }

    bool EventCallback::CanStepInFunction( IProcess* process, Address address )
    {
        OutputDebugStringA( "EventCallback::CanStepInFunction\n" );

        RefPtr<Program>             prog;
        RefPtr<Module>              mod;
        RefPtr<MagoST::ISession>    session;

        if ( !mEngine->FindProgram( process->GetId(), prog ) )
            return false;

        if ( !prog->FindModuleContainingAddress( address, mod ) )
            return false;

        if ( !mod->GetSymbolSession( session ) )
            return false;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        sec = session->GetSecOffsetFromVA( address, offset );
        if ( sec == 0 )
            return false;

        MagoST::LineNumber  line = { 0 };

        if ( !session->FindLine( sec, offset, line ) )
            return false;

        return true;
    }
}
