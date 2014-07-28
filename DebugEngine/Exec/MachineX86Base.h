/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Machine.h"

class BPAddressTable;
class Breakpoint;
class Thread;
class ThreadX86Base;
struct RangeStep;
enum ExpectedCode;
enum CpuSizeMode;
enum InstructionType;
enum Motion;


class MachineX86Base : public IMachine
{
    typedef std::map< uint32_t, ThreadX86Base* >    ThreadMap;
    typedef UniquePtr<RangeStep>                    RangeStepPtr;

    LONG            mRefCount;

    // keep a weak pointer to the process object, because process owns machine
    Process*        mProcess;
    HANDLE          mhProcess;
    BPAddressTable* mAddrTable;
    uint32_t        mStoppedThreadId;
    bool            mStoppedOnException;
    bool            mStopped;

    ThreadMap       mThreads;
    ThreadX86Base*  mCurThread;
    uint32_t        mIsolatedThreadId;
    bool            mIsolatedThread;

    IProbeCallback* mCallback;
    Address         mPendCBAddr;

public:
    MachineX86Base();
    ~MachineX86Base();

    virtual void    AddRef();
    virtual void    Release();

    HRESULT         Init();

    virtual void    SetProcess( HANDLE hProcess, Process* process );
    virtual void    SetCallback( IProbeCallback* callback );
    virtual void    GetPendingCallbackBP( Address& address );

    virtual HRESULT ReadMemory( 
        Address address, 
        uint32_t length, 
        uint32_t& lengthRead, 
        uint32_t& lengthUnreadable, 
        uint8_t* buffer );

    virtual HRESULT WriteMemory( 
        Address address, 
        uint32_t length, 
        uint32_t& lengthWritten, 
        uint8_t* buffer );

    virtual HRESULT SetBreakpoint( Address address );
    virtual HRESULT RemoveBreakpoint( Address address );
    virtual bool IsBreakpointActive( Address address );

    virtual HRESULT SetContinue();
    virtual HRESULT SetStepOut( Address targetAddress );
    virtual HRESULT SetStepInstruction( bool stepIn );
    virtual HRESULT SetStepRange( bool stepIn, AddressRange range );
    virtual HRESULT CancelStep();

    virtual HRESULT GetThreadContext( 
        uint32_t threadId, 
        uint32_t features,
        uint64_t extFeatures,
        void* context, 
        uint32_t size );
    virtual HRESULT SetThreadContext( uint32_t threadId, const void* context, uint32_t size );

