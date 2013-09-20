/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Utility.h"
#include "Process.h"
#include "Thread.h"
#include <Psapi.h>


#define MAX( a, b ) ((a) > (b) ? (a) : (b))


HRESULT GetProcessImageInfo( HANDLE hProcess, ImageInfo& imageInfo )
{
    HRESULT     hr = S_OK;
    BOOL        bRet = FALSE;
    HMODULE     hMods[1] = { 0 };
    DWORD       cbNeeded = 0;
    MODULEINFO  modInfo = { 0 };

    bRet = EnumProcessModules( hProcess, hMods, sizeof hMods, &cbNeeded );
    if ( !bRet )
        return GetLastHr();

    bRet = GetModuleInformation( hProcess, hMods[0], &modInfo, sizeof modInfo );
    if ( !bRet )
        return GetLastHr();

    hr = GetLoadedImageInfo( hProcess, modInfo.lpBaseOfDll, imageInfo );
    if ( FAILED( hr ) )
        return hr;

    return S_OK;
}

HRESULT GetLoadedImageInfo( HANDLE hProcess, void* dllBase, ImageInfo& imageInfo )
{
    IMAGE_DOS_HEADER    dosHeader = { 0 };
    SIZE_T              cActual = 0;

    if ( !::ReadProcessMemory( hProcess, dllBase, &dosHeader, sizeof dosHeader, &cActual ) ) 
    {
        _ASSERT( !"Failed to read IMAGE_DOS_HEADER from loaded module" );
        return GetLastHr();
    }

    BYTE    ntHeadersBuf[sizeof( IMAGE_NT_HEADERS64 )];
    void*   ntHeadersAddr = (void*) ((DWORD_PTR) dosHeader.e_lfanew + (DWORD_PTR) dllBase);
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*) ntHeadersBuf;

    // even if the file is 32-bit, it's OK if we read a little more
    if ( !::ReadProcessMemory( 
        hProcess, ntHeadersAddr, &ntHeadersBuf, sizeof( IMAGE_NT_HEADERS64 ), &cActual ) ) 
    {
        _ASSERT( !"Failed to read IMAGE_NT_HEADERS from loaded module" );
        return GetLastHr();
    }

    imageInfo.MachineType = ntHeaders->FileHeader.Machine;

    if ( ntHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC )
    {
        IMAGE_NT_HEADERS32* ntHeaders32 = (IMAGE_NT_HEADERS32*) ntHeadersBuf;
        imageInfo.Size = ntHeaders32->OptionalHeader.SizeOfImage;
        imageInfo.PrefImageBase = ntHeaders32->OptionalHeader.ImageBase;
    }
    else
    {
        imageInfo.Size = ntHeaders->OptionalHeader.SizeOfImage;
        imageInfo.PrefImageBase = (Address) ntHeaders->OptionalHeader.ImageBase;
    }

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
    BYTE                headerBuf[MAX( sizeof( IMAGE_DOS_HEADER ), sizeof( IMAGE_NT_HEADERS64 ) )];
    IMAGE_DOS_HEADER*   dosHeader = (IMAGE_DOS_HEADER*) headerBuf;
    IMAGE_NT_HEADERS64* ntHeaders = (IMAGE_NT_HEADERS64*) headerBuf;

    bRet = ReadFile( hFile.Get(), headerBuf, sizeof( IMAGE_DOS_HEADER ), &bytesRead, NULL );
    if ( !bRet )
        return GetLastHr();

    if ( dosHeader->e_magic != IMAGE_DOS_SIGNATURE || dosHeader->e_lfanew < 0 )
        return HRESULT_FROM_WIN32( ERROR_BAD_FORMAT );

    DWORD filePtr = SetFilePointer( hFile.Get(), dosHeader->e_lfanew, NULL, FILE_BEGIN );
    if ( filePtr == INVALID_SET_FILE_POINTER )
        return HRESULT_FROM_WIN32( ERROR_BAD_FORMAT );

    // even if the file is 32-bit, it's OK if we read a little more
    bRet = ReadFile( hFile.Get(), headerBuf, sizeof( IMAGE_NT_HEADERS64 ), &bytesRead, NULL );
    if ( !bRet )
        return GetLastHr();

    // all of these line up in 32 and 64-bit
    if ( ntHeaders->Signature != IMAGE_NT_SIGNATURE )
        return HRESULT_FROM_WIN32( ERROR_BAD_FORMAT );

    info.MachineType = ntHeaders->FileHeader.Machine;

    if ( ntHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC )
    {
        IMAGE_NT_HEADERS32* ntHeaders32 = (IMAGE_NT_HEADERS32*) headerBuf;
        info.Size = ntHeaders32->OptionalHeader.SizeOfImage;
        info.PrefImageBase = ntHeaders32->OptionalHeader.ImageBase;
    }
    else
    {
        info.Size = ntHeaders->OptionalHeader.SizeOfImage;
        info.PrefImageBase = (Address) ntHeaders->OptionalHeader.ImageBase;
    }

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

