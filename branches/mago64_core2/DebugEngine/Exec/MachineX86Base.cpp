/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DecodeX86.h"
#include "MachineX86Base.h"
#include "EventCallback.h"
#include "IProcess.h"
#include "Process.h"
#include "Thread.h"
#include "ThreadX86.h"


class Breakpoint;

class BPAddressTable : public std::map< Address, Breakpoint* >
{
};

typedef BPAddressTable::iterator BPIterator;


const uint8_t   BreakpointInstruction = 0xCC;
const uint32_t  STATUS_WX86_SINGLE_STEP = 0x4000001E;
const uint32_t  STATUS_WX86_BREAKPOINT = 0x4000001F;


class Breakpoint
{
    LONG            mRefCount;
    Address         mAddress;
    int32_t         mStepCount;
    uint8_t         mOrigInstByte;
    uint8_t         mTempInstByte;
    bool            mPatched;
    bool            mUser;
    bool            mLocked;

public:
    Breakpoint()
        :   mRefCount( 0 ),
            mAddress( 0 ),
            mStepCount( 0 ),
            mOrigInstByte( 0 ),
            mTempInstByte( 0 ),
            mPatched( false ),
            mUser( false ),
            mLocked( false )
    {
    }

    void AddRef()
    {
        mRefCount++;
    }

    void Release()
    {
        mRefCount--;
        _ASSERT( mRefCount >= 0 );
        if ( mRefCount == 0 )
        {
            delete this;
        }
    }

    Address GetAddress()
    {
        return mAddress;
    }

    void SetAddress( Address address )
    {
        mAddress = address;
    }

    uint8_t GetOriginalInstructionByte()
    {
        return mOrigInstByte;
    }

    void    SetOriginalInstructionByte( uint8_t data )
    {
        mOrigInstByte = data;
    }

    uint8_t GetTempInstructionByte()
    {
        return mTempInstByte;
    }

    void    SetTempInstructionByte( uint8_t data )
    {
        mTempInstByte = data;
    }

    bool    IsPatched()
    {
        return mPatched;
    }

    void    SetPatched( bool value )
    {
        mPatched = value;
    }

    bool    IsUser()
    {
        return mUser;
    }

    void    SetUser( bool value )
    {
        mUser = value;
    }

    bool IsActive()
    {
        return mUser || mStepCount > 0 || mLocked;
    }

    bool IsStepping()
    {
        return mStepCount > 0;
    }

    void AddStepper()
    {
        _ASSERT( mStepCount >= 0 );
        _ASSERT( mStepCount < limit_max( mStepCount ) );
        mStepCount++;
    }

    void RemoveStepper()
    {
        _ASSERT( mStepCount > 0 );
        mStepCount--;
    }

    bool    IsLocked()
    {
        return mLocked;
    }

    void    SetLocked( bool value )
    {
        mLocked = value;
    }
};


MachineX86Base::MachineX86Base()
:   mRefCount( 0 ),
    mProcess( NULL ),
    mhProcess( NULL ),
    mAddrTable( NULL ),
    mStoppedThreadId( 0 ),
    mStoppedOnException( false ),
    mStopped( false ),
    mCurThread( NULL ),
    mIsolatedThreadId( 0 ),
    mIsolatedThread( false ),
    mCallback( NULL ),
    mPendCBAddr( 0 )
{
}

MachineX86Base::~MachineX86Base()
{
    if ( mAddrTable != NULL )
    {
        for ( BPAddressTable::iterator it = mAddrTable->begin();
            it != mAddrTable->end();
            it++ )
        {
            it->second->Release();
        }
    }

    delete mAddrTable;

    for ( ThreadMap::iterator it = mThreads.begin();
        it != mThreads.end();
        it++ )
    {
        delete it->second;
    }

    if ( mCallback != NULL )
    {
        mCallback->Release();
    }
}


void MachineX86Base::AddRef()
{
    InterlockedIncrement( &mRefCount );
}

void MachineX86Base::Release()
{
    LONG    newRef = InterlockedDecrement( &mRefCount );
    _ASSERT( newRef >= 0 );
    if ( newRef == 0 )
    {
        delete this;
    }
}


HRESULT MachineX86Base::Init()
{
    std::auto_ptr< BPAddressTable > addrTable( new BPAddressTable() );

    if ( addrTable.get() == NULL )
        return E_OUTOFMEMORY;

    mAddrTable = addrTable.release();

    return S_OK;
}

void    MachineX86Base::SetProcess( HANDLE hProcess, uint32_t id, Process* process )
{
    _ASSERT( mProcess == NULL );
    _ASSERT( mhProcess == NULL );
    _ASSERT( hProcess != NULL );
    _ASSERT( id != 0 );
    _ASSERT( process != NULL );

    mProcess = process;
    mhProcess = hProcess;
}

void    MachineX86Base::SetCallback( IEventCallback* callback )
{
    if ( mCallback != NULL )
        mCallback->Release();

    mCallback = callback;

    if ( mCallback != NULL )
        mCallback->AddRef();
}

void MachineX86Base::GetPendingCallbackBP( Address& address )
{
    address = mPendCBAddr;
}


HRESULT MachineX86Base::ReadMemory( 
    Address address, 
    SIZE_T length, 
    SIZE_T& lengthRead, 
    SIZE_T& lengthUnreadable, 
    uint8_t* buffer )
{
    return ReadCleanMemory( address, length, lengthRead, lengthUnreadable, buffer );
}

