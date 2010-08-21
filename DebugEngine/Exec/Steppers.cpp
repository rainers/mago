/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Stepper.h"
#include "MachineX86Base.h"
#include "DecodeX86.h"

using namespace std;


HRESULT ReadInstruction( 
    IStepperMachine* machine, 
    MachineAddress curAddress, 
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
                          MachineAddress curAddress, 
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
        stepper.reset( new InstructionStepperBP( machine, instLen, false, curAddress, cookie ) );
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
                          MachineAddress curAddress, 
                          bool sourceMode, 
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
        stepper.reset( new InstructionStepperSS( machine, curAddress, cookie ) );
    }
    else if ( instType == Inst_Call )
    {
        if ( sourceMode )
            stepper.reset( new InstructionStepperProbeCall( machine, instLen, curAddress, cookie ) );
        else
            stepper.reset( new InstructionStepperSS( machine, curAddress, cookie ) );
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
                            MachineAddress curAddress, 
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
        stepper.reset( new InstructionStepperBP( machine, instLen, false, curAddress, cookie ) );
    }
    else if ( instType == Inst_Call )
    {
        stepper.reset( new InstructionStepperBPCall( machine, instLen, curAddress, cookie ) );
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

InstructionStepperSS::InstructionStepperSS( IStepperMachine* machine, MachineAddress curAddress, BPCookie cookie )
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

HRESULT         InstructionStepperSS::RequestSingleStep()
{
    // we single step as the main kind of stepping, so nothing extra to do
    return S_OK;
}

void            InstructionStepperSS::SetAddress( MachineAddress address )
{
    mCurAddr = address;
}

HRESULT     InstructionStepperSS::OnBreakpoint( MachineAddress address, MachineResult& result )
{
    address;    // not used

    result = MacRes_NotHandled;
    return S_OK;
}

HRESULT     InstructionStepperSS::OnSingleStep( MachineAddress address, MachineResult& result )
{
    address;    // not used

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
    bool allowSS, 
    MachineAddress curAddress, 
    BPCookie cookie )
    :   mMachine( machine ),
        mCurAddr( curAddress ),
        mCookie( cookie ),
        mIsComplete( false ),
        mInstLen( instLen ),
        mAllowSS( allowSS ),
        mRequestedSS( false ),
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

HRESULT         InstructionStepperBP::RequestSingleStep()
{
    mRequestedSS = true;

    if ( mAllowSS )
        return mMachine->SetStepperSingleStep( true );

    return S_OK;
}

void            InstructionStepperBP::SetAddress( MachineAddress address )
{
    mCurAddr = address;
}

HRESULT     InstructionStepperBP::OnBreakpoint( MachineAddress address, MachineResult& result )
{
    result = MacRes_NotHandled;

    if ( address == mAfterCallAddr )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;
        return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
    }

    return S_OK;
}

HRESULT     InstructionStepperBP::OnSingleStep( MachineAddress address, MachineResult& result )
{
    address;    // not used

    if ( mAllowSS && mRequestedSS )
        result = MacRes_HandledContinue;
    else
        result = MacRes_NotHandled;

    return S_OK;
}


//----------------------------------------------------------------------------
//  InstructionStepperBPCall
//----------------------------------------------------------------------------

InstructionStepperBPCall::InstructionStepperBPCall( 
    IStepperMachine* machine, 
    int instLen, 
    MachineAddress curAddress, 
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
    return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
}

bool            InstructionStepperBPCall::IsComplete()
{
    return mIsComplete;
}

HRESULT         InstructionStepperBPCall::RequestSingleStep()
{
    if ( mCurAddr == mStartingAddr )
    {
        mRequestedSS = true;

        return mMachine->SetStepperSingleStep( true );
    }

    return S_FALSE;
}

void            InstructionStepperBPCall::SetAddress( MachineAddress address )
{
    mCurAddr = address;
}

HRESULT     InstructionStepperBPCall::OnBreakpoint( MachineAddress address, MachineResult& result )
{
    mCurAddr = address;
    result = MacRes_NotHandled;

    if ( address == mAfterCallAddr )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;
        return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
    }

    return S_OK;
}

HRESULT     InstructionStepperBPCall::OnSingleStep( MachineAddress address, MachineResult& result )
{
    mCurAddr = address;
    result = MacRes_NotHandled;

    if ( mRequestedSS )
    {
        mRequestedSS = false;
        result = MacRes_HandledContinue;
        return S_OK;
    }

    return S_OK;
}


//----------------------------------------------------------------------------
//  InstructionStepperProbeCall
//----------------------------------------------------------------------------

InstructionStepperProbeCall::InstructionStepperProbeCall( 
    IStepperMachine* machine, 
    int instLen, 
    MachineAddress curAddress, 
    BPCookie cookie )
    :   mMachine( machine ),
        mCurAddr( curAddress ),
        mCookie( cookie ),
        mIsComplete( false ),
        mInstLen( instLen ),
        mStartingAddr( curAddress ),
        mAfterCallAddr( 0 ),
        mSSOrder( 0 )
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
    return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
}

bool            InstructionStepperProbeCall::IsComplete()
{
    return mIsComplete;
}

HRESULT         InstructionStepperProbeCall::RequestSingleStep()
{
    if ( mCurAddr == mStartingAddr )
    {
        // we single step as the main kind of stepping, so nothing extra to do
        return S_OK;
    }

    return S_FALSE;
}

