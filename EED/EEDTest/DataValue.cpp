/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DataValue.h"

using namespace std;


//----------------------------------------------------------------------------
//  DataObj
//----------------------------------------------------------------------------

DataObj::DataObj()
{
    memset( &Value, 0, sizeof Value );
}

DataObj::~DataObj()
{
}

MagoEE::Type* DataObj::GetType() const
{
    return mType.Get();
}

bool DataObj::GetAddress( MagoEE::Address& addr )
{
    return false;
}

bool DataObj::GetDeclaration( MagoEE::Declaration*& decl )
{
    return false;
}

void DataObj::SetType( MagoEE::Type* type )
{
    mType = type;
}

void DataObj::SetType( const RefPtr<MagoEE::Type>& type )
{
    mType = type;
}

void DataObj::AppendValue( std::wstring& str )
{
    wchar_t buf[200] = L"";

    if ( mType->IsIntegral() )
    {
        swprintf_s( buf, L"%I64d (integral)", Value.UInt64Value );
        str.append( buf );
    }
    else if ( mType->IsReal() )
    {
        Value.Float80Value.ToString( buf, _countof( buf ), 20 );
        if ( Value.Float80Value.IsNan() && Value.Float80Value.GetSign() < 0 )
            str.append( L"-" );
        str.append( buf );
        str.append( L" (real)" );

#if 0
        str.append( L"[" );
        for ( int i = 0; i < 5; i++ )
        {
            wchar_t buf[5] = L"";
            swprintf_s( buf, L"%04x", Value.Float80Value.Words[4 - i] );
            str.append( buf );
        }
        str.append( L"]" );
#endif
    }
    else if ( mType->IsImaginary() )
    {
        Value.Float80Value.ToString( buf, _countof( buf ), 20 );
        if ( Value.Float80Value.IsNan() && Value.Float80Value.GetSign() < 0 )
            str.append( L"-" );
        str.append( buf );
        str.append( L"i" );
    }
    else if ( mType->IsComplex() )
    {
        str.append( L"(" );

        Value.Complex80Value.RealPart.ToString( buf, _countof( buf ), 20 );
        if ( Value.Complex80Value.RealPart.IsNan() && Value.Complex80Value.RealPart.GetSign() < 0 )
            str.append( L"-" );
        str.append( buf );
        str.append( L" + " );

        Value.Complex80Value.ImaginaryPart.ToString( buf, _countof( buf ), 20 );
        if ( Value.Complex80Value.ImaginaryPart.IsNan() && Value.Complex80Value.ImaginaryPart.GetSign() < 0 )
            str.append( L"-" );
        str.append( buf );
        str.append( L"i" );

        str.append( L")" );
    }
    else if ( mType->IsPointer() )
    {
        swprintf_s( buf, L"%08I64x (pointer)", (uint64_t) Value.Addr );
        str.append( buf );
    }
    else if ( mType->IsDArray() )
    {
        swprintf_s( buf, L"(addr=%08I64x, len=%d)", (uint64_t) Value.Array.Addr, Value.Array.Length );
        str.append( buf );
    }
    // TODO: add delegate and A-array
    else
    {
        str.append( L"value with unsupported type" );
    }

    wstring typeStr;

    mType->ToString( typeStr );

    str.append( L" : " );
    str.append( typeStr );
}


//----------------------------------------------------------------------------
//  LValueObj
//----------------------------------------------------------------------------

LValueObj::LValueObj( MagoEE::Declaration* decl, MagoEE::Address addr )
    :   mDecl( decl ),
        mAddr( addr )
{
}

DataObj::Kind LValueObj::GetKind() const
{
    return LValue;
}

bool LValueObj::GetAddress( MagoEE::Address& addr )
{
    if ( mAddr != 0 )
    {
        addr = mAddr;
        return true;
    }

    if ( (mDecl.Get() != NULL) && mDecl->GetAddress( addr, nullptr ) )
        return true;

    return false;
}

bool LValueObj::GetDeclaration( MagoEE::Declaration*& decl )
{
    if ( mDecl.Get() == NULL )
        return false;

    decl = mDecl.Get();
    decl->AddRef();
    return true;
}

std::wstring LValueObj::ToString()
{
    wchar_t buf[200] = L"";
    wstring str = L"L-value: ";

    AppendValue( str );
    swprintf_s( buf, L"; Addr = %08x", (uint32_t) mAddr );
    str.append( buf );

    if ( mDecl != NULL )
    {
        swprintf_s( buf, L"; Decl = '%ls'", mDecl->GetName() );
        str.append( buf );
    }

    return str;
}


//----------------------------------------------------------------------------
//  RValueObj
//----------------------------------------------------------------------------

DataObj::Kind RValueObj::GetKind() const
{
    return RValue;
}

std::wstring RValueObj::ToString()
{
    wstring str = L"R-value: ";
    AppendValue( str );
    return str;
}
