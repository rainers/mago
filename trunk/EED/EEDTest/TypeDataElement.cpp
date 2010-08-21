/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DataElement.h"

using MagoEE::ITypeEnv;


bool FindDeclaration( const wchar_t* namePath, ITypeEnv* typeEnv, IScope* topScope, MagoEE::Declaration*& outDecl );


//----------------------------------------------------------------------------
// TypeDataElement
//----------------------------------------------------------------------------

RefPtr<TypeDataElement>   TypeDataElement::MakeTypeElement( const wchar_t* value )
{
    RefPtr<TypeRefDataElement>   typeRef( new TypeRefDataElement() );
    typeRef->SetAttribute( L"name", value );
    return typeRef.Get();
}


//----------------------------------------------------------------------------
// TypeRefDataElement
//----------------------------------------------------------------------------

void TypeRefDataElement::BindTypes( ITypeEnv* typeEnv, IScope* topScope )
{
    HRESULT hr = S_OK;
    RefPtr<MagoEE::Declaration> decl;

    FindDeclaration( mRefName.c_str(), typeEnv, topScope, decl.Ref() );

    if ( (decl.Get() == NULL) || !decl->IsType() )
        throw L"Type not found.";

    bool    ok = decl->GetType( mType.Ref() );
    _ASSERT( ok );
}

void TypeRefDataElement::AddChild( Element* elem )
{
    throw L"TypeRef can't have children.";
}

void TypeRefDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mRefName = value;
    }
}

void TypeRefDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void TypeRefDataElement::PrintElement()
{
    printf( "TypeRef to %ls\n", mRefName.c_str() );
}


//----------------------------------------------------------------------------
// TypePointerDataElement
//----------------------------------------------------------------------------

void TypePointerDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    HRESULT hr = S_OK;

    mPointed->BindTypes( typeEnv, scope );

    hr = typeEnv->NewPointer( mPointed->GetType(), mType.Ref() );
    _ASSERT( hr == S_OK );
}

void TypePointerDataElement::AddChild( Element* elem )
{
    throw L"TypePointer can't have children.";
}

void TypePointerDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"pointedType" ) == 0 )
    {
        mPointed = TypeDataElement::MakeTypeElement( value );
    }
}

void TypePointerDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"pointedType" ) == 0 )
    {
        mPointed = dynamic_cast<TypeDataElement*>( elemValue );
    }
}

void TypePointerDataElement::PrintElement()
{
    printf( "Type Pointer\n" );
    mPointed->PrintElement();
}


//----------------------------------------------------------------------------
// TypeDArrayDataElement
//----------------------------------------------------------------------------

void TypeDArrayDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    mElemType->BindTypes( typeEnv, scope );

    HRESULT hr = S_OK;

    hr = typeEnv->NewDArray( mElemType->GetType(), mType.Ref() );
    _ASSERT( hr == S_OK );
}

void TypeDArrayDataElement::AddChild( Element* elem )
{
    throw L"TypeDArray can't have children.";
}

void TypeDArrayDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"elementtype" ) == 0 )
    {
        mElemType = TypeDataElement::MakeTypeElement( value );
    }
}

void TypeDArrayDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"elementtype" ) == 0 )
    {
        mElemType = dynamic_cast<TypeDataElement*>( elemValue );
    }
}

void TypeDArrayDataElement::PrintElement()
{
    printf( "Type D Array\n" );
    mElemType->PrintElement();
}


//----------------------------------------------------------------------------
// TypeSArrayDataElement
//----------------------------------------------------------------------------

TypeSArrayDataElement::TypeSArrayDataElement()
:   mLen( 0 )
{
}

void TypeSArrayDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    mElemType->BindTypes( typeEnv, scope );

    HRESULT hr = S_OK;

    hr = typeEnv->NewSArray( mElemType->GetType(), mLen, mType.Ref() );
    _ASSERT( hr == S_OK );
}

void TypeSArrayDataElement::AddChild( Element* elem )
{
    throw L"TypeSArray can't have children.";
}

void TypeSArrayDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"elementtype" ) == 0 )
    {
        mElemType = TypeDataElement::MakeTypeElement( value );
    }
    else if ( _wcsicmp( name, L"length" ) == 0 )
    {
        mLen = wcstoul( value, NULL, 10 );
    }
}

void TypeSArrayDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"elementtype" ) == 0 )
    {
        mElemType = dynamic_cast<TypeDataElement*>( elemValue );
    }
}

void TypeSArrayDataElement::PrintElement()
{
    printf( "Type S Array\n" );
    mElemType->PrintElement();
}


//----------------------------------------------------------------------------
// TypeAArrayDataElement
//----------------------------------------------------------------------------

void TypeAArrayDataElement::BindTypes( ITypeEnv* typeEnv, IScope* scope )
{
    mElemType->BindTypes( typeEnv, scope );
    mKeyType->BindTypes( typeEnv, scope );

    HRESULT hr = S_OK;

    hr = typeEnv->NewAArray( mElemType->GetType(), mKeyType->GetType(), mType.Ref() );
    _ASSERT( hr == S_OK );
}

void TypeAArrayDataElement::AddChild( Element* elem )
{
    throw L"TypeAArray can't have children.";
}

void TypeAArrayDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"elementtype" ) == 0 )
    {
        mElemType = TypeDataElement::MakeTypeElement( value );
    }
    else if ( _wcsicmp( name, L"keytype" ) == 0 )
    {
        mKeyType = TypeDataElement::MakeTypeElement( value );
    }
}

void TypeAArrayDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"elementtype" ) == 0 )
    {
        mElemType = dynamic_cast<TypeDataElement*>( elemValue );
    }
    else if ( _wcsicmp( name, L"keytype" ) == 0 )
    {
        mKeyType = dynamic_cast<TypeDataElement*>( elemValue );
    }
}

void TypeAArrayDataElement::PrintElement()
{
    printf( "Type A Array\n" );
    mElemType->PrintElement();
    mKeyType->PrintElement();
}
