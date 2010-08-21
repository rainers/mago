/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoST
{
    class IAddressMap
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        // returns 0 on section not found
        virtual uint32_t MapSecOffsetToRVA( uint16_t secIndex, uint32_t offset ) = 0;
        virtual uint16_t MapRVAToSecOffset( uint32_t rva, uint32_t& offset ) = 0;
    };
}
