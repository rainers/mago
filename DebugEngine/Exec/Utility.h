/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


struct ImageInfo
{
    WORD    MachineType;
    DWORD   Size;
    Address PrefImageBase;
};


HRESULT GetProcessImageInfo( HANDLE hProcess, ImageInfo& imageInfo );
HRESULT GetLoadedImageInfo( HANDLE hProcess, void* dllBase, ImageInfo& imageInfo );
HRESULT GetImageInfo( const wchar_t* path, ImageInfo& info );

HRESULT ReadMemory( 
    HANDLE hProcess, 
    UINT_PTR address, 
    SIZE_T length, 
    SIZE_T& lengthRead, 
    SIZE_T& lengthUnreadable, 
    uint8_t* buffer );

typedef DWORD (__stdcall *ThreadControlProc)( HANDLE hThread );
class Process;

// there's no Wow64ResumeThread
HRESULT ControlThread( HANDLE hThread, ThreadControlProc controlProc );
HRESULT SuspendProcess( 
    Process* process,
    ThreadControlProc suspendProc );
HRESULT ResumeProcess(     
    Process* process,
    ThreadControlProc suspendProc );


inline HRESULT GetLastHr()
{
    return HRESULT_FROM_WIN32( GetLastError() );
}


template <class T>
inline T limit_max( const T& x )
{
    UNREFERENCED_PARAMETER( x );
    return std::numeric_limits<T>::max();
}


template <class T>
inline T limit_min( const T& x )
{
    UNREFERENCED_PARAMETER( x );
    return std::numeric_limits<T>::min();
}
