/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

class IProcess;
class Thread;
class IModule;


class IEventCallback
{
public:
    enum EventCode
    {
        Event_None,
        Event_ProcessStart,
        Event_ProcessExit,
        Event_ThreadStart,
        Event_ThreadExit,
        Event_ModuleLoad,
        Event_ModuleUnload,
        Event_OutputString,
        Event_Exception,
    };

    virtual ~IEventCallback() { }

    virtual void AddRef() = 0;
    virtual void Release() = 0;

    virtual void OnProcessStart( IProcess* process ) = 0;
    virtual void OnProcessExit( IProcess* process, DWORD exitCode ) = 0;
    virtual void OnThreadStart( IProcess* process, Thread* thread ) = 0;
    virtual void OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode ) = 0;
    virtual void OnModuleLoad( IProcess* process, IModule* module ) = 0;
    virtual void OnModuleUnload( IProcess* process, Address baseAddr ) = 0;
    virtual void OnOutputString( IProcess* process, const wchar_t* outputString ) = 0;
    virtual void OnLoadComplete( IProcess* process, DWORD threadId ) = 0;

    virtual RunMode OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec ) = 0;
    virtual RunMode OnBreakpoint( IProcess* process, uint32_t threadId, Address address, bool embedded ) = 0;
    virtual void OnStepComplete( IProcess* process, uint32_t threadId ) = 0;
    virtual void OnAsyncBreakComplete( IProcess* process, uint32_t threadId ) = 0;
    virtual void OnError( IProcess* process, HRESULT hrErr, EventCode event ) = 0;

    virtual RunMode OnCallProbe( IProcess* process, uint32_t threadId, Address address ) = 0;
};
