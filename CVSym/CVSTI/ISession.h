/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoST
{
    enum SymbolHeapId;
    struct EnumNamedSymbolsData;
    struct SymHandle;
    struct TypeHandle;
    struct SymbolScope;
    struct TypeScope;
    struct SymInfoData;
    class ISymbolInfo;
    struct CompilandInfo;
    struct SegmentInfo;
    struct FileInfo;
    struct LineInfo;
    struct FileSegmentInfo;

    class ISession
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual uint64_t GetLoadAddress() = 0;
        virtual void SetLoadAddress( uint64_t va ) = 0;

        // mapping addresses

        virtual uint32_t GetRVAFromSecOffset( uint16_t secIndex, uint32_t offset ) = 0;
        virtual uint64_t GetVAFromSecOffset( uint16_t secIndex, uint32_t offset ) = 0;
        virtual uint16_t GetSecOffsetFromRVA( uint32_t rva, uint32_t& offset ) = 0;
        virtual uint16_t GetSecOffsetFromVA( uint64_t va, uint32_t& offset ) = 0;

        // symbols

        virtual void CopySymbolInfo( const SymInfoData& sourceInfo, SymInfoData& destInfo, ISymbolInfo*& symInfo ) = 0;

        virtual HRESULT GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo ) = 0;

        virtual HRESULT FindFirstSymbol( 
            SymbolHeapId heapId, 
            const char* nameChars, 
            size_t nameLen, 
            EnumNamedSymbolsData& data ) = 0;
        virtual HRESULT FindNextSymbol( EnumNamedSymbolsData& handle ) = 0;
        virtual HRESULT GetCurrentSymbol( const EnumNamedSymbolsData& searchHandle, SymHandle& handle ) = 0;

        virtual HRESULT FindChildSymbol( 
            SymHandle parentHandle, 
            const char* nameChars, 
            size_t nameLen, 
            SymHandle& handle ) = 0;

        virtual HRESULT FindOuterSymbolByAddr( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle ) = 0;
        virtual HRESULT FindOuterSymbolByRVA( SymbolHeapId heapId, DWORD rva, SymHandle& handle ) = 0;
        virtual HRESULT FindOuterSymbolByVA( SymbolHeapId heapId, DWORD64 va, SymHandle& handle ) = 0;
        virtual HRESULT FindInnermostSymbol( SymHandle parentHandle, WORD segment, DWORD offset, std::vector<SymHandle>& handle ) = 0;

        virtual HRESULT SetChildSymbolScope( SymHandle handle, SymbolScope& scope ) = 0;

        virtual bool NextSymbol( SymbolScope& scope, SymHandle& handle ) = 0;

        // types

        virtual HRESULT GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo ) = 0;

        virtual HRESULT SetChildTypeScope( TypeHandle handle, TypeScope& scope ) = 0;

        virtual bool NextType( TypeScope& scope, TypeHandle& handle ) = 0;

        virtual bool GetTypeFromTypeIndex( TypeIndex typeIndex, TypeHandle& handle ) = 0;

        virtual HRESULT FindChildType( 
            TypeHandle parentHandle, 
            const char* nameChars, 
            size_t nameLen, 
            TypeHandle& handle ) = 0;

        // source files

        virtual HRESULT GetCompilandCount( uint32_t& count ) = 0;
        virtual HRESULT GetCompilandInfo( uint16_t index, CompilandInfo& info ) = 0;
        virtual HRESULT GetCompilandSegmentInfo( uint16_t index, uint16_t count, SegmentInfo* infos ) = 0;
        virtual HRESULT GetFileInfo( uint16_t compilandIndex, uint16_t fileIndex, FileInfo& info ) = 0;
        virtual HRESULT GetFileSegmentInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t count, SegmentInfo* infos ) = 0;
        virtual HRESULT GetLineInfo( uint16_t compIndex, uint16_t fileIndex, uint16_t segIndex, uint16_t count, LineInfo* infos) = 0;

        virtual bool GetFileSegment( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, FileSegmentInfo& segInfo ) = 0;

        virtual bool FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber ) = 0;
        virtual bool FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber) = 0;
        virtual bool FindNextLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber ) = 0;

        virtual bool FindLines( bool exactMatch, const char* fileName, size_t fileNameLen, uint16_t reqLineStart, uint16_t reqLineEnd, 
                                std::list<LineNumber>& lines ) = 0;

    };
}
