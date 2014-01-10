/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "App.h"
#include "MagoDECommon.h"


enum
{
    ConnectTimeoutMillis = AgentStartupTimeoutMillis,
    ConnectTimeoutErrorCode = -1,
};


namespace Mago
{
    HRESULT InitSingleSessionLoop( const wchar_t* sessionUuidStr )
    {
        HandlePtr   hEventPtr;
        wchar_t     eventName[AgentStartupEventNameLength + 1] = AGENT_STARTUP_EVENT_PREFIX;

        wcscat_s( eventName, sessionUuidStr );

        hEventPtr = CreateEvent( NULL, TRUE, FALSE, eventName );
        if ( hEventPtr.IsEmpty() )
            return HRESULT_FROM_WIN32( GetLastError() );

        // after starting to listen, notify our client that it can connect
        SetEvent( hEventPtr );

        return S_OK;
    }

    int RunSingleSessionLoop( const wchar_t* sessionUuidStr )
    {
        HRESULT     hr = S_OK;
        MSG         msg;
        UINT_PTR    timerId = 0;

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
                    PostQuitMessage( ConnectTimeoutErrorCode );
                    break;
                }
            }
        }

        if ( timerId != 0 )
            KillTimer( NULL, timerId );

        return msg.wParam;
    }
}
