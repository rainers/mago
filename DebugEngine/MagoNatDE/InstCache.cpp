/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "InstCache.h"
#include "Program.h"
#include "IDebuggerProxy.h"
#include "ICoreProcess.h"

#include <algorithm>

namespace Mago
{
    Address64 InstBlock::Align( Address64 addr )
    {
        return (addr / BlockSize) * BlockSize;
    }

    Address64 InstBlock::GetLimit()
    {
        return Address + BlockSize;
    }

    bool InstBlock::Contains( Address64 addr )
    {
        return (addr >= Address) && ((addr - Address) < BlockSize);
    }


    //------------------------------------------------------------------------
    //  InstReader
    //------------------------------------------------------------------------


    InstReader::InstReader( 
        uint32_t blockCount, 
        InstBlock** blocks, 
        Address64 startAddr,
        Address64 endAddr,
        Address64 anchorAddr,
        int ptrSize,
        IDebugDisassemblyStream2* disasmStream )
        :   mBlockCount( blockCount ),
            mBlocks( blocks ),
            mStartAddr( startAddr ),
            mEndAddr( endAddr ),
            mCurAddr( startAddr ),
            mAnchorAddr( anchorAddr )
    {
        _ASSERT( (blockCount > 0) && (blockCount <= 2) );
        _ASSERT( blocks != NULL );
        _ASSERT( blocks[0] != NULL );
        _ASSERT( (blockCount == 1) || (blocks[1] != NULL) );
        _ASSERT( (blockCount == 1) || (blocks[1]->Address == blocks[0]->GetLimit()) );
        _ASSERT( endAddr >= startAddr );
        _ASSERT( blocks[0]->Contains( startAddr ) );
        _ASSERT( blocks[blockCount - 1]->Contains( endAddr ) 
            ||  (blocks[blockCount - 1]->GetLimit() == endAddr) );
        _ASSERT( ptrSize == 4 || ptrSize == 8 );

        ud_init( &mDisasm );
        ud_set_mode( &mDisasm, (uint8_t) (ptrSize * 8) );
        ud_set_syntax( &mDisasm, UD_SYN_INTEL );

        if ( disasmStream != NULL )
        {
            mDisasm.symbolizer = &Symbolize;
            mDisasm.sym_context = disasmStream;
        }
    }

    const ud_t* InstReader::GetDisasmData()
    {
        return &mDisasm;
    }

    // callback from udis86 to translate an address to a symbol
    int InstReader::Symbolize( ud_t* ud, uint64_t addr )
    {
        IDebugDisassemblyStream2* dds = (IDebugDisassemblyStream2*) ud->sym_context;
        CComPtr<IDebugCodeContext2> pCodeContext;
        HRESULT hr = dds->GetCodeContext( addr, &pCodeContext );
        if( FAILED( hr ) )
            return 0;

        CONTEXT_INFO info = { 0 };
        hr = pCodeContext->GetInfo( CIF_FUNCTION | CIF_ADDRESSOFFSET, &info );
        if( FAILED( hr ) || info.bstrFunction == NULL )
            return 0;

        ud->insn_fill += wcstombs( ud->insn_buffer + ud->insn_fill, info.bstrFunction, 100 );
        if( info.dwFields & CIF_ADDRESSOFFSET )
            ud->insn_fill += wcstombs( ud->insn_buffer + ud->insn_fill, info.bstrAddressOffset, 30 );
        ud->insn_fill += sprintf( (char*) ud->insn_buffer + ud->insn_fill, " (0x%I64x)", addr );
            
        SysFreeString( info.bstrFunction );
        if( info.bstrAddressOffset != NULL )
            SysFreeString( info.bstrAddressOffset );

        return 1;
    }

    // If the last disassembled instruction crosses the anchor, then the last 
    // instruction is set to invalid and its length is trimmed up to the anchor.
    // Returns the number of bytes disassembled.

    uint32_t InstReader::TruncateBeforeAnchor()
    {
        uint32_t instLen = ud_insn_len( &mDisasm );
        Address64 limit = mCurAddr + instLen;

        if ( mCurAddr < mAnchorAddr && limit > mAnchorAddr )
        {
            mDisasm.mnemonic = UD_Iinvalid;
            instLen = (uint32_t) (mAnchorAddr - mCurAddr);
        }

        return instLen;
    }

    uint32_t InstReader::Decode()
    {
        uint32_t    instBufLen = 0;
        BYTE*       instBuf = NULL;
        uint32_t    instLen = 0;

        instBuf = GetInstBuffer( instBufLen );
        ud_set_input_buffer( &mDisasm, instBuf, instBufLen );

        instLen = ud_decode( &mDisasm );
        instLen = TruncateBeforeAnchor();
        mCurAddr += instLen;

        return instLen;
    }

