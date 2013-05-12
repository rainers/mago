/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "BPBinderCallback.h"


namespace Mago
{
    class BPCodeFileLineBinder : public BPBinder
    {
        CComBSTR            mFilename;
        DWORD               mReqLineStart;
        DWORD               mReqLineEnd;

    public:
        BPCodeFileLineBinder( IDebugBreakpointRequest2* request );

        virtual void Bind( Module* mod, ModuleBinding* binding, BPBoundBPMaker* maker, Error& err );

    private:
        void PutDocError( Error& err );
        void PutLineError( Error& err );

        bool BindToFile( bool exactMatch, const char* fileName, size_t fileNameLen, Module* mod, ModuleBinding* binding, BPBoundBPMaker* maker, Error& err );
    };


    class BPCodeAddressBinder : public BPBinder
    {
        Address     mAddress;

    public:
        BPCodeAddressBinder( IDebugBreakpointRequest2* request );

        virtual void Bind( Module* mod, ModuleBinding* binding, BPBoundBPMaker* maker, Error& err );
    };
}
