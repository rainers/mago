/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "App.h"
#include "RemoteCmdRpc.h"
#include <MagoDECommon.h>


enum
{
    WM_MAGO_SESSION_ADDED = WM_USER + 1,
    WM_MAGO_SESSION_DELETED = WM_USER + 2,

    ConnectTimeoutMillis = AgentStartupTimeoutMillis,
    ConnectTimeoutErrorCode = -1,
};

UINT32  gMainTid;
LONG    gStartedExclusiveSession;


namespace Mago
{
    HRESULT InitSingleSessionLoop( const wchar_t* sessionUuidStr )
    {
        HRESULT     hr = S_OK;
        HandlePtr   hEventPtr;
        wchar_t     eventName[AgentStartupEventNameLength + 1] = AGENT_STARTUP_EVENT_PREFIX;

        wcscat_s( eventName, sessionUuidStr );

        hEventPtr = CreateEvent( NULL, TRUE, FALSE, eventName );
        if ( hEventPtr.IsEmpty() )
            return HRESULT_FROM_WIN32( GetLastError() );

        hr = InitRpcServerLocal( sessionUuidStr );
        if ( FAILED( hr ) )
            return hr;

        // after starting to listen, notify our client that it can connect
        SetEvent( hEventPtr );

        return S_OK;
    }

    int RunSingleSessionLoop( const wchar_t* sessionUuidStr )
    {
        HRESULT     hr = S_OK;
        MSG         msg;
        int         sessionCount = 0;
        UINT_PTR    timerId = 0;

        // Make these available for as soon as the server starts listening for calls
        gMainTid = GetCurrentThreadId();

        hr = InitSingleSessionLoop( sessionUuidStr );
        if ( FAILED( hr ) )
            return hr;

        // force the OS to make a message queue
        PeekMessage( &msg, NULL, 0, 0, FALSE );

        timerId = SetTimer( NULL, 1, ConnectTimeoutMillis, NULL );
        if ( timerId == 0 )
            return HRESULT_FROM_WIN32( GetLastError() );

        while ( GetMessage( &msg, NULL, 0, 0 ) )
        {
            if ( msg.hwnd == NULL )
            {
                switch ( msg.message )
                {
                case WM_TIMER:
                    KillTimer( NULL, timerId );
                    timerId = 0;
                    if ( NewSession() )
                    {
                        // Because NewSession just claimed the only debugging session, it means 
                        // that no other debugging session was opened, no other session can be 
                        // opened, and time ran out. So, quit.
                        PostQuitMessage( ConnectTimeoutErrorCode );
                    }
                    break;

                case WM_MAGO_SESSION_ADDED:
                    sessionCount++;
                    break;

                case WM_MAGO_SESSION_DELETED:
                    sessionCount--;
                    if ( sessionCount == 0 )
                    {
                        PostQuitMessage( 0 );
                    }
                    break;
                }
            }
        }

        if ( timerId != 0 )
            KillTimer( NULL, timerId );

        UninitRpcServer();

        return msg.wParam;
    }

    void NotifyAddSession()
    {
        PostThreadMessage( gMainTid, WM_MAGO_SESSION_ADDED, 0, 0 );
    }

    void NotifyRemoveSession()
    {
        PostThreadMessage( gMainTid, WM_MAGO_SESSION_DELETED, 0, 0 );
    }

    bool NewSession()
    {
        // Allow only one debugging session to be opened
        UINT32 oldVal = InterlockedExchange( &gStartedExclusiveSession, 1 );
        return oldVal == 0;
    }
}