    uint32_t InstReader::Disassemble()
    {
        uint32_t    instBufLen = 0;
        BYTE*       instBuf = NULL;
        uint32_t    instLen = 0;

        instBuf = GetInstBuffer( instBufLen );
        ud_set_input_buffer( &mDisasm, instBuf, instBufLen );

        instLen = ud_disassemble( &mDisasm );
        instLen = TruncateBeforeAnchor();
        mCurAddr += instLen;

        return instLen;
    }

    uint32_t InstReader::Disassemble( Address64 curPC, bool symOps )
    {
        ud_set_pc( &mDisasm, curPC );

        if( mDisasm.sym_context )
            mDisasm.symbolizer = symOps ? &Symbolize : NULL;

        return Disassemble();
    }

    bool InstReader::ReadByte( BYTE& dataByte )
    {
        uint32_t    instBufLen = 0;
        BYTE*       instBuf = NULL;

        instBuf = GetInstBuffer( instBufLen );

        if ( (instBuf == NULL) || (instBufLen == 0) )
            return false;

        dataByte = instBuf[0];
        mCurAddr++;

        return true;
    }

    void InstReader::SetPC( Address64 pc )
    {
        ud_set_pc( &mDisasm, pc );
    }

    BYTE* InstReader::GetInstBuffer( uint32_t& length )
    {
        if ( mCurAddr >= mEndAddr )
        {
            length = 0;
            return NULL;
        }

        uint32_t    maxInstLen = (uint32_t) (mEndAddr - mCurAddr);
        uint32_t    virtPos = (uint32_t) (mCurAddr - mBlocks[0]->Address);
        uint32_t    pos = virtPos % InstBlock::BlockSize;
        BYTE*       buf = NULL;

        if ( maxInstLen > MaxInstructionSize )
            maxInstLen = MaxInstructionSize;

        if ( (mCurAddr + maxInstLen) <= mBlocks[0]->GetLimit() )
        {
            buf = &mBlocks[0]->Inst[pos];
        }
        else if ( mCurAddr >= mBlocks[0]->GetLimit() )
        {
            buf = &mBlocks[1]->Inst[pos];
        }
        else
        {
            size_t  sizeOnLeft = (size_t) (mBlocks[0]->GetLimit() - mCurAddr);
            size_t  sizeOnRight = maxInstLen - sizeOnLeft;

            memcpy( &mInstBuf[0], &mBlocks[0]->Inst[pos], sizeOnLeft );
            memcpy( &mInstBuf[sizeOnLeft], &mBlocks[1]->Inst[0], sizeOnRight );

            buf = mInstBuf;
        }

        length = maxInstLen;
        return buf;
    }


    //------------------------------------------------------------------------
    //  InstCache
    //------------------------------------------------------------------------


    InstCache::InstCache()
        :   mDebugger( NULL ),
            mAnchorAddr( 0 ),
            mPtrSize( 0 )
    {
    }

    HRESULT InstCache::Init( Program* program, IDebuggerProxy* debugger, int ptrSize )
    {
        _ASSERT( program != NULL );
        _ASSERT( debugger != NULL );

        for ( int i = 0; i < _countof( mBlockCache ); i++ )
        {
            mBlockCache[i].reset( new InstBlock() );
            if ( mBlockCache[i].get() == NULL )
                return E_OUTOFMEMORY;

            // mark it as unused
            mBlockCache[i]->Address = 0;
            mBlockCache[i]->State = BlockState_Invalid;
        }

        mPtrSize = ptrSize;
        mProg = program;
        mDebugger = debugger;

        return S_OK;
    }

    void InstCache::SetAnchor( Address64 anchorAddr )
    {
        mAnchorAddr = anchorAddr;
    }

