/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "Common.h"
#include "MakeMachine.h"

#if defined( _M_IX86 )
#include "MachineX86.h"
#elif defined( _M_X64 )
#include "MachineX86.h"
#include "MachineX64.h"
#endif


HRESULT MakeMachine( WORD machineType, IMachine*& machine )
{
    HRESULT hr = E_NOTIMPL;

#if defined( _M_IX86 )
    if ( machineType == IMAGE_FILE_MACHINE_I386 )
    {
        hr = MakeMachineX86( machine );
    }
#elif defined( _M_X64 )
    if (machineType == IMAGE_FILE_MACHINE_I386)
    {
        hr = MakeMachineX86(machine);
    }
    else if (machineType == IMAGE_FILE_MACHINE_AMD64)
    {
        hr = MakeMachineX64(machine);
    }
#elif defined( _M_ARM64 )
    if (machineType == IMAGE_FILE_MACHINE_ARM64)
    {
// todo:        hr = MakeMachineARM64(machine);
    }
#else
#error Mago doesn't implement a core debugger for the current architecture.
#endif

    return hr;
}
