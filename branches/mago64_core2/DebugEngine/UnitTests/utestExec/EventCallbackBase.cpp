/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "stdafx.h"
#include "EventCallbackBase.h"

using namespace boost;


const char* gEventNames[] = 
{
    "None",
    "ProcessStart",
    "ProcessExit",
    "ThreadStart",
    "ThreadExit",
    "ModuleLoad",
    "ModuleUnload",
    "OutputString",
    "LoadComplete",
    "Exception",
    "Breakpoint",
    "StepComplete",
    "AsyncBreakComplete",
    "Error",
};


const char* GetEventName( ExecEvent event )
{
    if ( (event < ExecEvent_None) || (event >= ExecEvent_Max) )
        return "";

    return gEventNames[ event ];
}


EventCallbackBase::EventCallbackBase()
    :   mRefCount( 0 ),
        mExec( NULL ),
        mVerbose( false ),
        mTrackEvents( false ),
        mTrackLastEvent( false ),
        mLastThreadId( 0 ),
        mLoadCompleted( false ),
        mProcExited( false ),
        mProcExitCode( 0 ),
        mCanStepInFuncRetVal( false )
{
}

void EventCallbackBase::SetVerbose( bool value )
{
    mVerbose = value;
}

void EventCallbackBase::SetCanStepInFunctionReturnValue( bool value )
{
    mCanStepInFuncRetVal = value;
}

void EventCallbackBase::SetTrackEvents( bool value )
{
    mTrackEvents = value;
}

void EventCallbackBase::SetTrackLastEvent( bool value )
{
    mTrackLastEvent = value;
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

uint32_t    EventCallbackBase::GetLastThreadId()
{
    return mLastThreadId;
}

const EventList& EventCallbackBase::GetEvents()
{
    return mEvents;
}

void EventCallbackBase::ClearEvents()
{
    mEvents.clear();
}

shared_ptr<EventNode> EventCallbackBase::GetLastEvent()
{
    return mLastEvent;
}

bool EventCallbackBase::GetLoadCompleted()
{
    return mLoadCompleted;
}

bool EventCallbackBase::GetProcessExited()
{
    return mProcExited;
}

uint32_t EventCallbackBase::GetProcessExitCode()
{
    return mProcExitCode;
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

void EventCallbackBase::TrackEvent( const shared_ptr<EventNode>& node )
{
    if ( mTrackEvents )
        mEvents.push_back( node );
    if ( mTrackLastEvent )
        mLastEvent = node;
}

void EventCallbackBase::OnProcessStart( IProcess* process )
{
    mLastThreadId = 0;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_ProcessStart;
        TrackEvent( node );
    }
}

void EventCallbackBase::OnProcessExit( IProcess* process, DWORD exitCode )
{
    mProcExited = true;
    mProcExitCode = exitCode;
    mLastThreadId = 0;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<ProcessExitEventNode>   node( new ProcessExitEventNode() );
        node->ExitCode = exitCode;
        TrackEvent( node );
    }
}

void EventCallbackBase::OnThreadStart( IProcess* process, Thread* thread )
{
    mLastThreadId = thread->GetId();
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_ThreadStart;
        node->ThreadId = thread->GetId();
        TrackEvent( node );
    }
}

void EventCallbackBase::OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
{
    mLastThreadId = threadId;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<ThreadExitEventNode>   node( new ThreadExitEventNode() );
        node->ThreadId = threadId;
        node->ExitCode = exitCode;
        TrackEvent( node );
    }
}

void EventCallbackBase::OnModuleLoad( IProcess* process, IModule* module )
{
    if ( mVerbose )
        printf( "  %p %ls\n", (uintptr_t) module->GetImageBase(), module->GetExePath() );

    mLastThreadId = 0;
    mModules.insert( ModuleMap::value_type( module->GetImageBase(), module ) );

    if ( mProcMod.Get() == NULL )
    {
        mProcMod = module;
    }

    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<ModuleLoadEventNode>   node( new ModuleLoadEventNode() );
        node->Module = module;
        TrackEvent( node );
    }
}

void EventCallbackBase::OnModuleUnload( IProcess* process, Address baseAddr )
{
    mLastThreadId = 0;
    if ( mTrackEvents || mTrackLastEvent )
    {
        ModuleMap::iterator it = mModules.find( baseAddr );

        if ( it != mModules.end() )
        {
            shared_ptr< ModuleUnloadEventNode >   node( new ModuleUnloadEventNode() );

            node->Module = it->second;

            TrackEvent( node );

            mModules.erase( it );
        }
    }
}

void EventCallbackBase::OnOutputString( IProcess* process, const wchar_t* outputString )
{
    if ( mVerbose )
        printf( "  '%ls'\n", outputString );

    mLastThreadId = 0;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<OutputStringEventNode>   node( new OutputStringEventNode() );
        node->String = outputString;
        TrackEvent( node );
    }
}

void EventCallbackBase::OnLoadComplete( IProcess* process, DWORD threadId )
{
    mLastThreadId = threadId;
    mLoadCompleted = true;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_LoadComplete;
        node->ThreadId = threadId;
        TrackEvent( node );
    }
}

RunMode EventCallbackBase::OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec )
{
    mLastThreadId = threadId;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<ExceptionEventNode>   node( new ExceptionEventNode() );
        node->ThreadId = threadId;
        node->FirstChance = firstChance;
        node->Exception = *exceptRec;
        TrackEvent( node );
    }
    return RunMode_Break;
}

RunMode EventCallbackBase::OnBreakpoint( IProcess* process, uint32_t threadId, Address address, bool embedded )
{
    mLastThreadId = threadId;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<BreakpointEventNode>   node( new BreakpointEventNode() );
        node->ThreadId = threadId;
        node->Address = address;
        
        TrackEvent( node );
    }
    return RunMode_Break;
}

void EventCallbackBase::OnStepComplete( IProcess* process, uint32_t threadId )
{
    mLastThreadId = threadId;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_StepComplete;
        node->ThreadId = threadId;
        TrackEvent( node );
    }
}

void EventCallbackBase::OnAsyncBreakComplete( IProcess* process, uint32_t threadId )
{
    mLastThreadId = threadId;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_AsyncBreakComplete;
        node->ThreadId = threadId;
        TrackEvent( node );
    }
}

void EventCallbackBase::OnError( IProcess* process, HRESULT hrErr, EventCode event )
{
    mLastThreadId = 0;
    if ( mTrackEvents || mTrackLastEvent )
    {
        shared_ptr<ErrorEventNode>   node( new ErrorEventNode() );
        node->Event = event;
        TrackEvent( node );
    }
}

RunMode EventCallbackBase::OnCallProbe( IProcess* process, uint32_t threadId, Address address )
{
    return mCanStepInFuncRetVal ? RunMode_Break : RunMode_Run;
}

void EventCallbackBase::PrintCallstacksX86( IProcess* process )
{
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
}

void EventCallbackBase::PrintCallstacksX64( IProcess* process )
{
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
}
