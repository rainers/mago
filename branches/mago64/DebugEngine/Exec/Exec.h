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
class ProcessMap;
class IProcess;
class Process;
class Thread;
class Module;
class PathResolver;
template <class T>
class RefPtr;

/*
Mode    Disp.   Thread  Method
        Allowed
------------------------------------
any     yes     any     ReadMemory
break   yes     any     WriteMemory
any     yes     any     SetBreakpoint
any     yes     any     RemoveBreakpoint
break   no*     Debug   StepOut
break   no*     Debug   StepInstruction
break   no*     Debug   StepRange
break   no*     Debug   CancelStep
run     yes     any     AsyncBreak
break   yes     any     GetThreadContext
break   yes     any     SetThreadContext

N/A     no      Debug   Init
N/A     no      Debug   Shutdown
N/A     no      Debug   WaitForDebug
N/A     no      Debug   DispatchEvent
break   no*     Debug   Continue
N/A     yes     Debug   Launch
N/A     yes     Debug   Attach
any     yes     Debug   Terminate
any     yes     Debug   Detach
any     yes     Debug   ResumeLaunchedProcess

Some actions are only possible while a debuggee is in break or run mode. 
Calling a method when the debuggee is in the wrong mode returns E_WRONG_STATE.

Methods that process debugging events cannot be called in the middle of an 
event callback. Doing so returns E_WRONG_STATE.

Some actions have an affinity to the thread where Exec was initialized. These 
have to do with debugging events, and controlling the run state of a process. 
Calling these methods outside the Debug Thread returns E_WRONG_THREAD.

If a method takes a process object, but the process is ending or has already 
ended, then the method fails with E_PROCESS_ENDED.

Init fails with E_ALREADY_INIT, if called after a successful call to Init.

All methods fail with E_WRONG_STATE, if called after Shutdown.
*/

class Exec
{
    // because of thread affinity of debugging APIs, 
    // keep track of starting thread ID
    uint32_t        mTid;

    IEventCallback* mCallback;

    DEBUG_EVENT     mLastEvent;

    wchar_t*        mPathBuf;
    size_t          mPathBufLen;

    ProcessMap*     mProcMap;
    PathResolver*   mResolver;

    bool            mIsDispatching;
    bool            mIsShutdown;

public:
    Exec();
    ~Exec();

    // Initializes the core debugger. Associates it with an event callback 
    // which will handle all debugging events. The core debugger is bound to 
    // the current thread (the Debug Thread), so that subsequent calls to 
    // event handling methods and some control methods must be made from that 
    // thread.
    //
    HRESULT Init( IEventCallback* callback );

    // Stops debugging all processes, and frees resources. 
    //
    HRESULT Shutdown();

    // Waits for a debugging event to happen.
    //
    // Returns: S_OK, if an event was captured.
    //          E_TIMEOUT, if the timeout elapsed.
    //          See the table above for other errors.
    //
    HRESULT WaitForDebug( uint32_t millisTimeout );

    // Handle a debugging event. The process object is marked stopped, and the 
    // appropriate event callback method will be called.
    //
    // Returns: S_OK, if ContinueDebug should be called after this call.
    //          S_FALSE to stay in break mode.
    //          See the table above for other errors.
    //
    HRESULT DispatchEvent();

    // Runs a process that reported a debugging event. Marks the process 
    // object as running.
    //
    HRESULT Continue( IProcess* process, bool handleException );

    // Starts a process. The process object that's returned is used to control 
    // the process.
    //
    HRESULT Launch( LaunchInfo* launchInfo, IProcess*& process );

    // Brings a running process under the control of the debugger. The process 
    // object that's returned is used to control the process.
    //
    HRESULT Attach( uint32_t id, IProcess*& process );

    // Ends a process. If the OnCreateProcess callback has not fired, then the 
    // debuggee is immediately ended and detached. Otherwise, the caller must 
    // keep pumping events until OnExitProcess is fired. Any other events 
    // before it are discarded and will not fire a callback. Also, if the 
    // debuggee is in break mode, then this method runs it.
    //
    HRESULT Terminate( IProcess* process );

    // Stops debugging a process. No event callbacks are called.
    //
    HRESULT Detach( IProcess* process );

    // Resumes a process, if it was started suspended.
    //
    HRESULT ResumeLaunchedProcess( IProcess* process );

    // Reads a block of memory from a process's address space. The memory is 
    // read straight from the debuggee and not cached.
    //
    HRESULT ReadMemory( 
        IProcess* process, 
        Address address,
        SIZE_T length, 
        SIZE_T& lengthRead, 
        SIZE_T& lengthUnreadable, 
        uint8_t* buffer );

    // Writes a block of memory to a process's address space. The memory is 
    // written straight to the debuggee and not cached.
    //
    HRESULT WriteMemory(
        IProcess* process,
        Address address,
        SIZE_T length,
        SIZE_T& lengthWritten, 
        uint8_t* buffer );

    // Adds or removes a breakpoint. If the process is running when this 
    // method is called, then all threads will be suspended first.
    //
    HRESULT SetBreakpoint( IProcess* process, Address address, BPCookie cookie );
    HRESULT RemoveBreakpoint( IProcess* process, Address address, BPCookie cookie );

    // Sets up or cancels stepping for the current thread. Only one stepping 
    // action is allowed in a thread at a time.
    //
    HRESULT StepOut( IProcess* process, Address address );
    HRESULT StepInstruction( IProcess* process, bool stepIn, bool sourceMode );
    HRESULT StepRange( IProcess* process, bool stepIn, bool sourceMode, AddressRange* ranges, int rangeCount );
    HRESULT CancelStep( IProcess* process );

    // Causes a running process to enter break mode. A subsequent event will 
    // fire the OnBreakpoint callback.
    //
    HRESULT AsyncBreak( IProcess* process );

    // Gets or sets the register context of a thread.
    // See the WinNT.h header file for processor-specific definitions of 
    // the context records to pass to this method.
    //
    HRESULT GetThreadContext( IProcess* process, uint32_t threadId, void* context, uint32_t size );
    HRESULT SetThreadContext( IProcess* process, uint32_t threadId, const void* context, uint32_t size );

private:
    HRESULT     DispatchAndContinue( Process* proc, const DEBUG_EVENT& debugEvent );
    HRESULT     DispatchProcessEvent( Process* proc, const DEBUG_EVENT& debugEvent );
    HRESULT     HandleCreateThread( Process* proc, const DEBUG_EVENT& debugEvent );
    HRESULT     HandleException( Process* proc, const DEBUG_EVENT& debugEvent );
    HRESULT     HandleOutputString( Process* proc, const DEBUG_EVENT& debugEvent );

    HRESULT     ContinueInternal( Process* proc, bool handleException );

    void        CleanupLastDebugEvent();
    void        ResumeSuspendedProcess( IProcess* process );

    Process*    FindProcess( uint32_t id );
    HRESULT     CreateModule( Process* proc, const DEBUG_EVENT& event, RefPtr<Module>& mod );
    HRESULT     CreateThread( Process* proc, const DEBUG_EVENT& event, RefPtr<Thread>& thread );
};
