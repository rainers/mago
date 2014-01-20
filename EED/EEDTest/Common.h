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

// C
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <tchar.h>
#include <inttypes.h>
#include <crtdbg.h>

// C++
#include <string>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <iostream>

// Boost
#include <boost/shared_ptr.hpp>

// Windows
#include <windows.h>
#include <msxml6.h>

// ATL
#include <atlbase.h>

// Magus
#include <SmartPtr.h>

#include "..\Real\Real.h"
#include "..\Real\Complex.h"
#include "..\EED\EED.h"
