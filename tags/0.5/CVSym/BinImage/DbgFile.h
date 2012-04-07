/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace BinImage
{
    class DbgFile
    {
        FileHandlePtr   mHFile;
        HandlePtr       mHMapping;

        IMAGE_SEPARATE_DEBUG_HEADER mHeader;
        BYTE*                       mSecHeaderBuf;

    public:
        DbgFile();
        ~DbgFile();

        HRESULT LoadFile( const wchar_t* filename );

        bool Validate( DWORD timeDateStamp );

        const IMAGE_SEPARATE_DEBUG_HEADER* GetHeader();
        const IMAGE_SECTION_HEADER* GetSectionHeaders();
        const IMAGE_DEBUG_DIRECTORY* GetDebugDirs();

        HRESULT GetFileHandle( HANDLE& handle );
        HRESULT GetMappingHandle( HANDLE& handle );
    };
}
