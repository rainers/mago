/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DataEnv.h"
#include "DataValue.h"
#include "DataElement.h"

using MagoEE::Type;
using MagoEE::Declaration;


DataEnv::DataEnv( size_t size )
:   mBufSize( size ),
    mAllocSize( 4 )
{
    mBuf.Attach( new uint8_t[size] );
}

DataEnv::DataEnv( const DataEnv& orig )
{
    mBuf.Attach( new uint8_t[ orig.mAllocSize ] );
    mBufSize = orig.mAllocSize;
    mAllocSize = orig.mAllocSize;

    memcpy( mBuf.Get(), orig.mBuf.Get(), mBufSize );
}

uint8_t*    DataEnv::GetBuffer()
{
    return mBuf.Get();
}

uint8_t*    DataEnv::GetBufferLimit()
{
    return mBuf.Get() + mAllocSize;
}

MagoEE::Address DataEnv::Allocate( uint32_t size )
{
    MagoEE::Address addr = mAllocSize;
    mAllocSize += size;

    if ( mAllocSize > mBufSize )
        throw L"Allocated memory out-of-bounds.";

    return addr;
}

std::shared_ptr<DataObj> DataEnv::GetValue( MagoEE::Declaration* decl )
{
    if ( decl->IsConstant() )
    {
        return ((ConstantDataElement*) decl)->Evaluate();
    }
    else if ( decl->IsVar() )
    {
        MagoEE::Address addr = 0;
        RefPtr<Type>    type;

        decl->GetAddress( addr );
        decl->GetType( type.Ref() );

        return GetValue( addr, type.Get(), decl );
    }

    throw L"Can't get value.";
}

std::shared_ptr<DataObj> DataEnv::GetValue( MagoEE::Address address, MagoEE::Type* type )
{
    return GetValue( address, type, NULL );
}

std::shared_ptr<DataObj> DataEnv::GetValue( MagoEE::Address address, MagoEE::Type* type, MagoEE::Declaration* decl )
{
    _ASSERT( type != NULL );

    std::shared_ptr<LValueObj>   val( new LValueObj( decl, address ) );
    size_t  size = 0;

    if ( type->IsPointer() 
        || type->IsIntegral()
        || type->IsFloatingPoint()
        || type->IsDArray() )
        size = type->GetSize();

    if ( (address + size) > mAllocSize )
        throw L"Reading memory out-of-bounds.";

    val->SetType( type );

    if ( type->IsPointer() )
    {
        memcpy( &val->Value.Addr, mBuf.Get() + address, size );
    }
    else if ( type->IsIntegral() )
    {
        val->Value.UInt64Value = ReadInt( address, size, type->IsSigned() );
    }
    else if ( type->IsReal() || type->IsImaginary() )
    {
        val->Value.Float80Value = ReadFloat( address, type );
    }
    else if ( type->IsComplex() )
    {
        val->Value.Complex80Value.RealPart = ReadFloat( address, type );
        val->Value.Complex80Value.ImaginaryPart = ReadFloat( address + (size / 2), type );
    }
    else if ( type->IsDArray() )
    {
        const size_t LengthSize = type->AsTypeDArray()->GetLengthType()->GetSize();
        const size_t AddressSize = type->AsTypeDArray()->GetPointerType()->GetSize();

        val->Value.Array.Length = (MagoEE::dlength_t) ReadInt( address, LengthSize, false );
        memcpy( &val->Value.Array.Addr, mBuf.Get() + address + LengthSize, AddressSize );
    }

    return val;
}

void DataEnv::SetValue( MagoEE::Address address, DataObj* val )
{
    _ASSERT( val->GetType() != NULL );

    Type*       type = val->GetType();
    uint8_t*    buf = mBuf.Get() + address;
    uint8_t*    bufLimit = mBuf.Get() + mBufSize;

    if ( type->IsPointer() )
    {
        IntValueElement::WriteData( buf, bufLimit, type, val->Value.Addr );
    }
    else if ( type->IsIntegral() )
    {
        IntValueElement::WriteData( buf, bufLimit, type, val->Value.UInt64Value );
    }
    else if ( type->IsReal() || type->IsImaginary() )
    {
        RealValueElement::WriteData( buf, bufLimit, type, val->Value.Float80Value );
    }
    else if ( type->IsComplex() )
    {
        const size_t PartSize = type->GetSize() / 2;
        RealValueElement::WriteData( buf, bufLimit, type, val->Value.Complex80Value.RealPart );
        RealValueElement::WriteData( buf + PartSize, bufLimit, type, val->Value.Complex80Value.ImaginaryPart );
    }
    else if ( type->IsDArray() )
    {
        RefPtr<Type> lenType = type->AsTypeDArray()->GetLengthType();
        RefPtr<Type> ptrType = type->AsTypeDArray()->GetPointerType();

        IntValueElement::WriteData( buf, bufLimit, lenType, val->Value.Array.Length );
        IntValueElement::WriteData( buf + lenType->GetSize(), bufLimit, ptrType, val->Value.Array.Addr );
    }
}

RefPtr<MagoEE::Declaration> DataEnv::GetThis()
{
    _ASSERT( false );
    return NULL;
}

