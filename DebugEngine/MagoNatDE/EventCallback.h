/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class Engine;
    class Program;
    class Thread;
    class EventBase;
    class ICoreThread;
    class ICoreModule;


    class EventCallback
    {
        long                    mRefCount;
        RefPtr<Engine>          mEngine;

    public:
        EventCallback( Engine* engine );

        virtual void AddRef();
        virtual void Release();

        virtual void OnProcessStart( DWORD uniquePid );
        virtual void OnProcessExit( DWORD uniquePid, DWORD exitCode );
        virtual void OnThreadStart( DWORD uniquePid, ICoreThread* thread );
        virtual void OnThreadExit( DWORD uniquePid, DWORD threadId, DWORD exitCode );
        virtual void OnModuleLoad( DWORD uniquePid, ICoreModule* module );
        virtual void OnModuleUnload( DWORD uniquePid, Address64 baseAddr );
        virtual void OnOutputString( DWORD uniquePid, const wchar_t* outputString );
        virtual void OnLoadComplete( DWORD uniquePid, DWORD threadId );

        virtual RunMode OnException( 
            DWORD uniquePid, DWORD threadId, bool firstChance, const EXCEPTION_RECORD64*exceptRec);
        virtual RunMode OnBreakpoint( 
            DWORD uniquePid, uint32_t threadId, Address64 address, bool embedded );
        virtual void OnStepComplete( DWORD uniquePid, uint32_t threadId );
        virtual void OnAsyncBreakComplete( DWORD uniquePid, uint32_t threadId );
        virtual void OnError( DWORD uniquePid, HRESULT hrErr, IEventCallback::EventCode event );
        virtual ProbeRunMode OnCallProbe( 
            DWORD uniquePid, uint32_t threadId, Address64 address, AddressRange64& thunkRange );

    private:
        HRESULT SendEvent( EventBase* eventBase, Program* program, Thread* thread );

        // return whether the debuggee should continue
        RunMode OnBreakpointInternal( 
            Program* program, Thread* thread, Address64 address, bool embedded );
        bool FindThunk( 
            MagoST::ISession* session, uint16_t section, uint32_t offset, AddressRange64& thunkRange );

        virtual void OnModuleLoadInternal( DWORD uniquePid, ICoreModule* module );
        virtual void OnModuleUnloadInternal( DWORD uniquePid, Address64 baseAddr );
    };
}
