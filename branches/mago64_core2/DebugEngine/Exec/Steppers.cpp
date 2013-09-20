/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Stepper.h"
#include "MachineX86Base.h"
#include "DecodeX86.h"
#include "ThreadX86.h"

using namespace std;


HRESULT ReadInstruction( 
    IStepperMachine* machine, 
    Address curAddress, 
    CpuSizeMode cpu, 
    InstructionType& type, 
    int& size )
{
    HRESULT         hr = S_OK;
    BYTE            mem[MAX_INSTRUCTION_SIZE] = { 0 };
    SIZE_T          lenRead = 0;
    SIZE_T          lenUnreadable = 0;
    int             instLen = 0;
    InstructionType instType = Inst_None;

    hr = machine->ReadStepperMemory( curAddress, MAX_INSTRUCTION_SIZE, lenRead, lenUnreadable, mem );
    if ( FAILED( hr ) )
        return hr;

    instType = GetInstructionTypeAndSize( mem, (int) lenRead, cpu, instLen );
    if ( instType == Inst_None )
        return E_UNEXPECTED;

    size = instLen;
    type = instType;
    return S_OK;
}

HRESULT MakeResumeStepper( 
                          IStepperMachine* machine, 
                          Address curAddress, 
                          BPCookie cookie, 
                          std::auto_ptr<IStepper>& stepper )
{
    HRESULT         hr = S_OK;
    int             instLen = 0;
    InstructionType instType = Inst_None;

    hr = ReadInstruction( machine, curAddress, Cpu_32, instType, instLen );
    if ( FAILED( hr ) )
        return hr;

    if ( instType == Inst_RepString )
    {
        stepper.reset( new InstructionStepperBP( machine, instLen, curAddress, cookie ) );
    }
    else if ( instType == Inst_Breakpoint )
    {
        stepper.reset( new InstructionStepperEmbeddedBP( machine, curAddress, true, cookie ) );
    }
    else
    {
        stepper.reset( new InstructionStepperSS( machine, curAddress, cookie ) );
    }

    if ( stepper.get() == NULL )
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT MakeStepInStepper( 
                          IStepperMachine* machine, 
                          Address curAddress, 
                          bool sourceMode, 
                          BPCookie cookie, 
                          bool handleEmbeddedBP, 
                          std::auto_ptr<IStepper>& stepper )
{
    HRESULT         hr = S_OK;
    int             instLen = 0;
    InstructionType instType = Inst_None;

    hr = ReadInstruction( machine, curAddress, Cpu_32, instType, instLen );
    if ( FAILED( hr ) )
        return hr;

    if ( instType == Inst_RepString )
    {
        stepper.reset( new InstructionStepperSS( machine, curAddress, cookie ) );
    }
    else if ( instType == Inst_Call )
    {
        if ( sourceMode )
            stepper.reset( new InstructionStepperProbeCall( machine, instLen, curAddress, cookie ) );
        else
            stepper.reset( new InstructionStepperSS( machine, curAddress, cookie ) );
    }
    else if ( instType == Inst_Breakpoint )
    {
        stepper.reset( new InstructionStepperEmbeddedBP( machine, curAddress, handleEmbeddedBP, cookie ) );
    }
    else
    {
        stepper.reset( new InstructionStepperSS( machine, curAddress, cookie ) );
    }

    if ( stepper.get() == NULL )
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT MakeStepOverStepper( 
                            IStepperMachine* machine, 
                            Address curAddress, 
                            BPCookie cookie, 
                            bool handleEmbeddedBP, 
                            std::auto_ptr<IStepper>& stepper )
{
    HRESULT         hr = S_OK;
    int             instLen = 0;
    InstructionType instType = Inst_None;

    hr = ReadInstruction( machine, curAddress, Cpu_32, instType, instLen );
    if ( FAILED( hr ) )
        return hr;

    if ( instType == Inst_RepString )
    {
        stepper.reset( new InstructionStepperBP( machine, instLen, curAddress, cookie ) );
    }
    else if ( instType == Inst_Call )
    {
        stepper.reset( new InstructionStepperBPCall( machine, instLen, curAddress, cookie ) );
    }
    else if ( instType == Inst_Breakpoint )
    {
        stepper.reset( new InstructionStepperEmbeddedBP( machine, curAddress, handleEmbeddedBP, cookie ) );
    }
    else
    {
        stepper.reset( new InstructionStepperSS( machine, curAddress, cookie ) );
    }

    if ( stepper.get() == NULL )
        return E_OUTOFMEMORY;

    return S_OK;
}


//----------------------------------------------------------------------------
//  InstructionStepperSS
//----------------------------------------------------------------------------

InstructionStepperSS::InstructionStepperSS( IStepperMachine* machine, Address curAddress, BPCookie cookie )
    :   mMachine( machine ),
        mCurAddr( curAddress ),
        mCookie( cookie ),
        mIsComplete( false )
{
    _ASSERT( curAddress != 0 );
    _ASSERT( machine != NULL );
}

HRESULT         InstructionStepperSS::Start()
{
    return mMachine->SetStepperSingleStep( true );
}

HRESULT         InstructionStepperSS::Cancel()
{
    return S_OK;
}

bool            InstructionStepperSS::IsComplete()
{
    return mIsComplete;
}

bool            InstructionStepperSS::CanSkipEmbeddedBP()
{
    return false;
}

HRESULT         InstructionStepperSS::RequestStepAway( Address pc )
{
    UNREFERENCED_PARAMETER( pc );

    // we single step as the main kind of stepping, so nothing extra to do
    return S_OK;
}

void            InstructionStepperSS::SetAddress( Address address )
{
    mCurAddr = address;
}

HRESULT     InstructionStepperSS::OnBreakpoint( Address address, MachineResult& result, bool& rewindPC )
{
    UNREFERENCED_PARAMETER( address );

    result = MacRes_NotHandled;
    rewindPC = true;
    return S_OK;
}

HRESULT     InstructionStepperSS::OnSingleStep( Address address, MachineResult& result )
{
    UNREFERENCED_PARAMETER( address );

    mIsComplete = true;
    result = MacRes_HandledStopped;
    return S_OK;
}


//----------------------------------------------------------------------------
//  InstructionStepperEmbeddedBP
//----------------------------------------------------------------------------

InstructionStepperEmbeddedBP::InstructionStepperEmbeddedBP( 
    IStepperMachine* machine, 
    Address curAddress, 
    bool handleBP, 
    BPCookie cookie )
    :   mMachine( machine ),
        mStartingAddr( curAddress ),
        mCookie( cookie ),
        mIsComplete( false ),
        mHandleBPException( handleBP )
{
    _ASSERT( curAddress != 0 );
    _ASSERT( machine != NULL );
}

HRESULT         InstructionStepperEmbeddedBP::Start()
{
    return S_OK;
}

HRESULT         InstructionStepperEmbeddedBP::Cancel()
{
    return S_OK;
}

bool            InstructionStepperEmbeddedBP::IsComplete()
{
    return mIsComplete;
}

bool            InstructionStepperEmbeddedBP::CanSkipEmbeddedBP()
{
    return false;
}

HRESULT         InstructionStepperEmbeddedBP::RequestStepAway( Address pc )
{
    UNREFERENCED_PARAMETER( pc );
    return S_OK;
}

void            InstructionStepperEmbeddedBP::SetAddress( Address address )
{
    UNREFERENCED_PARAMETER( address );
}

HRESULT     InstructionStepperEmbeddedBP::OnBreakpoint( 
    Address address, 
    MachineResult& result, 
    bool& rewindPC )
{
    UNREFERENCED_PARAMETER( address );

    result = MacRes_NotHandled;

    // the point of this stepper is to go past an embedded BP instruction
    rewindPC = false;

    if ( (address == mStartingAddr) && mHandleBPException )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;
    }

    // If the user doesn't want us to handle the BP exception, it's because they want
    // to single step this instruction and trigger a breakpoint notification, without 
    // using HW SS. If you HW SS an embedded BP, you end up single stepping the
    // instruction after the embedded BP, which is almost never intended.

    return S_OK;
}

HRESULT     InstructionStepperEmbeddedBP::OnSingleStep( Address address, MachineResult& result )
{
    UNREFERENCED_PARAMETER( address );

    mIsComplete = true;
    result = MacRes_HandledStopped;
    return S_OK;
}


//----------------------------------------------------------------------------
//  InstructionStepperBP
//----------------------------------------------------------------------------

InstructionStepperBP::InstructionStepperBP( 
    IStepperMachine* machine, 
    int instLen, 
    Address curAddress, 
    BPCookie cookie )
    :   mMachine( machine ),
        mCurAddr( curAddress ),
        mCookie( cookie ),
        mIsComplete( false ),
        mInstLen( instLen ),
        mAfterCallAddr( 0 )
{
    _ASSERT( curAddress != 0 );
    _ASSERT( instLen > 0 );
    _ASSERT( machine != NULL );
}

HRESULT         InstructionStepperBP::Start()
{
    mAfterCallAddr = mCurAddr + mInstLen;

    return mMachine->SetStepperBreakpoint( mAfterCallAddr, mCookie );
}

HRESULT         InstructionStepperBP::Cancel()
{
    return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
}

bool            InstructionStepperBP::IsComplete()
{
    return mIsComplete;
}

bool            InstructionStepperBP::CanSkipEmbeddedBP()
{
    return false;
}

HRESULT         InstructionStepperBP::RequestStepAway( Address pc )
{
    UNREFERENCED_PARAMETER( pc );
    return S_OK;
}

void            InstructionStepperBP::SetAddress( Address address )
{
    mCurAddr = address;
}

HRESULT     InstructionStepperBP::OnBreakpoint( Address address, MachineResult& result, bool& rewindPC )
{
    result = MacRes_NotHandled;
    rewindPC = true;

    if ( address == mAfterCallAddr )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;
        return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
    }

    return S_OK;
}

