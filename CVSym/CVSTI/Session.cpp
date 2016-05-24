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

        if ( rva == ~0 )
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
            SymString       pstrName;

            hr = mStore->GetSymbolInfo( childHandle, infoData, symInfo );
            if ( hr != S_OK )
                continue;

            if ( !symInfo->GetName( pstrName ) )
                continue;

            if ( (nameLen == pstrName.GetLength()) && (memcmp( nameChars, pstrName.GetName(), nameLen ) == 0) )
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

    HRESULT Session::FindInnermostSymbol( SymHandle parentHandle, WORD segment, DWORD offset, std::vector<SymHandle>& handles )
    {
        HRESULT         hr = S_OK;
        SymInfoData     infoData = { 0 };
        ISymbolInfo*    symInfo = NULL;
        uint16_t        funcSeg = 0;

        handles.resize( 0 );

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

            handles.push_back( parentHandle );

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

    bool Session::GetTypeFromTypeIndex( TypeIndex typeIndex, TypeHandle& handle )
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
            SymString       pstrName;

            hr = mStore->GetTypeInfo( childHandle, infoData, symInfo );
            if ( hr != S_OK )
                continue;

            if ( !symInfo->GetName( pstrName ) )
                continue;

            if ( (nameLen == pstrName.GetLength()) && (memcmp( nameChars, pstrName.GetName(), nameLen ) == 0) )
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

    bool Session::GetFileSegment( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, FileSegmentInfo& segInfo )
    {
        return mStore->GetFileSegment( compIndex, fileIndex, segInstanceIndex, segInfo );
    }

    bool Session::FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber )
    {
        return mStore->FindLine( seg, offset, lineNumber );
    }
    bool Session::FindLines( bool exactMatch, const char* fileName, size_t fileNameLen, uint16_t reqLineStart, uint16_t reqLineEnd, 
                             std::list<LineNumber>& lines )
    {
        return mStore->FindLines( exactMatch, fileName, fileNameLen, reqLineStart, reqLineEnd, lines );
    }

    bool Session::FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber )
    {
        return mStore->FindLineByNum( compIndex, fileIndex, line, lineNumber );
    }
    bool Session::FindNextLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber )
    {
        return mStore->FindNextLineByNum( compIndex, fileIndex, line, lineNumber );
    }
}
