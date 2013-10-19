/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once

#include "IModule.h"


class Module : public IModule
{
    LONG            mRefCount;

    Address         mImageBase;
    Address         mPrefImageBase;
    uint32_t        mDebugInfoFileOffset;
    uint32_t        mDebugInfoSize;
    uint32_t        mSize;
    uint16_t        mMachine;
    std::wstring    mExePath;

    bool            mDeleted;

public:
    Module( 
        Address imageBase, 
        uint32_t size, 
        uint16_t machine, 
        const wchar_t* exePath, 
        uint32_t debugInfoFileOffset, 
        uint32_t debugInfoSize );
    ~Module();

    void            AddRef();
    void            Release();

    Address         GetImageBase();
    uint32_t        GetDebugInfoFileOffset();
    uint32_t        GetDebugInfoSize();
    uint32_t        GetSize();
    uint16_t        GetMachine();
    const wchar_t*  GetExePath();
    Address         GetPreferredImageBase();
    void            SetPreferredImageBase( Address address );

    bool            IsDeleted();

    // internal
    void            SetDeleted();
    bool            Contains( Address addr );
};