HRESULT     InstructionStepperBP::OnSingleStep( Address address, MachineResult& result )
{
    UNREFERENCED_PARAMETER( address );

    result = MacRes_NotHandled;

    return S_OK;
}


//----------------------------------------------------------------------------
//  InstructionStepperBPCall
//----------------------------------------------------------------------------

InstructionStepperBPCall::InstructionStepperBPCall( 
    IStepperMachine* machine, 
    int instLen, 
    Address curAddress, 
    BPCookie cookie )
    :   mMachine( machine ),
        mCurAddr( curAddress ),
        mCookie( cookie ),
        mIsComplete( false ),
        mInstLen( instLen ),
        mRequestedSS( false ),
        mStartingAddr( curAddress ),
        mAfterCallAddr( 0 )
{
    _ASSERT( curAddress != 0 );
    _ASSERT( instLen > 0 );
    _ASSERT( machine != NULL );
}

HRESULT         InstructionStepperBPCall::Start()
{
    mAfterCallAddr = mCurAddr + mInstLen;

    return mMachine->SetStepperBreakpoint( mAfterCallAddr, mCookie );
}

HRESULT         InstructionStepperBPCall::Cancel()
{
    if ( mResumeStepper.get() != NULL )
    {
        mResumeStepper->Cancel();
    }

    return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
}

