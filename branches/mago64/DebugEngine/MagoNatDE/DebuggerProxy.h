/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "..\Exec\DebuggerProxy.h"


namespace Mago
{
    class ArchData;
    class IRegisterSet;
    class EventCallback;
    class ICoreProcess;
    class ICoreThread;


    class DebuggerProxy : public IEventCallback
    {
        MagoCore::DebuggerProxy mExecThread;
        RefPtr<ArchData>        mArch;
        RefPtr<EventCallback>   mCallback;

    public:
        DebuggerProxy();
        ~DebuggerProxy();

        HRESULT Init( EventCallback* callback );
        HRESULT Start();
        void    Shutdown();

        HRESULT GetSystemInfo( ICoreProcess* process, ArchData*& sysInfo );

        HRESULT Launch( LaunchInfo* launchInfo, ICoreProcess*& process );
        HRESULT Attach( uint32_t id, ICoreProcess*& process );

        HRESULT Terminate( ICoreProcess* process );
        HRESULT Detach( ICoreProcess* process );

        HRESULT ResumeLaunchedProcess( ICoreProcess* process );

        HRESULT ReadMemory( 
            ICoreProcess* process, 
            Address address,
            SIZE_T length, 
            SIZE_T& lengthRead, 
            SIZE_T& lengthUnreadable, 
            uint8_t* buffer );

        HRESULT WriteMemory( 
            ICoreProcess* process, 
            Address address,
            SIZE_T length, 
            SIZE_T& lengthWritten, 
            uint8_t* buffer );

        HRESULT SetBreakpoint( ICoreProcess* process, Address address );
        HRESULT RemoveBreakpoint( ICoreProcess* process, Address address );

        HRESULT StepOut( ICoreProcess* process, Address targetAddr, bool handleException );
        HRESULT StepInstruction( ICoreProcess* process, bool stepIn, bool handleException );
        HRESULT StepRange( 
            ICoreProcess* process, bool stepIn, AddressRange range, bool handleException );

        HRESULT Continue( ICoreProcess* process, bool handleException );
        HRESULT Execute( ICoreProcess* process, bool handleException );

        HRESULT AsyncBreak( ICoreProcess* process );

        HRESULT GetThreadContext( ICoreProcess* process, ICoreThread* thread, IRegisterSet*& regSet );
        HRESULT SetThreadContext( ICoreProcess* process, ICoreThread* thread, IRegisterSet* regSet );

        // IEventCallback

        virtual void AddRef();
        virtual void Release();

        virtual void OnProcessStart( IProcess* process );
        virtual void OnProcessExit( IProcess* process, DWORD exitCode );
        virtual void OnThreadStart( IProcess* process, ::Thread* thread );
        virtual void OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode );
        virtual void OnModuleLoad( IProcess* process, IModule* module );
        virtual void OnModuleUnload( IProcess* process, Address baseAddr );
        virtual void OnOutputString( IProcess* process, const wchar_t* outputString );
        virtual void OnLoadComplete( IProcess* process, DWORD threadId );

        virtual RunMode OnException( 
            IProcess* process, DWORD threadId, bool firstChance, const EXCEPTION_RECORD* exceptRec );

        virtual RunMode OnBreakpoint( 
            IProcess* process, uint32_t threadId, Address address, bool embedded );

        virtual void OnStepComplete( IProcess* process, uint32_t threadId );
        virtual void OnAsyncBreakComplete( IProcess* process, uint32_t threadId );
        virtual void OnError( IProcess* process, HRESULT hrErr, EventCode event );

        virtual ProbeRunMode OnCallProbe( 
            IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange );

    private:
        HRESULT CacheSystemInfo();
    };
}
