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

struct MappedDeleter
{
    static void Delete( void* p )
    {
        BOOL bRet = UnmapViewOfFile( p );
        UNREFERENCED_PARAMETER( bRet );
        _ASSERT( bRet );
    }
};

typedef UniquePtrBase<void*, NULL, MappedDeleter> MappedPtr;
