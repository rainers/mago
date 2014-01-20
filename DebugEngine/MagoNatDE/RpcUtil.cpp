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

// The documentation for RpcExceptionFilter says that it handles STATUS_POSSIBLE_DEADLOCK,
// STATUS_INSTRUCTION_MISALIGNMENT, and STATUS_HANDLE_NOT_CLOSABLE. But, these three macros 
// are defined in ntstatus.h instead of WinNT.h, which is the usual header file. Use their 
// values here.

#if !defined( STATUS_POSSIBLE_DEADLOCK )
#define STATUS_POSSIBLE_DEADLOCK            0xC0000194
#endif

#if !defined( STATUS_INSTRUCTION_MISALIGNMENT )
#define STATUS_INSTRUCTION_MISALIGNMENT     0xC00000AA
#endif

#if !defined( STATUS_HANDLE_NOT_CLOSABLE )
#define STATUS_HANDLE_NOT_CLOSABLE          0xC0000235
#endif

#if !defined( STATUS_STACK_BUFFER_OVERRUN )
#define STATUS_STACK_BUFFER_OVERRUN         0xC0000409
#endif

#if !defined( STATUS_ASSERTION_FAILURE )
#define STATUS_ASSERTION_FAILURE            0xC0000420
#endif

// This function is modeled after RpcExceptionFilter declared in rpcdce.h, which is only available 
// in Windows Vista and later.

int RPC_ENTRY CommonRpcExceptionFilter( unsigned long exceptionCode )
{
    switch ( exceptionCode )
    {
    case STATUS_ACCESS_VIOLATION:
    case STATUS_POSSIBLE_DEADLOCK:
    case STATUS_INSTRUCTION_MISALIGNMENT:
    case STATUS_DATATYPE_MISALIGNMENT:
    case STATUS_PRIVILEGED_INSTRUCTION:
    case STATUS_ILLEGAL_INSTRUCTION:
    case STATUS_BREAKPOINT:
    case STATUS_STACK_OVERFLOW:
    case STATUS_HANDLE_NOT_CLOSABLE:
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
