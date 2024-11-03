/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Expr.h"
#include "ExprContext.h"
#include "Property.h"
#include "ErrorProperty.h"
#include <MagoEED.h>


namespace Mago
{
    // Expr

    Expr::Expr()
    {
    }

    Expr::~Expr()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IDebugExpression2 

    HRESULT Expr::EvaluateAsync( 
            EVALFLAGS dwFlags,
            IDebugEventCallback2* pExprCallback )
    {
        return E_NOTIMPL;
    }

    HRESULT Expr::Abort()
    {
        return E_NOTIMPL;
    }

    HRESULT Expr::EvaluateSync( 
            EVALFLAGS dwFlags,
            DWORD dwTimeout,
            IDebugEventCallback2* pExprCallback,
            IDebugProperty2** ppResult )
    {
        Log::LogMessage( "Expr::EvaluateSync\n" );

        HRESULT hr = S_OK;
        RefPtr<Property>    prop;
        MagoEE::EvalOptions options = MagoEE::EvalOptions::defaults;
        MagoEE::EvalResult  result = { 0 };

        if ((dwFlags & EVAL_NOSIDEEFFECTS) == 0)
            options.AllowAssignment = true;
        if ((dwFlags & EVAL_NOFUNCEVAL) == 0)
            options.AllowFuncExec = true;
        options.Timeout = dwTimeout;

        hr = mParsedExpr->Evaluate( options, mContext, result, {} );
        if ( FAILED( hr ) )
        {
            return MakeErrorPropertyOrReturnOriginalError( hr, ppResult );
        }

        hr = MakeCComObject( prop );
        if ( FAILED( hr ) )
            return hr;

        hr = prop->Init( mExprText, mExprText, result, mContext, MagoEE::FormatOptions() );
        if ( FAILED( hr ) )
            return hr;

        *ppResult = prop.Detach();
        return S_OK;
    }

    HRESULT Expr::MakeErrorPropertyOrReturnOriginalError( HRESULT hrErr, IDebugProperty2** ppResult )
    {
        HRESULT      hr = S_OK;
        std::wstring errStr;

        hr = MagoEE::GetErrorString( hrErr, errStr );
        if ( hr == S_OK )
        {
            RefPtr<ErrorProperty>   errProp;

            hr = MakeCComObject( errProp );
            if ( SUCCEEDED( hr ) )
            {
                hr = errProp->Init( mExprText, mExprText, errStr.c_str() );
                if ( hr == S_OK )
                {
                    *ppResult = errProp.Detach();
                    return S_OK;
                }
            }
        }

        return hrErr;
    }

    HRESULT Expr::Init( MagoEE::IEEDParsedExpr* parsedExpr, const wchar_t* exprText, ExprContext* exprContext )
    {
        _ASSERT( parsedExpr != NULL );
        _ASSERT( exprText != NULL );
        _ASSERT( exprContext != NULL );

        if ( parsedExpr == NULL )
            return E_INVALIDARG;
        if ( exprText == NULL )
            return E_INVALIDARG;
        if ( exprContext == NULL )
            return E_INVALIDARG;

        mParsedExpr = parsedExpr;
        mExprText = exprText;
        mContext = exprContext;

        return S_OK;
    }
}
