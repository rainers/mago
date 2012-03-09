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
    virtual HRESULT Rewind( uint32_t threadId, uint32_t byteCount );
    virtual HRESULT SetSingleStep( uint32_t threadId, bool enable );
    virtual HRESULT GetCurrentPC( uint32_t threadId, MachineAddress& address );

    virtual HRESULT SuspendThread( Thread* thread );
    virtual HRESULT ResumeThread( Thread* thread );
};
