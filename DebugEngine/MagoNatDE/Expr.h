/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    class IEEDParsedExpr;
}


namespace Mago
{
    class ExprContext;


    class ATL_NO_VTABLE Expr : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugExpression2
    {
        CComBSTR                        mExprText;
        RefPtr<ExprContext>             mContext;
        RefPtr<MagoEE::IEEDParsedExpr>  mParsedExpr;

    public:
        Expr();
        ~Expr();

    DECLARE_NOT_AGGREGATABLE(Expr)

    BEGIN_COM_MAP(Expr)
        COM_INTERFACE_ENTRY(IDebugExpression2)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugExpression2 

        STDMETHOD( EvaluateAsync )( 
            EVALFLAGS dwFlags,
            IDebugEventCallback2* pExprCallback );
        
        STDMETHOD( Abort )();
        
        STDMETHOD( EvaluateSync )( 
            EVALFLAGS dwFlags,
            DWORD dwTimeout,
            IDebugEventCallback2* pExprCallback,
            IDebugProperty2** ppResult );

    public:
        HRESULT Init( MagoEE::IEEDParsedExpr* parsedExpr, const wchar_t* exprText, ExprContext* exprContext );

    private:
        HRESULT MakeErrorPropertyOrReturnOriginalError( HRESULT hrErr, IDebugProperty2** ppResult );
    };
}
