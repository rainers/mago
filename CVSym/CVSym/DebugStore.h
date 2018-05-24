/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


struct OMFDirEntry;
struct OMFDirHeader;
union CodeViewSymbol;
union CodeViewType;
union CodeViewFieldType;
struct OMFSourceModule;
struct OMFSourceFile;


namespace MagoST
{
    typedef off_t offset_t;
    struct SymHandleIn;
    class ISymbolInfo;

    class IDebugStore
    {
    public:
        virtual ~IDebugStore() {}

        // symbols

        virtual HRESULT SetSymbolScope( SymbolHeapId heapId, SymbolScope& scope ) = 0;
        virtual HRESULT SetCompilandSymbolScope( DWORD compilandIndex, SymbolScope& scope ) = 0;
        virtual HRESULT SetChildSymbolScope( SymHandle handle, SymbolScope& scope ) = 0;

        virtual bool NextSymbol( SymbolScope& scope, SymHandle& handle, DWORD addr ) = 0;

        virtual HRESULT FindFirstSymbol( SymbolHeapId heapId, const char* nameChars, size_t nameLen, EnumNamedSymbolsData& data ) = 0;
        virtual HRESULT FindNextSymbol( EnumNamedSymbolsData& handle ) = 0;
        virtual HRESULT GetCurrentSymbol( const EnumNamedSymbolsData& searchHandle, SymHandle& handle ) = 0;

        virtual HRESULT FindSymbol( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle, DWORD& symOff ) = 0;

        virtual HRESULT GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo ) = 0;

        // types

        virtual HRESULT SetGlobalTypeScope( TypeScope& scope ) = 0;
        virtual HRESULT SetChildTypeScope( TypeHandle handle, TypeScope& scope ) = 0;

        virtual bool NextType( TypeScope& scope, TypeHandle& handle ) = 0;

        virtual bool GetTypeFromTypeIndex( TypeIndex typeIndex, TypeHandle& handle ) = 0;

