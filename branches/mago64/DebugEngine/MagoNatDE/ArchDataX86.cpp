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


namespace
{
    const int RegCount = 155;
    extern const RegisterDesc gRegDesc[RegCount];
}


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

        RefPtr<RegisterSet> regs = new RegisterSet( gRegDesc, RegCount );
        if ( regs == NULL )
            return E_OUTOFMEMORY;

        HRESULT hr = regs->Init( threadContext, sizeof( CONTEXT_X86 ) );
        if ( FAILED( hr ) )
            return hr;

        regSet = regs.Detach();

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
            gRegDesc,
            RegCount,
            RegX86_EIP,
            RegX86_ESP,
            RegX86_EBP,
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


namespace
{
    const RegisterDesc gRegDesc[RegCount] = 
    {
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // NONE
        { 0xFF, RegType_Int8, RegX86_EAX, 8, 0, 0, 0 },    // AL
        { 0xFF, RegType_Int8, RegX86_ECX, 8, 0, 0, 0 },    // CL
        { 0xFF, RegType_Int8, RegX86_EDX, 8, 0, 0, 0 },    // DL
        { 0xFF, RegType_Int8, RegX86_EBX, 8, 0, 0, 0 },    // BL
        { 0xFF, RegType_Int8, RegX86_EAX, 8, 8, 0, 0 },    // AH
        { 0xFF, RegType_Int8, RegX86_ECX, 8, 8, 0, 0 },    // CH
        { 0xFF, RegType_Int8, RegX86_EDX, 8, 8, 0, 0 },    // DH
        { 0xFF, RegType_Int8, RegX86_EBX, 8, 8, 0, 0 },    // BH
        { 0xFFFF, RegType_Int16, RegX86_EAX, 16, 0, 0, 0 },    // AX
        { 0xFFFF, RegType_Int16, RegX86_ECX, 16, 0, 0, 0 },    // CX
        { 0xFFFF, RegType_Int16, RegX86_EDX, 16, 0, 0, 0 },    // DX
        { 0xFFFF, RegType_Int16, RegX86_EBX, 16, 0, 0, 0 },    // BX
        { 0xFFFF, RegType_Int16, RegX86_ESP, 16, 0, 0, 0 },    // SP
        { 0xFFFF, RegType_Int16, RegX86_EBP, 16, 0, 0, 0 },    // BP
        { 0xFFFF, RegType_Int16, RegX86_ESI, 16, 0, 0, 0 },    // SI
        { 0xFFFF, RegType_Int16, RegX86_EDI, 16, 0, 0, 0 },    // DI
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Eax ), 4 },    // EAX
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Ecx ), 4 },    // ECX
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Edx ), 4 },    // EDX
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Ebx ), 4 },    // EBX
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Esp ), 4 },    // ESP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Ebp ), 4 },    // EBP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Esi ), 4 },    // ESI
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Edi ), 4 },    // EDI
        { 0xFFFF, RegType_Int16, RegX86_ES, 16, 0, offsetof( CONTEXT_X86, SegEs ), 4 },    // ES
        { 0xFFFF, RegType_Int16, RegX86_CS, 16, 0, offsetof( CONTEXT_X86, SegCs ), 4 },    // CS
        { 0xFFFF, RegType_Int16, RegX86_SS, 16, 0, offsetof( CONTEXT_X86, SegSs ), 4 },    // SS
        { 0xFFFF, RegType_Int16, RegX86_DS, 16, 0, offsetof( CONTEXT_X86, SegDs ), 4 },    // DS
        { 0xFFFF, RegType_Int16, RegX86_FS, 16, 0, offsetof( CONTEXT_X86, SegFs ), 4 },    // FS
        { 0xFFFF, RegType_Int16, RegX86_GS, 16, 0, offsetof( CONTEXT_X86, SegGs ), 4 },    // GS
        { 0xFFFF, RegType_Int16, RegX86_EIP, 16, 0, 0, 0 },    // IP
        { 0xFFFF, RegType_Int16, RegX86_EFLAGS, 16, 0, 0, 0 },    // FLAGS
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Eip ), 4 },    // EIP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, EFlags ), 4 },    // EFLAGS

        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Dr0 ), 4 },    // DR0
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Dr1 ), 4 },    // DR1
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Dr2 ), 4 },    // DR2
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Dr3 ), 4 },    // DR3
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Dr6 ), 4 },    // DR6
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, Dr7 ), 4 },    // DR7

        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 0 * 10 ] ), 10 },  // ST0
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 1 * 10 ] ), 10 },  // ST1
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 2 * 10 ] ), 10 },  // ST2
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 3 * 10 ] ), 10 },  // ST3
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 4 * 10 ] ), 10 },  // ST4
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 5 * 10 ] ), 10 },  // ST5
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 6 * 10 ] ), 10 },  // ST6
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 7 * 10 ] ), 10 },  // ST7
        { 0xFFFF, RegType_Int16, RegX86_CTRL, 16, 0, offsetof( CONTEXT_X86, FloatSave.ControlWord ), 4 },   // CTRL
        { 0xFFFF, RegType_Int16, RegX86_STAT, 16, 0, offsetof( CONTEXT_X86, FloatSave.StatusWord ), 4 },    // STAT
        { 0xFFFF, RegType_Int16, RegX86_TAG, 16, 0, offsetof( CONTEXT_X86, FloatSave.TagWord ), 4 },        // TAG
        { 0xFFFF, RegType_Int16, RegX86_FPEIP, 16, 0, offsetof( CONTEXT_X86, FloatSave.ErrorOffset ), 4 }, // FPIP
        { 0xFFFF, RegType_Int16, RegX86_FPCS, 16, 0, offsetof( CONTEXT_X86, FloatSave.ErrorSelector ), 4 },   // FPCS
        { 0xFFFF, RegType_Int16, RegX86_FPEDO, 16, 0, offsetof( CONTEXT_X86, FloatSave.DataOffset ), 4 },    // FPDO
        { 0xFFFF, RegType_Int16, RegX86_FPDS, 16, 0, offsetof( CONTEXT_X86, FloatSave.DataSelector ), 4 },  // FPDS
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, FloatSave.ErrorOffset ), 4 }, // FPEIP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X86, FloatSave.DataOffset ), 4 },   // FPEDO

        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 0 * 10 ] ), 8 },    // MM0
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 1 * 10 ] ), 8 },    // MM1
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 2 * 10 ] ), 8 },    // MM2
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 3 * 10 ] ), 8 },    // MM3
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 4 * 10 ] ), 8 },    // MM4
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 5 * 10 ] ), 8 },    // MM5
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 6 * 10 ] ), 8 },    // MM6
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, FloatSave.RegisterArea[ 7 * 10 ] ), 8 },    // MM7

        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 10 * 16 ] ), 16 },    // XMM0
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 11 * 16 ] ), 16 },    // XMM1
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 12 * 16 ] ), 16 },    // XMM2
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 13 * 16 ] ), 16 },    // XMM3
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 14 * 16 ] ), 16 },    // XMM4
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 15 * 16 ] ), 16 },    // XMM5
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 16 * 16 ] ), 16 },    // XMM6
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 17 * 16 ] ), 16 },    // XMM7

        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 10 * 16 ] ) + 12, 4 },  // XMM00
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 10 * 16 ] ) + 8, 4 },   // XMM01
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 10 * 16 ] ) + 4, 4 },   // XMM02
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 10 * 16 ] ) + 0, 4 },   // XMM03
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 11 * 16 ] ) + 12, 4 },  // XMM10
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 11 * 16 ] ) + 8, 4 },   // XMM11
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 11 * 16 ] ) + 4, 4 },   // XMM12
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 11 * 16 ] ) + 0, 4 },   // XMM13
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 12 * 16 ] ) + 12, 4 },  // XMM20
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 12 * 16 ] ) + 8, 4 },   // XMM21
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 12 * 16 ] ) + 4, 4 },   // XMM22
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 12 * 16 ] ) + 0, 4 },   // XMM23
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 13 * 16 ] ) + 12, 4 },  // XMM30
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 13 * 16 ] ) + 8, 4 },   // XMM31
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 13 * 16 ] ) + 4, 4 },   // XMM32
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 13 * 16 ] ) + 0, 4 },   // XMM33
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 14 * 16 ] ) + 12, 4 },  // XMM40
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 14 * 16 ] ) + 8, 4 },   // XMM41
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 14 * 16 ] ) + 4, 4 },   // XMM42
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 14 * 16 ] ) + 0, 4 },   // XMM43
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 15 * 16 ] ) + 12, 4 },  // XMM50
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 15 * 16 ] ) + 8, 4 },   // XMM51
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 15 * 16 ] ) + 4, 4 },   // XMM52
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 15 * 16 ] ) + 0, 4 },   // XMM53
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 16 * 16 ] ) + 12, 4 },  // XMM60
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 16 * 16 ] ) + 8, 4 },   // XMM61
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 16 * 16 ] ) + 4, 4 },   // XMM62
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 16 * 16 ] ) + 0, 4 },   // XMM63
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 17 * 16 ] ) + 12, 4 },  // XMM70
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 17 * 16 ] ) + 8, 4 },   // XMM71
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 17 * 16 ] ) + 4, 4 },   // XMM72
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 17 * 16 ] ) + 0, 4 },   // XMM73

        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 10 * 16 ] ) + 8, 8 },   // XMM0L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 11 * 16 ] ) + 8, 8 },   // XMM1L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 12 * 16 ] ) + 8, 8 },   // XMM2L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 13 * 16 ] ) + 8, 8 },   // XMM3L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 14 * 16 ] ) + 8, 8 },   // XMM4L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 15 * 16 ] ) + 8, 8 },   // XMM5L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 16 * 16 ] ) + 8, 8 },   // XMM6L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 17 * 16 ] ) + 8, 8 },   // XMM7L

        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 10 * 16 ] ) + 0, 8 },   // XMM0H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 11 * 16 ] ) + 0, 8 },   // XMM1H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 12 * 16 ] ) + 0, 8 },   // XMM2H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 13 * 16 ] ) + 0, 8 },   // XMM3H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 14 * 16 ] ) + 0, 8 },   // XMM4H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 15 * 16 ] ) + 0, 8 },   // XMM5H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 16 * 16 ] ) + 0, 8 },   // XMM6H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 17 * 16 ] ) + 0, 8 },   // XMM7H

        { 0, RegType_Int32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X86, ExtendedRegisters[ 24 ] ), 4 },    // MXCSR

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM0L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM1L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM2L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM3L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM4L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM5L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM6L
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM7L

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM0H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM1H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM2H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM3H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM4H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM5H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM6H
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // EMM7H

        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM00
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM01
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM10
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM11
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM20
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM21
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM30
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM31
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM40
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM41
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM50
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM51
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM60
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM61
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM70
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // MM71
    };
}
}
