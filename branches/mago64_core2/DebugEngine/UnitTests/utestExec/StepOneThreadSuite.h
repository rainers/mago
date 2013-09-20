/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

struct Step;
class EventCallbackBase;


class StepOneThreadSuite : public Test::Suite
{
    Exec*               mExec;
    EventCallbackBase*  mCallback;

public:
    StepOneThreadSuite();

    void setup();
    void tear_down();

private:
    void StepInstructionInAssembly();
    void StepInstructionOver();
    void StepInstructionInSourceHaveSource();
    void StepInstructionInSourceNoSource();
    void StepInstructionOverInterruptedByBP();

    void RunDebuggee( Step* steps, int stepsCount );
};
