/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ThreadX86.h"
#include "Stepper.h"
#include "Thread.h"


const uint32_t  StepperMarker = 1;
const uint32_t  ResumeStepperMarker = 2;


ThreadX86Base::ThreadX86Base( Thread* execThread )
:   mExecThread( execThread ),
    mStepper( NULL ),
    mResumeStepper( NULL )
{
    if ( mExecThread != NULL )
        mExecThread->AddRef();
}

ThreadX86Base::~ThreadX86Base()
{
    delete mStepper;
    delete mResumeStepper;

    if ( mExecThread != NULL )
        mExecThread->Release();
}

Thread*     ThreadX86Base::GetExecThread()
{
    return mExecThread;
}

BPCookie    ThreadX86Base::GetStepperCookie()
{
    return (mExecThread->GetId() | ((BPCookie) StepperMarker << 32));
}

BPCookie    ThreadX86Base::GetResumeStepperCookie()
{
    return (mExecThread->GetId() | ((BPCookie) ResumeStepperMarker << 32));
}

IStepper*   ThreadX86Base::GetStepper()
{
    return mStepper;
}

IStepper*   ThreadX86Base::GetResumeStepper()
{
    return mResumeStepper;
}

void        ThreadX86Base::SetStepper( IStepper* stepper )
{
    mStepper = stepper;
}

void        ThreadX86Base::SetResumeStepper( IStepper* stepper )
{
    mResumeStepper = stepper;
}

uint32_t ThreadX86Base::GetCookieThreadId( BPCookie cookie )
{
    return cookie & 0xFFFFFFFF;
}