HRESULT ControlThread( HANDLE hThread, ThreadControlProc controlProc )
{
    _ASSERT( hThread != NULL );
    _ASSERT( controlProc != NULL );

    DWORD   suspendCount = controlProc( hThread );

    if ( suspendCount == (DWORD) -1 )
    {
        HRESULT hr = GetLastHr();

        // if the thread can't be accessed, then it's probably on the way out
        // and there's nothing we should do about it
        if ( hr == E_ACCESSDENIED )
            return S_OK;

        return hr;
    }

    return S_OK;
}

HRESULT SuspendProcess( Process* process, ThreadControlProc suspendProc )
{
    _ASSERT( process != NULL );
    _ASSERT( suspendProc != NULL );

    // already suspended?
    uint32_t count = process->GetSuspendCount();
    _ASSERT( count >= 0 && count < limit_max( count ) );
    if ( count > 0 )
    {
        process->SetSuspendCount( count + 1 );
        return S_OK;
    }

    HRESULT hr = S_OK;
    int     goodCount = 0;

    typedef Process::ThreadIterator It;

    for ( It it = process->ThreadsBegin(); it != process->ThreadsEnd(); it++, goodCount++ )
    {
        hr = ControlThread( it->Get()->GetHandle(), suspendProc );
        if ( FAILED( hr ) )
            goto Error;
    }

    process->SetSuspendCount( count + 1 );

Error:
    if ( FAILED( hr ) )
    {
        int i = 0;

        for ( It it = process->ThreadsBegin(); it != process->ThreadsEnd(); it++, i++ )
        {
            if ( i == goodCount )
                break;

            ControlThread( it->Get()->GetHandle(), ::ResumeThread );
        }
    }

    return hr;
}

HRESULT ResumeProcess( Process* process, ThreadControlProc suspendProc )
{
    _ASSERT( process != NULL );
    _ASSERT( suspendProc != NULL );

    // still suspended?
    uint32_t count = process->GetSuspendCount();
    _ASSERT( count > 0 );
    if ( count > 1 )
    {
        process->SetSuspendCount( count - 1 );
        return S_OK;
    }

    HRESULT hr = S_OK;
    int     goodCount = 0;

    typedef Process::ThreadIterator It;

    for ( It it = process->ThreadsBegin(); it != process->ThreadsEnd(); it++, goodCount++ )
    {
        hr = ControlThread( it->Get()->GetHandle(), ::ResumeThread );
        if ( FAILED( hr ) )
            goto Error;
    }

    process->SetSuspendCount( count - 1 );

Error:
    if ( FAILED( hr ) )
    {
        int i = 0;

        for ( It it = process->ThreadsBegin(); it != process->ThreadsEnd(); it++, i++ )
        {
            if ( i == goodCount )
                break;

            ControlThread( it->Get()->GetHandle(), suspendProc );
        }
    }

    return hr;
}
