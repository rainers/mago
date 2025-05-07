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
#include "ArchData.h"
#include "Thread.h"
#include "IDebuggerProxy.h"
#include "RegProperty.h"
#include "ICoreProcess.h"
#include <MagoEED.h>
#include <MagoCVConst.h>


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
            std::wstring& fullName,
            std::function<HRESULT(HRESULT hr, EvaluateNextResult)> complete );

        HRESULT Init( ExprContext* exprContext );

    private:
        bool AddClosureNames( MagoST::ISession* session, MagoST::ISymbolInfo* sym, const std::string& symname, bool recurse );
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
        std::wstring& fullName,
        std::function<HRESULT(HRESULT hr, EvaluateNextResult)> complete )
    {
        if ( mIndex >= GetCount() )
            return E_FAIL;

        HRESULT hr = S_OK;
        RefPtr<MagoEE::IEEDParsedExpr>  parsedExpr;
        uint32_t    curIndex = mIndex;

        mIndex++;

        name.clear();
        name.append( mNames[curIndex] );

        fullName.clear();
        fullName.append( name );

        hr = MagoEE::ParseText( 
            fullName.c_str(), 
            mExprContext->GetModuleContext()->GetTypeEnv(), 
            mExprContext->GetModuleContext()->GetStringTable(), 
            parsedExpr.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = parsedExpr->Bind( options, mExprContext );
        if ( FAILED( hr ) )
            return hr;

        auto completeExpr = !complete ? std::function<HRESULT(HRESULT hr, MagoEE::EvalResult)>{} :
        [complete, res = EvaluateNextResult{ std::move(name), std::move(fullName) }]
        (HRESULT hr, MagoEE::EvalResult evalres) mutable
        {
            res.result = evalres;
            return complete(hr, res);
        };
        hr = parsedExpr->Evaluate( options, mExprContext, result, completeExpr );
        return hr;
    }

    HRESULT EnumLocalValues::Init( ExprContext* exprContext )
    {
        _ASSERT( exprContext != NULL );

        HRESULT hr = S_OK;
        RefPtr<MagoST::ISession>    session;
        MagoST::SymHandle           childSH = { 0 };
        MagoST::SymbolScope         scope = { 0 };

        hr = exprContext->GetSession( session.Ref() );
        if ( FAILED( hr ) )
            return hr;

        // if both exist, __capture is always the same as __closptr.__chain
        bool hadClosure = false;
        bool hadCapture = false;
        const std::vector<MagoST::SymHandle>& blockSH = exprContext->GetBlockSH();

        for ( auto it = blockSH.rbegin(); it != blockSH.rend(); it++)
        {
            hr = session->SetChildSymbolScope( *it, scope );
            if ( FAILED( hr ) )
                return hr;

            while ( session->NextSymbol( scope, childSH, exprContext->GetPC() ) )
            {
                MagoST::SymInfoData     infoData = { 0 };
                MagoST::ISymbolInfo*    symInfo = NULL;
                SymString               pstrName;
                CComBSTR                bstrName;

                hr = session->GetSymbolInfo( childSH, infoData, symInfo );
                if ( FAILED( hr ) )
                    continue;
            
                if ( symInfo->GetSymTag() != MagoST::SymTagData )
                    continue;

                if ( !symInfo->GetName( pstrName ) )
                    continue;

                if ( pstrName.GetLength() == 9 && strncmp( pstrName.GetName(), "__closptr", 9 ) == 0 )
                {
                    AddClosureNames( session, symInfo, "__closptr", !hadCapture );
                    hadClosure = true;
                }
                if ( pstrName.GetLength() == 9 && strncmp( pstrName.GetName(), "__capture", 9 ) == 0 )
                {
                    if ( !hadClosure )
                        AddClosureNames( session, symInfo, "__capture", true );
                    hadCapture = true;
                }
                if ( pstrName.GetName()[0] == '_' && pstrName.GetName()[1] == '_' )
                {
                    std::wstring tupleName;
                    int tuple_idx = MagoEE::GetTupleName( pstrName.GetName(), pstrName.GetLength(), &tupleName );
                    if ( gOptions.recombineTuples )
                    {
                        if ( tuple_idx == 0 )
                            mNames.push_back( SysAllocStringLen( tupleName.data(), tupleName.length() ) );
                        if ( tuple_idx >= 0 )
                            continue;
                    }
                    // do not hide tuple or ... parameter symbols
                    if ( gOptions.hideInternalNames && tuple_idx < 0 &&
                         MagoEE::GetParamIndex( pstrName.GetName(), pstrName.GetLength() ) < 0 )
                        continue;
                }

                hr = Utf8To16( pstrName.GetName(), pstrName.GetLength(), bstrName.m_str );
                if ( FAILED( hr ) )
                    continue;

                mNames.push_back( bstrName );
                bstrName.Detach();
            }
        }

        mExprContext = exprContext;

        return S_OK;
    }

    bool EnumLocalValues::AddClosureNames( MagoST::ISession* session, MagoST::ISymbolInfo* sym, const std::string& symname, bool recurse )
    {
        MagoST::TypeIndex       pointeeTI = { 0 };
        MagoST::TypeHandle      pointeeTH = { 0 };
        MagoST::SymInfoData     pointeeInfoData = { 0 };
        MagoST::ISymbolInfo*    pointeeInfo = NULL;

        // get pointer type
        if ( !sym->GetType( pointeeTI ) )
            return false;
        if ( !session->GetTypeFromTypeIndex( pointeeTI, pointeeTH ) )
            return false;
        if ( session->GetTypeInfo( pointeeTH, pointeeInfoData, pointeeInfo ) != S_OK )
            return false;
        sym = pointeeInfo;

        // dereference pointer type
        if ( !sym->GetType( pointeeTI ) )
            return false;
        if ( !session->GetTypeFromTypeIndex( pointeeTI, pointeeTH ) )
            return false;
        if ( session->GetTypeInfo( pointeeTH, pointeeInfoData, pointeeInfo ) != S_OK )
            return false;
        sym = pointeeInfo;

        uint16_t            fieldCount = 0;
        MagoST::TypeIndex   fieldListTI = 0;
        MagoST::TypeHandle  fieldListTH = { 0 };

        if ( !sym->GetFieldCount( fieldCount ) )
            return false;

        if ( !sym->GetFieldList( fieldListTI ) )
            return false;

        if ( !session->GetTypeFromTypeIndex( fieldListTI, fieldListTH ) )
            return false;

        MagoST::TypeScope   flistScope = { 0 };

        if ( session->SetChildTypeScope( fieldListTH, flistScope ) != S_OK )
            return false;

        bool isClass = false;
        uint16_t cntData = 0;
        for ( uint16_t i = 0; /*i < fieldCount*/; i++ )
        {
            MagoST::TypeHandle      memberTH = { 0 };
            MagoST::SymInfoData     memberInfoData = { 0 };
            MagoST::ISymbolInfo*    memberInfo = NULL;
            MagoST::SymTag          tag = MagoST::SymTagNull;
            SymString               pstrName;

            if ( !session->NextType( flistScope, memberTH ) )
                // no more
                break;

            if ( session->GetTypeInfo( memberTH, memberInfoData, memberInfo ) != S_OK )
                continue;

            tag = memberInfo->GetSymTag();
            if ( tag == MagoST::SymTagBaseClass )
                isClass = true;
            if ( tag != MagoST::SymTagData )
                continue;

            cntData++;
            if ( !memberInfo->GetName( pstrName ) )
                continue;

            if ( pstrName.GetLength() == 7 && strncmp( pstrName.GetName(), "__chain", 7 ) == 0 )
            {
                if( recurse )
                    AddClosureNames( session, memberInfo, symname + ".__chain", true );
                continue;
            }
            if( cntData == 1 )
            {
                int32_t offset;
                if( isClass || ( memberInfo->GetOffset( offset ) && offset == 0 ) )
                // a __closptr or __capture that does not start with a __chain variable is
                //  actually a pointer to an aggregate also available through "this", so skip it
                    break;
            }
            if ( gOptions.hideInternalNames && pstrName.GetName()[0] == '_' && pstrName.GetName()[1] == '_' )
                continue;

            auto varname = /*symname + "." +*/ std::string( pstrName.GetName(), pstrName.GetLength() );
            CComBSTR bstrName;
            HRESULT hr = Utf8To16( varname.c_str(), varname.length(), bstrName.m_str );
            if (FAILED(hr))
                continue;

            mNames.push_back( bstrName );
            bstrName.Detach();
        }
        return true;
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

    HRESULT EnumRegisters(
        Thread* thread,
        IRegisterSet* regSet,
        DEBUGPROP_INFO_FLAGS dwFields,
        DWORD dwRadix,
        IEnumDebugPropertyInfo2** ppEnum )
    {
        _ASSERT( thread != NULL );

        ArchData*           archData = NULL;

        archData = thread->GetCoreProcess()->GetArchData();

        return EnumRegisters(
            archData,
            regSet,
            thread,
            dwFields,
            dwRadix,
            ppEnum );
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
            return EnumRegisters(
                mExprContext->GetThread(),
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

        MagoEE::FormatOptions fmtopts = { dwRadix };
        hr = enumProps->Init( enumVals, mExprContext, dwFields, fmtopts );
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
