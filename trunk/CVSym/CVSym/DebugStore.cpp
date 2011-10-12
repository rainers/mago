/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DebugStore.h"
#include "OMFHashTable.h"
#include "OMFAddrTable.h"
#include "SymbolInfo.h"
#include "TypeInfo.h"
#include "Util.h"
#include "cvinfo.h"

using namespace std;

typedef pair<DWORD, DWORD> OffsetPair;


namespace MagoST
{
    DebugStore::DebugStore()
        :   mInit( false ),
            mCVBuf( NULL ),
            mCVBufSize( 0 ),
            mDirHeader( NULL ),
            mDirs( NULL ),
            mGlobalTypesDir( NULL ),
            mCompilandCount( 0 )
    {
        memset( mSymsDir, 0, sizeof mSymsDir );

        C_ASSERT( sizeof( SymbolScopeIn ) == sizeof( SymbolScope ) );
        C_ASSERT( sizeof( TypeScopeIn ) == sizeof( TypeScope ) );
        C_ASSERT( sizeof( EnumNamedSymbolsDataIn ) == sizeof( EnumNamedSymbolsData ) );
        C_ASSERT( sizeof( TypeHandleIn ) == sizeof( TypeHandle ) );
        C_ASSERT( sizeof( SymHandleIn ) == sizeof( SymHandle ) );
    }

    DebugStore::~DebugStore()
    {
    }

    HRESULT DebugStore::SetCVBuffer( BYTE* buffer, DWORD size )
    {
        if ( (buffer == NULL) || (size == 0) )
            return E_INVALIDARG;
        if ( (buffer + size) < buffer )             // can't wrap around
            return E_INVALIDARG;
        if ( (buffer + size + 0xFFFF) < buffer )    // we compare a lot with sym lengths which are 16-bit
            return E_INVALIDARG;
        if ( mInit )
            return E_ALREADY_INIT;

        mCVBuf = buffer;
        mCVBufSize = size;

        return S_OK;
    }

