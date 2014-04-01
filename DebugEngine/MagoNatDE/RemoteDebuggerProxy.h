/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "IDebuggerProxy.h"
#include "IRemoteEventCallback.h"


typedef void* HCTXCMD;
typedef struct MagoRemote_LaunchInfo MagoRemote_LaunchInfo;
typedef struct MagoRemote_ProcInfo MagoRemote_ProcInfo;


namespace Mago
{
    class EventCallback;
    class ICoreProcess;
    class ICoreThread;
    class IRegisterSet;


    class RemoteDebuggerProxy : public IDebuggerProxy, public IRemoteEventCallback
    {
        long                    mRefCount;
        RefPtr<EventCallback>   mCallback;
        GUID                    mSessionGuid;
        HCTXCMD                 mhContext;

    public:
        RemoteDebuggerProxy();
        ~RemoteDebuggerProxy();

        void AddRef();
        void Release();

        HRESULT Init( EventCallback* callback );
        HRESULT Start();
        void Shutdown();

        // IDebuggerProxy

        HRESULT Launch( LaunchInfo* launchInfo, ICoreProcess*& process );
        HRESULT Attach( uint32_t id, ICoreProcess*& process );

        HRESULT Terminate( ICoreProcess* process );
        HRESULT Detach( ICoreProcess* process );

        HRESULT ResumeLaunchedProcess( ICoreProcess* process );

        HRESULT ReadMemory( 
            ICoreProcess* process, 
            Address64 address,
            uint32_t length, 
            uint32_t& lengthRead, 
            uint32_t& lengthUnreadable, 
            uint8_t* buffer );

        HRESULT WriteMemory( 
            ICoreProcess* process, 
            Address64 address,
            uint32_t length, 
            uint32_t& lengthWritten, 
            uint8_t* buffer );

        HRESULT SetBreakpoint( ICoreProcess* process, Address64 address );
        HRESULT RemoveBreakpoint( ICoreProcess* process, Address64 address );

        HRESULT StepOut( ICoreProcess* process, Address64 targetAddr, bool handleException );
        HRESULT StepInstruction( ICoreProcess* process, bool stepIn, bool handleException );
        HRESULT StepRange( 
            ICoreProcess* process, bool stepIn, AddressRange64 range, bool handleException );

        HRESULT Continue( ICoreProcess* process, bool handleException );
        HRESULT Execute( ICoreProcess* process, bool handleException );

        HRESULT AsyncBreak( ICoreProcess* process );

        HRESULT GetThreadContext( ICoreProcess* process, ICoreThread* thread, IRegisterSet*& regSet );
        HRESULT SetThreadContext( ICoreProcess* process, ICoreThread* thread, IRegisterSet* regSet );

        HRESULT GetPData( 
            ICoreProcess* process, 
            Address64 address, 
            Address64 imageBase, 
            uint32_t size, 
            uint32_t& sizeRead, 
            uint8_t* pdata );

        // IRemoteEventCallback

        const GUID& GetSessionGuid();

        virtual void OnProcessStart( uint32_t pid );
        virtual void OnProcessExit( uint32_t pid, DWORD exitCode );
        virtual void OnThreadStart( uint32_t pid, MagoRemote_ThreadInfo* thread );
        virtual void OnThreadExit( uint32_t pid, DWORD threadId, DWORD exitCode );
        virtual void OnModuleLoad( uint32_t pid, MagoRemote_ModuleInfo* modInfo );
        virtual void OnModuleUnload( uint32_t pid, MagoRemote_Address baseAddr );
        virtual void OnOutputString( uint32_t pid, const wchar_t* outputString );
        virtual void OnLoadComplete( uint32_t pid, DWORD threadId );

        virtual MagoRemote_RunMode OnException( 
            uint32_t pid, 
            DWORD threadId, 
            bool firstChance, 
            unsigned int recordCount,
            MagoRemote_ExceptionRecord* exceptRecords );

        virtual MagoRemote_RunMode OnBreakpoint( 
            uint32_t pid, uint32_t threadId, MagoRemote_Address address, bool embedded );

        virtual void OnStepComplete( uint32_t pid, uint32_t threadId );
        virtual void OnAsyncBreakComplete( uint32_t pid, uint32_t threadId );

        virtual MagoRemote_ProbeRunMode OnCallProbe( 
            uint32_t pid, 
            uint32_t threadId, 
            MagoRemote_Address address, 
            MagoRemote_AddressRange* thunkRange );

    private:
        HCTXCMD GetContextHandle();
        HRESULT LaunchNoException( 
            MagoRemote_LaunchInfo& cmdLaunchInfo, 
            MagoRemote_ProcInfo& cmdProcInfo );
        HRESULT AttachNoException( 
            uint32_t pid, 
            MagoRemote_ProcInfo& cmdProcInfo );
    };
}
