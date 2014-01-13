/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    // Notify the application that a debugging session was opened or closed.
    void NotifyAddSession();
    void NotifyRemoveSession();

    // Returns true only if opening a new debugging session is allowed.
    bool NewSession();

    int RunSingleSessionLoop( const wchar_t* sessionUuidStr );
}
