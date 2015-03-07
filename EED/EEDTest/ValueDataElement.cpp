/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DataElement.h"
#include "DataValue.h"

using MagoEE::ITypeEnv;


//bool FindDeclaration( const wchar_t* namePath, ITypeEnv* typeEnv, IScope* topScope, MagoEE::Declaration*& outDecl );


//----------------------------------------------------------------------------
// ValueDataElement
//----------------------------------------------------------------------------

std::shared_ptr<DataObj> ValueDataElement::Evaluate( MagoEE::Type* type )
{
    throw L"Can't evaluate to a single value.";
}

RefPtr<ValueDataElement>   ValueDataElement::MakeValueElement( const wchar_t* value )
{
    if ( (iswdigit( value[0] ) || (value[0] == L'-')) && (wcschr( value, L'.' ) != NULL) )
    {
        RefPtr<ValueDataElement> elem( new RealValueElement() );
        elem->SetAttribute( L"value", value );
        return elem;
    }
    else if ( iswdigit( value[0] ) || (value[0] == L'\'') || (value[0] == L'-') )
    {
        RefPtr<ValueDataElement> elem( new IntValueElement() );
        elem->SetAttribute( L"value", value );
        return elem;
    }

    return RefPtr<ValueDataElement>();
}


//----------------------------------------------------------------------------
// NullValueElement
//----------------------------------------------------------------------------

void NullValueElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    // this will work for pointers, D-arrays, A-arrays, and delegates
    memset( buffer, 0, type->GetSize() );
}

std::shared_ptr<DataObj> NullValueElement::Evaluate( MagoEE::Type* type )
{
    std::shared_ptr<DataObj>  val( new RValueObj() );

    // rely on the owner setting the right type
    if ( type->IsPointer() )
    {
        val->Value.Addr = 0;
    }
    else if ( type->IsDArray() )
    {
        val->Value.Array.Addr = 0;
        val->Value.Array.Length = 0;
    }
    // TODO: other types

    return val;
}

void NullValueElement::AddChild( Element* elem )
{
    throw L"Null can't have children.";
}

void NullValueElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
}

void NullValueElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void NullValueElement::PrintElement()
{
    printf( "null\n" );
}


//----------------------------------------------------------------------------
// IntValueElement
//----------------------------------------------------------------------------

IntValueElement::IntValueElement()
:   Value( 0 )
{
}

void IntValueElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    WriteData( buffer, bufLimit, type, Value );
}

void IntValueElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type, uint64_t val )
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

    switch ( size )
    {
    case 1: iVal.i8 = (uint8_t) val;    break;
    case 2: iVal.i16 = (uint16_t) val;   break;
    case 4: iVal.i32 = (uint32_t) val;   break;
    case 8: iVal.i64 = (uint64_t) val;   break;
    default:
        throw L"Bad integral type size";
    }

    memcpy( buffer, &iVal, size );
}

std::shared_ptr<DataObj> IntValueElement::Evaluate( MagoEE::Type* type )
{
    std::shared_ptr<DataObj>  val( new RValueObj() );

    // rely on the owner setting the right type
    val->Value.UInt64Value = Value;

    return val;
}

void IntValueElement::AddChild( Element* elem )
{
    throw L"Int can't have children.";
}

void IntValueElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"value" ) == 0 )
    {
        if ( value[0] == L'\'' )
            Value = value[1];
        else
        {
            int64_t sign = 1;

            if ( value[0] == L'-' )
            {
                sign = -1;
                value++;
            }

            if ( (value[0] == L'0') && (towlower( value[1] ) == L'x') )
                Value = _wcstoui64( value + 2, NULL, 16 );
            else
                Value = _wcstoui64( value, NULL, 10 );

            Value *= sign;
        }
    }
}

void IntValueElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void IntValueElement::PrintElement()
{
    printf( "integer( %I64d )\n", Value );
}


//----------------------------------------------------------------------------
// RealValueElement
//----------------------------------------------------------------------------

RealValueElement::RealValueElement()
{
    Value.Zero();
}

void RealValueElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    WriteData( buffer, bufLimit, type, Value );
}

void RealValueElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type, const Real10& val )
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
        throw L"Bad float type size";
    }

    memcpy( buffer, &fVal, size );
}

std::shared_ptr<DataObj> RealValueElement::Evaluate( MagoEE::Type* type )
{
    std::shared_ptr<DataObj>  val( new RValueObj() );

    // rely on the owner setting the right type
    val->Value.Float80Value = Value;

    return val;
}

void RealValueElement::AddChild( Element* elem )
{
    throw L"Real can't have children.";
}

void RealValueElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"value" ) == 0 )
    {
        errno_t err = Real10::Parse( value, Value );
        if ( err != 0 )
            throw L"Bad floating point value.";
    }
}

void RealValueElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void RealValueElement::PrintElement()
{
    wchar_t str[ Real10::Float80DecStrLen + 1 ] = L"";

    Value.ToString( str, _countof( str ) );
    printf( "real( %ls )\n", str );
}


