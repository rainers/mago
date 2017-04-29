/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Thread.h"
#include "Program.h"
#include "Module.h"
#include "StackFrame.h"
#include "CodeContext.h"
#include "EnumFrameInfo.h"
#include "IDebuggerProxy.h"
#include "RegisterSet.h"
#include "ArchData.h"
#include "ICoreProcess.h"


namespace Mago
{

class PdataCache
{
    typedef AddressRange64 MapKey;

    typedef bool (*RangePred)( const MapKey& left, const MapKey& right );
    static bool RangeLess( const MapKey& left, const MapKey& right );

    typedef std::vector<BYTE> PdataBuffer;
    typedef std::map<MapKey, int, RangePred> PdataMap;

    PdataBuffer mBuffer;
    PdataMap    mMap;
    int         mEntrySize;

public:
    PdataCache( int pdataSize );
    void* Find( Address64 address );
    void* Add( Address64 begin, Address64 end, void* pdata );
};

PdataCache::PdataCache( int pdataSize )
    :   mMap( RangeLess ),
        mEntrySize( pdataSize )
{
}

bool PdataCache::RangeLess( const MapKey& left, const MapKey& right )
{
    return left.End < right.Begin;
}

void* PdataCache::Find( Address64 address )
{
    MapKey range = { address, address };

    PdataMap::iterator it = mMap.find( range );
    if ( it == mMap.end() )
        return NULL;

    return &mBuffer[it->second];
}

void* PdataCache::Add( Address64 begin, Address64 end, void* pdata )
{
    size_t origSize = mBuffer.size();
    mBuffer.resize( mBuffer.size() + mEntrySize );

    memcpy( &mBuffer[origSize], pdata, mEntrySize );

    MapKey range = { begin, end };
    mMap.insert( PdataMap::value_type( range, origSize ) );
    return &mBuffer[origSize];
}

}


namespace Mago
{
    struct WalkContext
    {
        Mago::Thread*       Thread;
        PdataCache*         Cache;
        UniquePtr<BYTE[]>   TempEntry;
    };


    Thread::Thread()
        :   mDebugger( NULL ),
            mCurPC( 0 ),
            mCallerPC( 0 )
    {
    }

    Thread::~Thread()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugThread2 methods 

    HRESULT Thread::SetThreadName( LPCOLESTR pszName )
    { return E_NOTIMPL;}
    HRESULT Thread::GetName( BSTR* pbstrName )
    { return E_NOTIMPL;} 

    HRESULT Thread::GetProgram( IDebugProgram2** ppProgram )
    {
        if ( ppProgram == NULL )
            return E_INVALIDARG;

        _ASSERT( mProg.Get() != NULL );
        return mProg->QueryInterface( __uuidof( IDebugProgram2 ), (void**) ppProgram );
    }

    HRESULT Thread::CanSetNextStatement( IDebugStackFrame2* pStackFrame, 
                                         IDebugCodeContext2* pCodeContext )
    {
        if ( pCodeContext == NULL )
            return E_INVALIDARG;

        CComQIPtr<IMagoMemoryContext> magoMem = pCodeContext;
        if ( magoMem == NULL )
            return E_INVALIDARG;

        return S_OK;
    } 
    HRESULT Thread::SetNextStatement( IDebugStackFrame2* pStackFrame, 
                                      IDebugCodeContext2* pCodeContext )
    {
        if ( pCodeContext == NULL )
            return E_INVALIDARG;

        CComQIPtr<IMagoMemoryContext> magoMem = pCodeContext;
        if ( magoMem == NULL )
            return E_INVALIDARG;

        Address64 addr = 0;
        magoMem->GetAddress( addr );
        if ( addr == mCurPC )
            return S_OK;

        RefPtr<IRegisterSet>    topRegSet;

        HRESULT hr = mDebugger->GetThreadContext( mProg->GetCoreProcess(), mCoreThread, topRegSet.Ref() );
        if ( FAILED( hr ) )
            return hr;
        hr = topRegSet->SetPC( addr );
        if ( FAILED( hr ) )
            return hr;

        hr = mDebugger->SetThreadContext( mProg->GetCoreProcess(), mCoreThread, topRegSet );
        return hr;
    }

    HRESULT Thread::Suspend( DWORD* pdwSuspendCount )
    { return E_NOTIMPL;} 
    HRESULT Thread::Resume( DWORD* pdwSuspendCount )
    { return E_NOTIMPL;} 

