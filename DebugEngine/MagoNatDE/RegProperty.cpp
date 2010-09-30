/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "RegProperty.h"
#include "RegisterSet.h"
#include "EnumPropertyInfo.h"
#include "FormatNum.h"
#include <Real.h>


namespace Mago
{
    HRESULT EnumRegisters(
        const RegGroup* groups,
        uint32_t groupCount,
        IRegisterSet* regSet, 
        DEBUGPROP_INFO_FLAGS fields,
        DWORD radix,
        IEnumDebugPropertyInfo2** enumerator )
    {
        HRESULT             hr = S_OK;
        PropertyInfoArray   array( groupCount );

        if ( array.Get() == NULL )
            return E_OUTOFMEMORY;

        for ( uint32_t i = 0; i < groupCount; i++ )
        {
            RefPtr<RegGroupProperty>    groupProp;

            hr = MakeCComObject( groupProp );
            if ( FAILED( hr ) )
                return hr;

            hr = groupProp->Init( &groups[i], regSet );
            if ( FAILED( hr ) )
                return hr;

            hr = groupProp->GetPropertyInfo( 
                fields,
                radix,
                INFINITE,
                NULL,
                0,
                &array[i] );
            if ( FAILED( hr ) )
                return hr;
        }

        return MakeEnumWithCount<EnumDebugPropertyInfo>( array, enumerator );
    }


    //------------------------------------------------------------------------
    //  RegGroupProperty
    //------------------------------------------------------------------------

    RegGroupProperty::RegGroupProperty()
        :   mGroup( NULL )
    {
    }

    RegGroupProperty::~RegGroupProperty()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugProperty2 

