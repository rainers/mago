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

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// C
#include <crtdbg.h>
#include <inttypes.h>

// STL
#include <list>
#include <vector>

// Boost
#include <boost/scoped_array.hpp>

// Windows
#include <windows.h>

// Magus
#include <SmartPtr.h>

// CVSym project
#include "..\CVSym\Error.h"
#include "..\CVSym\CVSym.h"

// BinImage project
#include "..\BinImage\BinImage.h"

// Windows declarations that I don't want
#undef max
#undef min
