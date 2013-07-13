/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


inline HRESULT GetLastHr()
{
    return HRESULT_FROM_WIN32( GetLastError() );
}

struct MappedCloser
{
    void operator()( void* p )
    {
        BOOL bRet = UnmapViewOfFile( p );
        UNREFERENCED_PARAMETER( bRet );
        _ASSERT( bRet );
    }
};

typedef HandlePtrBase2<void*, NULL, MappedCloser> MappedPtr;
