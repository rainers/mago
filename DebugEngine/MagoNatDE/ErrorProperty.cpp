/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ErrorProperty.h"


namespace Mago
{
    // ErrorProperty

    ErrorProperty::ErrorProperty()
    {
    }

    ErrorProperty::~ErrorProperty()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugProperty2 

    HRESULT ErrorProperty::GetPropertyInfo( 
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
            pPropertyInfo->bstrName = SysAllocString( mExprText );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_NAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_FULLNAME) != 0 )
        {
            pPropertyInfo->bstrFullName = SysAllocString( mExprText );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_FULLNAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_VALUE) != 0 )
        {
            pPropertyInfo->bstrValue = SysAllocString( mValText );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_VALUE;
        }

        if ( (dwFields & DEBUGPROP_INFO_PROP) != 0 )
        {
            QueryInterface( __uuidof( IDebugProperty2 ), (void**) &pPropertyInfo->pProperty );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_PROP;
        }

        if ( (dwFields & DEBUGPROP_INFO_ATTRIB) != 0 )
        {
            pPropertyInfo->dwAttrib = DBG_ATTRIB_VALUE_READONLY | DBG_ATTRIB_VALUE_ERROR;
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_ATTRIB;
        }

        return S_OK;
    }
    
    HRESULT ErrorProperty::SetValueAsString( 
        LPCOLESTR pszValue,
        DWORD dwRadix,
        DWORD dwTimeout )
    {
        return E_NOTIMPL;
    }
    
    HRESULT ErrorProperty::SetValueAsReference( 
        IDebugReference2** rgpArgs,
        DWORD dwArgCount,
        IDebugReference2* pValue,
        DWORD dwTimeout )
    {
        return E_NOTIMPL;
    }
    
    HRESULT ErrorProperty::EnumChildren( 
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
    
    HRESULT ErrorProperty::GetParent( 
        IDebugProperty2** ppParent )
    {
        return E_NOTIMPL;
    }
    
    HRESULT ErrorProperty::GetDerivedMostProperty( 
        IDebugProperty2** ppDerivedMost )
    {
        return E_NOTIMPL;
    }
    
    HRESULT ErrorProperty::GetMemoryBytes( 
        IDebugMemoryBytes2** ppMemoryBytes )
    {
        return E_NOTIMPL;
    }
    
    HRESULT ErrorProperty::GetMemoryContext( 
        IDebugMemoryContext2** ppMemory )
    {
        return E_NOTIMPL;
    }
    
    HRESULT ErrorProperty::GetSize( 
        DWORD* pdwSize )
    {
        return E_NOTIMPL;
    }
    
    HRESULT ErrorProperty::GetReference( 
        IDebugReference2** ppReference )
    {
        return E_NOTIMPL;
    }
    
    HRESULT ErrorProperty::GetExtendedInfo( 
        REFGUID guidExtendedInfo,
        VARIANT* pExtendedInfo )
    {
        return E_NOTIMPL;
    }

    HRESULT ErrorProperty::Init( const wchar_t* exprText, const wchar_t* valText )
    {
        mExprText = exprText;
        mValText = valText;
        return S_OK;
    }
}
