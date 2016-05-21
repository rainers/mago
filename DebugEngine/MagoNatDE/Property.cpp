/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Property.h"
#include "EnumPropertyInfo.h"
#include "ExprContext.h"
#include "CodeContext.h"

#include "Thread.h"
#include "ICoreProcess.h"
#include "ArchData.h"


namespace Mago
{
    // Property

    Property::Property()
        :   mPtrSize( 0 )
    {
    }

    Property::~Property()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugProperty2 

    HRESULT Property::GetPropertyInfo( 
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
            pPropertyInfo->bstrFullName = SysAllocString( mFullExprText );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_FULLNAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_VALUE) != 0 )
        {
            pPropertyInfo->bstrValue = FormatValue( dwRadix );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_VALUE;
        }

        if ( (dwFields & DEBUGPROP_INFO_TYPE) != 0 )
        {
            if ( mObjVal.ObjVal._Type != NULL )
            {
                std::wstring    typeStr;
                mObjVal.ObjVal._Type->ToString( typeStr );

                pPropertyInfo->bstrType = SysAllocString( typeStr.c_str() );
                pPropertyInfo->dwFields |= DEBUGPROP_INFO_TYPE;
            }
        }

        if ( (dwFields & DEBUGPROP_INFO_PROP) != 0 )
        {
            QueryInterface( __uuidof( IDebugProperty2 ), (void**) &pPropertyInfo->pProperty );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_PROP;
        }

        if ( (dwFields & DEBUGPROP_INFO_ATTRIB) != 0 )
        {
            pPropertyInfo->dwAttrib = 0;
            if ( mObjVal.HasString )
                pPropertyInfo->dwAttrib |= DBG_ATTRIB_VALUE_RAW_STRING;
            if ( mObjVal.ReadOnly )
                pPropertyInfo->dwAttrib |= DBG_ATTRIB_VALUE_READONLY;
            if ( mObjVal.HasChildren )
                pPropertyInfo->dwAttrib |= DBG_ATTRIB_OBJ_IS_EXPANDABLE;
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_ATTRIB;
        }

        return S_OK;
    }
    
    HRESULT Property::SetValueAsString( 
        LPCOLESTR pszValue,
        DWORD dwRadix,
        DWORD dwTimeout )
    {
        // even though it could be read only, let's parse and eval, so we get good errors

        HRESULT         hr = S_OK;
        CComBSTR        assignExprText = mFullExprText;
        CComBSTR        errStr;
        UINT            errPos;
        CComPtr<IDebugExpression2>  expr;
        CComPtr<IDebugProperty2>    prop;

        if ( assignExprText == NULL )
            return E_OUTOFMEMORY;

        hr = assignExprText.Append( L" = " );
        if ( FAILED( hr ) )
            return hr;

        hr = assignExprText.Append( pszValue );
        if ( FAILED( hr ) )
            return hr;

        hr = mExprContext->ParseText( assignExprText, 0, dwRadix, &expr, &errStr, &errPos );
        if ( FAILED( hr ) )
            return hr;

        hr = expr->EvaluateSync( 0, dwTimeout, NULL, &prop );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }
    
    HRESULT Property::SetValueAsReference( 
        IDebugReference2** rgpArgs,
        DWORD dwArgCount,
        IDebugReference2* pValue,
        DWORD dwTimeout )
    {
        OutputDebugStringA( "Property::SetValueAsReference\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::EnumChildren( 
        DEBUGPROP_INFO_FLAGS dwFields,
        DWORD dwRadix,
        REFGUID guidFilter,
        DBG_ATTRIB_FLAGS dwAttribFilter,
        LPCOLESTR pszNameFilter,
        DWORD dwTimeout,
        IEnumDebugPropertyInfo2** ppEnum )
    {
        HRESULT                         hr = S_OK;
        RefPtr<EnumDebugPropertyInfo2>  enumProps;
        RefPtr<MagoEE::IEEDEnumValues>  enumVals;

        hr = MagoEE::EnumValueChildren( 
            mExprContext, 
            mFullExprText, 
            mObjVal.ObjVal, 
            mExprContext->GetTypeEnv(),
            mExprContext->GetStringTable(),
            enumVals.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = MakeCComObject( enumProps );
        if ( FAILED( hr ) )
            return hr;

        hr = enumProps->Init( enumVals, mExprContext, dwFields, dwRadix );
        if ( FAILED( hr ) )
            return hr;

        return enumProps->QueryInterface( __uuidof( IEnumDebugPropertyInfo2 ), (void**) ppEnum );
    }
    
    HRESULT Property::GetParent( 
        IDebugProperty2** ppParent )
    {
        OutputDebugStringA( "Property::GetParent\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetDerivedMostProperty( 
        IDebugProperty2** ppDerivedMost )
    {
        OutputDebugStringA( "Property::GetDerivedMostProperty\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetMemoryBytes( 
        IDebugMemoryBytes2** ppMemoryBytes )
    {
        OutputDebugStringA( "Property::GetMemoryBytes\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetMemoryContext( 
        IDebugMemoryContext2** ppMemory )
    {
        OutputDebugStringA( "Property::GetMemoryContext\n" );

        if ( ppMemory == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        RefPtr<CodeContext> codeCxt;
        Address64 addr = 0;

        // TODO: let the EE figure this out
        if ( mObjVal.ObjVal._Type->IsPointer() )
        {
            addr = (Address64) mObjVal.ObjVal.Value.Addr;
        }
        else if ( mObjVal.ObjVal._Type->IsIntegral() )
        {
            addr = (Address64) mObjVal.ObjVal.Value.UInt64Value;
        }
        else if ( mObjVal.ObjVal._Type->IsSArray() )
        {
            addr = (Address64) mObjVal.ObjVal.Addr;
        }
        else if ( mObjVal.ObjVal._Type->IsDArray() )
        {
            addr = (Address64) mObjVal.ObjVal.Value.Array.Addr;
        }
        else
            return S_GETMEMORYCONTEXT_NO_MEMORY_CONTEXT;

        hr = MakeCComObject( codeCxt );
        if ( FAILED( hr ) )
            return hr;

        hr = codeCxt->Init( addr, NULL, NULL, mPtrSize );
        if ( FAILED( hr ) )
            return hr;

        return codeCxt->QueryInterface( __uuidof( IDebugMemoryContext2 ), (void**) ppMemory );
    }
    
    HRESULT Property::GetSize( 
        DWORD* pdwSize )
    {
        OutputDebugStringA( "Property::GetSize\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetReference( 
        IDebugReference2** ppReference )
    {
        OutputDebugStringA( "Property::GetReference\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetExtendedInfo( 
        REFGUID guidExtendedInfo,
        VARIANT* pExtendedInfo )
    {
        OutputDebugStringA( "Property::GetExtendedInfo\n" );
        return E_NOTIMPL;
    }


    //////////////////////////////////////////////////////////// 
    // IDebugProperty3

    HRESULT Property::GetStringCharLength( ULONG* pLen )
    {
        OutputDebugStringA( "Property::GetStringCharLength\n" );

        return MagoEE::GetRawStringLength( mExprContext, mObjVal.ObjVal, *(uint32_t*) pLen );
    }
    
    HRESULT Property::GetStringChars( 
        ULONG bufLen,
        WCHAR* rgString,
        ULONG* pceltFetched )
    {
        OutputDebugStringA( "Property::GetStringChars\n" );

        return MagoEE::FormatRawString(
            mExprContext,
            mObjVal.ObjVal,
            bufLen,
            *(uint32_t*) pceltFetched,
            rgString );
    }
    
    HRESULT Property::CreateObjectID()
    {
        OutputDebugStringA( "Property::CreateObjectID\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::DestroyObjectID()
    {
        OutputDebugStringA( "Property::DestroyObjectID\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetCustomViewerCount( ULONG* pcelt )
    {
        OutputDebugStringA( "Property::GetCustomViewerCount\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetCustomViewerList( 
        ULONG celtSkip,
        ULONG celtRequested,
        DEBUG_CUSTOM_VIEWER* rgViewers,
        ULONG* pceltFetched )
    {
        OutputDebugStringA( "Property::GetCustomViewerList\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::SetValueAsStringWithError( 
        LPCOLESTR pszValue,
        DWORD dwRadix,
        DWORD dwTimeout,
        BSTR* errorString )
    {
        OutputDebugStringA( "Property::SetValueAsStringWithError\n" );
        return E_NOTIMPL;
    }


    //------------------------------------------------------------------------

    HRESULT Property::Init( 
        const wchar_t* exprText, 
        const wchar_t* fullExprText, 
        const MagoEE::EvalResult& objVal, 
        ExprContext* exprContext )
    {
        mExprText = exprText;
        mFullExprText = fullExprText;
        mObjVal = objVal;
        mExprContext = exprContext;

        Thread*     thread = exprContext->GetThread();
        ArchData*   archData = thread->GetCoreProcess()->GetArchData();

        mPtrSize = archData->GetPointerSize();

        return S_OK;
    }

    BSTR Property::FormatValue( int radix )
    {
        if ( mObjVal.ObjVal._Type == NULL )
            return NULL;

        HRESULT     hr = S_OK;
        CComBSTR    str;

        hr = MagoEE::EED::FormatValue( mExprContext, mObjVal.ObjVal, radix, str.m_str );
        if ( FAILED( hr ) )
            return NULL;

        return str.Detach();
    }
}
