/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DRuntime.h"
#include "DebuggerProxy.h"
#include <MagoEED.h>


struct aaA
{
    uint32_t    next;
    uint32_t    hash;
    // key
    // value
};

struct BB
{
    uint32_t    bLength;
    uint32_t    bAddr;
    uint32_t    nodes;
    uint32_t    keyti;
};

struct TypeInfo_Struct
{
    uint32_t    vptr;
    uint32_t    monitor;
    uint32_t    nameLength;
    uint32_t    namePtr;
    uint32_t    m_initLength;
    uint32_t    m_initPtr;
    uint32_t    xtoHash;
    uint32_t    xopEquals;
    uint32_t    xopCmp;
    uint32_t    xtoString;
    uint32_t    m_flags;
    uint32_t    xgetMembers;
    uint32_t    xdtor;
    uint32_t    xpostblit;
    uint32_t    m_align;
};


namespace Mago
{
    DRuntime::DRuntime( DebuggerProxy* debugger, IProcess* coreProcess )
        :   mDebugger( debugger ),
            mCoreProc( coreProcess )
    {
        _ASSERT( debugger != NULL );
        _ASSERT( coreProcess != NULL );
    }

    uint32_t AlignTSize( uint32_t size )
    {
        return (size + sizeof( uint32_t ) - 1) & ~(sizeof( uint32_t ) - 1);
    }

    HRESULT DRuntime::GetHash( MagoEE::Type* type, const MagoEE::DataValue& value, uint32_t& hash )
    {
        _ASSERT( type != NULL );

        if ( type->IsPointer() )
        {
            hash = (uint32_t) value.Addr;
        }
        else if ( type->IsIntegral() && (type->GetSize() <= sizeof( uint32_t )) )
        {
            hash = (uint32_t) value.UInt64Value;
        }
        else if ( type->GetBackingTy() == MagoEE::Tfloat32 || type->GetBackingTy() == MagoEE::Timaginary32 )
        {
            float f = value.Float80Value.ToFloat();
            hash = *(uint32_t*) &f;
        }
        else if ( type->IsIntegral() && (type->GetSize() > sizeof( uint32_t )) )
        {
            hash = HashOf( &value.UInt64Value, sizeof value.UInt64Value );
        }
        else if ( type->GetBackingTy() == MagoEE::Tfloat64 || type->GetBackingTy() == MagoEE::Timaginary64 )
        {
            double d = value.Float80Value.ToDouble();
            hash = HashOf( &d, sizeof d );
        }
        else if ( type->GetBackingTy() == MagoEE::Tfloat80 || type->GetBackingTy() == MagoEE::Timaginary80 )
        {
            hash = HashOf( &value.Float80Value, sizeof value.Float80Value );
        }
        else if ( type->GetBackingTy() == MagoEE::Tcomplex32 )
        {
            float c[2] = { value.Complex80Value.RealPart.ToFloat(), value.Complex80Value.ImaginaryPart.ToFloat() };
            hash = HashOf( c, sizeof c );
        }
        else if ( type->GetBackingTy() == MagoEE::Tcomplex64 )
        {
            double c[2] = { value.Complex80Value.RealPart.ToDouble(), value.Complex80Value.ImaginaryPart.ToDouble() };
            hash = HashOf( c, sizeof c );
        }
        else if ( type->GetBackingTy() == MagoEE::Tcomplex80 )
        {
            hash = HashOf( &value.Complex80Value, sizeof value.Complex80Value );
        }
        else if ( type->IsDelegate() )
        {
            uint32_t delPtrs[2] = { (uint32_t) value.Delegate.ContextAddr, (uint32_t) value.Delegate.FuncAddr };
            hash = HashOf( delPtrs, sizeof delPtrs );
        }
        else
            return E_FAIL;

        return S_OK;
    }

    HRESULT DRuntime::GetStructHash( 
        const MagoEE::DataObject& key, 
        const BB& bb, 
        HeapPtr& keyBuf, 
        uint32_t& hash )
    {
        _ASSERT( keyBuf.IsEmpty() );

        HRESULT hr = S_OK;
        TypeInfo_Struct tiStruct;

        if ( (key.Addr == 0) || (bb.keyti == 0) )
            return E_FAIL;

        hr = ReadMemory( bb.keyti, sizeof tiStruct, &tiStruct );
        if ( FAILED( hr ) )
            return hr;

        // if the user defined hash and compare functions,
        // then we can't do the standard hash and compare
        if ( (tiStruct.xtoHash != 0) || (tiStruct.xopCmp != 0) )
            return E_FAIL;

        keyBuf = (uint8_t*) HeapAlloc( GetProcessHeap(), 0, key._Type->GetSize() );
        if ( keyBuf.IsEmpty() )
            return E_OUTOFMEMORY;

        hr = ReadMemory( key.Addr, key._Type->GetSize(), keyBuf );
        if ( FAILED( hr ) )
            return hr;

        // TODO: we only need to hash upto init().length, but we can't get that 
        //       from the CV debug info. So, hope that the size is the same
        hash = HashOf( keyBuf, key._Type->GetSize() );
        return S_OK;
    }

