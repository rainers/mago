/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

// Common.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS  // some CString constructors will be explicit

// ATL
#ifdef _DEBUG
//#define _ATL_DEBUG_INTERFACES
#endif

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <atlstr.h>

using namespace ATL;

// C
#include <inttypes.h>
#include <crtdbg.h>

// C++
#include <string>
#include <map>
#include <vector>
#include <list>
#include <limits>

// Boost
#include <boost/scoped_array.hpp>

// VS Debug
#include <msdbg.h>

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

// Debug Symbol Table
#include <MagoCVSTI.h>

// This project
#include "Utility.h"
#include "Config.h"

// Windows declarations that I don't want
#undef max
#undef min
