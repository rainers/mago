/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EventCallbackBase.h"

using namespace std;
using namespace boost;


// TODO: this stubbed out base class should be provided the Exec library itself

EventCallbackBase::EventCallbackBase()
    :   mRefCount( 0 ),
        mExec( NULL ),
        mLoadCompleted( false ),
        mProcExited( false )
{
}

void EventCallbackBase::SetExec( Exec* exec )
{
    mExec = exec;
}

Exec* EventCallbackBase::GetExec()
{
    return mExec;
}

RefPtr<IModule> EventCallbackBase::GetProcessModule()
{
    return mProcMod;
}

bool EventCallbackBase::GetLoadCompleted()
{
    return mLoadCompleted;
}

bool EventCallbackBase::GetProcessExited()
{
    return mProcExited;
}

void EventCallbackBase::AddRef()
{
    mRefCount++;
}

void EventCallbackBase::Release()
{
    mRefCount--;
    if ( mRefCount == 0 )
    {
        delete this;
    }
}

void EventCallbackBase::OnProcessStart( IProcess* process )
{
}

void EventCallbackBase::OnProcessExit( IProcess* process, DWORD exitCode )
{
    mProcExited = true;
}

void EventCallbackBase::OnThreadStart( IProcess* process, Thread* thread )
{
}

void EventCallbackBase::OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
{
}

void EventCallbackBase::OnModuleLoad( IProcess* process, IModule* module )
{
    if ( mProcMod.Get() == NULL )
    {
        mProcMod = module;
    }
}

void EventCallbackBase::OnModuleUnload( IProcess* process, Address baseAddr )
{
}

void EventCallbackBase::OnOutputString( IProcess* process, const wchar_t* outputString )
{
}

void EventCallbackBase::OnLoadComplete( IProcess* process, DWORD threadId )
{
    mLoadCompleted = true;
}

bool EventCallbackBase::OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec )
{
    return false;
}

bool EventCallbackBase::OnBreakpoint( IProcess* process, uint32_t threadId, Address address, Enumerator<BPCookie>* iter )
{
    return false;
}

void EventCallbackBase::OnStepComplete( IProcess* process, uint32_t threadId )
{
}

void EventCallbackBase::OnAsyncBreakComplete( IProcess* process, uint32_t threadId )
{
}

void EventCallbackBase::OnError( IProcess* process, HRESULT hrErr, EventCode event )
{
}

bool EventCallbackBase::CanStepInFunction( IProcess* process, Address address )
{
    return false;
}

void EventCallbackBase::PrintCallstacksX86( IProcess* process )
{
#if 0
    Enumerator<Thread*>*    threads = NULL;

    process->EnumThreads( threads );

    while ( threads->MoveNext() )
    {
        Thread* t = threads->GetCurrent();
        std::list<FrameX86>  stack;

        ReadCallstackX86( process->GetHandle(), t->GetHandle(), stack );

        printf( "  TID=%d\n", t->GetId() );

        for ( std::list<FrameX86>::iterator it = stack.begin();
            it != stack.end();
            it++ )
        {
            printf( "    EIP=%08x, EBP=%08x\n", it->Eip, it->Ebp );
        }
    }

    threads->Release();
#endif
}

void EventCallbackBase::PrintCallstacksX64( IProcess* process )
{
#if 0
    Enumerator<Thread*>*    threads = NULL;

    process->EnumThreads( threads );

    while ( threads->MoveNext() )
    {
        Thread* t = threads->GetCurrent();
        std::list<FrameX64>  stack;

        ReadCallstackX64( process->GetHandle(), t->GetHandle(), stack );

        printf( "  TID=%d\n", t->GetId() );

        for ( std::list<FrameX64>::iterator it = stack.begin();
            it != stack.end();
            it++ )
        {
            printf( "    RIP=%016x, RBP=%016x\n", it->Rip, it->Rbp );
        }
    }

    threads->Release();
#endif
}