    HRESULT DRuntime::GetArrayHash( 
        const MagoEE::DataObject& key, 
        HeapPtr& keyBuf, 
        uint32_t& hash )
    {
        _ASSERT( keyBuf.IsEmpty() );
        _ASSERT( key._Type->IsSArray() || key._Type->IsDArray() );

        HRESULT         hr = S_OK;
        MagoEE::Address addr = 0;
        uint32_t        size = 0;
        RefPtr<MagoEE::Type> elemType;
        const uint8_t*  buf = NULL;

        elemType = key._Type->AsTypeNext()->GetNext();

        if ( key._Type->IsDArray() && (key.Value.Array.LiteralString != NULL) )
        {
            switch ( key.Value.Array.LiteralString->Kind )
            {
            case MagoEE::StringKind_Byte:
                buf = (uint8_t*) ((MagoEE::ByteString*) key.Value.Array.LiteralString)->Str;
                break;
            case MagoEE::StringKind_Utf16:
                buf = (uint8_t*) ((MagoEE::Utf16String*) key.Value.Array.LiteralString)->Str;
                break;
            case MagoEE::StringKind_Utf32:
                buf = (uint8_t*) ((MagoEE::Utf32String*) key.Value.Array.LiteralString)->Str;
                break;
            default:
                _ASSERT( false );
                return E_UNEXPECTED;
            }

            size = key.Value.Array.LiteralString->Length * elemType->GetSize();
        }
        else
        {
            if ( key._Type->IsSArray() )
            {
                addr = key.Addr;
                size = key._Type->GetSize();
            }
            else
            {
                addr = key.Value.Array.Addr;
                size = (uint32_t) key.Value.Array.Length * elemType->GetSize();
            }

            if ( addr == 0 )
                return E_FAIL;

            keyBuf = (uint8_t*) HeapAlloc( GetProcessHeap(), 0, size );
            if ( keyBuf.IsEmpty() )
                return E_OUTOFMEMORY;

            hr = ReadMemory( addr, size, keyBuf );
            if ( FAILED( hr ) )
                return hr;

            buf = keyBuf;
        }

        if ( key._Type->IsSArray() )
        {
            uint32_t    length = key._Type->AsTypeSArray()->GetLength();
            uint32_t    elemSize = elemType->GetSize();

            // TODO: for structs, we only need to hash upto init().length, but we can't 
            //       get that from the CV debug info. So, hope that the size is the same
            uint32_t    initSize = elemType->GetSize();

            hash = 0;

            for ( uint32_t i = 0; i < length; i++ )
            {
                uint32_t    offset = i * elemSize;
                uint32_t    elemHash = 0;

                if ( elemType->AsTypeStruct() != NULL )
                {
                    elemHash = HashOf( buf + offset, initSize );
                }
                else
                {
                    MagoEE::DataValue elem = { 0 };

                    hr = FromRawValue( buf + offset, elemType, elem );
                    if ( FAILED( hr ) )
                        return hr;

                    hr = GetHash( elemType, elem, elemHash );
                    if ( FAILED( hr ) )
                        return hr;
                }

                hash += elemHash;
            }
        }
        else
        {
            if ( elemType->IsBasic() && elemType->IsChar() && (elemType->GetSize() == 1) )
            {
                hash = 0;

                for ( uint32_t i = 0; i < key.Value.Array.Length; i++ )
                {
                    hash = hash * 11 + buf[i];
                }
            }
            // TODO: There's a bug in druntime, where the length in elements is used as the size in bytes to hash
            //       merge the non-basic case into the basic one when it's fixed
            else if ( elemType->IsBasic() )
            {
                hash = HashOf( buf, size );
            }
            else
            {
                hash = HashOf( buf, (uint32_t) key.Value.Array.Length );
            }
        }

        return S_OK;
    }

