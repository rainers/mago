/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ProgramNode.h"


namespace Mago
{
    // ProgramNode

    ProgramNode::ProgramNode()
    :   mPid( 0 )
    {
    }

    ProgramNode::~ProgramNode()
    {
    }

    void    ProgramNode::SetProcessId( DWORD pid )
    {
        _ASSERT( mPid == 0 );
        mPid = pid;
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugProgramNode2 methods 

    HRESULT ProgramNode::GetProgramName( BSTR* pbstrProgramName )
    { return E_NOTIMPL; } 
    HRESULT ProgramNode::GetHostName( DWORD dwHostNameType, BSTR* pbstrHostName )
    { return E_NOTIMPL; } 

    HRESULT ProgramNode::GetHostMachineName_V7( BSTR* pbstrHostMachineName )
    {
        UNREFERENCED_PARAMETER( pbstrHostMachineName );
        return E_NOTIMPL;
    }

    HRESULT ProgramNode::DetachDebugger_V7()
    {
        return E_NOTIMPL;
    }

    HRESULT ProgramNode::Attach_V7( IDebugProgram2* pMDMProgram,
                                IDebugEventCallback2* pCallback, 
                                DWORD dwReason )
    {
        UNREFERENCED_PARAMETER( pMDMProgram );
        UNREFERENCED_PARAMETER( pCallback );
        UNREFERENCED_PARAMETER( dwReason );
        return E_NOTIMPL;
    }

    HRESULT ProgramNode::GetEngineInfo( BSTR* pbstrEngine, GUID* pguidEngine )
    {
        if ( (pbstrEngine == NULL) || (pguidEngine == NULL) )
            return E_INVALIDARG;

        *pguidEngine = ::GetEngineId();
        *pbstrEngine = SysAllocString( ::GetEngineName() );

        return *pbstrEngine != NULL ? S_OK : E_OUTOFMEMORY;
    } 

    HRESULT ProgramNode::GetHostPid( AD_PROCESS_ID* pHostProcessId )
    {
        if ( pHostProcessId == NULL )
            return E_INVALIDARG;

        pHostProcessId->ProcessIdType = AD_PROCESS_ID_SYSTEM;
        pHostProcessId->ProcessId.dwProcessId = mPid;
        return S_OK;
    } 
}
