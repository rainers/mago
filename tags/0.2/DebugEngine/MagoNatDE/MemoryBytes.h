/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class DebuggerProxy;


    class ATL_NO_VTABLE MemoryBytes : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugMemoryBytes2
    {
        Address             mAddr;
        uint64_t            mSize;
        DebuggerProxy*      mDebugger;
        RefPtr<IProcess>    mProc;

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
        void Init( Address addr, uint64_t size, DebuggerProxy* debugger, IProcess* proc );
    };
}
