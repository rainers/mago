/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class DataEnv;
class TestFactory;


struct AppSettings
{
    bool    SelfTest;
    bool    PromoteTypedValue;
    bool    AllowAssignment;
    bool    TempAssignment;

    // TODO: I don't like this
    DataEnv*        TestEvalDataEnv;
    TestFactory*    TestElemFactory;
};


extern AppSettings  gAppSettings;
