/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

class Exec;
class EventCallbackBase;


class EventSuite : public Test::Suite
{
    Exec*               mExec;
    EventCallbackBase*  mCallback;
    IMachine*           mMachine;

public:
    EventSuite();

    void setup();
    void tear_down();

private:
    void TestModuleLoad();
    void TestOutputString();
    void TestExceptionHandledFirstChance();
    void TestExceptionNotHandledFirstChanceNotCaught();
    void TestExceptionNotHandledFirstChanceCaught();
    void TestExceptionNotHandledAllChances();

    void TryHandlingException( bool firstTimeHandled, bool expectedChanceSecondTime );
    void RunDebuggee( RefPtr<IProcess>& process );
    void AssertModuleLoads( IProcess* process );
    void AssertOutputStrings();
};
