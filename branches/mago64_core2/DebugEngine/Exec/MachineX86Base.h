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
    virtual ThreadX86Base* GetStoppedThread() = 0;
    virtual HRESULT SetStepperSingleStep( bool enable ) = 0;
    virtual HRESULT SetStepperBreakpoint( Address address, BPCookie cookie ) = 0;
    virtual HRESULT RemoveStepperBreakpoint( Address address, BPCookie cookie ) = 0;
    virtual HRESULT ReadStepperMemory( 
        Address address, 
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer ) = 0;

    virtual RunMode CanStepperStopAtFunction( Address address ) = 0;
};


class MachineX86Base : public IMachine, public IStepperMachine
{
    typedef std::map< uint32_t, ThreadX86Base* >    ThreadMap;
    typedef std::vector< BPCookie >                 CookieVec;

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
    Address         mRestoreBPAddress;
    bool            mStopped;
    uint32_t        mStoppedThreadId;
    bool            mStoppedOnException;
    
    ThreadMap       mThreads;
    ThreadX86Base*  mCurThread;
    uint32_t        mBPRestoringThreadId;
    bool            mIsolatedThread;

    // where most recent exception happened
    // for SS, after stepped instruction; for BP, the BP instruction
    Address         mExceptAddr;
    IEventCallback* mCallback;

    Address         mPendCBAddr;
    CookieVec       mPendCBCookies;

public:
    MachineX86Base();
    ~MachineX86Base();

    virtual void    AddRef();
    virtual void    Release();

    HRESULT         Init();

    virtual void    SetProcess( HANDLE hProcess, uint32_t id, IProcess* process );
    virtual void    SetCallback( IEventCallback* callback );
    virtual void    GetPendingCallbackBP( Address& address );

    virtual HRESULT ReadMemory( 
        Address address, 
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer );

    virtual HRESULT WriteMemory( 
        Address address, 
        SIZE_T length, 
        SIZE_T& lengthWritten, 
        uint8_t* buffer );

    virtual HRESULT SetBreakpoint( Address address );
    virtual HRESULT RemoveBreakpoint( Address address );
    virtual HRESULT IsBreakpointActive( Address address );

    virtual HRESULT SetStepOut( Address targetAddress );
    virtual HRESULT SetStepInstruction( bool stepIn, bool sourceMode );
    virtual HRESULT SetStepRange( bool stepIn, bool sourceMode, AddressRange range );
    virtual HRESULT CancelStep();

    virtual HRESULT GetThreadContext( uint32_t threadId, void* context, uint32_t size );
    virtual HRESULT SetThreadContext( uint32_t threadId, const void* context, uint32_t size );

    virtual void    OnStopped( uint32_t threadId );
    virtual void    OnCreateThread( Thread* thread );
    virtual void    OnExitThread( uint32_t threadId );
    virtual HRESULT OnException( uint32_t threadId, const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    virtual HRESULT OnContinue();
    virtual void    OnDestroyProcess();

protected:
    virtual HRESULT CacheThreadContext() = 0;
    virtual HRESULT FlushThreadContext() = 0;
    // Only call after caching the thread context
    virtual HRESULT ChangeCurrentPC( int32_t byteOffset ) = 0;
    virtual HRESULT SetSingleStep( bool enable ) = 0;
    virtual HRESULT GetCurrentPC( Address& address ) = 0;

    virtual HRESULT SuspendThread( Thread* thread ) = 0;
    virtual HRESULT ResumeThread( Thread* thread ) = 0;

    virtual HRESULT GetThreadContextInternal( uint32_t threadId, void* context, uint32_t size ) = 0;
    virtual HRESULT SetThreadContextInternal( uint32_t threadId, const void* context, uint32_t size ) = 0;

    bool    Stopped();

    // IStepperMachine
    virtual ThreadX86Base* GetStoppedThread();
    virtual HRESULT SetStepperSingleStep( bool enable );
    virtual HRESULT SetStepperBreakpoint( Address address, BPCookie cookie );
    virtual HRESULT RemoveStepperBreakpoint( Address address, BPCookie cookie );
    virtual HRESULT ReadStepperMemory( 
        Address address, 
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer );
    virtual HRESULT WriteStepperMemory( 
        Address address, 
        SIZE_T length, 
        SIZE_T& lengthWritten, 
        uint8_t* buffer );
    virtual RunMode CanStepperStopAtFunction( Address address );

private:
    HRESULT OnContinueInternal();
    HRESULT SetBreakpointInternal( Address address, BPCookie cookie, BPPriority priority );
    HRESULT RemoveBreakpointInternal( Address address, BPCookie cookie, BPPriority priority );

    void LockBreakpoint( Breakpoint* bp );
    void UnlockBreakpoint( Breakpoint* bp );

    HRESULT DispatchSingleStep( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    HRESULT DispatchBreakpoint( Address address, bool embedded, Breakpoint* bp, MachineResult& result );

    HRESULT PatchBreakpoint( Breakpoint* bp );
    HRESULT UnpatchBreakpoint( Breakpoint* bp );

    HRESULT Rewind();
    HRESULT SkipBPOnResume();
    bool AtEmbeddedBP( Address pc );

    HRESULT RestoreBPEnvironment();
    HRESULT SetupRestoreBPEnvironment( Breakpoint* bp, Address pc );
    bool    ShouldIsolateBPRestoringThread( uint32_t threadId, Breakpoint* bp );
    HRESULT IsolateBPRestoringThread();
    HRESULT UnisolateBPRestoringThread();

    ThreadX86Base* FindThread( uint32_t threadId );

    bool    CheckStepperEnded();
};
