/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


// During Breakpoint Recovery, we have overlapping results. BP Recovery either
// handles the BP exception and wants us to continue running, or doesn't handle
// it. If we have a stepper, then when it is notified of a BP exception, it
// can return any result. We have to define an ordering to take care of this
// conflict.
// If someone wants to stop and another wants to continue, then we should stop.
// If someone wants to continue and another doesn't handle it, then we should 
// continue. Use the GetMoreImportant function.

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

inline MachineResult GetMoreImportant( MachineResult a, MachineResult b )
{
    C_ASSERT( MacRes_HandledStopped > MacRes_HandledContinue );
    C_ASSERT( MacRes_HandledContinue > MacRes_NotHandled );

    if ( a > b )
        return a;
    return b;
}

class IEventCallback;
class IProcess;
class Thread;


class IMachine
{
public:
    virtual ~IMachine() { }

    virtual void    AddRef() = 0;
    virtual void    Release() = 0;

    virtual void    SetProcess( HANDLE hProcess, uint32_t id, IProcess* process ) = 0;
    virtual void    SetCallback( IEventCallback* callback ) = 0;
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
    virtual HRESULT IsBreakpointActive( Address address ) = 0;

    virtual HRESULT SetStepOut( Address targetAddress ) = 0;
    virtual HRESULT SetStepInstruction( bool stepIn, bool sourceMode ) = 0;
    virtual HRESULT SetStepRange( bool stepIn, bool sourceMode, AddressRange range ) = 0;
    virtual HRESULT CancelStep() = 0;

    virtual HRESULT GetThreadContext( uint32_t threadId, void* context, uint32_t size ) = 0;
    virtual HRESULT SetThreadContext( uint32_t threadId, const void* context, uint32_t size ) = 0;

    virtual ThreadControlProc GetWinSuspendThreadProc() = 0;

    virtual void    OnStopped( uint32_t threadId ) = 0;
    virtual void    OnCreateThread( Thread* thread ) = 0;
    virtual void    OnExitThread( uint32_t threadId ) = 0;
    virtual HRESULT OnException( uint32_t threadId, const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result ) = 0;
    virtual HRESULT OnContinue() = 0;
    virtual void    OnDestroyProcess() = 0;
};
