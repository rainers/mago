/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Object.h"


namespace MagoEE
{
    Object::Object()
        :   mRefCount( 0 )
    {
    }

    Object::~Object()
    {
    }

    void Object::AddRef()
    {
        mRefCount++;
    }

    void Object::Release()
    {
        mRefCount--;
        _ASSERT( mRefCount >= 0 );
        if ( mRefCount == 0 )
        {
            delete this;
        }
    }

    ObjectKind ObjectList::GetObjectKind()
    {
        return ObjectKind_ObjectList;
    }
}
