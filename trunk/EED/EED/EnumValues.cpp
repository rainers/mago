/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EnumValues.h"


namespace MagoEE
{
    const uint32_t  MaxArrayLength = 1024*1024*1024;


    EEDEnumValues::EEDEnumValues()
        :   mRefCount( 0 )
    {
    }

    void EEDEnumValues::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void EEDEnumValues::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    HRESULT EEDEnumValues::Init( 
        IValueBinder* binder, 
        const wchar_t* parentExprText, 
        const DataObject& parentVal,
        ITypeEnv* typeEnv,
        NameTable* strTable )
    {
        mBinder = binder;
        mParentExprText = parentExprText;
        mParentVal = parentVal;
        mTypeEnv = typeEnv;
        mStrTable = strTable;

        return S_OK;
    }


    //------------------------------------------------------------------------
    //  EEDEnumPointer
    //------------------------------------------------------------------------

    EEDEnumPointer::EEDEnumPointer()
        :   mCountDone( 0 )
    {
    }

    uint32_t EEDEnumPointer::GetCount()
    {
        return 1;
    }

    uint32_t EEDEnumPointer::GetIndex()
    {
        return mCountDone;
    }

    void EEDEnumPointer::Reset()
    {
        mCountDone = 0;
    }

    HRESULT EEDEnumPointer::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mCountDone) )
        {
            mCountDone = GetCount();
            return S_FALSE;
        }

        mCountDone += count;

        return S_OK;
    }

    HRESULT EEDEnumPointer::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EEDEnumPointer>  en = new EEDEnumPointer();

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( mBinder, mParentExprText.c_str(), mParentVal, mTypeEnv, mStrTable );
        if ( FAILED( hr ) )
            return hr;

        en->mCountDone = mCountDone;

        copiedEnum = en.Detach();
        return S_OK;
    }

    HRESULT EEDEnumPointer::EvaluateNext( 
        const EvalOptions& options, 
        EvalResult& result,
        std::wstring& name,
        std::wstring& fullName )
    {
        if ( mCountDone >= GetCount() )
            return E_FAIL;

        HRESULT hr = S_OK;
        RefPtr<IEEDParsedExpr>  parsedExpr;

        name.clear();
        fullName.clear();
        fullName.append( L"*(" );
        fullName.append( mParentExprText );
        fullName.append( 1, L')' );

        hr = ParseText( fullName.c_str(), mTypeEnv, mStrTable, parsedExpr.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Bind( options, mBinder );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Evaluate( options, mBinder, result );
        if ( FAILED( hr ) )
            return hr;

        mCountDone++;

        return S_OK;
    }


    //------------------------------------------------------------------------
    //  EEDEnumSArray
    //------------------------------------------------------------------------

    EEDEnumSArray::EEDEnumSArray()
        :   mCountDone( 0 )
    {
    }

    uint32_t EEDEnumSArray::GetCount()
    {
        uint32_t    count = 0;

        if ( mParentVal._Type->IsSArray() )
        {
            count = mParentVal._Type->AsTypeSArray()->GetLength();
        }
        else if ( mParentVal._Type->IsDArray() )
        {
            if ( mParentVal.Value.Array.Length > MaxArrayLength )
                count = MaxArrayLength;
            else
                count = (uint32_t) mParentVal.Value.Array.Length;
        }

        return count;
    }

    uint32_t EEDEnumSArray::GetIndex()
    {
        return mCountDone;
    }

    void EEDEnumSArray::Reset()
    {
        mCountDone = 0;
    }

    HRESULT EEDEnumSArray::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mCountDone) )
        {
            mCountDone = GetCount();
            return S_FALSE;
        }

        mCountDone += count;

        return S_OK;
    }

    HRESULT EEDEnumSArray::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EEDEnumSArray>  en = new EEDEnumSArray();

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( mBinder, mParentExprText.c_str(), mParentVal, mTypeEnv, mStrTable );
        if ( FAILED( hr ) )
            return hr;

        en->mCountDone = mCountDone;

        copiedEnum = en.Detach();
        return S_OK;
    }

    HRESULT EEDEnumSArray::EvaluateNext( 
        const EvalOptions& options, 
        EvalResult& result,
        std::wstring& name,
        std::wstring& fullName )
    {
        if ( mCountDone >= GetCount() )
            return E_FAIL;

        // 4294967295
        const int   MaxIntStrLen = 10;
        // "[indexInt]", and add some padding
        const int   MaxIndexStrLen = MaxIntStrLen + 2 + 10;

        HRESULT hr = S_OK;
        RefPtr<IEEDParsedExpr>  parsedExpr;
        wchar_t indexStr[ MaxIndexStrLen + 1 ] = L"";

        swprintf_s( indexStr, L"[%d]", mCountDone );

        name.clear();
        name.append( indexStr );

        fullName.clear();
        fullName.append( L"(" );
        fullName.append( mParentExprText );
        fullName.append( L")" );
        fullName.append( name );

        hr = ParseText( fullName.c_str(), mTypeEnv, mStrTable, parsedExpr.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Bind( options, mBinder );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Evaluate( options, mBinder, result );
        if ( FAILED( hr ) )
            return hr;

        mCountDone++;

        return S_OK;
    }
}