HRESULT MachineX86Base::WriteMemory( 
    Address address, 
    SIZE_T length, 
    SIZE_T& lengthWritten, 
    uint8_t* buffer )
{
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    return WriteCleanMemory( address, length, lengthWritten, buffer );
}

HRESULT MachineX86Base::SetBreakpoint( Address address )
{
    _ASSERT( mhProcess != NULL );
    if ( mhProcess == NULL )
        return E_UNEXPECTED;

    return SetBreakpointInternal( address, true );
}

HRESULT MachineX86Base::RemoveBreakpoint( Address address )
{
    _ASSERT( mhProcess != NULL );
    if ( mhProcess == NULL )
        return E_UNEXPECTED;

    return RemoveBreakpointInternal( address, true );
}

bool MachineX86Base::IsBreakpointActive( Address address )
{
    BPAddressTable::iterator    it = mAddrTable->find( address );
    bool                        isActive = false;

    if ( it != mAddrTable->end() )
        isActive = true;

    return isActive;
}

HRESULT MachineX86Base::SetBreakpointInternal( Address address, bool user )
{
    HRESULT                     hr = S_OK;
    BPAddressTable::iterator    it = mAddrTable->find( address );
    Breakpoint*                 bp = NULL;
    bool                        wasActive = false;

    if ( it == mAddrTable->end() )
    {
        RefPtr< Breakpoint >    newBp( new Breakpoint() );

        mAddrTable->insert( BPAddressTable::value_type( address, newBp.Get() ) );
        bp = newBp.Detach();

        bp->SetAddress( address );
    }
    else
    {
        bp = it->second;
        wasActive = true;
    }

    if ( user )
        bp->SetUser( true );
    else
        bp->AddStepper();

    // need to patch when going from not active to active
    if ( !wasActive )
    {
        hr = PatchBreakpoint( bp );
        if ( FAILED( hr ) )
            goto Error;
    }

Error:
    return hr;
}

HRESULT MachineX86Base::RemoveBreakpointInternal( Address address, bool user )
{
    HRESULT hr = S_OK;
    BPAddressTable::iterator    it = mAddrTable->find( address );

    if ( it == mAddrTable->end() )
        return S_OK;

    Breakpoint* bp = it->second;

    if ( user )
        bp->SetUser( false );
    else
        bp->RemoveStepper();

    // not the last one, so nothing else to do
    if ( bp->IsActive() )
        return S_OK;

    // need to unpatch when going from active to not active

    hr = UnpatchBreakpoint( bp );

    mAddrTable->erase( it );

    bp->Release();

    if ( FAILED( hr ) )
        goto Error;

Error:
    return hr;
}

HRESULT MachineX86Base::PatchBreakpoint( Breakpoint* bp )
{
    _ASSERT( bp != NULL );
    _ASSERT( !bp->IsPatched() );

    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
    uint8_t origData = 0;
    SIZE_T  bytesRead = 0;
    SIZE_T  bytesWritten = 0;
    void*   address = (void*) bp->GetAddress();

    bRet = ::ReadProcessMemory( mhProcess, address, &origData, 1, &bytesRead );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    bp->SetOriginalInstructionByte( origData );

    // looks like we don't have to worry about memory protection; VS doesn't
    // As a debugger, the only thing we can't write to is PAGE_NOACCESS.
    bRet = ::WriteProcessMemory( mhProcess, address, &BreakpointInstruction, 1, &bytesWritten );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    ::FlushInstructionCache( mhProcess, address, 1 );
    bp->SetPatched( true );

Error:
    return hr;
}

HRESULT MachineX86Base::UnpatchBreakpoint( Breakpoint* bp )
{
    _ASSERT( bp != NULL );
    _ASSERT( bp->IsPatched() );

    // already unpatched
    if ( !bp->IsPatched() )
        return S_OK;

    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
    uint8_t origData = bp->GetOriginalInstructionByte();
    SIZE_T  bytesWritten = 0;
    void*   address = (void*) bp->GetAddress();

    // looks like we don't have to worry about memory protection; VS doesn't
    // As a debugger, the only thing we can't write to is PAGE_NOACCESS.
    bRet = ::WriteProcessMemory( mhProcess, address, &origData, 1, &bytesWritten );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    ::FlushInstructionCache( mhProcess, address, 1 );
    bp->SetPatched( false );

Error:
    return hr;
}

HRESULT MachineX86Base::TempPatchBreakpoint( Breakpoint* bp )
{
    _ASSERT( bp != NULL );
    _ASSERT( bp->IsLocked() );

    HRESULT hr = S_OK;

    bp->SetLocked( false );

    if ( bp->IsActive() )
    {
        hr = PatchBreakpoint( bp );
        if ( FAILED( hr ) )
            return hr;
    }
    else
    {
        mAddrTable->erase( bp->GetAddress() );
    }

    return S_OK;
}

// If the step count and user go to 0 while it's unpatched, the BP would be deleted.
// This method locks the BP while it's unpatched. When about to patch it again, 
// the BP will be deleted instead if no one needs it anymore.

