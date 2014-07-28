/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DRuntime.h"
#include "IDebuggerProxy.h"
#include "ICoreProcess.h"
#include "ArchData.h"
#include <MagoEED.h>


struct DArray32
{
    uint32_t    length; // size_t
    uint32_t    ptr;    // T*
};

struct aaA32
{
    uint32_t    next;   // aaA*
    uint32_t    hash;   // size_t
    // key
    // value
};

struct BB32
{
    DArray32    b;      // aaA*[]
    uint32_t    nodes;  // size_t
    uint32_t    keyti;  // TypeInfo
    // aaA*[4]  binit
};

struct TypeInfo_Struct32
{
    uint32_t    vptr;
    uint32_t    monitor;

    DArray32    name;       // string
    DArray32    m_init;     // void[]
    uint32_t    xtoHash;    // size_t   function(in void*)
    uint32_t    xopEquals;  // bool     function(in void*, in void*)
    uint32_t    xopCmp;     // int      function(in void*, in void*)
    uint32_t    xtoString;  // char[]   function(in void*)
    uint32_t    m_flags;
    uint32_t    xdtor;      // void function(void*)
    uint32_t    xpostblit;  // void function(void*)
    uint32_t    m_align;
    // ...
};

struct DArray64
{
    uint64_t    length; // size_t
    uint64_t    ptr;    // T*
};

struct aaA64
{
    uint64_t    next;   // aaA*
    uint64_t    hash;   // size_t
    // key
    // value
};

struct BB64
{
    DArray64    b;      // aaA*[]
    uint64_t    nodes;  // size_t
    uint64_t    keyti;  // TypeInfo
    // aaA*[4]  binit
};

struct TypeInfo_Struct64
{
    uint64_t    vptr;
    uint64_t    monitor;

    DArray64    name;       // string
    DArray64    m_init;     // void[]
    uint64_t    xtoHash;    // size_t   function(in void*)
    uint64_t    xopEquals;  // bool     function(in void*, in void*)
    uint64_t    xopCmp;     // int      function(in void*, in void*)
    uint64_t    xtoString;  // char[]   function(in void*)
    uint32_t    m_flags;
    uint64_t    xdtor;      // void function(void*)
    uint64_t    xpostblit;  // void function(void*)
    uint32_t    m_align;
};

union aaAUnion
{
    aaA32 _32;
    aaA64 _64;
};


namespace Mago
{
    DRuntime::DRuntime( IDebuggerProxy* debugger, ICoreProcess* coreProcess )
        :   mDebugger( debugger ),
            mCoreProc( coreProcess ),
            mPtrSize( 0 )
    {
        _ASSERT( debugger != NULL );
        _ASSERT( coreProcess != NULL );

        ArchData* archData = coreProcess->GetArchData();
        mPtrSize = archData->GetPointerSize();
    }

    uint32_t DRuntime::AlignTSize( uint32_t size )
    {
        if ( mPtrSize == 4 )
            return (size + sizeof( uint32_t ) - 1) & ~(sizeof( uint32_t ) - 1);
        else
            return (size + 16 - 1) & ~(16 - 1);
    }

    uint64_t DRuntime::DHashOf( const void* buffer, uint32_t length )
    {
        if ( mPtrSize == 4 )
            return HashOf32( buffer, length );
        else
            return HashOf64( buffer, length );
    }

