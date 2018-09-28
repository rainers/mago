/*
   Copyright (c) 2018 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "stdafx.h"
#include "MultiEventCallbackBase.h"


MultiEventCallbackBase::MultiEventCallbackBase()
    :   mRefCount( 0 ),
        mExec( NULL ),
        mVerbose( false ),
        mTrackEvents( false ),
        mTrackLastEvent( false ),
        mCanStepInFuncRetVal( false )
{
}

void MultiEventCallbackBase::SetVerbose( bool value )
{
    mVerbose = value;
}

void MultiEventCallbackBase::SetCanStepInFunctionReturnValue( bool value )
{
    mCanStepInFuncRetVal = value;
}

void MultiEventCallbackBase::SetTrackEvents( bool value )
{
    mTrackEvents = value;
}

void MultiEventCallbackBase::SetTrackLastEvent( bool value )
{
    mTrackLastEvent = value;
}

void MultiEventCallbackBase::SetExec( Exec* exec )
{
    mExec = exec;
}

Exec* MultiEventCallbackBase::GetExec()
{
    return mExec;
}

RefPtr<IModule> MultiEventCallbackBase::GetProcessModule( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return NULL;

    return it->second.mProcMod;
}

uint32_t    MultiEventCallbackBase::GetLastThreadId( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return NULL;

    return it->second.mLastThreadId;
}

const EventList& MultiEventCallbackBase::GetEvents()
{
    return mEvents;
}

void MultiEventCallbackBase::ClearEvents()
{
    mEvents.clear();
}

IProcess* MultiEventCallbackBase::GetProcess( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return NULL;

    return it->second.mProcess.Get();
}

bool MultiEventCallbackBase::GetAllProcessesExited()
{
    for ( auto& proc : mProcesses )
    {
        if ( !proc.second.mProcExited )
            return false;
    }

    return true;
}

std::shared_ptr<EventNode> MultiEventCallbackBase::GetLastEvent()
{
    return mLastEvent;
}

const EventList* MultiEventCallbackBase::GetEvents( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return NULL;

    return &it->second.mEvents;
}

void MultiEventCallbackBase::ClearEvents( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return;

    it->second.mEvents.clear();
}

std::shared_ptr<EventNode> MultiEventCallbackBase::GetLastEvent( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return NULL;

    return it->second.mLastEvent;
}

bool MultiEventCallbackBase::GetLoadCompleted( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return false;

    return it->second.mLoadCompleted;
}

bool MultiEventCallbackBase::GetProcessExited( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return false;

    return it->second.mProcExited;
}

uint32_t MultiEventCallbackBase::GetProcessExitCode( DWORD pid )
{
    auto it = mProcesses.find( pid );
    if ( it == mProcesses.end() )
        return 0;

    return it->second.mProcExitCode;
}

void MultiEventCallbackBase::AddRef()
{
    mRefCount++;
}

void MultiEventCallbackBase::Release()
{
    mRefCount--;
    if ( mRefCount == 0 )
    {
        delete this;
    }
}

void MultiEventCallbackBase::TrackEvent( const std::shared_ptr<EventNode>& node )
{
    if ( mTrackEvents )
        mEvents.push_back( node );
    if ( mTrackLastEvent )
        mLastEvent = node;

    auto it = mProcesses.find( node->ProcessId );
    if ( it == mProcesses.end() )
        return;

    if ( mTrackEvents )
        it->second.mEvents.push_back( node );
    if ( mTrackLastEvent )
        it->second.mLastEvent = node;
}

void MultiEventCallbackBase::OnProcessStart( IProcess* process )
{
    Process proc;

    proc.mLastThreadId = 0;
    proc.mLoadCompleted = false;
    proc.mProcExitCode = 0;
    proc.mProcExited = false;
    proc.mProcess = process;

    mProcesses.insert( ProcessMap::value_type( process->GetId(), proc ) );

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_ProcessStart;
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

void MultiEventCallbackBase::OnProcessExit( IProcess* process, DWORD exitCode )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mProcExited = true;
    it->second.mProcExitCode = exitCode;
    it->second.mLastThreadId = 0;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<ProcessExitEventNode>   node( new ProcessExitEventNode() );
        node->ExitCode = exitCode;
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

void MultiEventCallbackBase::OnThreadStart( IProcess* process, Thread* thread )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mLastThreadId = thread->GetId();

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_ThreadStart;
        node->ThreadId = thread->GetId();
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

void MultiEventCallbackBase::OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mLastThreadId = threadId;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<ThreadExitEventNode>   node( new ThreadExitEventNode() );
        node->ProcessId = process->GetId();
        node->ThreadId = threadId;
        node->ExitCode = exitCode;
        TrackEvent( node );
    }
}

void MultiEventCallbackBase::OnModuleLoad( IProcess* process, IModule* module )
{
    if ( mVerbose )
        printf( "  %p %ls\n", (uintptr_t) module->GetImageBase(), module->GetPath() );

    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mLastThreadId = 0;
    it->second.mModules.insert( ModuleMap::value_type( module->GetImageBase(), module ) );

    if ( it->second.mProcMod.Get() == NULL )
    {
        it->second.mProcMod = module;
    }

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<ModuleLoadEventNode>   node( new ModuleLoadEventNode() );
        node->Module = module;
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

void MultiEventCallbackBase::OnModuleUnload( IProcess* process, Address baseAddr )
{
    auto procIt = mProcesses.find( process->GetId() );
    if ( procIt == mProcesses.end() )
        return;

    procIt->second.mLastThreadId = 0;

    ModuleMap& mModules = procIt->second.mModules;

    if ( mTrackEvents || mTrackLastEvent )
    {
        ModuleMap::iterator it = mModules.find( baseAddr );

        if ( it != mModules.end() )
        {
            std::shared_ptr< ModuleUnloadEventNode >   node( new ModuleUnloadEventNode() );

            node->Module = it->second;
            node->ProcessId = process->GetId();

            TrackEvent( node );

            mModules.erase( it );
        }
    }
}

void MultiEventCallbackBase::OnOutputString( IProcess* process, const wchar_t* outputString )
{
    if ( mVerbose )
        printf( "  '%ls'\n", outputString );

    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mLastThreadId = 0;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<OutputStringEventNode>   node( new OutputStringEventNode() );
        node->String = outputString;
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

void MultiEventCallbackBase::OnLoadComplete( IProcess* process, DWORD threadId )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mLastThreadId = threadId;
    it->second.mLoadCompleted = true;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_LoadComplete;
        node->ThreadId = threadId;
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

RunMode MultiEventCallbackBase::OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return RunMode_Break;

    it->second.mLastThreadId = threadId;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<ExceptionEventNode>   node( new ExceptionEventNode() );
        node->ProcessId = process->GetId();
        node->ThreadId = threadId;
        node->FirstChance = firstChance;
        node->Exception = *exceptRec;
        TrackEvent( node );
    }
    return RunMode_Break;
}

RunMode MultiEventCallbackBase::OnBreakpoint( IProcess* process, uint32_t threadId, Address address, bool embedded )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return RunMode_Break;

    it->second.mLastThreadId = threadId;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<BreakpointEventNode>   node( new BreakpointEventNode() );
        node->ProcessId = process->GetId();
        node->ThreadId = threadId;
        node->Address = address;
        
        TrackEvent( node );
    }
    return RunMode_Break;
}

void MultiEventCallbackBase::OnStepComplete( IProcess* process, uint32_t threadId )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mLastThreadId = threadId;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_StepComplete;
        node->ThreadId = threadId;
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

void MultiEventCallbackBase::OnAsyncBreakComplete( IProcess* process, uint32_t threadId )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mLastThreadId = threadId;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<EventNode>   node( new EventNode() );
        node->Code = ExecEvent_AsyncBreakComplete;
        node->ThreadId = threadId;
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

void MultiEventCallbackBase::OnError( IProcess* process, HRESULT hrErr, EventCode event )
{
    auto it = mProcesses.find( process->GetId() );
    if ( it == mProcesses.end() )
        return;

    it->second.mLastThreadId = 0;

    if ( mTrackEvents || mTrackLastEvent )
    {
        std::shared_ptr<ErrorEventNode>   node( new ErrorEventNode() );
        node->Event = event;
        node->ProcessId = process->GetId();
        TrackEvent( node );
    }
}

ProbeRunMode MultiEventCallbackBase::OnCallProbe( 
    IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange )
{
    return mCanStepInFuncRetVal ? ProbeRunMode_Break : ProbeRunMode_Run;
}

void MultiEventCallbackBase::PrintCallstacksX86( IProcess* process )
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

void MultiEventCallbackBase::PrintCallstacksX64( IProcess* process )
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
