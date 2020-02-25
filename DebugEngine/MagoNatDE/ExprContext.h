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
    class DRuntime;

    class ExprContext : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugExpressionContext2,
        public MagoEE::IValueBinder
    {
        Address64                       mPC;
        RefPtr<IRegisterSet>            mRegSet;
        RefPtr<Module>                  mModule;
        RefPtr<Thread>                  mThread;
        MagoST::SymHandle               mFuncSH;
        std::vector<MagoST::SymHandle>  mBlockSH;
        RefPtr<MagoEE::ITypeEnv>        mTypeEnv;
        RefPtr<MagoEE::NameTable>       mStrTable;

    public:
        ExprContext();
        virtual ~ExprContext();

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
            MagoEE::Declaration*& decl,
            uint32_t findFlags );
        virtual HRESULT FindObjectType( 
            MagoEE::Declaration* decl,
            const wchar_t* name, 
            MagoEE::Type*& type );

        virtual HRESULT GetThis( MagoEE::Declaration*& decl );
        virtual HRESULT GetSuper( MagoEE::Declaration*& decl );
        virtual HRESULT GetReturnType( MagoEE::Type*& type );
        virtual HRESULT NewTuple( const wchar_t* name, const std::vector<RefPtr<MagoEE::Declaration>>& decls, MagoEE::Declaration*& decl );

        virtual HRESULT GetValue( 
            MagoEE::Declaration* decl, 
            MagoEE::DataValue& value );

        virtual HRESULT GetValue( 
            MagoEE::Address addr, 
            MagoEE::Type* type, 
            MagoEE::DataValue& value );

        virtual HRESULT GetValue(
            MagoEE::Address aArrayAddr, 
            const MagoEE::DataObject& key, 
            MagoEE::Address& valueAddr );

        virtual int GetAAVersion();
        virtual HRESULT GetClassName( MagoEE::Address addr, std::wstring& className, bool derefOnce );

        virtual HRESULT SetValue( 
            MagoEE::Declaration* decl, 
            const MagoEE::DataValue& value );

        virtual HRESULT SetValue( 
            MagoEE::Address addr, 
            MagoEE::Type* type, 
            const MagoEE::DataValue& value );

        virtual HRESULT GetSession( MagoST::ISession*& session );

        virtual HRESULT ReadMemory( 
            MagoEE::Address addr, 
            uint32_t sizeToRead, 
            uint32_t& sizeRead, 
            uint8_t* buffer );

        virtual HRESULT WriteMemory( 
            MagoEE::Address addr, 
            uint32_t sizeToWrite, 
            uint32_t& sizeWritten, 
            uint8_t* buffer );

        virtual HRESULT SymbolFromAddr( MagoEE::Address addr, std::wstring& symName, MagoEE::Type** pType );

        virtual HRESULT CallFunction( MagoEE::Address addr, MagoEE::ITypeFunction* func, MagoEE::Address arg, MagoEE::DataObject& value );

        virtual HRESULT GetRegValue( DWORD reg, MagoEE::DataValueKind& kind, MagoEE::DataValue& value );

        virtual Address64 GetTebBase();

        virtual DRuntime* GetDRuntime();

        ////////////////////////////////////////////////////////////
        MagoEE::ITypeEnv* GetTypeEnv();
        MagoEE::NameTable* GetStringTable();
        MagoST::SymHandle GetFunctionSH();
        Mago::IRegisterSet* GetRegisterSet();
        const std::vector<MagoST::SymHandle>& GetBlockSH();
        DWORD GetPC(); // RVA

        HRESULT GetAddress( MagoEE::Declaration* decl, MagoEE::Address& addr );

        static MagoEE::ENUMTY GetBasicTy( DWORD diaBaseTypeId, DWORD size );

        HRESULT MakeDeclarationFromSymbol( 
            MagoST::SymHandle handle, 
            MagoEE::Declaration*& decl );

        HRESULT ExprContext::MakeDeclarationFromSymbolDerefClass(
            MagoST::SymHandle handle,
            MagoEE::Declaration*& decl, uint32_t findFlags );

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

        HRESULT Evaluate( 
            MagoEE::Declaration* decl, 
            MagoEE::DataObject& resultObj );

    public:
        HRESULT Init( 
            Module* module,
            Thread* thread,
            MagoST::SymHandle funcSH, 
            std::vector<MagoST::SymHandle>& blockSH,
            Address64 pc,
            IRegisterSet* regSet );

        Thread* GetThread();

    private:
        HRESULT FindLocalSymbol( const char* name, size_t nameLen, MagoST::SymHandle& localSH );
        HRESULT FindGlobalSymbol( const char* name, size_t nameLen, MagoEE::Declaration*& decl, uint32_t findFlags );
        HRESULT FindClosureSymbol( const char* name, size_t nameLen, MagoEE::Declaration*& decl );

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

        HRESULT MakeDeclarationFromFunctionSymbol( 
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo, 
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromTypedefSymbol( 
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo, 
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromBaseClassSymbol(
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo,
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromVTableShapeSymbol(
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo,
            MagoEE::Declaration*& decl );

	public:
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
    };
}
