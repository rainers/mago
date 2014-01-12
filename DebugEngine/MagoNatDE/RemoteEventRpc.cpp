/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "MagoRemoteEvent_i.h"


// The RPC runtime will call this function, if the connection to the client is lost.
void __RPC_USER HCTXEVENT_rundown( HCTXEVENT hContext )
{
    MagoRemoteEvent_Close( &hContext );
}

HRESULT MagoRemoteEvent_Open( 
    /* [in] */ handle_t hBinding,
    /* [in] */ GUID *sessionUuid,
    /* [out] */ HCTXEVENT *phContext)
{
    return E_NOTIMPL;
}

void MagoRemoteEvent_Close( 
    /* [out][in] */ HCTXEVENT *phContext)
{
}

void MagoRemoteEvent_OnProcessStart( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid)
{
}

void MagoRemoteEvent_OnProcessExit( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ DWORD exitCode)
{
}

void MagoRemoteEvent_OnThreadStart( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_ThreadInfo *thread)
{
}

void MagoRemoteEvent_OnThreadExit( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ DWORD threadId,
    /* [in] */ DWORD exitCode)
{
}

void MagoRemoteEvent_OnModuleLoad( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_ModuleInfo *modInfo)
{
}

void MagoRemoteEvent_OnModuleUnload( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ MagoRemote_Address baseAddress)
{
}

void MagoRemoteEvent_OnOutputString( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [string][in] */ const wchar_t *outputString)
{
}

void MagoRemoteEvent_OnLoadComplete( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ DWORD threadId)
{
}

MagoRemote_RunMode MagoRemoteEvent_OnException( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ DWORD threadId,
    /* [in] */ boolean firstChance,
    /* [in] */ unsigned int recordCount,
    /* [in][size_is] */ MagoRemote_ExceptionRecord *exceptRecords)
{
    return MagoRemote_RunMode_Run;
}

MagoRemote_RunMode MagoRemoteEvent_OnBreakpoint( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int threadId,
    /* [in] */ MagoRemote_Address address,
    /* [in] */ boolean embedded)
{
    return MagoRemote_RunMode_Run;
}

void MagoRemoteEvent_OnStepComplete( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int threadId)
{
}

void MagoRemoteEvent_OnAsyncBreak( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int threadId)
{
}

MagoRemote_ProbeRunMode MagoRemoteEvent_OnCallProbe( 
    /* [in] */ HCTXEVENT hContext,
    /* [in] */ unsigned int pid,
    /* [in] */ unsigned int threadId,
    /* [in] */ MagoRemote_Address address,
    /* [out] */ MagoRemote_AddressRange *thunkRange)
{
    return MagoRemote_PRunMode_Run;
}