    virtual void    OnStopped( uint32_t threadId );
    virtual HRESULT OnCreateThread( Thread* thread );
    virtual HRESULT OnExitThread( uint32_t threadId );
    virtual HRESULT OnException( uint32_t threadId, const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    virtual HRESULT OnContinue();
    virtual void    OnDestroyProcess();

    virtual HRESULT Detach();

protected:
    virtual bool Is64Bit() = 0;
    virtual HRESULT CacheThreadContext() = 0;
    virtual HRESULT FlushThreadContext() = 0;
    // Only call after caching the thread context
    virtual HRESULT ChangeCurrentPC( int32_t byteOffset ) = 0;
    virtual HRESULT SetSingleStep( bool enable ) = 0;
    virtual HRESULT ClearSingleStep() = 0;
    virtual HRESULT GetCurrentPC( Address& address ) = 0;
    virtual HRESULT GetReturnAddress( Address& address ) = 0;

    virtual HRESULT SuspendThread( Thread* thread ) = 0;
    virtual HRESULT ResumeThread( Thread* thread ) = 0;

    virtual HRESULT GetThreadContextInternal( 
        uint32_t threadId, 
        uint32_t features, 
        uint64_t extFeatures, 
        void* context, 
        uint32_t size ) = 0;
    virtual HRESULT SetThreadContextInternal( uint32_t threadId, const void* context, uint32_t size ) = 0;

    bool    Stopped();
    ThreadX86Base* GetStoppedThread();
    HANDLE  GetProcessHandle();

private:
    HRESULT ReadCleanMemory( 
        Address address, 
        uint32_t length, 
        uint32_t& lengthRead, 
        uint32_t& lengthUnreadable, 
        uint8_t* buffer );

    HRESULT WriteCleanMemory( 
        Address address, 
        uint32_t length, 
        uint32_t& lengthWritten, 
        uint8_t* buffer );

    HRESULT SetBreakpointInternal( Address address, bool user );
    HRESULT RemoveBreakpointInternal( Address address, bool user );

    HRESULT PatchBreakpoint( Breakpoint* bp );
    HRESULT UnpatchBreakpoint( Breakpoint* bp );
    HRESULT TempPatchBreakpoint( Breakpoint* bp );
    HRESULT TempUnpatchBreakpoint( Breakpoint* bp );

    HRESULT ReadInstruction( 
        Address address, 
        InstructionType& type, 
        int& size );

    HRESULT PassBP( Address pc, 
        InstructionType instType, 
        int instLen, 
        int notifier, 
        Motion motion,
        RangeStepPtr& rangeStep );
    HRESULT PassBPSimple( 
        Address pc, 
        int instLen, 
        int notifier, 
        Motion motion,
        RangeStepPtr& rangeStep );
    HRESULT PassBPCall( 
        Address pc, 
        int instLen, 
        int notifier, 
        Motion motion,
        RangeStepPtr& rangeStep );
    HRESULT PassBPSyscall( 
        Address pc, 
        int instLen, 
        int notifier, 
        Motion motion,
        RangeStepPtr& rangeStep );
    HRESULT PassBPRepString( 
        Address pc, 
        int instLen, 
        int notifier, 
        Motion motion,
        RangeStepPtr& rangeStep );

    HRESULT SetupInstructionStep( 
        Address pc, 
        int instLen, 
        int notifier,
        ExpectedCode code,
        bool unpatch,
        bool resumeThreads,
        bool clearTF,
        Motion motion,
        RangeStepPtr& rangeStep
        );

    HRESULT DontPassBP( 
        Motion motion, 
        Address pc, 
        InstructionType instType, 
        int instLen, 
        int notifier, 
        RangeStepPtr& rangeStep );
    HRESULT DontPassBPSimple( 
        Motion motion, 
        Address pc, 
        int instLen, 
        int notifier, 
        RangeStepPtr& rangeStep );
    HRESULT DontPassBPCall( 
        Motion motion, 
        Address pc, 
        int instLen, 
        int notifier, 
        RangeStepPtr& rangeStep );
    HRESULT DontPassBPSyscall( 
        Motion motion, 
        Address pc, 
        int instLen, 
        int notifier, 
        RangeStepPtr& rangeStep );
    HRESULT DontPassBPRepString(
        Motion motion, 
        Address pc, 
        int instLen, 
        int notifier,
        RangeStepPtr& rangeStep );

    HRESULT SetStepInstructionCore( Motion motion, RangeStepPtr& rangeStep, int notifier );

    HRESULT SuspendOtherThreads( UINT32 threadId );
    HRESULT ResumeOtherThreads( UINT32 threadId );

    HRESULT DispatchSingleStep( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    HRESULT DispatchBreakpoint( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    HRESULT RunAllActions( bool cancel, MachineResult& result );
    HRESULT RunNotifierAction( 
        int notifier, 
        Motion motion, 
        RangeStepPtr& rangeStep, 
        MachineResult& result );
    HRESULT RunNotifyCheckCall( Motion motion, RangeStepPtr& rangeStep, MachineResult& result );
    HRESULT RunNotifyCheckRange( Motion motion, RangeStepPtr& rangeStep, MachineResult& result );
    HRESULT RunNotifyStepOut( Motion motion, RangeStepPtr& rangeStep, MachineResult& result );

    HRESULT Rewind();
    Breakpoint* FindBP( Address address );
    ThreadX86Base* FindThread( uint32_t threadId );
    bool AtEmbeddedBP( Address address, Breakpoint* bp );

    // like the public SetStepRange, but uses the range info we already have
    HRESULT SetStepRange( bool stepIn, RangeStepPtr& rangeStep );
};
