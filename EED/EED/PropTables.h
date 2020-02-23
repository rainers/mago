/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    class StdProperty;


    HRESULT InitPropTables();
    void FreePropTables();

    StdProperty* FindBaseProperty( const wchar_t* name );
    StdProperty* FindIntProperty( const wchar_t* name );
    StdProperty* FindFloatProperty( const wchar_t* name );
    StdProperty* FindDArrayProperty( const wchar_t* name );
    StdProperty* FindSArrayProperty( const wchar_t* name );
    StdProperty* FindTupleProperty( const wchar_t* name );
    StdProperty* FindDelegateProperty( const wchar_t* name );
    StdProperty* FindFieldProperty( const wchar_t* name );
}
