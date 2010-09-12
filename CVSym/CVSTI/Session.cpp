/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Session.h"
#include "DataSource.h"
#include "IAddressMap.h"
// TODO:
#include "..\CVSym\cvconst.h"


namespace MagoST
{
    Session::Session( DataSource* dataSource )
        :   mRefCount( 0 ),
            mLoadAddr( 0 ),
            mDataSource( dataSource ),
            mStore( NULL )
    {
        _ASSERT( dataSource != NULL );

        mStore = dataSource->GetDebugStore();
        mAddrMap = dataSource->GetAddressMap();
        _ASSERT( mStore != NULL );
        _ASSERT( mAddrMap != NULL );
    }

    void Session::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void Session::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    uint64_t Session::GetLoadAddress()
    {
        return mLoadAddr;
    }

    void Session::SetLoadAddress( uint64_t va )
    {
        mLoadAddr = va;
    }

    uint32_t Session::GetRVAFromSecOffset( uint16_t secIndex, uint32_t offset )
    {
        return mAddrMap->MapSecOffsetToRVA( secIndex, offset );
    }

    uint64_t Session::GetVAFromSecOffset( uint16_t secIndex, uint32_t offset )
    {
        uint32_t    rva = mAddrMap->MapSecOffsetToRVA( secIndex, offset );

        if ( rva == 0 )
            return 0;

        return mLoadAddr + rva;
    }

    uint16_t Session::GetSecOffsetFromRVA( uint32_t rva, uint32_t& offset )
    {
        return mAddrMap->MapRVAToSecOffset( rva, offset );
    }

    uint16_t Session::GetSecOffsetFromVA( uint64_t va, uint32_t& offset )
    {
        uint64_t    rva = va - mLoadAddr;

        if ( rva > ULONG_MAX )
            return 0;

        return mAddrMap->MapRVAToSecOffset( (uint32_t) rva, offset );
    }

    void Session::CopySymbolInfo( const SymInfoData& sourceInfo, SymInfoData& destInfo, ISymbolInfo*& symInfo )
    {
        destInfo = sourceInfo;
        symInfo = (ISymbolInfo*) &destInfo;
    }

    HRESULT Session::FindFirstSymbol( 
        SymbolHeapId heapId, 
        const char* nameChars, 
        size_t nameLen, 
        EnumNamedSymbolsData& data )
    {
        return mStore->FindFirstSymbol( heapId, nameChars, nameLen, data );
    }

    HRESULT Session::FindNextSymbol( EnumNamedSymbolsData& handle )
    {
        return mStore->FindNextSymbol( handle );
    }

    HRESULT Session::GetCurrentSymbol( const EnumNamedSymbolsData& searchHandle, SymHandle& handle )
    {
        return mStore->GetCurrentSymbol( searchHandle, handle );
    }

    HRESULT Session::FindChildSymbol( SymHandle parentHandle, const char* nameChars, size_t nameLen, SymHandle& handle )
    {
        HRESULT     hr = S_OK;
        SymbolScope scope = { 0 };
        SymHandle   childHandle = { 0 };
        SymInfoData infoData = { 0 };

        hr = mStore->SetChildSymbolScope( parentHandle, scope );
        if ( FAILED( hr ) )
            return hr;

        for ( ; mStore->NextSymbol( scope, childHandle ); )
        {
            ISymbolInfo*    symInfo = NULL;
            PasString*      pstrName = NULL;

            hr = mStore->GetSymbolInfo( childHandle, infoData, symInfo );
            if ( hr != S_OK )
                continue;

            if ( !symInfo->GetName( pstrName ) )
                continue;

            if ( (nameLen == pstrName->GetLength()) && (memcmp( nameChars, pstrName->GetName(), nameLen ) == 0) )
            {
                handle = childHandle;
                return S_OK;
            }
        }

        return S_FALSE;
    }

    HRESULT Session::FindOuterSymbolByAddr( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle )
    {
        return mStore->FindSymbol( heapId, segment, offset, handle );
    }

    HRESULT Session::FindOuterSymbolByRVA( SymbolHeapId heapId, DWORD rva, SymHandle& handle )
    {
        uint16_t    sec = 0;
        uint32_t    offset = 0;

        sec = mAddrMap->MapRVAToSecOffset( rva, offset );
        if ( sec == 0 )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        return mStore->FindSymbol( heapId, sec, offset, handle );
    }

    HRESULT Session::FindOuterSymbolByVA( SymbolHeapId heapId, DWORD64 va, SymHandle& handle )
    {
        uint64_t    rva = va - mLoadAddr;

        if ( rva > ULONG_MAX )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        return FindOuterSymbolByRVA( heapId, (DWORD) rva, handle );
    }

