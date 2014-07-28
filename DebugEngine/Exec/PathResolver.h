/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


// TODO: speed up by caching
class PathResolver
{
    std::vector< wchar_t >  mScratchPath;

public:
    HRESULT Init();

    HRESULT GetFilePath( HANDLE hProcess, HANDLE hFile, void* baseAddr, std::wstring& path );
    HRESULT GetModulePath( HANDLE hProcess, HMODULE hMod, std::wstring& path );
    HRESULT GetProcessModulePath( HANDLE hProcess, std::wstring& path );

private:
    HRESULT ResolveDeviceName( const std::wstring& devPath, HANDLE hFile, std::wstring& filePath );
    HRESULT DriveResolveDeviceName( const std::wstring& devPath, HANDLE hFile, std::wstring& filePath );

    HRESULT GetMappedDeviceName( HANDLE hProcess, void* baseAddr );
    HRESULT QueryDosDeviceWithScratch( const wchar_t* driveName );
    
    bool    ExpandScratchPath();
};