    HRESULT RegGroupProperty::GetPropertyInfo( 
        DEBUGPROP_INFO_FLAGS dwFields,
        DWORD dwRadix,
        DWORD dwTimeout,
        IDebugReference2** rgpArgs,
        DWORD dwArgCount,
        DEBUG_PROPERTY_INFO* pPropertyInfo )
    {
        if ( pPropertyInfo == NULL )
            return E_INVALIDARG;

        pPropertyInfo->dwFields = 0;

        if ( (dwFields & DEBUGPROP_INFO_NAME) != 0 )
        {
            pPropertyInfo->bstrName = SysAllocString( mName );
            if ( pPropertyInfo->bstrName != NULL )
                pPropertyInfo->dwFields |= DEBUGPROP_INFO_NAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_FULLNAME) != 0 )
        {
            pPropertyInfo->bstrFullName = SysAllocString( mName );
            if ( pPropertyInfo->bstrFullName != NULL )
                pPropertyInfo->dwFields |= DEBUGPROP_INFO_FULLNAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_PROP) != 0 )
        {
            QueryInterface( __uuidof( IDebugProperty2 ), (void**) &pPropertyInfo->pProperty );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_PROP;
        }

        if ( (dwFields & DEBUGPROP_INFO_ATTRIB) != 0 )
        {
            pPropertyInfo->dwAttrib = DBG_ATTRIB_VALUE_READONLY;
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_ATTRIB;
        }

        return S_OK;
    }

    HRESULT RegGroupProperty::SetValueAsString( 
        LPCOLESTR pszValue,
        DWORD dwRadix,
        DWORD dwTimeout )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::SetValueAsReference( 
        IDebugReference2** rgpArgs,
        DWORD dwArgCount,
        IDebugReference2* pValue,
        DWORD dwTimeout )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::EnumChildren( 
        DEBUGPROP_INFO_FLAGS dwFields,
        DWORD dwRadix,
        REFGUID guidFilter,
        DBG_ATTRIB_FLAGS dwAttribFilter,
        LPCOLESTR pszNameFilter,
        DWORD dwTimeout,
        IEnumDebugPropertyInfo2** ppEnum )
    {
        HRESULT             hr = S_OK;
        PropertyInfoArray   array( mGroup->RegCount );

        if ( array.Get() == NULL )
            return E_OUTOFMEMORY;

        for ( uint32_t i = 0; i < mGroup->RegCount; i++ )
        {
            RefPtr<RegisterProperty>    regProp;

            hr = MakeCComObject( regProp );
            if ( FAILED( hr ) )
                return hr;

            hr = regProp->Init( &mGroup->Regs[i], mRegSet );
            if ( FAILED( hr ) )
                return hr;

            hr = regProp->GetPropertyInfo( 
                dwFields,
                dwRadix,
                dwTimeout,
                NULL,
                0,
                &array[i] );
            if ( FAILED( hr ) )
                return hr;
        }

        return MakeEnumWithCount<EnumDebugPropertyInfo>( array, ppEnum );
    }

    HRESULT RegGroupProperty::GetParent( 
        IDebugProperty2** ppParent )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::GetDerivedMostProperty( 
        IDebugProperty2** ppDerivedMost )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::GetMemoryBytes( 
        IDebugMemoryBytes2** ppMemoryBytes )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::GetMemoryContext( 
        IDebugMemoryContext2** ppMemory )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::GetSize( 
        DWORD* pdwSize )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::GetReference( 
        IDebugReference2** ppReference )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::GetExtendedInfo( 
        REFGUID guidExtendedInfo,
        VARIANT* pExtendedInfo )
    {
        return E_NOTIMPL;
    }

    HRESULT RegGroupProperty::Init( const RegGroup* group, IRegisterSet* regSet )
    {
        _ASSERT( group != NULL );
        _ASSERT( regSet != NULL );

        mGroup = group;

        if ( !GetString( mGroup->StrId, mName ) )
            return E_FAIL;

        mRegSet = regSet;

        return S_OK;
    }


    //------------------------------------------------------------------------
    //  RegisterProperty
    //------------------------------------------------------------------------

    RegisterProperty::RegisterProperty()
        :   mReg( NULL )
    {
    }

    RegisterProperty::~RegisterProperty()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugProperty2 

    HRESULT RegisterProperty::GetPropertyInfo( 
        DEBUGPROP_INFO_FLAGS dwFields,
        DWORD dwRadix,
        DWORD dwTimeout,
        IDebugReference2** rgpArgs,
        DWORD dwArgCount,
        DEBUG_PROPERTY_INFO* pPropertyInfo )
    {
        bool    isValid = false;

        if ( pPropertyInfo == NULL )
            return E_INVALIDARG;

        pPropertyInfo->dwFields = 0;

        if ( (dwFields & DEBUGPROP_INFO_NAME) != 0 )
        {
            pPropertyInfo->bstrName = SysAllocString( mReg->Name );
            if ( pPropertyInfo->bstrName != NULL )
                pPropertyInfo->dwFields |= DEBUGPROP_INFO_NAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_FULLNAME) != 0 )
        {
            pPropertyInfo->bstrFullName = SysAllocString( mReg->Name );
            if ( pPropertyInfo->bstrFullName != NULL )
                pPropertyInfo->dwFields |= DEBUGPROP_INFO_FULLNAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_VALUE) != 0 )
        {
            pPropertyInfo->bstrValue = GetValueStr( isValid );
            if ( pPropertyInfo->bstrValue != NULL )
                pPropertyInfo->dwFields |= DEBUGPROP_INFO_VALUE;
        }

        if ( (dwFields & DEBUGPROP_INFO_PROP) != 0 )
        {
            QueryInterface( __uuidof( IDebugProperty2 ), (void**) &pPropertyInfo->pProperty );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_PROP;
        }

        if ( (dwFields & DEBUGPROP_INFO_ATTRIB) != 0 )
        {
            bool    readOnly = true;

            mRegSet->IsReadOnly( mReg->FullReg, readOnly );

            pPropertyInfo->dwAttrib = DBG_ATTRIB_STORAGE_REGISTER;
            if ( readOnly )
                pPropertyInfo->dwAttrib |= DBG_ATTRIB_VALUE_READONLY;
            if ( !isValid )
                pPropertyInfo->dwAttrib |= DBG_ATTRIB_VALUE_NAT;
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_ATTRIB;
        }

        return S_OK;
    }

    HRESULT RegisterProperty::SetValueAsString( 
        LPCOLESTR pszValue,
        DWORD dwRadix,
        DWORD dwTimeout )
    {
        HRESULT         hr = S_OK;
        RegisterType    regType = GetRegisterType( mReg->FullReg );
        RegisterValue   regVal = { 0 };
        uint64_t        limit = 0;
        size_t          strValLen = wcslen( pszValue );

        regVal.Type = regType;

        switch ( regType )
        {
        case RegType_Int8:  limit = std::numeric_limits<uint8_t>::max();  goto Ints;
        case RegType_Int16: limit = std::numeric_limits<uint16_t>::max(); goto Ints;
        case RegType_Int32: limit = std::numeric_limits<uint32_t>::max(); goto Ints;
        case RegType_Int64: limit = std::numeric_limits<uint64_t>::max(); goto Ints;
        Ints:
            {
                _set_errno( 0 );
                wchar_t*    endPtr = NULL;
                uint64_t    val = _wcstoui64( pszValue, &endPtr, 16 );

                if ( (errno != 0) || (val > limit) 
                    || ((size_t) (endPtr - pszValue) < strValLen) )
                    return E_FAIL;

                if ( mReg->Length > 0 )
                {
                    RegisterValue   oldRegVal = { 0 };
                    uint64_t        oldVal = 0;

                    hr = mRegSet->GetValue( mReg->FullReg, oldRegVal );
                    if ( FAILED( hr ) )
                        return hr;

                    oldVal = oldRegVal.GetInt();

                    val = (val & mReg->Mask) << mReg->Shift;
                    val |= oldVal & ~((uint64_t) mReg->Mask << mReg->Shift);
                }

                regVal.SetInt( val );
            }
            break;

        case RegType_Float32:
        case RegType_Float64:
            {
                _set_errno( 0 );
                wchar_t*    endPtr = NULL;
                double      val = wcstod( pszValue, &endPtr );

                if ( (errno != 0) 
                    || ((size_t) (endPtr - pszValue) < strValLen) )
                    return E_FAIL;

                switch ( regType )
                {
                case RegType_Float32: regVal.Value.F32 = (float) val;   break;
                case RegType_Float64: regVal.Value.F64 = (double) val;  break;
                }
            }
            break;

        case RegType_Float80:
            {
                Real10  r;

                if ( Real10::Parse( pszValue, r ) != 0 )
                    return E_FAIL;

                memcpy( regVal.Value.F80Bytes, r.Words, sizeof r.Words );
            }
            break;

        default:
            return E_FAIL;
        }

        hr = mRegSet->SetValue( mReg->FullReg, regVal );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT RegisterProperty::SetValueAsReference( 
        IDebugReference2** rgpArgs,
        DWORD dwArgCount,
        IDebugReference2* pValue,
        DWORD dwTimeout )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::EnumChildren( 
        DEBUGPROP_INFO_FLAGS dwFields,
        DWORD dwRadix,
        REFGUID guidFilter,
        DBG_ATTRIB_FLAGS dwAttribFilter,
        LPCOLESTR pszNameFilter,
        DWORD dwTimeout,
        IEnumDebugPropertyInfo2** ppEnum )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::GetParent( 
        IDebugProperty2** ppParent )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::GetDerivedMostProperty( 
        IDebugProperty2** ppDerivedMost )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::GetMemoryBytes( 
        IDebugMemoryBytes2** ppMemoryBytes )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::GetMemoryContext( 
        IDebugMemoryContext2** ppMemory )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::GetSize( 
        DWORD* pdwSize )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::GetReference( 
        IDebugReference2** ppReference )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::GetExtendedInfo( 
        REFGUID guidExtendedInfo,
        VARIANT* pExtendedInfo )
    {
        return E_NOTIMPL;
    }

    HRESULT RegisterProperty::Init( const Reg* reg, IRegisterSet* regSet )
    {
        _ASSERT( reg != NULL );
        _ASSERT( regSet != NULL );

        mReg = reg;
        mRegSet = regSet;

        return S_OK;
    }

    BSTR RegisterProperty::GetValueStr( bool& valid )
    {
        const size_t    MaxVectorStrLen = (256 / 8) * 2;    // number of hex digits
        const size_t    MaxFloatStrLen = ::Float80DecStrLen;
        const size_t    MaxValueStrLen = 
            MaxVectorStrLen > MaxFloatStrLen ? MaxVectorStrLen : MaxFloatStrLen;

        HRESULT         hr = S_OK;
        RegisterValue   val = { 0 };
        wchar_t         str[ MaxValueStrLen + 1 ] = L"";
        size_t          digitLen = 0;

        hr = mRegSet->GetValue( mReg->FullReg, val );
        if ( FAILED( hr ) )
            return SysAllocString( L"" );

        valid = (hr == S_OK);

        if ( mReg->Length > 0 )
        {
            // we know it's an integer
            digitLen = (mReg->Length + 3) / 4;
        }
        else
        {
            switch ( val.Type )
            {
            case RegType_Int8:  digitLen = 1*2; break;
            case RegType_Int16: digitLen = 2*2; break;
            case RegType_Int32: digitLen = 4*2; break;
            case RegType_Int64: digitLen = 8*2; break;
            }
        }

        switch ( val.Type )
        {
        case RegType_Int8:
            if ( mReg->Length > 0 )
                val.Value.I8 = (val.Value.I8 >> mReg->Shift) & mReg->Mask;
            swprintf_s( str, L"%0.*X", digitLen, val.Value.I8 );
            break;

        case RegType_Int16:
            if ( mReg->Length > 0 )
                val.Value.I16 = (val.Value.I16 >> mReg->Shift) & mReg->Mask;
            swprintf_s( str, L"%0.*X", digitLen, val.Value.I16 );
            break;

        case RegType_Int32:
            if ( mReg->Length > 0 )
                val.Value.I32 = (val.Value.I32 >> mReg->Shift) & mReg->Mask;
            swprintf_s( str, L"%0.*X", digitLen, val.Value.I32 );
            break;

        case RegType_Int64:
            if ( mReg->Length > 0 )
                val.Value.I64 = (val.Value.I64 >> mReg->Shift) & mReg->Mask;
            swprintf_s( str, L"%0.*I64X", digitLen, val.Value.I64 );
            break;

        case RegType_Float32:
            FormatFloat32( val.Value.F32, str, ::Float32DecStrLen + 1 );
            break;

        case RegType_Float64:
            FormatFloat64( val.Value.F64, str, ::Float64DecStrLen + 1 );
            break;

        case RegType_Float80:
            FormatFloat80( val.Value.F80Bytes, str, ::Float80DecStrLen + 1 );
            break;
        }

        return SysAllocString( str );
    }
}
