/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

class DataObj;


class DeclDataElement : public DataElement, public MagoEE::Declaration
{
protected:
    std::wstring    mName;

public:
    // new
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void LayoutFields( IDataEnv* dataEnv );
    virtual void LayoutVars( IDataEnv* dataEnv );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit );

    // Element
    const wchar_t* GetName();

    // DataElement

    // MagoEE::Declaration
    virtual void AddRef() { DataElement::AddRef(); }
    virtual void Release() { DataElement::Release(); }

    virtual bool GetType( MagoEE::Type*& type );
    virtual bool GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder );

    virtual bool GetOffset( int& offset );
    virtual bool GetSize( uint32_t& size );
    virtual bool GetBackingTy( MagoEE::ENUMTY& ty );
    virtual bool GetUdtKind( MagoEE::UdtKind& kind );
    virtual bool GetBaseClassOffset( Declaration* baseClass, int& offset );
    virtual bool GetVTableShape( Declaration*& decl );
    virtual bool GetVtblOffset( int& offset );

    virtual bool IsField();
    virtual bool IsVar();
    virtual bool IsConstant();
    virtual bool IsType();
    virtual bool IsBaseClass();
    virtual bool IsStaticField();
    virtual bool IsRegister();
    virtual bool IsFunction();
    virtual bool IsStaticFunction();

    virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl );
    virtual bool EnumMembers( MagoEE::IEnumDeclarationMembers*& members );
    virtual HRESULT FindObjectByValue( uint64_t intVal, Declaration*& decl );
};


class VarDataElement : public DeclDataElement
{
    RefPtr<TypeDataElement>  mType;
    RefPtr<ValueDataElement> mInitVal;
    MagoEE::Address                     mAddr;

public:
    VarDataElement();

    // DeclDataElement
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void LayoutVars( IDataEnv* dataEnv );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit );

    // Element
    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    // Declaration
    virtual bool GetType( MagoEE::Type*& type );
    virtual bool GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder );
    virtual bool IsVar();
};


class ConstantDataElement : public DeclDataElement
{
    RefPtr<TypeDataElement>  mType;
    RefPtr<ValueDataElement> mVal;

public:
    std::shared_ptr<DataObj> Evaluate();

    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    virtual bool GetType( MagoEE::Type*& type );

    virtual bool IsConstant();
};


class NamespaceDataElement : public DeclDataElement
{
    typedef std::map< std::wstring, RefPtr< DeclDataElement > >  ElementMap;

    ElementMap      mChildren;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void LayoutFields( IDataEnv* dataEnv );
    virtual void LayoutVars( IDataEnv* dataEnv );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl );
};


class StructDataElement : public DeclDataElement, public IDataEnv
{
    typedef std::map< std::wstring, RefPtr< DeclDataElement > >  ElementMap;

    ElementMap      mChildren;
    uint32_t        mSize;

public:
    StructDataElement();

    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void LayoutFields( IDataEnv* dataEnv );
    virtual void LayoutVars( IDataEnv* dataEnv );
    virtual void WriteData( uint8_t* buffer, uint8_t* bufLimit );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    virtual bool GetType( MagoEE::Type*& type );

    virtual bool GetSize( uint32_t& size );
    virtual bool IsType();

    virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl );

    // IDataEnv
    virtual MagoEE::Address Allocate( uint32_t size );
};


class BasicTypeDefDataElement : public DeclDataElement
{
    RefPtr<MagoEE::Type>    mType;
    long                    mRefCount;

public:
    BasicTypeDefDataElement( MagoEE::Type* type );

    virtual void AddRef();
    virtual void Release();

    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    virtual bool GetType( MagoEE::Type*& type );
    virtual bool GetSize( uint32_t& size );
    virtual bool IsType();
};


class TypeDefDataElement : public DeclDataElement
{
    RefPtr<TypeDataElement>  mBaseType;

public:
    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    virtual bool GetType( MagoEE::Type*& type );
    virtual bool GetSize( uint32_t& size );
    virtual bool IsType();
};


class FieldDataElement : public DeclDataElement
{
    int                                 mOffset;
    RefPtr<TypeDataElement>  mType;

public:
    FieldDataElement();
    void    SetOffset( int offset );

    virtual void BindTypes( MagoEE::ITypeEnv* typeEnv, IScope* scope );
    virtual void LayoutFields( IDataEnv* dataEnv );

    virtual void AddChild( Element* elem );
    virtual void SetAttribute( const wchar_t* name, const wchar_t* value );
    virtual void SetAttribute( const wchar_t* name, Element* elemValue );
    virtual void PrintElement();

    virtual bool GetType( MagoEE::Type*& type );

    virtual bool GetOffset( int& offset );
    virtual bool IsField();
};
