/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "MachineX86Base.h"
#include "EventCallback.h"
#include "Iter.h"
#include "IProcess.h"
#include "Process.h"
#include "Thread.h"
#include "Stepper.h"
#include "ThreadX86.h"
#include <algorithm>

using namespace std;


class Breakpoint;

class CookieList : public std::list< BPCookie >
{
};

class BPAddressTable : public std::map< MachineAddress, Breakpoint* >
{
};


const uint8_t   BreakpointInstruction = 0xCC;
const uint32_t  STATUS_WX86_SINGLE_STEP = 0x4000001E;
const uint32_t  STATUS_WX86_BREAKPOINT = 0x4000001F;


class Breakpoint
{
    LONG            mRefCount;
    MachineAddress  mAddress;
    CookieList      mHiCookies;
    CookieList      mLoCookies;
    uint8_t         mOrigInstByte;
    uint8_t         mTempInstByte;
    bool            mPatched;
    int32_t         mLockCount;

public:
    Breakpoint()
        :   mRefCount( 0 ),
            mAddress( 0 ),
            mOrigInstByte( 0 ),
            mTempInstByte( 0 ),
            mPatched( false ),
            mLockCount( 0 )
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

    MachineAddress GetAddress()
    {
        return mAddress;
    }

    void SetAddress( MachineAddress address )
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

    bool    Patched()
    {
        return mPatched;
    }

    void    SetPatched( bool value )
    {
        mPatched = value;
    }

    CookieList& GetHighPriCookies()
    {
        return mHiCookies;
    }

    CookieList& GetLowPriCookies()
    {
        return mLoCookies;
    }

    bool IsActive()
    {
        return (mHiCookies.size() > 0) || (mLoCookies.size() > 0);
    }

    bool IsLocked()
    {
        return mLockCount > 0;
    }

    void Lock()
    {
        _ASSERT( mLockCount >= 0 );
        _ASSERT( mLockCount < limit_max( mLockCount ) );
        mLockCount++;
    }

    void Unlock()
    {
        _ASSERT( mLockCount > 0 );
        mLockCount--;
    }
};


MachineX86Base::MachineX86Base()
:   mRefCount( 0 ),
    mProcess( NULL ),
    mhProcess( NULL ),
    mProcessId( 0 ),
    mAddrTable( NULL ),
    mRestoreBPAddress( 0 ),
    mStopped( false ),
    mStoppedThreadId( 0 ),
    mStoppedOnException( false ),
    mCurThread( NULL ),
    mBPRestoringThreadId( 0 ),
    mIsolatedThread( false ),
    mExceptAddr( 0 ),
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

    // TODO: when do clean up the breakpoints? delete Breakpoint objects and restore any remaining BP instructions?
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
    auto_ptr< BPAddressTable >  addrTable( new BPAddressTable() );

    mAddrTable = addrTable.release();

    return S_OK;
}

void    MachineX86Base::SetProcess( HANDLE hProcess, uint32_t id, IProcess* process )
{
    _ASSERT( mProcess == NULL );
    _ASSERT( mhProcess == NULL );
    _ASSERT( mProcessId == 0 );
    _ASSERT( hProcess != NULL );
    _ASSERT( id != 0 );
    _ASSERT( process != NULL );

    mProcess = process;
    mhProcess = hProcess;
    mProcessId = id;
}

void    MachineX86Base::SetCallback( IEventCallback* callback )
{
    if ( mCallback != NULL )
        mCallback->Release();

    mCallback = callback;

    if ( mCallback != NULL )
        mCallback->AddRef();
}

void MachineX86Base::GetPendingCallbackBP( MachineAddress& address, int& count, BPCookie*& cookies )
{
    address = mPendCBAddr;
    count = mPendCBCookies.size();
    if ( count > 0 )
        cookies = &mPendCBCookies.front();
    else
        cookies = NULL;
}


HRESULT MachineX86Base::ReadMemory( 
    MachineAddress address, 
    SIZE_T length, 
    SIZE_T& lengthRead, 
    SIZE_T& lengthUnreadable, 
    uint8_t* buffer )
{
    return ReadStepperMemory( address, length, lengthRead, lengthUnreadable, buffer );
}

HRESULT MachineX86Base::WriteMemory( 
    MachineAddress address, 
    SIZE_T length, 
    SIZE_T& lengthWritten, 
    uint8_t* buffer )
{
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    return WriteStepperMemory( address, length, lengthWritten, buffer );
}

