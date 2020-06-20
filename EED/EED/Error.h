/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace MagoEE
{
    enum
    {
        HR_FACILITY = 0x1613
    };
}


enum
{
    E_MAGOEE_BASE = MAKE_HRESULT( SEVERITY_ERROR, MagoEE::HR_FACILITY, 0 ),

    // needs to be in sync with gErrStrs[]
    E_MAGOEE_SYNTAX_ERROR,
    E_MAGOEE_BAD_TYPES_FOR_OP,
    E_MAGOEE_VALUE_EXPECTED,
    E_MAGOEE_NO_TYPE,
    E_MAGOEE_TYPE_RESOLVE_FAILED,
    E_MAGOEE_BAD_CAST,
    E_MAGOEE_NO_ADDRESS,
    E_MAGOEE_LVALUE_EXPECTED,
    E_MAGOEE_BAD_BOOL_CAST,
    E_MAGOEE_DIVIDE_BY_ZERO,
    E_MAGOEE_BAD_INDEX,
    E_MAGOEE_SYMBOL_NOT_FOUND,
    E_MAGOEE_ELEMENT_NOT_FOUND,
    E_MAGOEE_TOO_MANY_ARGUMENTS,
    E_MAGOEE_TOO_FEW_ARGUMENTS,
    E_MAGOEE_CALL_NOT_IMPLEMENTED,
    E_MAGOEE_CALLARGS_NOT_IMPLEMENTED,
    E_MAGOEE_BADREGISTER,
    E_MAGOEE_BADCALLCONV,
    E_MAGOEE_NOFUNCCALL,
    E_MAGOEE_HASSIDEEFFECT,
    E_MAGOEE_CANNOTALLOCTRAMP,
    E_MAGOEE_INVALID_MAGOGC,
    E_MAGOEE_CALLFAILED,
    E_MAGOEE_DivideByZero, // integrates DkmILFailureReason as (E_MAGOEE_CALLFAILED + reason)
    E_MAGOEE_MemoryReadError,
    E_MAGOEE_MemoryWriteError,
    E_MAGOEE_RegisterReadError,
    E_MAGOEE_RegisterWriteError,
    E_MAGOEE_Aborted,
    E_MAGOEE_StringTooLong,
    E_MAGOEE_Timeout,
    E_MAGOEE_TooManyFuncEval,
    E_MAGOEE_AbortFailed,
    E_MAGOEE_MinidumpNotSupported,
    E_MAGOEE_AbortUnhandledException,
    E_MAGOEE_UserModeScheduledNotSupported,
    E_MAGOEE_ByteExtractionOutOfBounds,
    E_MAGOEE_InvalidPseudoAddressOperation,
    E_MAGOEE_UnknownFuncEvalError,
    E_MAGOEE_AttemptedToCrossEnclaveBoundaries,

    E_MAGOEE_NUMERRORS
    // TODO: maybe something like RUNTIME_ERROR or DEBUG_INFO_ERROR
    //  for cases like this: Decl->GetType( _Type.Ref() )   (see ThisExpr::Semantic)
};
