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
        return mStore->GetCurrentSymbol(searchHandle, handle);
    }

    HRESULT Session::FindSymbolDone( EnumNamedSymbolsData& handle )
    {
        return mStore->FindSymbolDone( handle );
    }

    HRESULT Session::_findGlobalSymbol( const char* symbol, std::function<bool(TypeIndex)> fnTest,
                                        SymHandle& handle, SymInfoData& infoData, ISymbolInfo*& symInfo )
    {
        HRESULT hr = S_OK;

        MagoST::EnumNamedSymbolsData enumData = { 0 };
        size_t len = strlen( symbol );
        hr = FindFirstSymbol( MagoST::SymHeap_GlobalSymbols, symbol, len, enumData );
        if (hr != S_OK)
            hr = FindFirstSymbol( MagoST::SymHeap_StaticSymbols, symbol, len, enumData );
        if (hr != S_OK)
            hr = FindFirstSymbol( MagoST::SymHeap_PublicSymbols, symbol, len, enumData );
        if (hr != S_OK)
            return hr;

        while( hr == S_OK )
        {
            hr = GetCurrentSymbol( enumData, handle );
            if( hr == S_OK )
                hr = GetSymbolInfo( handle, infoData, symInfo );
            if( !fnTest )
                break;

            TypeIndex index;
            if( symInfo->GetType( index ) )
                if( fnTest( index ) )
                    break;

            hr = FindNextSymbol( enumData );
        }

        FindSymbolDone( enumData );

        return hr;
    }

    HRESULT Session::FindGlobalSymbolAddress( const char* symbol, uint64_t& symaddr, std::function<bool(TypeIndex)> fnTest )
    {
        MagoST::SymHandle handle;
        MagoST::SymInfoData infoData = { 0 };
        MagoST::ISymbolInfo* symInfo = NULL;

        HRESULT hr = _findGlobalSymbol( symbol, fnTest, handle, infoData, symInfo );
        if ( hr != S_OK )
            return hr;

        uint16_t section = 0;
        uint32_t offset = 0;

        if ( !symInfo->GetAddressSegment( section ) || !symInfo->GetAddressOffset( offset ) )
            return E_FAIL;

        uint64_t addr = GetVAFromSecOffset( section, offset );
        if ( addr == 0 )
            return E_FAIL;

        symaddr = addr;
        return S_OK;
    }

    HRESULT Session::FindChildSymbol( SymHandle parentHandle, uint32_t pcrva, const char* nameChars, size_t nameLen, SymHandle& handle )
    {
        HRESULT     hr = S_OK;
        SymbolScope scope = { 0 };
        SymHandle   childHandle = { 0 };
        SymInfoData infoData = { 0 };

        hr = mStore->SetChildSymbolScope( parentHandle, scope );
        if ( FAILED( hr ) )
            return hr;

        for ( ; mStore->NextSymbol( scope, childHandle, pcrva ); )
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

    HRESULT Session::FindGlobalSymbolByAddr( uint64_t va, SymHandle& symHandle, uint16_t& sec, uint32_t& offset, uint32_t& symOff )
    {
        sec = GetSecOffsetFromVA( va, offset );
        if ( sec == 0 )
            return E_NOT_FOUND;

        DWORD off = 0;
        HRESULT hr = FindOuterSymbolByAddr( MagoST::SymHeap_GlobalSymbols, sec, offset, symHandle, off );
        if ( FAILED( hr ) )
            hr = FindOuterSymbolByAddr( MagoST::SymHeap_StaticSymbols, sec, offset, symHandle, off );
        if ( FAILED( hr ) )
            hr = FindOuterSymbolByAddr( MagoST::SymHeap_PublicSymbols, sec, offset, symHandle, off );

        if ( !FAILED( hr ) )
            symOff = off;

        return hr;
    }

    HRESULT Session::FindOuterSymbolByAddr( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle, DWORD& symOff )
    {
        return mStore->FindSymbol( heapId, segment, offset, handle, symOff );
    }

    HRESULT Session::FindOuterSymbolByRVA( SymbolHeapId heapId, DWORD rva, SymHandle& handle )
    {
        uint16_t    sec = 0;
        uint32_t    offset = 0;

        sec = mAddrMap->MapRVAToSecOffset( rva, offset );
        if ( sec == 0 )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        DWORD symOff;
        return mStore->FindSymbol( heapId, sec, offset, handle, symOff );
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

            for ( int j = 0; (j < USHRT_MAX) && !foundChild && mStore->NextSymbol( scope, curHandle, offset ); j++ )
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

    bool Session::NextSymbol( SymbolScope& scope, SymHandle& handle, DWORD addr )
    {
        return mStore->NextSymbol( scope, handle, addr );
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

        HRESULT found = S_FALSE;
        for ( ; mStore->NextType( scope, childHandle ); )
        {
            ISymbolInfo*    symInfo = NULL;
            SymString       pstrName;

            hr = mStore->GetTypeInfo( childHandle, infoData, symInfo );
            if ( hr != S_OK )
                continue;

            SymTag tag = symInfo->GetSymTag();
            if ( tag == SymTagBaseClass )
                continue;

            if ( !symInfo->GetName( pstrName ) )
                continue;

            if ( (nameLen == pstrName.GetLength()) && (memcmp( nameChars, pstrName.GetName(), nameLen ) == 0) )
            {
                if ( tag != SymTagFunction )
                {
                    handle = childHandle;
                    return S_OK;
                }

                // until we support overload sets, prefer an overload with zero arguments
                TypeIndex funcType;
                TypeHandle funcTypeHandle;
                SymInfoData funcInfoData = { 0 };
                ISymbolInfo* funcInfo = nullptr;
                if( !symInfo->GetType( funcType ) )
                    continue;
                if( !GetTypeFromTypeIndex( funcType, funcTypeHandle ) )
                    continue;
                hr = mStore->GetTypeInfo( funcTypeHandle, funcInfoData, funcInfo );
                if ( hr != S_OK )
                    continue;

                std::vector<TypeIndex> indexes;
                if ( !funcInfo->GetTypes( indexes ) )
                    continue;
                if ( indexes.size() == 0 )
                {
                    handle = childHandle;
                    return S_OK;
                }
                else if ( found == S_FALSE )
                {
                    handle = childHandle;
                    found = S_OK;
                }
            }
        }

        return found;
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

    bool Session::reverse_less::operator() ( const std::string& s1, const std::string& s2 ) const
    {
        size_t l1 = s1.length();
        size_t l2 = s2.length();
        size_t lmin = l1 < l2 ? l1 : l2;
        for ( size_t i = 1; i <= lmin; i++ )
            if ( s1[l1 - i] < s2[l2 - i] )
                return true;
            else if ( s1[l1 - i] > s2[l2 - i] )
                return false;

        return l1 < l2;
    }

    void Session::cacheGlobals()
    {
        if( !mGlobals.empty() || !mDebugFuncs.empty() )
            return;

        EnumNamedSymbolsData searchHandle;
        if( mStore->FindFirstSymbol( SymHeap_GlobalSymbols, nullptr, 0, searchHandle) != S_OK)
            return;

        do
        {
            SymHandle symHandle;
            MagoST::SymInfoData symData = { 0 };
            MagoST::ISymbolInfo* symInfo = NULL;
            if( mStore->GetCurrentSymbol( searchHandle, symHandle) != S_OK )
                break;
            if( mStore->GetSymbolInfo( symHandle, symData, symInfo ) != S_OK )
                break;

            DataKind kind = DataIsUnknown;
            SymTag tag = symInfo->GetSymTag();
            if ( tag == SymTagFunction || tag == SymTagMethod )
            {
                // same or preceded by module/struct name
                auto ends_with = [](const SymString& s, const char* name)
                    {
                        auto sptr = s.GetName();
                        size_t slen = s.GetLength();
                        size_t nlen = strlen(name);
                        if (slen == nlen && strncmp(sptr, name, nlen) == 0)
                            return true;
                        if (slen <= nlen)
                            return false;
                        size_t npos = slen - nlen;
                        return sptr[npos - 1] == '.' && strncmp(sptr + npos, name, nlen) == 0;
                    };
                SymString name;
                if (symInfo->GetName(name))
                {
                    if (ends_with(name, "__debugOverview") ||
                        ends_with(name, "__debugExpanded") ||
                        ends_with(name, "__debugStringView"))
                    {
                        std::string sname(name.GetName(), name.GetLength());
                        auto it = mDebugFuncs.find(sname);
                        if (it == mDebugFuncs.end())
                            mDebugFuncs.insert({ sname, { symHandle } });
                        else
                            it->second.push_back(symHandle);
                    }
                }
            }
            else if( symInfo->GetDataKind( kind ) )
            //    break;
            switch( kind )
            {
                case DataIsFileStatic:
                case DataIsGlobal:
                case DataIsStaticLocal:
                {
                    SymString name;
                    if( symInfo->GetName( name ) )
                    {
                        mGlobals.insert( std::string( name.GetName(), name.GetLength() ) );
                    }
                    break;
                }
            }

        } while ( mStore->FindNextSymbol( searchHandle ) == S_OK );

        mStore->FindSymbolDone( searchHandle );
    }

    HRESULT Session::_findMatchingGlobals( std::set<std::string, reverse_less>& symSet,
        const char* nameChars, size_t nameLen, std::vector<SymHandle>& handles )
    {
        cacheGlobals();
        
        std::string search( nameChars, nameLen );
        for( auto it = symSet.lower_bound( search ); it != symSet.end(); ++it )
        {
            if ( it->length() < nameLen || search != it->data() + it->length() - nameLen )
                break;
            if ( it->length() == nameLen ) // skip name without '.'
                continue;
            if ( (*it)[it->length() - nameLen - 1] != '.' )
                break;

            MagoST::SymInfoData infoData = { 0 };
            MagoST::ISymbolInfo* symInfo = NULL;
            SymHandle handle;

            HRESULT hr = _findGlobalSymbol( it->data(), nullptr, handle, infoData, symInfo );
            if( SUCCEEDED( hr ) )
                handles.push_back( handle );
        }
        return handles.empty() ? E_NOT_FOUND : S_OK;
    }

    HRESULT Session::FindMatchingGlobals( const char* nameChars, size_t nameLen, std::vector<SymHandle>& handles )
    {
        return _findMatchingGlobals( mGlobals, nameChars, nameLen, handles );
    }
    HRESULT Session::FindMatchingDebugFuncs( const char* nameChars, size_t nameLen, std::vector<SymHandle>& handles )
    {
        cacheGlobals();

        std::string search(nameChars, nameLen);
        for (auto it = mDebugFuncs.lower_bound(search); it != mDebugFuncs.end(); ++it)
        {
            if (it->first.length() < nameLen || search != it->first.data() + it->first.length() - nameLen)
                break;
            if (it->first.length() > nameLen && it->first[it->first.length() - nameLen - 1] != '.')
                break;

            for(auto& handle : it->second)
                handles.push_back( handle );
        }
        return handles.empty() ? E_NOT_FOUND : S_OK;
    }
}
