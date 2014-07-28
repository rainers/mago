/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class ProgramNode : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugProgramNode2
    {
        DWORD   mPid;

    public:
        ProgramNode();
        ~ProgramNode();

        void    SetProcessId( DWORD pid );

    DECLARE_NOT_AGGREGATABLE(ProgramNode)

    BEGIN_COM_MAP(ProgramNode)
        COM_INTERFACE_ENTRY(IDebugProgramNode2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugProgramNode2 

        STDMETHOD( GetProgramName )( BSTR* pbstrProgramName ); 
        STDMETHOD( GetHostName )( DWORD dwHostNameType, BSTR* pbstrHostName ); 
        STDMETHOD( GetHostPid )( AD_PROCESS_ID* pHostProcessId ); 
        STDMETHOD( GetHostMachineName_V7 )( BSTR* pbstrHostMachineName ); 
        STDMETHOD( Attach_V7 )( 
            IDebugProgram2* pMDMProgram, 
            IDebugEventCallback2* pCallback, 
            DWORD dwReason ); 
        STDMETHOD( GetEngineInfo )( BSTR* pbstrEngine, GUID* pguidEngine ); 
        STDMETHOD( DetachDebugger_V7 )(); 
    };
}
