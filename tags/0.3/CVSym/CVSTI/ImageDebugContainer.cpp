/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ImageDebugContainer.h"
#include "ImageAddrMap.h"
#include "STIUtil.h"
#include "ILoadCallback.h"

using namespace std;
using namespace boost;
using namespace BinImage;


namespace MagoST
{
    HRESULT ImageDebugContainer::LoadExe( const wchar_t* filename, ILoadCallback* callback )
    {
        HRESULT     hr = S_OK;
        auto_ptr<ImageFile>   image( new ImageFile() );
        DataDirInfo dirInfo = { 0 };
        HandlePtr   hMapping;
        DWORD       viewAlignedAddr = 0;
        DWORD       viewDiff = 0;
        MappedPtr   view;
        DWORD       debugDirCount = 0;
        IMAGE_DEBUG_DIRECTORY*  debugDir = NULL;

        if ( image.get() == NULL )
            return E_OUTOFMEMORY;

        hr = image->LoadFile( filename );
        if ( FAILED( hr ) )
            return hr;

        if ( !image->FindDataDirectoryData( IMAGE_DIRECTORY_ENTRY_DEBUG, dirInfo ) )
            return E_FAIL;

        hr = image->GetMappingHandle( hMapping.Ref() );
        if ( FAILED( hr ) )
            return hr;

        AlignAddress( dirInfo.FileOffset, viewAlignedAddr, viewDiff );

        view = MapViewOfFile( hMapping, FILE_MAP_READ, 0, viewAlignedAddr, dirInfo.Size + viewDiff );
        if ( view.IsEmpty() )
            return GetLastHr();

        debugDir = (IMAGE_DEBUG_DIRECTORY*) ((BYTE*) view.Get() + viewDiff);

        if ( callback != NULL )
            callback->NotifyDebugDir( true, dirInfo.Size, (BYTE*) debugDir );

        for ( debugDirCount = dirInfo.Size / sizeof *debugDir; debugDirCount > 0; debugDirCount--, debugDir++ )
        {
            if ( (debugDir->Type == IMAGE_DEBUG_TYPE_CODEVIEW)
                || (debugDir->Type == IMAGE_DEBUG_TYPE_MISC) )
                break;
        }

        if ( debugDirCount == 0 )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        if ( debugDir->Type == IMAGE_DEBUG_TYPE_MISC )
            return LoadDbg( image.get(), debugDir, callback );

        mDebugDir = *debugDir;
        mImage.reset( image.release() );

        return S_OK;
    }

    void ImageDebugContainer::AlignAddress( DWORD unalignedAddr, DWORD& alignedAddr, DWORD& diff )
    {
        SYSTEM_INFO sysInfo = { 0 };

        GetSystemInfo( &sysInfo );

        alignedAddr = (unalignedAddr / sysInfo.dwAllocationGranularity) * sysInfo.dwAllocationGranularity;
        diff = unalignedAddr - alignedAddr;
    }

    HRESULT ImageDebugContainer::LoadDbg( 
        BinImage::ImageFile* image, 
        const IMAGE_DEBUG_DIRECTORY* miscDebugDir, 
        ILoadCallback* callback )
    {
        _ASSERT( image != NULL );
        _ASSERT( miscDebugDir != NULL );

        HRESULT     hr = S_OK;
        HandlePtr   hMapping;
        DWORD       viewAlignedAddr = 0;
        DWORD       viewDiff = 0;
        MappedPtr   view;
        IMAGE_DEBUG_MISC*       misc = NULL;
        scoped_array<wchar_t>   strBuf;
        DWORD       dataLen = 0;

        if ( miscDebugDir->SizeOfData < sizeof( IMAGE_DEBUG_MISC ) )
            return E_FAIL;

        hr = image->GetMappingHandle( hMapping.Ref() );
        if ( FAILED( hr ) )
            return hr;

        AlignAddress( miscDebugDir->PointerToRawData, viewAlignedAddr, viewDiff );

        view = MapViewOfFile( hMapping, FILE_MAP_READ, 0, viewAlignedAddr, miscDebugDir->SizeOfData + viewDiff );
        if ( view.IsEmpty() )
            return GetLastHr();

        misc = (IMAGE_DEBUG_MISC*) view.Get();
        dataLen = misc->Length - offsetof( IMAGE_DEBUG_MISC, Data );

        if ( misc->DataType != IMAGE_DEBUG_MISC_EXENAME )
            return E_FAIL;

        // I don't know that the string in data is definitely \0 terminated, so we force it
        if ( misc->Unicode )
        {
            size_t  charCount = wcsnlen( (wchar_t*) misc->Data, dataLen / sizeof( wchar_t ) );

            // TODO: why were we calling wcsnlen twice?
            //charCount = wcsnlen( (wchar_t*) misc->Data, charCount );
            strBuf.reset( new wchar_t[ charCount + 1 ] );
            if ( strBuf.get() == NULL )
                return E_OUTOFMEMORY;

            // adds terminator
            wcsncpy_s( strBuf.get(), charCount + 1, (wchar_t*) misc->Data, charCount );
        }
        else
        {
            size_t  charCount = strnlen( (char*) misc->Data, dataLen );
            int     nChars = 0;

            nChars = MultiByteToWideChar( 
                CP_ACP, 
                MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                (char*) misc->Data,
                charCount,
                NULL,
                0 );
            if ( nChars == 0 )
                return GetLastHr();

            strBuf.reset( new wchar_t[ nChars + 1 ] );
            if ( strBuf.get() == NULL )
                return E_OUTOFMEMORY;

            nChars = MultiByteToWideChar( 
                CP_ACP, 
                MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                (char*) misc->Data,
                charCount,
                strBuf.get(),
                nChars + 1 );
            if ( nChars == 0 )
                return GetLastHr();

            strBuf[nChars] = L'\0';
        }

        hr = LoadDbg( strBuf.get(), callback );

        return hr;
    }

