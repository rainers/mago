/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "IProcess.h"


class IMachine;
class Module;

struct ShortDebugEvent
{
    uint32_t    EventCode;
    uint32_t    ThreadId;
    uint32_t    ExceptionCode;
};


class Process : public IProcess
{
public:
    typedef std::list< RefPtr<Thread> > ThreadList;
    typedef ThreadList::const_iterator ThreadIterator;

private:
    LONG            mRefCount;

    CreateMethod    mCreateWay;
    HANDLE          mhProcess;
    HANDLE          mhSuspendedThread;
    uint32_t        mId;
    std::wstring    mExePath;
    Address         mEntryPoint;
    IMachine*       mMachine;
    uint16_t        mMachineType;
    Address         mImageBase;

    bool            mReachedLoaderBp;
    bool            mTerminating;
    bool            mDeleted;
    bool            mStopped;
    bool            mStarted;
    int32_t         mSuspendCount;
    ShortDebugEvent mLastEvent;
    Module*         mOSMod;

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
    Address         GetImageBase();

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

    ThreadIterator  ThreadsBegin();
    ThreadIterator  ThreadsEnd();
    int32_t         GetSuspendCount();
    void            SetSuspendCount( int32_t count );

    IMachine*       GetMachine();
    void            SetMachine( IMachine* machine );

    void            SetEntryPoint( Address entryPoint );
    void            SetMachineType( uint16_t machineType );
    void            SetImageBase( Address address );
    HANDLE          GetLaunchedSuspendedThread();
    void            SetLaunchedSuspendedThread( HANDLE hThread );
    void            SetStopped( bool value );
    void            SetDeleted();
    void            SetTerminating();
    void            SetReachedLoaderBp();
    bool            IsStarted();
    void            SetStarted();
    ShortDebugEvent GetLastEvent();
    void            SetLastEvent( const DEBUG_EVENT& debugEvent );
    void            ClearLastEvent();
    Module*         GetOSModule();
    void            SetOSModule( Module* osModule );

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
