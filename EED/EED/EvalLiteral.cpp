/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Expression.h"
#include "Declaration.h"
#include "Eval.h"
#include "Type.h"
#include "ITypeEnv.h"
#include "NameTable.h"
#include "UniAlpha.h"


namespace MagoEE
{
    //----------------------------------------------------------------------------
    //  IntExpr
    //----------------------------------------------------------------------------

    HRESULT IntExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( binder );

        Kind = DataKind_Value;
        _ASSERT( _Type.Get() != NULL );
        return S_OK;
    }

    HRESULT IntExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        obj._Type = _Type;
        obj.Addr = 0;
        obj.Value.UInt64Value = Value;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  RealExpr
    //----------------------------------------------------------------------------

    HRESULT RealExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( typeEnv );
        UNREFERENCED_PARAMETER( binder );

        Kind = DataKind_Value;
        _ASSERT( _Type.Get() != NULL );
        return S_OK;
    }

    HRESULT RealExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        obj._Type = _Type;
        obj.Addr = 0;
        obj.Value.Float80Value = Value;
        return S_OK;
    }


    //----------------------------------------------------------------------------
    //  NullExpr
    //----------------------------------------------------------------------------

    HRESULT NullExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        _Type = typeEnv->GetVoidPointerType();
        Kind = DataKind_Value;
        return S_OK;
    }

    HRESULT NullExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        if ( _Type->IsPointer() )
        {
            obj.Value.Addr = 0;
        }
        else if ( _Type->IsDArray() )
        {
            obj.Value.Array.Addr = 0;
            obj.Value.Array.Length = 0;
        }
        else if ( _Type->IsAArray() )
        {
            obj.Value.Addr = 0;
        }
        else if ( _Type->IsDelegate() )
        {
            obj.Value.Delegate.ContextAddr = 0;
            obj.Value.Delegate.FuncAddr = 0;
        }
        else
            return E_FAIL;

        obj._Type = _Type;
        obj.Addr = 0;
        return S_OK;
    }

    bool NullExpr::TrySetType( Type* type )
    {
        if ( type->IsDArray() || type->IsAArray() || type->IsDelegate() )
        {
            _Type = type;
            return true;
        }

        return false;
    }


    //----------------------------------------------------------------------------
    //  StringExpr
    //----------------------------------------------------------------------------

    HRESULT MakeTypeForString( const String* string, ITypeEnv* typeEnv, Type*& type )
    {
        HRESULT hr = S_OK;
        ENUMTY  charTy = Tnone;
        Type*   charType = NULL;
        RefPtr<Type>    immutableCharType;

        switch ( string->Kind )
        {
        case StringKind_Byte:   charTy = Tchar; break;
        case StringKind_Utf16:  charTy = Twchar; break;
        case StringKind_Utf32:  charTy = Tdchar; break;
        default:
            return E_UNEXPECTED;
        }

        charType = typeEnv->GetType( charTy );

        immutableCharType = charType->MakeInvariant();
        if ( immutableCharType == NULL )
            return E_OUTOFMEMORY;

        hr = typeEnv->NewDArray( immutableCharType, type );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT StringExpr::Semantic( const EvalData& evalData, ITypeEnv* typeEnv, IValueBinder* binder )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        HRESULT hr = S_OK;

        _Type = NULL;

        Kind = DataKind_Value;

        if ( !IsSpecificType )
            Value = mUntypedStr;

        hr = MakeTypeForString( Value, typeEnv, _Type.Ref() );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT StringExpr::Evaluate( EvalMode mode, const EvalData& evalData, IValueBinder* binder, DataObject& obj )
    {
        UNREFERENCED_PARAMETER( evalData );
        UNREFERENCED_PARAMETER( binder );

        if ( mode == EvalMode_Address )
            return E_MAGOEE_NO_ADDRESS;

        obj._Type = _Type;
        obj.Addr = 0;
        obj.Value.Array.Addr = 0;
        obj.Value.Array.Length = Value->Length;
        obj.Value.Array.LiteralString = Value;
        return S_OK;
    }

    bool StringExpr::TrySetType( Type* type )
    {
        _ASSERT( type != NULL );

        // can only convert to other string types
        if ( !type->IsDArray() || !type->AsTypeDArray()->GetElement()->IsChar() )
            return false;

        Type* newCharType = type->AsTypeDArray()->GetElement();

        HRESULT     hr = S_OK;
        StringKind  newKind;

        switch ( newCharType->GetSize() )
        {
        case 1: newKind = StringKind_Byte;  break;
        case 2: newKind = StringKind_Utf16;  break;
        case 4: newKind = StringKind_Utf32;  break;
        default:    _ASSERT( false );   return false;
        }

        if ( IsSpecificType )
        {
            if ( Value->Kind != newKind )
                return false;
        }
        else if ( newCharType->GetSize() == 1 )
        {
            // no need to convert: untyped strings are stored as StringKind_Byte
            Value = mUntypedStr;
        }
        else
        {
            if ( mAlternates.get() == NULL )
            {
                mAlternates.reset( new AlternateStrings() );
                if ( mAlternates.get() == NULL )
                    return false;
            }

            if ( newCharType->GetSize() == 2 )
            {
                if ( mAlternates->GetUtf16String() == NULL )
                {
                    hr = mAlternates->SetUtf16String( mUntypedStr->Str, mUntypedStr->Length );
                    if ( FAILED( hr ) )
                        return false;
                }
                Value = mAlternates->GetUtf16String();
            }
            else if ( newCharType->GetSize() == 4 )
            {
                if ( mAlternates->GetUtf32String() == NULL )
                {
                    hr = mAlternates->SetUtf32String( mUntypedStr->Str, mUntypedStr->Length );
                    if ( FAILED( hr ) )
                        return false;
                }
                Value = mAlternates->GetUtf32String();
            }
        }

        _Type = type;

        return true;
    }


    StringExpr::AlternateStrings::AlternateStrings()
    {
        mNewUtf16Str.Kind = StringKind_Utf16;
        mNewUtf16Str.Length = 0;
        mNewUtf16Str.Str = NULL;

        mNewUtf32Str.Kind = StringKind_Utf32;
        mNewUtf32Str.Length = 0;
        mNewUtf32Str.Str = NULL;
    }

    Utf16String* StringExpr::AlternateStrings::GetUtf16String()
    {
        if ( mNewUtf16Str.Str != NULL )
            return &mNewUtf16Str;

        return NULL;
    }

    Utf32String* StringExpr::AlternateStrings::GetUtf32String()
    {
        if ( mNewUtf32Str.Str != NULL )
            return &mNewUtf32Str;

        return NULL;
    }

    template <class TChar, class TString>
    HRESULT SetString( 
        const char* utf8Str, 
        int utf8Length, 
        int (*Utf8ToX)( const char* utf8Str, int utf8Len, TChar* utfXStr, int utfXLen ), 
        StringKind kind, 
        TString& newUtfStr, 
        UniquePtr<TChar[]>& strBuf )
    {
        _ASSERT( utf8Str != NULL );

        int         len = 0;
        int         len2 = 0;
        TChar*      utfXStr = NULL;

        len = Utf8ToX( utf8Str, utf8Length, NULL, 0 );
        if ( (len == 0) && (GetLastError() == ERROR_NO_UNICODE_TRANSLATION) )
            return HRESULT_FROM_WIN32( ERROR_NO_UNICODE_TRANSLATION );
        _ASSERT( len > 0 );

        // len includes trailing '\0'
        utfXStr = new TChar[ len ];
        if ( utfXStr == NULL )
            return E_OUTOFMEMORY;

        len2 = Utf8ToX( utf8Str, utf8Length, utfXStr, len );
        _ASSERT( (len2 > 0) && (len2 == len) );

        strBuf.Attach( utfXStr );

        newUtfStr.Kind = kind;
        newUtfStr.Length = len;
        newUtfStr.Str = utfXStr;

        return S_OK;
    }

    HRESULT StringExpr::AlternateStrings::SetUtf16String( const char* utf8Str, int utf8Length )
    {
        return SetString( utf8Str, utf8Length, Utf8To16, StringKind_Utf16, mNewUtf16Str, mStrBuf16 );
    }

    HRESULT StringExpr::AlternateStrings::SetUtf32String( const char* utf8Str, int utf8Length )
    {
        return SetString( utf8Str, utf8Length, Utf8To32, StringKind_Utf32, mNewUtf32Str, mStrBuf32 );
    }
}