HRESULT MachineX86Base::TempUnpatchBreakpoint( Breakpoint* bp )
{
    _ASSERT( bp != NULL );
    _ASSERT( !bp->IsLocked() );

    HRESULT hr = S_OK;

    hr = UnpatchBreakpoint( bp );
    if ( FAILED( hr ) )
        return hr;

    bp->SetLocked( true );
    return S_OK;
}

bool    MachineX86Base::Stopped()
{
    return mStopped;
}

HANDLE  MachineX86Base::GetProcessHandle()
{
    return mhProcess;
}

HRESULT MachineX86Base::Detach()
{
    if ( mIsolatedThread )
    {
        ResumeOtherThreads( mIsolatedThreadId );

        mIsolatedThread = false;
        mIsolatedThreadId = 0;
    }

    if ( mAddrTable != NULL )
    {
        for ( BPAddressTable::iterator it = mAddrTable->begin();
            it != mAddrTable->end();
            it++ )
        {
            Breakpoint* bp = it->second;

            if ( bp->IsPatched() )
            {
                UnpatchBreakpoint( bp );
            }

            bp->Release();
        }

        mAddrTable->clear();
    }

    if ( mStopped )
    {
        FlushThreadContext();
    }

    return S_OK;
}

void MachineX86Base::OnStopped( uint32_t threadId )
{
    mStopped = true;
    mStoppedThreadId = threadId;
    mStoppedOnException = false;
    mCurThread = FindThread( threadId );
}

HRESULT MachineX86Base::OnCreateThread( Thread* thread )
{
    _ASSERT( thread != NULL );

    HRESULT hr = S_OK;
    std::auto_ptr< ThreadX86Base >   threadX86( new ThreadX86Base( thread ) );

    if ( threadX86.get() == NULL )
        return E_OUTOFMEMORY;

    mThreads.insert( ThreadMap::value_type( thread->GetId(), threadX86.get() ) );
    
    mCurThread = threadX86.release();

    if ( mIsolatedThread )
    {
        // add the created thread to the set of those suspended and waiting for a BP restore
        ThreadControlProc suspendProc = GetWinSuspendThreadProc();
        hr = ControlThread( thread->GetHandle(), suspendProc );
        if ( FAILED( hr ) )
            return hr;
    }

    return S_OK;
}

HRESULT MachineX86Base::OnExitThread( uint32_t threadId )
{
    HRESULT hr = S_OK;
    ThreadMap::iterator it = mThreads.find( threadId );

    hr = CancelStep();
    if ( FAILED( hr ) )
        return hr;

    if ( it != mThreads.end() )
    {
        delete it->second;
        mThreads.erase( it );
    }

    mCurThread = NULL;

    return S_OK;
}

ThreadX86Base* MachineX86Base::FindThread( uint32_t threadId )
{
    ThreadMap::iterator it = mThreads.find( threadId );

    if ( it == mThreads.end() )
        return NULL;

    return it->second;
}

HRESULT MachineX86Base::OnContinue()
{
    if ( mhProcess == NULL )
        return E_UNEXPECTED;

    HRESULT hr = S_OK;

    hr = FlushThreadContext();
    if ( FAILED( hr ) )
        goto Error;

    mStopped = false;
    mStoppedThreadId = 0;
    mCurThread = NULL;

Error:
    return hr;
}

HRESULT MachineX86Base::SuspendOtherThreads( UINT32 threadId )
{
    _ASSERT( !mIsolatedThread );

    HRESULT hr = S_OK;

    hr = ::SuspendOtherThreads( mProcess, threadId, GetWinSuspendThreadProc() );
    if ( FAILED( hr ) )
        return hr;

    mIsolatedThread = true;
    mIsolatedThreadId = threadId;
    return S_OK;
}

HRESULT MachineX86Base::ResumeOtherThreads( UINT32 threadId )
{
    _ASSERT( mIsolatedThread );

    HRESULT hr = S_OK;

    hr = ::ResumeOtherThreads( mProcess, threadId, GetWinSuspendThreadProc() );
    if ( FAILED( hr ) )
        return hr;

    mIsolatedThread = false;
    mIsolatedThreadId = 0;
    return S_OK;
}

bool MachineX86Base::AtEmbeddedBP( Address address, Breakpoint* bp )
{
    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
    uint8_t origData = 0;
    void*   ptr = (void*) address;

    if ( bp != NULL )
    {
        return bp->GetOriginalInstructionByte() == BreakpointInstruction;
    }

    bRet = ::ReadProcessMemory( mhProcess, ptr, &origData, 1, NULL );
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    if ( origData == BreakpointInstruction )
    {
        return true;
    }

Error:
    return false;
}

HRESULT MachineX86Base::Rewind()
{
    return ChangeCurrentPC( -1 );
}

void    MachineX86Base::OnDestroyProcess()
{
    mhProcess = NULL;
    mProcess = NULL;
    mCurThread = NULL;
}