void            InstructionStepperProbeCall::SetAddress( MachineAddress address )
{
    mCurAddr = address;
}

HRESULT     InstructionStepperProbeCall::OnBreakpoint( MachineAddress address, MachineResult& result )
{
    mCurAddr = address;
    result = MacRes_NotHandled;

    if ( address == mAfterCallAddr )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;
        return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
    }

    return S_OK;
}

HRESULT     InstructionStepperProbeCall::OnSingleStep( MachineAddress address, MachineResult& result )
{
    HRESULT hr = S_OK;

    mCurAddr = address;
    mSSOrder++;
    _ASSERT( mSSOrder != 0 );

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
            result = MacRes_HandledContinue;
            return mMachine->SetStepperSingleStep( true );
        }
    }

    if ( mMachine->CanStepperStopAtFunction( address ) )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;
        return mMachine->RemoveStepperBreakpoint( mAfterCallAddr, mCookie );
    }

    // go on to the breakpoint we set after the original call instruction
    result = MacRes_HandledContinue;
    return S_OK;
}


//----------------------------------------------------------------------------
//  RangeStepper
//----------------------------------------------------------------------------

RangeStepper::RangeStepper( 
    IStepperMachine* machine, 
    bool stepIn, 
    bool sourceMode, 
    MachineAddress curAddress, 
    BPCookie cookie, 
    AddressRange* ranges, 
    int rangeCount )
:   
    mMachine( machine ),
    mCurAddr( curAddress ),
    mCookie( cookie ),
    mStepIn( stepIn ),
    mSourceMode( sourceMode ),
    mRangeCount( rangeCount ),
    mIsComplete( false ),
    mInstStepper( NULL )
{
    _ASSERT( curAddress != 0 );
    _ASSERT( machine != NULL );
    _ASSERT( ranges != NULL );
    _ASSERT( rangeCount > 0 );

    if ( rangeCount == 1 )
    {
        mRangeSingle = ranges[0];
    }
    else
    {
        for ( int i = 0; i < rangeCount; i++ )
        {
            mRangesMany.push_back( ranges[i] );
        }
    }
}

RangeStepper::~RangeStepper()
{
    delete mInstStepper;
}

HRESULT         RangeStepper::Start()
{
    return StartOneStep();
}

HRESULT         RangeStepper::StartOneStep()
{
    _ASSERT( mInstStepper == NULL );

    HRESULT hr = S_OK;
    auto_ptr<IStepper> stepper;

    if ( mStepIn )
    {
        hr = MakeStepInStepper( mMachine, mCurAddr, mSourceMode, mCookie, stepper );
    }
    else
    {
        hr = MakeStepOverStepper( mMachine, mCurAddr, mCookie, stepper );
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

HRESULT         RangeStepper::RequestSingleStep()
{
    if ( mInstStepper == NULL )
        return E_FAIL;

    return mInstStepper->RequestSingleStep();
}

void            RangeStepper::SetAddress( MachineAddress address )
{
    mCurAddr = address;
}

HRESULT     RangeStepper::OnBreakpoint( MachineAddress address, MachineResult& result )
{
    HRESULT hr = S_OK;

    mCurAddr = address;

    if ( mInstStepper == NULL )
        return E_FAIL;

    hr = mInstStepper->OnBreakpoint( address, result );
    if ( FAILED( hr ) )
        return hr;

    if ( !mInstStepper->IsComplete() )
        return hr;

    return HandleComplete( address, result );
}

HRESULT     RangeStepper::OnSingleStep( MachineAddress address, MachineResult& result )
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

    return HandleComplete( address, result );
}

bool            RangeStepper::IsAddressInRange( MachineAddress address )
{
    AddressRange*   range = NULL;

    if ( mRangeCount == 1 )
        range = &mRangeSingle;
    else
        range = &mRangesMany[0];

    for ( int count = mRangeCount; count > 0; count-- )
    {
        if ( (address >= range->Begin) && (address <= range->End) )
            return true;

        range++;
    }

    return false;
}

HRESULT RangeStepper::HandleComplete( MachineAddress address, MachineResult& result )
{
    delete mInstStepper;
    mInstStepper = NULL;

    if ( IsAddressInRange( address ) )
    {
        result = MacRes_HandledContinue;
        return StartOneStep();
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
    MachineAddress curAddress,
    BPCookie cookie, 
    MachineAddress targetAddress )
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
    return mMachine->RemoveStepperBreakpoint( mTargetAddr, mCookie );
}

bool            RunToStepper::IsComplete()
{
    return mIsComplete;
}

HRESULT         RunToStepper::RequestSingleStep()
{
    return S_FALSE;
}

void            RunToStepper::SetAddress( MachineAddress address )
{
    mCurAddr = address;
}

HRESULT     RunToStepper::OnBreakpoint( MachineAddress address, MachineResult& result )
{
    result = MacRes_NotHandled;

    if ( address == mTargetAddr )
    {
        mIsComplete = true;
        result = MacRes_HandledStopped;
        return mMachine->RemoveStepperBreakpoint( mTargetAddr, mCookie );
    }

    return S_OK;
}

HRESULT     RunToStepper::OnSingleStep( MachineAddress address, MachineResult& result )
{
    mCurAddr = address;
    result = MacRes_NotHandled;

    return S_OK;
}
