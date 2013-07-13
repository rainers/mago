/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


struct LaunchInfo
{
    const wchar_t*  Exe;
    const wchar_t*  CommandLine;
    const wchar_t*  Dir;
    const wchar_t*  EnvBstr;
    HANDLE          StdInput;
    HANDLE          StdOutput;
    HANDLE          StdError;
    bool            NewConsole;
    bool            Suspend;
};


class IEventCallback;
class IMachine;
class ProcessMap;
class IProcess;
class Process;
class Thread;
class Module;
class PathResolver;
template <class T>
class RefPtr;


class Exec
{
    // because of thread affinity of debugging APIs, 
    // keep track of starting thread ID
    uint32_t        mTid;

    IMachine*       mMachine;
    IEventCallback* mCallback;

    DEBUG_EVENT     mLastEvent;

    wchar_t*        mPathBuf;
    size_t          mPathBufLen;

    ProcessMap*     mProcMap;
    PathResolver*   mResolver;

public:
    Exec();
    ~Exec();

    // TODO: how to handle more than one process? And if they're different machines (x86, x64)?
    //      should we restrict this to one process?
    HRESULT Init( IMachine* machine, IEventCallback* callback );
    void    Shutdown();

    // returns S_OK, E_TIMEOUT
    HRESULT WaitForDebug( uint32_t millisTimeout );
    // returns S_OK: continue; S_FALSE: don't continue
    HRESULT DispatchEvent();
    HRESULT ContinueDebug( bool handleException );

    HRESULT Launch( LaunchInfo* launchInfo, IProcess*& process );
    HRESULT Attach( uint32_t id, IProcess*& process );

    HRESULT Terminate( IProcess* process );
    HRESULT Detach( IProcess* process );

    HRESULT ResumeProcess( IProcess* process );
    HRESULT TerminateNewProcess( IProcess* process );

    // not tied to the exec debugger thread
    HRESULT ReadMemory( 
        IProcess* process, 
        Address address,
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer );

    HRESULT WriteMemory(
        IProcess* process,
        Address address,
        SIZE_T length,
        SIZE_T& lengthWritten, 
        uint8_t* buffer );

    HRESULT SetBreakpoint( IProcess* process, Address address, BPCookie cookie );
    HRESULT RemoveBreakpoint( IProcess* process, Address address, BPCookie cookie );

    HRESULT StepOut( IProcess* process, Address address );
    HRESULT StepInstruction( IProcess* process, bool stepIn, bool sourceMode );
    HRESULT StepRange( IProcess* process, bool stepIn, bool sourceMode, AddressRange* ranges, int rangeCount );
    HRESULT CancelStep( IProcess* process );

    HRESULT AsyncBreak( IProcess* process );

    // TODO: thread context

private:
    HRESULT     HandleOutputString( Process* proc );

    void        CleanupLastDebugEvent();
    void        ResumeSuspendedProcess( IProcess* process );

    Process*    FindProcess( uint32_t id );
    HRESULT     CreateModule( Process* proc, const DEBUG_EVENT& event, RefPtr<Module>& mod );
    HRESULT     CreateThread( Process* proc, const DEBUG_EVENT& event, RefPtr<Thread>& thread );
};
