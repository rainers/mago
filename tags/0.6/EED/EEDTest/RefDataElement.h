/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class RefDataElement : public DataElement
{
public:
    virtual bool    GetAddress( MagoEE::Address& addr ) = 0;

    static RefPtr<RefDataElement>   MakeRefElement( const wchar_t* value );
};


class DeclRefDataElement : public RefDataElement
{
    std::wstring        mPath;
    RefPtr<MagoEE::Declaration> mDecl;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual bool GetAddress( MagoEE::Address& addr );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};
