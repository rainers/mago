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


const HRESULT E_BAD_FORMAT          = HRESULT_FROM_WIN32( ERROR_BAD_FORMAT );


const DWORD MaxSectionCount = 96;