//----------------------------------------------------------------------------
// ComplexValueElement
//----------------------------------------------------------------------------

ComplexValueElement::ComplexValueElement()
{
    Value.Zero();
}

void ComplexValueElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    // this will turn complex types into their parts
    RealValueElement::WriteData( buffer, bufLimit, type, Value.RealPart );
    RealValueElement::WriteData( buffer, bufLimit, type, Value.ImaginaryPart );
}

std::shared_ptr<DataObj> ComplexValueElement::Evaluate( MagoEE::Type* type )
{
    std::shared_ptr<DataObj>  val( new RValueObj() );

    // rely on the owner setting the right type
    val->Value.Complex80Value = Value;

    return val;
}

void ComplexValueElement::AddChild( Element* elem )
{
    throw L"Int can't have children.";
}

void ComplexValueElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"real" ) == 0 )
    {
        errno_t err = Real10::Parse( value, Value.RealPart );
        if ( err != 0 )
            throw L"Bad floating point value.";
    }
    else if ( _wcsicmp( name, L"imaginary" ) == 0 )
    {
        errno_t err = Real10::Parse( value, Value.ImaginaryPart );
        if ( err != 0 )
            throw L"Bad floating point value.";
    }
}

void ComplexValueElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void ComplexValueElement::PrintElement()
{
    wchar_t str[ Real10::Float80DecStrLen + 1 ] = L"";

    Value.RealPart.ToString( str, _countof( str ) );
    printf( "complex( %ls + ", str );

    Value.ImaginaryPart.ToString( str, _countof( str ) );
    printf( "%lsi )\n", str );
}


//----------------------------------------------------------------------------
// StructValueDataElement
//----------------------------------------------------------------------------

void StructValueDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    for ( FieldList::iterator it = mFields.begin();
        it != mFields.end();
        it++ )
    {
        (*it)->BindTypes( typeEnv, scope );
    }
}

void StructValueDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    for ( FieldList::iterator it = mFields.begin();
        it != mFields.end();
        it++ )
    {
        (*it)->WriteData( buffer, bufLimit, type );
    }
}

void StructValueDataElement::AddChild( Element* elem )
{
    FieldValueDataElement*  fieldVal = dynamic_cast<FieldValueDataElement*>( elem );

    if ( fieldVal == NULL )
        throw L"StructValue can only have FieldValues.";

    mFields.push_back( RefPtr<FieldValueDataElement>( fieldVal ) );
}

void StructValueDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
}

void StructValueDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void StructValueDataElement::PrintElement()
{
    printf( "StructData\n" );
    for ( FieldList::iterator it = mFields.begin();
        it != mFields.end();
        it++ )
    {
        (*it)->PrintElement();
    }
}


//----------------------------------------------------------------------------
// FieldValueDataElement
//----------------------------------------------------------------------------

void FieldValueDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    mValue->BindTypes( typeEnv, scope );
}

void FieldValueDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    if ( mValue.Get() == NULL )
        return;

    RefPtr<MagoEE::Declaration> decl = type->AsTypeStruct()->FindObject( mName.c_str() );

    if ( (decl == NULL) || !decl->IsField() )
        throw L"Field not found.";

    FieldDataElement*  field = dynamic_cast<FieldDataElement*>( decl.Get() );
    RefPtr<MagoEE::Type>        fieldType;
    int offset = 0;

    _ASSERT( field != NULL );
    field->GetOffset( offset );
    field->GetType( fieldType.Ref() );

    uint8_t*    realAddr = buffer + offset;

    mValue->WriteData( realAddr, bufLimit, fieldType.Get() );
}

void FieldValueDataElement::AddChild( Element* elem )
{
    throw L"FieldData can't have children.";
}

void FieldValueDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
    else if ( _wcsicmp( name, L"value" ) == 0 )
    {
        mValue = ValueDataElement::MakeValueElement( value );
    }
}

void FieldValueDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"value" ) == 0 )
    {
        mValue = dynamic_cast<ValueDataElement*>( elemValue );
    }
}

void FieldValueDataElement::PrintElement()
{
    printf( "FieldData %ls\n", mName.c_str() );
    printf( " = \n" );
    if ( mValue.Get() == NULL )
        printf( " uninitialized\n" );
    else
        mValue->PrintElement();
}


//----------------------------------------------------------------------------
// AddressOfValueDataElement
//----------------------------------------------------------------------------

void AddressOfValueDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    mRef->BindTypes( typeEnv, scope );
}

void AddressOfValueDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    MagoEE::Address addr = 0;

    if ( !mRef->GetAddress( addr ) )
        throw L"Can't get address.";

    WriteData( buffer, bufLimit, type, addr );
}

void AddressOfValueDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type, MagoEE::Address addr )
{
    union AddressVal
    {
        uint32_t    i32;
        uint64_t    i64;
    };

    uint32_t    size = type->GetSize();
    AddressVal  aVal = { 0 };

    switch ( size )
    {
    case 4: aVal.i32 = (uint32_t) addr; break;
    case 8: aVal.i64 = (uint64_t) addr; break;
    default:
        throw L"Bad address type size";
    }

    memcpy( buffer, &aVal, size );
}

std::shared_ptr<DataObj> AddressOfValueDataElement::Evaluate( MagoEE::Type* type )
{
    MagoEE::Address addr = 0;

    if ( !mRef->GetAddress( addr ) )
        throw L"Can't get address.";

    std::shared_ptr<DataObj> val( new RValueObj() );

    val->Value.Addr = addr;

    return val;
}

void AddressOfValueDataElement::AddChild( Element* elem )
{
    throw L"AddressOf can't have children.";
}

void AddressOfValueDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"ref" ) == 0 )
    {
        mRef = RefDataElement::MakeRefElement( value );
    }
}

void AddressOfValueDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"ref" ) == 0 )
    {
        mRef = dynamic_cast<RefDataElement*>( elemValue );
    }
}

void AddressOfValueDataElement::PrintElement()
{
    printf( "AddressOf\n" );
    mRef->PrintElement();
}


//----------------------------------------------------------------------------
// ArrayValueDataElement
//----------------------------------------------------------------------------

ArrayValueDataElement::ArrayValueDataElement()
:   mStartIndex( 0 )
{
}

void ArrayValueDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    for ( ElementList::iterator it = mElems.begin();
        it != mElems.end();
        it++ )
    {
        (*it)->BindTypes( typeEnv, scope );
    }
}

void ArrayValueDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    RefPtr<MagoEE::Type>    elemType = type->AsTypeNext()->GetNext();
    uint32_t        size = elemType->GetSize();
    int             i = mStartIndex;

    for ( ElementList::iterator it = mElems.begin();
        it != mElems.end();
        it++, i++ )
    {
        uint8_t*    realAddr = buffer + (i * size);

        (*it)->WriteData( realAddr, bufLimit, elemType.Get() );
    }
}

void ArrayValueDataElement::AddChild( Element* elem )
{
    ValueDataElement*   valElem = dynamic_cast<ValueDataElement*>( elem );

    if ( valElem == NULL )
        throw L"Can only add values to ArrayValue.";

    mElems.push_back( valElem );
}

void ArrayValueDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"index" ) == 0 )
    {
        mStartIndex = wcstoul( value, NULL, 10 );
    }
}

void ArrayValueDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void ArrayValueDataElement::PrintElement()
{
    printf( "ArrayValue startIndex=%d\n", mStartIndex );

    for ( ElementList::iterator it = mElems.begin();
        it != mElems.end();
        it++ )
    {
        (*it)->PrintElement();
    }
}


//----------------------------------------------------------------------------
// SliceValueDataElement
//----------------------------------------------------------------------------

SliceValueDataElement::SliceValueDataElement()
:   mStartIndex( 0 ),
    mEndIndex( 0 )
{
}

void SliceValueDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    mPtr->BindTypes( typeEnv, scope );

    mPtrType = typeEnv->GetVoidPointerType();
    mSizeType = typeEnv->GetType( MagoEE::Tuns32 );
}

void SliceValueDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type )
{
    std::shared_ptr<DataObj> val = Evaluate( type );
    uint32_t    sizetLen = mSizeType->GetSize();

    IntValueElement::WriteData( buffer, bufLimit, mSizeType, val->Value.Array.Length );
    AddressOfValueDataElement::WriteData( buffer + sizetLen, bufLimit, mPtrType, val->Value.Array.Addr );
}

std::shared_ptr<DataObj> SliceValueDataElement::Evaluate( MagoEE::Type* type )
{
    std::shared_ptr<DataObj> val( new RValueObj() );
    std::shared_ptr<DataObj> ptr = mPtr->Evaluate( mPtrType );
    uint32_t            len = mEndIndex - mStartIndex;
    MagoEE::Address     addr = 0;

    addr = ptr->Value.Addr + (mStartIndex * type->AsTypeNext()->GetNext()->GetSize());

    val->Value.Array.Addr = addr;
    val->Value.Array.Length = len;

    return val;
}

void SliceValueDataElement::AddChild( Element* elem )
{
    throw L"Slice can't have children.";
}

void SliceValueDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"start" ) == 0 )
    {
        mStartIndex = wcstol( value, NULL, 10 );
    }
    else if ( _wcsicmp( name, L"end" ) == 0 )
    {
        mEndIndex = wcstol( value, NULL, 10 );
    }
}

void SliceValueDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"ptr" ) == 0 )
    {
        mPtr = dynamic_cast<ValueDataElement*>( elemValue );
    }
}

void SliceValueDataElement::PrintElement()
{
    printf( "Slice\n" );
    mPtr->PrintElement();
    printf( "  Start index = %d\n", mStartIndex );
    printf( "  End index = %d\n", mEndIndex );
}
