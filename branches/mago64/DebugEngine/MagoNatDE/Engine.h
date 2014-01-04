/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.

   Purpose: main object that Visual Studio talks with to carry out debugging
*/

#pragma once

#include "resource.h"       // main symbols
#include "MagoNatDE_i.h"
#include "DebuggerProxy.h"
#include "RemoteDebuggerProxy.h"
#include "ExceptionTable.h"


namespace Mago
{
    class Program;
    class PendingBreakpoint;

    // Engine

    class ATL_NO_VTABLE Engine :
        public CComObjectRootEx<CComMultiThreadModel>,
        public CComCoClass<Engine, &CLSID_MagoNativeEngine>,
        public IDebugEngine2,
        public IDebugEngineLaunch2
    {
        typedef std::map< DWORD, RefPtr<Program> >  ProgramMap;
        typedef std::map< DWORD, RefPtr<PendingBreakpoint> >    BPMap;

        DebuggerProxy       mDebugger;
        RemoteDebuggerProxy mRemoteDebugger;
        bool                mPollThreadStarted;
        bool                mSentEngineCreate;
        ProgramMap          mProgs;
        BPMap               mBPs;
        DWORD               mLastBPId;
        DWORD               mLastModId;
        CComPtr<IDebugEventCallback2>   mCallback;
        Guard               mBindBPGuard;
        Guard               mPendingBPGuard;
        Guard               mExceptionGuard;
        EngineExceptionTable    mExceptionInfos;

    public:
        Engine();

    DECLARE_REGISTRY_RESOURCEID(IDR_ENGINE)

    DECLARE_NOT_AGGREGATABLE(Engine)

    BEGIN_COM_MAP(Engine)
        COM_INTERFACE_ENTRY(IDebugEngine2)
        COM_INTERFACE_ENTRY(IDebugEngineLaunch2)
    END_COM_MAP()

        DECLARE_PROTECT_FINAL_CONSTRUCT()

        HRESULT FinalConstruct();
        void FinalRelease();

        //////////////////////////////////////////////////////////// 
        // IDebugEngine2 

        STDMETHOD( EnumPrograms )( IEnumDebugPrograms2** ppEnum ); 
        STDMETHOD( Attach )( 
            IDebugProgram2** rgpPrograms, 
            IDebugProgramNode2** rgpProgramNodes, 
            DWORD celtPrograms, 
            IDebugEventCallback2* pCallback, 
            ATTACH_REASON dwReason ); 
        STDMETHOD( CreatePendingBreakpoint )( 
            IDebugBreakpointRequest2* pBPRequest, 
            IDebugPendingBreakpoint2** ppPendingBP ); 
        STDMETHOD( SetException )( EXCEPTION_INFO* pException ); 
        STDMETHOD( RemoveSetException )( EXCEPTION_INFO* pException ); 
        STDMETHOD( RemoveAllSetExceptions )( REFGUID guidType ); 
        STDMETHOD( GetEngineId )( GUID* pguidEngine ); 
        STDMETHOD( DestroyProgram )( IDebugProgram2* pProgram ); 
        STDMETHOD( ContinueFromSynchronousEvent )( IDebugEvent2* pEvent ); 
        STDMETHOD( SetLocale )( WORD wLangID ); 
        STDMETHOD( SetRegistryRoot )( LPCOLESTR pszRegistryRoot ); 
        STDMETHOD( SetMetric )( LPCOLESTR pszMetric, VARIANT varValue ); 
        STDMETHOD( CauseBreak )(); 

        //////////////////////////////////////////////////////////// 
        // IDebugEngineLaunch2 

        STDMETHOD( LaunchSuspended )( 
           LPCOLESTR             pszMachine,
           IDebugPort2*          pPort,
           LPCOLESTR             pszExe,
           LPCOLESTR             pszArgs,
           LPCOLESTR             pszDir,
           BSTR                  bstrEnv,
           LPCOLESTR             pszOptions,
           LAUNCH_FLAGS          dwLaunchFlags,
           DWORD                 hStdInput,
           DWORD                 hStdOutput,
           DWORD                 hStdError,
           IDebugEventCallback2* pCallback,
           IDebugProcess2**      ppDebugProcess
        );
        STDMETHOD( ResumeProcess )( 
           IDebugProcess2* pProcess
        );
        STDMETHOD( CanTerminateProcess )( 
           IDebugProcess2* pProcess
        );
        STDMETHOD( TerminateProcess )( 
           IDebugProcess2* pProcess
        );

    public:
        bool FindProgram( DWORD id, RefPtr<Program>& prog );
        void DeleteProgram( Program* prog );
        HRESULT AddPendingBP( PendingBreakpoint* pendingBP );
        void OnPendingBPDelete( PendingBreakpoint* pendingBP );
        DWORD GetNextModuleId();
        void ForeachProgram( ProgramCallback* callback );

        // Returns true if exception info was found.
        // If found, then returns info in output param. A copy is returned instead 
        // of a pointer to the entry, because another thread can change the list 
        // before the user gets around to using the info.
        bool FindExceptionInfo( const GUID& guid, DWORD code, ExceptionInfo& excInfo );
        bool FindExceptionInfo( const GUID& guid, LPCOLESTR name, ExceptionInfo& excInfo );

        HRESULT BindPendingBPsToModule( Module* mod, Program* prog );
        HRESULT UnbindPendingBPsFromModule( Module* mod, Program* prog );

        void BeginBindBP();
        void EndBindBP();

    private:
        HRESULT EnsurePollThreadRunning();
        void ShutdownIfNeeded();

        HRESULT LaunchSuspendedInternal( 
           IDebugPort2*          pPort,
           LaunchInfo&           launchParams,
           IDebugEventCallback2* pCallback,
           IDebugProcess2**      ppDebugProcess
        );
        HRESULT ResumeProcessInternal( IDebugProcess2* pProcess );

        DWORD GetNextBPId();
    };

    OBJECT_ENTRY_AUTO(__uuidof(MagoNativeEngine), Engine)
}
