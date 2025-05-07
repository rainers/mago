/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <MagoEED.h>


namespace Mago
{
    class SymbolStore;


    class CVDecl : public MagoEE::Declaration
    {
        long                        mRefCount;
        CComBSTR                    mName;

    protected:
        RefPtr<SymbolStore>         mSymStore;

        RefPtr<MagoST::ISession>    mSession;
        MagoST::SymInfoData         mSymInfoData;
        MagoST::ISymbolInfo*        mSymInfo;

        RefPtr<MagoEE::ITypeEnv>    mTypeEnv;

    public:
        CVDecl( 
            SymbolStore* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo );
        virtual ~CVDecl();

        MagoST::ISymbolInfo* GetSymbol();

        virtual void AddRef();
        virtual void Release();

        virtual const wchar_t* GetName();

        virtual bool hasGetAddressOverload(); // workaround against stack-overflow if called by SymbolStore::GetAddress

        //virtual bool GetType( MagoEE::Type*& type );
        virtual bool GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder );
        virtual bool GetOffset( int& offset );
        virtual bool GetSize( uint32_t& size );
        virtual bool GetBackingTy( MagoEE::ENUMTY& ty );
        virtual bool GetUdtKind( MagoEE::UdtKind& kind );
        virtual bool GetBaseClassOffset( Declaration* baseClass, int& offset );
        virtual bool GetVTableShape( Declaration*& decl );
        virtual bool GetVtblOffset( int& offset );

        virtual bool IsField();
        virtual bool IsStaticField();
        virtual bool IsVar();
        virtual bool IsConstant();
        virtual bool IsType();
        virtual bool IsBaseClass();
        virtual bool IsRegister();
        virtual bool IsFunction();
        virtual bool IsStaticFunction();

        virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl );
        virtual bool EnumMembers( MagoEE::IEnumDeclarationMembers*& members );
        virtual HRESULT FindObjectByValue( uint64_t intVal, Declaration*& decl );
    };


    class GeneralCVDecl : public CVDecl
    {
        RefPtr<MagoEE::Type>    mType;

    public:
        GeneralCVDecl( 
            SymbolStore* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo );

        void SetType( MagoEE::Type* type );

        virtual bool GetType( MagoEE::Type*& type );
    };

    class FunctionCVDecl : public GeneralCVDecl
    {
    public:
        FunctionCVDecl( SymbolStore* symStore, const MagoST::SymInfoData& infoData, MagoST::ISymbolInfo* symInfo );

        virtual bool hasGetAddressOverload();
        virtual bool GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder );
        virtual bool IsFunction();
        virtual bool IsStaticFunction();
    };

    class ClosureVarCVDecl : public GeneralCVDecl
    {
        MagoST::SymHandle mClosureSH;
        std::vector<MagoST::TypeHandle> mChain;

    public:
        ClosureVarCVDecl( SymbolStore* symStore, const MagoST::SymInfoData& infoData, MagoST::ISymbolInfo* symInfo,
                          const MagoST::SymHandle& closureSH, const std::vector<MagoST::TypeHandle>& chain );

        virtual bool hasGetAddressOverload();
        virtual bool GetAddress( MagoEE::Address& addr, MagoEE::IValueBinder* binder );
        virtual bool IsField();
        virtual bool IsVar();
    };

    class TypeCVDecl : public CVDecl
    {
    public:
        TypeCVDecl( 
            SymbolStore* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo );

        //virtual const wchar_t* GetName();

        virtual bool GetType( MagoEE::Type*& type );
        virtual bool GetSize( uint32_t& size );
        virtual bool GetBackingTy( MagoEE::ENUMTY& ty );
        virtual bool GetBaseClassOffset( Declaration* baseClass, int& offset );

        virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl );
        virtual bool EnumMembers( MagoEE::IEnumDeclarationMembers*& members );
        virtual HRESULT FindObjectByValue( uint64_t intVal, Declaration*& decl );

    private:
        bool FindBaseClass( 
            const char* baseName, 
            size_t baseNameLen,
            MagoST::TypeHandle fieldListTH, 
            size_t fieldCount,
            int& offset );

        struct FindBaseClassParams
        {
            const char* Name;
            size_t      NameLen;
            int         OutOffset;
            bool        OutFound;
        };

        typedef bool (TypeCVDecl::*BaseClassFunc)( 
            MagoST::ISymbolInfo* memberInfo,
            MagoST::ISymbolInfo* classInfo,
            void* context );

        void ForeachBaseClass(
            MagoST::TypeHandle fieldListTH, 
            size_t fieldCount,
            BaseClassFunc functor,
            void* context
        );

        bool FindClassInList(
            MagoST::ISymbolInfo* memberInfo,
            MagoST::ISymbolInfo* classInfo,
            void* context );

        bool RecurseClasses(
            MagoST::ISymbolInfo* memberInfo,
            MagoST::ISymbolInfo* classInfo,
            void* context );
    };

    class RegisterCVDecl : public GeneralCVDecl
    {
    public:
        RegisterCVDecl( SymbolStore* symStore, uint32_t reg );
        ~RegisterCVDecl();
    
        virtual bool IsRegister();

        static RegisterCVDecl* CreateRegisterSymbol( SymbolStore* symStore, const char* name );
    };

    class VTableCVDecl : public GeneralCVDecl
    {
    public:
        VTableCVDecl( SymbolStore* symStore, uint32_t count, MagoEE::Type* type );
        ~VTableCVDecl();
    
        virtual bool IsField();
    };

    class TypeCVDeclMembers : public MagoEE::IEnumDeclarationMembers
    {
        long                        mRefCount;

        RefPtr<SymbolStore>         mSymStore;
        RefPtr<MagoST::ISession>    mSession;
        MagoST::SymInfoData         mSymInfoData;
        MagoST::ISymbolInfo*        mSymInfo;

        std::vector<MagoST::TypeScope> mListScopes;
        uint32_t                    mCount;
        uint32_t                    mIndex;

    public:
        TypeCVDeclMembers( 
            SymbolStore* symStore,
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo );

        virtual void AddRef();
        virtual void Release();

        virtual uint32_t GetCount();
        virtual bool Next( MagoEE::Declaration*& decl );
        virtual bool Skip( uint32_t count );
        virtual bool Reset();

    private:
        uint16_t CountMembers();
        // -1 for non-printable, 0 for no more, 1 for ok
        int NextMember( MagoST::TypeHandle& memberTH );
    };


    class ClassRefDecl : public MagoEE::Declaration
    {
        long                        mRefCount;
        RefPtr<MagoEE::Declaration> mOrigDecl;
        RefPtr<MagoEE::ITypeEnv>    mTypeEnv;

    public:
        ClassRefDecl( Declaration* decl, MagoEE::ITypeEnv* typeEnv );

        virtual void AddRef();
        virtual void Release();

        virtual const wchar_t* GetName();
        virtual bool hasGetAddressOverload();

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
        virtual bool IsStaticField();
        virtual bool IsVar();
        virtual bool IsConstant();
        virtual bool IsType();
        virtual bool IsBaseClass();
        virtual bool IsRegister();
        virtual bool IsFunction();
        virtual bool IsStaticFunction();

        virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl );
        virtual bool EnumMembers( MagoEE::IEnumDeclarationMembers*& members );
        virtual HRESULT FindObjectByValue( uint64_t intVal, Declaration*& decl );
    };


    class TupleDecl : public MagoEE::Declaration
    {
        std::wstring                mName;
        long                        mRefCount;
        RefPtr<MagoEE::TypeTuple>   mType;
        RefPtr<MagoEE::ITypeEnv>    mTypeEnv;

        Declaration* firstField();

    public:
        TupleDecl( const wchar_t* name, MagoEE::TypeTuple* tt, MagoEE::ITypeEnv* typeEnv );

        virtual void AddRef();
        virtual void Release();

        virtual const wchar_t* GetName();

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
        virtual bool IsStaticField();
        virtual bool IsVar();
        virtual bool IsConstant();
        virtual bool IsType();
        virtual bool IsBaseClass();
        virtual bool IsRegister();
        virtual bool IsFunction();
        virtual bool IsStaticFunction();

        virtual HRESULT FindObject( const wchar_t* name, Declaration*& decl );
        virtual bool EnumMembers( MagoEE::IEnumDeclarationMembers*& members );
        virtual HRESULT FindObjectByValue( uint64_t intVal, Declaration*& decl );
    };
}
