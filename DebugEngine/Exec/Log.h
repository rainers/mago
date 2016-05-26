/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class Log
{
public:
    // call Enable(false) to disable logs
    static void Enable(bool enabled);
    static void LogDebugEvent( const DEBUG_EVENT& event );
};
