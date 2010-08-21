/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "FrameProperty.h"
#include "ExprContext.h"
#include "EnumPropertyInfo.h"
#include "EnumX86Reg.h"
#include "RegisterSet.h"
#include <MagoEED.h>


namespace Mago
{
    class EnumLocalValues : public MagoEE::IEEDEnumValues
    {
        typedef std::vector<BSTR> NameList;

        long                    mRefCount;
        uint32_t                mIndex;
        NameList                mNames;
        RefPtr<ExprContext>     mExprContext;

    public:
        EnumLocalValues();
        ~EnumLocalValues();

        virtual void AddRef();
        virtual void Release();

        virtual uint32_t GetCount();
        virtual uint32_t GetIndex();
        virtual void Reset();
        virtual HRESULT Skip( uint32_t count );
        virtual HRESULT Clone( MagoEE::IEEDEnumValues*& copiedEnum );

        virtual HRESULT EvaluateNext( 
            const MagoEE::EvalOptions& options, 
            MagoEE::EvalResult& result,
            std::wstring& name,
            std::wstring& fullName );

        HRESULT Init( ExprContext* exprContext );
    };

    EnumLocalValues::EnumLocalValues()
        :   mRefCount( 0 ),
            mIndex( 0 )
    {
    }

    EnumLocalValues::~EnumLocalValues()
    {
        for ( NameList::iterator it = mNames.begin(); it != mNames.end(); it++ )
        {
            SysFreeString( *it );
        }
    }

    void EnumLocalValues::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void EnumLocalValues::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    uint32_t EnumLocalValues::GetCount()
    {
        return mNames.size();
    }

    uint32_t EnumLocalValues::GetIndex()
    {
        return mIndex;
    }

    void EnumLocalValues::Reset()
    {
        mIndex = 0;
    }

    HRESULT EnumLocalValues::Skip( uint32_t count )
    {
        if ( count > (GetCount() - mIndex) )
        {
            mIndex = GetCount();
            return S_FALSE;
        }

        mIndex += count;

        return S_OK;
    }

    HRESULT EnumLocalValues::Clone( IEEDEnumValues*& copiedEnum )
    {
        HRESULT hr = S_OK;
        RefPtr<EnumLocalValues>  en = new EnumLocalValues();

        if ( en == NULL )
            return E_OUTOFMEMORY;

        hr = en->Init( mExprContext );
        if ( FAILED( hr ) )
            return hr;

        en->mIndex = mIndex;

        copiedEnum = en.Detach();
        return S_OK;
    }

