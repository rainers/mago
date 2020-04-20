/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include <MagoEED.h>


namespace Mago
{
    class ExprContext;


    [uuid("4CA5A9D1-40F1-4DCE-A758-8EF32EDB4D9A")] 
    class Property : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugProperty3
    {
        CComBSTR              mExprText;
        CComBSTR              mFullExprText;
        MagoEE::EvalResult    mObjVal;
        RefPtr<ExprContext>   mExprContext;
        int                   mPtrSize;
        MagoEE::FormatOptions mFormatOpts;

    public:
        Property();
        ~Property();

    DECLARE_NOT_AGGREGATABLE(Property)

    BEGIN_COM_MAP(Property)
        COM_INTERFACE_ENTRY(IDebugProperty2)
        COM_INTERFACE_ENTRY(IDebugProperty3)
        COM_INTERFACE_ENTRY(Property)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugProperty2 

        STDMETHOD( GetPropertyInfo )( 
            DEBUGPROP_INFO_FLAGS dwFields,
            DWORD dwRadix,
            DWORD dwTimeout,
            IDebugReference2** rgpArgs,
            DWORD dwArgCount,
            DEBUG_PROPERTY_INFO* pPropertyInfo );
        
        STDMETHOD( SetValueAsString )( 
            LPCOLESTR pszValue,
            DWORD dwRadix,
            DWORD dwTimeout );
        
        STDMETHOD( SetValueAsReference )( 
            IDebugReference2** rgpArgs,
            DWORD dwArgCount,
            IDebugReference2* pValue,
            DWORD dwTimeout );
        
        STDMETHOD( EnumChildren )( 
            DEBUGPROP_INFO_FLAGS dwFields,
            DWORD dwRadix,
            REFGUID guidFilter,
            DBG_ATTRIB_FLAGS dwAttribFilter,
            LPCOLESTR pszNameFilter,
            DWORD dwTimeout,
            IEnumDebugPropertyInfo2** ppEnum );
        
        STDMETHOD( GetParent )( 
            IDebugProperty2** ppParent );
        
        STDMETHOD( GetDerivedMostProperty )( 
            IDebugProperty2** ppDerivedMost );
        
        STDMETHOD( GetMemoryBytes )( 
            IDebugMemoryBytes2** ppMemoryBytes );
        
        STDMETHOD( GetMemoryContext )( 
            IDebugMemoryContext2** ppMemory );
        
        STDMETHOD( GetSize )( 
            DWORD* pdwSize );
        
        STDMETHOD( GetReference )( 
            IDebugReference2** ppReference );
        
        STDMETHOD( GetExtendedInfo )( 
            REFGUID guidExtendedInfo,
            VARIANT* pExtendedInfo );

        //////////////////////////////////////////////////////////// 
        // IDebugProperty3

        STDMETHOD( GetStringCharLength )( ULONG* pLen );
        
        STDMETHOD( GetStringChars )( 
            ULONG buflen,
            WCHAR* rgString,
            ULONG* pceltFetched );
        
        STDMETHOD( CreateObjectID )();
        
        STDMETHOD( DestroyObjectID )();
        
        STDMETHOD( GetCustomViewerCount )( ULONG* pcelt );
        
        STDMETHOD( GetCustomViewerList )( 
            ULONG celtSkip,
            ULONG celtRequested,
            DEBUG_CUSTOM_VIEWER* rgViewers,
            ULONG* pceltFetched );
        
        STDMETHOD( SetValueAsStringWithError )( 
            LPCOLESTR pszValue,
            DWORD dwRadix,
            DWORD dwTimeout,
            BSTR* errorString );

    public:
        HRESULT Init( 
            const wchar_t* exprText, 
            const wchar_t* fullExprText, 
            const MagoEE::EvalResult& objVal, 
            ExprContext* exprContext,
            const MagoEE::FormatOptions& fmtopt );

        HRESULT GetStringViewerText( std::wstring& text );

    private:
        BSTR FormatValue( int radix );
    };

    bool GetPropertyType( ExprContext* exprContext, const MagoEE::DataObject& objVal, const wchar_t* exprText, std::wstring& type );
    DWORD GetPropertyAttr( ExprContext* exprContext, const MagoEE::EvalResult& objVal, const MagoEE::FormatOptions& fmtOpts );
}
