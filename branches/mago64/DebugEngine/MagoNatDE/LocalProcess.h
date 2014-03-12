/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "ICoreProcess.h"


namespace Mago
{
    class LocalProcess : public ICoreProcess
    {
        long                mRefCount;
        RefPtr<IProcess>    mExecProc;
        RefPtr<ArchData>    mArchData;

    public:
        LocalProcess( ArchData* archData );

        virtual void            AddRef();
        virtual void            Release();

        virtual CreateMethod    GetCreateMethod();
        virtual uint32_t        GetPid();
        virtual const wchar_t*  GetExePath();
        virtual uint16_t        GetMachineType();

        virtual ArchData*       GetArchData();
        virtual CoreProcessType GetProcessType();

        void                    Init( IProcess* execProc );
        IProcess*               GetExecProcess();

    private:
        LocalProcess( const LocalProcess& );
        LocalProcess& operator=( const LocalProcess& );
    };


    class LocalThread : public ICoreThread
    {
        long                mRefCount;
        RefPtr<::Thread>    mExecThread;

    public:
        LocalThread( ::Thread* execThread );

        virtual void            AddRef();
        virtual void            Release();

        virtual uint32_t        GetTid();
        virtual Address64       GetStartAddr();
        virtual Address64       GetTebBase();
        virtual CoreProcessType GetProcessType();

        ::Thread*               GetExecThread();

    private:
        LocalThread( const LocalThread& );
        LocalThread& operator=( const LocalThread& );
    };


    class LocalModule : public ICoreModule
    {
        long                mRefCount;
        RefPtr<IModule>     mExecMod;

    public:
        LocalModule( IModule* execModule );

        virtual void            AddRef();
        virtual void            Release();

        virtual Address64       GetImageBase();
        virtual Address64       GetPreferredImageBase();
        virtual uint32_t        GetSize();
        virtual uint16_t        GetMachine();
        virtual const wchar_t*  GetPath();

    private:
        LocalModule( const LocalModule& );
        LocalModule& operator=( const LocalModule& );
    };
}