    HRESULT ImageDebugContainer::LoadDbg( const wchar_t* filename, ILoadCallback* callback )
    {
        HRESULT     hr = S_OK;
        DWORD       debugDirCount = 0;
        auto_ptr<DbgFile>               dbg( new DbgFile() );
        const IMAGE_DEBUG_DIRECTORY*    debugDir = NULL;

        if ( dbg.get() == NULL )
            return E_OUTOFMEMORY;

        hr = dbg->LoadFile( filename );
        if ( callback != NULL )
            callback->NotifyOpenDBG( filename, hr );
        if ( FAILED( hr ) )
            return hr;

        debugDir = dbg->GetDebugDirs();
        if ( debugDir == NULL )
            return E_FAIL;

        if ( callback != NULL )
            callback->NotifyDebugDir( false, dbg->GetHeader()->DebugDirectorySize, (BYTE*) debugDir );

        debugDirCount = dbg->GetHeader()->DebugDirectorySize / sizeof( IMAGE_DEBUG_DIRECTORY );
        for ( ; debugDirCount > 0; debugDirCount--, debugDir++ )
        {
            if ( debugDir->Type == IMAGE_DEBUG_TYPE_CODEVIEW )
                break;
        }

        if ( debugDirCount == 0 )
            return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

        mDebugDir = *debugDir;
        mDbg.reset( dbg.release() );

        return S_OK;
    }

    HRESULT ImageDebugContainer::LockDebugSection( BYTE*& bytes, DWORD& size )
    {
        HRESULT     hr = S_OK;
        HandlePtr   hMapping;
        DWORD       viewAlignedAddr = 0;
        DWORD       viewDiff = 0;
        MappedPtr   view;

        if ( mImage.get() != NULL )
        {
            hr = mImage->GetMappingHandle( hMapping.Ref() );
        }
        else if ( mDbg.get() != NULL )
        {
            hr = mDbg->GetMappingHandle( hMapping.Ref() );
        }
        else
            hr = E_FAIL;

        if ( FAILED( hr ) )
            return hr;

        AlignAddress( mDebugDir.PointerToRawData, viewAlignedAddr, viewDiff );

        view = MapViewOfFile( hMapping, FILE_MAP_READ, 0, viewAlignedAddr, mDebugDir.SizeOfData + viewDiff );
        if ( view.IsEmpty() )
            return GetLastHr();

        bytes = (BYTE*) view.Detach() + viewDiff;
        size = mDebugDir.SizeOfData;

        return S_OK;
    }

    HRESULT ImageDebugContainer::UnlockDebugSection( BYTE* bytes )
    {
        BOOL    bRet = UnmapViewOfFile( bytes );
        if ( !bRet )
            return GetLastHr();

        return S_OK;
    }

    bool ImageDebugContainer::HasAddressMap()
    {
        return true;
    }

    HRESULT ImageDebugContainer::GetAddressMap( IAddressMap*& map )
    {
        HRESULT                 hr = S_OK;
        RefPtr<ImageAddrMap>    addrMap = new ImageAddrMap();

        if ( addrMap == NULL )
            return E_OUTOFMEMORY;

        if ( mImage.get() != NULL )
        {
            IMAGE_NT_HEADERS*   ntHeaders = (IMAGE_NT_HEADERS*) mImage->GetNtHeadersBase();

            hr = addrMap->LoadFromSections( 
                ntHeaders->FileHeader.NumberOfSections, 
                mImage->GetSectionHeaders() );
        }
        else if ( mDbg.get() != NULL )
        {
            IMAGE_SEPARATE_DEBUG_HEADER*   ntHeaders = (IMAGE_SEPARATE_DEBUG_HEADER*) mDbg->GetHeader();

            hr = addrMap->LoadFromSections( 
                (uint16_t) ntHeaders->NumberOfSections, 
                mDbg->GetSectionHeaders() );
        }
        else
            hr = E_FAIL;

        if ( FAILED( hr ) )
            return hr;

        map = addrMap.Detach();

        return S_OK;
    }
}
