/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "MachineX86Base.h"


class MachineX86 : public MachineX86Base
{
protected:
    virtual HRESULT ChangeCurrentPC( uint32_t threadId, int32_t byteOffset );
    virtual HRESULT SetSingleStep( uint32_t threadId, bool enable );
    virtual HRESULT GetCurrentPC( uint32_t threadId, MachineAddress& address );

    virtual HRESULT SuspendThread( Thread* thread );
    virtual HRESULT ResumeThread( Thread* thread );

    virtual HRESULT GetThreadContextInternal( uint32_t threadId, void* context, uint32_t size );
    virtual HRESULT SetThreadContextInternal( uint32_t threadId, const void* context, uint32_t size );

    virtual ThreadControlProc GetWinSuspendThreadProc();
};


HRESULT MakeMachineX86( IMachine*& machine );
