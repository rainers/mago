/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


enum MachineResult
{
    MacRes_NotHandled,
    MacRes_HandledContinue,
    MacRes_HandledStopped,
    MacRes_PendingCallbackBP,
    MacRes_PendingCallbackStep,
    MacRes_PendingCallbackEmbeddedBP,
    MacRes_PendingCallbackEmbeddedStep,
};

class IEventCallback;
class IProcess;
class Thread;

class IProbeCallback
{
public:
    virtual ProbeRunMode OnCallProbe( 
        IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange ) = 0;
};


class IMachine
{
public:
    virtual ~IMachine() { }

    virtual void    AddRef() = 0;
    virtual void    Release() = 0;

    virtual void    SetProcess( HANDLE hProcess, Process* process ) = 0;
    virtual void    SetCallback( IProbeCallback* callback ) = 0;
    virtual void    GetPendingCallbackBP( Address& address ) = 0;

    virtual HRESULT ReadMemory( 
        Address address,
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer ) = 0;

    virtual HRESULT WriteMemory( 
        Address address,
        SIZE_T length, 
        SIZE_T& lengthWritten, 
        uint8_t* buffer ) = 0;

    virtual HRESULT SetBreakpoint( Address address ) = 0;
    virtual HRESULT RemoveBreakpoint( Address address ) = 0;
    virtual bool IsBreakpointActive( Address address ) = 0;

    virtual HRESULT SetContinue() = 0;
    virtual HRESULT SetStepOut( Address targetAddress ) = 0;
    virtual HRESULT SetStepInstruction( bool stepIn ) = 0;
    virtual HRESULT SetStepRange( bool stepIn, AddressRange range ) = 0;
    virtual HRESULT CancelStep() = 0;

    virtual HRESULT GetThreadContext( uint32_t threadId, void* context, uint32_t size ) = 0;
    virtual HRESULT SetThreadContext( uint32_t threadId, const void* context, uint32_t size ) = 0;

    virtual ThreadControlProc GetWinSuspendThreadProc() = 0;

    virtual void    OnStopped( uint32_t threadId ) = 0;
    virtual HRESULT OnCreateThread( Thread* thread ) = 0;
    virtual HRESULT OnExitThread( uint32_t threadId ) = 0;
    virtual HRESULT OnException( 
        uint32_t threadId, 
        const EXCEPTION_DEBUG_INFO* exceptRec, 
        MachineResult& result ) = 0;
    virtual HRESULT OnContinue() = 0;
    virtual void    OnDestroyProcess() = 0;

    virtual HRESULT Detach() = 0;
};
