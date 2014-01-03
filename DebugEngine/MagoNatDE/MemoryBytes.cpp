/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "MemoryBytes.h"
#include "CodeContext.h"
#include "IDebuggerProxy.h"
#include "ICoreProcess.h"


namespace Mago
{
    MemoryBytes::MemoryBytes()
        :   mAddr( 0 ),
            mSize( 0 ),
            mDebugger( NULL )
    {
    }

    MemoryBytes::~MemoryBytes()
    {
    }


    //////////////////////////////////////////////////////////// 
    // IDebugMemoryBytes2 

    HRESULT MemoryBytes::ReadAt(
            IDebugMemoryContext2* pStartContext,
            DWORD                 dwCount,
            BYTE*                 rgbMemory,
            DWORD*                pdwRead,
            DWORD*                pdwUnreadable )
    {
        if ( pStartContext == NULL )
            return E_INVALIDARG;
        if ( (rgbMemory == NULL) || (pdwRead == NULL) )
            return E_INVALIDARG;

        HRESULT     hr = S_OK;
        Address     addr = 0;
        uint32_t    lenRead = 0;
        uint32_t    lenUnreadable = 0;
        CComQIPtr<IMagoMemoryContext>   memCxt = pStartContext;

        if ( memCxt == NULL )
            return E_INVALIDARG;

        memCxt->GetAddress( addr );

        hr = mDebugger->ReadMemory( 
            mProc,
            addr,
            dwCount,
            lenRead,
            lenUnreadable,
            rgbMemory );
        if ( FAILED( hr ) )
            return hr;

        *pdwRead = lenRead;

        if ( pdwUnreadable != NULL )
            *pdwUnreadable = lenUnreadable;

        return S_OK;
    }

    HRESULT MemoryBytes::WriteAt(
            IDebugMemoryContext2* pStartContext,
            DWORD                 dwCount,
            BYTE*                 rgbMemory )
    {
        if ( pStartContext == NULL )
            return E_INVALIDARG;
        if ( rgbMemory == NULL )
            return E_INVALIDARG;

        HRESULT     hr = S_OK;
        Address     addr = 0;
        uint32_t    lenWritten = 0;
        CComQIPtr<IMagoMemoryContext>   memCxt = pStartContext;

        if ( memCxt == NULL )
            return E_INVALIDARG;

        memCxt->GetAddress( addr );

        hr = mDebugger->WriteMemory( 
            mProc,
            addr,
            dwCount,
            lenWritten,
            rgbMemory );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT MemoryBytes::GetSize( UINT64* pqwSize )
    {
        if ( pqwSize == NULL )
            return E_INVALIDARG;

        *pqwSize = mSize;

        return S_OK;
    }


    //////////////////////////////////////////////////////////// 
    // MemoryBytes

    void MemoryBytes::Init( Address addr, uint64_t size, IDebuggerProxy* debugger, ICoreProcess* proc )
    {
        _ASSERT( debugger != NULL );
        _ASSERT( proc != NULL );

        mAddr = addr;
        mSize = size;
        mDebugger = debugger;
        mProc = proc;
    }
}
