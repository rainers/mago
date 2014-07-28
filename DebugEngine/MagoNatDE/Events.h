/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class ATL_NO_VTABLE EventBase
    {
    public:
        virtual HRESULT Send( 
            IDebugEventCallback2* callback, 
            IDebugEngine2* engine, 
            IDebugProgram2* program, 
            IDebugThread2* thread ) = 0;
    };

    template <class T, enum_EVENTATTRIBUTES TAttr = EVENT_ASYNCHRONOUS>
    class EventImpl : 
        public EventBase, 
        public IDebugEvent2,
        public T,
        public CComObjectRootEx< CComMultiThreadModel >
    {
    public:
        typedef EventImpl<T, TAttr> _EventImpl;

        BEGIN_COM_MAP(_EventImpl)
            COM_INTERFACE_ENTRY(IDebugEvent2)
            COM_INTERFACE_ENTRY(T)
        END_COM_MAP()

        STDMETHOD( GetAttributes )( DWORD* pdwAttrib )
        {
            if ( pdwAttrib == NULL )
                return E_INVALIDARG;

            *pdwAttrib = TAttr;
            return S_OK;
        }

        virtual HRESULT Send( 
            IDebugEventCallback2* callback, 
            IDebugEngine2* engine, 
            IDebugProgram2* program, 
            IDebugThread2* thread )
        {
            return callback->Event( engine, NULL, program, thread, (IDebugEvent2*) this, __uuidof( T ), TAttr );
        }
    };

    class EngineCreateEvent : public EventImpl<IDebugEngineCreateEvent2>
    {
        CComPtr<IDebugEngine2>  mEngine;

    public:
        void Init( IDebugEngine2* engine );

        STDMETHOD( GetEngine )( IDebugEngine2** pEngine );
    };

    class ProgramCreateEvent : public EventImpl<IDebugProgramCreateEvent2>
    {
    public:
    };

    class ProgramDestroyEvent : public EventImpl<IDebugProgramDestroyEvent2, EVENT_SYNCHRONOUS>
    {
        DWORD   mExitCode;

    public:
        ProgramDestroyEvent();
        void Init( DWORD exitCode );

        STDMETHOD( GetExitCode )( DWORD* pdwExit );
    };

    class LoadCompleteEvent : public EventImpl<IDebugLoadCompleteEvent2, EVENT_ASYNC_STOP>
    {
    public:
    };

    class EntryPointEvent : public EventImpl<IDebugEntryPointEvent2, EVENT_ASYNC_STOP>
    {
    public:
    };

    class ThreadCreateEvent : public EventImpl<IDebugThreadCreateEvent2>
    {
    public:
    };

    class ThreadDestroyEvent : public EventImpl<IDebugThreadDestroyEvent2>
    {
        DWORD   mExitCode;

    public:
        ThreadDestroyEvent();
        void Init( DWORD exitCode );

        STDMETHOD( GetExitCode )( DWORD* pdwExit );
    };

    class StepCompleteEvent : public EventImpl<IDebugStepCompleteEvent2, EVENT_ASYNC_STOP>
    {
    public:
    };

    class BreakEvent : public EventImpl<IDebugBreakEvent2, EVENT_ASYNC_STOP>
    {
    public:
    };

    class OutputStringEvent : public EventImpl<IDebugOutputStringEvent2>
    {
        CComBSTR    mStr;

    public:
        void Init( const wchar_t* str );

        STDMETHOD( GetString )( BSTR* pbstrString );
    };

    class ModuleLoadEvent : public EventImpl<IDebugModuleLoadEvent2>
    {
        CComPtr<IDebugModule2>  mMod;
        CComBSTR                mMsg;
        bool                    mLoad;

    public:
        ModuleLoadEvent();
        void Init(
           IDebugModule2*   module,
           const wchar_t*   debugMessage,
           bool             load );

        STDMETHOD( GetModule )( 
           IDebugModule2**  pModule,
           BSTR*            pbstrDebugMessage,
           BOOL*            pbLoad );
    };

    class SymbolSearchEvent : public EventImpl<IDebugSymbolSearchEvent2>
    {
        CComPtr<IDebugModule3>  mMod;
        CComBSTR                mMsg;
        MODULE_INFO_FLAGS       mInfoFlags;

    public:
        SymbolSearchEvent();
        void Init( IDebugModule3* mod, const wchar_t* msg, MODULE_INFO_FLAGS infoFlags );

        STDMETHOD( GetSymbolSearchInfo )(
           IDebugModule3**    pModule,
           BSTR*              pbstrDebugMessage,
           MODULE_INFO_FLAGS* pdwModuleInfoFlags );
    };

    class BreakpointEvent : public EventImpl<IDebugBreakpointEvent2, EVENT_ASYNC_STOP>
    {
        CComPtr<IEnumDebugBoundBreakpoints2>    mEnumBP;

    public:
        void Init( IEnumDebugBoundBreakpoints2* pEnum );

        STDMETHOD( EnumBreakpoints )( IEnumDebugBoundBreakpoints2** ppEnumBP );
    };

    class BreakpointBoundEvent : public EventImpl<IDebugBreakpointBoundEvent2>
    {
        CComPtr<IEnumDebugBoundBreakpoints2>    mEnumBoundBP;
        CComPtr<IDebugPendingBreakpoint2>       mPendingBP;

    public:
        void Init( 
            IEnumDebugBoundBreakpoints2* pEnum, 
            IDebugPendingBreakpoint2* pPending );

        STDMETHOD( GetPendingBreakpoint )( IDebugPendingBreakpoint2** ppPendingBP );
        STDMETHOD( EnumBoundBreakpoints )( IEnumDebugBoundBreakpoints2** ppEnum );
    };

    class BreakpointErrorEvent : public EventImpl<IDebugBreakpointErrorEvent2>
    {
        CComPtr<IDebugErrorBreakpoint2>         mErrorBP;

    public:
        void Init( IDebugErrorBreakpoint2* pError );

        STDMETHOD( GetErrorBreakpoint )( IDebugErrorBreakpoint2** ppErrorBP );
    };

    class BreakpointUnboundEvent : public EventImpl<IDebugBreakpointUnboundEvent2>
    {
        CComPtr<IDebugBoundBreakpoint2>         mBoundBP;
        BP_UNBOUND_REASON                       mReason;

    public:
        BreakpointUnboundEvent();
        void Init( IDebugBoundBreakpoint2* pBound, BP_UNBOUND_REASON reason );

        STDMETHOD( GetBreakpoint )( IDebugBoundBreakpoint2** ppBP );
        STDMETHOD( GetReason )( BP_UNBOUND_REASON* pdwUnboundReason );
    };

    class ExceptionEvent : public EventImpl<IDebugExceptionEvent2, EVENT_ASYNC_STOP>
    {
    public:
        enum SearchKey
        {
            Name,
            Code
        };

    private:
        RefPtr<Program>         mProg;
        CComBSTR                mExceptionName;
        CComBSTR                mExceptionInfo;
        DWORD                   mCode;
        EXCEPTION_STATE         mState;
        GUID                    mGuidType;
        const wchar_t*          mRootExceptionName;
        SearchKey               mSearchKey;
        bool                    mCanPassToDebuggee;

    public:
        ExceptionEvent();
        void Init( 
            Program* prog, 
            bool firstChance, 
            const EXCEPTION_RECORD64* exceptRec,
            bool canPassToDebuggee );

        STDMETHOD( GetException )( EXCEPTION_INFO* pExceptionInfo );
        STDMETHOD( GetExceptionDescription )( BSTR* pbstrDescription );
        STDMETHOD( CanPassToDebuggee )();
        STDMETHOD( PassToDebuggee )( BOOL fPass );

        // information that's useful for exception handling and ignoring

        EXCEPTION_STATE GetState() const { return mState; }
        DWORD GetCode() const { return mCode; }
        const GUID& GetGUID() const { return mGuidType; }
        LPCOLESTR GetExceptionName() const { return mExceptionName; }

        // If exception info can't be found based on the exception name or code, 
        // the user needs to look up the info for the root exception.
        // For example, the root exception of "Access Violation" is "Win32 Exceptions".

        LPCOLESTR GetRootExceptionName() const { return mRootExceptionName; }
        SearchKey GetSearchKey() const { return mSearchKey; }

        // If we have incomplete info based on calling Init, we can update it 
        // with this.

        void SetExceptionName( LPCOLESTR name );
    };

    class EmbeddedBreakpointEvent : public EventImpl<IDebugExceptionEvent2, EVENT_ASYNC_STOP>
    {
    private:
        RefPtr<Program>         mProg;
        CComBSTR                mExceptionName;

    public:
        EmbeddedBreakpointEvent();
        void Init( Program* prog );

        STDMETHOD( GetException )( EXCEPTION_INFO* pExceptionInfo );
        STDMETHOD( GetExceptionDescription )( BSTR* pbstrDescription );
        STDMETHOD( CanPassToDebuggee )();
        STDMETHOD( PassToDebuggee )( BOOL fPass );
    };

    class MessageTextEvent : public EventImpl<IDebugMessageEvent2>
    {
        MESSAGETYPE     mMessageType;
        CComBSTR        mMessage;

    public:
        MessageTextEvent();
        void Init( MESSAGETYPE reason, const wchar_t* msg );

        STDMETHOD( GetMessage )(
            MESSAGETYPE*    pMessageType,
            BSTR*           pbstrMessage,
            DWORD*          pdwType,
            BSTR*           pbstrHelpFileName,
            DWORD*          pdwHelpId );

        STDMETHOD( SetResponse )( DWORD dwResponse );
    };
}
