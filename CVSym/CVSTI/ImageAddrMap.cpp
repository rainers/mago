/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ImageAddrMap.h"

#include <dia2.h>

namespace MagoST
{
    ImageAddrMap::ImageAddrMap()
        :   mRefCount( 0 ),
            mSecCount( 0 )
    {
    }

    void ImageAddrMap::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void ImageAddrMap::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    uint32_t ImageAddrMap::MapSecOffsetToRVA( uint16_t secIndex, uint32_t offset )
    {
        if ( (secIndex == 0) || (secIndex > mSecCount) )
            return ~0U;

        uint16_t    zSec = secIndex - 1;    // zero-based section index

        return mSections[zSec].RVA + offset;
    }

    uint16_t ImageAddrMap::MapRVAToSecOffset( uint32_t rva, uint32_t& offset )
    {
        uint16_t    closestSec = USHRT_MAX;
        uint32_t    closestOff = ULONG_MAX;

        for ( uint16_t i = 0; i < mSecCount; i++ )
        {
            if ( rva >= mSections[i].RVA )
            {
                uint32_t    curOff = rva - mSections[i].RVA;

                if ( curOff < closestOff )
                {
                    closestOff = curOff;
                    closestSec = i;
                }
            }
        }

        if ( closestSec < USHRT_MAX )
        {
            offset = closestOff;
            return closestSec + 1;      // remember it's 1-based
        }

        return 0;
    }

    uint16_t ImageAddrMap::FindSection( const char* name )
    {
        for ( uint16_t i = 0; i < mSecCount; i++ )
            if( strncmp( name, mSections[i].Name, sizeof( mSections[i].Name ) ) == 0 )
                return i + 1;      // remember it's 1-based

        return 0;
    }

    HRESULT ImageAddrMap::LoadFromSections( uint16_t count, const IMAGE_SECTION_HEADER* secHeaders )
    {
        _ASSERT( secHeaders != NULL );
        _ASSERT( mSections.Get() == NULL );
        _ASSERT( count != USHRT_MAX );

        if ( count == USHRT_MAX )
            return E_INVALIDARG;

        mSections.Attach( new Section[ count ] );
        if ( mSections.Get() == NULL )
            return E_OUTOFMEMORY;

        mSecCount = count;

        for ( uint16_t i = 0; i < count; i++ )
        {
            DWORD   size = secHeaders[i].SizeOfRawData;

            if ( secHeaders[i].Misc.VirtualSize > 0 )
                size = secHeaders[i].Misc.VirtualSize;

            mSections[i].RVA = secHeaders[i].VirtualAddress;
            mSections[i].Size = size;
            memcpy( mSections[i].Name, (const char*) secHeaders[i].Name, sizeof( mSections[i].Name ) );
        }

        return S_OK;
    }

    ///////////////////////////////////////////////////////////////////////////////
    static IDiaEnumSegments* GetEnumSegments(IDiaSession *pSession)
    {
        IDiaEnumSegments* pUnknown  = NULL;
        IDiaEnumTables* pEnumTables = NULL;
        IDiaTable*      pTable      = NULL;
        ULONG           celt        = 0;

        if ( pSession->getEnumTables(&pEnumTables) != S_OK )
            return nullptr;

        while (pEnumTables->Next(1, &pTable, &celt) == S_OK && celt == 1)
        {
            // There is only one table that matches the given iid
            HRESULT hr = pTable->QueryInterface(__uuidof(IDiaEnumSegments), (void**)&pUnknown);
            pTable->Release();
            if (hr == S_OK)
                break;
        }
        pEnumTables->Release();
        return pUnknown;
    }

    HRESULT ImageAddrMap::LoadFromDiaSession( IDiaSession* session )
    {
        if ( auto pEnumSections = GetEnumSegments( session ) )
        {
            LONG count;
            if(pEnumSections->get_Count(&count) == S_OK && count < UINT16_MAX)
            {
                mSections.Attach( new Section[ count ] );
                mSecCount = (uint16_t) count;
                IDiaSegment* pSegment;
                ULONG celt = 0;
                for (int i = 0; i < count && pEnumSections->Next(1, &pSegment, &celt) == S_OK && celt == 1; i++)
                {
                    DWORD seg;
                    pSegment->get_addressSection(&seg);
                    pSegment->get_relativeVirtualAddress((DWORD*) &mSections[i].RVA);
                    pSegment->get_length((DWORD*) &mSections[i].Size);
                    // no name
                    pSegment->Release();
                }
            }
            pEnumSections->Release(); 
            return S_OK;
        }
        return S_FALSE;
    }
}
