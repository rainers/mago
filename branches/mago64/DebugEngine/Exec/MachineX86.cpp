/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "MachineX86.h"
#include "Thread.h"


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

HRESULT MachineX86::ChangeCurrentPC( uint32_t threadId, int32_t byteOffset )
{
    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
#ifdef _WIN64
    WOW64_CONTEXT context = { 0 };
#else
    CONTEXT context = { 0 };
#endif
    HANDLE  hThread = OpenThread( THREAD_ALL_ACCESS, FALSE, threadId );

    if ( hThread == NULL )
    {
        hr = GetLastHr();
        goto Error;
    }

#ifdef _WIN64
    context.ContextFlags = WOW64_CONTEXT_CONTROL;
#else
    context.ContextFlags = CONTEXT_CONTROL;
#endif

#ifdef _WIN64
    bRet = Wow64GetThreadContext( hThread, &context );
#else
    bRet = GetThreadContext( hThread, &context );
#endif
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    context.Eip += byteOffset;

#ifdef _WIN64
    bRet = Wow64SetThreadContext( hThread, &context );
#else
    bRet = SetThreadContext( hThread, &context );
#endif
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

Error:
    if ( hThread != NULL )
        CloseHandle( hThread );

    return hr;
}

HRESULT MachineX86::SetSingleStep( uint32_t threadId, bool enable )
{
    const DWORD TRACE_FLAG = 0x100;

    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
#ifdef _WIN64
    WOW64_CONTEXT context = { 0 };
#else
    CONTEXT context = { 0 };
#endif
    HANDLE  hThread = OpenThread( THREAD_ALL_ACCESS, FALSE, threadId );

    if ( hThread == NULL )
    {
        hr = GetLastHr();
        goto Error;
    }

#ifdef _WIN64
    context.ContextFlags = WOW64_CONTEXT_CONTROL;
#else
    context.ContextFlags = CONTEXT_CONTROL;
#endif

#ifdef _WIN64
    bRet = Wow64GetThreadContext( hThread, &context );
#else
    bRet = GetThreadContext( hThread, &context );
#endif
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    if ( enable )
        context.EFlags |= TRACE_FLAG;
    else
        context.EFlags &= ~TRACE_FLAG;

#ifdef _WIN64
    bRet = Wow64SetThreadContext( hThread, &context );
#else
    bRet = SetThreadContext( hThread, &context );
#endif
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

Error:
    if ( hThread != NULL )
        CloseHandle( hThread );

    return hr;
}

HRESULT MachineX86::GetCurrentPC( uint32_t threadId, MachineAddress& address )
{
    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;
#ifdef _WIN64
    WOW64_CONTEXT context = { 0 };
#else
    CONTEXT context = { 0 };
#endif
    HANDLE  hThread = OpenThread( THREAD_ALL_ACCESS, FALSE, threadId );

    if ( hThread == NULL )
    {
        hr = GetLastHr();
        goto Error;
    }

#ifdef _WIN64
    context.ContextFlags = WOW64_CONTEXT_CONTROL;
#else
    context.ContextFlags = CONTEXT_CONTROL;
#endif

#ifdef _WIN64
    bRet = Wow64GetThreadContext( hThread, &context );
#else
    bRet = GetThreadContext( hThread, &context );
#endif
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

    address = context.Eip;

    // TODO: why are we setting the thread context?
#ifdef _WIN64
    bRet = Wow64SetThreadContext( hThread, &context );
#else
    bRet = SetThreadContext( hThread, &context );
#endif
    if ( !bRet )
    {
        hr = GetLastHr();
        goto Error;
    }

Error:
    if ( hThread != NULL )
        CloseHandle( hThread );

    return hr;
}

HRESULT MachineX86::SuspendThread( Thread* thread )
{
#ifdef _WIN64
    DWORD   suspendCount = ::Wow64SuspendThread( thread->GetHandle() );
#else
    DWORD   suspendCount = ::SuspendThread( thread->GetHandle() );
#endif

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
