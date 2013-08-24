/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


enum CreateMethod
{
    Create_Launch,
    Create_Attach,
};

class Thread;
template <class T>
class Enumerator;


class IProcess
{
public:
    virtual ~IProcess() { }

    virtual void            AddRef() = 0;
    virtual void            Release() = 0;

    virtual CreateMethod    GetCreateMethod() = 0;
    virtual HANDLE          GetHandle() = 0;
    virtual uint32_t        GetId() = 0;
    virtual const wchar_t*  GetExePath() = 0;
    virtual Address         GetEntryPoint() = 0;
    virtual HANDLE          GetLaunchedSuspendedThread() = 0;
    virtual void            SetLaunchedSuspendedThread( HANDLE hThread ) = 0;
    
    virtual bool            IsStopped() = 0;
    virtual bool            IsDeleted() = 0;
    virtual bool            IsTerminating() = 0;
    virtual bool            ReachedLoaderBp() = 0;
    virtual void            SetDeleted() = 0;
    virtual void            SetTerminating() = 0;
    virtual void            SetReachedLoaderBp() = 0;

    // threads

    virtual size_t          GetThreadCount() = 0;
    virtual bool            FindThread( uint32_t id, Thread*& thread ) = 0;
    virtual HRESULT         EnumThreads( Enumerator< Thread* >*& en ) = 0;
};
