/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace BinImage
{
    class ImageFile
    {
        struct LoadedImage
        {
            FileHandlePtr   HFile;
            HandlePtr       HMapping;
            DWORD           Size;

            LoadedImage()
                :   Size( 0 )
            {
            }

            void TransferTo( LoadedImage& other );

        private:
            LoadedImage( const LoadedImage& other );
            LoadedImage& operator=( LoadedImage& other );
        };

        LoadedImage             mImage;
        BYTE*                   mNtHeaderBuf;
        DWORD                   mNtHeaderAddr;
        BYTE*                   mSecHeaderBuf;
        DWORD                   mSecHeaderAddr;
        IMAGE_DATA_DIRECTORY*   mDataDirs;
        DWORD                   mDataDirCount;

    public:
        ImageFile();
        ~ImageFile();

        HRESULT LoadFile( const wchar_t* filename );

        bool FindDataDirectoryData( uint16_t dirEntryId, DataDirInfo& info );

        BYTE* GetNtHeadersBase();
        IMAGE_DATA_DIRECTORY* GetDataDirs();
        IMAGE_SECTION_HEADER* GetSectionHeaders();

        HRESULT GetFileHandle( HANDLE& handle );
        HRESULT GetMappingHandle( HANDLE& handle );

    private:
        HRESULT EnsureSectionsLoaded();

        static HRESULT OpenAndMapImage( 
            const wchar_t* filename,
            LoadedImage& loadedImage );
    };
}
