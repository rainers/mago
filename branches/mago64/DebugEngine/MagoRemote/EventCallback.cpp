/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EventCallback.h"


namespace Mago
{
    //----------------------------------------------------------------------------
    //  EventCallback
    //----------------------------------------------------------------------------

    EventCallback::EventCallback()
        :   mRefCount( 0 )
    {
    }

    EventCallback::~EventCallback()
    {
    }

    void EventCallback::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void EventCallback::Release()
    {
        long refCount = InterlockedDecrement( &mRefCount );
        _ASSERT( refCount >= 0 );
        if ( refCount == 0 )
        {
            delete this;
        }
    }

    void EventCallback::OnProcessStart( IProcess* process )
    {
    }

    void EventCallback::OnProcessExit( IProcess* process, DWORD exitCode )
    {
    }

    void EventCallback::OnThreadStart( IProcess* process, Thread* thread )
    {
    }

    void EventCallback::OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
    {
    }

    void EventCallback::OnModuleLoad( IProcess* process, IModule* module )
    {
    }

    void EventCallback::OnModuleUnload( IProcess* process, Address baseAddr )
    {
    }

    void EventCallback::OnOutputString( IProcess* process, const wchar_t* outputString )
    {
    }

    void EventCallback::OnLoadComplete( IProcess* process, DWORD threadId )
    {
    }

    RunMode EventCallback::OnException( 
        IProcess* process, 
        DWORD threadId, 
        bool firstChance, 
        const EXCEPTION_RECORD* exceptRec )
    {
        RunMode mode = RunMode_Run;

        return mode;
    }

    RunMode EventCallback::OnBreakpoint( 
        IProcess* process, 
        uint32_t threadId, 
        Address address, 
        bool embedded )
    {
        RunMode mode = RunMode_Run;

        return mode;
    }

    void EventCallback::OnStepComplete( IProcess* process, uint32_t threadId )
    {
    }

    void EventCallback::OnAsyncBreakComplete( IProcess* process, uint32_t threadId )
    {
    }

    void EventCallback::OnError( IProcess* process, HRESULT hrErr, EventCode event )
    {
        UNREFERENCED_PARAMETER( process );
        UNREFERENCED_PARAMETER( hrErr );
        UNREFERENCED_PARAMETER( event );
    }

    ProbeRunMode EventCallback::OnCallProbe( 
        IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange )
    {
        ProbeRunMode mode = ProbeRunMode_Run;

        return mode;
    }
}