HRESULT MachineX86Base::OnException( 
    uint32_t threadId, 
    const EXCEPTION_DEBUG_INFO* exceptRec, 
    MachineResult& result )
{
    UNREFERENCED_PARAMETER( threadId );
    _ASSERT( mhProcess != NULL );
    if ( mhProcess == NULL )
        return E_UNEXPECTED;
    _ASSERT( threadId != 0 );
    _ASSERT( exceptRec != NULL );
    _ASSERT( mCurThread != NULL );

    HRESULT hr = S_OK;

    hr = CacheThreadContext();
    if ( FAILED( hr ) )
        return hr;

    mStoppedOnException = true;

    if ( exceptRec->ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP )
    {
        return DispatchSingleStep( exceptRec, result );
    }

    if ( exceptRec->ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT )
    {
        return DispatchBreakpoint( exceptRec, result );
    }

    hr = CancelStep();
    if ( FAILED( hr ) )
        return hr;

    result = MacRes_NotHandled;
    return S_OK;
}

HRESULT MachineX86Base::RunAllActions( bool cancel, MachineResult& result )
{
    HRESULT hr = S_OK;
    ExpectedEvent* event = mCurThread->GetTopExpected();
    _ASSERT( event != NULL );
    AddressRange range = event->Range;

    if ( event->ClearTF && !cancel )
    {
        hr = ClearSingleStep();
        if ( FAILED( hr ) )
            goto Error;
    }

    if ( event->PatchBP )
    {
        Breakpoint* bp = FindBP( event->UnpatchedAddress );

        hr = TempPatchBreakpoint( bp );
        if ( FAILED( hr ) )
            goto Error;
    }

    if ( event->RemoveBP )
    {
        hr = RemoveBreakpointInternal( event->BPAddress, false );
        if ( FAILED( hr ) )
            goto Error;
    }

    if ( event->ResumeThreads )
    {
        hr = ResumeOtherThreads( mStoppedThreadId );
        if ( FAILED( hr ) )
            goto Error;
    }

    int notifier = event->NotifyAction;
    Motion motion = event->Motion;

    mCurThread->PopExpected();

    if ( !cancel )
    {
        hr = RunNotifierAction( notifier, motion, &range, result );
    }

Error:
    return hr;
}

HRESULT MachineX86Base::RunNotifierAction( 
    int notifier, 
    Motion motion, 
    const AddressRange* range, 
    MachineResult& result )
{
    HRESULT hr = S_OK;

    switch ( notifier )
    {
    case NotifyRun:
        result = MacRes_HandledContinue;
        break;

    case NotifyStepComplete:
        result = MacRes_PendingCallbackStep;
        break;

    case NotifyTrigger:
        // This will run the next set of actions, 
        // because we already popped the one we were working on.
        hr = RunAllActions( false, result );
        break;

    case NotifyCheckRange:
        hr = RunNotifyCheckRange( motion, range, result );
        break;

    case NotifyCheckCall:
        hr = RunNotifyCheckCall( motion, range, result );
        break;

    case NotifyStepOut:
        hr = RunNotifyStepOut( motion, range, result );
        break;

    default:
        _ASSERT_EXPR( false, L"The notifier action is wrong." );
        result = MacRes_HandledContinue;
        break;
    }

    return hr;
}

HRESULT MachineX86Base::RunNotifyCheckRange( 
    Motion motion, 
    const AddressRange* range, 
    MachineResult& result )
{
    if ( range == NULL )
        return E_FAIL;

    HRESULT hr = S_OK;
    Address pc = 0;

    hr = GetCurrentPC( pc );
    if ( FAILED( hr ) )
        goto Error;

    if ( pc >= range->Begin && pc <= range->End )
    {
        bool stepIn = (motion == Motion_RangeStepIn) ? true : false;

        hr = SetStepRange( stepIn, *range );
        if ( FAILED( hr ) )
            goto Error;

        result = MacRes_HandledContinue;
    }
    else
    {
        result = MacRes_PendingCallbackStep;
    }

Error:
    return hr;
}

HRESULT MachineX86Base::RunNotifyCheckCall( 
    Motion motion, 
    const AddressRange* range, 
    MachineResult& result )
{
    HRESULT hr = S_OK;
    Address pc = 0;
    Address retAddr = 0;
    ExpectedEvent* event = NULL;
    bool setBP = false;
    RunMode mode = RunMode_Run;

    hr = GetCurrentPC( pc );
    if ( FAILED( hr ) )
        goto Error;

    if ( pc >= range->Begin && pc <= range->End )
    {
        // if you call a procedure in the same range, then there's no need to probe
        mode = RunMode_Run;
    }
    else
    {
        mode = mCallback->OnCallProbe( mProcess, mStoppedThreadId, pc );
    }

    if ( mode == RunMode_Break )
    {
        hr = CancelStep();
        if ( FAILED( hr ) )
            goto Error;

        result = MacRes_PendingCallbackStep;
    }
    else if ( mode == RunMode_Run )
    {
        hr = GetReturnAddress( retAddr );
        if ( FAILED( hr ) )
            goto Error;

        event = mCurThread->PushExpected( Expect_BP, NotifyCheckRange );
        if ( event == NULL )
        {
            hr = E_FAIL;
            goto Error;
        }

        event->BPAddress = retAddr;
        event->RemoveBP = true;
        event->Motion = motion;
        event->Range = *range;

        hr = SetBreakpointInternal( retAddr, false );
        if ( FAILED( hr ) )
            goto Error;
        setBP = true;

        hr = SetContinue();
        if ( FAILED( hr ) )
            goto Error;

        result = MacRes_HandledContinue;
    }
    else // RunMode_Wait
    {
        // leave the step active
        result = MacRes_HandledStopped;
    }

Error:
    if ( FAILED( hr ) )
    {
        if ( setBP )
            RemoveBreakpointInternal( retAddr, false );
        if ( event != NULL )
            mCurThread->PopExpected();
    }
    return hr;
}

