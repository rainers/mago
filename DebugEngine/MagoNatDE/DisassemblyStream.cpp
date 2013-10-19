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
#include "SingleDocumentContext.h"
#include <udis86.h>


namespace Mago
{
    DisassemblyStream::DisassemblyStream()
        :   mScope( 0 ),
            mAnchorAddr( 0 ),
            mReadAddr( 0 ),
            mInvalidInstLenAtReadPtr( 0 ),
            mStartOfRead( false )
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

        if ( (dwFields & DSF_CODEBYTES) != 0 )
        {
            uint32_t        len = ud_insn_len( (ud_t*) ud );
            const uint8_t*  bytes = ud_insn_ptr( (ud_t*) ud );
            CComBSTR        hexChars( len * 3 );            // 2 hex chars + 1 space
            wchar_t*        pchar = hexChars;
            wchar_t*        pcharEnd = pchar + hexChars.Length();

            for ( uint32_t i = 0; i < len; i++, pchar += 3 )
            {
                swprintf( pchar, pcharEnd - pchar, L"%02X ", bytes[i] );
            }

            pDisassembly->bstrCodeBytes = hexChars.Detach();
            pDisassembly->dwFields |= DSF_CODEBYTES;
        }

        if ( (dwFields & DSF_DOCUMENTURL) != 0 )
        {
            // we want to return a filename on a document change except if 
            // we're at the beginning of a block

            if ( mStartOfRead || (mDocInfo.HasLineInfo() && mDocInfo.DocChanged()) )
            {
                pDisassembly->bstrDocumentUrl = mDocInfo.GetFilename();
                if ( pDisassembly->bstrDocumentUrl != NULL )
                    pDisassembly->dwFields |= DSF_DOCUMENTURL;
            }
        }

        if ( (dwFields & DSF_POSITION) != 0 )
        {
            if ( mDocInfo.HasLineInfo() )
            {
                // only need to set the line info if at the beginning of an instruction
                if ( mDocInfo.GetByteOffset() == 0 )
                {
                    // no source code is shown if the end column is zero
                    // since there's no column data, span all columns

                    pDisassembly->posBeg.dwLine = mDocInfo.GetLine() - 1;
                    pDisassembly->posBeg.dwColumn = 0;
                    pDisassembly->posEnd.dwLine = mDocInfo.GetLineEnd() - 1;
                    pDisassembly->posEnd.dwColumn = 0xFFFFFFFF;
                    pDisassembly->dwFields |= DSF_POSITION;
                }
            }
        }

        if ( (dwFields & DSF_BYTEOFFSET) != 0 )
        {
            if ( mDocInfo.HasLineInfo() )
            {
                pDisassembly->dwByteOffset = mDocInfo.GetByteOffset();
                pDisassembly->dwFields |= DSF_BYTEOFFSET;
            }
        }

