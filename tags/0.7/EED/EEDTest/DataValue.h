/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    class Type;
    class Declaration;
}


class DataObj
{
public:
    enum Kind
    {
        LValue,
        RValue
    };

private:
    RefPtr<MagoEE::Type>    mType;

public:
    MagoEE::DataValue       Value;

    DataObj();
    virtual ~DataObj();

    virtual Kind GetKind() const = 0;
    MagoEE::Type* GetType() const;
    virtual bool GetAddress( MagoEE::Address& addr );
    virtual bool GetDeclaration( MagoEE::Declaration*& decl );
    void SetType( MagoEE::Type* type );
    void SetType( const RefPtr<MagoEE::Type>& type );
    virtual std::wstring ToString() = 0;

protected:
    void AppendValue( std::wstring& str );
};


class LValueObj : public DataObj
{
    RefPtr<MagoEE::Declaration> mDecl;
    MagoEE::Address             mAddr;

public:
    LValueObj( MagoEE::Declaration* decl, MagoEE::Address addr );

    virtual Kind GetKind() const;
    virtual bool GetAddress( MagoEE::Address& addr );
    virtual bool GetDeclaration( MagoEE::Declaration*& decl );
    virtual std::wstring ToString();
};


class RValueObj : public DataObj
{
public:
    virtual Kind GetKind() const;
    virtual std::wstring ToString();
};