HRESULT MachineX86Base::SetBreakpoint( MachineAddress address, BPCookie cookie )
{
    _ASSERT( mhProcess != NULL );
    _ASSERT( mProcessId != 0 );
    if ( (mhProcess == NULL) || (mProcessId == 0) )
        return E_UNEXPECTED;

    return SetBreakpointInternal( address, cookie, BPPri_High );
}

HRESULT MachineX86Base::RemoveBreakpoint( MachineAddress address, BPCookie cookie )
{
    _ASSERT( mhProcess != NULL );
    _ASSERT( mProcessId != 0 );
    if ( (mhProcess == NULL) || (mProcessId == 0) )
        return E_UNEXPECTED;

    return RemoveBreakpointInternal( address, cookie, BPPri_High );
}

HRESULT MachineX86Base::IsBreakpointActive( MachineAddress address )
{
    BPAddressTable::iterator    it = mAddrTable->find( address );
    bool                        isActive = false;

    if ( it != mAddrTable->end() )
    {
        Breakpoint* bp = it->second;
        isActive = bp->IsActive();
    }

    return isActive;
}

HRESULT MachineX86Base::SetBreakpointInternal( MachineAddress address, BPCookie cookie, BPPriority priority )
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
        _ASSERT( !wasActive );
    }
    else
    {
        bp = it->second;
        wasActive = bp->IsActive();
        _ASSERT( wasActive || bp->IsLocked() );
    }

    CookieList* list = NULL;

    if ( priority == BPPri_Low )
        list = &bp->GetLowPriCookies();
    else
        list = &bp->GetHighPriCookies();

    list->push_back( cookie );

    // need to patch when going from not active to active
    if ( !wasActive )
    {
        hr = PatchBreakpoint( bp );
        // if we suspended, we have to resume, so jump out on error after the resume
        if ( FAILED( hr ) )
            goto Error;
    }

Error:
    return hr;
}

HRESULT MachineX86Base::RemoveBreakpointInternal( MachineAddress address, BPCookie cookie, BPPriority priority )
{
    HRESULT hr = S_OK;
    BPAddressTable::iterator    it = mAddrTable->find( address );

    if ( it == mAddrTable->end() )
        return S_OK;

    Breakpoint* bp = it->second;
    CookieList* list = NULL;

    if ( priority == BPPri_Low )
        list = &bp->GetLowPriCookies();
    else
        list = &bp->GetHighPriCookies();

    CookieList::iterator cookieIt = find( list->begin(), list->end(), cookie );

    if ( cookieIt == list->end() )
        return S_OK;

    list->erase( cookieIt );

    // not the last one, so nothing else to do
    if ( bp->IsActive() )
        return S_OK;

    // need to unpatch when going from active to not active

    hr = UnpatchBreakpoint( bp );
    // if we suspended, we have to resume, so jump out on error after the resume

    if ( !bp->IsLocked() )
    {
        mAddrTable->erase( it );

        bp->Release();

        // we're deleting the BP, so there'll be no need to restore it
        mRestoreBPAddress = 0;
    }

    if ( FAILED( hr ) )
        goto Error;

Error:
    return hr;
}

