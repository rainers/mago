/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


namespace Mago
{
    class PendingBreakpoint;
    class BPDocumentContext;
    class ErrorBreakpoint;
    struct ModuleBinding;


    struct Error
    {
        BP_ERROR_TYPE   Type;
        BP_ERROR_TYPE   Sev;
        StringIds       StrId;
        // TODO: maybe add an optional string parameter for the formatted string message?

        Error()
            :   Type( 0 ),
                Sev( 0 ),
                StrId( (StringIds) 0 )
        {
        }

        void PutError( BP_ERROR_TYPE type, BP_ERROR_TYPE sev, StringIds strId )
        {
            if ( (type > Type) || ((type == Type) && (sev > Sev)) )
            {
                Type = type;
                Sev = sev;
                StrId = strId;
            }
        }
    };

    class BPBoundBPMaker
    {
    public:
        virtual HRESULT MakeDocContext( MagoST::ISession* session, uint16_t compIx, uint16_t fileIx, const MagoST::LineNumber& lineNumber ) = 0;
        virtual void AddBoundBP( UINT64 address, Module* mod, ModuleBinding* binding ) = 0;
    };

    class BPBinder
    {
    public:
        virtual void Bind( Module* mod, ModuleBinding* binding, BPBoundBPMaker* maker, Error& err ) = 0;
    };

    class BPBinderCallback : public ProgramCallback, public ModuleCallback, public BPBoundBPMaker
    {
        int                     mBoundBPCount;
        int                     mErrorBPCount;
        PendingBreakpoint*      mPendingBP;
        RefPtr<BPDocumentContext> mDocContext;
        CComPtr<IDebugDocumentContext2> mDocContextInterface;
        RefPtr<Program>         mCurProg;
        CComPtr<IDebugProgram2>         mCurProgInterface;
        RefPtr<ErrorBreakpoint> mLastErrorBP;
        BPBinder*               mBinder;

    public:
        BPBinderCallback( 
            BPBinder* binder,
            PendingBreakpoint* pendingBP, 
            BPDocumentContext* docContext );

        int GetBoundBPCount();
        int GetErrorBPCount();
        bool GetDocumentContext( RefPtr<BPDocumentContext>& docContext );
        bool GetLastErrorBP( RefPtr<ErrorBreakpoint>& errorBP );

        bool AcceptProgram( Program* prog );
        bool AcceptModule( Module* mod );

        HRESULT BindToModule( Module* mod, Program* prog );

    private:
        virtual HRESULT MakeDocContext( MagoST::ISession* session, uint16_t compIx, uint16_t fileIx, const MagoST::LineNumber& lineNumber );
        HRESULT MakeErrorBP( Error& errDesc, RefPtr<ErrorBreakpoint>& errorBP );
        void AddBoundBP( UINT64 address, Module* mod, ModuleBinding* binding );
    };
}
