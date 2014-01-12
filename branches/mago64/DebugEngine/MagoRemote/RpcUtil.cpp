/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"


// Procedures that the RPC runtime calls when a remote call needs dynamic memory

void* __RPC_USER midl_user_allocate( size_t size )
{
    void* p = malloc( size );
    return p;
}

void __RPC_USER midl_user_free( void* p )
{
    free( p );
}