    HRESULT Session::FindInnermostSymbol( SymHandle parentHandle, WORD segment, DWORD offset, SymHandle& handle )
    {
        HRESULT         hr = S_OK;
        SymInfoData     infoData = { 0 };
        ISymbolInfo*    symInfo = NULL;
        uint16_t        funcSeg = 0;

        hr = mStore->GetSymbolInfo( parentHandle, infoData, symInfo );
        if ( FAILED( hr ) )
            return hr;

        if ( !symInfo->GetAddressSegment( funcSeg ) || (funcSeg != segment) )
            return E_FAIL;

        // recursing down the block children has to stop somewhere
        for ( int i = 0; i < USHRT_MAX; i++ )
        {
            SymbolScope scope = { 0 };
            // in case there are no children
            SymHandle   curHandle = parentHandle;
            bool        foundChild = false;

            hr = mStore->SetChildSymbolScope( parentHandle, scope );
            if ( FAILED( hr ) )
                return hr;

            for ( int j = 0; (j < USHRT_MAX) && !foundChild && mStore->NextSymbol( scope, curHandle ); j++ )
            {
                SymTag          tag = SymTagNull;
                uint32_t        childOffset = 0;
                uint32_t        childLen = 0;

                hr = mStore->GetSymbolInfo( curHandle, infoData, symInfo );
                if ( FAILED( hr ) )
                    continue;

                tag = symInfo->GetSymTag();

                switch ( tag )
                {
                case SymTagBlock:
                case SymTagFunction:
                case SymTagThunk:
                // TODO: With
                    if ( !symInfo->GetAddressOffset( childOffset ) )
                        continue;
                    if ( !symInfo->GetLength( childLen ) )
                        continue;
                    if ( (offset >= childOffset) && (offset < (childOffset + childLen)) )
                        foundChild = true;
                    break;
                }
            }

            if ( foundChild )
                parentHandle = curHandle;
            else
                // didn't find a good child, return what we have
                break;
        }

        handle = parentHandle;
        return S_OK;
    }

    HRESULT Session::SetChildSymbolScope( SymHandle handle, SymbolScope& scope )
    {
        return mStore->SetChildSymbolScope( handle, scope );
    }

    bool Session::NextSymbol( SymbolScope& scope, SymHandle& handle )
    {
        return mStore->NextSymbol( scope, handle );
    }