    HRESULT DRuntime::GetHash( MagoEE::Type* type, const MagoEE::DataValue& value, uint64_t& hash )
    {
        _ASSERT( type != NULL );

        if ( type->IsPointer() )
        {
            hash = value.Addr;
        }
        else if ( type->IsIntegral() && (type->GetSize() <= sizeof( uint32_t )) )
        {
            hash = value.UInt64Value;
        }
        else if ( type->GetBackingTy() == MagoEE::Tfloat32 || type->GetBackingTy() == MagoEE::Timaginary32 )
        {
            float f = value.Float80Value.ToFloat();
            hash = *(uint32_t*) &f;
        }
        else if ( type->IsIntegral() && (type->GetSize() > sizeof( uint32_t )) )
        {
            hash = DHashOf( &value.UInt64Value, sizeof value.UInt64Value );
        }
        else if ( type->GetBackingTy() == MagoEE::Tfloat64 || type->GetBackingTy() == MagoEE::Timaginary64 )
        {
            double d = value.Float80Value.ToDouble();
            hash = DHashOf( &d, sizeof d );
        }
        else if ( type->GetBackingTy() == MagoEE::Tfloat80 || type->GetBackingTy() == MagoEE::Timaginary80 )
        {
            hash = DHashOf( &value.Float80Value, sizeof value.Float80Value );
        }
        else if ( type->GetBackingTy() == MagoEE::Tcomplex32 )
        {
            float c[2] = { value.Complex80Value.RealPart.ToFloat(), value.Complex80Value.ImaginaryPart.ToFloat() };
            hash = DHashOf( c, sizeof c );
        }
        else if ( type->GetBackingTy() == MagoEE::Tcomplex64 )
        {
            double c[2] = { value.Complex80Value.RealPart.ToDouble(), value.Complex80Value.ImaginaryPart.ToDouble() };
            hash = DHashOf( c, sizeof c );
        }
        else if ( type->GetBackingTy() == MagoEE::Tcomplex80 )
        {
            hash = DHashOf( &value.Complex80Value, sizeof value.Complex80Value );
        }
        else if ( type->IsDelegate() )
        {
            uint32_t delPtrs[2] = { (uint32_t) value.Delegate.ContextAddr, (uint32_t) value.Delegate.FuncAddr };
            hash = DHashOf( delPtrs, sizeof delPtrs );
        }
        else
            return E_FAIL;

        if ( mPtrSize == 4 )
            hash &= 0xFFFFFFFF;

        return S_OK;
    }

    HRESULT DRuntime::GetStructHash( 
        const MagoEE::DataObject& key, 
        const BB64& bb, 
        HeapPtr& keyBuf, 
        uint64_t& hash )
    {
        _ASSERT( keyBuf.IsEmpty() );

        HRESULT hr = S_OK;
        TypeInfo_Struct64 tiStruct;

        if ( (key.Addr == 0) || (bb.keyti == 0) )
            return E_FAIL;

        hr = ReadTypeInfoStruct( bb.keyti, tiStruct );
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

        // TODO: I think we can get init().length. Try again

        // TODO: we only need to hash upto init().length, but we can't get that 
        //       from the CV debug info. So, hope that the size is the same
        hash = DHashOf( keyBuf, key._Type->GetSize() );
        return S_OK;
    }

    HRESULT DRuntime::GetArrayHash( 
        const MagoEE::DataObject& key, 
        HeapPtr& keyBuf, 
        uint64_t& hash )
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
                uint64_t    elemHash = 0;

