/*
   Copyright (c) 2014 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "MachineX86Base.h"


class MachineX64 : public MachineX86Base
{
    // A cached context for the thread that reported an event
    CONTEXT         mContext;
    bool            mIsContextCached;
    bool            mEnableSS;

public:
    MachineX64();

protected:
    virtual bool Is64Bit();
    virtual HRESULT CacheThreadContext();
    virtual HRESULT FlushThreadContext();
    virtual HRESULT ChangeCurrentPC( int32_t byteOffset );
    virtual HRESULT SetSingleStep( bool enable );
    virtual HRESULT ClearSingleStep();
    virtual HRESULT GetCurrentPC( Address& address );
    virtual HRESULT GetReturnAddress( Address& address );

    virtual HRESULT SuspendThread( Thread* thread );
    virtual HRESULT ResumeThread( Thread* thread );

    virtual HRESULT GetThreadContextInternal( 
        uint32_t threadId, 
        uint32_t features, 
        uint64_t extFeatures, 
        void* context, 
        uint32_t size );
    virtual HRESULT SetThreadContextInternal( uint32_t threadId, const void* context, uint32_t size );

    virtual ThreadControlProc GetWinSuspendThreadProc();

private:
    HRESULT GetThreadContextWithCache( HANDLE hThread, void* context, uint32_t size );
    HRESULT SetThreadContextWithCache( HANDLE hThread, const void* context, uint32_t size );
};


HRESULT MakeMachineX64( IMachine*& machine );