HRESULT MachineX86Base::PatchBreakpoint( Breakpoint* bp )
{
    _ASSERT( bp != NULL );
    _ASSERT( !bp->Patched() );

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

    // already unpatched
    if ( !bp->Patched() )
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


bool    MachineX86Base::Stopped()
{
    return mStopped;
}

void MachineX86Base::OnStopped( uint32_t threadId )
{
    mStopped = true;
    mStoppedThreadId = threadId;
    mStoppedOnException = false;
    mCurThread = FindThread( threadId );
}

void    MachineX86Base::OnCreateThread( Thread* thread )
{
    _ASSERT( thread != NULL );

    auto_ptr< ThreadX86Base >   threadX86( new ThreadX86Base( thread ) );

    mThreads.insert( ThreadMap::value_type( thread->GetId(), threadX86.get() ) );
    
    mCurThread = threadX86.release();

    // in the middle of restoring a BP
    if ( mBPRestoringThreadId != 0 )
    {
        ThreadMap::iterator threadIt = mThreads.find( mBPRestoringThreadId );
        BPAddressTable::iterator it = mAddrTable->find( mRestoreBPAddress );

        if ( (it != mAddrTable->end()) && (threadIt != mThreads.end()) )
        {
            if ( ShouldIsolateBPRestoringThread( mBPRestoringThreadId, it->second ) )
            {
                mIsolatedThread = true;
                // add the created thread to the set of those suspended and waiting for a BP restore
                SuspendThread( thread );
            }
        }
    }
}

void    MachineX86Base::OnExitThread( uint32_t threadId )
{
    ThreadMap::iterator it = mThreads.find( threadId );

    if ( it != mThreads.end() )
    {
        delete it->second;
        mThreads.erase( it );
    }

    mCurThread = NULL;
}

ThreadX86Base* MachineX86Base::FindThread( uint32_t threadId )
{
    ThreadMap::iterator it = mThreads.find( threadId );

    if ( it == mThreads.end() )
        return NULL;

    return it->second;
}


HRESULT MachineX86Base::OnException( uint32_t threadId, const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result )
{
    UNREFERENCED_PARAMETER( threadId );
    _ASSERT( mhProcess != NULL );
    _ASSERT( mProcessId != 0 );
    if ( (mhProcess == NULL) || (mProcessId == 0) )
        return E_UNEXPECTED;
    _ASSERT( threadId != 0 );
    _ASSERT( exceptRec != NULL );
    _ASSERT( mCurThread != NULL );

    HRESULT hr = S_OK;

    mStoppedOnException = true;
    mExceptAddr = (MachineAddress) exceptRec->ExceptionRecord.ExceptionAddress;

    if ( mCurThread->GetStepper() != NULL )
        mCurThread->GetStepper()->SetAddress( mExceptAddr );

    hr = RestoreBPEnvironment();
    if ( FAILED( hr ) )
        return hr;

    if ( exceptRec->ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP 
#ifdef _WIN64
        || exceptRec->ExceptionRecord.ExceptionCode == STATUS_WX86_SINGLE_STEP 
#endif
        )
    {
        return DispatchSingleStep( exceptRec, result );
    }

    if ( exceptRec->ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT 
#ifdef _WIN64
        && exceptRec->ExceptionRecord.ExceptionCode != STATUS_WX86_BREAKPOINT
#endif
        )
    {
        result = MacRes_NotHandled;
        return S_OK;
    }

    BPAddressTable::iterator    it = mAddrTable->find( mExceptAddr );
    bool                        embedded = false;
    RefPtr<Breakpoint>          bp;

    if ( it == mAddrTable->end() )
    {
        // this is an embedded breakpoint only
        embedded = true;
    }
    else
    {
        bp = it->second;
        if ( bp->GetOriginalInstructionByte() == BreakpointInstruction )
            embedded = true;
    }

    return DispatchBreakpoint( mExceptAddr, embedded, bp, result );
}

HRESULT MachineX86Base::RestoreBPEnvironment()
{
    HRESULT hr = S_OK;

    if ( mIsolatedThread )
    {
        hr = UnisolateBPRestoringThread();
        if ( FAILED( hr ) )
            return hr;
    }

    if ( mRestoreBPAddress != 0 )
    {
        BPAddressTable::iterator    it = mAddrTable->find( mRestoreBPAddress );
    
        mRestoreBPAddress = 0;
        mBPRestoringThreadId = 0;

        if ( it != mAddrTable->end() )
        {
            hr = PatchBreakpoint( it->second );
            if ( FAILED( hr ) )
                return hr;
        }
    }

    return S_OK;
}

HRESULT MachineX86Base::OnContinue()
{
    if ( (mhProcess == NULL) || (mProcessId == 0) )
        return E_UNEXPECTED;

    HRESULT hr = S_OK;

    hr = OnContinueInternal();
    if ( FAILED( hr ) )
        goto Error;

    mStopped = false;
    mStoppedThreadId = 0;
    mExceptAddr = 0;
    mCurThread = NULL;

Error:
    return hr;
}

HRESULT MachineX86Base::OnContinueInternal()
{
    _ASSERT( mhProcess != NULL );
    _ASSERT( mProcessId != 0 );
    _ASSERT( mStoppedThreadId != 0 );

    HRESULT         hr = S_OK;
    MachineAddress  pc = 0;

    if ( mCurThread == NULL )
        return S_OK;

    if ( mCurThread->GetResumeStepper() != NULL )
    {
        // we only use a resume stepper to let us patch breakpoints back
        // if the resume stepper is interrupted, we can still do our job

        hr = mCurThread->GetResumeStepper()->Cancel();
        if ( FAILED( hr ) )
            return hr;

        delete mCurThread->GetResumeStepper();
        mCurThread->SetResumeStepper( NULL );
    }

    if ( !mStoppedOnException )
        return S_OK;

    hr = GetCurrentPC( mStoppedThreadId, pc );
    if ( hr == E_ACCESSDENIED )
        // the thread already died, move along
        return S_OK;
    if ( FAILED( hr ) )
        return hr;

    BPAddressTable::iterator    it = mAddrTable->find( pc );

    if ( it == mAddrTable->end() )
    {
        // at an embedded BP only
        if ( AtEmbeddedBP( pc ) 
            && ((mCurThread->GetStepper() == NULL) || mCurThread->GetStepper()->CanSkipEmbeddedBP()) )
        {
            SkipBPOnResume();
        }
        return S_OK;
    }

    // we're continuing from a breakpoint we own

    Breakpoint* bp = it->second;

    if ( bp->GetOriginalInstructionByte() == BreakpointInstruction )
    {
        if ( (mCurThread->GetStepper() == NULL) || mCurThread->GetStepper()->CanSkipEmbeddedBP() )
        {
            SkipBPOnResume();
            return S_OK;
        }
    }

    if ( bp->GetHighPriCookies().size() == 0 )
        return S_OK;

    hr = UnpatchBreakpoint( bp );
    if ( FAILED( hr ) )
        return hr;

    // we set it to 0 on the next exception along with patching the BP we just unpatched
    mRestoreBPAddress = pc;
    mBPRestoringThreadId = mStoppedThreadId;

    hr = SetupRestoreBPEnvironment( bp, pc );
    if ( FAILED( hr ) )
        return hr;

    return S_OK;
}

bool MachineX86Base::AtEmbeddedBP( MachineAddress pc )
{
    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
    uint8_t origData = 0;
    SIZE_T  bytesRead = 0;
    void*   address = (void*) pc;

    bRet = ::ReadProcessMemory( mhProcess, address, &origData, 1, &bytesRead );
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
    return ChangeCurrentPC( mStoppedThreadId, -1 );
}

HRESULT MachineX86Base::SkipBPOnResume()
{
    // when we came to break mode, we rewound the PC
    // move it back, because we don't want to run the BP instruction again

    return ChangeCurrentPC( mStoppedThreadId, 1 );
}

HRESULT MachineX86Base::SetupRestoreBPEnvironment( Breakpoint* bp, MachineAddress pc )
{
    HRESULT hr = S_OK;

    if ( mCurThread->GetStepper() != NULL )
    {
        hr = mCurThread->GetStepper()->RequestStepAway( pc );
        if ( FAILED( hr ) )
            return hr;
    }
    else
    {
        _ASSERT( mCurThread->GetResumeStepper() == NULL );
        auto_ptr<IStepper>  stepper;
        BPCookie            cookie = mCurThread->GetResumeStepperCookie();
        MachineAddress      pc = 0;

        hr = GetCurrentPC( mStoppedThreadId, pc );
        if ( FAILED( hr ) )
            return hr;

        hr = MakeResumeStepper( static_cast<IStepperMachine*>( this ), pc, cookie, stepper );
        if ( FAILED( hr ) )
            return hr;

        mCurThread->SetResumeStepper( stepper.release() );

        hr = mCurThread->GetResumeStepper()->Start();
        if ( FAILED( hr ) )
            return hr;
    }

    if ( ShouldIsolateBPRestoringThread( mStoppedThreadId, bp ) )
    {
        hr = IsolateBPRestoringThread();
    }

    return hr;
}

bool    MachineX86Base::ShouldIsolateBPRestoringThread( uint32_t threadId, Breakpoint* bp )
{
    // we don't use these params now, but what about in the future?
    UNREFERENCED_PARAMETER( threadId );
    UNREFERENCED_PARAMETER( bp );

    _ASSERT( threadId != 0 );
    _ASSERT( bp != NULL );

    // no other threads, no conflict with them
    if ( ((Process*) mProcess)->GetThreadCount() == 1 )
        return false;

    // more than one thread can lead to conflict, make sure this is an atomic step
    return true;
}

HRESULT MachineX86Base::IsolateBPRestoringThread()
{
    _ASSERT( !mIsolatedThread );
    _ASSERT( mBPRestoringThreadId != 0 );
    if ( mBPRestoringThreadId == 0 )
        return E_UNEXPECTED;
    if ( mIsolatedThread )
        return E_UNEXPECTED;

    HRESULT hr = S_OK;
    RefReleasePtr< Enumerator< Thread* > >  en;

    hr = mProcess->EnumThreads( en.Ref() );
    if ( FAILED( hr ) )
        return hr;

    while ( en->MoveNext() )
    {
        Thread* thread = en->GetCurrent();

        if ( thread->GetId() != mBPRestoringThreadId )
        {
            hr = SuspendThread( thread );
            if ( FAILED( hr ) )
                return hr;
        }
    }

    mIsolatedThread = true;

    return S_OK;
}

HRESULT MachineX86Base::UnisolateBPRestoringThread()
{
    _ASSERT( mIsolatedThread );
    _ASSERT( mBPRestoringThreadId != 0 );
    if ( mBPRestoringThreadId == 0 )
        return E_UNEXPECTED;
    if ( !mIsolatedThread )
        return E_UNEXPECTED;

    HRESULT hr = S_OK;
    RefReleasePtr< Enumerator< Thread* > >  en;

    hr = mProcess->EnumThreads( en.Ref() );
    if ( FAILED( hr ) )
        return hr;

    while ( en->MoveNext() )
    {
        Thread* thread = en->GetCurrent();

        if ( thread->GetId() != mBPRestoringThreadId )
        {
            hr = ResumeThread( thread );
            if ( FAILED( hr ) )
                return hr;
        }
    }

    mIsolatedThread = false;

    return S_OK;
}

void    MachineX86Base::OnDestroyProcess()
{
    mhProcess = NULL;
    mProcessId = 0;
    mProcess = NULL;
}


HRESULT MachineX86Base::DispatchSingleStep( const EXCEPTION_DEBUG_INFO* exceptRec, MachineResult& result )
{
    UNREFERENCED_PARAMETER( exceptRec );

    HRESULT hr = S_OK;

    result = MacRes_NotHandled;

    if ( mCurThread->GetStepper() != NULL )
    {
        hr = mCurThread->GetStepper()->OnSingleStep( mExceptAddr, result );
        if ( FAILED( hr ) )
            return hr;

        if ( CheckStepperEnded() )
            result = MacRes_PendingCallbackStep;
    }

    if ( (result == MacRes_NotHandled) && (mCurThread->GetResumeStepper() != NULL) )
    {
        hr = mCurThread->GetResumeStepper()->OnSingleStep( mExceptAddr, result );
        if ( FAILED( hr ) )
            return hr;

        if ( result == MacRes_HandledStopped )
            result = MacRes_HandledContinue;
    }

    // always treat the SS exception as a step complete, instead of an exception
    // even if the user wasn't stepping

    if ( result == MacRes_NotHandled )
    {
        result = MacRes_PendingCallbackStep;
    }

    return hr;
}

HRESULT    MachineX86Base::DispatchBreakpoint( MachineAddress address, bool embedded, Breakpoint* bp, MachineResult& result )
{
    _ASSERT( (bp == NULL) || ((bp->GetHighPriCookies().size() > 0) || (bp->GetLowPriCookies().size() > 0)) );

    HRESULT hr = S_OK;

    result = MacRes_NotHandled;

    if ( mCurThread->GetStepper() != NULL )
    {
        bool rewindPC = true;

        hr = mCurThread->GetStepper()->OnBreakpoint( address, result, rewindPC );
        if ( FAILED( hr ) )
            return hr;

        if ( (result == MacRes_NotHandled) || rewindPC )
            Rewind();

        // if stepper ended, then it handled the exception and stopped
        if ( CheckStepperEnded() )
            result = MacRes_PendingCallbackStep;
    }
    else if ( mCurThread->GetResumeStepper() != NULL )
    {
        Rewind();
        result = MacRes_HandledContinue;
    }
    else
    {
        Rewind();
    }

    if ( (result == MacRes_NotHandled) && embedded )
    {
        result = MacRes_PendingCallbackBP;
        mPendCBAddr = address;
        mPendCBCookies.clear();
    }
    else if ( (result == MacRes_NotHandled) && (bp != NULL) && (bp->GetHighPriCookies().size() > 0) )
    {
        result = MacRes_PendingCallbackBP;
        mPendCBAddr = address;
        mPendCBCookies.resize( bp->GetHighPriCookies().size() );
        std::copy( 
            bp->GetHighPriCookies().begin(), 
            bp->GetHighPriCookies().end(), 
            mPendCBCookies.begin() );
    }

    return hr;
}


HRESULT MachineX86Base::CancelStep()
{
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    HRESULT hr = S_OK;

    if ( mCurThread->GetStepper() == NULL )
        return S_OK;

    hr = mCurThread->GetStepper()->Cancel();
    if ( FAILED( hr ) )
        return hr;

    delete mCurThread->GetStepper();
    mCurThread->SetStepper( NULL );

    return S_OK;
}

bool MachineX86Base::CheckStepperEnded()
{
    if ( (mCurThread->GetStepper() != NULL) && mCurThread->GetStepper()->IsComplete() )
    {
        delete mCurThread->GetStepper();
        mCurThread->SetStepper( NULL );
        return true;
    }

    return false;
}

HRESULT MachineX86Base::SetStepOut( Address targetAddress )
{
    _ASSERT( mhProcess != NULL );
    _ASSERT( mProcessId != 0 );
    if ( (mhProcess == NULL) || (mProcessId == 0) )
        return E_UNEXPECTED;
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    HRESULT hr = S_OK;
    BPCookie                        cookie = mCurThread->GetStepperCookie();
    IStepperMachine*                stepMac = static_cast<IStepperMachine*>( this );
    auto_ptr< RunToStepper >        stepper;
    MachineAddress                  pc = 0;

    hr = GetCurrentPC( mStoppedThreadId, pc );
    if ( FAILED( hr ) )
        goto Error;

    stepper.reset( new RunToStepper( 
        stepMac, 
        pc, 
        cookie, 
        targetAddress ) );

    hr = CancelStep();
    if ( FAILED( hr ) )
        goto Error;

    hr = stepper->Start();
    if ( FAILED( hr ) )
        goto Error;

    mCurThread->SetStepper( stepper.release() );

Error:
    return hr;
}

HRESULT MachineX86Base::SetStepInstruction( bool stepIn, bool sourceMode )
{
    _ASSERT( mhProcess != NULL );
    _ASSERT( mProcessId != 0 );
    if ( (mhProcess == NULL) || (mProcessId == 0) )
        return E_UNEXPECTED;
    _ASSERT( mStoppedThreadId != 0 );
    _ASSERT( mCurThread != NULL );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    HRESULT hr = S_OK;
    IStepperMachine*                stepMac = static_cast<IStepperMachine*>( this );
    auto_ptr< IStepper >            stepper;
    BPCookie                        cookie = mCurThread->GetStepperCookie();
    MachineAddress                  pc = 0;

    hr = GetCurrentPC( mStoppedThreadId, pc );
    if ( FAILED( hr ) )
        goto Error;

    if ( stepIn )
        hr = MakeStepInStepper( stepMac, pc, sourceMode, cookie, true, stepper );
    else
        hr = MakeStepOverStepper( stepMac, pc, cookie, true, stepper );

    // TODO: shouldn't we get out if making the stepper failed?

    hr = CancelStep();
    if ( FAILED( hr ) )
        goto Error;

    hr = stepper->Start();
    if ( FAILED( hr ) )
        goto Error;

    mCurThread->SetStepper( stepper.release() );

Error:
    return hr;
}

HRESULT MachineX86Base::SetStepRange( bool stepIn, bool sourceMode, AddressRange* ranges, int rangeCount )
{
    _ASSERT( mhProcess != NULL );
    _ASSERT( mProcessId != 0 );
    if ( (mhProcess == NULL) || (mProcessId == 0) )
        return E_UNEXPECTED;
    _ASSERT( mStoppedThreadId != 0 );
    if ( mStoppedThreadId == 0 )
        return E_WRONG_STATE;

    HRESULT hr = S_OK;
    BPCookie                        cookie = mCurThread->GetStepperCookie();
    IStepperMachine*                stepMac = static_cast<IStepperMachine*>( this );
    auto_ptr< RangeStepper >        stepper;
    MachineAddress                  pc = 0;

    hr = GetCurrentPC( mStoppedThreadId, pc );
    if ( FAILED( hr ) )
        goto Error;

    stepper.reset( new RangeStepper( 
        stepMac, 
        stepIn, 
        sourceMode, 
        pc, 
        cookie, 
        ranges, 
        rangeCount ) );

    hr = CancelStep();
    if ( FAILED( hr ) )
        goto Error;

    hr = stepper->Start();
    if ( FAILED( hr ) )
        goto Error;

    mCurThread->SetStepper( stepper.release() );

Error:
    return hr;
}

HRESULT MachineX86Base::GetThreadContext( uint32_t threadId, void* context, uint32_t size )
{
    _ASSERT( mhProcess != NULL );
    _ASSERT( mProcessId != 0 );
    if ( (mhProcess == NULL) || (mProcessId == 0) )
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
    _ASSERT( mProcessId != 0 );
    if ( (mhProcess == NULL) || (mProcessId == 0) )
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

HRESULT MachineX86Base::SetStepperSingleStep( bool enable )
{
    _ASSERT( mStoppedThreadId != 0 );
    return SetSingleStep( mStoppedThreadId, enable );
}

HRESULT MachineX86Base::SetStepperBreakpoint( MachineAddress address, BPCookie cookie )
{
    return SetBreakpointInternal( address, cookie, BPPri_Low );
}

HRESULT MachineX86Base::RemoveStepperBreakpoint( MachineAddress address, BPCookie cookie )
{
    return RemoveBreakpointInternal( address, cookie, BPPri_Low );
}

HRESULT MachineX86Base::ReadStepperMemory( 
    MachineAddress address, 
    SIZE_T length, 
    SIZE_T& lengthRead, 
    SIZE_T& lengthUnreadable, 
    uint8_t* buffer )
{
    HRESULT hr = S_OK;

    hr = ::ReadMemory( mProcess->GetHandle(), address, length, lengthRead, lengthUnreadable, buffer );
    if ( FAILED( hr ) )
        return hr;

    if ( lengthRead > 0 )
    {
        MachineAddress  startAddr = address;
        MachineAddress  endAddr = address + lengthRead - 1;

        // unpatch all BPs from the memory area we're returning

        for ( BPAddressTable::iterator it = mAddrTable->begin();
            it != mAddrTable->end();
            it++ )
        {
            Breakpoint* bp = it->second;

            if ( bp->Patched() && (it->first >= startAddr) && (it->first <= endAddr) )
            {
                MachineAddress  offset = it->first - startAddr;
                buffer[ offset ] = bp->GetOriginalInstructionByte();
            }
        }
    }

    return hr;
}

HRESULT MachineX86Base::WriteStepperMemory( 
    MachineAddress address, 
    SIZE_T length, 
    SIZE_T& lengthWritten, 
    uint8_t* buffer )
{
    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;

    // The memory we're overwriting might be patched with BPs, so do it in 3 steps:
    // 1. For each BP in the target mem. range, 
    //      replace its original saved data with what's in the source buffer.
    // 2. For each BP in the target mem. range,
    //      patch a BP instruction in the source buffer.
    // 3. Write the source buffer to the process.

    if ( length == 0 )
        return S_OK;

    MachineAddress  startAddr = address;
    MachineAddress  endAddr = address + length - 1;

    for ( BPAddressTable::iterator it = mAddrTable->begin();
        it != mAddrTable->end();
        it++ )
    {
        Breakpoint* bp = it->second;

        if ( bp->Patched() && (it->first >= startAddr) && (it->first <= endAddr) )
        {
            MachineAddress  offset = it->first - startAddr;

            bp->SetTempInstructionByte( buffer[ offset ] );
            buffer[ offset ] = BreakpointInstruction;
        }
    }

    bRet = WriteProcessMemory( mProcess->GetHandle(), (void*) address, buffer, length, &lengthWritten );
    if ( !bRet )
        return HRESULT_FROM_WIN32( hr );

    // now commit all the bytes for the BPs we overwrote

    if ( lengthWritten == 0 )
        return S_OK;

    endAddr = address + lengthWritten - 1;

    for ( BPAddressTable::iterator it = mAddrTable->begin();
        it != mAddrTable->end();
        it++ )
    {
        Breakpoint* bp = it->second;

        if ( bp->Patched() && (it->first >= startAddr) && (it->first <= endAddr) )
        {
            bp->SetOriginalInstructionByte( bp->GetTempInstructionByte() );
        }
    }

    return S_OK;
}

RunMode MachineX86Base::CanStepperStopAtFunction( MachineAddress address )
{
    return mCallback->OnCallProbe( mProcess, mStoppedThreadId, (Address) address );
}
