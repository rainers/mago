/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "IProcess.h"


class IMachine;


class Process : public IProcess
{
    typedef std::list< RefPtr<Thread> > ThreadList;

    LONG            mRefCount;

    CreateMethod    mCreateWay;
    HANDLE          mhProcess;
    HANDLE          mhSuspendedThread;
    uint32_t        mId;
    std::wstring    mExePath;
    Address         mEntryPoint;
    IMachine*       mMachine;

    bool            mReachedLoaderBp;
    bool            mTerminating;
    bool            mDeleted;
    bool            mStopped;

    ThreadList      mThreads;

public:
    Process( CreateMethod way, HANDLE hProcess, uint32_t id, const wchar_t* exePath );
    ~Process();

    void            AddRef();
    void            Release();

    CreateMethod    GetCreateMethod();
    HANDLE          GetHandle();
    uint32_t        GetId();
    const wchar_t*  GetExePath();
    Address         GetEntryPoint();
    void            SetEntryPoint( Address entryPoint );
    HANDLE          GetLaunchedSuspendedThread();
    void            SetLaunchedSuspendedThread( HANDLE hThread );

    bool            IsStopped();
    bool            IsDeleted();
    bool            IsTerminating();
    bool            ReachedLoaderBp();
    void            SetStopped( bool value );
    void            SetDeleted();
    void            SetTerminating();
    void            SetReachedLoaderBp();

    // threads

    virtual size_t  GetThreadCount();
    virtual bool    FindThread( uint32_t id, Thread*& thread );
    virtual HRESULT EnumThreads( Enumerator< Thread* >*& en );

    // internal

    virtual void    AddThread( Thread* thread );
    virtual void    DeleteThread( uint32_t threadId );

    IMachine*       GetMachine();
    void            SetMachine( IMachine* machine );

private:
    Thread*         FindThread( uint32_t id );
};
