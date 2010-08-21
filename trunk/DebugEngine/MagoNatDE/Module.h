/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class ATL_NO_VTABLE Module : 
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDebugModule3
    {
        DWORD                       mId;
        RefPtr<IModule>             mCoreMod;
        DWORD                       mLoadIndex;
        RefPtr<MagoST::IDataSource> mDataSource;
        RefPtr<MagoST::ISession>    mSession;
        CComBSTR                    mLoadedSymPath;
        CComBSTR                    mSearchText;

    public:
        Module();
        ~Module();

    DECLARE_NOT_AGGREGATABLE(Module)

    BEGIN_COM_MAP(Module)
        COM_INTERFACE_ENTRY(IDebugModule2)
        COM_INTERFACE_ENTRY(IDebugModule3)
    END_COM_MAP()

        //////////////////////////////////////////////////////////// 
        // IDebugModule2 

        STDMETHOD( GetInfo )( 
           MODULE_INFO_FIELDS dwFields,
           MODULE_INFO*       pInfo );
        STDMETHOD( ReloadSymbols_Deprecated )( 
           LPCOLESTR pszUrlToSymbols,
           BSTR*     pbstrDebugMessage );

        //////////////////////////////////////////////////////////// 
        // IDebugModule3

        STDMETHOD( GetSymbolInfo )(
           SYMBOL_SEARCH_INFO_FIELDS  dwFields,
           MODULE_SYMBOL_SEARCH_INFO* pInfo
        );
        STDMETHOD( LoadSymbols )();
        STDMETHOD( IsUserCode )(
           BOOL* pfUser
        );
        STDMETHOD( SetJustMyCodeState )(
           BOOL fIsUserCode
        );

    public:
        DWORD   GetId();
        void    SetId( DWORD id );
        void    SetCoreModule( ::IModule* module );

        // TODO: could this benefit from r-value refs?
        void    GetName( CComBSTR& name );

        Address GetAddress();
        DWORD   GetSize();
        DWORD   GetLoadIndex();
        void    SetLoadIndex( DWORD index );
        bool    GetSymbolSession( RefPtr<MagoST::ISession>& session );

        HRESULT LoadSymbols( bool sendEvent );
    };
}
