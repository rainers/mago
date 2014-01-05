/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// Common.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// ATL
#include <atlbase.h>
#include <atlstr.h>

// C
#include <inttypes.h>
#include <crtdbg.h>

// Magus
#include <SmartPtr.h>
#include <Guard.h>

// Debug Exec
#include "..\Exec\Types.h"
#include "..\Exec\Enumerator.h"
#include "..\Exec\Error.h"
#include "..\Exec\Exec.h"
#include "..\Exec\EventCallback.h"
#include "..\Exec\IProcess.h"
#include "..\Exec\Thread.h"
#include "..\Exec\IModule.h"

// Windows declarations that I don't want
#undef max
#undef min
