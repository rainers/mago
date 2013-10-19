/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


enum CpuSizeMode
{
    Cpu_32,
    Cpu_64,
};

enum InstructionType
{
    Inst_None,
    Inst_Other,
    Inst_Call,
    Inst_RepString,
    Inst_Jmp,
    Inst_Breakpoint,
    Inst_Syscall,
};


// IA-32 Intel Architecture: Software Developer’s Manual
// Volume 2A: Instruction Set Reference, A-M
// 2.2.1 REX Prefixes (p. 2-9)
const int   MAX_INSTRUCTION_SIZE = 15;


InstructionType GetInstructionTypeAndSize( uint8_t* mem, int memLen, CpuSizeMode mode, int& size );
