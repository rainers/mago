/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "DebugStore.h"
#include <map>
#include <memory>

struct IDiaDataSource;
struct IDiaSession;
struct IDiaSymbol;
struct IDiaEnumLineNumbers;
struct IDiaLineNumber;
struct IDiaSourceFile;

namespace MagoST
{
    class IAddressMap;
    class PDBSymbolInfo;

    class PDBDebugStore : public IDebugStore
    {

    public:
        PDBDebugStore();
        virtual ~PDBDebugStore();

        HRESULT InitDebugInfo( BYTE* buffer, DWORD size, const wchar_t* filename, const wchar_t* searchPath );
        HRESULT InitDebugInfo( IDiaSession* session );
        void CloseDebugInfo();
        IDiaSession* getSession() const { return mSession; }

        // symbols

        virtual HRESULT SetSymbolScope( SymbolHeapId heapId, SymbolScope& scope );
        virtual HRESULT SetCompilandSymbolScope( DWORD compilandIndex, SymbolScope& scope );
        virtual HRESULT SetChildSymbolScope( SymHandle handle, SymbolScope& scope );

        virtual bool NextSymbol( SymbolScope& scope, SymHandle& handle, DWORD addr );
        virtual HRESULT EndSymbolScope( SymbolScope& scope );

        virtual HRESULT FindFirstSymbol( SymbolHeapId heapId, const char* nameChars, size_t nameLen, EnumNamedSymbolsData& data );
        virtual HRESULT FindNextSymbol( EnumNamedSymbolsData& handle );
        virtual HRESULT GetCurrentSymbol( const EnumNamedSymbolsData& searchHandle, SymHandle& handle );
        virtual HRESULT FindSymbolDone( EnumNamedSymbolsData& handle );

        virtual HRESULT FindSymbol( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle, DWORD& symOff );

        virtual HRESULT GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo );

        bool IsTLSData( SymHandleIn& internalHandle );

        // types

        virtual HRESULT SetGlobalTypeScope( TypeScope& scope );
        virtual HRESULT SetChildTypeScope( TypeHandle handle, TypeScope& scope );

        virtual bool NextType( TypeScope& scope, TypeHandle& handle );
        virtual HRESULT EndTypeScope( TypeScope& scope );

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

        virtual bool    FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber );
        virtual bool    FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber );
        virtual bool    FindNextLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber );

        virtual bool    FindLines( bool exactMatch, const char* fileName, size_t fileNameLen, uint16_t reqLineStart, uint16_t reqLineEnd, 
                                   std::list<LineNumber>& );

    private:
        void releaseFindLineEnumLineNumbers();
        HRESULT fillFileSegmentInfo( IDiaEnumLineNumbers *pEnumLineNumbers, FileSegmentInfo& segInfo );
        HRESULT findCompilandAndFile( IDiaSymbol *pCompiland, IDiaSourceFile *pSourceFile, uint16_t& compIndex, uint16_t& fileIndex );
        HRESULT setLineNumber( IDiaLineNumber* pLineNumber, uint16_t lineIndex, LineNumber& lineNumber );
        uint32_t getCompilandCount();
        HRESULT initSession();

        HRESULT _symbolById( DWORD id, IDiaSymbol** pSymbol );

        bool mInit;
        IDiaDataSource  *mSource;
        IDiaSession     *mSession;
        IDiaSymbol      *mGlobal;
        IDiaEnumLineNumbers *mFindLineEnumLineNumbers;

        DWORD            mMachineType;
        long             mCompilandCount;

        std::unique_ptr<char>  mLastCompilandName;
        std::unique_ptr<char>  mLastFileInfoName;
        std::unique_ptr<DWORD> mLastSegInfoOffsets;
        std::unique_ptr<WORD>  mLastSegInfoLineNumbers;

        std::vector<RefPtr<IDiaSymbol>> mSymbolCache;
        std::map<DWORD, RefPtr<IDiaSymbol>> mSymbolCacheMap;

        friend class PDBSymbolInfo;
    };
}
