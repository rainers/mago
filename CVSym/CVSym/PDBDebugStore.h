/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "DebugStore.h"

struct IDiaDataSource;
struct IDiaSession;
struct IDiaSymbol;
struct IDiaEnumLineNumbers;
struct IDiaSourceFile;

namespace MagoST
{
    class PDBDebugStore : public IDebugStore
    {

    public:
        PDBDebugStore();
        virtual ~PDBDebugStore();

        HRESULT InitDebugInfo( BYTE* buffer, DWORD size );
        void CloseDebugInfo();
        IDiaSession* getSession() const { return mSession; }

        // symbols

        virtual HRESULT SetSymbolScope( SymbolHeapId heapId, SymbolScope& scope );
        virtual HRESULT SetCompilandSymbolScope( DWORD compilandIndex, SymbolScope& scope );
        virtual HRESULT SetChildSymbolScope( SymHandle handle, SymbolScope& scope );

        virtual bool NextSymbol( SymbolScope& scope, SymHandle& handle );

        virtual HRESULT FindFirstSymbol( SymbolHeapId heapId, const char* nameChars, size_t nameLen, EnumNamedSymbolsData& data );
        virtual HRESULT FindNextSymbol( EnumNamedSymbolsData& handle );
        virtual HRESULT GetCurrentSymbol( const EnumNamedSymbolsData& searchHandle, SymHandle& handle );

        virtual HRESULT FindSymbol( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle );

        virtual HRESULT GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo );

        bool IsTLSData( SymHandleIn& internalHandle );

        // types

        virtual HRESULT SetGlobalTypeScope( TypeScope& scope );
        virtual HRESULT SetChildTypeScope( TypeHandle handle, TypeScope& scope );

        virtual bool NextType( TypeScope& scope, TypeHandle& handle );

        virtual bool GetTypeFromTypeIndex( TypeIndex typeIndex, TypeHandle& handle );

        virtual HRESULT GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo );

        // source files

        virtual HRESULT GetCompilandCount( uint32_t& count );
        virtual HRESULT GetCompilandInfo( uint16_t index, CompilandInfo& info );
        virtual HRESULT GetCompilandSegmentInfo( uint16_t index, uint16_t count, SegmentInfo* infos );
        virtual HRESULT GetFileInfo( uint16_t compilandIndex, uint16_t fileIndex, FileInfo& info );
        virtual HRESULT GetFileSegmentInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t count, SegmentInfo* infos );
        virtual HRESULT GetLineInfo( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, uint16_t count, LineInfo* infos);

        virtual bool    GetFileSegment( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, FileSegmentInfo& segInfo );

        virtual bool    FindCompilandFileSegment( WORD seg, DWORD offset, uint16_t& compIndex, uint16_t& fileIndex, FileSegmentInfo& segInfo );
        virtual bool    FindCompilandFileSegment( uint16_t line, uint16_t compIndex, uint16_t fileIndex, FileSegmentInfo& segInfo );

    private:
        HRESULT fillFileSegmentInfo( IDiaEnumLineNumbers *pEnumLineNumbers, FileSegmentInfo& segInfo,
                                     IDiaSymbol **ppCompiland = NULL, IDiaSourceFile **ppSourceFile = NULL );
        HRESULT findCompilandAndFile( IDiaSymbol *pCompiland, IDiaSourceFile *pSourceFile, uint16_t& compIndex, uint16_t& fileIndex );

        bool mInit;
        IDiaDataSource  *mSource;
        IDiaSession     *mSession;
        IDiaSymbol      *mGlobal;
        DWORD            mMachineType;
        long             mCompilandCount;

        std::auto_ptr<char>  mLastCompilandName;
        std::auto_ptr<char>  mLastFileInfoName;
        std::auto_ptr<DWORD> mLastSegInfoOffsets;
        std::auto_ptr<WORD>  mLastSegInfoLineNumbers;
    };
}