    HRESULT InstCache::LoadBlocks( Address64 addr, int instAway, int& instAwayAvail )
    {
        Address64 anchorBase = InstBlock::Align( addr );
        Address64 leftBase = anchorBase - InstBlock::BlockSize;
        Address64 rightBase = anchorBase + InstBlock::BlockSize;
        Address64 rightLimit = rightBase + InstBlock::BlockSize;
        Address64 sideBase = 0;

        bool    anchorFound = false;
        bool    sideFound = false;
        int     anchorIndex = -1;
        int     sideIndex = -1;

        if ( (leftBase < anchorBase) 
            && (instAway < ((intptr_t) (anchorBase - addr) / MaxInstructionSize)) )
        {
            sideBase = leftBase;
        }
        else if ( (rightLimit > anchorBase) 
            && (instAway > ((intptr_t) (rightBase - addr) / MaxInstructionSize)) )
        {
            sideBase = rightBase;
        }

        // now we know if user wants instructions that span two pages

        FindBlocks( anchorBase, sideBase, anchorFound, sideFound, anchorIndex, sideIndex );

        // load the instruction data

        if ( !anchorFound )
        {
            ReadInstData( anchorBase, anchorIndex );
        }

        if ( (sideBase != 0) && !sideFound )
        {
            ReadInstData( sideBase, sideIndex );
        }

        // calculate the maximum number of instructions we can get

        Address64 baseOnLeft = anchorBase;
        Address64 limitOnRight = rightBase;

        if ( sideFound || (sideBase != 0) )
        {
            if ( sideBase < anchorBase )
                baseOnLeft = leftBase;
            else
                limitOnRight = rightLimit;
        }

        if ( instAway < 0 )
        {
            instAwayAvail = (intptr_t) (baseOnLeft - addr) / MaxInstructionSize;
            instAwayAvail = std::max( instAwayAvail, instAway );
        }
        else
        {
            instAwayAvail = (intptr_t) (limitOnRight - addr) / MaxInstructionSize;
            instAwayAvail = std::min( instAwayAvail, instAway );
        }

        // map data

        if ( !anchorFound || ((sideBase != 0) && !sideFound) )
        {
            MapInstData( addr );
        }

        return S_OK;
    }

    // Builds the instruction map for the blocks around the given address.

    void InstCache::MapInstData( Address64 anchorAddr )
    {
        InstBlock*  anchorBlock = GetBlockContaining( anchorAddr );
        InstBlock*  leftBlock = GetBlockContaining( anchorAddr - InstBlock::BlockSize );
        InstBlock*  rightBlock = GetBlockContaining( anchorAddr + InstBlock::BlockSize );
        InstBlock*  sideBlock = NULL;
        InstBlock*  blocks[2] = { NULL };

        if ( anchorBlock == NULL )
            return;
        if ( (leftBlock != NULL) && (leftBlock->Address >= anchorBlock->Address) )
            leftBlock = NULL;
        if ( (rightBlock != NULL) && (rightBlock->Address <= anchorBlock->Address) )
            rightBlock = NULL;

        if ( leftBlock != NULL )
        {
            sideBlock = leftBlock;
            blocks[0] = leftBlock;
            blocks[1] = anchorBlock;
        }
        else if ( rightBlock != NULL )
        {
            sideBlock = rightBlock;
            blocks[0] = anchorBlock;
            blocks[1] = rightBlock;
        }

        if ( (sideBlock != NULL) 
            && (sideBlock->State == BlockState_Loaded) 
            && (anchorBlock->State == BlockState_Mapped) )
        {
            // do a partial map up to or from the anchor address
            if ( leftBlock != NULL )
                MapInstData( 2, blocks, leftBlock->Address, anchorAddr );
            else
                MapInstData( 2, blocks, anchorAddr, rightBlock->GetLimit() );
        }
        else if ( (anchorBlock->State == BlockState_Loaded)
            && (sideBlock != NULL)
            && (sideBlock->State != BlockState_Invalid) )
        {
            // do 2 whole maps in 2 runs
            if ( leftBlock != NULL )
            {
                MapInstData( 2, blocks, leftBlock->Address, anchorAddr );
                MapInstData( 1, &anchorBlock, anchorAddr, anchorBlock->GetLimit() );
            }
            else
            {
                MapInstData( 1, &anchorBlock, anchorBlock->Address, anchorAddr );
                MapInstData( 2, blocks, anchorAddr, rightBlock->GetLimit() );
            }
        }
        else if ( anchorBlock->State == BlockState_Loaded )
        {
            // do 1 whole map in 1 run
            blocks[0] = anchorBlock;
            MapInstData( 1, blocks, anchorBlock->Address, anchorBlock->GetLimit() );
        }
        else if ( (sideBlock != NULL) && (sideBlock->State == BlockState_Loaded) )
        {
            // do 1 whole map in 1 run
            blocks[0] = sideBlock;
            MapInstData( 1, blocks, sideBlock->Address, sideBlock->GetLimit() );
        }
    }

