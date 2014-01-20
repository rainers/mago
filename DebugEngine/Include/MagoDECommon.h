/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


#define AGENT_STARTUP_EVENT_PREFIX              L"MagoRemoteStartup"
#define AGENT_LOCAL_PROTOCOL_SEQUENCE           L"ncalrpc"
#define AGENT_CMD_IF_LOCAL_ENDPOINT_PREFIX      L"MagoRemoteCmd"
#define AGENT_EVENT_IF_LOCAL_ENDPOINT_PREFIX    L"MagoRemoteEvent"

enum
{
    GUID_LENGTH = 38,

    // The event's name's format is MagoRemoteStartup<GUID>
    // example: MagoRemoteStartup{00000000-0000-0000-0000-000000000000}
    AgentStartupEventNameLength = _countof( AGENT_STARTUP_EVENT_PREFIX ) - 1 + GUID_LENGTH,
    AgentStartupTimeoutMillis = 30 * 1000,
};

#ifndef PF_XSAVE_ENABLED // not available before SDK 7.0
#define PF_XSAVE_ENABLED 17
#endif

namespace Mago
{
    enum ProcFeaturesX86
    {
        PF_X86_None         = 0,
        PF_X86_MMX          = 1,
        PF_X86_3DNow        = 2,
        PF_X86_SSE          = 4,
        PF_X86_SSE2         = 8,
        PF_X86_SSE3         = 0x10,
        PF_X86_AVX          = 0x20,
    };
}