    HRESULT EnumLocalValues::EvaluateNext( 
        const MagoEE::EvalOptions& options, 
        MagoEE::EvalResult& result,
        std::wstring& name,
        std::wstring& fullName )
    {
        if ( mIndex >= GetCount() )
            return E_FAIL;

        HRESULT hr = S_OK;
        RefPtr<MagoEE::IEEDParsedExpr>  parsedExpr;

        name.clear();
        name.append( mNames[mIndex] );

        fullName.clear();
        fullName.append( name );

        hr = MagoEE::EED::ParseText( 
            fullName.c_str(), 
            mExprContext->GetTypeEnv(), 
            mExprContext->GetStringTable(), 
            parsedExpr.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Bind( options, mExprContext );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Evaluate( options, mExprContext, result );
        if ( FAILED( hr ) )
            return hr;

        mIndex++;

        return S_OK;
    }

    HRESULT EnumLocalValues::Init( ExprContext* exprContext )
    {
        _ASSERT( exprContext != NULL );

        HRESULT hr = S_OK;
        RefPtr<MagoST::ISession>    session;
        MagoST::SymHandle           funcSH = { 0 };
        MagoST::SymHandle           childSH = { 0 };
        MagoST::SymbolScope         scope = { 0 };

        hr = exprContext->GetSession( session.Ref() );
        if ( FAILED( hr ) )
            return hr;

        funcSH = exprContext->GetFunctionSH();

        hr = session->SetChildSymbolScope( funcSH, scope );
        if ( FAILED( hr ) )
            return hr;

        // TODO: need to go down the block's ancestors until you get to the function

        while ( session->NextSymbol( scope, childSH ) )
        {
            MagoST::SymInfoData     infoData = { 0 };
            MagoST::ISymbolInfo*    symInfo = NULL;
            PasString*              pstrName = NULL;
            CComBSTR                bstrName;

            hr = session->GetSymbolInfo( childSH, infoData, symInfo );
            if ( FAILED( hr ) )
                continue;

            if ( !symInfo->GetName( pstrName ) )
                continue;

            hr = Utf8To16( pstrName->name, pstrName->len, bstrName.m_str );
            if ( FAILED( hr ) )
                continue;

            mNames.push_back( bstrName );
            bstrName.Detach();
        }

        mExprContext = exprContext;

        return S_OK;
    }


    ////////////////////////////////////////////////////////////////////////////// 


    // FrameProperty

    FrameProperty::FrameProperty()
    {
    }

    FrameProperty::~FrameProperty()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugProperty2 

    HRESULT FrameProperty::GetPropertyInfo( 
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

        if ( (dwFields & DEBUGPROP_INFO_PROP) != 0 )
        {
            QueryInterface( __uuidof( IDebugProperty2 ), (void**) &pPropertyInfo->pProperty );
            pPropertyInfo->dwFields |= DEBUGPROP_INFO_PROP;
        }

        return S_OK;
    }
    
    HRESULT FrameProperty::SetValueAsString( 
        LPCOLESTR pszValue,
        DWORD dwRadix,
        DWORD dwTimeout )
    {
        return E_NOTIMPL;
    }
    
    HRESULT FrameProperty::SetValueAsReference( 
        IDebugReference2** rgpArgs,
        DWORD dwArgCount,
        IDebugReference2* pValue,
        DWORD dwTimeout )
    {
        return E_NOTIMPL;
    }
    
    HRESULT FrameProperty::EnumChildren( 
        DEBUGPROP_INFO_FLAGS dwFields,
        DWORD dwRadix,
        REFGUID guidFilter,
        DBG_ATTRIB_FLAGS dwAttribFilter,
        LPCOLESTR pszNameFilter,
        DWORD dwTimeout,
        IEnumDebugPropertyInfo2** ppEnum )
    {
        if ( ppEnum == NULL )
            return E_INVALIDARG;

        if ( (guidFilter == guidFilterLocalsPlusArgs) 
            || (guidFilter == guidFilterAllLocalsPlusArgs) )
        {
        }
        else if ( guidFilter == guidFilterRegisters )
        {
            return EnumX86Registers(
                mRegSet,
                dwFields,
                dwRadix,
                ppEnum );
        }
        else
            return E_NOTIMPL;

        HRESULT                         hr = S_OK;
        RefPtr<EnumDebugPropertyInfo2>  enumProps;
        RefPtr<EnumLocalValues>         enumVals;

        enumVals = new EnumLocalValues();
        if ( enumVals == NULL )
            return E_OUTOFMEMORY;

        hr = enumVals->Init( mExprContext );
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
    
    HRESULT FrameProperty::GetParent( 
        IDebugProperty2** ppParent )
    {
        return E_NOTIMPL;
    }
    
    HRESULT FrameProperty::GetDerivedMostProperty( 
        IDebugProperty2** ppDerivedMost )
    {
        return E_NOTIMPL;
    }
    
    HRESULT FrameProperty::GetMemoryBytes( 
        IDebugMemoryBytes2** ppMemoryBytes )
    {
        return E_NOTIMPL;
    }
    
    HRESULT FrameProperty::GetMemoryContext( 
        IDebugMemoryContext2** ppMemory )
    {
        return E_NOTIMPL;
    }
    
    HRESULT FrameProperty::GetSize( 
        DWORD* pdwSize )
    {
        return E_NOTIMPL;
    }
    
    HRESULT FrameProperty::GetReference( 
        IDebugReference2** ppReference )
    {
        return E_NOTIMPL;
    }
    
    HRESULT FrameProperty::GetExtendedInfo( 
        REFGUID guidExtendedInfo,
        VARIANT* pExtendedInfo )
    {
        return E_NOTIMPL;
    }

    HRESULT FrameProperty::Init( IRegisterSet* regSet, ExprContext* exprContext )
    {
        _ASSERT( regSet != NULL );
        _ASSERT( exprContext != NULL );

        mRegSet = regSet;
        mExprContext = exprContext;
        return S_OK;
    }
}
