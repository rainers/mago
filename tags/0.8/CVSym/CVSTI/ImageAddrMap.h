/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "IAddressMap.h"


namespace MagoST
{
    class ImageAddrMap : public IAddressMap
    {
        struct Section
        {
            uint32_t    RVA;
            uint32_t    Size;
            char        Name[8];
        };

        long    mRefCount;
        boost::scoped_array<Section>    mSections;
        uint16_t    mSecCount;

    public:
        ImageAddrMap();

        virtual void AddRef();
        virtual void Release();

        virtual uint32_t MapSecOffsetToRVA( uint16_t secIndex, uint32_t offset );
        virtual uint16_t MapRVAToSecOffset( uint32_t rva, uint32_t& offset );
        virtual uint16_t FindSection( const char* name );

        HRESULT LoadFromSections( uint16_t count, const IMAGE_SECTION_HEADER* secHeaders );
    };
}
