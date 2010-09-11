/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DataElement.h"
#include "DataValue.h"


//----------------------------------------------------------------------------
// DeclDataElement
//----------------------------------------------------------------------------

void DeclDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
}

void DeclDataElement::LayoutFields( IDataEnv* dataEnv )
{
}

void DeclDataElement::LayoutVars( IDataEnv* dataEnv )
{
}

void DeclDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit )
{
}

const wchar_t* DeclDataElement::GetName()
{
    return mName.c_str();
}

bool DeclDataElement::GetType( MagoEE::Type*& type )
{
    return false;
}

bool DeclDataElement::GetAddress( MagoEE::Address& addr )
{
    return false;
}

bool DeclDataElement::GetOffset( int& offset )
{
    return false;
}

bool DeclDataElement::GetSize( uint32_t& size )
{
    return false;
}

bool DeclDataElement::GetBackingTy( MagoEE::ENUMTY& ty )
{
    return false;
}

bool DeclDataElement::GetUdtKind( MagoEE::UdtKind& kind )
{
    return false;
}

bool DeclDataElement::IsField()
{
    return false;
}

bool DeclDataElement::IsVar()
{
    return false;
}

bool DeclDataElement::IsConstant()
{
    return false;
}

bool DeclDataElement::IsType()
{
    return false;
}

bool DeclDataElement::IsBaseClass()
{
    return false;
}

HRESULT DeclDataElement::FindObject( const wchar_t* name, Declaration*& decl )
{
    return E_FAIL;
}

bool DeclDataElement::EnumMembers( MagoEE::IEnumDeclarationMembers*& members )
{
    return false;
}


//----------------------------------------------------------------------------
// VarDataElement
//----------------------------------------------------------------------------

VarDataElement::VarDataElement()
:   mAddr( 0 )
{
}

void VarDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
    mType->BindTypes( typeEnv, scope );
    // now we can call mType->AsType() and it'll work

    if ( mInitVal.Get() != NULL )
        mInitVal->BindTypes( typeEnv, scope );
}

void VarDataElement::LayoutVars( IDataEnv* dataEnv )
{
    if ( (mType.Get() == NULL) || (mType->GetType() == NULL) )
        throw L"Variable has no type.";

    mAddr = dataEnv->Allocate( mType->GetType()->GetSize() );
}

void VarDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit )
{
    if ( mInitVal.Get() == NULL )
        return;

    uint8_t*    realAddr = buffer + mAddr;
    uint32_t    size = mType->GetType()->GetSize();

    if ( (realAddr + size) > bufLimit )
        throw L"Data too big to fit in memory area.";

    mInitVal->WriteData( realAddr, bufLimit, mType->GetType() );
}

void VarDataElement::AddChild( Element* elem )
{
    throw L"Variable can't have children.";
}

void VarDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
    else if ( _wcsicmp( name, L"type" ) == 0 )
    {
        mType = TypeDataElement::MakeTypeElement( value );
    }
    else if ( _wcsicmp( name, L"value" ) == 0 )
    {
        mInitVal = ValueDataElement::MakeValueElement( value );
    }
}

void VarDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"type" ) == 0 )
    {
        mType = dynamic_cast<TypeDataElement*>( elemValue );
        if ( mType == NULL )
            throw L"Type expected.";
    }
    else if ( _wcsicmp( name, L"value" ) == 0 )
    {
        mInitVal = dynamic_cast<ValueDataElement*>( elemValue );
        if ( mType == NULL )
            throw L"Init value expected.";
    }
}

bool VarDataElement::GetType( MagoEE::Type*& type )
{
    type = mType->GetType();
    _ASSERT( type != NULL );
    type->AddRef();
    return true;
}

bool VarDataElement::GetAddress( MagoEE::Address& addr )
{
    addr = mAddr;
    return true;
}

bool VarDataElement::IsVar()
{
    return true;
}

void VarDataElement::PrintElement()
{
    printf( "Var %ls\n", mName.c_str() );
    mType->PrintElement();
    printf( "    address = %08I64x\n", mAddr );
    if ( (mType.Get() != NULL) && (mType->GetType() != NULL) )
        printf( "    size = %d\n", mType->GetType()->GetSize() );
    printf( " = \n" );
    if ( mInitVal.Get() == NULL )
        printf( " uninitialized\n" );
    else
        mInitVal->PrintElement();
}


//----------------------------------------------------------------------------
// ConstantDataElement
//----------------------------------------------------------------------------

