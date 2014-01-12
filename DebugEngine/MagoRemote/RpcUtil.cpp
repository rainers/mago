/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "RpcUtil.h"


// Procedures that the RPC runtime calls when a remote call needs dynamic memory

void* __RPC_USER midl_user_allocate( size_t size )
{
    void* p = malloc( size );
    return p;
}

void __RPC_USER midl_user_free( void* p )
{
    free( p );
}

// This function is modeled after RpcExceptionFilter declared in rpcdce.h, which is only available 
// in Windows Vista and later.

int RPC_ENTRY CommonRpcExceptionFilter( unsigned long exceptionCode )
{
    // The documentation for RpcExceptionFilter says that it handles STATUS_POSSIBLE_DEADLOCK,
    // STATUS_INSTRUCTION_MISALIGNMENT, and STATUS_HANDLE_NOT_CLOSABLE. But, these three macros 
    // are defined in ntstatus.h instead of WinNT.h, which is the usual header file. Use their 
    // values here.

    switch ( exceptionCode )
    {
    case STATUS_ACCESS_VIOLATION:
    case 0xC0000194:                    // STATUS_POSSIBLE_DEADLOCK
    case 0xC00000AA:                    // STATUS_INSTRUCTION_MISALIGNMENT
    case STATUS_DATATYPE_MISALIGNMENT:
    case STATUS_PRIVILEGED_INSTRUCTION:
    case STATUS_ILLEGAL_INSTRUCTION:
    case STATUS_BREAKPOINT:
    case STATUS_STACK_OVERFLOW:
    case 0xC0000235:                    // STATUS_HANDLE_NOT_CLOSABLE
    case STATUS_IN_PAGE_ERROR:
    case STATUS_ASSERTION_FAILURE:
    case STATUS_STACK_BUFFER_OVERRUN:
    case STATUS_GUARD_PAGE_VIOLATION:
    case STATUS_REG_NAT_CONSUMPTION:
        return EXCEPTION_CONTINUE_SEARCH;

    default:
        return EXCEPTION_EXECUTE_HANDLER;
    }
}
