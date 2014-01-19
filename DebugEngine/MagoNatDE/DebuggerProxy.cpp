/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DebuggerProxy.h"
#include "ArchDataX86.h"
#include "RegisterSet.h"
#include "..\Exec\DebuggerProxy.h"
#include "EventCallback.h"
#include "LocalProcess.h"


namespace Mago
{
    DebuggerProxy::DebuggerProxy()
    {
    }

    DebuggerProxy::~DebuggerProxy()
    {
        Shutdown();
    }

    HRESULT DebuggerProxy::Init( EventCallback* callback )
    {
        _ASSERT( callback != NULL );
        if ( (callback == NULL) )
            return E_INVALIDARG;

        HRESULT     hr = S_OK;

        mCallback = callback;

        hr = CacheSystemInfo();
        if ( FAILED( hr ) )
            return hr;

        hr = mExecThread.Init( this );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT DebuggerProxy::Start()
    {
        return mExecThread.Start();
    }

    void DebuggerProxy::Shutdown()
    {
        mExecThread.Shutdown();
    }

    HRESULT DebuggerProxy::CacheSystemInfo()
    {
        mArch = new ArchDataX86();
        if ( mArch.Get() == NULL )
            return E_OUTOFMEMORY;

        return S_OK;
    }

//----------------------------------------------------------------------------
// Commands
//----------------------------------------------------------------------------

    HRESULT DebuggerProxy::Launch( LaunchInfo* launchInfo, ICoreProcess*& process )
    {
        HRESULT                 hr = S_OK;
        RefPtr<IProcess>        execProc;
        RefPtr<LocalProcess>    coreProc;

        coreProc = new LocalProcess( mArch );
        if ( coreProc.Get() == NULL )
            return E_OUTOFMEMORY;

        hr = mExecThread.Launch( launchInfo, execProc.Ref() );
        if ( FAILED( hr ) )
            return hr;

        coreProc->Init( execProc );
        process = coreProc.Detach();

        return S_OK;
    }

    HRESULT DebuggerProxy::Attach( uint32_t id, ICoreProcess*& process )
    {
        HRESULT                 hr = S_OK;
        RefPtr<IProcess>        execProc;
        RefPtr<LocalProcess>    coreProc;

        coreProc = new LocalProcess( mArch );
        if ( coreProc.Get() == NULL )
            return E_OUTOFMEMORY;

        hr = mExecThread.Attach( id, execProc.Ref() );
        if ( FAILED( hr ) )
            return hr;

        coreProc->Init( execProc );
        process = coreProc.Detach();

        return S_OK;
    }

    HRESULT DebuggerProxy::Terminate( ICoreProcess* process )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.Terminate( execProc );
    }

