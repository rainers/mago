/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EED.h"
#include "FormatValue.h"
#include "Properties.h"
#include "Type.h"
#include "UniAlpha.h"

#include <algorithm>
#include <CorError.h>

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


    HRESULT FormatSimpleReal( const Real10& val, int digits, std::wstring& outStr )
    {
        wchar_t buf[ Real10::Float80DecStrLen + 1 ] = L"";

        val.ToString( buf, _countof( buf ), digits + 2 ); // a more digit than what's exact

        outStr.append( buf );

        return S_OK;
    }

    HRESULT FormatComplex( const DataObject& objVal, std::wstring& outStr )
    {
        HRESULT hr = S_OK;
        int digits;
        if ( !PropertyFloatDigits::GetDigits(objVal._Type, digits) )
            return E_INVALIDARG;

        hr = FormatSimpleReal( objVal.Value.Complex80Value.RealPart, digits, outStr );
        if ( FAILED( hr ) )
            return hr;

        // Real10::ToString won't add a leading '+', but it's needed
        if ( objVal.Value.Complex80Value.ImaginaryPart.GetSign() >= 0 )
            outStr.append( 1, L'+' );

        hr = FormatSimpleReal( objVal.Value.Complex80Value.ImaginaryPart, digits, outStr );
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

    HRESULT FormatInt( uint64_t number, int width, bool sgnd, const FormatOptions& fmtopt, std::wstring& outStr )
    {
        // 18446744073709551616
        wchar_t buf[20 + 9 + 1] = L"";

        if ( fmtopt.radix == 16 )
        {
            if ( gRemoveLeadingHexZeroes )
            {
                swprintf_s( buf, L"0x%I64x", number );
            }
            else
            {
                if (width < 16)
                    number &= (1LL << 4 * width) - 1; // avoid sign extension beyond actual width
                swprintf_s( buf, L"0x%0*I64x", width, number );
            }
        }
        else    // it's 10, or make it 10
        {
            if ( sgnd )
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

    HRESULT FormatInt( const DataObject& objVal, const FormatOptions& fmtopt, std::wstring& outStr )
    {
        auto type = objVal._Type;
        return FormatInt( objVal.Value.UInt64Value, type->GetSize() * 2, type->IsSigned(), fmtopt, outStr);
    }

    HRESULT FormatAddress( Address addr, Type* type, std::wstring& outStr )
    {
        FormatOptions fmtopt (16);
        return FormatInt( addr, type->GetSize() * 2, type->IsSigned(), fmtopt, outStr );
    }

    HRESULT FormatChar( const DataObject& objVal, const FormatOptions& fmtopt, std::wstring& outStr )
    {
        // object replacement char U+FFFC
        // replacement character U+FFFD
        const wchar_t   ReplacementChar = L'\xFFFD';

        HRESULT hr = S_OK;

        hr = FormatInt( objVal, fmtopt, outStr );
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

    HRESULT FormatBasicValue( const DataObject& objVal, const FormatOptions& fmtopt, std::wstring& outStr )
    {
        _ASSERT( objVal._Type->IsBasic() );

        HRESULT hr = S_OK;
        Type*   type = NULL;
        int     digits;

        if ( objVal._Type == NULL )
            return E_FAIL;

        type = objVal._Type;

        if ( type->IsBool() )
        {
            hr = FormatBool( objVal, outStr );
        }
        else if ( type->IsChar() )
        {
            hr = FormatChar( objVal, fmtopt, outStr );
        }
        else if ( type->IsIntegral() )
        {
            hr = FormatInt( objVal, fmtopt, outStr );
        }
        else if ( type->IsComplex() )
        {
            hr = FormatComplex( objVal, outStr );
        }
        else if ( type->IsReal() && PropertyFloatDigits::GetDigits( type, digits ) )
        {
            hr = FormatSimpleReal( objVal.Value.Float80Value, digits, outStr );
        }
        else if ( type->IsImaginary() && PropertyFloatDigits::GetDigits( type, digits ) )
        {
            hr = FormatSimpleReal( objVal.Value.Float80Value, digits, outStr );
            outStr.append( 1, L'i' );
        }
        else if( type->Ty == Tvoid )
        {
            FormatInt( objVal.Value.UInt64Value, 2, false, fmtopt, outStr );
        }
        else
            return E_FAIL;

        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT FormatEnum( const DataObject& objVal, const FormatOptions& fmtopt, std::wstring& outStr )
    {
        _ASSERT( objVal._Type->AsTypeEnum() != NULL );

        HRESULT hr = S_OK;

        if ( (objVal._Type == NULL) || (objVal._Type->AsTypeEnum() == NULL) )
            return E_FAIL;

        ITypeEnum*  enumType = objVal._Type->AsTypeEnum();
        RefPtr<Declaration> decl;
        const wchar_t* name = NULL;

        decl = enumType->FindObjectByValue( objVal.Value.UInt64Value );

        if ( decl != NULL )
        {
            name = decl->GetName();
        }

        if ( name != NULL )
        {
            objVal._Type->ToString( outStr );
            outStr.append( 1, L'.' );
            outStr.append( name );
            outStr.append(1, L' ');
            outStr.append(1, L'(');
            hr = FormatInt(objVal, fmtopt, outStr);
            outStr.append(1, L')');
        }
        else
        {
            hr = FormatInt( objVal, fmtopt, outStr );
            if ( FAILED( hr ) )
                return hr;
        }

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
        size_t destCharLen,
        bool& truncated );

    template <> int Translate( 
        char* srcBuf, 
        size_t srcCharLen, 
        wchar_t* destBuf, 
        size_t destCharLen,
        bool& truncated )
    {
        truncated = false;
        if( destCharLen > 0 && srcCharLen > destCharLen )
        {
            srcCharLen = destCharLen; // MultiByteToWideChar returns 0 on overflow
            truncated = true;
        }

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
        size_t destCharLen,
        bool& truncated )
    {
        if ( destCharLen > 0 )
        {
            _ASSERT( destBuf != NULL );

            truncated = false;
            if( srcCharLen > destCharLen )
            {
                srcCharLen = destCharLen;
                truncated = true;
            }

            errno_t err = wmemcpy_s( destBuf, srcCharLen, srcBuf, srcCharLen );
            _ASSERT( err == 0 );
            UNREFERENCED_PARAMETER( err );
        }
        return srcCharLen;
    }

    template <> int Translate( 
        dchar_t* srcBuf, 
        size_t srcCharLen, 
        wchar_t* destBuf, 
        size_t destCharLen,
        bool& truncated )
    {
        return Utf32To16( 
            true, 
            srcBuf,
            srcCharLen,
            destBuf,
            destCharLen,
            truncated );
    }

    template <class T>
    int Translate(
        BYTE* buf,
        size_t bufByteSize,
        wchar_t* transBuf,
        size_t transBufCharSize,
        bool& truncated,
        bool& foundTerm
        )
    {
        // if we find a terminator, then translate up to that point, 
        // otherwise translate the whole buffer

        T*          end = tmemchr( (T*) buf, T( 0 ), bufByteSize / sizeof( T ) );
        uint32_t    unitsAvail = bufByteSize / sizeof( T );

        if ( end != NULL )
            unitsAvail = end - (T*) buf;

        int nChars = Translate( (T*) buf, unitsAvail, transBuf, transBufCharSize, truncated );

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
        bool& truncated, 
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
            nChars = Translate<char>( buf, sizeRead, translatedBuf, _countof( translatedBuf ), truncated, foundTerm );
            break;

        case 2:
            nChars = Translate<wchar_t>( buf, sizeRead, translatedBuf, _countof( translatedBuf ), truncated, foundTerm );
            break;

        case 4:
            nChars = Translate<dchar_t>( buf, sizeRead, translatedBuf, _countof( translatedBuf ), truncated, foundTerm );
            break;

        default:
            return E_FAIL;
        }

        outStr.append( translatedBuf, nChars );

        if( maxLengthKnown && sizeToRead < maxLength * unitSize )
            truncated = true;

        return S_OK;
    }

    void _formatString( IValueBinder* binder, Address addr, uint64_t slen, Type* elementType, std::wstring& outStr )
    {
        bool        foundTerm = true;
        bool        truncated = false;
        uint32_t    len = MaxStringLen;

        // cap it somewhere under the range of a long
        // do it this way, otherwise only truncating could leave us with a tiny array
        // which would not be useful

        if ( slen < MaxStringLen )
            len = (uint32_t) slen;

        outStr.append( L"\"" );

        FormatString( 
            binder, 
            addr, 
            elementType->GetSize(),
            true,
            len,
            outStr,
            truncated,
            foundTerm );

        outStr.append( 1, L'"' );

        ENUMTY ty = elementType->GetBackingTy();
        if ( ty == Tuns16 )
            outStr.append( 1, L'w' );
        else if ( ty == Tuns32 )
            outStr.append( 1, L'd' );

        if ( truncated )
            outStr.append( L"..." );
    }

    HRESULT _formatArray( IValueBinder* binder, Address addr, uint64_t length, MagoEE::Type* elemType,
                          FormatData& fmtdata, std::function<HRESULT(HRESULT, std::wstring)> complete )
    {
        TypeBasic* ubyteType = nullptr;
        if ( elemType->Ty == Tvoid )
        {
            elemType = ubyteType = new TypeBasic( Tuns8 );
            ubyteType->AddRef();
        }
        uint32_t elementSize = elemType->GetSize();
        HRESULT hres = S_OK;

        struct Closure
        {
            bool dotdotdot = false;
            size_t toComplete = 0;
            HRESULT hrCombined = S_OK;
            std::function<HRESULT(HRESULT, std::wstring)> complete;
            RefPtr<Type> ubyteType;
            std::vector<std::wstring> elemStr;
            std::wstring outStr;
            HRESULT done(HRESULT hr, size_t cnt)
            {
                hrCombine(hr);
                toComplete -= cnt;
                if (toComplete != 0)
                    return hr;

                outStr = L"[" + (elemStr.empty() ? std::wstring{} : elemStr[0]);
                for( size_t i = 1; i < elemStr.size(); i++ )
                    outStr.append( L", " ).append( elemStr[i] );
                if( dotdotdot )
                    outStr.append(L", ...");
                outStr.append(L"]");
                if (complete)
                    return complete(hrCombined, outStr);
                return hrCombined;
            }
            HRESULT hrCombine(HRESULT hr)
            {
                if (hrCombined != S_QUEUED && hrCombined != COR_E_OPERATIONCANCELED)
                    if (hr == S_QUEUED && hr == COR_E_OPERATIONCANCELED)
                        hrCombined = hr;
                return hrCombined;
            }
            size_t outLength() const
            {
                size_t sz = 2 + 2 * (elemStr.empty() ? 0 : elemStr.size() - 1);
                for (auto& s : elemStr)
                    sz += s.size();
                if (dotdotdot)
                    sz += 5;
                return sz;
            }
        };
        auto closure = std::make_shared<Closure>();

        if (length > 8)
        {
            closure->dotdotdot = true;
            length = 8;
        }

        closure->complete = complete;
        closure->toComplete = length + 1;     // support length == 0
        closure->ubyteType.Ref() = ubyteType; // take ownership
        closure->elemStr.resize(length);

        for (uint64_t i = 0; i < length; i++)
        {
            DataObject elementObj;
            elementObj._Type = elemType;
            elementObj.Addr = addr + elementSize * i;

            HRESULT hr = binder->GetValue(elementObj.Addr, elementObj._Type, elementObj.Value);
            if (FAILED(hr))
                return hr;

            auto completeElem = [i, closure](HRESULT hr, std::wstring elemStr)
            {
                closure->elemStr[i] = elemStr;
                return closure->done(hr, 1);
            };
            FormatData fmtdataElem = fmtdata.newScope(closure->outLength());
            hr = FormatValue( binder, elementObj, fmtdataElem,
                              complete ? completeElem : std::function<HRESULT(HRESULT, std::wstring)>{} );
            if (!complete || FAILED(hr))
            {
                closure->elemStr[i] = fmtdataElem.outStr;
                closure->done(hr, 1);
            }
            if (hr == COR_E_OPERATIONCANCELED)
            {
                closure->dotdotdot = true;
                closure->done(hr, length - i);
                break;
            }
        }
        closure->done(S_OK, 1);
        fmtdata.outStr.append(closure->outStr);
        return closure->toComplete > 0 ? S_QUEUED : closure->hrCombined;
    }

    HRESULT FormatSArray( IValueBinder* binder, Address addr, Type* type, FormatData& fmtdata,
                          std::function<HRESULT(HRESULT, std::wstring)> complete )
    {
        _ASSERT( type->IsSArray() );

        ITypeSArray*    arrayType = type->AsTypeSArray();

        if ( arrayType == NULL )
            return E_FAIL;

        if ( arrayType->GetElement()->IsChar() )
        {
            _formatString( binder, addr, arrayType->GetLength(), arrayType->GetElement(), fmtdata.outStr );
            if ( complete )
                return complete(S_OK, fmtdata.outStr);
            return S_OK;
        }
        else
        {
            uint32_t length = arrayType->GetLength();
            return _formatArray( binder, addr, length, arrayType->GetElement(), fmtdata, complete );
        }
    }

    HRESULT FormatDArray( IValueBinder* binder, DArray array, Type* type, FormatData& fmtdata,
                          std::function<HRESULT(HRESULT, std::wstring)> complete )
    {
        _ASSERT( type->IsDArray() );

        HRESULT         hr = S_OK;
        ITypeDArray*    arrayType = type->AsTypeDArray();

        if ( arrayType == NULL )
            return E_FAIL;

        if ( fmtdata.opt.specifier != FormatSpecRaw && arrayType->GetElement()->IsChar() )
        {
            _formatString( binder, array.Addr, array.Length, arrayType->GetElement(), fmtdata.outStr );
            if ( complete )
                hr = complete( hr, fmtdata.outStr );
        }
        else
        {
            bool showLength = fmtdata.opt.specifier == FormatSpecRaw || !gShowDArrayLengthInType;
            bool showPtr    = fmtdata.opt.specifier == FormatSpecRaw || !gHideReferencePointers;

            if( showLength || showPtr )
                fmtdata.outStr.append( 1, L'{' );

            if( showLength )
            {
                fmtdata.outStr.append( L"length=" );
                auto type = arrayType->GetLengthType();
                hr = FormatInt( array.Length, type->GetSize() * 2, type->IsSigned(), fmtdata.opt, fmtdata.outStr );
                if ( FAILED( hr ) )
                    return hr;
            }

            if ( showLength && showPtr )
                fmtdata.outStr.append( 1, L' ' );

            if ( showPtr )
            {
                fmtdata.outStr.append(L"ptr=");

                hr = FormatAddress( array.Addr, arrayType->GetPointerType(), fmtdata.outStr );
                if ( FAILED( hr ) )
                    return hr;
            }

            if ( showLength || showPtr )
                fmtdata.outStr.append( 1, L'}' );

            if (fmtdata.opt.specifier != FormatSpecRaw )
            {
                if ( showLength || showPtr )
                    fmtdata.outStr.append( 1, L' ' );

                hr = _formatArray( binder, array.Addr, array.Length, arrayType->GetElement(), fmtdata, complete );
            }
            else
            {
                if ( complete )
                    hr = complete( hr, fmtdata.outStr );
            }
        }

        return hr;
    }

    HRESULT FormatAArray( Address addr, Type* type, FormatData& fmtdata )
    {
        _ASSERT( type->IsAArray() );
        return FormatAddress( addr, type, fmtdata.outStr );
    }

    HRESULT FormatTuple( IValueBinder* binder, const DataObject& objVal, ITypeTuple* type, FormatData& fmtdata,
                         std::function<HRESULT(HRESULT, std::wstring)> complete )
    {
        if ( type == NULL || type->IsAmbiguousGlobals() )
            return E_FAIL;

        struct Closure
        {
            bool dotdotdot = false;
            size_t toComplete = 0;
            HRESULT hrCombined = S_OK;
            std::function<HRESULT(HRESULT, std::wstring)> complete;
            std::vector<std::wstring> elemStr;
            std::wstring outStr;

            HRESULT done(HRESULT hr, size_t cnt)
            {
                hrCombine(hr);
                toComplete -= cnt;
                if (toComplete != 0)
                    return hr;

                outStr = L"(";
                for (size_t i = 0; i < elemStr.size(); i++)
                    outStr.append(i == 0 ? L"" : L", ").append(elemStr[i]);
                if (dotdotdot)
                    outStr.append(L", ...");
                outStr.append(L")");
                if (complete)
                    return complete(hrCombined, outStr);
                return hr;
            }
            HRESULT hrCombine(HRESULT hr)
            {
                if (hrCombined != S_QUEUED && hrCombined != COR_E_OPERATIONCANCELED)
                    if (hr == S_QUEUED && hr == COR_E_OPERATIONCANCELED)
                        hrCombined = hr;
                return hrCombined;
            }
            size_t outLength() const
            {
                size_t sz = 2 + 2 * (elemStr.empty() ? 0 : elemStr.size() - 1);
                for (auto& s : elemStr)
                    sz += s.size();
                if (dotdotdot)
                    sz += 5;
                return sz;
            }
        };
        auto closure = std::make_shared<Closure>();
        closure->complete = complete;
        closure->toComplete = 2; // keep value higher so we can add iteratively

        HRESULT hr = S_OK;
        auto length = type->GetLength();
        for ( uint32_t i = 0; i < length; i++ )
        {
            FormatData fmtdataElem = fmtdata.newScope(closure->outLength());
            if (fmtdataElem.isTooLong())
            {
                closure->dotdotdot = true;
                break;
            }

            Declaration* decl = type->GetElementDecl( i );
            Address addr;
            int offset = 0;
            if ( decl->IsField() )
            {
                addr = objVal._Type->IsReference() ? objVal.Value.Addr : objVal.Addr;
                if (addr == 0)
                    hr = E_MAGOEE_NO_ADDRESS;
                else if ( !decl->GetOffset( offset ) )
                    hr = E_FAIL;
            }
            else if ( !decl->GetAddress( addr ) || addr == 0 )
                hr = E_MAGOEE_NO_ADDRESS;

            DataObject elementObj;
            if ( !FAILED( hr ) )
            {
                elementObj._Type = type->GetElementType( i );
                elementObj.Addr = addr + offset;

                hr = binder->GetValue( elementObj.Addr, elementObj._Type, elementObj.Value );
            }

            closure->toComplete++;
            closure->elemStr.push_back({});
            auto idx = closure->elemStr.size() - 1;
            auto completeElem = [idx, closure](HRESULT hr, std::wstring elemStr)
                {
                    closure->elemStr[idx] = elemStr;
                    return closure->done(hr, 1);
                };

            if ( !FAILED( hr ) )
                hr = FormatValue( binder, elementObj, fmtdataElem,
                                  complete ? completeElem : std::function<HRESULT(HRESULT, std::wstring)>{});
            if ( !complete || FAILED( hr ) )
            {
                closure->elemStr.back() = fmtdataElem.outStr;
                closure->done(hr, 1);
            }
            if ( hr == COR_E_OPERATIONCANCELED )
                break;
        }
        closure->done(S_OK, 2); // call complete if none queued
        hr = closure->toComplete > 0 ? S_QUEUED : closure->hrCombined;
        fmtdata.outStr.append(closure->outStr);
        return hr;
    }

    HRESULT _FormatStruct( IValueBinder* binder, Address addr, const char* srcBuf, Type* type,
                           FormatData& fmtdata, std::function<HRESULT(HRESULT, std::wstring)> complete )
    {
        HRESULT         hr = S_OK;

        _ASSERT( type->AsTypeStruct() );
        RefPtr<Declaration> decl = type->GetDeclaration();
        if ( decl == NULL )
            return E_INVALIDARG;

        RefPtr<IEnumDeclarationMembers> members;
        if ( !decl->EnumMembers( members.Ref() ) )
            return E_INVALIDARG;

        struct Closure
        {
            bool dotdotdot = false;
            size_t toComplete = 0;
            HRESULT hrCombined = S_OK;
            std::function<HRESULT(HRESULT, std::wstring)> complete;
            std::vector<std::wstring> names;
            std::vector<std::wstring> elemStr;
            std::wstring outStr;

            HRESULT done(HRESULT hr, size_t cnt)
            {
                hrCombine(hr);
                toComplete -= cnt;
                if (toComplete != 0)
                    return hr;

                outStr = L"{";
                for (size_t i = 0; i < elemStr.size(); i++)
                    outStr.append(i == 0 ? L"" : L", ").append(names[i]).append(L" = ").append(elemStr[i]);
                if (dotdotdot)
                    outStr.append(L", ...");
                outStr.append(L"}");
                if (complete)
                    return complete(hrCombined, outStr);
                return hr;
            }
            HRESULT hrCombine(HRESULT hr)
            {
                if (hrCombined != S_QUEUED && hrCombined != COR_E_OPERATIONCANCELED)
                    if (hr == S_QUEUED && hr == COR_E_OPERATIONCANCELED)
                        hrCombined = hr;
                return hrCombined;
            }
            size_t outLength() const
            {
                size_t sz = 2 + 2 * (elemStr.empty() ? 0 : elemStr.size() - 1);
                for (auto& n : names)
                    sz += n.size() + 3;
                for (auto& s : elemStr)
                    sz += s.size();
                if (dotdotdot)
                    sz += 5;
                return sz;
            }
        };
        auto closure = std::make_shared<Closure>();
        closure->complete = complete;
        closure->toComplete = 2; // keep value higher so we can add iteratively

        RefPtr<Declaration> next;
        for ( int i = 0; ; ++i )
        {
            RefPtr<Declaration> member;
            if ( next )
                member.Ref() = next.Detach(); // move
            else if ( !members->Next( member.Ref() ) )
                break;
            if ( member->IsBaseClass() || member->IsStaticField() )
                continue;

            if ( closure->names.size() >= 8 )
            {
                closure->dotdotdot = true;
                break;
            }
            closure->names.push_back(member->GetName());
            closure->elemStr.push_back({});

            if ( gRecombineTuples )
            {
                std::wstring tplname;
                const wchar_t* tname = member->GetName();
                int tplidx = GetTupleName( tname, &tplname );
                if ( tplidx > 0 )
                    continue;
                if ( tplidx == 0 )
                {
                    std::vector<RefPtr<Declaration>> fields;
                    fields.push_back( member );
                    member.Release();
                    while ( members->Next( member.Ref() ) )
                    {
                        tname = member->GetName();
                        tplidx = GetTupleName( tname );
                        if ( tplidx > 0 )
                            fields.push_back( member );
                        else
                            next = member;
                        member.Release();
                        if ( tplidx <= 0 )
                            break;
                    }
                    hr = binder->NewTuple( tplname.data(), fields, member.Ref() );
                    if ( FAILED( hr ) )
                    {
                        closure->hrCombine(hr);
                        break;
                    }
                }
            }
            FormatData fmtdataElem = fmtdata.newScope( closure->outLength() );
            if ( fmtdataElem.isTooLong() )
            {
                closure->dotdotdot = true;
                break;
            }

            DataObject memberObj;
            member->GetType( memberObj._Type.Ref() );
            memberObj.Addr = addr;
            int offset;
            if ( member->GetOffset( offset ) )
                memberObj.Addr += offset;

            auto idx = closure->elemStr.size() - 1;
            auto completeElem = [idx, closure]( HRESULT hr, std::wstring elemStr )
            {
                closure->elemStr[idx] = elemStr;
                return closure->done(hr, 1);
            };

            closure->toComplete++;
            std::wstring memberStr;
            if ( srcBuf && memberObj._Type->AsTypeStruct() )
            {
                hr = _FormatStruct( binder, 0, srcBuf + memberObj.Addr, memberObj._Type, fmtdataElem,
                                    complete ? completeElem : std::function<HRESULT(HRESULT, std::wstring)>{});
            }
            else
            {
                if( srcBuf )
                    hr = FromRawValue( srcBuf + memberObj.Addr, memberObj._Type, memberObj.Value );
                else
                    hr = binder->GetValue( memberObj.Addr, memberObj._Type, memberObj.Value );

                if ( !FAILED( hr ) )
                    hr = FormatValue( binder, memberObj, fmtdataElem,
                                      complete ? completeElem : std::function<HRESULT(HRESULT, std::wstring)>{});
            }
            if (!complete || FAILED(hr))
            {
                closure->elemStr.back() = fmtdataElem.outStr;
                closure->done(hr, 1);
            }
            if (hr == COR_E_OPERATIONCANCELED)
                break;
        }
        closure->done(S_OK, 2); // call complete if none queued
        hr = closure->toComplete > 0 ? S_QUEUED : closure->hrCombined;
        fmtdata.outStr.append(closure->outStr);
        return hr;
    }

    HRESULT FormatRange( IValueBinder* binder, Address addr, Type* type, FormatData& fmtdata,
                         std::function<HRESULT(HRESULT, std::wstring)> complete )
    {
        auto ts = type->AsTypeStruct();
        Address addrSave = 0, addrEmpty = 0, addrFront = 0, addrPop = 0;
        RefPtr<Type> typeSave  = GetDebuggerProp( binder, ts, L"save", addrSave );
        RefPtr<Type> typeEmpty = typeSave  ? GetDebuggerProp( binder, ts, L"empty", addrEmpty ) : nullptr;
        RefPtr<Type> typeFront = typeEmpty ? GetDebuggerProp( binder, ts, L"front", addrFront ) : nullptr;
        RefPtr<Type> typePop   = typeFront ? GetDebuggerProp( binder, ts, L"popFront", addrPop ) : nullptr;
        if ( !typePop )
            return E_MAGOEE_NOFUNCCALL;

        DataObject obj = { 0 };
        obj._Type = GetDebuggerPropType( typeSave );
        if ( !obj._Type->Equals( type ) )
            return E_MAGOEE_NOFUNCCALL;

        HRESULT hr = EvalDebuggerProp( binder, typeSave, addrSave, addr, obj, {} );
        if ( hr != S_OK )
            return hr;
        if ( obj.Addr == 0 )
            return E_MAGOEE_NO_ADDRESS;

        Address addrLength = 0;
        RefPtr<Type> typeLength = GetDebuggerProp( binder, ts, L"length", addrLength );
        if( typeLength )
        {
            DataObject length = { 0 };
            hr = EvalDebuggerProp( binder, typeLength, addrLength, obj.Addr, length, {} );
            if( hr != S_OK )
                return hr;

            fmtdata.outStr.append(L"length=");

            hr = FormatValue( binder, length, fmtdata, complete );
            if ( hr != S_OK )
                return hr;

            fmtdata.outStr.append(L" ");
        }

        fmtdata.outStr.append( L"{" );
        auto initLength = fmtdata.outStr.length();

        for ( uint32_t i = 0; ; i++ )
        {
            DataObject empty = { 0 };
            hr = EvalDebuggerProp( binder, typeEmpty, addrEmpty, obj.Addr, empty, {} );
            if ( hr != S_OK )
                return hr;
            if ( empty.Value.Int64Value != 0 ) // check type?
                break;

            if (fmtdata.outStr.length() > initLength )
                fmtdata.outStr.append( L", ");

            if (fmtdata.outStr.length () > fmtdata.maxLength )
            {
                fmtdata.outStr.append( L"..." );
                break;
            }

            DataObject elem = { 0 };
            hr = EvalDebuggerProp( binder, typeFront, addrFront, obj.Addr, elem, {} );
            if ( hr != S_OK )
                return hr;

            hr = FormatValue( binder, elem, fmtdata, complete );
            if ( hr != S_OK )
                return hr;

            DataObject dummy = { 0 };
            hr = EvalDebuggerProp( binder, typePop, addrPop, obj.Addr, dummy, {} );
            if ( hr != S_OK )
                return hr;
        }

        fmtdata.outStr.append( L"}" );
        return S_OK;
    }

	HRESULT FormatStruct( IValueBinder* binder, Address addr, Type* type, FormatData& fmtdata,
                          std::function<HRESULT(HRESULT, std::wstring)> complete )
	{
        static bool recurse = false;
        if ( gCallDebuggerFunctions && !recurse && fmtdata.opt.specifier != FormatSpecRaw )
        {
            recurse = true;
            HRESULT hr = E_FAIL;
            Address fnaddr;
            RefPtr<Type> fntype = GetDebuggerProp( binder, type->AsTypeStruct(), L"__debugOverview", fnaddr );
            if( fntype )
            {
                FormatData fmtdataProp = fmtdata.newScope();
                auto completeProp =
                    [binder, fmtdataProp, complete](HRESULT hr, DataObject obj) mutable
                    {
                        if (hr == S_OK)
                            hr = FormatValue(binder, obj, fmtdataProp, complete);
                        else
                            hr = complete(hr, L"<error>");
                        return hr;
                    };
                DataObject obj = { 0 };
                if (complete)
                    hr = EvalDebuggerProp( binder, fntype, fnaddr, addr, obj, completeProp );
                else
                {
                    hr = EvalDebuggerProp( binder, fntype, fnaddr, addr, obj, {} );
                    if( hr == S_OK )
                        hr = FormatValue( binder, obj, fmtdata, {} );
                }
            }
            else if ( gCallDebuggerRanges )
                hr = FormatRange( binder, addr, type, fmtdata, complete );

            recurse = false;
            if ( SUCCEEDED( hr ) || hr == COR_E_OPERATIONCANCELED )
                return hr;
        }
		return _FormatStruct( binder, addr, nullptr, type, fmtdata, complete );
	}

	HRESULT FormatRawStructValue( IValueBinder* binder, const void* srcBuf, Type* type, FormatData& fmtdata )
	{
        return _FormatStruct(binder, 0, (const char*)srcBuf, type, fmtdata, {});
	}

    HRESULT FormatPointer( IValueBinder* binder, const DataObject& objVal, FormatData& fmtdata,
                           std::function<HRESULT(HRESULT, std::wstring)> complete )
    {
        _ASSERT( objVal._Type->IsPointer() );

        HRESULT hr = S_OK;
        RefPtr<Type>    pointeeType = objVal._Type->AsTypeNext()->GetNext();

        if (fmtdata.opt.specifier == FormatSpecRaw || !gHideReferencePointers || !objVal._Type->IsReference() )
        {
            hr = FormatAddress( objVal.Value.Addr, objVal._Type, fmtdata.outStr );
            if ( FAILED( hr ) )
                return hr;
            fmtdata.outStr.append(L" ");
        }

        if ( pointeeType->IsChar() )
        {
            bool    foundTerm = false;
            bool    truncated = false;
            fmtdata.outStr.append( L"\"" );

            FormatString( binder, objVal.Value.Addr, pointeeType->GetSize(), false, 0, fmtdata.outStr, truncated, foundTerm );
            // don't worry about an error here, we still want to show the address

            if ( foundTerm )
                fmtdata.outStr.append( 1, L'"' );
            if ( truncated )
                fmtdata.outStr.append( L"..." );
        }
        else
        {
            std::wstring symName;
            hr = binder->SymbolFromAddr( objVal.Value.Addr, symName, nullptr );
            if ( hr == S_OK )
            {
                fmtdata.outStr.append( L"{" );
                fmtdata.outStr.append( symName );
                fmtdata.outStr.append( L"} " );
            }
            else
                hr = S_OK;

            if ( objVal.Value.Addr == NULL )
            {
                fmtdata.outStr.append(L" null");
            }
            else if ( fmtdata.outStr.length() < fmtdata.maxLength )
            {
                DataObject pointeeObj = { 0 };
                pointeeObj._Type = pointeeType;
                pointeeObj.Addr = objVal.Value.Addr;

                hr = binder->GetValue( pointeeObj.Addr, pointeeObj._Type, pointeeObj.Value );
                if ( !FAILED( hr ) )
                {
                    auto completeFmt = [complete, &fmtdata, outStr = fmtdata.outStr](HRESULT hr, std::wstring memberStr) mutable
                    {
                        if ( SUCCEEDED( hr ) && !memberStr.empty() )
                        {
                            if( memberStr[0] != '{' )
                                outStr.append( L"{" );
                            outStr.append( memberStr );
                            if( memberStr[0] != '{' )
                                outStr.append( L"}" );
                        }
                        if ( complete )
                            return complete( hr, outStr );
                        fmtdata.outStr = outStr;
                        return hr;
                    };
                    FormatData fmtdataPointee = fmtdata.newScope();
                    hr = FormatValue( binder, pointeeObj, fmtdataPointee,
                                      complete ? completeFmt : std::function<HRESULT(HRESULT hr, std::wstring memberStr)>{});
                    if ( !complete )
                        hr = completeFmt(hr, fmtdataPointee.outStr);
                    else
                        complete = {};
                }
            }
        }
        if( hr == S_OK && complete )
            hr = complete( hr, fmtdata.outStr );
        return hr;
    }

    HRESULT FormatDelegate( IValueBinder* binder, const DataObject& objVal, FormatData& fmtdata )
    {
        _ASSERT( objVal._Type->IsDelegate() );

        HRESULT hr = S_OK;
        RefPtr<Type>    funcType = objVal._Type->AsTypeNext()->GetNext();

        fmtdata.outStr.append(L"{ptr=");

        hr = FormatAddress( objVal.Value.Delegate.ContextAddr, funcType, fmtdata.outStr ); // any pointer type
        if ( FAILED( hr ) )
            return hr;

        fmtdata.outStr.append( L" funcptr=" );

        hr = FormatAddress( objVal.Value.Delegate.FuncAddr, funcType, fmtdata.outStr );
        if ( FAILED( hr ) )
            return hr;


        std::wstring symName;
        if( binder->SymbolFromAddr( objVal.Value.Delegate.FuncAddr, symName, nullptr ) == S_OK )
        {
            fmtdata.outStr.append( L" {" );
            fmtdata.outStr.append( symName );
            fmtdata.outStr.append( L"}" );
        }

        fmtdata.outStr.append( 1, L'}' );
        return hr;
    }

    struct HeapDeleter
    {
    public:
        static void Delete( uint8_t* p )
        {
            HeapFree( GetProcessHeap(), 0, p );
        }
    };

    typedef UniquePtrBase<uint8_t*, NULL, HeapDeleter> HeapPtr;

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
        bool        truncated = false;
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
                nChars = Translate<char>( chunk, sizeRead, curBufPtr, bufLenLeft, truncated, foundTerm );
                break;

            case 2:
                nChars = Translate<wchar_t>( chunk, sizeRead, curBufPtr, bufLenLeft, truncated, foundTerm );
                break;

            case 4:
                nChars = Translate<dchar_t>( chunk, sizeRead, curBufPtr, bufLenLeft, truncated, foundTerm );
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

    HRESULT FormatTextViewerString( IValueBinder* binder, const DataObject& objVal, std::wstring& text )
    {
        if ( objVal._Type == NULL )
            return E_INVALIDARG;

        HRESULT hr;
        DataObject dbgObj = { 0 };
        const DataObject* pVal = &objVal;
        if ( pVal->_Type->IsReference() )
        {
            dbgObj._Type = pVal->_Type->AsTypeNext()->GetNext();
            dbgObj.Addr = pVal->Value.Addr;
            if( dbgObj.Addr != 0 )
            hr = binder->GetValue(dbgObj.Addr, dbgObj._Type, dbgObj.Value);
            pVal = &dbgObj;
        }
        if ( auto ts = pVal->_Type->AsTypeStruct() )
        {
            if ( !gCallDebuggerFunctions )
                return E_INVALIDARG;
            Address fnaddr;
            RefPtr<Type> fntype = GetDebuggerProp( binder, ts, L"__debugStringView", fnaddr );
            if ( !fntype )
                return E_INVALIDARG;

            hr = EvalDebuggerProp( binder, fntype, fnaddr, pVal->Addr, dbgObj, {} );
            if ( FAILED( hr ) )
                return hr;
            pVal = &dbgObj;
        }
        uint32_t len, fetched;
        hr = MagoEE::GetRawStringLength( binder, *pVal, len );
        if ( FAILED( hr ) )
            return hr;
        text.resize( len + 1 );
        hr = MagoEE::FormatRawString( binder, *pVal, len + 1, fetched, (wchar_t*)text.data() );
        if ( FAILED( hr ) )
            return hr;
        if ( fetched <= len )
            text.resize( fetched );
        return S_OK;
    }

    HRESULT FormatValue( IValueBinder* binder, const DataObject& objVal, FormatData& fmtdata,
                         std::function<HRESULT(HRESULT, std::wstring)> complete )
    {
        HRESULT hr = S_OK;
        Type*   type = NULL;

        type = objVal._Type;

        if ( type == NULL )
            return E_FAIL;

        if ( fmtdata.isTooLong() )
        {
            fmtdata.outStr.append( L"..." );
            if ( hr == S_OK && complete )
                hr = complete( hr, fmtdata.outStr );
        }
        else if ( type->IsPointer() )
        {
            hr = FormatPointer( binder, objVal, fmtdata, complete );
        }
        else if ( type->IsBasic() )
        {
            hr = FormatBasicValue( objVal, fmtdata.opt, fmtdata.outStr );
            if ( hr == S_OK && complete )
                hr = complete( hr, fmtdata.outStr );
        }
        else if ( type->AsTypeEnum() != NULL )
        {
            hr = FormatEnum( objVal, fmtdata.opt, fmtdata.outStr );
            if ( hr == S_OK && complete )
                hr = complete( hr, fmtdata.outStr );
        }
        else if ( type->IsSArray() )
        {
            hr = FormatSArray( binder, objVal.Addr, objVal._Type, fmtdata, complete );
        }
        else if ( type->IsDArray() )
        {
            hr = FormatDArray( binder, objVal.Value.Array, objVal._Type, fmtdata, complete );
        }
        else if ( type->IsAArray() )
        {
            hr = FormatAArray( objVal.Value.Addr, objVal._Type, fmtdata );
            if( hr == S_OK && complete )
                hr = complete( hr, fmtdata.outStr );
        }
        else if ( auto tt = type->AsTypeTuple() )
        {
            hr = FormatTuple( binder, objVal, tt, fmtdata, complete );
        }
        else if ( type->AsTypeStruct() )
        {
            hr = FormatStruct( binder, objVal.Addr, type, fmtdata, complete );
        }
        else if ( type->IsDelegate() )
        {
            hr = FormatDelegate( binder, objVal, fmtdata );
            if( hr == S_OK && complete )
                hr = complete( hr, fmtdata.outStr );
        }
        else
            hr = E_FAIL;

        return hr;
    }
}
