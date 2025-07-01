/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EnumPropertyInfo.h"
#include "ExprContext.h"
#include "Property.h"
#include "ErrorProperty.h"
#include <MagoEED.h>

#include <memory>
#include <CorError.h>

using namespace std;


namespace Mago
{
    HRESULT _CopyPropertyInfo::copy( DEBUG_PROPERTY_INFO* dest, const DEBUG_PROPERTY_INFO* source )
    {
        _ASSERT( dest != NULL && source != NULL );
        _ASSERT( dest != source );

        dest->bstrFullName = SysAllocString( source->bstrFullName );
        dest->bstrName = SysAllocString( source->bstrName );
        dest->bstrType = SysAllocString( source->bstrType );
        dest->bstrValue = SysAllocString( source->bstrValue );
        
        dest->dwAttrib = source->dwAttrib;
        dest->dwFields = source->dwFields;

        if (dest->pProperty != NULL)
            dest->pProperty->Release();
        dest->pProperty = source->pProperty;
        if ( dest->pProperty != NULL )
            dest->pProperty->AddRef();

        return S_OK;
    }

    void _CopyPropertyInfo::init( DEBUG_PROPERTY_INFO* p )
    {
        _ASSERT( p != NULL );

        memset( p, 0, sizeof *p );
    }

    void _CopyPropertyInfo::destroy( DEBUG_PROPERTY_INFO* p )
    {
        _ASSERT( p != NULL );

        SysFreeString( p->bstrFullName );
        SysFreeString( p->bstrName );
        SysFreeString( p->bstrType );
        SysFreeString( p->bstrValue );
        
        if ( p->pProperty != NULL )
            p->pProperty->Release();
    }


    //------------------------------------------------------------------------

    EnumDebugPropertyInfo2::EnumDebugPropertyInfo2()
        :   mFields( 0 )
    {
        mFormatOpt.radix = 0;
    }

    EnumDebugPropertyInfo2::~EnumDebugPropertyInfo2()
    {
    }

    HRESULT EnumDebugPropertyInfo2::Next( ULONG celt, DEBUG_PROPERTY_INFO* rgelt, ULONG* pceltFetched )
    {
        if ( rgelt == NULL )
            return E_INVALIDARG;
        if ( pceltFetched == NULL )
            return E_INVALIDARG;

        HRESULT     hr = S_OK;
        uint32_t    countLeft = mEEEnum->GetCount() - mEEEnum->GetIndex();
        uint32_t    i = 0;
        MagoEE::EvalOptions options = MagoEE::EvalOptions::defaults;
        wstring     name;
        wstring     fullName;

        if ( celt > countLeft )
            celt = countLeft;

        for ( i = 0; i < celt; i++ )
        {
            // make sure this is in the loop so it gets cleaned up every time
            MagoEE::EvalResult  result = { 0 };

            // keep enumerating even if we fail to get an item
            hr = mEEEnum->EvaluateNext( options, result, name, fullName, {} );
            if ( FAILED( hr ) )
            {
                hr = GetErrorPropertyInfo( hr, name.c_str(), fullName.c_str(), rgelt[i] );
                if ( FAILED( hr ) )
                    return hr;
                continue;
            }

            hr = GetPropertyInfo( result, name.c_str(), fullName.c_str(), rgelt[i], {} );
            if ( FAILED( hr ) )
            {
                hr = GetErrorPropertyInfo( hr, name.c_str(), fullName.c_str(), rgelt[i] );
                if ( FAILED( hr ) )
                    return hr;
                continue;
            }
        }

        *pceltFetched = i;

        return S_OK;
    }

