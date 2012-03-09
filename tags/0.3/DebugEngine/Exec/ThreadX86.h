/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

class IStepper;
class Thread;


class ThreadX86Base
{
    Thread*         mExecThread;
    IStepper*       mStepper;
    IStepper*       mResumeStepper;

public:
    ThreadX86Base( Thread* execThread );
    virtual ~ThreadX86Base();

    Thread*         GetExecThread();

    BPCookie        GetStepperCookie();
    BPCookie        GetResumeStepperCookie();

    IStepper*       GetStepper();
    IStepper*       GetResumeStepper();
    void            SetStepper( IStepper* stepper );
    void            SetResumeStepper( IStepper* stepper );

    static uint32_t GetCookieThreadId( BPCookie cookie );
};
