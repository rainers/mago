/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Config.h"
#include "MagoNatDE_i.h"

#include "../MagoNatEE/Common.h"

#define MAGO_SUBKEY             L"SOFTWARE\\MagoDebugger"

// {B9D303A5-4EC7-4444-A7F8-6BFA4C7977EF}
static const GUID gGuidDLang = 
{ 0xb9d303a5, 0x4ec7, 0x4444, { 0xa7, 0xf8, 0x6b, 0xfa, 0x4c, 0x79, 0x77, 0xef } };

// {3B476D35-A401-11D2-AAD4-00C04F990171}
static const GUID gGuidWin32ExceptionType = 
{ 0x3B476D35, 0xA401, 0x11D2, { 0xAA, 0xD4, 0x00, 0xC0, 0x4F, 0x99, 0x01, 0x71 } };

static const wchar_t*   gStrings[] = 
{
    NULL,

    L"No symbols have been loaded for this document.",
    L"No executable code is associated with this line.",
    L"This is an invalid address.",

    L"CPU",
    L"CPU Segments",
    L"Floating Point",
    L"Flags",
    L"MMX",
    L"SSE",
    L"SSE2",

    L"Line",
    L"bytes",

    L"%1$s has triggered a breakpoint.",
    L"First-chance exception: %1$s",
    L"Unhandled exception: %1$s",
};


const GUID& GetEngineId()
{
    return __uuidof( MagoNativeEngine );
}

const wchar_t* GetEngineName()
{
    return L"Mago Native";
}

const GUID& GetDLanguageId()
{
    return gGuidDLang;
}

const GUID& GetDExceptionType()
{
    // we use the engine ID as the guid type for D exceptions
    return __uuidof( MagoNativeEngine );
}

const GUID& GetWin32ExceptionType()
{
    return gGuidWin32ExceptionType;
}

const wchar_t* GetRootDExceptionName()
{
    return L"D Exceptions";
}

const wchar_t* GetRootWin32ExceptionName()
{
    return L"Win32 Exceptions";
}

const wchar_t* GetString( DWORD strId )
{
    if ( strId >= _countof( gStrings ) )
        return NULL;

    return gStrings[strId];
}

bool GetString( DWORD strId, CString& str )
{
    const wchar_t*  s = GetString( strId );

    if ( s == NULL )
        return false;

    str = s;
    return true;
}

LSTATUS OpenRootRegKey( bool user, bool readWrite, HKEY& hKey )
{
    REGSAM samDesired = readWrite ? (KEY_READ | KEY_WRITE) : KEY_READ;
    HKEY hive = user ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    return RegOpenKeyEx( hive, MAGO_SUBKEY, 0, samDesired, &hKey );
}

LSTATUS GetRegString( HKEY hKey, const wchar_t* valueName, wchar_t* charBuf, int& charLen )
{
    if ( charBuf == NULL || charLen < 0 )
        return ERROR_INVALID_PARAMETER;

    DWORD   regType = 0;
    DWORD   bufLen = charLen * sizeof( wchar_t );
    DWORD   bytesRead= bufLen;
    LSTATUS ret = 0;

    ret = RegQueryValueEx(
        hKey,
        valueName,
        NULL,
        &regType,
        (BYTE*) charBuf,
        &bytesRead );
    if ( ret != ERROR_SUCCESS )
        return ret;

    if ( regType != REG_SZ || (bytesRead % sizeof( wchar_t )) != 0 )
        return ERROR_UNSUPPORTED_TYPE;

    int charsRead = bytesRead / sizeof( wchar_t );

    if ( charsRead == 0 || charBuf[charsRead - 1] != L'\0' )
    {
        // there's no more room to add a null
        if ( charsRead == charLen )
            return ERROR_MORE_DATA;

        charBuf[charsRead] = L'\0';
        charLen = charsRead + 1;
    }
    else
    {
        // the string we read already ends in null
        charLen = charsRead;
    }

    return ERROR_SUCCESS;
}

