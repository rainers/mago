/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <MagoEED.h>


namespace Mago
{
    class Thread;
    class IRegisterSet;


    class ATL_NO_VTABLE ExprContext : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugExpressionContext2,
        public MagoEE::IValueBinder
    {
        Address                         mPC;
        RefPtr<IRegisterSet>            mRegSet;
        RefPtr<Module>                  mModule;
        RefPtr<Thread>                  mThread;
        MagoST::SymHandle               mFuncSH;
        MagoST::SymHandle               mBlockSH;
        RefPtr<MagoEE::ITypeEnv>        mTypeEnv;
        RefPtr<MagoEE::NameTable>       mStrTable;

    public:
        ExprContext();
        ~ExprContext();

    DECLARE_NOT_AGGREGATABLE(ExprContext)

    BEGIN_COM_MAP(ExprContext)
        COM_INTERFACE_ENTRY(IDebugExpressionContext2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugExpressionContext2 

        STDMETHOD( GetName )( 
            BSTR* pbstrName );
        
        STDMETHOD( ParseText )( 
            LPCOLESTR pszCode,
            PARSEFLAGS dwFlags,
            UINT nRadix,
            IDebugExpression2** ppExpr,
            BSTR* pbstrError,
            UINT* pichError );

        //////////////////////////////////////////////////////////// 
        // MagoEE::IValueBinder 

        virtual HRESULT FindObject( 
            const wchar_t* name, 
            MagoEE::Declaration*& decl );

        virtual HRESULT GetThis( MagoEE::Declaration*& decl );
        virtual HRESULT GetSuper( MagoEE::Declaration*& decl );
        virtual HRESULT GetReturnType( MagoEE::Type*& type );

        virtual HRESULT GetValue( 
            MagoEE::Declaration* decl, 
            MagoEE::DataValue& value );

        virtual HRESULT GetValue( 
            MagoEE::Address addr, 
            MagoEE::Type* type, 
            MagoEE::DataValue& value );

        virtual HRESULT SetValue( 
            MagoEE::Declaration* decl, 
            const MagoEE::DataValue& value );

        virtual HRESULT SetValue( 
            MagoEE::Address addr, 
            MagoEE::Type* type, 
            const MagoEE::DataValue& value );

        virtual HRESULT ReadMemory( 
            MagoEE::Address addr, 
            uint32_t sizeToRead, 
            uint32_t& sizeRead, 
            uint8_t* buffer );

        //////////////////////////////////////////////////////////// 
        // IMagoSymStore 

        STDMETHOD( GetAddress )( Address& address ) { _ASSERT( false ); return E_NOTIMPL; }
        STDMETHOD( GetSession )( MagoST::ISession*& session );

        MagoEE::ITypeEnv* GetTypeEnv();
        MagoEE::NameTable* GetStringTable();
        MagoST::SymHandle GetFunctionSH();
        MagoST::SymHandle GetBlockSH();

        HRESULT GetAddress( MagoEE::Declaration* decl, MagoEE::Address& addr );

        static MagoEE::ENUMTY GetBasicTy( DWORD diaBaseTypeId, DWORD size );

        HRESULT MakeDeclarationFromSymbol( 
            MagoST::SymHandle handle, 
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromSymbol( 
            MagoST::TypeHandle handle, 
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromDataSymbol( 
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo, 
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromDataSymbol( 
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo, 
            MagoEE::Type* type,
            MagoEE::Declaration*& decl );

    public:
        HRESULT Init( 
            Module* module,
            Thread* thread,
            MagoST::SymHandle funcSH, 
            MagoST::SymHandle blockSH,
            Address pc,
            IRegisterSet* regSet );

    private:
        HRESULT FindLocalSymbol( const char* name, size_t nameLen, MagoST::SymHandle& localSH );
        HRESULT FindGlobalSymbol( const char* name, size_t nameLen, MagoST::SymHandle& globalSH );

        static HRESULT FindLocalSymbol( 
            MagoST::ISession* session, 
            MagoST::SymHandle blockSH,
            const char* name, 
            size_t nameLen, 
            MagoST::SymHandle& localSH );

        static HRESULT FindGlobalSymbol( 
            MagoST::ISession* session, 
            const char* name, 
            size_t nameLen, 
            MagoST::SymHandle& globalSH );

        HRESULT MakeDeclarationFromTypedefSymbol( 
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo, 
            MagoEE::Declaration*& decl );

        HRESULT GetTypeFromTypeSymbol( 
            MagoST::TypeIndex typeIndex,
            MagoEE::Type*& type );

        HRESULT GetFunctionTypeFromTypeSymbol( 
            MagoST::TypeHandle typeHandle,
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo,
            MagoEE::Type*& type );

        HRESULT GetUdtTypeFromTypeSymbol( 
            MagoST::TypeHandle typeHandle,
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo,
            MagoEE::Type*& type );

        HRESULT GetBasicTypeFromTypeSymbol( 
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo,
            MagoEE::Type*& type );

        HRESULT GetCustomTypeFromTypeSymbol( 
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo,
            MagoEE::Type*& type );

        HRESULT GetRegValue( DWORD reg, MagoEE::DataValueKind& kind, MagoEE::DataValue& value );
    };
}
