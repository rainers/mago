/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "TestElement.h"
#include "DataElement.h"


class DataEnv : public IValueEnv, public IDataEnv
{
    UniquePtr<uint8_t[]>            mBuf;
    size_t                          mBufSize;
    size_t                          mAllocSize;

public:
    DataEnv( size_t size );
    DataEnv( const DataEnv& orig );

    uint8_t*    GetBuffer();
    uint8_t*    GetBufferLimit();

    virtual MagoEE::Address Allocate( uint32_t size );

    virtual std::shared_ptr<DataObj> GetValue( MagoEE::Declaration* decl );
    virtual std::shared_ptr<DataObj> GetValue( MagoEE::Address address, MagoEE::Type* type );
    virtual void SetValue( MagoEE::Address address, DataObj* obj );
    virtual RefPtr<MagoEE::Declaration> GetThis();
    virtual RefPtr<MagoEE::Declaration> GetSuper();
    virtual bool GetArrayLength( MagoEE::dlength_t& length );

    static uint64_t ReadInt( uint8_t* srcBuf, MagoEE::Address addr, size_t size, bool isSigned );
    static Real10 ReadFloat( uint8_t* srcBuf, MagoEE::Address addr, MagoEE::Type* type );

private:
    std::shared_ptr<DataObj> GetValue( MagoEE::Address address, MagoEE::Type* type, MagoEE::Declaration* decl );

    uint64_t ReadInt( MagoEE::Address addr, size_t size, bool isSigned );
    Real10 ReadFloat( MagoEE::Address addr, MagoEE::Type* type );
};


class DataEnvBinder : public MagoEE::IValueBinder
{
    IValueEnv*  mDataEnv;
    IScope*     mScope;

public:
    DataEnvBinder( IValueEnv* env, IScope* scope );

    virtual HRESULT FindObject( const wchar_t* name, MagoEE::Declaration*& decl, uint32_t findFlags );

    virtual HRESULT GetThis( MagoEE::Declaration*& decl );
    virtual HRESULT GetSuper( MagoEE::Declaration*& decl );
    virtual HRESULT GetReturnType( MagoEE::Type*& type );

    virtual HRESULT GetValue( MagoEE::Declaration* decl, MagoEE::DataValue& value );
    virtual HRESULT GetValue( MagoEE::Address addr, MagoEE::Type* type, MagoEE::DataValue& value );
    virtual HRESULT GetValue( MagoEE::Address aArrayAddr, const MagoEE::DataObject& key, MagoEE::Address& valueAddr );
    virtual int GetAAVersion();
    virtual HRESULT GetClassName( MagoEE::Address addr, std::wstring& className );

    virtual HRESULT SetValue( MagoEE::Declaration* decl, const MagoEE::DataValue& value );
    virtual HRESULT SetValue( MagoEE::Address addr, MagoEE::Type* type, const MagoEE::DataValue& value );

    virtual HRESULT ReadMemory( MagoEE::Address addr, uint32_t sizeToRead, uint32_t& sizeRead, uint8_t* buffer );
    virtual HRESULT SymbolFromAddr( MagoEE::Address addr, std::wstring& symName, MagoEE::Type** pType );
    virtual HRESULT CallFunction( MagoEE::Address addr, MagoEE::ITypeFunction* func, MagoEE::Address arg, MagoEE::DataObject& value );
};
