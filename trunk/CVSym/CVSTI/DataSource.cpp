/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "DataSource.h"
#include "IDebugContainer.h"
#include "IAddressMap.h"
#include "ImageDebugContainer.h"
#include "Session.h"

using namespace std;


namespace MagoST
{
    DataSource::DataSource()
        :   mRefCount( 0 ),
            mDebugView( NULL ),
            mDebugSize( 0 )
    {
    }

    DataSource::~DataSource()
    {
        if ( mDebugView != NULL )
        {
            _ASSERT( mDebugContainer.get() != NULL );
            mDebugContainer->UnlockDebugSection( mDebugView );
        }
    }

    void DataSource::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void DataSource::Release()
    {
        long    newRef = InterlockedDecrement( &mRefCount );
        _ASSERT( newRef >= 0 );
        if ( newRef == 0 )
        {
            delete this;
        }
    }

    HRESULT DataSource::LoadDataForExe( 
        const wchar_t* filename,
        const wchar_t* searchPath,
        ILoadCallback* callback )
    {
        HRESULT hr = S_OK;
        auto_ptr<ImageDebugContainer>   container( new ImageDebugContainer() );
        RefPtr<IAddressMap> addrMap;

        if ( container.get() == NULL )
            return E_OUTOFMEMORY;

        hr = container->LoadExe( filename, callback );
        if ( FAILED( hr ) )
            return hr;

        // must have one
        hr = container->GetAddressMap( addrMap.Ref() );
        if ( FAILED( hr ) )
            return hr;

        hr = container->LockDebugSection( mDebugView, mDebugSize );
        if ( FAILED( hr ) )
            return hr;

        mAddrMap.Attach( addrMap.Detach() );
        mDebugContainer.reset( container.release() );

        return S_OK;
    }

    HRESULT DataSource::InitDebugInfo()
    {
        if( mAddrMap )
            mStore.SetTLSSegment( mAddrMap->FindSection( ".tls" ) );

        return mStore.InitDebugInfo( mDebugView, mDebugSize );
    }

    HRESULT DataSource::OpenSession( ISession*& session )
    {
        RefPtr<Session> newSession = new Session( this );

        if ( newSession == NULL )
            return E_OUTOFMEMORY;

        session = newSession.Detach();

        return S_OK;
    }

    DebugStore* DataSource::GetDebugStore()
    {
        return &mStore;
    }

    RefPtr<IAddressMap> DataSource::GetAddressMap()
    {
        return mAddrMap;
    }
}
