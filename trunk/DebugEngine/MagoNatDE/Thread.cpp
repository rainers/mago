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
#include "EnumFrameInfo.h"
#include "DebuggerProxy.h"
#include "RegisterSet.h"

#include <dbghelp.h>


namespace Mago
{
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
    { return E_NOTIMPL;} 
    HRESULT Thread::SetNextStatement( IDebugStackFrame2* pStackFrame, 
                                       IDebugCodeContext2* pCodeContext )
    { return E_NOTIMPL;} 
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
                ptp->dwThreadId = mCoreThread->GetId();
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

        *pdwThreadId = mCoreThread->GetId();
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

        HRESULT     hr = S_OK;
        CONTEXT     context = { 0 };
        Callstack   callstack;

        // TODO: we should get this another way
        context.ContextFlags = CONTEXT_FULL 
            | CONTEXT_FLOATING_POINT;
        if ( !GetThreadContext( mCoreThread->GetHandle(), &context ) )
            return GetLastHr();

        mCurPC = context.Eip;
        // in case we can't get the return address of top frame, 
        // make sure our StepOut method knows that we don't know the caller's PC
        mCallerPC = 0;

        hr = BuildCallstack( context, callstack );
        if ( FAILED( hr ) )
        {
            hr = BuildTopFrameCallstack( context, callstack );
            if ( FAILED( hr ) )
                return hr;
        }

        hr = MakeEnumFrameInfoFromCallstack( callstack, dwFieldSpec, nRadix, ppEnum );

