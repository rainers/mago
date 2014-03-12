/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class IDebuggerProxy;
    class ICoreProcess;


    class ATL_NO_VTABLE MemoryBytes : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugMemoryBytes2
    {
        Address64               mAddr;
        uint64_t                mSize;
        IDebuggerProxy*         mDebugger;
        RefPtr<ICoreProcess>    mProc;

    public:
        MemoryBytes();
        ~MemoryBytes();

    DECLARE_NOT_AGGREGATABLE(MemoryBytes)

    BEGIN_COM_MAP(MemoryBytes)
        COM_INTERFACE_ENTRY(IDebugMemoryBytes2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugMemoryBytes2 

        STDMETHOD( ReadAt )( 
            IDebugMemoryContext2* pStartContext,
            DWORD                 dwCount,
            BYTE*                 rgbMemory,
            DWORD*                pdwRead,
            DWORD*                pdwUnreadable );
        STDMETHOD( WriteAt )( 
            IDebugMemoryContext2* pStartContext,
            DWORD                 dwCount,
            BYTE*                 rgbMemory );
        STDMETHOD( GetSize )( 
            UINT64* pqwSize );

    public:
        void Init( Address64 addr, uint64_t size, IDebuggerProxy* debugger, ICoreProcess* proc );
    };
}
