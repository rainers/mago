/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DisassemblyStream.h"
#include "Program.h"
#include "Module.h"
#include "CodeContext.h"
#include <udis86.h>


namespace Mago
{
    DisassemblyStream::DisassemblyStream()
        :   mScope( 0 ),
            mAnchorAddr( 0 ),
            mReadAddr( 0 ),
            mInvalidInstLenAtReadPtr( 0 )
    {
    }

    DisassemblyStream::~DisassemblyStream()
    {
    }


    //////////////////////////////////////////////////////////// 
    // IDebugDisassemblyStream2 

    void DisassemblyStream::FillDataByteDisasmData( 
        BYTE dataByte, 
        DISASSEMBLY_STREAM_FIELDS dwFields, 
        DisassemblyData* pDisassembly )
    {
        _ASSERT( pDisassembly != NULL );

        if ( (dwFields & DSF_ADDRESS) != 0 )
        {
            wchar_t addrStr[20 + 1] = L"";

            swprintf_s( addrStr, L"%08X", mReadAddr );
            pDisassembly->bstrAddress = SysAllocString( addrStr );
            pDisassembly->dwFields |= DSF_ADDRESS;
        }

        if ( (dwFields & DSF_CODELOCATIONID) != 0 )
        {
            pDisassembly->uCodeLocationId = mReadAddr;
            pDisassembly->dwFields |= DSF_CODELOCATIONID;
        }

        if ( (dwFields & DSF_OPCODE) != 0 )
        {
            wchar_t opcodeStr[20 + 1] = L"";

            swprintf_s( opcodeStr, L"db 0x%02X", dataByte );
            pDisassembly->bstrOpcode = SysAllocString( opcodeStr );
            pDisassembly->dwFields |= DSF_OPCODE;
        }
    }

    void DisassemblyStream::FillInstDisasmData( 
        const ud_t* ud, 
        DISASSEMBLY_STREAM_FIELDS dwFields, 
        DisassemblyData* pDisassembly )
    {
        _ASSERT( (ud != NULL) && (pDisassembly != NULL) );

        if ( (dwFields & DSF_ADDRESS) != 0 )
        {
            wchar_t addrStr[20 + 1] = L"";

            swprintf_s( addrStr, L"%08X", mReadAddr );
            pDisassembly->bstrAddress = SysAllocString( addrStr );
            pDisassembly->dwFields |= DSF_ADDRESS;
        }

        if ( (dwFields & DSF_CODELOCATIONID) != 0 )
        {
            pDisassembly->uCodeLocationId = mReadAddr;
            pDisassembly->dwFields |= DSF_CODELOCATIONID;
        }

        if ( (dwFields & DSF_OPCODE) != 0 )
        {
            wchar_t opcodeStr[200 + 1] = L"";

            MultiByteToWideChar(
                CP_ACP,
                0,
                // this function should take a const ud_t*
                ud_insn_asm( (ud_t*) ud ),
                -1,
                opcodeStr,
                _countof( opcodeStr ) );

            pDisassembly->bstrOpcode = SysAllocString( opcodeStr );
            pDisassembly->dwFields |= DSF_OPCODE;
        }
    }

