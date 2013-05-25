/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


HRESULT FindBasicType( const wchar_t* name, MagoEE::ITypeEnv* typeEnv, MagoEE::Declaration*& decl );


struct IDiaSymbol;

void PrintSymProps( IDiaSymbol* sym );
