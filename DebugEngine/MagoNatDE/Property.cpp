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
            std::wstring txt = mExprText;
            MagoEE::AppendFormatSpecifier( txt, mFormatOpts );
            pPropertyInfo->bstrName = SysAllocString( txt.c_str() );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_NAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_FULLNAME) != 0 )
        {
            std::wstring txt = mFullExprText;
            MagoEE::AppendFormatSpecifier( txt, mFormatOpts );
            pPropertyInfo->bstrFullName = SysAllocString( txt.c_str() );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_FULLNAME;
        }

        if ( (dwFields & DEBUGPROP_INFO_VALUE) != 0 )
        {
            pPropertyInfo->bstrValue = FormatValue( dwRadix );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_VALUE;
        }

        if ( (dwFields & DEBUGPROP_INFO_TYPE) != 0 )
        {
            std::wstring typeStr;
            if ( GetPropertyType( mExprContext, mObjVal.ObjVal, mExprText, typeStr ) )
            {
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
            pPropertyInfo->dwAttrib = GetPropertyAttr( mExprContext, mObjVal, mFormatOpts );
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
        Log::LogMessage( "Property::SetValueAsReference\n" );
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
            mObjVal, 
            mExprContext->GetTypeEnv(),
            mExprContext->GetStringTable(),
            mFormatOpts,
            enumVals.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = MakeCComObject( enumProps );
        if ( FAILED( hr ) )
            return hr;

        MagoEE::FormatOptions fmtopts (dwRadix);
        hr = enumProps->Init( enumVals, mExprContext, dwFields, fmtopts );
        if ( FAILED( hr ) )
            return hr;

        return enumProps->QueryInterface( __uuidof( IEnumDebugPropertyInfo2 ), (void**) ppEnum );
    }
    
    HRESULT Property::GetParent( 
        IDebugProperty2** ppParent )
    {
        Log::LogMessage( "Property::GetParent\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetDerivedMostProperty( 
        IDebugProperty2** ppDerivedMost )
    {
        Log::LogMessage( "Property::GetDerivedMostProperty\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetMemoryBytes( 
        IDebugMemoryBytes2** ppMemoryBytes )
    {
        Log::LogMessage( "Property::GetMemoryBytes\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetMemoryContext( 
        IDebugMemoryContext2** ppMemory )
    {
        Log::LogMessage( "Property::GetMemoryContext\n" );

        if ( ppMemory == NULL )
            return E_INVALIDARG;

        HRESULT hr = S_OK;
        RefPtr<CodeContext> codeCxt;
        Address64 addr = 0;

        // TODO: let the EE figure this out
        auto type = mObjVal.ObjVal._Type;
        if ( type->IsPointer() )
        {
            addr = (Address64) mObjVal.ObjVal.Value.Addr;
        }
        else if ( type->IsIntegral() || type->Ty == MagoEE::Tvoid )
        {
            addr = (Address64) mObjVal.ObjVal.Value.UInt64Value;
        }
        else if ( type->IsSArray() )
        {
            addr = (Address64) mObjVal.ObjVal.Addr;
        }
        else if ( type->IsDArray() )
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
        Log::LogMessage( "Property::GetSize\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetReference( 
        IDebugReference2** ppReference )
    {
        Log::LogMessage( "Property::GetReference\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetExtendedInfo( 
        REFGUID guidExtendedInfo,
        VARIANT* pExtendedInfo )
    {
        Log::LogMessage( "Property::GetExtendedInfo\n" );
        return E_NOTIMPL;
    }


    //////////////////////////////////////////////////////////// 
    // IDebugProperty3

    HRESULT Property::GetStringCharLength( ULONG* pLen )
    {
        Log::LogMessage( "Property::GetStringCharLength\n" );

        return MagoEE::GetRawStringLength( mExprContext, mObjVal.ObjVal, *(uint32_t*) pLen );
    }
    
    HRESULT Property::GetStringChars( 
        ULONG bufLen,
        WCHAR* rgString,
        ULONG* pceltFetched )
    {
        Log::LogMessage( "Property::GetStringChars\n" );

        return MagoEE::FormatRawString(
            mExprContext,
            mObjVal.ObjVal,
            bufLen,
            *(uint32_t*) pceltFetched,
            rgString );
    }
    
    HRESULT Property::GetStringViewerText( std::wstring& text )
    {
        Log::LogMessage("Property::GetStringViewerText\n");

        return MagoEE::FormatTextViewerString( mExprContext, mObjVal.ObjVal, text );
    }

    HRESULT Property::CreateObjectID()
    {
        Log::LogMessage( "Property::CreateObjectID\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::DestroyObjectID()
    {
        Log::LogMessage( "Property::DestroyObjectID\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetCustomViewerCount( ULONG* pcelt )
    {
        Log::LogMessage( "Property::GetCustomViewerCount\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::GetCustomViewerList( 
        ULONG celtSkip,
        ULONG celtRequested,
        DEBUG_CUSTOM_VIEWER* rgViewers,
        ULONG* pceltFetched )
    {
        Log::LogMessage( "Property::GetCustomViewerList\n" );
        return E_NOTIMPL;
    }
    
    HRESULT Property::SetValueAsStringWithError( 
        LPCOLESTR pszValue,
        DWORD dwRadix,
        DWORD dwTimeout,
        BSTR* errorString )
    {
        Log::LogMessage( "Property::SetValueAsStringWithError\n" );
        return E_NOTIMPL;
    }


    //------------------------------------------------------------------------

    HRESULT Property::Init( 
        const wchar_t* exprText, 
        const wchar_t* fullExprText, 
        const MagoEE::EvalResult& objVal, 
        ExprContext* exprContext,
        const MagoEE::FormatOptions& fmtopt )
    {
        mExprText = exprText;
        mFullExprText = fullExprText;
        mObjVal = objVal;
        mExprContext = exprContext;
        mFormatOpts = fmtopt;

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

        MagoEE::FormatOptions fmtopts (mFormatOpts);
        if (fmtopts.radix == 0)
            fmtopts.radix = radix;
        hr = MagoEE::EED::FormatValue( mExprContext, mObjVal.ObjVal, fmtopts, str.m_str, {} );
        if ( FAILED( hr ) )
            return NULL;

        return str.Detach();
    }

    bool GetPropertyType( ExprContext* exprContext, const MagoEE::DataObject& objVal, const wchar_t* exprText, std::wstring& typeStr )
    {
        if ( objVal._Type == NULL )
            return false;

        objVal._Type->ToString( typeStr );

        if ( objVal.Addr != 0 &&
             wcsncmp( exprText, L"cast(", 5) != 0 && objVal._Type->IsReference() )
        {
            auto decl = objVal._Type->AsTypeNext()->GetNext()->GetDeclaration();
            MagoEE::UdtKind kind;
            std::wstring className;
            if ( decl && decl->GetUdtKind( kind ) && kind == MagoEE::Udt_Class )
                exprContext->GetClassName( objVal.Addr, className, true );
            if ( !className.empty() && className != typeStr )
                typeStr.append( L" {" ).append( className ).append( L"}" );
        }
        else if ( auto tda = objVal._Type->AsTypeDArray() )
        {
            if ( MagoEE::gShowDArrayLengthInType )
            {
                wchar_t buf[20 + 9 + 1] = L"";
                swprintf_s( buf, L"%I64u", objVal.Value.Array.Length );
                
                typeStr.append( L" {length=" ).append( buf ).append( L"}" );
            }
        }

        return true;
    }

    DWORD GetPropertyAttr( ExprContext* exprContext, const MagoEE::EvalResult& objVal, const MagoEE::FormatOptions& fmtOpts )
    {
        DWORD dwAttrib = 0;
        if ( objVal.HasString )
            dwAttrib |= DBG_ATTRIB_VALUE_RAW_STRING;
        if ( objVal.ReadOnly )
            dwAttrib |= DBG_ATTRIB_VALUE_READONLY;

        if ( fmtOpts.specifier != MagoEE::FormatSpecRaw && objVal.HasChildren )
            dwAttrib |= DBG_ATTRIB_OBJ_IS_EXPANDABLE;
        if (fmtOpts.specifier == MagoEE::FormatSpecRaw && objVal.HasRawChildren )
            dwAttrib |= DBG_ATTRIB_OBJ_IS_EXPANDABLE;

        if ( objVal.ObjVal._Type != NULL )
        {
            if ( !objVal.ObjVal._Type->IsMutable() )
                dwAttrib |= DBG_ATTRIB_TYPE_CONSTANT;
            if ( objVal.ObjVal._Type->IsShared() )
                dwAttrib |= DBG_ATTRIB_TYPE_SYNCHRONIZED;

            if ( auto fun = objVal.ObjVal._Type->AsTypeFunction() )
            {
                if ( fun->IsProperty() )
                    dwAttrib |= DBG_ATTRIB_PROPERTY;
                else
                    dwAttrib |= DBG_ATTRIB_METHOD;
            }
            else if ( auto clss = objVal.ObjVal._Type->AsTypeStruct() )
                dwAttrib |= DBG_ATTRIB_CLASS;
            else
                dwAttrib |= DBG_ATTRIB_DATA;
        }

        return dwAttrib;
    }
}
