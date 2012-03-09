/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "IDataSource.h"


namespace MagoST
{
    class IDebugContainer;
    class IAddressMap;


    class DataSource : public IDataSource
    {
        long                            mRefCount;

        std::auto_ptr<IDebugContainer>  mDebugContainer;
        BYTE*                           mDebugView;
        DWORD                           mDebugSize;
        MagoST::DebugStore              mStore;

        RefPtr<IAddressMap>             mAddrMap;

    public:
        DataSource();
        ~DataSource();

        virtual void AddRef();
        virtual void Release();

        virtual HRESULT LoadDataForExe( 
            const wchar_t* filename,
            const wchar_t* searchPath,
            ILoadCallback* callback );

        virtual HRESULT InitDebugInfo();

        virtual HRESULT OpenSession( ISession*& session );

        DebugStore* GetDebugStore();
        RefPtr<IAddressMap> GetAddressMap();
    };
}
