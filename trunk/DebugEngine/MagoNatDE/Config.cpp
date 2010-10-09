/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Config.h"
#include "MagoNatDE_i.h"


// {B9D303A5-4EC7-4444-A7F8-6BFA4C7977EF}
static const GUID gGuidDLang = 
{ 0xb9d303a5, 0x4ec7, 0x4444, { 0xa7, 0xf8, 0x6b, 0xfa, 0x4c, 0x79, 0x77, 0xef } };

static const wchar_t*   gStrings[] = 
{
    NULL,

    L"No symbols have been loaded for this document.",
    L"No executable code is associated with this line.",

    L"CPU",
    L"CPU Segments",
    L"Floating Point",
    L"Flags",
    L"MMX",
    L"SSE",
    L"SSE2",
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
