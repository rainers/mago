/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


enum ExecEvent
{
    ExecEvent_None,
    ExecEvent_ProcessStart,
    ExecEvent_ProcessExit,
    ExecEvent_ThreadStart,
    ExecEvent_ThreadExit,
    ExecEvent_ModuleLoad,
    ExecEvent_ModuleUnload,
    ExecEvent_OutputString,
    ExecEvent_LoadComplete,
    ExecEvent_Exception,
    ExecEvent_Breakpoint,
    ExecEvent_StepComplete,
    ExecEvent_AsyncBreakComplete,
    ExecEvent_Error,
    ExecEvent_Max
};


struct EventNode
{
    ExecEvent   Code;
    uint32_t    ThreadId;

    EventNode()
        :   Code( ExecEvent_None ),
            ThreadId( 0 )
    {
    }

    virtual ~EventNode() { }
};

struct ExceptionEventNode : public EventNode
{
    bool                FirstChance;
    EXCEPTION_RECORD    Exception;

    ExceptionEventNode()
        : FirstChance( false )
    {
        Code = ExecEvent_Exception;
        memset( &Exception, 0, sizeof Exception );
    }
};

struct ModuleLoadEventNode : public EventNode
{
    RefPtr<IModule> Module;

    ModuleLoadEventNode()
    {
        Code = ExecEvent_ModuleLoad;
    }
};

struct ModuleUnloadEventNode : public EventNode
{
    RefPtr<IModule> Module;

    ModuleUnloadEventNode()
    {
        Code = ExecEvent_ModuleUnload;
    }
};

struct OutputStringEventNode : public EventNode
{
    std::wstring String;

    OutputStringEventNode()
    {
        Code = ExecEvent_OutputString;
    }
};

struct ErrorEventNode : public EventNode
{
    IEventCallback::EventCode   Event;

    ErrorEventNode()
        : Event( IEventCallback::Event_None )
    {
        Code = ExecEvent_Error;
    }
};

struct ProcessExitEventNode : public EventNode
{
    uint32_t    ExitCode;

    ProcessExitEventNode()
        : ExitCode( 0 )
    {
        Code = ExecEvent_ProcessExit;
    }
};

struct ThreadExitEventNode : public EventNode
{
    uint32_t    ExitCode;

    ThreadExitEventNode()
        : ExitCode( 0 )
    {
        Code = ExecEvent_ThreadExit;
    }
};

struct BreakpointEventNode : public EventNode
{
    Address                 Address;
    std::list< BPCookie >   Cookies;

    BreakpointEventNode()
        : Address( 0 )
    {
        Code = ExecEvent_Breakpoint;
    }
};

typedef std::list< boost::shared_ptr< EventNode > >   EventList;


class EventCallbackBase : public IEventCallback
{
    typedef std::map< Address, RefPtr<IModule> > ModuleMap;

    long                    mRefCount;
    Exec*                   mExec;

    bool                    mVerbose;
    bool                    mTrackEvents;
    bool                    mTrackLastEvent;
    EventList               mEvents;
    boost::shared_ptr<EventNode>   mLastEvent;

    ModuleMap               mModules;
    RefPtr<IModule>         mProcMod;
    uint32_t                mLastThreadId;
    bool                    mLoadCompleted;
    bool                    mProcExited;
    bool                    mCanStepInFuncRetVal;

public:
    EventCallbackBase();

    void SetVerbose( bool value );
    void SetCanStepInFunctionReturnValue( bool value );
    void SetTrackEvents( bool value );
    void SetTrackLastEvent( bool value );

    void SetExec( Exec* exec );
    Exec* GetExec();

    RefPtr<IModule> GetProcessModule();
    uint32_t GetLastThreadId();
    const EventList& GetEvents();
    void ClearEvents();
    boost::shared_ptr<EventNode> GetLastEvent();
    bool GetLoadCompleted();
    bool GetProcessExited();

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
    virtual bool OnException( IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec );
    virtual bool OnBreakpoint( IProcess* process, uint32_t threadId, Address address, Enumerator< BPCookie >* iter );
    virtual void OnStepComplete( IProcess* process, uint32_t threadId );
    virtual void OnAsyncBreakComplete( IProcess* process, uint32_t threadId );
    virtual void OnError( IProcess* process, HRESULT hrErr, EventCode event );

    virtual bool CanStepInFunction( IProcess* process, Address address );

    void PrintCallstacksX86( IProcess* process );
    void PrintCallstacksX64( IProcess* process );

private:
    void TrackEvent( const boost::shared_ptr<EventNode>& node );
};


const char* GetEventName( ExecEvent event );
