/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

enum MachineResult;
class IStepperMachine;


// IStepper::RequestStepAway asks the stepper to perform something like an 
// instruction single step. The machine wants the stepper to step past the
// current instruction. Each stepper knows what's appropriate. Normally, a 
// hardware single step is enough. In the case of REP String instructions, 
// putting a BP after the instruction is what's needed.

// CanSkipEmbeddedBP
// If a stepper is ever stopped at any point of its execution by an embedded
// breakpoint, then it knows if that embedded breakpoint needs to be stepped
// or if can be skipped. For example, if we begin stepping right at an embedded 
// BP, then a stepper will expect to step over it instead of skipping it. But,
// if the embedded BP wasn't part of the stepper's normal execution, then it
// can safely be skipped. The machine asks about skipping for efficiency.

// OnBreakpoint( ... rewindPC )
// After a breakpoint instruction, the PC is one past the exception address, 
// which is at the breakpoint instruction. Each stepper knows if leaving the PC
// at the breakpoint instruction or moving past it is the intended result.

class IStepper
{
public:
    virtual ~IStepper() { }

    virtual HRESULT         Start() = 0;
    virtual HRESULT         Cancel() = 0;
    virtual bool            IsComplete() = 0;
    virtual bool            CanSkipEmbeddedBP() = 0;
    virtual HRESULT         RequestStepAway( Address pc ) = 0;
    virtual void            SetAddress( Address address ) = 0;

    virtual HRESULT         OnBreakpoint( Address address, MachineResult& result, bool& rewindPC ) = 0;
    virtual HRESULT         OnSingleStep( Address address, MachineResult& result ) = 0;
};


// Steps an instruction by single stepping.
// This isn't meant for the embedded BP instruction, as described for 
// InstructionStepperEmbeddedBP.

class InstructionStepperSS : public IStepper
{
    IStepperMachine*    mMachine;
    Address             mCurAddr;
    BPCookie            mCookie;
    bool                mIsComplete;

public:
    InstructionStepperSS( IStepperMachine* machine, Address curAddress, BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual bool            CanSkipEmbeddedBP();
    virtual HRESULT         RequestStepAway( Address pc );
    virtual void            SetAddress( Address address );

    virtual HRESULT         OnBreakpoint( Address address, MachineResult& result, bool& rewindPC );
    virtual HRESULT         OnSingleStep( Address address, MachineResult& result );
};


// Steps over a breakpoint instruction.
// Execution will stop at the BP exception, but it'll be treated as a step complete.
// Gives the choice to handle the BP exception as a step complete, or unhandled.
// Either way, the breakpoint instruction is stepped without a hardware single step,
// which would have made the following instruction single step.

class InstructionStepperEmbeddedBP : public IStepper
{
    IStepperMachine*    mMachine;
    Address             mStartingAddr;
    BPCookie            mCookie;
    bool                mIsComplete;
    bool                mHandleBPException;

public:
    InstructionStepperEmbeddedBP( 
        IStepperMachine* machine, 
        Address curAddress, 
        bool handleBP, 
        BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual bool            CanSkipEmbeddedBP();
    virtual HRESULT         RequestStepAway( Address pc );
    virtual void            SetAddress( Address address );

    virtual HRESULT         OnBreakpoint( Address address, MachineResult& result, bool& rewindPC );
    virtual HRESULT         OnSingleStep( Address address, MachineResult& result );
};


// Steps an instruction by running to a breakpoint set at the address right 
// after the current one that we're stepping.

class InstructionStepperBP : public IStepper
{
    IStepperMachine*    mMachine;
    Address             mCurAddr;
    BPCookie            mCookie;
    bool                mIsComplete;
    int                 mInstLen;
    Address             mAfterCallAddr;

public:
    InstructionStepperBP( 
        IStepperMachine* machine, 
        int instLen, 
        Address curAddress, 
        BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual bool            CanSkipEmbeddedBP();
    virtual HRESULT         RequestStepAway( Address pc );
    virtual void            SetAddress( Address address );

    virtual HRESULT         OnBreakpoint( Address address, MachineResult& result, bool& rewindPC );
    virtual HRESULT         OnSingleStep( Address address, MachineResult& result );
};


// Steps a call instruction by running to a breakpoint set at the address right
// after the current one that we're stepping. Supports step away by single 
// stepping the call.

class InstructionStepperBPCall : public IStepper
{
    IStepperMachine*    mMachine;
    Address             mCurAddr;
    BPCookie            mCookie;
    bool                mIsComplete;
    int                 mInstLen;
    bool                mRequestedSS;
    Address             mStartingAddr;
    Address             mAfterCallAddr;
    std::auto_ptr<IStepper> mResumeStepper;

public:
    InstructionStepperBPCall( 
        IStepperMachine* machine, 
        int instLen, 
        Address curAddress, 
        BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual bool            CanSkipEmbeddedBP();
    virtual HRESULT         RequestStepAway( Address pc );
    virtual void            SetAddress( Address address );

    virtual HRESULT         OnBreakpoint( Address address, MachineResult& result, bool& rewindPC );
    virtual HRESULT         OnSingleStep( Address address, MachineResult& result );
};


// Steps into a function. Once inside, if a call to IStepperMachine::
// CanStepperStopAtFunction returns true, the step is complete, otherwise 
// the stepper runs out of the function as if we had stepped over it.

class InstructionStepperProbeCall : public IStepper
{
    IStepperMachine*    mMachine;
    Address             mCurAddr;
    BPCookie            mCookie;
    bool                mIsComplete;
    int                 mInstLen;
    Address             mStartingAddr;
    Address             mAfterCallAddr;
    int                 mSSOrder;           // how many HW SS have been done
    int                 mSSToHandle;        // how many HW SS to do
    std::auto_ptr<IStepper> mResumeStepper;

public:
    InstructionStepperProbeCall( 
        IStepperMachine* machine, 
        int instLen, 
        Address curAddress, 
        BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual bool            CanSkipEmbeddedBP();
    virtual HRESULT         RequestStepAway( Address pc );
    virtual void            SetAddress( Address address );

    virtual HRESULT         OnBreakpoint( Address address, MachineResult& result, bool& rewindPC );
    virtual HRESULT         OnSingleStep( Address address, MachineResult& result );
};


// Uses instruction steppers to step over a range of addresses.

class RangeStepper : public IStepper
{
    IStepperMachine*    mMachine;
    Address             mCurAddr;
    BPCookie            mCookie;
    bool                mStepIn;
    bool                mSourceMode;
    AddressRange        mRangeSingle;
    int                 mRangeCount;
    bool                mIsComplete;
    bool                mFirstInstruction;
    IStepper*           mInstStepper;
    std::vector<AddressRange>   mRangesMany;

public:
    RangeStepper( 
        IStepperMachine* machine, 
        bool stepIn, 
        bool sourceMode, 
        Address curAddress, 
        BPCookie cookie, 
        AddressRange* ranges, 
        int rangeCount );
    ~RangeStepper();

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual bool            CanSkipEmbeddedBP();
    virtual HRESULT         RequestStepAway( Address pc );
    virtual void            SetAddress( Address address );

