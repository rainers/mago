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
    class ModuleContext;

    // delegate to ModuleContext to avoid circular references through MagoEE::Declaration
    class SymbolStore
    {
        long mRefCount;
        ModuleContext* mModuleContext; // weak reference

    public:
        SymbolStore(ModuleContext* context) : mRefCount(0), mModuleContext(context) {}
        
        void setModuleContext(ModuleContext* context) { mModuleContext = context; }

        void AddRef();
        void Release();

        HRESULT GetSession(MagoST::ISession*& session);
        MagoEE::ITypeEnv* GetTypeEnv();

        HRESULT MakeDeclarationFromSymbol(MagoST::TypeHandle handle, MagoEE::Declaration*& decl);
        HRESULT MakeDeclarationFromDataSymbol(const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo, MagoEE::Declaration*& decl);
        HRESULT MakeDeclarationFromDataSymbol(const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo,MagoEE::Type* type, MagoEE::Declaration*& decl);
        HRESULT GetTypeFromTypeSymbol(MagoST::TypeIndex typeIndex, MagoEE::Type*& type);

        HRESULT ReadMemory(MagoEE::Address addr, uint32_t sizeToRead, uint32_t& sizeRead, uint8_t* buffer);
    };

    // ModuleContext: PC/Stack agnostic methods
    class ModuleContext : 
        public CComObjectRootEx<CComMultiThreadModel>
    {
        RefPtr<SymbolStore>             mSymStore;
        RefPtr<Module>                  mModule;
        RefPtr<Program>                 mProgram;
        RefPtr<MagoEE::ITypeEnv>        mTypeEnv;
        RefPtr<MagoEE::NameTable>       mStrTable;
        // don't store Declaration because it creates a reference cycle to ModuleContext
        std::map<std::pair<std::wstring, std::wstring>,
                 std::pair<RefPtr<MagoEE::Type>, MagoEE::Address>> mDebugFuncCache;

        DECLARE_NOT_AGGREGATABLE(ModuleContext)
        BEGIN_COM_MAP(ModuleContext)
        END_COM_MAP()

        friend class ExprContext;

    public:
        ModuleContext();
        virtual ~ModuleContext();

    public:
        HRESULT Init( Module* module, Program* program );

        // IValueBinder implementations forwarded from ExprContext
        virtual HRESULT FindObjectType( MagoEE::Declaration* decl, const wchar_t* name, MagoEE::Type*& type );
        virtual HRESULT FindDebugFunc(const wchar_t* name, MagoEE::ITypeStruct* ts, MagoEE::Type*& type, MagoEE::Address& fnaddr);

        virtual HRESULT NewTuple( const wchar_t* name, const std::vector<RefPtr<MagoEE::Declaration>>& decls, MagoEE::Declaration*& decl );

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

        virtual HRESULT CallFunction( MagoEE::Address addr, MagoEE::ITypeFunction* func, MagoEE::Address arg,
                                      MagoEE::DataObject& value, bool saveGC,
                                      std::function < HRESULT(HRESULT, MagoEE::DataObject)> complete );

        virtual DRuntime* GetDRuntime();

        bool ShortenClassName( std::wstring& className );

        ////////////////////////////////////////////////////////////
        MagoEE::ITypeEnv* GetTypeEnv();
        MagoEE::NameTable* GetStringTable();

        static MagoEE::ENUMTY GetBasicTy( DWORD diaBaseTypeId, DWORD size );

        HRESULT MakeDeclarationFromSymbol( 
            MagoST::SymHandle handle, 
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromSymbolDerefClass(
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

    private:
        HRESULT MakeDeclarationFromFunctionSymbol( 
            const MagoST::SymInfoData& infoData,
            MagoST::ISymbolInfo* symInfo, 
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromTypedefSymbol( 
            const MagoST::SymInfoData& infoData, 
            MagoST::ISymbolInfo* symInfo, 
            MagoEE::Declaration*& decl );

        HRESULT MakeDeclarationFromEnumSymbol( 
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

        static HRESULT FindGlobalSymbol( 
            MagoST::ISession* session, 
            const char* name, 
            size_t nameLen, 
            MagoST::SymHandle& globalSH );
    };

    class ExprContext : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugExpressionContext2,
        public MagoEE::IValueBinder
    {
        Address64                       mPC;
        RefPtr<IRegisterSet>            mRegSet;
        RefPtr<Thread>                  mThread;
        MagoST::SymHandle               mFuncSH;
        std::vector<MagoST::SymHandle>  mBlockSH;

        RefPtr<ModuleContext> mModuleContext;

    DECLARE_NOT_AGGREGATABLE(ExprContext)

    BEGIN_COM_MAP(ExprContext)
        COM_INTERFACE_ENTRY(IDebugExpressionContext2)
    END_COM_MAP()

    public:
        ExprContext();

        HRESULT Init(
            Module* module,
            Thread* thread,
            MagoST::SymHandle funcSH,
            std::vector<MagoST::SymHandle>& blockSH,
            Address64 pc,
            IRegisterSet* regSet );

        Thread* GetThread();
        ModuleContext* GetModuleContext() { return mModuleContext; }

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

        HRESULT Evaluate(
            MagoEE::Declaration* decl,
            MagoEE::DataObject& resultObj);

        //////////////////////////////////////////////////////////// 
        // MagoEE::IValueBinder 
        virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl, uint32_t findFlags );
        virtual HRESULT FindObjectType( MagoEE::Declaration* decl, const wchar_t* name, MagoEE::Type*& type )
        { return mModuleContext->FindObjectType( decl, name, type ); }
        virtual HRESULT FindDebugFunc(const wchar_t* name, MagoEE::ITypeStruct* ts, MagoEE::Type*& type, MagoEE::Address& fnaddr )
        { return mModuleContext->FindDebugFunc( name, ts, type, fnaddr ); }

        virtual HRESULT GetThis( MagoEE::Declaration*& decl );
        virtual HRESULT GetSuper( MagoEE::Declaration*& decl );
        virtual HRESULT GetReturnType( MagoEE::Type*& type );

        virtual HRESULT NewTuple( const wchar_t* name, const std::vector<RefPtr<MagoEE::Declaration>>& decls, MagoEE::Declaration*& decl )
        { return mModuleContext->NewTuple( name, decls, decl ); }

        virtual HRESULT GetValue( MagoEE::Declaration* decl, MagoEE::DataValue& value );

        virtual HRESULT GetValue( MagoEE::Address addr, MagoEE::Type* type, MagoEE::DataValue& value )
        { return mModuleContext->GetValue( addr, type, value ); }

        virtual HRESULT GetValue( MagoEE::Address aArrayAddr, const MagoEE::DataObject& key, MagoEE::Address& valueAddr )
        { return mModuleContext->GetValue( aArrayAddr, key, valueAddr ); }

        virtual int GetAAVersion() { return mModuleContext->GetAAVersion(); }
        virtual HRESULT GetClassName( MagoEE::Address addr, std::wstring& className, bool derefOnce )
        { return mModuleContext->GetClassName( addr, className, derefOnce ); }

        virtual HRESULT SetValue( MagoEE::Declaration* decl, const MagoEE::DataValue& value );

        virtual HRESULT SetValue( MagoEE::Address addr, MagoEE::Type* type, const MagoEE::DataValue& value )
        { return mModuleContext->SetValue( addr, type, value ); }

        virtual HRESULT GetSession( MagoST::ISession*& session );

        virtual HRESULT ReadMemory( MagoEE::Address addr, uint32_t sizeToRead, uint32_t& sizeRead, uint8_t* buffer )
        { return mModuleContext->ReadMemory( addr, sizeToRead, sizeRead, buffer ); }

        virtual HRESULT WriteMemory( MagoEE::Address addr, uint32_t sizeToWrite, uint32_t& sizeWritten, uint8_t* buffer )
        { return mModuleContext->WriteMemory( addr, sizeToWrite, sizeWritten, buffer ); }

        virtual HRESULT FindGlobalSymbolAddr( const std::wstring& symName, MagoEE::Address& addr );

        virtual HRESULT SymbolFromAddr( MagoEE::Address addr, std::wstring& symName, MagoEE::Type** pType )
        { return mModuleContext->SymbolFromAddr( addr, symName, pType ); }

        virtual HRESULT CallFunction( MagoEE::Address addr, MagoEE::ITypeFunction* func, MagoEE::Address arg,
                                      MagoEE::DataObject& value, bool saveGC,
                                      std::function < HRESULT(HRESULT, MagoEE::DataObject)> complete )
        { return mModuleContext->CallFunction( addr, func, arg, value, saveGC, complete ); }

        virtual HRESULT GetRegValue( DWORD reg, MagoEE::DataValueKind& kind, MagoEE::DataValue& value );

        virtual Address64 GetTebBase();

        virtual DRuntime* GetDRuntime();
        ///////////////////////////////
        MagoST::SymHandle GetFunctionSH();
        Mago::IRegisterSet* GetRegisterSet();
        const std::vector<MagoST::SymHandle>& GetBlockSH();
        DWORD GetPC(); // RVA
        HRESULT GetFunctionName( bool names, bool types, bool values, int radix,
            std::string& funcName, std::function<HRESULT(HRESULT hr, std::string& funcName)> complete );
        std::wstring GetFunctionReturnType();

    private:
        HRESULT GetAddress( MagoEE::Declaration* decl, MagoEE::Address& addr );

        HRESULT FindLocalSymbol( const char* name, size_t nameLen, MagoST::SymHandle& localSH );
        HRESULT FindGlobalSymbol( const char* name, size_t nameLen, MagoEE::Declaration*& decl, uint32_t findFlags );
        HRESULT FindClosureSymbol( const char* name, size_t nameLen, MagoEE::Declaration*& decl );
    };

}
