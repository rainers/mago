/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <MagoEED.h>


namespace Mago
{
    class ExprContext;


    class CVDecl : public MagoEE::Declaration
    {
        long                        mRefCount;
        CComBSTR                    mName;

    protected:
        RefPtr<ExprContext>         mSymStore;

        RefPtr<MagoST::ISession>    mSession;
        MagoST::SymInfoData         mSymInfoData;
        MagoST::ISymbolInfo*        mSymInfo;

        RefPtr<MagoEE::ITypeEnv>    mTypeEnv;

    public:
        CVDecl( 
            ExprContext* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo );

        MagoST::ISymbolInfo* GetSymbol();

        virtual void AddRef();
        virtual void Release();

        virtual const wchar_t* GetName();

        //virtual bool GetType( MagoEE::Type*& type );
        virtual bool GetAddress( MagoEE::Address& addr );
        virtual bool GetOffset( int& offset );
        virtual bool GetSize( uint32_t& size );
        virtual bool GetBackingTy( MagoEE::ENUMTY& ty );

        virtual bool IsField();
        virtual bool IsVar();
        virtual bool IsConstant();
        virtual bool IsType();

        virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl );
    };


    class GeneralCVDecl : public CVDecl
    {
        RefPtr<MagoEE::Type>    mType;

    public:
        GeneralCVDecl( 
            ExprContext* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo );

        void SetType( MagoEE::Type* type );

        virtual bool GetType( MagoEE::Type*& type );
    };


    class TypeCVDecl : public CVDecl
    {
        MagoST::TypeHandle  mTypeHandle;

    public:
        TypeCVDecl( 
            ExprContext* symStore,
            MagoST::TypeHandle typeHandle,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo );

        //virtual const wchar_t* GetName();

        virtual bool GetType( MagoEE::Type*& type );
        virtual bool GetSize( uint32_t& size );
        virtual bool GetBackingTy( MagoEE::ENUMTY& ty );

        virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl );
    };
}
