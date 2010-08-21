/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class Guard
{
    CRITICAL_SECTION    mCritSec;

public:
    Guard()
    {
        InitializeCriticalSection( &mCritSec );
    }

    ~Guard()
    {
        DeleteCriticalSection( &mCritSec );
    }

    void Enter()
    {
        EnterCriticalSection( &mCritSec );
    }

    void Leave()
    {
        LeaveCriticalSection( &mCritSec );
    }
};

class GuardedArea
{
    Guard&  mGuard;

public:
    explicit GuardedArea( Guard& guard )
        :   mGuard( guard )
    {
        mGuard.Enter();
    }

    ~GuardedArea()
    {
        mGuard.Leave();
    }
};
