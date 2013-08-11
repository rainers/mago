/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <MagoEED.h>


struct BB;
class IProcess;


namespace Mago
{
    class DebuggerProxy;


    class DRuntime
    {
        DebuggerProxy*      mDebugger;
        RefPtr<IProcess>    mCoreProc;

    public:
        DRuntime( DebuggerProxy* debugger, IProcess* coreProcess );

        virtual HRESULT GetValue(
            MagoEE::Address aArrayAddr, 
            const MagoEE::DataObject& key, 
            MagoEE::Address& valueAddr );

        HRESULT GetClassName( Address addr, BSTR* pbstrClassName );

        HRESULT GetExceptionInfo( Address addr, BSTR* pbstrInfo );

    private:
        HRESULT GetStructHash( 
            const MagoEE::DataObject& key, 
            const BB& bb, 
            // To hash a struct, we need to read the key memory into a buffer.
            // Return it, because the caller needs it, too.
            HeapPtr& keyBuf,
            uint32_t& hash );

        HRESULT GetArrayHash( 
            const MagoEE::DataObject& key, 
            // To hash an array, we need to read the key memory into a buffer.
            // Return it, because the caller needs it, too.
            HeapPtr& keyBuf,
            uint32_t& hash );

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

        HRESULT GetHash( MagoEE::Type* type, const MagoEE::DataValue& value, uint32_t& hash );

        HRESULT ReadMemory( MagoEE::Address addr, uint32_t sizeToRead, void* buffer );

        DRuntime& operator=( const DRuntime& other );
        DRuntime( const DRuntime& other );
    };
}
