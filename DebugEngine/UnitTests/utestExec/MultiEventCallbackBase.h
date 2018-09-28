/*
   Copyright (c) 2018 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


#include "EventCallbackBase.h"


class MultiEventCallbackBase : public IEventCallback
{
    typedef std::map< Address, RefPtr<IModule> > ModuleMap;

    struct Process
    {
        RefPtr<IProcess>        mProcess;

        EventList               mEvents;
        std::shared_ptr<EventNode>   mLastEvent;

        ModuleMap               mModules;
        RefPtr<IModule>         mProcMod;
        uint32_t                mLastThreadId;
        uint32_t                mProcExitCode;
        bool                    mLoadCompleted;
        bool                    mProcExited;
    };

    typedef std::map< DWORD, Process > ProcessMap;

    long                    mRefCount;
    Exec*                   mExec;

    bool                    mVerbose;
    bool                    mTrackEvents;
    bool                    mTrackLastEvent;
    bool                    mCanStepInFuncRetVal;
    EventList               mEvents;
    std::shared_ptr<EventNode>   mLastEvent;
    ProcessMap              mProcesses;

public:
    MultiEventCallbackBase();

    void SetVerbose( bool value );
    void SetCanStepInFunctionReturnValue( bool value );
    void SetTrackEvents( bool value );
    void SetTrackLastEvent( bool value );

    void SetExec( Exec* exec );
    Exec* GetExec();

    IProcess* GetProcess( DWORD pid );
    bool GetAllProcessesExited();
    std::shared_ptr<EventNode> GetLastEvent();
    const EventList& GetEvents();
    void ClearEvents();

    RefPtr<IModule> GetProcessModule( DWORD pid );
    uint32_t GetLastThreadId( DWORD pid );
    std::shared_ptr<EventNode> GetLastEvent( DWORD pid );
    const EventList* GetEvents( DWORD pid );
    void ClearEvents( DWORD pid );
    bool GetLoadCompleted( DWORD pid );
    bool GetProcessExited( DWORD pid );
    uint32_t GetProcessExitCode( DWORD pid );

    virtual void AddRef();
    virtual void Release();

    virtual void OnProcessStart( IProcess* process );
    virtual void OnProcessExit( IProcess* process, DWORD exitCode );
    virtual void OnThreadStart( IProcess* process, Thread* thread );
    virtual void OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode );
    virtual void OnModuleLoad( IProcess* process, IModule* module );
    virtual void OnModuleUnload( IProcess* process, Address baseAddr );
    virtual void OnOutputString( IProcess* process, const wchar_t* outputString );
    virtual void OnLoadComplete( IProcess* process, DWORD threadId );
    virtual RunMode OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec );
    virtual RunMode OnBreakpoint( IProcess* process, uint32_t threadId, Address address, bool embedded );
    virtual void OnStepComplete( IProcess* process, uint32_t threadId );
    virtual void OnAsyncBreakComplete( IProcess* process, uint32_t threadId );
    virtual void OnError( IProcess* process, HRESULT hrErr, EventCode event );

    virtual ProbeRunMode OnCallProbe( 
        IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange );

    void PrintCallstacksX86( IProcess* process );
    void PrintCallstacksX64( IProcess* process );

private:
    void TrackEvent( const std::shared_ptr<EventNode>& node );
};
