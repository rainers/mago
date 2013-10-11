/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "ThreadX86.h"
#include "Thread.h"


ThreadX86Base::ThreadX86Base( Thread* execThread )
:   mExecThread( execThread ),
    mExpectedCount( 0 )
{
    if ( mExecThread != NULL )
        mExecThread->AddRef();
}

ThreadX86Base::~ThreadX86Base()
{
    if ( mExecThread != NULL )
        mExecThread->Release();
}

Thread*     ThreadX86Base::GetExecThread()
{
    return mExecThread;
}

int ThreadX86Base::GetExpectedCount()
{
    return mExpectedCount;
}

ExpectedEvent* ThreadX86Base::GetTopExpected()
{
    if ( mExpectedCount == 0 )
        return NULL;

    return &mExpectedEvents[mExpectedCount - 1];
}

ExpectedEvent* ThreadX86Base::PushExpected( ExpectedCode code, int notifier )
{
    _ASSERT( code == Expect_SS || code == Expect_BP );
    _ASSERT( notifier != 0 );
    _ASSERT( mExpectedCount < 2 );

    if ( mExpectedCount >= 2 )
        return NULL;

    ExpectedEvent* event = &mExpectedEvents[mExpectedCount];
    mExpectedCount++;

    memset( event, 0, sizeof *event );
    event->Code = code;
    event->NotifyAction = notifier;

    return event;
}

void ThreadX86Base::PopExpected()
{
    _ASSERT( mExpectedCount > 0 );

    if ( mExpectedCount > 0 )
        mExpectedCount--;
}
