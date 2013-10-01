/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

class Thread;


enum ExpectedCode
{
    Expect_None,
    Expect_SS,
    Expect_BP,
};

enum Motion
{
    Motion_None,
    Motion_StepIn,
    Motion_StepOver,
    Motion_RangeStepIn,
    Motion_RangeStepOver
};

enum NotifyAction
{
    NotifyNone,
    NotifyRun,
    NotifyStepComplete,
    NotifyTrigger,
    NotifyCheckRange,
    NotifyCheckCall,
    NotifyStepOut
};

struct ExpectedEvent
{
    AddressRange    Range;
    Address         BPAddress;
    Address         UnpatchedAddress;
    int             NotifyAction;
    Motion          Motion;
    ExpectedCode    Code;
    bool            PatchBP;
    bool            ResumeThreads;
    bool            ClearTF;
    bool            RemoveBP;
};

class ThreadX86Base
{
    Thread*         mExecThread;
    ExpectedEvent   mExpectedEvents[2];
    int             mExpectedCount;

public:
    ThreadX86Base( Thread* execThread );
    virtual ~ThreadX86Base();

    int             GetExpectedCount();
    ExpectedEvent*  GetTopExpected();
    ExpectedEvent*  PushExpected( ExpectedCode code, int notifier );
    void            PopExpected();

    Thread*         GetExecThread();
};
