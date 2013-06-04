/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class OMFHashTable
{
    BYTE*   mBase;
    WORD    mNumGroups;
    DWORD*  mOffsets;
    DWORD*  mCounts;
    BYTE*   mTable;

public:
    typedef std::pair<DWORD, DWORD> HashPair;


    OMFHashTable( BYTE* base )
        :   mBase( base ),
            mNumGroups( 0 ),
            mOffsets( NULL ),
            mCounts( NULL ),
            mTable( NULL )
    {
        if ( mBase != NULL )
        {
            mNumGroups = *(WORD*) mBase;
            mOffsets = (DWORD*) (mBase + 4);
            mCounts = (DWORD*) (mBase + 4 + (4 * mNumGroups));
            mTable = (mBase + 4 + (8 * mNumGroups));
        }
    }

    WORD GetNumGroups()
    {
        return mNumGroups;
    }

    DWORD GetNumItems( WORD bucket )
    {
        return mCounts[bucket];
    }

    HashPair*    GetGroup( WORD bucket )
    {
        return (HashPair*) (mTable + mOffsets[bucket]);
    }

    uint32_t GetSymbolOffset( const char* name )
    {
        size_t  nameLen = strlen( name );

        return GetSymbolOffset( name, nameLen );
    }

    bool GetSymbolOffsetIter( uint32_t hash, HashPair*& pairs, uint32_t& pairCount )
    {
        WORD    bucket = 0;

        HashPair*   offsets = NULL;
        DWORD       numOffsets = 0;

        bucket = hash % GetNumGroups();

        offsets = GetGroup( bucket );
        numOffsets = GetNumItems( bucket );

        if ( (numOffsets == 0xFFFFFFFF) )
            return false;

        pairs = offsets;
        pairCount = numOffsets;
        return true;
    }

    // doesn't guarantee that the actual strings match
    uint32_t GetSymbolOffset( const char* name, size_t nameLen )
    {
        UINT    hash = 0;
        WORD    bucket = 0;
        DWORD   i = 0;

        HashPair*   offsets = NULL;
        DWORD       numOffsets = 0;

        hash = GetSymbolNameHash( name, nameLen );
        bucket = hash % GetNumGroups();

        offsets = GetGroup( bucket );
        numOffsets = GetNumItems( bucket );

        if ( numOffsets == 0xFFFFFFFF )
            return false;

        for ( i = 0; i < numOffsets; i++ )
        {
            if ( hash == offsets[i].second )
            {
                return offsets[i].first;
            }
        }

        return 0;
    }

    static BYTE ByteToUpper( BYTE b )
    {
        return b & 0xDF;
    }

    static UINT IntToUpper( UINT i )
    {
        return i & 0xDFDFDFDF;
    }

    static UINT GetSymbolNameHash( PCSTR name, size_t nameLen )
    {
        UINT    end = 0;
        UINT    wordLen = 0;
        UINT*   wordName = (UINT*) name;
        UINT    sum = 0;

        while ( nameLen & 3 )
        {
            end |= ByteToUpper( name[nameLen - 1] );
            end <<= 8;
            nameLen--;
        }

        wordLen = nameLen / 4;
        for ( UINT i = 0; i < wordLen; i++ )
        {
            sum ^= IntToUpper( wordName[i] );
            sum = _lrotl( sum, 4 );
        }
        sum ^= end;

        return sum;
    }
};
