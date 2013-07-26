/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ArchDataX86.h"
#include "WinStackWalker.h"
#include "RegisterSet.h"
#include "EnumX86Reg.h"


namespace Mago
{
#if defined( _M_IX86 )
    // For now, because VS is only built for x86, a typedef is enough.
    // But, if ever it works for other architectures, then CONTEXT_X86 will need 
    // to be made an exact copy of the CONTEXT structure for x86, so that it can 
    // be used to refer to x86 debuggees.
    typedef CONTEXT CONTEXT_X86;
#else
#error Define a CONTEXT_X86 structure that looks and works exactly like CONTEXT for x86
#endif


    ArchDataX86::ArchDataX86()
    {
    }

    HRESULT ArchDataX86::BeginWalkStack( 
        const void* threadContext, 
        uint32_t threadContextSize,
        void* processContext,
        ReadProcessMemory64Proc readMemProc,
        FunctionTableAccess64Proc funcTabProc,
        GetModuleBase64Proc getModBaseProc,
        StackWalker*& stackWalker )
    {
        HRESULT hr = S_OK;

        if ( threadContext == NULL )
            return E_INVALIDARG;
        if ( threadContextSize != sizeof( CONTEXT_X86 ) )
            return E_INVALIDARG;

        auto context = (const CONTEXT_X86*) threadContext;
        std::unique_ptr<WindowsStackWalker> walker( new WindowsStackWalker(
            IMAGE_FILE_MACHINE_I386,
            context->Eip,
            context->Esp,
            context->Ebp,
            processContext,
            readMemProc,
            funcTabProc,
            getModBaseProc ) );

        hr = walker->Init( threadContext, sizeof( CONTEXT_X86 ) );
        if ( FAILED( hr ) )
            return hr;

        stackWalker = walker.release();
        return S_OK;
    }

    Address ArchDataX86::GetPC( const void* threadContext )
    {
        auto context = (const CONTEXT_X86*) threadContext;
        return context->Eip;
    }

    HRESULT ArchDataX86::BuildRegisterSet( 
        const void* threadContext,
        ::Thread* coreThread, 
        IRegisterSet*& regSet )
    {
        if ( threadContext == NULL )
            return E_INVALIDARG;
        if ( coreThread == NULL )
            return E_INVALIDARG;

        auto context = (const CONTEXT_X86*) threadContext;
        regSet = new RegisterSet( *context );
        if ( regSet == NULL )
            return E_OUTOFMEMORY;

        regSet->AddRef();

        return S_OK;
    }

    HRESULT ArchDataX86::BuildTinyRegisterSet( 
        const void* threadContext,
        ::Thread* coreThread, 
        IRegisterSet*& regSet )
    {
        if ( threadContext == NULL )
            return E_INVALIDARG;
        if ( coreThread == NULL )
            return E_INVALIDARG;

        auto context = (const CONTEXT_X86*) threadContext;
        regSet = new TinyRegisterSet( 
            (Address) context->Eip,
            (Address) context->Esp,
            (Address) context->Ebp );
        if ( regSet == NULL )
            return E_OUTOFMEMORY;

        regSet->AddRef();

        return S_OK;
    }

    void ArchDataX86::GetRegisterGroups( const RegGroup*& groups, uint32_t& count )
    {
        return GetX86RegisterGroups( groups, count );
    }
}
