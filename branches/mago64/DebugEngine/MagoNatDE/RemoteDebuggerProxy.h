/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "IDebuggerProxy.h"


namespace Mago
{
    class EventCallback;
    class ICoreProcess;
    class ICoreThread;
    class IRegisterSet;


    class RemoteDebuggerProxy : public IDebuggerProxy
    {
        RefPtr<EventCallback>   mCallback;
        GUID                    mSessionGuid;

    public:
        RemoteDebuggerProxy();
        ~RemoteDebuggerProxy();

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
            Address address,
            uint32_t length, 
            uint32_t& lengthRead, 
            uint32_t& lengthUnreadable, 
            uint8_t* buffer );

        HRESULT WriteMemory( 
            ICoreProcess* process, 
            Address address,
            uint32_t length, 
            uint32_t& lengthWritten, 
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
    };
}