bool            InstructionStepperBPCall::IsComplete()
{
    return mIsComplete;
}

bool            InstructionStepperBPCall::CanSkipEmbeddedBP()
{
    if ( (mCurAddr == mAfterCallAddr)
        || (mCurAddr == mStartingAddr) )
    {
        return false;
    }

    // We might run across an embedded BP in the middle of a call.
    // In that case, to resume running the procedure and keep this stepper going,
    // allow skipping that embedded BP.

    return true;
}

HRESULT         InstructionStepperBPCall::RequestStepAway( Address pc )
{
    HRESULT hr = S_OK;

    if ( mCurAddr == mStartingAddr )
    {
        mRequestedSS = true;

        return mMachine->SetStepperSingleStep( true );
    }

    if ( mResumeStepper.get() != NULL )
    {
        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );
    }

    ThreadX86Base* thread = mMachine->GetStoppedThread();
    _ASSERT( thread != NULL );

    hr = MakeResumeStepper( mMachine, pc, thread->GetResumeStepperCookie(), mResumeStepper );
    if ( FAILED( hr ) )
        return hr;

    return S_OK;
}

void            InstructionStepperBPCall::SetAddress( Address address )
{
    mCurAddr = address;
}

HRESULT     InstructionStepperBPCall::OnBreakpoint( Address address, MachineResult& result, bool& rewindPC )
{
    HRESULT hr = S_OK;

    mCurAddr = address;
    result = MacRes_NotHandled;
    rewindPC = true;

    if ( address == mAfterCallAddr )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;

        if ( mResumeStepper.get() != NULL )
            mResumeStepper->Cancel();

        return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
    }
    else if ( mResumeStepper.get() != NULL )
    {
        // Resume steppers are used for only one step, to move past a user BP.
        // So, use it, get rid of it, and continue

        hr = mResumeStepper->OnBreakpoint( address, result, rewindPC );
        if ( FAILED( hr ) )
            return hr;

        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );

        if ( result == MacRes_HandledStopped )
            result = MacRes_HandledContinue;
    }

    return S_OK;
}

