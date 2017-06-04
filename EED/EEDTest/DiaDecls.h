/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <MagoCVSTI.h>


class DiaDecl : public MagoEE::Declaration
{
    long                        mRefCount;
    CComBSTR                    mName;

protected:
    RefPtr<MagoST::ISession>    mSession;
    MagoST::SymInfoData         mSymInfoData;
    MagoST::ISymbolInfo*        mSymInfo;

    RefPtr<MagoEE::ITypeEnv>    mTypeEnv;

public:
    DiaDecl( 
        MagoST::ISession* session, 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv );

    MagoST::ISymbolInfo* GetSymbol();

    virtual void AddRef();
    virtual void Release();

    virtual const wchar_t* GetName();

    //virtual bool GetType( MagoEE::Type*& type );
    virtual bool GetAddress( MagoEE::Address& addr );
    virtual bool GetOffset( int& offset );
    virtual bool GetSize( uint32_t& size );
    virtual bool GetBackingTy( MagoEE::ENUMTY& ty );
    virtual bool GetUdtKind( MagoEE::UdtKind& kind );
    virtual bool GetBaseClassOffset( Declaration* baseClass, int& offset );

    virtual bool IsField();
    virtual bool IsVar();
    virtual bool IsConstant();
    virtual bool IsType();
    virtual bool IsBaseClass();
    virtual bool IsStaticField();
    virtual bool IsRegister();

    virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl );
    virtual bool EnumMembers( MagoEE::IEnumDeclarationMembers*& members );
    virtual HRESULT FindObjectByValue( uint64_t intVal, Declaration*& decl );
};


class GeneralDiaDecl : public DiaDecl
{
    RefPtr<MagoEE::Type>    mType;

public:
    GeneralDiaDecl( 
        MagoST::ISession* session, 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv );

    void SetType( MagoEE::Type* type );

#if 0
    virtual void AddRef();
    virtual void Release();

    virtual const wchar_t* GetName();
#endif

    virtual bool GetType( MagoEE::Type*& type );
#if 0
    virtual bool GetAddress( MagoEE::Address& addr );
    virtual bool GetOffset( int& offset );
    virtual bool GetSize( uint32_t& size );

    virtual bool IsField();
    virtual bool IsVar();
    virtual bool IsConstant();
    virtual bool IsType();
#endif
};


class TypeDiaDecl : public DiaDecl
{
    MagoST::TypeHandle  mTypeHandle;

public:
    TypeDiaDecl( 
        MagoST::ISession* session, 
        MagoST::TypeHandle typeHandle,
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv );

#if 0
    void SetType( MagoEE::Type* type );

    virtual void AddRef();
    virtual void Release();

    virtual const wchar_t* GetName();
#endif

    virtual bool GetType( MagoEE::Type*& type );
#if 0
    virtual bool GetAddress( MagoEE::Address& addr );
    virtual bool GetOffset( int& offset );
#endif
    virtual bool GetSize( uint32_t& size );
    virtual bool GetBackingTy( MagoEE::ENUMTY& ty );

#if 0
    virtual bool IsField();
    virtual bool IsVar();
    virtual bool IsConstant();
    virtual bool IsType();
#endif

    virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl );
    virtual bool EnumMembers( MagoEE::IEnumDeclarationMembers*& members );

private:
    MagoST::SymTag GetTag();
};
