/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "TestElement.h"
#include "DataElement.h"
#include <MagoCVSTI.h>


class Exec;
class IProcess;
struct IDiaSession;
struct IDiaSymbol;

namespace MagoST
{
    class ISession;
    struct SymHandle;
}


class ProgramValueEnv : public IValueEnv, public IScope
{
    std::wstring                mProgPath;
    uint32_t                    mStopRva;
    RefPtr<MagoEE::ITypeEnv>    mTypeEnv;
    Exec*                       mExec;
    IProcess*                   mProc;
    uint32_t                    mThreadId;
#if 0
    IDiaSession*                mSymSession;
    IDiaSymbol*                 mFuncSym;
    IDiaSymbol*                 mClassSym;
#else
    MagoST::ISession*           mSymSession;
    MagoST::SymHandle           mFuncSH;
    std::vector<MagoST::SymHandle> mBlockSH;
    MagoST::TypeIndex           mThisTI;
#endif

public:
    ProgramValueEnv( const wchar_t* progPath, uint32_t stopRva, MagoEE::ITypeEnv* typeEnv );
    ~ProgramValueEnv();

    HRESULT StartProgram();

    virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl );

    virtual std::shared_ptr<DataObj> GetValue( MagoEE::Declaration* decl );
    virtual std::shared_ptr<DataObj> GetValue( MagoEE::Address address, MagoEE::Type* type );
    virtual void SetValue( MagoEE::Address address, DataObj* obj );
    virtual RefPtr<MagoEE::Declaration> GetThis();
    virtual RefPtr<MagoEE::Declaration> GetSuper();
    virtual bool GetArrayLength( MagoEE::dlength_t& length );

    static MagoEE::ENUMTY GetBasicTy( DWORD diaBaseTypeId, DWORD size );

    static HRESULT MakeDeclarationFromSymbol( MagoST::ISession* session, MagoST::SymHandle handle, MagoEE::ITypeEnv* typeEnv, MagoEE::Declaration*& decl );
    static HRESULT MakeDeclarationFromSymbol( MagoST::ISession* session, MagoST::TypeHandle handle, MagoEE::ITypeEnv* typeEnv, MagoEE::Declaration*& decl );

    static HRESULT MakeDeclarationFromDataSymbol( 
        MagoST::ISession* session, 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv, 
        MagoEE::Type* type,
        MagoEE::Declaration*& decl );

private:
    HRESULT LoadSymbols( DWORD64 loadAddr );

    static HRESULT MakeDeclarationFromFunctionSymbol( 
        MagoST::ISession* session, 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv, 
        MagoEE::Declaration*& decl );

    static HRESULT MakeDeclarationFromTypedefSymbol( 
        MagoST::ISession* session, 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv, 
        MagoEE::Declaration*& decl );

    static HRESULT MakeDeclarationFromDataSymbol( 
        MagoST::ISession* session, 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo, 
        MagoEE::ITypeEnv* typeEnv, 
        MagoEE::Declaration*& decl );

    static RefPtr<MagoEE::Type> GetTypeFromTypeSymbol( 
        MagoST::ISession* session, 
        MagoST::TypeIndex typeIndex,
        MagoEE::ITypeEnv* typeEnv );

    static RefPtr<MagoEE::Type> GetTypeFromTypeSymbol( IDiaSymbol* typeSym, MagoEE::ITypeEnv* typeEnv );
    static RefPtr<MagoEE::Type> GetBasicTypeFromTypeSymbol( 
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo,
        MagoEE::ITypeEnv* typeEnv );
    static RefPtr<MagoEE::Type> GetCustomTypeFromTypeSymbol( 
        MagoST::ISession* session, 
        const MagoST::SymInfoData& infoData, 
        MagoST::ISymbolInfo* symInfo,
        MagoEE::ITypeEnv* typeEnv );
    static RefPtr<MagoEE::Type> GetUdtTypeFromTypeSymbol( 
        MagoST::ISession* session, 
        MagoST::TypeHandle type,
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo,
        MagoEE::ITypeEnv* typeEnv );
    static RefPtr<MagoEE::Type> GetFunctionTypeFromTypeSymbol( 
        MagoST::ISession* session, 
        MagoST::TypeHandle type,
        const MagoST::SymInfoData& infoData,
        MagoST::ISymbolInfo* symInfo,
        MagoEE::ITypeEnv* typeEnv );

    HRESULT GetRegValue( DWORD reg, MagoEE::DataValueKind& kind, MagoEE::DataValue& value );
    HRESULT GetSegBase( DWORD segReg, DWORD& base );

    std::shared_ptr<DataObj> GetValue( MagoEE::Address address, MagoEE::Type* type, MagoEE::Declaration* decl );

    HRESULT FindOuterSymbolByRVA( DWORD rva, MagoST::SymHandle& handle );
    HRESULT FindSymbolByRVA( DWORD rva, MagoST::SymHandle& handle, std::vector<MagoST::SymHandle>& innermostChild );
};