    bool DRuntime::EqualArray(
        MagoEE::Type* elemType,
        uint32_t length,
        const void* keyBuf, 
        const void* nodeArrayBuf )
    {
        // key and nodeKey array lengths match, because they're the same type
        uint32_t        size = length * elemType->GetSize();

        if ( elemType->IsIntegral() 
            || elemType->IsPointer() 
            || elemType->IsDelegate() )
        {
            return memcmp( keyBuf, nodeArrayBuf, size ) == 0;
        }
        else if ( elemType->IsFloatingPoint() )
        {
            uint32_t    elemSize = elemType->GetSize();
            EqualsFunc  equals = GetFloatingEqualsFunc( elemType );

            if ( equals == NULL )
                return false;

            for ( uint32_t i = 0; i < length; i++ )
            {
                uint32_t    offset = i * elemSize;
                if ( !equals( (uint8_t*) keyBuf + offset, (uint8_t*) nodeArrayBuf + offset ) )
                    return false;
            }

            return true;
        }
        else if ( elemType->AsTypeStruct() != NULL )
        {
            // TODO: you have to compare each element
            //       with structs, only compare upto init().length
            return memcmp( keyBuf, nodeArrayBuf, size ) == 0;
        }

        return false;
    }

    bool DRuntime::EqualSArray(
        const MagoEE::DataObject& key, 
        const void* keyBuf, 
        const void* nodeArrayBuf )
    {
        _ASSERT( key._Type->IsSArray() );

        MagoEE::Type*   elemType = key._Type->AsTypeSArray()->GetElement();
        uint32_t        length = key._Type->AsTypeSArray()->GetLength();

        return EqualArray( elemType, length, keyBuf, nodeArrayBuf );
    }

    bool DRuntime::EqualDArray( 
        const MagoEE::DataObject& key, 
        const void* keyBuf, 
        HeapPtr& inout_nodeArrayBuf, 
        const MagoEE::DataValue& nodeKey )
    {
        _ASSERT( key._Type->IsDArray() );

        HRESULT         hr = S_OK;
        MagoEE::Type*   elemType = key._Type->AsTypeDArray()->GetElement();
        uint32_t        size = (uint32_t) nodeKey.Array.Length * elemType->GetSize();
        const void*     buf = keyBuf;

        if ( nodeKey.Array.Length != key.Value.Array.Length )
            return false;

        if ( inout_nodeArrayBuf.IsEmpty() )
        {
            inout_nodeArrayBuf = (uint8_t*) HeapAlloc( GetProcessHeap(), 0, size );
            if ( inout_nodeArrayBuf.IsEmpty() )
                return false;
        }

        hr = ReadMemory( nodeKey.Array.Addr, size, inout_nodeArrayBuf );
        if ( FAILED( hr ) )
            return false;

        if ( key.Value.Array.LiteralString != NULL )
        {
            switch ( key.Value.Array.LiteralString->Kind )
            {
            case MagoEE::StringKind_Byte:
                buf = (uint8_t*) ((MagoEE::ByteString*) key.Value.Array.LiteralString)->Str;
                break;
            case MagoEE::StringKind_Utf16:
                buf = (uint8_t*) ((MagoEE::Utf16String*) key.Value.Array.LiteralString)->Str;
                break;
            case MagoEE::StringKind_Utf32:
                buf = (uint8_t*) ((MagoEE::Utf32String*) key.Value.Array.LiteralString)->Str;
                break;
            default:
                _ASSERT( false );
                return false;
            }
        }

        return EqualArray( elemType, (uint32_t) nodeKey.Array.Length, buf, inout_nodeArrayBuf );
    }

    bool DRuntime::EqualStruct(
        const MagoEE::DataObject& key, 
        const void* keyBuf, 
        const void* nodeArrayBuf )
    {
        // we would also check and run TypeInfo_Struct.xopCmp, if we supported func eval
        // TODO: actually, you have to compare upto init().length
        return memcmp( keyBuf, nodeArrayBuf, key._Type->GetSize() ) == 0;
    }