HRESULT     InstructionStepperBPCall::OnSingleStep( Address address, MachineResult& result )
{
    HRESULT hr = S_OK;

    mCurAddr = address;
    result = MacRes_NotHandled;

    if ( mRequestedSS )
    {
        mRequestedSS = false;
        result = MacRes_HandledContinue;

        if ( mResumeStepper.get() != NULL )
            mResumeStepper->Cancel();

        return S_OK;
    }
    else if ( mResumeStepper.get() != NULL )
    {
        // Resume steppers are used for only one step, to move past a user BP.
        // So, use it, get rid of it, and continue

        hr = mResumeStepper->OnSingleStep( address, result );
        if ( FAILED( hr ) )
            return hr;

        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );

        if ( result == MacRes_HandledStopped )
            result = MacRes_HandledContinue;
    }

    return S_OK;
}


//----------------------------------------------------------------------------
//  InstructionStepperProbeCall
//----------------------------------------------------------------------------

InstructionStepperProbeCall::InstructionStepperProbeCall( 
    IStepperMachine* machine, 
    int instLen, 
    Address curAddress, 
    BPCookie cookie )
    :   mMachine( machine ),
        mCurAddr( curAddress ),
        mCookie( cookie ),
        mIsComplete( false ),
        mInstLen( instLen ),
        mStartingAddr( curAddress ),
        mAfterCallAddr( 0 ),
        mSSOrder( 0 ),
        mSSToHandle( 1 )
{
    _ASSERT( curAddress != 0 );
    _ASSERT( instLen > 0 );
    _ASSERT( machine != NULL );
}

HRESULT         InstructionStepperProbeCall::Start()
{
    HRESULT hr = S_OK;

    hr = mMachine->SetStepperSingleStep( true );
    if ( FAILED( hr ) )
        return hr;

    mAfterCallAddr = mCurAddr + mInstLen;

    return mMachine->SetStepperBreakpoint( mAfterCallAddr, mCookie );
}

HRESULT         InstructionStepperProbeCall::Cancel()
{
    if ( mResumeStepper.get() != NULL )
    {
        mResumeStepper->Cancel();
    }

    return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
}

bool            InstructionStepperProbeCall::IsComplete()
{
    return mIsComplete;
}

