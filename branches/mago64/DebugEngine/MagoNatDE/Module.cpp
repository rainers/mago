/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Module.h"
#include "DiaLoadCallback.h"
#include "ICoreProcess.h"


namespace Mago
{
    // Module

    Module::Module()
        :   mId( 0 ),
            mLoadIndex( 0 )
    {
    }

    Module::~Module()
    {
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IModule2 methods 

    HRESULT Module::GetInfo( 
       MODULE_INFO_FIELDS dwFields,
       MODULE_INFO*       pInfo
    )
    {
        if ( pInfo == NULL )
            return E_POINTER;

        HRESULT hr = S_OK;

        pInfo->dwValidFields = 0;

        if ( (dwFields & MIF_NAME) != 0 )
        {
            CComBSTR    name;

            GetName( name );

            pInfo->m_bstrName = name.Detach();
            if ( pInfo->m_bstrName != NULL )
                pInfo->dwValidFields |= MIF_NAME;
        }

        if ( (dwFields & MIF_URL) != 0 )
        {
            pInfo->m_bstrUrl = SysAllocString( mCoreMod->GetExePath() );
            if ( pInfo->m_bstrUrl != NULL )
                pInfo->dwValidFields |= MIF_URL;
        }

        //if ( (dwFields & MIF_VERSION) != 0 )
        //{
        //}

        //if ( (dwFields & MIF_DEBUGMESSAGE) != 0 )
        //{
        //}

        if ( (dwFields & MIF_LOADADDRESS) != 0 )
        {
            pInfo->m_addrLoadAddress = mCoreMod->GetImageBase();
            pInfo->dwValidFields |= MIF_LOADADDRESS;
        }

        if ( (dwFields & MIF_PREFFEREDADDRESS) != 0 )
        {
            pInfo->m_addrPreferredLoadAddress = mCoreMod->GetPreferredImageBase();
            pInfo->dwValidFields |= MIF_PREFFEREDADDRESS;
        }

        if ( (dwFields & MIF_SIZE) != 0 )
        {
            pInfo->m_dwSize = mCoreMod->GetSize();
            pInfo->dwValidFields |= MIF_SIZE;
        }

        if ( (dwFields & MIF_LOADORDER) != 0 )
        {
            pInfo->m_dwLoadOrder = mLoadIndex;
            pInfo->dwValidFields |= MIF_LOADORDER;
        }

        //if ( (dwFields & MIF_TIMESTAMP) != 0 )
        //{
        //}

        if ( (dwFields & MIF_URLSYMBOLLOCATION) != 0 )
        {
            if ( mLoadedSymPath != NULL )
            {
                pInfo->m_bstrUrlSymbolLocation = mLoadedSymPath.Copy();
                if ( pInfo->m_bstrUrlSymbolLocation != NULL )
                    pInfo->dwValidFields |= MIF_URLSYMBOLLOCATION;
            }
        }

        if ( (dwFields & MIF_FLAGS) != 0 )
        {
            pInfo->m_dwModuleFlags = 0;

            if ( GetSession() != NULL )
                pInfo->m_dwModuleFlags |= MODULE_FLAG_SYMBOLS;

            pInfo->dwValidFields |= MIF_FLAGS;
        }

        return hr;
    }

    HRESULT Module::ReloadSymbols_Deprecated( 
       LPCOLESTR pszUrlToSymbols,
       BSTR*     pbstrDebugMessage
    )
    {
        UNREFERENCED_PARAMETER( pszUrlToSymbols );
        UNREFERENCED_PARAMETER( pbstrDebugMessage );
        return E_NOTIMPL;
    }


    ////////////////////////////////////////////////////////////////////////////// 
    // IModule3 methods 

    HRESULT Module::GetSymbolInfo(
       SYMBOL_SEARCH_INFO_FIELDS  dwFields,
       MODULE_SYMBOL_SEARCH_INFO* pInfo
    )
    {
        if ( pInfo == NULL )
            return E_INVALIDARG;

        pInfo->dwValidFields = 0;

        if ( (dwFields & SSIF_VERBOSE_SEARCH_INFO) != 0 )
        {
            if ( GetSession() != NULL )
            {
                pInfo->bstrVerboseSearchInfo = mSearchText.Copy();
                if ( pInfo->bstrVerboseSearchInfo != NULL )
                    pInfo->dwValidFields |= SSIF_VERBOSE_SEARCH_INFO;
            }
        }

        return S_OK;
    }

    HRESULT Module::LoadSymbols()
    {
        return LoadSymbols( true );
    }

    HRESULT Module::LoadSymbols( bool sendEvent )
    {
        HRESULT hr = S_OK;
        RefPtr<DiaLoadCallback>     callback;
        RefPtr<MagoST::ISession>    session;
        RefPtr<MagoST::IDataSource> dataSource;

        callback = new DiaLoadCallback();
        if ( callback == NULL )
            return E_OUTOFMEMORY;

        hr = MagoST::MakeDataSource( dataSource.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = dataSource->LoadDataForExe( mCoreMod->GetExePath(), NULL, callback );
        if ( FAILED( hr ) )
            return hr;

        hr = dataSource->InitDebugInfo();
        if ( FAILED( hr ) )
            return hr;

        hr = dataSource->OpenSession( session.Ref() );
        if ( FAILED( hr ) )
            return hr;

        session->SetLoadAddress( mCoreMod->GetImageBase() );

        SetSession( session );

        // it's OK to fail here, we'll just return a blank string
        callback->GetSearchText( &mSearchText );

        if ( callback->GetSearchList().size() > 0 )
        {
            mLoadedSymPath = callback->GetSearchList().back().Path.c_str();
        }
        else
        {
            mLoadedSymPath = mCoreMod->GetExePath();
        }

        if ( sendEvent )
        {
            // TODO: send the symbol load event
        }

        return hr;
    }

    HRESULT Module::IsUserCode(
       BOOL* pfUser
    )
    {
        return E_NOTIMPL;
    }

    HRESULT Module::SetJustMyCodeState(
       BOOL fIsUserCode
    )
    {
        return E_NOTIMPL;
    }


    //----------------------------------------------------------------------------

    DWORD   Module::GetId()
    {
        return mId;
    }

    void    Module::SetId( DWORD id )
    {
        mId = id;
    }

    void    Module::SetCoreModule( ICoreModule* module )
    {
        mCoreMod = module;
    }

    void    Module::Dispose()
    {
        // these have to be closed when we're told to close
        // all other resources can be left open

        SetSession( NULL );
    }

    void    Module::GetName( CComBSTR& name )
    {
        wchar_t fname[_MAX_FNAME] = L"";
        wchar_t ext[_MAX_EXT] = L"";
        errno_t err = 0;

        name.Empty();

        err = _wsplitpath_s( 
            mCoreMod->GetExePath(), 
            NULL, 0,
            NULL, 0,
            fname, _countof( fname ),
            ext, _countof( ext ) );

        if ( err == 0 )
            err = wcsncat_s( fname, ext, _TRUNCATE );

        if ( err == 0 )
            name = fname;
    }

    Address Module::GetAddress()
    {
        if ( mCoreMod == NULL )
            return 0;

        return mCoreMod->GetImageBase();
    }

    DWORD Module::GetSize()
    {
        if ( mCoreMod == NULL )
            return 0;

        return mCoreMod->GetSize();
    }

    DWORD   Module::GetLoadIndex()
    {
        return mLoadIndex;
    }

    void    Module::SetLoadIndex( DWORD index )
    {
        mLoadIndex = index;
    }

    bool    Module::GetSymbolSession( RefPtr<MagoST::ISession>& session )
    {
        session = GetSession();
        return session != NULL;
    }

    RefPtr<MagoST::ISession>    Module::GetSession()
    {
        GuardedArea guard( mSessionGuard );
        return mSession;
    }

    void    Module::SetSession( MagoST::ISession* session )
    {
        GuardedArea guard( mSessionGuard );
        mSession = session;
    }

    bool    Module::Contains( Address addr )
    {
        Address modAddr = GetAddress();
        return (addr >= modAddr) && ((addr - modAddr) < GetSize());
    }
}
