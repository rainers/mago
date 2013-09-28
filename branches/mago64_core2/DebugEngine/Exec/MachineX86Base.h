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
enum ExpectedCode;
enum CpuSizeMode;
enum InstructionType;
enum Motion;


class MachineX86Base : public IMachine
{
    typedef std::map< uint32_t, ThreadX86Base* >    ThreadMap;

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
    bool            mIsolatedThread;

    IEventCallback* mCallback;
    Address         mPendCBAddr;

public:
    MachineX86Base();
    ~MachineX86Base();

    virtual void    AddRef();
    virtual void    Release();

    HRESULT         Init();

    virtual void    SetProcess( HANDLE hProcess, uint32_t id, Process* process );
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
    virtual bool IsBreakpointActive( Address address );

    virtual HRESULT SetContinue();
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
    virtual HRESULT ClearSingleStep() = 0;
    virtual HRESULT GetCurrentPC( Address& address ) = 0;
    virtual HRESULT GetReturnAddress( Address& address ) = 0;

    virtual HRESULT SuspendThread( Thread* thread ) = 0;
    virtual HRESULT ResumeThread( Thread* thread ) = 0;

    virtual HRESULT GetThreadContextInternal( uint32_t threadId, void* context, uint32_t size ) = 0;
    virtual HRESULT SetThreadContextInternal( uint32_t threadId, const void* context, uint32_t size ) = 0;

    bool    Stopped();
    ThreadX86Base* GetStoppedThread();
    HANDLE  GetProcessHandle();

private:
    // TODO: rename as I like
    HRESULT ReadStepperMemory( 
        Address address, 
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer );

    HRESULT WriteStepperMemory( 
        Address address, 
        SIZE_T length, 
        SIZE_T& lengthWritten, 
        uint8_t* buffer );

    HRESULT SetBreakpointInternal( Address address, bool user );
    HRESULT RemoveBreakpointInternal( Address address, bool user );

    HRESULT PatchBreakpoint( Breakpoint* bp );
    HRESULT UnpatchBreakpoint( Breakpoint* bp );
    bool IsBreakpointPatched( Address address );

    HRESULT ReadInstruction( 
        Address address, 
        InstructionType& type, 
        int& size );

    HRESULT PassBP( Address pc, 
        InstructionType instType, 
        int instLen, 
        int notifier, 
        Motion motion,
        const AddressRange* range );
    HRESULT PassBPSimple( 
        Address pc, 
        int instLen, 
        int notifier, 
        Motion motion,
        const AddressRange* range );
    HRESULT PassBPCall( 
        Address pc, 
        int instLen, 
        int notifier, 
        Motion motion,
        const AddressRange* range );
    HRESULT PassBPSyscall( 
        Address pc, 
        int instLen, 
        int notifier, 
        Motion motion,
        const AddressRange* range );
    HRESULT PassBPRepString( 
        Address pc, 
        int instLen, 
        int notifier, 
        Motion motion,
        const AddressRange* range );

    HRESULT SetupInstructionStep( 
        Address pc, 
        int instLen, 
        int notifier,
        ExpectedCode code,
        bool unpatch,
        bool resumeThreads,
        bool clearTF,
        Motion motion,
        const AddressRange* range
        );

    HRESULT DontPassBP( 
        Motion motion, 
        Address pc, 
        InstructionType instType, 
        int instLen, 
        int notifier, 
        const AddressRange* range );
    HRESULT DontPassBPSimple( 
        Motion motion, 
        Address pc, 
        int instLen, 
        int notifier, 
        const AddressRange* range );
    HRESULT DontPassBPCall( 
        Motion motion, 
        Address pc, 
        int instLen, 
        int notifier, 
        const AddressRange* range );
    HRESULT DontPassBPSyscall( 
        Motion motion, 
        Address pc, 
        int instLen, 
        int notifier, 
        const AddressRange* range );
    HRESULT DontPassBPRepString(
        Motion motion, 
        Address pc, 
        int instLen, 
        int notifier,
        const AddressRange* range );

    HRESULT SuspendOtherThreads( UINT32 threadId );
    HRESULT ResumeOtherThreads( UINT32 threadId );

    HRESULT DispatchSingleStep( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    HRESULT DispatchBreakpoint( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result );
    HRESULT RunAllActions( bool cancel, MachineResult& result );
    HRESULT RunNotifierAction( 
        int notifier, 
        Motion motion, 
        const AddressRange* range, 
        MachineResult& result );
    HRESULT RunNotifyCheckCall( Motion motion, const AddressRange* range, MachineResult& result );
    HRESULT RunNotifyCheckRange( Motion motion, const AddressRange* range, MachineResult& result );

    HRESULT Rewind();
    Breakpoint* FindBP( Address address );
    ThreadX86Base* FindThread( uint32_t threadId );
    bool AtEmbeddedBP( Address address, Breakpoint* bp );
};
