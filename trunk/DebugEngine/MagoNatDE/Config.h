/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


const GUID& GetEngineId();
const wchar_t* GetEngineName();

const GUID& GetDLanguageId();

const GUID& GetDExceptionType();
const GUID& GetWin32ExceptionType();

const wchar_t* GetRootDExceptionName();
const wchar_t* GetRootWin32ExceptionName();

enum StringIds
{
    IDS_NONE,

    IDS_NO_SYMS_FOR_DOC,
    IDS_NO_CODE_FOR_LINE,
    IDS_INVALID_ADDRESS,

    IDS_REGGROUP_CPU,
    IDS_REGGROUP_CPU_SEGMENTS,
    IDS_REGGROUP_FLOATING_POINT,
    IDS_REGGROUP_FLAGS,
    IDS_REGGROUP_MMX,
    IDS_REGGROUP_SSE,
    IDS_REGGROUP_SSE2,

    IDS_LINE,
    IDS_BYTES,

    IDS_BP_TRIGGERED,
};

const wchar_t* GetString( DWORD strId );
bool GetString( DWORD strId, CString& str );
