/*
   Copyright (c) 2014 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "MachineX64.h"
#include "Thread.h"
#include "ThreadX86.h"


// Define a set of minimum registers to cache.
// It really only needs to be control registers, but you might as well cache 
// most of the rest, which are often read from the last thread that reported an event.

#if defined( _WIN64 )
enum
{ 
    MIN_CONTEXT_FLAGS = 
        CONTEXT_FULL 
        | CONTEXT_SEGMENTS 
};
typedef CONTEXT                         CONTEXT_X64;
#define CONTEXT_X64_AMD64               CONTEXT_AMD64
#define CONTEXT_X64_CONTROL             CONTEXT_CONTROL
#define CONTEXT_X64_INTEGER             CONTEXT_INTEGER
#define CONTEXT_X64_SEGMENTS            CONTEXT_SEGMENTS
#define CONTEXT_X64_FLOATING_POINT      CONTEXT_FLOATING_POINT
#define CONTEXT_X64_DEBUG_REGISTERS     CONTEXT_DEBUG_REGISTERS
#define GetThreadContextX86             ::GetThreadContext
#define SetThreadContextX86             ::SetThreadContext
#define SuspendThreadX86                ::SuspendThread
#else
#error MachineX64 must be enabled only for x64 builds.
#endif

const DWORD TRACE_FLAG = 0x100;


HRESULT MakeMachineX64( IMachine*& machine )
{
    HRESULT hr = S_OK;
    RefPtr<MachineX64>          machX64( new MachineX64() );

    if ( machX64.Get() == NULL )
        return E_OUTOFMEMORY;

    hr = machX64->Init();
    if ( FAILED( hr ) )
        return hr;

    machine = machX64.Detach();
    return S_OK;
}

MachineX64::MachineX64()
    :   mIsContextCached( false ),
        mEnableSS( false )
{
    memset( &mContext, 0, sizeof mContext );
}

bool MachineX64::Is64Bit()
{
    return true;
}

HRESULT MachineX64::CacheThreadContext()
{
    HRESULT hr = S_OK;
    ThreadX86Base* threadX86 = GetStoppedThread();
    Thread* thread = threadX86->GetExecThread();

    mContext.ContextFlags = MIN_CONTEXT_FLAGS;
    if ( !GetThreadContextX86( thread->GetHandle(), &mContext ) )
    {
        hr = GetLastHr();
        goto Error;
    }

    mIsContextCached = true;

Error:
    return hr;
}

HRESULT MachineX64::FlushThreadContext()
{
    if ( !mIsContextCached )
        return S_OK;

    HRESULT hr = S_OK;
    ThreadX86Base* threadX86 = GetStoppedThread();
    Thread* thread = threadX86->GetExecThread();

    if ( mEnableSS )
    {
        mContext.EFlags |= TRACE_FLAG;
        mEnableSS = false;
    }

    if ( !SetThreadContextX86( thread->GetHandle(), &mContext ) )
    {
        hr = GetLastHr();
        goto Error;
    }

    mIsContextCached = false;

Error:
    return hr;
}

HRESULT MachineX64::ChangeCurrentPC( int32_t byteOffset )
{
    _ASSERT( mIsContextCached );
    if ( !mIsContextCached )
        return E_FAIL;

    mContext.Rip += byteOffset;
    return S_OK;
}

HRESULT MachineX64::SetSingleStep( bool enable )
{
    _ASSERT( mIsContextCached );
    if ( !mIsContextCached )
        return E_FAIL;

    mEnableSS = enable;
    return S_OK;
}

HRESULT MachineX64::ClearSingleStep()
{
    _ASSERT( mIsContextCached );
    if ( !mIsContextCached )
        return E_FAIL;

    mContext.EFlags &= ~TRACE_FLAG;

    return S_OK;
}

HRESULT MachineX64::GetCurrentPC( Address& address )
{
    _ASSERT( mIsContextCached );
    if ( !mIsContextCached )
        return E_FAIL;

    address = mContext.Rip;
    return S_OK;
}

// Gets the return address in the newest stack frame.
HRESULT MachineX64::GetReturnAddress( Address& address )
{
    _ASSERT( mIsContextCached );
    if ( !mIsContextCached )
        return E_FAIL;

    BOOL bRet = ReadProcessMemory( 
        GetProcessHandle(), 
        (void*) mContext.Rsp, 
        &address, 
        sizeof address, 
        NULL );
    if ( !bRet )
        return GetLastHr();

    return S_OK;
}

HRESULT MachineX64::SuspendThread( Thread* thread )
{
    DWORD   suspendCount = SuspendThreadX86( thread->GetHandle() );

    if ( suspendCount == (DWORD) -1 )
    {
        HRESULT hr = GetLastHr();

        // if the thread can't be accessed, then it's probably on the way out
        // and there's nothing we should do about it
        if ( hr == E_ACCESSDENIED )
            return S_OK;

        return hr;
    }

    return S_OK;
}

HRESULT MachineX64::ResumeThread( Thread* thread )
{
    // there's no Wow64ResumeThread
    DWORD   suspendCount = ::ResumeThread( thread->GetHandle() );

    if ( suspendCount == (DWORD) -1 )
    {
        HRESULT hr = GetLastHr();

        // if the thread can't be accessed, then it's probably on the way out
        // and there's nothing we should do about it
        if ( hr == E_ACCESSDENIED )
            return S_OK;

        return hr;
    }

    return S_OK;
}

static void CopyContext( DWORD flags, const CONTEXT_X64* srcContext, CONTEXT_X64* dstContext )
{
    _ASSERT( srcContext != NULL );
    _ASSERT( dstContext != NULL );
    _ASSERT( (flags & ~MIN_CONTEXT_FLAGS) == 0 );

    if ( (flags & CONTEXT_X64_CONTROL) == CONTEXT_X64_CONTROL )
    {
        dstContext->SegCs = srcContext->SegCs;
        dstContext->SegSs = srcContext->SegSs;
        dstContext->Rsp = srcContext->Rsp;
        dstContext->Rip = srcContext->Rip;
        dstContext->EFlags = srcContext->EFlags;
    }

    if ( (flags & CONTEXT_X64_INTEGER) == CONTEXT_X64_INTEGER )
    {
        DWORD64 rsp = dstContext->Rsp;
        memcpy( &dstContext->Rax, &srcContext->Rax, sizeof( dstContext->Rax ) * 16 );
        dstContext->Rsp = rsp;
    }

    if ( (flags & CONTEXT_X64_SEGMENTS) == CONTEXT_X64_SEGMENTS )
    {
        dstContext->SegDs = srcContext->SegDs;
        dstContext->SegEs = srcContext->SegEs;
        dstContext->SegFs = srcContext->SegFs;
        dstContext->SegGs = srcContext->SegGs;
    }

    if ( (flags & CONTEXT_X64_FLOATING_POINT) == CONTEXT_X64_FLOATING_POINT )
    {
        dstContext->FltSave = srcContext->FltSave;
    }
}

HRESULT MachineX64::GetThreadContextWithCache( HANDLE hThread, void* contextBuf, uint32_t size )
{
    _ASSERT( hThread != NULL );
    _ASSERT( contextBuf != NULL );
    _ASSERT( size >= sizeof( CONTEXT_X64 ) );
    _ASSERT( mIsContextCached );
    if ( size < sizeof( CONTEXT_X64 ) )
        return E_INVALIDARG;

    // ContextFlags = 0 and CONTEXT_AMD64 are OK

    CONTEXT_X64* context = (CONTEXT_X64*) contextBuf;
    DWORD callerFlags       = context->ContextFlags & ~CONTEXT_X64_AMD64;
    DWORD cacheFlags        = mContext.ContextFlags & ~CONTEXT_X64_AMD64;
    DWORD cachedFlags       = callerFlags & cacheFlags;
    DWORD notCachedFlags    = callerFlags & ~cacheFlags;

    if ( notCachedFlags != 0 )
    {
        // only get from the target what isn't cached
        context->ContextFlags = notCachedFlags | CONTEXT_X64_AMD64;
        if ( !GetThreadContextX86( hThread, context ) )
            return GetLastHr();
    }

    if ( cachedFlags != 0 )
    {
        CopyContext( cachedFlags | CONTEXT_X64_AMD64, &mContext, context );
        context->ContextFlags |= cachedFlags | CONTEXT_X64_AMD64;
    }

    return S_OK;
}

HRESULT MachineX64::SetThreadContextWithCache( HANDLE hThread, const void* contextBuf, uint32_t size )
{
    _ASSERT( hThread != NULL );
    _ASSERT( contextBuf != NULL );
    _ASSERT( size >= sizeof( CONTEXT_X64 ) );
    _ASSERT( mIsContextCached );
    if ( size < sizeof( CONTEXT_X64 ) )
        return E_INVALIDARG;

    // ContextFlags = 0 and CONTEXT_AMD64 are OK

    const CONTEXT_X64* context = (const CONTEXT_X64*) contextBuf;
    DWORD callerFlags       = context->ContextFlags & ~CONTEXT_X64_AMD64;
    DWORD cacheFlags        = mContext.ContextFlags & ~CONTEXT_X64_AMD64;
    DWORD cachedFlags       = callerFlags & cacheFlags;
    DWORD notCachedFlags    = callerFlags & ~cacheFlags;

    if ( notCachedFlags != 0 )
    {
        // set everything, in order to avoid copying the context 
        // or writing restricted flags to the const context
        if ( !SetThreadContextX86( hThread, context ) )
            return GetLastHr();
    }

    if ( cachedFlags != 0 )
    {
        CopyContext( cachedFlags | CONTEXT_X64_AMD64, context, &mContext );
    }

    return S_OK;
}

HRESULT MachineX64::GetThreadContextInternal( 
    uint32_t threadId, 
    uint32_t features, 
    uint64_t extFeatures, 
    void* contextBuf, 
    uint32_t size )
{
    UNREFERENCED_PARAMETER( extFeatures );

    if ( size < sizeof( CONTEXT_X64 ) )
        return E_INVALIDARG;

    ThreadX86Base* threadX86 = GetStoppedThread();
    Thread* thread = threadX86->GetExecThread();
    CONTEXT_X64* context = (CONTEXT_X64*) contextBuf;

    context->ContextFlags = features;

    if ( threadId == thread->GetId() && mIsContextCached )
    {
        return GetThreadContextWithCache( thread->GetHandle(), context, size );
    }

    HRESULT hr = S_OK;
    HANDLE  hThread = OpenThread( THREAD_ALL_ACCESS, FALSE, threadId );

    if ( hThread == NULL )
    {
        hr = GetLastHr();
        goto Error;
    }

    if ( !GetThreadContextX86( hThread, context ) )
    {
        hr = GetLastHr();
        goto Error;
    }

Error:
    if ( hThread != NULL )
        CloseHandle( hThread );

    return hr;
}

HRESULT MachineX64::SetThreadContextInternal( uint32_t threadId, const void* context, uint32_t size )
{
    if ( size < sizeof( CONTEXT_X64 ) )
        return E_INVALIDARG;

    ThreadX86Base* threadX86 = GetStoppedThread();
    Thread* thread = threadX86->GetExecThread();

    if ( threadId == thread->GetId() && mIsContextCached )
    {
        return SetThreadContextWithCache( thread->GetHandle(), context, size );
    }

    HRESULT hr = S_OK;
    HANDLE  hThread = OpenThread( THREAD_ALL_ACCESS, FALSE, threadId );

    if ( hThread == NULL )
    {
        hr = GetLastHr();
        goto Error;
    }

    if ( !SetThreadContextX86( hThread, (const CONTEXT_X64*) context ) )
    {
        hr = GetLastHr();
        goto Error;
    }

Error:
    if ( hThread != NULL )
        CloseHandle( hThread );

    return hr;
}

ThreadControlProc MachineX64::GetWinSuspendThreadProc()
{
    return SuspendThreadX86;
}
