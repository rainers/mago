/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    struct CommandFunctor
    {
        virtual void    Run() = 0;
    };

    struct ExecCommandFunctor : public CommandFunctor
    {
        Exec&   Core;
        HRESULT OutHResult;

        ExecCommandFunctor( Exec& exec )
            :   Core( exec ),
                OutHResult( E_FAIL )
        {
        }
    };

    struct LaunchParams : public ExecCommandFunctor
    {
        LaunchInfo*         Settings;
        RefPtr<IProcess>    OutProcess;

        LaunchParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Settings( NULL ),
                OutProcess( NULL )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.Launch( Settings, OutProcess.Ref() );
        }
    };

    struct AttachParams : public ExecCommandFunctor
    {
        uint32_t            ProcessId;
        RefPtr<IProcess>    OutProcess;

        AttachParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                ProcessId( 0 ),
                OutProcess( NULL )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.Attach( ProcessId, OutProcess.Ref() );
        }
    };

    struct TerminateParams : public ExecCommandFunctor
    {
        IProcess*       Process;

        TerminateParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.Terminate( Process );
        }
    };

    struct DetachParams : public ExecCommandFunctor
    {
        IProcess*       Process;

        DetachParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.Detach( Process );
        }
    };

    struct ResumeLaunchedProcessParams : public ExecCommandFunctor
    {
        IProcess*       Process;

        ResumeLaunchedProcessParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.ResumeLaunchedProcess( Process );
        }
    };

    struct ReadMemoryParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        Address         Address;
        uint8_t*        Buffer;
        SIZE_T          Length;
        SIZE_T          OutLengthRead;
        SIZE_T          OutLengthUnreadable;

        ReadMemoryParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                Address( 0 ),
                Buffer( NULL ),
                Length( 0 )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.ReadMemory( Process, Address, Length, OutLengthRead, OutLengthUnreadable, Buffer );
        }
    };

    struct WriteMemoryParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        Address         Address;
        uint8_t*        Buffer;
        SIZE_T          Length;
        SIZE_T          OutLengthWritten;

        WriteMemoryParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                Address( 0 ),
                Buffer( NULL ),
                Length( 0 )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.WriteMemory( Process, Address, Length, OutLengthWritten, Buffer );
        }
    };

    struct SetBreakpointParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        Address         Address;

        SetBreakpointParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                Address( 0 )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.SetBreakpoint( Process, Address );
        }
    };

    struct RemoveBreakpointParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        Address         Address;

        RemoveBreakpointParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                Address( 0 )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.RemoveBreakpoint( Process, Address );
        }
    };

    struct StepOutParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        Address         TargetAddress;
        bool            HandleException;

        StepOutParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                TargetAddress( 0 ),
                HandleException( false )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.StepOut( Process, TargetAddress, HandleException );
        }
    };

    struct StepInstructionParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        bool            StepIn;
        bool            HandleException;

        StepInstructionParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                StepIn( false ),
                HandleException( false )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.StepInstruction( Process, StepIn, HandleException );
        }
    };

    struct StepRangeParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        bool            StepIn;
        AddressRange    Range;
        bool            HandleException;

        StepRangeParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                StepIn( false ),
                HandleException( false )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.StepRange( Process, StepIn, Range, HandleException );
        }
    };

    struct ContinueParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        bool            HandleException;

        ContinueParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                HandleException( false )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.Continue( Process, HandleException );
        }
    };

    struct ExecuteParams : public ExecCommandFunctor
    {
        IProcess*       Process;
        bool            HandleException;

        ExecuteParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL ),
                HandleException( false )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.CancelStep( Process );

            if ( SUCCEEDED( OutHResult ) )
                OutHResult = Core.Continue( Process, HandleException );
        }
    };

    struct AsyncBreakParams : public ExecCommandFunctor
    {
        IProcess*       Process;

        AsyncBreakParams( Exec& exec )
            :   ExecCommandFunctor( exec ),
                Process( NULL )
        {
        }

        virtual void    Run()
        {
            OutHResult = Core.AsyncBreak( Process );
        }
    };
}
