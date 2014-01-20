/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ImageFile.h"
#include "BinUtil.h"

const DWORD NtHeaderBufSize = 1024;


namespace BinImage
{
    void ImageFile::LoadedImage::TransferTo( LoadedImage& other )
    {
        other.HFile.Attach( HFile.Detach() );
        other.HMapping.Attach( HMapping.Detach() );
        other.Size = Size;
    }


    ImageFile::ImageFile()
        :   mNtHeaderBuf( NULL ),
            mNtHeaderAddr( 0 ),
            mSecHeaderBuf( NULL ),
            mSecHeaderAddr( 0 ),
            mDataDirs( NULL ),
            mDataDirCount( 0 )
    {
    }

    ImageFile::~ImageFile()
    {
        if ( mNtHeaderBuf != NULL )
            delete [] mNtHeaderBuf;
        if ( mSecHeaderBuf != NULL )
            delete [] mSecHeaderBuf;
    }

    HRESULT ImageFile::LoadFile( const wchar_t* filename )
    {
        HRESULT     hr = S_OK;
        LoadedImage image;
        DWORD       bytesRead = 0;
        BOOL        bRet = FALSE;
        DWORD       newFilePtr = 0;

        mNtHeaderBuf = new BYTE[ NtHeaderBufSize ];
        if ( mNtHeaderBuf == NULL )
            return E_OUTOFMEMORY;

        hr = OpenAndMapImage( filename, image );
        if ( FAILED( hr ) )
            return hr;

        bRet = ReadFile( image.HFile, mNtHeaderBuf, NtHeaderBufSize, &bytesRead, NULL );
        if ( !bRet )
            return GetLastHr();

        IMAGE_DOS_HEADER*   dosHeader = (IMAGE_DOS_HEADER*) mNtHeaderBuf;

        if ( (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) || (dosHeader->e_lfanew < 0) )
            return E_BAD_FORMAT;
        if ( image.Size <= (DWORD) dosHeader->e_lfanew )
            return E_BAD_FORMAT;

        // has to be saved before overwriting buffer
        mNtHeaderAddr = dosHeader->e_lfanew;

        newFilePtr = SetFilePointer( image.HFile, dosHeader->e_lfanew, NULL, FILE_BEGIN );
        if ( newFilePtr == INVALID_SET_FILE_POINTER )
            return GetLastHr();

        // should be enough to read NT headers, including all data dirs
        bRet = ReadFile( image.HFile, mNtHeaderBuf, NtHeaderBufSize, &bytesRead, NULL );
        if ( !bRet )
            return GetLastHr();

        IMAGE_NT_HEADERS32* ntHeaders32 = (IMAGE_NT_HEADERS32*) mNtHeaderBuf;

        if ( ntHeaders32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC )
        {
            mDataDirCount = ntHeaders32->OptionalHeader.NumberOfRvaAndSizes;
            mDataDirs = ntHeaders32->OptionalHeader.DataDirectory;
        }
        else if ( ntHeaders32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC )
        {
            IMAGE_NT_HEADERS64* ntHeaders64 = (IMAGE_NT_HEADERS64*) mNtHeaderBuf;

            mDataDirCount = ntHeaders64->OptionalHeader.NumberOfRvaAndSizes;
            mDataDirs = ntHeaders64->OptionalHeader.DataDirectory;
        }
        else
            return E_BAD_FORMAT;

        DWORD   totalNtHeadersOnlySize = (BYTE*) mDataDirs - mNtHeaderBuf;
        DWORD   sizeLeftForDataDirs = NtHeaderBufSize - totalNtHeadersOnlySize;
        DWORD   countLeftForDataDirs = sizeLeftForDataDirs / sizeof( IMAGE_DATA_DIRECTORY );

        if ( countLeftForDataDirs < mDataDirCount )
            return E_FAIL;

        if ( ntHeaders32->FileHeader.NumberOfSections > MaxSectionCount )
            return E_BAD_FORMAT;

        mSecHeaderAddr = mNtHeaderAddr + ((BYTE*) (mDataDirs + mDataDirCount) - mNtHeaderBuf);
        image.TransferTo( mImage );

        return S_OK;
    }

