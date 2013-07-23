/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ArchData.h"


namespace Mago
{
    ArchData::ArchData()
        :   mRefCount( 0 )
    {
    }

    void ArchData::AddRef()
    {
        InterlockedIncrement( (unsigned int*) &mRefCount );
    }

    void ArchData::Release()
    {
        int newRefCount = InterlockedDecrement( (unsigned int*) &mRefCount );
        if ( newRefCount == 0 )
            delete this;
    }
}
