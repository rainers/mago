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
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <errno.h>
#include <crtdbg.h>
#include <memory.h>

#include <inttypes.h>

// C++
#include <string>
#include <memory>
#include <vector>
#include <list>
#include <map>

// Boost
#include <boost/tr1/memory.hpp>
#include <boost/scoped_array.hpp>

// Windows
#include <windows.h>

// Magus
#include <SmartPtr.h>

// This project
#include "..\Real\Real.h"
#include "..\Real\Complex.h"


#undef min
#undef max


#include "EE.h"
