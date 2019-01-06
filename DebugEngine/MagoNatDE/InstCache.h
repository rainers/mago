/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <udis86.h>
#include <memory>


namespace Mago
{
    const int MaxInstructionSize = 15;


    class Program;
    class IDebuggerProxy;


    enum BlockState
    {
        BlockState_Invalid,
        BlockState_Loaded,
        BlockState_Mapped,
    };


    // Inst holds a sequence of instructions
    // Map holds instruction length and location information
    //  For each byte that starts an instruction in Inst, there's a corresponding
    //  byte at the same offset in Map set to the length of the instruction.
    //  All other bytes are zero.

    struct InstBlock
    {
        static const uint32_t  BlockSize = 4096;

        Address64   Address;
        BlockState  State;
        BYTE        Inst[ BlockSize ];
        BYTE        Map[ BlockSize ];

        Address64 GetLimit();
        bool Contains( Address64 addr );

        static Address64 Align( Address64 addr );
    };


    class InstReader
    {
        ud_t            mDisasm;
        uint32_t        mBlockCount;
        InstBlock**     mBlocks;
        Address64       mStartAddr;
        Address64       mEndAddr;
        Address64       mCurAddr;
        Address64       mAnchorAddr;
        IDebugDisassemblyStream2* mDisasmStream;

        BYTE            mInstBuf[ MaxInstructionSize ];
        std::string     mSymbolizeBuf;

    public:
        InstReader( uint32_t blockCount, InstBlock** blocks, Address64 startAddr, Address64 endAddr, 
            Address64 anchorAddr, int ptrSize, IDebugDisassemblyStream2* disasmStream );

        const ud_t* GetDisasmData();
        uint32_t Decode();
        uint32_t Disassemble();
        uint32_t Disassemble( Address64 curPC, bool symOps );
        bool ReadByte( BYTE& dataByte );
        void SetPC( Address64 pc );

    private:
        BYTE* GetInstBuffer( uint32_t& length );
        uint32_t TruncateBeforeAnchor();

        static const char* Symbolize( ud_t* ud, uint64_t addr, int64_t *offset );
    };


    class InstCache
    {
        RefPtr<Program>             mProg;
        IDebuggerProxy*             mDebugger;
        Address64                   mAnchorAddr;
        std::unique_ptr<InstBlock>  mBlockCache[2];
        uint32_t                    mPtrSize;

    public:
        InstCache();

        HRESULT Init( Program* program, IDebuggerProxy* debugger, int ptrSize );
        void SetAnchor( Address64 anchorAddr );

        HRESULT LoadBlocks( Address64 addr, int instAway, int& instAwayAvail );

        InstBlock* GetBlockContaining( Address64 addr );

    private:
        // Looks in the cache for an anchor block and a side block right next to it.
        // If a side block is not given (its base is 0), then this function 
        // looks for whatever block is supposed to be right next to it on either side.
        //
        // Returns whether the anchor block and a side block were found and what 
        // their cache indexes are. The indexes will always be valid. Each will
        // be set to either the index where the block was found or where the 
        // block should be put, if it wasn't found.

        void FindBlocks( 
            Address64 anchorBase, 
            Address64 sideBase, 
            bool& anchorFound, 
            bool& sideFound, 
            int& anchorIndex,
            int& sideIndex );

        // Returns the cache index of the block containing the given address, 
        // or -1 if not found.

        int GetBlockIndexContaining( Address64 addr );

        // Reads a block of instruction data into the given cache index.
        // On success, it marks the block as loaded, otherwise as invalid.

        HRESULT ReadInstData( Address64 baseAddr, int cacheIndex );

        void MapInstData( Address64 anchorAddr );
        void MapInstData( uint32_t blockCount, InstBlock** blocks, Address64 startAddr, Address64 endAddr );
    };
}