HRESULT MachineX86Base::RunNotifyStepOut( 
    Motion motion, 
    const AddressRange* range, 
    MachineResult& result )
{
    HRESULT hr = S_OK;
    Address pc = 0;
    Address retAddr = 0;
    ExpectedEvent* event = NULL;
    bool setBP = false;
    int notifier = NotifyNone;

    if ( motion == Motion_StepOver )
        notifier = NotifyStepComplete;
    else if ( motion == Motion_RangeStepOver )
        notifier = NotifyCheckRange;

    hr = GetCurrentPC( pc );
    if ( FAILED( hr ) )
        goto Error;

    hr = GetReturnAddress( retAddr );
    if ( FAILED( hr ) )
        goto Error;

    event = mCurThread->PushExpected( Expect_BP, notifier );
    if ( event == NULL )
    {
        hr = E_FAIL;
        goto Error;
    }

    event->BPAddress = retAddr;
    event->RemoveBP = true;
    event->Motion = motion;
    event->Range = *range;

    hr = SetBreakpointInternal( retAddr, false );
    if ( FAILED( hr ) )
        goto Error;
    setBP = true;

    hr = SetContinue();
    if ( FAILED( hr ) )
        goto Error;

    result = MacRes_HandledContinue;

Error:
    if ( FAILED( hr ) )
    {
        if ( setBP )
            RemoveBreakpointInternal( retAddr, false );
        if ( event != NULL )
            mCurThread->PopExpected();
    }
    return hr;
}

HRESULT MachineX86Base::DispatchSingleStep( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result )
{
    UNREFERENCED_PARAMETER( exceptRec );

    HRESULT hr = S_OK;

    result = MacRes_NotHandled;

    ExpectedEvent* event = mCurThread->GetTopExpected();

    if ( event == NULL )
    {
        // always treat the SS exception as a step complete, instead of an exception
        // even if the user wasn't stepping
        result = MacRes_PendingCallbackEmbeddedStep;
        return S_OK;
    }

    if ( event->Code == Expect_SS )
    {
        return RunAllActions( false, result );
    }

    if ( event->Code == Expect_BP )
    {
        hr = CancelStep();
        if ( FAILED( hr ) )
            return hr;

        result = MacRes_PendingCallbackEmbeddedStep;
        return S_OK;
    }

    return E_UNEXPECTED;
}

HRESULT    MachineX86Base::DispatchBreakpoint( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result )
{
    HRESULT hr = S_OK;
    Address exceptAddr = (Address) exceptRec->ExceptionRecord.ExceptionAddress;
    Breakpoint* bp = NULL;
    bool embeddedBP = false;

    result = MacRes_NotHandled;

    bp = FindBP( exceptAddr );
    embeddedBP = AtEmbeddedBP( exceptAddr, bp );

    ExpectedEvent* event = mCurThread->GetTopExpected();

    if ( event != NULL && event->Code == Expect_BP && event->BPAddress == exceptAddr )
    {
        if ( !embeddedBP )
        {
            hr = Rewind();
            if ( FAILED( hr ) )
                return hr;
        }

        return RunAllActions( false, result );
    }

    if ( embeddedBP )
    {
        hr = CancelStep();
        if ( FAILED( hr ) )
            return hr;

        Rewind();
        result = MacRes_PendingCallbackEmbeddedBP;
        mPendCBAddr = exceptAddr;
        return S_OK;
    }

    if ( bp->IsUser() )
    {
        Rewind();
        result = MacRes_PendingCallbackBP;
        mPendCBAddr = exceptAddr;
        return S_OK;
    }

    Rewind();
    result = MacRes_HandledContinue;
    return S_OK;
}

HRESULT MachineX86Base::CancelStep()
{
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    HRESULT hr = S_OK;
    MachineResult result = MacRes_NotHandled;

    while ( mCurThread->GetExpectedCount() > 0 )
    {
        hr = RunAllActions( true, result );
        if ( FAILED( hr ) )
            return hr;
    }

    return S_OK;
}

HRESULT MachineX86Base::ReadInstruction( 
    Address curAddress, 
    InstructionType& type, 
    int& size )
{
    HRESULT         hr = S_OK;
    BYTE            mem[MAX_INSTRUCTION_SIZE] = { 0 };
    SIZE_T          lenRead = 0;
    SIZE_T          lenUnreadable = 0;
    int             instLen = 0;
    InstructionType instType = Inst_None;
    CpuSizeMode     cpu = Is64Bit() ? Cpu_64 : Cpu_32;

    // this unpatches all BPs in the buffer
    hr = ReadCleanMemory( curAddress, MAX_INSTRUCTION_SIZE, lenRead, lenUnreadable, mem );
    if ( FAILED( hr ) )
        return hr;

    instType = GetInstructionTypeAndSize( mem, (int) lenRead, cpu, instLen );
    if ( instType == Inst_None )
        return E_UNEXPECTED;

    size = instLen;
    type = instType;
    return S_OK;
}

Breakpoint* MachineX86Base::FindBP( Address address )
{
    BPIterator      bpIt = mAddrTable->find( address );
    Breakpoint*     bp = NULL;

    if ( bpIt != mAddrTable->end() )
        bp = bpIt->second;

    return bp;
}

