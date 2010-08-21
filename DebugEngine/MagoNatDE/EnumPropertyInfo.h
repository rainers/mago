/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "ComEnumWithCount.h"


namespace MagoEE
{
    class IEEDEnumValues;
    struct EvalResult;
}


namespace Mago
{
    class _CopyPropertyInfo
    {
    public:
        static HRESULT copy( DEBUG_PROPERTY_INFO* dest, const DEBUG_PROPERTY_INFO* source );
        static void init( DEBUG_PROPERTY_INFO* p );
        static void destroy( DEBUG_PROPERTY_INFO* p );
    };

    typedef ScopedArray<DEBUG_PROPERTY_INFO, _CopyPropertyInfo> PropertyInfoArray;

    typedef CComEnumWithCount< 
        IEnumDebugPropertyInfo2, 
        &IID_IEnumDebugPropertyInfo2, 
        DEBUG_PROPERTY_INFO, 
        _CopyPropertyInfo, 
        CComMultiThreadModel
    > EnumDebugPropertyInfo;


    //------------------------------------------------------------------------

    class ExprContext;


    class ATL_NO_VTABLE EnumDebugPropertyInfo2 : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IEnumDebugPropertyInfo2
    {
        RefPtr<MagoEE::IEEDEnumValues>  mEEEnum;
        RefPtr<ExprContext>             mExprContext;
        DEBUGPROP_INFO_FLAGS            mFields;
        DWORD                           mRadix;

    public:
        EnumDebugPropertyInfo2();
        ~EnumDebugPropertyInfo2();

    DECLARE_NOT_AGGREGATABLE(EnumDebugPropertyInfo2)

    BEGIN_COM_MAP(EnumDebugPropertyInfo2)
        COM_INTERFACE_ENTRY(IEnumDebugPropertyInfo2)
    END_COM_MAP()

        STDMETHOD(Next)( ULONG celt, DEBUG_PROPERTY_INFO* rgelt, ULONG* pceltFetched );
        STDMETHOD(Skip)( ULONG celt );
        STDMETHOD(Reset)();
        STDMETHOD(Clone)( IEnumDebugPropertyInfo2** ppEnum );
        STDMETHOD(GetCount)( ULONG* count );

        HRESULT Init( 
            MagoEE::IEEDEnumValues* eeEnum, 
            ExprContext* exprContext,
            DEBUGPROP_INFO_FLAGS dwFields, 
            DWORD dwRadix );

    private:
        HRESULT GetPropertyInfo( 
            const MagoEE::EvalResult& result, 
            const wchar_t* name,
            const wchar_t* fullName,
            DEBUG_PROPERTY_INFO& info );
    };
}
