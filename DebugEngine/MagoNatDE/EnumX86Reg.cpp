/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EnumX86Reg.h"
#include "RegProperty.h"
#include "RegisterSet.h"


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

    static const RegGroup   gX86RegGroups[] = 
    {
        { IDS_REGGROUP_CPU, gX86CpuRegList, _countof( gX86CpuRegList ) },
        { IDS_REGGROUP_CPU_SEGMENTS, gX86SegmentsRegList, _countof( gX86SegmentsRegList ) },
        { IDS_REGGROUP_FLOATING_POINT, gX86FloatingRegList, _countof( gX86FloatingRegList ) },
        { IDS_REGGROUP_FLAGS, gX86FlagsRegList, _countof( gX86FlagsRegList ) },
    };


    HRESULT EnumX86Registers( 
        IRegisterSet* regSet, 
        DEBUGPROP_INFO_FLAGS fields,
        DWORD radix,
        IEnumDebugPropertyInfo2** enumerator )
    {
        OutputDebugStringA( "EnumX86Registers\n" );

        return EnumRegisters( 
            gX86RegGroups,
            _countof( gX86RegGroups ),
            regSet,
            fields,
            radix,
            enumerator );
    }
}