HRESULT MachineX86Base::DontPassBP( 
    Motion motion, 
    Address pc, 
    InstructionType instType, 
    int instLen, 
    int notifier, 
    const AddressRange* range )
{
    _ASSERT( instType != Inst_Breakpoint );
    HRESULT hr = S_OK;

    switch ( instType )
    {
    case Inst_Call:
        hr = DontPassBPCall( motion, pc, instLen, notifier, range );
        break;

    case Inst_Syscall:
        hr = DontPassBPSyscall( motion, pc, instLen, notifier, range );
        break;

    case Inst_RepString:
        hr = DontPassBPRepString( motion, pc, instLen, notifier, range );
        break;

    default:
        hr = DontPassBPSimple( motion, pc, instLen, notifier, range );
        break;
    }

    return hr;
}

HRESULT MachineX86Base::DontPassBPSimple( 
    Motion motion, 
    Address pc, 
    int instLen, 
    int notifier, 
    const AddressRange* range )
{
    return SetupInstructionStep( pc, instLen, notifier, Expect_SS, false, false, false, motion, range );
}

HRESULT MachineX86Base::DontPassBPCall( 
    Motion motion, 
    Address pc, 
    int instLen, 
    int notifier, 
    const AddressRange* range )
{
    if ( motion == Motion_RangeStepIn )
    {
        _ASSERT( notifier == NotifyCheckRange );
        notifier = NotifyCheckCall;
    }

    if ( motion == Motion_StepIn || motion == Motion_RangeStepIn )
        return SetupInstructionStep( pc, instLen, notifier, Expect_SS, false,false,false, motion, range);
    else if ( motion == Motion_StepOver || motion == Motion_RangeStepOver )
        return SetupInstructionStep( pc, instLen, notifier, Expect_BP, false,false,false, motion, range);

    return E_UNEXPECTED;
}

HRESULT MachineX86Base::DontPassBPSyscall( 
    Motion motion, 
    Address pc, 
    int instLen, 
    int notifier,
    const AddressRange* range )
{
    return SetupInstructionStep( pc, instLen, notifier, Expect_SS, false, false, true, motion, range );
}

HRESULT MachineX86Base::DontPassBPRepString( 
    Motion motion, 
    Address pc, 
    int instLen, 
    int notifier, 
    const AddressRange* range )
{
    if ( motion == Motion_StepIn )
        return SetupInstructionStep( pc, instLen, notifier, Expect_SS, false, false, false,motion,range);
    else if ( motion == Motion_StepOver 
        || motion == Motion_RangeStepIn 
        || motion == Motion_RangeStepOver )
        return SetupInstructionStep( pc, instLen, notifier, Expect_BP, false, false, false,motion,range);

    return E_UNEXPECTED;
}

HRESULT MachineX86Base::PassBP( 
    Address pc, 
    InstructionType instType, 
    int instLen, 
    int notifier, 
    Motion motion,
    const AddressRange* range )
{
    _ASSERT( instType != Inst_Breakpoint );
    HRESULT hr = S_OK;

    switch ( instType )
    {
    case Inst_Call:
        hr = PassBPCall( pc, instLen, notifier, motion, range );
        break;

    case Inst_Syscall:
        hr = PassBPSyscall( pc, instLen, notifier, motion, range );
        break;

    case Inst_RepString:
        hr = PassBPRepString( pc, instLen, notifier, motion, range );
        break;

    default:
        hr = PassBPSimple( pc, instLen, notifier, motion, range );
        break;
    }

    return hr;
}

HRESULT MachineX86Base::PassBPSimple( 
    Address pc, 
    int instLen, 
    int notifier, 
    Motion motion,
    const AddressRange* range )
{
    return SetupInstructionStep( pc, instLen, notifier, Expect_SS, true, true, false, motion, range );
}

HRESULT MachineX86Base::PassBPCall( 
    Address pc, 
    int instLen, 
    int notifier, 
    Motion motion,
    const AddressRange* range )
{
    if ( motion == Motion_RangeStepIn )
    {
        _ASSERT( notifier == NotifyCheckRange );
        notifier = NotifyCheckCall;
    }
    else if ( motion == Motion_StepOver || motion == Motion_RangeStepOver )
    {
        _ASSERT( notifier == NotifyStepComplete || notifier == NotifyCheckRange );
        notifier = NotifyStepOut;
    }

    return SetupInstructionStep( pc, instLen, notifier, Expect_SS, true, true, false, motion, range );
}

HRESULT MachineX86Base::PassBPSyscall( 
    Address pc, 
    int instLen, 
    int notifier, 
    Motion motion,
    const AddressRange* range )
{
    return SetupInstructionStep( pc, instLen, notifier, Expect_SS, true, false, true, motion, range );
}

HRESULT MachineX86Base::PassBPRepString( 
    Address pc, 
    int instLen, 
    int notifier, 
    Motion motion,
    const AddressRange* range )
{
    return SetupInstructionStep( pc, instLen, notifier, Expect_BP, true, true, false, motion, range );
}

