/*
   Copyright (c) 2014 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class IRemoteEventCallback
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual const GUID& GetSessionGuid() = 0;
    };
}