    void InstCache::MapInstData( 
        uint32_t blockCount, InstBlock** blocks, Address64 startAddr, Address64 endAddr )
    {
        // TODO: assert params

        InstReader  reader( blockCount, blocks, startAddr, endAddr, mAnchorAddr, mPtrSize, NULL );
        uint32_t    instLen = 0;
        uint32_t    virtPos = (uint32_t) (startAddr - blocks[0]->Address);

        if ( blockCount == 1 )
        {
            uint32_t    size = (uint32_t) (endAddr - startAddr);

            memset( &blocks[0]->Map[virtPos], 0, size );
        }
        else
        {
            _ASSERT( blockCount == 2 );
            uint32_t    size = (uint32_t) (blocks[1]->Address - startAddr);

            memset( &blocks[0]->Map[virtPos], 0, size );

            size = (uint32_t) (endAddr - blocks[1]->Address);

            memset( &blocks[1]->Map[0], 0, size );
        }

        for ( uint32_t i = 0; i < blockCount; i++ )
            blocks[i]->State = BlockState_Mapped;

        for ( instLen = reader.Decode(); instLen != 0; instLen = reader.Decode() )
        {
            const ud_t* ud = reader.GetDisasmData();
            uint32_t    pos = virtPos % InstBlock::BlockSize;

            if ( ud->mnemonic != UD_Iinvalid )
            {
                if ( virtPos < InstBlock::BlockSize )
                    blocks[0]->Map[ pos ] = (BYTE) instLen;
                else if ( blocks[1] != NULL )
                    blocks[1]->Map[ pos ] = (BYTE) instLen;

                virtPos += instLen;
            }
            else
            {
                for ( uint32_t i = 0; i < instLen; i++ )
                {
                    pos = virtPos % InstBlock::BlockSize;

                    if ( virtPos < InstBlock::BlockSize )
                        blocks[0]->Map[ pos ] = 1;
                    else if ( blocks[1] != NULL )
                        blocks[1]->Map[ pos ] = 1;

                    virtPos++;
                }
            }
        }
    }

    void InstCache::FindBlocks( 
        Address64 anchorBase, 
        Address64 sideBase, 
        bool& anchorFound, 
        bool& sideFound, 
        int& anchorIndex,
        int& sideIndex )
    {
        _ASSERT( anchorBase != 0 );
        _ASSERT( sideBase != anchorBase );

        anchorIndex = GetBlockIndexContaining( anchorBase );

        if ( sideBase != 0 )
            sideIndex = GetBlockIndexContaining( sideBase );
        else
        {
            sideIndex = -1;

            // try left first
            sideBase = anchorBase - InstBlock::BlockSize;
            if ( sideBase < anchorBase )
                sideIndex = GetBlockIndexContaining( sideBase );

            if ( sideIndex < 0 )
            {
                // then right
                sideBase = anchorBase + InstBlock::BlockSize;
                if ( sideBase > anchorBase )
                    sideIndex = GetBlockIndexContaining( sideBase );
            }
        }

        anchorFound = anchorIndex >= 0;
        sideFound = sideIndex >= 0;

        if ( !anchorFound && !sideFound )
        {
            anchorIndex = 0;
            sideIndex = 1;
        }
        else if ( !anchorFound )
        {
            anchorIndex = (sideIndex + 1) % 2;
        }
        else if ( !sideFound )
        {
            sideIndex = (anchorIndex + 1) % 2;
        }
        // else, both were found, and the indexes were set already
    }

    int InstCache::GetBlockIndexContaining( Address64 addr )
    {
        for ( int i = 0; i < _countof( mBlockCache ); i++ )
        {
            if ( mBlockCache[i]->Contains( addr ) )
                return i;
        }

        return -1;
    }

    InstBlock* InstCache::GetBlockContaining( Address64 addr )
    {
        int i = GetBlockIndexContaining( addr );
        _ASSERT( i < (int) _countof( mBlockCache ) );

        if ( i < 0 )
            return NULL;

        return mBlockCache[i].get();
    }

    HRESULT InstCache::ReadInstData( Address64 baseAddr, int cacheIndex )
    {
        _ASSERT( (cacheIndex >= 0) && (cacheIndex < _countof( mBlockCache )) );
        _ASSERT( InstBlock::Align( baseAddr ) == baseAddr );

        HRESULT                 hr = S_OK;
        RefPtr<ICoreProcess>    proc;
        uint32_t                lenRead = 0;
        uint32_t                lenUnreadable = 0;

        mBlockCache[cacheIndex]->Address = baseAddr;
        mBlockCache[cacheIndex]->State = BlockState_Invalid;

        proc = mProg->GetCoreProcess();

        hr = mDebugger->ReadMemory( 
            proc, 
            baseAddr, 
            InstBlock::BlockSize, 
            lenRead, 
            lenUnreadable, 
            mBlockCache[cacheIndex]->Inst );
        if ( FAILED( hr ) )
            return hr;
        if ( lenRead != InstBlock::BlockSize )
            return E_FAIL;

        mBlockCache[cacheIndex]->State = BlockState_Loaded;

        return S_OK;
    }
}