    HRESULT EnumDebugPropertyInfo2::NextAsync( ExprContext* exprContext, ULONG celt, AsyncCompletion complete )
    {
        _ASSERT( complete );
        HRESULT     hr = S_OK;
        uint32_t    countLeft = mEEEnum->GetCount() - mEEEnum->GetIndex();
        uint32_t    i = 0;
        MagoEE::EvalOptions options = MagoEE::EvalOptions::defaults;

        if ( exprContext )
            mExprContext = exprContext;

        if (celt > countLeft)
            celt = countLeft;

        struct Closure
        {
            Closure(ULONG cnt) : infos(cnt) {}

            AsyncCompletion complete;
            size_t toComplete = 0;
            HRESULT hrCombined = S_OK;
            std::vector<Mago::PropertyInfo> infos;
            HRESULT done(HRESULT hr, size_t cnt)
            {
                hrCombine(hr);
                toComplete -= cnt;
                if (toComplete == 0)
                    return complete(hrCombined, infos);
                return hr;
            }
            HRESULT hrCombine(HRESULT hr)
            {
                if (hrCombined != S_QUEUED && hrCombined != COR_E_OPERATIONCANCELED)
                    if (hr == S_QUEUED && hr == COR_E_OPERATIONCANCELED)
                        hrCombined = hr;
                return hrCombined;
            }
        };
        auto closure = std::make_shared<Closure>(celt);
        closure->complete = complete;
        closure->toComplete = celt;
        for (i = 0; i < celt; i++)
        {
            // keep enumerating even if we fail to get an item
            auto completeItem =
                [this, i, closure](HRESULT hr, MagoEE::IEEDEnumValues::EvaluateNextResult res)
                {
                    if( SUCCEEDED( hr ) )
                    {
                        auto completeInfo = [i, closure](HRESULT hr, const DEBUG_PROPERTY_INFO& info)
                        {
                            Mago::_CopyPropertyInfo::copy(&closure->infos[i], &info);
                            hr = closure->done(hr, 1);
                            return hr;
                        };
                        hr = GetPropertyInfo(res.result, res.name.c_str(), res.fullName.c_str(), closure->infos[i],
                            closure->complete ? completeInfo : std::function<HRESULT(HRESULT, const DEBUG_PROPERTY_INFO&)>{});
                        if (!closure->complete)
                            hr = closure->done(hr, 1);
                    }
                    else
                    {
                        hr = GetErrorPropertyInfo(hr, res.name.c_str(), res.fullName.c_str(), closure->infos[i]);
                        hr = closure->done(hr, 1);
                    }
                    return hr;
                };

            MagoEE::EvalResult result = { 0 };
            std::wstring name;
            std::wstring fullName;
            hr = mEEEnum->EvaluateNext( options, result, name, fullName, completeItem );
            if ( hr == COR_E_OPERATIONCANCELED )
            {
                closure->done( hr, celt - i );
                break;
            }
            if( FAILED( hr ) )
                completeItem(hr, { name, fullName });
            else
                closure->hrCombine( hr );
        }
        return closure->toComplete > 0 ? S_QUEUED : closure->hrCombined;
    }