    HRESULT DisassemblyStream::Read( 
        DWORD                     dwInstructions,
        DISASSEMBLY_STREAM_FIELDS dwFields,
        DWORD*                    pdwInstructionsRead,
        DisassemblyData*          prgDisassembly )
    {
        if ( (pdwInstructionsRead == NULL) || (prgDisassembly == NULL) )
            return E_INVALIDARG;

        {
            char    msg[500] = "";
            sprintf_s( msg, "Read began: anchor=%08x read=%08x iInst=%d\n", 
                mAnchorAddr, mReadAddr, dwInstructions );
            OutputDebugStringA( msg );
        }

        HRESULT     hr = S_OK;
        int         instWanted = std::min<DWORD>( dwInstructions, INT_MAX );
        int         instToFind = 0;
        int         instFound = 0;

        hr = mInstCache.LoadBlocks( mReadAddr, instWanted, instToFind );
        if ( FAILED( hr ) )
            return hr;

        InstBlock*  block = mInstCache.GetBlockContaining( mReadAddr );
        // nextBlock might be NULL
        InstBlock*  nextBlock = mInstCache.GetBlockContaining( block->GetLimit() );
        InstBlock*  blocks[2] = { block, nextBlock };
        uint32_t    blockCount = (nextBlock == NULL ? 1 : 2);
        Address     endAddr = (nextBlock == NULL ? block->GetLimit() : nextBlock->GetLimit());
        InstReader  reader( blockCount, blocks, mReadAddr, endAddr );
        uint32_t    instLen = 0;

        // we might have already determined there were invalid instructions there
        while ( (instFound < instToFind) && (mInvalidInstLenAtReadPtr > 0) )
        {
            BYTE    dataByte = 0;

            if ( !reader.ReadByte( dataByte ) )
                break;

            FillDataByteDisasmData(
                dataByte,
                dwFields,
                &prgDisassembly[instFound] );

            mInvalidInstLenAtReadPtr--;
            instFound++;
            mReadAddr++;
        }

        for ( instLen = reader.Disassemble( mReadAddr ); 
            (instLen != 0) && (instFound < instToFind); 
            instLen = reader.Disassemble( mReadAddr ) )
        {
            const ud_t* ud = reader.GetDisasmData();

            if ( ud->mnemonic != UD_Iinvalid )
            {
                FillInstDisasmData( ud, dwFields, &prgDisassembly[instFound] );

                instFound++;
                mReadAddr += instLen;
            }
            else
            {
                // this function should take a const ud_t*
                const BYTE* buf = ud_insn_ptr( (ud_t*) ud );
                uint32_t    countToHandle = 0;

                if ( instLen <= (uint32_t) (instToFind - instFound) )
                    countToHandle = instLen;
                else
                {
                    countToHandle = instToFind - instFound;
                    mInvalidInstLenAtReadPtr = instLen - countToHandle;
                }

                for ( uint32_t i = 0; i < countToHandle; i++ )
                {
                    BYTE    dataByte = buf[i];

                    FillDataByteDisasmData( 
                        dataByte, 
                        dwFields, 
                        &prgDisassembly[instFound] );

                    instFound++;
                    mReadAddr++;
                }
            }
        }

        *pdwInstructionsRead = instFound;

        {
            char    msg[500] = "";
            sprintf_s( msg, "Read ended: anchor=%08x read=%08x found=%d\n", 
                mAnchorAddr, mReadAddr, instFound );
            OutputDebugStringA( msg );
        }

        return S_OK;
    }

    HRESULT DisassemblyStream::Seek( 
        SEEK_START          dwSeekStart,
        IDebugCodeContext2* pCodeContext,
        UINT64              uCodeLocationId,
        INT64               iInstructions )
    {
        switch ( dwSeekStart )
        {
        case SEEK_START_CURRENT:
            // we don't need to change the anchor
            break;

        case SEEK_START_CODECONTEXT:
            {
                Address                         newAnchor = 0;
                CComQIPtr<IMagoMemoryContext>   magoMem = pCodeContext;

                if ( magoMem == NULL )
                    return E_INVALIDARG;

                magoMem->GetAddress( newAnchor );

                mAnchorAddr = newAnchor;
            }
            break;

        case SEEK_START_CODELOCID:
            if ( (Address) uCodeLocationId != uCodeLocationId )
                return E_INVALIDARG;

            mAnchorAddr = (Address) uCodeLocationId;
            break;

        case SEEK_START_BEGIN:
        case SEEK_START_END:
            return E_NOTIMPL;
        }

        {
            char    msg[500] = "";
            sprintf_s( msg, "Seek: anchor=%08x iInst=%I64d\n", 
                mAnchorAddr, iInstructions );
            OutputDebugStringA( msg );
        }

        return SeekOffset( iInstructions );
    }

