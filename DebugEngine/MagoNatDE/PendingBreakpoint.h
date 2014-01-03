/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class Engine;
    class BPDocumentContext;
    class BoundBreakpoint;
    class ErrorBreakpoint;


    struct ModuleBinding
    {
        typedef std::vector< RefPtr<BoundBreakpoint> >  BPList;

        RefPtr<ErrorBreakpoint>                 ErrorBP;
        BPList                                  BoundBPs;
    };


    class ATL_NO_VTABLE PendingBreakpoint : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugPendingBreakpoint2
    {
        typedef std::map< DWORD, ModuleBinding >    BindingMap; // key is module ID

        DWORD                                   mId;
        PENDING_BP_STATE_INFO                   mState;
        bool                                    mDeleted;
        bool                                    mSentEvent;
        CComPtr<IDebugBreakpointRequest2>       mBPRequest;
        CComPtr<IDebugEventCallback2>           mCallback;
        RefPtr<Engine>                          mEngine;
        RefPtr<BPDocumentContext>               mDocContext;    // optional
        BindingMap                              mBindings;
        DWORD                                   mLastBPId;
        Guard                                   mBoundBPGuard;

    public:
        PendingBreakpoint();
        ~PendingBreakpoint();

    DECLARE_NOT_AGGREGATABLE(PendingBreakpoint)

    BEGIN_COM_MAP(PendingBreakpoint)
        COM_INTERFACE_ENTRY(IDebugPendingBreakpoint2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugPendingBreakpoint2 

        STDMETHOD( CanBind )( IEnumDebugErrorBreakpoints2** ppErrorEnum );
        STDMETHOD( Bind )();
        STDMETHOD( GetState )( PENDING_BP_STATE_INFO* pState );
        STDMETHOD( GetBreakpointRequest )( IDebugBreakpointRequest2** ppBPRequest );
        STDMETHOD( Virtualize )( BOOL fVirtualize );
        STDMETHOD( Enable )( BOOL fEnable );
        STDMETHOD( SetCondition )( BP_CONDITION bpCondition );
        STDMETHOD( SetPassCount )( BP_PASSCOUNT bpPassCount );
        STDMETHOD( EnumBoundBreakpoints )( IEnumDebugBoundBreakpoints2** ppEnum );
        STDMETHOD( EnumErrorBreakpoints )( 
            BP_ERROR_TYPE bpErrorType, 
            IEnumDebugErrorBreakpoints2** ppEnum );
        STDMETHOD( Delete )();

    public:
        void    Init( 
            DWORD id,
            Engine* engine,
            IDebugBreakpointRequest2* pBPRequest,
            IDebugEventCallback2* pCallback );
        void    Dispose();
        DWORD   GetId();
        void    OnBoundBPDelete( BoundBreakpoint* boundBP );
        ModuleBinding*  GetBinding( DWORD modId );
        ModuleBinding*  AddOrFindBinding( DWORD modId );

        HRESULT EnumCodeContexts( IEnumDebugCodeContexts2** ppEnum );
        DWORD   GetNextBPId();

        HRESULT BindToModule( Module* mod, Program* prog );
        HRESULT UnbindFromModule( Module* mod, Program* prog );
        HRESULT EnumBoundBreakpoints( ModuleBinding* binding, IEnumDebugBoundBreakpoints2** ppEnum );

    private:
        HRESULT SendBoundEvent( IEnumDebugBoundBreakpoints2* enumBPs );
        HRESULT SendErrorEvent( ErrorBreakpoint* errorBP );
        HRESULT SendUnboundEvent( BoundBreakpoint* boundBP, Program* prog );

        HRESULT BindToAllModules();
    };
}
