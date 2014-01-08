/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class EventCallback : public IEventCallback
    {
        long            mRefCount;

    public:
        EventCallback();
        ~EventCallback();

        virtual void AddRef();
        virtual void Release();

        virtual void OnProcessStart( IProcess* process );
        virtual void OnProcessExit( IProcess* process, DWORD exitCode );
        virtual void OnThreadStart( IProcess* process, Thread* thread );
        virtual void OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode );
        virtual void OnModuleLoad( IProcess* process, IModule* module );
        virtual void OnModuleUnload( IProcess* process, Address baseAddr );
        virtual void OnOutputString( IProcess* process, const wchar_t* outputString );
        virtual void OnLoadComplete( IProcess* process, DWORD threadId );

        virtual RunMode OnException( 
            IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec );

        virtual RunMode OnBreakpoint( 
            IProcess* process, uint32_t threadId, Address address, bool embedded );

        virtual void OnStepComplete( IProcess* process, uint32_t threadId );
        virtual void OnAsyncBreakComplete( IProcess* process, uint32_t threadId );
        virtual void OnError( IProcess* process, HRESULT hrErr, EventCode event );

        virtual ProbeRunMode OnCallProbe( 
            IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange );
    };
}