HRESULT MachineX86Base::SetupInstructionStep( 
    Address pc, 
    int instLen, 
    int notifier,
    ExpectedCode code,
    bool unpatch,
    bool resumeThreads,
    bool clearTF,
    Motion motion,
    const AddressRange* range
    )
{
    HRESULT                     hr = S_OK;
    Breakpoint*                 bp = NULL;
    Address                     nextAddr = pc + instLen;
    bool                        suspended = false;
    bool                        unpatched = false;
    bool                        setBP = false;
    bool                        setSS = false;

    if ( resumeThreads )
    {
        hr = SuspendOtherThreads( mStoppedThreadId );
        if ( FAILED( hr ) )
            goto Error;
        suspended = true;
    }

    if ( unpatch )
    {
        bp = FindBP( pc );

        hr = TempUnpatchBreakpoint( bp );
        if ( FAILED( hr ) )
            goto Error;
        unpatched = true;
    }

    if ( code == Expect_SS )
    {
        hr = SetSingleStep( true );
        if ( FAILED( hr ) )
            goto Error;
        setSS = true;
    }
    else
    {
        hr = SetBreakpointInternal( nextAddr, false );
        if ( FAILED( hr ) )
            goto Error;
        setBP = true;
    }

    ExpectedEvent* event = mCurThread->PushExpected( code, notifier );
    if ( event == NULL )
    {
        hr = E_FAIL;
        goto Error;
    }

    if ( unpatch )
    {
        event->UnpatchedAddress = pc;
        event->PatchBP = true;
    }
    if ( code == Expect_BP )
    {
        event->BPAddress = nextAddr;
        event->RemoveBP = true;
    }
    event->ResumeThreads = resumeThreads;
    event->ClearTF = clearTF;
    event->Motion = motion;
    if ( range != NULL )
        event->Range = *range;

Error:
    if ( FAILED( hr ) )
    {
        if ( setSS )
            SetSingleStep( false );
        if ( setBP )
            RemoveBreakpointInternal( nextAddr, false );
        if ( unpatched )
            TempPatchBreakpoint( bp );
        if ( suspended )
            ResumeOtherThreads( mStoppedThreadId );
    }
    return hr;
}

HRESULT MachineX86Base::SetContinue()
{
    _ASSERT( mhProcess != NULL );
    _ASSERT( mStoppedThreadId != 0 );

    HRESULT         hr = S_OK;
    Address         pc = 0;
    Breakpoint*     bp = NULL;
    InstructionType instType = Inst_None;
    int             instLen = 0;

    if ( !mStoppedOnException )
        return S_OK;

    if ( mCurThread == NULL )
        return S_OK;

    hr = GetCurrentPC( pc );
    if ( FAILED( hr ) )
        goto Error;

    bp = FindBP( pc );

    hr = ReadInstruction( pc, instType, instLen );
    if ( FAILED( hr ) )
        goto Error;

    if ( instType == Inst_Breakpoint )
    {
        _ASSERT( mCurThread->GetExpectedCount() == 0 );

        // when we came to break mode, we rewound the PC
        // move it back, because we don't want to run the BP instruction again

        // skip over the BP instruction
        hr = ChangeCurrentPC( 1 );
        if ( FAILED( hr ) )
            goto Error;
    }
    else
    {
        _ASSERT( mCurThread->GetExpectedCount() < 2 );
        if ( bp != NULL && bp->IsPatched() )
        {
            int notifier = NotifyRun;

            ExpectedEvent* event = mCurThread->GetTopExpected();
            if ( event != NULL && event->Code == Expect_SS )
                notifier = NotifyTrigger;

            hr = PassBP( pc, instType, instLen, notifier, Motion_None, NULL );
            if ( FAILED( hr ) )
                goto Error;
        }
        // else, nothing to do, you can continue
    }

Error:
    return hr;
}

HRESULT MachineX86Base::SetStepOut( Address targetAddress )
{
    _ASSERT( mhProcess != NULL );
    if ( mhProcess == NULL )
        return E_UNEXPECTED;
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    HRESULT hr = S_OK;
    ExpectedEvent* event = NULL;

    if ( !mStoppedOnException )
        return S_OK;

    hr = CancelStep();
    if ( FAILED( hr ) )
        goto Error;

    event = mCurThread->PushExpected( Expect_BP, NotifyStepComplete );
    if ( event == NULL )
    {
        hr = E_FAIL;
        goto Error;
    }

    event->BPAddress = targetAddress;
    event->RemoveBP = true;

    hr = SetBreakpointInternal( targetAddress, false );
    if ( FAILED( hr ) )
        goto Error;

    hr = SetContinue();
    if ( FAILED( hr ) )
        goto Error;

Error:
    if ( FAILED( hr ) )
    {
        if ( event != NULL )
            mCurThread->PopExpected();
    }
    return hr;
}

HRESULT MachineX86Base::SetStepInstruction( bool stepIn )
{
    Motion motion = stepIn ? Motion_StepIn : Motion_StepOver;

    return SetStepInstructionCore( motion, NULL, NotifyStepComplete );
}

HRESULT MachineX86Base::SetStepRange( bool stepIn, AddressRange range )
{
    Motion motion = stepIn ? Motion_RangeStepIn : Motion_RangeStepOver;

    return SetStepInstructionCore( motion, &range, NotifyCheckRange );
}

