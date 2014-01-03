/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "RemoteDebuggerProxy.h"
#include "ArchData.h"
#include "EventCallback.h"
#include "RegisterSet.h"
#include "RemoteProcess.h"


namespace Mago
{
    //------------------------------------------------------------------------
    // RemoteDebuggerProxy
    //------------------------------------------------------------------------

    RemoteDebuggerProxy::RemoteDebuggerProxy()
    {
    }

    RemoteDebuggerProxy::~RemoteDebuggerProxy()
    {
    }

    HRESULT RemoteDebuggerProxy::Init( EventCallback* callback )
    {
        _ASSERT( callback != NULL );
        if ( (callback == NULL) )
            return E_INVALIDARG;

        mCallback = callback;

        return S_OK;
    }


    //----------------------------------------------------------------------------
    // IDebuggerProxy
    //----------------------------------------------------------------------------

    HRESULT RemoteDebuggerProxy::Launch( LaunchInfo* launchInfo, ICoreProcess*& process )
    {
        _ASSERT( launchInfo != NULL );
        if ( launchInfo == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Attach( uint32_t pid, ICoreProcess*& process )
    {
        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Terminate( ICoreProcess* process )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Detach( ICoreProcess* process )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::ResumeLaunchedProcess( ICoreProcess* process )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::ReadMemory( 
        ICoreProcess* process, 
        Address address,
        uint32_t length, 
        uint32_t& lengthRead, 
        uint32_t& lengthUnreadable, 
        uint8_t* buffer )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::WriteMemory( 
        ICoreProcess* process, 
        Address address,
        uint32_t length, 
        uint32_t& lengthWritten, 
        uint8_t* buffer )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::SetBreakpoint( ICoreProcess* process, Address address )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::RemoveBreakpoint( ICoreProcess* process, Address address )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::StepOut( ICoreProcess* process, Address targetAddr, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::StepInstruction( ICoreProcess* process, bool stepIn, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::StepRange( 
        ICoreProcess* process, bool stepIn, AddressRange range, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Continue( ICoreProcess* process, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::Execute( ICoreProcess* process, bool handleException )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::AsyncBreak( ICoreProcess* process )
    {
        _ASSERT( process != NULL );
        if ( process == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT hr = S_OK;

        // TODO: send the request to the remote agent

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::GetThreadContext( 
        ICoreProcess* process, ICoreThread* thread, IRegisterSet*& regSet )
    {
        _ASSERT( process != NULL );
        _ASSERT( thread != NULL );
        if ( process == NULL || thread == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote
            || thread->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        // TODO: get flags and context size from archData

        HRESULT hr = S_OK;
        CONTEXT context = { 0 };
        DWORD contextFlags = CONTEXT_FULL 
            | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;

        // TODO: send the request to the remote agent

        ArchData* archData = process->GetArchData();

        hr = archData->BuildRegisterSet( &context, sizeof context, regSet );
        if ( FAILED( hr ) )
            return hr;

        return E_NOTIMPL;
    }

    HRESULT RemoteDebuggerProxy::SetThreadContext( 
        ICoreProcess* process, ICoreThread* thread, IRegisterSet* regSet )
    {
        _ASSERT( process != NULL );
        _ASSERT( thread != NULL );
        _ASSERT( regSet != NULL );
        if ( process == NULL || thread == NULL || regSet == NULL )
            return E_INVALIDARG;

        if ( process->GetProcessType() != CoreProcess_Remote
            || thread->GetProcessType() != CoreProcess_Remote )
            return E_INVALIDARG;

        HRESULT         hr = S_OK;
        const void*     contextBuf = NULL;
        uint32_t        contextSize = 0;

        if ( !regSet->GetThreadContext( contextBuf, contextSize ) )
            return E_FAIL;

        // TODO: send it to the remote agent

        return E_NOTIMPL;
    }
}
