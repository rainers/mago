/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


#if defined( CVSTI_EXPORTS )
#define EXPORT __declspec( dllexport )
#else
#define EXPORT __declspec( dllimport )
#endif


namespace MagoST
{
    class IDataSource;


    EXPORT HRESULT MakeDataSource( IDataSource*& dataSource );
}
