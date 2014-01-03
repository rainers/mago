/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class PendingBreakpoint;
    class Program;


    class ATL_NO_VTABLE BoundBreakpoint : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugBoundBreakpoint2
    {
        DWORD                                   mId;
        BP_STATE                                mState;
        RefPtr<PendingBreakpoint>               mPendingBP;
        CComPtr<IDebugBreakpointResolution2>    mBPRes;
        Address                                 mAddr;
        RefPtr<Program>                         mProg;
        Guard                                   mStateGuard;

    public:
        BoundBreakpoint();
        ~BoundBreakpoint();

    DECLARE_NOT_AGGREGATABLE(BoundBreakpoint)

    BEGIN_COM_MAP(BoundBreakpoint)
        COM_INTERFACE_ENTRY(IDebugBoundBreakpoint2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugBoundBreakpoint2 

        STDMETHOD( GetPendingBreakpoint )( IDebugPendingBreakpoint2** ppPendingBreakpoint ); 
        STDMETHOD( GetState )( BP_STATE* pState ); 
        STDMETHOD( GetHitCount )( DWORD* pdwHitCount ); 
        STDMETHOD( GetBreakpointResolution )( IDebugBreakpointResolution2** ppBPResolution ); 
        STDMETHOD( Enable )( BOOL fEnable ); 
        STDMETHOD( SetHitCount )( DWORD dwHitCount ); 
        STDMETHOD( SetCondition )( BP_CONDITION bpCondition ); 
        STDMETHOD( SetPassCount )( BP_PASSCOUNT bpPassCount ); 
        STDMETHOD( Delete )(); 

    public:
        void    Init( 
            DWORD id,
            Address addr,
            PendingBreakpoint* pendingBreakpoint, 
            IDebugBreakpointResolution2* resolution,
            Program* prog );
        DWORD   GetId();
        void    Dispose();
    };
}
