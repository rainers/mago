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

bool ExactFileNameMatch( const char* pathA, size_t pathALen, const char* pathB, size_t pathBLen )
{
    if ( pathALen != pathBLen )
        return false;

    for ( size_t i = 0; i < pathALen; i++ )
    {
        if ( ((pathA[i] == '\\') && (pathB[i] == '/'))
            || ((pathA[i] == '/') && (pathB[i] == '\\')) )
        {
            // empty, mixed slash and back slash count as the same character
        }
        else if ( tolower( pathA[i] ) != tolower( pathB[i] ) )
            return false;
    }

    return true;
}

bool PartialFileNameMatch( const char* pathA, size_t pathALen, const char* pathB, size_t pathBLen )
{
    if ( (pathALen == 0) || (pathBLen == 0) )
        return false;

    const char* pca = pathA + pathALen;
    const char* pcb = pathB + pathBLen;

    do
    {
        pca--;
        pcb--;

        if ( ((*pca == '\\') && (*pcb == '/'))
            || ((*pca == '/') && (*pcb == '\\')) )
        {
            // empty, mixed slash and back slash count as the same character
        }
        else if ( tolower( *pca ) != tolower( *pcb ) )
            return false;

        if ( (*pca == '\\') || (*pca == '/') )
            return true;

    } while ( (pca != pathA) && (pcb != pathB) );

    if ( (pca == pathA) && (pcb != pathB) )
        return (*(pcb - 1) == '\\') || (*(pcb - 1) == '/');
    else if ( (pcb == pathB) && (pca != pathA) )
        return (*(pca - 1) == '\\') || (*(pca - 1) == '/');

    _ASSERT( (pca == pathA) && (pcb == pathB) );
    return true;
}


void ConvertVariantToDataVal( const MagoST::Variant& var, MagoEE::DataValue& dataVal )
{
    switch ( var.Tag )
    {
    case VarTag_Char:   dataVal.Int64Value = var.Data.I8;   break;
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


uint64_t ReadInt( uint8_t* srcBuf, uint32_t bufOffset, size_t size, bool isSigned )
{
    union Integer
    {
        uint8_t     i8;
        uint16_t    i16;
        uint32_t    i32;
        uint64_t    i64;
    };
    Integer     i = { 0 };
    uint64_t    u = 0;

    memcpy( &i, srcBuf + bufOffset, size );

    switch ( size )
    {
    case 1:
        if ( isSigned )
            u = (int8_t) i.i8;
        else
            u = (uint8_t) i.i8;
        break;

    case 2:
        if ( isSigned )
            u = (int16_t) i.i16;
        else
            u = (uint16_t) i.i16;
        break;

    case 4:
        if ( isSigned )
            u = (int32_t) i.i32;
        else
            u = (uint32_t) i.i32;
        break;

    case 8:
        u = i.i64;
        break;
    }

    return u;
}

Real10 ReadFloat( uint8_t* srcBuf, uint32_t bufOffset, MagoEE::Type* type )
{
    union Float
    {
        float   f32;
        double  f64;
        Real10  f80;
    };
    Float   f = { 0 };
    Real10  r;
    MagoEE::ENUMTY  ty = MagoEE::Tnone;
    size_t  size = 0;

    r.Zero();

    if ( type->IsComplex() )
    {
        switch ( type->GetSize() )
        {
        case 8:     ty = MagoEE::Tfloat32;  break;
        case 16:    ty = MagoEE::Tfloat64;  break;
        case 20:    ty = MagoEE::Tfloat80;  break;
        }
    }
    else    // is real or imaginary
    {
        switch ( type->GetSize() )
        {
        case 4:     ty = MagoEE::Tfloat32;  break;
        case 8:     ty = MagoEE::Tfloat64;  break;
        case 10:    ty = MagoEE::Tfloat80;  break;
        }
    }

    switch ( ty )
    {
    case MagoEE::Tfloat32:  size = 4;   break;
    case MagoEE::Tfloat64:  size = 8;   break;
    case MagoEE::Tfloat80:  size = 10;  break;
    }

    memcpy( &f, srcBuf + bufOffset, size );

    switch ( ty )
    {
    case MagoEE::Tfloat32:  r.FromFloat( f.f32 );   break;
    case MagoEE::Tfloat64:  r.FromDouble( f.f64 );  break;
    case MagoEE::Tfloat80:  r = f.f80;  break;
    }

    return r;
}

HRESULT WriteInt( uint8_t* buffer, uint32_t bufSize, MagoEE::Type* type, uint64_t val )
{
    union Integer
    {
        uint8_t     i8;
        uint16_t    i16;
        uint32_t    i32;
        uint64_t    i64;
    };

    uint32_t    size = type->GetSize();
    Integer     iVal = { 0 };

    _ASSERT( size <= bufSize );
    if ( size > bufSize )
        return E_FAIL;

    switch ( size )
    {
    case 1: iVal.i8 = (uint8_t) val;    break;
    case 2: iVal.i16 = (uint16_t) val;   break;
    case 4: iVal.i32 = (uint32_t) val;   break;
    case 8: iVal.i64 = (uint64_t) val;   break;
    default:
        return E_FAIL;
    }

    memcpy( buffer, &iVal, size );

    return S_OK;
}

HRESULT WriteFloat( uint8_t* buffer, uint32_t bufSize, MagoEE::Type* type, const Real10& val )
{
    MagoEE::ENUMTY  ty = MagoEE::Tnone;

    if ( type->IsComplex() )
    {
        switch ( type->GetSize() )
        {
        case 8:     ty = MagoEE::Tfloat32;  break;
        case 16:    ty = MagoEE::Tfloat64;  break;
        case 20:    ty = MagoEE::Tfloat80;  break;
        }
    }
    else    // is real or imaginary
    {
        switch ( type->GetSize() )
        {
        case 4:     ty = MagoEE::Tfloat32;  break;
        case 8:     ty = MagoEE::Tfloat64;  break;
        case 10:    ty = MagoEE::Tfloat80;  break;
        }
    }

    union Float
    {
        float       f32;
        double      f64;
        Real10      f80;
    };

    uint32_t    size = 0;
    Float       fVal = { 0 };

    switch ( ty )
    {
    case MagoEE::Tfloat32: fVal.f32 = val.ToFloat();    size = 4;   break;
    case MagoEE::Tfloat64: fVal.f64 = val.ToDouble();   size = 8;   break;
    case MagoEE::Tfloat80: fVal.f80 = val;   size = 10; break;
    default:
        return E_FAIL;
    }

    _ASSERT ( size <= bufSize );
    if ( size > bufSize )
        return E_FAIL;

    memcpy( buffer, &fVal, size );

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

HRESULT GetClassName( IProcess* process, MachineAddress addr, BSTR* pbstrClassName )
{
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

HRESULT GetExceptionInfo( IProcess* process, MachineAddress addr, BSTR* pbstrInfo )
{
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
