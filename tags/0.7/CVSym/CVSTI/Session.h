/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "ISession.h"


namespace MagoST
{
    class DataSource;
    class DebugStore;
    class IAddressMap;


    class Session : public ISession
    {
        long        mRefCount;

        uint64_t            mLoadAddr;
        RefPtr<DataSource>  mDataSource;
        DebugStore*         mStore;         // valid while we hold onto data source
        RefPtr<IAddressMap> mAddrMap;

    public:
        Session( DataSource* dataSource );

        virtual void AddRef();
        virtual void Release();

        virtual uint64_t GetLoadAddress();
        virtual void SetLoadAddress( uint64_t va );

        // mapping addresses

        virtual uint32_t GetRVAFromSecOffset( uint16_t secIndex, uint32_t offset );
        virtual uint64_t GetVAFromSecOffset( uint16_t secIndex, uint32_t offset );
        virtual uint16_t GetSecOffsetFromRVA( uint32_t rva, uint32_t& offset );
        virtual uint16_t GetSecOffsetFromVA( uint64_t va, uint32_t& offset );

        // symbols

        virtual void CopySymbolInfo( const SymInfoData& sourceInfo, SymInfoData& destInfo, ISymbolInfo*& symInfo );

        virtual HRESULT GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo );

        virtual HRESULT FindFirstSymbol( 
            SymbolHeapId heapId, 
            const char* nameChars, 
            size_t nameLen, 
            EnumNamedSymbolsData& data );
        virtual HRESULT FindNextSymbol( EnumNamedSymbolsData& handle );
        virtual HRESULT GetCurrentSymbol( const EnumNamedSymbolsData& searchHandle, SymHandle& handle );

        virtual HRESULT FindChildSymbol(
            SymHandle parentHandle, 
            const char* nameChars, 
            size_t nameLen, 
            SymHandle& handle );

        virtual HRESULT FindOuterSymbolByAddr( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle );
        virtual HRESULT FindOuterSymbolByRVA( SymbolHeapId heapId, DWORD rva, SymHandle& handle );
        virtual HRESULT FindOuterSymbolByVA( SymbolHeapId heapId, DWORD64 va, SymHandle& handle );
        virtual HRESULT FindInnermostSymbol( SymHandle parentHandle, WORD segment, DWORD offset, SymHandle& handle );

        virtual HRESULT SetChildSymbolScope( SymHandle handle, SymbolScope& scope );

        virtual bool NextSymbol( SymbolScope& scope, SymHandle& handle );

        // types

        virtual HRESULT GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo );

        virtual HRESULT SetChildTypeScope( TypeHandle handle, TypeScope& scope );

        virtual bool NextType( TypeScope& scope, TypeHandle& handle );

        virtual bool GetTypeFromTypeIndex( WORD typeIndex, TypeHandle& handle );

        virtual HRESULT FindChildType( 
            TypeHandle parentHandle, 
            const char* nameChars, 
            size_t nameLen, 
            TypeHandle& handle );

        // source files

        virtual HRESULT GetCompilandCount( uint32_t& count );
        virtual HRESULT GetCompilandInfo( uint16_t index, CompilandInfo& info );
        virtual HRESULT GetCompilandSegmentInfo( uint16_t index, uint16_t count, SegmentInfo* infos );
        virtual HRESULT GetFileInfo( uint16_t compilandIndex, uint16_t fileIndex, FileInfo& info );
        virtual HRESULT GetFileSegmentInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t count, SegmentInfo* infos );
        virtual HRESULT GetLineInfo( uint16_t compIndex, uint16_t fileIndex, uint16_t segIndex, uint16_t count, LineInfo* infos);

        virtual bool GetFileSegment( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, FileSegmentInfo& segInfo );

        virtual bool FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber );
        virtual bool FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber );

    private:
        void SetLineNumberFromSegment( uint16_t compIx, uint16_t fileIx, const FileSegmentInfo& segInfo, uint16_t lineIndex, LineNumber& lineNumber );

        template <class TElem>
        bool BinarySearch( TElem targetKey, TElem* array, int arrayLen, int& indexFound );
    };
}
