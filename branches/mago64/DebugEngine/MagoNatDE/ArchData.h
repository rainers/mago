/*
   Copyright (c) 2013 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

namespace Mago
{
    class IRegisterSet;

    struct Reg
    {
        const wchar_t*  Name;
        uint8_t         FullReg;
        uint8_t         Length;
        uint8_t         Shift;
        uint32_t        Mask;
    };

    struct RegGroup
    {
        uint32_t    StrId;
        const Reg*  Regs;
        uint32_t    RegCount;
    };

    struct RegGroupInternal
    {
        uint32_t    StrId;
        const Reg*  Regs;
        uint32_t    RegCount;
        uint32_t    NeededFeature;
    };


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

        virtual void GetThreadContext( const void*& context, uint32_t& contextSize ) = 0;
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
            IRegisterSet* topRegSet,
            void* processContext,
            ReadProcessMemory64Proc readMemProc,
            FunctionTableAccess64Proc funcTabProc,
            GetModuleBase64Proc getModBaseProc,
            StackWalker*& stackWalker ) = 0;

        virtual HRESULT BuildRegisterSet( 
            const void* threadContext,
            uint32_t threadContextSize,
            IRegisterSet*& regSet ) = 0;
        virtual HRESULT BuildTinyRegisterSet( 
            const void* threadContext,
            uint32_t threadContextSize,
            IRegisterSet*& regSet ) = 0;

        virtual uint32_t GetRegisterGroupCount() = 0;
        virtual bool GetRegisterGroup( uint32_t index, RegGroup& group ) = 0;

        // Maps a debug info register ID to an ID specific to this ArchData.
        // Returns the mapped register ID, and -1 if no mapping is found.
        virtual int GetArchRegId( int debugRegId ) = 0;

        static HRESULT MakeArchData( UINT32 procType, UINT64 procFeatures, ArchData*& archData );
    };
}