boost::shared_ptr<DataObj> ConstantDataElement::Evaluate()
{
    boost::shared_ptr<DataObj>  val( new RValueObj() );
    boost::shared_ptr<DataObj>  childVal = mVal->Evaluate( mType->GetType() );

    val->SetType( mType->GetType() );
    val->Value = childVal->Value;

    return val;
}

void ConstantDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
    mType->BindTypes( typeEnv, scope );
    // now we can call mType->AsType() and it'll work
}

void ConstantDataElement::AddChild( Element* elem )
{
    throw L"Constant can't have children.";
}

void ConstantDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
    else if ( _wcsicmp( name, L"type" ) == 0 )
    {
        mType = TypeDataElement::MakeTypeElement( value );
    }
    else if ( _wcsicmp( name, L"value" ) == 0 )
    {
        mVal = ValueDataElement::MakeValueElement( value );
    }
}

void ConstantDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"type" ) == 0 )
    {
        mType = dynamic_cast<TypeDataElement*>( elemValue );
    }
    else if ( _wcsicmp( name, L"value" ) == 0 )
    {
        mVal = dynamic_cast<ValueDataElement*>( elemValue );
    }
}

bool ConstantDataElement::GetType( MagoEE::Type*& type )
{
    type = mType->GetType();
    _ASSERT( type != NULL );
    type->AddRef();
    return true;
}

bool ConstantDataElement::IsConstant()
{
    return true;
}

void ConstantDataElement::PrintElement()
{
    printf( "Constant %ls\n", mName.c_str() );
    mType->PrintElement();
    printf( " = \n" );
    if ( mVal.Get() == NULL )
        printf( " uninitialized\n" );
    else
        mVal->PrintElement();
}


//----------------------------------------------------------------------------
// NamespaceDataElement
//----------------------------------------------------------------------------

void NamespaceDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->BindTypes( typeEnv, scope );
    }
}

void NamespaceDataElement::LayoutFields( IDataEnv* dataEnv )
{
    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->LayoutFields( dataEnv );
    }
}

void NamespaceDataElement::LayoutVars( IDataEnv* dataEnv )
{
    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->LayoutVars( dataEnv );
    }
}

void NamespaceDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit )
{
    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->WriteData( buffer, bufLimit );
    }
}

void NamespaceDataElement::AddChild( Element* elem )
{
    const wchar_t*      name = elem->GetName();
    DeclDataElement*    declElem = dynamic_cast<DeclDataElement*>( elem );

    if ( declElem == NULL )
        throw L"Can only add declarations to Namespace.";

    mChildren.insert( ElementMap::value_type( name, RefPtr<DeclDataElement>( declElem ) ) );
}

void NamespaceDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
}

void NamespaceDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

HRESULT NamespaceDataElement::FindObject( const wchar_t* name, Declaration*& decl )
{
    ElementMap::iterator    it = mChildren.find( name );

    if ( it == mChildren.end() )
        return E_FAIL;

    decl = dynamic_cast<Declaration*>( it->second.Get() );
    decl->AddRef();
    return S_OK;
}

void NamespaceDataElement::PrintElement()
{
    printf( "Namespace %ls\n", mName.c_str() );

    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->PrintElement();
    }
}


//----------------------------------------------------------------------------
// StructDataElement
//----------------------------------------------------------------------------

StructDataElement::StructDataElement()
:   mSize( 0 )
{
}

MagoEE::Address StructDataElement::Allocate( uint32_t size )
{
    MagoEE::Address addr = mSize;
    mSize += size;
    return addr;
}

void StructDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->BindTypes( typeEnv, scope );
    }
}

void StructDataElement::LayoutVars( IDataEnv* dataEnv )
{
    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->LayoutVars( dataEnv );
    }
}

void StructDataElement::LayoutFields( IDataEnv* dataEnv )
{
    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->LayoutFields( this );
    }
}

void StructDataElement::WriteData( uint8_t* buffer, uint8_t* bufLimit )
{
    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->WriteData( buffer, bufLimit );
    }
}

bool StructDataElement::GetType( MagoEE::Type*& type )
{
    type = new MagoEE::TypeStruct( this );
    type->AddRef();

    return true;
}

bool StructDataElement::GetSize( uint32_t& size )
{
    size = mSize;
    return true;
}

bool StructDataElement::IsType()
{
    return true;
}

void StructDataElement::AddChild( Element* elem )
{
    const wchar_t*      name = elem->GetName();
    DeclDataElement*    declElem = dynamic_cast<DeclDataElement*>( elem );

    if ( declElem == NULL )
        throw L"Can only add declarations to Struct.";

    mChildren.insert( ElementMap::value_type( name, RefPtr<DeclDataElement>( declElem ) ) );
}

void StructDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
}

void StructDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

