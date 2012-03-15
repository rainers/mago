/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Log.h"

#include <stdio.h>


static const char*      gEventNames[] = 
{
    "No event",
    "EXCEPTION_DEBUG_EVENT",
    "CREATE_THREAD_DEBUG_EVENT",
    "CREATE_PROCESS_DEBUG_EVENT",
    "EXIT_THREAD_DEBUG_EVENT",
    "EXIT_PROCESS_DEBUG_EVENT",
    "LOAD_DLL_DEBUG_EVENT",
    "UNLOAD_DLL_DEBUG_EVENT",
    "OUTPUT_DEBUG_STRING_EVENT",
    "RIP_EVENT",
};


void Log::LogDebugEvent( const DEBUG_EVENT& event )
{
    if ( event.dwDebugEventCode >= _countof( gEventNames ) )
        return;

    const char* eventName = gEventNames[event.dwDebugEventCode];
    char        msg[80] = "";
    char        part[80] = "";

    _snprintf_s( msg, _TRUNCATE, "%s (%d) : PID=%d, TID=%d", 
        eventName, 
        event.dwDebugEventCode, 
        event.dwProcessId, 
        event.dwThreadId );

    if ( event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT )
    {
        _snprintf_s( part, _TRUNCATE, ", exc=%08x at %p", 
            event.u.Exception.ExceptionRecord.ExceptionCode,
            event.u.Exception.ExceptionRecord.ExceptionAddress );
        strncat_s( msg, part, _TRUNCATE );
    }

    printf( "%s\n", msg );

    strncat_s( msg, "\n", _TRUNCATE );
    OutputDebugStringA( msg );
}
