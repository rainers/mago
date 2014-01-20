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
#include <MagoDECommon.h>


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

    const int DebugRegCount = 252;
    extern const uint8_t gRegMapX86[DebugRegCount];
}


    ArchDataX86::ArchDataX86()
        :   mProcFeatures( PF_X86_None )
    {
        UINT64 procFeatures = 0;

        if ( IsProcessorFeaturePresent( PF_MMX_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_MMX;

        if ( IsProcessorFeaturePresent( PF_3DNOW_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_3DNow;

        if ( IsProcessorFeaturePresent( PF_XMMI_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_SSE;

        if ( IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_SSE2;

        if ( IsProcessorFeaturePresent( PF_SSE3_INSTRUCTIONS_AVAILABLE ) )
            procFeatures |= PF_X86_SSE3;

        if ( IsProcessorFeaturePresent( PF_XSAVE_ENABLED ) )
            procFeatures |= PF_X86_AVX;

        mProcFeatures = (ProcFeaturesX86) procFeatures;
    }

    ArchDataX86::ArchDataX86( UINT64 procFeatures )
        :   mProcFeatures( (ProcFeaturesX86) procFeatures )
    {
    }

    HRESULT ArchDataX86::BeginWalkStack( 
        IRegisterSet* topRegSet, 
        void* processContext,
        ReadProcessMemory64Proc readMemProc,
        FunctionTableAccess64Proc funcTabProc,
        GetModuleBase64Proc getModBaseProc,
        StackWalker*& stackWalker )
    {
        if ( topRegSet == NULL )
            return E_INVALIDARG;

        HRESULT     hr = S_OK;
        const void* contextPtr = NULL;
        uint32_t    contextSize = 0;

        if ( !topRegSet->GetThreadContext( contextPtr, contextSize ) )
            return E_FAIL;

        _ASSERT( contextSize >= sizeof( CONTEXT_X86 ) );
        if ( contextSize < sizeof( CONTEXT_X86 ) )
            return E_INVALIDARG;

        const CONTEXT_X86*  context = (const CONTEXT_X86*) contextPtr;
        UniquePtr<WindowsStackWalker> walker( new WindowsStackWalker(
            IMAGE_FILE_MACHINE_I386,
            context->Eip,
            context->Esp,
            context->Ebp,
            processContext,
            readMemProc,
            funcTabProc,
            getModBaseProc ) );

        hr = walker->Init( context, contextSize );
        if ( FAILED( hr ) )
            return hr;

        stackWalker = walker.Detach();
        return S_OK;
    }

    HRESULT ArchDataX86::BuildRegisterSet( 
        const void* threadContext,
        uint32_t threadContextSize,
        IRegisterSet*& regSet )
    {
        _ASSERT( threadContextSize >= sizeof( CONTEXT_X86 ) );
        if ( threadContext == NULL )
            return E_INVALIDARG;
        if ( threadContextSize < sizeof( CONTEXT_X86 ) )
            return E_INVALIDARG;

        RefPtr<RegisterSet> regs = new RegisterSet( gRegDesc, RegCount, RegX86_EIP );
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
        uint32_t threadContextSize,
        IRegisterSet*& regSet )
    {
        _ASSERT( threadContextSize >= sizeof( CONTEXT_X86 ) );
        if ( threadContext == NULL )
            return E_INVALIDARG;
        if ( threadContextSize < sizeof( CONTEXT_X86 ) )
            return E_INVALIDARG;

        const CONTEXT_X86*  context = (const CONTEXT_X86*) threadContext;
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

    uint32_t ArchDataX86::GetRegisterGroupCount()
    {
        const RegGroupInternal* groups = NULL;
        uint32_t count = 0;

        GetX86RegisterGroups( groups, count );
        return count;
    }

    bool ArchDataX86::GetRegisterGroup( uint32_t index, RegGroup& group )
    {
        const RegGroupInternal* groups = NULL;
        uint32_t count = 0;

        GetX86RegisterGroups( groups, count );

        if ( index >= count )
            return false;

        uint32_t neededFeature = groups[index].NeededFeature;

        if ( neededFeature == 0
             || (mProcFeatures & neededFeature) == neededFeature )
        {
            group.RegCount = groups[index].RegCount;
            group.Regs = groups[index].Regs;
        }
        else
        {
            group.RegCount = 0;
            group.Regs = NULL;
        }

        group.StrId = groups[index].StrId;

        return true;
    }

    int ArchDataX86::GetArchRegId( int debugRegId )
    {
        if ( debugRegId < 0 || debugRegId >= DebugRegCount )
            return -1;

        return gRegMapX86[debugRegId];
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


    const uint8_t gRegMapX86[DebugRegCount] = 
    {
        RegX86_NONE,
        RegX86_AL,
        RegX86_CL,
        RegX86_DL,
        RegX86_BL,
        RegX86_AH,
        RegX86_CH,
        RegX86_DH,
        RegX86_BH,
        RegX86_AX,
        RegX86_CX,
        RegX86_DX,
        RegX86_BX,
        RegX86_SP,
        RegX86_BP,
        RegX86_SI,
        RegX86_DI,
        RegX86_EAX,
        RegX86_ECX,
        RegX86_EDX,
        RegX86_EBX,
        RegX86_ESP,
        RegX86_EBP,
        RegX86_ESI,
        RegX86_EDI,
        RegX86_ES,
        RegX86_CS,
        RegX86_SS,
        RegX86_DS,
        RegX86_FS,
        RegX86_GS,
        RegX86_IP,
        RegX86_FLAGS,
        RegX86_EIP,
        RegX86_EFLAGS,

        0,
        0,
        0,
        0,
        0,

        0, //RegX86_TEMP,
        0, //RegX86_TEMPH,
        0, //RegX86_QUOTE,
        0, //RegX86_PCDR3,
        0, //RegX86_PCDR4,
        0, //RegX86_PCDR5,
        0, //RegX86_PCDR6,
        0, //RegX86_PCDR7,

        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,

        0, //RegX86_CR0,
        0, //RegX86_CR1,
        0, //RegX86_CR2,
        0, //RegX86_CR3,
        0, //RegX86_CR4,

        0,
        0,
        0,
        0,
        0,

        RegX86_DR0,
        RegX86_DR1,
        RegX86_DR2,
        RegX86_DR3,
        0, //RegX86_DR4,
        0, //RegX86_DR5,
        RegX86_DR6,
        RegX86_DR7,

        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,

        0, //RegX86_GDTR,
        0, //RegX86_GDTL,
        0, //RegX86_IDTR,
        0, //RegX86_IDTL,
        0, //RegX86_LDTR,
        0, //RegX86_TR,

        0, //RegX86_PSEUDO1,
        0, //RegX86_PSEUDO2,
        0, //RegX86_PSEUDO3,
        0, //RegX86_PSEUDO4,
        0, //RegX86_PSEUDO5,
        0, //RegX86_PSEUDO6,
        0, //RegX86_PSEUDO7,
        0, //RegX86_PSEUDO8,
        0, //RegX86_PSEUDO9,

        0,
        0,
        0,

        RegX86_ST0,
        RegX86_ST1,
        RegX86_ST2,
        RegX86_ST3,
        RegX86_ST4,
        RegX86_ST5,
        RegX86_ST6,
        RegX86_ST7,
        RegX86_CTRL,
        RegX86_STAT,
        RegX86_TAG,
        RegX86_FPIP,
        RegX86_FPCS,
        RegX86_FPDO,
        RegX86_FPDS,
        0,
        RegX86_FPEIP,
        RegX86_FPEDO,

        RegX86_MM0,
        RegX86_MM1,
        RegX86_MM2,
        RegX86_MM3,
        RegX86_MM4,
        RegX86_MM5,
        RegX86_MM6,
        RegX86_MM7,

        RegX86_XMM0,
        RegX86_XMM1,
        RegX86_XMM2,
        RegX86_XMM3,
        RegX86_XMM4,
        RegX86_XMM5,
        RegX86_XMM6,
        RegX86_XMM7,

        RegX86_XMM00,
        RegX86_XMM01,
        RegX86_XMM02,
        RegX86_XMM03,
        RegX86_XMM10,
        RegX86_XMM11,
        RegX86_XMM12,
        RegX86_XMM13,
        RegX86_XMM20,
        RegX86_XMM21,
        RegX86_XMM22,
        RegX86_XMM23,
        RegX86_XMM30,
        RegX86_XMM31,
        RegX86_XMM32,
        RegX86_XMM33,
        RegX86_XMM40,
        RegX86_XMM41,
        RegX86_XMM42,
        RegX86_XMM43,
        RegX86_XMM50,
        RegX86_XMM51,
        RegX86_XMM52,
        RegX86_XMM53,
        RegX86_XMM60,
        RegX86_XMM61,
        RegX86_XMM62,
        RegX86_XMM63,
        RegX86_XMM70,
        RegX86_XMM71,
        RegX86_XMM72,
        RegX86_XMM73,

        RegX86_XMM0L,
        RegX86_XMM1L,
        RegX86_XMM2L,
        RegX86_XMM3L,
        RegX86_XMM4L,
        RegX86_XMM5L,
        RegX86_XMM6L,
        RegX86_XMM7L,

        RegX86_XMM0H,
        RegX86_XMM1H,
        RegX86_XMM2H,
        RegX86_XMM3H,
        RegX86_XMM4H,
        RegX86_XMM5H,
        RegX86_XMM6H,
        RegX86_XMM7H,

        0,

        RegX86_MXCSR,

        0, //EDXEAX
        0,
        0,
        0,
        0,
        0,
        0,
        0,

        RegX86_EMM0L,
        RegX86_EMM1L,
        RegX86_EMM2L,
        RegX86_EMM3L,
        RegX86_EMM4L,
        RegX86_EMM5L,
        RegX86_EMM6L,
        RegX86_EMM7L,

        RegX86_EMM0H,
        RegX86_EMM1H,
        RegX86_EMM2H,
        RegX86_EMM3H,
        RegX86_EMM4H,
        RegX86_EMM5H,
        RegX86_EMM6H,
        RegX86_EMM7H,

        RegX86_MM00,
        RegX86_MM01,
        RegX86_MM10,
        RegX86_MM11,
        RegX86_MM20,
        RegX86_MM21,
        RegX86_MM30,
        RegX86_MM31,
        RegX86_MM40,
        RegX86_MM41,
        RegX86_MM50,
        RegX86_MM51,
        RegX86_MM60,
        RegX86_MM61,
        RegX86_MM70,
        RegX86_MM71,
    };

    C_ASSERT( _countof( gRegMapX86 ) == 252 );
    C_ASSERT( RegX86_Max <= 256 );
}
}