    HRESULT DebugStore::InitDebugInfo( BYTE* buffer, DWORD size )
    {
        HRESULT hr = S_OK;

        hr = SetCVBuffer( buffer, size );
        if ( FAILED( hr ) )
            return hr;

        hr = InitDebugInfo();
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT DebugStore::InitDebugInfo()
    {
        if ( (mCVBuf == NULL) || (mCVBufSize == 0) )
            return E_INVALIDARG;
        if ( mInit )
            return E_ALREADY_INIT;
        mInit = true;

        HRESULT         hr = S_OK;
        OMFSignature*   sig = GetCVPtr<OMFSignature>( 0 );
        OMFDirHeader*   dirHeader = NULL;

        if ( sig == NULL )
            return E_BAD_FORMAT;

        if ( memcmp( sig->Signature, "NB09", 4 ) != 0 )
            return E_BAD_FORMAT;

        dirHeader = GetCVPtr<OMFDirHeader>( sig->filepos );
        if ( dirHeader == NULL )
            return E_BAD_FORMAT;

        BYTE* dirStart = GetCVPtr<BYTE>( sig->filepos + dirHeader->cbDirHeader, dirHeader->cDir * dirHeader->cbDirEntry);
        BYTE* dir = dirStart;

        if ( dir == NULL )
            return E_BAD_FORMAT;

        for ( DWORD i = 0; i < dirHeader->cDir; i++ )
        {
            OMFDirEntry*    entry = (OMFDirEntry*) dir;

            hr = ProcessDirEntry( entry );
            if ( FAILED( hr ) )
                return hr;

            dir += dirHeader->cbDirEntry;
        }

        mDirHeader = (OMFDirHeader*) dirHeader;
        mDirs = (OMFDirEntry*) dirStart;

        return S_OK;
    }

    HRESULT DebugStore::ProcessDirEntry( OMFDirEntry* entry )
    {
        _ASSERT( entry != NULL );

        if ( NULL == GetCVPtr<void>( entry->lfo, entry->cb ) )
            return E_BAD_FORMAT;

        switch ( entry->SubSection )
        {
        case sstModule:
            _ASSERT( mCompilandDetails.get() == NULL );
            if ( mCompilandDetails.get() == NULL )
                mCompilandCount++;
            // else, ignore all the ones after this stage, because they shouldn't be here
            break;

        case sstAlignSym:
        case sstSrcModule:
            if ( mCompilandDetails.get() == NULL )
            {
                mCompilandDetails.reset( new CompilandDetails[ mCompilandCount ] );
                if ( mCompilandDetails.get() == NULL )
                    return E_OUTOFMEMORY;
                memset( mCompilandDetails.get(), 0, mCompilandCount * sizeof( CompilandDetails ) );
            }

            if ( (entry->iMod == 0) || (entry->iMod > mCompilandCount) )
                return E_BAD_FORMAT;

            // modules are 1 based
            if ( entry->SubSection == sstAlignSym )
                mCompilandDetails[ entry->iMod - 1 ].SymbolEntry = entry;
            else if ( entry->SubSection == sstSrcModule )
                mCompilandDetails[ entry->iMod - 1 ].SourceEntry = entry;
            else
                _ASSERT( false );
            break;

        case sstGlobalTypes:
            mGlobalTypesDir = entry;
            break;

        case sstGlobalPub:
            mSymsDir[ SymHeap_PublicSymbols ] = entry;
            break;

        case sstGlobalSym:
            mSymsDir[ SymHeap_GlobalSymbols ] = entry;
            break;

        case sstStaticSym:
            mSymsDir[ SymHeap_StaticSymbols ] = entry;
            break;
        }

        return S_OK;
    }

    void DebugStore::CloseDebugInfo()
    {
        mCVBuf = NULL;
        mCVBufSize = 0;
    }

    HRESULT DebugStore::SetCompilandSymbolScope( DWORD compilandIndex, SymbolScope& scope )
    {
        if ( (compilandIndex < 1) || (compilandIndex > mCompilandCount) )
            return E_INVALIDARG;

        OMFDirEntry*    entry = mCompilandDetails[ compilandIndex - 1 ].SymbolEntry;

        if ( entry == NULL )
            return E_FAIL;

        return SetCompilandSymbolScope( entry, scope );
    }

    HRESULT DebugStore::SetSymbolScope( SymbolHeapId heapId, SymbolScope& scope )
    {
        if ( heapId >= SymHeap_Max )
            return E_INVALIDARG;
        if ( mSymsDir[heapId] == NULL )
            return E_FAIL;

        return SetSymHashScope( mSymsDir[heapId], scope );
    }

    HRESULT DebugStore::SetChildSymbolScope( SymHandle handle, SymbolScope& scope )
    {
        HRESULT         hr = S_OK;
        SymHandleIn*    internalHandle = (SymHandleIn*) &handle;
        SymbolScopeIn*  scopeIn = (SymbolScopeIn*) &scope;

        // 12 bytes, because this is supposed to be a scope symbol that has a parent and end field
        if ( !ValidateSymbol( internalHandle, 12 ) )
            return E_INVALIDARG;

        CodeViewSymbol*  sym = internalHandle->Sym;
        BYTE*            firstChildPtr = (BYTE*) sym + sym->Generic.len + 2;

        if ( (sym->Generic.id != S_LPROC32) 
            && (sym->Generic.id != S_GPROC32) 
            && (sym->Generic.id != S_THUNK32) 
            && (sym->Generic.id != S_BLOCK32) 
            && (sym->Generic.id != S_WITH32) )
            return E_FAIL;

        hr = SetSymbolScopeForDirEntry( internalHandle->HeapDir, scope );
        if ( FAILED( hr ) )
            return hr;
        // now HeapDir and HeapBase are set

        scopeIn->StartPtr = firstChildPtr;
        scopeIn->CurPtr = scopeIn->StartPtr;
            // pEnd is at the same offset in all of these
        scopeIn->Limit = scopeIn->HeapBase + sym->block.end;

        // Limit is supposed to point at an S_END, so make sure it's in bounds
        if ( !ValidateCVPtr( scopeIn->Limit, 4 ) )
            return E_FAIL;

        // also make sure that it is an S_END
        CodeViewSymbol*     endSym = (CodeViewSymbol*) scopeIn->Limit;
        if ( endSym->Generic.id != S_END )
            return E_FAIL;

        return S_OK;
    }

    HRESULT DebugStore::SetSymbolScopeForDirEntry( OMFDirEntry* entry, SymbolScope& scope )
    {
        if ( entry->SubSection == sstAlignSym )
            return SetCompilandSymbolScope( entry, scope );
        else 
            return SetSymHashScope( entry, scope );
    }

    HRESULT DebugStore::SetSymHashScope( OMFDirEntry* entry, SymbolScope& scope )
    {
        SymbolScopeIn*  scopeIn = (SymbolScopeIn*) &scope;
        OMFSymHash*     symHash = GetCVPtr<OMFSymHash>( entry->lfo, entry->cb );
        BYTE*           symStart = (BYTE*) (symHash + 1);

        if ( (symHash == NULL) || !ValidateCVPtr( symStart, symHash->cbSymbol ) )
            return E_FAIL;

        scopeIn->Dir = entry;
        scopeIn->HeapBase = symStart;
        scopeIn->StartPtr = symStart;
        scopeIn->CurPtr = symStart;
        scopeIn->Limit = symStart + symHash->cbSymbol;

        return S_OK;
    }

    HRESULT DebugStore::SetCompilandSymbolScope( OMFDirEntry* entry, SymbolScope& scope )
    {
        SymbolScopeIn* scopeIn = (SymbolScopeIn*) &scope;

        scopeIn->Dir = entry;
        scopeIn->HeapBase = GetCVPtr<BYTE>( entry->lfo );
        scopeIn->StartPtr = scopeIn->HeapBase + 4;        // +4 to skip the signature
        scopeIn->CurPtr = scopeIn->StartPtr;
        scopeIn->Limit = GetCVPtr<BYTE>( entry->lfo ) + entry->cb;

        return S_OK;
    }

    HRESULT DebugStore::GetSymbolHeapBaseForDirEntry( OMFDirEntry* entry, BYTE*& heapBase )
    {
        if ( entry->SubSection == sstAlignSym )
            return GetCompilandSymbolHeapBase( entry, heapBase );
        else 
            return GetSymHashSymbolHeapBase( entry, heapBase );
    }

    HRESULT DebugStore::GetSymHashSymbolHeapBase( OMFDirEntry* entry, BYTE*& heapBase )
    {
        OMFSymHash* symHash = GetCVPtr<OMFSymHash>( entry->lfo, entry->cb );
        BYTE*       symStart = (BYTE*) (symHash + 1);

        if ( (symHash == NULL) || !ValidateCVPtr( symStart, symHash->cbSymbol ) )
            return E_FAIL;

        heapBase = symStart;
        return S_OK;
    }

    HRESULT DebugStore::GetCompilandSymbolHeapBase( OMFDirEntry* entry, BYTE*& heapBase )
    {
        heapBase = GetCVPtr<BYTE>( entry->lfo );
        return S_OK;
    }

    bool DebugStore::NextSymbol( SymbolScope& scope, SymHandle& handle )
    {
        SymbolScopeIn* scopeIn = (SymbolScopeIn*) &scope;

        // do we have at least a length and tag field?
        if ( scopeIn->CurPtr + 3 >= scopeIn->Limit )
            return false;

        // get the current value

        SymHandleIn*        internalHandle = (SymHandleIn*) &handle;
        CodeViewSymbol*     sym = (CodeViewSymbol*) scopeIn->CurPtr;

        if ( sym->Generic.id == 0 )
            return false;
        if ( !ValidateCVPtr( sym, sym->Generic.len + 2 ) )
            return false;
        // at this point we assume that the rest of the record is in bounds

        if ( !FollowReference( sym, internalHandle ) )
        {
            internalHandle->Sym = sym;
            internalHandle->HeapDir = scopeIn->Dir;
        }

        // move to the next one for next time

        if ( (sym->Generic.id == S_LPROC32) 
            || (sym->Generic.id == S_GPROC32) 
            || (sym->Generic.id == S_THUNK32) 
            || (sym->Generic.id == S_BLOCK32) 
            || (sym->Generic.id == S_WITH32) )
        {
            // pEnd is at the same offset in all of these
            scopeIn->CurPtr = scopeIn->HeapBase + sym->block.end;

            // we're pointing at the S_END
        }
        else
        {
            scopeIn->CurPtr += sym->Generic.len + 2;
        }

        return true;
    }

    int DebugStore::ModuleSize( CodeViewSymbol* origSym )
    {
        if ( (origSym->Generic.id == S_PROCREF) || (origSym->Generic.id == S_DATAREF) )
        {
            DWORD   compilandIndex = origSym->symref.imod;

            if ( (compilandIndex >= 1) && (compilandIndex <= mCompilandCount) )
            {
                OMFDirEntry*    entry = mCompilandDetails[ compilandIndex - 1 ].SymbolEntry;
                return entry->cb;
            }
        }
        return -1;
    }

    bool DebugStore::FollowReference( CodeViewSymbol* origSym, SymHandleIn* internalHandle )
    {
        // try to chase down the reference
        if ( (origSym->Generic.id == S_PROCREF) || (origSym->Generic.id == S_DATAREF) )
        {
            DWORD   compilandIndex = origSym->symref.imod;

            if ( (compilandIndex >= 1) && (compilandIndex <= mCompilandCount) )
            {
                OMFDirEntry*    entry = mCompilandDetails[ compilandIndex - 1 ].SymbolEntry;

                if ( entry != NULL )
                {
                    CodeViewSymbol* pointedSym = GetCVPtr<CodeViewSymbol>( entry->lfo + origSym->symref.ibSym, 4 );

                    if ( pointedSym != NULL )
                    {
                        internalHandle->HeapDir = entry;
                        internalHandle->Sym = pointedSym;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    HRESULT DebugStore::FindFirstSymbol( SymbolHeapId heapId, const char* nameChars, size_t nameLen, EnumNamedSymbolsData& data )
    {
        if ( heapId >= SymHeap_Max )
            return E_INVALIDARG;
        if ( mSymsDir[heapId] == NULL )
            return E_FAIL;

        return FindFirstSymHashSymbol( nameChars, nameLen, mSymsDir[heapId], data );
    }

    bool DebugStore::ValidateNamedSymbol( 
        DWORD offset, 
        BYTE* heapBase, 
        OMFDirEntry* heapDir,
        uint32_t hash, 
        const char* nameChars, 
        size_t nameLen, 
        CodeViewSymbol*& newSymbol, 
        OMFDirEntry*& newHeapDir )
    {
        BYTE*       symPtr = heapBase + offset;

        if ( !ValidateCVPtr( symPtr, 4 ) )
            return false;

        CodeViewSymbol*     sym = (CodeViewSymbol*) symPtr;
        SymHandleIn         internalHandle = { 0 };

        if ( (sym->Generic.id == S_PROCREF) || (sym->Generic.id == S_DATAREF) )
        {
            if ( sym->symref.sumName != hash )
                return false;
        }

        if ( FollowReference( sym, &internalHandle ) )
        {
            sym = internalHandle.Sym;
            heapDir = internalHandle.HeapDir;
        }

        PasString*      pstrName = NULL;

        if ( !QuickGetName( sym, pstrName ) )
            return false;
        if ( nameLen != pstrName->GetLength() )
            return false;
        if ( memcmp( nameChars, pstrName->GetName(), nameLen ) != 0 )
            return false;

        newSymbol = sym;
        newHeapDir = heapDir;
        return true;
    }

    HRESULT DebugStore::FindFirstSymHashSymbol( const char* nameChars, size_t nameLen, OMFDirEntry* entry, EnumNamedSymbolsData& data )
    {
        EnumNamedSymbolsDataIn* internalData = (EnumNamedSymbolsDataIn*) &data;
        OMFSymHash*             symHash = GetCVPtr<OMFSymHash>( entry->lfo );

        if ( (symHash == NULL) || (symHash->symhash != 0xA) )
            return E_FAIL;

        BYTE*   heapBase = (BYTE*) (symHash + 1);
        BYTE*   tablePtr = heapBase + symHash->cbSymbol;

        if ( !ValidateCVPtr( tablePtr, symHash->cbHSym ) )
            return E_FAIL;

        OMFHashTable    table( tablePtr );
        uint32_t        hash = OMFHashTable::GetSymbolNameHash( nameChars, nameLen );
        OffsetPair*     pairs = NULL;
        uint32_t        pairCount = 0;

        if ( !table.GetSymbolOffsetIter( hash, pairs, pairCount ) )
            return E_FAIL;

        for ( uint32_t i = 0; i < pairCount; i++ )
        {
            if ( pairs[i].second == hash )
            {
                offset_t            offset = pairs[i].first;
                CodeViewSymbol*     sym = NULL;
                OMFDirEntry*        newDir = NULL;

                if ( !ValidateNamedSymbol( offset, heapBase, entry, hash, nameChars, nameLen, sym, newDir ) )
                    continue;

                internalData->NameLen = nameLen;
                internalData->NameChars = nameChars;
                internalData->Sym = sym;
                internalData->SymHeapDir = newDir;

                internalData->Hash = hash;
                internalData->CurPair = &pairs[i];
                internalData->LimitPair = pairs + pairCount;
                internalData->HeapBase = heapBase;
                internalData->HeapDir = entry;

                return S_OK;
            }
        }

        return S_FALSE;
    }

    HRESULT DebugStore::FindNextSymbol( EnumNamedSymbolsData& handle )
    {
        EnumNamedSymbolsDataIn* internalData = (EnumNamedSymbolsDataIn*) &handle;

        if ( (internalData->CurPair == NULL) || (internalData->LimitPair == NULL)
            || (internalData->HeapBase == NULL) || (internalData->HeapDir == NULL) )
            return E_INVALIDARG;

        OffsetPair* curPair = (OffsetPair*) internalData->CurPair;
        OffsetPair* limitPair = (OffsetPair*) internalData->LimitPair;

        for ( curPair++; curPair < limitPair; curPair++ )
        {
            if ( curPair->second == internalData->Hash )
            {
                offset_t            offset = curPair->first;
                CodeViewSymbol*     sym = NULL;
                OMFDirEntry*        newDir = NULL;

                if ( !ValidateNamedSymbol( 
                    offset, 
                    internalData->HeapBase, 
                    internalData->HeapDir,
                    internalData->Hash, 
                    internalData->NameChars, 
                    internalData->NameLen, 
                    sym,
                    newDir ) )
                    continue;

                internalData->Sym = sym;
                internalData->SymHeapDir = newDir;

                internalData->CurPair = curPair;
                return S_OK;
            }
        }

        return S_FALSE;
    }

    HRESULT DebugStore::GetCurrentSymbol( const EnumNamedSymbolsData& searchData, SymHandle& handle )
    {
        EnumNamedSymbolsDataIn* internalSearchData = (EnumNamedSymbolsDataIn*) &searchData;
        SymHandleIn*            internalHandle = (SymHandleIn*) &handle;

        if ( (internalSearchData->Sym == NULL) || (internalSearchData->SymHeapDir == NULL) )
            return E_FAIL;

        internalHandle->Sym = internalSearchData->Sym;
        internalHandle->HeapDir = internalSearchData->SymHeapDir;

        return S_OK;
    }

    HRESULT DebugStore::FindSymbol( SymbolHeapId heapId, WORD segment, DWORD offset, SymHandle& handle )
    {
        if ( heapId >= SymHeap_Max )
            return E_INVALIDARG;
        if ( mSymsDir[heapId] == NULL )
            return E_FAIL;

        return FindSymHashSymbol( segment, offset, mSymsDir[heapId], handle );
    }

    HRESULT DebugStore::FindSymHashSymbol( WORD segment, DWORD offset, OMFDirEntry* entry, SymHandle& handle )
    {
        SymHandleIn*    internalHandle = (SymHandleIn*) &handle;
        OMFSymHash*     symHash = GetCVPtr<OMFSymHash>( entry->lfo );

        if ( (symHash == NULL) || (symHash->addrhash != 0xC) )
            return E_FAIL;

        BYTE*   tablePtr = (BYTE*) (symHash + 1) + symHash->cbSymbol + symHash->cbHSym;

        if ( !ValidateCVPtr( tablePtr, symHash->cbHAddr ) )
            return E_FAIL;

        OMFAddrTable        table( tablePtr );
        uint32_t            symOffset = 0;

        if ( !table.GetSymbolOffset( segment, offset, symOffset ) )
            return E_FAIL;

        BYTE*   symPtr = (BYTE*) (symHash + 1) + symOffset;

        if ( !ValidateCVPtr( symPtr, 4 ) )
            return E_FAIL;

        CodeViewSymbol*     sym = (CodeViewSymbol*) symPtr;

        if ( FollowReference( sym, internalHandle ) )
        {
            // validate the offset is not out of bounds of the module
            if( (int) offset >= ModuleSize( sym ) )
                return E_FAIL;
            return S_OK;
        }

        internalHandle->Sym = sym;
        internalHandle->HeapDir = entry;

        return S_OK;
    }

    HRESULT DebugStore::GetSymbolInfo( SymHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo )
    {
        SymHandleIn*    internalHandle = (SymHandleIn*) &handle;

        // 4 bytes, at least as much as the generic symbol node: length and tag
        if ( !ValidateSymbol( internalHandle, 4 ) )
            return E_INVALIDARG;

        switch ( internalHandle->Sym->Generic.id )
        {
        case S_REGISTER:    symInfo = new (&privateData) RegSymbol( *internalHandle ); break;
        case S_CONSTANT:    symInfo = new (&privateData) ConstSymbol( *internalHandle ); break;
        case S_MANYREG:     symInfo = new (&privateData) ManyRegsSymbol( *internalHandle ); break;
        case S_BPREL32:     symInfo = new (&privateData) BPRelSymbol( *internalHandle ); break;
        case S_LDATA32:
        case S_GDATA32:     symInfo = new (&privateData) DataSymbol( *internalHandle ); break;
        case S_PUB32:       symInfo = new (&privateData) PublicSymbol( *internalHandle ); break;
        case S_LPROC32:
        case S_GPROC32:     symInfo = new (&privateData) ProcSymbol( *internalHandle ); break;
        case S_THUNK32:     symInfo = new (&privateData) ThunkSymbol( *internalHandle ); break;
        case S_BLOCK32:     symInfo = new (&privateData) BlockSymbol( *internalHandle ); break;
        case S_LABEL32:     symInfo = new (&privateData) LabelSymbol( *internalHandle ); break;
        case S_REGREL32:    symInfo = new (&privateData) RegRelSymbol( *internalHandle ); break;
        case S_LTHREAD32:
        case S_GTHREAD32:   symInfo = new (&privateData) TLSSymbol( *internalHandle ); break;
        case S_UDT:         symInfo = new (&privateData) UdtSymbol( *internalHandle ); break;
        case S_ENDARG:      symInfo = new (&privateData) EndOfArgsSymbol( *internalHandle ); break;
        default:
            return E_FAIL;
        }

        return S_OK;
    }

    HRESULT DebugStore::SetGlobalTypeScope( TypeScope& scope )
    {
        TypeScopeIn*    scopeIn = (TypeScopeIn*) &scope;
        OMFGlobalTypes* globalTypes = GetCVPtr<OMFGlobalTypes>( mGlobalTypesDir->lfo );

        if ( globalTypes == NULL )
            return E_FAIL;

        DWORD*  offsetTable = (DWORD*) (globalTypes + 1);

        if ( !ValidateCVPtr( offsetTable, globalTypes->cTypes * sizeof( DWORD ) ) )
            return E_FAIL;

        if ( globalTypes->cTypes == 0 )
            return E_FAIL;

        DWORD   typeBase = mGlobalTypesDir->lfo + sizeof( OMFGlobalTypes ) + (globalTypes->cTypes * sizeof( DWORD ));
        DWORD   firstTypeOffset = typeBase + offsetTable[0];
        DWORD   lastTypeOffset = typeBase + offsetTable[ globalTypes->cTypes - 1 ];
        BYTE*   lastTypePtr = GetCVPtr<BYTE>( lastTypeOffset, 4 );

        if ( NULL == lastTypePtr )
            return E_FAIL;

        scopeIn->TypeCount = (WORD) globalTypes->cTypes;
        scopeIn->StartPtr = GetCVPtr<BYTE>( firstTypeOffset, 4 );
        scopeIn->CurPtr = scopeIn->StartPtr;
        scopeIn->CurZIndex = 0;
        scopeIn->NextType = &DebugStore::NextTypeGlobal;

        scopeIn->Limit = GetCVPtr<BYTE>( lastTypeOffset + 2 + ((CodeViewType*) lastTypePtr)->Generic.len );

        return S_OK;
    }

    CodeViewType* DebugStore::GetTypeFromTypeIndex( uint16_t typeIndex )
    {
        if ( typeIndex < 0x1000 )
            return NULL;

        OMFGlobalTypes* globalTypes = GetCVPtr<OMFGlobalTypes>( mGlobalTypesDir->lfo );

        if ( globalTypes == NULL )
            return NULL;

        DWORD*  offsetTable = (DWORD*) (globalTypes + 1);
        DWORD   typeBase = mGlobalTypesDir->lfo + sizeof( OMFGlobalTypes ) + (globalTypes->cTypes * sizeof( DWORD ));
        DWORD   index = typeIndex - 0x1000;

        if ( index >= globalTypes->cTypes )
            return NULL;

        return GetCVPtr<CodeViewType>( typeBase + offsetTable[index], 4 );
    }

    bool DebugStore::GetTypeFromTypeIndex( WORD typeIndex, TypeHandle& handle )
    {
        TypeHandleIn*   internalHandle = (TypeHandleIn*) &handle;

        if ( typeIndex == 0 )
            return false;

        if ( typeIndex < 0x1000 )
        {
            internalHandle->Index = typeIndex;
            internalHandle->Type = NULL;
            internalHandle->Tag = 0;
            return true;
        }

        CodeViewType*   type = GetTypeFromTypeIndex( typeIndex );

        if ( type == NULL )
            return false;

        internalHandle->Index = typeIndex;
        internalHandle->Type = (BYTE*) type;
        internalHandle->Tag = type->Generic.id;

        return true;
    }

    HRESULT DebugStore::SetChildTypeScope( TypeHandle handle, TypeScope& scope )
    {
        TypeScopeIn*    scopeIn = (TypeScopeIn*) &scope;
        TypeHandleIn*   internalHandle = (TypeHandleIn*) &handle;
        OMFGlobalTypes* globalTypes = GetCVPtr<OMFGlobalTypes>( mGlobalTypesDir->lfo );

        if ( globalTypes == NULL )
            return E_FAIL;

        // it doesn't have to have a valid index, if it came from a field list
        // it has to have a valid tag, which will be checked below
        if ( internalHandle->Type == NULL )
            return E_INVALIDARG;

        // 4 bytes, at least as much as the generic symbol node: length and tag
        if ( !ValidateCVPtr( internalHandle->Type, 4 ) )
            return E_INVALIDARG;

        switch ( internalHandle->Tag )
        {
        case LF_FIELDLIST:
            {
                CodeViewType*     type = (CodeViewType*) internalHandle->Type;

                scopeIn->TypeCount = 0;
                scopeIn->StartPtr = internalHandle->Type + 4;
                scopeIn->CurPtr = scopeIn->StartPtr;
                scopeIn->CurZIndex = 0;
                scopeIn->NextType = &DebugStore::NextTypeFList;
                scopeIn->Limit = internalHandle->Type + type->Generic.len + 2;
            }
            break;

        case LF_METHOD:
            {
                CodeViewFieldType*  fieldtype = (CodeViewFieldType*) internalHandle->Type;
                BYTE*               mlist = (BYTE*) GetTypeFromTypeIndex( fieldtype->method.mlist );

                // TODO: check that mlist is really a LF_METHODLIST
                if ( mlist == NULL )
                    return E_FAIL;

                scopeIn->TypeCount = fieldtype->method.count;
                scopeIn->StartPtr = mlist + 4;            // after len and id
                scopeIn->CurPtr = scopeIn->StartPtr;
                scopeIn->CurZIndex = 0;
                scopeIn->NextType = &DebugStore::NextTypeMList;
                scopeIn->Limit = NULL;
            }
            break;

        default:
            return E_FAIL;
        }

        return S_OK;
    }

    bool DebugStore::NextType( TypeScope& scope, TypeHandle& handle )
    {
        TypeScopeIn* scopeIn = (TypeScopeIn*) &scope;

        if ( scopeIn->NextType == NULL )
            return false;

        return (this->*scopeIn->NextType)( scope, handle );
    }

    bool DebugStore::NextTypeMList( TypeScope& scope, TypeHandle& handle )
    {
        TypeScopeIn* scopeIn = (TypeScopeIn*) &scope;

        if ( scopeIn->CurZIndex >= scopeIn->TypeCount )
            return false;

        TypeHandleIn*       internalHandle = (TypeHandleIn*) &handle;

        internalHandle->Index = 0xFFFF;
        internalHandle->Type = scopeIn->CurPtr;
        internalHandle->Tag = LF_MAGO_METHODOVERLOAD;

        scopeIn->CurZIndex++;

        CV_fldattr_t*   attr = (CV_fldattr_t*) scopeIn->CurPtr;
        if ( (attr->mprop == CV_MTintro) || (attr->mprop == CV_MTpureintro) )
            scopeIn->CurPtr += 4;     // the optional vtab offset

        scopeIn->CurPtr += 4;         // the attribute and type

        return true;
    }

    bool DebugStore::SetFListContinuationScope( TypeIndex continuationIndex, TypeScopeIn* scopeIn )
    {
        CodeViewType*   contList = GetTypeFromTypeIndex( continuationIndex );

        if ( (contList == NULL) || (contList->Generic.id != LF_FIELDLIST) )
            return false;

        TypeHandleIn    contHandle = { 0 };

        contHandle.Index = continuationIndex;
        contHandle.Tag = contList->Generic.id;
        contHandle.Type = (BYTE*) contList;

        HRESULT hr = SetChildTypeScope( *(TypeHandle*) &contHandle, *(TypeScope*) scopeIn );
        if ( FAILED( hr ) )
            return false;

        return true;
    }

    bool DebugStore::NextTypeFList( TypeScope& scope, TypeHandle& handle )
    {
        TypeScopeIn*    scopeIn = (TypeScopeIn*) &scope;
        TypeHandleIn*   internalHandle = (TypeHandleIn*) &handle;

        if ( !ValidateField( scopeIn ) )
            return false;

        CodeViewFieldType*  type = (CodeViewFieldType*) scopeIn->CurPtr;

        if ( type->Generic.id == LF_INDEX )
        {
            if ( !SetFListContinuationScope( type->index.type, scopeIn ) )
                return false;

            // our internal scope pointer is still pointing at the user's scope arg
            _ASSERT( (void*) scopeIn == (void*) &scope );

            if ( !ValidateField( scopeIn ) )
                return false;

            type = (CodeViewFieldType*) scopeIn->CurPtr;

            if ( type->Generic.id == LF_INDEX )
            {
                // there's no reason to jump to an empty continuation record that jumps to another
                // cut the list short, so we only say we can't succeed if user calls us again
                scopeIn->Limit = scopeIn->CurPtr;
                return false;
            }
        }

        internalHandle->Type = (BYTE*) type;
        internalHandle->Index = 0xFFFF;
        internalHandle->Tag = type->Generic.id;

        // move to the next one for next time

        DWORD   len = GetFieldLength( type );

        scopeIn->CurPtr += len;
        scopeIn->CurZIndex++;

        return true;
    }

    bool DebugStore::NextTypeGlobal( TypeScope& scope, TypeHandle& handle )
    {
        TypeScopeIn* scopeIn = (TypeScopeIn*) &scope;

        // do we have at least a length and tag field?
        if ( scopeIn->CurPtr + 3 >= scopeIn->Limit )
            return false;

        // get the current value

        TypeHandleIn*       internalHandle = (TypeHandleIn*) &handle;
        CodeViewType*       type = (CodeViewType*) scopeIn->CurPtr;

        if ( type->Generic.id == 0 )
            return false;
        if ( !ValidateCVPtr( type, type->Generic.len + 2 ) )
            return false;
        // at this point we assume that the rest of the record is in bounds

        internalHandle->Type = (BYTE*) type;
        internalHandle->Index = 0x1000 + scopeIn->CurZIndex;
        internalHandle->Tag = type->Generic.id;

        // move to the next one for next time

        // the length field doesn't have to hold an aligned length
        scopeIn->CurPtr += (((type->Generic.len + 2) + 3) / 4) * 4;
        scopeIn->CurZIndex++;

        return true;
    }

    HRESULT DebugStore::GetTypeInfo( TypeHandle handle, SymInfoData& privateData, ISymbolInfo*& symInfo )
    {
        TypeHandleIn*   internalHandle = (TypeHandleIn*) &handle;
        uint16_t        mod = 0;

        if ( (internalHandle->Type == NULL) && (internalHandle->Index >= 0x1000) )
            return E_INVALIDARG;

        while ( internalHandle->Tag == LF_MODIFIER )
        {
            CodeViewType*   type = (CodeViewType*) internalHandle->Type;
            TypeIndex       newIndex = type->modifier.type;

            mod |= type->modifier.attr;

            type = GetTypeFromTypeIndex( newIndex );

            if ( newIndex >= 0x1000 )
            {
                if ( type == NULL )
                    return E_FAIL;

                internalHandle->Tag = type->Generic.id;
                internalHandle->Type = (BYTE*) type;
            }
            else
                internalHandle->Tag = 0;

            internalHandle->Index = newIndex;
        }

        switch ( internalHandle->Tag )
        {
        case LF_POINTER:      symInfo = new (&privateData) PointerTypeSymbol( *internalHandle ); break;
        case LF_ARRAY:        symInfo = new (&privateData) ArrayTypeSymbol( *internalHandle ); break;
        case LF_CLASS:        symInfo = new (&privateData) ClassTypeSymbol( *internalHandle ); break;
        case LF_STRUCTURE:    symInfo = new (&privateData) StructTypeSymbol( *internalHandle ); break;
        case LF_UNION:        symInfo = new (&privateData) UnionTypeSymbol( *internalHandle ); break;
        case LF_ENUM:         symInfo = new (&privateData) EnumTypeSymbol( *internalHandle ); break;
        case LF_PROCEDURE:    symInfo = new (&privateData) ProcTypeSymbol( *internalHandle ); break;
        case LF_MFUNCTION:    symInfo = new (&privateData) MProcTypeSymbol( *internalHandle ); break;
        case LF_VTSHAPE:      symInfo = new (&privateData) VTShapeTypeSymbol( *internalHandle ); break;
        case LF_OEM:          symInfo = new (&privateData) OEMTypeSymbol( *internalHandle ); break;

        case LF_BCLASS:       symInfo = new (&privateData) BaseClassTypeSymbol( *internalHandle ); break;
        case LF_ENUMERATE:    symInfo = new (&privateData) EnumMemberTypeSymbol( *internalHandle ); break;
        case LF_FRIENDFCN:    symInfo = new (&privateData) FriendFunctionTypeSymbol( *internalHandle ); break;
        case LF_MEMBER:       symInfo = new (&privateData) DataMemberTypeSymbol( *internalHandle ); break;
        case LF_STMEMBER:     symInfo = new (&privateData) StaticMemberTypeSymbol( *internalHandle ); break;
        case LF_METHOD:       symInfo = new (&privateData) MethodOverloadsTypeSymbol( *internalHandle ); break;
        case LF_NESTTYPE:     symInfo = new (&privateData) NestedTypeSymbol( *internalHandle ); break;
        case LF_VFUNCTAB:     symInfo = new (&privateData) VFTablePtrTypeSymbol( *internalHandle ); break;
        case LF_FRIENDCLS:    symInfo = new (&privateData) FriendClassTypeSymbol( *internalHandle ); break;
        case LF_ONEMETHOD:    symInfo = new (&privateData) MethodTypeSymbol( *internalHandle ); break;
        case LF_VFUNCOFF:     symInfo = new (&privateData) VFOffsetTypeSymbol( *internalHandle ); break;
        case LF_FIELDLIST:    symInfo = new (&privateData) FieldListTypeSymbol( *internalHandle ); break;
        case LF_ARGLIST:      symInfo = new (&privateData) TypeListTypeSymbol( *internalHandle ); break;
        case LF_DERIVED:      symInfo = new (&privateData) TypeListTypeSymbol( *internalHandle ); break;

        case LF_MAGO_METHODOVERLOAD:
            symInfo = new (&privateData) MListMethodTypeSymbol( *internalHandle );
            break;

        default:
            if ( internalHandle->Index >= 0x1000 )
                return E_FAIL;

            uint32_t    mode = CV_MODE( internalHandle->Index );
            if ( mode == 0 )
                symInfo = new (&privateData) BaseTypeSymbol( *internalHandle );
            else
                symInfo = new (&privateData) BasePointerTypeSymbol( *internalHandle );
            break;
        }

        TypeSymbol* typeSym = (TypeSymbol*) symInfo;

        typeSym->SetMod( mod );

        return S_OK;
    }

    HRESULT DebugStore::GetCompilandCount( uint32_t& count )
    {
        count = mCompilandCount;
        return S_OK;
    }

    HRESULT DebugStore::GetCompilandInfo( uint16_t index, CompilandInfo& info )
    {
        if ( (index < 1) || (index > mCompilandCount) )
            return E_INVALIDARG;

        const uint16_t      zIndex = index - 1;
        OMFDirEntry*        entry = &mDirs[zIndex];
        OMFModule*          mod = GetCVPtr<OMFModule>( entry->lfo );
        OMFSegDesc*         segDescTable = NULL;
        OMFSourceModule*    srcMod = NULL;

        if ( mod == NULL )
            return E_FAIL;

        segDescTable = (OMFSegDesc*) (mod + 1);
        info.Name = (PasString*) (segDescTable + mod->cSeg);

        if ( mCompilandDetails[zIndex].SourceEntry != NULL )
            srcMod = GetCVPtr<OMFSourceModule>( mCompilandDetails[zIndex].SourceEntry->lfo );

        if ( srcMod != NULL )
        {
            info.SegmentCount = srcMod->cSeg;
            info.FileCount = srcMod->cFile;
        }
        else
        {
            info.SegmentCount = mod->cSeg;
            info.FileCount = 0;
        }

        return S_OK;
    }

    HRESULT DebugStore::GetCompilandSegmentInfo( uint16_t index, uint16_t count, SegmentInfo* infos )
    {
        if ( infos == NULL )
            return E_INVALIDARG;
        if ( (index < 1) || (index > mCompilandCount) )
            return E_INVALIDARG;

        const uint16_t      zIndex = index - 1;
        OMFDirEntry*        entry = &mDirs[zIndex];
        OMFModule*          mod = GetCVPtr<OMFModule>( entry->lfo );
        OMFSegDesc*         segDescTable = NULL;
        OMFSourceModule*    srcMod = NULL;

        if ( mod == NULL )
            return E_FAIL;

        segDescTable = (OMFSegDesc*) (mod + 1);

        if ( mCompilandDetails[zIndex].SourceEntry != NULL )
            srcMod = GetCVPtr<OMFSourceModule>( mCompilandDetails[zIndex].SourceEntry->lfo );

        if ( srcMod != NULL )
        {
            DWORD*      srcFilePtrTable = (DWORD*) ((BYTE*) srcMod + 4);
            OffsetPair* offsetTable = (OffsetPair*) (srcFilePtrTable + srcMod->cFile);
            WORD*       segTable = (WORD*) (offsetTable + srcMod->cSeg);

            if ( count > srcMod->cSeg )
                count = srcMod->cSeg;

            for ( uint16_t i = 0; i < count; i++ )
            {
                infos[i].SegmentIndex = segTable[i];
                infos[i].StartOffset = offsetTable[i].first;
                infos[i].EndOffset = offsetTable[i].second;
            }
        }
        else
        {
            if ( count > mod->cSeg )
                count = mod->cSeg;

            for ( uint16_t i = 0; i < count; i++ )
            {
                infos[i].SegmentIndex = segDescTable[i].Seg;
                infos[i].LineCount = 0;
                infos[i].StartOffset = segDescTable[i].Off;
                infos[i].EndOffset = segDescTable[i].Off + segDescTable[i].cbSeg - 1;
            }
        }

        return S_OK;
    }

    HRESULT DebugStore::GetFileInfo( uint16_t compilandIndex, uint16_t fileIndex, FileInfo& info )
    {
        if ( (compilandIndex < 1) || (compilandIndex > mCompilandCount) )
            return E_INVALIDARG;
        if ( mCompilandDetails[compilandIndex - 1].SourceEntry == NULL )
            return E_INVALIDARG;

        const uint16_t      zCompIndex = compilandIndex - 1;
        OMFSourceModule*    srcMod = GetCVPtr<OMFSourceModule>( mCompilandDetails[zCompIndex].SourceEntry->lfo );
        DWORD*              filePtrTable = NULL;

        if ( srcMod == NULL )
            return E_FAIL;
        if ( fileIndex >= srcMod->cFile )
            return E_FAIL;

        filePtrTable = (DWORD*) ((BYTE*) srcMod + 4);

        OMFSourceFile*  file = (OMFSourceFile*) ((BYTE*) srcMod + filePtrTable[fileIndex]);
        BYTE*           backOfFile = (BYTE*) file + 4 + (4 * file->cSeg) + (8 * file->cSeg);

        // the spec says the name length field is 2 bytes long, but in practice I see that it's 1
        info.SegmentCount = file->cSeg;
        info.NameLength = *(BYTE*) backOfFile;
        info.Name = (char*) (backOfFile + 1);

        return S_OK;
    }

    HRESULT DebugStore::GetFileSegmentInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t count, SegmentInfo* infos )
    {
        if ( infos == NULL )
            return E_INVALIDARG;
        if ( (compilandIndex < 1) || (compilandIndex > mCompilandCount) )
            return E_INVALIDARG;
        if ( mCompilandDetails[compilandIndex - 1].SourceEntry == NULL )
            return E_INVALIDARG;

        const uint16_t      zCompIndex = compilandIndex - 1;
        OMFSourceModule*    srcMod = GetCVPtr<OMFSourceModule>( mCompilandDetails[zCompIndex].SourceEntry->lfo );
        DWORD*              filePtrTable = NULL;

        if ( srcMod == NULL )
            return E_FAIL;
        if ( fileIndex >= srcMod->cFile )
            return E_FAIL;

        filePtrTable = (DWORD*) ((BYTE*) srcMod + 4);

        OMFSourceFile*  file = (OMFSourceFile*) ((BYTE*) srcMod + filePtrTable[fileIndex]);
        DWORD*          srcLinePtrTable = (DWORD*) ((BYTE*) file + 4);
        OffsetPair*     offsetTable = (OffsetPair*) ((BYTE*) file + 4 + (4 * file->cSeg));

        if ( count > file->cSeg )
            count = file->cSeg;

        for ( uint16_t i = 0; i < count; i++ )
        {
            OMFSourceLine*  srcLine = (OMFSourceLine*) ((BYTE*) srcMod + srcLinePtrTable[i]);

            infos[i].SegmentIndex = srcLine->Seg;
            infos[i].LineCount = srcLine->cLnOff;
            infos[i].StartOffset = offsetTable[i].first;
            infos[i].EndOffset = offsetTable[i].second;
        }

        return S_OK;
    }

    HRESULT DebugStore::GetLineInfo( uint16_t compilandIndex, uint16_t fileIndex, uint16_t segInstanceIndex, uint16_t count, LineInfo* infos )
    {
        if ( infos == NULL )
            return E_INVALIDARG;
        if ( (compilandIndex < 1) || (compilandIndex > mCompilandCount) )
            return E_INVALIDARG;
        if ( mCompilandDetails[compilandIndex - 1].SourceEntry == NULL )
            return E_INVALIDARG;

        const uint16_t      zCompIndex = compilandIndex - 1;
        OMFSourceModule*    srcMod = GetCVPtr<OMFSourceModule>( mCompilandDetails[zCompIndex].SourceEntry->lfo );
        DWORD*              filePtrTable = NULL;

        if ( srcMod == NULL )
            return E_FAIL;
        if ( fileIndex >= srcMod->cFile )
            return E_FAIL;

        filePtrTable = (DWORD*) ((BYTE*) srcMod + 4);

        OMFSourceFile*  file = (OMFSourceFile*) ((BYTE*) srcMod + filePtrTable[fileIndex]);
        DWORD*          srcLinePtrTable = (DWORD*) ((BYTE*) file + 4);

        if ( segInstanceIndex >= file->cSeg )
            return E_FAIL;

        OMFSourceLine*  line = (OMFSourceLine*) ((BYTE*) srcMod + srcLinePtrTable[segInstanceIndex]);
        DWORD*          offsetTable = (DWORD*) ((BYTE*) line + 4);
        WORD*           numberTable = (WORD*) (offsetTable + line->cLnOff);

        if ( count > line->cLnOff )
            count = line->cLnOff;

        for ( uint16_t i = 0; i < count; i++ )
        {
            infos[i].Offset = offsetTable[i];
            infos[i].LineNumber = numberTable[i];
        }

        return S_OK;
    }

    bool DebugStore::GetFileSegment( uint16_t compIndex, uint16_t fileIndex, uint16_t segInstanceIndex, FileSegmentInfo& segInfo )
    {
        if ( (compIndex < 1) || (compIndex > mCompilandCount) )
            return false;

        const uint16_t      zCompIx = compIndex - 1;
        OMFSourceModule*    srcMod = NULL;

        if ( mCompilandDetails[zCompIx].SourceEntry != NULL )
            srcMod = GetCVPtr<OMFSourceModule>( mCompilandDetails[zCompIx].SourceEntry->lfo );

        if ( srcMod == NULL )
            return false;

        DWORD*      filePtrTable = (DWORD*) (((BYTE*) srcMod) + 4);

        if ( fileIndex >= srcMod->cFile )
            return false;

        OMFSourceFile*  file = (OMFSourceFile*) ((BYTE*) srcMod + filePtrTable[fileIndex]);

        DWORD*          srcLinePtrTable = (DWORD*) ((BYTE*) file + 4);
        OffsetPair*     startEndTable = (OffsetPair*) (srcLinePtrTable + file->cSeg);

        if ( segInstanceIndex >= file->cSeg )
            return false;

        OMFSourceLine*  srcLine = (OMFSourceLine*) ((BYTE*) srcMod + srcLinePtrTable[segInstanceIndex]);

        DWORD*  offsetTable = (DWORD*) ((BYTE*) srcLine + 4);
        WORD*   numberTable = (WORD*) (offsetTable + srcLine->cLnOff);

        segInfo.SegmentIndex = srcLine->Seg;
        segInfo.SegmentInstance = segInstanceIndex;
        segInfo.Start = startEndTable[segInstanceIndex].first;
        segInfo.End = startEndTable[segInstanceIndex].second;
        segInfo.LineCount = srcLine->cLnOff;
        segInfo.Offsets = offsetTable;
        segInfo.LineNumbers = numberTable;

        return true;
    }

    bool    DebugStore::FindCompilandFileSegment( uint16_t line, uint16_t compIndex, uint16_t fileIndex, FileSegmentInfo& segInfo )
    {
        if ( (compIndex < 1) || (compIndex > mCompilandCount) )
            return false;

        const uint16_t      zCompIx = compIndex - 1;
        OMFSourceModule*    srcMod = NULL;

        if ( mCompilandDetails[zCompIx].SourceEntry != NULL )
            srcMod = GetCVPtr<OMFSourceModule>( mCompilandDetails[zCompIx].SourceEntry->lfo );

        if ( srcMod == NULL )
            return false;

        DWORD*      filePtrTable = (DWORD*) (((BYTE*) srcMod) + 4);

        if ( fileIndex >= srcMod->cFile )
            return false;

        OMFSourceFile*  file = (OMFSourceFile*) ((BYTE*) srcMod + filePtrTable[fileIndex]);

        DWORD*          srcLinePtrTable = (DWORD*) ((BYTE*) file + 4);
        OffsetPair*     startEndTable = (OffsetPair*) (srcLinePtrTable + file->cSeg);

        uint16_t    zClosestAboveSegIx = 0xFFFF;
        uint16_t    zClosestBelowSegIx = 0xFFFF;
        int         closestDistAbove = INT_MAX;
        int         closestDistBelow = INT_MAX;

        for ( uint16_t zSegIx = 0; zSegIx < file->cSeg; zSegIx++ )
        {
            OMFSourceLine*  srcLine = (OMFSourceLine*) ((BYTE*) srcMod + srcLinePtrTable[zSegIx]);

            DWORD*  offsetTable = (DWORD*) ((BYTE*) srcLine + 4);
            WORD*   numberTable = (WORD*) (offsetTable + srcLine->cLnOff);

            if ( srcLine->cLnOff == 0 )
                continue;

            if ( (numberTable[0] <= line) && (numberTable[ srcLine->cLnOff - 1 ] >= line) )
            {
                segInfo.SegmentIndex = srcLine->Seg;
                segInfo.LineCount = srcLine->cLnOff;
                segInfo.LineNumbers = numberTable;
                segInfo.Offsets = offsetTable;
                segInfo.Start = startEndTable[zSegIx].first;
                segInfo.End = startEndTable[zSegIx].second;
                return true;
            }

            if ( numberTable[ srcLine->cLnOff - 1 ] < line )
            {
                int dist = line - numberTable[ srcLine->cLnOff - 1 ];

                if ( dist < closestDistAbove )
                {
                    closestDistAbove = dist;
                    zClosestAboveSegIx = zSegIx;
                }
            }
            else
            {
                int dist = numberTable[ 0 ] - line;

                if ( dist < closestDistBelow )
                {
                    closestDistBelow = dist;
                    zClosestBelowSegIx = zSegIx;
                }
            }
        }

        if ( (closestDistAbove == INT_MAX) && (closestDistBelow == INT_MAX) )
            return false;

        // at this point we know we didn't have a perfect match, 
        // so pick the best one above or below our target line

        uint16_t    zClosestSegIx = 0;

        if ( closestDistAbove < closestDistBelow )
            zClosestSegIx = zClosestAboveSegIx;
        else
            zClosestSegIx = zClosestBelowSegIx;

        {
            OMFSourceLine*  srcLine = (OMFSourceLine*) ((BYTE*) srcMod + srcLinePtrTable[zClosestSegIx]);

            DWORD*  offsetTable = (DWORD*) ((BYTE*) srcLine + 4);
            WORD*   numberTable = (WORD*) (offsetTable + srcLine->cLnOff);

            segInfo.SegmentIndex = srcLine->Seg;
            segInfo.LineCount = srcLine->cLnOff;
            segInfo.LineNumbers = numberTable;
            segInfo.Offsets = offsetTable;
            segInfo.Start = startEndTable[zClosestSegIx].first;
            segInfo.End = startEndTable[zClosestSegIx].second;
        }

        return true;
    }

    bool DebugStore::FindCompilandFileSegment( WORD seg, DWORD offset, uint16_t& compIndex, uint16_t& fileIndex, FileSegmentInfo& fileSegInfo )
    {
        uint32_t    compCount = 0;

        GetCompilandCount( compCount );

        for ( uint16_t zCompIx = 0; zCompIx < compCount; zCompIx++ )
        {
            // no source, no files
            if ( mCompilandDetails[ zCompIx ].SourceEntry == NULL )
                continue;

            OMFDirEntry*        entry = &mDirs[zCompIx];
            OMFModule*          mod = GetCVPtr<OMFModule>( entry->lfo );
            OMFSegDesc*         segDescTable = NULL;

            if ( mod == NULL )
                continue;

            segDescTable = (OMFSegDesc*) (mod + 1);

            for ( uint16_t modSegIx = 0; modSegIx < mod->cSeg; modSegIx++ )
            {
                if ( (segDescTable[modSegIx].Seg == seg) 
                    && (offset >= segDescTable[modSegIx].Off)
                    && ((offset - segDescTable[modSegIx].Off + 1) <= segDescTable[modSegIx].cbSeg) )
                {
                    if ( FindFileSegment( seg, offset, zCompIx + 1, fileIndex, fileSegInfo ) )
                    {
                        compIndex = zCompIx + 1;
                        return true;
                    }
                    break;
                }
            }
        }

        return false;
    }

    bool    DebugStore::FindFileSegment( WORD seg, DWORD offset, uint16_t compIndex, uint16_t& fileIndex, FileSegmentInfo& fileSegInfo )
    {
        if ( (compIndex < 1) || (compIndex > mCompilandCount) )
            return false;

        const uint16_t      zCompIx = compIndex - 1;
        OMFSourceModule*    srcMod = NULL;

        if ( mCompilandDetails[zCompIx].SourceEntry != NULL )
            srcMod = GetCVPtr<OMFSourceModule>( mCompilandDetails[zCompIx].SourceEntry->lfo );

        if ( srcMod == NULL )
            return false;

        DWORD*      filePtrTable = (DWORD*) (((BYTE*) srcMod) + 4);
        OffsetPair* offsetTable = (OffsetPair*) (filePtrTable + srcMod->cFile);
        WORD*       segTable = (WORD*) (offsetTable + srcMod->cSeg);

        for ( uint16_t zModSegIx = 0; zModSegIx < srcMod->cSeg; zModSegIx++ )
        {
            if ( segTable[zModSegIx] != seg )
                continue;

            if ( ((offsetTable[zModSegIx].first == 0) && (offsetTable[zModSegIx].second == 0))
                || ((offset >= offsetTable[zModSegIx].first) && (offset <= offsetTable[zModSegIx].second)) )
            {
                for ( uint16_t zFileIx = 0; zFileIx < srcMod->cFile; zFileIx++ )
                {
                    OMFSourceFile*  file = (OMFSourceFile*) ((BYTE*) srcMod + filePtrTable[zFileIx]);

                    if ( FindFileSegment( seg, offset, srcMod, file, fileSegInfo ) )
                    {
                        fileIndex = zFileIx;
                        return true;
                    }
                }
            }
            break;
        }

        return false;
    }

    bool DebugStore::FindFileSegment( 
        WORD seg, 
        DWORD offset, 
        OMFSourceModule* srcMod, 
        OMFSourceFile* file, 
        FileSegmentInfo& fileSegInfo )
    {
        DWORD*          srcLinePtrTable = (DWORD*) ((BYTE*) file + 4);
        OffsetPair*     startEndTable = (OffsetPair*) (srcLinePtrTable + file->cSeg);

        for ( uint16_t zSegIx = 0; zSegIx < file->cSeg; zSegIx++ )
        {
            OMFSourceLine*  line = (OMFSourceLine*) ((BYTE*) srcMod + srcLinePtrTable[zSegIx]);

            if ( line->Seg != seg )
                continue;

            if ( ((startEndTable[zSegIx].first == 0) && (startEndTable[zSegIx].second == 0))
                || ((startEndTable[zSegIx].first <= offset) && (startEndTable[zSegIx].second >= offset)) )
            {
                DWORD*  offsetTable = (DWORD*) ((BYTE*) line + 4);
                WORD*   numberTable = (WORD*) (offsetTable + line->cLnOff);

                fileSegInfo.SegmentIndex = line->Seg;
                fileSegInfo.SegmentInstance = zSegIx;
                fileSegInfo.Start = startEndTable[zSegIx].first;
                fileSegInfo.End = startEndTable[zSegIx].second;
                fileSegInfo.LineCount = line->cLnOff;
                fileSegInfo.LineNumbers = numberTable;
                fileSegInfo.Offsets = offsetTable;
                return true;
            }
            // don't leave yet, because the addr might be in another file/segment instance
        }

        return false;
    }

    HRESULT DebugStore::GetSymbolBytePtr( SymHandle handle, BYTE* bytes, DWORD& size )
    {
        SymHandleIn*    internalHandle = (SymHandleIn*) &handle;

        // 4 bytes, at least as much as the generic symbol node: length and tag
        if ( !ValidateSymbol( internalHandle, 4 ) )
            return E_INVALIDARG;

        CodeViewSymbol*   sym = internalHandle->Sym;
        BYTE*             symBytes = (BYTE*) internalHandle->Sym;
        DWORD             len = sym->Generic.len + 2;

        if ( (bytes == NULL) || (size == 0) )
        {
            size = len;
            return E_INSUFFICIENT_BUFFER;
        }

        if ( size > len )
            size = len;

        for ( DWORD i = 0; i < size; i++ )
        {
            bytes[i] = symBytes[i];
        }

        return S_OK;
    }

    HRESULT DebugStore::GetTypeBytePtr( TypeHandle handle, BYTE* bytes, DWORD& size )
    {
        TypeHandleIn*   internalHandle = (TypeHandleIn*) &handle;

        if ( (internalHandle->Type == NULL) || (internalHandle->Index < 0x1000) )
            return E_INVALIDARG;

        // 4 bytes, at least as much as the generic symbol node: length and tag
        if ( !ValidateCVPtr( internalHandle->Type, 4 ) )
            return E_INVALIDARG;

        BYTE*             typeBytes = (BYTE*) internalHandle->Type;
        DWORD             len = 0;

        if ( internalHandle->Tag < 0x0400 )
        {
            CodeViewType*     type = (CodeViewType*) internalHandle->Type;
            len = type->Generic.len + 2;
        }
        else if ( internalHandle->Tag == LF_MAGO_METHODOVERLOAD )
        {
            len = 4;            // the attribute and type

            CV_fldattr_t*   attr = (CV_fldattr_t*) internalHandle->Type;
            if ( (attr->mprop == CV_MTintro) || (attr->mprop == CV_MTpureintro) )
                len += 4;       // the optional vtab offset
        }
        else
        {
            CodeViewFieldType*  type = (CodeViewFieldType*) internalHandle->Type;
            len = GetFieldLength( type );
        }

        if ( (bytes == NULL) || (size == 0) )
        {
            size = len;
            return E_INSUFFICIENT_BUFFER;
        }

        if ( size > len )
            size = len;

        for ( DWORD i = 0; i < size; i++ )
        {
            bytes[i] = typeBytes[i];
        }

        return S_OK;
    }

    bool DebugStore::ValidateField( TypeScopeIn* scopeIn )
    {
        // we might be padding, so see if have enough for that
        if ( scopeIn->CurPtr >= scopeIn->Limit )
            return false;

        // in a complex list and reached padding, so skip ahead
        if ( *scopeIn->CurPtr >= 0xF0 )
        {
            DWORD   skipAmount = (*scopeIn->CurPtr - 0xF0);
            scopeIn->CurPtr += skipAmount;
        }

        // do we have at least a tag field?
        if ( scopeIn->CurPtr + 1 >= scopeIn->Limit )
            return false;

        // get the current value

        CodeViewFieldType*  type = (CodeViewFieldType*) scopeIn->CurPtr;

        if ( type->Generic.id == 0 )
            return false;
        // there's no length field, so we can't validate a length
        // at this point we assume that the rest of the record is in bounds

        return true;
    }

    bool DebugStore::ValidateSymbol( SymHandleIn* internalHandle, DWORD symSize )
    {
        if ( (internalHandle->Sym == NULL) || (internalHandle->HeapDir == NULL) )
            return false;

        CodeViewSymbol* sym = internalHandle->Sym;
        if ( sym->Generic.len < (symSize - 2) )
            return false;

        // as many bytes as the caller needs at the beginning of the symbol
        if ( !ValidateCVPtr( internalHandle->Sym, symSize ) )
            return false;

        if ( !ValidateCVPtr( internalHandle->HeapDir, mDirHeader->cbDirEntry ) )
            return false;

        return true;
    }

    template <class T>
    T* DebugStore::GetCVPtr( offset_t offset )
    {
        if ( (offset < 0) || ((DWORD) offset >= mCVBufSize) || ((offset + sizeof( T )) > mCVBufSize) )
            return NULL;

        return (T*) (mCVBuf + offset);
    }

    template <class T>
    T* DebugStore::GetCVPtr( offset_t offset, DWORD size )
    {
        if ( (offset < 0) || ((DWORD) offset >= mCVBufSize) || ((offset + size) > mCVBufSize) )
            return NULL;

        return (T*) (mCVBuf + offset);
    }

    bool DebugStore::ValidateCVPtr( void* p, DWORD size )
    {
        const BYTE* pb = (BYTE*) p;
        const BYTE* pbLimit = pb + size;

        return (pb >= mCVBuf) && (pbLimit >= pb) && (pbLimit <= (mCVBuf + mCVBufSize));
    }
}
