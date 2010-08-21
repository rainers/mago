/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EED.h"
#include "FormatValue.h"
#include "Type.h"
#include "UniAlpha.h"


namespace MagoEE
{
    const uint32_t  MaxStringLen = 1048576;
    const uint32_t  RawStringChunkSize = 65536;

    // RawStringChunkSize: FormatRawStringInternal uses this constant to define
    // the size of a buffer it uses for Unicode translation. This many bytes
    // are meant to be read from the debuggee at a time.
    //
    // I didn't want the size to be in the megabytes, but it should still be 
    // big enough that we don't need to make many cross process memory reads.
    //
    // It should be a multiple of four, so that if we break up a code point at
    // the end of a read, we can always get back on track quickly during the 
    // next chunk.


    HRESULT FormatSimpleReal( const Real10& val, std::wstring& outStr )
    {
        wchar_t buf[ Real10::Float80DecStrLen + 1 ] = L"";

        val.ToString( buf, _countof( buf ) );

        outStr.append( buf );

        return S_OK;
    }

    HRESULT FormatComplex( const DataObject& objVal, std::wstring& outStr )
    {
        HRESULT hr = S_OK;

        hr = FormatSimpleReal( objVal.Value.Complex80Value.RealPart, outStr );
        if ( FAILED( hr ) )
            return hr;

        // Real10::ToString won't add a leading '+', but it's needed
        if ( objVal.Value.Complex80Value.ImaginaryPart.GetSign() >= 0 )
            outStr.append( 1, L'+' );

        hr = FormatSimpleReal( objVal.Value.Complex80Value.ImaginaryPart, outStr );
        if ( FAILED( hr ) )
            return hr;

        outStr.append( 1, L'i' );

        return S_OK;
    }

    HRESULT FormatBool( const DataObject& objVal, std::wstring& outStr )
    {
        if ( objVal.Value.UInt64Value == 0 )
        {
            outStr.append( L"false" );
        }
        else
        {
            outStr.append( L"true" );
        }

        return S_OK;
    }

    HRESULT FormatInt( uint64_t number, Type* type, int radix, std::wstring& outStr )
    {
        // 18446744073709551616
        wchar_t buf[20 + 9 + 1] = L"";

        if ( radix == 16 )
        {
            int width = type->GetSize() * 2;
            
            swprintf_s( buf, L"0x%0*I64X", width, number );
        }
        else    // it's 10, or make it 10
        {
            if ( type->IsSigned() )
            {
                swprintf_s( buf, L"%I64d", number );
            }
            else
            {
                swprintf_s( buf, L"%I64u", number );
            }
        }

        outStr.append( buf );

        return S_OK;
    }

    HRESULT FormatInt( const DataObject& objVal, int radix, std::wstring& outStr )
    {
        return FormatInt( objVal.Value.UInt64Value, objVal._Type, radix, outStr );
    }

    HRESULT FormatAddress( Address addr, Type* type, std::wstring& outStr )
    {
        return FormatInt( addr, type, 16, outStr );
    }

    HRESULT FormatChar( const DataObject& objVal, int radix, std::wstring& outStr )
    {
        // object replacement char U+FFFC
        // replacement character U+FFFD
        const wchar_t   ReplacementChar = L'\xFFFD';

        HRESULT hr = S_OK;

        hr = FormatInt( objVal, radix, outStr );
        if ( FAILED( hr ) )
            return hr;

        outStr.append( L" '" );

        switch ( objVal._Type->GetSize() )
        {
        case 1:
            {
                uint8_t         c = (uint8_t) objVal.Value.UInt64Value;
                wchar_t         wc = ReplacementChar;

                if ( c < 0x80 )
                    wc = (wchar_t) c;

                outStr.append( 1, wc );
            }
            break;

        case 2:
            {
                wchar_t     wc = (wchar_t) objVal.Value.UInt64Value;

                if ( (wc >= 0xD800) && (wc <= 0xDFFF) )
                    wc = ReplacementChar;

                outStr.append( 1, wc );
            }
            break;

        case 4:
            AppendChar32( outStr, (dchar_t) objVal.Value.UInt64Value );
            break;
        }

        outStr.append( 1, L'\'' );

        return S_OK;
    }

