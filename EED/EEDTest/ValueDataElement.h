/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class ValueDataElement : public DataElement
{
public:
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type ) = 0;
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::Type* type );

    static RefPtr<ValueDataElement>   MakeValueElement( const wchar_t* value );
};


class NullValueElement : public ValueDataElement
{
public:
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::Type* type );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class IntValueElement : public ValueDataElement
{
public:
    uint64_t    Value;

    IntValueElement();

    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::Type* type );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    static void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type, uint64_t val );
};


class RealValueElement : public ValueDataElement
{
public:
    Real10      Value;

    RealValueElement();

    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::Type* type );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    static void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type, const Real10& val );
};


class ComplexValueElement : public ValueDataElement
{
public:
    Complex10   Value;

    ComplexValueElement();

    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::Type* type );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class FieldValueDataElement : public ValueDataElement
{
    std::wstring                        mName;
    RefPtr<ValueDataElement> mValue;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class StructValueDataElement : public ValueDataElement
{
    typedef std::list< RefPtr<FieldValueDataElement> > FieldList;

    FieldList   mFields;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );

    void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class AddressOfValueDataElement : public ValueDataElement
{
    RefPtr<RefDataElement>  mRef;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::Type* type );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    static void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type, MagoEE::Address val );
};


class ArrayValueDataElement : public ValueDataElement
{
    typedef std::list< RefPtr<ValueDataElement> >   ElementList;

    uint32_t                mStartIndex;
    ElementList             mElems;

public:
    ArrayValueDataElement();

    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};


class SliceValueDataElement : public ValueDataElement
{
    RefPtr<ValueDataElement>    mPtr;
    int32_t                     mStartIndex;
    int32_t                     mEndIndex;

    RefPtr<MagoEE::Type>        mPtrType;
    RefPtr<MagoEE::Type>        mSizeType;

public:
    SliceValueDataElement();

    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit, MagoEE::Type* type );
    virtual std::shared_ptr<DataObj> Evaluate( MagoEE::Type* type );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();
};
