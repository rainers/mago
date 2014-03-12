/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "InstCache.h"
#include "DocTracker.h"


namespace Mago
{
    class Program;
    class IDebuggerProxy;


    class ATL_NO_VTABLE DisassemblyStream : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugDisassemblyStream2
    {
        DISASSEMBLY_STREAM_SCOPE    mScope;

        Address64           mAnchorAddr;
        Address64           mReadAddr;
        uint32_t            mInvalidInstLenAtReadPtr;

        InstCache           mInstCache;
        DocTracker          mDocInfo;

        // When we start reading a block of instructions in a Read call, we 
        // want to force returning the document for the first instruction in 
        // the block. Normally, we would only return the document for an 
        // instruction if the document changed between that instruction and the
        // one before.

        bool                mStartOfRead;

        RefPtr<Program>     mProg;
        int                 mPtrSize;

    public:
        DisassemblyStream();
        ~DisassemblyStream();

    DECLARE_NOT_AGGREGATABLE(DisassemblyStream)

    BEGIN_COM_MAP(DisassemblyStream)
        COM_INTERFACE_ENTRY(IDebugDisassemblyStream2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugDisassemblyStream2 

        STDMETHOD( Read )( 
            DWORD                     dwInstructions,
            DISASSEMBLY_STREAM_FIELDS dwFields,
            DWORD*                    pdwInstructionsRead,
            DisassemblyData*          prgDisassembly );

        STDMETHOD( Seek )( 
            SEEK_START          dwSeekStart,
            IDebugCodeContext2* pCodeContext,
            UINT64              uCodeLocationId,
            INT64               iInstructions );

        STDMETHOD( GetCodeLocationId )( 
            IDebugCodeContext2* pCodeContext,
            UINT64*             puCodeLocationId );

        STDMETHOD( GetCodeContext )( 
            UINT64               uCodeLocationId,
            IDebugCodeContext2** ppCodeContext );

        STDMETHOD( GetCurrentLocation )( 
            UINT64* puCodeLocationId );

        STDMETHOD( GetDocument )( 
            BSTR              bstrDocumentUrl,
            IDebugDocument2** ppDocument );

        STDMETHOD( GetScope )( 
            DISASSEMBLY_STREAM_SCOPE* pdwScope );

        STDMETHOD( GetSize )( 
            UINT64* pnSize );

    public:
        HRESULT Init( 
            DISASSEMBLY_STREAM_SCOPE disasmScope, 
            Address64 address, 
            Program* program, 
            IDebuggerProxy* debugger );

    private:
        HRESULT SeekOffset( INT64 iInstructions );
        HRESULT SeekBack( int iInstructions, InstBlock* block );
        HRESULT SeekForward( int iInstructions, InstBlock* block );

        void FillDataByteDisasmData( 
            BYTE byte, 
            DISASSEMBLY_STREAM_FIELDS dwFields, 
            DisassemblyData* pDisassembly );

        void FillInstDisasmData( 
            const ud_t* ud, 
            DISASSEMBLY_STREAM_FIELDS dwFields, 
            DisassemblyData* pDisassembly );

        bool GetDocContext( 
            Address64 address, 
            Module* mod, 
            IDebugDocumentContext2** docContext );
    };
}
