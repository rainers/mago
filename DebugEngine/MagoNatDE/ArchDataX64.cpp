/*
   Copyright (c) 2014 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ArchDataX64.h"
#include "RegisterSet.h"
#include "WinStackWalker.h"
#include <MagoDECommon.h>
#include <WinPlat.h>


namespace Mago
{
namespace
{
    const int RegCount = 296;
    extern const RegisterDesc gRegDesc[RegCount];

    const int DebugRegCount = 688;
    extern const uint16_t gRegMapX64[DebugRegCount];

    void GetX64RegisterGroups( const RegGroupInternal*& groups, uint32_t& count );
}


    ArchDataX64::ArchDataX64()
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

    ArchDataX64::ArchDataX64( UINT64 procFeatures )
        :   mProcFeatures( (ProcFeaturesX86) procFeatures )
    {
    }

    HRESULT ArchDataX64::BeginWalkStack( 
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

        _ASSERT( contextSize >= sizeof( CONTEXT_X64 ) );
        if ( contextSize < sizeof( CONTEXT_X64 ) )
            return E_INVALIDARG;

        const CONTEXT_X64*  context = (const CONTEXT_X64*) contextPtr;
        UniquePtr<WindowsStackWalker> walker( new WindowsStackWalker(
            IMAGE_FILE_MACHINE_AMD64,
            context->Rip,
            context->Rsp,
            context->Rbp,
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

    HRESULT ArchDataX64::BuildRegisterSet( 
        const void* threadContext,
        uint32_t threadContextSize,
        IRegisterSet*& regSet )
    {
        _ASSERT( threadContextSize >= sizeof( CONTEXT_X64 ) );
        if ( threadContext == NULL )
            return E_INVALIDARG;
        if ( threadContextSize < sizeof( CONTEXT_X64 ) )
            return E_INVALIDARG;

        RefPtr<RegisterSet> regs = new RegisterSet( gRegDesc, RegCount, RegX64_RIP );
        if ( regs == NULL )
            return E_OUTOFMEMORY;

        HRESULT hr = regs->Init( threadContext, sizeof( CONTEXT_X64 ) );
        if ( FAILED( hr ) )
            return hr;

        regSet = regs.Detach();

        return S_OK;
    }

    HRESULT ArchDataX64::BuildTinyRegisterSet( 
        const void* threadContext,
        uint32_t threadContextSize,
        IRegisterSet*& regSet )
    {
        _ASSERT( threadContextSize >= sizeof( CONTEXT_X64 ) );
        if ( threadContext == NULL )
            return E_INVALIDARG;
        if ( threadContextSize < sizeof( CONTEXT_X64 ) )
            return E_INVALIDARG;

        const CONTEXT_X64*  context = (const CONTEXT_X64*) threadContext;
        regSet = new TinyRegisterSet( 
            gRegDesc,
            RegCount,
            RegX64_RIP,
            RegX64_RSP,
            RegX64_RBP,
            (Address64) context->Rip,
            (Address64) context->Rsp,
            (Address64) context->Rbp );
        if ( regSet == NULL )
            return E_OUTOFMEMORY;

        regSet->AddRef();

        return S_OK;
    }

    uint32_t ArchDataX64::GetRegisterGroupCount()
    {
        const RegGroupInternal* groups = NULL;
        uint32_t count = 0;

        GetX64RegisterGroups( groups, count );
        return count;
    }

    bool ArchDataX64::GetRegisterGroup( uint32_t index, RegGroup& group )
    {
        const RegGroupInternal* groups = NULL;
        uint32_t count = 0;

        GetX64RegisterGroups( groups, count );

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

    int ArchDataX64::GetArchRegId( int debugRegId )
    {
        if ( debugRegId < 0 || debugRegId >= DebugRegCount )
            return -1;

        return gRegMapX64[debugRegId];
    }

    int ArchDataX64::GetPointerSize()
    {
        return 8;
    }

    void ArchDataX64::GetThreadContextSpec( ArchThreadContextSpec& spec )
    {
        spec.FeatureMask = CONTEXT_FULL | CONTEXT_SEGMENTS;
        spec.ExtFeatureMask = 0;
        spec.Size = sizeof( CONTEXT_X64 );
    }

    int ArchDataX64::GetPDataSize()
    {
        return sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY );
    }

    void ArchDataX64::GetPDataRange( 
        Address64 imageBase, 
        const void* pdata, 
        Address64& begin, 
        Address64& end )
    {
        const IMAGE_RUNTIME_FUNCTION_ENTRY* funcEntry = (IMAGE_RUNTIME_FUNCTION_ENTRY*) pdata;
        begin = imageBase + funcEntry->BeginAddress;
        end = imageBase + funcEntry->EndAddress;
    }


namespace
{
    const RegisterDesc gRegDesc[RegCount] = 
    {
        { 0, RegType_None, 0, 0, 0, 0, 0 },    // NONE
        { 0xFF, RegType_Int8, RegX64_RAX, 8, 0, 0, 0 },    // AL
        { 0xFF, RegType_Int8, RegX64_RCX, 8, 0, 0, 0 },    // CL
        { 0xFF, RegType_Int8, RegX64_RDX, 8, 0, 0, 0 },    // DL
        { 0xFF, RegType_Int8, RegX64_RBX, 8, 0, 0, 0 },    // BL
        { 0xFF, RegType_Int8, RegX64_RAX, 8, 8, 0, 0 },    // AH
        { 0xFF, RegType_Int8, RegX64_RCX, 8, 8, 0, 0 },    // CH
        { 0xFF, RegType_Int8, RegX64_RDX, 8, 8, 0, 0 },    // DH
        { 0xFF, RegType_Int8, RegX64_RBX, 8, 8, 0, 0 },    // BH
        { 0xFFFF, RegType_Int16, RegX64_RAX, 16, 0, 0, 0 },    // AX
        { 0xFFFF, RegType_Int16, RegX64_RCX, 16, 0, 0, 0 },    // CX
        { 0xFFFF, RegType_Int16, RegX64_RDX, 16, 0, 0, 0 },    // DX
        { 0xFFFF, RegType_Int16, RegX64_RBX, 16, 0, 0, 0 },    // BX
        { 0xFFFF, RegType_Int16, RegX64_RSP, 16, 0, 0, 0 },    // SP
        { 0xFFFF, RegType_Int16, RegX64_RBP, 16, 0, 0, 0 },    // BP
        { 0xFFFF, RegType_Int16, RegX64_RSI, 16, 0, 0, 0 },    // SI
        { 0xFFFF, RegType_Int16, RegX64_RDI, 16, 0, 0, 0 },    // DI
        { 0xFFFFFFFF, RegType_Int32, RegX64_RAX, 32, 0, 0, 0 },    // EAX
        { 0xFFFFFFFF, RegType_Int32, RegX64_RCX, 32, 0, 0, 0 },    // ECX
        { 0xFFFFFFFF, RegType_Int32, RegX64_RDX, 32, 0, 0, 0 },    // EDX
        { 0xFFFFFFFF, RegType_Int32, RegX64_RBX, 32, 0, 0, 0 },    // EBX
        { 0xFFFFFFFF, RegType_Int32, RegX64_RSP, 32, 0, 0, 0 },    // ESP
        { 0xFFFFFFFF, RegType_Int32, RegX64_RBP, 32, 0, 0, 0 },    // EBP
        { 0xFFFFFFFF, RegType_Int32, RegX64_RSI, 32, 0, 0, 0 },    // ESI
        { 0xFFFFFFFF, RegType_Int32, RegX64_RDI, 32, 0, 0, 0 },    // EDI
        // TODO: are the ones in ArchDataX86 right?
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, SegEs ), 2 },    // ES
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, SegCs ), 2 },    // CS
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, SegSs ), 2 },    // SS
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, SegDs ), 2 },    // DS
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, SegFs ), 2 },    // FS
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, SegGs ), 2 },    // GS
        { 0xFFFF, RegType_Int16, RegX64_EFLAGS, 16, 0, 0, 0 },    // FLAGS
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rip ), 8 },    // RIP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, EFlags ), 4 },    // EFLAGS

        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[0] ), 10 }, // ST0
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[1] ), 10 }, // ST1
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[2] ), 10 }, // ST2
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[3] ), 10 }, // ST3
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[4] ), 10 }, // ST4
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[5] ), 10 }, // ST5
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[6] ), 10 }, // ST6
        { 0, RegType_Float80, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[7] ), 10 }, // ST7

        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, FltSave.ControlWord ), 2 },   // CTRL
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, FltSave.StatusWord ), 2 },    // STAT
        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, FltSave.TagWord ), 1 },        // TAG
        { 0xFFFF, RegType_Int16, RegX64_FPEIP, 16, 0, 0, 0 }, // FPIP
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, FltSave.ErrorSelector ), 2 },   // FPCS
        { 0xFFFF, RegType_Int16, RegX64_FPEDO, 16, 0, 0, 0 },    // FPDO
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, FltSave.DataSelector ), 2 },  // FPDS
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, FltSave.ErrorOffset ), 4 }, // FPEIP
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, FltSave.DataOffset ), 4 },   // FPEDO
        // TODO: what about MxCsr and MxCsr_Mask and ErrorOpcode?

        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[0] ), 8 },    // MM0
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[1] ), 8 },    // MM1
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[2] ), 8 },    // MM2
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[3] ), 8 },    // MM3
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[4] ), 8 },    // MM4
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[5] ), 8 },    // MM5
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[6] ), 8 },    // MM6
        { 0, RegType_Int64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[7] ), 8 },    // MM7

        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm0 ), 16 },    // XMM0
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm1 ), 16 },    // XMM1
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm2 ), 16 },    // XMM2
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm3 ), 16 },    // XMM3
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm4 ), 16 },    // XMM4
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm5 ), 16 },    // XMM5
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm6 ), 16 },    // XMM6
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm7 ), 16 },    // XMM7

        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm0 ) + 12, 4 },  // XMM00
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm0 ) + 8, 4 },   // XMM01
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm0 ) + 4, 4 },   // XMM02
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm0 ) + 0, 4 },   // XMM03
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm1 ) + 12, 4 },  // XMM10
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm1 ) + 8, 4 },   // XMM11
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm1 ) + 4, 4 },   // XMM12
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm1 ) + 0, 4 },   // XMM13
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm2 ) + 12, 4 },  // XMM20
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm2 ) + 8, 4 },   // XMM21
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm2 ) + 4, 4 },   // XMM22
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm2 ) + 0, 4 },   // XMM23
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm3 ) + 12, 4 },  // XMM30
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm3 ) + 8, 4 },   // XMM31
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm3 ) + 4, 4 },   // XMM32
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm3 ) + 0, 4 },   // XMM33
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm4 ) + 12, 4 },  // XMM40
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm4 ) + 8, 4 },   // XMM41
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm4 ) + 4, 4 },   // XMM42
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm4 ) + 0, 4 },   // XMM43
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm5 ) + 12, 4 },  // XMM50
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm5 ) + 8, 4 },   // XMM51
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm5 ) + 4, 4 },   // XMM52
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm5 ) + 0, 4 },   // XMM53
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm6 ) + 12, 4 },  // XMM60
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm6 ) + 8, 4 },   // XMM61
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm6 ) + 4, 4 },   // XMM62
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm6 ) + 0, 4 },   // XMM63
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm7 ) + 12, 4 },  // XMM70
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm7 ) + 8, 4 },   // XMM71
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm7 ) + 4, 4 },   // XMM72
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm7 ) + 0, 4 },   // XMM73

        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm0 ) + 8, 8 },   // XMM0L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm1 ) + 8, 8 },   // XMM1L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm2 ) + 8, 8 },   // XMM2L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm3 ) + 8, 8 },   // XMM3L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm4 ) + 8, 8 },   // XMM4L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm5 ) + 8, 8 },   // XMM5L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm6 ) + 8, 8 },   // XMM6L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm7 ) + 8, 8 },   // XMM7L

        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm0 ) + 0, 8 },   // XMM0H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm1 ) + 0, 8 },   // XMM1H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm2 ) + 0, 8 },   // XMM2H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm3 ) + 0, 8 },   // XMM3H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm4 ) + 0, 8 },   // XMM4H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm5 ) + 0, 8 },   // XMM5H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm6 ) + 0, 8 },   // XMM6H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm7 ) + 0, 8 },   // XMM7H

        { 0, RegType_Int32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, FltSave.MxCsr ), 4 },    // MXCSR

        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm0 ) + 0, 8 },    // EMM0L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm1 ) + 0, 8 },    // EMM1L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm2 ) + 0, 8 },    // EMM2L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm3 ) + 0, 8 },    // EMM3L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm4 ) + 0, 8 },    // EMM4L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm5 ) + 0, 8 },    // EMM5L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm6 ) + 0, 8 },    // EMM6L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm7 ) + 0, 8 },    // EMM7L

        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm0 ) + 8, 8 },    // EMM0H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm1 ) + 8, 8 },    // EMM1H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm2 ) + 8, 8 },    // EMM2H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm3 ) + 8, 8 },    // EMM3H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm4 ) + 8, 8 },    // EMM4H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm5 ) + 8, 8 },    // EMM5H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm6 ) + 8, 8 },    // EMM6H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm7 ) + 8, 8 },    // EMM7H

        // TODO: 3D Now! ?
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[0] ) + 0, 4 },    // MM00
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[0] ) + 4, 4 },    // MM01
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[1] ) + 0, 4 },    // MM10
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[1] ) + 4, 4 },    // MM11
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[2] ) + 0, 4 },    // MM20
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[2] ) + 4, 4 },    // MM21
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[3] ) + 0, 4 },    // MM30
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[3] ) + 4, 4 },    // MM31
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[4] ) + 0, 4 },    // MM40
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[4] ) + 4, 4 },    // MM41
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[5] ) + 0, 4 },    // MM50
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[5] ) + 4, 4 },    // MM51
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[6] ) + 0, 4 },    // MM60
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[6] ) + 4, 4 },    // MM61
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[7] ) + 0, 4 },    // MM70
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Legacy[7] ) + 4, 4 },    // MM71

        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm8  ), 16 },    // XMM8
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm9  ), 16 },    // XMM9
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm10 ), 16 },    // XMM10
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm11 ), 16 },    // XMM11
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm12 ), 16 },    // XMM12
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm13 ), 16 },    // XMM13
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm14 ), 16 },    // XMM14
        { 0, RegType_Vector128, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm15 ), 16 },    // XMM15

        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm8  ) + 12, 4 },  // XMM8_0
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm8  ) + 8, 4 },   // XMM8_1
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm8  ) + 4, 4 },   // XMM8_2
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm8  ) + 0, 4 },   // XMM8_3
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm9  ) + 12, 4 },  // XMM9_0
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm9  ) + 8, 4 },   // XMM9_1
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm9  ) + 4, 4 },   // XMM9_2
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm9  ) + 0, 4 },   // XMM9_3
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm10 ) + 12, 4 },  // XMM10_0
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm10 ) + 8, 4 },   // XMM10_1
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm10 ) + 4, 4 },   // XMM10_2
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm10 ) + 0, 4 },   // XMM10_3
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm11 ) + 12, 4 },  // XMM11_0
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm11 ) + 8, 4 },   // XMM11_1
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm11 ) + 4, 4 },   // XMM11_2
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm11 ) + 0, 4 },   // XMM11_3
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm12 ) + 12, 4 },  // XMM12_0
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm12 ) + 8, 4 },   // XMM12_1
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm12 ) + 4, 4 },   // XMM12_2
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm12 ) + 0, 4 },   // XMM12_3
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm13 ) + 12, 4 },  // XMM13_0
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm13 ) + 8, 4 },   // XMM13_1
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm13 ) + 4, 4 },   // XMM13_2
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm13 ) + 0, 4 },   // XMM13_3
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm14 ) + 12, 4 },  // XMM14_0
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm14 ) + 8, 4 },   // XMM14_1
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm14 ) + 4, 4 },   // XMM14_2
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm14 ) + 0, 4 },   // XMM14_3
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm15 ) + 12, 4 },  // XMM15_0
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm15 ) + 8, 4 },   // XMM15_1
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm15 ) + 4, 4 },   // XMM15_2
        { 0, RegType_Float32, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm15 ) + 0, 4 },   // XMM15_3

        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm8  ) + 8, 8 },   // XMM8L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm9  ) + 8, 8 },   // XMM9L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm10 ) + 8, 8 },   // XMM10L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm11 ) + 8, 8 },   // XMM11L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm12 ) + 8, 8 },   // XMM12L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm13 ) + 8, 8 },   // XMM13L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm14 ) + 8, 8 },   // XMM14L
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm15 ) + 8, 8 },   // XMM15L

        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm8  ) + 0, 8 },   // XMM8H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm9  ) + 0, 8 },   // XMM9H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm10 ) + 0, 8 },   // XMM10H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm11 ) + 0, 8 },   // XMM11H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm12 ) + 0, 8 },   // XMM12H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm13 ) + 0, 8 },   // XMM13H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm14 ) + 0, 8 },   // XMM14H
        { 0, RegType_Float64, 0, 0, 0, (uint16_t) offsetof( CONTEXT_X64, Xmm15 ) + 0, 8 },   // XMM15H

        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm8  ) + 0, 8 },    // EMM8L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm9  ) + 0, 8 },    // EMM9L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm10 ) + 0, 8 },    // EMM10L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm11 ) + 0, 8 },    // EMM11L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm12 ) + 0, 8 },    // EMM12L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm13 ) + 0, 8 },    // EMM13L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm14 ) + 0, 8 },    // EMM14L
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm15 ) + 0, 8 },    // EMM15L

        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm8  ) + 8, 8 },    // EMM8H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm9  ) + 8, 8 },    // EMM9H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm10 ) + 8, 8 },    // EMM10H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm11 ) + 8, 8 },    // EMM11H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm12 ) + 8, 8 },    // EMM12H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm13 ) + 8, 8 },    // EMM13H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm14 ) + 8, 8 },    // EMM14H
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm15 ) + 8, 8 },    // EMM15H

        { 0xFF, RegType_Int8, RegX64_RSI, 8, 0, 0, 0 }, // SIL
        { 0xFF, RegType_Int8, RegX64_RDI, 8, 0, 0, 0 }, // DIL
        { 0xFF, RegType_Int8, RegX64_RBP, 8, 0, 0, 0 }, // BPL
        { 0xFF, RegType_Int8, RegX64_RSP, 8, 0, 0, 0 }, // SPL

        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rax ), 8 }, // RAX
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rbx ), 8 }, // RBX
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rcx ), 8 }, // RCX
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rdx ), 8 }, // RDX
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rsi ), 8 }, // RSI
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rdi ), 8 }, // RDI
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rbp ), 8 }, // RBP
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Rsp ), 8 }, // RSP

        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, R8  ), 8 }, // R8
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, R9  ), 8 }, // R9
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, R10 ), 8 }, // R10
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, R11 ), 8 }, // R11
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, R12 ), 8 }, // R12
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, R13 ), 8 }, // R13
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, R14 ), 8 }, // R14
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, R15 ), 8 }, // R15

        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, R8  ), 1 }, // R8B
        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, R9  ), 1 }, // R9B
        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, R10 ), 1 }, // R10B
        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, R11 ), 1 }, // R11B
        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, R12 ), 1 }, // R12B
        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, R13 ), 1 }, // R13B
        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, R14 ), 1 }, // R14B
        { 0, RegType_Int8, 0, 0, 0, offsetof( CONTEXT_X64, R15 ), 1 }, // R15B

        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, R8  ), 2 }, // R8W
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, R9  ), 2 }, // R9W
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, R10 ), 2 }, // R10W
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, R11 ), 2 }, // R11W
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, R12 ), 2 }, // R12W
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, R13 ), 2 }, // R13W
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, R14 ), 2 }, // R14W
        { 0, RegType_Int16, 0, 0, 0, offsetof( CONTEXT_X64, R15 ), 2 }, // R15W

        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, R8  ), 4 }, // R8D
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, R9  ), 4 }, // R9D
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, R10 ), 4 }, // R10D
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, R11 ), 4 }, // R11D
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, R12 ), 4 }, // R12D
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, R13 ), 4 }, // R13D
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, R14 ), 4 }, // R14D
        { 0, RegType_Int32, 0, 0, 0, offsetof( CONTEXT_X64, R15 ), 4 }, // R15D

        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm0  ) + 0, 8 },    // XMM0IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm1  ) + 0, 8 },    // XMM1IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm2  ) + 0, 8 },    // XMM2IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm3  ) + 0, 8 },    // XMM3IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm4  ) + 0, 8 },    // XMM4IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm5  ) + 0, 8 },    // XMM5IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm6  ) + 0, 8 },    // XMM6IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm7  ) + 0, 8 },    // XMM7IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm8  ) + 0, 8 },    // XMM8IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm9  ) + 0, 8 },    // XMM9IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm10 ) + 0, 8 },    // XMM10IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm11 ) + 0, 8 },    // XMM11IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm12 ) + 0, 8 },    // XMM12IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm13 ) + 0, 8 },    // XMM13IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm14 ) + 0, 8 },    // XMM14IL
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm15 ) + 0, 8 },    // XMM15IL

        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm0  ) + 8, 8 },    // XMM0IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm1  ) + 8, 8 },    // XMM1IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm2  ) + 8, 8 },    // XMM2IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm3  ) + 8, 8 },    // XMM3IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm4  ) + 8, 8 },    // XMM4IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm5  ) + 8, 8 },    // XMM5IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm6  ) + 8, 8 },    // XMM6IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm7  ) + 8, 8 },    // XMM7IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm8  ) + 8, 8 },    // XMM8IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm9  ) + 8, 8 },    // XMM9IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm10 ) + 8, 8 },    // XMM10IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm11 ) + 8, 8 },    // XMM11IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm12 ) + 8, 8 },    // XMM12IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm13 ) + 8, 8 },    // XMM13IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm14 ) + 8, 8 },    // XMM14IH
        { 0, RegType_Int64, 0, 0, 0, offsetof( CONTEXT_X64, Xmm15 ) + 8, 8 },    // XMM15IH
    };


    const uint16_t gRegMapX64[DebugRegCount] = 
    {
        RegX64_NONE,
        RegX64_AL,
        RegX64_CL,
        RegX64_DL,
        RegX64_BL,
        RegX64_AH,
        RegX64_CH,
        RegX64_DH,
        RegX64_BH,
        RegX64_AX,
        RegX64_CX,
        RegX64_DX,
        RegX64_BX,
        RegX64_SP,
        RegX64_BP,
        RegX64_SI,
        RegX64_DI,
        RegX64_EAX,
        RegX64_ECX,
        RegX64_EDX,
        RegX64_EBX,
        RegX64_ESP,
        RegX64_EBP,
        RegX64_ESI,
        RegX64_EDI,
        RegX64_ES,
        RegX64_CS,
        RegX64_SS,
        RegX64_DS,
        RegX64_FS,
        RegX64_GS,
        0,
        RegX64_FLAGS,
        RegX64_RIP,
        RegX64_EFLAGS,

        // (unmapped)
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

        // Control registers
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

        // Debug registers
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
        0,
        0,
        0,
        0,
        0,
        0,

        RegX64_ST0,
        RegX64_ST1,
        RegX64_ST2,
        RegX64_ST3,
        RegX64_ST4,
        RegX64_ST5,
        RegX64_ST6,
        RegX64_ST7,
        RegX64_CTRL,
        RegX64_STAT,
        RegX64_TAG,
        RegX64_FPIP,
        RegX64_FPCS,
        RegX64_FPDO,
        RegX64_FPDS,
        0,
        RegX64_FPEIP,
        RegX64_FPEDO,

        RegX64_MM0,
        RegX64_MM1,
        RegX64_MM2,
        RegX64_MM3,
        RegX64_MM4,
        RegX64_MM5,
        RegX64_MM6,
        RegX64_MM7,

        RegX64_XMM0,   // KATMAI registers
        RegX64_XMM1,
        RegX64_XMM2,
        RegX64_XMM3,
        RegX64_XMM4,
        RegX64_XMM5,
        RegX64_XMM6,
        RegX64_XMM7,

        RegX64_XMM0_0,   // KATMAI sub-registers
        RegX64_XMM0_1,
        RegX64_XMM0_2,
        RegX64_XMM0_3,
        RegX64_XMM1_0,
        RegX64_XMM1_1,
        RegX64_XMM1_2,
        RegX64_XMM1_3,
        RegX64_XMM2_0,
        RegX64_XMM2_1,
        RegX64_XMM2_2,
        RegX64_XMM2_3,
        RegX64_XMM3_0,
        RegX64_XMM3_1,
        RegX64_XMM3_2,
        RegX64_XMM3_3,
        RegX64_XMM4_0,
        RegX64_XMM4_1,
        RegX64_XMM4_2,
        RegX64_XMM4_3,
        RegX64_XMM5_0,
        RegX64_XMM5_1,
        RegX64_XMM5_2,
        RegX64_XMM5_3,
        RegX64_XMM6_0,
        RegX64_XMM6_1,
        RegX64_XMM6_2,
        RegX64_XMM6_3,
        RegX64_XMM7_0,
        RegX64_XMM7_1,
        RegX64_XMM7_2,
        RegX64_XMM7_3,

        RegX64_XMM0L,
        RegX64_XMM1L,
        RegX64_XMM2L,
        RegX64_XMM3L,
        RegX64_XMM4L,
        RegX64_XMM5L,
        RegX64_XMM6L,
        RegX64_XMM7L,

        RegX64_XMM0H,
        RegX64_XMM1H,
        RegX64_XMM2H,
        RegX64_XMM3H,
        RegX64_XMM4H,
        RegX64_XMM5H,
        RegX64_XMM6H,
        RegX64_XMM7H,

        0,

        RegX64_MXCSR,   // XMM status register

        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,

        RegX64_EMM0L,   // XMM sub-registers (WNI integer)
        RegX64_EMM1L,
        RegX64_EMM2L,
        RegX64_EMM3L,
        RegX64_EMM4L,
        RegX64_EMM5L,
        RegX64_EMM6L,
        RegX64_EMM7L,

        RegX64_EMM0H,
        RegX64_EMM1H,
        RegX64_EMM2H,
        RegX64_EMM3H,
        RegX64_EMM4H,
        RegX64_EMM5H,
        RegX64_EMM6H,
        RegX64_EMM7H,

        // do not change the order of these regs, first one must be even too
        RegX64_MM00,
        RegX64_MM01,
        RegX64_MM10,
        RegX64_MM11,
        RegX64_MM20,
        RegX64_MM21,
        RegX64_MM30,
        RegX64_MM31,
        RegX64_MM40,
        RegX64_MM41,
        RegX64_MM50,
        RegX64_MM51,
        RegX64_MM60,
        RegX64_MM61,
        RegX64_MM70,
        RegX64_MM71,

        // Extended KATMAI registers
        RegX64_XMM8,   // KATMAI registers
        RegX64_XMM9,
        RegX64_XMM10,
        RegX64_XMM11,
        RegX64_XMM12,
        RegX64_XMM13,
        RegX64_XMM14,
        RegX64_XMM15,

        RegX64_XMM8_0,   // KATMAI sub-registers
        RegX64_XMM8_1,
        RegX64_XMM8_2,
        RegX64_XMM8_3,
        RegX64_XMM9_0,
        RegX64_XMM9_1,
        RegX64_XMM9_2,
        RegX64_XMM9_3,
        RegX64_XMM10_0,
        RegX64_XMM10_1,
        RegX64_XMM10_2,
        RegX64_XMM10_3,
        RegX64_XMM11_0,
        RegX64_XMM11_1,
        RegX64_XMM11_2,
        RegX64_XMM11_3,
        RegX64_XMM12_0,
        RegX64_XMM12_1,
        RegX64_XMM12_2,
        RegX64_XMM12_3,
        RegX64_XMM13_0,
        RegX64_XMM13_1,
        RegX64_XMM13_2,
        RegX64_XMM13_3,
        RegX64_XMM14_0,
        RegX64_XMM14_1,
        RegX64_XMM14_2,
        RegX64_XMM14_3,
        RegX64_XMM15_0,
        RegX64_XMM15_1,
        RegX64_XMM15_2,
        RegX64_XMM15_3,

        RegX64_XMM8L,
        RegX64_XMM9L,
        RegX64_XMM10L,
        RegX64_XMM11L,
        RegX64_XMM12L,
        RegX64_XMM13L,
        RegX64_XMM14L,
        RegX64_XMM15L,

        RegX64_XMM8H,
        RegX64_XMM9H,
        RegX64_XMM10H,
        RegX64_XMM11H,
        RegX64_XMM12H,
        RegX64_XMM13H,
        RegX64_XMM14H,
        RegX64_XMM15H,

        RegX64_EMM8L,   // XMM sub-registers (WNI integer)
        RegX64_EMM9L,
        RegX64_EMM10L,
        RegX64_EMM11L,
        RegX64_EMM12L,
        RegX64_EMM13L,
        RegX64_EMM14L,
        RegX64_EMM15L,

        RegX64_EMM8H,
        RegX64_EMM9H,
        RegX64_EMM10H,
        RegX64_EMM11H,
        RegX64_EMM12H,
        RegX64_EMM13H,
        RegX64_EMM14H,
        RegX64_EMM15H,

        // Low byte forms of some standard registers
        RegX64_SIL,
        RegX64_DIL,
        RegX64_BPL,
        RegX64_SPL,

        // 64-bit regular registers
        RegX64_RAX,
        RegX64_RBX,
        RegX64_RCX,
        RegX64_RDX,
        RegX64_RSI,
        RegX64_RDI,
        RegX64_RBP,
        RegX64_RSP,

        // 64-bit integer registers with 8-, 16-, and 32-bit forms (B, W, and D)
        RegX64_R8,
        RegX64_R9,
        RegX64_R10,
        RegX64_R11,
        RegX64_R12,
        RegX64_R13,
        RegX64_R14,
        RegX64_R15,

        RegX64_R8B,
        RegX64_R9B,
        RegX64_R10B,
        RegX64_R11B,
        RegX64_R12B,
        RegX64_R13B,
        RegX64_R14B,
        RegX64_R15B,

        RegX64_R8W,
        RegX64_R9W,
        RegX64_R10W,
        RegX64_R11W,
        RegX64_R12W,
        RegX64_R13W,
        RegX64_R14W,
        RegX64_R15W,

        RegX64_R8D,
        RegX64_R9D,
        RegX64_R10D,
        RegX64_R11D,
        RegX64_R12D,
        RegX64_R13D,
        RegX64_R14D,
        RegX64_R15D,

        // AVX registers 256 bits
        RegX64_YMM0,
        RegX64_YMM1,
        RegX64_YMM2,
        RegX64_YMM3,
        RegX64_YMM4,
        RegX64_YMM5,
        RegX64_YMM6,
        RegX64_YMM7,
        RegX64_YMM8, 
        RegX64_YMM9,
        RegX64_YMM10,
        RegX64_YMM11,
        RegX64_YMM12,
        RegX64_YMM13,
        RegX64_YMM14,
        RegX64_YMM15,

        // AVX registers upper 128 bits
        RegX64_YMM0H,
        RegX64_YMM1H,
        RegX64_YMM2H,
        RegX64_YMM3H,
        RegX64_YMM4H,
        RegX64_YMM5H,
        RegX64_YMM6H,
        RegX64_YMM7H,
        RegX64_YMM8H, 
        RegX64_YMM9H,
        RegX64_YMM10H,
        RegX64_YMM11H,
        RegX64_YMM12H,
        RegX64_YMM13H,
        RegX64_YMM14H,
        RegX64_YMM15H,

        //Lower/upper 8 bytes of XMM registers.  Unlike CV_AMD64_XMM<regnum><H/L>, these
        //values reprsesent the bit patterns of the registers as 64-bit integers, not
        //the representation of these registers as a double.
        RegX64_XMM0IL,
        RegX64_XMM1IL,
        RegX64_XMM2IL,
        RegX64_XMM3IL,
        RegX64_XMM4IL,
        RegX64_XMM5IL,
        RegX64_XMM6IL,
        RegX64_XMM7IL,
        RegX64_XMM8IL,
        RegX64_XMM9IL,
        RegX64_XMM10IL,
        RegX64_XMM11IL,
        RegX64_XMM12IL,
        RegX64_XMM13IL,
        RegX64_XMM14IL,
        RegX64_XMM15IL,

        RegX64_XMM0IH,
        RegX64_XMM1IH,
        RegX64_XMM2IH,
        RegX64_XMM3IH,
        RegX64_XMM4IH,
        RegX64_XMM5IH,
        RegX64_XMM6IH,
        RegX64_XMM7IH,
        RegX64_XMM8IH,
        RegX64_XMM9IH,
        RegX64_XMM10IH,
        RegX64_XMM11IH,
        RegX64_XMM12IH,
        RegX64_XMM13IH,
        RegX64_XMM14IH,
        RegX64_XMM15IH,

        RegX64_YMM0I0,        // AVX integer registers
        RegX64_YMM0I1,
        RegX64_YMM0I2,
        RegX64_YMM0I3,
        RegX64_YMM1I0,
        RegX64_YMM1I1,
        RegX64_YMM1I2,
        RegX64_YMM1I3,
        RegX64_YMM2I0,
        RegX64_YMM2I1,
        RegX64_YMM2I2,
        RegX64_YMM2I3,
        RegX64_YMM3I0,
        RegX64_YMM3I1,
        RegX64_YMM3I2,
        RegX64_YMM3I3,
        RegX64_YMM4I0,
        RegX64_YMM4I1,
        RegX64_YMM4I2,
        RegX64_YMM4I3,
        RegX64_YMM5I0,
        RegX64_YMM5I1,
        RegX64_YMM5I2,
        RegX64_YMM5I3,
        RegX64_YMM6I0,
        RegX64_YMM6I1,
        RegX64_YMM6I2,
        RegX64_YMM6I3,
        RegX64_YMM7I0,
        RegX64_YMM7I1,
        RegX64_YMM7I2,
        RegX64_YMM7I3,
        RegX64_YMM8I0,
        RegX64_YMM8I1,
        RegX64_YMM8I2,
        RegX64_YMM8I3,
        RegX64_YMM9I0,
        RegX64_YMM9I1,
        RegX64_YMM9I2,
        RegX64_YMM9I3,
        RegX64_YMM10I0,
        RegX64_YMM10I1,
        RegX64_YMM10I2,
        RegX64_YMM10I3,
        RegX64_YMM11I0,
        RegX64_YMM11I1,
        RegX64_YMM11I2,
        RegX64_YMM11I3,
        RegX64_YMM12I0,
        RegX64_YMM12I1,
        RegX64_YMM12I2,
        RegX64_YMM12I3,
        RegX64_YMM13I0,
        RegX64_YMM13I1,
        RegX64_YMM13I2,
        RegX64_YMM13I3,
        RegX64_YMM14I0,
        RegX64_YMM14I1,
        RegX64_YMM14I2,
        RegX64_YMM14I3,
        RegX64_YMM15I0,
        RegX64_YMM15I1,
        RegX64_YMM15I2,
        RegX64_YMM15I3,

        RegX64_YMM0F0,        // AVX floating-point single precise registers
        RegX64_YMM0F1,
        RegX64_YMM0F2,
        RegX64_YMM0F3,
        RegX64_YMM0F4,
        RegX64_YMM0F5,
        RegX64_YMM0F6,
        RegX64_YMM0F7,
        RegX64_YMM1F0,
        RegX64_YMM1F1,
        RegX64_YMM1F2,
        RegX64_YMM1F3,
        RegX64_YMM1F4,
        RegX64_YMM1F5,
        RegX64_YMM1F6,
        RegX64_YMM1F7,
        RegX64_YMM2F0,
        RegX64_YMM2F1,
        RegX64_YMM2F2,
        RegX64_YMM2F3,
        RegX64_YMM2F4,
        RegX64_YMM2F5,
        RegX64_YMM2F6,
        RegX64_YMM2F7,
        RegX64_YMM3F0,
        RegX64_YMM3F1,
        RegX64_YMM3F2,
        RegX64_YMM3F3,
        RegX64_YMM3F4,
        RegX64_YMM3F5,
        RegX64_YMM3F6,
        RegX64_YMM3F7,
        RegX64_YMM4F0,
        RegX64_YMM4F1,
        RegX64_YMM4F2,
        RegX64_YMM4F3,
        RegX64_YMM4F4,
        RegX64_YMM4F5,
        RegX64_YMM4F6,
        RegX64_YMM4F7,
        RegX64_YMM5F0,
        RegX64_YMM5F1,
        RegX64_YMM5F2,
        RegX64_YMM5F3,
        RegX64_YMM5F4,
        RegX64_YMM5F5,
        RegX64_YMM5F6,
        RegX64_YMM5F7,
        RegX64_YMM6F0,
        RegX64_YMM6F1,
        RegX64_YMM6F2,
        RegX64_YMM6F3,
        RegX64_YMM6F4,
        RegX64_YMM6F5,
        RegX64_YMM6F6,
        RegX64_YMM6F7,
        RegX64_YMM7F0,
        RegX64_YMM7F1,
        RegX64_YMM7F2,
        RegX64_YMM7F3,
        RegX64_YMM7F4,
        RegX64_YMM7F5,
        RegX64_YMM7F6,
        RegX64_YMM7F7,
        RegX64_YMM8F0,
        RegX64_YMM8F1,
        RegX64_YMM8F2,
        RegX64_YMM8F3,
        RegX64_YMM8F4,
        RegX64_YMM8F5,
        RegX64_YMM8F6,
        RegX64_YMM8F7,
        RegX64_YMM9F0,
        RegX64_YMM9F1,
        RegX64_YMM9F2,
        RegX64_YMM9F3,
        RegX64_YMM9F4,
        RegX64_YMM9F5,
        RegX64_YMM9F6,
        RegX64_YMM9F7,
        RegX64_YMM10F0,
        RegX64_YMM10F1,
        RegX64_YMM10F2,
        RegX64_YMM10F3,
        RegX64_YMM10F4,
        RegX64_YMM10F5,
        RegX64_YMM10F6,
        RegX64_YMM10F7,
        RegX64_YMM11F0,
        RegX64_YMM11F1,
        RegX64_YMM11F2,
        RegX64_YMM11F3,
        RegX64_YMM11F4,
        RegX64_YMM11F5,
        RegX64_YMM11F6,
        RegX64_YMM11F7,
        RegX64_YMM12F0,
        RegX64_YMM12F1,
        RegX64_YMM12F2,
        RegX64_YMM12F3,
        RegX64_YMM12F4,
        RegX64_YMM12F5,
        RegX64_YMM12F6,
        RegX64_YMM12F7,
        RegX64_YMM13F0,
        RegX64_YMM13F1,
        RegX64_YMM13F2,
        RegX64_YMM13F3,
        RegX64_YMM13F4,
        RegX64_YMM13F5,
        RegX64_YMM13F6,
        RegX64_YMM13F7,
        RegX64_YMM14F0,
        RegX64_YMM14F1,
        RegX64_YMM14F2,
        RegX64_YMM14F3,
        RegX64_YMM14F4,
        RegX64_YMM14F5,
        RegX64_YMM14F6,
        RegX64_YMM14F7,
        RegX64_YMM15F0,
        RegX64_YMM15F1,
        RegX64_YMM15F2,
        RegX64_YMM15F3,
        RegX64_YMM15F4,
        RegX64_YMM15F5,
        RegX64_YMM15F6,
        RegX64_YMM15F7,
    
        RegX64_YMM0D0,        // AVX floating-point double precise registers
        RegX64_YMM0D1,
        RegX64_YMM0D2,
        RegX64_YMM0D3,
        RegX64_YMM1D0,
        RegX64_YMM1D1,
        RegX64_YMM1D2,
        RegX64_YMM1D3,
        RegX64_YMM2D0,
        RegX64_YMM2D1,
        RegX64_YMM2D2,
        RegX64_YMM2D3,
        RegX64_YMM3D0,
        RegX64_YMM3D1,
        RegX64_YMM3D2,
        RegX64_YMM3D3,
        RegX64_YMM4D0,
        RegX64_YMM4D1,
        RegX64_YMM4D2,
        RegX64_YMM4D3,
        RegX64_YMM5D0,
        RegX64_YMM5D1,
        RegX64_YMM5D2,
        RegX64_YMM5D3,
        RegX64_YMM6D0,
        RegX64_YMM6D1,
        RegX64_YMM6D2,
        RegX64_YMM6D3,
        RegX64_YMM7D0,
        RegX64_YMM7D1,
        RegX64_YMM7D2,
        RegX64_YMM7D3,
        RegX64_YMM8D0,
        RegX64_YMM8D1,
        RegX64_YMM8D2,
        RegX64_YMM8D3,
        RegX64_YMM9D0,
        RegX64_YMM9D1,
        RegX64_YMM9D2,
        RegX64_YMM9D3,
        RegX64_YMM10D0,
        RegX64_YMM10D1,
        RegX64_YMM10D2,
        RegX64_YMM10D3,
        RegX64_YMM11D0,
        RegX64_YMM11D1,
        RegX64_YMM11D2,
        RegX64_YMM11D3,
        RegX64_YMM12D0,
        RegX64_YMM12D1,
        RegX64_YMM12D2,
        RegX64_YMM12D3,
        RegX64_YMM13D0,
        RegX64_YMM13D1,
        RegX64_YMM13D2,
        RegX64_YMM13D3,
        RegX64_YMM14D0,
        RegX64_YMM14D1,
        RegX64_YMM14D2,
        RegX64_YMM14D3,
        RegX64_YMM15D0,
        RegX64_YMM15D1,
        RegX64_YMM15D2,
        RegX64_YMM15D3,
    };


    static const Reg    gX64CpuRegList[] = 
    {
        { L"RAX", RegX64_RAX, 0, 0, 0 },
        { L"RBX", RegX64_RBX, 0, 0, 0 },
        { L"RCX", RegX64_RCX, 0, 0, 0 },
        { L"RDX", RegX64_RDX, 0, 0, 0 },
        { L"RSI", RegX64_RSI, 0, 0, 0 },
        { L"RDI", RegX64_RDI, 0, 0, 0 },
        { L"R8",  RegX64_R8,  0, 0, 0 },
        { L"R9",  RegX64_R9,  0, 0, 0 },
        { L"R10", RegX64_R10, 0, 0, 0 },
        { L"R11", RegX64_R11, 0, 0, 0 },
        { L"R12", RegX64_R12, 0, 0, 0 },
        { L"R13", RegX64_R13, 0, 0, 0 },
        { L"R14", RegX64_R14, 0, 0, 0 },
        { L"R15", RegX64_R15, 0, 0, 0 },
        { L"RIP", RegX64_RIP, 0, 0, 0 },
        { L"RSP", RegX64_RSP, 0, 0, 0 },
        { L"RBP", RegX64_RBP, 0, 0, 0 },
        { L"EFL", RegX64_EFLAGS, 32, 0, 0xFFFFFFFF },
    };

    static const Reg    gX64SegmentsRegList[] = 
    {
        { L"CS", RegX64_CS, 16, 0, 0xFFFF },
        { L"DS", RegX64_DS, 16, 0, 0xFFFF },
        { L"ES", RegX64_ES, 16, 0, 0xFFFF },
        { L"FS", RegX64_FS, 16, 0, 0xFFFF },
        { L"GS", RegX64_GS, 16, 0, 0xFFFF },
        { L"SS", RegX64_SS, 16, 0, 0xFFFF },
    };

    static const Reg    gX64FloatingRegList[] = 
    {
        { L"ST0", RegX64_ST0, 0, 0, 0 },
        { L"ST1", RegX64_ST1, 0, 0, 0 },
        { L"ST2", RegX64_ST2, 0, 0, 0 },
        { L"ST3", RegX64_ST3, 0, 0, 0 },
        { L"ST4", RegX64_ST4, 0, 0, 0 },
        { L"ST5", RegX64_ST5, 0, 0, 0 },
        { L"ST6", RegX64_ST6, 0, 0, 0 },
        { L"ST7", RegX64_ST7, 0, 0, 0 },
        { L"STAT", RegX64_STAT, 16, 0, 0xFFFF },
        { L"TAG", RegX64_TAG, 16, 0, 0xFFFF },
        { L"CTRL", RegX64_CTRL, 16, 0, 0xFFFF },
        { L"FPEDO", RegX64_FPEDO, 32, 0, 0xFFFFFFFF },
        { L"FPEIP", RegX64_FPEIP, 32, 0, 0xFFFFFFFF },
    };

    static const Reg    gX64FlagsRegList[] = 
    {
        { L"OF", RegX64_EFLAGS, 1, 11, 1 },
        { L"DF", RegX64_EFLAGS, 1, 10, 1 },
        { L"IF", RegX64_EFLAGS, 1, 9, 1 },
        { L"SF", RegX64_EFLAGS, 1, 7, 1 },
        { L"ZF", RegX64_EFLAGS, 1, 6, 1 },
        { L"AF", RegX64_EFLAGS, 1, 4, 1 },
        { L"PF", RegX64_EFLAGS, 1, 2, 1 },
        { L"CF", RegX64_EFLAGS, 1, 0, 1 },
    };

    static const Reg    gX64MMXRegList[] = 
    {
        { L"MM0", RegX64_MM0, 0, 0, 0 },
        { L"MM1", RegX64_MM1, 0, 0, 0 },
        { L"MM2", RegX64_MM2, 0, 0, 0 },
        { L"MM3", RegX64_MM3, 0, 0, 0 },
        { L"MM4", RegX64_MM4, 0, 0, 0 },
        { L"MM5", RegX64_MM5, 0, 0, 0 },
        { L"MM6", RegX64_MM6, 0, 0, 0 },
        { L"MM7", RegX64_MM7, 0, 0, 0 },
    };

    static const Reg    gX64SSERegList[] = 
    {
        { L"XMM0", RegX64_XMM0, 0, 0, 0 },
        { L"XMM1", RegX64_XMM1, 0, 0, 0 },
        { L"XMM2", RegX64_XMM2, 0, 0, 0 },
        { L"XMM3", RegX64_XMM3, 0, 0, 0 },
        { L"XMM4", RegX64_XMM4, 0, 0, 0 },
        { L"XMM5", RegX64_XMM5, 0, 0, 0 },
        { L"XMM6", RegX64_XMM6, 0, 0, 0 },
        { L"XMM7", RegX64_XMM7, 0, 0, 0 },

        { L"XMM00", RegX64_XMM0_0, 0, 0, 0 },
        { L"XMM01", RegX64_XMM0_1, 0, 0, 0 },
        { L"XMM02", RegX64_XMM0_2, 0, 0, 0 },
        { L"XMM03", RegX64_XMM0_3, 0, 0, 0 },

        { L"XMM10", RegX64_XMM1_0, 0, 0, 0 },
        { L"XMM11", RegX64_XMM1_1, 0, 0, 0 },
        { L"XMM12", RegX64_XMM1_2, 0, 0, 0 },
        { L"XMM13", RegX64_XMM1_3, 0, 0, 0 },

        { L"XMM20", RegX64_XMM2_0, 0, 0, 0 },
        { L"XMM21", RegX64_XMM2_1, 0, 0, 0 },
        { L"XMM22", RegX64_XMM2_2, 0, 0, 0 },
        { L"XMM23", RegX64_XMM2_3, 0, 0, 0 },

        { L"XMM30", RegX64_XMM3_0, 0, 0, 0 },
        { L"XMM31", RegX64_XMM3_1, 0, 0, 0 },
        { L"XMM32", RegX64_XMM3_2, 0, 0, 0 },
        { L"XMM33", RegX64_XMM3_3, 0, 0, 0 },

        { L"XMM40", RegX64_XMM4_0, 0, 0, 0 },
        { L"XMM41", RegX64_XMM4_1, 0, 0, 0 },
        { L"XMM42", RegX64_XMM4_2, 0, 0, 0 },
        { L"XMM43", RegX64_XMM4_3, 0, 0, 0 },

        { L"XMM50", RegX64_XMM5_0, 0, 0, 0 },
        { L"XMM51", RegX64_XMM5_1, 0, 0, 0 },
        { L"XMM52", RegX64_XMM5_2, 0, 0, 0 },
        { L"XMM53", RegX64_XMM5_3, 0, 0, 0 },

        { L"XMM60", RegX64_XMM6_0, 0, 0, 0 },
        { L"XMM61", RegX64_XMM6_1, 0, 0, 0 },
        { L"XMM62", RegX64_XMM6_2, 0, 0, 0 },
        { L"XMM63", RegX64_XMM6_3, 0, 0, 0 },

        { L"XMM70", RegX64_XMM7_0, 0, 0, 0 },
        { L"XMM71", RegX64_XMM7_1, 0, 0, 0 },
        { L"XMM72", RegX64_XMM7_2, 0, 0, 0 },
        { L"XMM73", RegX64_XMM7_3, 0, 0, 0 },

        { L"MXCSR", RegX64_MXCSR, 0, 0, 0 },
    };

    static const Reg    gX64SSE2RegList[] = 
    {
        { L"XMM8",  RegX64_XMM8,  0, 0, 0 },
        { L"XMM9",  RegX64_XMM9,  0, 0, 0 },
        { L"XMM10", RegX64_XMM10, 0, 0, 0 },
        { L"XMM11", RegX64_XMM11, 0, 0, 0 },
        { L"XMM12", RegX64_XMM12, 0, 0, 0 },
        { L"XMM13", RegX64_XMM13, 0, 0, 0 },
        { L"XMM14", RegX64_XMM14, 0, 0, 0 },
        { L"XMM15", RegX64_XMM15, 0, 0, 0 },

        { L"XMM0DL", RegX64_XMM0L, 0, 0, 0 },
        { L"XMM0DH", RegX64_XMM0H, 0, 0, 0 },
        { L"XMM1DL", RegX64_XMM1L, 0, 0, 0 },
        { L"XMM1DH", RegX64_XMM1H, 0, 0, 0 },
        { L"XMM2DL", RegX64_XMM2L, 0, 0, 0 },
        { L"XMM2DH", RegX64_XMM2H, 0, 0, 0 },
        { L"XMM3DL", RegX64_XMM3L, 0, 0, 0 },
        { L"XMM3DH", RegX64_XMM3H, 0, 0, 0 },
        { L"XMM4DL", RegX64_XMM4L, 0, 0, 0 },
        { L"XMM4DH", RegX64_XMM4H, 0, 0, 0 },
        { L"XMM5DL", RegX64_XMM5L, 0, 0, 0 },
        { L"XMM5DH", RegX64_XMM5H, 0, 0, 0 },
        { L"XMM6DL", RegX64_XMM6L, 0, 0, 0 },
        { L"XMM6DH", RegX64_XMM6H, 0, 0, 0 },
        { L"XMM7DL", RegX64_XMM7L, 0, 0, 0 },
        { L"XMM7DH", RegX64_XMM7H, 0, 0, 0 },

        { L"XMM8DL",  RegX64_XMM8L, 0, 0, 0 },
        { L"XMM8DH",  RegX64_XMM8H, 0, 0, 0 },
        { L"XMM9DL",  RegX64_XMM9L, 0, 0, 0 },
        { L"XMM9DH",  RegX64_XMM9H, 0, 0, 0 },
        { L"XMM10DL", RegX64_XMM10L, 0, 0, 0 },
        { L"XMM10DH", RegX64_XMM10H, 0, 0, 0 },
        { L"XMM11DL", RegX64_XMM11L, 0, 0, 0 },
        { L"XMM11DH", RegX64_XMM11H, 0, 0, 0 },
        { L"XMM12DL", RegX64_XMM12L, 0, 0, 0 },
        { L"XMM12DH", RegX64_XMM12H, 0, 0, 0 },
        { L"XMM13DL", RegX64_XMM13L, 0, 0, 0 },
        { L"XMM13DH", RegX64_XMM13H, 0, 0, 0 },
        { L"XMM14DL", RegX64_XMM14L, 0, 0, 0 },
        { L"XMM14DH", RegX64_XMM14H, 0, 0, 0 },
        { L"XMM15DL", RegX64_XMM15L, 0, 0, 0 },
        { L"XMM15DH", RegX64_XMM15H, 0, 0, 0 },

        { L"XMM0IL", RegX64_XMM0IL, 0, 0, 0 },
        { L"XMM0IH", RegX64_XMM0IH, 0, 0, 0 },
        { L"XMM1IL", RegX64_XMM1IL, 0, 0, 0 },
        { L"XMM1IH", RegX64_XMM1IH, 0, 0, 0 },
        { L"XMM2IL", RegX64_XMM2IL, 0, 0, 0 },
        { L"XMM2IH", RegX64_XMM2IH, 0, 0, 0 },
        { L"XMM3IL", RegX64_XMM3IL, 0, 0, 0 },
        { L"XMM3IH", RegX64_XMM3IH, 0, 0, 0 },
        { L"XMM4IL", RegX64_XMM4IL, 0, 0, 0 },
        { L"XMM4IH", RegX64_XMM4IH, 0, 0, 0 },
        { L"XMM5IL", RegX64_XMM5IL, 0, 0, 0 },
        { L"XMM5IH", RegX64_XMM5IH, 0, 0, 0 },
        { L"XMM6IL", RegX64_XMM6IL, 0, 0, 0 },
        { L"XMM6IH", RegX64_XMM6IH, 0, 0, 0 },
        { L"XMM7IL", RegX64_XMM7IL, 0, 0, 0 },
        { L"XMM7IH", RegX64_XMM7IH, 0, 0, 0 },

        { L"XMM8IL",  RegX64_XMM8IL, 0, 0, 0 },
        { L"XMM8IH",  RegX64_XMM8IH, 0, 0, 0 },
        { L"XMM9IL",  RegX64_XMM9IL, 0, 0, 0 },
        { L"XMM9IH",  RegX64_XMM9IH, 0, 0, 0 },
        { L"XMM10IL", RegX64_XMM10IL, 0, 0, 0 },
        { L"XMM10IH", RegX64_XMM10IH, 0, 0, 0 },
        { L"XMM11IL", RegX64_XMM11IL, 0, 0, 0 },
        { L"XMM11IH", RegX64_XMM11IH, 0, 0, 0 },
        { L"XMM12IL", RegX64_XMM12IL, 0, 0, 0 },
        { L"XMM12IH", RegX64_XMM12IH, 0, 0, 0 },
        { L"XMM13IL", RegX64_XMM13IL, 0, 0, 0 },
        { L"XMM13IH", RegX64_XMM13IH, 0, 0, 0 },
        { L"XMM14IL", RegX64_XMM14IL, 0, 0, 0 },
        { L"XMM14IH", RegX64_XMM14IH, 0, 0, 0 },
        { L"XMM15IL", RegX64_XMM15IL, 0, 0, 0 },
        { L"XMM15IH", RegX64_XMM15IH, 0, 0, 0 },

        { L"XMM80", RegX64_XMM8_0, 0, 0, 0 },
        { L"XMM81", RegX64_XMM8_1, 0, 0, 0 },
        { L"XMM82", RegX64_XMM8_2, 0, 0, 0 },
        { L"XMM83", RegX64_XMM8_3, 0, 0, 0 },

        { L"XMM90", RegX64_XMM9_0, 0, 0, 0 },
        { L"XMM91", RegX64_XMM9_1, 0, 0, 0 },
        { L"XMM92", RegX64_XMM9_2, 0, 0, 0 },
        { L"XMM93", RegX64_XMM9_3, 0, 0, 0 },

        { L"XMM100", RegX64_XMM10_0, 0, 0, 0 },
        { L"XMM101", RegX64_XMM10_1, 0, 0, 0 },
        { L"XMM102", RegX64_XMM10_2, 0, 0, 0 },
        { L"XMM103", RegX64_XMM10_3, 0, 0, 0 },

        { L"XMM110", RegX64_XMM11_0, 0, 0, 0 },
        { L"XMM111", RegX64_XMM11_1, 0, 0, 0 },
        { L"XMM112", RegX64_XMM11_2, 0, 0, 0 },
        { L"XMM113", RegX64_XMM11_3, 0, 0, 0 },

        { L"XMM120", RegX64_XMM12_0, 0, 0, 0 },
        { L"XMM121", RegX64_XMM12_1, 0, 0, 0 },
        { L"XMM122", RegX64_XMM12_2, 0, 0, 0 },
        { L"XMM123", RegX64_XMM12_3, 0, 0, 0 },

        { L"XMM130", RegX64_XMM13_0, 0, 0, 0 },
        { L"XMM131", RegX64_XMM13_1, 0, 0, 0 },
        { L"XMM132", RegX64_XMM13_2, 0, 0, 0 },
        { L"XMM133", RegX64_XMM13_3, 0, 0, 0 },

        { L"XMM140", RegX64_XMM14_0, 0, 0, 0 },
        { L"XMM141", RegX64_XMM14_1, 0, 0, 0 },
        { L"XMM142", RegX64_XMM14_2, 0, 0, 0 },
        { L"XMM143", RegX64_XMM14_3, 0, 0, 0 },

        { L"XMM150", RegX64_XMM15_0, 0, 0, 0 },
        { L"XMM151", RegX64_XMM15_1, 0, 0, 0 },
        { L"XMM152", RegX64_XMM15_2, 0, 0, 0 },
        { L"XMM153", RegX64_XMM15_3, 0, 0, 0 },
    };

    static const RegGroupInternal  gX64RegGroups[] = 
    {
        { IDS_REGGROUP_CPU, gX64CpuRegList, _countof( gX64CpuRegList ), 0 },
        { IDS_REGGROUP_CPU_SEGMENTS, gX64SegmentsRegList, _countof( gX64SegmentsRegList ), 0 },
        { IDS_REGGROUP_FLOATING_POINT, gX64FloatingRegList, _countof( gX64FloatingRegList ), 0 },
        { IDS_REGGROUP_FLAGS, gX64FlagsRegList, _countof( gX64FlagsRegList ), 0 },
        { IDS_REGGROUP_MMX, gX64MMXRegList, _countof( gX64MMXRegList ), PF_X86_MMX },
        { IDS_REGGROUP_SSE, gX64SSERegList, _countof( gX64SSERegList ), PF_X86_SSE },
        { IDS_REGGROUP_SSE2, gX64SSE2RegList, _countof( gX64SSE2RegList ), PF_X86_SSE2 },
    };


    void GetX64RegisterGroups( const RegGroupInternal*& groups, uint32_t& count )
    {
        groups = gX64RegGroups;
        count = _countof( gX64RegGroups );
    }
}
}
