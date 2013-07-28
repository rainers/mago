/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

namespace Mago
{
    class IRegisterSet;
    struct RegGroup;

    // Callbacks used by stack walkers to get data they need.
    // They're meant to work as with the DbgHelp StackWalk64 routine.

    typedef BOOL (CALLBACK *ReadProcessMemory64Proc)(
      void* processContext,
      DWORD64 baseAddress,
      void* buffer,
      DWORD size,
      DWORD* numberOfBytesRead );

    typedef void* (CALLBACK *FunctionTableAccess64Proc)(
      void* processContext,
      DWORD64 addrBase );

    typedef DWORD64 (CALLBACK *GetModuleBase64Proc)(
      void* processContext,
      DWORD64 address );


    class StackWalker
    {
    public:
        virtual ~StackWalker() { }

        // returns true if a new frame is available, 
        //         false at the end of the callstack
        virtual bool WalkStack() = 0;

        virtual const void* GetThreadContext() = 0;
    };


    class ArchData
    {
        int mRefCount;

    public:
        ArchData();
        virtual ~ArchData() { }

        void AddRef();
        void Release();

        virtual HRESULT BeginWalkStack( 
            const void* threadContext, 
            uint32_t threadContextSize,
            void* processContext,
            ReadProcessMemory64Proc readMemProc,
            FunctionTableAccess64Proc funcTabProc,
            GetModuleBase64Proc getModBaseProc,
            StackWalker*& stackWalker ) = 0;

        virtual Address GetPC( const void* threadContext ) = 0;
        virtual HRESULT BuildRegisterSet( 
            const void* threadContext,
            ::Thread* coreThread, 
            IRegisterSet*& regSet ) = 0;
        virtual HRESULT BuildTinyRegisterSet( 
            const void* threadContext,
            ::Thread* coreThread, 
            IRegisterSet*& regSet ) = 0;

        virtual void GetRegisterGroups( const RegGroup*& groups, uint32_t& count ) = 0;

        // Maps a debug info register ID to an ID specific to this ArchData.
        // Returns the mapped register ID, and -1 if no mapping is found.
        virtual int GetArchRegId( int debugRegId ) = 0;
    };
}