    HRESULT EnumDebugPropertyInfo2::GetErrorPropertyInfo( 
        HRESULT hrErr,
        const wchar_t* name,
        const wchar_t* fullName, 
        DEBUG_PROPERTY_INFO& info )
    {
        HRESULT hr = S_OK;
        CComPtr<IDebugProperty2>    errProp;

        hr = MakeErrorProperty( hrErr, name, fullName, &errProp );
        if ( FAILED( hr ) )
            return hr;

        hr = errProp->GetPropertyInfo( mFields, mFormatOpt.radix, INFINITE, NULL, 0, &info );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT EnumDebugPropertyInfo2::MakeErrorProperty( 
        HRESULT hrErr, 
        const wchar_t* name,
        const wchar_t* fullName, 
        IDebugProperty2** ppResult )
    {
        RefPtr<ErrorProperty> errProp;
        HRESULT hr = MakeCComObject( errProp );
        if ( SUCCEEDED( hr ) )
        {
            std::wstring errStr;
            MagoEE::GetErrorString( hrErr, errStr );
            hr = errProp->Init( name, fullName, errStr.c_str() );
            if ( hr == S_OK )
            {
                *ppResult = errProp.Detach();
                return S_OK;
            }
        }
        return hrErr;
    }

    HRESULT EnumDebugPropertyInfo2::GetPropertyInfo(
        const MagoEE::EvalResult& result, 
        const wchar_t* name,
        const wchar_t* fullName,
        DEBUG_PROPERTY_INFO& info,
        std::function<HRESULT(HRESULT, const DEBUG_PROPERTY_INFO&)> complete )
    {
        HRESULT hr = S_OK;

        info.dwFields = 0;

        if ( (mFields & DEBUGPROP_INFO_NAME) != 0 )
        {
            info.bstrName = SysAllocString( name );
            info.dwFields |= DEBUGPROP_INFO_NAME;
        }

        if ( (mFields & DEBUGPROP_INFO_FULLNAME) != 0 )
        {
            info.bstrFullName = SysAllocString( fullName );
            info.dwFields |= DEBUGPROP_INFO_FULLNAME;
        }

        if ( (mFields & DEBUGPROP_INFO_TYPE) != 0 )
        {
            std::wstring typeStr;
            if ( GetPropertyType( mExprContext, result.ObjVal, name, typeStr ) )
            {
                info.bstrType = SysAllocString( typeStr.c_str() );
                info.dwFields |= DEBUGPROP_INFO_TYPE;
            }
        }

        if ( (mFields & DEBUGPROP_INFO_PROP) != 0 )
        {
            RefPtr<Property>    prop;

            hr = MakeCComObject( prop );
            if ( SUCCEEDED( hr ) )
            {
                hr = prop->Init( name, fullName, result, mExprContext, mFormatOpt );
                if ( SUCCEEDED( hr ) )
                {
                    prop->QueryInterface( __uuidof( IDebugProperty2 ), (void**) &info.pProperty );
                    info.dwFields |= DEBUGPROP_INFO_PROP;
                }
            }
        }

        if ( (mFields & DEBUGPROP_INFO_ATTRIB) != 0 )
        {
            info.dwAttrib = GetPropertyAttr( result, mFormatOpt );
            info.dwFields |= DEBUGPROP_INFO_ATTRIB;
        }

        if ( (mFields & DEBUGPROP_INFO_VALUE) != 0 )
        {
            info.dwFields |= DEBUGPROP_INFO_VALUE;
            Mago::PropertyInfo info2;
            Mago::_CopyPropertyInfo::copy(&info2, &info);
            auto completeEE = [info2, complete](HRESULT hr, BSTR outStr) mutable
            {
                info2.bstrValue = outStr;
                return complete(hr, info2);
            };
            hr = MagoEE::EED::FormatValue( mExprContext, result.ObjVal, mFormatOpt, info.bstrValue,
                    complete ? completeEE : std::function<HRESULT(HRESULT, BSTR)>{} );
        }
        else if( complete )
            return complete( S_OK, info );

        return hr;
    }

    HRESULT EnumDebugPropertyInfo2::Skip( ULONG celt )
    {
        return mEEEnum->Skip( celt );
    }

    HRESULT EnumDebugPropertyInfo2::Reset()
    {
        mEEEnum->Reset();
        return S_OK;
    }

    HRESULT EnumDebugPropertyInfo2::Clone( IEnumDebugPropertyInfo2** ppEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<MagoEE::IEEDEnumValues>  eeEnumCopy;
        RefPtr<EnumDebugPropertyInfo2>  enumCopy;

        hr = mEEEnum->Clone( eeEnumCopy.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = MakeCComObject( enumCopy );
        if ( FAILED( hr ) )
            return hr;

        hr = enumCopy->Init( eeEnumCopy, mExprContext, mFields, mFormatOpt );
        if ( FAILED( hr ) )
            return hr;

        return S_OK;
    }

    HRESULT EnumDebugPropertyInfo2::GetCount( ULONG* count )
    {
        if ( count == NULL )
            return E_INVALIDARG;

        *count = mEEEnum->GetCount();
        return S_OK;
    }

    HRESULT EnumDebugPropertyInfo2::Init( 
        MagoEE::IEEDEnumValues* eeEnum, 
        ExprContext* exprContext,
        DEBUGPROP_INFO_FLAGS dwFields, 
        const MagoEE::FormatOptions& fmtopts )
    {
        _ASSERT( eeEnum != NULL );
        if ( eeEnum == NULL )
            return E_INVALIDARG;

        mEEEnum = eeEnum;
        mExprContext = exprContext;
        mFields = dwFields;
        mFormatOpt = fmtopts;

        return S_OK;
    }
}
