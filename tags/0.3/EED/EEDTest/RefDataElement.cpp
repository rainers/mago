/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DataElement.h"


bool FindDeclaration( const wchar_t* namePath, MagoEE::ITypeEnv* typeEnv, IScope* topScope, MagoEE::Declaration*& outDecl );


//----------------------------------------------------------------------------
// RefDataElement
//----------------------------------------------------------------------------

RefPtr<RefDataElement>   RefDataElement::MakeRefElement( const wchar_t* value )
{
    RefPtr<DeclRefDataElement>   declRef( new DeclRefDataElement() );
    declRef->SetAttribute( L"path", value );
    return declRef.Get();
}


//----------------------------------------------------------------------------
// DeclRefDataElement
//----------------------------------------------------------------------------

void DeclRefDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
    if ( !FindDeclaration( mPath.c_str(), typeEnv, scope, mDecl.Ref() ) )
        throw L"Declaration not found.";

    if ( !mDecl->IsVar() )
        throw L"Declaration must refer to a variable.";
}

bool DeclRefDataElement::GetAddress( MagoEE::Address& addr )
{
    if ( mDecl.Get() == NULL )
        throw L"Declaration not found.";

    return mDecl->GetAddress( addr );
}

void DeclRefDataElement::AddChild( Element* elem )
{
    throw L"DeclRef can't have children.";
}

void DeclRefDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"path" ) == 0 )
    {
        mPath = value;
    }
}

void DeclRefDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void DeclRefDataElement::PrintElement()
{
    printf( "DeclRef %ls\n", mPath.c_str() );
}
