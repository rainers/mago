/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <string>

union CodeViewFieldType;
union CodeViewSymbol;

struct SymString;

namespace MagoST
{
    struct Variant;
}


uint16_t GetNumLeafSize( uint16_t* numLeaf );
void     GetNumLeafValue( uint16_t* numLeaf, MagoST::Variant& value );
uint32_t GetUIntValue( uint16_t* numericLeaf );
DWORD    GetFieldLength( CodeViewFieldType* type );
bool     QuickGetAddrOffset( CodeViewSymbol* sym, uint32_t& offset );
bool     QuickGetName( CodeViewSymbol* sym, SymString& name );
bool     QuickGetAddrSegment( CodeViewSymbol* sym, uint16_t& segment );
bool     QuickGetLength( CodeViewSymbol* sym, uint32_t& length );

bool ExactFileNameMatch( const char* pathA, size_t pathALen, const char* pathB, size_t pathBLen );
bool PartialFileNameMatch( const char* pathA, size_t pathALen, const char* pathB, size_t pathBLen );

std::wstring SymStringToWString( const SymString& sym );
