/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "MagoRemoteCmd_h.h"


// The RPC runtime will call this function, if the connection to the client is lost.
void __RPC_USER HCTXCMD_rundown( HCTXCMD hContext )
{
    MagoRemoteCmd_Close( &hContext );
}

HRESULT MagoRemoteCmd_Open( 
    /* [in] */ handle_t hBinding,
    /* [in] */ GUID *sessionUuid,
    /* [out] */ HCTXCMD *phContext)
{
    return E_NOTIMPL;
}

void MagoRemoteCmd_Close( 
    /* [out][in] */ HCTXCMD *phContext)
{
}

HRESULT MagoRemoteCmd_Launch( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ MagoRemote_LaunchInfo *launchInfo,
    /* [out] */ MagoRemote_ProcInfo *procInfo)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Attach( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [out] */ MagoRemote_ProcInfo *procInfo)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Terminate( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Detach( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_ResumeProcess( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_ReadMemory( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address,
    /* [in] */ unsigned int length,
    /* [out] */ unsigned int *lengthRead,
    /* [out] */ unsigned int *lengthUnreadable,
    /* [out][size_is] */ byte *buffer)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_WriteMemory( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address,
    /* [in] */ unsigned int length,
    /* [out] */ unsigned int *lengthWritten,
    /* [in][size_is] */ byte *buffer)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_SetBreakpoint( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_RemoveBreakpoint( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address address)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_StepOut( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address targetAddr,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_StepInstruction( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean stepIn,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_StepRange( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean stepIn,
    /* [in] */ MagoRemote_AddressRange range,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Continue( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_Execute( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ boolean handleException)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_AsyncBreak( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_GetThreadContext( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int tid,
    /* [in] */ unsigned int mainFeatureMask,
    /* [in] */ unsigned __int64 extFeatureMask,
    /* [in] */ unsigned int size,
    /* [out] */ unsigned int *sizeRead,
    /* [out][length_is][size_is] */ byte *regBuffer)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT MagoRemoteCmd_SetThreadContext( 
    /* [in] */ HCTXCMD hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int tid,
    /* [in] */ unsigned int size,
    /* [in][size_is] */ byte *regBuffer)
{
    if ( hContext == NULL )
        return E_INVALIDARG;

    return E_NOTIMPL;
}
