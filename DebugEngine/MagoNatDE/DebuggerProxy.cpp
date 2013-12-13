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
        int procFeatures = 0;

        if ( IsProcessorFeaturePresent( PF_MMX_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_MMX;

        if ( IsProcessorFeaturePresent( PF_3DNOW_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_3DNow;

        if ( IsProcessorFeaturePresent( PF_XMMI_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_SSE;

        if ( IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_SSE2;

        if ( IsProcessorFeaturePresent( PF_SSE3_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_SSE3;

        if ( IsProcessorFeaturePresent( PF_XSAVE_ENABLED ) )
            procFeatures |= PF_X86_AVX;

        mArch = new ArchDataX86( (ProcFeaturesX86) procFeatures );
        if ( mArch.Get() == NULL )
            return E_OUTOFMEMORY;

        return S_OK;
    }

    HRESULT DebuggerProxy::GetSystemInfo( IProcess* process, ArchData*& sysInfo )
    {
        if ( process == NULL )
            return E_INVALIDARG;
        if ( mArch.Get() == NULL )
            return E_NOT_FOUND;

        sysInfo = mArch.Get();
        sysInfo->AddRef();

        return S_OK;
    }

//----------------------------------------------------------------------------
// Commands
//----------------------------------------------------------------------------

    HRESULT DebuggerProxy::Launch( LaunchInfo* launchInfo, IProcess*& process )
    {
        return mExecThread.Launch( launchInfo, process );
    }

    HRESULT DebuggerProxy::Attach( uint32_t id, IProcess*& process )
    {
        return mExecThread.Attach( id, process );
    }

    HRESULT DebuggerProxy::Terminate( IProcess* process )
    {
        return mExecThread.Terminate( process );
    }

    HRESULT DebuggerProxy::Detach( IProcess* process )
    {
        return mExecThread.Detach( process );
    }

    HRESULT DebuggerProxy::ResumeLaunchedProcess( IProcess* process )
    {
        return mExecThread.ResumeLaunchedProcess( process );
    }

    HRESULT DebuggerProxy::ReadMemory( 
        IProcess* process, 
        Address address,
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer )
    {
        return mExecThread.ReadMemory( process, address, length, lengthRead, lengthUnreadable, buffer );
    }

    HRESULT DebuggerProxy::WriteMemory( 
        IProcess* process, 
        Address address,
        SIZE_T length, 
        SIZE_T& lengthWritten, 
        uint8_t* buffer )
    {
        return mExecThread.WriteMemory( process, address, length, lengthWritten, buffer );
    }

    HRESULT DebuggerProxy::SetBreakpoint( IProcess* process, Address address )
    {
        return mExecThread.SetBreakpoint( process, address );
    }

    HRESULT DebuggerProxy::RemoveBreakpoint( IProcess* process, Address address )
    {
        return mExecThread.RemoveBreakpoint( process, address );
    }

    HRESULT DebuggerProxy::StepOut( IProcess* process, Address targetAddr, bool handleException )
    {
        return mExecThread.StepOut( process, targetAddr, handleException );
    }

    HRESULT DebuggerProxy::StepInstruction( IProcess* process, bool stepIn, bool handleException )
    {
        return mExecThread.StepInstruction( process, stepIn, handleException );
    }

    HRESULT DebuggerProxy::StepRange( 
        IProcess* process, bool stepIn, AddressRange range, bool handleException )
    {
        return mExecThread.StepRange( process, stepIn, range, handleException );
    }

    HRESULT DebuggerProxy::Continue( IProcess* process, bool handleException )
    {
        return mExecThread.Continue( process, handleException );
    }

    HRESULT DebuggerProxy::Execute( IProcess* process, bool handleException )
    {
        return mExecThread.Execute( process, handleException );
    }

    HRESULT DebuggerProxy::AsyncBreak( IProcess* process )
    {
        return mExecThread.AsyncBreak( process );
    }

    HRESULT DebuggerProxy::GetThreadContext( IProcess* process, ::Thread* thread, IRegisterSet*& regSet )
    {
        _ASSERT( process != NULL );
        _ASSERT( thread != NULL );
        if ( process == NULL || thread == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        CONTEXT context = { 0 };

        context.ContextFlags = CONTEXT_FULL 
            | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

        hr = mExecThread.GetThreadContext( process, thread, &context, sizeof context );
        if ( FAILED( hr ) )
            return hr;

        hr = mArch->BuildRegisterSet( &context, sizeof context, regSet );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT DebuggerProxy::SetThreadContext( IProcess* process, ::Thread* thread, IRegisterSet* regSet )
    {
        _ASSERT( process != NULL );
        _ASSERT( thread != NULL );
        _ASSERT( regSet != NULL );
        if ( process == NULL || thread == NULL || regSet == NULL )
            return E_INVALIDARG;

        HRESULT         hr = S_OK;
        const void*     contextBuf = NULL;
        uint32_t        contextSize = 0;

        if ( !regSet->GetThreadContext( contextBuf, contextSize ) )
            return E_FAIL;

        hr = mExecThread.SetThreadContext( process, thread, contextBuf, contextSize );
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

    void DebuggerProxy::OnThreadStart( IProcess* process, ::Thread* coreThread )
    {
        mCallback->OnThreadStart( process->GetId(), coreThread );
    }

    void DebuggerProxy::OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
    {
        mCallback->OnThreadExit( process->GetId(), threadId, exitCode );
    }

    void DebuggerProxy::OnModuleLoad( IProcess* process, IModule* module )
    {
        mCallback->OnModuleLoad( process->GetId(), module );
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
