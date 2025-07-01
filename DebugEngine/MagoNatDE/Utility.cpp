/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Utility.h"
#include <MagoEED.h>

using namespace std;
using namespace MagoST;


wstring BuildCommandLine( const wchar_t* exe, const wchar_t* args )
{
    _ASSERT( exe != NULL );

    wstring         cmdLine;
    const wchar_t*  firstSpace = wcschr( exe, L' ' );

    if ( firstSpace != NULL )
    {
        cmdLine.append( 1, L'"' );
        cmdLine.append( exe );
        cmdLine.append( 1, L'"' );
    }
    else
    {
        cmdLine = exe;
    }

    if ( (args != NULL) && (args[0] != L'\0') )
    {
        cmdLine.append( 1, L' ' );
        cmdLine.append( args );
    }

    return cmdLine;
}


HRESULT GetProcessId( IDebugProcess2* proc, DWORD& id )
{
    _ASSERT( proc != NULL );

    HRESULT         hr = S_OK;
    AD_PROCESS_ID   physProcId = { 0 };

    hr = proc->GetPhysicalProcessId( &physProcId );
    if ( FAILED( hr ) )
        return hr;

    if ( physProcId.ProcessIdType != AD_PROCESS_ID_SYSTEM )
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

    id = physProcId.ProcessId.dwProcessId;

    return S_OK;
}

HRESULT GetProcessId( IDebugProgram2* prog, DWORD& id )
{
    HRESULT         hr = S_OK;
    CComPtr<IDebugProcess2> proc;

    hr = prog->GetProcess( &proc );
    if ( FAILED( hr ) )
        return hr;

    return GetProcessId( proc, id );
}

HRESULT Utf8To16( const char* u8Str, size_t u8StrLen, BSTR& u16Str )
{
    int         nChars = 0;
    BSTR        bstr = NULL;
    CComBSTR    ccomBstr;

    nChars = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, u8Str, u8StrLen, NULL, 0 );
    if ( nChars == 0 )
        return GetLastHr();

    bstr = SysAllocStringLen( NULL, nChars );       // allocates 1 more char
    if ( bstr == NULL )
        return GetLastHr();

    ccomBstr.Attach( bstr );

    nChars = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, u8Str, u8StrLen, ccomBstr.m_str, nChars + 1 );
    if ( nChars == 0 )
        return GetLastHr();

    ccomBstr.m_str[nChars] = L'\0';

    u16Str = ccomBstr.Detach();

    return S_OK;
}

HRESULT Utf16To8( const wchar_t* u16Str, size_t u16StrLen, char*& u8Str, size_t& u8StrLen )
{
    int     nChars = 0;
    CAutoVectorPtr<char>    chars;
    DWORD   flags = 0;

#if _WIN32_WINNT >= _WIN32_WINNT_LONGHORN
    flags = WC_ERR_INVALID_CHARS;
#endif

    nChars = WideCharToMultiByte( CP_UTF8, flags, u16Str, u16StrLen, NULL, 0, NULL, NULL );
    if ( nChars == 0 )
        return HRESULT_FROM_WIN32( GetLastError() );

    chars.Attach( new char[ nChars + 1 ] );
    if ( chars == NULL )
        return E_OUTOFMEMORY;

    nChars = WideCharToMultiByte( CP_UTF8, flags, u16Str, u16StrLen, chars, nChars + 1, NULL, NULL );
    if ( nChars == 0 )
        return HRESULT_FROM_WIN32( GetLastError() );
    chars[nChars] = '\0';

    u8Str = chars.Detach();
    u8StrLen = nChars;

    return S_OK;
}


