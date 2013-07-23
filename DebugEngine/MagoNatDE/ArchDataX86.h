/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "ArchData.h"


namespace Mago
{
    class ArchDataX86 : public ArchData
    {
    public:
        ArchDataX86();

        virtual HRESULT BeginWalkStack( 
            const void* threadContext, 
            uint32_t threadContextSize,
            void* processContext,
            ReadProcessMemory64Proc readMemProc,
            FunctionTableAccess64Proc funcTabProc,
            GetModuleBase64Proc getModBaseProc,
            StackWalker*& stackWalker );

        virtual Address GetPC( const void* threadContext );
        virtual HRESULT BuildRegisterSet( 
            const void* threadContext,
            ::Thread* coreThread, 
            IRegisterSet*& regSet );
        virtual HRESULT BuildTinyRegisterSet( 
            const void* threadContext,
            ::Thread* coreThread, 
            IRegisterSet*& regSet );
    };
}
