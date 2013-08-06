/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "WinStackWalker.h"
#include <DbgHelp.h>


namespace Mago
{
    WindowsStackWalker::WindowsStackWalker(
        uint32_t machineType,
        uint64_t pc,
        uint64_t stack,
        uint64_t frame,
        void* processContext,
        ReadProcessMemory64Proc readMemProc,
        FunctionTableAccess64Proc funcTabProc,
        GetModuleBase64Proc getModBaseProc )
        :   mMachineType( machineType ),
            mThreadContextSize( 0 )
    {
        mProcessContext = processContext;
        mReadMemProc = readMemProc;
        mFuncTabProc = funcTabProc;
        mGetModBaseProc = getModBaseProc;

        memset( &mGenericFrame, 0, sizeof mGenericFrame );

        mGenericFrame.AddrPC.Mode = AddrModeFlat;
        mGenericFrame.AddrPC.Offset = pc;
        mGenericFrame.AddrStack.Mode = AddrModeFlat;
        mGenericFrame.AddrStack.Offset = stack;
        mGenericFrame.AddrFrame.Mode = AddrModeFlat;
        mGenericFrame.AddrFrame.Offset = frame;
    }

    HRESULT WindowsStackWalker::Init( const void* threadContext, uint32_t threadContextSize )
    {
        if ( threadContext == NULL )
            return E_INVALIDARG;

        mThreadContext.reset( new BYTE[threadContextSize] );
        if ( mThreadContext.get() == NULL )
            return E_OUTOFMEMORY;

        mThreadContextSize = threadContextSize;

        memcpy( mThreadContext.get(), threadContext, threadContextSize );
        return S_OK;
    }

    bool WindowsStackWalker::WalkStack()
    {
        if ( mThreadContext.get() == NULL )
            return false;

        return StackWalk64( 
            mMachineType,
            mProcessContext,
            NULL,
            &mGenericFrame,
            mThreadContext.get(),
            mReadMemProc,
            mFuncTabProc,
            mGetModBaseProc,
            NULL ) ? true : false;
    }

    void WindowsStackWalker::GetThreadContext( const void*& context, uint32_t& contextSize )
    {
        context = mThreadContext.get();
        contextSize = mThreadContextSize;
    }
}