    HRESULT Thread::GetThreadProperties( THREADPROPERTY_FIELDS dwFields, 
                                          THREADPROPERTIES* ptp )
    {
        if ( ptp == NULL )
            return E_INVALIDARG;

        ptp->dwFields = 0;

        if ( (dwFields & TPF_ID) != 0 )
        {
            if ( mCoreThread.Get() != NULL )
            {
                ptp->dwThreadId = mCoreThread->GetTid();
                ptp->dwFields |= TPF_ID;
            }
        }

        if ( (dwFields & TPF_SUSPENDCOUNT) != 0 )
        {
        }

        if ( (dwFields & TPF_STATE) != 0 )
        {
        }

        if ( (dwFields & TPF_PRIORITY) != 0 )
        {
        }

        if ( (dwFields & TPF_NAME) != 0 )
        {
        }

        if ( (dwFields & TPF_LOCATION) != 0 )
        {
        }

        return S_OK;
    }

    HRESULT Thread::GetLogicalThread( IDebugStackFrame2* pStackFrame, 
                                       IDebugLogicalThread2** ppLogicalThread )
    {
        UNREFERENCED_PARAMETER( pStackFrame );
        UNREFERENCED_PARAMETER( ppLogicalThread );
        return E_NOTIMPL;
    }

    HRESULT Thread::GetThreadId( DWORD* pdwThreadId )
    {
        if ( pdwThreadId == NULL )
            return E_INVALIDARG;

        if ( mCoreThread.Get() == NULL )
            return E_FAIL;

        *pdwThreadId = mCoreThread->GetTid();
        return S_OK;
    }

    HRESULT Thread::EnumFrameInfo( FRAMEINFO_FLAGS dwFieldSpec, 
                                    UINT nRadix, 
                                    IEnumDebugFrameInfo2** ppEnum )
    {
        if ( ppEnum == NULL )
            return E_INVALIDARG;
        if ( dwFieldSpec == 0 )
            return E_INVALIDARG;
        if ( nRadix == 0 )
            return E_INVALIDARG;

        HRESULT                 hr = S_OK;
        Callstack               callstack;
        RefPtr<IRegisterSet>    topRegSet;

        hr = mDebugger->GetThreadContext( mProg->GetCoreProcess(), mCoreThread, topRegSet.Ref() );
        if ( FAILED( hr ) )
            return hr;

        mCurPC = (Address64) topRegSet->GetPC();
        // in case we can't get the return address of top frame, 
        // make sure our StepOut method knows that we don't know the caller's PC
        mCallerPC = 0;

        hr = BuildCallstack( topRegSet, callstack );
        if ( FAILED( hr ) )
            return hr;

        hr = MakeEnumFrameInfoFromCallstack( callstack, dwFieldSpec, nRadix, ppEnum );

        return hr;
    }


    //------------------------------------------------------------------------

    ICoreThread* Thread::GetCoreThread()
    {
        return mCoreThread.Get();
    }

    void Thread::SetCoreThread( ICoreThread* thread )
    {
        mCoreThread = thread;
    }

    Program*    Thread::GetProgram()
    {
        return mProg;
    }

    void Thread::SetProgram( Program* prog, IDebuggerProxy* pollThread )
    {
        mProg = prog;
        mDebugger = pollThread;
    }

    ICoreProcess*   Thread::GetCoreProcess()
    {
        return mProg->GetCoreProcess();
    }

    IDebuggerProxy* Thread::GetDebuggerProxy()
    {
        return mDebugger;
    }

    HRESULT Thread::Step( ICoreProcess* coreProc, STEPKIND sk, STEPUNIT step, bool handleException )
    {
        _RPT1( _CRT_WARN, "Thread::Step (%d)\n", mCoreThread->GetTid() );

        if ( sk == STEP_BACKWARDS )
            return E_NOTIMPL;

        // works for statements and instructions
        if ( sk == STEP_OUT )
            return StepOut( coreProc, handleException );

        if ( step == STEP_INSTRUCTION )
            return StepInstruction( coreProc, sk, handleException );

        if ( (step == STEP_STATEMENT) || (step == STEP_LINE) )
            return StepStatement( coreProc, sk, handleException );

        return E_NOTIMPL;
    }

