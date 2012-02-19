/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "CVSTIPublic.h"
#include "DataSource.h"


namespace MagoST
{
    HRESULT MakeDataSource( IDataSource*& dataSource )
    {
        RefPtr<DataSource>  newDataSource;

        newDataSource = new DataSource();
        if ( newDataSource == NULL )
            return E_OUTOFMEMORY;

        dataSource = newDataSource.Detach();

        return S_OK;
    }
}
