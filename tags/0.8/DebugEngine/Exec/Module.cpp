/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#include "Common.h"
#include "Module.h"


Module::Module( 
               Address imageBase, 
               uint32_t size, 
               uint16_t machine, 
               const wchar_t* exePath, 
               uint32_t debugInfoFileOffset, 
               uint32_t debugInfoSize )
:   mRefCount( 0 ),
    mImageBase( imageBase ),
    mPrefImageBase( 0 ),
    mSize( size ),
    mMachine( machine ),
    mExePath( exePath ),
    mDebugInfoFileOffset( debugInfoFileOffset ),
    mDebugInfoSize( debugInfoSize ),
    mDeleted( false )
{
    _ASSERT( size > 0 );
}

Module::~Module()
{
}

void    Module::AddRef()
{
    InterlockedIncrement( &mRefCount );
}

void    Module::Release()
{
    LONG newRefCount = InterlockedDecrement( &mRefCount );
    _ASSERT( newRefCount >= 0 );
    if ( newRefCount == 0 )
    {
        delete this;
    }
}


Address         Module::GetImageBase()
{
    return mImageBase;
}

uint32_t        Module::GetDebugInfoFileOffset()
{
    return mDebugInfoFileOffset;
}

uint32_t        Module::GetDebugInfoSize()
{
    return mDebugInfoSize;
}

uint32_t        Module::GetSize()
{
    return mSize;
}

uint16_t        Module::GetMachine()
{
    return mMachine;
}

const wchar_t*  Module::GetExePath()
{
    return mExePath.c_str();
}

Address         Module::GetPreferredImageBase()
{
    return mPrefImageBase;
}

void            Module::SetPreferredImageBase( Address address )
{
    mPrefImageBase = address;
}

bool Module::IsDeleted()
{
    return mDeleted;
}

void Module::SetDeleted()
{
    mDeleted = true;
}
