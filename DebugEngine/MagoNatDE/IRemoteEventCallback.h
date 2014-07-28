/*
   Copyright (c) 2014 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


typedef UINT64 MagoRemote_Address;
typedef struct MagoRemote_ThreadInfo MagoRemote_ThreadInfo;
typedef struct MagoRemote_ModuleInfo MagoRemote_ModuleInfo;
typedef struct MagoRemote_AddressRange MagoRemote_AddressRange;
typedef struct MagoRemote_ExceptionRecord MagoRemote_ExceptionRecord;
enum MagoRemote_RunMode;
enum MagoRemote_ProbeRunMode;


namespace Mago
{
    class IRemoteEventCallback
    {
    public:
        virtual void AddRef() = 0;
        virtual void Release() = 0;

        virtual const GUID& GetSessionGuid() = 0;
        virtual void SetEventLogicalThread( bool beginThread ) = 0;

        virtual void OnProcessStart( uint32_t pid ) = 0;
        virtual void OnProcessExit( uint32_t pid, DWORD exitCode ) = 0;
        virtual void OnThreadStart( uint32_t pid, MagoRemote_ThreadInfo* thread ) = 0;
        virtual void OnThreadExit( uint32_t pid, DWORD threadId, DWORD exitCode ) = 0;
        virtual void OnModuleLoad( uint32_t pid, MagoRemote_ModuleInfo* modInfo ) = 0;
        virtual void OnModuleUnload( uint32_t pid, MagoRemote_Address baseAddr ) = 0;
        virtual void OnOutputString( uint32_t pid, const wchar_t* outputString ) = 0;
        virtual void OnLoadComplete( uint32_t pid, DWORD threadId ) = 0;

        virtual MagoRemote_RunMode OnException( 
            uint32_t pid, 
            DWORD threadId, 
            bool firstChance, 
            unsigned int recordCount,
            MagoRemote_ExceptionRecord* exceptRecords ) = 0;

        virtual MagoRemote_RunMode OnBreakpoint( 
            uint32_t pid, uint32_t threadId, MagoRemote_Address address, bool embedded ) = 0;

        virtual void OnStepComplete( uint32_t pid, uint32_t threadId ) = 0;
        virtual void OnAsyncBreakComplete( uint32_t pid, uint32_t threadId ) = 0;

        virtual MagoRemote_ProbeRunMode OnCallProbe( 
            uint32_t pid, 
            uint32_t threadId, 
            MagoRemote_Address address, 
            MagoRemote_AddressRange* thunkRange ) = 0;
    };
}