LSTATUS GetRegValue( HKEY hKey, const wchar_t* valueName, DWORD* pValue )
{
    if ( pValue == NULL )
        return ERROR_INVALID_PARAMETER;

    DWORD   regType = 0;
    DWORD   bytesRead = sizeof( *pValue );
    LSTATUS ret = 0;

    ret = RegQueryValueEx(
        hKey,
        valueName,
        NULL,
        &regType,
        (BYTE*) pValue,
        &bytesRead );
    if ( ret != ERROR_SUCCESS )
        return ret;

    if ( regType != REG_DWORD )
        return ERROR_UNSUPPORTED_TYPE;

    return ERROR_SUCCESS;
}

MagoOptions gOptions;

bool readMagoOptions()
{
    HKEY hKey;
    LSTATUS hr = OpenRootRegKey( true, false, hKey );
    if( hr != S_OK )
        return false;

    DWORD val;
    if ( GetRegValue( hKey, L"hideInternalNames", &val ) == S_OK )
        gOptions.hideInternalNames = val != 0;
    else
        gOptions.hideInternalNames = false;

    if ( GetRegValue( hKey, L"showStaticsInAggr", &val ) == S_OK )
        gOptions.showStaticsInAggr = val != 0;
    else
        gOptions.showStaticsInAggr = false;

    if ( GetRegValue( hKey, L"showVTable", &val ) == S_OK )
        gOptions.showVTable = val != 0;
    else
        gOptions.showVTable = true;

    if ( GetRegValue( hKey, L"flatClassFields", &val ) == S_OK )
        gOptions.flatClassFields = val != 0;
    else
        gOptions.flatClassFields = false;

    if ( GetRegValue( hKey, L"expandableStrings", &val ) == S_OK )
        gOptions.expandableStrings = val != 0;
    else
        gOptions.expandableStrings = false;

    if ( GetRegValue( hKey, L"hideReferencePointers", &val ) == S_OK )
        gOptions.hideReferencePointers = val != 0;
    else
        gOptions.hideReferencePointers = true;

    if ( GetRegValue( hKey, L"removeLeadingHexZeroes", &val ) == S_OK )
        gOptions.removeLeadingHexZeroes = val != 0;
    else
        gOptions.removeLeadingHexZeroes = false;

    if ( GetRegValue( hKey, L"recombineTuples", &val ) == S_OK )
        gOptions.recombineTuples = val != 0;
    else
        gOptions.recombineTuples = true;

    if (GetRegValue(hKey, L"maxArrayElements", &val) == S_OK)
        gOptions.maxArrayElements = val;
    else
        gOptions.maxArrayElements = 1000;

    if (GetRegValue(hKey, L"showDArrayLengthInType", &val) == S_OK)
        gOptions.showDArrayLengthInType = val != 0;
    else
        gOptions.showDArrayLengthInType = false;

    if (GetRegValue(hKey, L"callDebuggerFunctions", &val) == S_OK)
        gOptions.callDebuggerFunctions = val != 0;
    else
        gOptions.callDebuggerFunctions = true;

    if (GetRegValue(hKey, L"callDebuggerUseMagoGC", &val) == S_OK)
        gOptions.callDebuggerUseMagoGC = val != 0;
    else
        gOptions.callDebuggerUseMagoGC = true;

    MagoEE::gShowVTable = gOptions.showVTable;
    MagoEE::gMaxArrayLength = gOptions.maxArrayElements;
    MagoEE::gHideReferencePointers = gOptions.hideReferencePointers;
    MagoEE::gRemoveLeadingHexZeroes = gOptions.removeLeadingHexZeroes;
    MagoEE::gRecombineTuples = gOptions.recombineTuples;
    MagoEE::gShowDArrayLengthInType = gOptions.showDArrayLengthInType;
    MagoEE::gCallDebuggerFunctions = gOptions.callDebuggerFunctions;
    MagoEE::gCallDebuggerUseMagoGC = gOptions.callDebuggerUseMagoGC;

    RegCloseKey( hKey );
    return true;
}

static bool initMagoOption = readMagoOptions();