    HRESULT DRuntime::GetValue(
        MagoEE::Address aArrayAddr, 
        const MagoEE::DataObject& key, 
        MagoEE::Address& valueAddr )
    {
        const int MAX_AA_SEARCH_NODES = 1000000;

        HRESULT     hr = S_OK;
        BB          bb = { 0 };
        uint8_t     aaaBuf[ sizeof( aaA ) + sizeof( MagoEE::DataValue ) ] = { 0 };
        aaA*        aaa = (aaA*) aaaBuf;
        uint32_t    hash = 0;
        HeapPtr     keyBuf;

        hr = ReadMemory( aArrayAddr, sizeof bb, &bb );
        if ( FAILED( hr ) )
            return hr;

        if ( (bb.bAddr == 0) || (bb.bLength == 0) )
            return E_FAIL;

        if ( key._Type->AsTypeStruct() != NULL )
        {
            hr = GetStructHash( key, bb, keyBuf, hash );
            if ( FAILED( hr ) )
                return hr;
        }
        else if ( key._Type->IsSArray() || key._Type->IsDArray() )
        {
            hr = GetArrayHash( key, keyBuf, hash );
            if ( FAILED( hr ) )
                return hr;
        }
        else
        {
            hr = GetHash( key._Type, key.Value, hash );
            if ( FAILED( hr ) )
                return hr;
        }

        uint32_t    bucketIndex = hash % bb.bLength;
        uint32_t    aaAAddr = 0;
        uint32_t    lenToRead = sizeof( aaA ) + key._Type->GetSize();
        uint32_t    lenBeforeValue = sizeof( aaA ) + AlignTSize( key._Type->GetSize() );
        HeapPtr     nodeBuf;
        HeapPtr     nodeArrayBuf;

        if ( lenToRead > sizeof aaaBuf )
        {
            nodeBuf = (uint8_t*) HeapAlloc( GetProcessHeap(), 0, lenToRead );
            if ( nodeBuf.IsEmpty() )
                return E_OUTOFMEMORY;

            aaa = (aaA*) nodeBuf.Get();
        }

        hr = ReadMemory( bb.bAddr + (bucketIndex * sizeof aaAAddr), sizeof aaAAddr, &aaAAddr );
        if ( FAILED( hr ) )
            return hr;

        valueAddr = 0;

        for ( int i = 0; (i < MAX_AA_SEARCH_NODES) && (aaAAddr != 0); i++ )
        {
            MagoEE::DataValue nodeKey = { 0 };
            bool    found = false;

            hr = ReadMemory( aaAAddr, lenToRead, aaa );
            if ( FAILED( hr ) )
                return hr;

            if ( aaa->hash == hash )
            {
                if ( key._Type->AsTypeStruct() != NULL )
                {
                    found = EqualStruct( key, keyBuf, aaa + 1 );
                }
                else if ( key._Type->IsSArray() )
                {
                    found = EqualSArray( key, keyBuf, aaa + 1 );
                }
                else if ( key._Type->IsDArray() )
                {
                    hr = FromRawValue( aaa + 1, key._Type, nodeKey );
                    if ( FAILED( hr ) )
                        return hr;

                    found = EqualDArray( key, keyBuf, nodeArrayBuf, nodeKey );
                }
                else
                {
                    hr = FromRawValue( aaa + 1, key._Type, nodeKey );
                    if ( FAILED( hr ) )
                        return hr;

                    found = EqualValue( key._Type, key.Value, nodeKey );
                }
            }

            if ( found )
            {
                valueAddr = aaAAddr + lenBeforeValue;
                break;
            }

            aaAAddr = aaa->next;
        }

        if ( valueAddr == 0 )
            return E_NOT_FOUND;

        return S_OK;
    }

    HRESULT DRuntime::ReadMemory( MagoEE::Address addr, uint32_t sizeToRead, void* buffer )
    {
        HRESULT         hr = S_OK;
        SIZE_T          len = sizeToRead;
        SIZE_T          lenRead = 0;
        SIZE_T          lenUnreadable = 0;

        hr = mDebugger->ReadMemory(
            mCoreProc.Get(),
            (Address) addr,
            len,
            lenRead,
            lenUnreadable,
            (uint8_t*) buffer );
        if ( FAILED( hr ) )
            return hr;

        if ( lenRead < sizeToRead )
            return HRESULT_FROM_WIN32( ERROR_PARTIAL_COPY );

        return S_OK;
    }



    ////////////////////////////////////////////////////////////////////////
    template<typename T>
    struct DynamicArray
    {
        size_t length;
        T* ptr;
    };

    struct Interface;
    struct MemberInfo;
    struct OffsetTypeInfo;
    class Object;

    // D class info
    struct TypeInfo_Class
    {
        void* pvtbl; // vtable pointer of TypeInfo_Class object
        void* monitor;

        DynamicArray<uint8_t> init;   // class static initializer
        DynamicArray<char>    name;   // class name
        DynamicArray<void*>   vtbl;   // virtual function pointer table
        DynamicArray<Interface*> interfaces;
        TypeInfo_Class*       base;
        void*                 destructor;
        void (*classInvariant)(Object);
        uint32_t              m_flags;
        //  1:      // is IUnknown or is derived from IUnknown
        //  2:      // has no possible pointers into GC memory
        //  4:      // has offTi[] member
        //  8:      // has constructors
        // 16:      // has xgetMembers member
        // 32:      // has typeinfo member
        void*                 deallocator;
        DynamicArray<OffsetTypeInfo> m_offTi;
        void*                 defaultConstructor;
        const DynamicArray<MemberInfo> (*xgetMembers)(DynamicArray<char*>);
    };