                if ( elemType->AsTypeStruct() != NULL )
                {
                    elemHash = DHashOf( buf + offset, initSize );
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
                if ( mPtrSize == 4 )
                    hash &= 0xFFFFFFFF;
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
                    if ( mPtrSize == 4 )
                        hash &= 0xFFFFFFFF;
                }
            }
            // TODO: There's a bug in druntime, where the length in elements is used as the size in bytes to hash
            //       merge the non-basic case into the basic one when it's fixed
            else if ( elemType->IsBasic() )
            {
                hash = DHashOf( buf, size );
            }
            else
            {
                hash = DHashOf( buf, (uint32_t) key.Value.Array.Length );
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
        const int AAA_BUF_SIZE = sizeof( aaA64 ) + sizeof( MagoEE::DataValue );

        HRESULT     hr = S_OK;
        BB64        bb = { 0 };
        uint64_t    aaaBuf[ (AAA_BUF_SIZE + sizeof( uint64_t )-1) / sizeof( uint64_t ) ] = { 0 };
        aaAUnion*   aaa = (aaAUnion*) aaaBuf;
        uint64_t    hash = 0;
        HeapPtr     keyBuf;

        hr = ReadBB( aArrayAddr, bb );
        if ( FAILED( hr ) )
            return hr;

        if ( (bb.b.ptr == 0) || (bb.b.length == 0) )
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

        uint64_t    bucketIndex = hash % bb.b.length;
        uint64_t    aaAAddr = 0;
        uint32_t    lenToRead = key._Type->GetSize();
        uint32_t    lenBeforeValue = AlignTSize( key._Type->GetSize() );
        HeapPtr     nodeBuf;
        HeapPtr     nodeArrayBuf;

        if ( mPtrSize == 4 )
        {
            lenToRead += sizeof( aaA32 );
            lenBeforeValue += sizeof( aaA32 );
        }
        else
        {
            lenToRead += sizeof( aaA64 );
            lenBeforeValue += sizeof( aaA64 );
        }

        if ( lenToRead > sizeof aaaBuf )
        {
            nodeBuf = (uint8_t*) HeapAlloc( GetProcessHeap(), 0, lenToRead );
            if ( nodeBuf.IsEmpty() )
                return E_OUTOFMEMORY;

            aaa = (aaAUnion*) nodeBuf.Get();
        }

        hr = ReadAddress( bb.b.ptr, bucketIndex, aaAAddr );
        if ( FAILED( hr ) )
            return hr;

        valueAddr = 0;

        for ( int i = 0; (i < MAX_AA_SEARCH_NODES) && (aaAAddr != 0); i++ )
        {
            MagoEE::DataValue nodeKey = { 0 };
            bool     found = false;
            uint64_t aaaHash;
            uint64_t aaaNext;
            void*    pAaaKey;

            hr = ReadMemory( aaAAddr, lenToRead, aaa );
            if ( FAILED( hr ) )
                return hr;

            if ( mPtrSize == 4 )
            {
                aaaHash = aaa->_32.hash;
                aaaNext = aaa->_32.next;
                pAaaKey = &aaa->_32 + 1;
            }
            else
            {
                aaaHash = aaa->_64.hash;
                aaaNext = aaa->_64.next;
                pAaaKey = &aaa->_64 + 1;
            }

            if ( aaaHash == hash )
            {
                if ( key._Type->AsTypeStruct() != NULL )
                {
                    found = EqualStruct( key, keyBuf, pAaaKey );
                }
                else if ( key._Type->IsSArray() )
                {
                    found = EqualSArray( key, keyBuf, pAaaKey );
                }
                else if ( key._Type->IsDArray() )
                {
                    hr = FromRawValue( pAaaKey, key._Type, nodeKey );
                    if ( FAILED( hr ) )
                        return hr;

                    found = EqualDArray( key, keyBuf, nodeArrayBuf, nodeKey );
                }
                else
                {
                    hr = FromRawValue( pAaaKey, key._Type, nodeKey );
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

            aaAAddr = aaaNext;
        }

        if ( valueAddr == 0 )
            return E_NOT_FOUND;

        return S_OK;
    }

    HRESULT DRuntime::ReadMemory( MagoEE::Address addr, uint32_t sizeToRead, void* buffer )
    {
        HRESULT         hr = S_OK;
        uint32_t        len = sizeToRead;
        uint32_t        lenRead = 0;
        uint32_t        lenUnreadable = 0;

        hr = mDebugger->ReadMemory(
            mCoreProc.Get(),
            (Address64) addr,
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

    HRESULT DRuntime::ReadBB( Address64 address, BB64& bb )
    {
        HRESULT hr = S_OK;

        if ( mPtrSize == 4 )
        {
            BB32    bb32;

            hr = ReadMemory( address, sizeof bb32, &bb32 );
            if ( FAILED( hr ) )
                return hr;

            bb.b.length = bb32.b.length;
            bb.b.ptr = bb32.b.ptr;
            bb.keyti = bb32.keyti;
            bb.nodes = bb32.nodes;
        }
        else
        {
            hr = ReadMemory( address, sizeof bb, &bb );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }

    HRESULT DRuntime::ReadTypeInfoStruct( Address64 addr, TypeInfo_Struct64& ti )
    {
        HRESULT hr = S_OK;

        if ( mPtrSize == 4 )
        {
            TypeInfo_Struct32   ti32;

            hr = ReadMemory( addr, sizeof ti32, &ti32 );
            if ( FAILED( hr ) )
                return hr;

            ti.vptr = ti32.vptr;
            ti.monitor = ti32.monitor;
            ti.name.length = ti32.name.length;
            ti.name.ptr = ti32.name.ptr;
            ti.m_init.length = ti32.m_init.length;
            ti.m_init.ptr = ti32.m_init.ptr;
            ti.m_align = ti32.m_align;
            ti.m_flags = ti32.m_flags;
            ti.xdtor = ti32.xdtor;
            ti.xopCmp = ti32.xopCmp;
            ti.xopEquals = ti32.xopEquals;
            ti.xpostblit = ti32.xpostblit;
            ti.xtoHash = ti32.xtoHash;
            ti.xtoString = ti32.xtoString;
        }
        else
        {
            hr = ReadMemory( addr, sizeof ti, &ti );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }

    HRESULT DRuntime::ReadAddress( Address64 baseAddr, uint64_t index, uint64_t& ptrValue )
    {
        HRESULT hr = S_OK;
        uint64_t addr = baseAddr + (index * mPtrSize);

        if ( mPtrSize == 4 )
        {
            uint32_t    ptrValue32;

            hr = ReadMemory( addr, mPtrSize, &ptrValue32 );
            if ( FAILED( hr ) )
                return hr;

            ptrValue = ptrValue32;
        }
        else
        {
            hr = ReadMemory( addr, mPtrSize, &ptrValue );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }

    HRESULT DRuntime::ReadDArray( Address64 addr, DArray64& darray )
    {
        HRESULT hr = S_OK;

        if ( mPtrSize == 4 )
        {
            DArray32    darray32;

            hr = ReadMemory( addr, sizeof darray32, &darray32 );
            if ( FAILED( hr ) )
                return hr;

            darray.length = darray32.length;
            darray.ptr = darray32.ptr;
        }
        else
        {
            hr = ReadMemory( addr, sizeof darray, &darray );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }


    ////////////////////////////////////////////////////////////////////////
    // struct Interface;
    // class MemberInfo;
    // struct OffsetTypeInfo;
    // class Object;

    // D class info
    struct TypeInfo_Class32
    {
        uint32_t pvtbl;                 // vtable pointer of TypeInfo_Class object
        uint32_t monitor;

        DArray32 init;                  // byte[]; class static initializer
        DArray32 name;                  // string; class name
        DArray32 vtbl;                  // void*[]; virtual function pointer table
        DArray32 interfaces;            // Interface[]
        uint32_t base;                  // TypeInfo_Class
        uint32_t destructor;            // void*
        uint32_t classInvariant;        // void function(Object)
        uint32_t m_flags;
        //  1:      // is IUnknown or is derived from IUnknown
        //  2:      // has no possible pointers into GC memory
        //  4:      // has offTi[] member
        //  8:      // has constructors
        // 16:      // has xgetMembers member
        // 32:      // has typeinfo member
        uint32_t deallocator;           // void*
        DArray32 m_offTi;               // OffsetTypeInfo[]
        uint32_t defaultConstructor;    // void function(Object)
        // ...
    };

    // D class info
    struct TypeInfo_Class64
    {
        uint64_t pvtbl;                 // vtable pointer of TypeInfo_Class object
        uint64_t monitor;

        DArray64 init;                  // byte[]; class static initializer
        DArray64 name;                  // string; class name
        DArray64 vtbl;                  // void*[]; virtual function pointer table
        DArray64 interfaces;            // Interface[]
        uint64_t base;                  // TypeInfo_Class
        uint64_t destructor;            // void*
        uint64_t classInvariant;        // void function(Object)
        uint32_t m_flags;
        //  1:      // is IUnknown or is derived from IUnknown
        //  2:      // has no possible pointers into GC memory
        //  4:      // has offTi[] member
        //  8:      // has constructors
        // 16:      // has xgetMembers member
        // 32:      // has typeinfo member
        uint64_t deallocator;           // void*
        DArray64 m_offTi;               // OffsetTypeInfo[]
        uint64_t defaultConstructor;    // void function(Object)
        // ...
    };

    HRESULT DRuntime::GetClassName( Address64 addr, BSTR* pbstrClassName )
    {
        _ASSERT( pbstrClassName != NULL );

        Address64 vtbl, classinfo;
        uint32_t read, unread;
        HRESULT hr = ReadAddress( addr, 0, vtbl );
        if ( SUCCEEDED( hr ) )
        {
            hr = ReadAddress( vtbl, 0, classinfo );
            if ( SUCCEEDED( hr ) )
            {
                DArray64 className;
                Address64 nameAddr = classinfo;

                if ( mPtrSize == 4 )
                    nameAddr += offsetof( TypeInfo_Class32, name );
                else
                    nameAddr += offsetof( TypeInfo_Class64, name );

                hr = ReadDArray( nameAddr, className );
                if ( SUCCEEDED( hr ) )
                {
                    if ( className.length < 4096 )
                    {
                        char* buf = new char[(size_t) className.length];
                        if ( buf == NULL )
                            return E_OUTOFMEMORY;
                        hr = mDebugger->ReadMemory( 
                            mCoreProc, className.ptr, (uint32_t) className.length, read, unread, (uint8_t*) buf );
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

    struct Throwable32
    {
        uint32_t pvtbl; // vtable pointer of Throwable object
        uint32_t monitor;

        DArray32 msg;   // string
        DArray32 file;  // string
        uint32_t line;  // size_t
        // ...
    };

    struct Throwable64
    {
        uint64_t pvtbl; // vtable pointer of Throwable object
        uint64_t monitor;

        DArray64 msg;   // string
        DArray64 file;  // string
        uint64_t line;  // size_t
        // ...
    };

    HRESULT DRuntime::ReadThrowable( Address64 addr, Throwable64& throwable )
    {
        HRESULT hr = S_OK;

        if ( mPtrSize == 4 )
        {
            Throwable32 throwable32;

            hr = ReadMemory( addr, sizeof throwable32, &throwable32 );
            if ( FAILED( hr ) )
                return hr;

            throwable.pvtbl = throwable32.pvtbl;
            throwable.monitor = throwable32.monitor;
            throwable.msg.length = throwable32.msg.length;
            throwable.msg.ptr = throwable32.msg.ptr;
            throwable.file.length = throwable32.file.length;
            throwable.file.ptr = throwable32.file.ptr;
            throwable.line = throwable32.line;
        }
        else
        {
            hr = ReadMemory( addr, sizeof throwable, &throwable );
            if ( FAILED( hr ) )
                return hr;
        }

        return S_OK;
    }

    HRESULT DRuntime::GetExceptionInfo( Address64 addr, BSTR* pbstrInfo )
    {
        _ASSERT( pbstrInfo != NULL );

        Throwable64 throwable;
        uint32_t read, unread;
        HRESULT hr = S_OK;
    
        hr = ReadThrowable( addr, throwable );
        if ( FAILED( hr ) )
            return hr;

        if ( throwable.msg.length >= 1024 || throwable.file.length >= 1024 )
        {
            *pbstrInfo = SysAllocString( L"" );
            if ( *pbstrInfo == NULL )
                return E_OUTOFMEMORY;
        }
        else
        {
            CAutoVectorPtr<char>    buf;
            if ( !buf.Allocate( (size_t) (throwable.msg.length + throwable.file.length + 30) ) )
                return E_OUTOFMEMORY;
            char* p = buf;
            if ( throwable.msg.length > 0 )
            {
                hr = mDebugger->ReadMemory( mCoreProc, throwable.msg.ptr, (uint32_t) throwable.msg.length, 
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
                hr = mDebugger->ReadMemory( mCoreProc, throwable.file.ptr, (uint32_t) throwable.file.length, 
                                            read, unread, (uint8_t*) p );
                if ( SUCCEEDED( hr ) )
                {
                    p += read;    // read at most throwable.file.length
                    *p++ = '(';
                    p += sprintf_s( p, 12, "%d", (uint32_t) throwable.line );
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
