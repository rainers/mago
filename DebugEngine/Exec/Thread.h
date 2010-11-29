/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class Thread
{
    LONG            mRefCount;
    HANDLE          mhThread;
    uint32_t        mId;
    Address         mStartAddr;
    Address         mTlsBase;

public:
    Thread( HANDLE hThread, uint32_t id, Address startAddr, Address tlsBase );
    ~Thread();

    void            AddRef();
    void            Release();

    HANDLE          GetHandle();
    uint32_t        GetId();
    Address         GetStartAddr();
    Address         GetTlsBase();
};