    HRESULT DebuggerProxy::Detach( ICoreProcess* process )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.Detach( execProc );
    }

    HRESULT DebuggerProxy::ResumeLaunchedProcess( ICoreProcess* process )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.ResumeLaunchedProcess( execProc );
    }

    HRESULT DebuggerProxy::ReadMemory( 
        ICoreProcess* process, 
        Address address,
        uint32_t length, 
        uint32_t& lengthRead, 
        uint32_t& lengthUnreadable, 
        uint8_t* buffer )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.ReadMemory( execProc, address, length, lengthRead, lengthUnreadable, buffer );
    }

    HRESULT DebuggerProxy::WriteMemory( 
        ICoreProcess* process, 
        Address address,
        uint32_t length, 
        uint32_t& lengthWritten, 
        uint8_t* buffer )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.WriteMemory( execProc, address, length, lengthWritten, buffer );
    }

    HRESULT DebuggerProxy::SetBreakpoint( ICoreProcess* process, Address address )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.SetBreakpoint( execProc, address );
    }

    HRESULT DebuggerProxy::RemoveBreakpoint( ICoreProcess* process, Address address )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.RemoveBreakpoint( execProc, address );
    }

    HRESULT DebuggerProxy::StepOut( ICoreProcess* process, Address targetAddr, bool handleException )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.StepOut( execProc, targetAddr, handleException );
    }

    HRESULT DebuggerProxy::StepInstruction( ICoreProcess* process, bool stepIn, bool handleException )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.StepInstruction( execProc, stepIn, handleException );
    }

    HRESULT DebuggerProxy::StepRange( 
        ICoreProcess* process, bool stepIn, AddressRange range, bool handleException )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.StepRange( execProc, stepIn, range, handleException );
    }

    HRESULT DebuggerProxy::Continue( ICoreProcess* process, bool handleException )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.Continue( execProc, handleException );
    }

    HRESULT DebuggerProxy::Execute( ICoreProcess* process, bool handleException )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.Execute( execProc, handleException );
    }

    HRESULT DebuggerProxy::AsyncBreak( ICoreProcess* process )
    {
        if ( process->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();

        return mExecThread.AsyncBreak( execProc );
    }

    HRESULT DebuggerProxy::GetThreadContext( 
        ICoreProcess* process, ICoreThread* thread, IRegisterSet*& regSet )
    {
        _ASSERT( process != NULL );
        _ASSERT( thread != NULL );
        if ( process == NULL || thread == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Local
            || thread->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();
        ::Thread* execThread = ((LocalThread*) thread)->GetExecThread();

        HRESULT hr = S_OK;
        CONTEXT context = { 0 };

        context.ContextFlags = CONTEXT_FULL 
            | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

        hr = mExecThread.GetThreadContext( execProc, execThread->GetId(), &context, sizeof context );
        if ( FAILED( hr ) )
            return hr;

        hr = mArch->BuildRegisterSet( &context, sizeof context, regSet );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT DebuggerProxy::SetThreadContext( 
        ICoreProcess* process, ICoreThread* thread, IRegisterSet* regSet )
    {
        _ASSERT( process != NULL );
        _ASSERT( thread != NULL );
        _ASSERT( regSet != NULL );
        if ( process == NULL || thread == NULL || regSet == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Local
            || thread->GetProcessType() != CoreProcess_Local )
            return E_FAIL;

        IProcess* execProc = ((LocalProcess*) process)->GetExecProcess();
        ::Thread* execThread = ((LocalThread*) thread)->GetExecThread();

        HRESULT         hr = S_OK;
        const void*     contextBuf = NULL;
        uint32_t        contextSize = 0;

        if ( !regSet->GetThreadContext( contextBuf, contextSize ) )
            return E_FAIL;

        hr = mExecThread.SetThreadContext( execProc, execThread->GetId(), contextBuf, contextSize );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }


    //------------------------------------------------------------------------
    // IEventCallback
    //------------------------------------------------------------------------

    void DebuggerProxy::AddRef()
    {
        // There's nothing to do, because this will be allocated as part of the engine.
    }

    void DebuggerProxy::Release()
    {
        // There's nothing to do, because this will be allocated as part of the engine.
    }

    void DebuggerProxy::OnProcessStart( IProcess* process )
    {
        mCallback->OnProcessStart( process->GetId() );
    }

    void DebuggerProxy::OnProcessExit( IProcess* process, DWORD exitCode )
    {
        mCallback->OnProcessExit( process->GetId(), exitCode );
    }

    void DebuggerProxy::OnThreadStart( IProcess* process, ::Thread* thread )
    {
        RefPtr<LocalThread> coreThread;

        coreThread = new LocalThread( thread );

        mCallback->OnThreadStart( process->GetId(), coreThread );
    }

    void DebuggerProxy::OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
    {
        mCallback->OnThreadExit( process->GetId(), threadId, exitCode );
    }

    void DebuggerProxy::OnModuleLoad( IProcess* process, IModule* module )
    {
        RefPtr<LocalModule> coreModule;

        coreModule = new LocalModule( module );

        mCallback->OnModuleLoad( process->GetId(), coreModule );
    }

    void DebuggerProxy::OnModuleUnload( IProcess* process, Address baseAddr )
    {
        mCallback->OnModuleUnload( process->GetId(), baseAddr );
    }

    void DebuggerProxy::OnOutputString( IProcess* process, const wchar_t* outputString )
    {
        mCallback->OnOutputString( process->GetId(), outputString );
    }

    void DebuggerProxy::OnLoadComplete( IProcess* process, DWORD threadId )
    {
        mCallback->OnLoadComplete( process->GetId(), threadId );
    }

    RunMode DebuggerProxy::OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec )
    {
        return mCallback->OnException( process->GetId(), threadId, firstChance, exceptRec );
    }

    RunMode DebuggerProxy::OnBreakpoint( IProcess* process, uint32_t threadId, Address address, bool embedded )
    {
        return mCallback->OnBreakpoint( process->GetId(), threadId, address, embedded );
    }

    void DebuggerProxy::OnStepComplete( IProcess* process, uint32_t threadId )
    {
        mCallback->OnStepComplete( process->GetId(), threadId );
    }

    void DebuggerProxy::OnAsyncBreakComplete( IProcess* process, uint32_t threadId )
    {
        mCallback->OnAsyncBreakComplete( process->GetId(), threadId );
    }

    void DebuggerProxy::OnError( IProcess* process, HRESULT hrErr, EventCode event )
    {
        mCallback->OnError( process->GetId(), hrErr, event );
    }

    ProbeRunMode DebuggerProxy::OnCallProbe( 
        IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange )
    {
        return mCallback->OnCallProbe( process->GetId(), threadId, address, thunkRange );
    }
}
