/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class DiaLoadCallback : 
        public MagoST::ILoadCallback
    {
    public:
        struct SearchStatus
        {
            std::wstring    Path;
            HRESULT         ResultCode;
        };

        typedef std::list< SearchStatus >   SearchList;

    private:
        long                mRefCount;
        SearchList          mSearchList;

    public:
        DiaLoadCallback();
        ~DiaLoadCallback();

        //////////////////////////////////////////////////////////// 
        // ILoadCallback 

        virtual void AddRef();
        virtual void Release();

        virtual HRESULT NotifyDebugDir( 
            /* [in] */ bool fExecutable,
            /* [in] */ DWORD cbData,
            /* [size_is][in] */ BYTE* pbData );

        virtual HRESULT NotifyOpenDBG( 
            /* [in] */ LPCOLESTR dbgPath,
            /* [in] */ HRESULT resultCode );

        STDMETHOD( RestrictRegistryAccess )();

        STDMETHOD( RestrictSymbolServerAccess )();

    public:
        const SearchList&   GetSearchList();
        HRESULT GetSearchText( BSTR* text );
    };
}
