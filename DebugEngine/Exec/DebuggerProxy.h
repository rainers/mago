/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Exec.h"
#include <Guard.h>


namespace MagoCore
{
    struct CommandFunctor;


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

    public:
        DebuggerProxy();
        ~DebuggerProxy();

        HRESULT Init( IEventCallback* callback );
        HRESULT Start();
        void    Shutdown();

        HRESULT Launch( LaunchInfo* launchInfo, IProcess*& process );
        HRESULT Attach( uint32_t id, IProcess*& process );

        HRESULT Terminate( IProcess* process );
        HRESULT Detach( IProcess* process );

        HRESULT ResumeLaunchedProcess( IProcess* process );

        HRESULT ReadMemory( 
            IProcess* process, 
            Address address,
            uint32_t length, 
            uint32_t& lengthRead, 
            uint32_t& lengthUnreadable, 
            uint8_t* buffer );

        HRESULT WriteMemory( 
            IProcess* process, 
            Address address,
            uint32_t length, 
            uint32_t& lengthWritten, 
            uint8_t* buffer );

        HRESULT SetBreakpoint( IProcess* process, Address address );
        HRESULT RemoveBreakpoint( IProcess* process, Address address );

        HRESULT StepOut( IProcess* process, Address targetAddr, bool handleException );
        HRESULT StepInstruction( IProcess* process, bool stepIn, bool handleException );
        HRESULT StepRange( IProcess* process, bool stepIn, AddressRange range, bool handleException );

        HRESULT Continue( IProcess* process, bool handleException );
        HRESULT Execute( IProcess* process, bool handleException );

        HRESULT AsyncBreak( IProcess* process );

        HRESULT GetThreadContext( IProcess* process, uint32_t threadId, void* context, uint32_t size );
        HRESULT SetThreadContext( 
            IProcess* process, uint32_t threadId, const void* context, uint32_t size );

    private:
        static DWORD CALLBACK   DebugPollProc( void* param );

        HRESULT PollLoop();
        void    SetReadyThread();
        HRESULT CheckMessage();
        HRESULT ProcessCommand( CommandFunctor* cmd );

        HRESULT InvokeCommand( CommandFunctor& cmd );
    };
}
