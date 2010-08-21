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


    class EventCallback : public IEventCallback
    {
        long    mRefCount;

        RefPtr<Engine>          mEngine;

    public:
        EventCallback( Engine* engine );

        virtual void AddRef();
        virtual void Release();

        virtual void OnProcessStart( IProcess* process );
        virtual void OnProcessExit( IProcess* process, DWORD exitCode );
        virtual void OnThreadStart( IProcess* process, ::Thread* thread );
        virtual void OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode );
        virtual void OnModuleLoad( IProcess* process, IModule* module );
        virtual void OnModuleUnload( IProcess* process, Address baseAddr );
        virtual void OnOutputString( IProcess* process, const wchar_t* outputString );
        virtual void OnLoadComplete( IProcess* process, DWORD threadId );
        virtual void OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec );
        virtual void OnBreakpoint( IProcess* process, uint32_t threadId, Address address, Enumerator< BPCookie >* iter );
        virtual void OnStepComplete( IProcess* process, uint32_t threadId );
        virtual void OnAsyncBreakComplete( IProcess* process, uint32_t threadId );
        virtual void OnError( IProcess* process, HRESULT hrErr, EventCode event );

        virtual bool CanStepInFunction( IProcess* process, Address address );

    private:
        HRESULT SendEvent( EventBase* eventBase, Program* program, Thread* thread );
    };
}
