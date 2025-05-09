/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "ISession.h"

#include <set>
#include <map>
#include <string>

namespace MagoST
{
    class DataSource;
    class DebugStore;
    class IDebugStore;
    class IAddressMap;


    class Session : public ISession
    {
        long        mRefCount;

        uint64_t            mLoadAddr;
        RefPtr<DataSource>  mDataSource;
        IDebugStore*        mStore;         // valid while we hold onto data source
        RefPtr<IAddressMap> mAddrMap;

        struct reverse_less
        {
            bool operator() (const std::string& s1, const std::string& s2) const;
        };
        std::set<std::string, reverse_less> mGlobals;
        std::map<std::string, std::vector<SymHandle>, reverse_less> mDebugFuncs;

        std::map<std::string, std::string> mUDTshorts; // FQN -> short name
        std::map<std::string, std::string> mFuncShorts; // FQN -> short name
        // short name -> FQNs, values in mUDTshorts
        struct less_string_ptr {
            bool operator()(const std::string* s1, const std::string* s2) const { return *s1 < *s2; }
        };
        std::map<const std::string*, std::vector<const std::string*>, less_string_ptr> mUDTfqns;


        void _cacheGlobals();
        void _finalizeUDTshorts();
        void _buildUDTfqns();
        void _finalizeFuncShorts();

        HRESULT _findGlobalSymbol(const char* symbol, std::function<bool(TypeIndex)> fnTest,
                                  SymHandle& handle, SymInfoData& infoData, ISymbolInfo*& symInfo );
        HRESULT _findMatchingGlobals( std::set<std::string, reverse_less>& symSet,
            const char* nameChars, size_t nameLen, std::vector<SymHandle>& handles );

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
        virtual HRESULT FindSymbolDone( EnumNamedSymbolsData& handle );

        virtual HRESULT FindGlobalSymbolAddress( const char* symbol, uint64_t& symaddr, std::function<bool(TypeIndex)> fnTest = nullptr );

        virtual HRESULT FindChildSymbol(
            SymHandle parentHandle,
            uint32_t pcrva,
            const char* nameChars,
            size_t nameLen, 
            SymHandle& handle );

        virtual HRESULT FindMatchingGlobals( const char* nameChars, size_t nameLen, std::vector<SymHandle>& handles );
        virtual HRESULT FindMatchingDebugFuncs( const char* nameChars, size_t nameLen, std::vector<SymHandle>& handles );

        virtual HRESULT FindUDTShortName( const char* nameChars, size_t nameLen, std::string& shortName );
        virtual HRESULT FindUDTLongName( const char* nameChars, size_t nameLen, std::string& longName );
        virtual HRESULT FindFuncShortName( const char* nameChars, size_t nameLen, std::string& shortName );

        virtual HRESULT FindGlobalSymbolByAddr( uint64_t va, SymHandle& symHandle, uint16_t& sec, uint32_t& offset, uint32_t& symOff );

        virtual HRESULT FindOuterSymbolByAddr( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle, DWORD& symOff );
        virtual HRESULT FindOuterSymbolByRVA( SymbolHeapId heapId, DWORD rva, SymHandle& handle );
        virtual HRESULT FindOuterSymbolByVA( SymbolHeapId heapId, DWORD64 va, SymHandle& handle );
        virtual HRESULT FindInnermostSymbol( SymHandle parentHandle, WORD segment, DWORD offset, std::vector<SymHandle>& handles );

        virtual HRESULT SetChildSymbolScope( SymHandle handle, SymbolScope& scope );

        virtual bool NextSymbol( SymbolScope& scope, SymHandle& handle, DWORD addr );
        virtual HRESULT EndSymbolScope( SymbolScope& scope );

        // types

        virtual HRESULT GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo );

        virtual HRESULT SetChildTypeScope( TypeHandle handle, TypeScope& scope );

        virtual bool NextType( TypeScope& scope, TypeHandle& handle );
        virtual HRESULT EndTypeScope( TypeScope& scope );

        virtual bool GetTypeFromTypeIndex( TypeIndex typeIndex, TypeHandle& handle );

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
        virtual bool FindNextLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber );

        virtual bool FindLines( bool exactMatch, const char* fileName, size_t fileNameLen, uint16_t reqLineStart, uint16_t reqLineEnd, 
                                std::list<LineNumber>& lines );
    };
}
