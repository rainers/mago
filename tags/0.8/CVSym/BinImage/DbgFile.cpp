/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DbgFile.h"


namespace BinImage
{
    DbgFile::DbgFile()
        :   mSecHeaderBuf( NULL )
    {
    }

    DbgFile::~DbgFile()
    {
        if ( mSecHeaderBuf != NULL )
            delete [] mSecHeaderBuf;
    }

    HRESULT DbgFile::LoadFile( const wchar_t* filename )
    {
        FileHandlePtr   hFile;
        HandlePtr       hMapping;
        DWORD           loSize = 0;
        DWORD           hiSize = 0;
        BOOL            bRet = FALSE;
        DWORD           bytesRead = 0;

        hFile = CreateFile(
            filename,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL );
        if ( hFile.IsEmpty() )
            return GetLastHr();

        loSize = GetFileSize( hFile, &hiSize );
        if ( loSize == INVALID_FILE_SIZE )
            return GetLastHr();
        if ( (loSize == 0) && (hiSize == 0) )
            return E_BAD_FORMAT;

        hMapping = CreateFileMapping(
            hFile, 
            NULL,
            PAGE_READONLY,
            0,
            loSize,
            NULL );
        if ( hMapping.IsEmpty() )
            return GetLastHr();

        bRet = ReadFile( hFile, &mHeader, sizeof mHeader, &bytesRead, NULL );
        if ( !bRet )
            return GetLastHr();

        if ( mHeader.NumberOfSections > MaxSectionCount )
            return E_BAD_FORMAT;

        DWORD   allSecSize = mHeader.NumberOfSections * sizeof( IMAGE_SECTION_HEADER );
        DWORD   allHeaderSize = sizeof mHeader + mHeader.DebugDirectorySize + allSecSize;
        DWORD   secAndDirSize = allSecSize + mHeader.DebugDirectorySize;

        if ( (allHeaderSize > loSize) || (allHeaderSize < sizeof mHeader) )
            return E_BAD_FORMAT;

        mSecHeaderBuf = new BYTE[ secAndDirSize ];
        if ( mSecHeaderBuf == NULL )
            return E_OUTOFMEMORY;

        bRet = ReadFile( hFile, mSecHeaderBuf, secAndDirSize, &bytesRead, NULL );
        if ( !bRet )
            return GetLastHr();

        mHFile.Attach( hFile.Detach() );
        mHMapping.Attach( hMapping.Detach() );

        return S_OK;
    }

    bool DbgFile::Validate( DWORD timeDateStamp )
    {
        return mHeader.TimeDateStamp == timeDateStamp;
    }

    const IMAGE_SEPARATE_DEBUG_HEADER* DbgFile::GetHeader()
    {
        return &mHeader;
    }

    const IMAGE_SECTION_HEADER* DbgFile::GetSectionHeaders()
    {
        return (IMAGE_SECTION_HEADER*) mSecHeaderBuf;
    }

    const IMAGE_DEBUG_DIRECTORY* DbgFile::GetDebugDirs()
    {
        if ( mSecHeaderBuf == NULL )
            return NULL;

        IMAGE_SECTION_HEADER* secLimit = (IMAGE_SECTION_HEADER*) mSecHeaderBuf + mHeader.NumberOfSections;
        return (IMAGE_DEBUG_DIRECTORY*) secLimit;
    }

    HRESULT DbgFile::GetFileHandle( HANDLE& handle )
    {
        BOOL            bRet = FALSE;
        FileHandlePtr   hFile;

        bRet = DuplicateHandle( 
            GetCurrentProcess(), 
            mHFile, 
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

    HRESULT DbgFile::GetMappingHandle( HANDLE& handle )
    {
        BOOL            bRet = FALSE;
        HandlePtr       hMapping;

        bRet = DuplicateHandle( 
            GetCurrentProcess(), 
            mHMapping, 
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
}
