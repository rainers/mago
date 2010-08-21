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
struct PasString;
struct OMFSourceModule;
struct OMFSourceFile;


namespace MagoST
{
    typedef off_t offset_t;
    struct SymHandleIn;
    class ISymbolInfo;


    class DebugStore
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

        DWORD   mCompilandCount;
        boost::scoped_array<CompilandDetails>   mCompilandDetails;

    public:
        DebugStore();
        virtual ~DebugStore();

        HRESULT InitDebugInfo( BYTE* buffer, DWORD size );
        HRESULT InitDebugInfo();
        void CloseDebugInfo();

        // symbols

        HRESULT SetSymbolScope( SymbolHeapId heapId, SymbolScope& scope );
        HRESULT SetCompilandSymbolScope( DWORD compilandIndex, SymbolScope& scope );
        HRESULT SetChildSymbolScope( SymHandle handle, SymbolScope& scope );

        bool NextSymbol( SymbolScope& scope, SymHandle& handle );

        HRESULT FindFirstSymbol( SymbolHeapId heapId, const char* nameChars, BYTE nameLen, EnumNamedSymbolsData& data );
        HRESULT FindNextSymbol( EnumNamedSymbolsData& handle );
        HRESULT GetCurrentSymbol( const EnumNamedSymbolsData& searchHandle, SymHandle& handle );

        HRESULT FindSymbol( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle );

        HRESULT GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo );

        // types

        HRESULT SetGlobalTypeScope( TypeScope& scope );
        HRESULT SetChildTypeScope( TypeHandle handle, TypeScope& scope );

        bool NextType( TypeScope& scope, TypeHandle& handle );

        bool GetTypeFromTypeIndex( WORD typeIndex, TypeHandle& handle );

        HRESULT GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo );

        // source files

        HRESULT GetCompilandCount( uint32_t& count );
        HRESULT GetCompilandInfo( uint16_t index, CompilandInfo& info );
        HRESULT GetCompilandSegmentInfo( uint16_t index, uint16_t count, SegmentInfo* infos );
        HRESULT GetFileInfo( uint16_t compilandIndex, uint16_t fileIndex, FileInfo& info );
        HRESULT GetFileSegmentInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t count, SegmentInfo* infos );
        HRESULT GetLineInfo( uint16_t compIndex, uint16_t fileIndex, uint16_t segIndex, uint16_t count, LineInfo* infos);

        bool    FindCompilandFileSegment( WORD seg, DWORD offset, uint16_t& compIndex, uint16_t& fileIndex, FileSegmentInfo& segInfo );
        bool    FindCompilandFileSegment( uint16_t line, uint16_t compIndex, uint16_t fileIndex, FileSegmentInfo& segInfo );

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

        HRESULT FindFirstSymHashSymbol( const char* nameChars, BYTE nameLen, OMFDirEntry* entry, EnumNamedSymbolsData& data );
        HRESULT FindSymHashSymbol( WORD segment, DWORD offset, OMFDirEntry* entry, SymHandle& handle );

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
        bool SetFListContinuationScope( TypeIndex continuationIndex, TypeScopeIn* scopeIn );
        bool ValidateSymbol( SymHandleIn* internalHandle, DWORD symSize );
        bool ValidateNamedSymbol( 
            DWORD offset, 
            BYTE* heapBase, 
            OMFDirEntry* heapDir, 
            uint32_t hash, 
            const char* nameChars, 
            BYTE nameLen, 
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
    };
}