bool            InstructionStepperProbeCall::CanSkipEmbeddedBP()
{
    if ( (mCurAddr == mAfterCallAddr)
        || (mCurAddr == mStartingAddr) )
    {
        return false;
    }

    // We might run across an embedded BP in the middle of a call.
    // In that case, to resume running the procedure and keep this stepper going,
    // allow skipping that embedded BP.

    return true;
}

HRESULT         InstructionStepperProbeCall::RequestStepAway( Address pc )
{
    HRESULT hr = S_OK;

    if ( mCurAddr == mStartingAddr )
    {
        // we single step as the main kind of stepping, so nothing extra to do
        return S_OK;
    }

    if ( mResumeStepper.get() != NULL )
    {
        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );
    }

    ThreadX86Base* thread = mMachine->GetStoppedThread();
    _ASSERT( thread != NULL );

    hr = MakeResumeStepper( mMachine, pc, thread->GetResumeStepperCookie(), mResumeStepper );
    if ( FAILED( hr ) )
        return hr;

    return S_OK;
}

void            InstructionStepperProbeCall::SetAddress( Address address )
{
    mCurAddr = address;
}

HRESULT     InstructionStepperProbeCall::OnBreakpoint( Address address, MachineResult& result, bool& rewindPC )
{
    HRESULT hr = S_OK;

    mCurAddr = address;
    result = MacRes_NotHandled;
    rewindPC = true;

    if ( address == mAfterCallAddr )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;

        if ( mResumeStepper.get() != NULL )
            mResumeStepper->Cancel();

        return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
    }
    else if ( mResumeStepper.get() != NULL )
    {
        // Resume steppers are used for only one step, to move past a user BP.
        // So, use it, get rid of it, and continue

        hr = mResumeStepper->OnBreakpoint( address, result, rewindPC );
        if ( FAILED( hr ) )
            return hr;

        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );

        if ( result == MacRes_HandledStopped )
            result = MacRes_HandledContinue;
    }

    return S_OK;
}

HRESULT     InstructionStepperProbeCall::OnSingleStep( Address address, MachineResult& result )
{
    HRESULT hr = S_OK;

    mCurAddr = address;
    mSSOrder++;
    _ASSERT( mSSOrder != 0 );
    result = MacRes_NotHandled;

    if ( mSSOrder <= mSSToHandle )
    {
        if ( mSSOrder == 1 )
        {
            int             instLen = 0;
            InstructionType instType = Inst_None;

            hr = ReadInstruction( mMachine, address, Cpu_32, instType, instLen );
            if ( FAILED( hr ) )
                return hr;

            // We have a jmp as the first instruction in called function. Skip it.
            // This happens when the linker uses incremental linking.
            if ( instType == Inst_Jmp )
            {
                // remember that we're single stepping twice
                mSSToHandle = 2;

                result = MacRes_HandledContinue;
                return mMachine->SetStepperSingleStep( true );
            }
        }

        RunMode runMode = mMachine->CanStepperStopAtFunction( address );

        if ( runMode == RunMode_Break )
        {
            mIsComplete = true;
            result = MacRes_HandledStopped;

            if ( mResumeStepper.get() != NULL )
                mResumeStepper->Cancel();

            return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
        }
        else if ( runMode == RunMode_Run )
        {
            // go on to the breakpoint we set after the original call instruction
            result = MacRes_HandledContinue;
            return S_OK;
        }

        // Wait run-mode
        result = MacRes_HandledStopped;
        return S_OK;
    }
    else if ( mResumeStepper.get() != NULL )
    {
        // Resume steppers are used for only one step, to move past a user BP.
        // So, use it, get rid of it, and continue

        hr = mResumeStepper->OnSingleStep( address, result );
        if ( FAILED( hr ) )
            return hr;

        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );

        if ( result == MacRes_HandledStopped )
            result = MacRes_HandledContinue;
    }

    return S_OK;
}