    HRESULT Thread::StepStatement( ICoreProcess* coreProc, STEPKIND sk, bool handleException )
    {
        _ASSERT( (sk == STEP_OVER) || (sk == STEP_INTO) );
        if ( (sk != STEP_OVER) && (sk != STEP_INTO) )
            return E_NOTIMPL;

        HRESULT hr = S_OK;
        bool    stepIn = (sk == STEP_INTO);
        RefPtr<Module>          mod;
        RefPtr<MagoST::ISession>    session;
        AddressRange64              addrRange = { 0 };
        MagoST::LineNumber      line;

        if ( !mProg->FindModuleContainingAddress( mCurPC, mod ) )
            return E_NOT_FOUND;

        if ( !mod->GetSymbolSession( session ) )
            return E_NOT_FOUND;

        uint16_t    sec = 0;
        uint32_t    offset = 0;
        sec = session->GetSecOffsetFromVA( mCurPC, offset );
        if ( sec == 0 )
            return E_FAIL;

        if ( !session->FindLine( sec, offset, line ) )
            return E_FAIL;

        UINT64  addrBegin = 0;
        DWORD   len = 0;

        addrBegin = session->GetVAFromSecOffset( sec, line.Offset );
        if( addrBegin == 0 )
            return E_FAIL;
        len = line.Length;

        addrRange.Begin = (Address64) addrBegin;
        addrRange.End = (Address64) (addrBegin + len - 1);

        hr = mDebugger->StepRange( coreProc, stepIn, addrRange, handleException );

        return hr;
    }

    HRESULT Thread::StepInstruction( ICoreProcess* coreProc, STEPKIND sk, bool handleException )
    {
        _ASSERT( (sk == STEP_OVER) || (sk == STEP_INTO) );
        if ( (sk != STEP_OVER) && (sk != STEP_INTO) )
            return E_NOTIMPL;

        HRESULT hr = S_OK;
        bool    stepIn = (sk == STEP_INTO);

        hr = mDebugger->StepInstruction( coreProc, stepIn, handleException );

        return hr;
    }

    HRESULT Thread::StepOut( ICoreProcess* coreProc, bool handleException )
    {
        HRESULT hr = S_OK;
        Address64 targetAddr = mCallerPC;

        if ( targetAddr == 0 )
            return E_FAIL;

        hr = mDebugger->StepOut( coreProc, targetAddr, handleException );

        return hr;
    }


    //------------------------------------------------------------------------

    HRESULT Thread::BuildCallstack( IRegisterSet* topRegSet, Callstack& callstack )
    {
        Log::LogMessage( "Thread::BuildCallstack\n" );

        HRESULT             hr = S_OK;
        int                 frameIndex = 0;
        ArchData*           archData = NULL;
        StackWalker*        pWalker = NULL;
        UniquePtr<StackWalker> walker;
        WalkContext         walkContext;
        int                 pdataSize = 0;

        archData = mProg->GetCoreProcess()->GetArchData();
        pdataSize = archData->GetPDataSize();

        PdataCache          pdataCache( pdataSize );

        walkContext.Thread = this;
        walkContext.Cache = &pdataCache;
        walkContext.TempEntry.Attach( new BYTE[pdataSize] );

        if ( walkContext.TempEntry.IsEmpty() )
            return E_OUTOFMEMORY;

        hr = AddCallstackFrame( topRegSet, callstack );
        if ( FAILED( hr ) )
            return hr;

        hr = archData->BeginWalkStack( 
            topRegSet,
            &walkContext,
            ReadProcessMemory64,
            FunctionTableAccess64,
            GetModuleBase64,
            pWalker );
        if ( FAILED( hr ) )
            return hr;

        walker.Attach( pWalker );
        // walk past the first frame, because we have it already
        walker->WalkStack();

        while ( walker->WalkStack() )
        {
            RefPtr<IRegisterSet> regSet;
            UINT64              addr = 0;
            const void*         context = NULL;
            uint32_t            contextSize = 0;

            walker->GetThreadContext( context, contextSize );

            if ( frameIndex == 0 )
                hr = archData->BuildRegisterSet( context, contextSize, regSet.Ref() );
            else
                hr = archData->BuildTinyRegisterSet( context, contextSize, regSet.Ref() );

            if ( FAILED( hr ) )
                return hr;

            addr = regSet->GetPC();

            // if we haven't gotten the first return address, then do so now
            if ( frameIndex == 0 )
                mCallerPC = (Address64) addr;

            hr = AddCallstackFrame( regSet, callstack );
            if ( FAILED( hr ) )
                return hr;

            frameIndex++;
        }

        return S_OK;
    }

