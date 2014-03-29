/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


enum CreateMethod
{
    Create_Launch,
    Create_Attach,
};

class Thread;
template <class T>
class Enumerator;


// Methods of the IProcess class are safe to call from any thread.
// They are accurate when run from the debug thread, or when the process is stopped.

// See the WinNT.h header file for processor-specific definitions of 
// machine type. For x86, the machine type is IMAGE_FILE_MACHINE_I386.

class IProcess
{
public:
    virtual ~IProcess() { }

    virtual void            AddRef() = 0;
    virtual void            Release() = 0;

    virtual CreateMethod    GetCreateMethod() = 0;
    virtual HANDLE          GetHandle() = 0;
    virtual uint32_t        GetId() = 0;
    virtual const wchar_t*  GetExePath() = 0;
    virtual Address         GetEntryPoint() = 0;
    virtual uint16_t        GetMachineType() = 0;
    virtual Address         GetImageBase() = 0;
    virtual uint32_t        GetImageSize() = 0;

    virtual bool            IsStopped() = 0;
    virtual bool            IsDeleted() = 0;
    virtual bool            IsTerminating() = 0;
    virtual bool            ReachedLoaderBp() = 0;

    // threads

    virtual bool            FindThread( uint32_t id, Thread*& thread ) = 0;
    virtual HRESULT         EnumThreads( Enumerator< Thread* >*& en ) = 0;
};
