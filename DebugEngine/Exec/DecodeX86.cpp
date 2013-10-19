/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DecodeX86.h"


struct Prefixes32
{
    bool    AddressSize;    // 67
    bool    OperandSize;    // 66
    bool    Lock;           // F0
    bool    RepF2;          // F2
    bool    RepF3;          // F3
    bool    Cs;             // 2E
    bool    Ds;             // 3E
    bool    Es;             // 26
    bool    Fs;             // 64
    bool    Gs;             // 65
    bool    Ss;             // 36
};

union RexPrefix
{
    struct
    {
        bool    B : 1;
        bool    X : 1;
        bool    R : 1;
        bool    W : 1;
        uint8_t RexConst : 4;
    } Bits;
    uint8_t Byte;
};

struct Prefixes64
{
    RexPrefix   Rex;
};

struct Prefixes
{
    Prefixes32  Pre32;
    Prefixes64  Pre64;
};


// number of prefixes
static int ReadPrefixes( uint8_t* mem, int memLen, CpuSizeMode mode, Prefixes& prefixes )
{
    _ASSERT( (mode == Cpu_32) || (mode == Cpu_64) );

    int i;

    for ( i = 0; i < memLen; i++ )
    {
        bool    found = false;

        switch ( mem[i] )
        {
        case 0x67:  prefixes.Pre32.AddressSize = true; break;
        case 0x66:  prefixes.Pre32.OperandSize = true; break;
        case 0xF0:  prefixes.Pre32.Lock = true; break;
        case 0xF2:  prefixes.Pre32.RepF2 = true; break;
        case 0xF3:  prefixes.Pre32.RepF3 = true; break;
        case 0x2E:  prefixes.Pre32.Cs = true; break;
        case 0x3E:  prefixes.Pre32.Ds = true; break;
        case 0x26:  prefixes.Pre32.Es = true; break;
        case 0x64:  prefixes.Pre32.Fs = true; break;
        case 0x65:  prefixes.Pre32.Gs = true; break;
        case 0x36:  prefixes.Pre32.Ss = true; break;
        default:
            if ( mode == Cpu_64 )
            {
                if ( (mem[i] >= 0x40) && (mem[i] <= 0x4F) )
                {
                    found = true;
                    prefixes.Pre64.Rex.Byte = mem[i];
                }
            }

            if ( !found )
                return i;
            break;
        }
    }

    return i;
}

static int GetModRmSize16( uint8_t modRm )
{
    int     instSize = 1;       // already includes modRm byte
    BYTE    mod = (modRm >> 6) & 3;
    BYTE    rm = (modRm & 7);

    // mod == 3 is only for single direct register values
    if ( mod != 3 )
    {
        if ( mod == 2 )
            instSize += 2;      // disp16
        else if ( mod == 1 )
            instSize += 1;      // disp8

        if ( (mod == 0) && (rm == 6) )
            instSize += 2;      // disp16
    }

    return instSize;
}

static int GetModRmSize32( uint8_t modRm )
{
    int     instSize = 1;       // already includes modRm byte
    BYTE    mod = (modRm >> 6) & 3;
    BYTE    rm = (modRm & 7);

    // mod == 3 is only for single direct register values
    if ( mod != 3 )
    {
        if ( rm == 4 )
            instSize += 1;      // SIB

        if ( mod == 2 )
            instSize += 4;      // disp32
        else if ( mod == 1 )
            instSize += 1;      // disp8

        if ( (mod == 0) && (rm == 5) )
            instSize += 4;      // disp32
    }

    return instSize;
}

InstructionType GetInstructionTypeAndSize( uint8_t* mem, int memLen, CpuSizeMode mode, int& size )
{
    _ASSERT( (mode == Cpu_32) || (mode == Cpu_64) );

    InstructionType type = Inst_Other;
    int             instSize = 0;
    int             remSize = 0;
    int             prefixSize = 0;
    Prefixes        prefixes = { 0 };

    if ( memLen > MAX_INSTRUCTION_SIZE )
        memLen = MAX_INSTRUCTION_SIZE;

    prefixSize = ReadPrefixes( mem, memLen, mode, prefixes );
    if ( prefixSize >= memLen )
        return Inst_None;

    remSize = memLen - prefixSize;

    switch ( mem[0] )
    {
    case 0xCC:
        instSize = 1;
        type = Inst_Breakpoint;
        break;

        // call instructions
    case 0xE8:
        if ( prefixes.Pre32.OperandSize && (mode == Cpu_32) )
            instSize = 3;
        else
            instSize = 5;

        if ( instSize > 0 )
            type = Inst_Call;
        break;

    case 0x9A:
        if ( mode == Cpu_32 )
        {
            if ( prefixes.Pre32.OperandSize )
                instSize = 5;
            else
                instSize = 7;
        }

        if ( instSize > 0 )
            type = Inst_Call;
        break;

        // call or jmp instructions
    case 0xFF:
        {
            if ( remSize < 2 )
                break;

            BYTE    regOp = (mem[1] >> 3) & 7;
            if ( (regOp == 2) || (regOp == 3) )
            {
                if ( (mode == Cpu_64) || !prefixes.Pre32.AddressSize )
                    instSize = 1 + GetModRmSize32( mem[1] );
                else
                    instSize = 1 + GetModRmSize16( mem[1] );

                if ( instSize > 0 )
                    type = Inst_Call;
            }
            else if ( (regOp == 4) || (regOp == 5) )
            {
                if ( (mode == Cpu_64) || !prefixes.Pre32.AddressSize )
                    instSize = 1 + GetModRmSize32( mem[1] );
                else
                    instSize = 1 + GetModRmSize16( mem[1] );

                type = Inst_Jmp;
            }
        }
        break;

        // jmp instructions
    case 0xEB:
        instSize = 2;
        type = Inst_Jmp;
        break;

    case 0xE9:
        if ( prefixes.Pre32.OperandSize && (mode == Cpu_32) )
            instSize = 3;
        else
            instSize = 5;
        type = Inst_Jmp;
        break;

    case 0xEA:
        if ( mode == Cpu_32 )
        {
            if ( prefixes.Pre32.OperandSize )
                instSize = 5;
            else
                instSize = 7;
            type = Inst_Jmp;
        }
        break;

        // system call instructions
    case 0x0F:
        if ( remSize < 2 )
            break;
        if ( (mem[1] == 0x05) || (mem[1] == 0x34) )
            instSize = 2;

        if ( instSize > 0 )
            type = Inst_Syscall;
        break;

    default:
        // rep prefixed instructions
        if ( remSize < 2 )
            break;

        if ( prefixes.Pre32.RepF2 )
        {
            if ( (mem[1] == 0xA6) || (mem[1] == 0xA7) || (mem[1] == 0xAE) || (mem[1] == 0xAF) )
                instSize = 2;
        }
        else if ( prefixes.Pre32.RepF3 )
        {
            if ( ((mem[1] >= 0x6C) && (mem[1] <= 0x6F))
                || ((mem[1] >= 0xA4) && (mem[1] <= 0xA7))
                || ((mem[1] >= 0xAA) && (mem[1] <= 0xAF)) )
                instSize = 2;
        }

        if ( instSize > 0 )
            type = Inst_RepString;
        break;
    }

    // sanity check, is it longer than available memory?
    if ( instSize > memLen )
        return Inst_None;

    size = instSize;

    return type;
}