    HRESULT DisassemblyStream::GetCodeLocationId( 
        IDebugCodeContext2* pCodeContext,
        UINT64*             puCodeLocationId )
    {
        if ( (pCodeContext == NULL) || (puCodeLocationId == NULL) )
            return E_INVALIDARG;

        Address addr = 0;
        CComQIPtr<IMagoMemoryContext>   magoMem = pCodeContext;

        if ( magoMem == NULL )
            return E_INVALIDARG;

        magoMem->GetAddress( addr );
        
        *puCodeLocationId = addr;
        return S_OK;
    }

    HRESULT DisassemblyStream::GetCodeContext( 
        UINT64               uCodeLocationId,
        IDebugCodeContext2** ppCodeContext )
    {
        {
            char    msg[500] = "";
            sprintf_s( msg, "DisassemblyStream::GetCodeContext: %08x\n", (UINT32) uCodeLocationId );
            OutputDebugStringA( msg );
        }

        return E_NOTIMPL;
    }

    HRESULT DisassemblyStream::GetCurrentLocation( 
        UINT64* puCodeLocationId )
    {
        if ( puCodeLocationId == NULL )
            return E_INVALIDARG;

        *puCodeLocationId = mReadAddr;

        {
            char    msg[500] = "";
            sprintf_s( msg, "GetCurrentLocation: anchor=%08x read=%08x\n", 
                mAnchorAddr, mReadAddr );
            OutputDebugStringA( msg );
        }

        return S_OK;
    }

    HRESULT DisassemblyStream::GetDocument( 
        BSTR              bstrDocumentUrl,
        IDebugDocument2** ppDocument )
    {
        return E_NOTIMPL;
    }

    HRESULT DisassemblyStream::GetScope( 
        DISASSEMBLY_STREAM_SCOPE* pdwScope )
    {
        if ( pdwScope == NULL )
            return E_INVALIDARG;

        *pdwScope = mScope;
        return S_OK;
    }

    HRESULT DisassemblyStream::GetSize( 
        UINT64* pnSize )
    {
        if ( pnSize == NULL )
            return E_INVALIDARG;

        *pnSize = std::numeric_limits<uint64_t>::max();
        return S_OK;
    }


    //////////////////////////////////////////////////////////// 
    // DisassemblyStream

    HRESULT DisassemblyStream::Init( 
        DISASSEMBLY_STREAM_SCOPE disasmScope, 
        Address address, 
        Program* program, 
        DebuggerProxy* debugger )
    {
        HRESULT hr = S_OK;

        hr = mInstCache.Init( program, debugger );
        if ( FAILED( hr ) )
            return hr;

        mScope = disasmScope;
        mAnchorAddr = address;
        mReadAddr = address;

        return S_OK;
    }

    HRESULT DisassemblyStream::SeekOffset( INT64 iInstructions )
    {
        HRESULT     hr = S_OK;
        InstBlock*  block = NULL;
        int         instWanted = (int) iInstructions;   // we'll test below
        int         instAvail = 0;

        if ( iInstructions < INT_MIN )
            instWanted = INT_MIN;
        else if ( iInstructions > INT_MAX )
            instWanted = INT_MAX;

        // when seeking to a new place, assume we won't end up at an invalid instruction
        mInvalidInstLenAtReadPtr = 0;
        mReadAddr = mAnchorAddr;

        hr = mInstCache.LoadBlocks( mAnchorAddr, instWanted, instAvail );
        if ( FAILED( hr ) )
            return hr;

        block = mInstCache.GetBlockContaining( mAnchorAddr );
        // we already loaded it, so there should be no reason for it to fail
        if ( block == NULL )
            return E_FAIL;

        if ( instAvail < 0 )
            return SeekBack( instAvail, block );
        else if ( instAvail > 0 )
            return SeekForward( instAvail, block );

        return S_OK;
    }

