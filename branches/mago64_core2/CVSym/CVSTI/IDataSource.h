/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoST
{
    class ISession;
    class ILoadCallback;


    class IDataSource
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual HRESULT LoadDataForExe( 
            const wchar_t* filename,
            const wchar_t* searchPath,
            ILoadCallback* callback ) = 0;

        virtual HRESULT InitDebugInfo() = 0;

        virtual HRESULT OpenSession( ISession*& session ) = 0;
    };
}