//----------------------------------------------------------------------------
//  RangeStepper
//----------------------------------------------------------------------------

RangeStepper::RangeStepper( 
    IStepperMachine* machine, 
    bool stepIn, 
    bool sourceMode, 
    Address curAddress, 
    BPCookie cookie, 
    AddressRange range )
:   
    mMachine( machine ),
    mCurAddr( curAddress ),
    mCookie( cookie ),
    mStepIn( stepIn ),
    mSourceMode( sourceMode ),
    mIsComplete( false ),
    mFirstInstruction( true ),
    mInstStepper( NULL )
{
    _ASSERT( curAddress != 0 );
    _ASSERT( machine != NULL );

    mRangeSingle = range;
}

RangeStepper::~RangeStepper()
{
    delete mInstStepper;
}

HRESULT         RangeStepper::Start()
{
    return StartOneStep( mCurAddr );
}

HRESULT         RangeStepper::StartOneStep( Address pc )
{
    _ASSERT( mInstStepper == NULL );

    HRESULT hr = S_OK;
    auto_ptr<IStepper> stepper;

    // If the user starts a step at a breakpoint, then they expect to step over 
    // it as usual. If we hit a breakpoint after that and before the user sets 
    // another stepper, then don't handle it, so the user is notified.

    bool handleBP = mFirstInstruction;

    mFirstInstruction = false;

    if ( mStepIn )
    {
        hr = MakeStepInStepper( mMachine, pc, mSourceMode, mCookie, handleBP, stepper );
    }
    else
    {
        hr = MakeStepOverStepper( mMachine, pc, mCookie, handleBP, stepper );
    }

    if ( FAILED( hr ) )
        return hr;

    mInstStepper = stepper.release();

    return mInstStepper->Start();
}

HRESULT         RangeStepper::Cancel()
{
    HRESULT hr = S_OK;

    if ( mInstStepper != NULL )
    {
        hr = mInstStepper->Cancel();

        delete mInstStepper;
        mInstStepper = NULL;
    }

    return hr;
}

bool            RangeStepper::IsComplete()
{
    return mIsComplete;
}

bool            RangeStepper::CanSkipEmbeddedBP()
{
    if ( mInstStepper == NULL )
        return false;

    return mInstStepper->CanSkipEmbeddedBP();
}

HRESULT         RangeStepper::RequestStepAway( Address pc )
{
    if ( mInstStepper == NULL )
        return E_FAIL;

    return mInstStepper->RequestStepAway( pc );
}

void            RangeStepper::SetAddress( Address address )
{
    mCurAddr = address;
}

HRESULT     RangeStepper::OnBreakpoint( Address address, MachineResult& result, bool& rewindPC )
{
    HRESULT hr = S_OK;

    mCurAddr = address;

    if ( mInstStepper == NULL )
        return E_FAIL;

    hr = mInstStepper->OnBreakpoint( address, result, rewindPC );
    if ( FAILED( hr ) )
        return hr;

    if ( !mInstStepper->IsComplete() )
        return hr;

    // After hitting a BP, the PC is one past the exception address.
    // The instruction stepper might want it there or at the exception address.

    Address pc = address;

    if ( !rewindPC )
        pc++;

    // Either way, the PC is where we really need to check for step complete

    return HandleComplete( pc, result );
}

HRESULT     RangeStepper::OnSingleStep( Address address, MachineResult& result )
{
    HRESULT hr = S_OK;

    mCurAddr = address;

    if ( mInstStepper == NULL )
        return E_FAIL;

    hr = mInstStepper->OnSingleStep( address, result );
    if ( FAILED( hr ) )
        return hr;

    if ( !mInstStepper->IsComplete() )
        return hr;

    // After a HW SS, the PC is at the exception address: 
    // at the instruction following the one that was stepped

    return HandleComplete( address, result );
}

bool            RangeStepper::IsAddressInRange( Address pc )
{
    AddressRange*   range = NULL;

    range = &mRangeSingle;

    if ( (pc >= range->Begin) && (pc <= range->End) )
        return true;

    return false;
}

