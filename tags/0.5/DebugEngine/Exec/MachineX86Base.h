/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Machine.h"

class BPAddressTable;
class CookieList;
class Breakpoint;
class Thread;
template <class T>
class Enumerator;


class ThreadX86Base;
class IStepper;

class IStepperMachine
{
public:
    virtual HRESULT SetStepperSingleStep( bool enable ) = 0;
    virtual HRESULT SetStepperBreakpoint( MachineAddress address, BPCookie cookie ) = 0;
    virtual HRESULT RemoveStepperBreakpoint( MachineAddress address, BPCookie cookie ) = 0;
    virtual HRESULT ReadStepperMemory( 
        MachineAddress address, 
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer ) = 0;
    virtual void    SignalStepComplete() = 0;

    virtual bool    CanStepperStopAtFunction( MachineAddress address ) = 0;
};


class MachineX86Base : public IMachine, public IStepperMachine
{
    typedef std::map< uint32_t, ThreadX86Base* >    ThreadMap;

    enum BPPriority
    {
        BPPri_Low,
        BPPri_High
    };

    LONG            mRefCount;

    // keep a "weak" pointer to the process object, because process owns machine
    IProcess*       mProcess;
    HANDLE          mhProcess;
    uint32_t        mProcessId;
    BPAddressTable* mAddrTable;
    MachineAddress  mRestoreBPAddress;
    bool            mStopped;
    uint32_t        mStoppedThreadId;
    bool            mStoppedOnException;
    
    ThreadMap       mThreads;
    ThreadX86Base*  mCurThread;
    uint32_t        mBPRestoringThreadId;
    bool            mIsolatedThread;

    // where most recent exception happened
    // for SS, after stepped instruction; for BP, the BP instruction
    MachineAddress  mExceptAddr;
    IEventCallback* mCallback;
    int32_t         mSuspendCount;

public:
    MachineX86Base();
    ~MachineX86Base();

    virtual void    AddRef();
    virtual void    Release();

    HRESULT         Init();

    virtual void    SetProcess( HANDLE hProcess, uint32_t id, IProcess* process );
    virtual void    SetCallback( IEventCallback* callback );

    virtual HRESULT ReadMemory( 
        MachineAddress address, 
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer );

    virtual HRESULT WriteMemory( 
        MachineAddress address, 
        SIZE_T length, 
        SIZE_T& lengthWritten, 
        uint8_t* buffer );

    virtual HRESULT SetBreakpoint( MachineAddress address, BPCookie cookie );
    virtual HRESULT RemoveBreakpoint( MachineAddress address, BPCookie cookie );

    virtual HRESULT SetStepOut( Address targetAddress );
    virtual HRESULT SetStepInstruction( bool stepIn, bool sourceMode );
    virtual HRESULT SetStepRange( bool stepIn, bool sourceMode, AddressRange* ranges, int rangeCount );
    virtual HRESULT CancelStep();

    virtual void    OnStopped( uint32_t threadId );
    virtual void    OnCreateThread( Thread* thread );
    virtual void    OnExitThread( uint32_t threadId );
    virtual HRESULT OnException( uint32_t threadId, const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    virtual HRESULT OnContinue();
    virtual void    OnDestroyProcess();

protected:
    virtual HRESULT Rewind( uint32_t threadId, uint32_t byteCount ) = 0;
    virtual HRESULT SetSingleStep( uint32_t threadId, bool enable ) = 0;
    virtual HRESULT GetCurrentPC( uint32_t threadId, MachineAddress& address ) = 0;

    virtual HRESULT SuspendThread( Thread* thread ) = 0;
    virtual HRESULT ResumeThread( Thread* thread ) = 0;

    bool    Stopped();

    // IStepperMachine
    virtual HRESULT SetStepperSingleStep( bool enable );
    virtual HRESULT SetStepperBreakpoint( MachineAddress address, BPCookie cookie );
    virtual HRESULT RemoveStepperBreakpoint( MachineAddress address, BPCookie cookie );
    virtual HRESULT ReadStepperMemory( 
        MachineAddress address, 
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer );
    virtual HRESULT WriteStepperMemory( 
        MachineAddress address, 
        SIZE_T length, 
        SIZE_T& lengthWritten, 
        uint8_t* buffer );
    virtual void    SignalStepComplete();
    virtual bool    CanStepperStopAtFunction( MachineAddress address );

private:
    HRESULT SetBreakpointInternal( MachineAddress address, BPCookie cookie, BPPriority priority );
    HRESULT RemoveBreakpointInternal( MachineAddress address, BPCookie cookie, BPPriority priority );

    void LockBreakpoint( Breakpoint* bp );
    void UnlockBreakpoint( Breakpoint* bp );

    HRESULT DispatchSingleStep( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    HRESULT DispatchBreakpoint( Breakpoint* bp, MachineResult& result );

    HRESULT PatchBreakpoint( Breakpoint* bp );
    HRESULT UnpatchBreakpoint( Breakpoint* bp );

    HRESULT SuspendProcess( Enumerator< Thread* >*  threads );
    HRESULT ResumeProcess( Enumerator< Thread* >*  threads );

    HRESULT RestoreBPEnvironment();
    HRESULT SetupRestoreBPEnvironment( Breakpoint* bp );
    bool    ShouldIsolateBPRestoringThread( uint32_t threadId, Breakpoint* bp );
    HRESULT IsolateBPRestoringThread();
    HRESULT UnisolateBPRestoringThread();

    ThreadX86Base* FindThread( uint32_t threadId );

    void    CheckStepperEnded( bool signalStepComplete = true );
};
