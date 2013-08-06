/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "ArchData.h"
#include <DbgHelp.h>


namespace Mago
{
    class WindowsStackWalker : public StackWalker
    {
        uint32_t                    mMachineType;
        STACKFRAME64                mGenericFrame;
        void*                       mProcessContext;
        ReadProcessMemory64Proc     mReadMemProc;
        FunctionTableAccess64Proc   mFuncTabProc;
        GetModuleBase64Proc         mGetModBaseProc;
        std::unique_ptr<BYTE[]>     mThreadContext;
        uint32_t                    mThreadContextSize;

    public:
        // Works like the DbgHelp StackWalk64 routine. 
        WindowsStackWalker(
            uint32_t machineType,
            uint64_t pc,
            uint64_t stack,
            uint64_t frame,
            void* processContext,
            ReadProcessMemory64Proc readMemProc,
            FunctionTableAccess64Proc funcTabProc,
            GetModuleBase64Proc getModBaseProc );
        HRESULT Init( const void* threadContext, uint32_t threadContextSize );

        virtual bool WalkStack();

        virtual void GetThreadContext( const void*& context, uint32_t& contextSize );
    };
}
