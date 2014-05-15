/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


// Same definitions as in the first version of sdkddkver.h, which is found in 
// VS 2008's Platform SDK; but, copy them here in case we're building on 2005

#ifndef _INC_SDKDDKVER

#define _WIN32_WINNT_NT4                    0x0400
#define _WIN32_WINNT_WIN2K                  0x0500
#define _WIN32_WINNT_WINXP                  0x0501
#define _WIN32_WINNT_WS03                   0x0502
#define _WIN32_WINNT_LONGHORN               0x0600

#endif


// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run 
// your application.  The macros work by enabling all features available on platform versions up to and 
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.

// Specifies that the minimum required platform is Windows Server 2003/Windows XP SPx.
// Change this to the appropriate value to target other versions of Windows.

#ifdef _WIN64

#ifndef WINVER                          
#define WINVER _WIN32_WINNT_LONGHORN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_LONGHORN
#endif

#else

#ifndef WINVER                          
#define WINVER _WIN32_WINNT_WS03
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WS03
#endif

#endif  // _WIN64


#ifndef _WIN32_WINDOWS          // Specifies that the minimum required platform is Windows 98.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE                       // Specifies that the minimum required platform is Internet Explorer 7.0.
#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
#endif