HRESULT StructDataElement::FindObject( const wchar_t* name, Declaration*& decl )
{
    ElementMap::iterator    it = mChildren.find( name );

    if ( it == mChildren.end() )
        return E_FAIL;

    decl = dynamic_cast<Declaration*>( it->second.Get() );
    decl->AddRef();
    return S_OK;
}

void StructDataElement::PrintElement()
{
    printf( "struct %ls\n", mName.c_str() );

    for ( ElementMap::iterator it = mChildren.begin();
        it != mChildren.end();
        it++ )
    {
        it->second->PrintElement();
    }
}


//----------------------------------------------------------------------------
// BasicTypeDefDataElement
//----------------------------------------------------------------------------

BasicTypeDefDataElement::BasicTypeDefDataElement( MagoEE::Type* type )
:   mType( type ),
    mRefCount( 0 )
{
}

void BasicTypeDefDataElement::AddRef()
{
    mRefCount++;
}

void BasicTypeDefDataElement::Release()
{
    mRefCount--;
    if ( mRefCount == 0 )
        delete this;
}

void BasicTypeDefDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
}

bool BasicTypeDefDataElement::GetType( MagoEE::Type*& type )
{
    type = mType.Get();
    type->AddRef();

    return true;
}

bool BasicTypeDefDataElement::GetSize( uint32_t& size )
{
    return true;
}

bool BasicTypeDefDataElement::IsType()
{
    return true;
}

void BasicTypeDefDataElement::AddChild( Element* elem )
{
    throw L"BasicTypeDef can't have children.";
}

void BasicTypeDefDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
}

void BasicTypeDefDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
}

void BasicTypeDefDataElement::PrintElement()
{
    printf( "BasicTypeDef %ls\n", mName.c_str() );
}


//----------------------------------------------------------------------------
// TypeDefDataElement
//----------------------------------------------------------------------------

void TypeDefDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
    mBaseType->BindTypes( typeEnv, scope );
    // now we can call mType->AsType() and it'll work
}

bool TypeDefDataElement::GetType( MagoEE::Type*& type )
{
    type = mBaseType->GetType();
    type->AddRef();

    return true;
}

bool TypeDefDataElement::GetSize( uint32_t& size )
{
    return false;
}

bool TypeDefDataElement::IsType()
{
    return true;
}

void TypeDefDataElement::AddChild( Element* elem )
{
    throw L"TypeDef can't have children.";
}

void TypeDefDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
    else if ( _wcsicmp( name, L"type" ) == 0 )
    {
        mBaseType = TypeDataElement::MakeTypeElement( value );
    }
}

void TypeDefDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"type" ) == 0 )
    {
        mBaseType = dynamic_cast<TypeDataElement*>( elemValue );
    }
}

void TypeDefDataElement::PrintElement()
{
    printf( "TypeDef %ls\n", mName.c_str() );
    mBaseType->PrintElement();
}


//----------------------------------------------------------------------------
// FieldDataElement
//----------------------------------------------------------------------------

FieldDataElement::FieldDataElement()
:   mOffset( 0 )
{
}

void FieldDataElement::SetOffset( int offset )
{
    mOffset = offset;
}

void FieldDataElement::BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
{
    mType->BindTypes( typeEnv, scope );
    // now we can call mType->AsType() and it'll work
}

void FieldDataElement::LayoutFields( IDataEnv* dataEnv )
{
    mOffset = (int) dataEnv->Allocate( mType->GetType()->GetSize() );
}

bool FieldDataElement::GetType( MagoEE::Type*& type )
{
    type = mType->GetType();
    type->AddRef();

    return true;
}

bool FieldDataElement::GetOffset( int& offset )
{
    offset = mOffset;
    return true;
}

bool FieldDataElement::IsField()
{
    return true;
}

void FieldDataElement::AddChild( Element* elem )
{
    throw L"Field can't have children.";
}

void FieldDataElement::SetAttribute( const wchar_t* name, const wchar_t* value )
{
    if ( _wcsicmp( name, L"name" ) == 0 )
    {
        mName = value;
    }
    else if ( _wcsicmp( name, L"type" ) == 0 )
    {
        mType = TypeDataElement::MakeTypeElement( value );
    }
}

void FieldDataElement::SetAttribute( const wchar_t* name, Element* elemValue )
{
    if ( _wcsicmp( name, L"type" ) == 0 )
    {
        mType = dynamic_cast<TypeDataElement*>( elemValue );
    }
}

void FieldDataElement::PrintElement()
{
    printf( "Field %ls\n", mName.c_str() );
    printf( "    offset = %d\n", mOffset );
    mType->PrintElement();
}
