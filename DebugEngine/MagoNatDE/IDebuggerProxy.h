/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class IRegisterSet;
    class ICoreProcess;
    class ICoreThread;


    class IDebuggerProxy
    {
    public:
        virtual HRESULT Launch( LaunchInfo* launchInfo, ICoreProcess*& process ) = 0;
        virtual HRESULT Attach( uint32_t id, ICoreProcess*& process ) = 0;

        virtual HRESULT Terminate( ICoreProcess* process ) = 0;
        virtual HRESULT Detach( ICoreProcess* process ) = 0;

        virtual HRESULT ResumeLaunchedProcess( ICoreProcess* process ) = 0;

        virtual HRESULT ReadMemory( 
            ICoreProcess* process, 
            Address64 address,
            uint32_t length, 
            uint32_t& lengthRead, 
            uint32_t& lengthUnreadable, 
            uint8_t* buffer ) = 0;

        virtual HRESULT WriteMemory( 
            ICoreProcess* process, 
            Address64 address,
            uint32_t length, 
            uint32_t& lengthWritten, 
            uint8_t* buffer ) = 0;

        virtual HRESULT SetBreakpoint( ICoreProcess* process, Address64 address ) = 0;
        virtual HRESULT RemoveBreakpoint( ICoreProcess* process, Address64 address ) = 0;

        virtual HRESULT StepOut( ICoreProcess* process, Address64 targetAddr, bool handleException ) = 0;
        virtual HRESULT StepInstruction( ICoreProcess* process, bool stepIn, bool handleException ) = 0;
        virtual HRESULT StepRange( 
            ICoreProcess* process, bool stepIn, AddressRange64 range, bool handleException ) = 0;

        virtual HRESULT Continue( ICoreProcess* process, bool handleException ) = 0;
        virtual HRESULT Execute( ICoreProcess* process, bool handleException ) = 0;

        virtual HRESULT AsyncBreak( ICoreProcess* process ) = 0;

        virtual HRESULT GetThreadContext( ICoreProcess* process, ICoreThread* thread, IRegisterSet*& regSet ) = 0;
        virtual HRESULT SetThreadContext( ICoreProcess* process, ICoreThread* thread, IRegisterSet* regSet ) = 0;
    };
}
