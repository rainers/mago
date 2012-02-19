/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

enum MachineResult;
class IStepperMachine;


// IStepper::RequestSingleStep asks the stepper to perform an instruction 
// single step. If it can or if it shouldn't, then S_OK is returned.
// Otherwise, S_FALSE is returned, and it's up to the user to make and use 
// a resumption stepper. This gives us the efficiency of a stepper that's
// already made handling a single step, and the simplicity or consistency
// of a resumption stepper in other cases.

class IStepper
{
public:
    virtual ~IStepper() { }

    virtual HRESULT         Start() = 0;
    virtual HRESULT         Cancel() = 0;
    virtual bool            IsComplete() = 0;
    // returns S_FALSE if caller should use a resumption stepper
    virtual HRESULT         RequestSingleStep() = 0;
    virtual void            SetAddress( MachineAddress address ) = 0;

    virtual HRESULT         OnBreakpoint( MachineAddress address, MachineResult& result ) = 0;
    virtual HRESULT         OnSingleStep( MachineAddress address, MachineResult& result ) = 0;
};


// Steps an instruction by single stepping.

class InstructionStepperSS : public IStepper
{
    IStepperMachine*    mMachine;
    MachineAddress      mCurAddr;
    BPCookie            mCookie;
    bool                mIsComplete;

public:
    InstructionStepperSS( IStepperMachine* machine, MachineAddress curAddress, BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual HRESULT         RequestSingleStep();
    virtual void            SetAddress( MachineAddress address );

    virtual HRESULT         OnBreakpoint( MachineAddress address, MachineResult& result );
    virtual HRESULT         OnSingleStep( MachineAddress address, MachineResult& result );
};


// Steps an instruction by running to a breakpoint set at the address right 
// after the current one that we're stepping. Supports single stepping if
// allowSS constructor parameter is true.

class InstructionStepperBP : public IStepper
{
    IStepperMachine*    mMachine;
    MachineAddress      mCurAddr;
    BPCookie            mCookie;
    bool                mIsComplete;
    int                 mInstLen;
    bool                mAllowSS;
    bool                mRequestedSS;
    MachineAddress      mAfterCallAddr;

public:
    InstructionStepperBP( 
        IStepperMachine* machine, 
        int instLen, 
        bool allowSS, 
        MachineAddress curAddress, 
        BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual HRESULT         RequestSingleStep();
    virtual void            SetAddress( MachineAddress address );

    virtual HRESULT         OnBreakpoint( MachineAddress address, MachineResult& result );
    virtual HRESULT         OnSingleStep( MachineAddress address, MachineResult& result );
};


// Steps a call instruction by running to a breakpoint set at the address right
// after the current one that we're stepping. Supports single stepping.

class InstructionStepperBPCall : public IStepper
{
    IStepperMachine*    mMachine;
    MachineAddress      mCurAddr;
    BPCookie            mCookie;
    bool                mIsComplete;
    int                 mInstLen;
    bool                mRequestedSS;
    MachineAddress      mStartingAddr;
    MachineAddress      mAfterCallAddr;

public:
    InstructionStepperBPCall( IStepperMachine* machine, int instLen, MachineAddress curAddress, BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual HRESULT         RequestSingleStep();
    virtual void            SetAddress( MachineAddress address );

    virtual HRESULT         OnBreakpoint( MachineAddress address, MachineResult& result );
    virtual HRESULT         OnSingleStep( MachineAddress address, MachineResult& result );
};


// Steps into a function. Once inside, if a call to IStepperMachine::
// CanStepperStopAtFunction returns true, the step is complete, otherwise 
// the stepper runs out of the function as if we had stepped over it.

class InstructionStepperProbeCall : public IStepper
{
    IStepperMachine*    mMachine;
    MachineAddress      mCurAddr;
    BPCookie            mCookie;
    bool                mIsComplete;
    int                 mInstLen;
    MachineAddress      mStartingAddr;
    MachineAddress      mAfterCallAddr;
    int                 mSSOrder;

public:
    InstructionStepperProbeCall( IStepperMachine* machine, int instLen, MachineAddress curAddress, BPCookie cookie );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual HRESULT         RequestSingleStep();
    virtual void            SetAddress( MachineAddress address );

    virtual HRESULT         OnBreakpoint( MachineAddress address, MachineResult& result );
    virtual HRESULT         OnSingleStep( MachineAddress address, MachineResult& result );
};


// Uses instruction steppers to step over a range of addresses.

class RangeStepper : public IStepper
{
    IStepperMachine*    mMachine;
    MachineAddress      mCurAddr;
    BPCookie            mCookie;
    bool                mStepIn;
    bool                mSourceMode;
    AddressRange        mRangeSingle;
    int                 mRangeCount;
    bool                mIsComplete;
    IStepper*           mInstStepper;
    std::vector<AddressRange>   mRangesMany;

public:
    RangeStepper( 
        IStepperMachine* machine, 
        bool stepIn, 
        bool sourceMode, 
        MachineAddress curAddress, 
        BPCookie cookie, 
        AddressRange* ranges, 
        int rangeCount );
    ~RangeStepper();

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual HRESULT         RequestSingleStep();
    virtual void            SetAddress( MachineAddress address );

    virtual HRESULT         OnBreakpoint( MachineAddress address, MachineResult& result );
    virtual HRESULT         OnSingleStep( MachineAddress address, MachineResult& result );

private:
    HRESULT StartOneStep();
    bool    IsAddressInRange( MachineAddress address );
    HRESULT HandleComplete( MachineAddress address, MachineResult& result );
};


// Steps by running to a target address.

class RunToStepper : public IStepper
{
    IStepperMachine*    mMachine;
    MachineAddress      mCurAddr;
    BPCookie            mCookie;
    MachineAddress      mTargetAddr;
    bool                mIsComplete;

public:
    RunToStepper(
        IStepperMachine* machine,
        MachineAddress curAddress,
        BPCookie cookie, 
        MachineAddress targetAddress );

    virtual HRESULT         Start();
    virtual HRESULT         Cancel();
    virtual bool            IsComplete();
    virtual HRESULT         RequestSingleStep();
    virtual void            SetAddress( MachineAddress address );

    virtual HRESULT         OnBreakpoint( MachineAddress address, MachineResult& result );
    virtual HRESULT         OnSingleStep( MachineAddress address, MachineResult& result );
};


HRESULT MakeResumeStepper( 
                          IStepperMachine* machine, 
                          MachineAddress curAddress, 
                          BPCookie cookie, 
                          std::auto_ptr<IStepper>& stepper );

HRESULT MakeStepInStepper( 
                          IStepperMachine* machine, 
                          MachineAddress curAddress, 
                          bool sourceMode, 
                          BPCookie cookie, 
                          std::auto_ptr<IStepper>& stepper );

HRESULT MakeStepOverStepper( 
                            IStepperMachine* machine, 
                            MachineAddress curAddress, 
                            BPCookie cookie, 
                            std::auto_ptr<IStepper>& stepper );
