/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "MachineX86.h"
#include "Thread.h"
#include "ThreadX86.h"


// Define a set of minimum registers to cache.
// It really only needs to be control registers, but you might as well cache 
// most of the rest, which are often read from the last thread that reported an event.

#if defined( _WIN64 )
enum
{
    MIN_CONTEXT_FLAGS = 
        WOW64_CONTEXT_FULL 
        | WOW64_CONTEXT_FLOATING_POINT 
        | WOW64_CONTEXT_EXTENDED_REGISTERS
};
typedef WOW64_CONTEXT                   CONTEXT_X86;
#define CONTEXT_X86_i386                WOW64_CONTEXT_i386
#define CONTEXT_X86_CONTROL             WOW64_CONTEXT_CONTROL
#define CONTEXT_X86_INTEGER             WOW64_CONTEXT_INTEGER
#define CONTEXT_X86_SEGMENTS            WOW64_CONTEXT_SEGMENTS
#define CONTEXT_X86_FLOATING_POINT      WOW64_CONTEXT_FLOATING_POINT
#define CONTEXT_X86_DEBUG_REGISTERS     WOW64_CONTEXT_DEBUG_REGISTERS
#define CONTEXT_X86_EXTENDED_REGISTERS  WOW64_CONTEXT_EXTENDED_REGISTERS
#define GetThreadContextX86             ::Wow64GetThreadContext
#define SetThreadContextX86             ::Wow64SetThreadContext
#define SuspendThreadX86                ::Wow64SuspendThread
#else
enum
{ 
    MIN_CONTEXT_FLAGS = 
        CONTEXT_FULL 
        | CONTEXT_FLOATING_POINT 
        | CONTEXT_EXTENDED_REGISTERS
};
typedef CONTEXT                         CONTEXT_X86;
#define CONTEXT_X86_i386                CONTEXT_i386
#define CONTEXT_X86_CONTROL             CONTEXT_CONTROL
#define CONTEXT_X86_INTEGER             CONTEXT_INTEGER
#define CONTEXT_X86_SEGMENTS            CONTEXT_SEGMENTS
#define CONTEXT_X86_FLOATING_POINT      CONTEXT_FLOATING_POINT
#define CONTEXT_X86_DEBUG_REGISTERS     CONTEXT_DEBUG_REGISTERS
#define CONTEXT_X86_EXTENDED_REGISTERS  CONTEXT_EXTENDED_REGISTERS
#define GetThreadContextX86             ::GetThreadContext
#define SetThreadContextX86             ::SetThreadContext
#define SuspendThreadX86                ::SuspendThread
#endif

const DWORD TRACE_FLAG = 0x100;


HRESULT MakeMachineX86( IMachine*& machine )
{
    HRESULT hr = S_OK;
    RefPtr<MachineX86>          machX86( new MachineX86() );

    if ( machX86.Get() == NULL )
        return E_OUTOFMEMORY;

    hr = machX86->Init();
    if ( FAILED( hr ) )
        return hr;

    machine = machX86.Detach();
    return S_OK;
}

MachineX86::MachineX86()
    :   mIsContextCached( false )
{
    memset( &mContext, 0, sizeof mContext );
}

HRESULT MachineX86::CacheThreadContext()
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

HRESULT MachineX86::FlushThreadContext()
{
    if ( !mIsContextCached )
        return S_OK;

    HRESULT hr = S_OK;
    ThreadX86Base* threadX86 = GetStoppedThread();
    Thread* thread = threadX86->GetExecThread();

    if ( !SetThreadContextX86( thread->GetHandle(), &mContext ) )
    {
        hr = GetLastHr();
        goto Error;
    }

    mIsContextCached = false;

Error:
    return hr;
}

HRESULT MachineX86::ChangeCurrentPC( int32_t byteOffset )
{
    _ASSERT( mIsContextCached );
    if ( !mIsContextCached )
        return E_FAIL;

    mContext.Eip += byteOffset;
    return S_OK;
}

HRESULT MachineX86::SetSingleStep( bool enable )
{
    _ASSERT( mIsContextCached );
    if ( !mIsContextCached )
        return E_FAIL;

    if ( enable )
        mContext.EFlags |= TRACE_FLAG;
    else
        mContext.EFlags &= ~TRACE_FLAG;

    return S_OK;
}

HRESULT MachineX86::GetCurrentPC( Address& address )
{
    _ASSERT( mIsContextCached );
    if ( !mIsContextCached )
        return E_FAIL;

    address = mContext.Eip;
    return S_OK;
}

HRESULT MachineX86::SuspendThread( Thread* thread )
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

HRESULT MachineX86::ResumeThread( Thread* thread )
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