    virtual HRESULT         OnBreakpoint( Address address, MachineResult& result, bool& rewindPC );
    virtual HRESULT         OnSingleStep( Address address, MachineResult& result );

private:
    HRESULT StartOneStep( Address pc );
    bool    IsAddressInRange( Address pc );
    HRESULT HandleComplete( Address pc, MachineResult& result );
};


// Steps by running to a target address.

class RunToStepper : public IStepper
{
    IStepperMachine*    mMachine;
    Address             mCurAddr;
    BPCookie            mCookie;
    Address             mTargetAddr;
    bool                mIsComplete;
    std::auto_ptr<IStepper> mResumeStepper;

public:
    RunToStepper(
        IStepperMachine* machine,
        Address curAddress,
        BPCookie cookie, 
        Address targetAddress );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual bool            CanSkipEmbeddedBP();
    virtual HRESULT         RequestStepAway( Address pc );
    virtual void            SetAddress( Address address );

    virtual HRESULT         OnBreakpoint( Address address, MachineResult& result, bool& rewindPC );
    virtual HRESULT         OnSingleStep( Address address, MachineResult& result );
};


HRESULT MakeResumeStepper( 
                          IStepperMachine* machine, 
                          Address curAddress, 
                          BPCookie cookie, 
                          std::auto_ptr<IStepper>& stepper );

HRESULT MakeStepInStepper( 
                          IStepperMachine* machine, 
                          Address curAddress, 
                          bool sourceMode, 
                          BPCookie cookie, 
                          bool handleBP, 
                          std::auto_ptr<IStepper>& stepper );

HRESULT MakeStepOverStepper( 
                            IStepperMachine* machine, 
                            Address curAddress, 
                            BPCookie cookie, 
                            bool handleBP, 
                            std::auto_ptr<IStepper>& stepper );