    bool ImageFile::FindDataDirectoryData( uint16_t dirEntryId, DataDirInfo& info )
    {
        HRESULT hr = S_OK;

        hr = EnsureSectionsLoaded();
        if ( FAILED( hr ) )
            return false;

        return ::FindDataDirectoryData(
            (WORD) mDataDirCount,
            GetDataDirs(),
            ((IMAGE_NT_HEADERS32*) mNtHeaderBuf)->FileHeader.NumberOfSections,
            GetSectionHeaders(),
            false,
            dirEntryId,
            info );
    }

    BYTE* ImageFile::GetNtHeadersBase()
    {
        return mNtHeaderBuf;
    }

    IMAGE_DATA_DIRECTORY* ImageFile::GetDataDirs()
    {
        return mDataDirs;
    }

    IMAGE_SECTION_HEADER* ImageFile::GetSectionHeaders()
    {
        HRESULT hr = S_OK;

        hr = EnsureSectionsLoaded();
        if ( FAILED( hr ) )
            return NULL;

        return (IMAGE_SECTION_HEADER*) mSecHeaderBuf;
    }

    HRESULT ImageFile::GetFileHandle( HANDLE& handle )
    {
        BOOL            bRet = FALSE;
        FileHandlePtr   hFile;

        bRet = DuplicateHandle( 
            GetCurrentProcess(), 
            mImage.HFile, 
            GetCurrentProcess(), 
            &hFile.Ref(), 
            0, 
            FALSE, 
            DUPLICATE_SAME_ACCESS );
        if ( !bRet )
            return GetLastHr();

        handle = hFile.Detach();
        return S_OK;
    }

    HRESULT ImageFile::GetMappingHandle( HANDLE& handle )
    {
        BOOL            bRet = FALSE;
        HandlePtr       hMapping;

        bRet = DuplicateHandle( 
            GetCurrentProcess(), 
            mImage.HMapping, 
            GetCurrentProcess(), 
            &hMapping.Ref(), 
            0, 
            FALSE, 
            DUPLICATE_SAME_ACCESS );
        if ( !bRet )
            return GetLastHr();

        handle = hMapping.Detach();
        return S_OK;
    }

    HRESULT ImageFile::OpenAndMapImage( 
        const wchar_t* filename,
        LoadedImage& loadedImage )
    {
        LoadedImage image;
        DWORD       hiSize = 0;

        image.HFile = CreateFile(
            filename,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_RANDOM_ACCESS,
            NULL );
        if ( image.HFile.IsEmpty() )
            return GetLastHr();

        image.Size = GetFileSize( image.HFile, &hiSize );
        if ( image.Size == INVALID_FILE_SIZE )
            return GetLastHr();

        if ( (image.Size == 0) && (hiSize == 0) )
            return E_BAD_FORMAT;
        if ( hiSize > 0 )                       // we don't support huge files
            return E_BAD_FORMAT;

        image.HMapping = CreateFileMapping( 
            image.HFile, 
            NULL,
            PAGE_READONLY,
            0,
            0,
            NULL );
        if ( image.HMapping.IsEmpty() )
            return GetLastHr();

        image.TransferTo( loadedImage );
        return S_OK;
    }

    HRESULT ImageFile::EnsureSectionsLoaded()
    {
        if ( mSecHeaderBuf != NULL )
            return S_OK;
        if ( mNtHeaderBuf == NULL )
            return E_UNEXPECTED;

        IMAGE_NT_HEADERS32* ntHeaders32 = (IMAGE_NT_HEADERS32*) mNtHeaderBuf;

        BOOL        bRet = FALSE;
        DWORD       bytesRead = 0;
        DWORD       newFilePtr = 0;
        DWORD       totalSecSize = 0;

        totalSecSize = ntHeaders32->FileHeader.NumberOfSections * sizeof( IMAGE_SECTION_HEADER );

        mSecHeaderBuf = new BYTE[ totalSecSize ];
        if ( mSecHeaderBuf == NULL )
            return E_OUTOFMEMORY;

        _ASSERT( mSecHeaderAddr != 0 );
        newFilePtr = SetFilePointer( mImage.HFile, mSecHeaderAddr, NULL, FILE_BEGIN );
        if ( newFilePtr == INVALID_SET_FILE_POINTER )
            return GetLastHr();

        bRet = ReadFile( mImage.HFile, mSecHeaderBuf, totalSecSize, &bytesRead, NULL );
        if ( !bRet )
            return GetLastHr();

        return S_OK;
    }
}