static void CopyContext( DWORD flags, const CONTEXT_X86* srcContext, CONTEXT_X86* dstContext )
{
    _ASSERT( srcContext != NULL );
    _ASSERT( dstContext != NULL );
    _ASSERT( (flags & ~MIN_CONTEXT_FLAGS) == 0 );

    if ( (flags & CONTEXT_X86_CONTROL) == CONTEXT_X86_CONTROL )
    {
        memcpy( &dstContext->Ebp, &srcContext->Ebp, sizeof( DWORD ) * 6 );
    }

    if ( (flags & CONTEXT_X86_INTEGER) == CONTEXT_X86_INTEGER )
    {
        memcpy( &dstContext->Edi, &srcContext->Edi, sizeof( DWORD ) * 6 );
    }

    if ( (flags & CONTEXT_X86_SEGMENTS) == CONTEXT_X86_SEGMENTS )
    {
        memcpy( &dstContext->SegGs, &srcContext->SegGs, sizeof( DWORD ) * 4 );
    }

    if ( (flags & CONTEXT_X86_FLOATING_POINT) == CONTEXT_X86_FLOATING_POINT )
    {
        dstContext->FloatSave = srcContext->FloatSave;
    }

    if ( (flags & CONTEXT_X86_EXTENDED_REGISTERS) == CONTEXT_X86_EXTENDED_REGISTERS )
    {
        memcpy( 
            dstContext->ExtendedRegisters, 
            srcContext->ExtendedRegisters, 
            sizeof dstContext->ExtendedRegisters );
    }
}

HRESULT MachineX86::GetThreadContextWithCache( HANDLE hThread, void* contextBuf, uint32_t size )
{
    _ASSERT( hThread != NULL );
    _ASSERT( contextBuf != NULL );
    _ASSERT( size >= sizeof( CONTEXT_X86 ) );
    _ASSERT( mIsContextCached );

    // ContextFlags = 0 and CONTEXT_i386 are OK

    CONTEXT_X86* context = (CONTEXT_X86*) contextBuf;
    DWORD callerFlags       = context->ContextFlags & ~CONTEXT_X86_i386;
    DWORD cacheFlags        = mContext.ContextFlags & ~CONTEXT_X86_i386;
    DWORD cachedFlags       = callerFlags & cacheFlags;
    DWORD notCachedFlags    = callerFlags & ~cacheFlags;

    if ( notCachedFlags != 0 )
    {
        // only get from the target what isn't cached
        context->ContextFlags = notCachedFlags | CONTEXT_X86_i386;
        if ( !GetThreadContextX86( hThread, context ) )
            return GetLastHr();
    }

    if ( cachedFlags != 0 )
    {
        CopyContext( cachedFlags | CONTEXT_X86_i386, &mContext, context );
        context->ContextFlags |= cachedFlags | CONTEXT_X86_i386;
    }

    return S_OK;
}

HRESULT MachineX86::SetThreadContextWithCache( HANDLE hThread, const void* contextBuf, uint32_t size )
{
    _ASSERT( hThread != NULL );
    _ASSERT( contextBuf != NULL );
    _ASSERT( size >= sizeof( CONTEXT_X86 ) );
    _ASSERT( mIsContextCached );

    // ContextFlags = 0 and CONTEXT_i386 are OK

    const CONTEXT_X86* context = (const CONTEXT_X86*) contextBuf;
    DWORD callerFlags       = context->ContextFlags & ~CONTEXT_X86_i386;
    DWORD cacheFlags        = mContext.ContextFlags & ~CONTEXT_X86_i386;
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
        CopyContext( cachedFlags | CONTEXT_X86_i386, context, &mContext );
    }

    return S_OK;
}

HRESULT MachineX86::GetThreadContextInternal( uint32_t threadId, void* context, uint32_t size )
{
    if ( size < sizeof( CONTEXT_X86 ) )
        return E_INVALIDARG;

    ThreadX86Base* threadX86 = GetStoppedThread();
    Thread* thread = threadX86->GetExecThread();

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

    if ( !GetThreadContextX86( hThread, (CONTEXT_X86*) context ) )
    {
        hr = GetLastHr();
        goto Error;
    }

Error:
    if ( hThread != NULL )
        CloseHandle( hThread );

    return hr;
}

HRESULT MachineX86::SetThreadContextInternal( uint32_t threadId, const void* context, uint32_t size )
{
    if ( size < sizeof( CONTEXT_X86 ) )
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

    if ( !SetThreadContextX86( hThread, (const CONTEXT_X86*) context ) )
    {
        hr = GetLastHr();
        goto Error;
    }

Error:
    if ( hThread != NULL )
        CloseHandle( hThread );

    return hr;
}

ThreadControlProc MachineX86::GetWinSuspendThreadProc()
{
    return SuspendThreadX86;
}
