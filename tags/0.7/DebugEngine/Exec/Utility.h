/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


HRESULT GetImageInfoFromPEHeader( HANDLE hProcess, void* dllBase, uint16_t& machine, uint32_t& size, Address& prefBase );

HRESULT ReadMemory( 
    HANDLE hProcess, 
    UINT_PTR address, 
    SIZE_T length, 
    SIZE_T& lengthRead, 
    SIZE_T& lengthUnreadable, 
    uint8_t* buffer );


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
