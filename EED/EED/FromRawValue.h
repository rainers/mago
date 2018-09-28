/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

struct Real10;

namespace MagoEE
{
	class Type;
	union DataValue;

	HRESULT WriteInt( uint8_t* buffer, uint32_t bufSize, Type* type, uint64_t val );
	HRESULT WriteFloat( uint8_t* buffer, uint32_t bufSize, Type* type, const Real10& val );

	uint64_t ReadInt( const void* srcBuf, uint32_t bufOffset, size_t size, bool isSigned );
	Real10 ReadFloat( const void* srcBuf, uint32_t bufOffset, Type* type );
	HRESULT FromRawValue( const void* srcBuf, Type* type, DataValue& value );
}
