/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class OMFAddrTable
{
    BYTE*   mBase;
    WORD    mNumGroups;
    DWORD*  mOffsets;
    DWORD*  mCounts;
    BYTE*   mTable;

public:
    typedef std::pair<DWORD, DWORD> Pair;


    OMFAddrTable( BYTE* base )
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

    Pair*    GetGroup( WORD bucket )
    {
        return (Pair*) (mTable + mOffsets[bucket]);
    }

    bool GetSymbolOffset( WORD segment, DWORD offset, uint32_t& symOffset )
    {
        WORD    bucket = 0;
        DWORD   i = 0;

        Pair*   offsets = NULL;
        DWORD   numOffsets = 0;

        bucket = segment - 1;                       // segments are 1-based

        if ( (segment < 1) || (segment > GetNumGroups()) )
            return false;

        offsets = GetGroup( bucket );
        numOffsets = GetNumItems( bucket );

        if ( numOffsets == 0xFFFFFFFF )
            return false;

        bool        found = false;
        DWORD       last = 0;

        for ( i = 0; i < numOffsets; i++ )
        {
            if ( offset == offsets[i].second )
            {
                symOffset = offsets[i].first;
                return true;
            }
            else if ( offset > offsets[i].second )
            {
                found = true;
                last = i;
            }
            else if ( offset < offsets[i].second )
            {
                if ( found )
                {
                    symOffset = offsets[last].first;
                    return true;
                }
            }
        }
        if( found )
            symOffset = offsets[last].first;
        return found;
    }
};
