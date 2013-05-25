/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

enum TOK;


TOK MapToKeyword( const wchar_t* id );
TOK MapToKeyword( const wchar_t* id, size_t len );
