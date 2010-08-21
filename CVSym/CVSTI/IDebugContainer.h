/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoST
{
    class IAddressMap;


    class IDebugContainer
    {
    public:
        virtual ~IDebugContainer() { }

        virtual HRESULT LockDebugSection( BYTE*& bytes, DWORD& size ) = 0;
        virtual HRESULT UnlockDebugSection( BYTE* bytes ) = 0;

        virtual bool HasAddressMap() = 0;
        virtual HRESULT GetAddressMap( IAddressMap*& map ) = 0;
    };
}
