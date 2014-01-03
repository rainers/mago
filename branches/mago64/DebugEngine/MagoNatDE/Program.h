/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class IDebuggerProxy;
    class Thread;
    class Module;
    class Engine;
    class DRuntime;
    class ICoreProcess;
    class ICoreThread;
    class ICoreModule;

    typedef uint64_t    BPCookie;


    class ATL_NO_VTABLE Program : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugProgram2
    {
        typedef std::map< Address, RefPtr<Module> >     ModuleMap;
        typedef std::map< DWORD, RefPtr<Thread> >       ThreadMap;
        typedef std::vector< BPCookie >                 CookieVec;
        typedef std::map< Address, CookieVec >          BPMap;

        GUID                            mProgId;
        CComPtr<IDebugProcess2>         mProcess;
        CComPtr<IDebugEventCallback2>   mCallback;
        CComBSTR                        mName;
        RefPtr<ICoreProcess>            mCoreProc;
        bool                            mAttached;
        bool                            mPassExceptionToDebuggee;
        bool                            mCanPassExceptionToDebuggee;
        IDebuggerProxy*                 mDebugger;
        ThreadMap                       mThreadMap;
        ModuleMap                       mModMap;
        BPMap                           mBPMap;
        RefPtr<Engine>                  mEngine;
        Guard                           mThreadGuard;
        Guard                           mModGuard;
        Guard                           mBPGuard;
        DWORD                           mNextModLoadIndex;  // protected by mod guard
        Address                         mEntryPoint;
        RefPtr<Module>                  mProgMod;
        std::unique_ptr<DRuntime>       mDRuntime;

    public:
        Program();
        ~Program();

    DECLARE_NOT_AGGREGATABLE(Program)

    BEGIN_COM_MAP(Program)
        COM_INTERFACE_ENTRY(IDebugProgram2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugProgram2 

        STDMETHOD( EnumThreads )( IEnumDebugThreads2** ppEnum );
        STDMETHOD( GetName )( BSTR* pbstrName );
        STDMETHOD( GetProcess )( IDebugProcess2** ppProcess );
        STDMETHOD( Terminate )();
        STDMETHOD( Attach )( IDebugEventCallback2* pCallback );
        STDMETHOD( Detach )();
        STDMETHOD( GetProgramId )( GUID* pguidProgramId );
        STDMETHOD( GetDebugProperty )( IDebugProperty2** ppProperty );
        STDMETHOD( Execute )();
        STDMETHOD( Continue )( IDebugThread2* pThread );
        STDMETHOD( Step )( IDebugThread2* pThread, STEPKIND sk, STEPUNIT step );
        STDMETHOD( CauseBreak )();
        STDMETHOD( GetEngineInfo )( BSTR* pbstrEngine, GUID* pguidEngine ); 
        STDMETHOD( EnumCodeContexts )( 
            IDebugDocumentPosition2* pDocPos, 
            IEnumDebugCodeContexts2** ppEnum );
        STDMETHOD( GetMemoryBytes )( IDebugMemoryBytes2** ppMemoryBytes );
        STDMETHOD( GetDisassemblyStream )( 
            DISASSEMBLY_STREAM_SCOPE dwScope, 
            IDebugCodeContext2* pCodeContext, 
            IDebugDisassemblyStream2** ppDisassemblyStream );
        STDMETHOD( EnumModules )( IEnumDebugModules2** ppEnum );
        STDMETHOD( GetENCUpdate )( IDebugENCUpdate** ppUpdate );
        STDMETHOD( EnumCodePaths )( 
            LPCOLESTR pszHint, 
            IDebugCodeContext2* pStart, 
            IDebugStackFrame2* pFrame, 
            BOOL fSource, 
            IEnumCodePaths2** ppEnum, 
            IDebugCodeContext2** ppSafety );
        STDMETHOD( WriteDump )( DUMPTYPE DumpType, LPCOLESTR pszCrashDumpUrl );
        STDMETHOD( CanDetach )();

    public:
        void        Dispose();

        void        SetEngine( Engine* engine );
        ICoreProcess*   GetCoreProcess();
        void        GetCoreProcess( ICoreProcess*& proc );
        void        SetCoreProcess( ICoreProcess* proc );
        void        SetProcess( IDebugProcess2* proc );
        IDebugEventCallback2*   
                    GetCallback();
        void        SetCallback( IDebugEventCallback2* callback );
        void        SetPortSettings( IDebugProgram2* portProgram );
        void        SetDebuggerProxy( IDebuggerProxy* debugger );
        DRuntime*   GetDRuntime();
        void        SetDRuntime( std::unique_ptr<DRuntime>& druntime );

        bool        GetAttached();
        void        SetAttached();
        void        SetPassExceptionToDebuggee( bool value );
        bool        CanPassExceptionToDebuggee();
        void        NotifyException( bool firstChance, const EXCEPTION_RECORD* exceptRec );

        HRESULT     CreateThread( ICoreThread* coreThread, RefPtr<Thread>& thread );
        HRESULT     AddThread( Thread* thread );
        bool        FindThread( DWORD threadId, RefPtr<Thread>& thread );
        void        DeleteThread( Thread* thread );

        HRESULT     CreateModule( ICoreModule* coreMod, RefPtr<Module>& mod );
        HRESULT     AddModule( Module* mod );
        bool        FindModule( Address address, RefPtr<Module>& mod );
        bool        FindModuleContainingAddress( Address address, RefPtr<Module>& mod );
        void        DeleteModule( Module* mod );

        void        ForeachModule( ModuleCallback* callback );

        HRESULT     SetInternalBreakpoint( Address address, BPCookie cookie );
        HRESULT     RemoveInternalBreakpoint( Address address, BPCookie cookie );
        HRESULT     EnumBPCookies( Address, std::vector< BPCookie >& iter );
        Address     GetEntryPoint();
        void        SetEntryPoint( Address address );

    private:
        HRESULT     StepInternal( IDebugThread2* pThread, STEPKIND sk, STEPUNIT step );

        struct AddressBinding
        {
            uint64_t        Addr;
            RefPtr<Module>  Mod;
        };

        bool        BindCodeContextsToFile( 
            bool exactMatch, 
            const char* fileName, 
            size_t fileNameLen, 
            uint16_t reqStartLine, 
            uint16_t reqEndLine,
            std::list<AddressBinding>& bindings );
    };
}
