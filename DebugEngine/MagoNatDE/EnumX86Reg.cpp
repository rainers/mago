/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EnumX86Reg.h"
#include "RegProperty.h"
#include "ArchDataX86.h"
#include <MagoDECommon.h>


namespace Mago
{
    static const Reg    gX86CpuRegList[] = 
    {
        { L"EAX", RegX86_EAX, 32, 0, 0xFFFFFFFF },
        { L"EBX", RegX86_EBX, 32, 0, 0xFFFFFFFF },
        { L"ECX", RegX86_ECX, 32, 0, 0xFFFFFFFF },
        { L"EDX", RegX86_EDX, 32, 0, 0xFFFFFFFF },
        { L"ESI", RegX86_ESI, 32, 0, 0xFFFFFFFF },
        { L"EDI", RegX86_EDI, 32, 0, 0xFFFFFFFF },
        { L"EBP", RegX86_EBP, 32, 0, 0xFFFFFFFF },
        { L"ESP", RegX86_ESP, 32, 0, 0xFFFFFFFF },
        { L"EIP", RegX86_EIP, 32, 0, 0xFFFFFFFF },
        { L"EFL", RegX86_EFLAGS, 32, 0, 0xFFFFFFFF },
    };

    static const Reg    gX86SegmentsRegList[] = 
    {
        { L"CS", RegX86_CS, 16, 0, 0xFFFF },
        { L"DS", RegX86_DS, 16, 0, 0xFFFF },
        { L"ES", RegX86_ES, 16, 0, 0xFFFF },
        { L"FS", RegX86_FS, 16, 0, 0xFFFF },
        { L"GS", RegX86_GS, 16, 0, 0xFFFF },
        { L"SS", RegX86_SS, 16, 0, 0xFFFF },
    };

    static const Reg    gX86FloatingRegList[] = 
    {
        { L"ST0", RegX86_ST0, 0, 0, 0 },
        { L"ST1", RegX86_ST1, 0, 0, 0 },
        { L"ST2", RegX86_ST2, 0, 0, 0 },
        { L"ST3", RegX86_ST3, 0, 0, 0 },
        { L"ST4", RegX86_ST4, 0, 0, 0 },
        { L"ST5", RegX86_ST5, 0, 0, 0 },
        { L"ST6", RegX86_ST6, 0, 0, 0 },
        { L"ST7", RegX86_ST7, 0, 0, 0 },
        { L"STAT", RegX86_STAT, 16, 0, 0xFFFF },
        { L"TAG", RegX86_TAG, 16, 0, 0xFFFF },
        { L"CTRL", RegX86_CTRL, 16, 0, 0xFFFF },
        { L"FPEDO", RegX86_FPEDO, 32, 0, 0xFFFFFFFF },
        { L"FPEIP", RegX86_FPEIP, 32, 0, 0xFFFFFFFF },
    };

    static const Reg    gX86FlagsRegList[] = 
    {
        { L"OF", RegX86_EFLAGS, 1, 11, 1 },
        { L"DF", RegX86_EFLAGS, 1, 10, 1 },
        { L"IF", RegX86_EFLAGS, 1, 9, 1 },
        { L"SF", RegX86_EFLAGS, 1, 7, 1 },
        { L"ZF", RegX86_EFLAGS, 1, 6, 1 },
        { L"AF", RegX86_EFLAGS, 1, 4, 1 },
        { L"PF", RegX86_EFLAGS, 1, 2, 1 },
        { L"CF", RegX86_EFLAGS, 1, 0, 1 },
    };

    static const Reg    gX86MMXRegList[] = 
    {
        { L"MM0", RegX86_MM0, 0, 0, 0 },
        { L"MM1", RegX86_MM1, 0, 0, 0 },
        { L"MM2", RegX86_MM2, 0, 0, 0 },
        { L"MM3", RegX86_MM3, 0, 0, 0 },
        { L"MM4", RegX86_MM4, 0, 0, 0 },
        { L"MM5", RegX86_MM5, 0, 0, 0 },
        { L"MM6", RegX86_MM6, 0, 0, 0 },
        { L"MM7", RegX86_MM7, 0, 0, 0 },
    };

