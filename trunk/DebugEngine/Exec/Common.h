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

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// C
#include <stdlib.h>
#include <inttypes.h>
#include <crtdbg.h>

// C++
#include <string>
#include <list>
#include <map>
#include <vector>
#include <limits>

// Boost
#include <boost/scoped_array.hpp>
#include <boost/type_traits/remove_pointer.hpp>

// Windows
#include <windows.h>
// these are just a few of many that are more trouble than they're worth
#undef min
#undef max

// Magus
#include <SmartPtr.h>


const DWORD     NO_DEBUG_EVENT = 0;


// This project
#include "Types.h"
#include "Utility.h"
#include "Log.h"
#include "Error.h"