HRESULT RangeStepper::HandleComplete( Address pc, MachineResult& result )
{
    delete mInstStepper;
    mInstStepper = NULL;

    if ( IsAddressInRange( pc ) )
    {
        result = MacRes_HandledContinue;
        return StartOneStep( pc );
    }
    
    mIsComplete = true;
    result = MacRes_HandledStopped;
    return S_OK;
}


//----------------------------------------------------------------------------
//  RunToStepper
//----------------------------------------------------------------------------

RunToStepper::RunToStepper( 
    IStepperMachine* machine,
    Address curAddress,
    BPCookie cookie, 
    Address targetAddress )
    :   mMachine( machine ),
        mCurAddr( curAddress ),
        mCookie( cookie ),
        mTargetAddr( targetAddress ),
        mIsComplete( false )
{
    _ASSERT( targetAddress != 0 );
    _ASSERT( curAddress != 0 );
    _ASSERT( machine != NULL );
}

HRESULT         RunToStepper::Start()
{
    return mMachine->SetStepperBreakpoint( mTargetAddr, mCookie );
}

HRESULT         RunToStepper::Cancel()
{
    if ( mResumeStepper.get() != NULL )
    {
        mResumeStepper->Cancel();
    }

    return mMachine->RemoveStepperBreakpoint( mTargetAddr, mCookie );
}

bool            RunToStepper::IsComplete()
{
    return mIsComplete;
}

bool            RunToStepper::CanSkipEmbeddedBP()
{
    if ( mCurAddr == mTargetAddr )
    {
        return false;
    }

    // We might run across an embedded BP while running to the target.
    // In that case, to resume running and keep this stepper going,
    // allow skipping that embedded BP.

    return true;
}

HRESULT         RunToStepper::RequestStepAway( Address pc )
{
    HRESULT hr = S_OK;

    if ( mResumeStepper.get() != NULL )
    {
        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );
    }

    ThreadX86Base* thread = mMachine->GetStoppedThread();
    _ASSERT( thread != NULL );

    hr = MakeResumeStepper( mMachine, pc, thread->GetResumeStepperCookie(), mResumeStepper );
    if ( FAILED( hr ) )
        return hr;

    return S_OK;
}

void            RunToStepper::SetAddress( Address address )
{
    mCurAddr = address;
}

HRESULT     RunToStepper::OnBreakpoint( Address address, MachineResult& result, bool& rewindPC )
{
    HRESULT hr = S_OK;

    result = MacRes_NotHandled;
    rewindPC = true;

    if ( address == mTargetAddr )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;

        if ( mResumeStepper.get() != NULL )
            mResumeStepper->Cancel();

        return mMachine->RemoveStepperBreakpoint( mTargetAddr, mCookie );
    }
    else if ( mResumeStepper.get() != NULL )
    {
        // Resume steppers are used for only one step, to move past a user BP.
        // So, use it, get rid of it, and continue

        hr = mResumeStepper->OnBreakpoint( address, result, rewindPC );
        if ( FAILED( hr ) )
            return hr;

        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );

        if ( result == MacRes_HandledStopped )
            result = MacRes_HandledContinue;
    }

    return S_OK;
}

HRESULT     RunToStepper::OnSingleStep( Address address, MachineResult& result )
{
    HRESULT hr = S_OK;

    mCurAddr = address;
    result = MacRes_NotHandled;

    if ( mResumeStepper.get() != NULL )
    {
        // Resume steppers are used for only one step, to move past a user BP.
        // So, use it, get rid of it, and continue

        hr = mResumeStepper->OnSingleStep( address, result );
        if ( FAILED( hr ) )
            return hr;

        mResumeStepper->Cancel();
        mResumeStepper.reset( NULL );

        if ( result == MacRes_HandledStopped )
            result = MacRes_HandledContinue;
    }

    return S_OK;
}
