/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class TypeDataElement : public DataElement
{
protected:
    RefPtr<MagoEE::Type>    mType;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope ) = 0;

    MagoEE::Type* GetType()
    {
        return mType.Get();
    }

    static RefPtr<TypeDataElement>   MakeTypeElement( const wchar_t* value );
};


class TypeRefDataElement : public TypeDataElement
{
    std::wstring            mRefName;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class TypePointerDataElement : public TypeDataElement
{
    RefPtr<TypeDataElement>  mPointed;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class TypeDArrayDataElement : public TypeDataElement
{
    RefPtr<TypeDataElement>  mElemType;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class TypeSArrayDataElement : public TypeDataElement
{
    RefPtr<TypeDataElement>  mElemType;
    int                                 mLen;

public:
    TypeSArrayDataElement();

    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class TypeAArrayDataElement : public TypeDataElement
{
    RefPtr<TypeDataElement>  mElemType;
    RefPtr<TypeDataElement>  mKeyType;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};
