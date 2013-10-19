/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


class IModule
{
public:
    virtual ~IModule() { }

    virtual void            AddRef() = 0;
    virtual void            Release() = 0;

    virtual Address         GetImageBase() = 0;
    virtual uint32_t        GetDebugInfoFileOffset() = 0;
    virtual uint32_t        GetDebugInfoSize() = 0;
    virtual uint32_t        GetSize() = 0;
    virtual uint16_t        GetMachine() = 0;
    virtual const wchar_t*  GetExePath() = 0;
    virtual Address         GetPreferredImageBase() = 0;

    virtual bool            IsDeleted() = 0;
};