        if ( (dwFields & DSF_FLAGS) != 0 )
        {
            pDisassembly->dwFlags = 0;
            if ( mDocInfo.HasLineInfo() )
                pDisassembly->dwFlags |= DF_HASSOURCE;
            if ( mDocInfo.DocChanged() )
                pDisassembly->dwFlags |= DF_DOCUMENTCHANGE;
            pDisassembly->dwFields |= DSF_FLAGS;
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

        _RPT3( _CRT_WARN, "Read began: anchor=%08x read=%08x iInst=%d\n", 
            mAnchorAddr, mReadAddr, dwInstructions );

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
        InstReader  reader( blockCount, blocks, mReadAddr, endAddr, mAnchorAddr );
        uint32_t    instLen = 0;

        // we might have already determined there were invalid instructions there
        while ( (instFound < instToFind) && (mInvalidInstLenAtReadPtr > 0) )
        {
            BYTE    dataByte = 0;

            if ( mReadAddr == mAnchorAddr )
            {
                // at the anchor, stop treating this as an invalid instruction, 
                // and try to read a full instruction from scratch
                mInvalidInstLenAtReadPtr = 0;
                break;
            }

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

        mStartOfRead = true;

        // update for the byte before the first one in the range, so that we 
        // always get an accurate document change indication no matter where 
        // we start reading

        mDocInfo.Update( mReadAddr - 1 );

        for ( instLen = reader.Disassemble( mReadAddr ); 
            (instLen != 0) && (instFound < instToFind); 
            instLen = reader.Disassemble( mReadAddr ) )
        {
            const ud_t* ud = reader.GetDisasmData();

            if ( ud->mnemonic != UD_Iinvalid )
            {
                mDocInfo.Update( mReadAddr );
                FillInstDisasmData( ud, dwFields, &prgDisassembly[instFound] );
                mStartOfRead = false;

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

        _RPT3( _CRT_WARN, "Read ended: anchor=%08x read=%08x found=%d\n", 
            mAnchorAddr, mReadAddr, instFound );

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
            // the read address is where we left off, so set the anchor there
            mAnchorAddr = mReadAddr;
            mInstCache.SetAnchor( mAnchorAddr );
            break;

        case SEEK_START_CODECONTEXT:
            {
                Address                         newAnchor = 0;
                CComQIPtr<IMagoMemoryContext>   magoMem = pCodeContext;

                if ( magoMem == NULL )
                    return E_INVALIDARG;

                magoMem->GetAddress( newAnchor );

                mAnchorAddr = newAnchor;
                mInstCache.SetAnchor( mAnchorAddr );
            }
            break;

        case SEEK_START_CODELOCID:
            if ( (Address) uCodeLocationId != uCodeLocationId )
                return E_INVALIDARG;

            mAnchorAddr = (Address) uCodeLocationId;
            mInstCache.SetAnchor( mAnchorAddr );
            break;

        case SEEK_START_BEGIN:
        case SEEK_START_END:
            return E_NOTIMPL;
        }

        _RPT3( _CRT_WARN, "Seek: anchor=%08x iInst=%I64d (seekStart=%d)\n", 
            mAnchorAddr, iInstructions, dwSeekStart );

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

    bool DisassemblyStream::GetDocContext( 
        Address address, 
        Module* mod, 
        IDebugDocumentContext2** docContext )
    {
        _ASSERT( mod != NULL );
        _ASSERT( docContext != NULL );

        if ( (mod == NULL) || (docContext == NULL) )
            return false;

        RefPtr<MagoST::ISession>        session;
        RefPtr<SingleDocumentContext>   docCtx;

        HRESULT             hr = S_OK;
        CComBSTR            filename;
        CComBSTR            langName;
        GUID                langGuid;
        TEXT_POSITION       posBegin = { 0 };
        TEXT_POSITION       posEnd = { 0 };
        MagoST::FileInfo    fileInfo = { 0 };
        MagoST::LineNumber  line = { 0 };
        uint16_t            section = 0;
        uint32_t            offset = 0;

        if ( !mod->GetSymbolSession( session ) )
            return false;

        section = session->GetSecOffsetFromVA( address, offset );
        if ( section == 0 )
            return false;

        if ( !session->FindLine( section, offset, line ) )
            return false;

        hr = session->GetFileInfo( line.CompilandIndex, line.FileIndex, fileInfo );
        if ( FAILED( hr ) )
            return false;

        hr = Utf8To16( fileInfo.Name.ptr, fileInfo.Name.length, filename.m_str );
        if ( FAILED( hr ) )
            return false;

        // TODO:
        //compiland->get_language();

        posBegin.dwLine = line.Number;
        posEnd.dwLine = line.NumberEnd;

        // AD7 lines are 0-based, DIA ones are 1-based
        posBegin.dwLine--;
        posEnd.dwLine--;

        hr = MakeCComObject( docCtx );
        if ( FAILED( hr ) )
            return false;

        hr = docCtx->Init( filename, posBegin, posEnd, langName, langGuid );
        if ( FAILED( hr ) )
            return false;

        hr = docCtx->QueryInterface( 
            __uuidof( IDebugDocumentContext2 ), (void**) docContext );
        if ( FAILED( hr ) )
            return false;

        return true;
    }

    HRESULT DisassemblyStream::GetCodeContext( 
        UINT64               uCodeLocationId,
        IDebugCodeContext2** ppCodeContext )
    {
        _RPT1( _CRT_WARN, "DisassemblyStream::GetCodeContext: %08x\n", (UINT32) uCodeLocationId );

        HRESULT             hr = S_OK;
        Address             addr = (Address) uCodeLocationId;
        RefPtr<CodeContext> codeContext;
        RefPtr<Module>      mod;
        CComPtr<IDebugDocumentContext2> docCtx;

        if ( mProg->FindModuleContainingAddress( addr, mod ) )
        {
            GetDocContext( addr, mod, &docCtx );
            // it's OK if doc's not found
        }
        // it's OK if module's not found

        hr = MakeCComObject( codeContext );
        if ( FAILED( hr ) )
            return hr;

        hr = codeContext->Init( addr, mod, docCtx );
        if ( FAILED( hr ) )
            return hr;

        return codeContext->QueryInterface( 
            __uuidof( IDebugCodeContext2 ), (void**) ppCodeContext );
    }

    HRESULT DisassemblyStream::GetCurrentLocation( 
        UINT64* puCodeLocationId )
    {
        if ( puCodeLocationId == NULL )
            return E_INVALIDARG;

        *puCodeLocationId = mReadAddr;

        _RPT2( _CRT_WARN, "GetCurrentLocation: anchor=%08x read=%08x\n", 
            mAnchorAddr, mReadAddr );

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
        _ASSERT( program != NULL );

        HRESULT hr = S_OK;

        hr = mInstCache.Init( program, debugger );
        if ( FAILED( hr ) )
            return hr;

        mDocInfo.Init( program );

        mScope = disasmScope;
        mAnchorAddr = address;
        mReadAddr = address;
        mProg = program;

        mInstCache.SetAnchor( mAnchorAddr );

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

        _RPT4( _CRT_WARN, "SeekBack ended: anchor=%08x read=%08x iInst=%d found=%d\n", 
            mAnchorAddr, mReadAddr, iInstructions, instFound );

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
        InstReader  reader( blockCount, blocks, mAnchorAddr, endAddr, mAnchorAddr );
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

        _RPT4( _CRT_WARN, "SeekForward ended: anchor=%08x read=%08x iInst=%d found=%d\n", 
            mAnchorAddr, mReadAddr, iInstructions, instFound );

        return S_OK;
    }
}
