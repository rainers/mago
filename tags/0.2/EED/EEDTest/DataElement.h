/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Element.h"

namespace MagoEE
{
    class Type;
    enum ENUMTY;
    class Declaration;
}


// TODO: make Declaration derive from this
class IScope
{
public:
    virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl ) = 0;
};


// TODO: should be IMemoryEnv
class IDataEnv
{
public:
    virtual MagoEE::Address Allocate( uint32_t size ) = 0;
};


class DataFactory : public ElementFactory
{
public:
    Element* NewElement( const wchar_t* name );
};


class DataElement : public Element
{
public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope )
    {
    }
};


class DeclDataElement;
class TypeDataElement;
class ValueDataElement;
class RefDataElement;

#include "DeclDataElement.h"
#include "TypeDataElement.h"
#include "ValueDataElement.h"
#include "RefDataElement.h"
