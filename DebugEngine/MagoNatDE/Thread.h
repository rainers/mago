/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class Program;
    class IDebuggerProxy;
    class StackFrame;
    class IRegisterSet;
    class ICoreProcess;
    class ICoreThread;


    class ATL_NO_VTABLE Thread : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugThread2
    {
        typedef std::vector< RefPtr<StackFrame> > Callstack;

        RefPtr<ICoreThread> mCoreThread;
        RefPtr<Program>     mProg;
        Address64           mCurPC;
        Address64           mCallerPC;
        IDebuggerProxy*     mDebugger;

    public:
        Thread();
        ~Thread();

    DECLARE_NOT_AGGREGATABLE(Thread)

    BEGIN_COM_MAP(Thread)
        COM_INTERFACE_ENTRY(IDebugThread2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugThread2 

        STDMETHOD( EnumFrameInfo )( 
            FRAMEINFO_FLAGS dwFieldSpec, 
            UINT nRadix, 
            IEnumDebugFrameInfo2** ppEnum );
        STDMETHOD( GetName )( BSTR* pbstrName );
        STDMETHOD( SetThreadName )( LPCOLESTR pszName );
        STDMETHOD( GetProgram )( IDebugProgram2** ppProgram );
        STDMETHOD( CanSetNextStatement )( 
            IDebugStackFrame2* pStackFrame, 
            IDebugCodeContext2* pCodeContext );
        STDMETHOD( SetNextStatement )( 
            IDebugStackFrame2* pStackFrame, 
            IDebugCodeContext2* pCodeContext );
        STDMETHOD( GetThreadId )( DWORD* pdwThreadId );
        STDMETHOD( Suspend )( DWORD* pdwSuspendCount );
        STDMETHOD( Resume )( DWORD* pdwSuspendCount );
        STDMETHOD( GetThreadProperties )( 
            THREADPROPERTY_FIELDS dwFields, 
            THREADPROPERTIES* ptp );
        STDMETHOD( GetLogicalThread )( 
            IDebugStackFrame2* pStackFrame, 
            IDebugLogicalThread2** ppLogicalThread );

    public:
        ICoreThread*    GetCoreThread();
        void            SetCoreThread( ICoreThread* thread );
        Program*        GetProgram();
        void            SetProgram( Program* prog, IDebuggerProxy* pollThread );
        ICoreProcess*   GetCoreProcess();
        IDebuggerProxy* GetDebuggerProxy();

        HRESULT Step( ICoreProcess* coreProc, STEPKIND sk, STEPUNIT step, bool handleException );

    private:
        HRESULT BuildCallstack( IRegisterSet* topRegSet, Callstack& callstack );
        HRESULT AddCallstackFrame( IRegisterSet* regSet, Callstack& callstack );
        HRESULT MakeEnumFrameInfoFromCallstack( 
            const Callstack& callstack,
            FRAMEINFO_FLAGS dwFieldSpec, 
            UINT nRadix, 
            IEnumDebugFrameInfo2** ppEnum );

        HRESULT StepStatement( ICoreProcess* coreProc, STEPKIND sk, bool handleException );
        HRESULT StepInstruction( ICoreProcess* coreProc, STEPKIND sk, bool handleException );
        HRESULT StepOut( ICoreProcess* coreProc, bool handleException );

        static BOOL CALLBACK ReadProcessMemory64(
          HANDLE hProcess,
          DWORD64 lpBaseAddress,
          PVOID lpBuffer,
          DWORD nSize,
          LPDWORD lpNumberOfBytesRead
        );
        static PVOID CALLBACK FunctionTableAccess64(
          HANDLE hProcess,
          DWORD64 AddrBase
        );
        static DWORD64 CALLBACK GetModuleBase64(
          HANDLE hProcess,
          DWORD64 Address
        );
    };
}
