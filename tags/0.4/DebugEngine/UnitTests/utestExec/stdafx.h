/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRTDBG_MAP_ALLOC

#include "targetver.h"

// C
#include <stdio.h>
#include <tchar.h>
#include <inttypes.h>
#include <crtdbg.h>

// C++
#include <iostream>
#include <fstream>
#include <list>
#include <map>

// Windows
#include <windows.h>

// Boost
#include <boost/shared_ptr.hpp>

// Other
#include <cpptest.h>
#include <SmartPtr.h>

// Exec (test target)
#include "..\..\Exec\Types.h"
#include "..\..\Exec\Exec.h"
#include "..\..\Exec\Error.h"
#include "..\..\Exec\EventCallback.h"
#include "..\..\Exec\Machine.h"
#include "..\..\Exec\MakeMachineX86.h"
#include "..\..\Exec\IProcess.h"
#include "..\..\Exec\IModule.h"
#include "..\..\Exec\Thread.h"
#include "..\..\Exec\Enumerator.h"

// This project
#include "Utility.h"


const wchar_t   SimplestDebuggee[] = L"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\Debug\\test1.exe";
const wchar_t   SleepingDebuggee[] = L"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\Debug\\testSleep.exe";
const wchar_t   OptionsDebuggee[] = L"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\Debug\\testOptions.exe";
const wchar_t   EventsDebuggee[] = L"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\Debug\\testEvents.exe";
const wchar_t   StepOneThreadDebuggee[] = L"F:\\Users\\Magus\\Documents\\Visual Studio 2008\\Projects\\Debugger1\\Debug\\testStepOneThread.exe";


#define TEST_ASSERT_RETURN( expr )                                  \
    {                                                               \
        if (!(expr))                                                \
        {                                                           \
            assertment(::Test::Source(__FILE__, __LINE__, #expr));  \
            return;                                                 \
        }                                                           \
    }

#define TEST_ASSERT_RETURN_MSG( expr, msg )                         \
    {                                                               \
        if (!(expr))                                                \
        {                                                           \
            assertment(::Test::Source(__FILE__, __LINE__, msg));    \
            return;                                                 \
        }                                                           \
    }

#define TEST_FAIL_MSG( msg ) \
    {                                                               \
        assertment(::Test::Source(__FILE__, __LINE__, (msg) != 0 ? msg : "")); \
        if (!continue_after_failure()) return;                      \
    }
