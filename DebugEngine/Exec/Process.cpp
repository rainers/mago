/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Process.h"
#include "Machine.h"
#include "Thread.h"
#include "Iter.h"


Process::Process( CreateMethod way, HANDLE hProcess, uint32_t id, const wchar_t* exePath )
:   mRefCount( 0 ),
    mCreateWay( way ),
    mhProcess( hProcess ),
    mhSuspendedThread( NULL ),
    mId( id ),
    mExePath( exePath ),
    mEntryPoint( 0 ),
    mMachine( NULL ),
    mReachedLoaderBp( false ),
    mTerminating( false ),
    mDeleted( false ),
    mStopped( false )
{
    _ASSERT( hProcess != NULL );
    _ASSERT( id != 0 );
    _ASSERT( (way == Create_Attach) || (way == Create_Launch) );
    InitializeCriticalSection( &mLock );
}

Process::~Process()
{
    DeleteCriticalSection( &mLock );

    if ( mMachine != NULL )
    {
        mMachine->OnDestroyProcess();
        mMachine->Release();
    }

    if ( mhProcess != NULL )
    {
        CloseHandle( mhProcess );
    }

    if ( mhSuspendedThread != NULL )
    {
        CloseHandle( mhSuspendedThread );
    }
}


void    Process::AddRef()
{
    InterlockedIncrement( &mRefCount );
}

void    Process::Release()
{
    LONG newRefCount = InterlockedDecrement( &mRefCount );
    _ASSERT( newRefCount >= 0 );
    if ( newRefCount == 0 )
    {
        delete this;
    }
}


CreateMethod Process::GetCreateMethod()
{
    return mCreateWay;
}

HANDLE Process::GetHandle()
{
    return mhProcess;
}

uint32_t Process::GetId()
{
    return mId;
}

const wchar_t*  Process::GetExePath()
{
    return mExePath.c_str();
}

Address Process::GetEntryPoint()
{
    return mEntryPoint;
}

void Process::SetEntryPoint( Address entryPoint )
{
    mEntryPoint = entryPoint;
}

HANDLE Process::GetLaunchedSuspendedThread()
{
    return mhSuspendedThread;
}

void Process::SetLaunchedSuspendedThread( HANDLE hThread )
{
    if ( mhSuspendedThread != NULL )
        CloseHandle( mhSuspendedThread );

    mhSuspendedThread = hThread;
}


IMachine* Process::GetMachine()
{
    return mMachine;
}

void Process::SetMachine( IMachine* machine )
{
    if ( mMachine != NULL )
    {
        mMachine->OnDestroyProcess();
        mMachine->Release();
    }

    mMachine = machine;

    if ( machine != NULL )
    {
        machine->AddRef();
    }
}


bool Process::IsStopped()
{
    return mStopped;
}

void Process::SetStopped( bool value )
{
    mStopped = value;
}

bool Process::IsDeleted()
{
    return mDeleted;
}

void Process::SetDeleted()
{
    mDeleted = true;
}

bool Process::IsTerminating()
{
    return mTerminating;
}

void Process::SetTerminating()
{
    mTerminating = true;
}

bool Process::ReachedLoaderBp()
{
    return mReachedLoaderBp;
}

void Process::SetReachedLoaderBp()
{
    mReachedLoaderBp = true;
}


size_t  Process::GetThreadCount()
{
    return mThreads.size();
}

HRESULT Process::EnumThreads( Enumerator< Thread* >*& enumerator )
{
    ProcessGuard guard( this );

    RefReleasePtr< ArrayRefEnum<Thread*> > en = new ArrayRefEnum<Thread*>();

    if ( en == NULL )
        return E_OUTOFMEMORY;

    if ( !en->Init( mThreads.begin(), mThreads.end(), mThreads.size() ) )
        return E_OUTOFMEMORY;

    enumerator = en.Detach();

    return S_OK;
}

void    Process::AddThread( Thread* thread )
{
    _ASSERT( FindThread( thread->GetId() ) == NULL );

    mThreads.push_back( thread );
}

void    Process::DeleteThread( uint32_t threadId )
{
    for ( std::list< RefPtr<Thread> >::iterator it = mThreads.begin();
        it != mThreads.end();
        it++ )
    {
        if ( threadId == (*it)->GetId() )
        {
            mThreads.erase( it );
            break;
        }
    }
}

Thread* Process::FindThread( uint32_t id )
{
    for ( std::list< RefPtr<Thread> >::iterator it = mThreads.begin();
        it != mThreads.end();
        it++ )
    {
        if ( id == (*it)->GetId() )
            return it->Get();
    }

    return NULL;
}

bool    Process::FindThread( uint32_t id, Thread*& thread )
{
    ProcessGuard guard( this );

    Thread* t = FindThread( id );

    if ( t == NULL )
        return false;

    thread = t;
    thread->AddRef();

    return true;
}

void    Process::Lock()
{
    EnterCriticalSection( &mLock );
}

void    Process::Unlock()
{
    LeaveCriticalSection( &mLock );
}
