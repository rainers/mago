/*
   Copyright (c) 2010 Aldo J. Nunez

   Licensed under the Apache License, Version 2.0.
   See the LICENSE text file for details.
*/

#pragma once


template <class T>
class Enumerator
{
public:
    virtual ~Enumerator() { }

    virtual void    Release() = 0;

    virtual int     GetCount() = 0;
    virtual T       GetCurrent() = 0;
    virtual bool    MoveNext() = 0;
    virtual void    Reset() = 0;
};
