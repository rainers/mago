/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Utility.h"


HRESULT GetImageInfoFromPEHeader( HANDLE hProcess, void* dllBase, uint16_t& machine, uint32_t& size, Address& prefBase )
{
    IMAGE_DOS_HEADER    dosHeader = { 0 };
    SIZE_T              cActual = 0;

    if ( !::ReadProcessMemory( hProcess, dllBase, &dosHeader, sizeof dosHeader, &cActual ) ) 
    {
        _ASSERT( !"Failed to read IMAGE_DOS_HEADER from loaded module" );
        return GetLastHr();
    }

    IMAGE_NT_HEADERS    ntHeaders = { 0 };
    void*               ntHeadersAddr = (void*) ((DWORD_PTR) dosHeader.e_lfanew + (DWORD_PTR) dllBase);
    
    if ( !::ReadProcessMemory( hProcess, ntHeadersAddr, &ntHeaders, sizeof ntHeaders, &cActual ) ) 
    {
        _ASSERT( !"Failed to read IMAGE_NT_HEADERS from loaded module" );
        return GetLastHr();
    }

    // These fields line up for 32 and 64-bit IMAGE_NT_HEADERS; make sure of it
    // otherwise, we would have had to check fileHeader.Characteristics & IMAGE_FILE_32BIT_MACHINE
    C_ASSERT( &((IMAGE_NT_HEADERS32*) 0)->OptionalHeader.SizeOfImage == 
              &((IMAGE_NT_HEADERS64*) 0)->OptionalHeader.SizeOfImage );
    C_ASSERT( &((IMAGE_NT_HEADERS32*) 0)->FileHeader.Machine == 
              &((IMAGE_NT_HEADERS64*) 0)->FileHeader.Machine );

    machine = ntHeaders.FileHeader.Machine;
    size = ntHeaders.OptionalHeader.SizeOfImage;
    prefBase = ntHeaders.OptionalHeader.ImageBase;

    return S_OK;
}


HRESULT GetImageInfo( const wchar_t* path, ImageInfo& info )
{
    FileHandlePtr   hFile;

    hFile = CreateFile(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL );
    if ( hFile.IsEmpty() )
        return GetLastHr();

    BOOL                bRet = FALSE;
    DWORD               bytesRead = 0;
    IMAGE_DOS_HEADER    dosHeader;

    bRet = ReadFile( hFile.Get(), &dosHeader, sizeof dosHeader, &bytesRead, NULL );
    if ( !bRet )
        return GetLastHr();

    if ( dosHeader.e_magic != IMAGE_DOS_SIGNATURE || dosHeader.e_lfanew < 0 )
        return HRESULT_FROM_WIN32( ERROR_BAD_FORMAT );

    DWORD filePtr = SetFilePointer( hFile.Get(), dosHeader.e_lfanew, NULL, FILE_BEGIN );
    if ( filePtr == INVALID_SET_FILE_POINTER )
        return HRESULT_FROM_WIN32( ERROR_BAD_FORMAT );

    DWORD signature = 0;

    bRet = ReadFile( hFile.Get(), &signature, sizeof signature, &bytesRead, NULL );
    if ( !bRet )
        return GetLastHr();

    bRet = ReadFile( hFile.Get(), &info.MachineType, sizeof info.MachineType, &bytesRead, NULL );
    if ( !bRet )
        return GetLastHr();

    return S_OK;
}


HRESULT ReadMemory( 
   HANDLE hProcess, 
   UINT_PTR address, 
   SIZE_T length, 
   SIZE_T& lengthRead, 
   SIZE_T& lengthUnreadable, 
   uint8_t* buffer )
{
    _ASSERT( hProcess != NULL );
    _ASSERT( buffer != NULL );
    _ASSERT( length < limit_max( length ) );

    HRESULT hr = S_OK;
    BOOL    bRet = FALSE;

    MEMORY_BASIC_INFORMATION    memInfo = { 0 };
    SIZE_T      sizeRet = 0;
    SIZE_T      lenReadable = 0;
    SIZE_T      lenUnreadable = 0;
    UINT_PTR    nextAddr = address;

    while ( ((lenReadable + lenUnreadable) < length) && (nextAddr >= address) )
    {
        bool    readable = true;

        sizeRet = VirtualQueryEx( hProcess, (void*) nextAddr, &memInfo, sizeof memInfo );
        if ( sizeRet == 0 )
            return HRESULT_FROM_WIN32( ERROR_PARTIAL_COPY );

        if ( (memInfo.State != MEM_COMMIT)
            || (memInfo.Protect == 0)
            || ((memInfo.Protect & PAGE_NOACCESS) != 0) )
            readable = false;

        SIZE_T  curSize = memInfo.RegionSize - (nextAddr - (UINT_PTR) memInfo.BaseAddress);

        if ( readable )
        {
            // we went from (readable to) unreadable to readable, 
            // this last readable won't be returned, so we finished
            if ( lenUnreadable > 0 )
                break;

            lenReadable += curSize;
        }
        else
        {
            lenUnreadable += curSize;
        }

        nextAddr = (UINT_PTR) memInfo.BaseAddress + memInfo.RegionSize;
    }
    
    // cap the length to read to the length the user asked for
    SIZE_T  lenToRead = (lenReadable > length) ? length : lenReadable;

    bRet = ::ReadProcessMemory( 
        hProcess, 
        (const void*) address, 
        buffer, 
        lenToRead, 
        &lengthRead );
    if ( !bRet )
        return GetLastHr();

    lengthUnreadable = (lenUnreadable > (length - lengthRead)) ? length - lengthRead : lenUnreadable;

    _ASSERT( lengthRead <= length );
    _ASSERT( lengthUnreadable <= length );
    _ASSERT( (lengthRead + lengthUnreadable) <= length );

    return hr;
}