    HRESULT DisassemblyStream::SeekBack( int iInstructions, InstBlock* block )
    {
        _ASSERT( block != NULL );
        _ASSERT( iInstructions < 0 );

        int         instToFind = iInstructions;
        int         instFound = 0;
        int32_t     virtAnchorOffset = 0;
        int32_t     virtPos = 0;
        Address     baseAddr = 0;

        baseAddr = block->Address - InstBlock::BlockSize;
        virtAnchorOffset = InstBlock::BlockSize + mAnchorAddr - block->Address;
        virtPos = virtAnchorOffset;

        // we don't want it going all the way to the end of the range
        if ( instToFind == INT_MIN )
            instToFind++;

        // now to actually make it make sense (no negative counts)
        instToFind = -instToFind;

        // go back one byte at a time until you reach the beginning of the block
        // or the beginning of an instruction

        for ( ; (virtPos > 0) && (instFound < instToFind); virtPos-- )
        {
            if ( virtPos == InstBlock::BlockSize )
            {
                block = mInstCache.GetBlockContaining( baseAddr );
                if ( block == NULL )
                    break;
            }

            int32_t pos = virtPos % InstBlock::BlockSize;
            BYTE    instLen = block->Map[pos - 1];

            if ( instLen == 0 )
                continue;

            // Found an instruction. Is it a whole one, or do we need to 
            // break it up into data bytes?

            if ( ((virtPos - 1) + instLen) <= virtAnchorOffset )
            {
                instFound++;
                mReadAddr -= instLen;
            }
            else
            {
                BYTE    byteCount = (BYTE) (virtAnchorOffset - virtPos + 1);

                if ( byteCount <= (instToFind - instFound) )
                {
                    mReadAddr -= byteCount;
                    instFound += byteCount;
                }
                else
                {
                    mReadAddr -= (instToFind - instFound);
                    instFound = instToFind;
                }
            }
        }

        {
            char    msg[500] = "";
            sprintf_s( msg, "SeekBack ended: anchor=%08x read=%08x iInst=%d found=%d\n", 
                mAnchorAddr, mReadAddr, iInstructions, instFound );
            OutputDebugStringA( msg );
        }

        return S_OK;
    }

    HRESULT DisassemblyStream::SeekForward( int iInstructions, InstBlock* block )
    {
        _ASSERT( block != NULL );
        _ASSERT( iInstructions > 0 );

        int         instToFind = iInstructions;
        int         instFound = 0;

        // nextBlock might be NULL
        InstBlock*  nextBlock = mInstCache.GetBlockContaining( block->GetLimit() );
        InstBlock*  blocks[2] = { block, nextBlock };
        uint32_t    blockCount = (nextBlock == NULL ? 1 : 2);
        Address     endAddr = (nextBlock == NULL ? block->GetLimit() : nextBlock->GetLimit());
        InstReader  reader( blockCount, blocks, mAnchorAddr, endAddr );
        uint32_t    instLen = 0;

        for ( instLen = reader.Decode(); 
            (instLen != 0) && (instFound < instToFind); 
            instLen = reader.Decode() )
        {
            const ud_t* ud = reader.GetDisasmData();

            if ( ud->mnemonic != UD_Iinvalid )
            {
                instFound++;
                mReadAddr += instLen;
            }
            else
            {
                if ( instLen <= (uint32_t) (instToFind - instFound) )
                {
                    mReadAddr += instLen;
                    instFound += instLen;
                }
                else
                {
                    mReadAddr += instToFind - instFound;
                    mInvalidInstLenAtReadPtr = instLen - (instToFind - instFound);
                    instFound = instToFind;
                }
            }
        }

        {
            char    msg[500] = "";
            sprintf_s( msg, "SeekForward ended: anchor=%08x read=%08x iInst=%d found=%d\n", 
                mAnchorAddr, mReadAddr, iInstructions, instFound );
            OutputDebugStringA( msg );
        }

        return S_OK;
    }
}
