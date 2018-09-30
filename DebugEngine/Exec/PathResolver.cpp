/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "PathResolver.h"
#include <Psapi.h>

using namespace std;


const size_t    MAX_BIG_PATH = 32767;


HRESULT PathResolver::Init()
{
    const size_t    MaxSize = MAX_PATH * 2;

    mScratchPath.resize( MaxSize, L'\0' );

    return S_OK;
}

HRESULT PathResolver::GetFilePath( HANDLE hProcess, HANDLE hFile, void* baseAddr, std::wstring& filePath )
{
    HRESULT hr = S_OK;
    wstring devPath;

    filePath.clear();

    hr = GetMappedDeviceName( hProcess, baseAddr );
    if ( FAILED( hr ) )
        return hr;

    devPath = &mScratchPath[0];

    hr = ResolveDeviceName( devPath, hFile, filePath );

    return hr;
}

HRESULT PathResolver::ResolveDeviceName( const std::wstring& devPath, HANDLE hFile, std::wstring& filePath )
{
    return DriveResolveDeviceName( devPath, hFile, filePath );
}

HRESULT PathResolver::DriveResolveDeviceName( const std::wstring& devPath, HANDLE hFile, std::wstring& filePath )
{
    // TODO: considering removing formal param if not used; can be used for verifying before caching mapped name
    hFile;      // TODO: getting rid of unused param warning, look at TODO above

    HRESULT hr = S_OK;
    DWORD   driveBits = 0;

    driveBits = GetLogicalDrives();
    if ( driveBits == 0 )
    {
        hr = GetLastHr();
        goto Error;
    }

    for ( unsigned int i = 0; i < 26; i++ )
    {
        // is the drive letter mapped?
        if ( (driveBits & (1 << i)) == 0 )
            continue;

        const wchar_t   DriveName[3] = { wchar_t( L'A' + i ), L':', 0 };
        DWORD           nameLen = 0;

        hr = QueryDosDeviceWithScratch( DriveName );
        if ( FAILED( hr ) )
            continue;

        // the first string in the multi-string is the active mapping, so ignore the rest
        nameLen = static_cast<DWORD>( wcslen( &mScratchPath[0] ) );

        if ( _wcsnicmp( &mScratchPath[0], devPath.c_str(), nameLen ) == 0 )
        {
            filePath.append( DriveName );
            filePath.append( devPath.c_str() + nameLen );

            // TODO: this would be where we verify the file exists and add the path to the cache
            return hr;
        }
    }

    hr = E_NOT_FOUND;

Error:
    return hr;
}

HRESULT PathResolver::GetModulePath( HANDLE hProcess, HMODULE hMod, std::wstring& path )
{
    HRESULT hr = S_OK;

    for ( ; ; )
    {
        DWORD   nRet = 0;

        // returns a file system path
        nRet = GetModuleFileNameEx( hProcess, hMod, &mScratchPath[0], static_cast<DWORD>( mScratchPath.size() ) );
        _ASSERT( nRet < mScratchPath.size() );
        if ( nRet == 0 )
        {
            // getting ERROR_PARTIAL_COPY here usually means we won't be able to attach
            hr = GetLastHr();
            goto Error;
        }
        else if ( nRet == mScratchPath.size() )
        {
            if ( !ExpandScratchPath() )
            {
                hr = E_INSUFFICIENT_BUFFER;
                goto Error;
            }
        }
        else
        {
            path = &mScratchPath[0];
            break;
        }
    }

Error:
    return hr;
}

HRESULT PathResolver::GetProcessModulePath( HANDLE hProcess, std::wstring& path )
{
    HRESULT hr = S_OK;

    for ( ; ; )
    {
        DWORD   nRet = 0;

        // returns a device path
        nRet = GetProcessImageFileName( hProcess, &mScratchPath[0], (DWORD) mScratchPath.size() );
        if ( nRet == 0 )
        {
            DWORD err = GetLastError();
            if ( err == ERROR_INSUFFICIENT_BUFFER )
            {
                if ( !ExpandScratchPath() )
                {
                    hr = E_INSUFFICIENT_BUFFER;
                    goto Error;
                }
            }
            else
            {
                // getting ERROR_PARTIAL_COPY here usually means we won't be able to attach
                hr = HRESULT_FROM_WIN32( err );
                goto Error;
            }
        }
        else
        {
            std::wstring devPath = &mScratchPath[0];
            hr = ResolveDeviceName( devPath, NULL, path );
            break;
        }
    }

Error:
    return hr;
}

HRESULT PathResolver::GetMappedDeviceName( HANDLE hProcess, void* baseAddr )
{
    HRESULT hr = S_OK;
    DWORD   charsCopied = 0;

    for ( ; ; )
    {
        // appends a '\0', but doesn't include it in count
        // truncates and returns your size, if necessary
        charsCopied = ::GetMappedFileName( 
            hProcess, 
            baseAddr, 
            &mScratchPath.at( 0 ), 
            static_cast<DWORD>( mScratchPath.size() ) );
        if ( charsCopied == 0 )
        {
            hr = GetLastHr();
            goto Error;
        }
        else if ( charsCopied == mScratchPath.size() )
        {
            if ( !ExpandScratchPath() )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
                goto Error;
            }
        }
        else
            break;
    }

Error:
    return hr;
}

HRESULT PathResolver::QueryDosDeviceWithScratch( const wchar_t* driveName )
{
    DWORD           totalLen = 0;

    for ( ; ; )
    {
        totalLen = QueryDosDevice( driveName, &mScratchPath.at( 0 ), static_cast<DWORD>( mScratchPath.size() ) );
        if ( totalLen != 0 )
            break;

        DWORD   err = GetLastError();

        if ( err != ERROR_INSUFFICIENT_BUFFER )
            return HRESULT_FROM_WIN32( err );

        if ( !ExpandScratchPath() )
            return HRESULT_FROM_WIN32( err );
    }

    return S_OK;
}

bool    PathResolver::ExpandScratchPath()
{
    size_t  size = mScratchPath.size();

    if ( size >= MAX_BIG_PATH )
        return false;

    size_t  newSize = size * 2;

    if ( newSize > MAX_BIG_PATH )
        newSize = MAX_BIG_PATH;

    mScratchPath.resize( newSize, L'\0' );

    return true;
}