    BOOL Thread::ReadProcessMemory64(
      HANDLE hProcess,
      DWORD64 lpBaseAddress,
      PVOID lpBuffer,
      DWORD nSize,
      LPDWORD lpNumberOfBytesRead
    )
    {
        _ASSERT( hProcess != NULL );
        WalkContext*    walkContext = (WalkContext*) hProcess;
        Thread*         pThis = walkContext->Thread;

        HRESULT     hr = S_OK;
        uint32_t    lenRead = 0;
        uint32_t    lenUnreadable = 0;
        RefPtr<ICoreProcess>    proc;

        pThis->mProg->GetCoreProcess( proc.Ref() );

        hr = pThis->mDebugger->ReadMemory( 
            proc.Get(), 
            (Address64) lpBaseAddress, 
            nSize, 
            lenRead, 
            lenUnreadable, 
            (uint8_t*) lpBuffer );
        if ( FAILED( hr ) )
            return FALSE;

        *lpNumberOfBytesRead = lenRead;

        return TRUE;
    }

    PVOID Thread::FunctionTableAccess64(
      HANDLE hProcess,
      DWORD64 addrBase
    )
    {
        _ASSERT( hProcess != NULL );

        HRESULT         hr = S_OK;
        WalkContext*    walkContext = (WalkContext*) hProcess;
        Thread*         pThis = walkContext->Thread;
        ArchData*       archData = pThis->GetCoreProcess()->GetArchData();
        uint32_t        size = 0;
        int             pdataSize = archData->GetPDataSize();
        void*           pdata = NULL;

        if ( pdataSize == 0 )
            return NULL;

        pdata = walkContext->Cache->Find( addrBase );
        if ( pdata != NULL )
            return pdata;

        RefPtr<Module>      mod;

        if ( !pThis->mProg->FindModuleContainingAddress( (Address64) addrBase, mod ) )
            return NULL;

        IDebuggerProxy* debugger = pThis->GetDebuggerProxy();

        hr = debugger->GetPData( 
            pThis->GetCoreProcess(), addrBase, mod->GetAddress(), pdataSize, size, 
            walkContext->TempEntry.Get() );
        if ( hr != S_OK )
            return NULL;

        Address64   begin;
        Address64   end;

        pThis->GetCoreProcess()->GetArchData()->GetPDataRange( 
            mod->GetAddress(), walkContext->TempEntry.Get(), begin, end );

        pdata = walkContext->Cache->Add( begin, end, walkContext->TempEntry.Get() );
        return pdata;
    }

    DWORD64 Thread::GetModuleBase64(
      HANDLE hProcess,
      DWORD64 address
    )
    {
        _ASSERT( hProcess != NULL );
        WalkContext*    walkContext = (WalkContext*) hProcess;
        Thread*         pThis = walkContext->Thread;

        RefPtr<Module>      mod;

        if ( !pThis->mProg->FindModuleContainingAddress( (Address64) address, mod ) )
            return 0;

        return mod->GetAddress();
    }

    HRESULT Thread::AddCallstackFrame( IRegisterSet* regSet, Callstack& callstack )
    {
        HRESULT             hr = S_OK;
        const Address64     addr = (Address64) regSet->GetPC();
        RefPtr<Module>      mod;
        RefPtr<StackFrame>  stackFrame;
        ArchData*           archData = NULL;

        mProg->FindModuleContainingAddress( addr, mod );

        hr = MakeCComObject( stackFrame );
        if ( FAILED( hr ) )
            return hr;

        archData = mProg->GetCoreProcess()->GetArchData();

        stackFrame->Init( addr, regSet, this, mod.Get(), archData->GetPointerSize() );

        callstack.push_back( stackFrame );

        return hr;
    }

    HRESULT Thread::MakeEnumFrameInfoFromCallstack( 
        const Callstack& callstack,
        FRAMEINFO_FLAGS dwFieldSpec, 
        UINT nRadix, 
        IEnumDebugFrameInfo2** ppEnum )
    {
        _ASSERT( ppEnum != NULL );
        _ASSERT( dwFieldSpec != 0 );
        _ASSERT( nRadix != 0 );

        HRESULT hr = S_OK;
        FrameInfoArray  array( callstack.size() );
        RefPtr<EnumDebugFrameInfo>  enumFrameInfo;
        int i = 0;

        for ( Callstack::const_iterator it = callstack.begin();
            it != callstack.end();
            it++, i++ )
        {
            hr = (*it)->GetInfo( dwFieldSpec, nRadix, &array[i] );
            if ( FAILED( hr ) )
                return hr;
        }

        hr = MakeCComObject( enumFrameInfo );
        if ( FAILED( hr ) )
            return hr;

        hr = enumFrameInfo->Init( array.Get(), array.Get() + array.GetLength(), NULL, AtlFlagTakeOwnership );
        if ( FAILED( hr ) )
            return hr;

        array.Detach();
        return enumFrameInfo->QueryInterface( __uuidof( IEnumDebugFrameInfo2 ), (void**) ppEnum );
    }
}
