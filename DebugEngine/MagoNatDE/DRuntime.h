/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <MagoEED.h>


struct BB64;
struct TypeInfo_Struct64;
struct DArray64;


namespace Mago
{
    class IDebuggerProxy;
    class ICoreProcess;
    struct Throwable64;


    class DRuntime
    {
        IDebuggerProxy*         mDebugger;
        RefPtr<ICoreProcess>    mCoreProc;
        int                     mPtrSize;
        int                     mAAVersion;
        Address64               mClassInfoVtblAddr;

    public:
        DRuntime( IDebuggerProxy* debugger, ICoreProcess* coreProcess );

        void SetAAVersion( int ver );
        int GetAAVersion() const { return mAAVersion; }

        void SetClassInfoVtblAddr( Address64 addr );

        virtual HRESULT GetValue(
            MagoEE::Address aArrayAddr, 
            const MagoEE::DataObject& key, 
            MagoEE::Address& valueAddr );

        HRESULT GetClassName( Address64 addr, BSTR* pbstrClassName );

        HRESULT GetExceptionInfo( Address64 addr, BSTR* pbstrInfo );

    private:
        HRESULT GetStructHash( 
            const MagoEE::DataObject& key, 
            const BB64& bb, 
            // To hash a struct, we need to read the key memory into a buffer.
            // Return it, because the caller needs it, too.
            HeapPtr& keyBuf,
            uint64_t& hash );

        HRESULT GetArrayHash( 
            const MagoEE::DataObject& key, 
            // To hash an array, we need to read the key memory into a buffer.
            // Return it, because the caller needs it, too.
            HeapPtr& keyBuf,
            uint64_t& hash );

        bool EqualArray(
            MagoEE::Type* elemType,
            uint32_t length,
            const void* keyBuf, 
            const void* nodeArrayBuf );

        bool EqualSArray(
            const MagoEE::DataObject& key, 
            const void* keyBuf, 
            const void* nodeArrayBuf );

        bool EqualDArray(
            const MagoEE::DataObject& key, 
            const void* keyBuf, 
            HeapPtr& inout_nodeArrayBuf, 
            const MagoEE::DataValue& nodeKey );

        bool EqualStruct(
            const MagoEE::DataObject& key, 
            const void* keyBuf, 
            const void* nodeArrayBuf );

        HRESULT GetHash( MagoEE::Type* type, const MagoEE::DataValue& value, uint64_t& hash );
        uint64_t DHashOf( const void* buffer, uint32_t length );
        uint32_t AlignTSize( uint32_t size );

        HRESULT ReadMemory( MagoEE::Address addr, uint32_t sizeToRead, void* buffer );

        HRESULT ReadBB( Address64 addr, BB64& bb, BB64_V1& bb_v1 );
        HRESULT ReadTypeInfoStruct( Address64 addr, TypeInfo_Struct64& ti );
        HRESULT ReadAddress( Address64 baseAddr, uint64_t index, uint64_t& ptrValue );
        HRESULT ReadDArray( Address64 addr, DArray64& darray );
        HRESULT ReadThrowable( Address64 addr, Throwable64& throwable );

        HRESULT FindValue( BB64& bb, uint64_t hash, 
                           const MagoEE::DataObject& key, uint8_t*& keybuf, MagoEE::Address& valueAddr );
        HRESULT FindValue_V1( BB64_V1& bb, uint64_t hash, 
                              const MagoEE::DataObject& key, uint8_t*& keybuf, MagoEE::Address& valueAddr );

        DRuntime& operator=( const DRuntime& other );
        DRuntime( const DRuntime& other );
    };
}
