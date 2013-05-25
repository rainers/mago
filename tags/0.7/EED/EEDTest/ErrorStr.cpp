/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ErrorStr.h"


static const wchar_t*  gErrStrs[] = 
{
    L"Expression couldn't be evaluated",
    L"Syntax error",
    L"Incompatible types for operator",
    L"Value expected",
    L"Expression has no type",
    L"Type resolve failed",
    L"Bad cast",
    L"Expression has no address",
    L"L-value expected",
    L"Can't cast to bool",
    L"Divide by zero",
    L"Bad indexing operation",
};

static wchar_t  gTempStr[256] = L"";


const wchar_t*  GetErrorString( HRESULT hr )
{
    DWORD   fac = HRESULT_FACILITY( hr );
    DWORD   code = HRESULT_CODE( hr );

    if ( fac != MagoEE::HR_FACILITY )
    {
        DWORD   nRet = 0;
        
        nRet = FormatMessage(
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            hr,
            0,
            gTempStr,
            _countof( gTempStr ),
            NULL );
        if ( nRet == 0 )
            return gErrStrs[0];

        return gTempStr;
    }

    if ( code >= _countof( gErrStrs ) )
        return gErrStrs[0];

    return gErrStrs[code];
}
