/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "EventCallback.h"
#include "MagoRemoteEvent_h.h"
#include "RpcUtil.h"


namespace Mago
{
    //----------------------------------------------------------------------------
    //  EventCallback
    //----------------------------------------------------------------------------

    EventCallback::EventCallback( HCTXEVENT hEventContext )
        :   mRefCount( 0 ),
            mhEventCtx( hEventContext )
    {
    }

    EventCallback::~EventCallback()
    {
    }

    HCTXEVENT EventCallback::GetContextHandle()
    {
        return mhEventCtx;
    }

    void EventCallback::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void EventCallback::Release()
    {
        long refCount = InterlockedDecrement( &mRefCount );
        _ASSERT( refCount >= 0 );
        if ( refCount == 0 )
        {
            delete this;
        }
    }

    void EventCallback::OnProcessStart( IProcess* process )
    {
        __try
        {
            MagoRemoteEvent_OnProcessStart( 
                GetContextHandle(),
                process->GetId() );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnProcessStart: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnProcessExit( IProcess* process, DWORD exitCode )
    {
        __try
        {
            MagoRemoteEvent_OnProcessExit( 
                GetContextHandle(),
                process->GetId(),
                exitCode );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnProcessExit: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnThreadStart( IProcess* process, Thread* thread )
    {
        MagoRemote_ThreadInfo   threadInfo = { 0 };

        threadInfo.Tid = thread->GetId();
        threadInfo.StartAddr = thread->GetStartAddr();
        threadInfo.TebBase = thread->GetTebBase();

        __try
        {
            MagoRemoteEvent_OnThreadStart( 
                GetContextHandle(),
                process->GetId(),
                &threadInfo );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnThreadStart: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnThreadExit( IProcess* process, DWORD threadId, DWORD exitCode )
    {
        __try
        {
            MagoRemoteEvent_OnThreadExit( 
                GetContextHandle(),
                process->GetId(),
                threadId,
                exitCode );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnThreadExit: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnModuleLoad( IProcess* process, IModule* module )
    {
        MagoRemote_ModuleInfo   modInfo = { 0 };

        modInfo.ImageBase = module->GetImageBase();
        modInfo.PreferredImageBase = module->GetPreferredImageBase();
        modInfo.Size = module->GetSize();
        modInfo.MachineType = module->GetMachine();
        modInfo.Path = module->GetPath();

        __try
        {
            MagoRemoteEvent_OnModuleLoad( 
                GetContextHandle(),
                process->GetId(),
                &modInfo );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnModuleLoad: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnModuleUnload( IProcess* process, Address baseAddr )
    {
        __try
        {
            MagoRemoteEvent_OnModuleUnload( 
                GetContextHandle(),
                process->GetId(),
                baseAddr );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnModuleUnload: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnOutputString( IProcess* process, const wchar_t* outputString )
    {
        __try
        {
            MagoRemoteEvent_OnOutputString( 
                GetContextHandle(),
                process->GetId(),
                outputString );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnOutputString: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnLoadComplete( IProcess* process, DWORD threadId )
    {
        __try
        {
            MagoRemoteEvent_OnLoadComplete( 
                GetContextHandle(),
                process->GetId(),
                threadId );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnLoadComplete: %08X\n", 
                RpcExceptionCode() );
        }
    }

    RunMode EventCallback::OnException( 
        IProcess* process, 
        DWORD threadId, 
        bool firstChance, 
        const EXCEPTION_RECORD* exceptRec )
    {
        const int MaxTransmitExceptionRecords = 4;

        RunMode mode = RunMode_Run;
        int     recordCount = 0;
        MagoRemote_ExceptionRecord  eventExceptRecs[MaxTransmitExceptionRecords];
        const EXCEPTION_RECORD*     srcRec = exceptRec;
        MagoRemote_ExceptionRecord* destRec = &eventExceptRecs[0];

        for ( int i = 0; i < MaxTransmitExceptionRecords && srcRec != NULL; i++ )
        {
            recordCount++;

            destRec->ExceptionAddress = (DWORD64) srcRec->ExceptionAddress;
            destRec->ExceptionCode = srcRec->ExceptionCode;
            destRec->ExceptionFlags = srcRec->ExceptionFlags;
            destRec->NumberParameters = srcRec->NumberParameters;

            for ( DWORD j = 0; j < destRec->NumberParameters; j++ )
            {
                destRec->ExceptionInformation[j] = srcRec->ExceptionInformation[j];
            }

            srcRec = srcRec->ExceptionRecord;
            destRec++;
        }

        __try
        {
            mode = (RunMode) MagoRemoteEvent_OnException( 
                GetContextHandle(),
                process->GetId(),
                threadId,
                firstChance,
                recordCount,
                eventExceptRecs );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnException: %08X\n", 
                RpcExceptionCode() );
        }

        return mode;
    }

    RunMode EventCallback::OnBreakpoint( 
        IProcess* process, 
        uint32_t threadId, 
        Address address, 
        bool embedded )
    {
        RunMode mode = RunMode_Run;

        __try
        {
            mode = (RunMode) MagoRemoteEvent_OnBreakpoint( 
                GetContextHandle(),
                process->GetId(),
                threadId,
                address,
                embedded );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnBreakpoint: %08X\n", 
                RpcExceptionCode() );
        }

        return mode;
    }

    void EventCallback::OnStepComplete( IProcess* process, uint32_t threadId )
    {
        __try
        {
            MagoRemoteEvent_OnStepComplete( 
                GetContextHandle(),
                process->GetId(),
                threadId );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnStepComplete: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnAsyncBreakComplete( IProcess* process, uint32_t threadId )
    {
        __try
        {
            MagoRemoteEvent_OnAsyncBreak( 
                GetContextHandle(),
                process->GetId(),
                threadId );
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnAsyncBreakComplete: %08X\n", 
                RpcExceptionCode() );
        }
    }

    void EventCallback::OnError( IProcess* process, HRESULT hrErr, EventCode event )
    {
        UNREFERENCED_PARAMETER( process );
        UNREFERENCED_PARAMETER( hrErr );
        UNREFERENCED_PARAMETER( event );
    }

    ProbeRunMode EventCallback::OnCallProbe( 
        IProcess* process, uint32_t threadId, Address address, AddressRange& thunkRange )
    {
        ProbeRunMode mode = ProbeRunMode_Run;

        __try
        {
            MagoRemote_AddressRange eventThunkRange = { 0 };

            mode = (ProbeRunMode) MagoRemoteEvent_OnCallProbe( 
                GetContextHandle(),
                process->GetId(),
                threadId,
                address,
                &eventThunkRange );

            thunkRange.Begin = (Address) eventThunkRange.Begin;
            thunkRange.End = (Address) eventThunkRange.End;
        }
        __except ( CommonRpcExceptionFilter( RpcExceptionCode() ) )
        {
            _RPT1( _CRT_WARN, "Exception from call to MagoRemoteEvent_OnCallProbe: %08X\n", 
                RpcExceptionCode() );
        }

        return mode;
    }
}