RefPtr<MagoEE::Declaration> DataEnv::GetSuper()
{
    _ASSERT( false );
    return NULL;
}

bool DataEnv::GetArrayLength( MagoEE::dlength_t& length )
{
    return false;
}

uint64_t DataEnv::ReadInt( MagoEE::Address address, size_t size, bool isSigned )
{
    return ReadInt( mBuf.Get(), address, size, isSigned );
}

uint64_t DataEnv::ReadInt( uint8_t* srcBuf, MagoEE::Address address, size_t size, bool isSigned )
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

    memcpy( &i, srcBuf + address, size );

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

Real10 DataEnv::ReadFloat( MagoEE::Address address, MagoEE::Type* type )
{
    return ReadFloat( mBuf.Get(), address, type );
}

Real10 DataEnv::ReadFloat( uint8_t* srcBuf, MagoEE::Address address, MagoEE::Type* type )
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

    if ( type->IsComplex() )
    {
        switch ( type->GetSize() )
        {
        case 8:     ty = MagoEE::Tfloat32;  break;
        case 16:    ty = MagoEE::Tfloat64;  break;
        case 20:    ty = MagoEE::Tfloat80;  break;
        }
    }
    else if ( type->IsImaginary() )
    {
        switch ( type->GetSize() )
        {
        case 4:     ty = MagoEE::Tfloat32;  break;
        case 8:     ty = MagoEE::Tfloat64;  break;
        case 10:    ty = MagoEE::Tfloat80;  break;
        }
    }
    else    // is real
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
    case MagoEE::Tfloat64:  size = 8;  break;
    case MagoEE::Tfloat80:  size = 10;  break;
    }

    memcpy( &f, srcBuf + address, size );

    switch ( ty )
    {
    case MagoEE::Tfloat32:  r.FromFloat( f.f32 );   break;
    case MagoEE::Tfloat64:  r.FromDouble( f.f64 );  break;
    case MagoEE::Tfloat80:  r = f.f80;  break;
    }

    return r;
}


//----------------------------------------------------------------------------

DataEnvBinder::DataEnvBinder( IValueEnv* env, IScope* scope )
    :   mDataEnv( env ),
        mScope( scope )
{
}

HRESULT DataEnvBinder::FindObject( const wchar_t* name, MagoEE::Declaration*& decl )
{
    return mScope->FindObject( name, decl );
}

HRESULT DataEnvBinder::GetThis( MagoEE::Declaration*& decl )
{
    return E_NOTIMPL;
}

HRESULT DataEnvBinder::GetSuper( MagoEE::Declaration*& decl )
{
    return E_NOTIMPL;
}

HRESULT DataEnvBinder::GetReturnType( MagoEE::Type*& type )
{
    return E_NOTIMPL;
}


HRESULT DataEnvBinder::GetValue( MagoEE::Declaration* decl, MagoEE::DataValue& value )
{
    std::shared_ptr<DataObj> val = mDataEnv->GetValue( decl );

    if ( val == NULL )
        throw L"No value.";

    value = val->Value;
    return S_OK;
}

HRESULT DataEnvBinder::GetValue( MagoEE::Address addr, MagoEE::Type* type, MagoEE::DataValue& value )
{
    std::shared_ptr<DataObj> val = mDataEnv->GetValue( addr, type );

    if ( val == NULL )
        throw L"No value.";

    value = val->Value;
    return S_OK;
}

HRESULT DataEnvBinder::GetValue( MagoEE::Address aArrayAddr, const MagoEE::DataObject& key, MagoEE::Address& valueAddr )
{
    return E_NOTIMPL;
}

int DataEnvBinder::GetAAVersion ()
{
    return -1;
}

HRESULT DataEnvBinder::GetClassName( MagoEE::Address addr, std::wstring& className )
{
    return E_NOTIMPL;
}

HRESULT DataEnvBinder::SetValue( MagoEE::Declaration* decl, const MagoEE::DataValue& value )
{
    MagoEE::Address addr = 0;
    RefPtr<Type>    type;
    RValueObj       obj;

    if ( !decl->GetAddress( addr ) )
        throw L"Couldn't get address.";
    if ( !decl->GetType( type.Ref() ) )
        throw L"Couldn't get type.";

    obj.SetType( type );
    obj.Value = value;

    mDataEnv->SetValue( addr, &obj );
    return S_OK;
}

HRESULT DataEnvBinder::SetValue( MagoEE::Address addr, MagoEE::Type* type, const MagoEE::DataValue& value )
{
    RValueObj       obj;

    obj.SetType( type );
    obj.Value = value;

    mDataEnv->SetValue( addr, &obj );
    return S_OK;
}

HRESULT DataEnvBinder::ReadMemory( MagoEE::Address addr, uint32_t sizeToRead, uint32_t& sizeRead, uint8_t* buffer )
{
    return E_NOTIMPL;
}

HRESULT DataEnvBinder::SymbolFromAddr( MagoEE::Address addr, std::wstring& symName )
{
    return E_NOTIMPL;
}

HRESULT DataEnvBinder::CallFunction( MagoEE::Address addr, MagoEE::ITypeFunction* func, MagoEE::Address arg, MagoEE::DataObject& value )
{
    return E_NOTIMPL;
}