        virtual HRESULT GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo ) = 0;

        // source files

        virtual HRESULT GetCompilandCount( uint32_t& count ) = 0;
        virtual HRESULT GetCompilandInfo( uint16_t index, CompilandInfo& info ) = 0;
        virtual HRESULT GetCompilandSegmentInfo( uint16_t index, uint16_t count, SegmentInfo* infos ) = 0;
        virtual HRESULT GetFileInfo( uint16_t compilandIndex, uint16_t fileIndex, FileInfo& info ) = 0;
        virtual HRESULT GetFileSegmentInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t count, SegmentInfo* infos ) = 0;
        virtual HRESULT GetLineInfo( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, uint16_t count, LineInfo* infos) = 0;

        virtual bool    GetFileSegment( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, FileSegmentInfo& segInfo ) = 0;

        virtual bool    FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber ) = 0;
        virtual bool    FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber ) = 0;
        virtual bool    FindNextLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber ) = 0;

        virtual bool    FindLines( bool exactMatch, const char* fileName, size_t fileNameLen, uint16_t reqLineStart, uint16_t reqLineEnd, 
                                   std::list<LineNumber>& lines ) = 0;
    };

    class DebugStore : public IDebugStore
    {
        typedef bool (DebugStore::*NextTypeFunc)( TypeScope& scope, TypeHandle& handle );

        struct CompilandDetails
        {
            OMFDirEntry*    SymbolEntry;
            OMFDirEntry*    SourceEntry;
        };

        struct SymbolScopeIn
        {
            OMFDirEntry*        Dir;
            BYTE*               HeapBase;
            BYTE*               StartPtr;
            BYTE*               CurPtr;
            BYTE*               Limit;
        };

        struct TypeScopeIn
        {
            BYTE*           StartPtr;
            BYTE*           CurPtr;
            BYTE*           Limit;
            NextTypeFunc    NextType;
            WORD            CurZIndex;
            WORD            TypeCount;
        };

        bool    mInit;
        BYTE*   mCVBuf;
        DWORD   mCVBufSize;

        OMFDirHeader*   mDirHeader;
        OMFDirEntry*    mDirs;
        OMFDirEntry*    mGlobalTypesDir;
        OMFDirEntry*    mSymsDir[SymHeap_Count];

        WORD    mTLSSegment;
        DWORD   mCompilandCount;
        UniquePtr<CompilandDetails[]>   mCompilandDetails;

        uint16_t mTextSegment;
        std::vector<bool> mMarkOffsets;

    public:
        DebugStore();
        virtual ~DebugStore();

        HRESULT InitDebugInfo( BYTE* buffer, DWORD size );
        HRESULT InitDebugInfo();
        void CloseDebugInfo();

        void SetTLSSegment( WORD seg );
        void SetTextSegment( WORD seg );

        // symbols

        virtual HRESULT SetSymbolScope( SymbolHeapId heapId, SymbolScope& scope );
        virtual HRESULT SetCompilandSymbolScope( DWORD compilandIndex, SymbolScope& scope );
        virtual HRESULT SetChildSymbolScope( SymHandle handle, SymbolScope& scope );

        virtual bool NextSymbol( SymbolScope& scope, SymHandle& handle, DWORD addr );

        virtual HRESULT FindFirstSymbol( SymbolHeapId heapId, const char* nameChars, size_t nameLen, EnumNamedSymbolsData& data );
        virtual HRESULT FindNextSymbol( EnumNamedSymbolsData& handle );
        virtual HRESULT GetCurrentSymbol( const EnumNamedSymbolsData& searchHandle, SymHandle& handle );

        virtual HRESULT FindSymbol( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle, DWORD& symOff );

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

        virtual bool    FindLine( WORD seg, uint32_t offset, LineNumber& lineNumber );
        virtual bool    FindLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber );
        virtual bool    FindNextLineByNum( uint16_t compIndex, uint16_t fileIndex, uint16_t line, LineNumber& lineNumber );

        virtual bool    FindLines( bool exactMatch, const char* fileName, size_t fileNameLen, uint16_t reqLineStart, uint16_t reqLineEnd, 
                                   std::list<LineNumber>& lines );

        // for debugging
        HRESULT GetSymbolBytePtr( SymHandle handle, BYTE* bytes, DWORD& size );
        HRESULT GetTypeBytePtr( TypeHandle handle, BYTE* bytes, DWORD& size );

    protected:
        HRESULT SetCVBuffer( BYTE* buffer, DWORD size );

    private:
        template <class T>
        T* GetCVPtr( offset_t offset );

        template <class T>
        T* GetCVPtr( offset_t offset, DWORD size );

        bool ValidateCVPtr( void* p, DWORD size );

        HRESULT ProcessDirEntry( OMFDirEntry* entry );

        HRESULT SetCompilandSymbolScope( OMFDirEntry* entry, SymbolScope& scope );
        HRESULT SetSymHashScope( OMFDirEntry* entry, SymbolScope& scope );
        HRESULT SetSymbolScopeForDirEntry( OMFDirEntry* entry, SymbolScope& scope );

        HRESULT FindFirstSymHashSymbol( const char* nameChars, size_t nameLen, OMFDirEntry* entry, EnumNamedSymbolsData& data );
        HRESULT FindSymHashSymbol( WORD segment, DWORD offset, OMFDirEntry* entry, SymHandle& handle, DWORD& symOff );

        HRESULT GetSymbolHeapBaseForDirEntry( OMFDirEntry* entry, BYTE*& heapBase );
        HRESULT GetSymHashSymbolHeapBase( OMFDirEntry* entry, BYTE*& heapBase );
        HRESULT GetCompilandSymbolHeapBase( OMFDirEntry* entry, BYTE*& heapBase );

        bool FollowReference( CodeViewSymbol* origSym, SymHandleIn* internalHandle );

        bool NextTypeGlobal( TypeScope& scope, TypeHandle& handle );
        bool NextTypeFList( TypeScope& scope, TypeHandle& handle );
        bool NextTypeAList( TypeScope& scope, TypeHandle& handle );
        bool NextTypeMList( TypeScope& scope, TypeHandle& handle );
        bool NextTypeDList( TypeScope& scope, TypeHandle& handle );

        CodeViewType* GetTypeFromTypeIndex( uint16_t typeIndex );
        bool ValidateField( TypeScopeIn* scopeIn );
        bool SetFListContinuationScope( WORD continuationIndex, TypeScopeIn* scopeIn );
        bool ValidateSymbol( SymHandleIn* internalHandle, DWORD symSize );
        bool ValidateNamedSymbol( 
            DWORD offset, 
            BYTE* heapBase, 
            OMFDirEntry* heapDir, 
            uint32_t hash, 
            const char* nameChars, 
            size_t nameLen, 
            CodeViewSymbol*& newSymbol, 
            OMFDirEntry*& newHeapDir );

        bool FindFileSegment( 
            WORD seg, 
            DWORD offset, 
            uint16_t compIndex, 
            uint16_t& fileIndex, 
            FileSegmentInfo& segInfo );
        bool FindFileSegment( 
            WORD seg, 
            DWORD offset, 
            OMFSourceModule* srcMod, 
            OMFSourceFile* file, 
            FileSegmentInfo& segInfo );

        bool FindCompilandFileSegmentByOffset( WORD seg, DWORD offset, uint16_t& compIndex, uint16_t& fileIndex, FileSegmentInfo& segInfo );
        bool FindCompilandFileSegmentByLine( uint16_t line, uint16_t compIndex, uint16_t fileIndex, uint16_t firstSegIndex, FileSegmentInfo& segInfo );
        void SetLineNumberFromSegment( uint16_t compIx, uint16_t fileIx, const FileSegmentInfo& segInfo, uint16_t lineIndex, LineNumber& lineNumber );

        template <class TElem>
        bool BinarySearch( TElem targetKey, TElem* array, int arrayLen, int& indexFound );

        // patching line info
        bool MarkLineNumbers( OMFDirEntry* entry );
        bool MarkLineOffsetInBitmap( size_t adr );
        bool FixEndOffset( DWORD lastLineOffset, DWORD& off );
        bool PatchLineNumberInfo();
    };
}
