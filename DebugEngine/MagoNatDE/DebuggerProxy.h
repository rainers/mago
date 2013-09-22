/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

namespace Mago
{
    struct CommandFunctor;
    class ArchData;
    class IRegisterSet;


    class DebuggerProxy
    {
        Exec                mExec;
        HANDLE              mhThread;
        DWORD               mWorkerTid;
        IEventCallback*     mCallback;
        HANDLE              mhReadyEvent;
        HANDLE              mhCommandEvent;
        HANDLE              mhResultEvent;
        CommandFunctor*     mCurCommand;
        volatile bool       mShutdown;
        Guard               mCommandGuard;
        RefPtr<ArchData>    mArch;

    public:
        DebuggerProxy();
        ~DebuggerProxy();

        HRESULT Init( IEventCallback* callback );
        HRESULT Start();
        void    Shutdown();

        HRESULT GetSystemInfo( IProcess* process, ArchData*& sysInfo );

        HRESULT InvokeCommand( CommandFunctor& cmd );

        HRESULT Launch( LaunchInfo* launchInfo, IProcess*& process );
        HRESULT Attach( uint32_t id, IProcess*& process );

        HRESULT Terminate( IProcess* process );
        HRESULT Detach( IProcess* process );

        HRESULT ResumeLaunchedProcess( IProcess* process );

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

        HRESULT SetBreakpoint( IProcess* process, Address address );
        HRESULT RemoveBreakpoint( IProcess* process, Address address );

        HRESULT StepOut( IProcess* process, Address targetAddr, bool handleException );
        HRESULT StepInstruction( IProcess* process, bool stepIn, bool sourceMode, bool handleException );
        HRESULT StepRange( IProcess* process, bool stepIn, bool sourceMode, AddressRange range, bool handleException );

        HRESULT Continue( IProcess* process, bool handleException );
        HRESULT Execute( IProcess* process, bool handleException );

        HRESULT AsyncBreak( IProcess* process );

        HRESULT GetThreadContext( IProcess* process, ::Thread* thread, IRegisterSet*& regSet );
        HRESULT SetThreadContext( IProcess* process, ::Thread* thread, IRegisterSet* regSet );

    private:
        HRESULT CacheSystemInfo();

        static DWORD CALLBACK   DebugPollProc( void* param );

        HRESULT PollLoop();
        void    SetReadyThread();
        HRESULT CheckMessage();
        HRESULT ProcessCommand( CommandFunctor* cmd );
    };
}