    HRESULT DRuntime::GetClassName( MachineAddress addr, BSTR* pbstrClassName )
    {
        IProcess* process = mCoreProc;

        _ASSERT( process != NULL );
        _ASSERT( pbstrClassName != NULL );
        _ASSERT( process->GetMachine() != NULL );

        IMachine* machine = process->GetMachine();
        MachineAddress vtbl, classinfo;
        SIZE_T read, unread;
        HRESULT hr = machine->ReadMemory( addr, sizeof( vtbl ), read, unread, (uint8_t*) &vtbl );
        if ( SUCCEEDED( hr ) )
        {
            hr = machine->ReadMemory( vtbl, sizeof( classinfo ), read, unread, (uint8_t*) &classinfo );
            if ( SUCCEEDED( hr ) )
            {
                DynamicArray<char> className;
                hr = machine->ReadMemory( classinfo + offsetof( TypeInfo_Class, name ), sizeof( className ), read, unread, (uint8_t*) &className );
                if ( SUCCEEDED( hr ) )
                {
                    if ( className.length < 4096 )
                    {
                        char* buf = new char[className.length];
                        if ( buf == NULL )
                            return E_OUTOFMEMORY;
                        hr = machine->ReadMemory( (MachineAddress) className.ptr, className.length, read, unread, (uint8_t*) buf );
                        if ( SUCCEEDED( hr ) )
                        {
                            // read at most className.length
                            hr = Utf8To16( buf, read, *pbstrClassName );
                        }
                        delete [] buf;
                    }
                    else
                    {
                        *pbstrClassName = SysAllocString( L"" );
                        if ( *pbstrClassName == NULL )
                            hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        return hr;
    }

    struct Throwable
    {
        void* pvtbl; // vtable pointer of TypeInfo_Class object
        void* monitor;

        DynamicArray<uint8_t> msg;
        DynamicArray<uint8_t> file;
        size_t                line;
        // ...
    };

    HRESULT DRuntime::GetExceptionInfo( MachineAddress addr, BSTR* pbstrInfo )
    {
        IProcess* process = mCoreProc;

        _ASSERT( process != NULL );
        _ASSERT( pbstrInfo != NULL );
        _ASSERT( process->GetMachine() != NULL );

        IMachine* machine = process->GetMachine();
        Throwable throwable;
        SIZE_T read, unread;
        HRESULT hr = S_OK;
    
        hr = machine->ReadMemory( addr, sizeof( throwable ), read, unread, (uint8_t*) &throwable );
        if ( FAILED( hr ) )
            return hr;
        if ( read < sizeof( throwable ) )
            return HRESULT_FROM_WIN32( ERROR_PARTIAL_COPY );

        if ( throwable.msg.length >= 1024 || throwable.file.length >= 1024 )
        {
            *pbstrInfo = SysAllocString( L"" );
            if ( *pbstrInfo == NULL )
                return E_OUTOFMEMORY;
        }
        else
        {
            CAutoVectorPtr<char>    buf;
            if ( !buf.Allocate( throwable.msg.length + throwable.file.length + 30 ) )
                return E_OUTOFMEMORY;
            char* p = buf;
            if ( throwable.msg.length > 0 )
            {
                hr = machine->ReadMemory( (MachineAddress) throwable.msg.ptr, throwable.msg.length, 
                                            read, unread, (uint8_t*) p );
                if ( SUCCEEDED( hr ) )
                {
                    p += read;    // read at most throwable.msg.length
                    *p++ = ' ';
                }
            }
            if ( throwable.file.length > 0 )
            {
                *p++ = 'a';
                *p++ = 't';
                *p++ = ' ';
                hr = machine->ReadMemory( (MachineAddress) throwable.file.ptr, throwable.file.length, 
                                            read, unread, (uint8_t*) p );
                if ( SUCCEEDED( hr ) )
                {
                    p += read;    // read at most throwable.file.length
                    *p++ = '(';
                    p += sprintf_s( p, 12, "%d", throwable.line );
                    *p++ = ')';
                }
            }
            hr = Utf8To16( buf, p - buf, *pbstrInfo );
            if ( FAILED( hr ) )
                return hr;
        }
        return S_OK;
    }
}