        return hr;
    }


    //------------------------------------------------------------------------

    ::Thread* Thread::GetCoreThread()
    {
        return mCoreThread.Get();
    }

    void Thread::SetCoreThread( ::Thread* thread )
    {
        mCoreThread = thread;
    }

    void Thread::SetProgram( Program* prog, DebuggerProxy* pollThread )
    {
        mProg = prog;
        mDebugger = pollThread;
    }

    IProcess*   Thread::GetCoreProcess()
    {
        return mProg->GetCoreProcess();
    }

    DebuggerProxy* Thread::GetDebuggerProxy()
    {
        return mDebugger;
    }

    HRESULT Thread::Step( ::IProcess* coreProc, STEPKIND sk, STEPUNIT step, bool handleException )
    {
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

    HRESULT Thread::StepStatement( ::IProcess* coreProc, STEPKIND sk, bool handleException )
    {
        _ASSERT( (sk == STEP_OVER) || (sk == STEP_INTO) );
        if ( (sk != STEP_OVER) && (sk != STEP_INTO) )
            return E_NOTIMPL;

        HRESULT hr = S_OK;
        bool    stepIn = (sk == STEP_INTO);
        RefPtr<Module>          mod;
        RefPtr<MagoST::ISession>    session;
        AddressRange            addrRanges[1] = { 0 };
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
        len = line.Length;

        addrRanges[0].Begin = (Address) addrBegin;
        addrRanges[0].End = (Address) (addrBegin + len - 1);

        hr = mDebugger->StepRange( coreProc, stepIn, true, addrRanges, 1, handleException );

        return hr;
    }

    HRESULT Thread::StepInstruction( ::IProcess* coreProc, STEPKIND sk, bool handleException )
    {
        _ASSERT( (sk == STEP_OVER) || (sk == STEP_INTO) );
        if ( (sk != STEP_OVER) && (sk != STEP_INTO) )
            return E_NOTIMPL;

        HRESULT hr = S_OK;
        bool    stepIn = (sk == STEP_INTO);

        hr = mDebugger->StepInstruction( coreProc, stepIn, false, handleException );

        return hr;
    }

    HRESULT Thread::StepOut( ::IProcess* coreProc, bool handleException )
    {
        HRESULT hr = S_OK;
        Address targetAddr = mCallerPC;

        if ( targetAddr == 0 )
            return E_FAIL;

        hr = mDebugger->StepOut( coreProc, targetAddr, handleException );

        return hr;
    }


    //------------------------------------------------------------------------

    HRESULT Thread::BuildCallstack( const CONTEXT& context, Callstack& callstack )
    {
        OutputDebugStringA( "Thread::BuildCallstack\n" );

        HRESULT         hr = S_OK;
        STACKFRAME64    stackFrame64 = { 0 };
        CONTEXT         newContext = context;
        int             frameIndex = 0;

        stackFrame64.AddrPC.Mode = AddrModeFlat;
        stackFrame64.AddrPC.Offset = context.Eip;
        stackFrame64.AddrStack.Mode = AddrModeFlat;
        stackFrame64.AddrStack.Offset = context.Esp;
        stackFrame64.AddrFrame.Mode = AddrModeFlat;
        stackFrame64.AddrFrame.Offset = context.Ebp;

        callstack.clear();

        while ( WalkStack( stackFrame64, &newContext ) )
        {
            RefPtr<Module>      mod;
            RefPtr<StackFrame>  stackFrame;
            RefPtr<IRegisterSet> regSet;
            UINT64              addr = 0;

            addr = stackFrame64.AddrPC.Offset;

            // if we haven't gotten the first return address, then do so now
            if ( frameIndex == 1 )
                mCallerPC = (Address) addr;

            mProg->FindModuleContainingAddress( (Address) addr, mod );

            hr = MakeCComObject( stackFrame );
            if ( FAILED( hr ) )
                return hr;

            if ( frameIndex == 0 )
                regSet = new RegisterSet( context, mCoreThread );
            else
                regSet = new TinyRegisterSet( 
                    (Address) stackFrame64.AddrPC.Offset,
                    (Address) stackFrame64.AddrStack.Offset,
                    (Address) stackFrame64.AddrFrame.Offset );

            if ( regSet == NULL )
                return E_OUTOFMEMORY;

            stackFrame->Init( (Address) addr, regSet, this, mod.Get() );

            callstack.push_back( stackFrame );

            frameIndex++;
        }

        // TODO: what if we couldn't get any frames?

        return S_OK;
    }

    bool Thread::WalkStack( STACKFRAME64& stackFrame, void* context )
    {
        return StackWalk64( 
            IMAGE_FILE_MACHINE_I386,
            this,
            NULL,
            &stackFrame,
            context,
            ReadProcessMemory64,
            FunctionTableAccess64,
            GetModuleBase64,
            NULL ) ? true : false;
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
        Thread* pThis = (Thread*) hProcess;

        HRESULT hr = S_OK;
        SIZE_T  lenRead = 0;
        SIZE_T  lenUnreadable = 0;
        RefPtr<IProcess>    proc;

        pThis->mProg->GetCoreProcess( proc.Ref() );

        hr = pThis->mDebugger->ReadMemory( 
            proc.Get(), 
            (Address) lpBaseAddress, 
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
        return NULL;
    }

    DWORD64 Thread::GetModuleBase64(
      HANDLE hProcess,
      DWORD64 address
    )
    {
        _ASSERT( hProcess != NULL );
        Thread* pThis = (Thread*) hProcess;

        RefPtr<Module>      mod;

        if ( !pThis->mProg->FindModuleContainingAddress( (Address) address, mod ) )
            return 0;

        return mod->GetAddress();
    }

    HRESULT Thread::BuildTopFrameCallstack( const CONTEXT& context, Callstack& callstack )
    {
        HRESULT             hr = S_OK;
        const Address       addr = context.Eip;
        RefPtr<Module>      mod;
        RefPtr<StackFrame>  stackFrame;
        RefPtr<RegisterSet> regSet;

        callstack.clear();

        mProg->FindModuleContainingAddress( addr, mod );

        hr = MakeCComObject( stackFrame );
        if ( FAILED( hr ) )
            return hr;

        regSet = new RegisterSet( context, mCoreThread );
        if ( regSet == NULL )
            return E_OUTOFMEMORY;

        stackFrame->Init( addr, regSet, this, mod.Get() );

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
