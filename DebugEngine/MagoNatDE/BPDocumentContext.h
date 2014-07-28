/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "DocumentContext.h"


namespace Mago
{
    class PendingBreakpoint;


    // Relies on a pending breakpoint for enumerating code contexts.

    class BPDocumentContext : 
        public DocumentContext,
        public CComObjectRootEx<CComMultiThreadModel>
    {
        RefPtr<PendingBreakpoint>   mBPParent;

    public:

    DECLARE_NOT_AGGREGATABLE(BPDocumentContext)

    BEGIN_COM_MAP(BPDocumentContext)
        COM_INTERFACE_ENTRY(IDebugDocumentContext2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugDocumentContext2 

        STDMETHOD( EnumCodeContexts )( IEnumDebugCodeContexts2** ppEnumCodeCxts );

    public:
        HRESULT Init(
            PendingBreakpoint* pendingBP,
            const wchar_t* filename,
            TEXT_POSITION& statementBegin,
            TEXT_POSITION& statementEnd,
            const wchar_t* langName,
            const GUID& langGuid );

        virtual HRESULT Clone( DocumentContext** ppDocContext );

        void Dispose();
    };
}
