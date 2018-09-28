/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/


#include "Common.h"
#include "EED.h"

namespace MagoEE {

uint64_t ReadInt( const void* srcBuf, uint32_t bufOffset, size_t size, bool isSigned )
{
    union Integer
    {
        uint8_t     i8;
        uint16_t    i16;
        uint32_t    i32;
        uint64_t    i64;
    };
    Integer     i = { 0 };
    uint64_t    u = 0;

    _ASSERT( size <= sizeof u );

    memcpy( &i, (uint8_t*) srcBuf + bufOffset, size );

    switch ( size )
    {
    case 1:
        if ( isSigned )
            u = (int8_t) i.i8;
        else
            u = (uint8_t) i.i8;
        break;

    case 2:
        if ( isSigned )
            u = (int16_t) i.i16;
        else
            u = (uint16_t) i.i16;
        break;

    case 4:
        if ( isSigned )
            u = (int32_t) i.i32;
        else
            u = (uint32_t) i.i32;
        break;

    case 8:
        u = i.i64;
        break;
    }

    return u;
}

Real10 ReadFloat( const void* srcBuf, uint32_t bufOffset, MagoEE::Type* type )
{
    union Float
    {
        float   f32;
        double  f64;
        Real10  f80;
    };
    Float   f = { 0 };
    Real10  r;
    MagoEE::ENUMTY  ty = MagoEE::Tnone;
    size_t  size = 0;

    r.Zero();

    if ( type->IsComplex() )
    {
        switch ( type->GetSize() )
        {
        case 8:     ty = MagoEE::Tfloat32;  break;
        case 16:    ty = MagoEE::Tfloat64;  break;
        case 20:    ty = MagoEE::Tfloat80;  break;
        }
    }
    else    // is real or imaginary
    {
        switch ( type->GetSize() )
        {
        case 4:     ty = MagoEE::Tfloat32;  break;
        case 8:     ty = MagoEE::Tfloat64;  break;
        case 10:    ty = MagoEE::Tfloat80;  break;
        }
    }

    switch ( ty )
    {
    case MagoEE::Tfloat32:  size = 4;   break;
    case MagoEE::Tfloat64:  size = 8;   break;
    case MagoEE::Tfloat80:  size = 10;  break;
    }

    memcpy( &f, (uint8_t*) srcBuf + bufOffset, size );

    switch ( ty )
    {
    case MagoEE::Tfloat32:  r.FromFloat( f.f32 );   break;
    case MagoEE::Tfloat64:  r.FromDouble( f.f64 );  break;
    case MagoEE::Tfloat80:  r = f.f80;  break;
    }

    return r;
}

HRESULT WriteInt( uint8_t* buffer, uint32_t bufSize, MagoEE::Type* type, uint64_t val )
{
    union Integer
    {
        uint8_t     i8;
        uint16_t    i16;
        uint32_t    i32;
        uint64_t    i64;
    };

    uint32_t    size = type->GetSize();
    Integer     iVal = { 0 };

    _ASSERT( size <= bufSize );
    if ( size > bufSize )
        return E_FAIL;

    switch ( size )
    {
    case 1: iVal.i8 = (uint8_t) val;    break;
    case 2: iVal.i16 = (uint16_t) val;   break;
    case 4: iVal.i32 = (uint32_t) val;   break;
    case 8: iVal.i64 = (uint64_t) val;   break;
    default:
        return E_FAIL;
    }

    memcpy( buffer, &iVal, size );

    return S_OK;
}

HRESULT WriteFloat( uint8_t* buffer, uint32_t bufSize, MagoEE::Type* type, const Real10& val )
{
    MagoEE::ENUMTY  ty = MagoEE::Tnone;

    if ( type->IsComplex() )
    {
        switch ( type->GetSize() )
        {
        case 8:     ty = MagoEE::Tfloat32;  break;
        case 16:    ty = MagoEE::Tfloat64;  break;
        case 20:    ty = MagoEE::Tfloat80;  break;
        }
    }
    else    // is real or imaginary
    {
        switch ( type->GetSize() )
        {
        case 4:     ty = MagoEE::Tfloat32;  break;
        case 8:     ty = MagoEE::Tfloat64;  break;
        case 10:    ty = MagoEE::Tfloat80;  break;
        }
    }

    union Float
    {
        float       f32;
        double      f64;
        Real10      f80;
    };

    uint32_t    size = 0;
    Float       fVal = { 0 };

    switch ( ty )
    {
    case MagoEE::Tfloat32: fVal.f32 = val.ToFloat();    size = 4;   break;
    case MagoEE::Tfloat64: fVal.f64 = val.ToDouble();   size = 8;   break;
    case MagoEE::Tfloat80: fVal.f80 = val;   size = 10; break;
    default:
        return E_FAIL;
    }

    _ASSERT ( size <= bufSize );
    if ( size > bufSize )
        return E_FAIL;

    memcpy( buffer, &fVal, size );

    return S_OK;
}

HRESULT FromRawValue( const void* srcBuf, MagoEE::Type* type, MagoEE::DataValue& value )
{
    _ASSERT( srcBuf != NULL );
    _ASSERT( type != NULL );

    size_t size = type->GetSize();

    if ( type->IsPointer() )
    {
        value.Addr = ReadInt( srcBuf, 0, size, false );
    }
    else if ( type->IsIntegral() )
    {
        value.UInt64Value = ReadInt( srcBuf, 0, size, type->IsSigned() );
    }
    else if ( type->IsReal() || type->IsImaginary() )
    {
        value.Float80Value = ReadFloat( srcBuf, 0, type );
    }
    else if ( type->IsComplex() )
    {
        const size_t    PartSize = type->GetSize() / 2;

        value.Complex80Value.RealPart = ReadFloat( srcBuf, 0, type );
        value.Complex80Value.ImaginaryPart = ReadFloat( srcBuf, PartSize, type );
    }
    else if ( type->IsDArray() )
    {
        const size_t LengthSize = type->AsTypeDArray()->GetLengthType()->GetSize();
        const size_t AddressSize = type->AsTypeDArray()->GetPointerType()->GetSize();

        value.Array.Length = (MagoEE::dlength_t) ReadInt( srcBuf, 0, LengthSize, false );
        value.Array.Addr = ReadInt( srcBuf, LengthSize, AddressSize, false );
    }
    else if ( type->IsAArray() )
    {
        value.Addr = ReadInt( srcBuf, 0, size, false );
    }
    else if ( type->IsDelegate() )
    {
        MagoEE::TypeDelegate* delType = (MagoEE::TypeDelegate*) type;
        const size_t AddressSize = delType->GetNext()->GetSize();

        value.Delegate.ContextAddr = (MagoEE::dlength_t) ReadInt( srcBuf, 0, AddressSize, false );
        value.Delegate.FuncAddr = ReadInt( srcBuf, AddressSize, AddressSize, false );
    }
    else
        return E_FAIL;

    return S_OK;
}

} // namespace MagoEE