HRESULT MachineX86Base::SetStepInstructionCore( Motion motion, const AddressRange* range, int notifier )
{
    _ASSERT( mhProcess != NULL );
    if ( mhProcess == NULL )
        return E_UNEXPECTED;
    _ASSERT( mStoppedThreadId != 0 );
    _ASSERT( mCurThread != NULL );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    HRESULT hr = S_OK;
    Address pc = 0;
    Breakpoint* bp = NULL;
    InstructionType instType = Inst_None;
    int instLen = 0;

    if ( !mStoppedOnException )
        return S_OK;

    hr = CancelStep();
    if ( FAILED( hr ) )
        goto Error;

    hr = GetCurrentPC( pc );
    if ( FAILED( hr ) )
        goto Error;

    bp = FindBP( pc );

    hr = ReadInstruction( pc, instType, instLen );
    if ( FAILED( hr ) )
        goto Error;

    if ( instType == Inst_Breakpoint )
    {
        ExpectedEvent* event = mCurThread->PushExpected( Expect_BP, notifier );
        if ( event == NULL )
        {
            hr = E_FAIL;
            goto Error;
        }

        event->BPAddress = pc;
        // don't try to remove a BP
        event->ClearTF = true;
        event->Motion = motion;
        if ( range != NULL )
            event->Range = *range;
    }
    else
    {
        if ( bp != NULL && bp->IsPatched() )
        {
            hr = PassBP( pc, instType, instLen, notifier, motion, range );
            if ( FAILED( hr ) )
                goto Error;
        }
        else
        {
            hr = DontPassBP( motion, pc, instType, instLen, notifier, range );
            if ( FAILED( hr ) )
                goto Error;
        }
    }

Error:
    return hr;
}

HRESULT MachineX86Base::GetThreadContext( uint32_t threadId, void* context, uint32_t size )
{
    _ASSERT( mhProcess != NULL );
    if ( mhProcess == NULL )
        return E_UNEXPECTED;
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    if ( context == NULL )
        return E_INVALIDARG;

    return GetThreadContextInternal( threadId, context, size );
}

HRESULT MachineX86Base::SetThreadContext( uint32_t threadId, const void* context, uint32_t size )
{
    _ASSERT( mhProcess != NULL );
    if ( mhProcess == NULL )
        return E_UNEXPECTED;
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    if ( context == NULL )
        return E_INVALIDARG;

    return SetThreadContextInternal( threadId, context, size );
}


ThreadX86Base* MachineX86Base::GetStoppedThread()
{
    return mCurThread;
}

HRESULT MachineX86Base::ReadCleanMemory( 
    Address address, 
    SIZE_T length, 
    SIZE_T& lengthRead, 
    SIZE_T& lengthUnreadable, 
    uint8_t* buffer )
{
    HRESULT hr = S_OK;

    hr = ::ReadMemory( mhProcess, address, length, lengthRead, lengthUnreadable, buffer );
    if ( FAILED( hr ) )
        return hr;

    if ( lengthRead > 0 )
    {
        Address startAddr = address;
        Address endAddr = address + lengthRead - 1;

        // unpatch all BPs from the memory area we're returning

        for ( BPAddressTable::iterator it = mAddrTable->begin();
            it != mAddrTable->end();
            it++ )
        {
            Breakpoint* bp = it->second;

            if ( bp->IsPatched() && (it->first >= startAddr) && (it->first <= endAddr) )
            {
                Address offset = it->first - startAddr;
                buffer[ offset ] = bp->GetOriginalInstructionByte();
            }
        }
    }

    return hr;
}

HRESULT MachineX86Base::WriteCleanMemory( 
    Address address, 
    SIZE_T length, 
    SIZE_T& lengthWritten, 
    uint8_t* buffer )
{
    BOOL    bRet = FALSE;

    // The memory we're overwriting might be patched with BPs, so do it in 3 steps:
    // 1. For each BP in the target mem. range, 
    //      replace its original saved data with what's in the source buffer.
    // 2. For each BP in the target mem. range,
    //      patch a BP instruction in the source buffer.
    // 3. Write the source buffer to the process.

    if ( length == 0 )
        return S_OK;

    Address startAddr = address;
    Address endAddr = address + length - 1;

    for ( BPAddressTable::iterator it = mAddrTable->begin();
        it != mAddrTable->end();
        it++ )
    {
        Breakpoint* bp = it->second;

        if ( bp->IsPatched() && (it->first >= startAddr) && (it->first <= endAddr) )
        {
            Address offset = it->first - startAddr;

            bp->SetTempInstructionByte( buffer[ offset ] );
            buffer[ offset ] = BreakpointInstruction;
        }
    }

    bRet = WriteProcessMemory( mhProcess, (void*) address, buffer, length, &lengthWritten );
    if ( !bRet )
        return GetLastHr();

    // now commit all the bytes for the BPs we overwrote

    if ( lengthWritten != length )
        return HRESULT_FROM_WIN32( ERROR_PARTIAL_COPY );

    endAddr = address + lengthWritten - 1;

    for ( BPAddressTable::iterator it = mAddrTable->begin();
        it != mAddrTable->end();
        it++ )
    {
        Breakpoint* bp = it->second;

        if ( bp->IsPatched() && (it->first >= startAddr) && (it->first <= endAddr) )
        {
            bp->SetOriginalInstructionByte( bp->GetTempInstructionByte() );
        }
    }

    return S_OK;
}