void ConvertVariantToDataVal( const MagoST::Variant& var, MagoEE::DataValue& dataVal )
{
    switch ( var.Tag )
    {
    case VarTag_Char:   dataVal.Int64Value = var.Data.I8;   break;
    case VarTag_UChar:  dataVal.UInt64Value = var.Data.U8;  break;
    case VarTag_Short:  dataVal.Int64Value = var.Data.I16;  break;
    case VarTag_UShort: dataVal.UInt64Value = var.Data.U16;  break;
    case VarTag_Long:   dataVal.Int64Value = var.Data.I32;  break;
    case VarTag_ULong:  dataVal.UInt64Value = var.Data.U32;  break;
    case VarTag_Quadword:   dataVal.Int64Value = var.Data.I64;  break;
    case VarTag_UQuadword:  dataVal.UInt64Value = var.Data.U64;  break;

    case VarTag_Real32: dataVal.Float80Value.FromFloat( var.Data.F32 ); break;
    case VarTag_Real64: dataVal.Float80Value.FromDouble( var.Data.F64 ); break;
    case VarTag_Real80: memcpy( &dataVal.Float80Value, var.Data.RawBytes, sizeof dataVal.Float80Value ); break;
        break;

    case VarTag_Complex32:  
        dataVal.Complex80Value.RealPart.FromFloat( var.Data.C32.Real );
        dataVal.Complex80Value.ImaginaryPart.FromFloat( var.Data.C32.Imaginary );
        break;
    case VarTag_Complex64: 
        dataVal.Complex80Value.RealPart.FromDouble( var.Data.C64.Real );
        dataVal.Complex80Value.ImaginaryPart.FromDouble( var.Data.C64.Imaginary );
        break;
    case VarTag_Complex80: 
        C_ASSERT( offsetof( Complex10, RealPart ) < offsetof( Complex10, ImaginaryPart ) );
        memcpy( &dataVal.Complex80Value, var.Data.RawBytes, sizeof dataVal.Complex80Value );
        break;

    case VarTag_Real48:
    case VarTag_Real128:
    case VarTag_Complex128:
    case VarTag_Varstring: 
    default:
        memset( &dataVal, 0, sizeof dataVal );
        break;
    }
}


//----------------------------------------------------------------------------
//  Compare values
//----------------------------------------------------------------------------

bool EqualFloat32( const void* leftBuf, const void* rightBuf )
{
    const float* left = (const float*) leftBuf;
    const float* right = (const float*) rightBuf;

    // NaNs have to match
    if ( (*left != *left) && (*right != *right) )
        return true;

    return *left == *right;
}

bool EqualFloat64( const void* leftBuf, const void* rightBuf )
{
    const double* left = (const double*) leftBuf;
    const double* right = (const double*) rightBuf;

    // NaNs have to match
    if ( (*left != *left) && (*right != *right) )
        return true;

    return *left == *right;
}

bool EqualFloat80( const void* leftBuf, const void* rightBuf )
{
    const Real10* left = (const Real10*) leftBuf;
    const Real10* right = (const Real10*) rightBuf;

    if ( left->IsNan() && right->IsNan() )
        return true;

    uint16_t comp = Real10::Compare( *left, *right );
    return Real10::IsEqual( comp );
}

bool EqualComplex32( const void* leftBuf, const void* rightBuf )
{
    const float* left = (const float*) leftBuf;
    const float* right = (const float*) rightBuf;

    if ( !EqualFloat32( &left[0], &right[0] ) )
        return false;

    return EqualFloat32( &left[1], &right[1] );
}

bool EqualComplex64( const void* leftBuf, const void* rightBuf )
{
    const double* left = (const double*) leftBuf;
    const double* right = (const double*) rightBuf;

    if ( !EqualFloat64( &left[0], &right[0] ) )
        return false;

    return EqualFloat64( &left[1], &right[1] );
}

bool EqualComplex80( const void* leftBuf, const void* rightBuf )
{
    const Real10* left = (const Real10*) leftBuf;
    const Real10* right = (const Real10*) rightBuf;

    if ( !EqualFloat80( &left[0], &right[0] ) )
        return false;

    return EqualFloat80( &left[1], &right[1] );
}

EqualsFunc GetFloatingEqualsFunc( MagoEE::Type* type )
{
    _ASSERT( type != NULL );
    _ASSERT( type->IsFloatingPoint() );

    EqualsFunc  equals = NULL;

    if ( type->IsComplex() )
    {
        switch ( type->GetSize() )
        {
        case 8: equals = EqualComplex32;  break;
        case 16: equals = EqualComplex64;  break;
        case 20: equals = EqualComplex80;  break;
        default: _ASSERT( false ); break;
        }
    }
    else
    {
        switch ( type->GetSize() )
        {
        case 4: equals = EqualFloat32;  break;
        case 8: equals = EqualFloat64;  break;
        case 10: equals = EqualFloat80;  break;
        default: _ASSERT( false ); break;
        }
    }

    return equals;
}

bool EqualFloat( size_t typeSize, const Real10& left, const Real10& right )
{
    if ( left.IsNan() && right.IsNan() )
        return true;

    switch ( typeSize )
    {
    case 4: return left.ToFloat() == right.ToFloat();
    case 8: return left.ToDouble() == right.ToDouble();
    case 10: 
        uint16_t comp = Real10::Compare( left, right );
        return Real10::IsEqual( comp );
    }

    return false;
}

