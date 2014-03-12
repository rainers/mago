/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Common.h"
#include "LocalProcess.h"
#include "ArchData.h"


namespace Mago
{
    //------------------------------------------------------------------------
    //  LocalProcess
    //------------------------------------------------------------------------

    LocalProcess::LocalProcess( ArchData* archData )
        :   mRefCount( 0 ),
            mArchData( archData )
    {
        _ASSERT( archData != NULL );
    }

    void LocalProcess::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void LocalProcess::Release()
    {
        long ref = InterlockedDecrement( &mRefCount );
        _ASSERT( ref >= 0 );
        if ( ref == 0 )
        {
            delete this;
        }
    }

    CreateMethod LocalProcess::GetCreateMethod()
    {
        return mExecProc->GetCreateMethod();
    }

    uint32_t LocalProcess::GetPid()
    {
        return mExecProc->GetId();
    }

    const wchar_t* LocalProcess::GetExePath()
    {
        return mExecProc->GetExePath();
    }

    uint16_t LocalProcess::GetMachineType()
    {
        return mExecProc->GetMachineType();
    }

    ArchData* LocalProcess::GetArchData()
    {
        return mArchData.Get();
    }

    CoreProcessType LocalProcess::GetProcessType()
    {
        return CoreProcess_Local;
    }

    void LocalProcess::Init( IProcess* execProc )
    {
        _ASSERT( execProc != NULL );
        mExecProc = execProc;
    }

    IProcess* LocalProcess::GetExecProcess()
    {
        return mExecProc;
    }


    //------------------------------------------------------------------------
    //  LocalThread
    //------------------------------------------------------------------------

    LocalThread::LocalThread( ::Thread* execThread )
        :   mRefCount( 0 ),
            mExecThread( execThread )
    {
        _ASSERT( execThread != NULL );
    }

    void LocalThread::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void LocalThread::Release()
    {
        long ref = InterlockedDecrement( &mRefCount );
        _ASSERT( ref >= 0 );
        if ( ref == 0 )
        {
            delete this;
        }
    }

    uint32_t LocalThread::GetTid()
    {
        return mExecThread->GetId();
    }

    Address64    LocalThread::GetStartAddr()
    {
        return mExecThread->GetStartAddr();
    }

    Address64    LocalThread::GetTebBase()
    {
        return mExecThread->GetTebBase();
    }

    CoreProcessType LocalThread::GetProcessType()
    {
        return CoreProcess_Local;
    }

    ::Thread* LocalThread::GetExecThread()
    {
        return mExecThread;
    }


    //------------------------------------------------------------------------
    //  LocalModule
    //------------------------------------------------------------------------

    LocalModule::LocalModule( IModule* execModule )
        :   mRefCount( 0 ),
            mExecMod( execModule )
    {
        _ASSERT( execModule != NULL );
    }

    void LocalModule::AddRef()
    {
        InterlockedIncrement( &mRefCount );
    }

    void LocalModule::Release()
    {
        long ref = InterlockedDecrement( &mRefCount );
        _ASSERT( ref >= 0 );
        if ( ref == 0 )
        {
            delete this;
        }
    }

    Address64        LocalModule::GetImageBase()
    {
        return mExecMod->GetImageBase();
    }

    Address64        LocalModule::GetPreferredImageBase()
    {
        return mExecMod->GetPreferredImageBase();
    }

    uint32_t LocalModule::GetSize()
    {
        return mExecMod->GetSize();
    }

    uint16_t LocalModule::GetMachine()
    {
        return mExecMod->GetMachine();
    }

    const wchar_t* LocalModule::GetPath()
    {
        return mExecMod->GetPath();
    }
}
