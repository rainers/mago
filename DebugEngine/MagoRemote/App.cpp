/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "App.h"


enum
{
    ConnectTimeoutMillis = 30 * 1000,
    ConnectTimeoutErrorCode = -1,
};


namespace Mago
{
    int RunSingleSessionLoop( const wchar_t* sessionUuidStr )
    {
        MSG         msg;
        UINT_PTR    timerId = 0;

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
