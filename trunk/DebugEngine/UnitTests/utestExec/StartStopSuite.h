/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

class EventCallbackBase;


class StartStopSuite : public Test::Suite
{
    Exec*               mExec;
    EventCallbackBase*  mCallback;
    IMachine*           mMachine;

public:
    StartStopSuite();

    void setup();
    void tear_down();

private:
    void TestInit();
    void TestLaunchDestroyExecStopped();
    void TestLaunchDestroyExecRunning();
    void TestBeginToEnd();
    void TestTerminateStopped();
    void TestTerminateRunning();
    void TestOptionsSameConsole();
    void TestOptionsNewConsole();

    void TryOptions( bool newConsole );
    void BuildEnv( wchar_t* env, int envSize, 
                    char* expectedEnv, int expectedEnvSize, 
                    char* requestedEnv, int requestedEnvSize );
    void MakeStdPipes( HandlePtr& inFileRead, HandlePtr& inFileWrite, 
                      HandlePtr& outFileRead, HandlePtr& outFileWrite, 
                      HandlePtr& errFileRead, HandlePtr& errFileWrite, bool& ok );

    void AssertProcessFinished( uint32_t pid );
    void AssertConsoleWindow( bool newConsole, DWORD procId );
};