    HRESULT FormatBasicValue( const DataObject& objVal, int radix, std::wstring& outStr )
    {
        _ASSERT( objVal._Type->IsBasic() );

        HRESULT hr = S_OK;
        Type*   type = NULL;

        if ( (objVal._Type == NULL) || !objVal._Type->IsScalar() )
            return E_FAIL;

        type = objVal._Type;

        if ( type->IsBool() )
        {
            hr = FormatBool( objVal, outStr );
        }
        else if ( type->IsChar() )
        {
            hr = FormatChar( objVal, radix, outStr );
        }
        else if ( type->IsIntegral() )
        {
            hr = FormatInt( objVal, radix, outStr );
        }
        else if ( type->IsComplex() )
        {
            hr = FormatComplex( objVal, outStr );
        }
        else if ( type->IsReal() )
        {
            hr = FormatSimpleReal( objVal.Value.Float80Value, outStr );
        }
        else if ( type->IsImaginary() )
        {
            hr = FormatSimpleReal( objVal.Value.Float80Value, outStr );
            outStr.append( 1, L'i' );
        }
        else
            return E_FAIL;

        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    template <class T> T* tmemchr( T* buf, T c, size_t size );
    template <> char*    tmemchr( char* buf, char c, size_t size ) { return (char*) memchr( buf, c, size ); }
    template <> wchar_t* tmemchr( wchar_t* buf, wchar_t c, size_t size ) { return wmemchr( buf, c, size ); }
    template <> dchar_t* tmemchr( dchar_t* buf, dchar_t c, size_t size ) { return dmemchr( buf, c, size ); }

    // Returns the number of translated characters written, or the required 
    // number of characters for destCharBuf, if destCharLen is 0.

    template <class T> int Translate( 
        T* srcBuf, 
        size_t srcCharLen, 
        wchar_t* destBuf, 
        size_t destCharLen );

    template <> int Translate( 
        char* srcBuf, 
        size_t srcCharLen, 
        wchar_t* destBuf, 
        size_t destCharLen )
    {
        return MultiByteToWideChar( 
            CP_UTF8, 
            0,                      // ignore errors
            srcBuf,
            srcCharLen,
            destBuf,
            destCharLen );
    }

    template <> int Translate( 
        wchar_t* srcBuf, 
        size_t srcCharLen, 
        wchar_t* destBuf, 
        size_t destCharLen )
    {
        if ( destCharLen > 0 )
        {
            _ASSERT( destBuf != NULL );

            srcCharLen = std::min( srcCharLen, destCharLen );

            errno_t err = wmemcpy_s( destBuf, srcCharLen, srcBuf, srcCharLen );
            _ASSERT( err == 0 );
        }
        return srcCharLen;
    }

    template <> int Translate( 
        dchar_t* srcBuf, 
        size_t srcCharLen, 
        wchar_t* destBuf, 
        size_t destCharLen )
    {
        return Utf32To16( 
            true, 
            srcBuf,
            srcCharLen,
            destBuf,
            destCharLen );
    }

    template <class T>
    int Translate(
        BYTE* buf,
        size_t bufByteSize,
        wchar_t* transBuf,
        size_t transBufCharSize,
        bool& foundTerm
        )
    {
        // if we find a terminator, then translate up to that point, 
        // otherwise translate the whole buffer

        T*          end = tmemchr( (T*) buf, T( 0 ), bufByteSize / sizeof( T ) );
        uint32_t    unitsAvail = bufByteSize / sizeof( T );

        if ( end != NULL )
            unitsAvail = end - (T*) buf;

        int nChars = Translate( (T*) buf, unitsAvail, transBuf, transBufCharSize );

        foundTerm = (end != NULL);

        return nChars;
    }

    HRESULT FormatString( 
        IValueBinder* binder, 
        Address addr, 
        uint32_t unitSize, 
        bool maxLengthKnown,
        uint32_t maxLength,
        std::wstring& outStr, 
        bool& foundTerm )
    {
        const int   MaxBytes = 400;
        HRESULT     hr = S_OK;
        BYTE        buf[ MaxBytes ];
        wchar_t     translatedBuf[ MaxBytes / sizeof( wchar_t ) ] = { 0 };
        uint32_t    sizeToRead = _countof( buf );
        uint32_t    sizeRead = 0;
        int         nChars = 0;

        if ( maxLengthKnown )
        {
            sizeToRead = std::min<uint32_t>( sizeToRead, maxLength * unitSize );
        }

        hr = binder->ReadMemory( addr, sizeToRead, sizeRead, buf );
        if ( FAILED( hr ) )
            return hr;

        switch ( unitSize )
        {
        case 1:
            nChars = Translate<char>( buf, sizeRead, translatedBuf, _countof( translatedBuf ), foundTerm );
            break;

        case 2:
            nChars = Translate<wchar_t>( buf, sizeRead, translatedBuf, _countof( translatedBuf ), foundTerm );
            break;

        case 4:
            nChars = Translate<dchar_t>( buf, sizeRead, translatedBuf, _countof( translatedBuf ), foundTerm );
            break;

        default:
            return E_FAIL;
        }

        outStr.append( translatedBuf, nChars );

        return S_OK;
    }

    HRESULT FormatSArray( IValueBinder* binder, Address addr, Type* type, int radix, std::wstring& outStr )
    {
        UNREFERENCED_PARAMETER( radix );
        _ASSERT( type->IsSArray() );

        ITypeSArray*    arrayType = type->AsTypeSArray();

        if ( arrayType == NULL )
            return E_FAIL;

        if ( arrayType->GetElement()->IsChar() )
        {
            bool    foundTerm = true;

            outStr.append( L"\"" );

            FormatString( 
                binder, 
                addr, 
                arrayType->GetElement()->GetSize(),
                true,
                arrayType->GetLength(),
                outStr,
                foundTerm );

            outStr.append( 1, L'"' );
        }

        return S_OK;
    }

    HRESULT FormatDArray( IValueBinder* binder, DArray array, Type* type, int radix, std::wstring& outStr )
    {
        _ASSERT( type->IsDArray() );

        HRESULT         hr = S_OK;
        ITypeDArray*    arrayType = type->AsTypeDArray();

        if ( arrayType == NULL )
            return E_FAIL;

        outStr.append( L"{length=" );

        hr = FormatInt( array.Length, arrayType->GetLengthType(), radix, outStr );
        if ( FAILED( hr ) )
            return hr;

        outStr.append( L" ptr=" );

        hr = FormatAddress( array.Addr, arrayType->GetPointerType(), outStr );
        if ( FAILED( hr ) )
            return hr;

        if ( arrayType->GetElement()->IsChar() )
        {
            bool        foundTerm = true;
            uint32_t    len = MaxStringLen;

            // cap it somewhere under the range of a long
            // do it this way, otherwise only truncating could leave us with a tiny array
            // which would not be useful

            if ( array.Length < MaxStringLen )
                len = (uint32_t) array.Length;

            outStr.append( L" \"" );

            FormatString( 
                binder, 
                array.Addr, 
                arrayType->GetElement()->GetSize(),
                true,
                len,
                outStr,
                foundTerm );

            outStr.append( 1, L'"' );
        }

        outStr.append( 1, L'}' );

        return S_OK;
    }

    HRESULT FormatAArray( Address addr, Type* type, int radix, std::wstring& outStr )
    {
        _ASSERT( type->IsAArray() );
        UNREFERENCED_PARAMETER( radix );
        return FormatAddress( addr, type, outStr );
    }

    HRESULT FormatPointer( IValueBinder* binder, const DataObject& objVal, std::wstring& outStr )
    {
        _ASSERT( objVal._Type->IsPointer() );

        HRESULT hr = S_OK;
        RefPtr<Type>    pointed = objVal._Type->AsTypeNext()->GetNext();

        hr = FormatAddress( objVal.Value.Addr, objVal._Type, outStr );
        if ( FAILED( hr ) )
            return hr;

        if ( pointed->IsChar() )
        {
            bool    foundTerm = false;

            outStr.append( L" \"" );

            FormatString( binder, objVal.Value.Addr, pointed->GetSize(), false, 0, outStr, foundTerm );
            // don't worry about an error here, we still want to show the address

            if ( foundTerm )
                outStr.append( 1, L'"' );
        }

        return S_OK;
    }

    class HeapCloser
    {
    public:
        void operator()( uint8_t* p )
        {
            HeapFree( GetProcessHeap(), 0, p );
        }
    };

    typedef HandlePtrBase2<uint8_t*, NULL, HeapCloser> HeapPtr;

    //------------------------------------------------------------------------
    //  FormatRawStringInternal
    //
    //      Reads string data from a binder and converts it to UTF-16.
    //      Lengths refer to whole numbers of Unicode code units.
    //      A terminating character is not added to the output buffer or 
    //      included in the output length.
    //
    //      This function works by reading fixed size chunks of the original
    //      string. When it reaches the maximum known length, a terminator 
    //      character, memory that can't be read, or the end of the output
    //      buffer, then it stops reading and translating.
    //
    //  Parameters:
    //      binder - allows us to read original string data
    //      unitSize - original code unit size. 1, 2, or 4 (char, wchar, dchar)
    //      knownLength - the maximum length of the original string
    //      bufLen - the length of the buffer we'll translate to
    //      length - length written to the output buffer, or,
    //               if outBuf is NULL, the required length for outBuf
    //      outBuf - the buffer we'll write translated UTF-16 characters to
    //------------------------------------------------------------------------

    HRESULT FormatRawStringInternal( 
        IValueBinder* binder, 
        Address address, 
        uint32_t unitSize, 
        uint32_t knownLength,
        uint32_t bufLen,
        uint32_t& length,
        wchar_t* outBuf )
    {
        _ASSERT( (unitSize == 1) || (unitSize == 2) || (unitSize == 4) );
        _ASSERT( binder != NULL );

        HRESULT     hr = S_OK;
        HeapPtr     chunk( (uint8_t*) HeapAlloc( GetProcessHeap(), 0, RawStringChunkSize ) );
        bool        foundTerm = false;
        uint32_t    totalSizeToRead = (knownLength * unitSize);
        uint32_t    chunkCount = (totalSizeToRead + RawStringChunkSize - 1) / RawStringChunkSize;
        Address     addr = address;
        uint32_t    totalSizeLeftToRead = totalSizeToRead;
        uint32_t    transLen = 0;
        wchar_t*    curBufPtr = outBuf;
        uint32_t    bufLenLeft = 0;

        if ( chunk == NULL )
            return E_OUTOFMEMORY;

        if ( outBuf != NULL )
            bufLenLeft = bufLen;

        for ( uint32_t i = 0; i < chunkCount; i++ )
        {
            uint32_t    sizeToRead = totalSizeLeftToRead;
            uint32_t    sizeRead = 0;
            int         nChars = 0;

            if ( sizeToRead > RawStringChunkSize )
                sizeToRead = RawStringChunkSize;

            hr = binder->ReadMemory( addr, sizeToRead, sizeRead, chunk );
            if ( FAILED( hr ) )
                return hr;

            switch ( unitSize )
            {
            case 1:
                nChars = Translate<char>( chunk, sizeRead, curBufPtr, bufLenLeft, foundTerm );
                break;

            case 2:
                nChars = Translate<wchar_t>( chunk, sizeRead, curBufPtr, bufLenLeft, foundTerm );
                break;

            case 4:
                nChars = Translate<dchar_t>( chunk, sizeRead, curBufPtr, bufLenLeft, foundTerm );
                break;
            }

            transLen += nChars;

            // finish counting when there's a terminator, or we can't read any more memory
            if ( foundTerm || (sizeRead < sizeToRead) )
                break;

            if ( outBuf != NULL )
            {
                curBufPtr += nChars;
                bufLenLeft -= nChars;

                // one more condition for stopping: no more space to write in
                if ( bufLenLeft == 0 )
                    break;
            }

            addr += sizeRead;
            totalSizeLeftToRead -= sizeRead;
        }

        // when we get here we either found a terminator,
        // read to the known length and found no terminator (i >= chunkCount),
        // reached the end of contiguous readable memory,
        // or reached the end of the writable buffer
        // in any case, it's success, and tell the user how many wchars there are
        length = transLen;

        return S_OK;
    }

    HRESULT GetStringTypeData( 
        const DataObject& objVal, 
        Address& address,
        uint32_t& unitSize,
        uint32_t& knownLength )
    {
        if ( objVal._Type == NULL )
            return E_INVALIDARG;

        if ( objVal._Type->IsSArray() )
        {
            address = objVal.Addr;
            knownLength = objVal._Type->AsTypeSArray()->GetLength();
        }
        else if ( objVal._Type->IsDArray() )
        {
            dlength_t   bigLen = objVal.Value.Array.Length;

            if ( bigLen > MaxStringLen )
                knownLength = MaxStringLen;
            else
                knownLength = (uint32_t) bigLen;

            address = objVal.Value.Array.Addr;
        }
        else if ( objVal._Type->IsPointer() )
        {
            knownLength = MaxStringLen;
            address = objVal.Value.Addr;
        }
        else
            return E_FAIL;

        _ASSERT( objVal._Type->AsTypeNext() != NULL );
        unitSize = objVal._Type->AsTypeNext()->GetNext()->GetSize();

        if ( (unitSize != 1) && (unitSize != 2) && (unitSize != 4) )
            return E_FAIL;

        return S_OK;
    }

    HRESULT GetRawStringLength( IValueBinder* binder, const DataObject& objVal, uint32_t& length )
    {
        if ( binder == NULL )
            return E_INVALIDARG;

        HRESULT     hr = S_OK;
        Address     address = 0;
        uint32_t    unitSize = 0;
        uint32_t    knownLen = 0;

        hr = GetStringTypeData( objVal, address, unitSize, knownLen );
        if ( FAILED( hr ) )
            return hr;

        return FormatRawStringInternal( binder, address, unitSize, knownLen, 0, length, NULL );
    }

    HRESULT FormatRawString(
        IValueBinder* binder, 
        const DataObject& objVal, 
        uint32_t bufCharLen,
        uint32_t& bufCharLenWritten,
        wchar_t* buf )
    {
        if ( binder == NULL )
            return E_INVALIDARG;
        if ( buf == NULL )
            return E_INVALIDARG;

        HRESULT     hr = S_OK;
        Address     address = 0;
        uint32_t    unitSize = 0;
        uint32_t    knownLen = 0;

        hr = GetStringTypeData( objVal, address, unitSize, knownLen );
        if ( FAILED( hr ) )
            return hr;

        return FormatRawStringInternal( binder, address, unitSize, knownLen, bufCharLen, bufCharLenWritten, buf );
    }

    HRESULT FormatValue( IValueBinder* binder, const DataObject& objVal, int radix, std::wstring& outStr )
    {
        HRESULT hr = S_OK;
        Type*   type = NULL;

        type = objVal._Type;

        if ( type == NULL )
            return E_FAIL;

        if ( type->IsPointer() )
        {
            hr = FormatPointer( binder, objVal, outStr );
        }
        else if ( type->IsBasic() )
        {
            hr = FormatBasicValue( objVal, radix, outStr );
        }
        // TODO: enum: look up enum member that corresponds to integer value
        else if ( type->IsSArray() )
        {
            hr = FormatSArray( binder, objVal.Addr, objVal._Type, radix, outStr );
        }
        else if ( type->IsDArray() )
        {
            hr = FormatDArray( binder, objVal.Value.Array, objVal._Type, radix, outStr );
        }
        else if ( type->IsAArray() )
        {
            hr = FormatAArray( objVal.Value.Addr, objVal._Type, radix, outStr );
        }
        else
            hr = E_FAIL;

        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }
}