bool EqualValue( MagoEE::Type* type, const MagoEE::DataValue& left, const MagoEE::DataValue& right )
{
    if ( type->IsPointer() )
    {
        return left.Addr == right.Addr;
    }
    else if ( type->IsIntegral() )
    {
        return left.UInt64Value == right.UInt64Value;
    }
    else if ( type->IsReal() || type->IsImaginary() )
    {
        return EqualFloat( type->GetSize(), left.Float80Value, right.Float80Value );
    }
    else if ( type->IsComplex() )
    {
        size_t size = type->GetSize() / 2;
        if ( !EqualFloat( size, left.Complex80Value.RealPart, right.Complex80Value.RealPart ) )
            return false;

        return EqualFloat( size, left.Complex80Value.ImaginaryPart, right.Complex80Value.ImaginaryPart );
    }
    else if ( type->IsDelegate() )
    {
        return (left.Delegate.ContextAddr == right.Delegate.ContextAddr) 
            && (left.Delegate.FuncAddr == right.Delegate.FuncAddr);
    }

    return false;
}


static uint32_t Get16bits( const uint8_t* x )
{
    return *(uint16_t*) x;
}

static uint32_t Get32bits( const uint8_t* x )
{
    return *(uint32_t*) x;
}

template <class T>
T HashOf( const void* buffer, T length )
{
    // This is what druntime uses to hash most values longer than 32 bits
    /*
    * This is Paul Hsieh's SuperFastHash algorithm, described here:
    *   http://www.azillionmonkeys.com/qed/hash.html
    * It is protected by the following open source license:
    *   http://www.azillionmonkeys.com/qed/weblicense.html
    */
    _ASSERT( buffer != NULL );

    const uint8_t* data = (uint8_t*) buffer;
    int rem = 0;
    T hash = 0;

    rem = length & 3;
    length >>= 2;

    for ( ; length > 0; length-- )
    {
        hash += Get16bits( data );
        T temp = (Get16bits( data + 2 ) << 11) ^ hash;
        hash = (hash << 16) ^ temp;
        data += 2 * sizeof( uint16_t );
        hash += hash >> 11;
    }

    /* Handle end cases */
    switch ( rem )
    {
    case 3: hash += Get16bits( data );
            hash ^= hash << 16;
            hash ^= data[sizeof( uint16_t )] << 18;
            hash += hash >> 11;
            break;
    case 2: hash += Get16bits( data );
            hash ^= hash << 11;
            hash += hash >> 17;
            break;
    case 1: hash += *data;
            hash ^= hash << 10;
            hash += hash >> 1;
            break;
        default:
            break;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

uint32_t MurmurHashOf( const void* bytes, uint32_t length, uint32_t seed )
{
    auto len = length;
    auto data = (uint8_t*) bytes;
    auto nblocks = len / 4;

    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;
    const uint32_t c3 = 0xe6546b64;

    //----------
    // body
    auto end_data = data + nblocks * 4;
    for ( ; data != end_data; data += 4 )
    {
        uint32_t k1 = Get32bits( data );
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> (32 - 15));
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> (32 - 13));
        h1 = h1 * 5 + c3;
    }

    //----------
    // tail
    uint32_t k1 = 0;

    switch (len & 3)
    {
    case 3: k1 ^= data[2] << 16; // fall through
    case 2: k1 ^= data[1] << 8;  // fall through
    case 1: k1 ^= data[0];
            k1 *= c1; k1 = (k1 << 15) | (k1 >> (32 - 15)); k1 *= c2; h1 ^= k1;
    }

    //----------
    // finalization
    h1 ^= len;
    // Force all bits of the hash block to avalanche.
    h1 = (h1 ^ (h1 >> 16)) * 0x85ebca6b;
    h1 = (h1 ^ (h1 >> 13)) * 0xc2b2ae35;
    h1 ^= h1 >> 16;
    return h1;
}


uint32_t HashOf32( const void* buffer, uint32_t length )
{
    return HashOf<uint32_t>( buffer, length );
}

uint64_t HashOf64( const void* buffer, uint32_t length )
{
    return HashOf<uint64_t>( buffer, length );
}


namespace Mago
{
    const wchar_t AddressPrefix[] = L"0x";

    void FormatAddress( wchar_t* str, size_t length, Address64 address, int ptrSize, bool usePrefix )
    {
        const wchar_t* activePrefix = L"";

        if ( usePrefix )
            activePrefix = AddressPrefix;

        if ( ptrSize == 8 )
            swprintf_s( str, length, L"%s%016I64x", activePrefix, (uint64_t) address );
        else
            swprintf_s( str, length, L"%s%08x", activePrefix, (uint32_t) address );
    }
}
