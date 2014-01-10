/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


#define AGENT_STARTUP_EVENT_PREFIX    L"MagoRemoteStartup"

enum
{
    GUID_LENGTH = 38,

    // The event's name's format is MagoRemoteStartup<GUID>
    // example: MagoRemoteStartup{00000000-0000-0000-0000-000000000000}
    AgentStartupEventNameLength = _countof( AGENT_STARTUP_EVENT_PREFIX ) - 1 + GUID_LENGTH,
    AgentStartupTimeoutMillis = 30 * 1000,
};