    static const Reg    gX86SSERegList[] = 
    {
        { L"XMM0", RegX86_XMM0, 0, 0, 0 },
        { L"XMM1", RegX86_XMM1, 0, 0, 0 },
        { L"XMM2", RegX86_XMM2, 0, 0, 0 },
        { L"XMM3", RegX86_XMM3, 0, 0, 0 },
        { L"XMM4", RegX86_XMM4, 0, 0, 0 },
        { L"XMM5", RegX86_XMM5, 0, 0, 0 },
        { L"XMM6", RegX86_XMM6, 0, 0, 0 },
        { L"XMM7", RegX86_XMM7, 0, 0, 0 },

        { L"XMM00", RegX86_XMM00, 0, 0, 0 },
        { L"XMM01", RegX86_XMM01, 0, 0, 0 },
        { L"XMM02", RegX86_XMM02, 0, 0, 0 },
        { L"XMM03", RegX86_XMM03, 0, 0, 0 },

        { L"XMM10", RegX86_XMM10, 0, 0, 0 },
        { L"XMM11", RegX86_XMM11, 0, 0, 0 },
        { L"XMM12", RegX86_XMM12, 0, 0, 0 },
        { L"XMM13", RegX86_XMM13, 0, 0, 0 },

        { L"XMM20", RegX86_XMM20, 0, 0, 0 },
        { L"XMM21", RegX86_XMM21, 0, 0, 0 },
        { L"XMM22", RegX86_XMM22, 0, 0, 0 },
        { L"XMM23", RegX86_XMM23, 0, 0, 0 },

        { L"XMM30", RegX86_XMM30, 0, 0, 0 },
        { L"XMM31", RegX86_XMM31, 0, 0, 0 },
        { L"XMM32", RegX86_XMM32, 0, 0, 0 },
        { L"XMM33", RegX86_XMM33, 0, 0, 0 },

        { L"XMM40", RegX86_XMM40, 0, 0, 0 },
        { L"XMM41", RegX86_XMM41, 0, 0, 0 },
        { L"XMM42", RegX86_XMM42, 0, 0, 0 },
        { L"XMM43", RegX86_XMM43, 0, 0, 0 },

        { L"XMM50", RegX86_XMM50, 0, 0, 0 },
        { L"XMM51", RegX86_XMM51, 0, 0, 0 },
        { L"XMM52", RegX86_XMM52, 0, 0, 0 },
        { L"XMM53", RegX86_XMM53, 0, 0, 0 },

        { L"XMM60", RegX86_XMM60, 0, 0, 0 },
        { L"XMM61", RegX86_XMM61, 0, 0, 0 },
        { L"XMM62", RegX86_XMM62, 0, 0, 0 },
        { L"XMM63", RegX86_XMM63, 0, 0, 0 },

        { L"XMM70", RegX86_XMM70, 0, 0, 0 },
        { L"XMM71", RegX86_XMM71, 0, 0, 0 },
        { L"XMM72", RegX86_XMM72, 0, 0, 0 },
        { L"XMM73", RegX86_XMM73, 0, 0, 0 },

        { L"MXCSR", RegX86_MXCSR, 0, 0, 0 },
    };

    static const Reg    gX86SSE2RegList[] = 
    {
        { L"XMM0DL", RegX86_XMM0L, 0, 0, 0 },
        { L"XMM0DH", RegX86_XMM0H, 0, 0, 0 },
        { L"XMM1DL", RegX86_XMM1L, 0, 0, 0 },
        { L"XMM1DH", RegX86_XMM1H, 0, 0, 0 },
        { L"XMM2DL", RegX86_XMM2L, 0, 0, 0 },
        { L"XMM2DH", RegX86_XMM2H, 0, 0, 0 },
        { L"XMM3DL", RegX86_XMM3L, 0, 0, 0 },
        { L"XMM3DH", RegX86_XMM3H, 0, 0, 0 },
        { L"XMM4DL", RegX86_XMM4L, 0, 0, 0 },
        { L"XMM4DH", RegX86_XMM4H, 0, 0, 0 },
        { L"XMM5DL", RegX86_XMM5L, 0, 0, 0 },
        { L"XMM5DH", RegX86_XMM5H, 0, 0, 0 },
        { L"XMM6DL", RegX86_XMM6L, 0, 0, 0 },
        { L"XMM6DH", RegX86_XMM6H, 0, 0, 0 },
        { L"XMM7DL", RegX86_XMM7L, 0, 0, 0 },
        { L"XMM7DH", RegX86_XMM7H, 0, 0, 0 },
    };

    static const RegGroupInternal  gX86RegGroups[] = 
    {
        { IDS_REGGROUP_CPU, gX86CpuRegList, _countof( gX86CpuRegList ), 0 },
        { IDS_REGGROUP_CPU_SEGMENTS, gX86SegmentsRegList, _countof( gX86SegmentsRegList ), 0 },
        { IDS_REGGROUP_FLOATING_POINT, gX86FloatingRegList, _countof( gX86FloatingRegList ), 0 },
        { IDS_REGGROUP_FLAGS, gX86FlagsRegList, _countof( gX86FlagsRegList ), 0 },
        { IDS_REGGROUP_MMX, gX86MMXRegList, _countof( gX86MMXRegList ), PF_X86_MMX },
        { IDS_REGGROUP_SSE, gX86SSERegList, _countof( gX86SSERegList ), PF_X86_SSE },
        { IDS_REGGROUP_SSE2, gX86SSE2RegList, _countof( gX86SSE2RegList ), PF_X86_SSE2 },
    };


    void GetX86RegisterGroups( const RegGroupInternal*& groups, uint32_t& count )
    {
        groups = gX86RegGroups;
        count = _countof( gX86RegGroups );
    }
}
