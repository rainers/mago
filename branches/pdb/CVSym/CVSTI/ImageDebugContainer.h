/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "IDebugContainer.h"


namespace MagoST
{
    class ILoadCallback;


    class ImageDebugContainer : public IDebugContainer
    {
        std::auto_ptr<BinImage::ImageFile>  mImage;
        std::auto_ptr<BinImage::DbgFile>    mDbg;
        IMAGE_DEBUG_DIRECTORY               mDebugDir;

    public:
        HRESULT LoadExe( const wchar_t* filename, ILoadCallback* callback );
        HRESULT LoadDbg( const wchar_t* filename, ILoadCallback* callback );

        virtual HRESULT LockDebugSection( BYTE*& bytes, DWORD& size );
        virtual HRESULT UnlockDebugSection( BYTE* bytes );

        virtual bool HasAddressMap();
        virtual HRESULT GetAddressMap( IAddressMap*& map );

    private:
        HRESULT LoadDbg( 
            BinImage::ImageFile* image, 
            const IMAGE_DEBUG_DIRECTORY* miscDebugDir, 
            ILoadCallback* callback );

        static void AlignAddress( DWORD unalignedAddr, DWORD& alignedAddr, DWORD& diff );
    };
}
