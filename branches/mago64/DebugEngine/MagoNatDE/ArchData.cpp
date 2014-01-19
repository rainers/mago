/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ArchData.h"
#include "ArchDataX86.h"


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

    HRESULT ArchData::MakeArchData( UINT32 procType, UINT64 procFeatures, ArchData*& archData )
    {
        switch ( procType )
        {
        case IMAGE_FILE_MACHINE_I386:
            archData = new ArchDataX86( procFeatures );
            break;

        default:
            return E_UNSUPPORTED_BINARY;
        }

        if ( archData == NULL )
            return E_OUTOFMEMORY;

        archData->AddRef();

        return S_OK;
    }
}