    HRESULT Session::GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo )
    {
        return mStore->GetSymbolInfo( handle, privateData, symInfo );
    }

    bool Session::GetTypeFromTypeIndex( WORD typeIndex, TypeHandle& handle )
    {
        return mStore->GetTypeFromTypeIndex( typeIndex, handle );
    }

    HRESULT Session::GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo )
    {
        return mStore->GetTypeInfo( handle, privateData, symInfo );
    }

    HRESULT Session::SetChildTypeScope( TypeHandle handle, TypeScope& scope )
    {
        return mStore->SetChildTypeScope( handle, scope );
    }

    bool Session::NextType( TypeScope& scope, TypeHandle& handle )
    {
        return mStore->NextType( scope, handle );
    }

    HRESULT Session::FindChildType( 
        TypeHandle parentHandle, 
        const char* nameChars, 
        size_t nameLen, 
        TypeHandle& handle )
    {
        HRESULT     hr = S_OK;
        TypeScope   scope = { 0 };
        TypeHandle  childHandle = { 0 };
        SymInfoData infoData = { 0 };

        hr = mStore->SetChildTypeScope( parentHandle, scope );
        if ( FAILED( hr ) )
            return hr;

        for ( ; mStore->NextType( scope, childHandle ); )
        {
            ISymbolInfo*    symInfo = NULL;
            PasString*      pstrName = NULL;

            hr = mStore->GetTypeInfo( childHandle, infoData, symInfo );
            if ( hr != S_OK )
                continue;

            if ( !symInfo->GetName( pstrName ) )
                continue;

            if ( (nameLen == pstrName->GetLength()) && (memcmp( nameChars, pstrName->GetName(), nameLen ) == 0) )
            {
                handle = childHandle;
                return S_OK;
            }
        }

        return S_FALSE;
    }

    // source files

    HRESULT Session::GetCompilandCount( uint32_t& count )
    {
        return mStore->GetCompilandCount( count );
    }

    HRESULT Session::GetCompilandInfo( uint16_t index, CompilandInfo& info )
    {
        return mStore->GetCompilandInfo( index, info );
    }

    HRESULT Session::GetCompilandSegmentInfo( uint16_t index, uint16_t count, SegmentInfo* infos )
    {
        return mStore->GetCompilandSegmentInfo( index, count, infos );
    }

    HRESULT Session::GetFileInfo( uint16_t compilandIndex, uint16_t fileIndex, FileInfo& info )
    {
        return mStore->GetFileInfo( compilandIndex, fileIndex, info );
    }

    HRESULT Session::GetFileSegmentInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t count, SegmentInfo* infos )
    {
        return mStore->GetFileSegmentInfo( compilandIndex, fileIndex, count, infos );
    }

    HRESULT Session::GetLineInfo( uint16_t compIndex, uint16_t fileIndex, uint16_t segIndex, uint16_t count, LineInfo* infos)
    {
        return mStore->GetLineInfo( compIndex, fileIndex, segIndex, count, infos );
    }

    bool Session::FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber )
    {
        uint16_t        compIx = 0;
        uint16_t        fileIx = 0;
        FileSegmentInfo fileSegInfo = { 0 };
        int             i = 0;

        if ( !mStore->FindCompilandFileSegment( seg, offset, compIx, fileIx, fileSegInfo ) )
            return false;

        if ( !BinarySearch<DWORD>( offset, fileSegInfo.Offsets, fileSegInfo.LineCount, i ) )
            return false;

        SetLineNumberFromSegment( compIx, fileIx, fileSegInfo, (uint16_t) i, lineNumber );
        return true;
    }

    template <class TElem>
    bool Session::BinarySearch( TElem targetKey, TElem* array, int arrayLen, int& indexFound )
    {
        int     lo = 0;
        int     hi = arrayLen;

        for ( int i = arrayLen / 2; lo < hi ; i = (lo + hi) / 2 )
        {
            if ( array[i] == targetKey )
            {
                indexFound = i;
                return true;
            }

            if ( array[i] < targetKey )
            {
                if ( (i == arrayLen - 1) || (array[i + 1] > targetKey) )
                {
                    indexFound = i;
                    return true;
                }
                lo = i + 1;
            }
            else
            {
                hi = i;
            }
        }

        return false;
    }

    bool Session::FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber )
    {
        FileSegmentInfo fileSegInfo = { 0 };

        if ( !mStore->FindCompilandFileSegment( line, compIndex, fileIndex, fileSegInfo ) )
            return false;

        if ( fileSegInfo.LineCount == 0 )
            return false;

        int     lastLine = 0;
        int     closestDist = std::numeric_limits<int>::max();
        int     lastLineIndex = -1;
        int     closestLineIndex = -1;

        for ( int i = 0; i < fileSegInfo.LineCount; i++ )
        {
            int curLine = fileSegInfo.LineNumbers[i];

            if ( curLine > lastLine )
            {
                lastLine = curLine;
                lastLineIndex = i;
            }

            if ( (curLine >= line) && ((curLine - line) < closestDist) )
            {
                closestDist = curLine - line;
                closestLineIndex = i;
            }
        }

        // a line was found after or at the target line
        // it's the closest one, so use it

        if ( closestLineIndex >= 0 )
        {
            SetLineNumberFromSegment( compIndex, fileIndex, fileSegInfo, (uint16_t) closestLineIndex, lineNumber );
        }
        else
        {
            // there are lines, so one of them has to be the last one
            _ASSERT( lastLineIndex >= 0 );

            SetLineNumberFromSegment( compIndex, fileIndex, fileSegInfo, (uint16_t) lastLineIndex, lineNumber );
        }

        return true;
    }

    void Session::SetLineNumberFromSegment( uint16_t compIx, uint16_t fileIx, const FileSegmentInfo& segInfo, uint16_t lineIndex, LineNumber& lineNumber )
    {
        lineNumber.CompilandIndex = compIx;
        lineNumber.FileIndex = fileIx;

        lineNumber.Number = segInfo.LineNumbers[ lineIndex ];
        lineNumber.Offset = segInfo.Offsets[ lineIndex ];
        lineNumber.Section = segInfo.SegmentIndex;

        if ( lineIndex == segInfo.LineCount - 1 )
        {
            // TODO: do we have to worry about segInfo.End being 0?
            lineNumber.Length = segInfo.End - lineNumber.Offset + 1;
            lineNumber.NumberEnd = 0x7fff;
        }
        else
        {
            lineNumber.Length = segInfo.Offsets[ lineIndex + 1 ] - lineNumber.Offset;
            lineNumber.NumberEnd = segInfo.LineNumbers[ lineIndex + 1 ] - 1;
        }

        //lineNumber.NumberEnd = lineNumber.Number + 1;
    }
}
