/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


#define MAGO_VERSION_MAJOR      0
#define MAGO_VERSION_MINOR      1

#ifndef MAGO_VERSION_BUILD
#define MAGO_VERSION_BUILD      0
#endif

#define MAGO_VERSION_REVISION   0


#define MAGO_MAKE_STRING2( x )  #x
#define MAGO_MAKE_STRING( x )   MAGO_MAKE_STRING2( x )

#define MAGO_FILEVERSION        MAGO_VERSION_MAJOR,MAGO_VERSION_MINOR,MAGO_VERSION_BUILD,MAGO_VERSION_REVISION
#define MAGO_FILEVERSION_STR    MAGO_MAKE_STRING( MAGO_FILEVERSION )
#define MAGO_PRODVERSION        MAGO_FILEVERSION
#define MAGO_PRODVERSION_STR    MAGO_MAKE_STRING( MAGO_PRODVERSION )
