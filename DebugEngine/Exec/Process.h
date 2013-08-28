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
    uint16_t        mMachineType;
    IMachine*       mMachine;

    bool            mReachedLoaderBp;
    bool            mTerminating;
    bool            mDeleted;
    bool            mStopped;

    ThreadList      mThreads;

    CRITICAL_SECTION    mLock;

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
    uint16_t        GetMachineType();

    bool            IsStopped();
    bool            IsDeleted();
    bool            IsTerminating();
    bool            ReachedLoaderBp();

    // threads

    virtual bool    FindThread( uint32_t id, Thread*& thread );
    virtual HRESULT EnumThreads( Enumerator< Thread* >*& en );

    // internal

    size_t          GetThreadCount();
    void            AddThread( Thread* thread );
    void            DeleteThread( uint32_t threadId );

    IMachine*       GetMachine();
    void            SetMachine( IMachine* machine );

    void            SetEntryPoint( Address entryPoint );
    void            SetMachineType( uint16_t machineType );
    HANDLE          GetLaunchedSuspendedThread();
    void            SetLaunchedSuspendedThread( HANDLE hThread );
    void            SetStopped( bool value );
    void            SetDeleted();
    void            SetTerminating();
    void            SetReachedLoaderBp();

    void            Lock();
    void            Unlock();

private:
    Thread*         FindThread( uint32_t id );
};


class ProcessGuard
{
    Process*    mProcess;

public:
    explicit ProcessGuard( Process* process )
        :   mProcess( process )
    {
        _ASSERT( process != NULL );
        mProcess->Lock();
    }

    ~ProcessGuard()
    {
        mProcess->Unlock();
    }

private:
    ProcessGuard( const ProcessGuard& );
    ProcessGuard& operator=( const ProcessGuard& );
};
